# Independent integrated-fix re-review

Date: 2026-10-13. Range: 5b2f527..e28bc53. Read-only reviewer: fix_rereview (GPT-6.1-sol). Verdict: PASS, all four Important findings addressed; no new Critical/Important breakage in this fix range.

1. App binary burst quarantine: App_Poll no longer releases quarantine when a valid QUERY is answered. It ends only after more than 2000ms idle. The real application-port regression checks malformed length, QUERY, SJPR at one clock and normal commands after idle.
2. Key debounce: each unconsumed falling edge restarts the 30ms stable-low interval. Board event consumption is interrupt protected. Actual helper tests cover bounce timing, held single trigger, release/rearm and timestamp wrap.
3. Corrupt pending stage recovery: ota_recover separates invalid source validation from I/O or destination failures, checks old A, then durably commits VALID or EMPTY with request=1 before permitting a new transfer. Valid pending candidates remain protected. Tests cover fresh upgrades and recovery journal power cuts.
4. GUI exit: transport opening and shutdown are serialized; explicit close survives Tk termination through a non-daemon bounded cleanup timer. Fresh-process tests cover receiving, COMMIT and delayed opening, without claiming cancellation on window close.

Evidence: the reviewer inspected code and regression changes against integration-fix-report.md; it did not rerun the suite or alter the checkout. Whole-branch review and final publication follow this scoped checkpoint. Hardware validation remains pending.
