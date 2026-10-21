# Architecture review of the O3DS render and audio path

Written 2026-10-21 by ARCH. Read-only review. No build, no Azahar start, no console access.
Code basis: HEAD 96b2d857f plus the uncommitted GV1c diff in the working tree (pm_gbi.c, pm_gpu.c, pm_gpu.h, pm4.v.pica). Line numbers are working-tree lines of 2026-10-21.
Hardware data: build 926, O3DS, 3ds/logs/perf/hw_pull3/hw_o3ds_log.txt and docs/plans/hw_p3_findings.md. Build 902 numbers come from docs/plans/hw_rerank.md and docs/plans/topology.md.
Tags: M = measured on the O3DS (or a fit of O3DS lines). A = measured in Azahar. E = estimate from code and counts. G = guess. "citro3d internals" are from the citro3d and libctru sources as I know them. These sources are not in this tree (only the headers in /opt/devkitpro/libctru/include). Every statement about them is E or G and the bench of part 3 checks it.
All ms are per drawn world step (mode 4) unless the text says otherwise.

## 0. Summary

The port is an immediate-mode emulator of the RSP and the RDP. Each frame it decodes every display-list op, derives the PICA state from the raw RDP words, transforms and tests every vertex and triangle on the ARM11, and rebuilds the citro3d state for every draw. This work does not depend on what changed. It depends on what is on screen. Most of the geometry and much of the state come from static data (map shape files, texture and render-mode lists, entity and animator lists) that are equal in every frame.

Three design faults:
1. No compile step. Static display lists are translated again every frame: 4032 ops, 894 triangle commands and 1357 vertices per world step (M). Geometry (gt + gv) is 13.2 ms, the rest of gb ("gb self") is about 16 ms after the timer reads (M, hw_p3_findings.md part 2).
2. The draw path is a general-purpose library path that costs 66 us per draw (M). citro3d plus our flush_draw rebuild and compare state for each of 98 to 190 draws per step. 98 draws x 66 us = 6.5 ms in the world, 8 to 12.5 ms in menus and the intro (M).
3. Synchronous system calls and waits on the critical path. Each GSPGPU_FlushDataCache costs about 0.9 ms (E from M). Each texture miss has a fixed cost of 1.21 ms (M fit, R2 0.996), and the bytes add only 39 ns/B (M fit). The "80 to 437 ns/B" of the tx1 lines is this fixed cost divided by small textures. The frame loop also waits 9.9 ms per drawn step in FrameBegin for the vblank alignment (M), while the CPU is the late part.

The GPU needs 7.7 ms of a 55 ms step (M). Every row that moved work to the GPU and kept the CPU work per triangle gave no gain on the O3DS (TX1, DR1, KICK1, gpuvtx, M). The user's principle holds: an offload that keeps the per-element CPU work is the wrong design. The target is a CPU whose work scales with changes (new lists, new textures, changed matrices), not with triangles.

## 1. Verdict on the current design

### 1.1 The per-triangle and per-vertex CPU path

What runs today on the default path (gv1 off, the base state of build 926):

Per vertex, G_VTX (pm_gbi.c:2115 to 2248, loop 2206 to 2244):
- 6 stores of slot tables (vert_ring, vert_gmi, vert_tss, vert_mid, vert_live, vert_scr).
- S and T scale: 2 int-to-float conversions, 2 multiplies, 2 multiplies by 1/65536 (2230, 2231).
- vert_color (2232, body at 581): unpack of 4 bytes when unlit, the light loop when lit (0.1 percent of triangles lit, A).
- Transform by `combined`: 16 VFP multiply-adds in 4 dependent chains (2236 to 2240).
- clip_bits4: 6 float compares (2241, body at 1154).
- Cost: gv 2.78 ms for 1357 vertices = 2.05 us per vertex (M, state phasegbi, includes part of 1.9 us of timer reads per G_VTX op).

Per triangle command, draw_tri (pm_gbi.c:1361 to 1647):
- Index decode, range and vert_live checks (1362 to 1380). po3a branch (1384).
- off_screen_check and box_fold_sync once per G_TRI op (1392, PG4). off_screen_check calls img_role (177 to 184). box_fold_sync scans all texels of tile 0 in tile0_white (464 to 478) while a box window combiner is set.
- Reject: AND of the clip bits (1406). Cull: 2D homogeneous cross, 8 multiplies (1411 to 1422). The n_keep loop, 3 x 5 compares (1423 to 1429).
- comb_audit at loglevel debug (1432, body 1021 to 1072: FNV of 8 words, table scan). `noaudit` in HW runs.
- tri_stats (1434, tri_flags 1214 to 1224). ensure_tile (1436, a compare when the tile is bound). dual_bind (1438, body 1079 to 1111: FNV of 11 words for every triangle of a TEXEL1 plan).
- For a kept triangle: 3 x xform_vtx into ClipV with uv (1458 to 1473), pm_tex_size per vertex when the texture is off (1463), t1_fold for TEXEL1 (1474 to 1477), near clip (1514), colour overrides ENV, plan constant, alpha 255, fog, boost (1531 to 1634), sync_render (1560, body 505 to 555: blender decode, 2 PMD look-ups, img_role, 4 to 6 pm_gpu_set_* calls that compare), pm_gpu_set_mvp(ident) (1639: memcmp of 64 B per triangle, pm_gpu.c:1420 to 1422), 3 x pm_gpu_vert_clip and put4 (pm_gpu.c:1720 to 1729, 828 to 854): 10 floats, 4 table loads, 40 B into the linear vertex pool.
- Cost: gt 10.37 ms for 894 commands = 11.6 us per command (M). 72 percent end at the reject (M). Split (E): a rejected command about 3 us, a kept triangle about 30 to 35 us ((10.37 - 644 x 3 us) / 250).

The GV1 path (switch gv1, GV1a and GV1b committed, GV1c in the working tree) removes the transform, the colour and the S T of unlit loads (2149 to 2228) and runs the state prep once per epoch (gv_tri, 1282 to 1358). Per triangle it still runs the op decode, the slot checks, the AND of the box bits, the epoch compare, the parameter key compare and pm_gpu_gappend (1343: 3 index stores and 3 palette checks). Per vertex it still copies 16 B to the ring (pm_gpu_gvtx_load) and, for loads under 16 vertices, still transforms the position for the reject bits (2216 to 2225). Azahar: gb 12.7 to 11.5 ms in the intro dip window with GV1b (A, commit 96b2d857f). GV1a alone made gb worse (+0.9 ms, A), because the CPU kept the per-triangle prep.
Verdict: GV1 is the right direction for the part it covers. It keeps work per triangle (about 1 to 3 us on the O3DS, E) and per state op. It cannot reach "0 CPU per static triangle". The epoch rule (gv_epoch++ at every op that is not a vertex, triangle, matrix or list op, pm_gbi.c:2089) ends an epoch at every repeated state op, so the full prep runs again after each of the 207 G_SETCOMBINE per frame of the intro dip (A, intro_dip.md 1).

### 1.2 The 66 us per draw

Measured split per draw (M, state phasegbi, 99.4 draws per world step, each value includes about 1 us of timer reads): texture bind (gbind) 12.6 us, texenv (gtev) 8.9 us, depth, blend, scissor, cull (gst) 12.4 us, the rest of flush_draw (gp) 11.4 us, C3D_DrawArrays or C3D_DrawElements (gd) 31.9 us. Base without the fine timers: 66.1 us (M). DR1 changed it by -0.09 us (M). 66 us is about 17,700 ARM11 cycles at 268 MHz.

What our code does per draw (pm_gpu.c:412 to 803):
- Builds a TevKey (memset and 20 stores, 506 to 514) and memcmp against the last key (515). On a miss: C3D_TexEnvInit of every used stage and up to 6 stages of setters (518 to 653).
- pm_tex_bind (489) and C3D_TexBind on a change. 
- Up to 7 sh_* compares (336 to 360) and their citro3d setters, a scissor computation (683 to 705), the projection uniform compare (477 to 484).
- A raw batch (gpuvtx or gv1) also calls C3D_BindProgram twice per draw (449 or 468, then 730) and C3D_SetAttrInfo twice (450 or 469, then 731), BufInfo_Init and BufInfo_Add (721 to 723), and 4 to 5 C3D_FVUnifSet plus one C3D_FVUnifMtx4x4 per palette entry (451 to 466).

What citro3d does per draw (citro3d internals, E):
- Every setter only stores a value and sets a dirty flag. C3D_FVUnifSet marks the uniform dirty (uniforms.h:53, inline, no compare).
- C3D_DrawArrays calls C3Di_UpdateContext. For each dirty group it writes PICA registers with GPUCMD_Add, an out-of-line function that copies word by word and checks the buffer end (gpu.h:65): shader program (when the DVLB changes, the whole shader code and the operand descriptors, up to about 600 words), attribute info (about 12 words), buffer info (3 + 3 per buffer), effect state (depth, alpha, stencil, blend, cull, early depth, about 20 words), each dirty texenv stage (about 6 words), each dirty texture unit (about 8 words plus GPUREG_TEXUNIT_CONFIG). Then C3D_UpdateUniforms scans the dirty flags of all 96 float uniforms and writes each dirty run with GPUREG_VSH_FLOATUNIFORM_CONFIG and _DATA (registers.h:720, 721). Then the draw packet: about 12 writes (GPUREG_PRIMITIVE_CONFIG, GPUREG_RESTART_PRIMITIVE 0x25F, GPUREG_INDEXBUFFER_CONFIG, GPUREG_NUMVERTICES, GPUREG_VERTEX_OFFSET, GPUREG_START_DRAW_FUNC0, GPUREG_DRAWARRAYS 0x22E or GPUREG_DRAWELEMENTS 0x22F, GPUREG_VTX_FUNC 0x231).
- C3D_BindProgram marks the program and the attribute info dirty. pm.v.pica, pm3.v.pica and pm4.v.pica are 3 separate DVLBs (CMakeLists.txt:339 to 343, pm_gpu.c:892 to 926). So each switch between them most probably sends the shader code again (G: the code-upload condition in citro3d is not checked here). A raw batch switches twice per draw. This can explain why gd grew from 1.98 to 2.44 ms in the gpuvtx A/B (A, hw_rerank.md 2.2) and why every raw-path draw costs more than a CPU-path draw.
- No register work is needed for the 17,700 cycles. The packet of a draw with all state already set is about 24 words. The cost is in the generic path: flag tests, out-of-line writes, the 96-entry uniform scan, our key build and compares, and data cache misses (16 KB L1 per core, no L2 on the O3DS, the state of citro3d, our tables, the command buffer and the GBI working set share it, E).

A raw command-list renderer (what homebrew that does not use citro3d does, E): keep a shadow of the PICA registers, pre-build per-state register blocks once, and per draw append (a) the block id change as a memcpy of its words, (b) the changed uniforms, (c) the draw packet. Register sources: 3dbrew "GPU/Internal Registers" and libctru registers.h (in this tree: /opt/devkitpro/libctru/include/3ds/gpu/registers.h). Command format: header word with register id, byte mask and an increment bit (gpu.h:11), then the parameters, 8-byte aligned. The PICA command processor also supports a call of a second command buffer: GPUREG_CMDBUF_SIZE0/1, ADDR0/1 and JUMP0/1 (registers.h:597 to 602, 0x238 to 0x23D). A jump does not return. The called buffer must end with a jump back to the next word of the main list, so the return address must be patched per call and the patch must be in memory before the GPU runs (G: exact rules of size units and alignment are not checked here).
Expected CPU cost of a raw draw (E): 24-word draw packet 0.3 to 0.6 us, a state block of 30 to 60 words 0.5 to 1.5 us, a 4x4 matrix uniform (17 words) 0.2 to 0.4 us, a texture bind (about 10 words) 0.2 to 0.4 us. Total 2 to 5 us per draw, against 66 us. The target "10 us per draw" has a margin of 2 to 5 times.

### 1.3 State handling: accuracy work in the hot path

The interpreter derives the PICA state lazily, at the triangle, from raw RDP words. Each check is correct, but each runs per triangle or per draw instead of once per change. HW cost: Azahar GA-basis numbers (post_po3_path.md 1b) times the gbi ratio 3.5 (hw_rerank.md 1.3). These numbers are from before PG4, DIP2 and GV1b, so they are upper bounds for the code of today.

| Check | Where | Runs | HW cost per world step | Tag | Note |
|---|---|---|---|---|---|
| comb_audit | pm_gbi.c:1021 to 1072, called 1294, 1432 | per triangle at loglevel debug | 2.2 to 2.6 ms in test runs, 0 in play | E (A 0.75 x 3.5) | `noaudit` removes it. Never in the HW numbers since Push 3 |
| off_screen_check | 177 to 184, called 1392 | per G_TRI op (PG4), per epoch with gv1 | part of 6.6 ms (section A, A 1.9 x 3.5) before PG4 | E | img_role (130) per call |
| box_fold_sync and tile0_white | 482 to 497, 464 to 478 | per G_TRI op while a box combiner is set, a scan of up to w x h texels | in section A | E | draw_box windows (menus, UI). Result depends only on tile 0 and the combiner |
| dual_bind | 1079 to 1111 | per kept triangle (CPU path), per epoch (gv1) | 1.75 ms | E (A 0.5 x 3.5) | FNV of 11 words. A TEXEL1 plan only |
| sync_render | 505 to 555 | per kept triangle (CPU path, 1560), per epoch (gv1, 1298) | 1.4 ms | E (A 0.4 x 3.5) | Inputs change only at G_SETOTHERMODE, G_SETCIMG, G_SETBLENDCOLOR, G_SETFOGCOLOR |
| ensure_tile | 1943 to 1956 | per triangle | under 0.2 ms | E (A 0.05 x 3.5) | A compare, a flush and pm_tex_load on a change |
| sync_combine | 969 to 1016 | per combiner-related op | 2.2 ms of the intro dip in Azahar before DIP2 (A) | A | DIP2 hash of 11 words skips equal inputs (970 to 976) |
| pm_gpu_set_mvp(ident) | pm_gbi.c:1639, pm_gpu.c:1417 to 1431 | per kept triangle | 0.1 to 0.3 ms | E | memcmp of 64 B with the identity and 64 B with the last matrix |

Verdict: the checks are needed for exactness. The fault is when they run. A state change happens a few hundred times per frame, a triangle 900 to 1800 times, and a static state list is equal in every frame. The right form: compute the PICA-side result of a state change once (epoch, GV1b), and for a static state list once per list (part 2b).

### 1.4 Texture conversion

Fit of the 81 `tx1 f` lines with misses in the build 926 log (M, scratch script, not committed): us = 1211 x misses + 40.0 x KB, R2 = 0.996. 14,674 misses, 105,626 KB, 21.6 s in total. Mean 7.2 KB per miss.
- Fixed cost per miss: 1.21 ms (M). Cost per output byte: 39 ns (M). For a mean texture: 1.21 ms fixed against 0.29 ms of bytes. So 81 percent of the texture time is per-miss overhead.
- The range 80 to 437 ns/B of the tx1 lines is this fixed cost divided by the texture size. Small textures show a high ns/B. The cost does not depend on the decode path, so TX1 (decode speed) could not change it (M: 247 ms/MB with TX1, 251 without).
- Spike steps: step 332 tex 101.2 ms for 64 misses and 400 KB (M). The fit gives 64 x 1.21 + 400 x 0.040 = 93.4 ms. Step 334: 104.5 ms for 57 misses and 417 KB, fit 85.7 ms.

What runs per miss inside the tx1 timer (pm_tex.c:664 to 770): the slot scan of 512 slots (676), evict_oldest_done (309 to 322, a scan and C3D_TexDelete = linearFree), C3D_TexInit (692, linearAlloc), 2 setters (698, 699), the decode into the texture memory (706), C3D_TexFlush (766). C3D_TexFlush calls GSPGPU_FlushDataCache for the texture range (citro3d internals, E). The old path calls C3D_TexLoadImage (764), which copies and flushes the same way (E).
Cause (E from M): one GSPGPU_FlushDataCache costs about 0.86 to 0.89 ms on the O3DS. Evidence: gcache was 0.86 ms per world step with 1 call per frame in build 902 (M), and 1.77 ms with 2 calls per frame in build 926 (KICK1 split plus the end of frame, gpu_flush_written, pm_gpu.c:238 to 247) (M). The call is an IPC to the GSP module, which runs on core 1, where the audio threads hold up to 80 percent of the time. The texture write itself is a cached write (linear FCRAM is cached), so "uncached memory writes" is not the cause (E). Small stores: TX1 already uses 16-bit and 32-bit stores. The remaining 39 ns/B is about 10 cycles per byte, near the cost of the decode itself (E).
Verdict: the texture path is bound by system calls, not by conversion. An offline conversion (texbake, PG2L) removes only the 39 ns/B part and adds SD read calls. That is why PG2L and TX1 showed no gain.

### 1.5 The audio mixer on core 1

Build 926 (M, 829 `audio cost` lines and 131 `audio2:` lines):
- Control job (au_thread: sequencers, voices, AU2 command emit): 3.07 ms per 60 Hz tick (mean, wall time on core 1). Azahar gives 0.1 to 0.4 ms per tick for the same work (A, AU2_design.md 8), so the HW value is mostly wait for core 1 time slices (E).
- Render thread (pm_au2.c, the C RSP audio ops of pm_rsp_audio.c: ADPCM 185, resample 358, envelope mixer 540, mix 661, pole filter 756, reverb in src/audio): 3.3 to 4.0 ms per job, 873 to 1338 jobs per 300 game steps. At 24.3 game steps per second that is 71 to 108 jobs per second and 234 to 433 ms of core 1 per second (E, wall time, upper bound).
- Sum: 418 to 617 ms per second, 42 to 62 percent of core 1, 52 to 77 percent of the 80 percent app share (E). Build 902 had 14.6 to 17.0 ms per 33.3 ms step (M), the same order.
- Core 0 pays only the join wait: `audio wait avg` 3.37 ms per step (M). This is the control part. The render part is off the core 0 path since AU2.

ndsp hardware voices (the DSP firmware through libctru ndsp): 24 channels, PCM8, PCM16 and DSP-ADPCM (the GameCube-style codec, not VADPCM), per-channel rate (pitch), interpolation, 12 mix gains (front, back and 2 aux buses), and an aux callback that gives the ARM the aux bus samples for CPU effects.
What the N64 engine does that ndsp does not:
- VADPCM. Different codec. Options: decode once to PCM16 (exact samples) or re-encode to DSP-ADPCM (lossy). The audio data is the pm_sbn segment, ROM 0xF00000 to 0x1942C40 = 10,764 KB (ver/us/splat.yaml:14778). PCM16 is about 3.5 times VADPCM, so up to about 35 MB (E). That does not fit the O3DS heap (31,608 KB, 27,106 KB used by the resident ROM, M). DSP-ADPCM fits (about the same size) but is not exact.
- The N64 resampler (4-tap table, pm_rsp_audio.c:66) and the envelope ramp per sample (envMixer). ndsp applies volume per DSP frame and uses its own interpolation (E). Not bit exact.
- Reverb, delay and chorus of the 4 fx buses (AuFx). Not on the DSP through ndsp. They stay on the CPU through the aux callback, on the summed bus (as the N64 does).
- The sequencer, the SFX queue, the envelope step per tick and the callbacks. These stay on the CPU in any design (the control job).
What a port to ndsp voices frees: the render part minus the effect part. The effect share is not measured (G: 20 to 40 percent of the render). So 15 to 30 percent of core 1 (G). It frees 0 ms on core 0, because the core 0 cost is the control join (3.37 ms, M), and the control stays.
Risk to exactness: the posted PCM changes, so the audiohash gate (AU2_design.md 6) cannot pass. A new gate would compare the control stream (voice start, stop, pitch, volume, pan per tick) and a listening test by the user. This is a user decision.
Verdict: the audio is not the frame-rate limit after AU2. The ndsp port has a high cost (codec, memory, exactness) and no core 0 gain. Keep the C mixer. If core 1 time is needed later, optimize the mixer with ARMv6 SIMD (SMLAD, QADD16, SSAT; the code already uses ssat at pm_rsp_audio.c:95) and keep the bit-exact gate. The core 0 join wait can fall by moving the control job to a point where it is done before the join (or to core 0 with `audiocore0`, AU2 option A, pm_audio.c:181), which the bench of part 3 can measure.

### 1.6 The frame structure

- The GPU needs 7.7 ms per world step (M, phc gpu field). Steps take 55.2 ms cpu (M). The GPU idles more than 85 percent.
- KICK1 had no gain (M: -0.31 to -0.88 ms) because the CPU, not the GPU, is late. Earlier GPU start cannot help a CPU-bound frame.
- Each drawn step waits 9.9 ms in FrameBegin (fb, M). FrameBegin(C3D_FRAME_SYNCDRAW) at pm_gpu.c:1326 waits for the previous GPU frame and for the vblank tick of the citro3d frame rate (citro3d internals, E). The GPU part of the previous frame is short, so most of the 9.9 ms is the alignment of a late step to the next vblank (E, hw_rerank.md 2.1: steps with cpu 18 to 21 ms land on 33.3 ms in 94 to 97 percent of cases). A drawn world step is 69.5 ms of wall time for 55.2 ms of cpu (M).
- FS1 decouples the game step from the draw: 24.3 game steps per second at 14.4 drawn per second (M). The skipped steps cost almost no wall time (M). So the vblank wait is the largest single CPU stall left on core 0.
- citro3d uses one command buffer (C3D_Init(0x180000), pm_gpu.c:872) and holds the GX queue until C3D_FrameEnd (pm_gpu.c:1381). So the CPU cannot write the next frame while the GPU reads the last one (E). A renderer with 2 command buffers can overlap them.

## 2. Target design

Principle: the CPU does work per change, not per element. The common case per frame is: walk the per-frame main list, look up each static sub-list in a cache, test one bounding box per compiled list, patch a matrix and a few uniforms, and append a pre-built command block. The GPU does every per-vertex and per-pixel operation. The CPU path of today stays as the exact fallback for every case the compiler refuses.

### 2a. Display-list compile and cache (DLC)

What is static (from src/model.c and the decomp structure):
- Map models: appendGfx_model (src/model.c:1369) emits, per model and per frame, a short preamble into gMainGfxPos (pipe sync, lights, gSPDisplayList of the texture list `textureHandle->gfx` (1510, 1514), gSPDisplayList of `ModelRenderModes[i]` (1623 to 1924), tint and fog ops, gSPMatrix of `model->finalMtx` (1930, 1941)) and then gSPDisplayList of `modelNode->displayData->displayList` (1992). The geometry list and its Vtx are in the map shape data (gMapShapeData). The texture list is built once per texture handle at map load. ModelRenderModes is static .data. Only the preamble and the matrix are per frame.
- Model groups (3035 to 3151), entity models and animators: static lists with per-frame matrices (animator nodes). Matrices change per frame. Vertices do not.
- Billboards: a static list with gCameras[].mtxBillboard (1981). Static list, per-frame matrix.
- Texture panning: tile-size ops in the main list (per frame), not in the static list.
- Dynamic Vtx (H06a.md Cause 1, 16 files that write `.v.ob`, `.v.tc` or `.v.cn` at run time): draw_box.c (100 lines), imgfx.c (68), msg_draw.c (26), hud_element.c (20), sprite.c (14), effects whirlwind and water_block, pra_29 bridge, model.c (local vertex copies, mdl_make_local_vertex_copy 4153, mdl_get_copied_vertices 4189), animator.c (3), and 7 more. Effects, sprites (imgfx meshes), text, HUD and UI boxes are dynamic.

Share of static triangles: not measured. The counters that exist (po3a: 85.8 percent of drawn triangles plain with one matrix, 0.1 percent lit, 0 percent texgen, A) do not split by source. Estimate: in world scenes 80 to 95 percent of the triangle commands come from map, entity and animator lists (G), because sprites, text and UI are 2-triangle quads and texture rectangles. The intro dip (1808 triangle commands, 84 percent rejected, A) is one large animated model (G: static Vtx, animator matrices). The title and file select have 52 drawn triangles (M): DLC does not matter there, the draw count does. The bench counter `dlsrc` (part 3, B6) measures the share before any DLC code.

Compiled form (one entry per static list, key = list address plus a 64-bit hash of its bytes up to G_ENDDL, the bytes of the Vtx it loads, and the entry state that changes the translation):
- Vertex buffer: the Vtx of the list in the native 16 B layout (the gring layout of pm3.v.pica and pm4.v.pica), one copy per entry, in linear memory or in VRAM (4,354 KB VRAM free, M).
- Index buffer: the RSP 64-slot semantics resolved at compile time. Each G_TRI1 or G_TRI2 index becomes an absolute index into the entry vertex buffer. The slot rules of today (vert_live, tri_since_vtx, the FX3f cross-load case, H06a Cause 2) run once in the compiler. A triangle that reads a slot not loaded in the same list makes the list "not compilable" (fallback).
- Runs: a list with state ops inside is split into runs. Each run has a state-block id (2b), a first index and a count.
- A palette index per vertex when the list has G_MTX ops inside (the pm4 palette, GV1c). One matrix per list in the common case.
- An object-space AABB per run.
Replay at a G_DL whose target hits the cache:
- One AABB test with the current `combined` (8 corners, 256 multiply-adds, about 2 us on the O3DS, E). All corners off one plane: skip the whole list. A corner with w at or under 0.05: the near case (fallback to the CPU path or GV1, as GV1 Q1).
- Compose post x game once per matrix (as GV1c does, pm_gpu.c:455 to 466), set the uniforms, append the pre-built command block of each run (2c). The cull mode comes from the entry geometry mode and the winding sign of the post matrix (GV1.md 1.6). The GPU clips the far, x and y planes.
- CPU cost per static triangle: 0. CPU cost per list: the hash-free cache lookup (address to entry, about 0.5 us), the box test, the uniform set and the block copy (3 to 6 us per list, E).
Fallback: a miss or a refused list runs the interpreter as today (gv1 path, then the CPU path). Refusal reasons get counters, like gvfb: lit, texgen, fog mix, depth image role, TEXEL1 slot, screen vertex (G_MODIFYVTX), cross-list slot use, dynamic Vtx source, debug switch.
Invalidation (the address-key lesson of H06a Class 2, never key by address alone):
- A registry of static ranges: gMapShapeData, the texture handle lists, ModelRenderModes, the entity and animator data. An entry is created only when the list and every Vtx it loads lie in a registered range.
- The registered ranges are cleared by the hooks that already invalidate the texture cache: every ROM read (pm_os.c), every Yay0 decode (pm_map.c:375), the map load, and mdl_make_local_vertex_copy (a list redirected to copied Vtx becomes dynamic).
- Check switch `dlccheck`: hash the list and Vtx bytes at every replay and count mismatches (gate 0 in Azahar and in one HW run).
- The main display list (gDisplayContext buffers) is never cached. It is per frame by design (the game builds it each step).
Main-list cost that stays: the preamble ops per model. With the state lists compiled (2b), a model costs about 10 main-list ops (E). The main list is about 1000 to 1500 ops per world step (G), at 1 to 2 us per op after the state shadow (E): 1 to 3 ms.

### 2b. PICA state-block cache

- A state block is the full PICA-side result of the RDP state that a draw reads: the combiner plan (pm_comb.c), the texenv stages, blend, depth test and write, alpha test, stencil mode, depth map and decal bias, the texture unit config and the cull mode. Today flush_draw builds this per draw from about 40 variables (TevKey, pm_gpu.c:311, 506 to 515, and the sh_* group, 317 to 360).
- Compile: on the first use of a key, build the register words (texenv 0x0C0 to 0x0FD range, blend and depth groups, registers.h) into a block of 30 to 80 words. Key: a hash of the inputs that flush_draw reads (the TevKey fields plus the render and scissor state). Table of 256 to 1024 blocks.
- Static state lists (the texture list and the render-mode list of a model): compile the list into a "state delta" (texture id, tile values, combiner plan id, render-state id) and apply it at replay with a few stores, not with an op decode per op. Same registry and invalidation as 2a. The texture list includes the TMEM ops (G_SETTIMG, G_LOADBLOCK, G_SETTILE, G_SETTILESIZE): its delta holds the texture cache slot. A texture eviction invalidates the deltas that hold that slot (a generation number per slot).
- Apply: compare the block id with the id of the last draw. Equal: no words. Different: copy the block words (1 to 2 us, E).
- Colour constants (prim, env, keys) change per draw. They stay out of the block as 1 to 3 register writes (texenv constant colour) or uniforms.

### 2c. Draw path: raw command lists or a lean wrapper

Recommended: a lean wrapper. Keep citro3d for C3D_Init, render targets, C3D_FrameBegin, C3D_FrameSplit, C3D_FrameEnd and the display transfer. Do every draw through our own packets with GPUCMD_AddRawCommands (gpu.h:57) and a full register shadow. Do not call C3D_Draw*, C3D_TexEnv*, C3D_TexBind, C3D_FVUnif* or the effect setters inside a frame. Reason: C3Di_UpdateContext runs only inside citro3d draw calls, so citro3d's own dirty state is not sent while we do not call them (E). We must then also send the framebuffer, viewport and attribute-buffer registers ourselves after each FrameBegin and target change (about 30 words, E).
- One shader program for all 3D work (merge pm.v.pica, pm3.v.pica, pm4.v.pica into one DVLB with 3 entry points, or one program with a mode uniform). This removes the program switch per raw draw (1.2).
- Per draw: state block id compare and copy, changed uniforms (matrix palette entries), texture unit words on a change, the 24-word draw packet. Target: 10 us per draw (E: 2 to 5 us expected).
- Pre-built blocks for compiled lists (2a): the whole run packet (state id, uniforms slot, draw) is built at compile time. At replay only the uniform values and, for DrawElements, the index offset change. Copy, do not call: a GPU-side call (GPUREG_CMDBUF_JUMP) needs a patched return per call and a cache flush of the patch. A copy of 40 to 80 words costs about 1 us (E) and has no such rule. Use the jump only if the bench shows a copy cost over 5 us per block.
- Two command buffers (one per frame, alternating), so the CPU writes frame N+1 while the GPU runs frame N (2f).

### 2d. Texture path

1. One flush per split point for all GPU-read data (FL1). Textures, the vertex pool and the rings go to one or few contiguous linear arenas (a slab allocator by size class, 512 B to 512 KB). The texture writes of a frame are flushed with the vertex data in the one GSPGPU_FlushDataCache per split point (gpu_flush_written, pm_gpu.c:238). C3D_TexFlush per miss (pm_tex.c:766) goes away. The GPU reads a texture only after the split or FrameEnd that follows its upload, so this is exact.
   Expected (E): per miss 1.21 ms to about 0.2 to 0.35 ms (decode plus slab alloc). Step 332: 101 ms to 25 to 35 ms. Steady tex 0.92 ms to 0.3 ms. gcache 1.77 ms to 0.9 ms (one call per split, or 0.1 ms with the alternatives of the bench B2).
2. Slot pool by size class (no linearAlloc and linearFree per miss, topology.md hazard 8). The 512-slot limit (live 512, M) rises with it.
3. Per-map bulk load (optional, after FL1): at map load, one sequential read of a per-map pack of the pre-converted textures of that map (PICA tiled bytes, the exact bytes TX1 writes, gate memcmp). PG2L read each texture with its own lseek and read on a miss (pm_tex.c:653), so it paid one SD call per texture. One read per map in the fade (the screen is black for 10 to 20 steps) costs SD time where no frame is drawn. Gain over FL1: only the 39 ns/B part, about 16 ms of step 332 (M fit) and 0.3 ms steady (E). Low priority.
4. VRAM residency (O13): the GPU is not the limit (7.7 ms of 55, M). GX_TextureCopy from linear to VRAM is a GX command in the same queue (pm_gpu.c:270 uses it for snapshots). It costs no CPU per byte, but the FCRAM copy must stay for an eviction. Hold until the GPU field of phc rises over 15 ms per frame.
5. Texture lookup on a hit: G_SETTILESIZE calls pm_tex_load each time (palette FNV and slot scan, post_po3_path.md A2, A3). With 2b, a static texture list holds its slot id, so the lookup runs once per list compile, not per frame.

### 2e. Audio

- Keep the C mixer and the bit-exact audiohash gate. The audio is off the core 0 critical path since AU2 (M).
- Core 0 join (3.37 ms per step, M): measure the control job CPU time and its wait on core 1 (bench B5). If the wait is core 1 scheduling, start the control job earlier (at the start of the gbi phase, not at nuGfxTaskStart) so it ends before the join, or run it on core 0 (`audiocore0`). Expected 1 to 3 ms on core 0 (G).
- Mixer speed (only if core 1 time is needed): ARMv6 SIMD in pm_rsp_audio.c (resample 4-tap with SMLAD, mix with QADD16, ADPCM with SMLAD). 1.5 to 2.5 times faster mixer (G). Exact by construction (same integer results), audiohash gate.
- ndsp voices: not recommended (1.5). A user decision if the user accepts a non-exact audio path. The bench B5 measures the ndsp CPU cost so the decision has a number.
- Pipelined interpreter on core 1 (O8): rejected on the O3DS today (gbi 35 ms does not fit 26.7 ms, hw_rerank.md 2.4). After DLC the core 0 render work can fall to about 8 to 12 ms (G). Then a split "core 0: game logic of step N+1, core 1: replay of step N" could fit if the mixer is faster. Revisit after stage 3.

### 2f. CPU and GPU split

Target per world step (E, play build):
- Core 0: game logic and list build (lo + dl, 6.0 ms M), main-list walk (1 to 3 ms), cache lookups, box tests and block copies of 100 to 300 compiled lists (0.5 to 2 ms), dynamic lists through GV1 (sprites, effects, text: 1 to 3 ms), 98 draws at 5 to 10 us (0.5 to 1 ms), texture misses at 0.3 ms each (about 0.3 ms steady), one flush per split (0.1 to 0.9 ms), FrameEnd (0.6 ms). Sum: 10 to 17 ms.
- GPU: every vertex transform, clip, cull, raster, combine and blend of static lists. 7.7 ms today. The transform of all kept and rejected static vertices moves there, about 3 to 6 ms more (G). 11 to 14 ms of 33.3.
- Core 1: audio (40 to 60 percent of the core, M and E), log writer, flash.
- No per-triangle or per-vertex CPU work in the common case. Per-element CPU work stays only on the refused cases (lit, near plane, screen vertices, depth images, debug switches) and on dynamic Vtx (sprites, text, UI, effects), which are few triangles.
- Frame loop: the game step clock stays 2 vblanks per step (PC01 pacing, statehash exact). The CPU does not wait for the vblank alignment before a drawn step when it is late. It waits only for a free command buffer. With 2 command buffers the CPU writes frame N+1 while the GPU runs frame N.

## 3. Gain, effort, risk, metric and the cheapest test per part

All gains are O3DS ms per drawn world step. Reference: base build 926, world, hand play: cpu50 55.2, gb 35.05, gt 10.37, gv 2.78, gp+gd 66.1 us per draw, 98 draws, tex 0.92, gcache 1.77, fb 9.9, 24.3 game steps and 14.4 drawn steps per second (M).

| Part | Saved (ms per world step) | Tag | Effort (lines) | Risk | Log metric that decides | Cheapest test before the build |
|---|---|---|---|---|---|---|
| FL1 one flush per split, texture slab | 0.9 to 1.7 steady (gcache, tex). Spike steps 332 and 334: -55 to -75 ms each | E from M fit | 80 to 150 | Low: a missed range is a HW-only fault (GV0a lesson) | phs `gc` and `tx` columns, `tx1 f` us per miss, pc lines of steps 330 to 340 | B2 and B3 |
| FB1 frame loop without the vblank alignment wait, 2 command buffers | 5 to 9.9 of wall per drawn step (fb). Drawn rate 14.4 to 17 to 18 per second | E from M | 60 to 150 (2 buffers need RAW1, the wait change alone is 20 to 40) | Medium: pacing, F06 key frames, the D03 and F03 readbacks of the last frame | `pace:` lines (drawn per second, intervals), phs `fb`, `fs1 skipped` | B7 (one A/B state of the wait, no new renderer) |
| RAW1 lean draw path with state blocks (2b, 2c) | 98 draws x (66 - 5 to 10 us) = 5.5 to 6 in the world, 7 to 11.5 in menus and the intro (129 to 190 draws) | E | 500 to 900 | Medium to high: citro3d coexistence, framebuffer and attribute registers, every state of flush_draw (decal2, zmark, memblend, off-screen, noise unit) | phs `gp`, `gd`, `draw`, pcx `tev bind st` | B1 |
| DLC-G compile of static geometry lists (2a) | 2 to 5.5 more than GV1c. 9 to 12 without GV1 | E (gt+gv 13.2 M, GV1 ceiling 9.8 E) | 800 to 1200 | High: slot rules, near plane, invalidation, exactness of the float24 transform (GV1.md G1) | phs `gt`, `gv`, `gb`, new `dlc` line (hits, misses, refused by reason, lists replayed, triangles replayed) | B4 and B6 |
| DLC-S compile of static state lists (2b) | 4 to 9 of "gb self" (about 16 ms real, M) | G: the static share of the state ops is not measured | 400 to 700 | High: TMEM and texture slot coupling, tile-size panning, epoch rules | phs `gb`, `gbiops`, `dlc` line | B6 |
| Texture per-map pack | 0.3 steady, about 16 in spike steps | E from M fit | 300 to 500 | Medium: SD time at map load | `tx1 f`, map-load step wall | none needed before FL1. Do after FL1 only if spikes stay over 35 ms |
| Audio control join | 1 to 3 | G | 20 to 60 | Low to medium: control state exclusivity (AU2 cut line) | `audio wait avg`, phs `au`, audiohash | B5 |
| Mixer SIMD | 0 on core 0. 30 to 60 percent of the render time on core 1 | G | 150 to 250 | Low (bit exact) | `audio2:` render busy, `audio pcm:` underruns, audiohash | B5 (op profile) |
| ndsp voices | 0 on core 0. 15 to 30 percent of core 1 | G | 1000 to 1600 plus a sample converter | High: exactness, memory | `audio2:`, user listening | B5 (ndsp cost) |

### 3.1 The one hardware push (P5): micro-benchmarks and counters

One code row (BENCH, about 400 to 550 lines, all under switches, no change to the default path). One push with the user. The bench runs at boot, after the GPU and audio init and before the logos, about 20 s. Then the normal hands-off cycle runs with the counters and the GV1c A/B, so the same push is also GV1 push 4.

```
export O=<ip> B=<build>; sh 3ds/push_3ds.sh --no-build --ip $O -t 480 bench=draw,flush,tex,replay,ndsp,auprof,fbwait dlsrc audiostat pacestat logstat phase phsum noaudit loglevel=debug fixedseed statehash=10 phasecsv=300:400 abtog=base,gv1,fbwait:30
python3 -u 3ds/tools/ftppull.py $O o build3ds; cp build3ds/hw_o3ds_log.txt 3ds/logs/perf/hw_${B}_P5_pull.txt
python3 3ds/tools/phs_report.py 3ds/logs/perf/hw_${B}_P5_pull.txt > 3ds/logs/perf/hw_${B}_P5_table.txt
```
The switch names `bench`, `dlsrc`, `auprof`, `fbwait` and the abtog state `fbwait` are new (ab_words, pm_phase.c:63, needs the new name). User hands: none during the run.

Bench items (each writes one `bench <item>` line, cpu time with svcGetSystemTick, GPU time with the P3D event timing of the phc gpu field):
- B1 draw ladder. 500 draws each, 3-vertex triangles, scissor 8 x 8 px (GPU cost about 0): d0 C3D_DrawArrays with no state change; d1 d0 + C3D_TexEnvInit and one stage; d2 d0 + 6 stages; d3 d0 + C3D_TexBind of 2 alternating textures; d4 d0 + 4 C3D_FVUnifSet and one C3D_FVUnifMtx4x4; d5 d0 + C3D_BindProgram alternating prog and prog3; d6 the full pm_gpu path (pm_gpu_vert_clip of one triangle and a plan change per draw); r0 GPUCMD_AddRawCommands of a pre-built 24-word draw packet; r1 r0 + a pre-built 60-word state block; r2 a GPU call of a pre-built buffer with GPUREG_CMDBUF_JUMP0 and a patched return (also writes a known pixel and checks it with pm_gpu_read_frame). Line: `bench draw <item> n <N> us <cpu per draw x10> gpu <us per frame>`.
- B2 flush cost. GSPGPU_FlushDataCache of 64 B, 4 KB, 64 KB and 1 MB, 50 calls each, first before the audio init and then with the audio threads running. GX_FlushCacheRegions put in the GX queue (cpu cost of the enqueue). svcFlushProcessDataCache last and only with `bench=svcflush` (an SVC that the process may not call ends the process: log the line "svcflush start" first and flush the log). Line: `bench flush <kind> <size> audio <0|1> us <per call>`.
- B3 texture miss stages. 200 synthetic textures (32 x 32 RGBA5551 and 64 x 64 CI8 to RGBA5551): time C3D_TexInit, tx1_build, C3D_TexFlush and C3D_TexDelete apart. Line: `bench tex <size> init <us> decode <us> flush <us> free <us>`.
- B4 cached batch replay. One synthetic static mesh of 1000 triangles (16 B Vtx, index buffer) in linear memory and a copy in VRAM. Replay it 100 times per frame with prog4 and one matrix uniform per draw, for 10 frames. Line: `bench replay <linear|vram> draws 100 tris 100000 cpu_us_per_draw <n> gpu_ms <n>`. This gives the CPU cost of one replay and the PICA vertex rate (the GPU budget of 2f).
- B5 audio. (a) `auprof`: in the normal run, time per op class of pm_rsp_audio.c (ADPCM, resample, envelope mixer, mix, pole filter, interleave, reverb) per 300 steps, and the control job CPU time against its wall time (svcGetThreadInfo or tick reads around the work only). Line `audio3: adpcm <us> res <us> env <us> mix <us> fx <us> ctl_cpu <us> ctl_wall <us> per tick`. (b) `bench ndsp`: 24 ndsp channels, 16 PCM16 and 8 DSP-ADPCM looping buffers, for 5 s, with ndspChnSetMix and ndspChnSetRate on every channel at 60 Hz. Line: `bench ndsp ch 24 cpu_us_per_tick <n> underrun <n>`. Run it before the game audio starts.
- B6 `dlsrc` counters in the normal run, one line per 300 drawn steps per scene: ops, triangle commands and vertices by the region of the list pc and of the Vtx pointer (main list = gDisplayContext buffers, map shape = gMapShapeData, static data = .data and .rodata and the entity and animator data, other = heap and .bss), the number of distinct G_DL targets per frame, and the share of static-region targets whose 32-bit hash of the list bytes equals the hash of 30 frames earlier. Line: `dlsrc <mode> <map> ops m <n> s <n> o <n> tri m <n> s <n> o <n> vtx m <n> s <n> o <n> dl <n> stable <n>`.
- B7 `fbwait` A/B state: C3D_FrameBegin(0) instead of C3D_FRAME_SYNCDRAW at pm_gpu.c:1326, with an explicit wait for the GPU of the last frame only (gspWaitForP3D or the citro3d queue wait). The game step clock stays as today. Read: `pace:` drawn per second, phs `fb`, statehash equal.

### 3.2 Decision gates of the push

| Question | Go | Kill |
|---|---|---|
| Is the draw cost in citro3d's generic path? (B1) | d6 over 40 us and r0 + r1 under 10 us: go RAW1 | d0 under 15 us and d6 over 40 us: the cost is in our flush_draw code. Fix that in place (key build, compares, bind) and do not write RAW1 |
| Is the program switch the cost of the raw path? (B1 d5) | d5 - d0 over 20 us: one DVLB for all programs, at once (small row) | under 5 us: no change |
| Is a flush call a fixed cost? (B2) | 64 B call over 0.4 ms: go FL1 | 64 B call under 0.1 ms and the 1 MB call over 0.5 ms: the cost is per byte. Then flush only the written bytes and keep the per-texture flush |
| Where is the per-miss cost? (B3) | flush over 0.6 ms of the 1.21 ms: FL1 | init or free over 0.6 ms: slot pool first |
| Can a replay be cheap? (B4) | cpu per replayed draw under 15 us and gpu under 10 ms for 100,000 triangles: go DLC-G | cpu over 30 us: RAW1 first, then repeat B4 |
| Is the geometry static? (B6) | static share of triangle commands at or over 70 percent in world scenes and stable share at or over 95 percent: go DLC-G. Static share of state ops at or over 40 percent: go DLC-S | static triangles under 50 percent: no DLC. Stay with GV1 and RAW1 |
| Does the vblank wait cost drawn frames? (B7) | drawn per second +2 or more in world scenes, statehash equal: go FB1 | under +1: keep SYNCDRAW |
| Is ndsp worth a user question? (B5) | ndsp cpu under 0.5 ms per tick and auprof shows render over 50 percent of core 1: ask the user about non-exact audio | otherwise close the ndsp option |
| GV1c (abtog state gv1) | gb lower by 6 ms or more against base in world scenes, draws not higher (GV1.md 5) | under 4 ms: stop GV1d and later. DLC-G replaces the rest |

## 4. Staged roadmap

Order and gates:

1. Stage 0, now: BENCH row (part 3.1) and the P5 push with GV1c in it. No other HW push before it. Gate: the table of 3.2.
2. Stage 1 (small rows, any order, after P5): FL1 (if B2 and B3 pass), the one-DVLB change (if B1 d5 passes), FB1 wait change (if B7 passes). Then one HW push. Gate: gc under 1 ms, tex per miss under 0.4 ms in the `tx1 f` lines, step 332 under 40 ms, drawn per second in world scenes higher by 2 or more.
3. Stage 2: RAW1 with state blocks (if B1 passes). One HW push. Gate: gp+gd per draw under 15 us (now 66), menu and intro drawn per second higher by 4 or more, golden 10 scenes within TOL, statehash equal.
4. Stage 3: DLC-G, then DLC-S (if B4 and B6 pass). One HW push after each. Gate DLC-G: gt+gv under 3 ms in world scenes (now 13.2), `dlccheck` 0 mismatches, golden 10 scenes. Gate DLC-S: gb under 15 ms in world scenes. Two command buffers (FB1 part 2) go with DLC-G, because the compiled blocks need the same buffer ownership.
5. Stage 4 (optional): audio control join, mixer SIMD, texture per-map pack, VRAM tier, O8 on core 1. Each only if its metric shows a need after stage 3.

Stop doing now:
- The partial GPU offload paths that keep the CPU work per element: gpuvtx (PO3b, PO3c). Remove it with GV1f. No more A/B windows of it.
- GV1d and GV1e (decal2, zmark, fog on the raw path) until P5 decides DLC. They add GPU cases to a path that DLC-G replaces for static lists.
- Per-draw shadow work inside the citro3d path (more DR1-type compares). DR1 measured -0.09 us per draw. The cost is in the path, not in the repeated values.
- KICK1 tuning. The GPU is not late.
- Texture decode speed work (TX1-type) and the texbake pack in its per-texture form. The per-miss fixed cost is the limit.
- O13 VRAM texture tier and O8 pipelined interpreter on the O3DS, until stage 3 numbers exist.
- An ndsp port, unless the user accepts non-exact audio.
- Azahar-only gains in cost classes where Azahar does not predict the O3DS (system calls, cache, VFP latency, hw_rerank.md 5). A row that changes those classes needs a HW bench item first.

Honest range of the final speed (O3DS, play build, game steps stay at about 24 to 30 per second through FS1, the table gives drawn steps per second):

| Scene type | Today (M, build 926) | After stage 1 (E) | After stage 2 (E) | After stage 3 (G) |
|---|---|---|---|---|
| World, average | 14.4 drawn, 24.3 game, cpu50 55.2 ms | 16 to 18 | 18 to 21 (cpu 44 to 48) | 24 to 30 (cpu 25 to 36) |
| World, heavy (1:1, cpu50 87 ms, M) | under 12 | 12 to 14 | 13 to 16 | 18 to 25 |
| Battle | 15.8 drawn, 24.8 game, cpu50 51.6 | 17 to 19 | 19 to 23 | 24 to 30 |
| Intro | 16.6 drawn, 24.7 game, cpu50 37.7 | 18 to 20 | 22 to 27 | 27 to 30 |
| Menu (title, file select) | 20.1 drawn, 26.1 game, cpu50 36.1 | 22 to 24 | 27 to 30 | 30 |

Confidence: stage 1 high for the texture spikes (the fit has R2 0.996) and medium for the drawn rate. Stage 2 medium: it depends on B1. Stage 3 low to medium: it depends on the static share (B6) and on the main-list cost that stays (1 to 3 ms, G). A 30 drawn-steps-per-second world in every scene is not likely on the O3DS (G: 30 to 50 percent chance for average world scenes, under 20 percent for the heavy ones). The game speed (game steps per second) can reach 28 to 30 in most scenes with FS1b after stage 1 (E), because skipped steps cost about 0 ms of wall time (M).

## 5. Not measured, needed for the decisions

- The static share of triangles and ops (B6).
- The cost of one GSPGPU_FlushDataCache by size and with and without the audio threads (B2).
- The split of the 66 us per draw between citro3d and our flush_draw (B1).
- The shader-code upload at C3D_BindProgram between separate DVLBs (B1 d5).
- The PICA vertex rate for compiled replays and VRAM against linear vertex buffers (B4).
- The CPU time (not wall time) of the audio control job and the op split of the mixer (B5).
- The part of fb that is vblank alignment and the part that is the GPU tail (B7).
- The 4 ms of gb that the phasegbi split does not explain (hw_p3_findings.md part 2).
