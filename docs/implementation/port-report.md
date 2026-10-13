# STM32 port and build report

## Historical implementation checkpoint

This original port report is a historical checkpoint, not the final delivery status. The original sizes and test counts below are retained as checkpoint evidence. See `docs/validation.md` and `docs/implementation/delivery-report.md` for the final 2026-10-13 results.

Root implemented firmware/port/{board.h,board.c,jump.s}, boot/main.c, app/{game.c,app_support.*,characters.c}, tools/{build.py,packaging.py,verify.py,keil_project.py}, GameOTA.uvprojx, packaging/native app-port tests and Chinese setup docs.

User constraints: C8T6, B16KB+A23KB+stage23KB+dual1KB metadata, PA9/10 UART1 9600 8N1, PA0 active-low EXTI, OLED PB6/7. Original source at prior path preserved. Copy's startup stack enlarged4KB; system VTOR define now configurable via compile flags.

Board flash callbacks1success0failure restrict mutation to>=A and<endFlash, force HSIready, unlock/clearflags/program/lock with saved interrupt mask, verify writes and page erase. USART ring512 has overflow/error latch; damaged burst discarded until100ms idle. TIM2 free-runs2kHz (noIRQ) for monotonic sampled milliseconds independent of legacy SysTick delay; must sample before32.768s wrap, satisfied normal polling and bounded install. IRQ key event deferred with30ms stable-low debounce. Jump disables/clears8NVIC banks, SysTick,pendingPendSV/SysTick, RCC teardown, VTOR toA, asmsetsCONTROL/BASEPRI/FAULTMASK/MSP without stack access after switch; appboardinit reenables interrupts.

Bootnormal waits3000ms, pressedkey persistsrequest; previousrequest orinvalidA enters updater. INSTALLING recover copy on boot. Shared dispatcher synchronously installsCOMMIT; DONE shown then10sgrace for retries before systemreset. Apprequest persists and freezesgamewaitingmanualreset. QUERY supported in app; updatecommands NACK with APPmode flag. Appbinary quarantine persists untilvalidframe or2secidle, sobadheader/payload never leaksASCIIgamecommands. Existing game adapting character selection and title/version; T-rexpage-packed24x24 twoframes, originaldeerretained. Main/game logic sourcepath unchanged; adaptedcopy underfirmware/app.

Packaging tests first failedmissingtool thenpassed3cases: fixed partition/vector rejection, journalmarker/CRC andHEXchecksum. Native app_port_test exercises actualApp_Poll with hardwaretransportstub and realota/protocol: ASCIIqueue, valid/badCRC/badlengthframe isolation, UARTerror quarantine and durablekey request.

Evidence: python tools/verify.py executed:199393corechecks/0failedgroups; nativeappportPASS;3PythonpackagingtestsPASS. python tools/build.py:boot12128bytes,deer19092,dinosaur19092,eachwithin16384/23552limits. UV4 commandline boot/deer/dinosaur each0errors0warnings. ActualCLI commands/logs in build/, generateddist/manifest.json.

Initial minimaluVisionXML failed missingmemoryselection/includeinheritance. Regenerated fullschema fromworkinguserproject, preserve ordering/options and per-targetIncludeInBuild, XMLdeclarationrequired toavoidhiddenload dialog. CurrentGameOTA was tested acrossall3targets. CLItool uses samecompiler butdeterministic explicitflags andflashboundschecks.

## Resolved final status (2026-10-13)

The original checkpoint identified pending INSTALLING with corrupt stage blocking fresh BEGIN. This issue is now fixed and tested; it is not an unresolved delivery bug. `ota_recover` revalidates stage before copying. Valid candidate metadata survives source read errors, erase/write failures and final A verification failure. Only confirmed source length/vector/CRC failure permits abandoning it, and only after old A verification completes. A read failure preserves INSTALLING. Otherwise recovery first persists request=1 with VALID(old A) or EMPTY, clears the invalid candidate/session, and permits fresh transfer without autojumping invalid A.

Fresh final evidence: `python tools/verify.py` exited0;199984 core checks,0 failures; receive/copy cuts12034/12003; journal rollover544 in each direction; recovery-transition journal cuts32 for each old-A validity case; actual app-port PASS; debounce23 checks PASS; Python39 tests OK/no skips. `python tools/build.py` exited0: boot12344/16384 bytes and both apps19124/23552. Coordinator's same-day UV4 builds reported0 errors/0 warnings for all three targets. Actual30ms debounce is covered by firmware-module tests.

Existing version aliases deer_v1.bin/dinosaur_v2.bin are now covered by a compiler-free publication regression test, and included with merged factory HEX, manifest and SHA256SUMS under releases/2026-10-13. Real8MHz clock, module configuration, LoRa link, physical Flash/power/key/OLED/SWD checks are still pending; see docs/validation.md. No physical validation has been claimed.
