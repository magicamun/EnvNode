# EnvNode: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('rain_analysis', Path(__file__).resolve().parents[1]/'tools/analyze_logs.py')
analysis = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = analysis
spec.loader.exec_module(analysis)


def session(function, levels=lambda t:0, seconds=40):
    return analysis.Session([analysis.Sample(t, function(t), levels(t))
                             for t in range(250, seconds*1000,250)], [], [])


class AnalysisTests(unittest.TestCase):
    def test_new_wetting_on_already_wet_surface(self):
        log = session(lambda t: 20000 if t < 20000 else 18000)
        events = analysis.detect_drops(log)
        self.assertEqual(len(events),1)
        self.assertTrue(20000 <= events[0]['time_ms'] <= 22000)

    def test_monotonic_drift_and_recovery_are_not_new_water(self):
        for slope in (-2,2,30):
            self.assertEqual(analysis.detect_drops(session(lambda t:34000+slope*t/1000)),[])

    def test_heater_step_and_partial_pwm_are_excluded(self):
        log = session(lambda t:34000 if t<20000 else 33000,
                      levels=lambda t:0 if t<20000 else 100)
        self.assertEqual(analysis.detect_drops(log),[])
        pwm = session(lambda t:34000 if t<20000 else 30000,levels=lambda t:50)
        self.assertEqual(analysis.detect_drops(pwm),[])

    def test_no_interpolation_through_gaps_or_invalid_samples(self):
        log = session(lambda t:34000 if t<20000 else 30000)
        log.samples=[s for s in log.samples if not 18000<=s.time_ms<=23000]
        self.assertEqual(analysis.detect_drops(log),[])
        invalid = session(lambda t:34000 if t<20000 else float('nan'))
        self.assertEqual(analysis.detect_drops(invalid),[])
        missing = session(lambda t:34000 if t<20000 else 0)
        self.assertEqual(analysis.detect_drops(missing),[])

    def test_boot_preamble_duplicates_and_reset(self):
        text='99999,10,30000,0\n99999,10,30000,0\n# heater,53,0,0,boot_off\n250,10,34000,0\n250,10,34000,0\n500,10,34000,0\n# heater,53,0,0,boot_off\n250,10,33000,0\n'
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'test.log';path.write_text(text)
            sessions=analysis.parse_log(path)
        self.assertEqual(len(sessions),2)
        self.assertEqual([s.time_ms for s in sessions[0].samples],[250,500])
        self.assertEqual(sessions[1].samples[0].frequency_hz,33000)

    def test_annotations_match_only_once_and_do_not_label_everything_false(self):
        observations=[dict(time_ms=20000,kind='wetting'),dict(time_ms=21000,kind='wetting'),
                      dict(time_ms=30000,kind='handling')]
        matched,unassigned=analysis.match_annotations([dict(time_ms=20500),dict(time_ms=60000)],observations,10000)
        self.assertEqual(matched[0]['detection_ms'],20500)
        self.assertIsNone(matched[1]['detection_ms'])
        self.assertEqual(unassigned,[dict(time_ms=60000)])

    def test_minmax_sampling_retains_brief_excursion(self):
        log=session(lambda t:10000 if t==20250 else 34000)
        sampled=analysis.downsample(log.samples)
        self.assertIn(10000,[p[1] for p in sampled])

class AutoLogTests(unittest.TestCase):
    def test_auto_records_follow_boot_sessions(self):
        text = """# heater,53,0,0,boot_off
# auto,100,enabled,1,0,unknown,0,0,0,0,0
1000,100,35000,0
# auto,1200,wetting,1,1,active,1,1,0,0,0
# auto,61200,rain_end,1,1,end_estimated,0,1,0,60000,0
# auto,90000,dry,1,1,end_estimated,0,0,1,60000,10000
# auto,bad
# heater,53,0,0,boot_off
1000,100,35000,0
# auto,1000,boot_disabled,0,0,unknown,0,0,0,0,0
"""
        sessions = analysis.parse_log(Path('example.log'), text)
        self.assertEqual(len(sessions), 2)
        self.assertEqual([e['reason'] for e in sessions[0].automation],
                         ['enabled', 'wetting', 'rain_end', 'dry'])
        self.assertEqual(sessions[0].automation[-1]['dry_ms'], 10000)
        self.assertEqual(sessions[0].ignored, 1)
        self.assertEqual(len(sessions[1].automation), 1)

if __name__=='__main__':
    unittest.main()
