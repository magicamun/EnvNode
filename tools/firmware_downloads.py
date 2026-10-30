#!/usr/bin/env python3
"""Publish firmware and render its download catalog. See repository LICENSE."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TARGETS = {
    "envnode": ("EnvNode", "esp32dev", "include/FirmwareVersion.h"),
    "tester": ("Tester", "testboard", "src/testboard/TestBoardFirmwareVersion.h"),
}


def run(*args):
    return subprocess.check_output(args, cwd=ROOT, text=True).strip()


def publish(target):
    title, environment, header = TARGETS[target]
    version = re.search(r'FirmwareVersion\s*=\s*"([0-9]+\.[0-9]+\.[0-9]+)"',
                        (ROOT / "firmware" / header).read_text())[1]
    notes = (ROOT / "docs/firmware-releases" / target / f"{version}.md").read_text().strip()
    if not notes:
        raise ValueError("Release notes must not be empty")
    filename = f"{target}-{version}.bin"
    binary = ROOT / "firmware/bin" / filename
    metadata = dict(target=target, version=version, commit=run("git", "rev-parse", "HEAD"),
                    built_at=datetime.now(timezone.utc).isoformat(), filename=filename,
                    sha256=hashlib.sha256(binary.read_bytes()).hexdigest(), notes=notes)
    tag = f"firmware-{target}-{version}"
    releases = json.loads(run("gh", "api", "--paginate", "--slurp", "repos/{owner}/{repo}/releases"))
    existing = [r for page in releases for r in page if r["tag_name"] == tag]
    with tempfile.TemporaryDirectory() as directory:
        manifest = Path(directory) / "firmware.json"
        manifest.write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n")
        if not existing:
            run("gh", "release", "create", tag, "--target", metadata["commit"],
                "--title", f"{title} {version}", "--latest=false", "--notes", notes + "\n\nLizenz: siehe LICENSE im Repository.")
        else:
            run("gh", "release", "edit", tag, "--notes", notes + "\n\nLizenz: siehe LICENSE im Repository.")
        run("gh", "release", "upload", tag, str(binary), str(manifest), str(ROOT / "LICENSE"), "--clobber")


def catalog():
    pages = json.loads(run("gh", "api", "--paginate", "--slurp", "repos/{owner}/{repo}/releases"))
    entries = []
    for release in (r for page in pages for r in page):
        if release["draft"] or not release["tag_name"].startswith("firmware-"):
            continue
        if not any(a["name"] == "firmware.json" for a in release["assets"]):
            continue
        with tempfile.TemporaryDirectory() as directory:
            run("gh", "release", "download", release["tag_name"], "--pattern", "firmware.json", "--dir", directory)
            entry = json.loads((Path(directory) / "firmware.json").read_text())
        assets = {a["name"]: a["browser_download_url"] for a in release["assets"]}
        entry["url"] = assets[entry["filename"]]
        entries.append(entry)
    render(entries)


def render(entries):
    page = ROOT / "docs/site/firmware/downloads.md"
    intro = (ROOT / "docs/firmware-releases/introduction.md").read_text().rstrip()
    lines = [intro, ""]
    for target, (title, _, _) in TARGETS.items():
        lines += [f"## {title}", ""]
        versions = sorted((e for e in entries if e["target"] == target),
                          key=lambda e: tuple(map(int, e["version"].split("."))), reverse=True)
        if not versions:
            lines += ["Noch kein Build veröffentlicht.", ""]
        for entry in versions:
            lines += [f"### Version {entry['version']}", "",
                      f"[**{entry['filename']} herunterladen**]({entry['url']})", "",
                      f"Commit: [`{entry['commit'][:7]}`](https://github.com/magicamun/EnvNode/commit/{entry['commit']}) · Build (UTC): {entry['built_at']}", "",
                      entry["notes"], "", f"SHA-256: `{entry['sha256']}`", ""]
    page.write_text("\n".join(lines))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["publish", "catalog", "preview"])
    parser.add_argument("--target", choices=TARGETS)
    args = parser.parse_args()
    if args.action == "publish":
        if not args.target:
            parser.error("publish requires --target")
        publish(args.target)
    elif args.action == "catalog":
        catalog()
    else:
        render([])
