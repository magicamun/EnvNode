# EnvNode: see repository LICENSE.
"""Backport the SD transaction lifetime fix without modifying PlatformIO packages."""

import hashlib
from pathlib import Path
import re
import shutil


ORIGINAL_UNINIT_SHA256 = "f70d2698bd324dd49574297572217213781411926a68b6d87e47a58af030972b"
PATCHED_UNINIT_SHA256 = "d56dd5f402c5a368b910536b554796d3d5ae7aba3890528c10eb416b3884cc92"
OLD_TRANSACTION = "    AcquireSPI lock(card);\n    sdTransaction(pdrv, GO_IDLE_STATE, 0, NULL);"
NEW_TRANSACTION = (
    "    {\n"
    "        AcquireSPI lock(card);\n"
    "        sdTransaction(pdrv, GO_IDLE_STATE, 0, NULL);\n"
    "    } // End the transaction before freeing card."
)


def uninit_function(source):
    match = re.search(r"^uint8_t sdcard_uninit\([^\n]*\)\n\{.*?^\}\n", source, re.M | re.S)
    if not match:
        raise RuntimeError("SD patch: cannot identify sdcard_uninit; review the framework")
    return match


def patch_source(source):
    match = uninit_function(source)
    digest = hashlib.sha256(match.group().encode("utf-8")).hexdigest()
    if digest == PATCHED_UNINIT_SHA256:
        return source
    if digest != ORIGINAL_UNINIT_SHA256:
        raise RuntimeError("SD patch: unknown sdcard_uninit SHA-256 " + digest
                           + "; review the framework before building")
    patched = match.group().replace(OLD_TRANSACTION, NEW_TRANSACTION, 1)
    return source[:match.start()] + patched + source[match.end():]


def prepare_library(framework_dir, destination):
    framework_dir, destination = Path(framework_dir), Path(destination)
    version = (framework_dir / "cores/esp32/esp_arduino_version.h").read_text()
    for name, expected in (("MAJOR", 2), ("MINOR", 0), ("PATCH", 17)):
        if not re.search(r"^#define ESP_ARDUINO_VERSION_" + name
                         + r"\s+" + str(expected) + r"\s*$", version, re.M):
            raise RuntimeError("SD patch supports Arduino-ESP32 2.0.17 only; review toolchain upgrade")
    original = framework_dir / "libraries/SD"
    source = original / "src/sd_diskio.cpp"
    patched = patch_source(source.read_text())
    # Copy all upstream headers and sources, retaining their copyright/license
    # notices. The global framework package is only read, never written.
    destination.mkdir(parents=True, exist_ok=True)
    for path in (original / "src").rglob("*"):
        if not path.is_file():
            continue
        output = destination / "src" / path.relative_to(original / "src")
        contents = patched.encode("utf-8") if path == source else path.read_bytes()
        output.parent.mkdir(parents=True, exist_ok=True)
        if not output.exists() or output.read_bytes() != contents:
            output.write_bytes(contents)
    manifest = original / "library.properties"
    output_manifest = destination / manifest.name
    if not output_manifest.exists() or output_manifest.read_bytes() != manifest.read_bytes():
        shutil.copyfile(manifest, output_manifest)
    return destination
