# plan_dl_cleanup: display-list code that the stateful record path replaces, keeps, or must clear first

Plan only (Opus). Read at HEAD 9491c1222 (PF3). No source edit, no run. Rules: CLAUDE.md 2, 5, 7 (no toggles, the finished
path replaces the old one and the same row deletes the old path, no stubs). Memory: no culling.
Sources: design_inputs_stateful.md (plan_stateful_records.md does not exist), plan_dl_rows.md, plan_mr2_record.md,
agent_drec1/dlf/cfi/cfd/tbrep/bat1/cfp/dlm/gen0/icf/pf1.md, and grep of 3ds/src, src, include, 3ds/ld.

Stateful path (SR): one retained record per model, version counters per input, a raw PICA command list per record, one
batch per frame. Matrix, lights, camera go into one per-frame block.

SR scope from docs/handoff/plan_stateful_records.md (section f): SR1 version counters (no reader), SR2 header w1 and
load_mtx matrix cache, SR3 capture in mg_run plus reccheck (records built, not drawn), SR4 glue, jumps, citro3d dirty, glue
flush, run end before kick splits, SR5 fog/tint/panner/billboard/custom classes. Plan section (e): the old path (mg_run
words, dlc_try) STAYS for every model without a valid record and is the rebuild path. Runs keep game order, no reorder.
So no SR row deletes an old path. Every superseded-looking item below is KEEP.

Tags: DEL-PRE = delete before SR1 (dead or stale, not used by SR). KEEP = must coexist. HOOK SRn = keep, and row SRn adds
a call, a counter or a guard there.

## (a) Compiled static map-shape path (dlc), 3ds/src/pm_dlc.inc.c (included by pm_gbi.c 1643)

| Item | File:line | What it does now | SR relation | Added by |
|---|---|---|---|---|
| pm_dlc_gen | pm_dlc.inc.c 7; bumped pm_tex.c ~854, pm_map.c ~379 | Version of the shape buffer (gMapShapeData). Key of every dlc entry | KEEP: global key word of every record (SR plan a1) | 6e2cb90ce DLC1 |
| dlc_compile, DlcGrp/DlcEnt | 88-156, 9-10 | Cuts a shape list into groups (G_VTX at slot 0 after triangles), box per group, geometry-mode ops per group | KEEP in place: old path and rebuild path. SR3 captures its output, it does not move it | 6e2cb90ce DLC1 |
| dlc_static_build, pm_gpu_gstat_alloc/commit/reset | 162-212; pm_gpu.c 1981-1997 | Copies each group's Vtx once into the static area (gring tail), one cache flush per group at compile | KEEP: record attribute buffers point into this area. HOOK SR1: gstat_gen++ in pm_gpu_gstat_reset (1996) | b7aa7c739 DLC2 |
| dlc_batch_compile, dlc_gop_same_geom | 54-83, 45-51 | Merges index ranges of a group that share geometry mode | KEEP in place | 588b189a5 |
| dlc_try, dlc_find, dlc_tab hash, dlc_reset | 440-532, 261-276, 433-437 | Per G_DL into the shape buffer: hash lookup, box test per group, replay | KEEP: old path for models without a valid record, and the capture run (plan e). Never deleted | 6e2cb90ce DLC1 |
| Group skip: dlc_box_out in dlc_try | 473-481 (dlc_box_out 289-304) | Skips a group whose box is outside one clip plane (80 % of groups in SPLIT0C) | KEEP on the old path. HOOK SR3: the capture run of a model calls dlc_try with the box test off (plan h5), so the record holds every group. Do NOT delete before SR1. Risk at the SR4 push: GPU vertex load rises for record models; read ft against wk | 6e2cb90ce DLC1 |
| dlc_box_out function | 289-304 | Also used by gp1_box_class (337-348), which sets vert_cb for interpreted G_VTX loads (pm_gbi.c 3799, 3813-3819) | KEEP | 6e2cb90ce, GP1 588b189a5 |
| dlc_static_group, dlc_seg, dlc_set_geom | 236-259, 215-233, 278-283 | Per group in view: epoch enter, gv_params_calc, pm_gpu_grange | KEEP (old path; its words are what SR3 captures) | b7aa7c739 DLC2 |
| dlc_run_group (ring path), pm_gbi_run fallback per group | 381-430, 506-511 | Group through the GV1 ring or the interpreter (near, lit, ring full) | KEEP. A capture that draws from the ring is refused (plan e: vertex index below GRING_VTX) | 6e2cb90ce DLC1 |
| pm_gpu_grange index copy + C3 cross-list batch (grange_batch_open) | pm_gpu.c 2001-2055 (memcpy 2027, 2051) | Copies the static indices into gidx each draw (5.2 KB per frame); merges draws of adjacent lists | KEEP for the old path. SR3 copies captured indices into the arena and patches the index address (plan a2). Before a run SR4 calls pm_gpu_flush to close the open C3 batch | 588b189a5 |
| dlccheck, dlc_verify_skip, dlc2 line (dlc_report) | 27, 351-378, 534-537; pm_gbi.c 2387 | Check tool and counters for dlc | KEEP (debug tool of the old path) | 6e2cb90ce |
| Header comment "Switch dlc (default off...)" | 1-2 | Stale: dlc_enabled (25) has no dlc switch; dlc is always on | Fix in DLX2 (text only) | - |

## (b) Texture-bind list path (C1)

| Item | File:line | What it does now | SR relation | Added by |
|---|---|---|---|---|
| G_PM_TEX producer MODEL_TEX_OP | src/model.c 183-188, 1568, 1572 | One op per model bind (only map models emit G_PM_TEX) | KEEP: record key word (w0 handle index, w1 list) | 6e50f68a0 C1-1 |
| tb_list[128], pm_texbind_set | pm_gbi.c 2784-2796; call model.c 2160 | Checks a handle list once; the one funnel for a handle (re)build | HOOK SR1: tb_gen[idx]++ here (plan b) | 6e50f68a0 C1-1 |
| tb_run | pm_gbi.c 2801-2817 | Runs the handle list through tex_op | KEEP: record build and miss path. Called from mg_run 3594 and the walker 3712 | 6e50f68a0 C1 |
| tb_inline, G_PM_TEXI | pm_gbi.c 2819-2827, 3707 | Glyph, hud, sprite, shadow texture runs | KEEP (not map models: msg_draw.c, hud_element.c, sprite.c, Shadow.c) | decf43341 GLYPH |
| tb_probe | pm_gbi.c 1637-1642; calls 3623, pm_dlc.inc.c 465 | Under phase, scans each list up to 4096 words for G_PM_TEX and adds to RS_TBG/RS_TBDL | DEAD: pm_rs_cnt is never printed (see h). Distorts phase timing. DEL-PRE (DLX1) | e3b6268d7 C1-1 verify |

## (c) Render-mode list path (C2)

| Item | File:line | What it does now | SR relation | Added by |
|---|---|---|---|---|
| G_PM_RM, rm_apply | pm_gbi.c 2879-2913; walker 3716, mg_run 3596; producers model.c 1681-1982, sprite.c 335, entity_model.c 370 | Applies a render-mode list in place | KEEP permanently: sprites and entities use it (DLF4 640255940, DLF5 4267a3536). For a record model the G_PM_RM w1 is a key word | a6c525673 C2 |

## (d) Model group op (MR1)

| Item | File:line | What it does now | SR relation | Added by |
|---|---|---|---|---|
| Header emit | src/model.c 1512-1516, 2066-2068 | G_PM_MODEL n before the model words | HOOK SR2: w1 = pmSlot, static-matrix bit, pmMtxVer (model.c 1513-1515) | 7d00a451c MR1 |
| G_PM_MODEL case | pm_gbi.c 3697-3706 | sp0 group begin/end, mg_run, push for a non-dlc G_DL | HOOK SR3/SR4: key test, capture (SR3), glue and run jump (SR4) here and in mg_run | 7d00a451c MR1 |
| mg_run | pm_gbi.c 3563-3619 | Runs the model words; returns at once when dd_on, dls_on, gp_on, pm_ph_fine (3564) | KEEP: capture and old path. HOOK SR3: no capture and no run under the same guard (dd_on, dls_on, gp_on, pm_ph_fine) | 7d00a451c MR1 |

## (e) DLF and PDL leftovers

grep for pdl_try, pdl_tpl, pdl0, GbiState, pm_pdl, gbidump, pdlsh, pdlcheck, dlr, pm_pdl_heap_gen: no code hit. PDL compiler,
GbiState and PSO path were deleted in b17262105 (DLF23); gbidump in 37fb8665c (DLF1). Left:

| Item | File:line | SR relation | Added by |
|---|---|---|---|
| RS_PDL, RS_PDLM enum names | 3ds/include/pm_port.h 109 | DEAD (no add site). DEL-PRE (DLX1) | 5541577c1 |
| "PSO" in comments | pm_dlc.inc.c 44, 53; pm_gpu.c 2024 | Text only. Leave | 588b189a5 |

## (f) TBREP, BAT1, FLH, TEXH, LAY2, TST1

| Item | File:line | What it does now | SR relation | Added by |
|---|---|---|---|---|
| TBREP: tex_gen, tl_last, tl_rep, skip in tb_run | pm_gbi.c 173-177, 2802-2806, 2816; tex_gen++ at 252, 1180, 2545, 2659, 2728, 2786, 3060, 3363, 3442 | Skips a G_PM_TEX list equal to the last list run with no texture write since | KEEP. HOOK SR4: a run skips G_PM_TEX and binds textures outside tb_run. Add tl_last = NULL (or tex_gen++) to the run-end dirty list (plan d step 5 does not name it) | 52d082f32 |
| BAT1 sort | src/model.c 4810-4861 (bat1_plain, bat1_key, bat1_sort_runs), call 4935 | Stable sort of each run of plain opaque tasks by texture | KEEP: runs keep game order (plan h5), and BAT1 makes plain records adjacent, so runs are longer. SR3 eligibility may call bat1_plain (one predicate) | c4ebe5451 |
| FLH split | pm_gpu.c 752-852 (flush_fl_dbg, flush_light_tab, pcomp_calc, scissor_off, blend_off, flush_draw_cpu) | Code moves out of flush_draw | KEEP: interpreted closes stay for non-record draws | 589021dd3 |
| TEXH split | pm_gbi.c 2949-3058 (tex_win_s25, tlut_v20, tex_tile1, tex_shaded0), gp_note 20 | Code moves out of tex_op | KEEP (record build and other producers use tex_op) | 08b1848ed |
| LAY2 / CF4 hot list | 3ds/ld/3dsx.ld.in 32-40 | Names dlc_try, tb_run, mg_run, rm_apply, dlc_run_group, dlc_static_group, dlc_box_out.*, dlc_seg | HOOK SR3, SR4: add the capture, key test and glue functions to the list in the same commit. No name is removed | 9bb97f3fb, 595cb8efc |
| TST1 state block | pm_gbi.c 178-194 | Tile, combine, image state in one 32-byte aligned struct in .data.pmhot | KEEP: the record build can copy it in one block | 88d689bcd |
| Dropped rows (DREC1, EPO1, SORT1, MTXM, GEN0, DBGF, HOOK1, MCEN, CF6) | - | No code in the tree (agent_dlm, agent_gen0, agent_cfp, agent_cfd) | Nothing to delete. Note: SR2 load_mtx cache is a new MTXM by version, not by a 16-slot address table | - |

## (g) Profiling switches and per-op hooks

| Item | File:line | What it does now | SR relation | Added by |
|---|---|---|---|---|
| prof (PF1 to PF3) | pm_phase.c 286-430; pm_main.c 491 | One drawn frame in N runs with pm_ph_on; reads pm_gbi_sp0_dlc() = sp0_T[2] (pm_gbi.c 2157) | KEEP. prof wk is the SR decision quantity (plan g). The dlc field falls when records skip dlc_try; no change needed | 6656d5870, 286dcac01, 9491c1222 |
| pmu | pm_phase.c 236-285 | PMU window line, read at window end | KEEP. No per-op cost | f0001e7a4 CF0 |
| phase per-op hooks: os_op_hook, mdc_op, mr0_op, gv/tex_streak_op, sp0_kind | pm_gbi.c 2009, 2217, 2186, 47-66, 2146; walker 3659-3680, mg_run 3570-3571 | Cold call per walker op when pm_ph_on | KEEP. A record model runs only its per-frame words, so opsplit, mdlsum and mr0 counts fall. Not an SR gate; read them as direction only after SR4 | 7bd0205f1, c488b5ec2, 38c25809b, 037dc0317 |
| ms0 (removable closes) | pm_gbi.c 2190-2211; model.c 1498-1510 | Upper bound of closes BAT1 can remove | KEEP (BAT1 census; SR does not change it) | fb9e8f0fc |
| fsplit | pm_phase.c 155-235 | 12 tick reads per drawn frame | KEEP. Frame level, no conflict | 311532db5 |
| dcsum, dc_on sampling | pm_gpu.c 2217-2240, DC_T 731 | Samples one close in 8: words, us per part | KEEP. Record words are outside flush_draw; the SR4 rec line (glkb, fl) counts them | 3dcfc661d DS0 |
| phasegbi (pm_ph_fine), gbiprof (gp_on) | pm_phase.c 77, 432; pm_gbi.c 18, 3564, 3671 | mg_run returns at once; per-op tick reads | HOOK SR3 (same guard as mg_run 3564) | 0ffb72520 |
| kick_check, split_frame | pm_gpu.c 542-552, 282-290 | Up to 2 C3D_FrameSplit per frame at 55 % of last frame draws; sh_clear at each split | HOOK SR4: a run ends before a kick split (plan d step 5, return sizing) | 362746be7 KICK1 |

## (h) Stale or dead switches and tools

| Item | File:line | State | Action | Added by |
|---|---|---|---|---|
| RS_* counters (pm_rs_cnt, RS_ADD, RS_MAX) | pm_port.h 107-124; 23 sites pm_gbi.c, 8 pm_dlc.inc.c, 3 pm_gpu.c, pm_nusys.c 83-85; pm_phase.c 20, 467, 655 | DEAD: the rsplit print was removed in 3141a0913 (LOGCLEAN). Written under phase, never read | DEL-PRE (DLX1): enum, macros, all sites, tb_probe | ae60862f9, 5541577c1 |
| imglog() in pm_gbi.c | 262-272; calls 3066, 3487; unused `on` in grab_cur 253, 260 | DEAD: the body computes `kind` and logs nothing; a cold call per SETTIMG | DEL-PRE (DLX1). The imglog lines in pm_tex.c 217-229, 1483-1487 are live logs: KEEP | 42bc5759a B1 |
| po3a | pm_gbi.c 333-337, 1644-1668, 1807, 1886, 2414, 2929, 3772-3808; guards pm_dlc.inc.c 25, pm_gbi.c 1450, 3782 | STALE: go/no-go census of PO3b (done). Per-triangle test, and a guard in dlc_enabled and the GV1 path | DEL-PRE (DLX2) | 5a98914d5 PO3a |
| dlsrc (dls_*) | pm_gbi.c 1548-1632, 1871, 2388, 3564, 3623-3629, 3664, 3695, 3731, 3745, 3764 | STALE: DLC0 census that led to dlc. Six tests in the walker and one in draw_tri; also a guard in mg_run | DEL-PRE (DLX2). KEEP dls_hash (1579): dlc_try uses it (450, 457). Move it into pm_dlc.inc.c as dlc_hash | cad4e73ba DLC0 |
| mops (R0) | pm_gbi.c 2019-2083, call 3679, report 2390 | STALE: counters for R1 to R3 (done). Under phase it unpacks and multiplies every G_MTX a second time | DEL-PRE (DLX2). Edit scenario mac01 (azt_scenarios.json 413-432, until `mops f`) and test_catalog.md 34 in the same commit | 25d191c70 |
| drawlog/ddonly, xtrace, rmlog, suntile, modellog, gv1check, gbiprof, tx1check, fl1log, texdump, texstat | pm_gbi.c 91-158, 1268, 1433, 116; model.c 1413-1424; pm_tex.c | Live debug tools | KEEP. The record test must honor the dd_on, xt_on, rmlog_on guards (dlc_enabled 25) | various |
| gbidump, pdlsh, pdlcheck, dlr, dls (old) | - | Not in the tree | Nothing | 37fb8665c, b17262105 |

## Order

All rows wait until no other code agent is live (rule 2). PF3 is committed (9491c1222). DLX1 touches pm_phase.c.

1. DLX1 (dead code, no picture change, about 70 lines removed): RS_* enum, macros and every site; tb_probe; RS_PDL/RS_PDLM;
   imglog() in pm_gbi.c with its 2 calls and the unused `on` in grab_cur. Gate: build with no new warning; mac01t_cap1 top
   shots byte-equal to HEAD (shared draw file pm_gbi.c). 1 to 3 starts.
2. DLX2 (stale census tools, about 200 lines removed): po3a (all sites and the 3 guards); dlsrc (all sites, mg_run guard;
   dls_hash moved to dlc_hash); mops (and scenario mac01 until line, catalog row); the stale dlc header comment. Gate: build;
   mac01t_cap1 and yard top shots byte-equal; opsplit and sp0c lines still print. 2 to 4 starts.
3. SR1 to SR5 (plan_stateful_records.md f). Hooks only, no deletion: SR1 tb_gen in pm_texbind_set, gstat_gen in
   pm_gpu_gstat_reset. SR2 G_PM_MODEL header w1. SR3 capture in mg_run with dlc_try box test off, debug guards, hot list.
   SR4 run-end dirty list plus TBREP tl_last reset, pm_gpu_flush before a run, run end before kick splits, hot list.
   SR5 nothing named here. The old path (mg_run, tb_run, rm_apply, dlc_try, grange, C3 merge, TBREP, BAT1) stays.

Left alone (before and after SR): pm_dlc_gen, gstat area, dlc_compile/static_build/batch_compile, the dlc group skip, dlc_try, C3 batch merge, tb_list/tb_run/tb_inline, rm_apply/G_PM_RM, G_PM_MODEL/mg_run, TBREP, BAT1, FLH, TEXH,
LAY2, TST1, prof, pmu, fsplit, dcsum, phase census lines, live debug tools.
