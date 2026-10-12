# Task 1: portable OTA engine (read this first)
Root: C:/毕业设计_许剑峰/game_bootloader. Own firmware/common/* and tests/native/* only. Do not change legacy or hardware port. Use apply_patch. Tests first; native GCC at D:/tools/qt/application/Tools/mingw530_32/bin/gcc.exe. Keep portable C99 compatible ARMCC5. No malloc. Tests must call actual C implementation.

## Fixed addresses and state
FLASH_BASE=0x08000000, FLASH_SIZE=0x10000, PAGE_SIZE=0x400, APP_BASE=0x08004000, APP_SIZE=0x5C00, STAGE_BASE=0x08009C00, META0=0x0800F800, META1=0x0800FC00. RAM_BASE=0x20000000 RAM_END=0x20005000. MCU_ID=0x103C8.
States EMPTY=0, VALID=1, RECEIVING=2, INSTALLING=3. Metadata retains old app length/crc/version while receiving and pending candidate length/crc/version. Update request stored in record. Initialize erased flash as EMPTY and no auto-jump. Install only from validated candidate; record INSTALLING before any A erase. Recovery restarts copy from first page. CRC32 ISO-HDLC standard zlib (123456789=0xCBF43926). Valid vectors: 8-byte aligned MSP >RAM_BASE and <=RAM_END; Thumb reset pointer in [APP_BASE, APP_BASE+actual image length).

## C API (declare in ota.h)
typedef struct { void *user; int (*read)(void*,uint32_t,uint8_t*,uint32_t); int (*erase)(void*,uint32_t); int (*write)(void*,uint32_t,const uint8_t*,uint32_t); } ota_flash_t;
typedef struct { uint32_t length,crc,version; } ota_image_t;
typedef enum { OTA_OK=0, OTA_ERR_STATE=1, OTA_ERR_LENGTH=2, OTA_ERR_TARGET=3, OTA_ERR_VECTOR=4, OTA_ERR_CRC=5, OTA_ERR_FLASH=6, OTA_ERR_SESSION=7, OTA_ERR_SEQUENCE=8, OTA_ERR_OFFSET=9, OTA_ERR_CONTENT=10 } ota_result_t;
Define ota_t as static-allocatable context containing flash callbacks, state, request, app/candidate descriptors, received offset, session, expected seq and journal generation/slot. API:
ota_result_t ota_init(ota_t*,const ota_flash_t*);
ota_result_t ota_request_update(ota_t*);
ota_result_t ota_begin(ota_t*,uint32_t session,uint32_t mcu,uint32_t base,const ota_image_t*);
ota_result_t ota_data(ota_t*,uint32_t session,uint16_t seq,uint32_t offset,const uint8_t*,uint16_t len);
ota_result_t ota_finish(ota_t*,uint32_t session); // validate full stage, persist INSTALLING, NO install yet
ota_result_t ota_install(ota_t*); // verifies stage, erases/copies ALL APP pages, validates A, commits VALID/request=0
ota_result_t ota_cancel(ota_t*,uint32_t session); // only RECEIVING, returns VALID if old app valid otherwise EMPTY, clear request
ota_result_t ota_validate_app(ota_t*);
uint32_t ota_crc32(const uint8_t*,uint32_t);
Native test harness may directly populate factory record through callback. After reboot RECEIVING session=0 and received=0; only a new BEGIN can restart. Reject BEGIN while INSTALLING. Repeated same BEGIN during active matching session/image must ACK without erasing progress. DATA seq starts 1; offsets contiguous; duplicate last block accepted only if session, seq, offset,length,bytes match. Reject wrong session, wrong seq, zero/oversized/non-final short chunk. Last odd-byte length supported by adapter padding FF. Keep app unmodified on any reception failure. Failed metadata write must not commit state in RAM (reload latest committed record or mark unusable). Flash mutations restricted to designated region. HSI hardware responsibility external.

## Journal record exact 64 bytes, little-endian
offset0 magic u32=0x4F54414D; 4 generation; 8 state; 12 request; 16 app.length;20 app.crc;24 app.version;28 candidate.length;32 candidate.crc;36 candidate.version;40..55 zeros (4 u32 reserved);56 crc32(first56 bytes);60 commit u16=0xA55A;62 pad u16=0xFFFF.
Write first60 bytes, readback verify, then commit halfword last. CRC and marker validated on scan, additionally state,length,request sanity. Two pages 16 records each; never erase page holding newest valid record before committed newer record exists elsewhere. On torn records skip and advance to next erased record; no reprogramming occupied slots. Generation compare wrap-safe. Record write errors preserve previous committed state. Factory record seq1 stateVALID request0 with deer app descriptor, candidate zero.

## protocol.h/.c exact wire format
magic ASCII GBOT (4), protocol version u8=1, type u8, session u32 LE, sequence u16 LE, payload_length u16 LE, payload up to132, crc32(all header+payload), total18+payload.
types QUERY=1 BEGIN=2 DATA=3 COMMIT=4 CANCEL=5 ACK=6 NACK=7 DONE=8.
QUERY len0; BEGIN len20: MCU_ID,APP_BASE,image.length,image.crc,image.version (u32 each); DATA offset u32 +1..128 bytes; COMMIT/CANCEL len0. ACK/NACK/DONE payload7*u32: error,expected_offset,state,request,app.version,app.length,app.crc. Header echoes request session/sequence. BEGIN sequence0; DATA starts1; COMMIT uses next seq. Firmware caller validates control seq.
Expose ota_frame_t { uint8_t type; uint32_t session; uint16_t sequence,length; uint8_t payload[132]; };
parser static buffer max150; int ota_parser_feed(ota_parser_t*,uint8_t,ota_frame_t*) returns1 when complete valid; uint16_t ota_frame_encode(const ota_frame_t*,uint8_t*); ota_parser_init; parser malformed CRC/length resynchronizes and never exceeds bounds. Main port tracks timeout and handles text outside protocol, parser has explicit active indication API/field for isolation. Encoder reject invalid lengths with zero return. Document parser behavior for junk, split frames, overlapping magic, CRC failure.

## Native tests/harness
Write tests first and show expected failures. Real emulated flash implements 1KB erase and erased halfword programming, read bounds; injectable failures/power loss per halfword incl erase. Cover normal receive/install; wrong target/link/vector/length/CRC; corrupt last byte; odd length; duplicates/mismatch/out-of-order; cancellation; request survives reboot; receive loss leaves A; interrupted copy resumes; all journal rollover cuts preserve previous/new committed state. Use setjmp/longjmp for power cuts to exercise real code after cold init. Add native simulator stdin/stdout hex frames for later Python integration (test-only under tests/native). CLI mode `test` or `serve`. Serve loads deterministic old valid image fixture and parses input lines hex, emits one hex encoded response per line; COMMIT finish+install => DONE. QUERY/BEGIN/DATA/CANCEL => ACK/NACK. Implement C dispatcher reusable by hardware later as ota_handle_frame(ctx,frame,response,allow_updates) if useful, define actual API in report. Root writes build runner later; include concrete GCC compile command in report.

Report docs/implementation/core-report.md: API, files, tests actual command/output, constraints/risks. Return brief status. Do not git commit unrelated root edits; root owns final commits.
