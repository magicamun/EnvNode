# EnvNode: see repository LICENSE.
"""Derive direct-net continuity expectations and display positions from a DUT PCB.

Only explicit PCB net-tie groups join distinct net names; component paths are not inferred. A contact with no matching
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
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROFILES = [
    dict(symbol='Weather', id='weather-0.2-pcb0.2.1', name='EnvNode Weather', identity='0.2', pcb='0.2.1',
         source='hardware/kicad/MainBoards/Weatherstation/Weatherstation.kicad_pcb',
         refs=['GPIO16', 'GPIO14', 'GPIO17', 'GPIO27', 'GPIO32', 'GPIO4', 'ADC1', 'ADC2', 'ADC3', 'ADC4']
              + [f'I2C{bus}_{i}' for bus in (0, 1) for i in range(1, 5)] + ['J1'],
         modules=['J1'], i2c=[], labels={}),
    dict(symbol='Mini', id='mini-0.8-pcb0.8.1', name='EnvNode Mini', identity='0.8', pcb='0.8.1',
         source='hardware/kicad/MainBoards/EnvNode Mini/EnvNode Mini.kicad_pcb',
         refs=['ADC1', 'ADC2', 'J4', 'J5', 'J6', 'J7', 'J1', 'J2'], modules=['J1', 'J2'],
         i2c=['J4', 'J5', 'J6', 'J7'], labels={'J4':'I2C0 · J4', 'J5':'I2C0 · J5',
             'J6':'I2C1 · J6', 'J7':'I2C1 · J7', 'J1':'Slot A · J1', 'J2':'Slot B · J2'}),
]

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
    data = (ROOT / source).read_bytes()
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
    connectors = []
    for ref in refs:
        pins = pads(footprints[ref])
        kind = 'module' if ref in profile['modules'] else 'i2c' if ref.startswith('I2C') or ref in profile['i2c'] else 'gpio'
        assert len(pins) == (14 if kind == 'module' else 4)
        for pin in pins:
            pin['expected'] = [s['number'] for s in socket if pin['net'] and canonical(s['net']) == canonical(pin['net'])]
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
    cli = shutil.which('kicad-cli') or '/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli'
    with tempfile.TemporaryDirectory() as directory:
        output = Path(directory) / 'board.svg'
        subprocess.run([cli, 'pcb', 'export', 'svg', '--layers', 'F.Silkscreen,F.Fab,Edge.Cuts',
            '--mode-single', '--page-size-mode', '1', '--exclude-drawing-sheet',
            '--output', str(output), str(ROOT / source)], check=True, capture_output=True)
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
    model += f', {symbol}Socket, {symbol}Connectors, sizeof({symbol}Connectors) / sizeof({symbol}Connectors[0])}};\n'
    assets = f'static const char {symbol}DefinitionJson[] PROGMEM = R"BOARD(' + json.dumps(definition, separators=(',', ':')) + ')BOARD";\n'
    assets += f'static const uint8_t {symbol}SvgGzip[] PROGMEM = {{\n'
    assets += '\n'.join(','.join(str(n) for n in zipped[i:i+32]) + ',' for i in range(0, len(zipped), 32))
    assets += '\n};\n'
    return definition, model, assets

def generate():
    outputs = {}
    header = '// EnvNode: see repository LICENSE. Generated; do not edit.\n'
    assets = header + '#pragma once\n#include <Arduino.h>\nnamespace EnvNode { namespace Testing {\n'
    for profile in PROFILES:
        definition, model, fragment = generate_profile(profile)
        outputs[f'firmware/testboard/boards/{profile["id"]}.json'] = json.dumps(definition, indent=2) + '\n'
        outputs[f'firmware/src/testboard/testing/{profile["symbol"]}Definition.inc'] = model
        assets += fragment
    assets += 'struct BoardAsset { const char* id; const char* json; size_t jsonSize; const uint8_t* svg; size_t svgSize; };\nstatic const BoardAsset BoardAssets[] = {\n'
    for profile in PROFILES:
        symbol = profile['symbol']
        assets += f'    {{"{profile["id"]}", {symbol}DefinitionJson, sizeof({symbol}DefinitionJson)-1, {symbol}SvgGzip, sizeof({symbol}SvgGzip)}},\n'
    assets += '};\n} }\n'
    outputs['firmware/src/testboard/testing/BoardAssets.h'] = assets
    return outputs

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for name, content in generate().items():
        path = ROOT / name
        if args.check:
            if not path.exists() or path.read_text() != content:
                raise SystemExit('Profile assets are out of date: ' + name)
        else:
            path.write_text(content)
        print(('Checked ' if args.check else 'Generated ') + name)
