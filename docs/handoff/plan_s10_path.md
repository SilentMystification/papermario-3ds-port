# Plan S10: pm_gbi.c deleted (oracle re-plan, 2026-11-06)

User decision 2026-11-06: commit to S10. Model (oracle review 5b2ee3c48): the N64 word is the record, one dispatcher, no opt-in, G_PM_ND only for native-only payloads. This file replaces the earlier S10 path plan (opt-in streams UI, FXW, WBO, ND_* record kinds, record-mode leaves: obsolete). Running rows CORE-3, MDL, RECT-3/PM5-4 continue.

## What is left
The model now (review 5b2ee3c48): the word is the record, and one dispatcher runs it. There is no opt-in. G_PM_ND is only for native-only payloads. pm_gbi.c (5204 lines at 5b2ee3c48) still holds the dispatcher and every handler. The function groups (grep the function definitions in 3ds/src/pm_gbi.c), with their target:
| group | functions (anchors) | target |
|---|---|---|
| W walk | gbi_run_body (pc loop, stack, G_DL/G_ENDDL/G_DL_NOPUSH, orb_tag), pm_gbi_run, is_host, host_range, ptr_of, g1_skip, g2_same, g2_note, gv_epoch, gbi_cold_bad_dl/next/vtx/mtx, gbi_cold_dd_load, gbi_cold_modifyvtx, pm_gbi_fallback, pm_gbi_init | nd_walk.c (new) |
| S state | set_geom, set_scissor, set_primdepth, set_fillcolor, set_blendcolor, set_fogcolor, op_setprimcolor, op_setothermode, gbi_setenv, gbi_moveword (not the segment part), sync_render, rm_apply/rm_apply_impl | nd_state.c |
| C combine | sync_combine, sync_combine_work(_timed), nd_comb_decide, decide, scm_* memo, sc_sig_now, comb_in_walker, cc_* (cc_decode, cc_eval, cc_rdp, cc_cycle1, cc_rgb_in, cc_text, cc_uses, cc_const_in, cc_alpha_k), const_rgb_tex_alpha, dual_bind, t1_fold, tile0_white, box_fold_sync, op_setcombine, pm_gbi_sticky, gvk_bits, pm_gbi_gvkey. cc_legacy, sync_legacy, set_leg are deleted once the CPU batch is gone (RECT-3) | nd_comb.c (new) |
| T texture | set_timg/op_settimg, set_tile/op_settile, set_loadblock, set_loadtlut, set_tilesize/op_settilesize, set_texture/op_texture, tex_op, tex_op_impl, tb_inline, tb_ops, tb_ops_generic, ts_save, ts_load, tb_diff, ensure_tile, tile_bound, tile_texel, glyph_atlas_bound, load_shaded_sprite, tex_win_s25, tlut_v20, tex_tile1, tex_shaded0, pal_rgba, unpack8, rgba_from_5551, shift_mul, fb_prev_base, ens_clear, tex_streak_* | nd_tex.c |
| G geometry | G_VTX case, draw_tri, gv_tri, gv_state_why, gv_epoch_enter, gv_params_calc, pm_gbi_gv_at, gv_xf, lit_freeze, gv_lit_ok, gv_fog_gpu_ok, tri_flags, cull_off, cull_cross, clip_bits(4), gbi_vtx_cpu1. CPU only, deleted: draw_tri_cpu, xform_vtx, vert_color, vert_uv, pack_color, lit_color, fold_axis | nd_geom.c (new) |
| M matrix | load_mtx, mtx_elem, gbi_popmtx, upload_mvp, pm_gbi_eye_changed, gbi_viewport, gbi_movemem, rec_lights, gm_hash, gv_matrix_id, mtx_cache/mtx_tag | nd_mtx.c |
| R rect and target | CORE-3 running: gbi_texrect, gbi_fillrect, texr_early. Plus rect_to_image, box_pattern_rect, set_cimg/gbi_setcimg, img_role, grab_cur, off_screen_check, depth_of_word, word_5551, pm_depth_query, prim_ndc_z | nd_rect.c; nd_target.c (new) for cimg, role, grab, depth |
| D map shapes | dlc_* (dlc_try, dlc_seg, dlc_build, dlc_batch_compile, dlc_static_group, dlc_run_group, dlc_find, dlc_hash, dlc_free, dlc_reset, dlc_report), gp1_box_*, nd_env_*, pm_dlc_native_ranges, pm_gbi_dlcc, dnr | nd_shape.c (new), next to nd_dlc.c (compile) |
| E entities | ent_gdl, ent_draw, ent_run, ent_compile, ent_flat, ent_hash(_walk), ent_lookup, ent_find, ent_segkey, ent_reset, ent_refcheck, ent_log, ent_seg_native, pm_gbi_ent_texchk/segchk | nd_ent.c (new) |
| P records | rec_run, rec_apply, rec_matrix, rec_op, rec_fog, rec_fogpos, rec_dl, rec_aux (G_PM_REC); pm_gpu_spr_rec plus the G_PM_SPR case; the G_PM_ENT case | nd_recs.c (new) for REC; nd_sprite.c for SPR; nd_ent.c for ENT |
| K census | pm_gpu_cen_mark, pm_gbi_draw_note, pm_gbi_prod_mark, os_* (os_report, os_op_hook, os_win, os_close, os_calibrate, os_class_of, os_dlc_hit), pk_*, mdc_*, pm_gbi_census_close, pm_gbi_mdl_mark, pm_gbi_task_id, pm_gbi_win_reset, pm_gbi_end_frame, pm_gbi_stats | nd_cen.c (new, cold) |
| X debug, deleted | dd_* drawlog, suntile_*, tritrace_*, tri_dd_log, tri_rm_log, rmlog_on, combchk_*, cc07_dbg, dlcchk_on, stdump_on, skipdump_on, nobox(int)_on, behroute_on, xt_on, gv1chk_on, ph_pick, gp_*/gp3_*, sp0_*, mr0_*, ms0_*, mtx_dbg, tri_stats, ep_flush_stats, lcg_f, the orbdbg blocks (after FX closes) | deleted in the move row of its group, unless an open row names it (D9) |
| Y sun | pm_sun_spawn, pm_sun_draw, pm_sun_base | nd_geom.c if live; else delete (grep callers) |
Producer hacks:
- G_PM_TEXI: grep `G_PM_TEXI|gsSPPmTexInline` in src. Sites: hud_element.c HT1 x2, msg_draw.c GLYPH, entity/Shadow.c EN2, sprite.c NP1. The header makes tb_inline run the n texture words that follow.
- G_PM_RM: sprite.c SPR_RM_LIST, entity_model.c rm_first. rm_apply applies a render-mode list in place, with a G_DL fallback.
- Splash mark: water_splash.c gPmSplashMark (a G_NOOP carrying a value).
- List mutation: the G_PM_SPR case writes G_NOOP over the record's vertex and triangle words (srec[k] = 0). A replay or the eye-2 walk then sees changed memory.
- Side-buffer lists (animator heap lists, windows, effect sub-lists): G_DL into frame memory, walked or compiled (ent_gdl). No special case.
- Interp key replay (pm_nusys.c pm_interp_key_draw, then gbi_task over key_list) and the stereo eye-2 walk (nd_emit twice): both walk the same list memory again.
- nd_core.c LEGACY (nd_bridge_cut, nd_emit, nd_legacy): a wrapper with one span per task.
Obsolete text, to be marked when the docs are rewritten:
- plan_s10_path.md: everything after the CORE-2 oracle plan (opt-in streams UI, FXW, WBO; ND_* kinds; the record-mode leaves).
- plan_walker_end.md: RECT-1/2, TRI-1..3, OFF-1..3, PM5-1..3 are done.
- Still open from WEND: RECT-3, PM5-4, TRI-4 (the CPU deletes), MDL-1..4, CEN-2, END-1.

## Design
Dispatcher:
- nd_walk.c owns the pc loop: the G_DL stack of 12, segments, the host range check, the g1/g2 skip. It dispatches by `switch (op)`, which compiles to a jump table.
- Each case calls a field decoder plus a setter, or a handler, in the owning module.
- Prototypes go in a new internal header 3ds/include/nd_int.h (one section per module, each section owned by its stream).
Field decode lives in nd_ops.h as static inline pure functions (nd_dec_settile, nd_dec_othermode, ...). The walk and nd_dlc's compile both call them, so there is one decoder. nd_words (the host reference) also calls them.
State:
- nd_ws (NdWs) is the only render state. Module-private hot globals (vertex slots vert_live/vert_ring, the tile set td[], the matrix stack) stay static in their owning module, with accessors in nd_int.h.
- A global read by 2 or more modules moves into NdWs. A global written by 2 modules is not allowed: the writer owns it.
G_PM_ND (new opcode, the next free custom code) carries only payloads that have no N64 encoding:
- SPLASH(on/off);
- TEXBIND, only if D6 measures a cost.
Nothing else. Each G_PM_ND kind gets one handler in its own module.
Word records: G_PM_REC, G_PM_SPR and G_PM_ENT stay as words; only their handlers move. G_PM_TEXI and G_PM_RM are deleted (D6, D7).
Replay rule: no handler writes list memory. G_PM_SPR keeps a skip in its own record word instead of NOOPing the words. A walk is a pure function of list memory plus nd_ws at entry. Key replay and eye 2 are then safe.
Hot path (docs/handoff/hw_guidelines.md, rules 1, 5, 6, 10):
- The loop plus VTX, TRI1/2, MTX, setothermode, setcombine, DL/ENDDL, settile and the loads stay in the hot span (S10-3 linker order).
- Every census and debug function is `cold, noinline` in nd_cen.c.
- No function pointer per vertex. The g2_same skip stays before the dispatch.
- Each move row reports the sampled gbi-phase us at mac_01 (opsplit) and must stay within +3% of the row's base.

## Streams
| id | owns | waits for |
|---|---|---|
| WK walk, census, target, debug | nd_walk.c, nd_int.h (skeleton plus W section), nd_cen.c, nd_target.c; groups W, K, X, R-target | CORE-3 (it owns nd_rect.c and the texrect group) |
| ST state and combine | nd_state.c, nd_comb.c; groups S, C; G_PM_RM removal (sprite.c, entity_model.c) | WK-1 (nd_int.h); RECT-3 for the cc_legacy delete |
| TX texture | nd_tex.c; group T; G_PM_TEXI removal (msg_draw.c, Shadow.c, sprite.c, hud_element.c last per D11) | WK-1; MDL-1 (rec_aux reads the T globals) |
| GM geometry and matrix | nd_geom.c, nd_mtx.c; groups G, M, Y | WK-1; TRI-4 for the CPU-only deletes |
| LS lists and records | nd_shape.c, nd_ent.c, nd_recs.c, nd_sprite.c; groups D, E, P; splash G_PM_ND; the SPR no-mutation fix | WK-1; MDL-1..4 (rec_* and nd_model.c); CORE-3 |
pm_gbi.c is the shared file. Each stream cuts only its own groups. A switch case line belongs to the stream of that op, and WK owns the loop structure. Rebase before each commit. Two streams never cut the same function.
Merge order:
1. CORE-3, then WK-1, then all the rest in parallel.
2. MDL-1..4 before LS-3. TRI-4 and RECT-3 before GM-3 and ST-3.
3. S10 after every stream's last row and WEND END-1.

## Rows
Gates used below:
- Move rows: mac_01 byte-equal (spot_mac01_pic, 3 shots); iwa_10 and battle_start by eye; the census per handler module (WK-2) shows the group's ops in the new module and 0 in pm_gbi; gbi-phase us within +3%.
- Done proof for a move row: `grep -n "<each moved function>(" 3ds/src/pm_gbi.c` finds no definition.
- Producer rows: picture by eye at the named scene, plus the producer's ops in WCEN.
WK-1 SKELETON:
- Create nd_int.h with empty sections and nd_walk.c.
- Move group W: the loop as nd_walk_run; pm_gbi_run becomes a one-line call to it for now.
- 150 lines. Gate: move gate.
WK-2 CENSUS: move group K to nd_cen.c and add a module column to os_report (op handled in module m). Gate: move gate, plus `prod` lines show per-module op counts.
WK-3 TARGET: move set_cimg/gbi_setcimg, img_role, grab_cur, off_screen_check, depth_of_word, word_5551, pm_depth_query and prim_ndc_z to nd_target.c. Gate: move gate plus pause_spin_grab by eye.
WK-4 DEBUG: delete group X. For each switch: grep the docs/handoff row files; if no open row names it, delete it (D9). Gate: build; mac_01 byte-equal; `grep -c 'PMDL("' pm_gbi.c` goes down by the switches removed.
ST-1: move group S (the setters) to nd_state.c. Gate: move gate.
ST-2: move group C to nd_comb.c, 2 rows if over 150 lines: decide/scm/cc_* first, then sync_*/dual_bind/t1_fold. Gate: move gate plus combchk equal, if still alive.
ST-3: delete cc_legacy, sync_legacy and set_leg (after RECT-3). Gate: build; byte-equal.
ST-4 RM:
- sprite.c SPR_RM_LIST and entity_model.c rm_first write plain gSPDisplayList.
- The G_DL handler gets a memo: a static list that holds only state ops is applied through the cached NdMode delta (rm_apply's nd_mode_compile moves into the G_DL path).
- Delete the G_PM_RM case and gSPPmRenderMode.
- Gate: byte-equal at mac_01 and battle_start; grep `gSPPmRenderMode|SPR_RM_LIST|G_PM_RM` empty.
TX-1: move set_*/op_* tile, timg, load, tilesize, texture (group T, part 1). Gate: move gate.
TX-2: move tex_op/tex_op_impl, ensure_tile, tile_texel, tile_bound, the tex_* forms, pal_rgba, unpack8 and shift_mul. Gate: move gate plus entcheck_mac01 texchk diff 0.
TX-3: move tb_inline/tb_ops/ts_*/tb_diff, glyph_atlas_bound and load_shaded_sprite. Gate: move gate plus text_menu by eye.
TX-4 TEXI (one producer per row: msg_draw.c, then Shadow.c, then sprite.c, then hud_element.c):
- Delete the G_PM_TEXI header; its words run through the normal texture handlers.
- Gate: the scene by eye (text_menu, coin_kmr03, battle_start, ui1_choice) and the gbi-phase us within +3%. If the us rises above that, stop and apply D6 (TEXBIND).
- Last row deletes the G_PM_TEXI case and gsSPPmTexInline. Grep empty.
GM-1: move group M to nd_mtx.c. Gate: move gate plus yard_stereo_l by eye (eye-2 matrices).
GM-2: move the G_VTX case and draw_tri, gv_tri, gv_* and the tri/cull/clip helpers to nd_geom.c. Gate: move gate plus iwa_10 and nok11_splash by eye.
GM-3: delete the CPU-only functions (draw_tri_cpu, xform_vtx, vert_color, vert_uv, pack_color, lit_color, fold_axis, gbi_vtx_cpu1), after TRI-4. Gate: build; byte-equal; `grep draw_tri_cpu` empty.
GM-4 SUN: grep the callers of pm_sun_*. Move to nd_geom.c if used, else delete. Gate: sbk56 sun by eye.
LS-1: move group D (dlc_*, gp1_box, nd_env, pm_dlc_native_ranges) to nd_shape.c. Gate: move gate plus the dlc report lines equal.
LS-2: move group E to nd_ent.c, including the G_PM_ENT case. Gate: move gate; entcheck_mac01 and entcheck_kmr03 diff 0.
LS-3:
- Move group P (rec_*) to nd_recs.c, after MDL-4.
- Move the G_PM_SPR handler and pm_gpu_spr_rec to nd_sprite.c.
- SPR no-mutation: the record carries the offset of its vertex and triangle words, and the handler skips them, instead of writing G_NOOP.
- Gate: move gate; an interp-on run (key replay) byte-equal on 3 frames; yard_stereo_l by eye.
LS-4 SPLASH: water_splash.c writes G_PM_ND SPLASH; delete gPmSplashMark and the NOOP value read. Gate: nok11_splash by eye; grep `gPmSplashMark` empty.

## S10 rows
Precondition (all must hold):
1. WK-2 census over the full catalog: 0 ops handled in module pm_gbi, and the legacy-handler counter is 0.
2. The grep `G_PM_TEXI|gsSPPmTexInline|gSPPmRenderMode|SPR_RM_LIST|gPmSplashMark|G_PM_RM` over src and 3ds is empty.
3. wd = 0 (WEND).
4. pm_gbi.c holds only the pm_gbi_run shim: `grep -cE "^[a-z].*\(" pm_gbi.c` returns 1 or less.
5. END-1 done: opcodes with 0 hits and no caller deleted.
S10-1 DELETE: delete pm_gbi.c and pm_gbi.h. Callers use nd_walk_run (pm_nusys.c gbi_task, the nd_dlc pm_gbi_run sites, and the entity segment state runs, which become nd_walk_run). CMake line. Gate: build links; mac_01 byte-equal.
S10-2 BRIDGE: delete the LEGACY parts of nd_core.c (nd_bridge_cut, nd_legacy, ND_LEGACY). gbi_task calls nd_walk_run once per eye. Delete nd_order and nd_eye_copy if a grep finds no caller. Gate: build; an interp-on run and yard_stereo_l right.
S10-3 HOT: 3ds/ld/3dsx.ld.in hot span = nd_walk_run, the hot handlers and nd_native_submit in call order; cold nd files at -Os. Gate: nd_size_test.sh (8 KB or less); map file; gbi-phase us not worse than the pre-S10 base.
S10-4 FULL: full catalog once (epic merge) and a console prof push. Numbers go in docs/handoff/agent_S10.md.

## Decisions
- D1 The word is the record. No re-encoding, no force-include, no ND_* record structs.
- D2 One decoder: the nd_ops.h inline field decoders, used by the walk, nd_dlc and nd_words.
- D3 G_PM_ND carries only SPLASH (and TEXBIND under D6).
- D4 G_PM_REC, G_PM_SPR and G_PM_ENT stay as words; only their handlers move.
- D5 Module map as in the table. New files: nd_walk.c, nd_comb.c, nd_geom.c, nd_target.c, nd_shape.c, nd_ent.c, nd_recs.c, nd_cen.c, nd_int.h. Haiku adds no other file.
- D6 G_PM_TEXI goes back to plain words. TEXBIND (G_PM_ND, slot key) is added only if a TX-4 row measures the gbi-phase us above +3%.
- D7 G_PM_RM goes back to G_DL plus the state-list memo in the G_DL handler.
- D8 No handler writes list memory (the replay rule).
- D9 A debug switch survives a move only if an open row file names it. Otherwise it is deleted in that move. The census (K) stays, cold.
- D10 Ownership of globals: the writing module owns a global. A global shared by modules goes in NdWs. Two modules never write the same global.
- D11 hud_element.c and the status bar files are touched only after the HUD1 worktree commits, and as the last TX-4 row.
- D12 Each move is a pure cut and paste plus prototypes, with no logic change in the same row (byte-equal proves it). A fix found during a move becomes its own row.
- D13 Two tries per named cause, then hand back with the measurement.

## Risks
- Hidden coupling through static globals in pm_gbi.c (vertex slots, td[], sc_ok/sr_ok, gv_epoch, orb_tag). Each move row starts with a grep of the globals the group touches, and applies D10 to them.
- Inlining lost across files raises walk cost. Measure per row (the +3% rule); S10-3 restores locality with the hot span.
- Moves collide with the running rows: MDL-1 (rec_*, the T globals), CORE-3 (texrect), FX (orbdbg blocks in the loop). Follow the merge order; LS-3 and WK-4 wait for them.
- The replay and stereo walks hide mutation bugs. LS-3's gate runs both.
- Azahar is not the console: one console push at S10-4.
- Census attribution is per op under the cursor. The module column (WK-2) must count the handler, not the producer.

## Oracle review CORE record and CORE-3 (2026-11-06, Opus; replaces the CORE-3 G_PM_ND reply and the opt-in steps of the oracle plan CORE-2)
(1) Record word, 64d344659: confirmed. The N64 word is self-contained: opcode plus fields, and a pointer that is read when the op runs. Re-encoding it buys nothing, so the word is the record, and decoding its fields in the setter is the record reader. My G_PM_ND layout was the wrong tool for this.
What follows from it (decided):
- At 2da2a6947, nd_rec_leaf is a memcpy of the leaf words, and the bridge leaves (nd_gbi.h) no longer decode. Both force-includes are now layers that change nothing. Delete nd_rec.h, nd_rec_leaf.h, nd_gbi.h, nd_gbi_leaf.h, the gen_nd_gbi.py record and bridge output, and the CMake lines 338-339, in CORE-3 (rule 7: no dead layers). Keep nd_rec_test's "replay state equals word state" check, rewritten against the dispatcher.
- So "opt a file in" no longer means anything. Producer work in UI, FXW and WBO shrinks to:
  1. replace the custom word hacks (G_PM_TEXI, G_PM_RM, the gPmSplashMark NOOP) with handled ops or native-only records;
  2. the WEND draw paths;
  3. side-buffer lists compiled through nd_dlc.
  Per-file force-includes are dropped from plan_s10_path.md.
- S10 now means: the dispatcher (opcode to handler table) and every handler live in nd_state.c, nd_rect.c, nd_geom.c, nd_mtx.c, nd_dlc.c and nd_core.c; pm_gbi.c is gone. There is one decoder for both walked lists and nd_dlc compile, never a second copy.
- Rule for G_PM_ND: only for native-only payloads that have no N64 encoding (glyph by sheet and index, splash on/off, target role, the TEXI/RM replacements). It never wraps N64 words.
(2) CORE-3: replaced. Texrect and fillrect keep their words. G_TEXRECT is the fixed F3DEX2 three-slot form (TEXRECT, then RDPHALF_1, then RDPHALF_2), and gbi_texrect already reads the two halves. G_FILLRECT is one slot, so wrapping it in G_PM_ND would also break rule (1).
CORE-3 corrected:
- Move gbi_texrect and gbi_fillrect (plus FILL_SX10 and the HUDB1 inclusive-corner rule) to nd_rect.c as the handlers of G_TEXRECT and G_FILLRECT. They read nd_ws and call pm_gpu_tex_rect / pm_gpu_fill_rect.
- The pm_gbi.c pc loop and the g1/g2 tables call them through the dispatcher entry.
- No G_PM_ND, no opt-in. Delete the bridge and record layers as above.
- The census counts ops by handler module, not by "word vs record".
- Gate: for camera_render and the 2 background lists, ops served by pm_gbi.c handlers = 0 (all in module nd_rect); mac_00 sky and mac_01 byte-equal against the CORE-2c build; pause sky by eye.
S10 precondition (census over the full catalog plus a grep). All must hold:
1. Per-opcode census of every walked list (main, bg, key replay, ND_CALL side lists): every opcode seen has its handler outside pm_gbi.c, and the "legacy handler" counter is 0.
2. G_PM_TEXI, G_PM_RM and the splash NOOP mark: 0 hits, and grep `G_PM_TEXI|gSPPmRenderMode|SPR_RM_LIST|gsSPPmTexInline|gPmSplashMark` over src and 3ds is empty.
3. wd = 0 (WEND).
4. No pm_gbi.c function has a caller outside pm_gbi.c (nm or grep of every exported pm_gbi_* symbol). The structured records G_PM_REC, G_PM_SPR and G_PM_ENT are allowed; their handlers move to nd_model.c, nd_sprite.c and nd_dlc.c in CORE-8.
5. Opcodes with 0 hits and no macro caller are deleted, not moved (WEND END-1).
