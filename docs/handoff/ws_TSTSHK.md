# ws_TSTSHK (Haiku): tst_13 shockwave lists move to device keys, entity_model.c dead arm goes (C5 -115, C1 -1, C2 -1)
Designer: the oracle (SendMessage). Worktree build3ds/wt/TSTSHK, PM_ROW=TSTSHK PM_LANE=TSTSHK. Main at release: c687bd4660. FACT: `git apply --check` OK on main and on main + FXOPS + FXLT + ENTCAPE patches.
Start: now. Files (all in docs/handoff/ws_TSTSHK.patch, 6 files): src/world/area_tst/tst_13/main.c, src/entity_model.c,
new 3ds/tools/gfxin/src/world/area_tst/tst_13/main.c, new 3ds/tools/gset.d/TSTSHK.txt, new 3ds/tools/azt.d/TSTSHK.json,
new 3ds/tools/bench.d/TSTSHK.json. No other page edits them. Edit only these.
## Objective
- FACT (gset_ok.txt of main): tst_13_shockwave_gfx and tst_13_dummy_gfx convert ok. GSETI 3a (63681c790b) held them back only
  because main.c:301 passes the list to mdl_project_tex_coords. FACT: that function reads the list only through
  mdl_get_vertex_count (model.c:3865), and VCOUNT (model.c:3639) reads a moved key (NDN) through its device units.
- FACT (entity_model.c:344-345, :357, :467): entRec is set when USE_IMAGE is clear. :467 sits in the block of :357, the same test.
  So the gSPDisplayList arm at :470 never runs. The patch deletes it.
- Counters (FACT, oracle listcen on the patched tree): build 386 -> 385, gfxpos 89 -> 88, data 1183 -> 1068,
  data_arrays 90 -> 88, gfxtype 580 -> 579, recdl 11 (no change).
## Edit shapes (the patch holds every edit; these are for reading only)
- tst_13/main.c:278. Before: `#include "world/area_tst/tst_13/shockwave.gfx.inc.c"`. After: `NDN_DECL(N(shockwave_gfx));`
  (the include moves to the twin). main.c:301. Before: `mdl_project_tex_coords(MODEL_o152, N(shockwave_gfx), sp10, nullptr);`
  After: `mdl_project_tex_coords(MODEL_o152, NDN(N(shockwave_gfx)), sp10, nullptr);`
- entity_model.c:467-471. Before: `if (entRec != nullptr) { nd_rec_ent_draw(...); } else { gSPDisplayList(gMainGfxPos++, ...); }`
  After: `nd_rec_ent_draw(model->gfx.displayList, entRec[1].words.w0, entRec[1].words.w1, 0);`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_TSTSHK.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 6 files: `TSTSHK 1: tst_13 shockwave lists to device keys (gset.d TSTSHK), entity_model.c dead G_DL arm, bench tst13_drec`.
3. `python 3ds/tools/bench.py base tst13_drec > build3ds/out/tstshk_base.txt 2>&1`. A base fail is a tool fault: stop (rule 4).
4. `sh 3ds/tools/rowloop.sh --row TSTSHK --bench tst13_drec,spot_mac01_drec --why "TSTSHK gate" > build3ds/out/tstshk_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal. FACT (oracle host run, patched tree): ND HOST TESTS PASS,
  `ntab tables 1934 draws 36469 bad 0`, `gset test ok`.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 385, gfxpos 88, data 1068, data_arrays 88, gfxtype 579.
- GUESS: the shockwave floor (MODEL_o152, builder main.c:324-325) is in view at tst_13 entry 0 (0, 0, 100).
  OPEN: the oracle did not run the new bench. The base run of step 3 is its first run.
- Seeded fault (new bench self-test): in tst_13/main.c:295 change `+ 0.1;` to `+ 0.2;` (uncommitted). Run
  `sh 3ds/tools/rowloop.sh --row TSTSHK --bench tst13_drec --no-host`. The cmp must say DIFF. Undo it (git checkout the file).
## What you may edit to fix a fail
Only the 6 files. A base run that fails or hangs, or a DIFF of the real edit: stop. Send
build3ds/out/loop_TSTSHK_cmp_<bench>.txt to the oracle. Budget: 2 builds (edit, seeded fault), 6 Azahar starts.
## Stop and hand-back
Last commit `TSTSHK 2: gate. tst13_drec, spot_mac01_drec byte-equal; seeded fault DIFF; build 385 gfxpos 88 data 1068`
deletes ws_TSTSHK.md and the patch. One SubagentHandback: commit hashes, the LOOP lines, the listcen totals, the fault cmp line.
