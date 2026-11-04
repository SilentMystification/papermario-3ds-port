# plan_native_slices: slice table and code-agent rows for the native draw system

Design: docs/handoff/plan_native_system.md (PNS). Section numbers below refer to PNS. Tags as in PNS: [C file:line] read in source at nds1 HEAD c4740fdce, [U] unverified.
Line anchors are at nds1 HEAD c4740fdce. A row re-reads its anchors with grep before the first edit. If an anchor moved more than 40 lines or the function changed, the row stops and the planner re-anchors it.

Row format: id | full name | files | numbered edit items (each 30 changed lines or less) | host test (written first, fails first) | gate | scenarios.
Scenario names: (E) exists in 3ds/tools/azt_scenarios.json [C]. (N) new, added in the named row with its catalog line (cap 45 s or less, until-line printed by the build).
Every row: PM_ROW=<row> in the shell, justification line in 3ds/logs/perf/status_<row>.txt before each Azahar start, commit of the agent's files only (never 3ds/src/map_restore_table.c), no attribution.

## 1. Slice table

| Slice | Full name | Rows | Owner files (only this slice edits them while open) | Deleted | Gate | Expected ms (phase on, Toad Town) |
|---|---|---|---|---|---|---|
| S0 | NDTEST test harness and measurement | S0-1 to S0-7 | 3ds/src/pm_gpu.c (cold hook only), 3ds/src/pm_phase.c, 3ds/tools/nd_*, 3ds/tests/ndref/, azt_scenarios.json, test_catalog.md | nothing | tools run. Host tests compile and fail for the missing nd code. ndref committed | 0 |
| S1 | NDCORE native core and bridge | S1-1 to S1-6 | 3ds/include/nd.h, 3ds/src/nd_core.c, nd_tmpl.c, nd_tex.c, nd_mtx.c, 3ds/src/pm_nusys.c, 3ds/src/pm_native.c, 3ds/include/pm_native.h, 3ds/src/pm_gbi.c, 3ds/src/pm_dlc.inc.c, 3ds/CMakeLists.txt | learn-and-replay, nat_tmpl_strip | G1, G2 (all LEGACY), G5 | 0 |
| S2 | NDFRAME frame state and background | S2-1 to S2-3 | 3ds/include/nd_gbi.h, 3ds/CMakeLists.txt, src/main_loop.c, src/cam_main.c, src/background_gfx.c, src/background.c (force-include only) | render_frame frame-start words | G1 to G4, G10 | 0 to 0.3 [U] |
| S3 | NDMODEL map models | S3-1 to S3-5 | src/model.c, 3ds/src/pm_map.c, 3ds/src/nd_dlc.c, 3ds/src/pm_dlc.inc.c, 3ds/src/pm_gbi.c, include/gbi_custom.h, nd_core.c | G_PM_MODEL, G_PM_TEX, G_PM_RM, mg_run, rec_*, pm_dlc.inc.c | G1 to G10 | 7.9 to 12.7 (ceiling 20.3) |
| S4 | NDSPRITE sprites, NPCs, player | S4-1 to S4-3 | src/sprite.c, src/imgfx.c, src/sprite_shading.c, src/npc.c, src/77480.c | G_PM_TEXI in sprite.c | G1 to G10 | part of 3.2 to 5.4 |
| S5 | NDHUD HUD, text, boxes, windows | S5-1 to S5-4 | src/hud_element.c, src/msg_draw.c, src/msg.c, src/draw_box.c, src/windows.c, src/draw_img_util.c, src/status_icons.c, src/status_star_shimmer.c, src/inventory.c | G_PM_TEXI in hud_element.c and msg_draw.c | G1 to G10 | rest of 3.2 to 5.4 [U] |
| S6 | NDENT entities, items, animators | S6-1 to S6-4 | src/entity_model.c, src/entity.c, src/entity/*.c, src/item_entity.c, src/animator.c | G_PM_TEXI in Shadow.c, G_PM_RM in entity_model.c | G1 to G10 | 1 to 3 [U] |
| S7 | NDWORLD world areas and battle | S7-1 to S7-3 | src/world/**, src/battle/** | world and battle words | G1 to G10 | [U] |
| S8 | NDFX effects | S8-1 to S8-6 | src/effects/*.c, src/effects.c, src/effect_utils.c | PM_SPLASH words | G1 to G10 | 1 to 4 [U] |
| S9 | NDSCREEN overlays, menus, title, targets | S9-1 to S9-5 | S9 files of PNS table 5.1 | walker target roles | G1 to G10 | small [U] |
| S10 | NDDEL delete the walker | S10-1 to S10-4 | 3ds/src/pm_gbi.c, pm_gpu.c, pm_nusys.c, include/gbi_custom.h, 3ds/include/nd_gbi.h, 3ds/ld/3dsx.ld.in, 3ds/CMakeLists.txt | the walker | full catalog once, console prof | rest of the walk, eye-2 walk |
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

### S1 NDCORE

**S1-1 | NDDROP delete learn-and-replay**
Files: 3ds/src/pm_dlc.inc.c, 3ds/src/pm_gbi.c, 3ds/src/pm_gpu.c, 3ds/src/pm_native.c, 3ds/include/pm_native.h.
1. pm_dlc.inc.c:639: `dlc_learn(...) : dlc_static_group(...)` becomes `dlc_static_group(e, g, gmi, gg)`. Delete dlc_learn (565-573). 12 lines.
2. pm_gbi.c dlc_learn_end (3639) and the NatRec writes: delete. rec_try (3662) and rec_done (3765) capture of templates: delete the nat_tmpl_strip calls. 30 lines (split in 2 items if larger).
3. pm_gpu.c: delete pm_gpu_rec_play, the rec index arena (REC_IDX_N, 2548-2560) and nat_pool_set (2947-2952). 30 lines.
4. pm_native.c: delete nat_tmpl_strip and nat_rec_write. Keep nat_eye_copy and the order sort until S1-3 moves them. 30 lines.
Host test (first): nat_host_test.sh with the capture tests removed still passes the eye and order tests. Gate: census_mac01 (E) 64 batches, 810 triangles (the replay-off numbers [M diag_e53_merge.md 3 option A]). Picture: census_toad_shot (E) region_diff against the S0 shot.

**S1-2 | NDREC records, frame driver, LEGACY bridge**
Files: 3ds/include/nd.h, 3ds/src/nd_core.c, 3ds/src/pm_nusys.c, 3ds/CMakeLists.txt.
1. nd.h: NdRec (PNS 2.2) with _Static_assert size 32, kinds, nd_frame_begin, nd_frame_end, nd_emit, nd_bridge_cut. 30 lines.
2. nd_core.c: record array (2048, aligned 32), nd_bridge_cut (LEGACY record from gMainGfxPos), nd_frame_end. 30 lines.
3. nd_core.c nd_emit: loop with PLD, DRAW_IDX first, LEGACY arm calls a cold function that runs pm_gbi_run(start, bytes), pm_gpu_flush, and the shadow reset. 30 lines.
4. pm_nusys.c gbi_task (76-82): call nd_frame_end and nd_emit instead of pm_gbi_run. Stereo: eye 1 runs nd_emit again (LEGACY only now). 10 lines.
5. CMakeLists.txt (365-366): add nd_core.c, place nd_emit in the hot span list later (S10-3). 3 lines.
Host test (first): nd_host_test case "legacy only": a frame of 3 LEGACY records gives 3 walker calls in order (stub pm_gbi_run). Gate: G1, G5 (nd_size_test.sh), picture census_toad_shot equal, yard_shot (E) equal.

**S1-3 | NDTMPL templates from the key**
Files: 3ds/src/nd_tmpl.c, 3ds/include/nd.h.
1. Key struct, 256-entry table, open hash with a 512 index. 30 lines.
2. TEV words from pm_comb_build (pm_comb.c:393): stage source, operand, function, scale, constant, buffer colour (same register values flush_tev_rebuild writes today). 30 lines.
3. Depth, early depth, depth map (DECAL_EPS rule, pm_gpu.c:907), alpha test, stencil, cull, blend words: moved from the sh_* setters (pm_gpu.c:575-600). 30 lines.
4. Render state rules moved from sync_render (pm_gbi.c:650-706) and rm_apply (2838): othermode to z, decal, blend, atest, zmark. 30 lines.
Host test (first): for every ndref draw, nd_tmpl_build(key) words decode (nd_pica_decode) to the ndref TEV, depth, blend, alpha and cull fields. Gate: G1.

**S1-4 | NDMTX matrices, lights, eye patch**
Files: 3ds/src/nd_mtx.c, 3ds/src/nd_core.c, 3ds/src/pm_native.c (deleted), 3ds/include/pm_native.h (deleted), 3ds/CMakeLists.txt.
1. nd_mtx_push, nd_mtx_pop (16 deep), unpack moved from pm_gbi.c:451, pcomp = post x game (pm_gpu.c:230 order). 30 lines.
2. Matrix pool per eye, eye 1 product with the stereo shift (pm_gbi.c:457-464 upload_mvp rule). 25 lines.
3. Light blocks (pm5 layout). 20 lines.
4. Move nat_eye_copy and the order sort from pm_native.c into nd_core.c. Delete pm_native.c and pm_native.h. 30 lines.
Host test (first): nd_seam_test (S0-5) passes. Eye copy equals a direct eye 1 emit. Gate: G1, G9.

**S1-5 | NDINTERP fps60 interpolation with records**
Files: 3ds/src/nd_mtx.c, 3ds/src/pm_nusys.c.
1. Read pm_interp.c in full. nd_mtx_push looks up pm_interp_find(m) (pm_interp.c:117). 15 lines.
2. Key-frame replay (pm_nusys.c:128 pm_interp_key_draw): re-emit the kept records with the unreplaced matrices. 20 lines.
Host test (first): two emits of one record set with and without the replacement table differ only in matrix words. Gate: G1. Picture: merlon_ball60 (E) equal.

**S1-6 | NDTEX texture descriptors and unit words**
Files: 3ds/src/nd_tex.c, 3ds/include/nd.h.
1. nd_tex_desc, nd_tex_get: hash probe on (raster, palette, fmt, siz, w, h), pm_tex load on a miss, 4-word unit block from pm_tex_slot_words. 30 lines.
2. Wrap, mirror, mask, shift to the parameter word (moved from tile_bound and ensure_tile, pm_gbi.c:2535, 2567). 30 lines.
Host test (first): for every ndref texture, nd_tex words equal the ndref unit 0 and unit 1 words. Gate: G1.

### S2 NDFRAME

**S2-1 | NDGBI redirect header, state half**
Files: 3ds/include/nd_gbi.h, 3ds/src/nd_core.c.
1. nd_gbi.h: `#undef` and redefine the state macros (PNS table 5) as nd_* calls plus the bridge dual-write of the N64 word. 30 lines per item, 3 items.
2. Draw macros: nd_vtx, nd_tri, nd_tri2, nd_fill_rect, nd_tex_rect, nd_obj_draw. Unknown macro: `#error`. 30 lines.
3. grep list of every gSP and gDP macro name used in src/ (R10) in a comment block, each mapped or marked `#error`.
Host test (first): a host file that uses every mapped macro compiles and produces the expected nd call log. Gate: G1.

**S2-2 | NDPASS frame state calls**
Files: 3ds/src/nd_core.c, 3ds/CMakeLists.txt.
1. nd_pass_begin, nd_viewport (post matrix as gv_post, pm_gpu.c:224-230), nd_scissor, nd_clear, nd_target. 30 lines.
2. CMakeLists.txt: force-include nd_gbi.h for src/main_loop.c, src/cam_main.c, src/background_gfx.c, src/background.c. 6 lines.
Host test (first): the background draws of ndref_title and ndref_yard equal (nd_stream_cmp). Gate: G1 to G4, G10. Picture: yard_shot (E), golden_multi (E) title shot.

**S2-3 | NDN64 N64-same check for S2 files**
Files: none changed (check only). Run nd_n64_same.sh on the 4 files. Gate: no diff.

### S3 NDMODEL

**S3-1 | NDDLC static list compiler**
Files: 3ds/src/nd_dlc.c, 3ds/src/pm_dlc.inc.c, 3ds/src/pm_map.c, 3ds/CMakeLists.txt.
1. Move dlc_compile (pm_dlc.inc.c:145) and dlc_static_build (219) into nd_dlc.c. Output NdObj into the static pool and the packed index pool (no padding, map_e4.md 3). 30 lines per item, 2 items.
2. Extend the op set: SETCOMBINE, SETOTHERMODE, colour ops, texture loads (to nd_tex_desc), nested static DL. Compute the dep mask. 30 lines per item, 2 items.
3. pm_map.c (after fix_shape, 97-128, and the pm_dlc_gen bump at 377-379): compile the shape lists at map load. 10 lines.
4. Move dlc_box_out as is (the only allowed skip, memory feedback_no-culling). 20 lines.
Host test (first): for the mac_01 and mac_02 shapes, NdObj index lists equal the dlc_static_build sd lists (the P1 host check of DPM 10). Gate: G1.

**S3-2 | NDMDL1 plain opaque models**
Files: src/model.c (TARGET_3DS blocks), 3ds/CMakeLists.txt.
1. model.c:176-187: MODEL_TEX_OP becomes nd_tex_handle(index). MODEL_RM_OP becomes nd_obj_draw of the render mode list. 10 lines.
2. model.c load_texture_impl (2109-2160): `nd_tex_handle_set(index, header, raster, palette, aux)` after pm_texbind_set. 8 lines.
3. model.c:1513-1521 and 2072-2073: delete the G_PM_MODEL header. 15 lines.
4. CMakeLists.txt: force-include nd_gbi.h for src/model.c. 2 lines.
Host test (first): host appendGfx_model with the ndref model inputs (plain, SURFACE_OPA) gives draws equal to the ndref model draws (nd_stream_cmp, tag filter). nd_n64_same.sh on src/model.c. Gate: G1 to G4, G9, G10. Picture: census_toad_shot (E), mac01_seam (E).

**S3-3 | NDMDL2 all render modes and classes**
Files: src/model.c, 3ds/src/nd_tmpl.c.
1. Fog, depth tint, remap tint, shroud: their colour and fog calls go to the uniform block (no template split per value). 25 lines.
2. Alpha test, decal, translucent render modes: ordered flag. 15 lines.
Host test (first): all tagged models of ndref_toad, ndref_yard equal. Gate: G1 to G4. Picture: forest_mim (E), toad_water (E).

**S3-4 | NDMDL3 panner, billboard, custom gfx, local vertex copy, projected coords**
Files: src/model.c.
1. Panner: tile offsets per frame to uvso (moved gv_params_calc rule, pm_gbi.c:1351). 15 lines.
2. Custom gfx: build_custom_gfx output recorded as range ids, call sites 2016, 2063 repeat the range. 30 lines.
3. Local vertex copy (2054, 4252-4280): index lists compiled once, vertices through the ring. 20 lines.
4. mdl_project_tex_coords (4339): read, then the agent reports one of: no edit, or a named edit for the planner. 0 to 20 lines.
Host test (first): the panner, billboard and custom gfx models of ndref_toad and ndref_yard equal. Gate: G1 to G4. Picture: toad_fountain (E), kmr_10_band (E).

**S3-5 | NDMDL4 floor reflection, sort, delete**
Files: src/model.c, 3ds/src/nd_core.c, 3ds/src/pm_gbi.c, 3ds/src/pm_dlc.inc.c (deleted), include/gbi_custom.h.
1. execute_render_tasks (4960-4995): nd_range_begin, nd_range_end, nd_range_repeat with the flip matrix. 20 lines.
2. nd_core.c: sort unordered runs by (tmpl, tex0, tex1). Merge adjacent DRAW_IDX with equal key (grange rule). 30 lines.
3. pm_gbi.c: delete the G_PM_MODEL, G_PM_TEX, G_PM_RM arms, mg_run (3889), rec_try, rec_done, the census mr0 and ms0 code, the `#include "pm_dlc.inc.c"` (1628). Delete pm_dlc.inc.c. gbi_custom.h: delete G_PM_MODEL, G_PM_TEX, G_PM_RM. 30 lines per item, 3 items.
Host test (first): batches of ndref_toad not above the reference after the sort. Gate: G1 to G10. Picture: census_toad_shot, yard_shot, forest_mim, toad_pond (all E). Console prof at the S3 end (expected 7.9 to 12.7 ms phase on).

### S4 NDSPRITE

**S4-1 | NDSPR1 sprite components**
Files: src/sprite.c, 3ds/CMakeLists.txt.
1. sprite.c:236-279 (G_PM_TEXI blocks in spr_appendGfx_component_flat 227 and spr_appendGfx_component 339): one nd_tex_load per component. 25 lines.
2. sprite.c:335 SPR_RM_LIST: nd_obj_draw. 3 lines. Force-include for sprite.c. 2 lines.
Host test (first): the sprite draws of ndref_toad (tag filter) equal. nd_n64_same.sh. Gate: G1 to G4, G10. Picture: census_toad_shot NPC strip.

**S4-2 | NDSPR2 imgfx meshes and shading**
Files: src/imgfx.c, src/sprite_shading.c, 3ds/CMakeLists.txt.
1. Force-include only. The imgfx ring vertices go through nd_vtx (imgfx_appendGfx_mesh 285). The 4-bit strip load rule moves into nd_tile (pm_gbi.c:3152). 20 lines in nd_tex.c.
2. Shaded sprite (two tiles on one 4-bit image, pm_gbi.c:2493-2534 load_shaded_sprite): nd_tex two-unit form. 30 lines.
Host test (first): ndref_battle and ndref_toad sprite draws equal, unit 1 words included. Gate: G1 to G4. Picture: battle_start (E), talk_off (E).

**S4-3 | NDSPR3 NPC and player call paths**
Files: src/npc.c, src/77480.c, 3ds/CMakeLists.txt.
1. Force-include. Confirm by grep that render_npcs (npc.c:963) and render_player (77480.c:1468) write gMainGfxPos only. 0 to 10 lines.
Host test (first): all sprite tags of ndref_toad equal. Gate: G1 to G4. Console prof at the S4 end.

### S5 NDHUD

**S5-1 | NDHUD1 HUD elements**
Files: src/hud_element.c, 3ds/CMakeLists.txt.
1. hud_element.c:365-402 and 477-600: the G_PM_TEXI blocks become nd_tex_load plus nd_tex_rect. 30 lines per item, 2 items.
2. The one gSPEndDisplayList list (grep): nd_range. 15 lines.
Host test (first): ndref_text and ndref_pause HUD tags equal. nd_n64_same.sh. Gate: G1 to G4. Picture: ui1_choice (E), mapchange_hud (E).

**S5-2 | NDTXT text glyphs**
Files: src/msg_draw.c, src/msg.c, 3ds/src/nd_tex.c, 3ds/CMakeLists.txt.
1. msg_draw.c:1758-1802 (msg_draw_char 1669): nd_glyph(sheet, palette, index). 20 lines.
2. nd_tex.c: glyph atlas moved from glyph_atlas_bound (pm_gbi.c:2543). 30 lines.
Host test (first): ndref_text glyph draws equal. Gate: G1 to G4. Picture: text_menu (E), house_sign (E), super_text (E).

**S5-3 | NDBOX boxes and windows**
Files: src/draw_box.c, src/windows.c, src/draw_img_util.c, 3ds/CMakeLists.txt.
1. Force-include. Fill rectangles through nd_fill_rect with the inclusive rule (pm_gbi.c:3449) and the signed 10-bit field (3413). 25 lines in nd_core.c.
Host test (first): box and window draws of ndref_text and ndref_pause equal. Gate: G1 to G4. Picture: toad_house_menu (E).

**S5-4 | NDSTAT status icons and inventory**
Files: src/status_icons.c, src/status_star_shimmer.c, src/inventory.c, 3ds/CMakeLists.txt.
1. Force-include. 5 lines.
Host test (first): status tags of ndref_toad equal. Gate: G1 to G4. Console prof at the S5 end.

### S6 NDENT

**S6-1 | NDEMDL entity models**
Files: src/entity_model.c, 3ds/src/nd_dlc.c, 3ds/CMakeLists.txt.
1. entity_model.c:282, 370: delete the G_PM_RM rewrite. 10 lines.
2. nd_dlc: key on (list, segment base) for draw_entity_model_B and D (492, 561). 25 lines.
Host test (first): ndref_toad entity tags equal. nd_n64_same.sh. Gate: G1 to G4, NdObj count per map logged. Picture: toad_gate (E).

**S6-2 | NDENT2 entity files and shadows**
Files: src/entity/*.c, src/entity/Shadow.c, src/entity.c, 3ds/CMakeLists.txt.
1. Shadow.c:28-50: delete gsSPPmTexInline headers. 6 lines.
2. Force-include for src/entity/*.c and src/entity.c. 3 lines.
Host test (first): shadow and block draws equal (ndref_toad, ndref_yard). Gate: G1 to G4. Picture: coin_kmr03 (E).

**S6-3 | NDITEM item entities**
Files: src/item_entity.c, 3ds/CMakeLists.txt.
1. Force-include. 2 lines. Host test (first): item tags equal (ndref_yard). Gate: G1 to G4. Picture: coin_dgb13 (E).

**S6-4 | NDANIM animators**
Files: src/animator.c, 3ds/CMakeLists.txt.
1. appendGfx_animator (779) and appendGfx_animator_node (851): lists built for later become ranges. 30 lines.
Host test (first): animator tags of ndref_battle equal. nd_n64_same.sh. Gate: G1 to G4. Picture: battle_start (E). Console prof at the S6 end.

### S7 NDWORLD

**S7-1 | NDWLIST list world and battle files with deferred lists**
No code. grep for local Gfx pointers, gSPEndDisplayList and custom gfx builders in src/world and src/battle. Output: the file list for S7-2 and S7-3 in docs/handoff/agent_S7.md.

**S7-2 | NDWORLD2 world area files**
Files: the S7-1 list for src/world, 3ds/CMakeLists.txt. 1. Force-include. 2. Range edits per listed file (30 lines each, one file per item). Host test (first): world tags of ndref_toad and ndref_yard equal. Gate: G1 to G4. Picture: tour10 is not used (full test). Use house_sign (E), toad_fountain (E).

**S7-3 | NDBTL battle files**
Files: the S7-1 list for src/battle, 3ds/CMakeLists.txt. As S7-2. Host test (first): ndref_battle equal. Gate: G1 to G4. Picture: battle_start (E), battle_menu_bar (E), battle_danger (E).

### S8 NDFX

**S8-1 | NDFXLIST effect files with buffer reads and blob reads**
No code. grep src/effects for gDPSetColorImage, frame buffer reads, big-endian blob reads (risk constraint 7). Output: list for S8-6.

**S8-2 to S8-5 | NDFX1 to NDFX4 effect files A to Z in 4 blocks of about 32 files**
Files: src/effects/<block>.c, 3ds/CMakeLists.txt. 1. Force-include for the block. 2 lines. 2. Fix list per file only if the host test fails (one file per item).
Host test (first): ndref_splash effect tags equal (S8-2 to S8-5 each add one fxtest ndref scene for a block effect, scenario (N) ndref_fx_<block>, base nok11_splash with another effect id). Gate: G1 to G4. Picture: nok11_splash (E), chapter_wide (E), bowser_intro_fire (E).

**S8-6 | NDFXS splash markers, buffer effects, effects.c**
Files: src/effects/water_splash.c, the S8-1 list, src/effects.c, src/effect_utils.c.
1. water_splash.c: PM_SPLASH_ON and _OFF words become nd_splash(1) and nd_splash(0). 6 lines.
2. Buffer effects: nd_target(role) per call site. 30 lines per file.
Host test (first): ndref_splash equal. nd_n64_same.sh. Gate: G1 to G4. Picture: nok11_splash. Console prof at the S8 end.

### S9 NDSCREEN

**S9-1 | NDTGT target roles and CPU-path cases**
Files: 3ds/src/nd_core.c, 3ds/src/nd_tmpl.c.
1. nd_target roles from img_role (pm_gbi.c:243): main, off-screen texture, depth, CPU fill, colour capture. 30 lines.
2. List every case that the walker draws on the CPU (seam_obj, seam_clip, pm_gbi.c:364, pm_gpu_grange refusals, R6). One GPU form per case (zmark, decal with blend, pm_wide). 30 lines per case.
Host test (first): CPU-path count 0 in all ndref scenes with the native path. Gate: G1.

**S9-2 | NDOVL screen overlays and curtains**
Files: src/screen_overlays.c, src/screen_render_util.c, src/curtains.c, 3ds/CMakeLists.txt. Force-include. Role edits per call site. Host test (first): overlay tags equal. Gate: G1 to G4. Picture: intro_native (E), merlon_door (E).

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
3. pm_nusys.c gbi_task: delete the eye-2 walk and pm_gbi_run. 10 lines.
Host test (first): no symbol pm_gbi_run referenced (nm on the host build). Gate: G1.

**S10-2 | NDDEL2 delete the walker**
Files: 3ds/src/pm_gbi.c (deleted), 3ds/src/pm_gpu.c, include/gbi_custom.h, 3ds/CMakeLists.txt.
1. Delete pm_gbi.c and its CMake line. 5 lines.
2. pm_gpu.c: delete flush_draw (952), grange (2314-2340), the CPU triangle path and vert_base (40, 1406), cen_*, rec_*. One function per item, 30 lines or less of residue each.
3. gbi_custom.h: delete G_PM_TEXI and the PM_SPLASH words. 10 lines.
Host test (first): build links. Gate: G1, G5.

**S10-3 | NDHOT hot span and cold split**
Files: 3ds/ld/3dsx.ld.in, 3ds/CMakeLists.txt.
1. 3dsx.ld.in (32-38 CF4 span): nd_emit and its callees in call order. 10 lines.
2. CMakeLists.txt: nd_dlc.c and the cold nd files at -Os. 3 lines.
Host test (first): nd_size_test.sh 8 KB or less. Gate: G5, G7 (map file).

**S10-4 | NDFULL epic merge test**
No code. Full catalog once (CLAUDE.md rule 5, epic merge). Console prof push with `prof` and `prof profstall` (Q1). Gate: G2 on all catalog shots, G6 to G8 numbers recorded in docs/handoff/agent_S10.md.

### S11 NDTUNE (reopen rules only)

**S11-1 | NDSORT2 deeper sort or merge** only if G4 shows draws above 64 in Toad Town. Files: nd_core.c.
**S11-2 | NDPIPE GPU pipeline depth 2** only if prof gwait is above 2 ms. Files: pm_gpu.c (second command buffer and vertex ring), nd_core.c. Plan by Opus first (PC1 section 6).
**S11-3 | NDCORE1 emit on core 1** only if prof nd is above 3 ms after S10 and the user answers Q11. Files: pm_main.c, nd_core.c. Thread rules: no blocking call under a lock, one lock at a time, event wait not busy wait (AC plan rules).

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
