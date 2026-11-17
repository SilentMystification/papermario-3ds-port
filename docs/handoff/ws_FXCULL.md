# ws_FXCULL (Haiku): impact and shockwave parts drawn in 2 cull passes with a stated entry each (C10 recdl -3)
Designer: oracle DESIGN3 (SendMessage). Worktree build3ds/wt/FXCULL, branch FXCULL from main, PM_ROW=FXCULL PM_LANE=FXCULL.
Edit only src/effects/green_impact.c, red_impact.c, shockwave.c; add 3ds/tools/azt.d/FXCULL.json, 3ds/tools/bench.d/FXCULL.json.
## Objective and counters
- FACT (code read): each file records its parts once in an NdSub range and replays it twice (cull front, cull back). So one
  nd_rec_dl site has 2 walker tuples (fxrest.d/classes.txt:3,5,6). NDC_CALL is a pure replay (nd_rec.c:602).
- Design: one loop computes the part matrices (no record). A helper records the parts once per cull pass with its own entry.
  Same records in the same order as the replays (GUESS: drec byte-equal; the gate decides). Move (FACT, listcen): recdl -3.
## Descs (fxrest.d/cap.txt:8-11, 128-135). Put them after the includes. Tail of every desc: `1, 0, 1, 0, 0, 0, 1, 0, K, 0, 0, 0`.
- pe_green_impact_front: 0xfc3133ff, 0x5ffefe38, 0x1e182cf0, 0x0c184b50, 0x00000205, three 0x00000000, K 1. _back: 0x00000405.
- pe_red_impact_front/_back: the green words. pe_shockwave_b_front/_back (types 2-5): 0xfc60b2a0, 0x23fffef8, 0x1e182ca0,
  0x0c184b50, 0x205/0x405, K 2. pe_shockwave_a_front/_back (types 0-1, setup D_090009A0_359D50 as red): the red words (GUESS).
  shockwave passes `temp_v0 >= 2 ? &pe_shockwave_b_front : &pe_shockwave_a_front` (and _back).
## Edit shape (green_impact.c; red :150-224 and shockwave :275-353 take the same shape with their own values and names)
Delete :139 `NdLst dlist2;`, :153 `NdSub sub;`, :181 nd_sub_open, :205 nd_sub_close. Add `u16 m0;` and before :181 `m0 = gMatrixListPos;`.
In the loop the last gu line becomes `guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos++]);`. Delete :195-201.
:211 nd_sub_call -> `green_impact_parts(((EffectInstance*)effect)->data.greenImpact, ((EffectInstance*)effect)->numParts, m0, &pe_green_impact_front);`
:217 the same with `&pe_green_impact_back`. shockwave: delete :275 `NdLst dlist = ...`; keep :276 dlist2 (the setup list).
```c
/* FXCULL: one cull pass of the parts (the N64 list for later drawn twice). m: the matrix of part 1 (written by appendGfx). */
static void green_impact_parts(GreenImpactFXData* part, s32 numParts, u16 m, const NdPsoDesc* e) {
    s32 i;
    part++;
    for (i = 1; i < numParts; i++, part++) {
        if (part->unk_4C >= 0) {
            nd_val_tile_size(G_TX_RENDERTILE, 0, (s32) part->unk_3C, 252, (s32) part->unk_3C + 124);
            nd_val_tile_size(1, 0, (s32) part->unk_44, 124, (s32) part->unk_44 + 124);
            nd_xf_push(&gDisplayContext->matrixStack[m++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            nd_draw_list(NDN(D_09000540_365EF0), *e);
            nd_xf_pop(G_MTX_MODELVIEW);
        }
    }
}
```
## Bench: copy azt.d/FXDIG.json and bench.d/FXDIG.json; name fxcull_drec (fxtest numbers: FXLIST line - 33, pm_fxtest.c)
`warp=mac_01:0 pos=0:0:200 fxtest=32,44,98,143 loglevel=debug det drec=412:4:44 psochk`, until `fxtest: done`, cap 45, deadline 40,
drec event `drec f544 draws [1-9]\d* bytes \d+`.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edit. `git grep -n "nd_rec_dl\|nd_sub_" src/effects/green_impact.c src/effects/red_impact.c src/effects/shockwave.c` prints nothing.
   Commit `FXCULL 1: impact and shockwave parts in 2 cull passes with stated entries; fxcull_drec`.
2. `python 3ds/tools/bench.py base fxcull_drec --row FXCULL --commit $(git merge-base HEAD main)` (base run, not charged).
3. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row FXCULL --bench fxcull_drec --why "FXCULL gate" > build3ds/out/fxcull_loop.txt 2>&1`.
## Tests that decide (budget 1 build, 2 Azahar starts)
- `LOOP PASS`: host PASS, build rc 0, fxcull_drec byte-equal. Last psochk line: bad equal to the base, draws higher than the base.
- listcen total recdl 3 lower than on `$(git merge-base HEAD main)`.
## Fix rights: only the 5 files. A compile error, a DIFF, a psochk bad line or a timeout: stop, send the output path to DESIGN3.
## Stop and hand-back: last commit `FXCULL 2: gate. fxcull_drec byte-equal, psochk bad equal; recdl <n>` deletes this page.
One SubagentHandback: hashes, LOOP line, psochk lines, recdl.
