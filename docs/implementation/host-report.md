# PC upgrade tool implementation report

## Delivered files and interfaces

- `host/protocol.py`: `MessageType`, immutable `Frame(type, session, sequence, payload).encode()`, incremental `FrameParser.feed(bytes)` returning decoded frames, `Response.from_frame(frame)`, validated `FirmwareImage(data, version)` with length/CRC/BEGIN payload.
- `host/uploader.py`: `Uploader(transport, response_timeout=5.0, control_timeout=30.0, max_retries=5, cancel_event=None, log=None)`, `query()`, `upload(image, progress=None)`, `request_cancel()`, and observable `phase`. Transport supplies `read(size)`, `write(bytes)`, and a finite read `timeout` no longer than the response deadline. Frontends set serial write timeout as well. Frontends own/close the transport.
- `host/cli.py`: Chinese command-line messages, query/upload subcommands, default 9600 baud, filename `v1`/`v2` inference, explicit integer version override, validation before opening a serial port. Exit codes: 0 verified success/query; 1 failure; 2 cancellation/interruption; 3 uncertain outcome.
- `host/gui.py`: Chinese Tk window with COM refresh, baud rate, BIN chooser, inferred/editable integer version, query/upload/cancel, log and progress. Serial communication and BIN reading/validation run on a worker. Workers send events through a queue; only the Tk thread updates widgets. Launch opens no serial port and performs no flashing.
- `requirements.txt`: only `pyserial>=3.5,<4`; tkinter is Python standard library.
- `tests/test_host_protocol.py`, `tests/test_host_uploader.py`, `tests/test_host_frontends.py`: 34 tests including the real C flash engine and withdrawn Tk construction/query/upload.

Run from repository root:

```powershell
python -m host.gui
python -m host.cli --port COM3 query
python -m host.cli --port COM3 upload dist/dinosaur.bin --version 2
```

## Wire and outcome guarantees

The codec implements GBOT v1 little-endian frames, full header/payload CRC32, max132-byte payload/max150-byte parser buffer, incremental split frames, junk/overlapping magic resynchronization, and early bad-version/bad-length rejection matching the locked C wire. Response payloads must be exactly 28 bytes; error/state/request/offset/length ranges are checked. Responses must match request session and sequence. Unexpected, malformed, corrupt and stale frames are ignored.

Each upload obtains a new session and BEGIN always starts at offset zero. DATA uses 128-byte stop-and-wait chunks. Default six total attempts resend exactly the same bytes/session/sequence; ACK offsets must equal the last chunk end before progress advances. BEGIN and COMMIT use 30-second response deadlines. Short serial writes are completed; zero/invalid writes fail finitely. A parser waiting on a truncated frame resets between retry deadlines.

Application-mode QUERY is supported, but upload is rejected with the Chinese instruction to press KEY and manually reset into the bootloader. BIN files are checked before BEGIN: 8..23552 bytes, 8-byte aligned MSP inside `(0x20000000,0x20005000]`, Thumb reset address inside the actual image linked at `0x08004000`, and integer version 1..0xffffffff.

COMMIT requires a matching DONE/error0/VALID/request0/version/length/CRC. Regardless of DONE, a fresh QUERY must confirm VALID/request0 and matching uploaded version/length/CRC before reporting success. Lost DONE gets identical retries, then QUERY reconciliation. A simulated session-loss response in the commit window is reconciled by QUERY including application mode. A disconnected port, failed QUERY, installation state or mismatched metadata yields explicit `UnknownOutcome`; success is never inferred from transmitted progress or a plain COMMIT ACK.

Cancellation is only attempted before COMMIT. A fresh QUERY resolves whether an in-flight DATA with lost ACK was consumed, so CANCEL uses the sequence derived from actual receive offset. A reliable CANCEL ACK is required to report `Cancelled`; missing ACK or inconsistent/non-receiving state reports uncertain outcome. Keyboard interruption during receive attempts this cancellation; interruption during COMMIT instead queries the installed outcome. During installation the GUI states that cancellation cannot roll back and continues waiting. Window close requests receive cancellation and gives its worker a one-second nonblocking grace period, then closes an active port; closing during installation requires a fresh query next time. No rollback is promised.

## TDD and actual verification

Initial test-first RED: protocol/uploader absent, 20 assertions failed with explicit missing implementation messages. Frontend RED: 5 missing-frontend assertions failed. Separate receive/commit keyboard-interruption RED assertions failed before implementation. Final parser bad-version and uncertain-cancel phase RED failed at `5 != 0` and `receiving != unknown`; fixes then passed.

Final commands, from `C:/毕业设计_许剑峰/game_bootloader`, all exit code 0:

```powershell
python -m unittest discover -s tests -p 'test_host*.py' -v
python -m host.cli --help
python -m compileall -q host
```

Actual final targeted output:

```text
Ran 34 tests in 1.476s
OK
```

No skips in this final run. CLI help printed the required port, baud, version, query and upload options. Compileall completed without diagnostics. Hidden Windows Tk tests construct/withdraw the root immediately, establish no serial connection on construction, then query and upload through the worker against the actual C device; they verify queue-delivered status/progress and closed transport, then destroy the window.

Native integration uses test-only `NativeTransport` around `build/native/ota_native.exe serve`: Python wire packets become hex stdin lines and actual C response lines become serial-like bytes, including partial read/write fragmentation. If missing, tests call `tools.verify.native_build()`; this final run used the existing executable. Fault injection only alters/drops responses or interrupts the transport boundary. The engine owns the real flash emulator and all state changes.

The 389-byte v2 image, 9/23551/23552-byte images and the actual `dist/dinosaur.bin` (19092 bytes, uploaded as version2) install successfully with fresh C QUERY metadata validation. Cases cover lost first DATA ACK and stale duplicate ACKs; CRC corruption; short reply payload; wrong session/sequence; out-of-range and bounded incorrect offsets; six identical retries and exhaustion; application flag; cancel after BEGIN; cancel after a consumed DATA whose ACK was lost; uncertain cancel acknowledgement; cancellation/keyboard interrupt during COMMIT; all DONE loss; commit-window session loss; wrong DONE/post-query version/length/CRC/state; COMMIT ACK without DONE plus failed query; correct DONE followed by wrong QUERY metadata; closed commit transport; unbounded reads and zero writes; invalid BIN rejected before serial open.

## Limits and scope

These checks prove the host tool against the actual native C engine and modeled flash. They do not prove real LoRa throughput, board UART timing, real MCU flashing or physical power interruption. The commit-window reset test injects the response/session-loss condition at the boundary; it is not a physical reset test. No firmware, shared core, tools or other tests were edited by this host task. No git commits were made.
