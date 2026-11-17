# ws_GTSMALL (Haiku): 9 Gfx tokens in 6 files leave, same code (C7 gfxtype -9)
Designer: oracle NEXT2 (SendMessage). Worktree build3ds/wt/GTSMALL, branch GTSMALL from main, PM_ROW=GTSMALL PM_LANE=GTSMALL.
Main at release: 1aa6e7fbc6. Edit only src/pause/pause_gfx.c, src/battle/area/kzn2/actor/lava_piranha.c, src/world/common/lava_piranha/part2.inc.c,
src/entity.c, src/evt/map_api.c, 3ds/src/nd_direct.c. Shares no file with ENTLST, EFXGT, RECBUF, EFFCEN, DRECTEX, FLTGT, BLANKET. Model: Haiku.
## Objective
- Every edit keeps the same machine code: a pointer type, the same byte count, or the same initializer words.
- FACT (gbi.h:1062-1092): sizeof(Gfx) 8, sizeof(Vtx) 16. FACT (nd_gbi.h:19, :50-51): `ND_GS(w0, w1)` is `{{ ND_X0(w0, w1), ND_X1(w0, w1) }}`,
  `ND_CMB_W(...)` is `{ ND_X0(..), ND_X1(..) }` of the same ND_W0_CMB / ND_W1_CMB words. gsDPSetCombineMode (gbi.h:2683) is gsDPSetCombineLERP,
  and nd_gbi.h:46-48 makes that ND_GS(ND_W0_CMB(..), ND_W1_CMB(..)). nd_direct.c includes nd_gbi.h at :17, before :241.
- FACT (common_structs.h:1115): StaticAnimatorNode.displayList is `void*`. FACT (functions.h:509): set_custom_gfx takes Gfx*; a void* argument converts.
- Counter FACT (listcen 038ac581fe): pause_gfx.c 1, lava_piranha.c 1, part2.inc.c 1, entity.c 1, map_api.c 4, nd_direct.c 1. Move: gfxtype -9.
## Edits (match the text)
1. src/pause/pause_gfx.c:382 `Gfx* PauseGfxLabels[] = {` -> `NdLst PauseGfxLabels[] = {` (the file includes nd_gtab.h, :2; one user, :425).
2. lava_piranha.c:453 and part2.inc.c:213: `nd_rec_data(2 * (2 * numPoints) * sizeof(Gfx))` -> `nd_rec_data(2 * numPoints * sizeof(Vtx))` (32 bytes per point both).
3. src/entity.c:924 `node->displayList = (Gfx*)((s32)baseGfx + ...` -> `node->displayList = (void*)((s32)baseGfx + ...` (rest of the line stays).
4. src/evt/map_api.c:248-249: `Gfx* pre = (Gfx*) evt_get_variable(` -> `void* pre = (void*) evt_get_variable(`; the same for `post`. :251 stays.
5. 3ds/src/nd_direct.c:241 before `static const struct { Gfx cc; uint32_t rm0, rm1; } kFillKind[4] = {`
   after `static const struct { struct { uint32_t w0, w1; } cc; uint32_t rm0, rm1; } kFillKind[4] = {`.
   :242, :243, :246 `gsDPSetCombineMode(G_CC_PRIMITIVE, G_CC_PRIMITIVE)` -> `ND_CMB_W(G_CC_PRIMITIVE, G_CC_PRIMITIVE)`.
   :245 `gsDPSetCombineLERP(0, 0, 0, PRIMITIVE, ...)` -> `ND_CMB_W(0, 0, 0, PRIMITIVE, ...)` (the 16 arguments stay).
   :262, :279, :280 `kFillKind[k].cc.words.w0` -> `kFillKind[k].cc.w0`, `.cc.words.w1` -> `.cc.w1`.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edit. `grep -nw Gfx src/pause/pause_gfx.c src/entity.c src/evt/map_api.c 3ds/src/nd_direct.c src/battle/area/kzn2/actor/lava_piranha.c
   src/world/common/lava_piranha/part2.inc.c` prints nothing. `python3 3ds/tools/lkey_cen.py 3ds/tests/lkey/allow.txt .` prints `lkey census ok`.
2. Commit `GTSMALL 1: 9 Gfx tokens leave (handle, byte counts, void casts, fill kind words)`.
3. `python 3ds/tools/bench.py base wscmb_pause_drec --row GTSMALL`, then the same for axns_kzn2_drec (base runs, not charged).
4. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row GTSMALL --bench wscmb_pause_drec,axns_kzn2_drec --why "GTSMALL gate" > build3ds/out/gtsmall_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal (base = merge-base of HEAD and main). Budget: 2 builds, 4 Azahar starts.
- `python 3ds/tools/listcen.py | grep "^listcen total gfxtype"`: 9 lower than on the merge-base (read both values).
## Fix rights: only the 6 files. A compile error, an lkey line, a host FAIL or a cmp DIFF: stop, send the output path to the oracle, wait for the reply.
## Stop and hand-back
Last commit `GTSMALL 2: gate. wscmb_pause_drec, axns_kzn2_drec byte-equal; gfxtype <n>` deletes this page. One SubagentHandback: hashes, LOOP lines, gfxtype.
