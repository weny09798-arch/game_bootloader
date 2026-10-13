# Final delivery packaging report — 2026-10-13

Root: `C:/毕业设计_许剑峰/game_bootloader`. Delivery brief: `docs/implementation/delivery-brief.md`. No commit or push performed by this delivery task; no firmware, host, original game or LabVIEW source modified.

## Changed and supplied files

- `tools/build.py`: extracted the existing all-target publication block into `publish_artifacts`, called by the normal build. Version-tagged aliases were already present in the baseline; this change exposes their real publication behavior for compiler-free regression verification, while retaining generic deer.bin/dinosaur.bin compatibility. No target/toolchain/layout behavior changed.
- `tests/test_packaging.py`: synthetic valid application images and temporary real files test aliases against the original image, generic BIN, and manifest version/length/CRC, without invoking an ARM compiler. Only module loading and the temporary root use patching; file publication and CRC checks are real.
- `README.md`: Chinese precompiled-package entry point, dependency install before GUI command, version-tagged BIN paths, first-only full erase, pinout, transparent UART9600 8N1, manual reset, persistent waiting mode and serial exclusivity. Unsupported specific LoRa air-rate recommendation removed; module parameters must match and follow the module manual. CRC-not-authentication, no factory rollback,23KB app limit and no hardware-validation caveats retained.
- `docs/protocol.md`: exact INSTALLING recovery transition and persistence requirements; valid source survives I/O/installation failures, confirmed invalid source plus completed old-A check permits request=1 VALID(old A)/EMPTY and fresh transfer, with no invalid autojump.
- `docs/validation.md`: actual fresh final software statistics and all pending physical validation.
- `docs/implementation/port-report.md`: historical checkpoint labelled, old corrupt-stage issue marked fixed/tested with final evidence.
- `.vscode/tasks.json`: preserved existing build/verify/GUI tasks and added dependency installation task; JSON parse verified. `GameOTA.code-workspace` inspected and preserved; root path and UTF8 setting remain valid.
- `.gitattributes`: scoped `releases/** -text` disables Git newline conversion for shipped artifacts and SHA256SUMS, preserving published bytes across Windows/Linux checkouts and raw Git downloads despite repository core.autocrlf=true.
- `releases/2026-10-13/`: supplied two merged factory HEX files, two versioned BIN files, manifest and SHA256SUMS. Each of the five build artifacts copied from current dist using PowerShell Copy-Item and matched by SHA256. No AXF/MAP/object files included. These files are non-ignored and ready for the coordinator to track; this task did not stage/commit them.

## Fresh command evidence

1. `python -m unittest discover -s tests -p test_packaging.py -v` before the helper extraction: exit1,4 tests,1 expected failure (`artifact publication must be usable without an ARM compiler`). Existing3 packaging cases passed. No production code written before this failing test.
2. Same packaging command after extraction: exit0,4 tests OK. New test checks aliases/generic bytes and manifest relationship directly on temporary files.
3. `python tools/verify.py`: exit0.199984 core checks,0 failed tests; receive cuts12034, copy cuts12003, journal rollover544 each direction; recovery-transition journal cuts32 each for valid/invalid old A. Actual app-port PASS. Debounce23 checks PASS. Python39 tests OK, no skips.
4. `python tools/build.py`: exit0. Boot12344/16384 bytes, deer/dinosaur each19124/23552 bytes. CRC32 boot701A033A, deerF587127D, dinosaur149D4AF9. Publication includes both generic and versioned BIN names and factory HEX/manifest.
5. PowerShell release Copy-Item and SHA256 comparison: all five copied artifacts equal current dist. After writing SHA256SUMS, every listed checksum was rechecked against both the shipped file and current dist:5 PASS. `.vscode/tasks.json` and `GameOTA.code-workspace` parse as JSON. `git diff --check` reports no whitespace errors (Git emits only its Windows LF-to-CRLF notices).
6. Keil native UV4 evidence supplied by coordinator from the same day: boot/deer/dinosaur all0 errors/0 warnings. Firmware unchanged by this packaging task.
7. `git check-attr text --` for all six release files reports `text: unset`. For each file, `git hash-object --no-filters` matches `git hash-object --path`, proving Git clean filtering preserves the exact release bytes under the active attributes. All five published SHA256 values still match current dist and the shipped files. No staging or commit performed.

The new packaging unit test runs without requiring ARMCC; full verify additionally requires host GCC and the host test environment. Compiler-generated dist/build files remain ignored local artifacts.

## Shipped SHA256 values

| File | SHA256 |
| --- | --- |
| factory_deer.hex | 203611d320961a026ce6489323c4fa21942902991ace9e4658437195d7349796 |
| factory_dinosaur.hex | f8e75d2a5dfec58ade4ebeb2f1d311c5822ffd56879d270257f275384eb615b6 |
| deer_v1.bin | f6ad60cdbcc097adfa036e4782eb3c2717db3e79a70bc951d01e57cd4ecdee87 |
| dinosaur_v2.bin | 0e4efe045862bd1e2c27ab1894170f67a32022465ed136ce843fcf0775c9ef50 |
| manifest.json | 0183e77c8f22fde7ee25929177aa198d211f97c352a2ca92076d2c46fb61643c |

`SHA256SUMS.txt` covers these five files. Manifest still uses original target/version/bytes/limit/base/CRC32 schema, preserving consumers.

## Remaining physical validation

No board/radio was attached for this task. ST-Link first erase/program, physical8MHz clock/pinout/OLED/key30ms debounce, manual reset and persistent waiting, UART/LoRa transparent setup and actual V1→V2→V1, LabVIEW control, real Flash interruption during receive/install/journal writes, malformed image and link interruption recovery remain pending in docs/validation.md. Hardware acceptance cannot be inferred from the simulator. App23KB limit, CRC integrity without authentication, and absence of factory/logic-failure rollback remain intentional limitations.
