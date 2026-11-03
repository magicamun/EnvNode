#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
"""Generate and validate the canonical product-board image set.

The selected PCB sources live in sibling worktrees below ROOT.parent. KiCad
source files are read only; bare views are made from a temporary board copy with
all component model entries removed.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

import yaml


ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / "docs/board-views.yml"
OUTPUT_ROOT = ROOT / "docs/site/assets/product-boards"
SCHEMA = 1

RENDERS = {
    "3d-assembled-front.png": ("assembled", "front", True),
    "3d-assembled-back.png": ("assembled", "back", True),
    "3d-bare-front.png": ("bare", "front", True),
    "3d-bare-back.png": ("bare", "back", True),
    "2d-assembled-front.png": ("assembled", "front", False),
    "2d-assembled-back.png": ("assembled", "back", False),
    "2d-bare-front.png": ("bare", "front", False),
    "2d-bare-back.png": ("bare", "back", False),
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def selected_entries(ids: list[str] | None = None) -> list[dict]:
    entries = [entry for entry in yaml.safe_load(CONFIG.read_text())
               if entry.get("shop_assets") and not entry.get("archive")]
    if ids:
        requested = set(ids)
        entries = [entry for entry in entries if entry["id"] in requested]
        found = {entry["id"] for entry in entries}
        if found != requested:
            raise ValueError(f"Unknown or non-shop board ids: {sorted(requested - found)}")
    return entries


def source_path(relative: str) -> Path:
    """Resolve both the local sibling-worktree layout and a normal CI checkout."""
    path = Path(relative)
    candidates = [ROOT.parent / path, ROOT / path]
    if path.parts and path.parts[0] == "main":
        candidates.append(ROOT.joinpath(*path.parts[1:]))
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    return candidates[0]


def source_bytes(entry: dict) -> tuple[bytes, str]:
    if entry.get("git_ref"):
        raw = subprocess.check_output([
            "git", "-C", str(ROOT), "show",
            f"{entry['git_ref']}:{entry['git_path']}",
        ])
        source = f"{entry['git_ref']}:{entry['git_path']}"
    else:
        path = source_path(entry["pcb"])
        raw = path.read_bytes()
        source = entry["pcb"]
    actual = sha256(raw)
    if actual != entry["sha256"]:
        raise ValueError(
            f"PCB source changed for {entry['id']}: expected {entry['sha256']}, got {actual}"
        )
    return raw, source


def sexp_nodes(tree: list, key: str) -> list:
    return [node for node in tree
            if isinstance(node, list) and node and str(node[0]) == key]


def bare_board(raw: bytes) -> tuple[bytes, int]:
    import sexpdata

    tree = sexpdata.loads(raw.decode())
    removed = 0
    for footprint in sexp_nodes(tree, "footprint"):
        models = sexp_nodes(footprint, "model")
        removed += len(models)
        footprint[:] = [node for node in footprint
                        if not (isinstance(node, list) and node and str(node[0]) == "model")]
    return sexpdata.dumps(tree).encode(), removed


def run(command: list[str]) -> str:
    env = {key: value for key, value in os.environ.items()
           if key not in ("DYLD_LIBRARY_PATH", "DYLD_FALLBACK_LIBRARY_PATH")}
    result = subprocess.run(command, capture_output=True, text=True, env=env)
    if result.returncode:
        raise RuntimeError(result.stdout[-1500:] + result.stderr[-1500:])
    return result.stdout + result.stderr


def png_size(path: Path) -> tuple[int, int]:
    data = path.read_bytes()[:24]
    if len(data) != 24 or data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"Not a valid PNG: {path}")
    return struct.unpack(">II", data[16:24])


def render(cli: str, board: Path, output: Path, side: str, perspective: bool) -> None:
    rotation = ("-25,0,5" if side == "front" else "-25,0,-5") if perspective else "0,0,0"
    command = [
        cli, "pcb", "render",
        "--width", "1800" if perspective else "2400",
        "--height", "1800" if perspective else "2400",
        "--side", "top" if side == "front" else "bottom",
        "--background", "transparent",
        "--quality", "high",
        "--rotate", rotation,
        "--zoom", "0.62" if perspective else "0.78",
    ]
    if perspective:
        command.append("--perspective")
    command.extend(["--output", str(output), str(board)])
    run(command)
    width, height = png_size(output)
    if width < 1000 or height < 1000:
        raise ValueError(f"Unexpectedly small render {output}: {width}x{height}")


def git_commit() -> str | None:
    result = subprocess.run(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD"],
        capture_output=True, text=True,
    )
    return result.stdout.strip() if result.returncode == 0 else None


def generate(entry: dict, cli: str) -> None:
    raw, source = source_bytes(entry)
    stripped, removed_models = bare_board(raw)
    destination = OUTPUT_ROOT / entry["id"]
    destination.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="envnode-product-assets-") as temp_dir:
        temp = Path(temp_dir)
        assembled = temp / "assembled.kicad_pcb"
        bare = temp / "bare.kicad_pcb"
        assembled.write_bytes(raw)
        bare.write_bytes(stripped)
        for filename, (population, side, perspective) in RENDERS.items():
            render(cli, assembled if population == "assembled" else bare,
                   destination / filename, side, perspective)

    assets = {}
    for filename, (population, side, perspective) in RENDERS.items():
        path = destination / filename
        assets[filename] = {
            "sha256": sha256(path.read_bytes()),
            "size_px": list(png_size(path)),
            "side": side,
            "projection": "perspective" if perspective else "orthographic",
            "component_models": population == "assembled",
        }

    manifest = {
        "schema": SCHEMA,
        "id": entry["id"],
        "guide": entry["guide"],
        "pcb_source": source,
        "pcb_sha256": entry["sha256"],
        "repository_commit": git_commit(),
        "generator": "tools/product_assets.py",
        "renderer": "KiCad pcb render",
        "bare_render_policy": (
            "Component model entries are removed from a temporary PCB copy; "
            "source copper, mask, silkscreen, footprints and pads are unchanged."
        ),
        "removed_component_models": removed_models,
        "assets": assets,
    }
    (destination / "asset-manifest.json").write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False) + "\n"
    )
    print(f"generated {entry['id']}: {len(assets)} renders")


def check(entry: dict) -> list[str]:
    errors = []
    try:
        source_bytes(entry)
    except (OSError, subprocess.CalledProcessError, ValueError) as exc:
        return [str(exc)]

    destination = OUTPUT_ROOT / entry["id"]
    manifest_path = destination / "asset-manifest.json"
    if not manifest_path.is_file():
        return [f"Missing manifest: {manifest_path.relative_to(ROOT)}"]
    try:
        manifest = json.loads(manifest_path.read_text())
    except (OSError, json.JSONDecodeError) as exc:
        return [f"Invalid manifest {manifest_path.relative_to(ROOT)}: {exc}"]

    if manifest.get("schema") != SCHEMA:
        errors.append(f"{entry['id']}: unsupported manifest schema")
    if manifest.get("id") != entry["id"]:
        errors.append(f"{entry['id']}: manifest id mismatch")
    if manifest.get("guide") != entry["guide"]:
        errors.append(f"{entry['id']}: guide mismatch")
    if manifest.get("pcb_sha256") != entry["sha256"]:
        errors.append(f"{entry['id']}: stale PCB digest in manifest")

    manifest_assets = manifest.get("assets", {})
    for filename, (_, side, perspective) in RENDERS.items():
        path = destination / filename
        record = manifest_assets.get(filename)
        if not path.is_file():
            errors.append(f"{entry['id']}: missing {filename}")
            continue
        if not isinstance(record, dict):
            errors.append(f"{entry['id']}: missing manifest record for {filename}")
            continue
        data = path.read_bytes()
        try:
            size = list(png_size(path))
        except ValueError as exc:
            errors.append(str(exc))
            continue
        if record.get("sha256") != sha256(data):
            errors.append(f"{entry['id']}: changed generated asset {filename}")
        if record.get("size_px") != size:
            errors.append(f"{entry['id']}: image size mismatch for {filename}")
        expected_projection = "perspective" if perspective else "orthographic"
        if record.get("projection") != expected_projection or record.get("side") != side:
            errors.append(f"{entry['id']}: metadata mismatch for {filename}")
    unexpected = set(manifest_assets) - set(RENDERS)
    if unexpected:
        errors.append(f"{entry['id']}: unexpected manifest assets: {sorted(unexpected)}")
    return errors


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("generate", "check"))
    parser.add_argument("--id", action="append", help="Select one or more shop board ids")
    parser.add_argument(
        "--kicad-cli",
        default=shutil.which("kicad-cli")
        or "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli",
    )
    args = parser.parse_args()
    entries = selected_entries(args.id)
    if args.command == "generate":
        if not Path(args.kicad_cli).is_file():
            parser.error(f"KiCad CLI not found: {args.kicad_cli}")
        for entry in entries:
            generate(entry, args.kicad_cli)
    else:
        errors = [error for entry in entries for error in check(entry)]
        if errors:
            raise SystemExit("Product asset check failed:\n- " + "\n- ".join(errors))
        print(f"product assets valid: {', '.join(entry['id'] for entry in entries)}")


if __name__ == "__main__":
    main()
