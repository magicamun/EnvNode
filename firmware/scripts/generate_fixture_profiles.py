# EnvNode: see repository LICENSE.
"""Generate the new ESP32 adapter map and verify Controller 0.2 compatibility."""
import argparse,hashlib,json
from pathlib import Path
from generate_testboard_profile import parse,children,child,pads
from generate_fixture_routing import CHANNEL_PADS,generate,GPIO_BY_PAD
ROOT=Path(__file__).resolve().parents[2]
def footprints(path):
    data=(ROOT/path).read_bytes();board=parse(data.decode())
    return data,board,{next(x[2] for x in children(f,'property') if x[1]=='Reference'):{p['number']:p['net'] for p in pads(f)} for f in children(board,'footprint')}
def generate_profiles():
    path=Path('hardware/kicad/Adapter/Adapter ESP32 DevKitC/Adapter ESP32 DevKitC.kicad_pcb')
    data,board,f=footprints(path)
    assert child(child(board,'title_block'),'rev')==['rev','0.3']
    assert len(f['J2'])==38
    def address(net):
        hits=[(i,c) for i,ref in enumerate(('U4','U5','U6')) for c,p in enumerate(CHANNEL_PADS) if f[ref][p]==net]
        assert len(hits)==1,(net,hits)
        return hits[0]
    inputs=[address(f['J2'][i]) for i in range(1,39)]+[address(f['TP1'][i]) for i in (2,1)]
    assert len(set(inputs))==40
    for i,ref in enumerate(('U4','U5','U6'),1):
        assert f[ref][1]=='/COM' and f[ref][12]=='GND' and f[ref][24]=='3V3_SYS'
        assert [f[ref][p] for p in (10,11,14,13)]==['/IN_S'+str(n) for n in range(4)]
        assert f[ref][15]=='/IN_EN'+str(i)
    assert f['J1']=={1:'3V3_SYS',2:'GND',3:'/IN_EN1',4:'/IN_S3',5:'/IN_EN2',6:'/IN_S2',7:'/IN_EN3',8:'/IN_S1',9:'/COM',10:'/IN_S0'}
    adapter=dict(license='EnvNode-owned material; see repository LICENSE',schema=1,id='esp32-devkitc-0.3',revision='0.3',source=str(path),sha256=hashlib.sha256(data).hexdigest(),socketReference='J2',inputs=inputs,referenceInputs={'39':'TP1/2 MB_3V3_SYS','40':'TP1/1 MB_GND'})
    header='// EnvNode: see repository LICENSE. Generated from saved PCB; see fixture-esp32-devkitc0.3.json.\nstatic const MuxAddress Esp32Adapter03Inputs[MaxInputs] = {'+', '.join('{%d,%d}'%x for x in inputs)+'};\n'
    path=Path('hardware/kicad/MainBoards/Test-Controller/Test-Controller.kicad_pcb')
    data,board,f=footprints(path);assert child(child(board,'title_block'),'rev')==['rev','0.2']
    legacy=json.loads(generate()['firmware/testboard/fixture-rev0.1.json'])
    def gpio(net):
        hits=[GPIO_BY_PAD[p] for p,n in f['U3'].items() if n==net and p in GPIO_BY_PAD]
        assert len(hits)==1;return hits[0]
    g=legacy['gpios']
    for key,prefix,count in [('inputSelect','/IN_S',4),('inputEnable','/IN_EN',3),('outputSelect','/OUT_S',4),('outputEnable','/OUT_EN',2)]:
        assert [gpio(prefix+str(i+(1 if 'Enable' in key else 0))) for i in range(count)]==g[key]
    assert [gpio(n) for n in ('/COM_IN','/COM_OUT','/SDA','/SCL')]==[g[n] for n in ('sense','drive','sda','scl')]
    for key,ref,count in [('gpio','GPIO1',4),('i2c','I2C1',4),('module','J3',14)]:
        for pin in range(1,count+1):
            d,c=legacy['outputs'][key][pin-1];assert f['U'+str(d+1)][CHANNEL_PADS[c]]==f[ref][pin]
    assert f['GPIO1']==f['GPIO2']
    assert all(f['I2C1'][p]==f['I2C2'][p] for p in range(1,5))
    assert f['RST1']==f['RST2']
    reset=[]
    for pin in (1,2):
        hits=[(d,c) for d,ref in enumerate(('U1','U2')) for c,pad in enumerate(CHANNEL_PADS) if f[ref][pad]==f['RST1'][pin]]
        assert len(hits)==1,(pin,hits)
        reset.append(hits[0])
    assert reset==[(0,0),(0,15)]
    reset_header='// EnvNode: see repository LICENSE. Generated from Controller 0.2 PCB.\nstatic const FixtureDefinition FixtureRevision02 = [] { auto f = FixtureRevision01; f.routing.reset[0] = {%d,%d}; f.routing.reset[1] = {%d,%d}; return f; }();\n'%(*reset[0],*reset[1])
    assert f['TP2']=={1:'GND',2:'/TEST_DRIVE',3:'/COM_IN'}
    assert set(f['R6'].values())=={'/COM_OUT','/TEST_DRIVE'}
    assert f['U1'][1]==f['U2'][1]=='/TEST_DRIVE'
    controller=dict(license='EnvNode-owned material; see repository LICENSE',schema=1,id='controller-0.2',revision='0.2',source=str(path),sha256=hashlib.sha256(data).hexdigest(),gpios=g,outputs=dict(legacy['outputs'],reset=reset),testConnector={'reference':'TP2','pins':{'1':'GND','2':'TEST_DRIVE after R6','3':'COM_IN'}})
    return {'firmware/src/testboard/testing/Controller02.inc':reset_header,'firmware/src/testboard/testing/Esp32Adapter03.inc':header,'firmware/testboard/fixture-esp32-devkitc0.3.json':json.dumps(adapter,indent=2)+'\n','firmware/testboard/fixture-controller0.2.json':json.dumps(controller,indent=2)+'\n'}
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    for name,content in generate_profiles().items():
        p=ROOT/name
        if args.check:assert p.read_text()==content,'Stale hardware profile: '+name
        else:p.write_text(content)
        print(('Checked ' if args.check else 'Generated ')+name)
