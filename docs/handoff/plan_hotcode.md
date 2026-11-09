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
- HC4 TYPED STATE OPS (plan_direct section 5): the recorder (core 1) decodes each op into a typed NdOp and drops ops equal to its
  shadow (the shadow is invalidated at every NDC_MODEL/NDC_SPR/NDC_ENT that can draw natively). Core 0: a table of about 12 typed
  setters. 4a scalar state, 4b combine (words + input key, scm memo on core 0), 4c texture tile/load/TLUT (resolved tile
  descriptors), 4d matrix (NDC_MTX). Deletes g2_same, g2_note, the word cases and the op_* decode. Target: class A <= 4 KB,
  nd_walk_state <= 0.8 KB.
- HC5 COMBINE OFF CORE 0: the recorder computes the plan key and plan with a memo on core 1; core 0 applies the plan id. Premise
  check first: every walker-state read of sc_sig_now and decide. Target: class B <= 3 KB on core 0; report core 1 lgms.
- HC6 MODEL HIT PATH: rec_run hit fast path <= 600 B plus a cold rec_miss; pm_gpu_model_key on the recorder; each cached draw keeps a
  pre-built GPU command block, and the hit copies it and patches the per-frame words; inline hint check, cold stale check.
  Target: classes C+D <= 8 KB. Gate: byte-equal spot_mac01_det, mdl5_iwa10_det, spot_mac00_det; console cycles.
- HC7 RECT PATH: NDC_TEXRECT/NDC_FILLRECT carry resolved fields; nd_texrect fast path <= 1 KB, rare paths cold. Target class G <= 5 KB.
  Gate: msgbox_talk_nowide, pause_spin_grab_det_w300, menu_open_det.
- HC8 SPRITE AND ENTITY: a sprite state cache keyed like the model cache; ent_run/dlc_try fast paths. Targets H <= 3 KB, I <= 5 KB.
  Gate: battle_start_f182_cold, nok11_det, fx_direct_det.
- HC9 DATA LAYOUT: NdNatC 64 B hot head; NdShadow compares by generation ids; NdWs tile tables native after HC4; PLD 64 B ahead in
  stream_run; hash index for pm_tex_slot_find; mc_keyw/mc_tab probes in one line. Target: console D-miss on mac_01 down >= 30%.
- HC10 CLOSE: regenerate hot_order.ld; delete the remaining word-path code (NDC_SPR/NDC_ENT fallback words, if DIRECT D11 (b) is
  done). Gate: M3 <= 16 KB on mac_01, class <= 8 KB, console I-miss per frame < 10k (stretch).
- Order: HC0-HC3 are cheap and byte-equal; HC4-HC6 are the structural cut (they need typed ops); HC7-HC9 by class size.
- Optional after HC3 (user decision): PGO (-fprofile-generate in Azahar, -fprofile-use) for an automatic hot/cold block split.

## 3. Measurement
- M1 class sum (hotset.py, bench hotset_size, every build). M2 linker group bytes and stale names. M3 bytes run per frame and class
  (fncov lane, Azahar). Console pmu (prof, mac_01 0 0 200, plus kmr_10 and mac_00): one run per stage that passes the size gate.
- Correctness: byte-equal det shots per the DIRECT gate set; nd_host for the typed op format (extend nd_cmd_test.c).

## 4. Open questions
1. M3 bytes per frame; capacity vs conflict misses (fncov, pmu window B).
2. Does the pmu line count core 0 kernel, GSP and interrupt code?
3. PMU reads from user mode (expected no).
4. HC4 record-side dedupe must model the native-draw invalidations (sc_ok = 0, nd_sr_forget).
5. Core 1 budget at 60 fps: the moved decode must fit about 8 ms; measure lgms per stage.
6. User: accept PGO and -Os on the draw files?
7. Answered (HC0): spot_mac01_prof prints the pmu counter line with the recipe as it is; no switch is needed.
