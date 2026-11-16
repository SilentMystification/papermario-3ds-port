# ws_WSCMB (Haiku): window style combine words become NdCmbW values (C5 data -114, data_arrays -1, gfxtype -4)
Designer: the oracle (SendMessage). Worktree build3ds/wt/WSCMB, branch WSCMB. PM_ROW=WSCMB PM_LANE=WSCMB. Start now.
No shared file with MODELRM, DLSITE2, GMERLEE, FXWRAP.
## Objective and counters
- FACT (code read): WindowStyleCustom has `Gfx opaqueCombineMode` and `Gfx transparentCombineMode` (include/common_structs.h:2208-2209).
  Its only reader is src/draw_box.c:460-462: it hands the 2 words to nd_gx_w. gBoxCombineModes (src/draw_box.c:314-319) is the same.
  The writers use gsDPSetCombineMode or gsDPNoOp as values: pause_styles.c 58, filemenu_styles.c 46, tattle_window.c 2, kkj_19/main.c 2, kmr_23/npc.c 2.
- The patch: type NdCmbW {u32 w0, w1} (common_structs.h), macros ND_CMB_W and ND_CMB_NOP (3ds/include/nd_gbi.h, after gsDPSetCombineLERP).
  draw_box.c reads `cw->w0, cw->w1`. The call `nd_gx_w(gMainGfxPos++, ..)` stays.
- FACT (oracle host run of 3ds/tests/host.d/wscmb.sh): all 18 pairs in use give the same 2 words as before (`wscmb 18 pairs bad 0`). The 6 C files pass gcc -fsyntax-only.
- FACT (oracle listcen run, main 3771a3edbb with the patch): data 1942 -> 1828 (-114), data_arrays 182 -> 181, gfxtype 794 -> 790.
  build, gfxpos, recdl, reader, model: 0. Bench difference: none (0 differing bits; the data words are equal).
## Files (all in docs/handoff/ws_WSCMB.patch). Edit only these 12.
include/common_structs.h, 3ds/include/nd_gbi.h, src/draw_box.c, src/pause/pause_styles.c, src/filemenu/filemenu_styles.c,
src/effects/tattle_window.c, src/world/area_kkj/kkj_19/main.c, src/world/area_kmr/kmr_23/npc.c, new: 3ds/tests/wscmb/wscmb.c,
3ds/tests/host.d/wscmb.sh, 3ds/tools/azt.d/WSCMB.json, 3ds/tools/bench.d/WSCMB.json.
## Edit shape (the patch does it; use it to check a reject)
Before (src/effects/tattle_window.c:26): `    .opaqueCombineMode = gsDPSetCombineMode(PM_CC_15, G_CC_PASS2),`
After: `    .opaqueCombineMode = ND_CMB_W(PM_CC_15, G_CC_PASS2),`
Before (src/pause/pause_styles.c:265): `    .opaqueCombineMode = gsDPNoOp(),`. After: `    .opaqueCombineMode = ND_CMB_NOP,`
Before (src/draw_box.c:462): `nd_gx_w(gMainGfxPos++, cw->words.w0, cw->words.w1);`. After: `nd_gx_w(gMainGfxPos++, cw->w0, cw->w1);`
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. `python 3ds/tools/listcen.py > build3ds/out/wscmb_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_WSCMB.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/wscmb_host.txt 2>&1`. Pass: `wscmb ok`, `ND HOST TESTS PASS`.
4. `python 3ds/tools/listcen.py > build3ds/out/wscmb_lc1.txt`. Pass vs wscmb_lc0.txt: total data -114, data_arrays -1, gfxtype -4,
   no other total line changes. Commit the 12 files: `WSCMB 1: window style combine words are NdCmbW values; wscmb host test; 2 drec benches`.
5. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
6. `python 3ds/tools/bench.py base wscmb_pause_drec` and `python 3ds/tools/bench.py base wscmb_fsel_drec`.
   Pass: each base log has its drec line and `game mode 10 frame` (pause) or `game mode 14 frame` (file select).
7. Gate, for b in wscmb_pause_drec wscmb_fsel_drec: `python 3ds/tools/bench.py run $b --row WSCMB --why "WSCMB gate"`, then
   `python 3ds/tools/bench.py cmp $b --row WSCMB --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
## Fail
- A failing test or a compile error: you may edit only the 12 files. Compare each with the patch. 2 tries for one cause.
  Then write the output to build3ds/out/ and send the path to the oracle. A byte diff in a gate: stop, send the cmp output path.
- A base run that fails or hangs (new scenario): stop. Send the log path to the oracle.
## Stop
Both gates pass. The last commit `WSCMB 2: gate` deletes ws_WSCMB.md and ws_WSCMB.patch.
## Hand-back (one SubagentHandback message)
Commits. `wscmb` line. listcen data, data_arrays, gfxtype before and after. memcheck margin. The 2 cmp lines.
