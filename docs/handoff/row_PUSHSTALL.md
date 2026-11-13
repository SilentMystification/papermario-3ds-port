# Row PUSHSTALL: the send watchdog reads network bytes, not 3dslink text, so a healthy send never reads as stalled

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## User fact (2026-11-12)
Build 5025 RAN on the O3DS .170. The log said "no send progress for 30 s" (build3ds/wt/FPS60FL/build3ds/out/push_cmd2.txt).

## Cause (oracle code read, ORACLE8)
- The 3dslink output (build3ds/wt/FPS60FL/build3ds/out/push_192.168.1.170_20261112-190659_a1.txt) is 72 bytes, no CR: "Sending pm_3ds.3dsx, 38373092 bytes\n" and, at the end of the send, "11958812 sent (31.16%), 1028 blocks\n". 3dslink
  prints nothing while it sends. 31.16 % is the deflated size against the file size: the send was complete.
- 3ds/push_3ds.sh:408-417 counts progress as growth of that file. So every healthy send counts as "no progress": the warning at
  :420-423 fires at 30 s, and the kill at :434-437 fires at NOGROW_SECS (60 s) for any send longer than 60 s after "Sending".
- The fake 3dslink of 3ds/tests/push/run.sh cases (e) and (f) prints progress lines, which the real one does not.

## Start
`sh 3ds/tools/wt_new.sh PUSHSTALL`. PM_ROW=PUSHSTALL PM_LANE=PUSHSTALL on every call.

## Touch only (places)
3ds/push_3ds.sh: the line after `memflag=""` (:50), the option parser (:52-64), the poll loop (:407-423).
3ds/tests/push/run.sh: after `check none 3ds/tests/push/connect_only.txt` (:26). Nothing else.

## Exact edits
1. After `memflag=""` (:50, before the parser) add:
```
# PUSHSTALL: netio_tx "<rx> / <tx>" prints the tx part of docker stats NetIO in bytes (decimal units B, kB, MB, GB, TB).
netio_tx() {
    printf '%s\n' "$1" | awk -F'/' '{ v = $2; gsub(/ /, "", v); n = v + 0; u = v; sub(/^[0-9.]+/, "", u); m = 1
        if (u == "kB") m = 1e3; else if (u == "MB") m = 1e6; else if (u == "GB") m = 1e9; else if (u == "TB") m = 1e12
        printf "%.0f\n", n * m }'
}
```
2. Parser (:52-64): add the line `--netio) netio_tx "$2"; exit 0 ;;` right before `*) break ;;`.
3. Poll loop: before the loop (after `warned=0`, :407) add `last_tx=0`. Inside the loop, right after the
   `[ "$started" -eq 1 ] && sent_for=$((sent_for + 1))` line (:418) add:
```
        # PUSHSTALL: 3dslink prints nothing while it sends. In docker the container's sent bytes are the progress (every 5 s).
        if [ "$usedocker" -eq 1 ] && [ "$started" -eq 1 ] && [ $((sent_for % 5)) -eq 0 ]; then
            tx=$(netio_tx "$(docker stats --no-stream --format '{{.NetIO}}' "$cname" 2>/dev/null || true)")
            if [ -n "$tx" ] && [ "$tx" -gt "$last_tx" ]; then last_tx=$tx; nogrow=0; fi
        fi
```
4. The warning text at :422: `echo "attempt $i: no send progress for 30 s (no output, no network bytes, last line: ...)"`: keep
   the `$(tr ...)` part as it is, change only the words before "last line".
5. run.sh after :26 add case (g):
```
# (g) PUSHSTALL: docker stats NetIO to tx bytes.
for c in '0B / 0B=0' '1.45kB / 12.3MB=12300000' '648B / 1.2GB=1200000000' '3.5MB / 950kB=950000' '12kB / 512B=512'; do
    got=$(sh 3ds/push_3ds.sh --netio "${c%=*}"); if [ "$got" = "${c##*=}" ]; then echo "ok   netio ${c%=*}"; else echo "FAIL netio ${c%=*}: $got"; fail=1; fi
done
```
6. Commit: `PUSHSTALL: send progress from docker stats tx bytes, 3dslink prints nothing while it sends`.

## Gate (expected values)
1. Seeded fault: make edits 1-5, but in edit 1 write `m = 1` in place of `m = 1e3` for kB. Run `sh 3ds/tests/push/run.sh >
   build3ds/out/push_test_before.txt 2>&1`. Expected: `FAIL netio 3.5MB / 950kB: 950` and `PUSH TESTS FAIL`.
2. Put `m = 1e3` back, run again into build3ds/out/push_test.txt. Expected: 5 lines `ok   netio`, cases (e) and (f) ok, `PUSH TESTS
   PASS`, exit 0. No console push, no Azahar start. Counter: none (tool row).

## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit. Hand back: the ready hash and the two test outputs.
