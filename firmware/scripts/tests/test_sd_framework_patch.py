# EnvNode: see repository LICENSE.
"""Run after installing Arduino 2.0.17; requires clang++ with AddressSanitizer."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from sd_framework_patch import patch_source, prepare_library, uninit_function


FRAMEWORK = Path(os.environ.get("ENVNODE_ARDUINO_FRAMEWORK",
    str(Path.home() / ".platformio/packages/framework-arduinoespressif32")))


class SdFrameworkPatchTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source_path = FRAMEWORK / "libraries/SD/src/sd_diskio.cpp"
        cls.source = cls.source_path.read_text()

    def test_repeated_patch_is_identical_and_other_source_is_preserved(self):
        patched = patch_source(self.source)
        self.assertEqual(patch_source(patched), patched)
        old, new = uninit_function(self.source), uninit_function(patched)
        self.assertEqual(self.source[:old.start()], patched[:new.start()])
        self.assertEqual(self.source[old.end():], patched[new.end():])

    def test_unexpected_framework_source_fails_closed(self):
        altered = self.source.replace("AcquireSPI lock(card);", "AcquireSPI changed(card);")
        with self.assertRaisesRegex(RuntimeError, "unknown sdcard_uninit"):
            patch_source(altered)

    def test_prepare_preserves_installed_package_and_all_driver_sources(self):
        before = self.source_path.read_bytes()
        with tempfile.TemporaryDirectory() as directory:
            output = prepare_library(FRAMEWORK, Path(directory) / "SD")
            self.assertEqual((output / "src/sd_diskio.cpp").read_text(), patch_source(self.source))
            for source in (FRAMEWORK / "libraries/SD/src").rglob("*"):
                if source.is_file() and source != self.source_path:
                    self.assertEqual(source.read_bytes(), (output / "src" / source.relative_to(
                        FRAMEWORK / "libraries/SD/src")).read_bytes())
            first = (output / "src/sd_diskio.cpp").stat().st_mtime_ns
            prepare_library(FRAMEWORK, output)
            self.assertEqual(first, (output / "src/sd_diskio.cpp").stat().st_mtime_ns)
        self.assertEqual(before, self.source_path.read_bytes())

    def test_original_use_after_free_and_corrected_lifetime_under_asan(self):
        compiler = shutil.which("clang++")
        self.assertIsNotNone(compiler, "clang++ required for the SD lifetime regression")
        harness = r"""
#include <cstdlib>
#include <cstdint>
struct Spi { int ended=0; void endTransaction() { ++ended; } };
struct ardu_sdcard_t { Spi* spi; char* base_path; };
constexpr unsigned FF_VOLUMES=1;
constexpr int ESP_OK=0, GO_IDLE_STATE=0;
using esp_err_t=int;
ardu_sdcard_t* s_cards[FF_VOLUMES];
struct AcquireSPI {
    ardu_sdcard_t* card;
    explicit AcquireSPI(ardu_sdcard_t* value):card(value) {}
    ~AcquireSPI() { card->spi->endTransaction(); }
};
void sdTransaction(uint8_t,int,int,void*) {}
void ff_diskio_register(uint8_t,void*) {}
int esp_vfs_fat_unregister_path(const char*) { return 0; }
FUNCTION
int main() {
    Spi spi;
    s_cards[0]=static_cast<ardu_sdcard_t*>(malloc(sizeof(ardu_sdcard_t)));
    s_cards[0]->spi=&spi; s_cards[0]->base_path=nullptr;
    if(sdcard_uninit(0)!=0 || spi.ended!=1 || s_cards[0]!=nullptr) return 1;
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            for label, source, expected in (("original", self.source, False),
                    ("patched", patch_source(self.source), True)):
                path = Path(directory) / (label + ".cpp")
                binary = Path(directory) / label
                path.write_text(harness.replace("FUNCTION", uninit_function(source).group()))
                subprocess.run([compiler, "-std=c++11", "-O1", "-g", "-fsanitize=address",
                    "-fno-omit-frame-pointer", str(path), "-o", str(binary)], check=True,
                    capture_output=True, text=True)
                result = subprocess.run([str(binary)], capture_output=True, text=True,
                    env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"))
                if expected:
                    self.assertEqual(result.returncode, 0, result.stderr)
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("heap-use-after-free", result.stderr)


if __name__ == "__main__":
    unittest.main()
