# Integrated corrections: verification record

The previous implementation agent's edits were recovered from the working tree and independently checked by the coordinator on 2026-10-13. Its final report was not present; this record states observed evidence rather than claiming an agent report.

Changes include actual App_Poll binary-burst quarantine until idle; a portable edge-restarting debounce helper; ota_recover for a persistently invalid staging candidate while protecting valid pending candidates; and explicit bounded GUI transport cleanup before process exit. Regression cases are in tests/native/main.c, tests/native/app_port_test.c, tests/native/debounce_test.c and tests/test_host_frontends.py. Common C and native tests were mechanically formatted for readability.

Fresh command: `python tools/verify.py`, exit 0.

- Core: 199984 checks, 0 failed tests.
- Recovery journal transition cuts: 32 with a valid old app and 32 with an invalid old app.
- Receive cuts: 12034; copy cuts: 12003; journal rollover cuts: 544 in each direction.
- Actual application port isolation/request test: PASS.
- Actual debounce helper: 23 checks PASS.
- Python: 38 tests, OK, no skips, including fresh-process active GUI close cleanup.

Fresh command: `python tools/build.py`, exit 0. Boot 12344 bytes of 16384; deer and dinosaur each 19124 of 23552. Application vectors and bounds validated by the build script.

Fresh uVision native builds of boot, deer and dinosaur all exited 0 with 0 errors and 0 warnings. Commands use `UV4.exe -b GameOTA.uvprojx -t TARGET -o build/keil-final-TARGET.log` with a hidden window. Fresh `git diff --check` reported no whitespace errors.

No physical board, LoRa radio or real power-interruption validation has been performed. Version-tagged release aliases and final documentation remain delivery work at this checkpoint.
