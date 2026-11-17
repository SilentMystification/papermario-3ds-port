# design_nstate.md: the core 1 draw state and the native draw record (stage S2 of plan_walker_delete.md; oracle PHASEE2)
Main 69e2ef9357, 2026-11-17. D1 = A, D2 = A (orchestrator). Edit in place. The oracle writes each S3-S6 row page from this file.
Tags: FACT (file:line), GUESS, OPEN. Core 1 = the recorder (game thread). Core 0 = the stream runner (nd_stream_run).

## 1. Facts (code read)
- F1 Every setter writes one typed unit through nd_rec_nr (nd_rec.c:375). Core 0 runs it in nd_walk_state (nd_walk.c:390): skip test
  nd_op_same (nd_set.c:122, called nd_walk.c:280), then gv_epoch++ (nd_walk.c:283), then the handler nd_op_apply (nd_set.c:159).
- F2 The handlers mix state writes and GPU side effects: nd_set_prim calls pm_gpu_set_prim and sync_combine (nd_set.c:74-80), set_om
  flushes on a filter change (:83-96), NDO_TEXTURE and NDO_COMB call sync_combine (:172, :175).
- F3 Walker state lives in nd_ws (nd.h:142-163) and in globals: conv_k4, keyc, keys, dual_on, box_prod, box_folded, comb_set,
  comb_alpha_shade (nd_comb.c:23-31), sci_x0..sci_y1, fog_fm, fog_fo, fogmix_on (nd_set.c:25-26), fogc, blendc (nd_set.c:41),
  lt[9], lt_n, vp_sx.. (nd_mtx_set.c:18-26).
- F4 Core 1 already keeps a partial shadow (sh, nd_rec.c:65-135). It drops ops that core 0 would skip. Any other command resets it
  (sh_all, nd_rec.c:132, :177). It does not follow the state of a list, record, rectangle or light.
- F5 Batches: a triangle appends to the open GPU draw only when ep_open_epoch == gv_epoch (nd_geom.c:275). A new epoch runs
  gv_epoch_enter (nd_geom.c:250, :203). So every state op that core 0 does not skip ends a batch. drec writes one record per GPU draw
  (pm_gpu.c:630): a different batch cut gives a different drec.txt.
- F6 sc_ok gates the skip of OMH, PRIM, ENV, TEXTURE, COMB (nd_set.c:125-141). Core 0 clears it at: list start keep 0 (nd_walk.c:433),
  :688, particles (:461, :469, :509, :517), rectangles (nd_rect.c:122), direct draws (nd_direct.c:182), model records (nd_recs.c:242,
  :291, :328). NDO_GEOM also tests pm_gpu_mode_is (nd_set.c:129). MOVEMEM bumps gv_epoch unless lt_mem_same (nd_walk.c:295-298).
- F7 Replay rule: eye 2 and the interp key frame replay the stream, the replay writes nothing (nd_cmd.h:5-6). A record holds values or
  pointers into the stream or into frame data, never a pointer to core 1 state.
- F8 Static lists: a v2 table holds its typed units (NDN_UNIT, nd_gtab.h:104). Core 0 runs them (gt_run nd_walk.c:633, walk_call :246).
  A state-only list is recorded in line (rec_inline nd_rec.c:399). A range replays (nd_rec_call nd_rec.c:479).
- F9 Pure decision code exists: nd_pso_find + nd_pso_fix (nd_psolk.c, nd_psokey.c after PSOFB), nd_render_state (nd_tmpl.c:292),
  the bind facts of ensure_tile_slow (nd_tex_set.c:335), the texel test of box_fold_sync (nd_comb.c:359).
- F10 listcen reader greps NDO_ and G_ op names (listcen.py:52). model greps 15 function names (listcen.py:196-198).

## 2. End form (D1 A)
- Game setters stay. They write N64-valued fields into one core 1 state, NdDs. They record nothing.
- Each draw call on core 1 makes one NdSnap from NdDs: pso key index, render block, 2 binds, colours, the ep bit. Core 0 runs the
  snap and the geometry. Core 0 holds native GPU state only. It decides nothing.
- Every pure decision runs on core 1 at snap time: the pso lookup, nd_pso_fix, nd_render_state, the bind facts, the texel test.
- A static list runs on core 1: its field-set ops change NdDs, each of its draws makes one NdSnap (section 7).
- The N64 field meaning stays on core 1 because the game API states it (UD1, section 10).

## 3. NdDs (new 3ds/include/nd_ds.h)
- `typedef struct NdDs { NdWs w; } NdDs;` after row NST0 moved the F3 globals into NdWs. One NdWs type serves both cores.
- Core 1 owns `NdDs nd_ds`. Core 0 keeps `nd_ws` until S7. dschk (section 5) proves nd_ds.w equals nd_ws at every command.
- Fields that core 0 writes at draw time are not inputs and are left out of the compare: td[0..1] (ensure_tile), tile_x0, tile_y0,
  tile_fx, tile_fy, cur_tile, box_folded, the decision outputs (leg_mode, comb_mode5, box_pat, plan). Core 1 makes them at snap time (S4).

## 4. One pure apply for both cores (row NST1, Sonnet)
- New `unsigned nd_ws_apply(NdWs* w, uint32_t k, uint32_t v)` in new 3ds/src/nd_wsap.c: the state writes of nd_op_apply (nd_set.c:159-183),
  set_om, set_geom, nd_set_prim, gbi_setenv, set_fog/blend/fill/primdepth, set_scissor (with the pm_wide sign rule), set_mw, set_rare,
  op_setcombine (nd_comb.c:473) and the nd_tex_set.c setters of TIMG, TILE, TILESIZE, LOADBLOCK, LOADTILE, LOADTLUT, and the light and
  viewport state of gbi_movemem (nd_mtx_set.c:180). It returns a mask: bit 0 a value changed, bit 1 the filter bits changed (F2).
- Core 0 handlers call nd_ws_apply(&nd_ws, ...) first, then their GPU side effects in the old order. Read old values before the call.
- Example (nd_set.c:99-104), after: `void gbi_setenv(u32 w1) { GP_SET(2, nd_ws.envc == w1); nd_ws_apply(&nd_ws, NDO_W0(NDO_ENV, 0), w1);
  pm_gpu_set_env(nd_ws.envc); sync_combine(); }`.
- Core 1 hook: nd_rec_nr (nd_rec.c:375) calls `nd_ws_apply(&nd_ds.w, w0, w1)` before rec_op_t. Geometry units (VTX, TRI, MTX, POPMTX)
  change no NdWs field. Gate: drec byte-equal (the core 0 output does not change).

## 5. Feed every producer, prove it (rows NST2a-d, NST3; Haiku)
- NST3 first: switch dschk. With it on, core 1 records one NDC_NAPI record `nd_dschk_run` before each non-state command: the FNV-1a 32 of
  nd_ds.w (section 3 fields only) and the ep bit (section 6). Core 0 compares with the hash of nd_ws and with `st_epoch` (a counter that
  op_typed and the MOVEMEM arm of op_geo raise, reset at each command). Log `dschk f<n> cmds <c> bad <b> epbad <e>` every 100 frames,
  the first 8 bad field names, and `dschk end` at fxtest done. The first value of bad is the S3 counter.
- NST2a static lists: at nd_rec_dl (nd_gtab.c:108) and rec_inline (nd_rec.c:399) core 1 applies the list units (nt->u, nt->nu; a
  nested NDO_CALL unit applies its table, depth 12, ND_LIST_DEPTH nd_walk.c:203). NST2b model records: the R0 render mode units and
  the R13 aux units at nd_rec_model. NST2c entity, sprite and particle units (nd_rec_ent_draw, nd_rec_spr, nd_parts_add pre and post).
  NST2d nd_rec_list, map shapes (dlc), ranges (nd_rec_call: core 1 walks the units of [a, b) of its own stream).
- Each NST2 row: drec byte-equal and dschk bad lower than its merge-base. NST2d: bad 0 and epbad 0 in the 4 dschk scenes.
- dschk scenes: copies of spot_mac01_drec, wscmb_pause_drec, axns_sam2_drec, fxchk_a with `dschk` instead of `drec` (row NST3).

## 6. The batch rule: the ep bit (R3)
- Core 1 sets ep = 1 between two commands when one of these occurs:
  (a) nd_ws_apply returns bit 0 for a unit other than VTX, TRI1, TRI2, MTX, POPMTX;
  (b) a unit OMH, PRIM, ENV, TEXTURE or COMB arrives while sc_lost = 1;
  (c) a MOVEMEM unit that is not a light with the same bytes (lt_mem_same rule, nd_walk.c:741, on core 1 data).
- sc_lost = 1 after core 1 records a command of a kind in F6. sc_lost = 0 after it records a draw.
- GUESS: NDO_GEOM with pm_gpu_mode_is false after a rectangle or direct draw is rare. epbad in dschk measures the rule. A row that finds
  epbad above 0 sends the dschk log to the oracle.
- S4 core 0 snap runner: `if (s->ep) gv_epoch++;` before the draw. Nothing else bumps gv_epoch for state.

## 7. NdSnap and the pso id (row NDR0, Sonnet; then NDR1-4, Haiku)
- Core 1 resolves the pso at snap time. Memo: the key index stays while no pso input changed (a dirty bit that nd_ws_apply sets for
  cw0, cw1, omh, key_c, key_s, conv_k4, the 8 flags, vprim, prim_lo8). A change runs nd_pso_find (binary search) and nd_pso_fix with the
  core 1 copies of comb_mode5 and box_pat. A miss gives the PSOFB fallback and its log entry.
- NdSnap (8-byte units in the record): key index (16 bits), ep, cc (leg 4 colour), mode5, pat, dual, the NdRender block (memo on omh,
  oml, zimg), prim, prim_lod, env, fog, blend, fill, prim_z, keys, conv_k4, scissor (4 x 16 bits), fog_fm, fog_fo, vprim, and 2 pointers
  to bind blocks. A bind block (NdBindDesc), the lights block and the viewport block go into the stream with nd_rec_data when they change.
  The snap points to them (F7: stream memory lives for the replay).
- Core 0 `nd_snap_run(const NdSnap*)`: set the GPU state as sync_combine_work (nd_comb.c:297-318) does from the NdCombOut of the key
  index, as sync_render does from the NdRender block, nd_tex_bind_desc for each bind, the colours. NDR0 pilots it on nd_mesh draws.
- NDR1-4 (one draw kind each): rectangles (nd_rect.c), direct draws (nd_direct.c), glyphs, sprite records. Gate: drec byte-equal,
  dschk bad 0. model falls by the walker functions the kind no longer calls (nd_texrect, nd_fillrect: -2, FACT listcen file nd_rect.c 2).

## 8. The v2 field-set op (rows NCONV1 Sonnet, NCONV2 Haiku)
- NDN_UNIT (nd_gtab.h:104) is replaced by NDN_FS: `a` = field id (enum NdFs in nd_ds.h, one per NdWs field group of nd_ws_apply),
  `v` = value, `idx_pos` = mask (othermode, geometry clear), `p` = pointer value (image, palette, light, viewport; a segment address stays
  a value and core 1 resolves it with nd_ds.w.segbase).
- Core 1 runs a table at nd_draw_list: NDN_FS through `nd_ds_fs(&nd_ds, op)`, NDN_XF as a transform record, each NDN_DRAW as one NdSnap
  and its vertex and index ranges. No table unit reaches core 0. NCONV2 deletes the NDN_UNIT path (gt_run, nd_ntab_run).
- Gate: fxops_drec, fxcull_drec, entcape_kmr03_drec byte-equal, dschk bad 0. reader falls by the NDO_ uses of the deleted path.

## 9. Records (S6: NREC1 Sonnet model record; NREC2-5 Haiku entity, sprite, shapes, particles)
- Each record kind takes NDN_FS ops and NdSnap draws instead of typed units. The core 1 producer runs them as in section 8.
- NREC1 is Sonnet: a record format with writers in model.c, entity_model.c, animator.c and readers in nd_model.c, nd_recs.c.

## 10. Rows and order (S3 moves no listcen counter; its counter is dschk bad)
| row | model | blocker | counter |
|---|---|---|---|
| NST0 globals of F3 into NdWs (ws_NST0.patch, mechanical) | Haiku | PSOFB | none, drec equal |
| NST1 nd_ws_apply, core 0 handlers call it, core 1 hook | Sonnet | NST0 | none, drec equal |
| NST3 dschk switch and 4 scenes | Haiku | NST1 | dschk bad base value |
| NST2a-d producers | Haiku | NST3 | dschk bad to 0, epbad 0 |
| NDR0 NdSnap, core 1 resolve, nd_snap_run, pilot nd_mesh | Sonnet | NST2d | model -1 GUESS |
| NDR1-4 | Haiku | NDR0 | model -2 (nd_rect.c) and more |
| NCONV1, NCONV2 | Sonnet, Haiku | NDR0 | reader GUESS -40 |
| NREC1, NREC2-5 | Sonnet, Haiku | NCONV2 | reader GUESS -190 |
Sonnet reasons: NST1 cuts shared handler code in 5 files with byte-equal output and an order of side effects that only the cut shows.
NDR0 is a new record form shared by every draw kind. NCONV1 is one algorithm over 922 tables. NREC1 changes a record with 3 writers.

## 11. Risks
- R1 S3 doubles the state work on core 1 until S6 (every unit runs twice). GUESS 0.3 ms per frame on the O3DS (3000 units x 30 cycles).
- R2 epbad above 0 from a core 0 skip rule that core 1 cannot see (F6 GEOM). The dschk count finds it before S4.
- R3 Stream size: NdSnap is about 64 bytes per draw. GUESS 1500 draws per frame: 96 KB per context. memcheck decides.

## 12. User decisions (open until the user answers; the rows S3-S6 follow option 1 of each)
- UD1 census of the core 1 field model. Evidence: F10, section 2 (the N64 field meaning stays on core 1 under D1 A). Option 1: accept;
  listcen gets a reported class `core1` (ND_FS_ uses), not part of the end proof. Option 2: D1 B (static desc at every draw, about 84 rows).
- UD2 matrix composition. Evidence: load_mtx (nd_mtx_set.c:130) is in the model and elfgate name lists. Option 1: core 0 keeps a native
  transform stack (push, load, mul, pop as native enums; the G_MTX flags are translated on core 1). Option 2: core 1 composes and the
  record carries the final 64-byte matrices.
- UD3 core 1 time of R1. Option 1: accept and measure at the NST1 gate with one prof push. Option 2: one prof push before NST1 starts.
