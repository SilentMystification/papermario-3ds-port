# Row PUSHWARP: a console push boots to the file select screen, -Toad boots to Toad Town, warp=random only for Azahar

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## User fact (2026-11-12)
The go.ps1 push default adds warp=random on the console. Wrong. Random warp only when Azahar has no specific scene. On the
console: no warp word = the file select screen. One switch word = Toad Town (warp=mac_01:0 pos=0:0:200).

## Cause (oracle code read, ORACLE8, main 3c7d977ab, 3ds/go.ps1)
- :142 `$randWarp = @('warp=random')` when the game args name no spot. :163 (Azahar) and :173 (console, `$extra += $randWarp`)
  both add it. push_3ds.sh and bench_defs.json add no warp to a push (grep warp=random: only go.ps1 and one Azahar bench).

## Start
`sh 3ds/tools/wt_new.sh PUSHWARP`. PM_ROW=PUSHWARP PM_LANE=PUSHWARP on every call.

## Touch only (places)
3ds/go.ps1: the param line after `[switch]$Fg,` (:66), the RANDWARP comment lines (:139-140), the console line
`$extra += $randWarp` in Push-One (:173). 3ds/tests/push/run.sh: after the (f) block (after :94). Nothing else.

## Exact edits
1. After `[switch]$Fg,` (:66) add two lines:
```
    # PUSHWARP: push o/n boots to Toad Town (warp=mac_01:0 pos=0:0:200). Without it a console push boots to the file select screen.
    [switch]$Toad,
```
2. Replace the comment lines :139-140 with:
```
# RANDWARP: an Azahar run (push a) that names no spot (no warp=, tour=, tourlist=, preset=, loadslot=, savefile=) boots to a random
# spot: warp=random. A console push never gets it (user 2026-11-12). The run log repeats the seed: give warp=random:<n> to repeat.
```
3. Replace :173 `$extra += $randWarp` with:
   `            if ($Toad) { $extra += @('warp=mac_01:0', 'pos=0:0:200') } # PUSHWARP: no warp word = the file select screen`
4. run.sh after the (f) block add case (h):
```
# (h) PUSHWARP: a console push sends no warp=random. -Toad sends warp=mac_01:0 pos=0:0:200. The fake 3dslink prints its argv.
fake_args="$tmp/fake_args.sh"; printf '#!/bin/sh\necho "Sending argv: $*"\nexit 0\n' >"$fake_args"; chmod +x "$fake_args"
oh=$(env PM_3DSLINK="$fake_args" PM_SRC3DSX="$tmp/pm_3ds.3dsx" PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" \
    powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -Fg -ip 127.0.0.1 -Seconds 5 -IdleSeconds 5 2>&1) || true
oh2=$(env PM_3DSLINK="$fake_args" PM_SRC3DSX="$tmp/pm_3ds.3dsx" PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" \
    powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -Fg -Toad -ip 127.0.0.1 -Seconds 5 -IdleSeconds 5 2>&1) || true
if echo "$oh" | grep -q '^Sending argv:' && ! echo "$oh" | grep -q 'warp='; then echo "ok   console push: no warp word"
else echo "FAIL console push warp ($(echo "$oh" | grep '^Sending argv:'))"; fail=1; fi
if echo "$oh2" | grep -q '^Sending argv:.*warp=mac_01:0 pos=0:0:200' && ! echo "$oh2" | grep -q 'warp=random'; then echo "ok   -Toad: Toad Town"
else echo "FAIL -Toad ($(echo "$oh2" | grep '^Sending argv:'))"; fail=1; fi
```
5. Commit: `PUSHWARP: console push boots to the file select screen, -Toad to Toad Town, warp=random only on Azahar`.

## Gate (expected values)
1. Seeded fault: make edit 4 only, `sh 3ds/tests/push/run.sh > build3ds/out/push_test_before.txt 2>&1`. Expected `FAIL console
   push warp (Sending argv: ... warp=random ...)` and a FAIL -Toad line (go.ps1 has no -Toad yet), `PUSH TESTS FAIL`.
2. Make edits 1-3, run again into build3ds/out/push_test.txt. Expected `ok   console push: no warp word`, `ok   -Toad: Toad Town`,
   every other case ok, `PUSH TESTS PASS`. No console push, no Azahar start. Counter: none (tool row).

## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit. Hand back: the ready hash and the two test outputs.
