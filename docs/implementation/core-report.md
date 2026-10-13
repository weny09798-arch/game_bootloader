# Portable OTA core report

> Historical checkpoint: this report preserves the original core implementation and test evidence. Its statement below that an invalid candidate in `INSTALLING` is retained for diagnostics describes the behavior at that checkpoint; current firmware uses durable invalid-source recovery through `ota_recover`. See [the protocol recovery description](../protocol.md#installing-重启恢复) and [the integration fix report](integration-fix-report.md) for the current behavior.

## Files and interface

- `firmware/common/ota.h`, `ota.c`: portable C99 update engine and two-page metadata journal.
- `firmware/common/protocol.h`, `protocol.c`: bounded GBOT parser/encoder and shared dispatcher.
- `tests/native/main.c`: actual C engine exercised against a 64 KiB flash emulator; `test` and `serve` modes.

All fixed addresses, enum values, public OTA APIs and the 64-byte record layout follow `core-brief.md`. Context is caller allocated; no allocation or hardware dependencies. Flash callbacks return **1 for success, 0 for failure**. Erase takes an aligned 1 KiB page base. Write takes an even address and the hardware adapter must pad a final odd byte with `0xFF`, excluding that padding from the image CRC.

Public context fields: `flash`, `state`, `request`, `app`, `candidate`, `received`, `session`, `expected_seq`, `generation`, `journal_slot`, `usable`, `last_seq`, `last_length`, `last_offset`, `last_data[128]`. `journal_slot` is -1 when no committed record exists. `OTA_STATE_EMPTY`, `OTA_STATE_VALID`, `OTA_STATE_RECEIVING`, `OTA_STATE_INSTALLING` alias the brief's state values. Cold init sets session/received to zero and expected sequence to one; a new BEGIN restarts reception.

Additional integration API:

```c
int ota_handle_frame(ota_t *ctx, const ota_frame_t *request,
                     ota_frame_t *response, int allow_updates);
```

Returns 1 when a response is generated; 0 for null arguments. Every decoded command yields ACK, NACK, or DONE. The error code is in the first response payload word, not the function return. Response length is 28 bytes and echoes request session/sequence. BEGIN requires sequence zero; COMMIT and CANCEL require `expected_seq`; DATA sequence/offset/duplicate validation is handled by the engine. COMMIT synchronously finishes and installs, emitting DONE only after VALID is committed. Repeated matching COMMIT in the same live context emits DONE again. After reboot the volatile session is unavailable; QUERY reveals committed state instead.

As requested during integration, response state has `OTA_INFO_APPLICATION` (`0x80000000`) set when `allow_updates==0`. An `OTA_APPLICATION` build always sets the flag and compiles out all update dispatcher branches, even if the caller passes 1. Low 31 bits remain the numeric OTA state. QUERY works in either mode; application update commands receive NACK/STATE. This permits the uploader to distinguish the running application from the bootloader.

## Persistence and recovery

Records serialize explicitly in little endian, validate magic/CRC/marker/padding/reserved zeros/state/request/descriptor lengths, and compare generations modulo 32 bits. Occupied torn records are skipped. Append searches erased slots and only erases the page opposite the newest committed record when both pages are full. First 60 bytes are programmed and verified before the commit halfword; full committed record is verified afterward. Metadata errors reload the latest committed record; a failing reload leaves `usable==0`.

Reception writes only stage/metadata. Repeated matching active BEGIN and last DATA block preserve progress. A failed stage programming callback invalidates the volatile session so a new BEGIN erases and restarts safely. CRC covers every actual image byte, including an odd final byte. Vector checks use an 8-byte aligned MSP in `(RAM_BASE,RAM_END]`, Thumb reset bit, and the reset address with Thumb bit removed inside the actual application image.

`ota_finish` validates stage and commits INSTALLING without touching A. `ota_install` validates stage again, erases all 23 application pages, copies the candidate, validates A, and commits VALID/request=0. Interrupted installation therefore cold starts from page one and never authorizes a partially copied image. An invalid candidate in INSTALLING is retained for recovery diagnostics; BEGIN/CANCEL cannot bypass it. Cancel during reception restores VALID only if the old application still validates, otherwise EMPTY.

## Parser and simulator behavior

Parser buffer is 150 bytes. `active` means a magic prefix or candidate frame is buffered; callers should isolate those bytes from text commands. Junk bytes are discarded; matching trailing magic prefixes are retained, so `GGBOT` and overlapping magic resynchronize. Split frames are accepted byte by byte. Invalid version, payload length above 132, or CRC discards one leading byte and scans again, retaining possible embedded frames. A plausible incomplete frame waits for its declared bytes; hardware timeout handling is external and resets it with `ota_parser_init`. Encode returns zero for a payload above 132. CRC-valid but unknown/semantically malformed commands are decoded and rejected by the dispatcher.

`ota_native serve` cold loads a deterministic old VALID application: length257, version1, CRC `0x6B3D46C8`, bytes `(offset*37+13)&255` with MSP `0x20005000` and reset `0x08004009`. It accepts whitespace-separated hex bytes, one request line at a time, and flushes one lowercase hex response line. Parser state persists across lines so split frames work; a line without a complete valid frame yields an empty response line. Malformed hex resets the parser. Use one command per line; if multiple complete commands occur in one line, all are processed and the last response is emitted.

Example QUERY:

```text
stdin:  47424f54010100000000000000004cf9375a
stdout: 47424f5401060000000000001c00000000000000000001000000000000000100000001010000c8463d6be7023058
```

## Actual verification

Initial RED run against minimal callable stubs: `11 checks, 11 failed tests`, each failing because the corresponding implementation was missing. Application-mode flag RED run later failed at `le32(r.payload+8)==0x80000001UL`; implementation then passed. Serve-line RED run failed because its dispatcher adapter returned no response before implementation.

Final command from the repository root, PowerShell:

```powershell
& 'D:/tools/qt/application/Tools/mingw530_32/bin/gcc.exe' -std=c99 -O1 -Wall -Wextra -Werror -pedantic -Ifirmware/common firmware/common/ota.c firmware/common/protocol.c tests/native/main.c -o tests/native/ota_native.exe
& tests/native/ota_native.exe test
```

Actual output, exit code 0:

```text
receive power cuts: 12034
copy power cuts: 12003
journal rollover direction 0 power cuts: 544
journal rollover direction 1 power cuts: 544
199393 checks, 0 failed tests
```

The emulator checks read bounds, page alignment, erased-halfword-only programming, and odd-byte padding. It injects failure at every metadata programming halfword and uses setjmp/longjmp to cut power before every halfword mutation, including partial erase. Every power-cut iteration cold initializes the actual engine. The journal cut loop preserves its nested counters with `volatile`; the context altered during a cut is cold-initialized before any field is read. Integration verification with `-O1 -Werror` reproduced `-Wclobbered` for the original counters, and the corrected harness compiled without diagnostics and passed the full suite with those optimization flags. Tests cover erased startup/factory metadata, request persistence, normal/maximum/odd image lengths, reception loss preserving A, target/link/vector/length/CRC errors, final-stage-byte corruption, matching/mismatching duplicates and order errors, repeated BEGIN, cancellation with/without old app, failed stage writes, torn metadata slot skipping, invalid record sanity, generation wrap, partial copy recovery, both full-journal erase directions, parser junk/overlap/splits/CRC/length rejection, dispatcher control sequence/DONE retry, application flag, and hex serve QUERY.

## Limits and integration responsibilities

Native verification proves the C state machine and modeled flash behavior; MCU programming voltage/timing/interrupt/HSI handling, serial transport timeout, UART overflow/text isolation, watchdog policy, jump teardown and real-board power interruption remain hardware integration responsibilities. The emulator's partial erase model provides deterministic halfword cuts, not an analog model of physical Flash failures. CRC is integrity checking rather than authentication. No changes were made to legacy firmware or the STM32 hardware port, and no commits were created by this task.
