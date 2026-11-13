# Row PUSHLANE: push -NoBuild from a worktree sends its lane build, with the stale check of that lane, no main folder build

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## User fact (2026-11-12)
A push from a worktree with a lane build must work without the main folder, so a push "exactly when asked" needs no extra build.
## Cause (oracle code read, ORACLE8, main 3c7d977ab)
- 3ds/go.ps1:236-238 (PUSHFIX3) removes PM_LANE and always sends build3ds/pm_3ds.3dsx. A lane build (PM_LANE=<row>) writes
  build3ds/lane-<row>/pm_3ds.3dsx and build_key.txt, so a worktree needs a second, default build before a push.
- 3ds/push_3ds.sh:75 takes PM_SRC3DSX. :175 runs the key check with PM_SRC3DSX only when PM_BUILD_KEY_FILE is set. :176 computes
  the key with `PM_LANE=` (the default output folder): its generators then look at stamps of build3ds/, not of the lane folder.
## Blockers
PUSHKEY, PUSHEND1 and PUSHEND2 merged (PUSHEND1 edits :179 next to :176, PUSHEND2 the lines after :238). Branch after them.

Start: `sh 3ds/tools/wt_new.sh PUSHLANE`. PM_ROW=PUSHLANE PM_LANE=PUSHLANE on every call.
## Touch only (places)
3ds/go.ps1: the PUSHFIX3 lines (:236-238). 3ds/push_3ds.sh: the key line `now=$(PM_LANE= PM_KEY_ONLY=1 sh 3ds/build.sh || true)`
(:176). New file 3ds/tests/push/lane.sh. Nothing else.
## Exact edits
1. go.ps1: replace the 3 PUSHFIX3 lines with:
```
    # PUSHLANE: -NoBuild with PM_LANE set, no PM_SRC3DSX, and that lane's 3dsx present sends the lane build with that lane's key check.
    # Else a push builds and sends the default output (build3ds/pm_3ds.3dsx), never a lane's output (PUSHFIX3).
    $laneDir = if ($env:PM_LANE) { Join-Path $repo "build3ds/lane-$($env:PM_LANE)" } else { $null }
    if ($NoBuild -and -not $env:PM_SRC3DSX -and $laneDir -and (Test-Path (Join-Path $laneDir 'pm_3ds.3dsx'))) {
        $env:PM_SRC3DSX = Join-Path $laneDir 'pm_3ds.3dsx'; $env:PM_BUILD_KEY_FILE = Join-Path $laneDir 'build_key.txt'; $env:PM_KEY_LANE = $env:PM_LANE
        Write-Output "push: lane output build3ds/lane-$($env:PM_LANE)/pm_3ds.3dsx (no build)"
    } else {
        Remove-Item Env:PM_LANE -ErrorAction SilentlyContinue
        Write-Output "push: default output build3ds/pm_3ds.3dsx"
    }
```
2. push_3ds.sh :176: `now=$(PM_LANE=${PM_KEY_LANE:-} PM_KEY_ONLY=1 sh 3ds/build.sh || true)` (PUSHLANE: the key of the lane folder
   that built the 3dsx). Change nothing else on the line.
3. New file 3ds/tests/push/lane.sh (LF line ends):
```
#!/bin/sh
# PUSHLANE test: run after `PM_LANE=<lane> sh 3ds/build.sh` in a worktree, with PM_LANE=<lane>. A fake 3dslink, no console.
set -u
cd "$(dirname "$0")/../../.."
[ -n "${PM_LANE:-}" ] && [ -f "build3ds/lane-$PM_LANE/pm_3ds.3dsx" ] || { echo "LANE TEST FAIL: no lane build of PM_LANE='${PM_LANE:-}'"; exit 1; }
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
printf '#!/bin/sh\necho "Sending pm_3ds.3dsx"\nexit 0\n' >"$tmp/fake_ok.sh"; chmod +x "$tmp/fake_ok.sh"
o=$(env PM_3DSLINK="$tmp/fake_ok.sh" PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" \
    powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -Fg -ip 127.0.0.1 -Seconds 5 -IdleSeconds 5 2>&1) || true
if echo "$o" | grep -q "SEND FILE: .*lane-$PM_LANE/pm_3ds.3dsx" && ! echo "$o" | grep -q 'STALE 3DSX' && echo "$o" | grep -q '^Sending'; then echo "LANE TEST PASS"; exit 0; fi
echo "LANE TEST FAIL"; echo "$o" | tail -n 20; exit 1
```
4. Commit: `PUSHLANE: push -NoBuild sends the lane build of PM_LANE with that lane's key check`.
## Gate (expected values)
1. `PM_ROW=PUSHLANE PM_LANE=PUSHLANE sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: build OK, memcheck OK.
2. Seeded fault: make edit 3 only. `PM_LANE=PUSHLANE sh 3ds/tests/push/lane.sh > build3ds/out/lane_before.txt 2>&1`. Expected
   `LANE TEST FAIL` (the output shows `push: default output` and no build3ds/pm_3ds.3dsx or a STALE line).
3. Make edits 1-2, build again as in step 1, run lane.sh into build3ds/out/lane_test.txt. Expected `LANE TEST PASS`.
4. `PM_LANE=PUSHLANE sh 3ds/tests/push/run.sh > build3ds/out/push_test.txt 2>&1`: `PUSH TESTS PASS`. No console push, no Azahar
   start. Counter: none (tool row).
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit. Hand back: the ready hash and the test outputs.
