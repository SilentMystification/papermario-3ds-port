# ws_SHIMMER (Haiku): the 2 star power shimmer lists leave the device (counters C5 data, C6 data_arrays)
Designer: the oracle (SendMessage). Worktree build3ds/wt/SHIMMER, branch SHIMMER, PM_ROW=SHIMMER PM_LANE=SHIMMER. Start now.
No shared file with another page.
## Objective and counters (FACT: oracle dry run on main 023db9ca81, listcen.py totals)
- C5 data 1207 -> 1183 (-24). C6 data_arrays 92 -> 90 (-2). All other listcen totals do not change.
- The moved lists: D_80109710 (assets/us/A2BC0.gfx.inc.c, 19 data) and D_801097D8 (assets/us/A2C88.gfx.inc.c, 5 data).
## Cause (FACT, code read)
- src/status_star_shimmer.c is in 3ds/tools/gset.d/GSETI.txt:7. Both lists are in gset_ok.txt (lines 1662-1663).
- gset.py:33 INC needs an include line with no trailing text. status_star_shimmer.c:20-21 have a `// ...` comment. So gset
  moved 0 lists. Without the comments, gset.py prints `gset moved 2 files 1 keep 0` and `gset ok`.
## Edit (all in docs/handoff/ws_SHIMMER.patch, 2 files; edit only these)
- src/status_star_shimmer.c and the new twin 3ds/tools/gfxin/src/status_star_shimmer.c.
- Shape. Before (status_star_shimmer.c:20): `#include "A2BC0.gfx.inc.c" // Gfx_StatusShimmer_SetupTex`.
  After: `NDN_DECL(D_80109710);`. The include moves to the twin.
- Shape. Before (:189): `gSPDisplayList(gMainGfxPos++, D_801097D8); // draw particle`.
  After: `gSPDisplayList(gMainGfxPos++, NDN(D_801097D8)); // draw particle` (same form as curtains.c:155).
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_SHIMMER.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. `python 3ds/tools/listcen.py > build3ds/out/shim_lc.txt`. Pass: `listcen total data 1183` and
   `listcen total data_arrays 90` (main 023db9ca81 values; on a newer main: data -24 and data_arrays -2 against main).
3. Commit the 2 files: `SHIMMER 1: star power shimmer lists to the gfxin twin (data -24, data_arrays -2)`.
4. `sh 3ds/tools/rowloop.sh --row SHIMMER --bench spot_mac01_drec --why "SHIMMER: shimmer lists moved" > build3ds/out/shim_loop.txt 2>&1`.
## Pass and fail
- Pass: shim_loop.txt has `ND HOST TESTS PASS`, `host rc 0, build rc 0`, `cmp spot_mac01_drec: ... byte-equal` and `LOOP PASS`.
- Reach (code read): inventory.c:1150 calls star_power_shimmer_draw each world frame when statusBar->disabled is 0
  (inventory.c:695). That runs nd_draw_list(NDN(D_80109710)). GUESS: disabled is 0 at mac_01 frames 200-201.
- OPEN: no bench reaches the particle draw (:189). It needs add_star_power (inventory.c:2018). No debug switch calls it
  (FACT grep 3ds/src). The link of ndd_D_801097D8 and the host table (gconv def D_801097D8) are the proof for it.
- Host fail or build fail: you may edit only the 2 files of the patch. 2 tries per cause. Then send the output path to the oracle.
- cmp DIFF: do not change the bench or the base. Send build3ds/out/loop_SHIMMER_cmp_spot_mac01_drec.txt to the oracle.
Budget: 1 rowloop call (1 build, at most 2 Azahar runs). No other host run.
## Stop and hand-back
Last commit `SHIMMER 2: gate` deletes ws_SHIMMER.md and ws_SHIMMER.patch. One SubagentHandback: commit hashes, the 2 listcen
total lines, the LOOP summary lines (host, build, cmp, LOOP TIME).
