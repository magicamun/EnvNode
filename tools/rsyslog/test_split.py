#!/usr/bin/env python3
# EnvNode-owned: see repository LICENSE.
"""Integration test on Debian receiver; root, rsyslogd and logrotate required.
Uses only a temporary directory and a loopback UDP listener; no service changes.
"""
import json
import os
from pathlib import Path
import signal
import socket
import subprocess
import tempfile
import time


def wait_for(check):
    for _ in range(100):
        if check():
            return
        time.sleep(0.05)
    raise AssertionError('Timed out waiting for test output')


def main():
    with tempfile.TemporaryDirectory(prefix='envnode-rsyslog-test-') as tmp:
        base = Path(tmp)
        output = base / 'logs'
        snippet = Path(__file__).with_name('10-envnode-json.conf').read_text()
        snippet = snippet.replace('/var/log/envnode', str(output))
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(('127.0.0.1', 0))
        port = sock.getsockname()[1]
        sock.close()
        config = base / 'rsyslog.conf'
        config.write_text('module(load="imudp")\n' + snippet + '\nruleset(name="Test") {\n'
                          f'action(type="omfile" file="{base}/raw.log" template="RSYSLOG_FileFormat")\n'
                          'call EnvNodeJsonSplit\n}\n'
                          f'input(type="imudp" address="127.0.0.1" port="{port}" ruleset="Test")\n')
        subprocess.run(['/usr/sbin/rsyslogd', '-N1', '-f', str(config)], check=True)
        with (base / 'stderr').open('w+') as stderr:
            daemon = subprocess.Popen(['/usr/sbin/rsyslogd', '-n', '-i', str(base / 'pid'), '-f', str(config)], stderr=stderr)
            try:
                time.sleep(0.3)
                assert daemon.poll() is None
                sender = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                device = '000000000001'
                def send(record, app='envnode-diagnostic'):
                    payload = record if isinstance(record, str) else json.dumps(record, ensure_ascii=False, separators=(',', ':'))
                    sender.sendto(f'<134>1 - {device} {app} - - - {payload}'.encode(), ('127.0.0.1', port))
                    return payload
                def record(n, kind='sensor', source=4, event='rain_window', **extra):
                    return dict(device_id=device, type='diagnostic', sequence=n,
                                source_kind=kind, source_id=source, event=event, **extra)
                fixtures = [record(1, value=9007199254740993, text='Grüße "Test"'),
                            record(2, 'controller', 1, 'rain_decision'),
                            record(3, 'system', 0, 'test_marker'),
                            record(4, 'sensor', 2, 'sample'), record(5, 'actuator', 3, 'state'),
                            record(6, 'module', 1, 'state'), record(7, 'system', 0, 'runtime_health'),
                            record(8, source='../escape'), record(9, kind='unknown')]
                payloads = [send(r) for r in fixtures]
                send(dict(device_id=device, type='system', message='boot'), 'envnode-system')
                other = record(10); other['device_id'] = '000000000002'; send(other)
                bad = record(11); bad['device_id'] = '../escape'; send(bad)
                send('{bad json')
                send(record(12), 'unrelated-app')
                d = output / device
                def lines(path):
                    return path.read_text().splitlines() if path.exists() else []
                wait_for(lambda: len(lines(base / 'raw.log')) == 14)
                wait_for(lambda: len(lines(d / 'diagnostics.jsonl')) == 9)
                assert lines(d / 'diagnostics.jsonl') == payloads
                expected = {'rain-detector.jsonl': [1, 2], 'controllers.jsonl': [2],
                            'test-markers.jsonl': [3], 'by-source/sensor-4.jsonl': [1],
                            'by-source/controller-1.jsonl': [2], 'by-source/system-0.jsonl': [3, 7],
                            'by-source/sensor-2.jsonl': [4], 'by-source/actuator-3.jsonl': [5],
                            'by-source/module-1.jsonl': [6]}
                for name, sequences in expected.items():
                    assert [json.loads(s)['sequence'] for s in lines(d / name)] == sequences, name
                assert len(lines(d / 'system.jsonl')) == 1
                assert len(lines(output / '000000000002/diagnostics.jsonl')) == 1
                assert len(lines(output / 'unrouted.log')) == 4
                assert not (base / 'escape').exists()
                rotation = Path(__file__).with_name('envnode.logrotate').read_text()
                rotation = rotation.replace('/var/log/envnode', str(output)).replace(
                    '/usr/bin/systemctl kill --kill-whom=main --signal=HUP rsyslog.service',
                    f'/bin/kill -HUP {daemon.pid}')
                rotate = base / 'logrotate.conf'; rotate.write_text(rotation)
                subprocess.run(['/usr/sbin/logrotate', '--force', '--state', str(base / 'rotate.state'), str(rotate)], check=True)
                assert lines(d / 'diagnostics.jsonl.1') == payloads
                time.sleep(0.2)
                after = send(record(13))
                wait_for(lambda: lines(d / 'diagnostics.jsonl') == [after])
                assert lines(d / 'rain-detector.jsonl') == [after]
                assert lines(d / 'diagnostics.jsonl.1') == payloads
                sender.close()
                print('PASS: routing, device/source isolation, malformed input, original JSON bytes, rotation/reopen')
            finally:
                daemon.send_signal(signal.SIGTERM)
                daemon.wait(timeout=10)


if __name__ == '__main__':
    main()
