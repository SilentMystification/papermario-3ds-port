# plan_native_slices: slice table and code-agent rows for the native draw system

Design: docs/handoff/plan_native_system.md (PNS). Section numbers below refer to PNS. Tags as in PNS: [C file:line] read in source at nds1 HEAD c4740fdce, [U] unverified.
S0 line anchors are at nds1 HEAD c4740fdce. S1 to S11 anchors are symbols (function or macro name plus a grep string), checked with git grep at nds1 HEAD 115a06200. Line numbers are hints only. The anchor form and the stop rule are in section 2, "Anchor form", and PNS 12 (anchor rule). A line shift never stops a row.

Row format: id | full name | files | numbered edit items (each 30 changed lines or less) | host test (written first, fails first) | gate | scenarios.
Scenario names: (E) exists in 3ds/tools/azt_scenarios.json [C]. (N) new, added in the named row with its catalog line (cap 45 s or less, until-line printed by the build).
Every row: PM_ROW=<row> in the shell, justification line in 3ds/logs/perf/status_<row>.txt before each Azahar start, commit of the agent's files only (never 3ds/src/map_restore_table.c), no attribution.

## 1. Slice table

| Slice | Full name | Rows | Owner files (only this slice edits them while open) | Deleted | Gate | Expected ms (phase on, Toad Town) |
|---|---|---|---|---|---|---|
| S0 | NDTEST test harness and measurement | S0-1 to S0-7 | 3ds/src/pm_gpu.c (cold hook only), 3ds/src/pm_phase.c, 3ds/tools/nd_*, 3ds/tests/ndref/, azt_scenarios.json, test_catalog.md | nothing | tools run. Host tests compile and fail for the missing nd code. ndref committed | 0 |
| S1 | NDCORE native core and bridge | S1-1 to S1-6 | 3ds/include/nd.h, 3ds/src/nd_core.c, nd_tmpl.c, nd_tex.c, nd_mtx.c, 3ds/src/pm_nusys.c, 3ds/src/pm_native.c, 3ds/include/pm_native.h, 3ds/src/pm_gbi.c, 3ds/src/pm_dlc.inc.c, 3ds/src/pm_gpu.c (capture and replay parts), 3ds/src/pm_tex.c (pm_tex_rec_use), 3ds/tools/nat_host_test.c, 3ds/CMakeLists.txt | learn-and-replay, record capture, nat_tmpl_strip, PMOD bake | G1, G2 (all LEGACY), G5 | 0 |
| S2 | NDFRAME frame state and background | S2-1 to S2-3 | 3ds/include/nd_gbi.h, 3ds/CMakeLists.txt, src/main_loop.c, src/cam_main.c, src/background_gfx.c, src/background.c (force-include only) | render_frame frame-start words | G1 to G4, G10 | 0 to 0.3 [U] |
| S3 | NDMODEL map models | S3-1 to S3-5 | src/model.c, 3ds/src/pm_map.c, 3ds/src/nd_dlc.c, 3ds/src/pm_dlc.inc.c, 3ds/src/pm_gbi.c, include/gbi_custom.h, nd_core.c | G_PM_MODEL, G_PM_TEX, mg_run, pm_dlc.inc.c (G_PM_RM stays until S10: sprite.c and entity_model.c write it) | G1 to G10 | 7.9 to 12.7 (ceiling 20.3) |
| S4 | NDSPRITE sprites, NPCs, player | S4-1 to S4-3 | src/sprite.c, src/imgfx.c, src/sprite_shading.c, src/npc.c, src/77480.c | G_PM_TEXI in sprite.c | G1 to G10 | part of 3.2 to 5.4 |
| S5 | NDHUD HUD, text, boxes, windows | S5-1 to S5-4 | src/hud_element.c, src/msg_draw.c, src/msg.c, src/draw_box.c, src/windows.c, src/draw_img_util.c, src/status_icons.c, src/status_star_shimmer.c, src/inventory.c | G_PM_TEXI in hud_element.c and msg_draw.c | G1 to G10 | rest of 3.2 to 5.4 [U] |
| S6 | NDENT entities, items, animators | S6-1 to S6-4 | src/entity_model.c, src/entity.c, src/entity/*.c, src/item_entity.c, src/animator.c | G_PM_TEXI in Shadow.c, G_PM_RM in entity_model.c | G1 to G10 | 1 to 3 [U] |
| S7 | NDWORLD world areas and battle | S7-1 to S7-3 | src/world/**, src/battle/** | world and battle words | G1 to G10 | [U] |
| S8 | NDFX effects | S8-1 to S8-6 | src/effects/*.c, src/effects.c, src/effect_utils.c | PM_SPLASH words | G1 to G10 | 1 to 4 [U] |
| S9 | NDSCREEN overlays, menus, title, targets | S9-1 to S9-5 | S9 files of PNS table 5.1 | walker target roles | G1 to G10 | small [U] |
| S10 | NDDEL delete the walker | S10-1 to S10-4 | 3ds/src/pm_gbi.c, pm_gpu.c, pm_nusys.c, include/gbi_custom.h, 3ds/include/nd_gbi.h, 3ds/ld/3dsx.ld.in, 3ds/CMakeLists.txt | the walker, G_PM_RM, G_PM_TEXI | full catalog once, console prof | rest of the walk, eye-2 walk |
| S11 | NDTUNE by reopen rule only | S11-1 to S11-3 | nd_core.c, pm_gpu.c, pm_main.c | nothing | console prof | [U] |

## 2. Rows

### S0 NDTEST

**S0-1 | NDREF1 reference capture hook in the old walker**
Files: 3ds/src/pm_gpu.c, 3ds/src/pm_gbi.c.
1. pm_gpu.c, after cen_add at flush_draw (line 1100): `if (ndref_on) ndref_draw(...)`, one cold noinline call with the TevKey, z, decal, blend, atest, cull, both slot ids. 3 lines.
2. pm_gpu.c, new cold function ndref_draw near cen_frame_end (line 522): write the draw header, both texture word blocks (pm_tex_slot_words for unit 0 and unit 1, pm_tex_cur_slots pm_tex.c:712), the uniforms (g_uvso, g_cmul, g_cadd, g_zmix, g_fx), pcomp (16 floats). 30 lines.
3. pm_gpu.c, same function: the resolved triangle list (for graw: gidx[gi_start..+gni] mapped to the 16-byte ring Vtx values. Else the CPU Vert values). 25 lines.
4. pm_gbi.c pm_gbi_prod_mark (1943) and pm_gbi_mdl_mark (2045): when ndref_on, write a tag record (producer id). 6 lines.
5. pm_gbi.c, switch parse: `ndref=<frame>` sets ndref_on for that frame, writes sdmc:/3ds/PaperMario/ndref.bin, logs `ndref f<frame> done draws <n>`. 15 lines.
Host test: none (capture tool). Gate: Azahar ndref_toad writes a file and the until-line. Scenarios: ndref_toad (N, base census_mac01 settings, cap 25, until `ndref f200 done`).

**S0-2 | NDREF2 reference files, format, dumper**
Files: 3ds/tests/ndref/FORMAT.md, 3ds/tools/nd_ref_dump.py, azt_scenarios.json, docs/handoff/test_catalog.md, 3ds/tests/ndref/*.bin.
1. FORMAT.md: header, draw, tag and model-input records, field offsets, pinned commit. 30 lines.
2. nd_ref_dump.py: read, print one line per draw, `--check` validates sizes and counts. 30 lines.
3. Scenarios (N): ndref_yard (base census_yard), ndref_pause (base pause_menu), ndref_battle (base battle_start), ndref_text (base text_menu), ndref_title (base golden_multi title shot), ndref_splash (base nok11_splash), ndref_stereo (base yard_stereo_l). Each cap 45 or less, until `ndref f<n> done`. Catalog lines in the same commit.
4. Run each once. Copy the files to 3ds/tests/ndref/. Commit.
Host test: nd_ref_dump.py --check on all files. Gate: every file passes --check. Draw counts match the census line of the same run.

**S0-3 | NDHOST host harness, decoder, compare, N64-same check**
Files: 3ds/tools/nd_host_test.sh, nd_host_stubs.c, nd_pica_decode.c, nd_stream_cmp.py, nd_n64_same.sh.
1. nd_pica_decode.c: parse PICA command words (header format GPUCMD_HEADER, inv_citro3d_format.md) into draws: TEV regs, depth, blend, alpha, cull, unit 0 and 1 texture words, uniforms, index address and count. Resolve indices against the host vertex pools. 30 lines per item, 2 items.
2. nd_host_stubs.c: stubs for citro3d, GPUCMD_AddRawCommands (append to a host array), linearAlloc (malloc aligned 32), svc ticks, pm_tex upload (returns slot words from a table). 30 lines.
3. nd_stream_cmp.py: the PNS 9.3 equality rule. Output: first differing draw and field. 30 lines.
4. nd_host_test.sh: build with the compiler of Q2 (32-bit), run all nd tests, exit 1 on a fail. nd_n64_same.sh: `gcc -E -UTARGET_3DS` before and after on a named file, diff. 20 lines.
Host test: the harness itself compiles and reports "nd_core missing" as a fail. Gate: G1 runs.

**S0-4 | NDSIZE hot loop size test**
Files: 3ds/tools/nd_size_test.sh.
1. docker pm3ds-build:1, arm-none-eabi-gcc with the flags of 3ds/CMakeLists.txt, compile nd_core.c to an object. 10 lines.
2. nm -S: sum nd_emit and its static callees. objdump -d: count instructions of the DRAW_IDX path between two labels. Fail above 8192 bytes or 120 instructions. 20 lines.
Host test: runs on an empty nd_emit stub file and reports the size. Gate: script exit 0 on the stub.

**S0-5 | NDSEAM seam host test**
Files: 3ds/tools/nd_seam_test.c, nd_host_test.sh.
1. Read assets/us/mapfs/geom/mac_01_shape.bin and mac_02_shape.bin (big-endian, SHAPE_BASE 0x80210000, pm_map.c:17 [M DPM 1]). List shared-edge vertex pairs between models (equal s16 positions). 30 lines.
2. With the ndref model matrices: fp24 dp4 (ndt_host_test.c:159-205 method) on both vertices of each pair, through the old matrix path and through nd_mtx. Fail if any pair differs. 30 lines.
Host test: fails until nd_mtx exists (S1-4). Gate: compiles.

**S0-6 | NDPROF prof fields and GPU busy probe**
Files: 3ds/src/pm_phase.c, 3ds/src/pm_gpu.c.
1. pm_phase.c pf_report (314) and the prof line (334): add `nd %.2f lg %.2f dr %u tc %u tw %u tri %u leg %u gb %.2f`. Sums in pf_take (365). 20 lines.
2. pm_gpu.c: in a prof sample frame, tick at the last submit (pm_gpu_end, 2050) and at the P3D event in the next pm_gpu_begin_wait (1982). gb = difference. Counters dr, tc, tw, tri read from pm_gpu today, from nd_core later. 25 lines.
Host test: none. Gate: Azahar mac01t_prof (E) prints the new fields. Then the console push at the S0 end (Q1, Q3).

**S0-7 | NDBASE console baseline (reinstated 2026-11-04)**
Build 2020 (commit 42f6fac7e, build3ds/pm_3ds.3dsx) is the baseline. Status line: BASELINE BUILD READY 2020 42f6fac7e in 3ds/logs/perf/status_NDS_S0.txt. The orchestrator pushes it.

**S0-8 | NDBUDGET frame budget file and the prof budget line**
Files: docs/handoff/frame_budget.md, 3ds/src/pm_phase.c. Done in commit 799307404. The budget line prints the stage targets next to the measured nd, lg and gb.

Changes by the coordinator (2026-11-04): (1) the S0 console push at the end of S0 is dropped and reinstated as S0-7 (baseline). (2) The prof fields (S0-6) and the GPU busy probe stay. (3) S0-8 is the frame budget row. (4) Host tests, code size and instructions per record stay as slice gates.

**S0-7 (original text, superseded):** No code. The orchestrator pushes `phase fsplit` once and `prof` once (Q1). The planner writes the prod top-8 per scene (Toad Town, a battle, pause) into PNS section 7 expected ms for S4 to S9.
No code. The orchestrator pushes `phase fsplit` once and `prof` once (Q1). The planner writes the prod top-8 per scene (Toad Town, a battle, pause) into PNS section 7 expected ms for S4 to S9.

### Anchor form (S1 to S11, re-anchored 2026-11-04 at nds1 HEAD 115a06200)

Each anchor is `file` › `symbol` › grep `unique string` (hint Lnnn). The symbol and the grep string are the anchor. The line number is a hint only. All anchors below were checked with git grep at 115a06200.
- Before the first edit, the agent greps each symbol of the row. If the symbol exists, the agent edits it at its current line and continues, whatever the line shift.
- The agent stops only if the symbol is gone, or the function no longer does what the row says. Then the planner re-anchors the row.
- Delete items: an item deletes whole named symbols. The 30-line cap counts added and changed lines. Deleted lines do not count.
- After a delete, a compiler error or an unused-static warning names a symbol that only deleted code used. The agent deletes it too and lists it in the commit message. A symbol with a caller outside the row's list stays. The agent reports it.

### S1 NDCORE

**S1-1 | NDDROP delete learn-and-replay, the record capture and the PMOD bake**
Files: 3ds/src/pm_dlc.inc.c, 3ds/src/pm_gbi.c, 3ds/src/pm_gpu.c, 3ds/src/pm_native.c, 3ds/include/pm_native.h, 3ds/src/pm_tex.c, 3ds/tools/nat_host_test.c.
Scope: the whole capture and replay path (UC1 to UC3e, RC1, RK1, RK2, N1 to N4, E5-2, E5-3, P3a, PMOD P1, P2, E4b). The DLC2 live static path stays (dlc_try, dlc_compile, dlc_static_build, dlc_static_group, dlc_seg, dlc_box_out, dlc_run_group). S3 moves it into nd_dlc.

PMOD bake decision: DELETE in this row. Reasons:
- nat_seg_add and nat_entry_state read walker state as input: pm_gpu_rec_rmode (z_on, r_decal, r_blend, r_atest), ep_p and ep_ct (gv_params_calc output), pm_tex_cur_slots, and dlc_gr (the pm_gpu_grange merge result). This breaks PNS 0 (no walker output as a design input or a runtime input).
- No consumer draws from it. nk_draw is read only by the NATB log line. nat_pool_put copies the indices, but g->sd stays the draw list. g->pidx is read only by nat_seg_add. nat_model_bake feeds only the PM1 log line.
- It costs 128 KB of linear memory (NAT_POOL_BYTES) and one memcpy per group at each compile.
- S3-1 nd_dlc builds its own packed index pool from asset words (PNS 2.6, 2.8). The packed-no-padding rule (map_e4.md 3) carries. The code is in git at 115a06200 (pm_native.c nat_pool_put).

1. `pm_dlc.inc.c` › `dlc_learn` and its use.
   - Delete the prototypes grep `void dlc_learn_begin(void);` and `int dlc_learn_end(u32 geom, int* rr);` (hint L562-563), and the function `dlc_learn` grep `static __attribute__((noinline)) int dlc_learn(` (hint L565-572).
   - `dlc_try` (hint L574): grep `!rec_on && !g->tid1 ? dlc_learn(` (hint L639). The condition becomes `if (!why && g->sd && dlc_static_group(e, g, gmi, gg)) {`.
   - `dlc_try`: grep `if (!rec_on && !nobox_on() && dlc_box_out(g, m))` (hint L606) becomes `if (!nobox_on() && dlc_box_out(g, m))`. Delete the lines grep `if (rec_on) { rec_dln++;` (hint L593) and `if (rec_on) { rec_gout = geom;` (hint L658).
   - `DlcGrp` typedef grep `u8 tsv_exit; u16 tid1;` (hint L9): delete the fields tid1 and pidx.
2. `pm_dlc.inc.c` › the PMOD bake and E4b NatRec shadow.
   - Delete the block from grep `/* E4b: NatRec draws of the static groups` to the end of `nat_seg_add` (hint L277-324). It holds the prototypes of pm_gpu_rec_rmode and pm_tex_cur_slots, NatKey, nk_key, nk_open, dlc_gr, nk_rm_in, nk_sl_in, nk_draw, nk_nd, nk_end, nk_mis, nat_key_reset, nat_entry_state and nat_seg_add.
   - `dlc_seg` (hint L327): grep `if (ep_cullboth) { dlc_gr = -1;` becomes `if (ep_cullboth) { n_out += (int)nt; return 1; }`. Delete grep `dlc_gr = gr;` (hint L340).
   - `dlc_static_group` (hint L350): delete grep `nat_entry_state();` (hint L366) and `nat_seg_add(g, t, end, gmi, dlc_gr);` (hint L369).
   - `dlc_static_build` (hint L219): delete grep `pm_gpu_nat_pool_init();` (hint L221), `g->pidx = nat_pool_put(` (hint L268), and the bake summary from `unsigned bnv = 0, bni = 0, bok = 1;` to the PM1 log line `PM1 bake dl` (hint L271-274).
   - `dlc_reset` (hint L554): delete grep `nat_model_reset();`. `dlc_try`: delete grep `nat_key_reset();` (hint L599) and the NATB log line grep `NATB dl %08lx` (hint L656).
3. `pm_gbi.c` › the record capture block. Delete from the comment grep `/* RC1: record capture (copy path).` (hint L3560) up to the line before grep `/* MR1: runs the n words of one model group` (hint L3895). The block holds these symbols: the externs gstat_gen, pm_rec_ver, pm_rec_tex. RecKey, REC_NO_SLOT, REC_ARENA_WORDS, REC_MAX_WORDS, rk, ra, ra_n, ra_gen, ra_fail. The rec_* counters, rec_g0, NAT_MTX_MAX, REC_CEN_MAX, nat_logged. The pm_gpu_rec_* and pm_tex_rec_use prototypes. rec_ibad, rec_runs. The NW enum, nw_name, nat_why, nat_gw, NW, nat_why_report. rec_chk, rec_chk_sw, rec_play_sw, rec_rp, rec_hgen, rec_tgen, rec_g0_set. dlc_learn_begin, dlc_learn_end, rec_reset, rec_try, uc2d_first, uc2l_first, rec_check, p3a_log, p3a_pre, rec_done, rec_report. NAT_OUT_WORDS, nat_ord_v, nat_log_v, nat_nl, nat_out, rec_play.
4. `pm_gbi.c` › the call sites and globals of the capture, outside the block.
   - File statics: grep `static u32 rec_gdl;` (hint L86) and `static int rec_on; static unsigned rec_dln` (hint L87).
   - `pm_gbi_end_frame`: grep `static void rec_report(void); /* RC1` (hint L2311) and `if (++rec_fr >= 300) { rec_fr = 0; rec_report(); }` (hint L2321).
   - `pm_texbind_set` (hint L2752): grep `u32 tb_gen[128];` (hint L2751) and `tb_gen[idx & 0x7f]++;` (hint L2755). tb_gen had only rec_hgen as its reader.
   - `mg_run` (hint L3898), case G_DL: grep `if (rec_rp && rec_play(next)) break;` (hint L3945). The next line `if (dlc_on && dlc_try(next)) break;` stays.
   - `gbi_run_body`, case G_PM_MODEL (hint L4021): delete grep `Gfx* g0 = pc; /* RC1`, `int rc = (w1 & 0x200u) ? rec_try(w1) : 0;`, `rec_rp = NULL; /* UC3e` and `if (rc) rec_done(w1, pc == g0 + n && !nx);` (hint L4026-4030). The line `pc = mg_run(pc, n, &nx, w1);` stays.
   - grep `#include "pm_native.h"` (hint L10): delete it if no nat_ symbol is left in pm_gbi.c.
5. `pm_gpu.c` › the capture arena, replay and probes.
   - Delete from grep `/* RC1: the capture only observes.` (hint L2592) to the end of `pm_gpu_rec_cls` grep `void pm_gpu_rec_cls(unsigned* c)` (hint L2809). Keep `pm_gpu_flush` above it and grep `unsigned pm_ec_n, pm_ec_svc;` below it. The range holds REC_DIRTY_CAP, pm_gpu_rec_fw, pm_gpu_rec_begin, pm_gpu_grp_get, pm_gpu_rec_end, REC_IDX_N, ia, ia_n, ia_fail, pm_gpu_rec_idx, pm_gpu_rec_idx_reset, rec_ia_frame, pm_gpu_view_key, pm_gpu_rec_ia_phys, pm_gpu_rec_ia_eq, pm_gpu_rec_ichk, RecPatch, REC_PT_MAX, rec_pt, rec_foreign, pm_gpu_rec_walk, pm_gpu_rec_pt_copy, pm_gpu_rec_gm_cap, NAT_MTX_MAX, REC_NAT_WORDS, nat_buf, nat_row, nat_pos, nat_fill, nat_patch, pm_gpu_rec_palok, pm_gpu_rec_cen_n, pm_gpu_rec_rmode, pm_gpu_rec_cen_copy, **pm_gpu_rec_play** (hint L2748), pm_gpu_rec_count, REC_CHK_MAX, rec_msk, rec_cl, rec_cc, rec_pa, rec_fj, pm_gpu_rec_same, pm_gpu_rec_fj, pm_gpu_rec_cls.
   - Capture state: grep `static uint32_t rec_fw[16][4];` to `static u32 rcw[REC_CEN_MAX * 4u];` (hint L496-499). grep `static int rec_ref; static u32* rb_buf;` to `static int ia_hold, ia_rst;` (hint L637-640).
   - `cen_add` (hint L510): grep `if (rec_cap) { if (rcw_n < REC_CEN_MAX)` (hint L518).
   - `flush_draw` (hint L1028): every `if (rec_cap)` statement. grep `rec_cap` gives hint L1065-1069, L1142-1146, L1148, L1155. At L1155 delete only `if (rec_cap) rec_ref = 1;` inside `if (noise) {`. Also the two ring-draw lines grep `if (rec_cap) rec_ref = 1; /* UC3b: a ring draw` (hint L2235, L2301).
   - `pm_gpu_begin` area: grep `void rec_ia_frame(void); /* UC3e` (hint L2120) and `rec_ia_frame(); /* UC3e` (hint L2122).
   - `cen_frame_end` (hint L596): grep `void nat_why_report(unsigned fr);` (hint L595) and `nat_why_report((unsigned)fr);` (hint L606).
   - `pm_gpu_pal_fill` grep `void pm_gpu_pal_fill(const float (*m)[4], unsigned gm, unsigned eye)` (hint L945). Its only caller is rec_play. Keep `pm_eye_idx` (S1-4 decides) and `pal_mul` (pcomp_calc calls it).
   - `pm_gpu_stc_ctx_log` grep `void pm_gpu_stc_ctx_log(const char* at)` (hint L144). Its only caller is uc2d_first.
   - `pm_gpu_nat_pool_init` grep `void pm_gpu_nat_pool_init(void)` (hint L3028-3033). This is the nat_pool_set call.
   - grep `unsigned gstat_gen;` (hint L121) and the `gstat_gen++;` in `pm_gpu_gstat_reset` (hint L2389). rec_try was the only reader.
6. `pm_native.c`, `pm_native.h`, `pm_tex.c`, `nat_host_test.c`.
   - pm_native.c: delete `nat_tmpl_strip` grep `int nat_tmpl_strip(unsigned id` (hint L61) and its table `nat_kind` (hint L53). Keep NAT_GOFF_PAL, because nat_emit uses it. Delete `nat_rec_write` grep `int nat_rec_write(NatRec* out` (hint L180) with nat_wr_why, nat_wr_off, nat_wr_base (hint L179). Delete the PMOD block grep `/* PMOD P1: bake records.` (hint L216-250): nat_model, nat_nmodel, nat_pool_base, nat_pool_size, nat_pool_used, nat_pool_set, nat_model_reset, nat_pool_put, nat_pool_off, nat_model_bake. Keep `nat_gpu_addr` (hint L227).
   - pm_native.h: delete the matching prototypes (nat_tmpl_strip L51, nat_rec_write L56) and the PMOD block grep `/* PMOD P1: bake record of one static model DL` (hint L73-85), except `nat_gpu_addr`. Keep nat_tmpl_add, nat_tex_id, nat_order, nat_emit, nat_eye_copy until S1-4.
   - pm_tex.c: delete `pm_tex_rec_use` grep `int pm_tex_rec_use(int s)` (hint L743). Its only caller is rec_play.
   - nat_host_test.c: delete `test_bake` grep `static void test_bake(void)` (hint L228) and its call in main (hint L266).
   - Leave for S3-5 (model.c is an S3 file): `pm_rec_ver`, `pm_rec_tex` (src/model.c hint L5123-5130) lose their caller here. The G_PM_MODEL header bit 9 grep `0x200u : 0u` (model.c hint L1520) becomes unread.
Host test: this is a delete row, so no red step is possible (agent_nds_s1.md finding 4). nat_host_test.sh passes record, template, order, layout and tex_id after the delete. The build links with no capture symbol: `git grep -nE "rec_try|rec_done|rec_play|pm_gpu_rec_|nat_tmpl_strip|nat_rec_write|nat_pool_|nat_model_|dlc_learn" 3ds/src 3ds/include` returns only model.c pm_rec_*.
Gate: census_mac01 (E): triangles 810, batches 64 or less (the replay-off numbers [M diag_e53_merge.md 3 option A]). dlc_learn flushed once per group at the first frame after a map load, so fewer batches are allowed. The `mem` line (pm_mem.c, grep `PM_LINEAR_FIXED_KB`) shows linear memory down by about 256 KB (nat pool 128 KB and rec index arena 128 KB) [U]. Picture: census_toad_shot (E) region_diff against the S0 shot.

**S1-2 | NDREC records, frame driver, LEGACY bridge**
Files: 3ds/include/nd.h, 3ds/src/nd_core.c, 3ds/src/pm_nusys.c, 3ds/CMakeLists.txt.
1. nd.h (new): NdRec (PNS 2.2) with _Static_assert size 32, kinds, nd_frame_begin, nd_frame_end, nd_emit, nd_bridge_cut. 30 lines.
2. nd_core.c (new): record array (2048, aligned 32), nd_bridge_cut (LEGACY record from gMainGfxPos), nd_frame_end. 30 lines.
3. nd_core.c nd_emit: loop with PLD, DRAW_IDX first, LEGACY arm calls a cold function that runs pm_gbi_run(start, bytes), pm_gpu_flush, and the shadow reset. Once per emit: `pm_phase.c` › `pm_pf_nd_add` grep `void pm_pf_nd_add(unsigned long long ticks, unsigned legacy)` (hint L321) with the emit ticks and the LEGACY count. 30 lines.
4. `pm_nusys.c` › `gbi_task` grep `static void gbi_task(Gfx* gfxList, u32 n)` (hint L76). Replace grep `pm_gbi_run(gfxList, n);` (hint L80) with nd_frame_end and nd_emit. Stereo line grep `if (pm_gpu_stereo()) { pm_gpu_eye(1); pm_gbi_run(gfxList, n);` (hint L81): eye 1 runs nd_emit again (LEGACY only now). 10 lines.
5. `3ds/CMakeLists.txt` › the port source list: add src/nd_core.c after grep `src/pm_native.c` (hint L366). The hot span placement is S10-3. 3 lines.
Host test (first): nd_host_test case `legacy only`: a frame of 3 LEGACY records gives 3 walker calls in order (stub pm_gbi_run). Gate: G1, G5 (nd_size_test.sh), picture census_toad_shot equal, yard_shot (E) equal.

**S1-3 | NDTMPL templates from the key**
Files: 3ds/src/nd_tmpl.c, 3ds/include/nd.h.
1. Key struct, 256-entry table, open hash with a 512 index. 30 lines.
2. TEV words from `pm_comb.c` › `pm_comb_build` grep `int pm_comb_build(unsigned w0, unsigned w1, int two, int t1only, PmPlan* out)` (hint L393): stage source, operand, function, scale, constant, buffer colour. The values are those that `pm_gpu.c` › `flush_tev_rebuild` grep `static __attribute__((noinline)) void flush_tev_rebuild(` (hint L736) writes today. 30 lines.
3. Depth, early depth, depth map, alpha test, stencil, cull, blend words, moved from `pm_gpu.c` › sh_depth_test, sh_depth_map, sh_alpha_test, sh_blend, sh_cull, sh_early, sh_user_clip grep `static void sh_depth_test(bool en` to `static void sh_user_clip(int on)` (hint L647-680). DECAL_EPS rule: grep `#define DECAL_EPS` (hint L487) and `sh_depth_map(true, -1.0f, -DECAL_EPS)` in flush_draw (hint L983). 30 lines.
4. Render state rules moved from `pm_gbi.c` › `sync_render` grep `static void sync_render(void)` (hint L650) and `rm_apply` grep `static int rm_apply(u32 w1)` (hint L2847): othermode to z, decal, blend, atest, zmark. The walker copies stay until S10. 30 lines.
Host test (first): for every ndref draw, nd_tmpl_build(key) words decode (nd_pica_decode) to the ndref TEV, depth, blend, alpha and cull fields. Gate: G1.

**S1-4 | NDMTX matrices, lights, eye patch**
Files: 3ds/src/nd_mtx.c, 3ds/src/nd_core.c, 3ds/src/pm_native.c (deleted), 3ds/include/pm_native.h (deleted), 3ds/tools/nat_host_test.c and nat_host_test.sh (deleted, tests moved), 3ds/src/pm_gbi.c and pm_gpu.c (include line only), 3ds/CMakeLists.txt.
1. nd_mtx_push, nd_mtx_pop (16 deep). Unpack moved from `pm_gbi.c` › `unpack` grep `static void unpack(const Mtx* src, float d[4][4])` (hint L451). pcomp = post x game, as `pm_gpu.c` › `pal_mul` grep `Mtx_Multiply(o, &gv_post, &game);` (hint L936) and `rebuild_3d` grep `Mtx_Multiply(&shader_3d, post, &game);` (hint L230). 30 lines.
2. Matrix pool per eye. Eye 1 product with the stereo shift, as `pm_gbi.c` › `upload_mvp` grep `static void upload_mvp(void)` (hint L457). 25 lines.
3. Light blocks (pm5 layout). 20 lines.
4. Move `nat_eye_copy` grep `void nat_eye_copy(uint32_t* dst` (pm_native.c hint L173) and `nat_order` grep `unsigned nat_order(const NatRec* rec` (hint L127) into nd_core.c on NdRec. Move their nat_host_test.c cases (test_order, test_layout) into nd_host_test. Delete pm_native.c, pm_native.h, nat_host_test.c, nat_host_test.sh, the CMake line grep `src/pm_native.c` and the `#include "pm_native.h"` lines (pm_gbi.c hint L10, pm_gpu.c hint L13). Delete `pm_eye_idx` (pm_gpu.c hint L944) if nd_core does not read it. 30 lines.
Host test (first): nd_seam_test (S0-5) passes. Eye copy equals a direct eye 1 emit. Gate: G1, G9.

**S1-5 | NDINTERP fps60 interpolation with records**
Files: 3ds/src/nd_mtx.c, 3ds/src/pm_nusys.c.
1. Read pm_interp.c in full. nd_mtx_push looks up `pm_interp.c` › `pm_interp_find` grep `pm_interp_find(const void* m))[4]` (hint L117). 15 lines.
2. Key-frame replay `pm_nusys.c` › `pm_interp_key_draw` grep `int pm_interp_key_draw(void)` (hint L128): re-emit the kept records with the unreplaced matrices. 20 lines.
Host test (first): two emits of one record set with and without the replacement table differ only in matrix words. Gate: G1. Picture: merlon_ball60 (E) equal.

**S1-6 | NDTEX texture descriptors and unit words**
Files: 3ds/src/nd_tex.c, 3ds/include/nd.h.
1. nd_tex_desc, nd_tex_get: hash probe on (raster, palette, fmt, siz, w, h), pm_tex load on a miss, 4-word unit block from `pm_tex.c` › `pm_tex_slot_words` grep `int pm_tex_slot_words(int i, uint32_t* out)` (hint L715). 30 lines.
2. Wrap, mirror, mask, shift to the parameter word, moved from `pm_gbi.c` › `tile_bound` grep `static int tile_bound(int n)` (hint L2544) and `ensure_tile` grep `static void ensure_tile(int n) {` (hint L2576). 30 lines.
Host test (first): for every ndref texture, nd_tex words equal the ndref unit 0 and unit 1 words. Gate: G1.

### S2 NDFRAME

**S2-1 | NDGBI redirect header, state half**
Files: 3ds/include/nd_gbi.h (new), 3ds/src/nd_core.c. No old-code anchor (new files).
1. nd_gbi.h: `#undef` and redefine the state macros (PNS table 5) as nd_* calls plus the bridge dual-write of the N64 word. 30 lines per item, 3 items.
2. Draw macros: nd_vtx, nd_tri, nd_tri2, nd_fill_rect, nd_tex_rect, nd_obj_draw. Unknown macro: `#error`. 30 lines.
3. grep list of every gSP and gDP macro name used in src/ (R10) in a comment block, each mapped or marked `#error`.
Host test (first): a host file that uses every mapped macro compiles and produces the expected nd call log. Gate: G1.

**S2-2 | NDPASS frame state calls**
Files: 3ds/src/nd_core.c, 3ds/CMakeLists.txt.
1. nd_pass_begin, nd_viewport, nd_scissor, nd_clear, nd_target. nd_viewport writes the post matrix as `pm_gpu.c` › `gv_post` grep `static C3D_Mtx gv_post;` (hint L184) does in `rebuild_3d` grep `static void rebuild_3d(void)` (hint L201-230). 30 lines.
2. CMakeLists.txt: force-include nd_gbi.h for src/main_loop.c, src/cam_main.c, src/background_gfx.c, src/background.c. Pattern: grep `APPEND PROPERTY COMPILE_OPTIONS -include common.h` (hint L336, L338). 6 lines.
Host test (first): the background draws of ndref_title and ndref_yard equal (nd_stream_cmp). Gate: G1 to G4, G10. Picture: yard_shot (E), golden_multi (E) title shot.

**S2-3 | NDN64 N64-same check for S2 files**
Files: none changed (check only). Run nd_n64_same.sh on the 4 files. Gate: no diff.

### S3 NDMODEL

**S3-1 | NDDLC static list compiler**
Files: 3ds/src/nd_dlc.c, 3ds/src/pm_dlc.inc.c, 3ds/src/pm_map.c, 3ds/CMakeLists.txt.
1. Move `pm_dlc.inc.c` › `dlc_compile` grep `static __attribute__((cold, noinline)) int dlc_compile(DlcEnt* e, const Gfx* l)` (hint L145) and `dlc_static_build` grep `void dlc_static_build(DlcEnt* e, const Gfx* l)` (hint L219) into nd_dlc.c. Output NdObj into the static pool and a new packed index pool (no padding, map_e4.md 3). The S1-1 PMOD pool is gone, so this row writes the pool (the old nat_pool_put at 115a06200 is the pattern). 30 lines per item, 2 items.
2. Extend the op set: SETCOMBINE, SETOTHERMODE, colour ops, texture loads (to nd_tex_desc), nested static DL. Compute the dep mask. 30 lines per item, 2 items.
3. `pm_map.c`: compile the shape lists at map load, after grep `fix_shape(dst, dec);` (hint L378) and the bump grep `pm_dlc_gen++; } /* DLC1: the shape file is final now` (hint L379). The list words are already swapped by `fix_gfx` grep `static void fix_gfx(ShapeFix* s, u8* dl)` (hint L97). 10 lines.
4. Move `dlc_box_out` grep `static __attribute__((noinline)) int dlc_box_out(const DlcGrp* g` (hint L405) as is (the only allowed skip, memory feedback_no-culling). 20 lines.
Host test (first): for the mac_01 and mac_02 shapes, NdObj index lists equal the dlc_static_build sd lists (the P1 host check of DPM 10). Gate: G1.

**S3-2 | NDMDL1 plain opaque models**
Files: src/model.c (TARGET_3DS blocks), 3ds/CMakeLists.txt.
1. `src/model.c` › grep `#define MODEL_TEX_OP(pos, h) gSPPmTexBind(` (hint L185) becomes nd_tex_handle(index). grep `#define MODEL_RM_OP(pos, rm) gSPPmRenderMode(pos, rm)` (hint L177) becomes nd_obj_draw of the render mode list. 10 lines.
2. `load_texture_impl` grep `void load_texture_impl(u32 romOffset, TextureHandle* handle` (hint L2115): add `nd_tex_handle_set(index, header, raster, palette, aux)` after grep `pm_texbind_set(handle - TextureHandles, handle->gfx);` (hint L2166). 8 lines.
3. `appendGfx_model` grep `void appendGfx_model(void* data)` (hint L1384): delete the G_PM_MODEL header, every line with grep `mgHdr` (hint L1406, L1515-1521, L2073). 15 lines.
4. CMakeLists.txt: force-include nd_gbi.h for src/model.c (pattern of S2-2 item 2). 2 lines.
Host test (first): host appendGfx_model with the ndref model inputs (plain, SURFACE_OPA) gives draws equal to the ndref model draws (nd_stream_cmp, tag filter). nd_n64_same.sh on src/model.c. Gate: G1 to G4, G9, G10. Picture: census_toad_shot (E), mac01_seam (E).

**S3-3 | NDMDL2 all render modes and classes**
Files: src/model.c, 3ds/src/nd_tmpl.c.
1. Fog, depth tint, remap tint, shroud in `appendGfx_model`: their colour and fog calls go to the uniform block (no template split per value). 25 lines.
2. Alpha test, decal, translucent render modes: ordered flag. 15 lines.
Host test (first): all tagged models of ndref_toad, ndref_yard equal. Gate: G1 to G4. Picture: forest_mim (E), toad_water (E).

**S3-4 | NDMDL3 panner, billboard, custom gfx, local vertex copy, projected coords**
Files: src/model.c.
1. Panner: tile offsets per frame to uvso, moved from `pm_gbi.c` › `gv_params_calc` grep `static void gv_params_calc(unsigned tss, GvP* o)` (hint L1351). 15 lines.
2. Custom gfx: `build_custom_gfx` grep `void build_custom_gfx(void) {` (hint L4090) output is recorded as range ids. The two call sites in `appendGfx_model` grep `gSPDisplayList((*gfxPos)++, (*gCurrentCustomModelGfxPtr)[customGfxIndex]);` (hint L2016, L2063) repeat the range. 30 lines.
3. Local vertex copy: grep `if (!(flags & MODEL_FLAG_HAS_LOCAL_VERTEX_COPY))` (hint L2055), `mdl_local_gfx_update_vtx_pointers` (hint L4252), `mdl_local_gfx_copy_vertices` (hint L4272), `mdl_make_local_vertex_copy` (hint L4280). Index lists compiled once, vertices through the ring. 20 lines.
4. `mdl_project_tex_coords` grep `void mdl_project_tex_coords(s32 modelID` (hint L4339): read, then the agent reports one of: no edit, or a named edit for the planner. 0 to 20 lines.
Host test (first): the panner, billboard and custom gfx models of ndref_toad and ndref_yard equal. Gate: G1 to G4. Picture: toad_fountain (E), kmr_10_band (E).

**S3-5 | NDMDL4 floor reflection, sort, delete**
Files: src/model.c, 3ds/src/nd_core.c, 3ds/src/pm_gbi.c, 3ds/src/pm_dlc.inc.c (deleted), include/gbi_custom.h, 3ds/ld/3dsx.ld.in (names only).
1. `execute_render_tasks` grep `void execute_render_tasks(void)` (hint L4888), the savedGfxPos branch grep `savedGfxPos` (hint L4965-4991): nd_range_begin, nd_range_end, nd_range_repeat with the flip matrix. 20 lines.
2. nd_core.c: sort unordered runs by (tmpl, tex0, tex1). Merge adjacent DRAW_IDX with equal key (grange rule, `pm_gpu.c` › `grange_batch_open` grep `static int grange_batch_open(unsigned gm` hint L2394). 30 lines.
3. Delete in pm_gbi.c, 3 items:
   - `gbi_run_body` grep `case G_PM_MODEL: { /* MR1` (hint L4021) and its G_PM_TEX arm grep `case G_PM_TEX: /* C1: a texture bind list` (hint L4039). `mg_run` grep `static __attribute__((noinline)) Gfx* mg_run(` (hint L3898). Both `dlc_try(next)` calls (mg_run, and gbi_run_body case G_DL grep `if (dlc_on && dlc_try(next)) { /* DLC1` hint L4058). `tb_run`, `tb_list`, `pm_texbind_set` (hint L2750-2769) when no caller is left, with the model.c call grep `pm_texbind_set(handle - TextureHandles` and prototype (model.c hint L184, L2166).
   - The model census: grep `mr0_`, `ms0_`, `sp0_` (hint L2089-2160, L2323, L4013), and the model.c caller grep `pm_gbi_ms0_mark(` (hint L1505-1508). The include grep `#include "pm_dlc.inc.c"` (hint L1628). Delete pm_dlc.inc.c. Remove .text.dlc_try, .text.mg_run, .text.dlc_* names from 3ds/ld/3dsx.ld.in grep `CF4 hot set first` (hint L31-40).
   - model.c: `pm_rec_ver`, `pm_rec_tex` grep `unsigned pm_rec_ver(unsigned slot)` (hint L5123-5130), no caller since S1-1. gbi_custom.h: delete G_PM_MODEL grep `#define G_PM_MODEL 0x0C` and G_PM_TEX grep `#define G_PM_TEX 0x0A`, gSPPmTexBind. 30 lines per item.
   - Keep G_PM_RM, its arms (gbi_run_body grep `case G_PM_RM: /* C2`, mg_run grep `case G_PM_RM:`) and rm_apply. sprite.c (SPR_RM_LIST, S4-1) and entity_model.c (G_PM_RM rewrite, S6-1) still write it as LEGACY words. S10-2 deletes them. Keep PM_REC_BUMP and the Model field pmRecVer (include/model.h, src/evt/map_api.c users). S10-2 decides.
Host test (first): batches of ndref_toad not above the reference after the sort. Gate: G1 to G10. Picture: census_toad_shot, yard_shot, forest_mim, toad_pond (all E). Console prof at the S3 end (expected 7.9 to 12.7 ms phase on).

### S4 NDSPRITE

**S4-1 | NDSPR1 sprite components**
Files: src/sprite.c, 3ds/CMakeLists.txt.
1. `spr_appendGfx_component_flat` grep `void spr_appendGfx_component_flat(` (hint L227): the G_PM_TEXI block from grep `Gfx* texHdr = gMainGfxPos++; /* NP1` (hint L236) to the two header writes grep `texHdr->words.w0 = ((u32)G_PM_TEXI << 24)` (hint L246, L279) becomes one nd_tex_load per component. 25 lines.
2. grep `#define SPR_RM_LIST(list) gSPPmRenderMode(gMainGfxPos++, list)` (hint L335), used in `spr_appendGfx_component` (hint L339): nd_obj_draw. 3 lines. Force-include for sprite.c. 2 lines.
Host test (first): the sprite draws of ndref_toad (tag filter) equal. nd_n64_same.sh. Gate: G1 to G4, G10. Picture: census_toad_shot NPC strip.

**S4-2 | NDSPR2 imgfx meshes and shading**
Files: src/imgfx.c, src/sprite_shading.c, 3ds/src/nd_tex.c, 3ds/CMakeLists.txt.
1. Force-include only. The imgfx ring vertices go through nd_vtx (`imgfx_appendGfx_mesh` grep `void imgfx_appendGfx_mesh(ImgFXState* state, Matrix4f mtx) {` hint L861). The 4-bit strip load rule moves into nd_tile from `pm_gbi.c` › `tex_op` grep `4-bit strips (imgfx meshes) load a window` (hint L3161, tex_op at L3019). 20 lines in nd_tex.c.
2. Shaded sprite (two tiles on one 4-bit image) from `pm_gbi.c` › `load_shaded_sprite` grep `static int load_shaded_sprite(int dx4, int dy4)` (hint L2505): nd_tex two-unit form. 30 lines.
Host test (first): ndref_battle and ndref_toad sprite draws equal, unit 1 words included. Gate: G1 to G4. Picture: battle_start (E), talk_off (E).

**S4-3 | NDSPR3 NPC and player call paths**
Files: src/npc.c, src/77480.c, 3ds/CMakeLists.txt.
1. Force-include. Confirm by grep that `render_npcs` grep `void render_npcs(void)` (npc.c hint L963) and `render_player` grep `void render_player(void)` (77480.c hint L1468) write gMainGfxPos only. 0 to 10 lines.
Host test (first): all sprite tags of ndref_toad equal. Gate: G1 to G4. Console prof at the S4 end.

### S5 NDHUD

**S5-1 | NDHUD1 HUD elements**
Files: src/hud_element.c, 3ds/CMakeLists.txt.
1. The G_PM_TEXI blocks: grep `Gfx* texHdr = gMainGfxPos++; /* HT1` (hint L365) to `texHdr->words.w0 = ((u32)G_PM_TEXI` (hint L402), and grep `Gfx* tileHdr = gMainGfxPos++; /* HT1` (hint L477) to `tileHdr->words.w0 = ((u32)G_PM_TEXI` (hint L600). They become nd_tex_load plus nd_tex_rect. 30 lines per item, 2 items.
2. The one gSPEndDisplayList list in hud_element.c (grep `gSPEndDisplayList`): nd_range. 15 lines.
Host test (first): ndref_text and ndref_pause HUD tags equal. nd_n64_same.sh. Gate: G1 to G4. Picture: ui1_choice (E), mapchange_hud (E).

**S5-2 | NDTXT text glyphs**
Files: src/msg_draw.c, src/msg.c, 3ds/src/nd_tex.c, 3ds/CMakeLists.txt.
1. `msg_draw_char` grep `void msg_draw_char(MessagePrintState* printer, MessageDrawState* drawState, s32 charIndex, s32 palette, s32 posX, s32 posY) {` (hint L1669): the block grep `Gfx* glyphHdr = gMainGfxPos++; /* GLYPH` (hint L1758) to `glyphHdr->words.w0 = ((u32)G_PM_TEXI` (hint L1802) becomes nd_glyph(sheet, palette, index). 20 lines.
2. nd_tex.c: glyph atlas moved from `pm_gbi.c` › `glyph_atlas_bound` grep `static int glyph_atlas_bound(void)` (hint L2552). 30 lines.
Host test (first): ndref_text glyph draws equal. Gate: G1 to G4. Picture: text_menu (E), house_sign (E), super_text (E).

**S5-3 | NDBOX boxes and windows**
Files: src/draw_box.c, src/windows.c, src/draw_img_util.c, 3ds/src/nd_core.c, 3ds/CMakeLists.txt.
1. Force-include. Fill rectangles through nd_fill_rect, moved from `pm_gbi.c` › `gbi_fillrect` grep `static __attribute__((noinline)) void gbi_fillrect(u32 w0, u32 w1)` (hint L3420): the signed 10-bit field grep `#define FILL_SX10(v)` (hint L3422) and the inclusive rule grep `HUDB1: the lower right corner of a fill is inclusive only` (hint L3458). 25 lines in nd_core.c.
Host test (first): box and window draws of ndref_text and ndref_pause equal. Gate: G1 to G4. Picture: toad_house_menu (E).

**S5-4 | NDSTAT status icons and inventory**
Files: src/status_icons.c, src/status_star_shimmer.c, src/inventory.c, 3ds/CMakeLists.txt.
1. Force-include. 5 lines.
Host test (first): status tags of ndref_toad equal. Gate: G1 to G4. Console prof at the S5 end.

### S6 NDENT

**S6-1 | NDEMDL entity models**
Files: src/entity_model.c, 3ds/src/nd_dlc.c, 3ds/CMakeLists.txt.
1. Delete the G_PM_RM rewrite: grep `Gfx* rm_first = gMainGfxPos; /* DLF4` (hint L282) and `rm_first->words.w0 = (u32)G_PM_RM << 24;` (hint L370). The render mode list becomes nd_obj_draw. 10 lines.
2. nd_dlc: key on (list, segment base) for `draw_entity_model_B` grep `void draw_entity_model_B(s32 modelIdx` (hint L492) and `draw_entity_model_D` grep `void draw_entity_model_D(s32 modelIdx` (hint L561). 25 lines.
Host test (first): ndref_toad entity tags equal. nd_n64_same.sh. Gate: G1 to G4, NdObj count per map logged. Picture: toad_gate (E).

**S6-2 | NDENT2 entity files and shadows**
Files: src/entity/*.c, src/entity/Shadow.c, src/entity.c, 3ds/CMakeLists.txt.
1. Shadow.c: delete the two headers grep `gsSPPmTexInline(` (hint L31, L50) and the EN2 comment grep `/* EN2: the texture load of each shadow` (hint L28). 6 lines.
2. Force-include for src/entity/*.c and src/entity.c. 3 lines.
Host test (first): shadow and block draws equal (ndref_toad, ndref_yard). Gate: G1 to G4. Picture: coin_kmr03 (E).

**S6-3 | NDITEM item entities**
Files: src/item_entity.c, 3ds/CMakeLists.txt.
1. Force-include. 2 lines. Host test (first): item tags equal (ndref_yard). Gate: G1 to G4. Picture: coin_dgb13 (E).

**S6-4 | NDANIM animators**
Files: src/animator.c, 3ds/CMakeLists.txt.
1. `appendGfx_animator` grep `void appendGfx_animator(ModelAnimator* animator) {` (hint L779) and `appendGfx_animator_node` grep `void appendGfx_animator_node(ModelAnimator* animator, AnimatorNode* node, Matrix4f mtx) {` (hint L851): lists built for later become ranges. 30 lines.
Host test (first): animator tags of ndref_battle equal. nd_n64_same.sh. Gate: G1 to G4. Picture: battle_start (E). Console prof at the S6 end.

### S7 NDWORLD

**S7-1 | NDWLIST list world and battle files with deferred lists**
No code. grep for local Gfx pointers, gSPEndDisplayList and custom gfx builders in src/world and src/battle. Output: the file list for S7-2 and S7-3 in docs/handoff/agent_S7.md, with a symbol anchor (function name plus grep string) for each site.

**S7-2 | NDWORLD2 world area files**
Files: the S7-1 list for src/world, 3ds/CMakeLists.txt. 1. Force-include. 2. Range edits per listed file (30 lines each, one file per item, anchors from agent_S7.md). Host test (first): world tags of ndref_toad and ndref_yard equal. Gate: G1 to G4. Picture: tour10 is not used (full test). Use house_sign (E), toad_fountain (E).

**S7-3 | NDBTL battle files**
Files: the S7-1 list for src/battle, 3ds/CMakeLists.txt. As S7-2. Host test (first): ndref_battle equal. Gate: G1 to G4. Picture: battle_start (E), battle_menu_bar (E), battle_danger (E).

### S8 NDFX

**S8-1 | NDFXLIST effect files with buffer reads and blob reads**
No code. grep src/effects for gDPSetColorImage, frame buffer reads, big-endian blob reads (risk constraint 7). Output: list for S8-6, with a symbol anchor for each site.

**S8-2 to S8-5 | NDFX1 to NDFX4 effect files A to Z in 4 blocks of about 32 files**
Files: src/effects/<block>.c, 3ds/CMakeLists.txt. 1. Force-include for the block. 2 lines. 2. Fix list per file only if the host test fails (one file per item).
Host test (first): ndref_splash effect tags equal (S8-2 to S8-5 each add one fxtest ndref scene for a block effect, scenario (N) ndref_fx_<block>, base nok11_splash with another effect id). Gate: G1 to G4. Picture: nok11_splash (E), chapter_wide (E), bowser_intro_fire (E).

**S8-6 | NDFXS splash markers, buffer effects, effects.c**
Files: src/effects/water_splash.c, the S8-1 list, src/effects.c, src/effect_utils.c.
1. water_splash.c: grep `gPmSplashMark(gMainGfxPos++, PM_SPLASH_ON);` (hint L190) and `gPmSplashMark(gMainGfxPos++, PM_SPLASH_OFF);` (hint L221) become nd_splash(1) and nd_splash(0). 6 lines.
2. Buffer effects: nd_target(role) per call site of the S8-1 list. 30 lines per file.
Host test (first): ndref_splash equal. nd_n64_same.sh. Gate: G1 to G4. Picture: nok11_splash. Console prof at the S8 end.

### S9 NDSCREEN

**S9-1 | NDTGT target roles and CPU-path cases**
Files: 3ds/src/nd_core.c, 3ds/src/nd_tmpl.c.
1. nd_target roles from `pm_gbi.c` › `img_role` grep `static int img_role(int for_fill)` (hint L243): main, off-screen texture, depth, CPU fill, colour capture. 30 lines.
2. List every case that the walker draws on the CPU: `draw_tri_cpu` grep `static __attribute__((noinline)) void draw_tri_cpu(const int* idx)` (hint L1657), the counters grep `static unsigned seam_obj, seam_clip;` (hint L364) and their increments grep `seam_obj++` and `seam_clip++` (hint L1826, L1830), and the refusals of `pm_gpu.c` › `pm_gpu_grange` grep `int pm_gpu_grange(const float (*tab)[4][4]` (hint L2403) (R6). One GPU form per case (zmark, decal with blend, pm_wide). 30 lines per case.
Host test (first): CPU-path count 0 in all ndref scenes with the native path. Gate: G1.

**S9-2 | NDOVL screen overlays and curtains**
Files: src/screen_overlays.c, src/screen_render_util.c, src/curtains.c, 3ds/CMakeLists.txt. Force-include. Role edits per call site (gDPSetColorImage sites by grep; fade full cover is screen_overlays.c grep `OVERLAY_SCREEN_COLOR`, PNS 15 Q10). Host test (first): overlay tags equal. Gate: G1 to G4. Picture: intro_native (E), merlon_door (E).

**S9-3 | NDPAUSE pause and file menu**
Files: src/pause/*.c, src/filemenu/*.c, 3ds/CMakeLists.txt. Force-include. Host test (first): ndref_pause equal. Gate: G1 to G4. Picture: pause_spin (E), fileselect_rotate (E), file_select (E).

**S9-4 | NDTITLE title, logos, misc**
Files: src/state_title_screen.c, src/state_logos.c, src/43F0.c, src/8a860_len_3f30.c, src/speech_bubble.c, src/i_spy.c, src/inspect_icon.c, src/pulse_stone.c, src/level_up*.c, src/starpoint*.c, src/main.c, src/evt/virtual_entity.c, src/os/nusys/nugfxinit.c. Force-include. Host test (first): ndref_title equal. Gate: G1 to G4. Picture: golden_multi title shot (E), kmr10_pos (E).

**S9-5 | NDZERO LEGACY count zero**
No new code. prof `leg` field 0 in every catalog scene (one run of each scene that a slice used, not the full catalog). Gate: leg 0. Any scene with leg above 0 names its producer for a fix row.

### S10 NDDEL

**S10-1 | NDDEL1 delete the bridge**
Files: 3ds/include/nd_gbi.h, 3ds/src/nd_core.c, 3ds/src/pm_nusys.c.
1. nd_gbi.h: delete the dual-write half. 30 lines.
2. nd_core.c: delete the LEGACY kind, nd_bridge_cut, nd_unset_use. 30 lines.
3. `pm_nusys.c` › `gbi_task` grep `static void gbi_task(Gfx* gfxList, u32 n)` (hint L76): delete the eye-2 walk and pm_gbi_run. 10 lines.
Host test (first): no symbol pm_gbi_run referenced (nm on the host build). Gate: G1.

**S10-2 | NDDEL2 delete the walker**
Files: 3ds/src/pm_gbi.c (deleted), 3ds/src/pm_gpu.c, include/gbi_custom.h, src/model.c, src/sprite.c (only if words remain), 3ds/CMakeLists.txt.
1. Delete pm_gbi.c and its CMake line grep `src/pm_gbi.c` (hint L364). 5 lines.
2. pm_gpu.c, one function per item, 30 lines or less of residue each: `flush_draw` grep `static void flush_draw(void) {` (hint L1028), `pm_gpu_grange` and `grange_batch_open` (hint L2394-2403), the CPU vertex path grep `static Vert* vert_base;` (hint L44) and its alloc grep `vert_base = (Vert*)linearAlloc(` (hint L1483), the census grep `static void cen_add(` (hint L510), `static void cen_frame_end(void)` (hint L596), and the ndref hook grep `ndref_draw(` (hint L551, L1179; the reference files stay, PNS 9.2).
3. gbi_custom.h: delete G_PM_RM grep `#define G_PM_RM 0x09`, G_PM_TEXI grep `#define G_PM_TEXI 0x0B`, gsSPPmTexInline, gSPPmRenderMode and the PM_SPLASH words grep `#define PM_SPLASH_ON`. sprite.c and model.c: delete the dead `#else` forms of SPR_RM_LIST and MODEL_RM_OP if no caller is left. PM_REC_BUMP and pmRecVer (include/model.h grep `u32 pmRecVer;`): delete only if the Model layout stays 0xB0 with a pad field. Else keep and report. 10 lines.
Host test (first): build links. Gate: G1, G5.

**S10-3 | NDHOT hot span and cold split**
Files: 3ds/ld/3dsx.ld.in, 3ds/CMakeLists.txt.
1. `3ds/ld/3dsx.ld.in` › the CF4 span grep `/* .text: CF4 hot set first` (hint L31-40): replace the walker names with nd_emit and its callees in call order. 10 lines.
2. CMakeLists.txt: nd_dlc.c and the cold nd files at -Os. 3 lines.
Host test (first): nd_size_test.sh 8 KB or less. Gate: G5, G7 (map file).

**S10-4 | NDFULL epic merge test**
No code. Full catalog once (CLAUDE.md rule 5, epic merge). Console prof push with `prof` and `prof profstall` (Q1). Gate: G2 on all catalog shots, G6 to G8 numbers recorded in docs/handoff/agent_S10.md.

### S11 NDTUNE (reopen rules only)

**S11-1 | NDSORT2 deeper sort or merge** only if G4 shows draws above 64 in Toad Town. Files: nd_core.c (nd_emit and the S3-5 sort).
**S11-2 | NDPIPE GPU pipeline depth 2** only if prof gwait is above 2 ms. Files: pm_gpu.c (`pm_gpu_begin_wait` grep `void pm_gpu_begin_wait(void)` hint L2059, `pm_gpu_end` grep `void pm_gpu_end(void)` hint L2129, the C3D_Init command buffer grep `C3D_Init(0x180000)` hint L1383, the vertex ring grep `GRING_VTX = 16384` hint L116), nd_core.c. Plan by Opus first (PC1 section 6).
**S11-3 | NDCORE1 emit on core 1** only if prof nd is above 3 ms after S10 and the user answers Q11. Files: pm_main.c, nd_core.c. Thread rules: no blocking call under a lock, one lock at a time, event wait not busy wait (AC plan rules). The planner anchors pm_main.c by symbol when the row opens.

## 3. Scenarios to add (all in S0-2 unless stated)

| Name | Base | Cap s | Until-line | Row |
|---|---|---|---|---|
| ndref_toad | census_mac01 | 25 | `ndref f200 done` | S0-1 |
| ndref_yard | census_yard | 25 | `ndref f200 done` | S0-2 |
| ndref_pause | pause_menu | 40 | `ndref f2\d\d done` | S0-2 |
| ndref_battle | battle_start | 20 | `ndref f18\d done` | S0-2 |
| ndref_text | text_menu | 45 | `ndref f17\d done` | S0-2 |
| ndref_title | golden_multi (title part) | 30 | `ndref f\d+ done` | S0-2 |
| ndref_splash | nok11_splash | 20 | `ndref f\d+ done` | S0-2 |
| ndref_stereo | yard_stereo_l | 45 | `ndref f20\d done` | S0-2 |
| ndref_fx_<block> | nok11_splash with another effect id | 20 | `ndref f\d+ done` | S8-2 to S8-5 |

Caps: the base scenario caps of yard_stereo_l and text_menu are 90 s [C azt_scenarios.json]. The new caps are 45 s or less (memory feedback_test-time-cap). If the subject is not on screen by the cap, the row reports it and the planner picks another base.
