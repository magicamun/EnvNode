#!/usr/bin/env python3
# EnvNode-owned: see repository LICENSE.
import contextlib
import io
import json
import tempfile
import unittest

from analyze_udp_logging import analyze


class ReceiverAnalysisTests(unittest.TestCase):
    def run_records(self, records):
        with tempfile.NamedTemporaryFile(mode="w+", suffix=".jsonl") as source:
            for record in records:
                source.write(json.dumps(record) + "\n")
            source.flush()
            result = io.StringIO()
            with contextlib.redirect_stdout(result):
                code = analyze([source.name])
            return code, [json.loads(line) for line in result.getvalue().splitlines()]

    def record(self, sequence, boot="boot"):
        return dict(device_id="device", boot_id=boot, type="diagnostic", sequence=sequence)

    def test_wrap_and_reordered_first_packet_with_duplicate_and_one_gap(self):
        code, output = self.run_records([self.record(n) for n in [0, 0xFFFFFFFF, 2, 2]])
        self.assertEqual(code, 0)
        self.assertEqual(output[0]["sequence_gaps"], 1)
        self.assertEqual(output[0]["duplicates"], 1)

    def test_empty_capture_is_a_failed_receipt_check(self):
        with tempfile.NamedTemporaryFile(mode="w+", suffix=".jsonl") as source:
            result = io.StringIO()
            with contextlib.redirect_stdout(result):
                code = analyze([source.name])
            self.assertEqual(code, 1)
            self.assertIn("receipt is not validated", result.getvalue())

    def test_boots_are_separate_and_missing_prefix_is_not_invented(self):
        code, output = self.run_records([self.record(40), self.record(42), self.record(1, "other")])
        self.assertEqual(code, 0)
        self.assertEqual([row["sequence_gaps"] for row in output], [1, 0])

    def test_full_rate_windows_and_lost_revision_remain_distinct(self):
        records = []
        for sequence, revision in [(1, 100), (2, 101), (3, 103)]:
            record = self.record(sequence)
            record.update(event="rain_window", source_id=4, values=dict(
                revision=revision, start_us=revision*250000,
                end_us=(revision+1)*250000, fault="none"))
            records.append(record)
        code, output = self.run_records(records)
        self.assertEqual(code, 0)
        self.assertEqual(output[0]["sequence_gaps"], 0)
        self.assertEqual(output[1]["revision_gaps"], 1)
        self.assertEqual(output[1]["gate_us_max"], 250000)
        self.assertEqual(output[1]["faults"], {"none": 3})


if __name__ == "__main__":
    unittest.main()
