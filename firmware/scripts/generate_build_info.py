Import("env")

from datetime import datetime, timezone
import json
from pathlib import Path
import re
import subprocess


project_dir = Path(env.subst("$PROJECT_DIR"))
generated_dir = Path(env.subst("$BUILD_DIR")) / "generated"
generated_header = generated_dir / "FirmwareBuildInfo.h"
version_header = project_dir / "include" / "FirmwareVersion.h"


def git_output(*arguments):
    try:
        result = subprocess.run(
            ["git", *arguments],
            cwd=str(project_dir),
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )
        return result.stdout.strip()
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None


def semantic_version():
    try:
        contents = version_header.read_text(encoding="utf-8")
    except OSError:
        return "unknown"
    match = re.search(r'FirmwareVersion\s*=\s*"([^"]+)"', contents)
    return match.group(1) if match else "unknown"


version = semantic_version()
commit = git_output("rev-parse", "--short=7", "HEAD") or "unknown"
build_number = git_output("rev-list", "--count", "HEAD") or "unknown"
branch = git_output("symbolic-ref", "--short", "-q", "HEAD")
if not branch:
    branch = "detached" if commit != "unknown" else "unknown"
status = git_output(
    "status",
    "--porcelain",
    "--untracked-files=normal",
    "--",
    ".",
    ":(exclude).pio/**",
)
source_state = "unknown" if status is None else ("dirty" if status else "clean")
built_at = datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")

compact_identity = version
if build_number != "unknown" and commit != "unknown":
    compact_identity += "+{}.g{}".format(build_number, commit)
else:
    compact_identity += "+unknown"
if source_state == "dirty":
    compact_identity += ".dirty"


def cpp_string(value):
    return json.dumps(value, ensure_ascii=True)


header = """#pragma once

#include \"FirmwareVersion.h\"

namespace EnvNode {
namespace FirmwareBuildInfo {

constexpr const char* SemanticVersion = FirmwareVersion;
constexpr const char* BuildNumber = %s;
constexpr const char* GitCommit = %s;
constexpr const char* GitBranch = %s;
constexpr const char* SourceState = %s;
constexpr const char* BuildTimestampUtc = %s;
constexpr const char* CompactIdentity = %s;
constexpr bool SourceDirty = %s;

} // namespace FirmwareBuildInfo
} // namespace EnvNode
""" % (
    cpp_string(build_number),
    cpp_string(commit),
    cpp_string(branch),
    cpp_string(source_state),
    cpp_string(built_at),
    cpp_string(compact_identity),
    "true" if source_state == "dirty" else "false",
)

generated_dir.mkdir(parents=True, exist_ok=True)
if not generated_header.exists() or generated_header.read_text(encoding="utf-8") != header:
    generated_header.write_text(header, encoding="utf-8")

env.Append(CPPPATH=[str(generated_dir)])
