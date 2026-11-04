# Branch consolidation — 2026-11-04

EnvNode-owned: see repository LICENSE.

The validated classic ESP32 baseline was consolidated into main from
codex/esp32-rain-integration (8d2bcf1). This fast-forward includes the RainDetector
merge, UDP diagnostics, SD removal (e39d774), current documentation and hardware
validation evidence. The firmware source is unchanged by this consolidation;
validated hardware build 0.8.7+474.ge39d774.dirty remains the reference image.
The old codex/rain-detector head e3ecde6 is already an ancestor of that baseline.
RainDetector therefore receives the network logger by fast-forwarding to main,
not by implementing or cherry-picking a second transport.

## Continuing local branches/worktrees

| Branch | Purpose after consolidation |
| --- | --- |
| main | Shared classic ESP32 baseline; next web-response work starts here. |
| codex/rain-detector | Same baseline, ready for later head/controller validation. |
| feature/analog-hydro-pressure | Preserve the separate committed change and ongoing Analog/SCT013 hardware/documentation work. |
| SX1262-remote | Preserve ongoing KiCad work; firmware integration remains deferred to S3. |
| feature/test-provision-board | Its committed software is integrated; preserve local adapter/fixture/test-controller hardware work. |

The three hardware worktrees are intentionally retained with their existing
uncommitted files. They are not rebased or automatically committed/published.
Main's existing derived-measurements documentation edit is preserved byte for
byte. The user's local firmware/firmware.bin deletion is transferred from the
closed integration worktree to main, and remains uncommitted. Neither user
change is included in the consolidation documentation commit.

## Archived and completed work

| Checkpoint tag | Preserved head |
| --- | --- |
| milestone/classic-esp32-udp-2026-11-04 | 8d2bcf1, complete tested baseline and validation ledger |
| archive/2026-11-04/esp32-rain-integration | 8d2bcf1, closed integration topic |
| archive/2026-11-04/sd-card | 8ea0a42, final old SD topic |
| archive/2026-11-04/platformio-8-port | cc3e6db, paused framework migration for later S3 work |
| archive/2026-11-04/rain-detector-before-consolidation | e3ecde6, previous RainDetector topic head |

The integration, SD and paused framework worktrees are removed after verified
backups. Their local topic branch refs are closed; the archived commits remain
reachable through tags. The fully merged local branches codex/fix-timezone-doc-link,
codex/geoposition-sea-level-pressure and codex/localized-location-input are also
removed. Their history remains in main and the pre-consolidation Git bundle.
The new Arduino framework is not merged into the classic ESP32 baseline.

## Local recovery and artifacts

A private sibling archive, `_archive/consolidation-2026-11-04`, contains a verified
all-ref Git bundle, pre-operation heads/status, separate binary index/worktree
patches and verified changed/untracked-file archives for all eight former
worktrees. Closing worktrees additionally preserve ignored files except explicitly
listed regenerable build/dependency/Python caches. A NAS-busy residual
integration directory was moved into that private archive instead of forced
deletion; it has no Git registration and is not a development checkout. Do not publish that archive:
it includes private device/NVS backups and captured diagnostics.

Continuing copies of firmware packages are under main/firmware/bin/; baseline
ELFs/maps, resource evidence and private flash/log backups are under
main/firmware/.pio/logging-validation/. Both directories remain Git-ignored.
Previously existing main artifacts that needed replacement were also saved in
the private consolidation archive. SHA-256 manifests record the transfers.

No firmware flash, server configuration or firewall change is part of this
consolidation. Web latency, JSON graphical analysis and further RainDetector
hardware tests remain separate subsequent tasks.
