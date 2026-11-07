# Plan S10: pm_gbi.c deleted (oracle re-plan 2026-11-06, state review 2026-11-07)

User decision 2026-11-06: commit to S10. Model (oracle review 5b2ee3c48): the N64 word is the record, one dispatcher, no opt-in, G_PM_ND only for native-only payloads.

## State (nds1 after the RS3 merge fa6406ee7)
- Done: WK-1..4, ST-1..4 (ST-3 was a move: the legacy combine is live), TX-1..4, GM-1..4, LS-1..4, CORE-3, RECT-3, TRI-4, FB-4, T1F. Producer-hack grep (G_PM_TEXI, gsSPPmTexInline, gSPPmRenderMode, SPR_RM_LIST, gPmSplashMark, G_PM_RM) is empty. G_PM_SPR does not write list memory (nd_walk.c). Walker draws wd 0, tridrop 0 (TDC).
- pm_gbi.c holds only global definitions (vertex slots, gv_fb, gp1_*, census counters, comb_*, dual_*, box_*, lt/lk, ls_*, ...) plus fold_axis (live: nd_rect.c, nd_geom.c) and pm_gbi_bind_tile1 (live: nd_model.c, nd_recs.c, pm_gpu.c).
- pm_gbi.h: every function it declares is defined in nd_walk.c, nd_cen.c or nd_mtx_set.c. pm_gbi_run (nd_walk.c) is the phase wrapper and stays.

## Rows (order)
1. GLOB + END-1 (one agent, row_GLOB.md; both touch nd_cen.c): globals of pm_gbi.c to their writing module (D10); fold_axis to nd_rect.c, pm_gbi_bind_tile1 to nd_tex_set.c. END-1: delete walker case labels with no macro producer and 0 hits in a whole-run ophit count.
2. S10-2 BRIDGE (parallel with 1, row_S10B.md): gbi_task calls pm_gbi_run directly (eye 2 under stereo); nd_core.c record/emit/order/eye_copy code and ND_LEGACY/NdRec deleted; nd_frame_begin, nd_st_log and the pm_pf_nd_add timing stay.
3. S10-1 DELETE (after GLOB): pm_gbi.c and pm_gbi.h deleted; the 6 prototypes move into nd.h; names pm_gbi_* stay (no rename).
4. S10-3 HOT (after S10-1 and S10-2, Sonnet): new .text hot list in 3ds/ld/3dsx.ld.in from the sampled top ops (nd_walk_run, nd_geom_vtx, draw_tri, gv_tri, gv_epoch_enter, gtrip/gappend, nd_native_submit, nd_raw_native, flush_draw, tex_op, ensure_tile, load_mtx, sync_combine, dlc_try, pm_tex_bind); cold nd files -Os. Gate: byte gates, nd_size_test.sh, gbi-phase us at wcen_mac01 no worse; one console prof push with user consent.
5. S10-4 FULL: epic merge run (full catalog once, console prof push). Ask the user first (CLAUDE.md rule 5). Orchestrator only.
Byte gates for every row (cold det): spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015, pause_spin_grab_det_w300. Not byte gates: spot_mac00_*, super_text_pic, nok11 shot 0016 (layout or run dependent, row_UB1.md).
Out of scope: an "always plan" combine that removes the legacy modes (changes pixels where d->use is 0; needs its own design row with an el/ep census).

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

## Decisions
- D1 The word is the record. No re-encoding, no force-include, no ND_* record structs.
- D2 One decoder: the nd_ops.h inline field decoders, used by the walk, nd_dlc and nd_words.
- D3 G_PM_ND carries only SPLASH (and TEXBIND under D6).
- D4 G_PM_REC, G_PM_SPR and G_PM_ENT stay as words; only their handlers move.
- D5 Module map as in the table. New files: nd_walk.c, nd_set.c (ST-1, 3DS only: the setters that call pm_gpu_*), nd_comb.c, nd_geom.c, nd_target.c, nd_shape.c, nd_ent.c, nd_recs.c, nd_cen.c, nd_int.h. Haiku adds no other file.
- D6 G_PM_TEXI goes back to plain words. TEXBIND (G_PM_ND, slot key) is added only if a TX-4 row measures the gbi-phase us above +3%.
- D7 G_PM_RM goes back to G_DL plus the state-list memo in the G_DL handler.
- D8 No handler writes list memory (the replay rule).
- D9 A debug switch survives a move only if an open row file names it. Otherwise it is deleted in that move. The census (K) stays, cold.
- D10 Ownership of globals: a global is defined in the module that sets it from the display-list words or by its own decision (else in its main hot reader). Other modules may write it only to save or restore the walker state, to reset it at a list, window or frame start, to replay the walk (dlc and entity groups), or to bump a generation counter; they use the extern in nd_int.h. A shared global moves into NdWs only in a later row.
- D11 hud_element.c and the status bar files are touched only after the HUD1 worktree commits, and as the last TX-4 row.
- D12 Each move is a pure cut and paste plus prototypes, with no logic change in the same row (byte-equal proves it). A fix found during a move becomes its own row.
- D13 Two tries per named cause, then hand back with the measurement.

## Risks
- Hidden coupling through globals: GLOB checks that every extra writer of a global is a save/restore, reset, replay or generation write (D10).
- Inlining lost across files raises walk cost: each row reports gbi-phase us at wcen_mac01 (+3% rule); S10-3 restores locality.
- Azahar is not the console: one console push at S10-4.
