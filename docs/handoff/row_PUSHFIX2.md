# Row PUSHFIX2: a push is killed only when its send stops making progress (no fixed 60 s send limit)

(Model: Haiku. One loop of 3ds/push_3ds.sh and two host test cases; every edit is below. No console run.)

## User facts (2026-11-12)
Pushes stall at a random percentage. The XL Luma dump 02 (data abort, far 0, process 3dsx_app) came from a push.

## Cause (orchestrator and ORACLE7 code read, main e2f67f9d3)
- push_3ds.sh:360 `send_secs=${SEND_SECS:-60}`; :437-440: once the console answered (`started` 1), the attempt is killed when
  `sent_for` reaches 60 s, whatever the progress. The comment :356-359 sizes it for a 13 MB 3dsx; the 3dsx is now 38 to 51 MB, so a
  healthy Wi-Fi send is cut at a random percentage.
- :342-348 says a kill after the console answered leaves the console running corrupted code (data abort, null-ish address): the
  XL dump 02 matches. User decision 2026-11-12 (through the orchestrator): a size-agnostic 60 s kill is wrong; kill only a send
  that stopped making progress, with a hard cap that grows with the file size.
- Progress is seen as growth of the attempt's output file (3dslink prints `N sent (P%)` per block). It runs under `stdbuf -oL`
  (:388, :395), which flushes only at a newline; progress lines end with `\r`, so the file may not grow during a send. Use
  `stdbuf -o0` so every progress print reaches the file.
- go.ps1 sets no environment of its own for SEND_SECS or NOGROW_SECS and clears none: the bash child (and the detached cmd child)
  inherits the caller's environment. No go.ps1 edit.

## Start
`sh 3ds/tools/wt_new.sh PUSHFIX2`. PM_ROW=PUSHFIX2.

## Touch only
3ds/push_3ds.sh (the comment block :353-360, the attempt line :381, the two `stdbuf -oL` at :388 and :395, the kill test :437-440,
the owner comment :427), 3ds/tests/push/run.sh (two new cases before the summary).

## Exact edits
1. Replace :356-360 with: a comment naming the user decision 2026-11-12 (a fixed send time kills healthy sends of a large 3dsx;
   kill only when no progress for NOGROW_SECS, plus a hard cap of 15 s per MB, at least 120 s; SEND_SECS overrides the cap), then
   `nogrow_secs=${NOGROW_SECS:-60}`, `size_mb=$(( ($(wc -c <"$SRC3DSX") + 1048575) / 1048576 ))`,
   `send_cap=$(( size_mb * 15 )); [ "$send_cap" -lt 120 ] && send_cap=120`, `send_cap=${SEND_SECS:-$send_cap}`.
2. :381 text: `(connect timeout ${stall_secs}s, no-progress ${nogrow_secs}s, send cap ${send_cap}s for ${size_mb} MB)`.
3. :388 and :395: `stdbuf -oL` becomes `stdbuf -o0`.
4. :437-440 becomes two arms: `elif [ "$started" -eq 1 ] && [ "$nogrow" -ge "$nogrow_secs" ]` -> echo `attempt $i: no send
   progress for ${nogrow_secs}s - killing 3dslink (last line: <tail -n 1 of outfile, \r as newline>)`, stage=send,
   reason="no send progress for ${nogrow_secs}s", killed=1; `elif [ "$started" -eq 1 ] && [ "$sent_for" -ge "$send_cap" ]` ->
   echo `attempt $i: send not finished after ${send_cap}s (cap) - killing 3dslink`, stage=send, reason="send cap ${send_cap}s",
   killed=1. The 30 s warning stays. :427 comment: "its own end, the no-progress kill or the cap".
5. run.sh, two cases with fake 3dslink scripts in $tmp, each `env PM_3DSLINK=<fake> PM_SRC3DSX="$tmp/pm_3ds.3dsx"
   PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" NOGROW_SECS=4 sh 3ds/push_3ds.sh --no-build --ip 127.0.0.1 -t 1 --idle 1`:
   (e) slow: prints `Sending pm_3ds.3dsx`, then `<n> sent (<p>%)` every 2 s for 16 s, then `100.00%` and exits 0. Expect: output
       has no `killing 3dslink`, has `send cap 195s for 13 MB` (the dummy file is 13359084 bytes), and the run takes 15 s or more.
   (f) hung: prints `Sending pm_3ds.3dsx` and one `10 sent (5%)`, then sleeps 60 s. Expect: `no send progress for 4s - killing
       3dslink` in the output and the whole case under 45 s (3 attempts of about 6 s each; without the kill one attempt takes 60 s).
   Print `ok   slow send not killed` / `ok   hung send killed` or FAIL lines, set fail=1 on a FAIL.

## Gate (expected values)
- `sh 3ds/tests/push/run.sh > build3ds/out/push_tests.txt 2>&1`: every old case and (e), (f) print ok; the last line is
  `PUSH TESTS PASS`. Hand back the file's last 12 lines.
- `sh 3ds/push_3ds.sh --dry --no-build`: rc 0, no build line.
- No Azahar start, no console. Budget 0.

## Stop
A failing case: stop, commit, hand back the output (2 tries per named cause). Delete this row file in the completing commit.
