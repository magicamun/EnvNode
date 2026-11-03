# EnvNode: see repository LICENSE.
"""Generate runtime/identity tables and downloadable records from hardware-profiles."""
import argparse
import hashlib
import json
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = '// EnvNode: see repository LICENSE. Generated; do not edit.\n'
ORDER = [('EnvNodeMainboard','0.3'),('EnvNodeWeather','0.2'),('EnvNodeMini','0.8'),('EnvNodeWeather','0.3'),('EnvNodeMini','0.9')]

def records():
    values = [json.loads(p.read_text()) for p in sorted((ROOT/'firmware/hardware-profiles').glob('*.json'))]
    values.sort(key=lambda r: ORDER.index((r['boardProfile'],r['identityRevision'])) if (r['boardProfile'],r['identityRevision']) in ORDER else len(ORDER))
    ids, identities = set(), set()
    for r in values:
        identity = (r['boardProfileId'],r['identityRevision'])
        if type(r['boardProfileId']) is not int or not 0<=r['boardProfileId']<=254: raise ValueError('Invalid board family ID')
        if r['schemaVersion'] != 1 or r['id'] in ids or identity in identities:
            raise ValueError('Invalid/duplicate hardware profile')
        ids.add(r['id']); identities.add(identity)
        version = r['minimumRuntimeFirmwareVersion'].split('.')
        if len(version)!=3 or any(not x.isdigit() or int(x)>65535 for x in version):
            raise ValueError('Invalid runtime version')
        p = r['provisioning']
        if p['identityBus']!='I2C0' or p['identityAddress']!=0x50: raise ValueError('Invalid identity bus')
        gpios = [g['gpio'] for g in p['gpios']]
        if len(gpios)!=len(set(gpios)) or any(g<0 or g>254 for g in gpios): raise ValueError('Invalid GPIO map')
        for slot in p['moduleSlots']:
            if any(not 0 <= slot[k] <= 254 for k in ('aux1','aux2','cs')): raise ValueError('Invalid slot GPIO')
        test = r['testing']
        if test is not None:
            if test['id']!=r['id'] or test['name']!=r['name'] or test['identityRevision']!=r['identityRevision']:
                raise ValueError('Test and provisioning identities differ')
        image = r['image']
        if image:
            origin = 'https://docs.envnode.de/assets/'
            if not image.startswith(origin): raise ValueError('Invalid image origin')
            relative = Path(image[len(origin):])
            if '..' in relative.parts or relative.is_absolute(): raise ValueError('Invalid image path')
            if not (ROOT/'docs/site/assets'/relative).is_file(): raise ValueError('Missing documentation image')
    return values

def port_map_bytes(r):
    spec = importlib.util.spec_from_file_location('test_cbor', ROOT/'firmware/scripts/generate_testboard_profile.py')
    helper = importlib.util.module_from_spec(spec); spec.loader.exec_module(helper)
    # The legacy test encoder distinguishes bools as integers; encode these explicitly.
    def cbor(v):
        if isinstance(v,bool): return b'\xf5' if v else b'\xf4'
        if isinstance(v,list):
            n=len(v)
            head=bytes([0x80+n]) if n<24 else bytes([0x98,n])
            return head+b''.join(cbor(x) for x in v)
        return helper.cbor(v)
    p=r['provisioning'];capabilities={'DigitalInput':1,'DigitalOutput':2,'Interrupt':4,'InternalPullup':8,'AnalogInput':16}
    return cbor([1,r['boardProfileId'],[int(x) for x in r['identityRevision'].split('.')],r['name'],
        [int(x) for x in r['minimumRuntimeFirmwareVersion'].split('.')],'',
        [[g['gpio'],sum(capabilities[x.split('::')[1]] for x in g['capabilities'])] for g in p['gpios']],
        [[int(b['bus'][-1]),b['sda'],b['scl']] for b in p['i2cBuses']],
        [[0 if x['slot']=='A' else 1,x['identityAddress'],x['aux1'],x['aux2'],x['cs'],x['fiveVolt']] for x in p['moduleSlots']],
        [p['spiBus'][k] for k in ('clock','miso','mosi')]])

def generate():
    runtime, identity, maps = HEADER, HEADER, HEADER
    outputs, catalog, provisioning_catalog = {}, [], []
    resource_symbols = {}
    for i,r in enumerate(records()):
        symbol = f'Hardware{i}'
        p=r['provisioning']
        resource_start=len(runtime)
        runtime+=f'const BoardGpioCapability {symbol}Gpios[] = {{\n'
        for g in p['gpios']:
            runtime+='    {GpioResource(%d), %s, %s},\n' % (g['gpio'],json.dumps(g['name']),' | '.join(g['capabilities']))
        runtime+='};\n'+f'const BoardI2CBusCapability {symbol}Buses[] = {{\n'
        for b in p['i2cBuses']:
            runtime+='    {I2CBus::%s, GpioResource(%d), GpioResource(%d)},\n'%(b['bus'],b['sda'],b['scl'])
        runtime+='};\n'+f'const BoardModuleSlotCapability {symbol}Slots[] = {{\n'
        for slot in p['moduleSlots']:
            runtime+='    {ModuleSlot::%s, %d, GpioResource(%d), GpioResource(%d), GpioResource(%d), %s},\n'%(slot['slot'],slot['identityAddress'],slot['aux1'],slot['aux2'],slot['cs'],str(slot['fiveVolt']).lower())
        version='{'+r['minimumRuntimeFirmwareVersion'].replace('.',',')+'}'
        rev='{'+r['identityRevision'].replace('.',',')+'}'
        image=json.dumps(r['image']) if r['image'] else 'nullptr'
        runtime+='};\n'
        resource_key=json.dumps(p,sort_keys=True)
        if resource_key in resource_symbols:
            runtime=runtime[:resource_start]
        else:
            resource_symbols[resource_key]=symbol
        resource_symbol=resource_symbols[resource_key]
        runtime+=f'const BoardProfile {symbol}Profile = {{static_cast<BoardProfileId>({r["boardProfileId"]}), {json.dumps(r["name"])}, {rev}, {resource_symbol}Gpios, {len(p["gpios"])}, {resource_symbol}Buses, {len(p["i2cBuses"])}, {resource_symbol}Slots, {len(p["moduleSlots"])}, GpioResource({p["spiBus"]["clock"]}), GpioResource({p["spiBus"]["miso"]}), GpioResource({p["spiBus"]["mosi"]})}};\n'
        data=port_map_bytes(r)
        maps+=f'const uint8_t {symbol}PortMap[] = {{'+','.join(str(b) for b in data)+'};\n'
        map_hash=hashlib.sha256(data).hexdigest()
        map_name=r['id']+'.'+map_hash+'.portmap.cbor'
        outputs['docs/site/assets/hardware/v1/'+map_name]=data
        identity+=f'    {{static_cast<BoardProfileId>({r["boardProfileId"]}),{json.dumps(r["name"])},{rev},{version},{image},{symbol}PortMap,sizeof({symbol}PortMap)}} ,\n'
        public={k:v for k,v in r.items() if k!='testGeneration'}
        content=json.dumps(public,ensure_ascii=False,sort_keys=True,separators=(',',':'))+'\n'
        digest=hashlib.sha256(content.encode()).hexdigest()
        name=r['id']+'.'+digest+'.json'
        outputs['docs/site/assets/hardware/v1/'+name]=content
        catalog.append({'id':r['id'],'filename':name,'sha256':digest,'portMap':map_name,'portMapSha256':map_hash,'portMapBytes':len(data)})
        provisioning_catalog.append([r['id'],r['name'],0,[int(x) for x in r['identityRevision'].split('.')],
            [int(x) for x in r['minimumRuntimeFirmwareVersion'].split('.')],
            r['image'].removeprefix('https://docs.envnode.de/') if r['image'] else '',map_name,len(data),map_hash])
    spec = importlib.util.spec_from_file_location('descriptor_cbor', ROOT/'tools/measure_descriptor_encoding.py')
    encoder = importlib.util.module_from_spec(spec); spec.loader.exec_module(encoder)
    encoder.validate_encoder()
    for path in sorted((ROOT/'firmware/hardware-profiles/modules').glob('*.json')):
        module=json.loads(path.read_text());d=module['provisioning']
        if module['schemaVersion']!=1 or d['objectKind']!='module' or module['name']!=d['description']['name']:
            raise ValueError('Invalid module profile')
        if any(x[0]==module['id'] for x in provisioning_catalog): raise ValueError('Duplicate profile ID')
        data=encoder.encode_cbor(encoder.compact_value(None,d))
        if len(data)>2016 or d['identity']['instanceId']!='00000000-0000-0000-0000-000000000000': raise ValueError('Invalid module template')
        digest=hashlib.sha256(data).hexdigest();filename=module['id']+'.'+digest+'.module.cbor'
        outputs['docs/site/assets/hardware/v1/'+filename]=data
        if module['id']=='duo-relay-0.3':
            outputs['firmware/lib/EnvNodeIdentity/src/DuoRelayProfile.generated.inc']=HEADER+'const uint8_t DuoRelayProfileTemplate[] = {'+','.join(str(b) for b in data)+'};\n'
        provisioning_catalog.append([module['id'],module['name'],1,[d['identity']['hardwareRevision'][k] for k in ('major','minor')],
            [int(x) for x in d['compatibility']['minimumFirmwareVersion'].split('.')],
            module['image'].removeprefix('https://docs.envnode.de/') if module['image'] else '',filename,len(data),digest])
    outputs['docs/site/assets/hardware/v1/provisioning-catalog.cbor']=encoder.encode_cbor([1,provisioning_catalog])
    runtime+='const BoardProfile* const BoardProfiles[] = {&Hardware0Profile,&Hardware1Profile,&Hardware2Profile};\n'
    runtime+='const BoardProfile* const RevisionProfiles[] = {'+','.join('&Hardware%dProfile'%i for i in range(len(catalog)))+'};\n'
    outputs['firmware/src/HardwareProfiles.generated.inc']=runtime
    outputs['firmware/lib/EnvNodeIdentity/src/HardwareIdentityProfiles.generated.inc']=identity
    outputs['firmware/lib/EnvNodeIdentity/src/HardwarePortMaps.generated.inc']=maps
    outputs['docs/site/assets/hardware/v1/LICENSE.txt']=(ROOT/'LICENSE').read_text()
    outputs['docs/site/assets/hardware/v1/catalog.json']=json.dumps({'schemaVersion':1,'profiles':catalog},indent=2)+'\n'
    spec = importlib.util.spec_from_file_location('test_profiles', ROOT/'firmware/scripts/generate_testboard_profile.py')
    test_profiles = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(test_profiles)
    outputs.update(test_profiles.generate())
    spec = importlib.util.spec_from_file_location('browser_profiles', ROOT/'firmware/scripts/generate_browser_profile_assets.py')
    browser_profiles = importlib.util.module_from_spec(spec);spec.loader.exec_module(browser_profiles)
    outputs.update({str(path.relative_to(ROOT)): content for path,content in browser_profiles.generate().items()})
    return outputs

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    for name,content in generate().items():
        path=ROOT/name
        if args.check:
            if not path.exists() or (path.read_bytes() if isinstance(content,bytes) else path.read_text())!=content: raise SystemExit('Out of date: '+name)
        else:
            path.parent.mkdir(parents=True,exist_ok=True)
            if isinstance(content,bytes): path.write_bytes(content)
            else: path.write_text(content)
    print('Hardware profiles checked' if args.check else 'Hardware profiles generated')
