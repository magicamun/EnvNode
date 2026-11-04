#!/usr/bin/env python3
# EnvNode: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
"""Replay the exact C++ evaluation core used by the lab firmware; no device access."""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tempfile

from analyze_logs import parse_log, match_annotations


def main():
    cli=argparse.ArgumentParser(description=__doc__)
    cli.add_argument('logs',nargs='+',type=Path)
    cli.add_argument('--output',type=Path,required=True)
    args=cli.parse_args()
    tools=Path(__file__).resolve().parent
    root=tools.parents[2]
    observations=json.loads((tools/'observations.json').read_text())['logs']
    args.output.mkdir(parents=True,exist_ok=True)
    summaries=[]
    with tempfile.TemporaryDirectory(prefix='envnode-rain-core-') as folder:
        binary=Path(folder)/'replay'
        subprocess.run(['c++','-std=c++11','-O2','-Wall','-Wextra','-Werror',
                        '-I',str(root/'firmware/lib/RainDetectorEvaluation/src'),
                        str(tools/'replay_core.cpp'),'-o',str(binary)],check=True)
        for path in dict.fromkeys(args.logs):
            snapshot=path.read_bytes()
            for index,session in enumerate(parse_log(path,snapshot.decode(errors='replace'))):
                input_text=''.join(f'{r.time_ms},{r.frequency_hz},{r.heater_percent}\n' for r in session.samples)
                replay=subprocess.run([str(binary)],input=input_text,text=True,capture_output=True,check=True)
                columns=['time_ms','fast_hz','slow_hz','trend_hz_s','valid','estimate','new_wetting']
                output=args.output/f'{path.stem}-session-{index}-core.csv'
                output.write_text(','.join(columns)+'\n'+replay.stdout)
                rows=list(csv.DictReader(io.StringIO(replay.stdout),fieldnames=columns))
                detections=[dict(time_ms=int(r['time_ms'])) for r in rows if r['new_wetting']=='1']
                annotations=[o for o in observations.get(path.name,{}).get('observations',[]) if o.get('session',0)==index]
                matched,extra=match_annotations(detections,annotations,10000)
                transitions=[];previous='unknown'
                for row in rows:
                    if row['estimate']!=previous:
                        transitions.append(dict(time_ms=int(row['time_ms']),estimate=row['estimate']))
                        previous=row['estimate']
                summaries.append(dict(log=path.name,session=index,sha256=hashlib.sha256(snapshot).hexdigest(),
                    memory=replay.stderr.strip(),detections=detections,transitions=transitions,
                    observations=matched,unassigned=extra))
    result=dict(core_sha256=hashlib.sha256((root/'firmware/lib/RainDetectorEvaluation/src/RainDetectorEvaluation.h').read_bytes()).hexdigest(),
                note='Explorative neue Filterversion gegen bekannte Daten. Kein unabhängiger Blindtest. C++-Host-Replay; ESP32-Build und Hardwareversuch separat.',logs=summaries)
    (args.output/'core-results.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    lines=['# Lokaler RainDetector-Kern: Replay','',result['note'],'',
           '| Log | Ereigniskandidaten | Notierte Benetzungen gefunden | Nicht zugeordnet | Zustandswechsel (s seit Boot) |',
           '| --- | ---: | ---: | ---: | --- |']
    for log in summaries:
        wet=[o for o in log['observations'] if o['kind']=='wetting'];hits=sum(o['detection_ms'] is not None for o in wet)
        changes='; '.join(f"{e['time_ms']/1000:.1f}: {e['estimate']}" for e in log['transitions'])
        lines.append(f"| {log['log']} | {len(log['detections'])} | {hits}/{len(wet)} | {len(log['unassigned'])} | {changes or 'unknown'} |")
    lines+=['','Zusätzliche Kandidaten sind keine automatisch bestätigten Fehlalarme. '
            'Regenende ist eine unvalidierte Schätzung, kein Trockenheitsnachweis. '
            'Die feste Speichermenge pro Instanz wird im JSON als Host-Größe dokumentiert; '
            'der tatsächliche ESP32-Wert wird beim Boot ausgegeben.','']
    (args.output/'core-summary.md').write_text('\n'.join(lines))
    print('\n'.join(lines))

if __name__=='__main__':main()
