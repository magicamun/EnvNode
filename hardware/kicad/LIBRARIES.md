# KiCad project library paths

EnvNode-owned documentation: LicenseRef-EnvNode-Use-and-Redistribution-1.0;
see the repository root LICENSE. Third-party libraries retain their own licenses.

Every KiCad project, including module templates and archived Revision-0.1
projects, registers the shared repository libraries in its own `sym-lib-table`
and `fp-lib-table`:

- `EnvNode-Symbols` → `Libraries/EnvNode-Symbols.kicad_sym`
- `EnvNode-Footprints` → `Libraries/EnvNode-Footprints.pretty`

Paths start with `${KIPRJMOD}` and are relative to the project directory. This
keeps each worktree on its own library version without machine-specific paths.
For example, modules in `Modules/FullSize/<project>` or
`Modules/HalfSize/<project>` use `${KIPRJMOD}/../../../Libraries/...`, whereas
projects in `Sensors/<project>` use `${KIPRJMOD}/../../Libraries/...`.

Copy both library-table files when creating a project from a template. If the
new project changes directory depth, adjust the relative paths. Commit the
library tables with the project. Library changes travel between branches by
normal commits/merges; do not point at another worktree's library directory.

Remove global entries for these repository library nicknames from the local
KiCad configuration. Keep KiCad standard libraries and separately installed
third-party libraries (such as `PCM_Espressif`) configured globally. This change
registers the repository-owned libraries; it does not vendor all external
libraries or make existing projects independent of their external dependencies.

Additional module-specific libraries should be registered locally only in the
projects that need them. For example, SD-Card registers `Adafruit-Breakouts`
for both symbols and footprints; those files retain Adafruit's license.

The mainboard, module and sensor designs are unchanged by this configuration.
