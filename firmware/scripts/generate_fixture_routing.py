# EnvNode: see repository LICENSE.
"""Generate fixture revision 0.1 routing from the three saved KiCad PCBs."""
import argparse
import hashlib
import json
from pathlib import Path
from generate_testboard_profile import parse, children, child, pads

ROOT = Path(__file__).resolve().parents[2]
CHANNEL_PADS = [9, 8, 7, 6, 5, 4, 3, 2, 23, 22, 21, 20, 19, 18, 17, 16]
GPIO_BY_PAD = {5:34,7:32,8:33,9:25,10:26,11:27,12:14,15:13,
               26:4,27:16,28:17,29:5,30:18,31:19,33:21,36:22,37:23}

def generate():
    boards, sources = {}, {}
    for name in ('Input-Adapter', 'Test-Controller', 'Module-Adapter'):
        path = Path('hardware/kicad/MainBoards') / name / (name + '.kicad_pcb')
        data = (ROOT / path).read_bytes()
        board = parse(data.decode())
        assert child(child(board, 'title_block'), 'rev') == ['rev', '0.1']
        boards[name] = {next(x[2] for x in children(f, 'property') if x[1] == 'Reference'):
                       {p['number']:p['net'] for p in pads(f)} for f in children(board, 'footprint')}
        sources[str(path)] = hashlib.sha256(data).hexdigest()
    inp, out, module = (boards[n] for n in ('Input-Adapter','Test-Controller','Module-Adapter'))
    assert [inp['J1'][i] for i in range(1,11) if i != 9] == [out['J8'][i] for i in range(1,11) if i != 9]
    assert inp['J1'][9] == '/COM' and out['J8'][9] == '/COM_IN'
    assert inp['TP1'][1] == '/IN/MB_GND' and inp['TP1'][2] == '/IN/MB_3V3_SYS'
    assert module['J1'] == module['J4'], 'Module adapter must remain pin-for-pin'
    def mux_address(footprints, refs, net):
        hits = [(i,ch) for i,ref in enumerate(refs) for ch,pad in enumerate(CHANNEL_PADS)
                if footprints[ref][pad] == net]
        assert len(hits) == 1, (net,hits)
        return hits[0]
    inputs = [mux_address(inp,('U4','U5','U6'),inp['J4'][i]) for i in range(1,39)]
    inputs += [mux_address(inp,('U4','U5','U6'),inp['TP1'][i]) for i in (2,1)]
    assert len(set(inputs)) == 40
    outputs = {key:[mux_address(out,('U1','U2'),out[ref][i]) for i in range(1,count+1)]
               for key,ref,count in [('gpio','J1',4),('i2c','J2',4),('module','J3',14)]}
    assert len(set(sum(outputs.values(),[]))) == 22
    def gpio(net):
        hits = [GPIO_BY_PAD[pad] for pad,n in out['U3'].items() if n == net and pad in GPIO_BY_PAD]
        assert len(hits) == 1, (net,hits)
        return hits[0]
    controls = dict(inputSelect=[gpio('/IN_S'+str(i)) for i in range(4)],
        inputEnable=[gpio('/IN_EN'+str(i)) for i in range(1,4)],
        outputSelect=[gpio('/OUT_S'+str(i)) for i in range(4)],
        outputEnable=[gpio('/OUT_EN'+str(i)) for i in range(1,3)],
        sense=gpio('/COM_IN'), drive=gpio('/COM_OUT'), sda=gpio('/SDA'), scl=gpio('/SCL'))
    for ref in ('U4','U5','U6'):
        assert inp[ref][1] == '/COM'
        assert [inp[ref][i] for i in (10,11,14,13)] == ['/IN_S'+str(i) for i in range(4)]
    for ref in ('U1','U2'):
        assert [out[ref][i] for i in (10,11,14,13)] == ['/OUT_S'+str(i) for i in range(4)]
    for i,ref in enumerate(('U4','U5','U6'),1): assert inp[ref][15] == '/IN_EN'+str(i)
    for i,ref in enumerate(('U1','U2'),1): assert out[ref][15] == '/OUT_EN'+str(i)
    assert set(out['R6'].values()) == {'/COM_OUT',out['U1'][1]}
    assert out['U1'][1] == out['U2'][1]
    def pairs(values): return '{' + ', '.join('{%d, %d}' % pair for pair in values) + '}'
    def numbers(values): return '{' + ', '.join(map(str,values)) + '}'
    header = '// EnvNode: see repository LICENSE. Generated; do not edit.\n'
    header += 'static const FixtureDefinition FixtureRevision01 = {\n    "controller0.1-input0.1-module0.1",\n'
    header += '    {' + ',\n     '.join(pairs(v) for v in [inputs,outputs['gpio'],outputs['i2c'],outputs['module']]) + '},\n'
    header += '    ' + ', '.join(numbers(controls[k]) for k in ('inputSelect','inputEnable','outputSelect','outputEnable')) + ',\n'
    header += '    %d, %d, %d, %d\n};\n' % tuple(controls[k] for k in ('sense','drive','sda','scl'))
    definition = dict(schema=1,id='controller0.1-input0.1-module0.1',sources=sources,
        channelNumbering='zero-based; device indexes input U4/U5/U6, output U1/U2',
        inputs=inputs,outputs=outputs,gpios=controls,
        referenceConnector=dict(reference='TP1',pins={'1':'MB_GND','2':'MB_3V3_SYS'}))
    return {'firmware/src/testboard/testing/FixtureRevision01.inc':header,
            'firmware/testboard/fixture-rev0.1.json':json.dumps(definition,indent=2)+'\n'}

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    for name,content in generate().items():
        path=ROOT/name
        if args.check:
            assert path.exists() and path.read_text()==content, 'Stale fixture routing: '+name
        else: path.write_text(content)
        print(('Checked ' if args.check else 'Generated ')+name)
