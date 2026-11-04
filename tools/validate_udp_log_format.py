#!/usr/bin/env python3
"""Compile the portable codec and parse its actual RFC5424 JSON payloads.
EnvNode-owned: see repository LICENSE. No network operations.
"""
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include "UdpLogging.h"
#include <cstdio>
#include <cstring>
using namespace EnvNode;
int main() {
    UdpLogIdentity id;
    strcpy(id.device,"001122aabbcc");strcpy(id.boot,"0123456789abcdef0123456789abcdef");
    memset(id.build,'a',sizeof(id.build)-1);
    DiagnosticRecord r;r.source={DiagnosticSourceKind::Sensor,1};r.sequence=42;r.uptimeMs=250;
    r.event=DiagnosticEvent("rain_window");r.event.integer("revision",17);
    r.event.integer("start_us",0);r.event.integer("end_us",250000);
    r.event.integer("pulses",123);r.event.number("frequency_hz",492);r.event.text("fault","none");
    char out[UdpLogMaxBytes];
    if(!encodeSyslogDiagnostic(id,r,out,sizeof(out)))return 1;puts(out);
    r.event=DiagnosticEvent("maximum");char text[DiagnosticTextBytes];
    memset(text,1,sizeof(text)-1);text[sizeof(text)-1]=0;
    for(unsigned i=0;i<6;++i){char key[DiagnosticKeyBytes];memset(key,'a',sizeof(key)-1);key[sizeof(key)-2]='0'+i;key[sizeof(key)-1]=0;r.event.text(key,text);}
    if(!encodeSyslogDiagnostic(id,r,out,sizeof(out)))return 2;puts(out);
    LogEntry entry;entry.sequence=7;entry.wallClockValid=true;entry.epochSeconds=1793700000;
    memset(entry.message,1,sizeof(entry.message)-1);
    if(!encodeSyslogSystem(id,entry,out,sizeof(out)))return 3;puts(out);
}
'''


def unique_object(pairs):
    obj = {}
    for key, value in pairs:
        if key in obj:
            raise ValueError(f"duplicate JSON key: {key}")
        obj[key] = value
    return obj


def main():
    base = ROOT / "firmware/lib"
    with tempfile.TemporaryDirectory(prefix="envnode-codec-") as temp:
        fixture = Path(temp) / "fixture.cpp"
        binary = Path(temp) / "fixture"
        fixture.write_text(SOURCE)
        command = ["c++", "-std=c++11"]
        for library in ["EnvNodeUdpLogging", "EnvNodeDiagnostics", "EnvNodeLogging"]:
            command += ["-I", str(base / library / "include")]
        command += [str(fixture), str(base / "EnvNodeUdpLogging/src/UdpLogging.cpp"),
                    str(base / "EnvNodeDiagnostics/src/DiagnosticRecorder.cpp"),
                    str(base / "EnvNodeLogging/src/LogLevel.cpp"), "-o", str(binary)]
        subprocess.run(command, check=True)
        messages = subprocess.check_output([str(binary)], text=True).splitlines()
    assert len(messages) == 3
    records = []
    for wire in messages:
        parts = wire.split(" ", 7)
        assert len(parts) == 8 and parts[0] == "<134>1"
        assert parts[1] == "-" and parts[2] == "001122aabbcc"
        assert parts[3] in {"envnode-system", "envnode-diagnostic"}
        record = json.loads(parts[7], object_pairs_hook=unique_object)
        assert record["device_id"] == parts[2] and record["boot_id"] == parts[4]
        assert len(wire.encode()) < 2048
        records.append(record)
    assert records[0]["values"] == dict(revision=17, start_us=0, end_us=250000,
                                      pulses=123, frequency_hz=492, fault="none")
    assert records[0]["epoch_s"] is None and records[0]["sequence"] == 42
    assert all(value == "\x01" * 32 for value in records[1]["values"].values())
    assert records[2]["message"] == "\x01" * 255
    assert records[2]["epoch_s"] == 1793700000
    print("3 RFC5424/JSON fixtures valid, no duplicate keys or field truncation")


if __name__ == "__main__":
    main()
