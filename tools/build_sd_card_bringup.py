#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
"""Export the canonical web/PDF SD guide to its historical hardware-tree path.

The Markdown source, template and renderer are shared with docs.envnode.de.
Run without arguments to build the documentation first; --from-site reuses an
already reviewed build after checking its source and PDF digests.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--from-site', type=Path, help='Reuse an existing MkDocs site directory')
    args = parser.parse_args()
    if args.from_site:
        site = args.from_site.resolve()
    else:
        subprocess.run([sys.executable, '-m', 'mkdocs', 'build', '--strict'], cwd=ROOT, check=True)
        site = ROOT / 'site'
    source = ROOT / 'docs/site/modules/sd-card/index.md'
    pdf = site / 'assets/downloads/SD_Card_Rev0.1_Anleitung.pdf'
    manifest = json.loads(pdf.with_suffix('.json').read_text())
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    if manifest['markdown_sha256'] != digest(source) or manifest['pdf_sha256'] != digest(pdf):
        raise ValueError('SD guide is stale or changed; rebuild and review the documentation')
    destination = ROOT / 'hardware/kicad/Modules/FullSize/SD-Card/Documentation/SD_Card_Bestueckungs_und_Bringup_Rev0.1.pdf'
    shutil.copyfile(pdf, destination)
    print(f'Canonical SD guide exported: {destination}')


if __name__ == '__main__':
    main()
