# ws_DLSITE2 (Haiku): codemod reads past a comment (ctx keep), 28 value sites and 4 list sites in 14 files (C1 -32, C2 -32)
Designer: the oracle (SendMessage). Worktree build3ds/wt/DLSITE2, branch DLSITE2. PM_ROW=DLSITE2 PM_LANE=DLSITE2. Start now. No shared file with CURT, MODELRM, FXHOLD, FXREN, GMERLEE. Do not edit src/model.c (MODELRM owns it).
## Objective and counters
- FACT (code read, 3ds/tools/codemod.py:277-289): the previous token of a call can be a comment. Then the call is kept as `ctx`.
  The patch skips comment tokens before the call (codemod.py:277). Host case 3ds/tests/codemod/cases/st_ctxcmt.* proves it.
- FACT (oracle run of the patched tool, main 4c02b18543): 28 ctx sites and 4 `gSPDisplayList` sites of v2 tables become native.
  Oracle listcen count of the 14 files: build 55 -> 23, gfxpos 58 -> 26. data, data_arrays, gfxtype, recdl: 0. Main 4c02b18543: build 551, gfxpos 186. Pass after the row: 519, 154 (if main is unchanged).
## Files (all in docs/handoff/ws_DLSITE2.patch). Edit only these 20.
3ds/tools/codemod.py, 3ds/tests/codemod/cases/st_ctxcmt.{in.c,out.c,log}, 3ds/tools/azt.d/DLSITE2.json, 3ds/tools/bench.d/DLSITE2.json,
src/imgfx.c, src/background_gfx.c, src/hud_element.c, src/screen_overlays.c, src/main_loop.c, src/status_star_shimmer.c, src/inspect_icon.c,
src/pause/pause_partners.c, src/speech_bubble.c, src/effects/{sparkles,hieroglyphs,blast,firework}.c, src/world/area_flo/flo_14/bubbles.c.
## Edit shapes (the patch does them; use them to check a reject)
Before (src/imgfx.c:917): `gDPSetCombineMode(gMainGfxPos++, G_CC_MODULATEIDECALA_PRIM, G_CC_MODULATEIDECALA_PRIM);`
After: `nd_pso_cc(G_CC_MODULATEIDECALA_PRIM, G_CC_MODULATEIDECALA_PRIM);` (same form as src/imgfx.c:926).
Before (src/hud_element.c:1976): `gDPPipeSync(gMainGfxPos++);`. After: the line is gone (nd_gx_w drops a sync, 3ds/src/nd_rec.c:469).
Before (src/status_star_shimmer.c:170): `gSPDisplayList(gMainGfxPos++, D_80109710); // setup texture`.
After: `nd_draw_list(D_80109710, ND_SELF); // setup texture`, and `#include "nd_gtab.h"` after line 1 (ND_SELF: 3ds/include/nd_gtab.h:88). Added includes: nd_st.h in sparkles.c, nd_xf.h in main_loop.c and bubbles.c, nd_gtab.h in status_star_shimmer.c.
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/dls2_host0.txt 2>&1`. `python 3ds/tools/listcen.py > build3ds/out/dls2_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_DLSITE2.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tests/codemod/run.sh > build3ds/out/dls2_cm.txt 2>&1`. Pass: `codemod: seeded fault caught` and `codemod: 47/47 PASS`.
   Self-test of the new rule: `git show main:3ds/tools/codemod.py > build3ds/out/cm_old.py`, copy cases/st_ctxcmt.in.c to
   build3ds/out/st_ctxcmt.c, run `python cm_old.py --state st_ctxcmt.c` in build3ds/out. Pass: 4 `codemod keep` lines (the new tool gives 1).
4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/dls2_host1.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `lkey census ok`.
   The 4 names inspect_icon_gfx, gPausePartnersDL, speech_bubble_gfx, D_80109710 are lines of build3ds/out/ntab_tables.txt.
5. `python 3ds/tools/listcen.py > build3ds/out/dls2_lc1.txt`. Pass vs dls2_lc0.txt: build -32, gfxpos -32, no other total changes.
   Commit the 20 files: `DLSITE2 1: codemod skips a comment before the call; 32 sites in 14 files`.
6. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
7. `python 3ds/tools/bench.py base spot_mac01_drec` and `python 3ds/tools/bench.py base dls2_fx_drec`.
8. Gate, for b in spot_mac01_drec dls2_fx_drec: `python 3ds/tools/bench.py run $b --row DLSITE2 --why "DLSITE2 gate"`, then
   `python 3ds/tools/bench.py cmp $b --row DLSITE2 --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
   Coverage: main_loop.c:202 runs every frame (FACT). dls2_fx_drec runs fxtest 12 sparkles, 56 blast, 61 firework, 106 hieroglyphs
   (FACT, 3ds/src/pm_fxtest.c FXLIST order). imgfx, hud_element, pause, shimmer, flo_14 bubbles: not proven by a bench (GUESS).
## Fail
- A compile error in one of the 14 src files: add the missing nd_*.h include to that file only. 2 tries. Then send the output path.
- A byte diff in a gate: stop. Send the cmp output path to the oracle.
## Stop
Both gates pass. The last commit `DLSITE2 2: gate` deletes ws_DLSITE2.md and ws_DLSITE2.patch.
## Hand-back (one SubagentHandback message)
Commits. listcen build and gfxpos before and after. codemod run.sh line. memcheck margin. The 2 cmp lines.
