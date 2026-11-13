# Row PUSHEND1: push_3ds.sh ends every push with one result line and a clear exit code, and a failed send reads no live log

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## User facts (2026-11-12)
The push must succeed or end with one clear end line (ok or failed, the reason) and an exit code. Never a silent wait.
A failed send must not start the live log wait. Build 5025 ran on the console, but the output ended "PUSH FAILED stage=no log".
## Cause (oracle code read, ORACLE8, main 3c7d977ab, 3ds/push_3ds.sh)
- :498-500: a failed send with stage=send prints PUSH FAILED, then still runs read_live_log and report_log (up to -t seconds,
  86400 with go.ps1 -Interactive, ended only by the idle limit).
- :284: a complete send whose game sent no line ends "PUSH FAILED stage=no log", exit 1: the send result and the result of the
  live log are one word. Exit sites print different words (:121, :179, :285, :304, :305, :333, :347, :493, :497).

Start: `sh 3ds/tools/wt_new.sh PUSHEND1`. PM_ROW=PUSHEND1 PM_LANE=PUSHEND1 on every call.
## Touch only (places)
3ds/push_3ds.sh: the line after `cd "$(dirname "$0")/.."` (:37), mem_gate's `exit 4` (:121), the stale `exit 5` (:179), the echo of
read_live_log (:201), report_log (:271-306), :333, :347, the end block (:491-504). 3ds/tests/push/run.sh: after the (c) block (:60).
## Exact edits (3ds/push_3ds.sh)
1. After :37 add:
```
# PUSHEND1: end_push <rc> <result> <reason>: the one end line of a push (result ok, sent-no-log or failed), also in
# build3ds/out/push_result.txt (go.ps1 puts it into its PUSH END line). Exit codes: 0 ok, 6 sent but the game sent no line, else failed.
end_push() { line="PUSH RESULT $2 rc $1: $3"; echo "$line"; mkdir -p build3ds/out; printf '%s\n' "$line" >build3ds/out/push_result.txt; exit "$1"; }
```
2. :121 `exit 4` -> `end_push 4 failed "memory OVER, push not sent (go.ps1 -MemOk sends anyway)"`.
3. :179 `exit 5` -> `end_push 5 failed "stale 3dsx, nothing sent"`.
4. :201 both echo texts: append ` (stops at $idle s without a line, or when the game closes)` inside the quotes, before the
   closing quote.
5. report_log: delete the `send)` arm (:281) and the whole `if [ "$1" = send ]; then ... fi` block (:287-299). Replace the lines
   :280-285 (`attach) ...`, `*) echo "PUSH FAILED stage=no log" ;;`, `esac`, `refusal_hint`, `exit 1`) with:
```
        [ "$1" = attach ] && end_push 1 failed "attach: no live log from $ip"
        end_push 6 sent-no-log "the 3dsx was sent, the game sent no line to $ip:17492 (see the NO LOG line above)"
```
   and replace :304-305 with `push) end_push 0 ok "sent, $nlines lines read" ;;` and `attach) end_push 0 ok "attach, $nlines lines read" ;;`.
6. :333 `exit 1` -> `end_push 1 failed "no 3DS netloader found (press Y in the Homebrew Launcher)"` (delete its echo).
7. :347 -> `[ -f "$SRC3DSX" ] || end_push 1 failed "no $SRC3DSX, nothing was pushed"`.
8. End block :492-501: replace everything from `if [ "$ok" -ne 1 ]; then` to its `fi` with:
```
if [ "$ok" -ne 1 ]; then
    [ "$ownerdead" -eq 1 ] && end_push 1 failed "the caller of this push is gone ($reason)"
    refusal_hint
    end_push 1 failed "stage=$stage: $reason (after $attempts attempts, $ip). The live log is not read."
fi
```
9. run.sh after :60 add case (c2):
```
# (c2) PUSHEND1: a failed send ends with one PUSH RESULT failed line and reads no live log.
if echo "$oc" | grep -q '^PUSH RESULT failed rc 1: stage=send' && ! echo "$oc" | grep -q "reading the game's log"; then echo "ok   failed send: result line, no live log"
else echo "FAIL failed send result"; echo "$oc" | tail -n 8; fail=1; fi
```
10. Commit: `PUSHEND1: one PUSH RESULT line per push, exit 6 = sent but the game sent no line, no live log after a failed send`.
## Gate (expected values)
1. Seeded fault: make edit 9 only, `sh 3ds/tests/push/run.sh > build3ds/out/push_test_before.txt 2>&1`. Expected `FAIL failed
   send result`, `PUSH TESTS FAIL`.
2. Make edits 1-8, run again into build3ds/out/push_test.txt. Expected `ok   failed send: result line, no live log`, every other
   case ok, `PUSH TESTS PASS`. No console push, no Azahar start. Counter: none (tool row).
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit. Hand back: the ready hash and the two test outputs.
