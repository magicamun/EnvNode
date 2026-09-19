#!/usr/bin/env python3
"""Measure the proposed schema 0.1 compact deterministic CBOR profile.

This is a format exploration tool, not a production encoder. It deliberately
uses no third-party dependency so that the measurements remain reproducible.
"""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path
from typing import Any


ENVELOPE_SIZE = 32
DESCRIPTOR_BANK_SIZE = 1792
MUTABLE_BANK_SIZE = 256
COMPLETE_BANK_SIZE = 2048

# Each schema field has a globally unique number. This costs one extra byte for
# keys above 23 but prevents ambiguous maps as the schema evolves.
KEY_NAMES = (
    "schemaVersion", "objectKind", "identity", "compatibility", "description",
    "hardware", "manufacturing", "calibration", "typeId", "instanceId",
    "manufacturer", "hardwareRevision", "legacyProfileId", "major", "minor",
    "minimumFirmwareVersion", "platform", "safetyProfile", "drivers",
    "capabilities", "id", "apiVersion", "name", "summary", "documentationUrl",
    "resources", "slots", "kind", "platformBinding", "voltageMillivolts",
    "properties", "interface", "identityAddress", "bindings", "requirements",
    "devices", "resource", "driver", "parameters", "measurements",
    "serialNumber", "productionBatch", "productionDate", "target", "schema",
    "values",
)
KEYS = {name: number for number, name in enumerate(KEY_NAMES)}

# Only standardized vocabulary receives integer codes. Manufacturer-controlled
# product IDs, instance-local IDs and logical resource names remain strings.
VOCABULARY = {
    "board": 0,
    "module": 1,
    "gpio": 0,
    "i2c": 1,
    "spi": 2,
    "power": 3,
    "sensor": 0,
    "actuator": 1,
    "infrastructure": 2,
    "org.envnode.platform.esp32": 1,
    "org.envnode.platform.any": 2,
    "org.envnode.safety.esp32-envnode-mini": 1,
    "org.envnode.safety.module-interface": 2,
    "org.envnode.interface.module-2x7": 1,
    "org.envnode.driver.gpio-on-off": 1,
    "org.envnode.driver.ads1115": 2,
    "digital-input": 1,
    "digital-output": 2,
    "analog-input": 3,
    "supply": 4,
    "actuator.on-off": 5,
    "sensor.pressure": 6,
    "sensor.current": 7,
}


def _head(major: int, value: int) -> bytes:
    if value < 24:
        return bytes([(major << 5) | value])
    if value <= 0xFF:
        return bytes([(major << 5) | 24, value])
    if value <= 0xFFFF:
        return bytes([(major << 5) | 25]) + struct.pack(">H", value)
    if value <= 0xFFFFFFFF:
        return bytes([(major << 5) | 26]) + struct.pack(">I", value)
    return bytes([(major << 5) | 27]) + struct.pack(">Q", value)


def encode_cbor(value: Any) -> bytes:
    if value is None:
        return b"\xf6"
    if value is False:
        return b"\xf4"
    if value is True:
        return b"\xf5"
    if isinstance(value, int):
        if value >= 0:
            return _head(0, value)
        return _head(1, -1 - value)
    if isinstance(value, float):
        return b"\xfb" + struct.pack(">d", value)
    if isinstance(value, bytes):
        return _head(2, len(value)) + value
    if isinstance(value, str):
        encoded = value.encode("utf-8")
        return _head(3, len(encoded)) + encoded
    if isinstance(value, list):
        return _head(4, len(value)) + b"".join(encode_cbor(item) for item in value)
    if isinstance(value, dict):
        items = [(encode_cbor(key), encode_cbor(item)) for key, item in value.items()]
        items.sort(key=lambda pair: (len(pair[0]), pair[0]))
        return _head(5, len(items)) + b"".join(key + item for key, item in items)
    raise TypeError(f"unsupported CBOR value: {type(value).__name__}")


def validate_encoder() -> None:
    if len(KEYS) != len(KEY_NAMES):
        raise RuntimeError("compact key assignments are not unique")
    vectors = (
        (0, "00"),
        (24, "1818"),
        (-1, "20"),
        ("a", "6161"),
        ([1, 2], "820102"),
        ({1: 2}, "a10102"),
        ({24: 0, 1: 0}, "a20100181800"),
    )
    for value, expected_hex in vectors:
        actual_hex = encode_cbor(value).hex()
        if actual_hex != expected_hex:
            raise RuntimeError(
                f"CBOR self-test failed for {value!r}: {actual_hex} != {expected_hex}"
            )


def compact_value(key: str | None, value: Any) -> Any:
    if key == "schemaVersion" or key == "minimumFirmwareVersion":
        return [int(part) for part in value.split(".")]
    if key == "instanceId":
        return bytes.fromhex(value.replace("-", ""))
    if (key in {"objectKind", "kind", "capabilities", "interface", "contractId"}
            and isinstance(value, str) and value in VOCABULARY):
        return VOCABULARY[value]
    if isinstance(value, list):
        return [compact_value(key, item) for item in value]
    if isinstance(value, dict):
        if key in {"bindings", "parameters", "properties", "values"}:
            return {
                child_key: compact_value(child_key, child_value)
                for child_key, child_value in value.items()
            }
        compact = {}
        for child_key, child_value in value.items():
            if child_key not in KEYS:
                raise ValueError(f"no compact key assigned for {child_key!r}")
            child_context = (
                "contractId"
                if child_key == "id" and "apiVersion" in value
                else child_key
            )
            compact[KEYS[child_key]] = compact_value(child_context, child_value)
        return compact
    return value


def split_payload(document: dict[str, Any]) -> tuple[dict[str, Any], dict[str, Any]]:
    identity = dict(document["identity"])
    instance_id = identity.pop("instanceId")
    static = {
        "schemaVersion": document["schemaVersion"],
        "objectKind": document["objectKind"],
        "identity": identity,
        "compatibility": document["compatibility"],
        "description": document["description"],
        "hardware": document["hardware"],
    }
    mutable = {
        "schemaVersion": document["schemaVersion"],
        "identity": {"instanceId": instance_id},
        "manufacturing": document["manufacturing"],
        "calibration": document["calibration"],
    }
    return static, mutable


def measure(path: Path) -> dict[str, int | str]:
    document = json.loads(path.read_text(encoding="utf-8"))
    complete = {
        key: value
        for key, value in document.items()
        if key not in {"$schema", "assessment"}
    }
    static, mutable = split_payload(document)
    complete_size = len(encode_cbor(compact_value(None, complete)))
    static_size = len(encode_cbor(compact_value(None, static)))
    mutable_size = len(encode_cbor(compact_value(None, mutable)))
    return {
        "descriptor": path.name,
        "complete": complete_size,
        "complete_with_envelope": complete_size + ENVELOPE_SIZE,
        "complete_free": COMPLETE_BANK_SIZE - complete_size - ENVELOPE_SIZE,
        "static": static_size,
        "static_with_envelope": static_size + ENVELOPE_SIZE,
        "static_free": DESCRIPTOR_BANK_SIZE - static_size - ENVELOPE_SIZE,
        "mutable": mutable_size,
        "mutable_with_envelope": mutable_size + ENVELOPE_SIZE,
        "mutable_free": MUTABLE_BANK_SIZE - mutable_size - ENVELOPE_SIZE,
    }


def main() -> int:
    validate_encoder()
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "paths",
        nargs="*",
        type=Path,
        default=sorted(Path("docs/identity/examples").glob("*.json")),
    )
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    args = parser.parse_args()
    rows = [measure(path) for path in args.paths]
    if any(row["complete_free"] < 0 for row in rows):
        raise RuntimeError("at least one complete descriptor exceeds its 2048-byte bank")
    if args.json:
        print(json.dumps(rows, indent=2))
        return 0

    print("descriptor                         full+env  free  static+env  free  mutable+env  free")
    for row in rows:
        print(
            f"{row['descriptor']:<34}"
            f"{row['complete_with_envelope']:>7} B"
            f"{row['complete_free']:>7} B"
            f"{row['static_with_envelope']:>7} B"
            f"{row['static_free']:>7} B"
            f"{row['mutable_with_envelope']:>11} B"
            f"{row['mutable_free']:>7} B"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
