# ws_NMDL6 (Sonnet, user yes 2026-11-17): the last list calls of entity_model.c, model.c and 3 world files go (C1 build -7, C2 gfxpos -7, C7 gfxtype -1)
Designer: the oracle (SendMessage). Worktree build3ds/wt/NMDL6, branch NMDL6 from main, PM_ROW=NMDL6 PM_LANE=NMDL6.
Main at release: 3ad3164cdd. Start: now, parallel with VSLOT3 (no shared file). Edit only the 5 files of docs/handoff/ws_NMDL6.patch:
src/entity_model.c, src/model.c, src/world/area_flo/flo_10/waterfx.c, src/world/area_mac/mac_00/water_gfx.c, src/world/area_mac/mac_01/crystal_ball_gfx.c.
## Objective
- entity_model.c:490 (reflect arm of appendGfx_entity_model) takes the producer of the main draw (:469): nd_rec_ent_draw with the words of entRec[1] (:348-349).
- entity_model.c:772 and :784 (draw_entity_model_E) take the same producer. The new locals entSeg and entArr hold the vertex segment words (as :348-349).
- model.c:4340, waterfx.c:231, water_gfx.c:20, crystal_ball_gfx.c:95 call a map model list: NDCTL_CALL(nd_nr, list) (nd_at.h:35). model.c:4325 `Gfx* dl` -> `NdLst dl`.
- FACT (code): for a list in gMapShapeData, nd_gx_w records nd_rec_nr(ND_OP_CALL_W0, list) (nd_rec.c:400 rec_inline returns 0, :470; nd_op.h:107, :135). NDCTL_CALL(nd_nr, l) records the same unit. pbuf.c proves the unit words (pbuf ok).
- FACT (grep): no code sets ENTITY_MODEL_FLAG_CAM3, so draw_entity_model_E returns at :676. No scene reaches :772 :784. The build is their proof.
- FACT (code): direct_pra13_det reaches :490: pra_13 main.c:166-168 -> Reflection.data.inc.c:33-35 -> Reflection.inc.c:82-84 sets REFLECT on the player shadow model each frame.
- GUESS: no det scene draws the 3 world lists or the end_00 parade (wide only). spot_mac01_drec is the regression scene.
- FACT (oracle listcen, main + patch): build 324 -> 317, gfxpos 22 -> 15, gfxtype 420 -> 419. data 323 does not move.
- FACT (oracle, main + this patch + ws_VSLOT3.patch, PM_CACHE=off): ND HOST TESTS PASS, `pbuf ok`, `nmdl0 ok 30855`, `lkey census ok files 14 lines 121`.
## Edit shapes (real code)
- entity_model.c:490. Before: `gSPDisplayList(gMainGfxPos++, model->gfx.displayList);` After: `nd_rec_ent_draw(model->gfx.displayList, (model->vertexArray != nullptr) ? (u32)D_80154374 : 0u, (model->vertexArray != nullptr) ? (u32)VIRTUAL_TO_PHYSICAL(model->vertexArray) : 0u, 0);`
- water_gfx.c:20. Before: `gSPDisplayList(gMainGfxPos++, model->modelNode->displayData->displayList);` After: `NDCTL_CALL(nd_nr, model->modelNode->displayData->displayList);` plus `#include "nd_at.h"`.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_NMDL6.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 5 files: `NMDL6 1: entity reflect and E draws take nd_rec_ent_draw, map model list calls take NDCTL_CALL (build 317 gfxpos 15 gfxtype 419)`.
3. `PM_CACHE=off sh 3ds/tools/nd_host_test.sh > build3ds/out/nmdl6_host.txt 2>&1`. With the cache on, nmdl0.sh can fail on a missing /tmp/ndcombs.c (FACT, oracle run: a hostsel tool fault).
4. `python 3ds/tools/bench.py base direct_pra13_det,spot_mac01_drec --row NMDL6 > build3ds/out/nmdl6_base.txt 2>&1` (base runs, not charged).
5. `sh 3ds/tools/rowloop.sh --row NMDL6 --bench direct_pra13_det,spot_mac01_drec --no-host --why "NMDL6 gate" > build3ds/out/nmdl6_loop.txt 2>&1`.
## Tests that decide
- `ND HOST TESTS PASS` in nmdl6_host.txt, with `pbuf ok` and `lkey census ok`.
- `LOOP PASS`: build rc 0, both cmp lines byte-equal (base = merge-base of HEAD and main).
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 317, gfxpos 15, gfxtype 419 (alone on main; with VSLOT3 merged: 315, 13, 417).
## What you may edit to fix a fail
- A build error: fix the one line it names, 1 try.
- direct_pra13_det DIFF (one try): in entity_model.c replace the 3 nd_rec_ent_draw calls of the patch with `NDCTL_CALL(nd_nr, model->gfx.displayList);`, delete the
  2 lines `const u32 entSeg ...` and `const u32 entArr ...`, add `#include "nd_at.h"` after `#include "nd_op.h"`. FACT: this records the old unit (nd_rec.c:453-470) for a list with a geometry unit.
  Commit `NMDL6 2: reflect draws take NDCTL_CALL`. Run step 5 again.
- Any other fail: stop. Send the path of build3ds/out/loop_NMDL6_cmp_<bench>.txt or nmdl6_host.txt to the oracle. Budget: 2 builds, 4 Azahar starts.
## Stop and hand-back
Last commit `NMDL6 <n>: gate. direct_pra13_det, spot_mac01_drec byte-equal; build 317 gfxpos 15 gfxtype 419` deletes ws_NMDL6.md and ws_NMDL6.patch.
One SubagentHandback: commit hashes, LOOP lines, listcen totals.
