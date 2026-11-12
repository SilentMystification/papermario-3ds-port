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

## Design note (written by the S6T agent before code; code read of main b52831546)
Terms: a def (NdSDef) is one producer range: its typed units as static data (gs forms) in stream order. A value unit is a unit whose
content changes per frame or that only sets a value (no model): it stays a run-time op. A static unit is folded by the compiler.
A compiled entry (NdSComp) is the output for one def under one set of inherited inputs.

1. Key: the def (its units are static data, so the def names its content; no content hash at run time) plus the inputs of the
   combine decision after the fold: comb_w0, comb_w1, the 16 tst1 combine fields, the cycle bits of omode_h, prim, envc,
   prim_lod low 8, keyc, keys, conv_k4, tex_on, use_t1, td[1].valid, dual_on, box_prod, box_folded, comb_set, comb_alpha_shade,
   pm_comb_vprim (NdCombIn of nd_comb_decide, which combchk proves equal to sync_combine, plus vprim, which decide reads live).
   A field the def sets takes the def's value in the key (constant per def), so only inherited inputs vary. The masked writes
   (othermode bits, geometry) are applied on the live words at run time, so their inherited bits need no key. Fields per op kind
   (R read, W written; every kind of nd_set.c nd_op_same and nd_op_apply):
   | kind | reads | writes | class in S6T |
   |------|-------|--------|--------------|
   | OMH | omode_h outside the mask; skip: sc_ok, sc_sig | omode_h, om_kh; TEXTFILT change: flush, pm_tex_set_filter; CYCLETYPE in the mask: sync_combine | static (mask, value) |
   | OML | omode_l outside the mask | omode_l, om_kl | static (mask, value) |
   | GEOM | geom bits kept by the clear mask; skip: pm_gpu_mode_is | geom, pm_gpu_set_mode | static (clear, set; ops compose) |
   | TEXTURE | tex_on, tex_ss, tex_ts; skip: sc_ok, sc_sig | tex_on, tex_ss, tex_ts; sync_combine | static |
   | COMB | comb words, comb_set, box_folded, cmb_gen_seen; skip: sc_ok, sc_sig | comb_w0/w1, tst1 combine fields, use_t1, box_prod, box_folded 0, comb_set 1, comb_alpha_shade, cmb_gen_seen, flush on change; sync_combine | static |
   | PRIM | prim, prim_lod; skip: sc_ok, sc_sig | prim, prim_lod, pm_gpu_set_prim; sync_combine | refused in S6T (def recorded as ops); later a uniform of the bind |
   | ENV | envc; skip: sc_ok, sc_sig | envc, pm_gpu_set_env; sync_combine | refused in S6T, as PRIM |
   | RARE | keyc, keys (other bytes) | conv_k4 or keyc, keys; sync_combine | refused in S6T (later: static) |
   | FOG, BLEND, FILL, PRIMZ | (skip: the field) | fogc, blendc, nd_ws.fill, prim_z | value |
   | SCISSOR | pm_wide | sci_*, pm_gpu_set_scissor | value |
   | MW | NUMLIGHT, LIGHTCOL, FOG, SEG: the light, fog or segment field | the same | value; MW none (clip ratio, persp normalize): no field, static no-op |
   | TIMG, TILE, TILESIZE, LOADBLOCK, LOADTILE, LOADTLUT | the texture model (timg, tst1 tile fields, td[], tlut) | the same | refused in S6T (the NdTex part: S6E, S6M) |
   | MTX, POPMTX, MOVEMEM | matrix stack, lights, viewport | the same, upload_mvp | value |
   | MISC CIMG | cimg | cimg, cimg_siz, cimg_stride, cimg_zero, pm_gpu_off_end | value (answer 3) |
   | MISC ND | - | native payload flags | value |
   | MISC NOP | - | - | dropped |
   | VTX, TRI1, TRI2, RECT, CALL, MISC LST, MISC MODVTX, END, DROP | draws and control | - | refused (not a state range) |
   sync_combine reads exactly the key inputs above (nd_comb_decide NdCombIn: leg_mode and box_pat are inputs it overwrites first).
   Intermediate syncs inside a range are dead: their outputs are overwritten by the last one, which reads the final inputs
   (the decision reads omode_h only by its cycle bits, and every op after the last sync in a range leaves those bits).
2. Output form:
   - NdSDef (static, one per producer range; analysed once on core 1 at its first record, read only after):
     `{ const Gfx* u; uint8_t n; uint8_t st (0 new, 1 compiled, 2 refused), id, nv, pre, last_op;
        uint32_t omh_m, omh_v, oml_m, oml_v, geom_c, geom_s, tex_a, tex_b, cw0, cw1; uint8_t has_geom, has_tex, has_comb; }`
     pre = value units before the first static unit (they run first); the rest run after the folded block, in their order
     (a value kind reads no field a static kind writes: checked by the table above).
   - NdSComp (core 0, heap table of 32, allocated at the first compile): `{ uint32_t key[26]; NdCombOut out; }`, out = the combine
     decision (leg, mode5, pat, use, why, cyc, dual, plan_on, plan, cc_on, cc, env_on). This is the pso of the range: the
     nd_pso_n64 form cannot hold it (its oml always sets bits 3 to 31 and takes 0 to 2 from the walker; the status icons
     prologue sets only the alpha compare bits 0 to 1), so the entry keeps the def masks and the decision itself.
   - Record (NDC_NAPI, runner nd_scomp_run): unit 0 = the def pointer and nv; units 1..nv = the value units (typed, made with
     the g macros into a local packet).
   - Run time: value units before the first static unit (nd_walk_state); the block: masked othermode writes (+ om_kh, om_kl; the
     filter flush and pm_tex_set_filter on a TEXTFILT change), geometry (clear, set; pm_gpu_set_mode), texture fields,
     op_setcombine when the words, comb_set, box_folded or cmb_gen_seen differ; then the entry (key compare, miss: nd_comb_decide
     cold) and the replay (scm_replay form: const, env, plan or legacy mode; ls_* and sl_* fields); then sc_ok 1, sc_sig =
     sc_sig_now(), sr_ok 0 (the next draw makes the render state again, as after any range sync that worked), gv_epoch++,
     pm_gbi_cur_op = the last static op; then the other value units.
3. Off-screen targets: the compiled form carries SetColorImage as a run-time value op (in order with the other value units). The
   decision reads no target field; img_role is read by sync_render at the next draw (sr_ok 0). So S1DARK, FSELFIX and the pause
   path may compile ranges that hold a target switch.
4. Invalidation: an S6T entry holds only the combine decision, a pure function of its key; no counter drops it. pm_tex_epoch and
   run-time palettes: no texture in S6T entries; when S6E/S6M add NdTex the bind goes through nd_tex_bind_desc every run (the
   texture layer checks its own epoch and key), so the entry never caches a texture slot. nd_tmpl_gen: the entry holds the PmPlan,
   not a template id (pm_gpu_set_plan finds the template), so no drop; a later entry that keeps a template id must drop on a
   nd_tmpl_gen change. pm_dlc_gen and dlc_reset: no static range reads map data; S6M defs that point into map data drop all their
   entries on a pm_dlc_gen change.
5. Where: the def analysis on core 1 at its first record (cold; the frame push publishes it, as nd_rm_tab); the entry on core 0
   at the first run of each key (cold: nd_comb_decide). The record carries the def pointer (the range id).
6. Proof: switch s6chk runs both on core 0: snapshot the state fields, compiled path, snapshot A, restore, op path (the def units
   with the value units in place, nd_walk_state), snapshot B, compare. Fields compared: omode_h, omode_l, om_kh, om_kl, geom,
   tex_on, tex_ss, tex_ts, comb_w0, comb_w1, comb_set, the 16 tst1 combine fields, use_t1, box_prod, box_folded, box_pat,
   comb_alpha_shade, cmb_gen_seen, leg_mode, comb_mode5, plan_active, cur_plan (when active), ls_use, ls_why, ls_cyc, ls_dual,
   prim, envc, prim_lod, keyc, keys, conv_k4, dual_on, sc_ok. Not compared (cache marks, the compiled path sets them to the
   synced value): sc_sig, sr_ok, gv_epoch, pm_gbi_cur_op, sl_*, ls_frame. Log every 100 frames: `s6chk f<n> ranges <r> bad <b>`
   and one line per field with a mismatch. The op path runs last, so the picture under s6chk is the op path's.

## First use in this row (to prove the form)
Compile the cam_main render_frame range (src/cam_main.c:155-208) and the status_icons prologue (src/status_icons.c:602-614).
Today they record typed ops. After this row they record one NDC_NAPI record with the range id plus the per-frame values; the
runner applies the compiled output.

## Files and places (no running row edits them)
Merge rule: rows running in parallel append to the end of nd_api.h and nd_direct.c, and two appends at the same end conflict. So
this row does not touch nd_api.h or nd_direct.c. Its declarations go in a new header 3ds/include/nd_scomp.h, and its source line
goes in 3ds/CMakeLists.txt right after `src/nd_rec.c` (:384).
3ds/src/nd_scomp.c (new), 3ds/include/nd_scomp.h (new), 3ds/CMakeLists.txt (one line after :384), 3ds/src/nd_set.c (only a read helper for the
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
