# Row S6T: the state compiler: typed state-op ranges compiled once into pso and texture ids (NATIVEAPI F1, design and first use)

Model: Opus (user decides).
- Why not Sonnet: this row fixes the core form that S6E, S6M, S6S and S6DEL1-3 build on. Its key must hold for every caller, or a
  compiled range draws with state that differs from the walk, and no later row can see that from its own gate.
- Why not a smaller split: the key choice, the invalidation rule and the off-screen target rule depend on each other.
- Opus may choose its steps inside the gate below. It writes the design note (section "Design note to write") into this row
  before code, so the oracle can check it.

## Goal (plan_nativeapi.md S6)
Core 0 must stop running the N64 state model per frame for state that does not change: other mode, combine decide, tile and TMEM
model, render-state sync. A typed state-op range (a run of state ops between draws) is compiled once into what it leaves for the
next draw: an NdPso id (nd_pso_n64 form), an NdTex binding (NdBindDesc), and the walker fields the next walker op reads. The run
time then binds ids. The model code still runs, but only cold, when an id is made (nd_comb_decide, nd_render_state,
nd_tex_win_desc).

## What exists (code read, main after NAPI2B)
- Per-frame state work: nd_ops_run and nd_walk_state (nd_walk.c) run typed ops (nd_op.h) through the op table of nd_set.c. The
  skip tests are nd_set.c:130-141 (NDO_TEXTURE: sc_sig_now() == sc_sig). sync_combine (nd_comb.c:560-590) is keyed by sc_sig_now
  (nd_comb.c:468-475), which hashes comb words, omode_h, prim, env, prim_lod, keyc, keys, conv_k4, leg_mode and the walker flags.
  The texture model is set_tilesize, tex_win0/1 and ensure_tile (nd_tex_set.c; ensure_tile_slow :384-434, bind body
  nd_tex_bind_desc :439-488).
- Direct helpers (NAPI2B, nd_direct.c): nd_pso_decide caches a decision per pso id under a key of the walker inputs (dec_key: mh,
  ml, flags, keyc, keys, conv_k4, prim_lod, vprim). nd_direct_bind writes the walker fields the stream would have left.
  nd_direct_end invalidates the walker caches: ens_clear, cur_tile -1, sc_ok 0, nd_sr_forget, gv_epoch++.
- Entities: ent_run (nd_ent.c:389-417) replays the state pieces between segments with ENT_GBI = nd_ops_run (:381). A segment
  draws natively (ent_seg_native) or is interpreted.
- Heap-built lists: ent_heap_list (nd_ent.c:191) marks imgfx and battle-heap lists, and their content is hashed per frame.

## Producers that are pure state ranges (napicen3, ops per frame), first users of the compiler
| producer | file:lines | ops | class |
|----------|-----------|-----|-------|
| render_frame camera setup | src/cam_main.c:155-217, :252, :272-275 | mac01 28, btl 57 | static range plus a few per-camera values (scissor, viewport, perspNorm, matrix) |
| draw_all_status_icons prologue | src/status_icons.c:602-614 | 13 | static range plus scissor |
| gfx_init_state and the background task | src/background_gfx.c (gfx_init_state) | 17-25 | static range |
| btl prim quad setup list D_80293970 | src/battle/190B20.c:44-55 | in S5PQUAD | static list |
| SaveBlock custom render | src/entity/SaveBlock.c:38-43, :60-68 | 13 | entity: matrix, render mode, combine, prim, static list |
| mac_01 crystal ball | src/world/area_mac/mac_01/crystal_ball_gfx.c | 32 | map custom gfx |
Follow-up rows (written after this design merges): S6S (static ranges: cam_main, status_icons, gfx_init_state), S6E (entities incl.
SaveBlock through ent_run), S6M (map custom gfx incl. crystal_ball_gfx).

## Design questions the row must answer (the design note)
1. Key.
   - Content hash of the range's typed units plus the inherited-state signature, or a tighter set?
   - The signature must cover every walker field that an op in the range reads and does not set itself. Example:
     G_SETOTHERMODE_L with a mask keeps the bits outside the mask.
   - List the fields per op kind. Every op kind in nd_set.c must be in the list.
2. Output form. What one compiled range stores:
   - pso id, tex desc or none, prim, env, fill colour, scissor, viewport, geometry mode, matrix ops (kept as run-time ops: they
     are values, not state);
   - the walker fields to write after (so a later walker op sees the same state).
   - Give the struct.
3. Off-screen targets: SetColorImage to an off-screen image (draw_box ROTSCALE contents, the darkness stencil, pause grabs, file
   select). Either a range with a color image switch is not compiled (it stays per frame), or the compiled form carries the
   target op as a run-time op. Choose one. S1DARK, FSELFIX and the pause path wait on this answer.
4. Invalidation:
   - texture source epochs (pm_tex_epoch), palettes edited at run time (tattle lerp, actor palette swaps);
   - pso table generation (nd_tmpl_gen);
   - dlc_reset and pm_dlc_gen. Say which counters drop a compiled range.
5. Where it runs. Core 1 (the range is known when recorded) or core 0 at first walk? The walker owns nd_ws and the texture
   layer (nd_wq.c:1-20), so compile is on core 0, cold. The record carries a range id.
6. Byte-equal proof plan: the compiled path leaves the walker the same fields as the op path. A debug switch `s6chk` runs both
   and compares the fields after each range (cold, debug only), and counts mismatches by field.

## First use in this row (to prove the form)
Compile the cam_main render_frame range (src/cam_main.c:155-208) and the status_icons prologue (src/status_icons.c:602-614).
Today they record typed ops. After this row they record one NDC_NAPI record with the range id plus the per-frame values; the
runner applies the compiled output.

## Files and places (no running row edits them)
3ds/src/nd_scomp.c (new), 3ds/include/nd_api.h (new declarations at the end), 3ds/src/nd_set.c (only a read helper for the
fields, if needed), src/cam_main.c (render_frame :150-220 under TARGET_3DS), src/status_icons.c (:600-615 under TARGET_3DS),
3ds/tools/nd_host_test.sh (one line for a new host test). Pre-flight before you edit: grep `nd_direct_end` and
`nd_pso_decide` (NAPI2B merged) and `nd_rec_napi` (nd_api.h:11).

## Gate (expected values)
- nd_host PASS with a new host test of the key: two ranges that differ only in an inherited field the range reads get different
  ids; two ranges that differ only in a field the range sets get the same id.
- DIRECT byte set: `bench.py base` then `bench.py cmp` for spot_mac01_det, mdl5_iwa10_det and spot_mac00_det, one list (3 starts).
  Every shot byte-equal.
- `s6chk` on spot_mac01_det (1 start): 0 field mismatches.
- napicen_mac01 (1 start): cam_main.o and status_icons.o ops drop to the per-frame value ops only (hand back the numbers).
- Budget 8 Azahar starts. Console: in CONSOLE C5 (with the S5 batch).

## Stop
A byte difference or an s6chk mismatch: stop, commit, hand back the field counts (2 tries per named cause). The design note
stays in this row. Delete this row file in the completing commit; write the follow-up row list (S6S, S6E, S6M) into the
hand-back.
