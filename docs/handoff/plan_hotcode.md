# plan_hotcode: shrink the core 0 draw code and data to fit the ARM11 L1

Rules: CLAUDE.md 5, 7, 8, 9, 11; hw_guidelines.md (rules 1, 2, 3, 10). Base: main 54e0c8a09, lane build HOTCODE.
Tags: [M] measured from the map or ELF; [C] read in source; [E] estimate; [U] not checked.

## 0. Where the code is now
- Toolchain [M]: devkitARM GCC 16.1.0, -O2 -ffunction-sections, armv6k/mpcore, hard float, ARM mode. No .cold parts: GCC does not
  split blocks without profile data; only explicit `cold, noinline` functions leave hot functions (133 in .text.unlikely).
- Draw-side object text [M]: pm_gpu.o 84.6 KB, pm_tex.o 47.6, nd_comb.o 16.2, nd_tex_set.o 15.0, nd_model.o 13.2, pm_comb.o 12.5,
  nd_shape.o 11.7, nd_dlc.o 9.7, nd_ent.o 9.0, nd_walk.o 8.7, nd_geom.o 7.2, nd_rect.o 6.5, pm_gpu_rect.o 5.9, nd_tmpl.o 5.3; about 280 KB.
- Static reach from nd_stream_run [M]: 774 functions, 339 KB; without log, libc, compile, miss and census paths 438 functions, 163 KB.
- Hand-picked per-frame set, steady state, by class [M sizes, E membership] (baseline of M1):

| Class | Bytes | Largest functions |
|---|---|---|
| A stream and op loop | 13.9 KB | nd_walk_state 2532, set_tilesize 2140, op_setcombine 1292, tex_tile1 1188, op_settile 716, g2_same 712 |
| B combine sync | 14.6 KB | pm_comb_eval 2960, pm_gpu_set_plan 1636, decide 1376, const_rgb_tex_alpha 1344, name_colour 940 |
| C model record hit | 13.3 KB | rec_run 2344, pm_gpu_model_hit 1300, pm_gpu_model_key 732, texsrc_chk 704, nd_tex_get 692 |
| D native submit and emit | 11.5 KB | nd_native_submit 2424, nd_emit_draw 2072, C3Di_UpdateContext 1256, C3D_UpdateUniforms 656 |
| E batch close | 12.8 KB | nd_raw_native 3132, nd_tev_build 2328, rect_sig_check 2072, nd_tmpl_build 1220 |
| F vertex and triangle | 14.1 KB | gv_tri 1864, pm_gpu_gtrip 1840, nd_geom_vtx 1736, nd_gv_params 1228 |
| G texrect and fillrect | 21.7 KB | nd_texrect 2992, ensure_tile 1976, pm_gpu_tex_rect 1572, box_pattern_rect 1476 |
| H sprite | 9.5 KB | nd_model_state_in 3820, pm_gpu_spr_build 1768, nd_comb_decide 1224 |
| I entity and dlc | 12.6 KB | dlc_try 1464, pm_gpu_grange 1408, dlc_box_out 1280, ent_run 1092 |
| Union | 124.0 KB | spans 3.1 MB of .text: 26-39 lines per I-cache set against 4 ways |

- Console reading [E]: mac_01 40.8k I-miss, 22.4k D-miss per frame; at 30-60 cycles per miss about 1.6-3.8M of 4.49M cycles. Each
  command class touches 8-15 KB of code and the classes alternate, so each evicts the last: capacity misses, not only conflict.
- The linker hot group (3ds/ld/3dsx.ld.in:34-37) is stale [M]: it names the deleted nd_walk_run and pm_gbi_run, holds 21,036 B of the
  legacy triangle path and batch close, and only 20 functions of the 124 KB set. plan_direct section 0 "core 0 hot code about 23 KB"
  measures this group, not the code that runs.
- Tools [M, HC0]: nd_size_test.sh (missing nd_size_check.py) and hotspan.py (linker group only) are deleted. 3ds/tools/hotset.py
  replaces them: M1 per class from the call graph, M2 the linker group with its stale names. prof=30 prints the pmu counters
  ("prof f<n> w<n> pmu m <ic> <dc> cyc <c>", pm_phase.c) with no pmu switch; the pmu switch is off in prof mode by design.

## 1. Big contributors
1. Word decode and dedupe on core 0: nd_walk_state (51 cases, inlined handlers), g2_same/g2_note, op_* handlers, sc_sig_now/scm_sig.
2. Combine decision on core 0 (class B): decide, pm_comb_eval, cc_*, name_colour, fold_colour on each scm memo miss; pm_gpu_set_plan.
3. Multi-purpose functions: rec_run (miss path, ndm, waterlog, wskip, R11, POND), nd_texrect (box pattern, dual bind, t1 fold, depth
   query), nd_raw_native (ndm_cmp, orb, plan check, decal/zmark passes, wl_log), nd_model_state_in (every sprite draw).
4. Debug and census in hot functions [M]: 74 debug calls, 38 timer calls, 24 log calls, 16 pm_frame_count in the hand set.
   pm_tex_stale_chk + texsrc_chk (1.26 KB) on every texture hint hit (nd_tex.c:114); rect_sig_check (2.07 KB) on each rect batch
   close (pm_gpu.c:682); PMDL tests inline in rec_run, sync_combine, ensure_tile, nd_texrect, pm_gpu_tex_rect, pm_gpu_fill_rect,
   pm_gpu_spr_build, ent_run, nd_geom_vtx, nd_raw_native; NDT_BD/NDT_E timers and walk_dbg in nd_walk_state.
5. Division: minor (pm_gpu_tex_rect __divsi3, tile_texel __aeabi_idivmod). No double in the hot set.
6. citro3d/libctru per draw: C3Di_UpdateContext 1.26 KB, C3D_UpdateUniforms 656, shaderProgramConfigure 2.3 KB on a program switch,
   GPUCMD_Add, BufInfo_*, osConvertVirtToPhys; per split gpu_flush_written. None in the hot link group except C3D_TexBind.
7. Data: NdNatC about 290 B per cached draw, read by the hit loop; NdShadow about 1.9 KB, compared by 64 B memcmp per draw; NdWs.tst1
   about 700 B; pm_tex_slot_find scans 1024 x 136 B slots (139 KB) on a hint miss and on every model miss (pm_gpu.c:3054); stream_run
   has no PLD; big tables near the path: mc_keyw 368 KB, mc_pool 144 KB, tmpl_ent 140 KB, slots 136 KB, mc_tab 64 KB.

## 2. Stages (one row each)
End target: code that runs in one mac_01 frame <= 16 KB, any one command class <= 8 KB, the per-op loop <= 2 KB.
- HC0 MEASURE (tools only): 3ds/tools/hotset.py + hotset.json replace hotspan.py (M1 per class; M2 linker group bytes with a
  stale-name warning; per hot function its log/debug/timer/division/double calls). Bench hotset_size (tool kind); build.sh prints the
  hotset line after memcheck. fncov debug tool: a census lane build with -finstrument-functions on the draw TUs, call counts per
  function, switch fncov=<frame> prints what ran; hotset.py joins it into M3. Scenarios fncov_mac01_det, fncov_kmr10_det,
  fncov_mac00_det (cap about 15 s, until "fncov f<n> done"). nd_size_test.sh is deleted (HC0). Console baseline (user): spot_mac01_prof
  with pmu, plus kmr_10 and mac_00.
- HC1 COLD SPLIT (byte-equal): debug, census, log and rare branches into `cold, noinline` helpers, the switch tested at the call site
  through one global debug word (as pm_walk_dbg). pm_gpu.c debug groups (ndm_*, sbkw, rect_sig_*, flushlog, ndref, orb, cen) move to a
  new pm_gpu_dbg.c at -Os. Rare walk arms cold (G_SETCIMG, G_MODIFYVTX, G_SETCONVERT, G_SETKEY*, G_PM_ND, bad pointers).
  Gate: byte-equal spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold; M1 down >= 12 KB; M3 down >= 10%.
- HC2 LINK ORDER: a script writes 3ds/ld/hot_order.ld from fncov (by class, then calls per frame; citro3d/libctru by archive
  pattern); 3dsx.ld.in includes it; the hotset bench fails on a missing name. Optional -falign-functions=32 on hot TUs.
- HC3 -Os PER FILE (experiment): each draw TU in turn, then all; record M1/M3; one console pmu run all -Os vs -O2; keep -Os where
  cycles do not grow.
- HC4 TYPED STATE OPS (design: section 5): the recorder (core 1) translates each op into a typed NdOp and drops ops the walker
  would skip; core 0 runs typed setters by table. 4a scalar state, 4b combine and render mode table, 4c texture, 4d matrix and
  geometry. Deletes g2_same, g2_note, the word cases, tex_op, tb_ops and the op_* word wrappers. Target: class A <= 4 KB,
  nd_walk_state <= 0.8 KB.
- HC5 COMBINE OFF CORE 0 (section 5.3): the combine memo runs on core 1 (T_PLAN); core 0 applies the plan id. Target: class B <= 3 KB
  on core 0.
- HC6 MODEL HIT PATH (section 5.4): hit state equal to the miss state, key A on core 1, rec_run hit <= 600 B plus a cold rec_miss,
  a pre-built GPU command block per cached draw. Target: classes C+D <= 8 KB. Gate: byte-equal spot_mac01_det, mdl5_iwa10_det,
  spot_mac00_det.
- HC7 RECT PATH: NDC_TEXRECT/NDC_FILLRECT carry resolved fields; nd_texrect fast path <= 1 KB, rare paths cold. Target class G <= 5 KB.
  Gate: msgbox_talk_nowide, pause_spin_grab_det_w300, menu_open_det.
- HC8 SPRITE AND ENTITY: a sprite state cache keyed like the model cache; ent_run/dlc_try fast paths. Targets H <= 3 KB, I <= 5 KB.
  Gate: battle_start_f182_cold, nok11_det, fx_direct_det.
- HC9 DATA LAYOUT: NdNatC 64 B hot head; NdShadow compares by generation ids; NdWs tile tables native after HC4; PLD 64 B ahead in
  stream_run; hash index for pm_tex_slot_find; mc_keyw/mc_tab probes in one line. Target: console D-miss on mac_01 down >= 30%.
- HC10 CLOSE: regenerate hot_order.ld; delete the remaining word-path code (NDC_SPR/NDC_ENT fallback words, if DIRECT D11 (b) is
  done). Gate: M3 <= 16 KB on mac_01, class <= 8 KB, console I-miss per frame < 10k (stretch).
- Schedule (waves by file set, no two rows in one wave touch the same file; user 2026-11-09):
  - Wave 1 (now): HC2 link order (3ds/ld, tools; also the M1-hot line) | HC9A slot index (pm_tex.c lookup) | HC4 design only.
    HC1 closed 2026-11-09, premise false: 19.4 KB of the M1 union (39 functions, ndm_cmp 6.7 KB, rect_sig_check 2.0 KB) is already
    in .text.unlikely, and the inline debug left in hot roots is about 0.5 KB. M1 counts cold callees, so it cannot show a cold
    split; M1-hot (without .text.unlikely) is about 106.7 KB at 40be9ddc5. The real cut is HC4-HC6.
  - Wave 2 (after HC2 merges): HC4 code slices = DIRECT completion + HC4-HC6 (nd_walk.c, nd_recs.c, nd_rec*.c, pm_gpu.c,
    nd_comb.c, nd_model.c): alone in the draw core. In parallel only: HC3 -Os experiment (CMake flags; no source edits).
  - Wave 3 (after HC4 merges): HC7 rect path and HC8 sprite/entity (one at a time if both touch pm_gpu.c), HC9 rest (NdNatC head,
    NdShadow ids, PLD in stream_run).
  - Wave 4: DEADCODE (deletes across files: runs alone), then HC10 (link order regenerated, final sizes).
  - Rows outside the draw core (audio, menus, tools, git audit) may run in any wave.
- Optional after HC3 (user decision): PGO (-fprofile-generate in Azahar, -fprofile-use) for an automatic hot/cold block split.

## 3. Measurement
- M1 class sum (hotset.py, bench hotset_size, every build). M2 linker group bytes and stale names. M3 bytes run per frame and class
  (fncov lane, Azahar). Console pmu (prof, mac_01 0 0 200, plus kmr_10 and mac_00): one run per stage that passes the size gate.
- Correctness: byte-equal det shots per the DIRECT gate set; nd_host for the typed op format (extend nd_cmd_test.c).

## 4. Open questions
1. M3 bytes per frame; capacity vs conflict misses (fncov, pmu window B).
2. Does the pmu line count core 0 kernel, GSP and interrupt code?
3. PMU reads from user mode (expected no).
4. User: accept PGO and -Os on the draw files?
5. Answered (HC0): spot_mac01_prof prints the pmu counter line with the recipe as it is; no switch is needed.

## 5. HC4-HC6 design (completes DIRECT: no N64 command word decoded on core 0)
Base: main 5529d8bcf, [C] read in source, line numbers of that base. Done means (row_HC4): the nd_walk.c word cases, g2_same and g2_note
are deleted; no per-frame word decode on core 0; row_DIRECT.md is deleted.

### 5.0 Where N64 words reach core 0 now [C]
| Source | Reader on core 0 | When |
|---|---|---|
| W1 NDC_STATE runs (nd_rec.c:155 nd_rec_state: every nd_gbi.h macro, rec_inline, nd_rec_list) | nd_walk_state (nd_walk.c:290) | every frame, most words |
| W2 record words built from fields (rec_op nd_recs.c:25, rec_fog, shroud word), rec_lights stack list (nd_mtx_set.c:165) | rec_op, g2_same | model miss |
| W3 render mode list ModelRenderModes[i] (rec_dl nd_recs.c:51, 4 ops, static data) | nd_list_run | model miss |
| W4 bind list R0 w1 (tb_ops nd_tex_set.c:608, make_texture_gfx output) and the R13 words (rec_aux nd_recs.c:62) | tb_ops, nd_ops_run | model miss |
| W5 sprite fallback (NDC_SPR vertex and triangle words, nd_walk.c:193) | nd_ops_run | battle, imgfx |
| W6 entity G_DL fallback (nd_walk.c:209) and a push G_DL in a run (walk_call: ent_gdl, dlc_try, else the executor) | walk_call | entities |
| W7 ENT_GBI pieces of a flattened entity list (nd_ent.c:372), dlc interpreted group ranges (nd_shape.c:608) | nd_ops_run | refused groups |
| W8 static list readers: ent_compile and dlc compile on a cache miss, the shape fixup at map load (pm_map.c:63-190 ShapeFix: relocation and byte order), pm_gbi_fallback | once per list or map | not in a steady frame |

W1 to W8 go (user decision 2026-11-09: no N64 display list reader in the ELF, cold code included; reasons: ELF size and any
performance effect). Static lists are typed before the game runs (5.7 S1, S2); run-time builders call the native path (5.7 R).

### 5.1 The typed op (NdOp)
- One 8-byte unit, as the stream unit: w0 = T << 24 | a (24 bits), w1 = b. T = 0x20 + k (k < 32), free of the F3DEX2 and G_PM_* ids
  (nd_cmd_test.c checks it). The NDC_STATE payload holds NdOps only. One translator `nd_op_xlate(w0, w1, out)` (new nd_op.c, host-built;
  core 1, the gs* macros of 5.7 S1 and the host converter of 5.7 S2) writes 0, 1 or 2 NdOps per word (syncs and G_SPNOOP 0; G_TEXRECT stays NDC_TEXRECT).
- Core 0: one table dispatch per op (`nd_op_tab[k]`). The typed same-test of the old g2_same case is the first line of its setter, its
  g2_note part is the skip branch, gv_epoch++ follows the test for the ops the old prologue counted. Ops: T_OM (a = H/L, shift,
  length; b = value), T_GEOM (a = clear mask, b = set bits), T_COMB (a = w0 low 24 bits, b = w1), T_PRIM (a = lod, b), T_ENV, T_FOG,
  T_BLEND, T_FILL, T_PRIMZ, T_SCISSOR (corner pairs; the wide sign rule on core 1), T_TEXTURE (a = on, b = ss << 16 | ts), T_MW (a = kind
  and slot: segment, light colour, light count, fog factors), T_LIGHT (a = slot, b = pointer), T_MTX (a = flags, b = pointer), T_POPMTX,
  T_VTX (a = n, v0; b = pointer), T_TRI (a = first, b = second or ~0), T_TIMG, T_TILE, T_TILESIZE, T_LOADBLOCK, T_LOADTILE, T_LOADTLUT
  (decoded fields), T_CALL (b = list for ent_gdl and dlc_try), T_LST (G_PM_LST), T_ND (G_PM_ND), T_RARE (SETCONVERT, SETKEYR/GB,
  MODIFYVTX, SETCIMG; pre-decoded values, one cold setter).
- rec_op, rec_fog, the shroud word and rec_lights call the typed setters with fields: no word is built and decoded again (W2).
- W3: the recorder writes the typed ops of render mode list i once into a static table (static data; built on first use on core 1,
  immutable after, published by the frame push). W4: the recorder appends the typed bind ops and R13 ops to the NDC_MODEL payload.
  Premise check: stream KB per frame (prof ndc line) before and after, at most 2 chunks more on mac_01; else a recorder-side typed
  cache by list pointer, reset at map load. W5: two typed ops in the NDC_SPR payload. W6: the recorder resolves a push G_DL (host or
  segment address from its segment shadow) to T_CALL; a list that ent_gdl and dlc_try refuse at replay runs the typed ops of its
  typed static list (5.7), or for a dynamic list the typed range the recorder wrote after T_CALL (nd_rec_list rule).
- Core 0 then holds no word decoder: nd_walk_state becomes the typed loop nd_ops_exec (<= 0.8 KB with the table); g2_same, g2_note,
  tex_op, tb_ops, the op_* word wrappers and the 51 word cases go in the slice that types their last op.

### 5.2 Recorder shadow and its invalidation (core 1)
- The recorder drops an op only when the walker would skip it (old g2_same true). Per field it keeps a value and a valid bit, reset at
  task open, at nd_rec_mark, nd_sub_open/close and nd_rec_end (a range replays elsewhere: NDC_CALL; nd_range_run sets sc_ok = 0), and at
  every command other than NDC_STATE: NDC_MODEL (a hit keeps the old walker state, a miss leaves the record's: rec_run nd_recs.c:131 vs
  rec_apply; both set sc_ok = 0 and nd_sr_forget), NDC_SPR and NDC_ENT (native draws: sc_ok = 0), NDC_TEXRECT (box_pattern_rect
  clears sc_ok), NDC_CALL.
- Ops whose skip reads sc_ok and sc_sig (combine, texture, prim, env, othermode H) drop only with no barrier between the emitted equal
  op and this one. Barriers inside a run: a draw op (T_TRI, T_VTX: box_fold_sync and dual_bind run at a draw), a tile, load or image op
  (td[1].valid and dual_on, nd_tex_set.c:189 and 548), T_CALL, T_LST, T_ND. Ops whose skip reads only nd_ws (othermode L, fog, blend,
  geometry, light colour) drop while the field is valid.
- The shadow is core 1 data only (nd_rec.c); core 0 reads none of it (hw rule: no shared table).
- Premise check (debug switch t4chk, cold): the recorder writes each dropped op as T_DROP; core 0 runs the old same-test on it and
  counts drops it would not skip ("t4chk f<n> drops <d> bad <b>"). Gate: bad 0 on the slice scenes. Typing without dedupe is
  byte-equal by construction; the dedupe follows in the same stage.

### 5.3 Combine off core 0 (HC5): the walker-state reads of decide and sc_sig_now [C]
- Inputs (nd_comb.c:381, 435, 466): comb_w0/w1, omode_h cycle bits, prim, prim_lod low byte, env, keyc, keys, conv_k4 are op values.
  tex_on, use_t1, comb_set, comb_alpha_shade, box_prod and the tst comb fields derive from the ops (op_setcombine decode). leg_mode and
  box_pat are outputs of the last sync. td[1].valid and dual_on come from the texture model (core 1 after HC4c). box_folded comes from
  tile0_white, a read of tile 0 texels at a draw (box_fold_sync): core 0 only. After NDC_MODEL all are unknown (hit vs miss, 5.2).
- Design: the recorder runs the scm memo of sync_combine on its shadow and writes T_PLAN (b = plan id in a plan table written by core 1,
  published with the frame; a = legacy mode) when every input is valid and box_prod is 0; else T_SYNC (core 0 runs the old sync, cold).
  Premise check: census of T_PLAN vs T_SYNC per frame (switch c5cen); class B <= 3 KB on core 0 needs T_SYNC < 5% of syncs on mac_01.
  Before HC6 the first sync after NDC_MODEL is T_SYNC.

### 5.4 Model hit (HC6)
- Premise 1 (hit state): a hit skips rec_apply, so the walker keeps the pre-record omode, combine, prim, fog and tile state; a miss
  keeps the record's. HC6 sets the record's scalar state on a hit too (typed copy, no GPU call), so the state after NDC_MODEL is known
  to the recorder. Check: census hitst of walker draws after a hit that read a field the hit left stale, by field (mac_01, mdl5_iwa10,
  mac_00); the byte-equal gate decides; a pixel change keeps the old rule.
- Premise 2 (key): pm_gpu_model_key (pm_gpu.c:2976) reads record words, nd_ws (omode, om_k, geom, tss, combine, prim) and core 0
  state (gs.r_mix, r_mixcol, cconst, decal_bias, dview, r_memblend, r_replace, gv_wind, zmark, r_decal, r_blend, off_active, ui_ortho,
  nd_handle_gen, pm_dlc_gen, pm_gbi_gvkey). Split: the record part and the op-known part hashed on core 1 into the payload (key A);
  core 0 builds key B from its render shadow. Check: switch mkchk, key A + B against the old key on every record, 0 differences.
- Fast path: rec_run hit <= 600 B (memo by record address, key B compare, vis test, matrix, submit loop); all else in a cold rec_miss
  (rec_apply, ndm, waterlog, wskip, R11, POND, stale check).
- Pre-built command block: the miss records for each cached draw the GPU command words of pm_gpu_native_submit_draw into a linear
  block with the full state (no redundant-state filter) and the offsets of the per-frame words: matrix palette rows (pal0), the zmark
  row, the shroud TEV colour (mix_st), unit 0 address and dim when e->tex, the template program when tgen moved. A hit copies the
  block (aligned) into the command buffer and patches. Check: switch blkchk compares the hit's command span old vs block per draw (0
  differences outside the filtered state words; picture byte-equal). More GPU words per hit: console cycles decide.

### 5.5 Core 1 budget
- Logic step at 30 fps: 33.3 ms on core 1 (O3DS, up to 20% system work). Measure "core2 lgms" of the prof line at each stage
  (console push mac_01 0 0 200). Estimate [E]: translate and shadow about 60 cycles per op, about 4k ops per frame on mac_01 = 0.9 ms;
  combine memo 0.3 ms; key A 0.2 ms. Limit per stage: lgms grows <= 2 ms and stays < 30 ms; over it, that work stays on core 0.

### 5.6 Slices (wave 2, after HC2 merges; each a commit after its gates)
| Slice | Content | Files | Deletes |
|---|---|---|---|
| HC4a | nd_op.c translator and ids, typed executor for the W1 scalar ops (OM, GEOM, PRIM, ENV, FOG, BLEND, FILL, PRIMZ, SCISSOR, TEXTURE, MW, RARE), typed rec_op/rec_fog/shroud, nd_cmd_test.c | nd_op.c (new), nd_cmd.h, nd_rec.c, nd_walk.c, nd_set.c, nd_recs.c, nd_int.h, 3ds/tools/nd_cmd_test.c, CMakeLists.txt | g2_same/g2_note cases of these ops |
| HC4b | T_COMB, render mode table (W3), shadow dedupe and t4chk | nd_rec.c, nd_recs.c, nd_comb.c, nd_walk.c | rec_dl list path |
| HC4c | texture ops typed, bind and R13 ops in the payload (W4), R1 and R2 typed at their producer | nd_tex_set.c, nd_recs.c, nd_rec.c, nd_ent.c, nd_shape.c | tex_op, tb_ops, op_* wrappers |
| HC4d | T_MTX/POPMTX/LIGHT/VTX/TRI, sprite fallback (W5), T_CALL (W6), typed rec_lights, run-time producers R1-R9 (5.7) | nd_mtx_set.c, nd_geom.c, nd_walk.c, nd_rec.c, nd_ent.c | word cases, g2_same, g2_note, walk_call word path, row_DIRECT.md |
| HC4e | S1: typed gs* macros (ELF static lists), consumers (ent, dlc, sprite rmList, entity displayList) read typed ops | nd_gbi.h, nd_op.h, nd_ent.c, nd_shape.c, pm_gpu.c (sprite) | the word readers of ent_compile and dlc |
| HC4f | S2: host shape converter, sidecar loader, node pointers to typed lists | 3ds/tools/nd_shape_conv.c (new, host), build.sh, pm_map.c | ShapeFix word walk (pm_map.c:63-190) |
| HC5 | combine memo on core 1, T_PLAN/T_SYNC | nd_comb.c, pm_comb.c, nd_rec.c, pm_gpu.c (set_plan) | - |
| HC6 | hit state, key A/B, fast path, command block | nd_recs.c, pm_gpu.c, nd_model.c, nd_emit.c | - |
- Gates per slice: nd_host PASS; byte-equal spot_mac01_det and battle_start_f182_cold against stored base shots (plus mdl5_iwa10_det
  and spot_mac00_det for HC4b and HC6; the DIRECT set at the end of HC4d); hotset_size M1 and fncov M3 per class before and after.
  Done check after HC4f: no function in the ELF compares a list word to a G_* op (a grep of the source for `>> 24` against
  G_*/OP_* in 3ds/src and src, listed in the slice commit). Targets: class A <= 4 KB after HC4d, B <= 3 KB on core 0 after HC5, C+D <= 8 KB after HC6. Console per stage (user): prof push
  mac_01 0 0 200 (pmu_i, pmu_d, cycles, lgms).
- A try with no change in the gate quantity is not repeated in that area; the facts go to the orchestrator (user, 2026-11-09).

### 5.7 No display list reader in the ELF (user decision 2026-11-09)
- One encoding source: nd_op.h defines each typed op as integer constant expressions (`ND_OP_<T>_W0(...)`, `ND_OP_<T>_W1(...)`), used
  by nd_op.c (host and core 1 translator), by the gs* macros (S1) and by the shape converter (S2). nd_cmd_test.c checks all three
  give the same units for every F3DEX2 op the game uses.
- S1, static lists compiled into the ELF [C]: assets/*.gfx.inc.c (856 files, 986 includes), src gs lists (121 files, about 8.7k gs
  macros: entity 57 files, world 15, battle 11, effects 9, menus), sprite render mode lists (sprite.c rmList, spr_final_list), the entity
  model lists (entity_model.c:347 model->gfx.displayList). nd_gbi.h redefines the static gs* forms as typed unit initializers (a
  translation by the compiler: no reader, no host step). gsSPDisplayList becomes T_CALL, gsSPBranchList T_JMP, gsSPEndDisplayList
  T_END, the syncs a T_NOP unit (a static array keeps its length; the dlc and ent compilers skip T_NOP). Segment addresses stay values:
  their resolution through the segment table is a pointer step, not a list read. Premise check: the ELF .data size before and after
  (memcheck line); equal size expected, since every gs macro stays one 8-byte unit.
- S2, map shapes (ROM data loaded per map into gMapShapeData, pm_map.c:37; pm_map.c:63-190 walks the list words at load to relocate
  and swap them): a host C converter `3ds/tools/nd_shape_conv.c` (built and run by build.sh; user decision 2026-11-09: build time only,
  no first-boot path) reads the N64 shape file and writes a sidecar per map:
  `NdShp { magic, version, n_lists, n_ops, n_reloc; u32 list_off[n_lists] (N64 list offset -> typed op index, sorted); NdOp ops[n_ops];
  u32 reloc[n_reloc] (op indices whose b is a shape offset) }`, little-endian, 8-byte aligned, position-free (offsets, relocated by one
  pass over reloc at load: no list read). The loader rewrites each ModelNode display list pointer to its typed list (a node walk, not a
  word walk). build.sh writes one sidecar per map into romfs next to the shape file.
  Size [E, measured by the converter's report line per map and in total for the 411 maps]: typed ops <= the N64 list bytes (one unit per
  word, syncs dropped), plus 4 B per list and 4 B per relocated op; the vertex, matrix, light and texture data stay in the shape file.
  The gMapShapeData buffer (256 KB) then holds the shape data without its lists plus the typed lists.
- R, lists that game code builds or changes at run time [C]; each producer calls the native path or writes typed ops (as DIRECT did):

| Id | File:line | What | Size class | Slice |
|---|---|---|---|---|
| R1 | src/model.c:3486-3744 make_texture_gfx | texture bind list per handle, at texture load (W4) | medium | HC4c |
| R2 | src/model.c:1390-2361 appendGfx_model | R13 aux words built with gDP macros (1569-1655, 1901-2195); record fields | medium | HC4c |
| R3 | src/model.c:3418-3482 render_transform_group(_node) | gSPMatrix/gSPPopMatrix into the group list | small | HC4d |
| R4 | src/model.c:4487 mdl_local_gfx_update_vtx_pointers | copies a node list, rewrites G_VTX pointers (reads words) | small | HC4d |
| R5 | src/animator.c:854-973 appendGfx_animator_node | copied mesh list with new vertex buffers per frame (reads words) | medium | HC4d |
| R6 | src/imgfx.c:1252-1320 imgfx_load_anim | patches vertex pointers in a copied list (reads words) | small | HC4d |
| R7 | src/sprite.c:325 | spr_geo_out vertex and triangle words (W5) | small | HC4d |
| R8 | src/os/nusys/nugfxinit.c:75 | boot list (G_DL rdpstateinit_dl, full sync) | small | HC4d |
| R9 | 3ds/src/nd_mtx_set.c:150-170 rec_lights; nd_walk.c:500 pm_gbi_fallback; nd_rec.c:195/285 rec_inline, nd_list | port-side builders and readers | small | HC4d |
| R10 | src/effects/hieroglyphs.c:151 (savedGfxPos), src/effects/underwater.c:172-256, src/world/dead/area_flo/flo_10/waterfx.c:308 | position patches and local lists (D5 audit); check each is typed or dead | small | HC4d |
- Typed static lists change the pointer-typed consumers: dlc compile (nd_shape.c), ent_compile (nd_ent.c), sprite rmList, the model
  record R12 geometry pointer and ModelRenderModes (W3: then a static typed table from S1, no recorder build).
