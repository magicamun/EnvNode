# EnvNode: see repository LICENSE.
"""Derive direct-net continuity expectations and display positions from a DUT PCB.

Only explicit PCB net-tie groups join distinct net names. New reference diode paths are checked individually; other component paths are not inferred. A contact with no matching
socket net remains explicitly uncovered. KiCad sources are read-only inputs.
"""
import argparse
import gzip
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import subprocess
import struct
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROFILES = [
    dict(symbol='Weather', id='weather-0.2-pcb0.2.1', name='EnvNode Weather', identity='0.2', pcb='0.2.1',
         source='hardware/kicad/MainBoards/Weatherstation/Weatherstation.kicad_pcb', commit='ff906605036ccac222eda68e9d515e630a9cae6c',
         refs=['GPIO16', 'GPIO14', 'GPIO17', 'GPIO27', 'GPIO32', 'GPIO4', 'ADC1', 'ADC2', 'ADC3', 'ADC4']
              + [f'I2C{bus}_{i}' for bus in (0, 1) for i in range(1, 5)] + ['J1'],
         modules=['J1'], i2c=[], labels={}),
    dict(symbol='Mini', id='mini-0.8-pcb0.8.1', name='EnvNode Mini', identity='0.8', pcb='0.8.1',
         source='hardware/kicad/MainBoards/EnvNode Mini/EnvNode Mini.kicad_pcb', commit='ff906605036ccac222eda68e9d515e630a9cae6c',
         refs=['ADC1', 'ADC2', 'J4', 'J5', 'J6', 'J7', 'J1', 'J2'], modules=['J1', 'J2'],
         i2c=['J4', 'J5', 'J6', 'J7'], labels={'J4':'I2C0 · J4', 'J5':'I2C0 · J5',
             'J6':'I2C1 · J6', 'J7':'I2C1 · J7', 'J1':'Slot A · J1', 'J2':'Slot B · J2'}),
]

# New revisions are pinned to their reviewed hardware commit; never read mutable
# sibling checkouts when regenerating published profiles.
PROFILES += [
    dict(**{**PROFILES[0], 'symbol':'Weather03', 'id':'weather-0.3-pcb0.3.0',
         'identity':'0.3', 'pcb':'0.3.0', 'commit':'98704fb', 'references':True}),
    dict(**{**PROFILES[1], 'symbol':'Mini09', 'id':'mini-0.9-pcb0.9.0',
         'identity':'0.9', 'pcb':'0.9.0', 'commit':'98704fb', 'references':True,
         'refs':['ADC1','ADC2','J7','J8','J9','J10','J1','J2'],
         'i2c':['J7','J8','J9','J10'], 'labels':{'J7':'I2C0 · J7','J8':'I2C0 · J8',
         'J9':'I2C1 · J9','J10':'I2C1 · J10','J1':'Slot A · J1','J2':'Slot B · J2'}}),
]

def cbor(value):
    def head(major,n):
        if n < 24: return bytes([(major<<5)|n])
        for tag,fmt,limit in [(24,'>B',256),(25,'>H',65536),(26,'>I',2**32),(27,'>Q',2**64)]:
            if n < limit: return bytes([(major<<5)|tag])+struct.pack(fmt,n)
        raise ValueError('CBOR integer too large')
    if isinstance(value,int): return head(0,value)
    if isinstance(value,float): return b'\xfb'+struct.pack('>d',value)
    if isinstance(value,str):
        data=value.encode(); return head(3,len(data))+data
    if isinstance(value,list): return head(4,len(value))+b''.join(cbor(x) for x in value)
    raise ValueError(type(value))

def wire_profile(d,image):
    return cbor([1,d['id'],d['name'],d['identityRevision'],d['pcbRevision'],d['sha256'],d['source'],
        [float(n) for n in d['bounds']],
        [[p['number'],p['net'],float(p['x']),float(p['y'])] for p in d['socket']],
        [[c['id'],c['label'],['gpio','i2c','module'].index(c['pigtail']),
          [[p['number'],p['net'],float(p['x']),float(p['y']),sum(1<<(n-1) for n in p['expected'])] for p in c['pins']]]
         for c in d['connectors']],image,
        [[p['input'],p['reference'],p['pin'],p['diode'],p['anode'],p['cathode']] for p in d.get('referencePaths',[])],
        d.get('sourceCommit','')])

def children(node, key):
    return [x for x in node if isinstance(x, list) and x and x[0] == key]

def child(node, key):
    return next(iter(children(node, key)), [])

def parse(text):
    tokens = re.findall(r'"(?:\\.|[^"\\])*"|\(|\)|[^\s()]+', text)
    def node(i):
        assert tokens[i] == '('
        result, i = [], i + 1
        while tokens[i] != ')':
            if tokens[i] == '(':
                value, i = node(i)
                result.append(value)
            else:
                t = tokens[i]
                result.append(json.loads(t) if t.startswith('"') else t)
                i += 1
        return result, i + 1
    result, end = node(0)
    assert end == len(tokens)
    return result

def without_component_values(text):
    """Remove value fields only in the temporary rendering copy, never the PCB source."""
    pattern = r'\((?:property\s+"Value"|fp_text\s+value)(?=\s)'
    for match in reversed(list(re.finditer(pattern, text))):
        depth, quoted, escaped = 0, False, False
        for end in range(match.start(), len(text)):
            c = text[end]
            if quoted:
                if escaped: escaped = False
                elif c == '\\': escaped = True
                elif c == '"': quoted = False
            elif c == '"': quoted = True
            elif c == '(': depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    text = text[:match.start()] + text[end + 1:]
                    break
        else:
            raise ValueError('Unterminated component value field')
    return text

def pads(footprint):
    at = child(footprint, 'at')
    x, y = map(float, at[1:3])
    angle = math.radians(float(at[3]) if len(at) > 3 else 0)
    result = []
    for pad in children(footprint, 'pad'):
        if not pad[1].isdigit():
            continue
        px, py = map(float, child(pad, 'at')[1:3])
        net = child(pad, 'net')
        result.append(dict(number=int(pad[1]), net=net[-1] if net else '',
            x=round(x + px * math.cos(angle) + py * math.sin(angle), 5),
            y=round(y - px * math.sin(angle) + py * math.cos(angle), 5)))
    return sorted(result, key=lambda p: p['number'])

def generate_profile(profile):
    source = Path(profile['source'])
    symbol = profile['symbol']
    data = subprocess.check_output(['git','show',profile['commit']+':'+str(source)], cwd=ROOT) if profile.get('commit') else (ROOT / source).read_bytes()
    board = parse(data.decode())
    revision = child(child(board, 'title_block'), 'rev')
    assert revision == ['rev', profile['pcb']], 'PCB revision changed: create a new profile'
    footprints = {next(x[2] for x in children(f, 'property') if x[1] == 'Reference'): f
                  for f in children(board, 'footprint')}
    socket = pads(footprints['U3'])
    assert [p['number'] for p in socket] == list(range(1, 39))
    refs = profile['refs']
    # Only groups explicitly declared as copper net ties may merge net names.
    parents, net_ties = {}, []
    def canonical(net):
        parents.setdefault(net, net)
        if parents[net] != net:
            parents[net] = canonical(parents[net])
        return parents[net]
    for reference, footprint in footprints.items():
        groups = child(footprint, 'net_tie_pad_groups')
        if not groups:
            continue
        pad_nets = {str(p['number']): p['net'] for p in pads(footprint)}
        for group in groups[1:]:
            nets = [pad_nets[number.strip()] for number in group.split(',')]
            assert all(nets)
            for net in nets[1:]:
                parents[canonical(net)] = canonical(nets[0])
            net_ties.append(dict(reference=reference, nets=nets))
    reference_paths = []
    if profile.get('references'):
        reference = {p['number']:p for p in pads(footprints['TP1'])}
        for number, pin, diode, rail, label in [(39,2,'D2','+3V3_SYS','MB_3V3_SYS'),(40,1,'D1','GND','MB_GND')]:
            diode_pads = {p['number']:p for p in pads(footprints[diode])}
            assert diode_pads[2]['net'] == rail
            assert diode_pads[1]['net'] == reference[pin]['net']
            socket.append(dict(**{**reference[pin], 'number':number, 'label':label}))
            reference_paths.append(dict(input=number, reference='TP1', pin=pin,
                diode=diode, anode=rail, cathode=reference[pin]['net'], direction='output-to-input'))
    connectors = []
    for ref in refs:
        pins = pads(footprints[ref])
        kind = 'module' if ref in profile['modules'] else 'i2c' if ref.startswith('I2C') or ref in profile['i2c'] else 'gpio'
        assert len(pins) == (14 if kind == 'module' else 4)
        for pin in pins:
            pin['expected'] = [s['number'] for s in socket if pin['net'] and canonical(s['net']) == canonical(pin['net'])]
            for path in reference_paths:
                if canonical(pin['net']) == canonical(path['anode']): pin['expected'].append(path['input'])
            pin['expected'] = sorted(set(pin['expected']))
            pin['covered'] = bool(pin['expected'])
        connectors.append(dict(id=ref, label=profile['labels'].get(ref, ref), pigtail=kind, pins=pins))
    points = []
    for item in board:
        if isinstance(item, list) and child(item, 'layer') == ['layer', 'Edge.Cuts']:
            for key in ('start', 'mid', 'end'):
                if child(item, key):
                    points.append(tuple(map(float, child(item, key)[1:3])))
    assert points
    x, y = min(p[0] for p in points) - 1, min(p[1] for p in points) - 1
    width, height = max(p[0] for p in points) + 1 - x, max(p[1] for p in points) + 1 - y
    bounds = [round(v, 5) for v in (x, y, width, height)]
    definition = dict(schema=1, id=profile['id'], name=profile['name'],
        identityRevision=profile['identity'], pcbRevision=profile['pcb'], source=str(source), netTies=net_ties,
        sha256=hashlib.sha256(data).hexdigest(), bounds=bounds,
        socket=socket, connectors=connectors)
    if reference_paths:
        definition['referencePaths'] = reference_paths
        definition['sourceCommit'] = subprocess.check_output(['git','rev-parse',profile['commit']], cwd=ROOT, text=True).strip()
    cli = shutil.which('kicad-cli') or '/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli'
    with tempfile.TemporaryDirectory() as directory:
        output = Path(directory) / 'board.svg'
        rendering_pcb = Path(directory) / 'render.kicad_pcb'
        rendering_pcb.write_text(without_component_values(data.decode()))
        subprocess.run([cli, 'pcb', 'export', 'svg', '--layers', 'F.Silkscreen,F.Fab,Edge.Cuts',
            '--mode-single', '--page-size-mode', '1', '--exclude-drawing-sheet',
            '--output', str(output), str(rendering_pcb)], check=True, capture_output=True)
        svg = output.read_text()
    svg = re.sub(r'<title>.*?</title>', f'<title>{profile["name"]} PCB {profile["pcb"]}; KiCad top view</title>', svg, flags=re.S)
    svg = re.sub(r'width="[^"]*" height="[^"]*" viewBox="[^"]*"',
        f'width="{width}mm" height="{height}mm" viewBox="{x} {y} {width} {height}"', svg, count=1)
    zipped = gzip.compress(svg.encode(), mtime=0)
    header = '// EnvNode: see repository LICENSE. Generated; do not edit.\n'
    model = header
    model += f'static const SocketContact {symbol}Socket[] = {{\n'
    for p in socket:
        model += '    {%d, %s, %sf, %sf},\n' % (p['number'], json.dumps(p['net']), float(p['x']), float(p['y']))
    model += '};\n'
    for i, connector in enumerate(connectors):
        model += f'static const Contact {symbol}Contacts{i}[] = {{\n'
        for p in connector['pins']:
            mask = sum(1 << (n - 1) for n in p['expected'])
            model += '    {%d, %s, %sf, %sf, %dULL},\n' % (p['number'], json.dumps(p['net']), float(p['x']), float(p['y']), mask)
        model += '};\n'
    model += f'static const Connector {symbol}Connectors[] = {{\n'
    for i, c in enumerate(connectors):
        kind = dict(gpio='Gpio', i2c='I2c', module='Module')[c['pigtail']]
        model += f'    {{"{c["id"]}", Pigtail::{kind}, {len(c["pins"])}, {symbol}Contacts{i}}},\n'
    model += f'}};\nstatic const BoardDefinition {symbol}Board{{'
    model += ', '.join(json.dumps(definition[k]) for k in ('id', 'name', 'identityRevision', 'pcbRevision'))
    model += f', {symbol}Socket, {symbol}Connectors, sizeof({symbol}Connectors) / sizeof({symbol}Connectors[0]), {len(socket)}}};\n'
    assets = f'static const char {symbol}DefinitionJson[] PROGMEM = R"BOARD(' + json.dumps(definition, separators=(',', ':')) + ')BOARD";\n'
    assets += f'static const uint8_t {symbol}SvgGzip[] PROGMEM = {{\n'
    assets += '\n'.join(','.join(str(n) for n in zipped[i:i+32]) + ',' for i in range(0, len(zipped), 32))
    assets += '\n};\n'
    return definition, model, assets, svg

def generate():
    outputs = {}
    header = '// EnvNode: see repository LICENSE. Generated; do not edit.\n'
    assets = header + '#pragma once\n#include <Arduino.h>\nnamespace EnvNode { namespace Testing {\n'
    catalog = []
    for profile in PROFILES:
        definition, model, fragment, svg = generate_profile(profile)
        if not profile.get('references'): model = model.replace(', 38};', ', SocketPins};')
        outputs[f'firmware/testboard/boards/{profile["id"]}.json'] = json.dumps(definition, indent=2) + '\n'
        outputs[f'firmware/src/testboard/testing/{profile["symbol"]}Definition.inc'] = model
        assets += fragment
        published_svg = svg.replace('</svg>', '<!-- EnvNode: see LICENSE.txt in this directory (LicenseRef-EnvNode-Use-and-Redistribution-1.0). --></svg>')
        published_svg = '\n'.join(line.rstrip() for line in published_svg.splitlines())+'\n'
        image = hashlib.sha256(published_svg.encode()).hexdigest()+'.svg'
        binary = wire_profile(definition,image)
        digest = hashlib.sha256(binary).hexdigest()
        filename = definition['id']+'.'+digest+'.cbor'
        prefix = 'docs/site/assets/testboard/v1/'
        outputs[prefix+image] = published_svg
        outputs[prefix+filename] = binary
        catalog.append([definition['id'],definition['name'],definition['identityRevision'],definition['pcbRevision'],filename,len(binary),digest])
    assets += 'struct BoardAsset { const char* id; const char* json; size_t jsonSize; const uint8_t* svg; size_t svgSize; };\nstatic const BoardAsset BoardAssets[] = {\n'
    for profile in PROFILES:
        symbol = profile['symbol']
        assets += f'    {{"{profile["id"]}", {symbol}DefinitionJson, sizeof({symbol}DefinitionJson)-1, {symbol}SvgGzip, sizeof({symbol}SvgGzip)}},\n'
    assets += '};\n} }\n'
    outputs['firmware/src/testboard/testing/BoardAssets.h'] = assets
    outputs['docs/site/assets/testboard/v1/catalog.cbor'] = cbor([1,catalog])
    outputs['docs/site/assets/testboard/v1/LICENSE.txt'] = (ROOT/'LICENSE').read_text()
    return outputs

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for name, content in generate().items():
        path = ROOT / name
        if args.check:
            if not path.exists() or (path.read_bytes() if isinstance(content,bytes) else path.read_text()) != content:
                raise SystemExit('Profile assets are out of date: ' + name)
        else:
            path.parent.mkdir(parents=True,exist_ok=True)
            if isinstance(content,bytes): path.write_bytes(content)
            else: path.write_text(content)
        print(('Checked ' if args.check else 'Generated ') + name)
