# ws_LCNOBUILD (Haiku): listcen does not count an asset whose only include is commented out (C5 -20, C6 -2)
Worktree build3ds/wt/LCNOBUILD, branch LCNOBUILD. Start now. No shared file with CGFX, DLSITE1 or MODELRM.
## Objective and counters
- FACT (listcen on main 93a5a7bd50): 2 asset files have only a commented include. They are not in the device build.
  assets/us/effects/gfx/D_09000800_3543B0.gfx.inc.c (src/effects/gfx/flame.c:11) has 17 data, 1 array.
  assets/us/effects/gfx/D_0900C328_3F8188.gfx.inc.c (src/effects/gfx/chapter_change.c:49) has 3 data, 1 array.
- C5 data 2239 -> 2219. C6 data_arrays 211 -> 209. FACT: oracle run of the patched listcen on main 93a5a7bd50.
## Files (this order). Edit only these.
1. 3ds/tools/listcen.py (patch). 2. 3ds/tests/host.d/lcnobuild.sh (new, patch).
## Edit (all of it is in docs/handoff/ws_LCNOBUILD.patch)
Before, listcen.py:259:
    assets_c = [p for p in assets_c if not _in_twin(p)]
After (the patch adds lines after it):
    assets_c = [p for p in assets_c if not _in_twin(p)]
    # LCNOBUILD: an asset whose only include in src/ and include/ is commented out ... is not in the device build
    if NOBUILD_RULE:
        ... (live set from LIVE_INC_RX, dead set from DEAD_INC_RX)
        assets_c = [p for p in assets_c if not _unbuilt(p)]
The patch also adds NOBUILD_RULE, LIVE_INC_RX and DEAD_INC_RX above SRC_GROUPS (listcen.py:40).
## Loop (one group, one commit)
1. `python 3ds/tools/listcen.py > build3ds/out/lcnobuild_lc0.txt` (before the patch).
2. `tr -d '\r' < docs/handoff/ws_LCNOBUILD.patch | git apply -` (the checkout gives the patch CRLF ends). If it fails, stop and send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/lcnobuild_host.txt 2>&1`. Read the last 20 lines.
4. `python 3ds/tools/listcen.py > build3ds/out/lcnobuild_lc1.txt`.
   `diff build3ds/out/lcnobuild_lc0.txt build3ds/out/lcnobuild_lc1.txt > build3ds/out/lcnobuild_diff.txt`.
5. Do not build. Do not run Azahar. Do not run bench.py. listcen.py and host.d are not in the ELF. No device byte changes.
6. Delete docs/handoff/ws_LCNOBUILD.md and docs/handoff/ws_LCNOBUILD.patch. Commit the 2 changed files and the 2 deletions.
## Pass (all must be true)
- lcnobuild_host.txt has `lcnobuild ok` and `ND HOST TESTS PASS`. The test has its own seeded fault: a copy with NOBUILD_RULE = False must count 2.
- lcnobuild_diff.txt has exactly 4 changed lines: `group assets/us data` -20, `group assets/us data_arrays` -2,
  `total data` -20, `total data_arrays` -2. If main moved, the start values can differ. The changes must be -20 and -2.
## Fail
- A failing test: you may edit only the 2 files above. Make 2 tries for one cause. Then write the output to build3ds/out/
  and send the path to the oracle with SendMessage.
- A diff with another line: stop. Send lcnobuild_diff.txt to the oracle.
## Stop
Stop after the commit. Commit text:
`LCNOBUILD 1: listcen does not count an asset whose only include is commented out (C5 <a> -> <b>, C6 <c> -> <d>); host test lcnobuild.sh with a seeded fault. Page deleted.`
## Hand-back (one SubagentHandback message)
Commit hash. `lcnobuild ok` line. The 4 diff lines. C5 and C6 before and after.
