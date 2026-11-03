#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
"""Native orthographic KiCad PCB renderings without component models.

Run with requirements-docs-render.txt and KiCad installed. Source boards are read
only. Model entries are removed in a temporary copy; all PCB layers stay unchanged.
"""
from __future__ import annotations
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from PIL import Image
import sexpdata
import yaml

ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / 'docs/site'

def nodes(tree, key):
    return [n for n in tree if isinstance(n, list) and n and str(n[0]) == key]

def node(tree, key, fallback=None):
    return next(iter(nodes(tree, key)), fallback)

def digest(data):
    return hashlib.sha256(data).hexdigest()

def board_bytes(entry):
    if entry.get('git_ref'):
        raw = subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                       entry['git_ref'] + ':' + entry['git_path']])
        source = entry['git_ref'] + ':' + entry['git_path']
    else:
        path = ROOT.parent / entry['pcb']
        raw = path.read_bytes()
        source = entry['pcb']
    if digest(raw) != entry['sha256']:
        raise ValueError(f"Source changed; select/review the documented PCB: {entry['id']}")
    return raw, source

def back_components(tree):
    refs = []
    for fp in nodes(tree, 'footprint'):
        props = {p[1]: p[2] for p in nodes(fp, 'property')}
        ref = props.get('Reference', '')
        if node(fp, 'layer', ['', ''])[1] == 'B.Cu' and not ref.startswith(('H', 'NT')):
            refs.append(ref)
    return sorted(refs)

def run(cmd):
    # The optional Python Cairo runtime must not override KiCad's bundled dylibs.
    env = {key: value for key, value in os.environ.items()
           if key not in ('DYLD_LIBRARY_PATH', 'DYLD_FALLBACK_LIBRARY_PATH')}
    result = subprocess.run(cmd, capture_output=True, text=True, env=env)
    if result.returncode:
        raise RuntimeError(result.stdout[-1000:] + result.stderr[-1000:])

def render_hero(cli, entry, side, output, source_board):
    output.parent.mkdir(parents=True, exist_ok=True)
    run([cli, 'pcb', 'render', '--width', '1800', '--height', '1800',
         '--side', side, '--background', 'transparent', '--quality', 'high',
         '--rotate', '-25,0,5' if side == 'top' else '-25,0,-5',
         '--perspective', '--zoom', '0.6', '--output', str(output), str(source_board)])
    image = Image.open(output).convert('RGBA')
    bounds = image.getbbox()
    if bounds:
        image = image.crop((max(0, bounds[0]-16), max(0, bounds[1]-16),
                            min(image.width, bounds[2]+16), min(image.height, bounds[3]+16)))
        image.save(output)

def create(entry, cli):
    raw, source = board_bytes(entry)
    tree = sexpdata.loads(raw.decode())
    back = back_components(tree)
    dest = SITE / 'assets/board-views' / entry['id']
    dest.mkdir(parents=True, exist_ok=True)
    previous = dest / 'views.json'
    cached_source_matches = previous.is_file() and json.loads(previous.read_text()).get('pcb_sha256') == entry['sha256']
    views = {}
    with tempfile.TemporaryDirectory(prefix='envnode-board-view-') as temp_dir:
        temp = Path(temp_dir)
        bare = copy.deepcopy(tree)
        removed_models = 0
        for fp in nodes(bare, 'footprint'):
            removed_models += len(nodes(fp, 'model'))
            fp[:] = [n for n in fp if not (isinstance(n, list) and n and str(n[0]) == 'model')]
        plot = temp / 'assembly.kicad_pcb'
        plot.write_text(sexpdata.dumps(bare))
        for side in ['front'] + (['back'] if back else []):
            png = dest / f'assembly-{side}.png'
            run([cli, 'pcb', 'render', '--width', '2800', '--height', '2800',
                 '--side', 'top' if side == 'front' else 'bottom',
                 '--background', 'transparent', '--quality', 'high',
                 '--rotate', '0,0,0', '--zoom', '0.78',
                 '--output', str(png), str(plot)])
            image = Image.open(png).convert('RGBA')
            bounds = image.getbbox()
            if not bounds:
                raise ValueError(f"Empty board rendering: {entry['id']} {side}")
            image = image.crop((max(0, bounds[0]-24), max(0, bounds[1]-24),
                                min(image.width, bounds[2]+24), min(image.height, bounds[3]+24)))
            image.save(png)
            (dest / f'assembly-{side}.svg').unlink(missing_ok=True)
            views[side] = {'png': str(png.relative_to(SITE)),
                           'png_size_px': list(image.size),
                           'side': 'F.Cu' if side == 'front' else 'B.Cu',
                           'projection': 'orthographic', 'component_models': False,
                           'renderer': 'KiCad pcb render', 'quality': 'high',
                           'canvas_size_px': [2800, 2800], 'rotation_deg': [0, 0, 0],
                           'zoom': 0.78, 'mirrored_for_back_view': side == 'back',
                           'overlay_calibration': 'PCB coordinates are provided separately; pixel mapping requires calibration to the cropped rendering.'}
        heroes = {}
        for side in ['front'] + (['back'] if back else []):
            relative = entry.get('hero_' + side)
            if relative and (SITE / relative).is_file():
                heroes[side] = relative
            else:
                output = dest / f'3d-{side}.png'
                if entry.get('git_ref'):
                    source_board = temp / 'source.kicad_pcb'
                    source_board.write_bytes(raw)
                else:
                    source_board = ROOT.parent / entry['pcb']
                if not output.is_file() or not cached_source_matches:
                    render_hero(cli, entry, 'top' if side == 'front' else 'bottom', output, source_board)
                heroes[side] = str(output.relative_to(SITE))
    footprints = []
    for fp in nodes(tree, 'footprint'):
        props = {p[1]: p[2] for p in nodes(fp, 'property')}
        at = node(fp, 'at', ['', 0, 0])[1:]
        footprints.append({'reference': props.get('Reference'), 'value': props.get('Value'),
                           'side': node(fp, 'layer')[1], 'position_mm': at[:2],
                           'rotation_deg': at[2] if len(at) > 2 else 0})
    assets = [dest / f'assembly-{side}.{suffix}' for side in views for suffix in ('png',)]
    assets += [SITE / relative for relative in heroes.values()]
    metadata = {'id': entry['id'], 'guide': entry['guide'], 'pcb_source': source,
                'pcb_sha256': entry['sha256'], 'back_components': back,
                'hero': heroes, 'plans': views, 'footprints': footprints,
                'plot_policy': 'Orthographic native KiCad rendering. Only model entries are removed in a temporary copy; original copper, solder mask, silkscreen, references and source files remain unchanged.',
                'removed_model_entries': removed_models,
                'asset_sha256': {str(p.relative_to(SITE)): digest(p.read_bytes()) for p in assets},
                'license': 'EnvNode documentation: see root LICENSE; native KiCad model rights unchanged.'}
    (dest / 'views.json').write_text(json.dumps(metadata, indent=2, ensure_ascii=False) + '\n')
    return entry['id'], back

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--id', action='append', help='Select one or more board-view ids')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--kicad-cli', default=shutil.which('kicad-cli') or '/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli')
    args = parser.parse_args()
    entries = yaml.safe_load((ROOT / 'docs/board-views.yml').read_text())
    if args.id:
        entries = [e for e in entries if e['id'] in args.id]
        if {e['id'] for e in entries} != set(args.id):
            parser.error('Unknown board id')
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for future in as_completed([pool.submit(create, e, args.kicad_cli) for e in entries]):
            board, back = future.result()
            print(f"{board}: front plan; back components: {', '.join(back) or 'none'}", flush=True)
    catalogue = []
    for entry in yaml.safe_load((ROOT / 'docs/board-views.yml').read_text()):
        metadata = SITE / 'assets/board-views' / entry['id'] / 'views.json'
        if not metadata.is_file():
            continue
        views = json.loads(metadata.read_text())
        catalogue.append({'id': entry['id'], 'guide': entry['guide'],
                          'archive': entry.get('archive', False),
                          'metadata': str(metadata.relative_to(SITE)),
                          'front': views['plans']['front'],
                          'back_components': views['back_components']})
    (SITE / 'assets/board-views/index.json').write_text(
        json.dumps({'schema': 2, 'path_base': 'site root', 'boards': catalogue},
                   indent=2, ensure_ascii=False) + '\n')


if __name__ == '__main__':
    main()
