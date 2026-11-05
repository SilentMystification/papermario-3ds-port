# Plan S3N: map models draw natively (the walker no longer runs model records)

## Context
Goal of the user: replace the N64 display list walker. Slice S3 so far (M4-2, S3F, S3G, commits up to 5b44537c4) makes every map model write one producer record (G_PM_REC, 14 words, src/model.c appendGfx_model). But the walker still draws them:

record -> `rec_apply` (3ds/src/pm_gbi.c ~L4194) runs the bind list (`tb_ops`), combine (`rec_op`), render mode list (`rm_apply`), fog, lights, `load_mtx` into WALKER GLOBALS -> the geometry list (R12) runs through `rec_dl` -> `dlc_try` -> `dlc_static_group` -> `dlc_seg` -> `pm_gpu_grange` (index ranges into the open batch) -> `flush_draw` (3ds/src/pm_gpu.c ~L1003) -> `nd_raw_native` (pm_gpu.c ~L913) builds NdTmplKey, NdUnit, palette and uniforms FROM THOSE WALKER GLOBALS and calls `nd_emit_draw`.

So the walker interprets every model each frame (the 211 us per plain model cost on the console). The Haiku agent stopped at S3N-1 because nd_raw_native reads walker globals and geometry goes through walker ops. This plan removes the walker from that path for map models.

Intended outcome: record -> `nd_model_draw` -> NdDraw built from the record plus data compiled once per map -> `nd_emit_draw`. No walker state, no flush_draw, no rec_apply for models it can draw.

## Design

### Data compiled once per map (cold, new file 3ds/src/nd_model.c)
- Render mode list -> `NdMode` {om_h, om_l, geom and/or, NdRender}: run `nd_words` (3ds/src/nd_state.c, decodes list words into an NdSt with no walker side effect) over `ModelRenderModes[i]`, then `nd_render_state` (nd_tmpl.c). Table by render mode index (R0 bits 16-23).
- Texture bind list (TextureHandles[i].gfx, R0 w1) -> `NdModelTex` {NdTexDesc for tile 0 (and tile 1 when the list loads two), tile size and shift words for uvso, combine-independent flags}: `nd_words` over the list gives timg, tile_w0/w1, tsz words; build NdTexDesc from them (nd_tex.c helpers nd_tex_wrap_of, nd_tex_wrap_final). Cache by list pointer, reset with `pm_dlc_gen` (map load, pm_map.c).
- Geometry: the R12 shape list is already compiled by the DLC2 path into a `DlcEnt` with groups in the static vertex and index pools (pm_gbi.c `dlc_find`, `DlcGrp` sd/ends/bat_q/gop). Reuse it as is (no second compiler).

### Per draw (hot, nd_model.c, `nd_model_draw(const Gfx* rec)`)
1. Resolve the state from the record: R4 combine words, R3 flags (fog, prim, env, blend, shroud, depth), R6/R7 colours, the NdMode and NdModelTex entries -> NdTmplKey (the same field rules as nd_raw_native, but from these inputs) -> `nd_tmpl_build`. TEV colours through `nd_tev_build` (cached by TevKey as nd_raw_native does).
2. uvso, cmul, cadd, zmix: MOVE `gv_params_calc` (pm_gbi.c L1290-1340) into nd_model.c as a pure function of (combine plan, tile size and shift, fog, prim, env). The walker calls the moved function too (one copy, no fork).
3. Texture unit 0 (and 1): `nd_tex_get(desc)` -> `nd_tex_block(id)` -> NdUnit (verified words: TEXW 56 of 57, ndunit check).
4. Matrix: `nd_mtx_load` with R1/R2 (finalMtx, flags), billboard R8, then `nd_mtx_combine` and `nd_mtx_pcomp(post, game)` (seam rule 1: same product and order as pal_mul). The walker matrix stack is already nd_mv (nd_mtx.c), so the stack stays in step for later walker draws.
5. Geometry: per DlcGrp batch range of the R12 list, one NdDraw (idx_word from the static index pool address, nidx). MERGE adjacent ranges with an equal key into one draw (packed pool, map_e4.md 3; nds1_status finding 3). Cull mode from the group geometry ops (gop) as dlc_static_group computes geom_at.
6. Emit: factor the submit tail of nd_raw_native (buffer room check, stage program pm4 and buffer, scissor, user clip, stencil off, C3Di_UpdateContext, dirty-texture shadow reset, nd_emit_draw, GPUCMD_AddRawCommands, DrawUsed bit, counters) into one function `nd_native_submit(const NdDraw*)` in pm_gpu.c. nd_raw_native and nd_model_draw both call it (one path). Before it: `pm_gpu_flush()` closes any open walker batch.

### Fallback (bounded, counted, deleted when 0)
nd_model_draw returns 0 for a record it cannot draw yet: lit (pm5 program, `g_use5`), two texture units or noise, off-screen target, zmark or decal pass, custom gfx pre/post (R11), aux words (R13), a geometry list that DLC2 did not compile, cache full. Then the record goes to `rec_apply` as today. Counter `ndm fb <n> ok <n>` in a 300-frame log line. Target 0 fallback for unlit plain models at the 3 spots. Lit models get their own later row (pm5 light block in nd_emit_draw: nd_lights is ready).

### Walker state after a native model (bridge risk R3)
Native models do not set walker globals. A later walker producer (sprite, HUD) that relied on state a model set would draw wrong. Mitigation: after each native model, mark the walker's combine, render mode and bind state stale (the next walker op re-derives it: tb_last = NULL, the g2_same shadows of the ops records replace cleared). Picture check at the 3 spots shows any leak.

## Rows (each one commit; small tests per user rule)
- S3N-1: nd_model.c compile tables (NdMode, NdModelTex), reset on map load. Move gv_params_calc into nd_model.c, the walker calls it. Gate: build, host test nd_host.
- S3N-2: `nd_native_submit` factored out of nd_raw_native (pure move). Gate: build, host, spot_mac01_pic pixel-equal to HEAD (pure refactor of shared draw code: byte-equal check is required here by CLAUDE.md rule 5).
- S3N-3: `nd_model_draw` resolve plus debug switch `ndmcheck`: for each record, resolve natively, then run rec_apply and the walker path as today, and at the walker's nd_raw_native compare the NdDraw it builds with the native one (tmpl key, tev colours, units, uvso/cmul/cadd/zmix, palette entry 0, idx ranges). Draw stays on the walker. Log `ndmcheck f<n> recs <r> eq <e> diff <d> fb <f>` with the first 20 differing fields. Gate: diff 0 at spot_mac01, spot_mac00, spot_sbk56 (the walker is the test oracle here, plan 9.2 allows that; not a runtime input).
- S3N-4: switch: the G_PM_REC arm calls nd_model_draw, rec_apply only on fallback. Walker-state-stale marks. Gate: 3 spot pictures right by eye, `ndm fb` count logged.
- S3N-5: delete rec_apply branches the fallback no longer reaches, and the tb_ops/rm_apply calls used only by them. Keep ndmcheck as a debug tool. Console prof on the 3 spots at the epic end (orchestrator push).
- Later epic S3L: lit models (pm5 in nd_emit_draw) and two-unit textures, so fallback reaches 0 for all model classes.

## Files
- New: 3ds/src/nd_model.c, its prototypes in 3ds/include/nd.h.
- Edit: 3ds/src/pm_gbi.c (G_PM_REC arm, gv_params_calc moved out, stale marks, ndmcheck hook), 3ds/src/pm_gpu.c (nd_native_submit, ndmcheck compare at nd_raw_native), 3ds/src/pm_map.c (reset), 3ds/CMakeLists.txt, 3ds/tools/nd_host_test.sh plus a host case for the moved gv_params_calc.
- Not touched: src/model.c (the record already holds the inputs), azt_scenarios.json, bench_defs.json (other session; ask it for one scenario `spot_mac01_ndmcheck` = spot_mac01_pic plus switch ndmcheck, until-line the ndmcheck log line).

## Reused code
nd_words (nd_state.c), nd_render_state, nd_tmpl_build, nd_tev_build (nd_tmpl.c), nd_tex_get, nd_tex_block, wrap helpers (nd_tex.c), nd_mtx_load, nd_mtx_combine, nd_mtx_pcomp (nd_mtx.c), DLC2 static groups (pm_gbi.c dlc_find, DlcGrp), nd_emit_draw and NdShadow (nd_emit.c), the submit tail of nd_raw_native (pm_gpu.c).

## Verification
- Host: `python 3ds/tools/bench.py run nd_host --row S3N --why ...` after each row.
- S3N-2: spot_mac01_pic pixel-equal to the previous build.
- S3N-3: ndmcheck diff 0 on the 3 spots (one Azahar run each).
- S3N-4: spot_mac01_pic, spot_mac00_pic, spot_sbk56_pic right by eye. Log shows `ndm` ok count above 0 and the fallback count.
- Epic end: console push, spot_*_prof on the O3DS XL, recorded in the DB (commit field). Compare by `bench.py build-at` if a before number is needed.

## Execution
In this session after approval (user: "act on it here"). One code agent per row with the row text from this plan, or this session codes the rows directly if the user says so. Collision rules as before (pathspec commits, other session in its own worktree).
