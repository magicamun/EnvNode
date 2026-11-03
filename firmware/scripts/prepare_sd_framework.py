# EnvNode: see repository LICENSE.
Import("env")

from pathlib import Path
import sys

sys.path.insert(0, str(Path(env.subst("$PROJECT_DIR")) / "scripts"))
from sd_framework_patch import prepare_library

framework = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
if not framework:
    raise RuntimeError("SD patch: Arduino-ESP32 framework package is missing")
libraries = Path(env.subst("$BUILD_DIR")) / "patched-libraries"
prepare_library(framework, libraries / "SD")
# Project-local library search precedes framework libraries. Both SD.h users and
# direct sd_diskio.h users therefore compile/link the same corrected driver.
env.Prepend(LIBSOURCE_DIRS=[str(libraries)])
print("SD: using project-local Arduino 2.0.17 transaction lifetime fix")
