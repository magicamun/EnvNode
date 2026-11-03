"""Firmware publication policy tests. See repository LICENSE."""
import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import firmware_downloads as downloads


class PublicationPolicyTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        header = self.root / 'firmware/include/FirmwareVersion.h'
        header.parent.mkdir(parents=True)
        header.write_text('constexpr const char* FirmwareVersion = "1.2.3";')
        self.root_patch = patch.object(downloads, 'ROOT', self.root)
        self.root_patch.start()
        self.addCleanup(self.root_patch.stop)
        self.calls = []
        self.releases = []

    def run_command(self, *args):
        self.calls.append(args)
        if args[:2] == ('gh', 'api'):
            return json.dumps([self.releases])
        if args[:2] == ('git', 'rev-parse'):
            return 'a' * 40
        return ''

    def prepare_build(self):
        notes = self.root / 'docs/firmware-releases/envnode/1.2.3.md'
        notes.parent.mkdir(parents=True)
        notes.write_text('Wesentliche Änderungen')
        image = self.root / 'firmware/bin/envnode-1.2.3.bin'
        image.parent.mkdir(parents=True)
        image.write_bytes(b'firmware')
        (self.root / 'LICENSE').write_text('License notice')

    def publish(self):
        with patch.object(downloads, 'run', side_effect=self.run_command):
            with contextlib.redirect_stdout(io.StringIO()):
                downloads.publish('envnode')

    def test_published_version_is_untouched_even_without_local_image_or_notes(self):
        self.releases = [{'tag_name': 'firmware-envnode-1.2.3', 'draft': False}]
        self.publish()
        self.assertEqual(len(self.calls), 1)  # Only the read-only release listing.

    def test_new_version_is_hidden_until_assets_are_uploaded(self):
        self.prepare_build()
        self.publish()
        mutations = [c for c in self.calls if c[:2] == ('gh', 'release')]
        self.assertEqual([c[2] for c in mutations], ['create', 'upload', 'edit'])
        self.assertIn('--draft', mutations[0])
        self.assertIn('--draft=false', mutations[-1])

    def test_interrupted_draft_can_resume(self):
        self.prepare_build()
        self.releases = [{'tag_name': 'firmware-envnode-1.2.3', 'draft': True}]
        self.publish()
        mutations = [c for c in self.calls if c[:2] == ('gh', 'release')]
        self.assertNotIn('create', [c[2] for c in mutations])
        self.assertEqual(mutations[-1][2], 'edit')
        self.assertIn('--draft=false', mutations[-1])


if __name__ == '__main__':
    unittest.main()
