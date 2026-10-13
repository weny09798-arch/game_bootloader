# Final documentation fix report

Added a historical-checkpoint note to `core-report.md` clarifying that its invalid-candidate diagnostic statement describes the original implementation, and linking to the current `ota_recover` behavior in `protocol.md` and `integration-fix-report.md`.

Reviewed `protocol.md`'s `INSTALLING` recovery section: it distinguishes unreadable staging from confirmed invalid source; retains `INSTALLING` on source read/flash/copy verification failures; and, only after source validation fails, validates old A, persists `VALID` or `EMPTY` with `request=1` before accepting a new `BEGIN`. This matches the historical note's description of durable invalid-source recovery.

Verification: `git diff --check` passed. No firmware, host code, or other documentation was changed; no tests were rerun.
