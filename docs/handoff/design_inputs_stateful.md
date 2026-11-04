# design_inputs_stateful: inputs for the retained per-model record design

Read only. HEAD 6c157996c plus the working tree (src/model.c has local edits). No source edit, no run.
Sources: CLAUDE.md, plan_mr2_record.md, agent_drec1.md, plan_dl_rows.md, agent_tbrep.md, agent_bat1.md,
plan_dl_architecture.md, diag_dlr.md, memory project_console-fps-logging-off.md, build3ds/hw_live_log_170.txt.
Tags: [C] read from code, [M] read from the log, [I] inference.

Design under study: each static or near-static model keeps a GPU-ready record of its unchanging state, with a
version per input. Only matrix, lights and camera are written each frame.

Limits on all numbers: every log value has phase on. The console with logging off runs 24 to 28 fps
(memory file). Phase-on shares give direction only.

## 1. Inputs that appendGfx_model reads (src/model.c 1384 to 2069)

"Counter w/o game code": can the port keep a version for this input without an edit in src/? "Compare" = the
port can compare the value (or the emitted word) each frame. "Counter" = a real write-time counter, which needs an
edit at the write site.

### 1a. Static for the model's life (map load to map exit)

| Input | Read at | Written at | Counter w/o game code |
|---|---|---|---|
| model->modelNode, displayData->displayList | 1411, 2050 | mdl_create_model 2620 (from the shape file) | Yes: pm_dlc_gen (pm_dlc.inc.c 7) already covers a write to the shape buffer |
| model->groupData->lightingGroup, numLights (Lightsn data in the shape file) | 1520 to 1550 | shape file load only (include/model.h 40); no runtime writer in src/ [C grep] | Yes: pm_dlc_gen. Content is static, but the MOVEMEM words still go out per frame |
| model->texPannerID | 2017 to 2020 | mdl_create_model 2666/2668; set_tex_panner 4024 (evt EnableTexPanning, map_api.c 184) | Compare only |
| textureHeader (extraTiles, auxCombineType, auxCombineSubType, mainW/H, auxW/H) | 1438, 1595 to 1602, 2024 to 2032 | load_texture_impl 2154 (memcpy) | Yes: pm_texbind_set (model.c 2160, pm_gbi.c 2784) runs after every rebuild of a handle |
| ModelRenderModes[], SolidCombineModes[], AlphaTestCombineModes[] | 1607, 1609, 1681 ff | const tables | not needed |

### 1b. Changes by game code (event scripts, map setup)

| Input | Read at | Written by (file:line, function) | Counter w/o game code |
|---|---|---|---|
| model->textureID | 1433 | mdl_create_model (model.c 2620 ff) | Compare |
| model->textureVariation | 1434 | model.c 2690 (create), map_api.c 180 SetModelTexVariant | Compare. Emitted G_PM_TEX w0 (handle index) shows it |
| TextureHandles[i].gfx, raster, palette, auxRaster, auxPalette | 1434 to 1437, 1562 | load_texture_impl 2109 to 2160 (writes 2118 to 2155); mdl_load_all_textures 2481 (gfx = NULL) | Yes: pm_texbind_set at 2160 is the one funnel. tb_gen (plan_mr2 3b) is not built (DREC1 dropped) |
| Texture texel bytes in TextureHeap | through handle->gfx (SETTIMG) | ROM DMA into TextureHeapPos (pm_os.c 394 calls pm_tex_invalidate) | Partial: pm_tex content-hash slots (pm_tex.c 918). No per-handle counter |
| model->renderMode | 1447 | model.c 2684/2686 (create), map_api.c 770 SetRenderMode | Compare. Emitted G_PM_RM w1 shows it |
| model->customGfxIndex (low nibble gfx set, high nibble tint type) | 1476, 2008 | model.c 2670 (create), mdl_group_set_custom_gfx 3952/3957/3962, set_mdl_custom_gfx_set 4053 | Compare |
| model->flags (USES_CUSTOM_GFX, HAS_TEX_PANNER, BILLBOARD, IGNORE_FOG, TRANSFORM_GROUP_MEMBER, IGNORE_MATRIX, HAS_LOCAL_VERTEX_COPY) | 1393, 1469, 1986 to 2059 | model.c 2657 (create), 2699, 2829/2841/2857 (matrix update), 3808/3825 (transform groups), 3880 to 3900 (mdl_group_set_visibility), 3975; map_api.c 26/49/76/101/138/149/151; animator.c 708; battle/190B20.c 3080 to 3130; battle/level_up.c 432 to 450; common/FoliageTransform.inc.c 69 | Compare only. Many writers (more than 20 sites) |
| gFogSettings->enabled, color, startDistance, endDistance | 1469, 1810, 1811, 1878, 1947 to 1954 | init 2563 to 2569; enable_world_fog 3988, disable_world_fog 3992, set_world_fog_dist 3996, set_world_fog_color 4001 | Compare (8 bytes). A counter needs an edit in 4 setters |
| Tint globals DepthTint*, RemapTint*, ShroudTint* | 1483 to 1494, 1815, 1878 to 1880, 1947 to 1952 | mdl_set_shroud_tint_params 4118, mdl_set_depth_tint_params 4132, mdl_set_remap_tint_params 4157; clear 2524 to 2527 | Compare. A counter needs an edit in 3 setters |
| gCurrentCustomModelGfxPtr | 2009, 2056 | 2518, 2532, 2583, 2593 (world or battle context) | Compare (pointer) |

### 1c. Per frame

| Input | Read at | Written at | Note |
|---|---|---|---|
| model->finalMtx (pointer) | 1988, 1999 | mdl_update_transform_matrices 2794: static model gets &model->savedMtx (2825); a model that moved in the last 2 frames gets a new matrixStack entry (2821, 2854) | The pointer is stable only for a static model. The port can test `w1 == &model->savedMtx` with no game edit [I] |
| model->savedMtx (content) | via finalMtx | mdl_create_model 2695/2698; mdl_update_transform_matrices 2817 (when matrixFreshness reaches 0) | A counter is possible only with an edit at 2817. MTXM (64-byte compare) was dropped on its gate |
| texPannerMainU/V, texPannerAuxU/V[texPannerID] | 2017 to 2020 | set_main_pan_u/v 4028/4032, set_aux_pan_u/v 4036/4040 (scripts, often each frame) | Per frame for panner models |
| gCameras[gCurrentCamID].mtxBillboard | 2039 | camera update (per frame) | Billboard models only |
| Custom gfx lists (*gCurrentCustomModelGfxPtr)[i] | 2010, 2057 | build_custom_gfx 4066 (per frame) | Custom-gfx models only |
| Camera projection, viewport | outside the model group | camera code; port upload_mvp (pm_gbi.c 438) | Per frame, one value per camera |
| gCurrentCameraID | 1422 (modellog only) | camera loop | appendGfx_model uses gCurrentCamID only for the billboard (2039) |

Result for (1): the inputs of a plain model (MR0 test, model.c 1504 to 1505) that are not per frame are textureID,
textureVariation, renderMode, customGfxIndex, flags, gFogSettings, the handle and its header. All of them show up in
the words the model emits each frame (G_PM_TEX w0 and w1, SETCOMBINE, G_PM_RM w1). A compare of those words needs no
game edit. A true write-time version counter needs edits at map_api.c 180, 770, model.c 3952 to 3962, 4053, the 4
fog setters, and the flags writers.

## 2. Invalidation sources that exist now

| Source | Where | Covers | Misses |
|---|---|---|---|
| pm_pdl_heap_gen | was pm_tex.c about 762 | Deleted with the PDL compiler (b17262105 DLF23). Not in the tree now [C grep]. diag_dlr line 39: it changed often, so no entry lived for a frame | Not usable |
| pm_dlc_gen | pm_dlc.inc.c 7; bumped pm_tex.c 854 (write into gMapShapeData), pm_map.c 379 (shape file final) | Shape buffer writes: display lists, vertices, lights of map models. Key of dlc_try (pm_dlc.inc.c 442 to 448) | Texture handles, TextureHeap, model fields, fog, tint, render mode |
| pm_tex_epoch_v | pm_tex.c 91; bumped 96, 115, 270, 644, 812 (pm_tex_begin_frame, every frame), 853 (every pm_tex_invalidate), 981, 1434, 1537, 1578 | Any change to what pm_tex_bind gives, within one frame | Bumped every frame (812): never valid across frames |
| pm_tex_wrap_gen_v | pm_tex.c 93; bumped 271 (set_wrap), 813 (frame start) | Wrap state, within a frame | Bumped every frame |
| gv_epoch | pm_gbi.c 1437; bumped 2392, 3573 (mg_run), 3621, 3676 (walker), pm_dlc.inc.c 282, 424 | Draw-state change since the last triangle (gv_epoch_enter 1455) | Bumped by every non-vertex, non-matrix, non-DL op: about every model. Within frame only |
| tex_gen (TBREP) | pm_gbi.c 175; bumped 252, 1180, 2545, 2659, 2728, 2785, 3060, 3363, 3442 | Writes of texture memory and texture state between two G_PM_TEX lists | Within frame; tb_run skip only for the same list twice in a row (pm_gbi.c 2801) |
| comb_gen | pm_gbi.c 312; bumped 601 | Combine state writes outside SETCOMBINE (box_fold_sync) | Within frame |
| tb_list[] / pm_texbind_set | pm_gbi.c 2783 to 2790; called model.c 2160 | Handle (re)build: the one texture reload path for map models | Texel bytes rewritten at the same address without load_texture_impl (none found in src/ for map textures [I]) |
| pm_tex slot validity | pm_tex.c 918 pm_tex_invalidate, content hash (plan_tex_invalidate 9f24be83e) | Texture cache slots for any ROM DMA, CFB write, Yay0 decode | Not a per-handle or per-model key |
| pm_gpu_flush_gen | pm_gpu.c 1840 | Batch boundary in pm_gpu | Within frame |

Texture reload paths: load_texture_impl (model.c 2109) from load_texture_by_name (2164) and load_texture_variants
(2305), both at map load; mdl_load_all_textures (2466) clears all gfx. Each ends at pm_texbind_set (2160).

Result for (2): only pm_dlc_gen and pm_texbind_set are cross-frame keys. Every other generation restarts each frame.

## 3. Static models in the scenes

Toad Town (mac_01, `load map area 1 map 2` log line 1345), still windows f21585 to f22185 [M]:

| Line | Value per frame |
|---|---|
| mr0 (11101, 11265, 11426) | plain 89; per model us ops 185 to 187, close 46 |
| ms0 (11102, 11266, 11427) | opaque plain 70, runs 13, removable closes 4, texture changes 36 |
| mdlsum (11088, 11252, 11413) | models 103 to 105, static 101 (matrix bytes equal in 90 % or more of frames, pm_gbi.c 2261, 2281) |
| dcsum (11114, 11290, 11451) | closes 64 to 69, 7 cache flushes at 26 to 28 us each |

- The plan's figure of 88 to 90 plain per frame holds (89 here, 90 in plan_dl_rows section 0).
- ms0 "texture changes 36" counts binds between adjacent models in draw order (after BAT1). It is not a change of
  one model's texture across frames.
- mdlsum "static" tests only the matrix bytes (pm_gbi.c 2242 to 2248). The 4 non-static slots are fn and task entries
  (lines 11563, 11564, 11569, 11572), not map models [M].
- Per-model texture, matrix or render-mode change per frame: UNKNOWN. The log has 0 modellog lines. No census
  counts a per-model change of G_PM_TEX w1 or G_PM_RM w1 across frames.
- yard (kmr_20): UNKNOWN. This log has no kmr_20 load. The other load (area 5 map 5, line 195) shows mr0 plain 0 or 1.
  A yard run with phase is needed for mr0 and ms0.

## 4. GPU-side state per model (PICA200)

State a plain model draw sets, and the call that sets it [C]:

| State | Call | Where |
|---|---|---|
| Shader program and attributes | C3D_BindProgram, C3D_SetAttrInfo | pm_gpu.c flush_draw 892, 924 |
| Vertex uniforms (uvso, cmul, cadd, zmix, light flags, fog, palette matrices, projection) | C3D_FVUnifSet, C3D_FVUnifMtx4x4 | flush_draw 893 to 950; light table flush_light_tab 901 |
| Texture unit 0 and 1 | C3D_TexBind | pm_tex_bind pm_tex.c 1465 (1482), pm_tex_bind_aux 1553 to 1569 |
| Filter and wrap | C3D_TexSetFilter, C3D_TexSetWrap (through pm_tex_set_wrap 268) | pm_tex.c |
| Combiner (TEV stages, buffer color) | C3D_TexEnvInit, Src, OpRgb, OpAlpha, Func, Color, Scale, C3D_TexEnvBufUpdate | flush_tev_rebuild 569 to 660; key test 986 |
| Depth test, depth map, early depth | sh_depth_test 480, sh_depth_map 484, sh_early 501 | flush_draw 993 to 1006 |
| Alpha test | sh_alpha_test 488 | 1007, 1008 |
| Stencil | C3D_StencilTest, C3D_StencilOp | 994, 995, 1004 |
| Scissor, cull, blend, user clip | sh_scissor 492, sh_cull 500, sh_blend 496, sh_user_clip 505 | 1024 to 1037 |
| Draw | C3D_DrawElements | 1048 |
| Plan and mode inputs | pm_gpu_set_plan 2143, pm_gpu_set_mode 2071 | called from pm_gbi.c |

- The sh_* functions keep a shadow and skip an equal value. sh_clear (pm_gpu.c 479) drops the shadow at each split.
- citro3d writes the dirty state into the command buffer at C3D_DrawElements (C3Di_UpdateContext, library code).
- Replay as one call sequence: the repo uses no raw command path now (no GPUCMD_AddRawCommands, no CMDBUF_JUMP)
  [C grep]. A record can be a stored list of PICA register writes, copied into the command buffer with
  GPUCMD_AddRawCommands (libctru) [I]. Two limits [I]: (a) citro3d keeps its own copy of the state, and a raw write
  bypasses it, so after a replay the port must mark the citro3d state dirty or replay all state the next draw reads;
  (b) the sh_* shadows and tev_valid must be reset (as sh_clear does). Uniform values (matrix, lights) stay outside
  the record.
- The static map geometry already has a retained GPU record: dlc static groups commit their vertices once to the
  gring ring (pm_dlc.inc.c 208, pm_gpu_gstat_commit pm_gpu.c 1990), and pm_gpu_grange (2010) copies the indices each
  draw.

## 5. Open questions for the user

1. Game code: (a) edit src/ write sites (map_api.c 180, 770, model.c 3952 to 3962, 4053, fog and tint setters,
   flags writers) to bump version counters, or (b) keep src/ unchanged and compare the emitted words each frame.
2. Key for the record: (a) per model (model slot plus word compare), or (b) per texture handle plus per render mode
   (shared records).
3. Rebuild rule: (a) rebuild every record on any pm_dlc_gen or pm_texbind_set change (map load, rare), or (b) rebuild
   only the records whose handle or shape range changed.
4. Tint, billboard, tex-panner and custom-gfx models: (a) stay on the current word path, or (b) get a record with
   per-frame patch fields.
5. Fog models (RENDER_CLASS_FOG): (a) in the record with fog color and range as per-frame values, or (b) old path.
6. Record form: (a) a raw PICA register list replayed with GPUCMD_AddRawCommands (citro3d state marked dirty after),
   or (b) a decoded state struct that drives the existing pm_gpu and citro3d calls (no raw commands).
7. Measurement first: (a) add a census row (per-model change of G_PM_TEX w1, G_PM_RM w1, finalMtx across frames, plus
   a yard run) before the plan, or (b) plan now on the Toad Town numbers only.
8. Matrix of a static model: (a) treat `finalMtx == &savedMtx` as unchanged (needs a savedMtx version, so an edit at
   model.c 2817), or (b) upload the matrix every frame as now.

## 6. Hardware constraints

- O3DS: 2 ARM11 MPCore cores at 268 MHz, 16 KB L1I and 16 KB L1D each, 4-way, 32-byte lines, no L2
  (docs/handoff/hw_guidelines.md 5 to 7). App code runs on core 0; core 1 is the system core with a small slice
  (hw_guidelines 6). N3DS: 4 cores at up to 804 MHz with a 2 MB L2 [I, not in the repo]. The design must fit the O3DS
  and must not rely on the L2.
- FCRAM is shared by CPU and GPU. Command lists, vertices and indices live in linear FCRAM (hw_guidelines 9).
  VRAM is 6 MB (log line 198: `vram free ... of 6144KB`). The CPU must not read VRAM (hw_guidelines 9). The port
  writes textures to VRAM only by GPU copy from a linear stage: C3D_TexLoadImage from vram_stage (pm_tex.c 543 to 546).
  No CPU read of VRAM was found [C grep].
- Submits per frame: C3D_FrameEnd (pm_gpu.c 1786) plus up to 2 C3D_FrameSplit from kick_check (541 to 552, KICK1
  and KICK2) = up to 3 on the main target. split_frame (284 to 290) and the off-screen pass (1616) add one each when
  used. This log has no split count line.
- Cache flushes: gpu_flush_written (pm_gpu.c 268 to 280) flushes the vertex, gring and index spans once before each
  submit, plus pending texture spans (pm_tex.c 536). dcsum: 7 flushes per frame at 26 to 28 us (log 11114 to 11451).
  pm_gpu_gstat_commit (1990) flushes once per static group at compile, not per frame.
- Per-model hops now [C, M]: 0 submits, 0 cache flushes, 0 VRAM reads, 0 DMA per model in the steady state. A
  "close" (pm_gpu_flush 2199, flush_draw 852) writes commands into the CPU-side buffer only: 64 to 69 per frame,
  about 46 us each inside plain models (mr0). A texture miss adds 1 flush and 1 C3D_TexLoadImage (pm_tex.c 545,
  627); tx1 misses are 0 in steady state (plan_dl_rows section 0).
- Design constraint: static per-model records live in linear memory, written only when a version changes. The CPU
  reads only the dirty check each frame. Per-frame values (matrix, lights, camera) go into one small block, written
  once per frame. No CPU read from VRAM. No per-model flush. Target: one batch and one flush per frame for the
  static records (now 7 flushes and up to 3 submits per frame, which are per split, not per model).
