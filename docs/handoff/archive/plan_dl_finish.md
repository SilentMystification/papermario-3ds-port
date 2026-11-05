# plan_dl_finish: finish the display-list refactor, then CACHE-FIT

Plan only (Opus). Rules: CLAUDE.md 2, 5, 7 and docs/handoff/hw_guidelines.md. Target O3DS 30 fps (33.4 ms). No culling. No lower effect rates.
Tags: [M] measured (source named), [C] read from code or from build3ds/pm_3ds.map / objdump of build3ds/pm_3ds.elf, [I] inference.
Line anchors: tree acdace10b (main b4f9e92ff + SUN5). pm_gbi.c lines after 3160 are +5 against main. Byte counts: pm_3ds.map and `arm-none-eabi-objdump -d -l` of the 00:27 elf (docker image devkitpro/devkitarm), bytes per source line summed per function.

## 1. Display-list rows: done, dropped, open

| Row | What | State | Commit / evidence |
|---|---|---|---|
| W1, P1 | op-class timer, producer census | done | 7bd0205f1, a1a3ced7d |
| C2 | model render-mode lists as G_PM_RM | done | a6c525673 |
| G1 | sync/no-op skip before the prologue | done | 9268569ae |
| H1 | rare gbi_run_body paths to cold helpers | done | 3449398b0 |
| C1-0 | tex/combine cases in noinline tex_op | done | 8d157f28d |
| BG | sky as one bind | done | 78b4d2db1 |
| G2 | equal-state skip | done | 07aebffa5, merge 76fe66592 |
| H3 | gv_matrix_id hash index | done | b831aa956 |
| H2 part 1 | flush_draw boxlog cold | done | e1f8847ba |
| W2 | 1-in-8 phase timers | done | 8ee2d859f, merge 29016a9e9 |
| H5/H4 | texrect reciprocal, PLD of G_DL target and vertex block | done | 413deb696 |
| C1-1 | G_PM_TEX bind op (model textures) | done | e3b6268d7, merge f93c91312 |
| GLYPH | G_PM_TEXI glyph loads | done | decf43341 |
| R0 text, R2a | glyphbind log gate, comb audit gate | done | 363a0e8e1, baa1cb683 |
| MODELOPS R0, DS0, DS1 | mops counters, dcsum, svc cache flush | done | 25d191c70, cbab0e929, cd248cbc8 |
| CROSS2 | second kick split | done | ec9a7dc13 |
| T0, T1, T2b, T1B | census by task, tail rows | done | 41662a62a, eff0a5c43, 60de38e8e, b7e847d8c |
| EN2, NP1B, HT1 | shadow, sprite, HUD loads as G_PM_TEXI | done | 35b41c5eb, 64be3494b, be50979c1 |
| R5B, R2B | equal light / combine / texture / geom skip | done | b1d5d425e, f230e46c6 |
| LB1 | lazy FrameBegin | done | c294e993f |
| PAUSE4A, PAUSE5A | pause background as one load | done | 9461b95a8, a02d5ac09 |
| MR0, MS0, DC6 (1,2), SPK1 | censuses, dcsum fields | done | 38c25809b, fb9e8f0fc, 2d41644a1, 542f2d72d |
| M1 then D5, D6 | general state-list replay | DROPPED: M1 gate failed (hit ops 501/frame, 58.7 %, 456 B per call) | 8fa77b440, census removed 2dfa2c3e1 |
| H1b | tile cases to gbi_tile_op | DROPPED: walk +86 us | agent_dl.md |
| W4 | opf flag-table prologue | DROPPED: -21 us, gate 150 us | agent_dl.md (branch dl2-w4) |
| GA1 | glyph atlas keeps the bind | DROPPED: walk gate missed | 8362b7864 (branch) |
| E4 | divides in glyph path | DROPPED: -100 ns per call | agent_dl.md |
| DS2 (-Os part only), W6 | -Os for pm_gpu.c | DROPPED: +8 % close sum | agent_drawcost.md. The cold-branch moves of DS2/H2 were NOT done: still open (CF3b) |
| DS3, DS4, DS5, CROSS1, DC6 (3), PAUSE4B | pal gens, TexBind record, raw GPUCMD, one linear block, repeated groups, label textures | DROPPED (valid gates) | agent_crossfix.md, agent_draw.md, orchestrator_handoff.md 4.6 |
| MR1, MS1 | G_PM_MODEL, close sort | DROPPED by census gate (107 < 120 us, 15 < 20 closes). MR1 reopens only on console data | agent_model.md |
| MODELOPS R1, R3 | bind skip, equal-matrix skip | DROPPED (bind_w1 7 of 78, mtx_same 0) | b9fbebaf0 |
| E1, E2, E3 | delete the dead pdl paths | OPEN (rows DLF2, DLF3) | plan_dl_replay_v2.md 6 |
| EN1 | entity RM list as G_PM_RM | OPEN (DLF4) | plan_entity_npc.md |
| NP2 | sprite setup list as G_PM_RM | OPEN (DLF5) | plan_entity_npc.md |
| TXR1 (plan_text_batch R1) | G_PM_GLYPHS text run op | OPEN (DLF6) | plan_text_batch.md 2 |
| H2 rest, H6 | flush_draw/tex cold split, hot state struct | OPEN (CF3b, CF5) | agent_dl.md, plan_source_opts.md |
| LB1b, CORE1, FPS20, MR1 | need console fsplit or a user decision | OPEN, last (epic CON) | plan_toad_33.md |

## 2. Facts traced for the open rows
- [C] The pdl paths are dead in play but still in the hot code. pdl_tpl_gdl (pm_dlc.inc.c:1337) calls pdl_try. The first class 2/3 miss sets pdl_compile_one = 1 and pdl_depth = 1 (900-910). pdl_tpl_drive pops G_ENDDL itself (1399-1402), so pdl_compile_finish (875) never runs. Every entity shadow emits a class 3 G_DL (src/entity_model.c:310 Gfx_RM1_DECAL_XLU_NO_AA), so the stuck state comes in the first frame with a shadow [I]. [M diag_dl dl1]: 0 pdl calls after f299. Still paid per op or per G_DL: pm_gbi.c:3426, 3482 (3 stores per state op), 3489-3490, 3536-3547, 1120, 1457-1467, 2720, 2910, 2913; pm_gpu.c:984.
- [C] Inlined pdl code in gbi_run_body: pdl_tpl_drive 604 B, pdl_comp_done 492, pdl_tpl_compile 468, pdl_tpl_replay 240, gbi_pack 188, pdl_tpl_segment 172, pdl_tpl_find 160, others 140: about 2.5 KB. Own functions: pdl_try 1520, pdl_try_m 2020, pdl0_gl_enter 1360, pdl0_gl_leave 528, gbi_state_get 1760, gbi_state_apply 1812, pdl_m_refuse 560, pdl_tpl_gdl 616, pm_pso_bake 1532.
- [C] A THUNDER-BOX diagnostic is still on main: gbidump_on() (pm_gbi.c:103) calls pm_frame_count() (other file, a real call) on every op of every frame, and logs every op of frames 259 and 260 with 11 fields (3461). The console push uses loglevel=debug, so it pays about 2 x 1,645 vsnprintf lines [I from 1,645 ops per frame, orchestrator_handoff 3].
- [C] gv_streak_op and tex_streak_op (pm_gbi.c:47, 61) run on every op with phase off: a 9-way compare, then a call to the flush function.
- [C] Hot code size (bytes) against the 16 KB L1I: gbi_run_body 19,896; flush_draw 13,784; draw_tri 11,112; tex_op 8,800; dlc_try.part.0 7,812; load_impl 16,036; sync_combine + work 2,564; ensure_tile 1,484; C3Di_UpdateContext 1,256. Sum about 83 KB per frame loop.
- [C] Cold code inside those functions: gbi_run_body TEXRECT case 1,468 + inlined box_pattern_rect 1,380 + rect_to_image 580 + shift_mul 180; FILLRECT 648 + rgba_from_5551 76; phase-only hooks inlined (os_op_hook 1,060, dls_target 384, dls_kind 116, os_dlc_hit 84, streak flush/op 948); G_VTX CPU vertex path 496 + vert_color 60. draw_tri: GV head 760 + gv_tri 2,388 are hot, the CPU clip path is 5,296 + helpers. tex_op: suntile_t0 344 (debug), settdbg block. dlc_try.part.0: dlc_compile 1,176, dlc_static_build 828, dlc_verify_skip 588, dlc_batch_compile 348 run only on a miss or the dlccheck switch. load_impl: hit path 3,160, miss path 4,820 plus about 8 KB of inlined converters. flush_draw: off-screen snapshot 900, off-screen scissor 884, decal2 + zmark pass 2 768, zmark pass 1 260, noise 192.
- [C] The linker does not group cold code. 3ds/ld/3dsx.ld.in:31-33 has only `*(.text)` and `*(.text.*)`, so `.text.unlikely.*` sections sit between hot functions (map lines 28149-28208: mdc_report, mops_report in the pm_gbi.o range). The hot set spans pm_gpu.o 0x116e40 to pm_gbi.o 0x148000 plus citro3d at 0x3db130.
- [C] Hot dispatch state is scattered. The prologue and g2_same read about 30 scalars: geom, omode_h, omode_l, comb_w0/w1, comb_set, prim, envc, fogc, blendc, tex_on, tex_ss, tex_ts, sc_ok, sr_ok, gv_epoch, ep_epoch, cur_tile, use_t1, box_folded, cmb_gen_seen, comb_gen, n_ops, dlc_on, dls_on, tri_prep_ok, tri_since_vtx, mv_sp. Map addresses put them on about 21 different 32-byte lines on 9 different 4 KB pages (0x57d4xx .data; 0x124df4c, 0x1250a48, 0x12ab42c, 0x12ad434, 0x12b1560, 0x12b32b8, 0x12ba4xx-5xx, 0x12bb8xx-cxx .bss).
- [M console_walk_split, plan_draw_cost] A texture cache hit costs 16.7 to 17.1 us on the console (64 to 86 per frame, 1.1 to 1.4 ms). [C pm_tex.c:1085-1125] The hit path is: the TLUT FNV, one probe of a 256-entry direct-mapped index (lidx), then 12 recent slots, then a linear scan of all slots up to slot_hi. Slot is about 136 bytes (pm_tex.c:24-51), not line aligned; a key compare touches 2 lines. With about 90 textures per frame in 256 entries, about 30 % of keys share an index entry with another texture of the same frame [I, birthday estimate]. Those pairs evict each other every frame and fall to the scan: up to 512 slots x 2 lines = 32 KB, twice the L1D. This fits the measured average [I]. The scan count is not measured.
- [C] Two full slot scans run every frame: pm_tex_upload_vram_before_frame (pm_tex.c:558, called pm_gpu.c:1799) and pm_tex_begin_frame (pm_tex.c:810-821, called pm_gpu.c:1809). Each reads 1 to 2 lines per slot up to slot_hi. slot_hi is not in any log.
- [C] libctru has svcControlPerformanceCounter with CORE_INST_CACHE_MISS, CORE_DATA_CACHE_READ_MISS, CORE_MAIN_TLB_MISS, CORE_STALL_BY_LACK_OF_INST (svc.h 197-300). Two event counters plus the cycle counter. The kernel may refuse it; Azahar may not implement it [I].
- Azahar has no cache model. H1b and C1-0 show that an out-of-line split costs +86 to +150 us on Azahar (call cost only). For CACHE-FIT rows, the Azahar number is a guard, not the result. The moving quantity is the byte count in pm_3ds.map. The console result comes from the CF0 counters at the epic push.

## 3. Epic order
1. DLF (6 rows): finish the DL refactor in code. Dead paths go first, so CACHE-FIT measures the final code.
2. CFI (5 rows): CACHE-FIT, instruction side.
3. CFD (3 rows): CACHE-FIT, data side.
4. CON (last): rows that wait for console data or a user decision.
Build, gen_map_restore and restore of 3ds/src/map_restore_table.c per CLAUDE.md 9. PM_ROW=<row> in every shell. One scenario per row from test_catalog.md. Commit per row on a pass. Revert the row files on a miss (rule 7).

## 4. Epic DLF: finish the display-list refactor

### DLF1 PRO: remove the gbidump diagnostic, gate the streak calls
- Files: pm_gbi.c:102-103 (gbidump_on), 3461 (the call), 3463-3464 (gv_streak_op, tex_streak_op).
- Change: delete gbidump_on and its call (THUNDER-BOX is closed, b4f9e92ff). Change 3463-3464 to `if (pm_ph_on) { gv_streak_op(op); tex_streak_op(op); }`. The flush calls at list end stay (they test their own flag).
- Mechanism: removes one cross-file call per op (pm_frame_count) and a 9-way compare per op with phase off, and about 3,300 debug log lines in frames 259-260 on the console push.
- Size: about 5 lines. Scenario: mac01t_fsplit. Quantity: count of `gbidump` lines in the run log, about 3,000 to 0. Guard: top shots 2 of 2 byte-equal to HEAD.
- Stop: a changed shot reverts the row.

### DLF2 E1+E2: delete pdl_tpl, pdl_try_m and pdl0
- Files: pm_dlc.inc.c 570-772 (pdl0_*, keep pdl_op_rw only if a later line uses it), 988-1166 (PdlM, pdl_try_m), 1164-1415 (pdl_tpl_*), pdl_report fields 1416+; pm_gbi.c:374-376 prototypes, 3426-3432 (tpl call), 3489 and 3536 (pdl0 calls), 3543-3547 (pdl_try_m), 2462-2465 (pdl0 report). The agent greps every symbol before the delete.
- Mechanism: [C] dead after the stuck point (section 2). Removes about 2.5 KB from gbi_run_body and per-G_DL tests. Also removes the risk that a scene with no class 2/3 G_DL runs pdl_tpl_drive, which walks the main list one op per pm_gbi_run call (pm_dlc.inc.c:1409).
- Size: about -700 lines, +0. Scenario: mac01t. Quantity: gbi_run_body bytes in pm_3ds.map, 19,896 to about 17,400 or less. Guards: census tot ops per frame within 1 %; top shots byte-equal.
- Stop: ops change above 1 % or a changed shot: revert, then a read-only diagnosis. No second try.

### DLF3 E3: delete pdl_try, the PSO replay and GbiState
- Files: pm_dlc.inc.c 774-987 (pdl_nops, pdl_refuse_ops, pdl_find, pdl_heap_gen, pdl_pso_capture, pdl_pso_use, pdl_lazy_tile, pdl_key, pdl_compile_*, pdl_comp_*, pdl_try) and 543-569 (gbi_pack, gbi_fld) if no user is left; pm_gbi.c:371-378, 1120 (pdl_ep_pso branch), 1457-1467 (pdl branches of gv_epoch_enter), 1691-1760 (gbi_state_get/fnv/hash/apply), 2720 (pdl_lazy_tile), 2910 and 2913 (`!pdl_ep_pso`), 3482 (keep only gv_epoch++), 3490 (pdl_comp_done), 3537-3542; pm_gpu.c:801 (pm_pso_bake), 984 and the pso_pkt_try / pso_sh_restore use; include/pm_pdl.h; pm_tex.c:826-841 and 861 (pm_pdl_* gens, only pdl_heap_gen reads them). Keep dlc (class 1) and pm_dlc_gen.
- Mechanism: pdl_pso_cur is set only by a pdl_try hit [C grep]. After DLF2 nothing sets it, so every consumer is a constant branch. The prologue writes 1 store per state op, not 4. flush_draw loses pso_pkt_try 232 B and pso_sh_restore 236 B.
- Size: about -450 lines. Scenario: mac01t. Quantity: sum of gbi_run_body + flush_draw bytes, down by 700 or more, and gbi_state_get/apply, pdl_try, pm_pso_bake absent from the map. Guards: ops within 1 %, shots byte-equal.
- Stop: as DLF2.

### DLF4 EN1: entity render-mode list as G_PM_RM
- Files: src/entity_model.c:281-365 (the RM switch in appendGfx_entity_model), TARGET_3DS guard.
- Change: save gMainGfxPos before the switch. After it, if exactly one word was written and it is G_DL, set its w0 to `G_PM_RM << 24`. rm_apply (pm_gbi.c:2944) accepts the Gfx_RM1/RM3 lists as they are (OML, OMH, GEOMETRYMODE x2, ENDDL; plan_entity_npc 1). A list with another op falls back to G_DL inside the handler (3513-3518).
- Mechanism: one op in place of a G_DL push, 4 dispatched ops and an ENDDL pop per entity draw (about 30 shadows per frame in the tour view [M diag_toadt]).
- Size: about 8 lines. Scenario: mac01t. Quantity: census ops per draw of appendGfx_entity_model (T2b row), 17.3 to 14.3 or less; rmsplit fb 0. Guard: shots byte-equal.
- Stop: no ops drop or a changed shot: revert.

### DLF5 NP2: sprite setup list as G_PM_RM
- Files: src/sprite.c:376-385 (the four gSPDisplayList of Opaque/Translucent(Shaded)SpriteGfx), pm_gbi.c:2944-2975 (rm_apply).
- Change: rm_apply also accepts G_SETCOMBINE and G_TEXTURE and runs them through tex_op(a0, a1) in list order (the same handler the dispatch uses, 3731-3737). sprite.c emits gSPPmRenderMode under TARGET_3DS.
- Mechanism: 5 dispatched ops plus a push/pop per sprite to 1 op [plan_entity_npc NP2]. The handler order stays.
- Size: about 15 lines. Scenario: mac01t. Quantity: census ops per draw of spr_appendGfx_component, 11.1 to 7 or less. Guard: shots byte-equal.
- Stop: as DLF4.

### DLF6 TXR1: G_PM_GLYPHS text run op
- Files and design: plan_text_batch.md section 2 (msg_draw.c:1754-1802, pm_gbi.c G_PM_TEXI handler 3504-3507, tb_inline 2885).
- Mechanism: [M agent_dl R0] glyph path 0b + e4 + f0 = 5.8 ms per frame on Azahar pause. A run op removes the per-glyph header, 7 load ops and the texrect dispatch. [M R1 probe] it does not remove the 76 tile 0 closes (texture changes between glyphs), so the gain is dispatch and setup only.
- Size: about 70 lines. Scenario: pause_menu. Quantity: draw_msg ops per frame 1,092 to 750 or less. Guard: pause shots 0000-0002 byte-equal (coin at x 90-101, y 192-202 is known noise).
- Stop: plan_text_batch stop rule. Walk not down: revert, no second try.

## 5. Epic CFI: CACHE-FIT, instruction side
Each row moves code only. No logic change. Gate per row: the map byte quantity moves; mac01t_fsplit top shots byte-equal; Azahar gbi ms at f900 not above HEAD + 2 % (the call cost shows on Azahar, the cache gain does not).

### CF0 PMU: console cache counters
- Files: pm_main.c or pm_phase.c beside the fsplit line (switch `pmu`).
- Change: at boot, PERFCOUNTEROP_ENABLE; window A sets PMN0 = CORE_INST_CACHE_MISS, PMN1 = CORE_DATA_CACHE_READ_MISS; the next 300 frames window B sets CORE_MAIN_TLB_MISS and CORE_STALL_BY_LACK_OF_INST. Reset at the first gbi task, read at pm_gpu_end. One line per 300 frames: `pmu f%d ev %u %u per frame %u %u cyc %u rc %08x`. Two svc calls per frame, only with the switch.
- Mechanism: gives the console miss counts that Azahar cannot give. Archive the CF0 build as the console baseline (push_3ds.sh --archive-only).
- Size: about 35 lines. Scenario: yard_fsplit (-Try). Quantity: the pmu line is present with its rc. Azahar may print rc nonzero; that is a pass for the line, the values come from the console.
- Stop: if the console rc is nonzero at the epic push, delete the row code in the next epic.

### CF1 gbi_run_body: rare cases and phase hooks out of line
- Files: pm_gbi.c 3782-3816 (FILLRECT), 3817-3899 (TEXRECT with box_pattern_rect, rect_to_image, shift_mul), 3765-3775 (SETCIMG), 3706-3717 (viewport part of MOVEMEM), 3646-3660 (CPU vertex path), and the phase-only hooks 3468, 3484-3486, 3502, 3527-3529, 3554, 3573.
- Change: TEXRECT and FILLRECT to `static __attribute__((noinline)) Gfx* gbi_rect_op(...)` (not cold: pause and HUD use them). SETCIMG, viewport and the CPU vertex path to noinline helpers. All phase hooks into one `__attribute__((cold, noinline)) gbi_ph_op(pc, sp, op, w0, w1)` called under one `if (pm_ph_on)`.
- Mechanism: about 7.6 KB of rarely run code leaves the loop. The hot loop (prologue, g2_same, G_DL, G_VTX GV part, ENDDL, MTX, state cases) becomes about 9 KB and packs into fewer lines.
- Size: about 60 lines moved. Scenario: mac01t_fsplit. Quantity: gbi_run_body bytes, about 17 KB (after DLF) to 10 KB or less.
- Stop: rule above. A miss of the Azahar guard reverts only the helper that a hot case calls.

### CF2 draw_tri: CPU clip path out of line
- Files: pm_gbi.c:1816-2014 (after the gv_tri return at 1812), 716 (suntile_tri), 1803-1810 (po3a block).
- Change: the CPU path to `static __attribute__((noinline)) void draw_tri_cpu(int* idx)`. suntile_tri and the po3a block to cold noinline.
- Mechanism: the GV path (head 760 + gv_tri 2,388) runs for ring vertices; the 5.3 KB CPU clip path runs only for decals, zmark, wide UI and cross loads.
- Size: about 15 lines. Scenario: mac01t_fsplit. Quantity: draw_tri bytes 11,112 to about 3,500.
- Stop: rule above.

### CF3a tex_op and dlc_try: debug and miss paths out of line
- Files: pm_gbi.c:701 (suntile_t0) cold noinline; 3031-3050 (settdbg block) to a cold helper; 3127-3130 (empty imglog block) delete. pm_dlc.inc.c:54 (dlc_batch_compile), 88 (dlc_compile), 162 (dlc_static_build), 351 (dlc_verify_skip), 433 (dlc_reset): `__attribute__((cold, noinline))`.
- Mechanism: about 0.6 KB debug code in tex_op and about 2.9 KB of compile code in dlc_try leave the hot functions. dlc_try runs 56 to 96 times per frame [M plan_toad_33, plan_model_ops].
- Size: about 12 lines. Scenario: mac01t_fsplit. Quantity: tex_op + dlc_try.part.0 bytes, 16,612 to about 13,000.
- Stop: rule above.

### CF3b load_impl and flush_draw: miss and off-screen paths out of line (H2 rest)
- Files: pm_tex.c:1126-1408 (load_inner after the scan, from `ph_miss = 1`) to `static __attribute__((noinline)) int load_miss(...)`. pm_gpu.c flush_draw 866-874 (off snapshot), 975-981 (noise), 1006-1017 (zmark pass 1), 1027-1051 (off-screen scissor), 1107-1119 and 1127-1138 (decal2, zmark pass 2) to cold noinline helpers.
- Mechanism: the hit path (3.2 KB) is split from 12.8 KB of miss and converter code. flush_draw loses about 3 KB of rare state paths; the pal block (gv4 1,624 or gv5 2,700) stays.
- Size: about 50 lines moved. Scenario: mac01t_fsplit. Quantity: load_impl + flush_draw bytes, 29,820 to about 21,000.
- Stop: rule above. DS2 failed on -Os, not on moves; do not add -Os.

### CF4 LINK: one hot text section, cold code grouped
- Files: 3ds/ld/3dsx.ld.in:31-33; include/pm_port.h (one macro `PM_HOT` = `__attribute__((section(".text.pmhot")))`); tags on the hot functions.
- Change: in the linker script, before `*(.text)`: `*(.text.pmhot .text.pmhot.*)`, then the citro3d input sections `*libcitro3d.a:*(.text.C3Di_UpdateContext .text.C3D_DrawElements .text.C3D_DrawArrays .text.C3D_TexBind)` (names from the map), then `*(.text.unlikely .text.unlikely.*)`. Tag: pm_gbi_run, gbi_run_body, draw_tri, tex_op, tb_run, tb_inline, ensure_tile, tile_bound, dual_bind, gv_epoch_enter, gv_state_why, gv_params_calc, gv_matrix_id, sync_combine, sync_combine_work, sc_sig_now, sync_render, dlc_try, pm_gpu_grange, pm_gpu_gvtx_load, pm_gpu_flush_at, flush_draw, pm_tex_load, load_impl, pm_tex_bind, pm_tex_bind_slot, pm_tex_set_wrap.
- Mechanism: the hot set today spans about 200 KB of pm_*.o text plus citro3d at 0x3db130. Contiguous hot code removes set conflicts between hot functions and needs fewer instruction TLB entries [I]. Cold sections stop sharing lines with hot ones.
- Size: about 35 lines. Scenario: mac01t_fsplit. Quantity: span (highest end minus lowest start) of the tagged set in pm_3ds.map, from about 2.9 MB to its byte sum (about 60 KB after CF1-CF3).
- Stop: rule above. A link failure or a changed shot reverts the script edit.

## 6. Epic CFD: CACHE-FIT, data side

### CF5 HOT: dispatch state in one block (H6)
- Files: pm_gbi.c declarations of the scalars in section 2 (lines 78-90, 265-373, 1079, 1396 area; the agent greps each). include/pm_port.h macro `PM_HOTD` = `__attribute__((section(".data.pmhot")))`; the first one also aligned(32). Line 3617: write vert_mid only under po3a_on().
- Mechanism: about 21 lines on 9 pages to 4 or 5 lines on 1 page per op. One section per object keeps them in order [C -fdata-sections].
- Size: about 35 lines. Scenario: mac01t_fsplit. Quantity: count of distinct 32-byte lines of the listed symbols in pm_3ds.map, about 21 to 5 or less.
- Stop: rule of section 5.

### CF6 TEXLK: texture index without the slot scan
- Files: pm_tex.c:963-965 (lidx), 1085-1125 (hit path), 1404 (insert at load), drop/stale sites that free a slot.
- Change, step 1 in the same row: a cold counter of full scans per frame (in the existing thsplit / texstat line). Run HEAD logic once. If scans are below 1 per frame, stop: the fix pays nothing (valid gate). Step 2: lidx becomes 256 sets x 4 ways (short[256][4]); probe 4 entries; on a scan hit or a load, insert into the oldest way. The full key compare stays, so a stale entry only misses.
- Mechanism: section 2 (collision pairs fall to a 32 KB scan).
- Size: about 30 lines. Scenario: mac01t. Quantity: full scans per frame, to under 1. Direction: thsplit hit us per call on Azahar.
- Stop: step 1 gate; then no drop of scans: revert.

### CF7 SCAN: no full slot walk per frame
- Files: pm_tex.c:550-559 (pm_tex_vram_pending, pm_tex_upload_vram_before_frame), 805-821 (pm_tex_begin_frame loop), every site that sets vram_up, stale or vram_stage (grep).
- Change: a pending index list (u16 pend_slot[SLOTS], count). The setter appends the slot once (a per-slot flag). The two walks visit only the list, then clear it. Step 1 in the same row: print slot_hi per 300 frames (texstat). If slot_hi is below 32, stop (2 KB per walk).
- Mechanism: two walks of slot_hi x 136-byte slots per frame read 1 to 2 lines per slot and evict the L1D before the gbi run.
- Size: about 35 lines. Scenario: mac01t. Quantity: slots visited per frame by the two walks, from 2 x slot_hi to the pending count.
- Stop: step 1 gate; a changed shot or a texture fault (texfail line) reverts.

## 7. Epic CON: needs console data or a user decision (last)
| Row | Waits on | Decision |
|---|---|---|
| CFV | the pmu line from the CFI/CFD push (CF0 build is the baseline) | I-miss per frame down: keep the epics. No change: next cache rows need a new plan, not a retry |
| MR1 | fsplit gbi share of the frame | run plan_toad_33 MR1 only if gbi is 55 % or more of the frame |
| LB1b | fsplit gwait after LB1 | only if gwait is above 1 ms |
| BATTLE | wm/tk fields after a battle (forest 51.7 ms) | a fix row follows the data |
| CORE1 | frame time after all epics | only if above 36 ms; high risk |
| FPS20 | user decision | stable 20 fps cap |

## 8. Excluded (dropped with a valid gate)
D5, D6 (M1), H1b, W4, GA1, E4, DS2 -Os and W6, DS3, DS4, DS5, CROSS1, DC6 (3), PAUSE4B, MS1, MODELOPS R1 and R3. Culling and lower effect rates are forbidden.
