# ws_TSTSHK (Haiku, row TSTSHK2): tst_13 shockwave lists move to device keys, entity_model.c dead arm goes (C1 -1, C2 -1, C5 -115)
Designer: the oracle (SendMessage). Worktree build3ds/wt/TSTSHK2, branch TSTSHK2 from main, PM_ROW=TSTSHK2 PM_LANE=TSTSHK2.
Main at release: 52f9b15c42. Start: now. Do not use the old branch TSTSHK. Edit only the 4 files of docs/handoff/ws_TSTSHK.patch:
src/world/area_tst/tst_13/main.c, src/entity_model.c, new 3ds/tools/gfxin/src/world/area_tst/tst_13/main.c, new 3ds/tools/gset.d/TSTSHK.txt.
## Gate decision (oracle, 2026-11-17)
- FACT: bench tst13_drec cannot run. pm_warp.c:747-750 ignores a warp to a `tst_` map (PT01b, cut test maps). The patch has no bench.
- Proof of the tst_13 part is the host test. FACT (oracle host run, main + this patch + 3 other pages, PM_HOSTSEL=all): ND HOST TESTS
  PASS, `ntab tables 1984 draws 36795 bad 0`, `gset test ok`, tst_13_shockwave_gfx and tst_13_dummy_gfx are in gset_ok.txt.
- FACT: mdl_project_tex_coords (main.c:301) reads the list only through mdl_get_vertex_count. model.c:3639 reads a key through
  nd_ntab_find and nd_vcount. The host unit vcount.sh tests nd_vcount with a seeded fault.
- FACT (entity_model.c:344-345, :357, :467): entRec is set when USE_IMAGE is clear. :467 is in the block of :357 (same test), so the
  gSPDisplayList arm at :470 never runs. Swap scene for the gate: entcape_kmr03_drec (entities) and spot_mac01_drec.
- Counters (FACT, oracle listcen on main + patch): build 341 -> 340, gfxpos 44 -> 43, data 482 -> 367, data_arrays 15 -> 13,
  gfxtype 428 -> 427, recdl 11.
## Edit shapes (the patch holds every edit)
- tst_13/main.c:278. Before: `#include "world/area_tst/tst_13/shockwave.gfx.inc.c"`. After: `NDN_DECL(N(shockwave_gfx));`
- main.c:301. Before: `mdl_project_tex_coords(MODEL_o152, N(shockwave_gfx), sp10, nullptr);`
  After: `mdl_project_tex_coords(MODEL_o152, NDN(N(shockwave_gfx)), sp10, nullptr);`
- entity_model.c:467-471. Before: `if (entRec != nullptr) { nd_rec_ent_draw(...); } else { gSPDisplayList(gMainGfxPos++, ...); }`
  After: `nd_rec_ent_draw(model->gfx.displayList, entRec[1].words.w0, entRec[1].words.w1, 0);`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_TSTSHK.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 4 files: `TSTSHK2 1: tst_13 shockwave lists to device keys (gset.d TSTSHK), entity_model.c dead G_DL arm`.
3. `sh 3ds/tools/rowloop.sh --row TSTSHK2 --bench entcape_kmr03_drec,spot_mac01_drec --why "TSTSHK2 gate" > build3ds/out/tstshk2_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal.
- `grep -c "tst_13_shockwave_gfx" build3ds/out/gset_ok.txt` prints 1 or more.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 340, gfxpos 43, data 367, data_arrays 13, gfxtype 427.
## What you may edit to fix a fail
Only the 4 files. A host FAIL in a unit that names tst_13, a DIFF or a base fail: stop. Send the output path to the oracle.
Budget: 1 build, 4 Azahar starts.
## Stop and hand-back
Last commit `TSTSHK2 2: gate. entcape_kmr03_drec, spot_mac01_drec byte-equal; tst_13 proof host (tst13_drec blocked by pm_warp.c:747
PT01b); build 340 gfxpos 43 data 367` deletes ws_TSTSHK.md and ws_TSTSHK.patch. One SubagentHandback: hashes, LOOP lines, totals.
