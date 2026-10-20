#!/usr/bin/env python3
# EnvNode: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
"""Offline, standard-library analysis; no firmware dependency or device access."""
import argparse
from collections import defaultdict
import csv
from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import re
import statistics

PARAMETERS = dict(bin_ms=1000, absolute_drop_hz=250, relative_drop=0.01,
                  guard_ms=10000, refractory_ms=15000, match_tolerance_ms=10000,
                  recovery_hz_s=3.0, drift_hz_s=1.5)

@dataclass
class Sample:
    time_ms: int
    frequency_hz: float
    heater_percent: int

@dataclass
class Session:
    samples: list
    heaters: list
    errors: list
    ignored: int = 0


def parse_log(path, raw_text=None):
    """A boot marker supersedes preamble; real resets create separate sessions."""
    sessions = []
    current = Session([], [], [])
    saw_boot = False
    text = Path(path).read_text(errors='replace') if raw_text is None else raw_text
    for raw in text.splitlines():
        line = raw.strip()
        if line.startswith('# heater,'):
            fields = line.split(',')
            try:
                event = dict(time_ms=int(fields[1]), percent=int(fields[2]),
                             remaining_ms=int(fields[3]), reason=fields[4])
            except (ValueError, IndexError):
                current.ignored += 1
                continue
            if event['reason'] == 'boot_off':
                if saw_boot and current.samples:
                    sessions.append(current)
                # Initial boot discards cached samples/partial ROM preamble.
                current = Session([], [], [])
                saw_boot = True
            current.heaters.append(event)
            continue
        if line.startswith('# error,'):
            current.errors.append(line)
            continue
        match = re.fullmatch(r'(\d+),(\d+),([\d.]+|nan),(\d+)', line)
        if not match:
            if line:
                current.ignored += 1
            continue
        t, frequency, level = int(match[1]), float(match[3]), int(match[4])
        if not 0 <= level <= 100:
            current.ignored += 1
            continue
        if current.samples and t < current.samples[-1].time_ms:
            sessions.append(current)
            current = Session([], [], [])
        if current.samples and t == current.samples[-1].time_ms:
            current.ignored += 1
            continue
        current.samples.append(Sample(t, frequency, level))
    if current.samples:
        sessions.append(current)
    return sessions


def bins_for(session, bin_ms):
    groups = defaultdict(list)
    for sample in session.samples:
        groups[sample.time_ms // bin_ms].append(sample)
    result = {}
    for key, samples in groups.items():
        levels = {s.heater_percent for s in samples}
        finite = [s.frequency_hz for s in samples if math.isfinite(s.frequency_hz) and s.frequency_hz > 0]
        # Incomplete, mixed-heater and invalid windows cannot imply a transition.
        if len(samples) >= 2 and len(finite) == len(samples) and len(levels) == 1:
            result[key] = (statistics.median(finite), next(iter(levels)))
    return result


def heater_changes(session):
    changes = [e['time_ms'] for e in session.heaters
               if e['reason'] in ('command', 'off', 'timeout', 'boot_off')]
    for left, right in zip(session.samples, session.samples[1:]):
        if left.heater_percent != right.heater_percent:
            changes.append(right.time_ms)
    return sorted(set(changes))


def detect_drops(session, params=PARAMETERS):
    bins = bins_for(session, params['bin_ms'])
    changes = heater_changes(session)
    detections = []
    last_event = -10**15
    for key in sorted(bins):
        time_ms = (key + 1) * params['bin_ms']
        # Four preceding seconds form the local baseline; one current second
        # confirms a sustained drop. The intermediate second is left as transition.
        keys = list(range(key - 5, key + 1))
        if any(k not in bins for k in keys):
            continue
        levels = {bins[k][1] for k in keys}
        if len(levels) != 1 or next(iter(levels)) not in (0, 100):
            continue
        if any(time_ms - params['guard_ms'] <= t <= time_ms for t in changes):
            continue
        if time_ms - last_event < params['refractory_ms']:
            continue
        baseline = statistics.median(bins[k][0] for k in range(key - 5, key - 1))
        current = bins[key][0]
        threshold = max(params['absolute_drop_hz'], baseline * params['relative_drop'])
        if baseline > 0 and baseline - current >= threshold:
            detections.append(dict(time_ms=time_ms, baseline_hz=round(baseline, 3),
                                   current_hz=round(current, 3),
                                   drop_hz=round(baseline-current, 3)))
            last_event = time_ms
    return detections


def trends(session, params=PARAMETERS):
    bins = bins_for(session, params['bin_ms'])
    changes = heater_changes(session)
    result = []
    # Non-overlapping ten-second blocks; these are descriptive, not weather states.
    for start in range(min(bins, default=0), max(bins, default=-1), 10):
        keys = list(range(start, start + 10))
        if any(k not in bins for k in keys):
            continue
        levels = {bins[k][1] for k in keys}
        end_ms = (start + 10) * params['bin_ms']
        level = next(iter(levels)) if len(levels) == 1 else -1
        guarded = any(start*params['bin_ms']-params['guard_ms'] <= t <= end_ms
                      for t in changes)
        if level not in (0, 100) or guarded:
            label, slope = 'nicht auswertbar', None
        else:
            values = [bins[k][0] for k in keys]
            slope = sum((i-4.5)*(v-statistics.mean(values))
                        for i, v in enumerate(values)) / 82.5
            if slope >= params['recovery_hz_s']:
                label = 'Erholung'
            elif abs(slope) <= params['drift_hz_s']:
                label = 'stabil / langsame Drift'
            else:
                label = 'unklar'
        result.append(dict(start_ms=start*params['bin_ms'], end_ms=end_ms,
                           label=label, slope_hz_s=round(slope, 3) if slope is not None else None))
    return result


def match_annotations(detections, observations, tolerance):
    """One-to-one matching; unlabelled detections are not automatically false."""
    used = set()
    matched = []
    for observation in observations:
        item = dict(observation)
        if item['kind'] == 'wetting':
            candidates = [(abs(d['time_ms']-item['time_ms']), i)
                          for i, d in enumerate(detections) if i not in used
                          and abs(d['time_ms']-item['time_ms']) <= tolerance]
            index = min(candidates)[1] if candidates else None
            item['detection_ms'] = detections[index]['time_ms'] if index is not None else None
            if index is not None:
                used.add(index)
        matched.append(item)
    return matched, [d for i, d in enumerate(detections) if i not in used]


def downsample(samples, width_ms=1000):
    """Keep extrema and endpoints, including gaps; never average away a dip."""
    groups = defaultdict(list)
    for sample in samples:
        groups[sample.time_ms // width_ms].append(sample)
    points = []
    for group in groups.values():
        finite = [s for s in group if math.isfinite(s.frequency_hz) and s.frequency_hz > 0]
        if not finite:
            continue
        selected = {s.time_ms:s for s in [finite[0], min(finite,key=lambda x:x.frequency_hz),
                                         max(finite,key=lambda x:x.frequency_hz),finite[-1]]}
        points.extend([s.time_ms,round(s.frequency_hz,2),s.heater_percent]
                      for s in sorted(selected.values(),key=lambda x:x.time_ms))
    return points


def analyze(paths, annotations):
    analyses = []
    for path in paths:
        meta = annotations.get('logs', {}).get(path.name, {})
        snapshot = path.read_bytes()
        for index, session in enumerate(parse_log(path, snapshot.decode(errors='replace'))):
            detections = detect_drops(session)
            observations = [o for o in meta.get('observations', []) if o.get('session',0)==index]
            matched, extra = match_annotations(detections, observations, PARAMETERS['match_tolerance_ms'])
            analyses.append(dict(name=path.name, session=index,
                role=meta.get('role','unannotated'), sha256=hashlib.sha256(snapshot).hexdigest(),
                samples=len(session.samples), ignored=session.ignored, errors=session.errors,
                heaters=session.heaters, detections=detections, observations=matched,
                unassigned=extra, trends=trends(session), points=downsample(session.samples)))
    return analyses


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('logs', nargs='+', type=Path)
    parser.add_argument('--annotations', type=Path, default=Path(__file__).with_name('observations.json'))
    parser.add_argument('--output', type=Path, default=Path('logs/analysis/offline'))
    args = parser.parse_args()
    args.logs = list(dict.fromkeys(args.logs))
    annotations = json.loads(args.annotations.read_text())
    missing = [p for p in args.logs if not p.is_file()]
    if missing:
        parser.error('Missing log(s): '+', '.join(map(str, missing)))
    analyses = analyze(args.logs, annotations)
    if not analyses:
        parser.error('No measurements found')
    args.output.mkdir(parents=True, exist_ok=True)
    payload = dict(parameters=PARAMETERS, annotation_notes=annotations['notes'], logs=analyses)
    (args.output/'results.json').write_text(json.dumps(payload, ensure_ascii=False, indent=2)+'\n')
    with (args.output/'events.csv').open('w', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(['log','session','role','time_ms','baseline_hz','current_hz','drop_hz'])
        for log in analyses:
            for d in log['detections']:
                writer.writerow([log['name'],log['session'],log['role'],d['time_ms'],d['baseline_hz'],d['current_hz'],d['drop_hz']])
    template = Path(__file__).with_name('report.html').read_text()
    embedded = json.dumps(payload, ensure_ascii=False, separators=(',',':')).replace('<','\\u003c')
    (args.output/'report.html').write_text(template.replace('__DATA__',embedded))
    report = ['# Offline-Prüfung RainDetector','',annotations['notes'],'',
              'Parameter: `'+json.dumps(PARAMETERS,sort_keys=True)+'`','',
              '| Log / Sitzung | Rolle | Benetzungsmarken gefunden | Nicht zugeordnete Kandidaten |',
              '| --- | --- | ---: | ---: |']
    for log in analyses:
        wet = [o for o in log['observations'] if o['kind']=='wetting']
        hits = sum(o['detection_ms'] is not None for o in wet)
        report.append(f"| {log['name']} / {log['session']} | {log['role']} | {hits}/{len(wet)} | {len(log['unassigned'])} |")
    report += ['', 'Nicht zugeordnet heißt nicht automatisch Fehlalarm: Beobachtungsmarken sind unvollständig. '
               'Übereinstimmung mit groben Zeitmarken ist keine Genauigkeitsgarantie. '
               'Stabil bedeutet nicht trocken; Erholung bedeutet nicht Regenende. '
               'Keine Schwellwerte aus den Ergebnissen automatisch nachführen.', '']
    (args.output/'summary.md').write_text('\n'.join(report))
    print('\n'.join(report))
    print('Report:', (args.output/'report.html').resolve())

if __name__ == '__main__':
    main()
