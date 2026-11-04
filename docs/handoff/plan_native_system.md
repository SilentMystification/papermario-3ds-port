# plan_native_system: replace the N64 display list walker with a native 3DS draw system

Plan only (Opus). No game source edit. Branch at the time of writing: nds1 (HEAD c4740fdce).
Row detail: docs/handoff/plan_native_slices.md. Rules: CLAUDE.md, docs/handoff/hw_guidelines.md.
Tags: [C file:line] read in source for this plan. [M doc] measured, from the named log or document. [U] unverified (an estimate, a target or a fact not checked in code, in a console log or in the ARM11 TRM / 3dbrew). Every number without [C] or [M] is [U].

Abbreviations: DL = display list. GBI = N64 graphics binary interface. Walker = the per-frame DL interpreter (3ds/src/pm_gbi.c, pm_dlc.inc.c, the batch close in pm_gpu.c, the texture ops that feed pm_tex.c). PICA = the PICA200 GPU. TEV = texture combiner stage. VS = vertex shader. ND = native draw (the new system). NdRec = one native draw record. Template = one prebuilt block of PICA state words. Bridge = the interim path that lets the walker draw producers that are not ported yet. ndref = the reference draw stream file (section 9). O3DS = Old 3DS.

## 0. Summary

- Today the walker costs 32.8 to 39.8 ms of a 46.6 to 55.0 ms frame (phase on, console build 1588, Toad Town) [M inv_frame_cost.md table 1]. The close stages add 4.4 to 5.8 ms. Game logic plus list build is 7.6 ms.
- Target: 60 fps on the O3DS (16.7 ms per drawn frame). Floor: 30 fps (33.4 ms) [M memory project_perf-targets].
- Design: game code calls typed native functions (`nd_*`). Static N64 asset lists (map shapes, entity, effect and battle lists) are compiled once into native objects. One tight loop turns the frame's records into PICA words. The walker, the batch close (flush_draw), the custom G_PM_* ops and the second-eye walk are deleted in slice S10.
- No step uses walker output as a design input or as a runtime input. The walker output is used once, in slice S0, as a test oracle (the ndref files). Section 9 states why this is different from the failed learn-and-replay.

## 1. Why the earlier attempts failed, and the rule that follows

| Attempt | Failure | Source | Rule for this plan |
|---|---|---|---|
| Learn-and-replay of walker batches (UC3e, E5-3) | 71 batches against 64. The replay closed the open live batch and lost the cross-group merge. Unit 1 texture never checked. The census hid the wrong texture. | diag_e53_merge.md 1.2, 2.6. agent_nds1_e6.md | No record is learned from a walk. Records come from producer arguments and from compiled asset data. |
| Templates captured from live records (nat_tmpl_strip) | Template words come from the first flush of a learn window. A wrong window gives a wrong template. | pm_native.h:1-4 [C]. diag_e53_merge.md 1.2 | Templates are built from the state key with pm_comb.c and the moved state-word writers. Host test checks them. |
| Census equality as the gate | The census holds live ids, not emitted words. It passed with wrong textures. | diag_e53_merge.md 1.2 | Gate = picture viewed first, then triangles equal, batches not above, both texture words equal (section 9). |
| Route B, cut flush_draw cost | Ceiling 0.1 to 1.1 ms. 38 percent texture share not reproduced by any log. | map_rtb1.md 0 | flush_draw is deleted, not tuned. |
| Record replay saved nothing in Azahar | Azahar has no cache model. No console number exists. | nds1_status_2026-11-05.md finding 2 | Speed gates use the console `prof` line only. Azahar is for pictures and crashes. |

## 2. Native data model

### 2.1 Frame flow

1. Game thread, during the step (src/main_loop.c gfx_task_background and gfx_draw_frame [C main_loop.c:158, 187]): producers call `nd_*`. Each draw appends one NdRec to the frame record array in call order. State calls change the native current state (`nd_st`) only.
2. At `nuGfxTaskStart` with the swap flag (3ds/src/pm_nusys.c:95 [C]): `nd_frame_end()` closes the record array.
3. `nd_emit(eye 0)`: one loop over the records writes PICA words into the citro3d command buffer.
4. Stereo: `nd_eye_copy()` copies the eye 0 span and patches the matrix words and the render target registers (section 4.8).
5. C3D_FrameEnd as today (pm_gpu_end, pm_gpu.c:2050 [C]).

### 2.2 Record format (NdRec, 32 bytes, aligned 32, one cache line)

| Offset | Field | Size | Meaning |
|---|---|---|---|
| 0 | kind | u8 | DRAW_IDX (indexed static mesh), DRAW_RING (dynamic vertices), RECT (fill rectangle), TEXRECT, CLEAR, TARGET (render target change), SCISSOR, LEGACY (bridge only, deleted in S10) |
| 1 | tmpl | u8 | template id 0 to 255 |
| 2 | tex0 | u8 | texture unit 0 block id (0 = none) |
| 3 | tex1 | u8 | texture unit 1 block id (0 = none) |
| 4 | mtx | u16 | matrix slot in the frame matrix pool |
| 6 | unif | u16 | per-draw uniform block id (uvso, cmul, cadd, zmix, fog, prim and env constants). 0 = the template default |
| 8 | idx_word | u32 | index buffer address, already in the GPU register form (0x80000000 or offset) |
| 12 | nidx | u16 | index count |
| 14 | vbuf | u16 | attribute buffer id (0 = dynamic ring, 1 = static pool) |
| 16 | sort | u32 | sort key: tmpl << 24, tex0 << 16, tex1 << 8, layer |
| 20 | seq | u32 | game order index. Bit 31 = ordered (blend, decal or depth off: keep game order) |
| 24 | aux | u32 | rectangle corners (packed s16 pairs), or legacy span index |
| 28 | aux2 | u32 | second rectangle word, or the light block id |

- The record holds GPU-ready words (idx_word) so the loop does no address math [U].
- The existing 16-byte NatRec (pm_native.h:15-24 [C]) is too small for tex1, unif, sort and seq without a side table. A side table is pointer chasing (ARM11 rule 1 below). So the record grows to one full line.

### 2.3 Templates (NdTmpl)

- Key (24 bytes): combine w0 and w1, othermode H and L, geometry mode, a flag byte (textured, t1only, two-cycle, lit, fog, ordered).
- Block: the PICA words for the TEV stages (6 stages), the buffer colour, the alpha test, the depth test and depth map, the early depth, the stencil, the blend, the cull, the shader program and attribute set. Size cap 128 words (512 bytes), aligned 32. The existing capture measured 234 to 278 template words for one draw [C pm_native.h:11]. That count includes uniforms that move to the per-draw block, so 128 words is a target [U].
- Built by `nd_tmpl_build(key)`. Inputs: pm_comb_build (pm_comb.c:393 [C]) for the TEV plan. The render state rules move from the walker (sync_render pm_gbi.c:650, rm_apply pm_gbi.c:2838, the sh_* setters and flush_tev_rebuild in pm_gpu.c) into word writers in nd_tmpl.c. This is moved code, so the RDP quirks stay (section 7).
- Table: 256 templates, open hash on the key, 512-entry index [U]. Built at first use, kept for the map. A frame uses 23 to 53 distinct keys [M agent_nds1 census_toad 23, census_mac01 52 to 53].

### 2.4 Textures and palettes

- `nd_tex_desc` (16 bytes): raster pointer, palette pointer, format, size, width, height, wrap and mirror per axis, mask and shift per axis, palette index. This is what make_texture_gfx and the gDPLoadTexture* macros pass. No TMEM emulation is needed for the compound macros.
- `nd_tex_get(desc)` returns a texture block id. The block is 4 words per unit: dimensions, parameters (filter, wrap), LOD or border, GPU address. These are the words pm_tex_slot_words gives today (TEXW: 56 of 57 verified against live flush words [M nds1_status_2026-11-05.md]).
- The texture cache stays pm_tex.c: content-hash keys, 1024 slots (pm_tex.c:21 [C]), evict only on allocation failure, the current and the previous frame protected (pm_tex.c:831 [C]), TEXF1 pending frees (pm_tex.c:640 [C]).
- Palettes: the TLUT stays part of the texture key (TX1 converts CI through the palette at upload). A palette write changes the content hash, so a stale palette cannot be bound (plan_dl_design.md path 1 risk).
- Per frame, a texture lookup is one hash probe on (raster, palette, format, size, w, h). The decode runs only on a cache miss.

### 2.5 Matrices and lights

- `nd_mtx_push(const Mtx* m, flags)` and `nd_mtx_pop()` keep a 16-deep stack in native memory (same depth rule as G_MTX). The N64 fixed-point Mtx is unpacked once per matrix with the same function as today (unpack, pm_gbi.c:451 [C]). Reason: bit-equal positions with the walker during the bridge and the same seam rule (section 13).
- Composition: one fp32 product per draw matrix on the CPU, same inputs and same order as today: pcomp = post x game (pm_gpu.c:230 SEAM comment [C]). The result is stored once per slot in the frame matrix pool (64 bytes, fp32 4x4). The PICA converts fp32 to fp24 at uniform load.
- Normal matrix rows (3 x vec4) only for lit templates (pm5 VS layout, 3ds/shaders/pm5.v.pica [C]).
- fps60 interpolation: `nd_mtx_push` calls pm_interp_find(m) (pm_interp.c:117 [C]) as load_mtx does today. The key-frame replay (pm_nusys.c:128 [C]) re-emits the same records with the second matrix set. Matrix slots hold both values [U, row S1-5 reads pm_interp.c in full first].
- Lights: `nd_lights(const Lightsn*, n)` writes one light block (lamb, lcol[7], ldir[7], VS layout pm5.v.pica:9-12 [C]). A record refers to the block by id. The block is uploaded only when the id changes in the emit loop.

### 2.6 Vertex storage

- Format: the N64 16-byte Vtx, unchanged: s16 x y z, s16 flag (palette index), s16 s t, u8 rgba (or normal). The loaders already read it as GPU_SHORT 4, GPU_SHORT 2, GPU_UNSIGNED_BYTE 4 (pm_gpu.c:1376-1378 [C]). Shape bytes need no conversion (design_producer_models.md 0.3).
- Static pool: linear memory, one base address, 65536 vertices (1 MB), u16 indices. Map shapes, entity lists, effect lists and battle lists compile into it. The existing DLC2 static area (GSTAT_VTX 16384, pm_gpu.c:116 [C]) is the seed of this pool. Flag 0 on every static vertex. The draw matrix sits in palette entry 0 (design_producer_models.md 0.4).
- Index pool: linear memory, 512 KB of u16 [U]. Index lists are built once, packed with no padding, so adjacent groups of one list form one range (map_e4.md 3).
- Dynamic ring: the existing gring area (GRING_VTX 16384 x 16 bytes, pm_gpu.c:116, 1362 [C]) for imgfx meshes, effect vertices and rectangles. `nd_vtx()` copies 16-byte Vtx with word copies (ldm/stm).

### 2.7 Command buffer build

- The emit loop writes raw words into the citro3d command buffer at its current offset (the copy path, GPUCMD_AddRawCommands is a memcpy [M decision_jump_closed.md]). No PICA jump (section 4.1).
- Per record: if tmpl differs from the last record, copy the template block. If tex0 or tex1 differs, write the texture block words. If mtx differs, write the matrix uniform (header plus 16 words, or 28 for lit). If unif differs, write the uniform block. Then write the index address, the count and the draw trigger.
- During the bridge, after native records the citro3d dirty state is set and the walker shadows (sh_* and tev_valid) are reset (the REC_DIRTY_CAP method of pm_gpu_rec_play and sh_clear [M design_inputs_stateful.md 4]).

### 2.8 Memory budget (O3DS)

The build takes 64 MB of application FCRAM: 4 MB N64 RAM, 40 MB cartridge image, 14 MB linear pool, the rest heap (pm_mem.c:9-21 [C]). So the usable size is 64 MB on the O3DS. The linear pool has a fixed need of 4968 KB plus a 4096 KB texture budget (pm_mem.c:20 and comment [C]).

| Structure | Memory | Bytes | Note |
|---|---|---|---|
| NdRec array | heap | 2048 x 32 = 64 KB | Toad Town uses about 300 [U] |
| Matrix pool, 2 eyes | heap | 512 x 64 x 2 = 64 KB | eye 1 values for the patch |
| Uniform blocks | heap | 512 x 64 = 32 KB | |
| Light blocks | heap | 32 x 240 = 8 KB | |
| Templates | heap | 256 x (512 + 32) = 136 KB | only the used ones are hot |
| Texture unit blocks | heap | 1024 x 16 = 16 KB | one per pm_tex slot |
| NdObj table (compiled lists) | heap | 2048 x 64 = 128 KB | |
| NdObj segments | heap | 8192 x 16 = 128 KB | |
| Static vertex pool | linear | 1024 KB | replaces gstat (256 KB) |
| Static index pool | linear | 512 KB | replaces the nat pool (128 KB) and the rec index arena (128 KB) |
| Dynamic ring | linear | 512 KB (existing gring) | |
| Command buffer | linear | 1.5 MB (existing, C3D_Init 0x180000, pm_gpu.c:1306 [C]) | native emit is about 15 KB per eye [U] |
| Freed in S10 | linear | vert_base 32768 slots (pm_gpu.c:40, 1406 [C]), rec arena, nat pool, gm_tab | the CPU triangle path goes |

Net: about 1.3 MB more linear memory before S10 and less after S10 [U]. Row S1-1 re-measures PM_LINEAR_FIXED_KB with the `mem` log line (pm_mem.c:259 [C]).

### 2.9 Static list compiler (nd_dlc)

- Input: a static Gfx list in asset data (map shape, entity model list, effect list, battle list, HUD static list), plus the segment table at the call when the list uses segment addresses (gSPSegment has 134 call sites [C grep]).
- Output: NdObj = a vertex range in the static pool, an index range, and up to 16 segments. A segment = {state delta (the fields the list sets), texture desc id or none, index sub-range, dep mask (state bits the list reads before it sets them), matrix op}.
- The compiler is the existing dlc_compile and dlc_static_build (pm_dlc.inc.c:145, 219 [C]), extended with the texture, combine, othermode and colour ops. It reads asset words, never walker state. It runs once per (list pointer, segment base, pm_dlc_gen generation). It is cold code (-Os, own file).
- At draw time an NdObj with dep 0 gives final template ids at compile time. An NdObj with dep not 0 merges nd_st with the delta and looks up the template (one hash probe per segment).
- Lists that change per frame (local vertex copies, MODEL_FLAG_HAS_LOCAL_VERTEX_COPY, model.c:2054 [M design_producer_models.md 8.6]) are compiled for their index lists only. Their vertices go through the ring each frame.

## 3. ARM11 design (O3DS: ARM11 MPCore, ARMv6K, 268 MHz, 16 KB L1I and 16 KB L1D per core, 4-way, 32-byte lines, no L2, VFPv2, no NEON, no integer divide)

Sources: hw_guidelines.md, inv_pmu_events.md, plan_icache_rows.md, inv_core_assignment.md, plan_core1.md. Numbers from these are [M]. TRM facts not checked on disk are [U].

1. Layout. NdRec is one 32-byte line, aligned 32. The record array is read in order with `__builtin_prefetch` (PLD) two records ahead. Template blocks are aligned 32 and read only on a template change. Texture unit blocks are 16 bytes (two per line). Matrix slots are 64 bytes (two lines). No pointer in a record: ids index flat arrays.
2. Draw loop. `nd_emit` plus its callees stay under 8 KB of code (target 4 KB [U]). One `switch (kind)` with DRAW_IDX first. Cold paths (LEGACY, TARGET, CLEAR, overflow, split, debug, prof) are `__attribute__((cold, noinline))` in another section. The loop goes in the hot span (.data.pmhot, CF4 span in 3ds/ld/3dsx.ld.in [M plan_icache_rows.md 1]).
3. Data in flight per draw: the record (32 B), the matrix (64 B, on change), the uniform block (64 B, on change), two texture blocks (32 B, on change), the template block (up to 512 B, on change). Sorted runs read each template once per run. Working set per frame: records 300 x 32 = 9.6 KB streamed, plus used templates 50 x 512 = 25 KB read once each [U]. The output command words are a write-only stream.
4. Math. No division, no double. Index addresses, counts and uvso scales are computed at compile time. The CPU does one fp32 4x4 product per matrix slot (VFP, about 64 multiply-adds [U]). The VS does the vertex transform, the lighting and the fog factor (already so in pm5.v.pica [C]). What moves from CPU to GPU: nothing new per vertex. What moves from per frame to load time: DL decode, index list build, texture decode, template build. What leaves the CPU entirely: the CPU triangle path (draw_tri_cpu, pm_gbi.c:1657 [C]) after its cases get a GPU form (risk R6).
5. Cache maintenance. Static pools: one GSPGPU_FlushDataCache per compile (as pm_gpu_gstat_commit does [M design_producer_models.md B5]). Dynamic ring and command words: one flush of the written span per split, in the order: write data, flush span, submit (GX). Today 7 flushes per frame at 26 to 28 us [M design_inputs_stateful.md 6]. Target 2 per frame [U]. No per-record flush.
6. Cores. Core 0 runs the game, the list build and the walk. Core 1 runs audio, the audio mixer, the log writer, the flash thread and the watchdog [M inv_core_assignment.md]. A producer thread on core 1 is NO-GO (gain 0 to 2 ms, 4 high-risk races [M plan_core1.md 0, 5]). An emit-only thread on core 1 is allowed only after S10, when records are immutable at frame end. Cost: one event per frame, the records read from FCRAM by core 1 (the L1 is per core), core 1 shared with the mixer under the APT time limit (pm_log.c:732 [C], value not read). Reopen rule: prof shows emit above 3 ms after S10 (row S11-3).
7. Word access. Command words are u32 stores in order. Template copies use aligned memcpy (ldm/stm, hw_guidelines rule 3). Record fields are read as two 32-bit words where possible. No byte stores in the emit loop. No unaligned access. LDRD/STRD need 8-byte alignment on ARMv6 [U, check the TRM before use].
8. FPU mode. Check FPSCR RunFast (flush-to-zero, default NaN) at boot, as the earlier AC plan asked [U, not checked in this repo].
9. Per-slice ARM11 gate (section 10): hot loop bytes from the map file, instructions per DRAW_IDX record from objdump (target 120 or less for a record with no state change [U]), and the PMU counters through `prof` (set m: I-cache miss and D-cache read miss, set s: instruction-fetch stall). Baseline: I-cache misses about 151,000 per frame, stall share 0.41 to 0.43 (phase on) [M inv_pmu_events.md, plan_icache_rows.md 0]. Target after S10: stall share 0.15 or less, I-cache misses 30,000 or less per frame [U].

## 4. PICA design

Sources: research_pica_jump.md, research_pica_jump2.md, research_sm64_3ds_jump.md, inv_citro3d_format.md, decision_jump_closed.md, plan_dl_design.md, plan_core1.md. 3dbrew pages returned HTTP 403 to earlier agents [M research_pica_jump.md], so register facts below are from the libctru and citro3d headers in the docker image or are [U].

1. Command buffer reuse. Prebuilt blocks per template and per compiled object are copied into the frame buffer (memcpy). Verified: the copy path (GPUCMD_AddRawCommands is a memcpy) [M decision_jump_closed.md]. Not verified: GPUREG_CMDBUF_JUMP0/1 (0x23C, 0x23D [M research_pica_jump2.md registers.h 597-602]). The jump stopped the console after a map load in every variant JT5 to JT11. Azahar passed [M decision_jump_closed.md]. So no jump or call is used. Question Q5 asks the user if a new jump probe is wanted.
2. State changes. Records are sorted by (tmpl, tex0, tex1) inside each run of unordered records. Ordered records (blend, decal, depth off) keep game order (the existing order rule, nat_host_test test_order [C]). A template block is written only on a template change. The emit loop compares one byte per field.
3. Vertex data. 16-byte packed Vtx (short and byte attributes) [C pm_gpu.c:1376-1378]. s16 values are exact in fp24 (16-bit mantissa) [M glittery-nautilus seam rules]. Indexed draws (u16) for all static meshes. Pools in linear memory, aligned 16 or more. Batches: adjacent segments with the same template, textures, matrix and uniforms merge into one draw (the live grange rule, pm_gpu.c:2314-2321 [M diag_e53_merge.md 2.2]). Draw is 43 percent of a close on the console [M map_rtb1.md 0], so fewer draws is the main PICA lever.
4. Vertex shader. No geometry shader. Per draw upload: one matrix (header plus 16 words, plus 12 for lit). Lights and fog uniforms only on a block change. Lighting and fog stay in the VS (pm5.v.pica), combine and the fog mix in TEV (V06 last stage, pm_gpu.h comment [C]).
5. Textures. The TX1 converter gives RGBA8, RGBA5551, LA8, LA4, L8 in 8x8 Morton tiles at conversion time [M tex_conv_test.py header]. ETC1 is not used (lossy, the fidelity rule allows only one-pixel differences, question Q6). Mipmaps stay as today. 6 TEV stages and 3 texture units: unit 2 is the noise texture (PM_S_TEX2, pm_gpu.h [C]). A combine that needs more than 6 stages (pm_comb_build rc 1) is counted per frame. No two-pass draw is added.
6. Fill rate. The game draws its own full-screen fades and backgrounds. No draw is skipped (no-culling rule). The scissor is the game scissor. Early depth stays as set today (sh_early, pm_gpu.c:594 [C]). Render targets are RGBA8 with DEPTH24_STENCIL8 (pm_gpu.c:1316, 1331 [C]). A change to RGBA5551 is a user decision (Q7).
7. CPU and GPU overlap. Today C3D_FrameBegin waits for the GPU tail of the last frame. The tail is about 6.6 ms and hidden by logic and list build [M plan_core1.md 5]. When the CPU part shrinks, the wait grows. So row S11-2 adds a second command buffer and a second vertex ring (GPU pipeline depth 2) if prof shows gwait above 2 ms. GX queue: 32 entries, pm_gx_room drains at 24 (pm_gpu.c:1830-1840 [C]).
8. Stereo. Both eyes use the same vertex, index and texture data. Eye 1 = memcpy of the eye 0 command span plus patches of the matrix words and the render target registers (0x110 to 0x11E [M glittery-nautilus stereo]). Ortho draws get target patches only.
9. Per-slice PICA gate: draws per frame, template changes per frame, texture block writes, vertices and triangles per frame (nd counters in the prof line), and the GPU busy time. GPU busy probe (row S0-6): in a prof sample frame, take the tick at the last submit and at the P3D event, so `gb` = GPU busy ms. Targets: draws not above the old path (Toad Town 47 to 77 closes per frame [M inv_frame_cost.md 3, plan_icache_rows.md 1]), triangles equal (852 static plus 202 walker per frame [M plan_dl_design.md 1]), template changes not above the distinct count, GPU busy under 8 ms per eye for 60 fps in 3D [U].

## 5. Producer API (what replaces each gSP and gDP family)

The API is a set of typed C functions in 3ds/include/nd.h. A game file opts in on the port side: 3ds/CMakeLists.txt adds `-include nd_gbi.h` to that file only (set_source_files_properties). nd_gbi.h redefines the dynamic gSP* and gDP* macros (not the gs* static macros) as nd_* calls. The decomp text stays as it is where the redirect is enough. This follows the user rule against hand-written stand-ins for decomp code (memory feedback_no-stubs). Section 6a lists the places where a game source edit is needed.

| N64 macro family | Native call | Note |
|---|---|---|
| gSPMatrix, gSPPopMatrix, gSPForceMatrix | nd_mtx_push(const Mtx*, flags), nd_mtx_pop(n) | projection and modelview stacks |
| gSPViewport | nd_viewport(const Vp*) | into the post matrix as today |
| gSPDisplayList(static asset list) | nd_obj_draw(nd_obj_get(list)) | compiled once (2.9) |
| gSPDisplayList(list built this frame), gSPBranchList, gSPEndDisplayList | nd_range_begin(), nd_range_end(), nd_range_repeat(id) | floor reflection repeats a range (model.c:4960-4995 [C]) |
| gSPVertex, gSP1Triangle, gSP2Triangles, gSP1Quadrangle | nd_vtx(ptr, n, v0), nd_tri(a, b, c), nd_tri2 | ring vertices, merged into one draw per state run |
| gSPModifyVertex | nd_vtx_modify(slot, what, value) | 4 sites (underwater) [C grep] |
| gSPSetGeometryMode, gSPClearGeometryMode, gSPLoadGeometryMode | nd_geom(clear, set) | state field |
| gDPSetCombineMode, gDPSetCombineLERP | nd_combine(w0, w1) | key field, TEV plan at template build |
| gDPSetRenderMode, gDPSetOtherMode, gDPSetCycleType, gDPSetTextureFilter, gDPSetTextureLUT, gDPSetAlphaCompare, gDPPipelineMode, gDPSetTexturePersp, gDPSetTextureLOD, gDPSetTextureDetail, gDPSetDepthSource, gDPSetColorDither, gDPSetAlphaDither, gDPSetCombineKey, gDPSetTextureConvert | nd_othermode(hmask, h, lmask, l) | key field |
| gDPSetPrimColor, gDPSetEnvColor, gDPSetFogColor, gDPSetBlendColor, gDPSetPrimDepth, gSPFogPosition, gDPSetConvert | nd_color(slot, rgba), nd_primdepth(z, dz), nd_fog(min, max) | per-draw uniform block, not template |
| gDPLoadTextureBlock (all variants), gDPLoadTextureTile, gDPLoadTLUT_pal16, gDPLoadTLUT_pal256, gDPLoadMultiBlock | nd_tex_load(unit, &nd_tex_desc) | one call per compound macro |
| gDPSetTextureImage, gDPSetTile, gDPLoadBlock, gDPLoadTile, gDPLoadTLUTCmd, gDPSetTileSize used one by one | nd_timg, nd_tile, nd_load, nd_tile_size | small tile model (moved tex_op logic, pm_gbi.c:3010 [C]) |
| gSPTexture | nd_texture(s, t, level, tile, on) | |
| gSPSetLights*, gSPLight, gSPLookAt | nd_lights(const Lightsn*, n), nd_lookat | light block |
| gDPFillRectangle | nd_fill_rect(ulx, uly, lrx, lry) | N64 inclusive rule in FILL and COPY only (pm_gbi.c:3449 [C]) |
| gSPTextureRectangle, gSPScisTextureRectangle, gSPTextureRectangleFlip | nd_tex_rect(...) | inclusive lower right (pm_gbi.c:3371-3380 [C]) |
| gDPSetScissor | nd_scissor | |
| gDPSetColorImage, gDPSetDepthImage | nd_target(role) | off-screen pass, colour buffer capture, depth query (pm_gbi.c:243 img_role [C]) |
| gSPSegment | nd_segment(id, base) | segment table for compiled lists |
| gDPPipeSync, gDPLoadSync, gDPTileSync, gDPFullSync, gSPClipRatio, gSPPerspNormalize, gDPNoOp | nothing | |
| Port ops G_PM_MODEL, G_PM_TEX, G_PM_TEXI, G_PM_RM (include/gbi_custom.h:204-220 [C]) | deleted with their producers | |

### 5.1 Producer families

Counts are source lines with dynamic gSP/gDP calls, static gs* lines, and gSPDisplayList calls [C grep of src/, this plan]. Per-frame cost: [M] where a log gives it, else to be read from the `prod` line (pm_gbi.c os_report, top 8 producers) in row S0-7.

| Family | Files | gSP/gDP lines | gs lines | DL calls | Per frame cost | Native calls | Slice |
|---|---|---|---|---|---|---|---|
| Frame state, background | src/main_loop.c, src/cam_main.c (render_frame 118), src/background_gfx.c (gfx_init_state 35, gfx_draw_background 336), src/background.c | 5, 26, 73, 14 | 0 | 2 | small [U] | nd_pass_begin, nd_viewport, nd_scissor, nd_clear, nd_tex_rect | S2 |
| Static map models | src/model.c (appendGfx_model 1384, render_transform_group 3264, execute_render_tasks 4888, mdl_load_all_textures 2472), 3ds/src/pm_map.c | 141 | many in shapes | 8 | 0c class 19.7 to 21.1 ms, 90 plain models at 230 us [M plan_dl_design.md 1] | nd_obj_draw, nd_mtx_push, nd_lights, nd_tex_load, nd_range_* | S3 |
| Sprites, NPCs, player, partners | src/sprite.c (spr_appendGfx_component 339, _flat 227), src/imgfx.c (imgfx_appendGfx_mesh 285), src/sprite_shading.c, src/npc.c (render_npcs 963), src/77480.c (render_player 1468) | 33, 107, 22 | | 1, 3 | part of 0b class 3.2 to 6.8 ms [M] | nd_tex_load, nd_vtx, nd_tri, nd_mtx_push | S4 |
| HUD, text, boxes, windows | src/hud_element.c (draw_rect 265), src/msg_draw.c (msg_draw_char 1669, prim_rect 1818, frame 2073), src/msg.c, src/draw_box.c (338), src/windows.c (370), src/draw_img_util.c, src/status_icons.c, src/status_star_shimmer.c, src/inventory.c | 122, 112, 30, 74, 9, 21, 13, 11 | in 35 other files: 1852 | 2, 2, 2 | part of 0b class [M]. Rest to read from prod | nd_tex_rect, nd_fill_rect, nd_tex_load, nd_vtx | S5 |
| Entities, items, animators, shadows | src/entity_model.c (appendGfx_entity_model 244, draw_entity_model_A..E 440-592), src/entity.c (render_entities 340), src/entity/*.c (74 files), src/item_entity.c (render_item_entities 1121), src/animator.c (render_animated_model 718) | 95, 6, 164, 110, 36 | 5262 | 48, 21, 10, 16 | not split [U] | nd_obj_draw (segment-keyed), nd_mtx_push, nd_tex_load | S6 |
| World (area-specific gfx) and battle | src/world/** (45 files), src/battle/** (9 files with dynamic calls, 11 with gs) | 409, 72 | 1663, 281 | 36, 7 | not split [U] | as models and effects | S7 |
| Effects | src/effects/*.c (127 files), src/effects.c (render_effects_scene 117, render_effects_UI 143), src/effect_utils.c | 1698, 8, 17 | 70 | 370 | not split [U] | nd_obj_draw, nd_color, nd_mtx_push, nd_vtx | S8 |
| Screen overlays, menus, title, transitions | src/screen_overlays.c (render_screen_overlay_frontUI 492), src/screen_render_util.c, src/curtains.c (render_curtains 239), src/state_title_screen.c, src/state_logos.c, src/pause/*.c, src/filemenu/*.c, src/43F0.c, src/8a860_len_3f30.c, src/speech_bubble.c, src/i_spy.c, src/inspect_icon.c, src/pulse_stone.c, src/level_up*.c, src/starpoint*.c, src/main.c, src/evt/virtual_entity.c, src/os/nusys/nugfxinit.c | 71, 70, 13, 46, 19, 70 (pause), 60 (filemenu), 11, 11, 3, 3, 3, 3 | | 16, 2, 5, 3, 3 | pause and file select only [U] | nd_fill_rect, nd_tex_rect, nd_target, nd_obj_draw | S9 |

Total: 3931 dynamic lines in 246 files, 581 gSPDisplayList calls [M glittery-nautilus evidence, recounted here: effects 1698 in 127 files, 370 DL calls].

### 5.2 Rules for a file to be ported

1. Every draw of the file goes to gMainGfxPos in call order, or the file uses nd_range_* for the lists it builds for later. Files with deferred lists (model.c custom gfx, execute_render_tasks reflection, animator.c, hud_element.c with gSPEndDisplayList) get explicit nd_range edits under TARGET_3DS.
2. A list pointer passed to gSPDisplayList is static asset data, a compiled NdObj, or a range id. A list that the game writes at run time into a buffer that another file calls is ported together with its caller in one row.
3. The redirect covers every macro the file uses. A macro with no nd_* form stops the compile (`#error` in nd_gbi.h), so a missing case cannot draw wrong.

## 6. Interim bridge and its deletion

- Purpose: un-ported files keep drawing while the families move one slice at a time. The walker is not the long-term path. It exists only as a callee for LEGACY records.
- Mechanism:
  1. Each nd_* draw call first calls `nd_bridge_cut()`. If gMainGfxPos moved since the last cut, it appends a LEGACY record {start, end} for the words that un-ported code wrote.
  2. nd_gbi.h in its bridge form dual-writes every STATE macro: it calls the nd_* setter and also writes the N64 word to gMainGfxPos. So the walker sees all state that ported code sets (matrices, render modes, colours, viewport). Draw macros write no word.
  3. nd_emit meets a LEGACY record: it closes the native span, calls pm_gbi_run(start, bytes) and pm_gpu_flush, then resets the walker shadows. In stereo, the eye 1 pass re-runs the LEGACY spans (as gbi_task does today, pm_nusys.c:81 [C]).
  4. A counter `nd_unset_use` (cold) counts native draws that read a state field no native call set this frame. Gate: 0 in the slice scenarios. It catches state that un-ported code sets for ported code.
- Bounds: the bridge has 3 parts: the LEGACY record kind, the dual-write half of nd_gbi.h, and the LEGACY arm of nd_emit. Nothing else depends on the walker.
- Deletion step: slice S10 (NDDEL), rows S10-1 to S10-4. It deletes pm_gbi.c, pm_dlc.inc.c (already gone in S3), the walker parts of pm_gpu.c (flush_draw, grange, the CPU triangle path, vert_base), the G_PM_* ops in include/gbi_custom.h, the dual-write half of nd_gbi.h, the LEGACY kind and the second-eye walk in pm_nusys.c. S10 starts only when the LEGACY record count is 0 in every catalog scene (counter in the prof line).

## 6a. Game source edits

Rule: prefer the port side (3ds/src, 3ds/include/nd_gbi.h, the CMake force-include). Edit src/ or include/ only where the port side cannot give the right order or the right data. Every game edit sits inside `#ifdef TARGET_3DS`. The decomp build files exist at the repo root (build.ninja, configure [C ls]). Whether the N64 build still builds and matches today is not checked [U]. So the rule is: an edit must not change the preprocessed text of the file without TARGET_3DS. Host test for every game edit (nd_n64_same.sh, row S0-3): `gcc -E -UTARGET_3DS` of the file before and after the edit, with the decomp include paths, gives the same output.

| Family | Game source edit needed | Files and functions | What the edit does | Why the port side is not enough | Risk |
|---|---|---|---|---|---|
| Frame state, background | no | src/main_loop.c, cam_main.c, background_gfx.c | none. Force-include only. nd_frame_end runs in pm_nusys.c | the macros and nuGfxTaskStart cover it | - |
| Shadows | yes | src/entity/Shadow.c:28-50 | the static lists lose the port header gsSPPmTexInline and go back to the plain N64 words | the header is a port op that S6 deletes. nd_dlc compiles the plain words | 1 (lowest) |
| Sprites | yes | src/sprite.c:236-279 (G_PM_TEXI blocks), sprite.c:335 SPR_RM_LIST | the TARGET_3DS header words become one nd_tex_load call and nd_obj_draw for the render mode list | these blocks are port code that writes port ops. They go with the walker | 1 |
| HUD | yes | src/hud_element.c:365-402, 477-600 (G_PM_TEXI blocks) | as sprites | as sprites | 1 |
| Text | yes | src/msg_draw.c:1758-1802 (glyph G_PM_TEXI) | nd_glyph(sheet, palette, index): one call to the glyph atlas (GA1 atlas, pm_gbi.c:2541 [C]) | the atlas lookup is walker code today. A typed call removes the decode | 2 |
| Entity models | yes | src/entity_model.c:282, 370 (G_PM_RM rewrite) | delete the rewrite. The render mode list becomes nd_obj_draw | port op deleted | 1 |
| Map models: header and texture op | yes | src/model.c:176-187 (MODEL_RM_OP, MODEL_TEX_OP), 1513-1521 (G_PM_MODEL header), 2072-2073 (count) | MODEL_TEX_OP becomes nd_tex_handle(handle index). The header goes | port ops deleted. The handle index is the texture key the producer already has | 2 |
| Map models: texture handle descriptor | yes | src/model.c load_texture_impl (2109-2160, the pm_texbind_set funnel [M design_inputs_stateful.md 2]) | one TARGET_3DS call nd_tex_handle_set(index, header, raster, palette, aux) after the handle is built | the descriptor is known only there. Without it, each frame decodes the handle gfx list | 2 |
| Map models: floor reflection | yes | src/model.c execute_render_tasks 4960-4995 | the savedGfxPos branch trick becomes nd_range_begin, nd_range_end, nd_range_repeat with the flip matrix | the trick patches a branch word into the list. With no list there is nothing to patch | 3 |
| Map models: custom gfx | yes | src/model.c build_custom_gfx (4066 [M DIS 1c]) and the gSPDisplayList sites 2016, 2063 | the builder output is recorded as a range id per slot. The call sites repeat the range | the builders run before the draw. Under the redirect they would draw at build time, in the wrong order | 3 |
| Map models: projected texture coords | maybe | src/model.c mdl_project_tex_coords 4339 | writes a gfx word or Vtx tc values. Row S3-4 reads it first and decides | not read in full by this plan | 3 |
| Animated models | maybe | src/animator.c appendGfx_animator 779, appendGfx_animator_node 851 (8 local gfx pointers, 1 gSPEndDisplayList [C grep]) | lists built per node for later become ranges | same order problem as custom gfx | 3 |
| Imgfx | no | src/imgfx.c | none | gMainGfxPos only (122 uses, 1 local pointer [C grep]) | - |
| World areas, battle | maybe, per file | src/world/**, src/battle/** | only files with deferred lists or custom gfx builders. S7-1 lists them by grep | most write gMainGfxPos directly | 2 |
| Effects: plain | no | src/effects/*.c | none | force-include covers them | - |
| Effects: splash markers | yes | src/effects/water_splash.c (PM_SPLASH_ON and _OFF G_NOOP words, gbi_custom.h comment [C]) | two nd_splash(on) calls | the marker is a port word for the walker | 2 |
| Effects and overlays that read frame or depth buffers | yes | the files that call gDPSetColorImage to a texture or read the colour buffer (S8-1 and S9-1 list them by grep) | nd_target(role) with the role the walker decides today in img_role (pm_gbi.c:243 [C]) | the role is decided from addresses inside the walker. A typed role is clearer, but the call site must name it | 4 (highest) |
| Screen overlays, curtains, pause, file menu | maybe | src/screen_overlays.c, curtains.c, pause/*.c, filemenu/*.c | only the frame buffer roles above | as above | 4 |

Rank by risk (lowest first): Shadow.c, sprite.c, hud_element.c, entity_model.c (port blocks only), then msg_draw.c, model.c texture handle, splash markers, world and battle deferred lists, then model.c floor reflection, custom gfx and animator ranges, then frame and depth buffer roles. Each edit is one row item in plan_native_slices.md with its own host test (the producer ndref test and the nd_n64_same check).

## 7. Slice order

Slices run one at a time, in this order. One code agent owns one slice. Inside a slice the rows run in order. A file has one owner row at a time. No two slices are open at once, so no two slices edit the same file at once. The file lists below are complete for each slice. Expected ms are phase on from inv_frame_cost.md unless marked. Phase off = phase on x 0.74 to 0.79 [M plan_dl_design.md 2].

| Slice | Name | Files touched | Deleted in the slice | Gate (section 10) | Expected ms saved per frame (Toad Town) |
|---|---|---|---|---|---|
| S0 | NDTEST: test harness and measurement | 3ds/src/pm_gpu.c (ndref hook, cold), 3ds/src/pm_phase.c (prof fields), 3ds/tools/nd_*.py, nd_host_test.sh, nd_size_test.sh, 3ds/tests/ndref/, azt_scenarios.json, test_catalog.md | nothing | host tests run and fail for the missing nd code. ndref files committed. prof line prints the new fields | 0 |
| S1 | NDCORE: native core and bridge | new 3ds/include/nd.h, 3ds/src/nd_core.c, nd_tmpl.c, nd_tex.c, nd_mtx.c. 3ds/src/pm_nusys.c (frame driver), 3ds/src/pm_native.c and 3ds/include/pm_native.h (deleted, eye patch moved), 3ds/src/pm_gbi.c and pm_dlc.inc.c (learn and replay deleted), 3ds/CMakeLists.txt | learn-and-replay (diag_e53_merge.md option A, A1 to A6), nat_tmpl_strip capture path | host: template words, texture words, eye patch, record layout, size test. Picture equal (all drawing is LEGACY) | 0 (bridge cost about 0.1 ms [U]) |
| S2 | NDFRAME: frame state and background | src/main_loop.c, src/cam_main.c, src/background_gfx.c, src/background.c, new 3ds/include/nd_gbi.h | the frame-start words of render_frame (they become nd_pass_begin) | host: ndref background draws equal. Picture equal on title and yard | 0 to 0.3 [U] |
| S3 | NDMODEL: static and dynamic map models | src/model.c, 3ds/src/pm_map.c, new 3ds/src/nd_dlc.c, 3ds/src/pm_dlc.inc.c (deleted), 3ds/src/pm_gbi.c (G_PM_MODEL, G_PM_TEX, G_PM_RM arms deleted), include/gbi_custom.h | G_PM_MODEL, G_PM_TEX, G_PM_RM, mg_run, rec_try, rec_done, dlc_try, the dlc census | host: ndref model draws equal. Picture equal on toad, yard, forest_mim. Seam count not up | 7.9 to 12.7 realistic, 20.3 ceiling [M inv_frame_cost.md 3, table 1 class 0c] |
| S4 | NDSPRITE: sprites, NPCs, player | src/sprite.c, src/imgfx.c, src/sprite_shading.c, src/npc.c, src/77480.c | G_PM_TEXI in sprite.c | host: ndref sprite draws equal. Picture: toad NPC strip, battle_start | part of 3.2 to 5.4 (0b class path 1 [M inv_frame_cost.md 3]) |
| S5 | NDHUD: HUD, text, boxes, windows | src/hud_element.c, src/msg_draw.c, src/msg.c, src/draw_box.c, src/windows.c, src/draw_img_util.c, src/status_icons.c, src/status_star_shimmer.c, src/inventory.c | G_PM_TEXI in hud_element.c and msg_draw.c | host: ndref HUD and text draws equal. Picture: text_menu, house_sign, ui1_choice | rest of 3.2 to 5.4 (0b) plus the texrect ops [U, prod line] |
| S6 | NDENT: entities, items, animators | src/entity_model.c, src/entity.c, src/entity/*.c, src/entity/Shadow.c, src/item_entity.c, src/animator.c | G_PM_TEXI in Shadow.c, G_PM_RM in entity_model.c | host: ndref entity draws equal. Picture: coin_kmr03, toad_gate | 1 to 3 [U, prod line] |
| S7 | NDWORLD: world areas and battle | src/world/**/*.c, src/battle/**/*.c | the world and battle words | host: ndref battle draws equal. Picture: battle_start, battle_menu_bar | not priced [U, prod line in battle] |
| S8 | NDFX: effects | src/effects/*.c in 5 rows, src/effects.c, src/effect_utils.c | PM_SPLASH words become nd calls | host: ndref effect draws equal. Picture: nok11_splash, chapter_wide, bowser_intro_fire | 1 to 4 [U, prod line] |
| S9 | NDSCREEN: overlays, menus, title, targets | the S9 files of table 5.1 | the off-screen, capture and depth-query roles of the walker | host: ndref pause and file select draws equal. Picture: pause_spin, fileselect_rotate, file_select, intro_native, merlon_door | small, pause and file select only [U] |
| S10 | NDDEL: delete the walker | 3ds/src/pm_gbi.c (deleted), 3ds/src/pm_gpu.c, 3ds/src/pm_nusys.c, include/gbi_custom.h, 3ds/include/nd_gbi.h, 3ds/ld/3dsx.ld.in, 3ds/CMakeLists.txt | pm_gbi.c, flush_draw, grange, CPU triangle path, dual-write, LEGACY, eye-2 walk | full catalog once (epic merge rule 5). Console prof. Hot loop 8 KB or less | the remaining walker part of 32.8 to 39.8, and the second walk in 3D (about 2x the walk [M glittery-nautilus stereo]) |
| S11 | NDTUNE: after measurement only | 3ds/src/nd_core.c, pm_gpu.c, pm_main.c | nothing | console prof | GPU pipeline depth 2, core 1 emit, by reopen rules only |

Expected end state (phase off, Toad Town): logic 3.3 + list build 4.4 + native emit 1 to 2 + swap and rest 1.5 = 10 to 11 ms CPU [U]. The GPU busy time (unknown today) then decides 60 fps (S0-6 measures it).

## 8. Process rules for agents

1. One code agent at a time (CLAUDE.md rule 2). Haiku writes code. Opus writes the row prompt and, for S3 and S8, re-reads the line anchors before the launch.
2. Task size: one row = 3 to 5 numbered edit items. Each item is 30 changed lines or less and names its file, its function and the current line anchor. The rows in plan_native_slices.md follow this size.
3. TDD order inside a row (section 9): (a) the host test of the row is written and fails, (b) the code edit, (c) the host test passes, (d) the size test passes, (e) build, (f) the one Azahar shot of the row at the end of the slice.
4. A failed gate. First open and view the two pictures (region_diff output and both PNGs). Then decide:
   - Implementation fault: the picture shows one object wrong (missing, wrong texture, wrong blend) and the host stream diff names the field. Fix inside the row. At most 2 fix tries.
   - Method fault: the picture is right but a count gate fails because of the design (for example batches above the old path because the native batching splits where the walker merged), or the same fault stays after 2 tries, or the fix needs walker state as input. Stop the row. Report to the planner. The planner changes the design. Code of the row is reverted.
   - Test fault: the ndref reference or the decoder is wrong (for example the reference was captured in a mixed learn window). Prove it with the stream diff and the picture. Fix the tool, not the code.
5. Stop conditions: two unchanged shots of the same quantity (CLAUDE.md rule 5). A gate that needs walker output at run time. Hot loop above 8 KB after the cold split. Console prof at the end of a slice shows less than 25 percent of the expected ms. A crash or hang at a map load. A row that needs a game source edit outside a TARGET_3DS block, or an edit that section 6a does not list (the planner adds it first).
6. Commits: one commit per passed row (rule 7), the agent's files only, never 3ds/src/map_restore_table.c. A failed row is reverted before the next row.
7. Handoff: each slice ends with docs/handoff/agent_<slice>.md (state, commits, open items, next row, commands).

## 9. TDD

### 9.1 What the existing host tests cover

| Test | Covers | Does not cover |
|---|---|---|
| 3ds/tools/nat_host_test.sh (nat_host_test.c, pm_native.c, pm_comb.c, fixture nat_fixture_toad.txt) | NatRec size and offsets, template plan equals pm_comb_build for 9 census pairs, order rule (stable sort inside unordered runs), split of 2 captured records into template and group, eye copy equals direct eye 1 emission, bake pool packing [C nat_host_test.c:24-235] | templates from a key (it splits captured words), texture unit 1, any producer, batching, triangles |
| 3ds/tools/ndt_host_test.sh (ndt_host_test.c, pm_comb.c) | combiner against the N64 cycle formula, matrix product order, fp24 dp4 rounding, record packing [C ndt_host_test.c:29-213] | shared-edge cracks on real map data, depth and decal values, any emit |
| 3ds/tools/tex_conv_test.sh (tex_conv_drive.c, tex_conv_test.py) | TX1 texel conversion for I4 to RGBA32 into PICA formats and Morton tiles, written from the formats [C tex_conv_test.py:1-14] | wrap words, palette changes over time, slot words |
| 3ds/tools/tx1_test.sh, texbake_test.sh | TX1 and texbake decode against the CPU path extracted from pm_tex.c [C] | GPU words |
| 3ds/tests/au1f, au2, dspdec | audio only | draw |

Host compiler: /c/msys64/mingw64/bin/gcc (64-bit) [C]. /c/msys64/mingw32 has no gcc [C]. Game producer code casts pointers to u32. So a host build of game files needs a 32-bit compiler (question Q2). The ARM compiler is in the docker image pm3ds-build:1 [C docker images].

### 9.2 Reference draw stream (ndref)

- Captured once, in S0, from the old walker build at a pinned commit (S0 start HEAD). It is a test oracle. It is not read by the game, it is not a design input, and it is not golden.py or statehash.
- Content per draw (at each flush_draw close that draws, pm_gpu.c:952 [C]): template key (combine w0 w1, othermode h l, geometry), z, decal, blend, atest, cull, scissor, the unit 0 and unit 1 texture words (pm_tex_slot_words for both units), TEV plan id, uniforms (uvso, cmul, cadd, zmix, fx, fog), the composed matrix (16 floats), the triangle list as resolved vertices (16-byte Vtx values, not slot numbers), and the producer tag (pm_gbi_prod_mark and pm_gbi_mdl_mark ids [C pm_gbi.c:1943, 2045]).
- Producer inputs per tagged producer call (for the host replay of the ported function): S3 models: modelID, flags, renderMode, textureID and variation, the Model matrix values, the light block, the shape offset of the DL. Each later slice adds its own input record in its first row.
- Format: binary, little-endian, one header per frame, documented in 3ds/tests/ndref/FORMAT.md, plus 3ds/tools/nd_ref_dump.py. Size: about 25 to 100 KB per scene frame [U]. Committed.
- Scenes: ndref_toad, ndref_yard, ndref_pause, ndref_battle, ndref_text, ndref_title, ndref_splash, ndref_stereo (scenarios added in S0, section 5 of plan_native_slices.md).
- Regeneration: `PM_ROW=S0-2 powershell -File 3ds/tools/azt.ps1 ndref_<scene> -Row S0-2` on the pinned commit, then `python 3ds/tools/nd_ref_dump.py --check`. The pinned commit is recorded in FORMAT.md. After S10 the walker is gone, so references are regenerated only from that commit.

### 9.3 Equality rule (host)

Native stream (decoded from the PICA words that nd_emit writes, by 3ds/tools/nd_pica_decode.c) against the ndref stream, per frame:
1. Same triangles in the same order inside each run of equal state (template key, both texture words, matrix, uniforms). Triangles compare by resolved vertex values.
2. Batches (draw triggers) at most the reference count.
3. Texture words equal for unit 0 and unit 1.
4. Combiner plan, blend, depth, alpha test and cull equal.
5. Draw order may change only inside unordered runs (opaque, depth test on, no decal).

### 9.4 Each row starts with a failing host test

- The row's producer function (for example appendGfx_model) is compiled on the host with nd_core.c and the stubs of 3ds/tools/nd_host_stubs.c. Stubs replace only platform calls (citro3d, GPUCMD, svc, linearAlloc, pm_tex upload). Game logic is not stubbed.
- The test feeds the ndref producer inputs, runs the producer, emits, decodes and compares (9.3). It fails before the row's code and passes after.

### 9.5 Other host tests

| Test | Check |
|---|---|
| nd_size_test.sh | compile nd_core.c (and callees) with the docker arm-none-eabi-gcc, same flags as 3ds/CMakeLists.txt. Sum of `nm -S` sizes of nd_emit and its callees: 8 KB or less. Instruction count of the DRAW_IDX path from objdump: 120 or less [U target] |
| layout asserts | `_Static_assert(sizeof(NdRec) == 32)`, alignment 32 of the record array, template blocks, matrix slots. Vtx 16 bytes |
| eye patch | eye 1 by copy-plus-patch equals a second emit with eye 1 matrices, word for word (extends nat_host_test test_layout) |
| texture parity | tex_conv_test.sh and tx1_test.sh stay. New: nd_tex words equal pm_tex_slot_words for the ndref textures |
| seam | for each pair of models in mac_01 and mac_02 shapes with a shared edge: the native composed matrices and the old matrices give bit-equal fp24 dp4 outputs on the shared vertices. The fp24-truncated positions of the two triangles on a shared edge are identical, so no crack opens. Uses the ndt fp24 code (ndt_host_test.c:159-205 [C]) and the ndref matrices |
| f24 pack | the native fp32 to fp24 uniform pack equals the citro3d pack for random and ndref matrices [U, citro3d source not on disk, inv_citro3d_format.md] |

### 9.6 Azahar and console

- Azahar: once per slice, at the end, for the picture (region_diff viewed) and for crashes. Not for speed.
- Console: the `prof` line at the end of a slice that claims ms (rule 6 push by the orchestrator). Not per row.

### 9.7 New host tools (slice S0 rows)

| Tool | Row |
|---|---|
| ndref hook in the old walker (switch ndref=<frame>) and the input record for models | S0-1 |
| ndref scenarios and committed reference files, FORMAT.md, nd_ref_dump.py | S0-2 |
| nd_pica_decode.c (PICA words to draw stream), nd_host_stubs.c, nd_host_test.sh, nd_stream_cmp.py | S0-3 |
| nd_size_test.sh (docker arm-none-eabi-gcc, nm, objdump) | S0-4 |
| seam host test (nd_seam_test.c) on the shape assets | S0-5 |
| prof fields: nd emit ms, legacy ms, draws, template changes, texture writes, triangles, LEGACY count, gb (GPU busy ms) | S0-6 |
| baseline prod line run on the console (user push) | S0-7 |

## 10. Gates (every slice)

| # | Gate | Pass | Tool |
|---|---|---|---|
| G1 | host tests | all pass, including the row's new test | nd_host_test.sh, nat/ndt/tex tests |
| G2 | picture equal | the agent views both PNGs first. region_diff shows only scattered one-pixel diffs, no whole object missing, extra, wrong texture or wrong blend | 3ds/tools/region_diff.py, azt.ps1 shot scenario of the slice |
| G3 | triangle count equal | native triangles per frame = ndref triangles for the same scene and frame | nd_stream_cmp.py |
| G4 | batches not above the old path | draws per frame at most ndref draws | nd_stream_cmp.py, prof `dr` field |
| G5 | hot loop under 8 KB | nd_emit plus callees 8 KB or less (map file and nd_size_test.sh) | mapsz.py method (plan_icache_rows.md 0) |
| G6 | prof shows the stage ms | the `prof` line (logging off, not debug logs) prints nd, legacy, gbi and gb. The slice's expected ms (section 7) is stated before the run | console push at the slice end |
| G7 | ARM11 | instructions per record, PMU set m and set s against section 3 targets | prof, objdump |
| G8 | PICA | draws, template changes, vertices, triangles, GPU busy against section 4 targets | prof |
| G9 | seam | seam host test passes. Seam shot count of background pixels inside the silhouette union not up (mac01_seam scenario) | nd_seam_test.c, mac01_seam |
| G10 | bridge | nd_unset_use 0 in the slice scenes | prof or log counter |

## 11. Constraints carried

| # | Constraint | How the design honors it | Source |
|---|---|---|---|
| 1 | Stereo: eye 2 is copy-plus-patch of eye 1 records, matrices and target registers only, no second producer pass | nd_eye_copy (2.1 step 4, 4.8). The pm_native eye copy is moved into nd_core.c with its host test | pm_native.c nat_eye_copy, nat_host_test.c test_layout, glittery-nautilus stereo |
| 2 | No culling beyond the game's own (back-face, entity distance, MODEL_FLAG_DO_BOUNDS_CULLING in model.c, the existing dlc_box_out group skip) | every submitted draw is emitted. dlc_box_out moves into nd_dlc as is (the only allowed skip). No new test of any kind | memory feedback_no-culling, plan_dl_design.md path 3 |
| 3 | DLC2 static groups copy Vtx once and replay index lists | nd_dlc is the DLC2 compiler extended. Static pool and index pool keep the once-per-map copy. G3 and G4 check no regression | pm_dlc.inc.c:145, 219, design_producer_models.md 0 |
| 4 | Fixed VRAM layout at boot (6144 KB VRAM, texture cache, render targets, copy block), cache policy (evict on allocation failure, current and previous frame protected, 1024 slots), content keys | pm_tex.c stays the texture cache. nd_tex only maps descriptors to slots. No new VRAM user | pm_tex.c:21, 831, pm_gpu.c:280, 383, pm_mem.c:271. The 3136 KB cache size is not a literal in pm_tex.c (Q9) |
| 5 | GX queue limit 32, async GX copies, pm_gx_room, TEXF1 pending frees, cache flush order | no new GX user in the emit loop. Order: write, flush span, submit. pm_gx_room stays before texture copies | pm_gpu.c:1830-1860, pm_tex.c:551, 579, 640 |
| 6 | Fewer closes and draws (draw is 43 percent of a close), grange merge rules, class 0b first cost | records merge by the grange key. S4 and S5 cut class 0b. G4 | map_rtb1.md 0, diag_e53_merge.md 2.2, inv_frame_cost.md 5 |
| 7 | fp24 VS precision. Matrix and effect data pitfalls (big-endian blobs read as host words, the Quizmo matrix) | one fp32 product in the SEAM order, s16 positions, seam host test. nd_dlc reads list words the way fix_shape swapped them (pm_map.c:97-128). Effect blobs: each S8 row lists the blob reads | pm_gpu.c:230, pm_map.c:376, agent_grab3.md 21 (Quizmo stretch, not confirmed) |
| 8 | RDP quirks already fixed in the walker: fill lower-right inclusive in FILL and COPY only, signed 10-bit fill field, 1-cycle fill uses the blender, texrect lower edge inclusive, 4-bit texture parity, primitive depth rounding, depth source PRIM | the code moves into nd_fill_rect, nd_tex_rect, nd_tile and nd_tmpl (moved, not re-written). ndref compares the results | pm_gbi.c:3449, 3413, 3459, 3371-3380, 2963, 3127, 184, 1769. "Fade full cover" and "star depth" rules are not located by this plan (Q10) |
| 9 | 24 to 28 fps today with logging off. Target 60 fps O3DS, floor 30. The console judges speed. Azahar only for pictures | G6 on the console only. 9.6 | memory project_console-fps-logging-off, project_perf-targets |
| 10 | prof is the tool (logging off). No feature toggles. Finished work replaces the old path in the same slice. Sleep, hang and audio paths untouched | each slice deletes its old path (section 7 column). No switch selects old or new. No file of pm_audio, pm_au2, pm_wdog, pm_main sleep or HOME path is in any slice | memory feedback_no-feature-toggles, CLAUDE.md rule 7 |
| 11 | Census, drawlog and region_diff tools | region_diff stays. drawlog (pm_gbi.c:105) and the census (pm_gpu.c cen_add) feed S0 ndref and are deleted with the walker in S10. A native drawlog (nd record dump, cold) replaces them in S1 | pm_gbi.c:105, diag_e53_merge.md 2.1 |
| 12 | hw_guidelines rules 1 to 10 | section 3 | hw_guidelines.md |
| 13 | Test scope, until-line, caps, budget | each slice names its scenes. New scenarios have caps 45 s or less and print their until-line | CLAUDE.md rule 5, memory feedback_test-time-cap |

## 12. Carried from the earlier plan

Earlier plan: C:\Users\Andrew\.claude\plans\you-are-not-an-glittery-nautilus.md (GN). Other plans read: no-no-hop-to-quirky-frog.md (QF), snappy-sparking-elephant.md and i-want-to-discuss-fancy-elephant.md (both for the Animal Crossing port, only their hardware rules carry), 3ds/SLICES.md, design_producer_models.md (DPM), plan_dl_design.md (PDD), design_inputs_stateful.md (DIS), plan_icache_rows.md (PIR), plan_core1.md (PC1).

| Item | Where it lands | Choice and reason when it conflicts |
|---|---|---|
| GN goal: 60 fps 2D and 3D, interpreter removed at the end | section 0, S10 | same |
| GN method: track relative change of gbi, dl, PMU stall per slice | G6, G7 | same, through `prof` |
| GN feasibility: fps60 shares logic and list build over two frames, the walk gets about 11 ms | 2.5 fps60, section 7 end state | same |
| GN GPU busy time never measured, measure early | S0-6 (gb field) | moved to S0 so every slice has it |
| GN fidelity rule: same things render, 1-pixel diffs allowed, order may change | G2, 9.3 rule 5 | same |
| GN check against HEAD on demand, region diff plus census | G2 plus ndref | conflict: GN used the census as the semantic gate. The census hid wrong textures (diag_e53_merge.md 1.2). Choice: ndref stream with both texture words |
| GN evidence: dl 4.4, gbi 34 phase off, stall 41 to 44 percent, 150,000 I misses | section 3.9, section 0 | baselines |
| GN 90 models 20.7 ms, split texture 38 / dlc 24 / close 23 / rest 15 percent | S3 expected ms | the 38 percent split is not reproduced by a log (map_rtb1.md 0). S3 uses the 0c class total only |
| GN code footprint 55.5 KB per-model walker code against 16 KB L1I | section 3.2 target 8 KB | same direction |
| GN RSP contrast (4 KB IMEM) | section 3.2 | carried as the reason for the 8 KB loop |
| GN flush_draw re-derives state per close | 2.3 templates, S10 deletes flush_draw | same |
| GN vertices: keep 16-byte Vtx, skip the per-G_VTX ring copy for static data | 2.6 | same |
| GN copy path settled, jump hung the console | 4.1 | same |
| GN producer counts 246 files, 3931 lines, effects 127 files 1700 lines, 581 DL calls | 5.1 | recounted |
| GN ARM11 rules (loop under 16 KB, fixed records, prebuilt blocks, PLD, no division, cold hooks) | section 3 | target tightened to 8 KB (user request) |
| GN record about 16 bytes | 2.2 | conflict: 32 bytes. Reason: tex1, uniform id, sort key and order fit with no side table (no pointer chasing) |
| GN template per (combiner, other-mode, geometry, tile format, blend, depth) | 2.3 | same, built from the key, not captured |
| GN renderer: bucket by template and texture, opaque only, alpha and decal in game order | 4.2 | same |
| GN producers write records inside TARGET_3DS blocks, all of src/effects | section 5 | conflict: per-file redirect header instead of hand edits. Reason: decomp text unchanged (no-stubs rule), one line per file. TARGET_3DS edits only for deferred lists |
| GN effects need a dynamic path | 2.6 ring, nd_vtx, S8 | same |
| GN stereo: build once emit twice, eye copy, mono eye count, ortho once, eye2 us field | 4.8, S1 eye patch, S0-6 field | same |
| GN seam rule 1: one fp32 product, same inputs and order (post x game) | 2.5, 9.5 seam test | same |
| GN seam rule 2: one dp4 on s16 positions | 2.6, 4.3 | same |
| GN seam rule 3: no new per-vertex shader arithmetic, new scale goes in the matrix | 2.5 | same. The VS is not changed by any slice |
| GN seam rule 4: models sharing an edge rely on matrices that agree to fp24 rounding, viewport and tilt stay where they are | 2.5 (post matrix unchanged) | same |
| GN seam rule 5: near clip plane, depth map, decal bias unchanged | nd_tmpl moves sh_user_clip, sh_depth_map(-1, -DECAL_EPS) (pm_gpu.c:907) as is | same |
| GN seam rule 6: sorting opaque depth-tested draws changes no seams | 4.2 | same |
| GN seam shot test (solid colour, count holes) and fp24 host test | G9, 9.5 | same. Host test now on all shared edges of mac_01 and mac_02 |
| GN tests layer 1 census | replaced by ndref | see the census conflict above |
| GN tests layer 2 golden.py reference scenes, multi-scene run | dropped | conflict: the user dropped golden.py (CLAUDE.md rule 5). Choice: one Azahar shot scenario per slice plus ndref |
| GN tests layer 3 host unit tests under 1 s | 9.5 | same |
| GN tests layer 4 console deltas per slice | G6 | same, per slice not per row |
| GN config changes (subagent model, rule 5 text, orchestrator opt-in) | not in this plan | CLAUDE.md rule 5 still forbids golden.py, so the GN rule 5 edit was not applied. This plan follows the file as it is |
| GN order of work 0 to 6 (design doc, tests, models, sprites, HUD, entities and battle, effects, delete) | section 7 S0 to S10 | same order. Frame state (S2) added before models, because native draws need native camera state |
| GN verification: hot loop under 16 KB, final 60 fps or a measured statement | G5, S10 | same |
| GN open items: push approval, console for the GPU probe | section 14 Q1, Q3 | same |
| DPM 0.1 bake exists (dlc_compile, dlc_static_build) | 2.9 | reused |
| DPM 0.2 DLC on by default | 2.9, constraint 3 | folded into native |
| DPM 0.3 to 0.5 shape bytes need no conversion, flag is the palette index, no zero-copy from the shape array | 2.6 | same |
| DPM 0.6 eye 2 matrix is post x combined_e, not a copy | 2.5, 4.8 | same |
| DPM v1 keeps lights, matrix and POPMTX on the walker (O1 option A) | dropped | conflict: the native system owns them. Reason: no walker in the end state |
| DPM eligibility rules (plain, SURFACE_OPA, no local vertex copy, no MTX in DL) | S3-1 scope of the first row | widened to all models in S3-2 to S3-4 |
| PDD path 1 texture snapshot per handle | 2.4, S4, S5 | same aim. The key includes raster, palette, tile state |
| PDD path 2 per-model record | S3 | same aim, built from producer arguments |
| PDD path 3 model-level bounding test | dropped | user rule: no culling |
| PDD path 4 dlc replay | S3 (nd_dlc) | folded |
| PDD phase inflation factor 0.74 to 0.79 | section 7 | same |
| DIS inputs of appendGfx_model and their writers | S3 rows (inputs per draw are read each frame, no version counters) | DIS question 1: choice (b), no write-site edits, the producer passes values each frame |
| DIS invalidation sources (pm_dlc_gen, pm_texbind_set cross-frame) | 2.9 key, 2.4 | same |
| DIS open questions 2 to 8 (record key, rebuild rule, tint and fog models, raw list or decoded struct, census first, matrix of a static model) | S3 rows | choices: key per object plus template, rebuild on pm_dlc_gen, tint and fog in the record as uniform values, raw word blocks, ndref first, upload the matrix each frame (one product per draw) |
| PIR OS1, FLH, TEXH, HOOK1, DBGF, LAY2, TST1, MCEN | dropped for the walker files | the walker is deleted. The method (map-file sizes, cold split, hot span list) carries into G5 and section 3.2 |
| PC1 core 1 producer NO-GO, GTAIL measurement, deeper GPU pipeline | 3.6, 4.7, S0-6, S11 | same |
| QF opsplit, mr0, ms0, prod lines | S0-7 baseline | same |
| SLICES.md PO3 GPU vertex transform | done by design (VS does the transform) | carried |
| SLICES.md V18 intro flicker, PT10 and PT43 seams | G9 scenes, risk R4 | carried as known seam and depth cases |
| AC plans: RunFast FPSCR, LUT rules, threading rules (no blocking under a lock, one lock at a time) | section 3.8, S11-3 thread rules | carried as rules, not facts of this port |
| Precision: fp24 VS against fp32 CPU | 2.5, 4.3, 9.5 | s16 exact in fp24. One fp32 product. The PICA converts at uniform load |
| Seams: shared-edge positions, no snapping or rounding change | 9.5 seam test | no snapping is added. Same matrices give same fp24 results |
| Depth bias, z precision, depth range map | nd_tmpl moves sh_depth_map and DECAL_EPS. zmix per draw uniform (pm_gpu.c:123) | unchanged values |
| Primitive depth rounding (PRIMZFIX) | nd_primdepth moves pm_gbi.c:184 rule | unchanged |
| Subpixel and viewport offset | nd_viewport writes the same post matrix (gv_post, pm_gpu.c:224-230) | unchanged |
| Coordinate range and clip | the GPU clips. The CPU-path cases (r_zmark, decal with blend, pm_wide matrix test, pm_gpu_grange refusals) get a GPU form before S10 | risk R6 |
| Matrix precision: compose once in fp32, upload once, avoid cancellation | 2.5 | no inverse or difference of large matrices is added |
| Texture coordinate precision | tc s16 times uvso in the VS. uvso from the moved gv_params_calc (pm_gbi.c:1351) with the quarter-texel tile offset (pm_gbi.c:218) | unchanged |
| Combiner, blend, lighting, fog to TEV | pm_comb.c (TEV), moved sync_render (blend), pm5 VS (lighting, fog factor), V06 last stage (fog mix) | unchanged code moved |
| Alpha compare | moved atest rule from sync_render | unchanged |
| Z-buffer modes (zcmp, zupd, decal, zmark, depth source PRIM) | moved pm_gpu_set_render and the X01 rule | unchanged |
| Decal and coplanar | depth map offset DECAL_EPS, decal templates are ordered | unchanged |
| Colour and alpha dither | the walker ignores G_CD and G_AD (no dither code in pm_gbi.c or pm_gpu.c [C grep]). The native path ignores them too | unchanged |

## 13. Risks

| # | Risk | Check |
|---|---|---|
| R1 | A producer builds a list for later and the redirect emits in the wrong order | rule 5.2.1, nd_range, `#error` for unknown macros, ndref order compare |
| R2 | Segment-keyed entity lists (draw_entity_model_B and D take a vertex segment, entity_model.c:492, 561) make many NdObj variants | key on (list, segment base). Count NdObj per map in the S6 gate |
| R3 | State set by un-ported code and read by ported code during the bridge | nd_unset_use counter (G10), frame state first (S2) |
| R4 | Seams and depth: PT43 Toad Town seams, V18 intro flicker are open today (3ds/SLICES.md) | seam host test and the mac01_seam shot. The plan must not make them worse. It does not claim to fix them |
| R5 | Template table too small or a combine with more than 6 stages | counters in prof. Table size 256 [U] |
| R6 | Triangles that the walker draws on the CPU path (seam_obj, seam_clip counters, pm_gbi.c:364, refusals in pm_gpu_grange) | S9 row lists every refusal with a GPU form. S10 waits for 0 CPU-path triangles |
| R7 | fps60 key-frame replay with native records | S1-5 reads pm_interp.c in full and adds a host test |
| R8 | GPU busy time above 16.7 ms in 3D (two eyes) | S0-6 measures it. If true, 60 fps in 3D is not reachable whatever the CPU does |
| R9 | A host build of game files needs a 32-bit compiler | Q2 |
| R10 | The redirect header misses a compound macro form (for example gDPLoadTextureBlock_4b variants with different tile sizes) | `#error` default plus a grep list in S2-1 |

## 14. Open questions for the user

- Q1. Console pushes: approve one `prof` push at the end of S0 (baseline and GPU busy), S3, S5, S8 and S10.
- Q2. Host compiler: may the agent install a 32-bit gcc (msys2 mingw-w64-i686-gcc), or use gcc -m32 inside the docker image, to compile game producer files for the host tests?
- Q3. Console time for the GPU busy probe (S0-6).
- Q4. Static list compiler at load time (nd_dlc, cold, once per list), or a build-time converter that writes native files for all asset lists? This plan assumes load time.
- Q5. Do you want one more PICA jump or call probe, or is the copy path final?
- Q6. ETC1 textures: not allowed under the fidelity rule? This plan assumes not allowed.
- Q7. Render target format: keep RGBA8, or test RGBA5551?
- Q8. Base branch for S1: nds1 (has PROF4 fixes and the census tools) or native? This plan assumes a new branch from nds1, and S1 deletes the nds1 replay code.
- Q9. The texture cache size 3136 KB is not a literal in pm_tex.c (the size comes from the VRAM slabs at boot, pm_tex.c:475-484). Which source holds the 3136 KB number?
- Q10. The rules "fade full cover" and "star depth" are not located in code by this plan. Which rows or files hold them?
- Q11. A thread on core 1 for the emit loop after S10: allowed, if prof shows emit above 3 ms?

## 15. Answers to the open questions (S0 code agent, 2026-11-04)

- Q1 and Q3: the console baseline is approved (coordinator, 2026-11-04). The baseline build is 2020 (commit 42f6fac7e, build3ds/pm_3ds.3dsx). The orchestrator pushes it. No push by the agent.
- Q2: the 32-bit host compiler is the docker image pm3ds-host:1 (docker/Dockerfile.host: debian bookworm-slim with gcc-multilib). pm3ds-build:1 has gcc -m32 but cannot link it (no 32-bit libgcc). No host install.
- Q4: nd_dlc compiles at load time (plan default).
- Q5: the copy path is final (plan default).
- Q6: no ETC1 (plan default).
- Q7: the render target stays RGBA8 (plan default).
- Q8: the work runs on nds1 (plan default).
- Q9: the texture cache size 3136 KB is the boot VRAM split: 928 KB in bank A and 2208 KB in bank B. The bank sizes come from pm_vram_bank_free_kb (3ds/src/pm_mem.c:203) at boot. The measured boot cap is in docs/handoff/agent_vram3.md:65. The slab code is 3ds/src/pm_tex.c:470-490.
- Q10: "fade full cover" is src/screen_overlays.c:222 (OVERLAY_SCREEN_COLOR fills the whole screen under TARGET_3DS, decision in docs/handoff/agent_hudb.md). "Star depth" is src/battle/16C8E0.c:217-231 (hud_element_set_render_depth for the star point HUD elements). docs/handoff/agent_playfix3.md:16 records depth 20 for the star points bar.
- Q11: a core 1 emit thread is allowed only if the prof line shows nd above 3 ms (plan default).
