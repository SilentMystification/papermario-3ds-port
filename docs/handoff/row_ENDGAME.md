# Endgame plan: remove the display lists completely (closed plan, version 3, ORACLE8 re-cut of v2, 2026-11-12)

The oracle writes only row_*.md files. This file is the full new text of plan_endgame.md. The orchestrator replaces plan_endgame.md
with it (`git mv -f docs/handoff/row_ENDGAME.md docs/handoff/plan_endgame.md`). Every row follows from the tables below. A row that
moves no counter of section 3 and builds no part of the test harness of section 5 is not a row.

## 0. Rules of this plan
- Done = both proofs pass (section 4): the source counters are all 0 AND the ELF check of the final pm_3ds.elf finds no list
  symbol and no list data. Source-present code that the linker drops counts as gone only when the ELF check shows it gone.
  Nobody reports the epic done before both pass. Every stage report gives the remaining counts (LISTCEN2 output), not a verdict.
- Every compare is exact: one differing bit fails (user, 2026-11-12). No screenshot is stored or compared (user, 2026-11-12).
- Test harness first (user, 2026-11-12): the host harness and the golden captures of the current code come before the conversion
  waves. Nobody edits a golden file after its capture row merges.
- Freeze: nothing new launches until the user accepts this plan. On acceptance the orchestrator deletes, in one commit, the row
  files that section 6 does not name: S7TEX2, S7MDL, S7ENT1, S7ENTX, S7ENT3, S7FLAME, S7EFX7/8 if not started, the old S5*, S6M*,
  S1PREV, S2W, S5PQUAD2 rows, and the v2 design rows NPSO, CONV2, NXF, FRAMEHASH, S9DEL1, S9DEL2, S9DEL3, S9DEL4 (their scope is
  in the v3 rows below). FPS60FL and FPS60VAL are bug rows outside this plan and stay.
- User code rules: no stubs, no stand-ins, no toggles, no feature switches, no culling. Test inputs (fixture files of fixed values
  under 3ds/tests) and debug check switches (psochk, fhash) are tests, not product code. They read. They never choose a draw path.
- Discovery rule: an item found later goes into section 7 with the row that covers it. Section 8's totals are recomputed.
  The plan is not redesigned.
- Model rule (user, 2026-11-12): most rows, if not all, are Haiku-sized. Haiku is the default. A row is cut until Haiku can do it:
  one file, one function family or one call-site group, the exact edit, one gate. A Sonnet or Opus row stays only for a design core
  (a new record form, a compiler, a converter), with a one-line reason. The mechanical call-site moves after a core are Haiku rows.
  The oracle writes each Haiku row from a code read of main at the time it writes the row, after the core of that row merged.
  The exact edit is in the row. A Haiku failure that the row text caused is a row fault: the oracle corrects the row, Haiku stays.

## 1. Census from the code (main 235bb8721, scratch census: TARGET_3DS-active lines vs N64-only `#else`/`#ifndef` lines)
listcen.py totals: build 4468, gfxpos 3967, data 22134, data_arrays 1873, gfxtype 3469, reader 714, model 15.
Group rows are sums of the per-file census (within a few counts. LISTCEN2 prints the exact values). Active = compiled for the 3DS.
n64 = inside `#ifndef TARGET_3DS` or the `#else` of `#ifdef TARGET_3DS`.
| group | files | build active | build n64 | gfxpos active | gfxpos n64 | gs data | Gfx arrays |
|---|---|---|---|---|---|---|---|
| src/effects (127 effect files) | 127 | 1388 | 379 | 1365 | 438 | 2 | 7 |
| src/effects/gfx + assets/effects (static effect lists) | 738 | 0 | 0 | 0 | 0 | 6822 | 739 |
| src/world (45 files with calls) + assets/world | 116 | 383 | 42 | 379 | 47 | 3738 | 253 |
| src/entity (17 files with calls, model data 72 files) | 93 | 148 | 29 | 141 | 29 | 8112 | 751 |
| src/model.c, entity_model.c, animator.c | 3 | 155 | 49 | 100 | 35 | 541 | 83 |
| HUD and text: hud_element.c, msg_draw.c, msg.c, draw_box.c, windows.c | 5 | 402 | 15 | 388 | 19 | 60 | 11 |
| item_entity.c, imgfx.c, sprite.c, sprite_shading.c | 4 | 282 | 15 | 284 | 15 | 108 | 24 |
| overlays: screen_overlays.c, screen_render_util.c, curtains.c | 3 | 133 | 22 | 133 | 22 | 249 | 23 |
| backgrounds: background_gfx.c, background.c | 2 | 88 | 7 | 89 | 7 | 9 | 2 |
| title and logos: state_title_screen.c, state_logos.c, draw_img_util.c, titlemenu.c | 4 | 80 | 13 | 80 | 13 | 42 | 11 |
| src/battle (9 files with calls) + assets/battle, battle_ui_gfx.c | 27 | 44 | 36 | 39 | 41 | 577 | 44 |
| src/pause (7) | 8 | 72 | 0 | 72 | 0 | 293 | 15 |
| src/filemenu (4) | 6 | 65 | 0 | 69 | 0 | 99 | 5 |
| small root files: effect_utils, main, 43F0, cam_main, status_icons, 8a860_len_3f30, status_star_shimmer, src/evt, effects.c, entity.c, inspect_icon, i_spy, pulse_stone, speech_bubble, inventory, main_loop, src/os | 19 | 116 | 41 | 112 | 49 | 134 | 10 |
| star points and level-up letters (starpoint*.c, level_up*.c, assets) | 13 | 0 | 0 | 0 | 0 | 951 | 107 |
| include (gbi.h 382, gbi_custom.h 61, gs2dex.h 18, gt.h 3: the macro definitions) | 15 | 464 | 0 | 2 | 0 | 384 | 0 |
| TOTAL | | 3820 | 648 | 3254 | 715 | 22103 | 2107 |
Readers and the list state model in 3ds/src (reader-class hits / lines): nd_op.h 173/153, nd_rec.c 80/586, nd_walk.c 78/705,
nd_op.c 62/75, nd_scomp.c 49/354, nd_recs.c 48/333, nd_cen.c 42/601, nd_set.c 39/247, nd_model.c 33/678, nd_dlc.c 27/493,
nd_ent.c 21/447, nd_shape.c 12/953, nd_direct.c 11/370 (fallbacks), nd_gbi.h 9/231, nd_seam.c 9/428, nd_tex_set.c 7/725,
nd_gtab.c 6/87, nd_mtx_set.c 3/304, nd_cmd.h, nd.h, nd_api.h, pm_phase.c 1 each. Model-only files: nd_comb.c, nd_tex_set.c,
nd_texwin.c, nd_rect.c, nd_target.c (cimg roles), the gv walker part of nd_geom.c, pm_comb.c decide inputs. Generated:
nd_gfx_tab.c (612 Gfx uses: S7 v1 tables point into list units).
Call sites per function in the large and medium files (ORACLE8 scan, main 3c7d977ab, active and n64 together): the row tables of
section 6 name the functions. Examples: hud_rect_emit 43, appendGfx_message 40, imgfx_appendGfx_mesh 56, draw_box 79.

## 2. End-state architecture (the one replacement form per category)
| category | today | replacement (end form) | design row |
|---|---|---|---|
| pipeline state (combine, other mode, geometry, filter) | gDP/gSP state calls, typed units, run-time nd_comb decide | a static `NdPsoDesc` at the call site. gen_pso_tab (host) decides every description once at build time into `nd_pso_tab[]` (TEV and blend blocks). The device looks up by id. No decide code on the device | NPSO2 (Sonnet core), NPSO1, NPSO3-5 |
| texture | TIMG/TILE/LOAD*/TILESIZE, TMEM model | `NdBindDesc` from native fields at the call site (raster, palette, format, size, window) or from the converter. nd_tex_bind_desc | NPSO1 (desc form), CONV2C (static) |
| transform | gSPMatrix/PopMatrix list stack | native transform calls nd_xf_push(mtx, flags) / nd_xf_pop recorded in the native stream | NXF1 |
| geometry, built at run time | VTX/TRI into the stream | nd_mesh(pso, tex, vtx, n, idx, ni, prim, env) and the existing nd_rect, nd_quad, nd_glyphs, nd_parts | NXF2 |
| render target (colour and depth image) | gDPSetColorImage/DepthImage, nd_target.c role from walker state | native target records nd_target_begin(role, slot) / nd_target_end on the pm_gpu off-screen slots | NTGT (Sonnet core, section 7) |
| static lists (src and assets Gfx arrays) | S7 v1 tables pointing into list units, walker for units | CONV2 native tables: per (list, entry pso) pair a sequence of native ops (pso id, bind desc, transform op, vertex range, index range). No pointer into a Gfx array. The Gfx arrays become host-only inputs | CONV2C (Sonnet core), CONV2A-B, D-H |
| map shapes | .dlc v2 + nd_shape list parse | .dlc v3 from CONV2's draw form. nd_shape reads no list word | CONV2C (+ GSET-MAP) |
| model records | R0 render-mode lists, R13 aux ops, rec_apply list runs | records carry pso ids and bind descs from CONV2 (render modes are static lists) | NMDL0 (Sonnet core) |
| the stream | NDC_STATE typed units, NDC_NAPI | NDC_NAPI native records only (the core 1 to core 0 pipe and the draw order) | S9DEL3A-D |
Proof while the old model still exists: debug switch `psochk` (NPSO4) compares, at every native draw, the stated pso and bind with
what the walker state gives. `psochk bad 0` in the gate scenes proves that no inherited state is missing.

## 3. Counters (LISTCEN2 prints each. The gate of every row names which it moves and to what)
C1 build_active, C2 gfxpos_active, C3 build_n64, C4 gfxpos_n64, C5 data (gs macros), C6 data_arrays, C7 gfxtype, C8 reader (per
3ds/src module), C9 model, C10 nd_rec_dl + nd_scomp_rec uses (S6/S7 forms are list forms too), C11 elf_func (list symbols in
pm_3ds.elf), C12 elf_data (Gfx array symbols and v1 table symbols in pm_3ds.elf). Final: every counter 0.

## 4. End proof (both parts. Script rows LISTCEN2 and ELFGATE)
- Source: LISTCEN2 all classes 0, and `grep -rnE "\bg[DS]P[A-Za-z]|\bgs[DS]P[A-Za-z]|\bGfx\b|gMainGfxPos|NDO_|nd_ops_run|nd_walk_state|
  NDC_STATE|nd_rec_state|ENT_GBI" src include 3ds/src 3ds/include` returns nothing (3ds/tools keeps the host converter inputs).
- ELF (3ds/tools/elfgate.py, python ELF .symtab parse, no dependency): function symbols matching
  `^(nd_walk_state|nd_ops_run|op_typed|op_geo|walk_call|nd_op_apply|nd_op_same|nd_op_xlate|nd_rec_state|sync_combine|nd_comb_decide|
  decide|decide_miss|sync_render|set_tilesize|tex_win[01]|ensure_tile|ensure_tile_slow|nd_texrect|nd_fillrect|gv_tri|op_setcombine|
  load_mtx|ent_flat|ent_run|ent_compile|dlc_try|dlc_build|nd_gtab_run|gt_ops|nd_scomp_exec.*|nd_rec_dl|nd_set_(timg|tile|tilesize|
  loadblock|loadtile|loadtlut|texture)|gbi_.*|nd_lstcen|img_role|off_screen_check)$` (and GCC clones `.constprop`, `.isra`,
  `.part`): 0. Data symbols `^ndg[odx]?_` (S7 v1 tables), `^gMainGfxPos$`, and every name of the Gfx-array list that elfgate.py
  builds from the source and asset scan (the gen_gfx_syms.py scan over all of src and assets/us): 0. No DisplayContext member of
  type Gfx. Prints `elfgate func <n> data <m> gfxnames <k>`.

## 5. Gates (row_TESTIDEAS.md G1-G12, adopted by the user 2026-11-12)
| gate | where | exact rule |
|---|---|---|
| G8 compile ban | every converted file includes 3ds/include/nd_nolist.h (poisons gDP*/gSP*/gs*/Gfx/gMainGfxPos). Native draws take a static ND_PSO_DESC | the build fails on one leftover |
| Golden master + gcov | small and medium groups (about 2460 of 3356 call sites, 73 %): the converted draw functions run on the host with the fixed test inputs of their GOLD row and print the same draw records as the golden file captured from the old code. gcov shows 100 % of the converted functions' lines run | text equal byte for byte, coverage 100 % |
| G1 host differential | every static list (CONV2 and GSET rows): old model vs native table, per draw | field equal |
| psochk + frame hash | large groups (about 900 sites, 27 %: model, HUD, messages, imgfx, sprites, item entities, target switches) in scenes that COVMAP shows run at least 90 % of the row's sites. Phase ends | psochk bad 0, every frame hash equal |
| G6 PICA validator | on the frame-hash capture of each phase end and each large-group row | 0 findings |
| G11 console soak, G10 dump pull | each phase end (one push, user action) | crash.txt empty, no hang dump |
| G9 end proof | final | LISTCEN2 all 0, elfgate func 0 data 0 gfxnames 0 |
| N64DEL gate | N64DEL1-3 | `gcc -E -P` with the device defines gives identical output before and after, per file |
| deletion gate | S9DEL rows | full build, nd_host PASS, the row's grep terms return nothing, the frame-hash set equal, G6 0 findings |
Today's `bench.py base` + `bench.py cmp` on ppm shots is no longer a gate anywhere after FRAMEHASH5 merges.
Rule text for CLAUDE.md rule 2 (the user confirms): "2. NO VISUAL CHECKS AND NO SHOTS. No agent looks at a shot, describes a
picture, counts pixels or compares images. Proof is the code read and exact compares: nd_host, the golden draw records of the
converted functions (3ds/tests/golden, gcov 100 % of their lines), and for large inputs and phase ends the frame hash (`bench.py
base <scene>_hash`, then `bench.py cmp <scene>_hash --row <row>`) with psochk. One differing bit fails. The user checks the look
on the console." (Rule 4's exact-compare text is already in, b9aff62a6.)

## 6. Row families, order and blockers
Row text: the oracle writes each row (60 lines or fewer for Haiku) when its blockers merged, from a code read of main at that time.
A Haiku row in these tables gets the exact edit. "after X" = the row branches from main after X merged.
Phase H (harness and scripts):
| row | model | blocker | edits (file: places) and what it builds |
|---|---|---|---|
| LISTCEN2 | Haiku | none | 3ds/tools/listcen.py: census v2, counters C1-C10 (row_LISTCEN2.md) |
| ELFGATE | Haiku | none | new elfgate.py, elfgate_patterns.txt: C11-C12 (row_ELFGATE.md) |
| NOLIST | Haiku | none | new 3ds/include/nd_nolist.h and a build check that a poisoned name fails (G8) |
| HOSTCORE1 | Haiku | none | new 3ds/include/nd_drec.h, 3ds/tools/nd_drec.c: the draw record format (fields given by the oracle: pso output, bind content, transform rows, vertex and index bytes) as text, a writer, a host unit test with a seeded fault |
| HOSTCORE2 | Sonnet | HOSTCORE1 | nd_host_test.sh link set: the core 0 native emit on the host down to the GPUCMD boundary, the record hook at the emit boundary. Reason: the link closure of about 10 shared emit files and the host side of the citro3d calls appear only while it links, so no exact step list exists before |
| HOSTCORE3 | Haiku | HOSTCORE2 | nd_host runner entry: one record file per test program, a seeded-fault self-test |
| GMHARN | Haiku | HOSTCORE3 | one host test program per game file (the file, the real recorder, gu math, HOSTCORE, its test-input file), text output, a runner in nd_host |
| GMCOV | Haiku | GMHARN | gcov of the host runs, per function line coverage |
| FRAMEHASH1 | Haiku | none | new 3ds/src/nd_fhash.c (pure C, device and host): PICA header decode, FNV-1a of (register, mask, value), address registers masked (register table given by the oracle), host test with a seeded fault |
| FRAMEHASH2 | Haiku | FRAMEHASH1 | nd_fhash.c: the content feed (vertex range bytes per draw, index bytes, level 0 texels, target role, GX entry kind and size, formulas given by the oracle), host test |
| FRAMEHASH3 | Haiku | FRAMEHASH2, FPS60FL | pm_gpu.c: the fhash switch, `|| fh_on` on the kc_rec line (begin_wait), the fh_frame call at the frame end, the fhash entry (pm_log_evt) |
| FRAMEHASH4 | Haiku | none | bench.py: kind azahar_hash in base, cmp and base --fill (fhash.txt, no image) |
| FRAMEHASH5 | Haiku | FRAMEHASH3, FRAMEHASH4 | per-row azt.d and bench.d files for 6 hash twins, the determinism gate (base twice equal, head vs merge-base 6/6 equal) |
| PICAVAL1 | Haiku | FRAMEHASH3 | pm_gpu.c fh_frame: switch fhdump writes the words of one frame and its GX entries to the SD card |
| PICAVAL2 | Haiku | PICAVAL1 | new 3ds/tools/picaval.c rules R1-R5 (rules given by the oracle, among them: every list with a draw ends with the framebuffer flush, the FPS60FL cause), one seeded-fault fixture per rule |
| PICAVAL3 | Haiku | PICAVAL2 | picaval.c rules R6-R10 (attribute count against shader input count, draw mode around config writes, index and attribute ranges inside linear memory or VRAM, list end, GX entry targets), fixtures |
| PICAVAL4 | Haiku | PICAVAL3, FRAMEHASH5 | bench.py: a hash bench with `fhdump` runs picaval on the dump and prints `picaval findings <n>` |
| COVMAP | Haiku | FRAMEHASH5 | G7 for the large-group files only: per-site counters in the scenes |
| N64DEL1, N64DEL2, N64DEL3 | Haiku | LISTCEN2 (N64DEL2/3 also N64DEL1's tool) | C3, C4 to 0 |
Phase G (golden capture from the current code. Haiku. Blocker GMCOV. Each row writes test inputs until gcov shows 100 % of its
draw functions' lines, captures the text twice, commits it under 3ds/tests/golden/<group>/): GOLD-FX1..GOLD-FX16 (src/effects,
8 files each), GOLD-ENT1..2 (src/entity setupGfx, 17 files), GOLD-WLD1..5 (src/world, 45 files), GOLD-OVL, GOLD-BG, GOLD-TTL,
GOLD-BOX, GOLD-MISC1..2, GOLD-PAUSE, GOLD-FMENU, GOLD-BTL. 32 rows. They run in parallel with phase B and use idle slots.
Phase B (design cores and their mechanical parts):
| row | model | blocker | edits and what it builds |
|---|---|---|---|
| NPSO1 | Haiku | none | 3ds/include/nd_api.h: NdPsoDesc (the field list from the oracle's read of every input of nd_comb_decide, nd_render_state, nd_tev_build), NdPsoOut, ND_PSO_DESC, nd_pso() declaration |
| NPSO2 | Sonnet | NPSO1 | new 3ds/src/nd_decide.c: the pure decision split out of nd_comb.c and nd_tmpl.c as nd_pso_build(desc, out). The device keeps calling the same code. Reason: a code move across 3 shared files that cuts the walker reads into desc fields while the device output stays byte-equal, and the order of the cut is found while moving |
| NPSO3 | Haiku | NPSO2 | new 3ds/tools/gen_pso_tab.sh (copy of the gen_gfx_tab.sh pattern), one build.sh generator step, nd_pso() lookup, nd_host compare `psotab entries <n> bad 0`, delete the nd_comb_decide test shim (nd_host_test.sh:57) |
| NPSO4 | Haiku | NPSO2 | nd_direct.c: the psochk hook (the walker desc of the moment, field sources given by the oracle), the psochk entry (pm_log_evt) |
| NPSO5 | Haiku | NPSO3, NPSO4, GOLD-TTL | pilot src/draw_img_util.c: 26 sites to table pso ids, NdBindDesc and rectangles, nd_nolist.h. Gate golden + gcov |
| NXF1 | Haiku | NPSO3 | nd_api.h and new 3ds/src/nd_xf.c: nd_xf_push / nd_xf_pop records. The core 0 runner calls load_mtx and gbi_popmtx with the same arguments (record pattern of nd_rect, named by the oracle) |
| NXF2 | Haiku | NXF1 | nd_xf.c: nd_mesh record, runner through the existing native indexed emit (function named by the oracle) |
| NXF3, NXF4 | Haiku | NXF2, their GOLD-FX row | pilots src/effects/ring_blast.c (NXF3), src/effects/stat_change.c (NXF4). Gate golden + gcov |
| NTGT | Sonnet | NPSO3 | native target records nd_target_begin / nd_target_end on the pm_gpu off-screen slots, the role table that nd_target.c img_role decides today. Reason: a new record form that replaces a walker-state decision shared by about 18 files |
| CONV2A | Haiku | NPSO1 | 3ds/include/nd_gtab.h: v2 types NdNOp (NDN_PSO, NDN_TEX, NDN_XF, NDN_COL, NDN_DRAW, NDN_CALL) next to v1, text given by the oracle |
| CONV2B | Haiku | none | new 3ds/tools/nd_vslot.c: the N64 vertex slot resolver (VTX loads into 32 slots, TRI names slots, output absolute index triples), host unit test with fixtures |
| CONV2C | Sonnet | CONV2A, CONV2B, NPSO2 | 3ds/tools/nd_gfx_conv.c v2 output: (list, entry) pairs from the nd_draw_list call scan and the static call graph, state units to pso via nd_pso_build, generator errors that name the call site. Reason: one algorithm over every static list, and its error cases are not known per list before it runs |
| CONV2D | Haiku | CONV2A | new 3ds/src/nd_ntab.c: nd_ntab_run, one switch from NdNOp to the native emit calls (mapping given by the oracle) |
| CONV2E | Haiku | CONV2C | 3ds/tools/nd_gtab_test.c v2: per draw compare of the v1 host model against the v2 table, `ntab tables <n> draws <d> bad <b>` |
| CONV2F | Haiku | CONV2C, CONV2D | nd_rec.c nd_draw_list recorder, nd_gtab_find v2 key (list, entry), nd_part_draw on v2 |
| CONV2G, CONV2H | Haiku | CONV2E, CONV2F, their GOLD-FX rows | pilot effect files: sparkles, effect_3D, snowfall (G), snaking_static, water_block, embers (H). Gate G1 + golden |
Phase C (Haiku rows per file or function family. The oracle writes them after NPSO5, NXF4, NTGT and CONV2H merge. Small groups
need their GOLD row: gate golden + gcov + G8. Large groups need FRAMEHASH5, COVMAP and PICAVAL4: gate psochk + frame hash + G6):
| row | file: functions (call sites) |
|---|---|
| NFX0A, NFX0B | the first two effect files of GOLD-FX1 that call static lists (one file each) |
| NENT1..NENT4 | src/entity setupGfx functions, 4 to 5 files each (Block, SaveBlock, SuperBlock, HeartBlock, ItemBlock, ShatteringBlock_common, WoodenCrate, BlueWarpPipe, Chest, HiddenPanel and the rest) |
| NWLD0A, NWLD0B | src/world/area_mac/mac_01/crystal_ball_gfx.c (44), the next world file of GOLD-WLD1 |
| NOVL1..NOVL4 | screen_overlays.c _render_transition_stencil (72), screen_render_util.c appendGfx_darkness_stencil (49), screen_render_util.c appendGfx_draw_prev_frame_buffer + appendGfx_screen_transition_stencil (21), curtains.c render_curtains (13) |
| NBG1..NBG3 | background_gfx.c gfx_draw_background (56), background_gfx.c func_80027BAC + gfx_init_state (14), background.c appendGfx_background_texture + bg_emit (25) |
| NBOX1..NBOX3 | draw_box.c draw_box first half, draw_box second half (79 together, NBOX2 after NBOX1: same place), windows.c render_window_root (9) |
| NPAUSE1..NPAUSE3 | pause_badges + pause_items, pause_main + pause_map + pause_spirits, pause_partners + pause_gfx pause_draw_menu_label |
| NFMENU1, NFMENU2 | filemenu_common.c, filemenu_msg.c + filemenu_createfile.c + filemenu_selectlanguage.c |
| NBTL1..NBTL3 | 16C8E0.c tattle_cam_pre_render (53), 16C8E0.c blend_tattle_background_channel (28), 190B20.c + btl_states_menus.c + popup_messages.c |
| NMDL0 | Sonnet. nd_model.c and the model.c record producers: R0 render-mode lists and R13 aux ops become pso ids and bind descs in the model record. Reason: a record format change with readers in nd_model.c and writers in model.c, entity_model.c, animator.c |
| NMDL1..NMDL8 (after NMDL0) | model.c appendGfx_model (37), make_texture_gfx (30), is_model_center_visible + is_point_visible (40, after NTGT), render_transform_group + _node + nd_range_repeat (7), build_custom_gfx + parade_build_copies + load_texture_* (9), entity_model.c appendGfx_entity_model (32), entity_model.c draw_entity_model_E (21), animator.c (22) |
| NHUD1..NHUD4 | hud_element.c hud_rect_emit + hud_rect_run (55), immediately_render_complex_hud_element + render_complex_hud_element (48), render_transformed_hud_elements (16), draw_hud_element_internal + pm_anchor_* + render_hud_elements_* (22) |
| NMSG1..NMSG7 | msg_draw.c appendGfx_message (40), msg_draw_frame (33), msg_draw_speech_bubble + msg_draw_speech_arrow (37), msg_shadow_flush + msg_glyph_run + msg_draw_char (31), appendGfx_msg_prim_rect + msg_reset_gfx_state (13), msg.c draw_number + draw_digit (15), msg.c render_messages + msg_update_rewind_arrow + msg_draw_choice_pointer + pm_msg_active (27) |
| NIMG1..NIMG4 | imgfx.c imgfx_appendGfx_mesh first half, second half (56 together, NIMG2 after NIMG1), imgfx_appendGfx_mesh_basic (32), _strip + _grid + _anim (30) |
| NSPR1..NSPR3 | sprite.c spr_flat_gfx (26), spr_appendGfx_component_flat + spr_appendGfx_component (22), sprite_shading.c (23) |
| NITEM1..NITEM3 | item_entity.c appendGfx_item_entity (50), render_item_entities (49), draw_coin_sparkles (16) |
Phase D (Haiku waves after their pilot. Gate golden + gcov + G8): NFX1..NFX15 (8 to 9 effect files each, one idiom per row. The
oracle splits a row when its files use more than one idiom), NWLD1..NWLD5, NTITLE, NMISC1..2 (23 rows). GSET1..GSET6 (CONV2 sets:
effects, entity, world, model render modes, star points and level-up, battle and UI arrays. G1 gate. They move C5, C6 and C12).
29 rows.
Phase E (deletes, Haiku, one file or one symbol family per row. A stage starts when C1, C2 and C10 are 0 for the code it deletes.
In a stage the caller rows run first, in parallel on different files, then the definition row. The oracle lists every caller by
grep in each row. A caller with a list path left is a fault of an earlier row: stop and hand back):
| stage | rows | files |
|---|---|---|
| 1 runners | S9DEL1A..S9DEL1D | caller arms in nd_rec.c, nd_recs.c, nd_model.c, nd_shape.c, pm_phase.c (A), nd_ent.c ENT_GBI (B), nd_dlc.c and its CMake line (C), nd_walk.c nd_walk_state, nd_range_run, walk_call (D) |
| 2 state model | S9DEL2A..S9DEL2G | nd_comb.c device decide (A), nd_set.c sync_render (B), nd_tex_set.c tile model (C), nd_rect.c (D), nd_geom.c gv walker part (E), nd_mtx_set.c list stack and the NdWs fields only these read (F), sprite.c spr_state_run and the dead SET_VIEWPORT arms (G) |
| 5 static list parse | S9DEL5A..S9DEL5C | nd_shape.c list parse (A), nd_gtab.h v1 types and gt_ops (B), gen_gfx_tab.sh v1 output (C) |
| 3 typed stream | S9DEL3A..S9DEL3D | nd_rec.c NDC_STATE, st_put, nd_rec_state (A), nd_op.h and nd_op.c (B), nd_gbi.h (C), nd_walk.c G_* and NDO_ arms (D) |
| 4 Gfx type | S9DEL4A..S9DEL4D | display context Gfx members and their users (A), gMainGfxPos and nd_rec_task's list role (B), nuGfxTaskStart list parameters (C), gbi.h, gbi_custom.h, gs2dex.h, gt.h includes and draw_img_util.c draw_tiled_image + draw_adjustable_tiled_image (D) |
Before S9DEL1A the orchestrator tags main `pre-list-removal`. Then S9GATE (Haiku): G9, the frame-hash set, the console batch (G10, G11).
Phase ends (after B, C, D, E): one frame-hash set (spot_mac01_hash, mdl5_iwa10_hash, spot_mac00_hash plus the group scenes), G6
on its capture, one console soak push (user).
First rows to start now (disjoint files): LISTCEN2, ELFGATE, NOLIST, HOSTCORE1, FRAMEHASH1, FRAMEHASH4, NPSO1, CONV2B.
Next when they merge: HOSTCORE2 (after HOSTCORE1), FRAMEHASH2, NPSO2 and CONV2A (after NPSO1), N64DEL1 (after LISTCEN2).

## 7. What the census cannot see, and how it is covered (discoveries are appended here)
| item | covered by |
|---|---|
| lists built at run time into heap or local buffers (effects writing strips per frame, imgfx meshes, sprite quadBuffers, model vertex copies) | C1 counts every gDP/gSP call whatever the packet pointer. The family row of the file converts it to nd_mesh |
| map shape lists inside the ROM image (resident blob, not the ELF) | CONV2C + GSET-MAP make every map's .dlc v3. S9DEL5A deletes nd_shape's list parse, so no list word of the ROM is read. The blob itself leaves with ASSETFREE (parked) |
| battle stage shapes and other ROM-loaded shapes | as map shapes (GSET-MAP takes every *_shape the game loads) |
| segment-addressed lists (0x09 effect segment, entity vertex segments) | CONV2C maps by symbol. A run-time segment base becomes a native vertex base in the draw |
| lists named through data tables (preset->dlist, model texture lists) | CONV2 tables keyed by list address (nd_gtab_find v2). The call site keeps the pointer |
| version-conditioned data (#if VERSION_PAL) | the GSET scan reads only the active branch (S7ENTX form, moved into GSET2) |
| S6 compiled defs in game code (static const Gfx X_u[] for nd_scomp_rec) | counted in C5, C6, C10. Their call sites move to NPSO descs in the family rows |
Discoveries:
| item | covered by |
|---|---|
| colour and depth image switches (gDPSetColorImage, gDPSetDepthImage in about 18 files: background_gfx.c 7, model.c 4, screen_render_util.c 4, crystal_ball_gfx.c 4, 16C8E0.c 3, cam_main.c 3 and others). v2 section 2 had no end form for them. nd_target.c decides the role from walker state (nd_ws.cimg, :32-33) | NTGT (Sonnet core). The sites move in their family rows |

## 8. Totals and wall time (recomputed for v3)
| model | phase H | G | B | C | D | E | total |
|---|---|---|---|---|---|---|---|
| Haiku | 20 (LISTCEN2, ELFGATE, NOLIST, HOSTCORE1, HOSTCORE3, GMHARN, GMCOV, FRAMEHASH1-5, PICAVAL1-4, COVMAP, N64DEL1-3) | 32 | 15 (NPSO1, NPSO3-5, NXF1-4, CONV2A, CONV2B, CONV2D-H) | 55 | 29 | 23 (22 S9DEL rows, S9GATE) | 174 |
| Sonnet | 1 (HOSTCORE2) | 0 | 3 (NPSO2, CONV2C, NTGT) | 1 (NMDL0) | 0 | 0 | 5 |
| Opus | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
Total 179 rows (Haiku 174, Sonnet 5, Opus 0). v2 had 100 rows (Haiku 71, Sonnet 27, Opus 2). The earlier Opus approval of NPSO and
CONV2 is not used: their design cores are NPSO2 and CONV2C (Sonnet), and the oracle fixes the forms in NPSO1 and CONV2A.
With about 20 % retries: about 215 row runs. Rate: 79 merges in 14 h this session (5.6 per hour, mostly small rows). Each row has
a fixed cost (worktree, lane build, merge), so 6 coders give about 4.5 rows per hour.
Critical path: HOSTCORE1 (0.5 h) -> HOSTCORE2 (about 4 h) -> HOSTCORE3, GMHARN, GMCOV (about 2.5 h) -> the GOLD rows of the pilot
groups (about 2 h). In parallel: NPSO1 -> NPSO2 (about 3 h) -> CONV2C (about 4 h) -> CONV2D-H (about 2 h), and FRAMEHASH1-5,
PICAVAL1-4, COVMAP (about 7 h). Phase B ends at about 10 h. Phases C and D: 84 rows at about 4.5 per hour, about 19 h (the large
groups of C wait for COVMAP and PICAVAL4, which are ready before C starts). Phase E: 5 stages, about 1.5 h each with parallel rows,
about 8 h. S9GATE and the console batch: about 2 h. Ideal about 39 h of continuous running. Honest estimate with retries and tool
faults: 50 to 65 h, that is 3 to 3.5 days. v2 said 45 to 60 h. More rows add a fixed cost per row. Haiku rows cost fewer tokens.
The golden rows remove most Azahar runs from the waves (seconds per rerun instead of 20 to 40 minutes per try).

## 9. Earlier stages that were not tied to the end state (so they did not move these counters)
S6T/S6E/S6M (compiled ranges: still typed units and run-time decide), S7CONV/S7RUN/S7EFX1-6/S7MAP (tables that point into list
units. S7EFX swapped only gSPDisplayList: C1 in src/effects is still 1388), NAPI2B/S1/S5 direct executors (each kept the old ops
as a fallback), S7TEX2 and S7MDL as written (walker state kept). From now on a row's gate is its counters going to 0 for its
files plus the gates of section 5.
