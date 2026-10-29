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
        if r['schemaVersion'] != 1 or r['id'] in ids or identity in identities:
            raise ValueError('Invalid/duplicate hardware profile')
        ids.add(r['id']); identities.add(identity)
        version = r['minimumRuntimeFirmwareVersion'].split('.')
        if len(version)!=3 or any(not x.isdigit() or int(x)>65535 for x in version):
            raise ValueError('Invalid runtime version')
        p = r['provisioning']
        if p['identityBus']!='I2C0' or p['identityAddress']!=0x50: raise ValueError('Invalid identity bus')
        gpios = [g['gpio'] for g in p['gpios']]
        if len(gpios)!=len(set(gpios)) or any(g<0 or g>39 for g in gpios): raise ValueError('Invalid GPIO map')
        for slot in p['moduleSlots']:
            if any(not 0 <= slot[k] <= 39 for k in ('aux1','aux2','cs')): raise ValueError('Invalid slot GPIO')
        test = r['testing']
        if test is not None:
            if test['id']!=r['id'] or test['name']!=r['name'] or test['identityRevision']!=r['identityRevision']:
                raise ValueError('Test and provisioning identities differ')
            image = r['image']
            if not image or not image.startswith('https://docs.envnode.de/assets/testboard/v1/'):
                raise ValueError('Invalid image origin')
            image_path = ROOT/'docs/site/assets/testboard/v1'/image.rsplit('/',1)[1]
            if hashlib.sha256(image_path.read_bytes()).hexdigest()+'.svg' != image_path.name:
                raise ValueError('Image hash mismatch')
    return values

def generate():
    runtime, identity = HEADER, HEADER
    outputs, catalog = {}, []
    for i,r in enumerate(records()):
        symbol = f'Hardware{i}'
        p=r['provisioning']
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
        runtime+='};\n'+f'const BoardProfile {symbol}Profile = {{BoardProfileId::{r["boardProfile"]}, {json.dumps(r["name"])}, {rev}, {symbol}Gpios, {len(p["gpios"])}, {symbol}Buses, {len(p["i2cBuses"])}, {symbol}Slots, {len(p["moduleSlots"])}, GpioResource({p["spiBus"]["clock"]}), GpioResource({p["spiBus"]["miso"]}), GpioResource({p["spiBus"]["mosi"]})}};\n'
        identity+=f'    {{BoardProfileId::{r["boardProfile"]},{json.dumps(r["name"])},{rev},{version},{image}}},\n'
        public={k:v for k,v in r.items() if k!='testGeneration'}
        content=json.dumps(public,ensure_ascii=False,sort_keys=True,separators=(',',':'))+'\n'
        digest=hashlib.sha256(content.encode()).hexdigest()
        name=r['id']+'.'+digest+'.json'
        outputs['docs/site/assets/hardware/v1/'+name]=content
        catalog.append({'id':r['id'],'filename':name,'sha256':digest})
    runtime+='const BoardProfile* const BoardProfiles[] = {&Hardware0Profile,&Hardware1Profile,&Hardware2Profile};\n'
    runtime+='const BoardProfile* const RevisionProfiles[] = {'+','.join('&Hardware%dProfile'%i for i in range(len(catalog)))+'};\n'
    outputs['firmware/src/HardwareProfiles.generated.inc']=runtime
    outputs['firmware/lib/EnvNodeIdentity/src/HardwareIdentityProfiles.generated.inc']=identity
    outputs['docs/site/assets/hardware/v1/catalog.json']=json.dumps({'schemaVersion':1,'profiles':catalog},indent=2)+'\n'
    spec = importlib.util.spec_from_file_location('test_profiles', ROOT/'firmware/scripts/generate_testboard_profile.py')
    test_profiles = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(test_profiles)
    outputs.update(test_profiles.generate())
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
