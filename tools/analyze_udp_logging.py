#!/usr/bin/env python3
"""Summarize receiver JSONL without changing it. EnvNode-owned: see LICENSE."""
import argparse
from collections import Counter, defaultdict
import json


def analyze(paths):
    streams = defaultdict(list)
    rain = defaultdict(list)
    bad = 0
    for path in paths:
        with open(path, encoding="utf-8") as source:
            for number, line in enumerate(source, 1):
                if not line.strip():
                    continue
                try:
                    record = json.loads(line)
                    key = (record["device_id"], record["boot_id"], record["type"])
                    sequence = int(record["sequence"])
                    if not 0 <= sequence <= 0xFFFFFFFF:
                        raise ValueError("sequence is not uint32")
                    if not all(isinstance(part, str) and part for part in key):
                        raise ValueError("missing stream identity")
                    if record.get("event") == "rain_window":
                        values = record["values"]
                        if not all(type(values[field]) is int for field in ["revision", "start_us", "end_us"]):
                            raise ValueError("invalid rain integer field")
                        if not isinstance(values["fault"], str) or type(record["source_id"]) is not int:
                            raise ValueError("invalid rain source/fault")
                        rain[key[:2] + (record["source_id"],)].append(values)
                    streams[key].append(sequence)
                except (ValueError, KeyError, TypeError) as error:
                    print(f"{path}:{number}: invalid record: {error}")
                    bad += 1
    for key, sequences in sorted(streams.items()):
        # Order within half the uint32 range, allowing UDP reordering before the first record.
        offsets = sorted({((n - sequences[0] + 0x80000000) & 0xFFFFFFFF) - 0x80000000 for n in sequences})
        gaps = sum(max(0, b - a - 1) for a, b in zip(offsets, offsets[1:]))
        print(json.dumps(dict(stream=key, records=len(sequences), duplicates=len(sequences)-len(offsets),
                              sequence_gaps=gaps), ensure_ascii=False))
    for key, windows in sorted(rain.items()):
        ordered = sorted(windows, key=lambda row: row["end_us"])
        durations = [row["end_us"]-row["start_us"] for row in ordered]
        deltas = [b["end_us"]-a["end_us"] for a, b in zip(ordered, ordered[1:])]
        revision_gaps = resets = duplicate_revisions = 0
        for previous, current in zip(ordered, ordered[1:]):
            delta = (current["revision"] - previous["revision"]) & 0xFFFFFFFF
            if delta == 0:
                duplicate_revisions += 1
            elif delta < 0x80000000:
                revision_gaps += delta - 1
            else:
                resets += 1
        span = ordered[-1]["end_us"] - ordered[0]["end_us"]
        unique_ends = len({row["end_us"] for row in ordered})
        print(json.dumps(dict(rain_source=key, windows=len(windows),
                              revision_gaps=revision_gaps, revision_resets=resets,
                              duplicate_revisions=duplicate_revisions,
                              observed_windows_per_second=(unique_ends-1)*1e6/span if span else None,
                              gate_us_min=min(durations), gate_us_max=max(durations),
                              end_delta_us_max=max(deltas, default=0),
                              faults=dict(Counter(row["fault"] for row in windows))), ensure_ascii=False))
    return 1 if bad else 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("jsonl", nargs="+")
    args = parser.parse_args()
    raise SystemExit(analyze(args.jsonl))
