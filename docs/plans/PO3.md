# PO3 plan: GPU vertex transform on the PICA200

Written 2026-10-18. Read-only plan. No build. No Azahar run. Numbers are Azahar numbers from PT01 (docs/HISTORY.md, "Table 1") unless marked.

## 1. Current draw path (offsets in 3ds/src)

| Step | Where | What it does |
|---|---|---|
| G_VTX | pm_gbi.c:1389-1414 | memcpy of n Vtx (16 B each) into `verts[64]`. memcpy of the 64 B `combined` matrix into `vert_m[slot]` for each vertex. Limit: 64 slots. |
| G_MTX, G_POPMTX, viewport | pm_gbi.c:1003-1026, 1442-1462, `upload_mvp` 205 | Unpack the fixed point matrix, multiply into `mv[mv_sp]` (stack 10) or `proj`, build `combined = mv * proj`. |
| G_TRI1, G_TRI2 | `draw_tri` pm_gbi.c:839-1001 | Per corner: `xform_vtx` 471 (16 mul-add), `vert_color` 418 (fixed light if G_LIGHTING), `vert_uv` 435 (calls `pm_tex_size`, 2 divides). Per triangle: `clip_bits` x3 (528), cull by clip space area (885), `clip_near` at w = 0.05 (503), colour overrides (929-991), `G_ZS_PRIM` depth (960), `pack_color` x3, `pm_gpu_set_mvp(ident_m)` (64 B memcmp), `pm_gpu_vert_clip` x3. |
| Emit | pm_gpu.c:378-404 `put4`, 904 | 40 B float vertex (`Vert`) per corner, 4 divides by 255. Batch flush at 510 vertices. |
| Draw | pm_gpu.c:146-349 `flush` | Uniform `projection` (shader_3d = viewport and tilt), texenv, `C3D_DrawArrays`. Pool `VERT_SLOTS` 32,768 x 40 B = 1,280 KB linear (pm_gpu.c:20, 446). |
| Shader | 3ds/shaders/pm.v.pica | One dp4 x4 against `projection`. 3D vertices arrive in clip space with an identity game matrix. |

Cost now (mac_00, steady frame): 1,357 triangle commands, 291 drawn (79 % rejected after the full transform), 2,237 vertices loaded, 4,071 corners transformed. GBI interpret 11.1 ms (mac_00) to 15.3 ms (kmr_20). Triangle ops are about half of it (R3, timer inflates it). The per corner work is done again for each triangle that uses the vertex (about 1.8 times per vertex). EST on the O3DS: slower than Azahar (no cache model in Azahar).

Reference ports: soh-3ds (`src/pica/fast3d.v.pica`) and sm64-3ds (`src/pc/gfx/shader.v.pica`) both transform on the CPU and send clip space vertices. Neither does a full GPU transform. The useful parts: soh-3ds sends colour as `GPU_UNSIGNED_BYTE x4` and scales by 1/255 in the shader. soh-3ds folds the PICA depth range [-w, 0] into the CPU matrix. PO3 has no reference to copy. It is new port-layer work.

## 2. What moves to the GPU

| Work | Now | After PO3 |
|---|---|---|
| Position transform | CPU per corner | Shader. Matrix = `combined` composed with the viewport and tilt (`rebuild_3d`), one uniform per batch |
| Trivial reject (`clip_bits`) | CPU | GPU clipper |
| Back and front cull | CPU clip space area | `C3D_CullFace`, winding chosen per target (rotated screen or off-screen) |
| Near clip | CPU at w = 0.05 | GPU clipper at the near plane (z = -w after the remap) |
| UV | CPU `vert_uv` per corner | Shader: `uv = tc * s + o`, uniform per batch (s = 1/(32 tw), o = -tile_x0/tw). The existing `uvx` uniform stays for TEXEL1 |
| Colour unpack | CPU, 4 divides | Shader: u8 x4 times 1/255 |
| Colour overrides (mode 5 ENV, plan constant, alpha 255, boost, see 929-991) | CPU per corner | Shader: `clr = clr * cmul + cadd`, two uniforms per batch. Boost: cmul 0.5, cadd (1 - env)/2 (the output colour clamps) |
| `G_ZS_PRIM` depth | CPU `z = zn * w` | Shader: `z = z * za + w * zb`, one uniform |
| Fixed light (`vert_color`) | CPU | Shader, same formula: `0.35 + 0.65 * clamp(n . (0.25, 0.85, 0.45))`. Normals are the same 4 bytes, read as signed in the shader (`slt` and a subtract of 256) |
| Per vertex matrix (`vert_m`) | 64 B copy per vertex | Matrix id per slot (from PO2). A triangle whose 3 slots share one id takes the GPU path |
| Index list | none (3 vertices per triangle) | u16 index per corner into a per-frame vertex ring, `C3D_DrawElements` |

Stays on the CPU: the display list walk, state ops, `vert_live` checks, `off_screen_check`, `box_fold_sync`, `comb_audit`, `ensure_tile`, `sync_render`, 2D rectangles and fills.

CPU fallback path (the PO2 code) for: a triangle with mixed matrix ids, debug switches that need positions (`drawlog`, `ddverb`, `depthview`, `rmlog`, `stats` keep counts), and `nogpuvtx` (A/B switch).

## 3. Vertex shader and attributes

New program (second DVLE next to pm.v.pica, or a second shader library if `ctr_add_shader_library` takes one file only). Keep the rule from pm.v.pica: write each output component once.

| Attribute | Offset in Vtx | Format | Note |
|---|---|---|---|
| v0 position | 0 | GPU_SHORT x4 | ob[3] plus `flag`. Shader sets w = 1 |
| v1 texcoord | 8 | GPU_SHORT x2 | tc[2], s10.5 |
| v2 colour or normal | 12 | GPU_UNSIGNED_BYTE x4 | cn[4] |

Stride 16 B, no padding. One buffer. The Vtx is copied raw (no conversion) from the game buffer into a linear ring at G_VTX (game heap is not GPU readable). Vtx must be native little endian in RAM (the CPU path already reads it so).

Uniforms per batch: `mvp[4]`, `uvso` (s, o), `uvx` (exists), `cmul`, `cadd`, `zmix`, `light` (dir and lit flag, or bool uniform with `ifu`). About 10 vectors of 96. citro3d uploads only dirty uniforms.

Batch break (flush) on: matrix id change, uv uniform change, colour uniform change, cull mode change, path change (GPU and CPU path have different shaders and layouts), plus every break that exists now.

## 4. How each command is handled

| Command | Handling |
|---|---|
| G_VTX | memcpy n x 16 B into the ring. Slot table: slot -> ring index, slot -> matrix id. No 64 B matrix copy. |
| G_MTX, G_POPMTX, G_MOVEMEM viewport | As now, plus a new matrix id (epoch counter) when `combined` changes. The GPU matrix is built once per id, not per triangle. |
| G_TRI1, G_TRI2 | Checks as now. Same id on 3 slots: append 3 u16 indices. Else: CPU path. |
| G_LIGHTING | Shader, fixed light (same as today). Real light directions stay a later row. |
| G_FOG | Not done now (F08). Fog factor is a later shader or PICA fog LUT job. PO3 keeps today's output. |
| G_TEXTURE_GEN | Not done now (counted only). PO3 keeps today's output. A later row can add it in the shader. |
| Clipping | GPU. CPU near clip and reject are skipped on the GPU path. |
| G_ZS_PRIM, overrides | Uniforms (section 2). |

## 5. Memory cost (budget: docs/MEMORY_BUDGET.md, pool margin 2,712 KB)

| Item | KB | Tag |
|---|---|---|
| Vertex ring 16,384 x 16 B (2 x a frame; mac_00 loads 2,237, kmr_20 max to measure in PO3a) | +256 | EST |
| Index ring 32,768 x u16 (max 2,599 triangle commands per frame seen) | +64 | EST |
| Code (C about 3 to 5 KB) and shader (under 0.5 KB) | +5 | EST |
| `Vert` pool shrink after the GPU path is the default: 32,768 to 8,192 slots (peak use now 4,344, most of it 3D) | -960 | EST, measure with the `mem ... vertex slots` line first |
| Net at the end | -635 | EST |

The +320 KB comes from the linear pool margin (2,712 KB). The pool size and the heap do not change. Each slice stays under the 10 KB code and data rule. The ring sizes are linear data, not image. They are inside the pool margin.

## 6. A/B check

- Add `--extra "<switches>"` to 3ds/tools/golden.py (it has no switch pass-through now). Run `golden.py` (switch off, must stay 0 differences) and `golden.py --extra gpuvtx` (switch on). Same TOL 8 and PCT 0.5 (battle 3.0).
- Extra scenes for near plane and cull: the yard cliff skirt (V02, in `yard`), a camera close to a wall (kmr_20 door), pause pages (off-screen targets, `pause1` to `pause4`).
- Smoke on presets yard and toad with `-Turbo -Smoke`, both switch states.
- Timing in Azahar with `phase`: report phase 3 and 4c. Azahar timing is not valid for the O3DS.

## 7. Risks

| # | Risk | Effect | Mitigation |
|---|---|---|---|
| R1 | Draw count rises: one batch per matrix id. Today 120 to 208 draws per frame | DrawElements and uniform cost may eat the gain | PO3a counts matrix ids per frame first. If high: matrix palette (up to about 18 matrices in uniforms, index by `mova`) as a later slice |
| R2 | Clip and cull semantics change: today a CPU cut at w = 0.05, reject only when all corners are off one side. The GPU clips partly at near and far | Pixel changes near the camera and at far geometry | Golden A/B, near wall scenes. Per mesh CPU fallback if a scene differs |
| R3 | Cull winding differs between the rotated screen and the off-screen target (y flip) | Missing walls or missing pause pages | Test yard and pause1 to 4 |
| R4 | Azahar shader, attribute and clipper emulation differ from the PICA | Golden passes in Azahar but hardware differs | Hardware shots on the O3DS (PO3f) |
| R5 | Mixed matrix triangles are common | Small gain | PO3a measures it before PO3b |
| R6 | Command buffer (4 MB) use grows with uniforms | Draws drop without a message | H02 stat `command buffer %` in the `mem` line |
| R7 | RISK (RSP to PICA port): effects that render into or read back N64 memory images (z-buffer, colour image, framebuffer grabs: darkness X02, depth readback F03, water_splash and similar current-frame grabs, see docs/plans/render_gaps.md and docs/plans/buffer_bridge.md when it exists) may break or need rework when RSP vertex work moves to the PICA GPU (PO3, PO2). Check each such case again at every PO3 slice (A/B with golden frames and the render_gaps list) | Buffer readers render wrong or stop working after the move | Check each case at every PO3 slice: golden A/B and the render_gaps list |

## 8. What only the O3DS run can measure

- Real GBI interpret time before and after (phase 3), with PO2 in place.
- PICA vertex cost for all triangle commands (now 79 % are rejected on the CPU and never reach the GPU).
- Real cost per `C3D_DrawElements` and per uniform upload (Azahar models GSP and GPU time).
- FrameBegin wait: whether the GPU becomes the limit.
- Whether the PICA clipper and culling give the same picture as Azahar (R4).

## 9. Dependencies and conflicts

| Item | Kind | Note |
|---|---|---|
| PO2 | Needs first | PO3 uses the PO2 matrix id per slot and the PO2 per vertex cache as the CPU fallback. PO2 touches the same code (`draw_tri`, G_VTX case). PO3a can run before PO2. PO3b and later wait for PO2. PO2 waits for the user answer (QUESTIONS.md PO2). |
| S28 | Gate | Go or no-go for PO3b: start only if the O3DS run (after PO2) still shows a large GBI interpret share. PO3f needs the user (O3DS on). |
| X02 | File conflict now | Uncommitted edits in pm_gpu.c, pm_gbi.c, pm_gpu.h, pm_tex.c, CMakeLists.txt (off-screen slots). PO3 code waits for the X02 commit. |
| V06, V10, F03 | File conflict | Graphics layer, pm_gbi.c and pm_gpu.c. Do not run at the same time as a PO3 code slice. |
| F08 | Logic conflict | The per vertex fog factor goes into the vertex colour in `draw_tri`. Do F08 on the CPU path first, then PO3c copies the formula into the shader (or F08 uses the PICA fog LUT). |
| F06, F07 | Later users | 60 fps interpolation and widescreen change the matrix and viewport. With PO3 they change uniforms only. Plan them after PO3. |

## 10. Slice list (MODEL RULE: no opus for code, sonnet for graphics layer, haiku for table runs)

1. PO3a (sonnet): counters under `stats`: vertices loaded, triangle commands, triangles with mixed matrix ids, matrix ids per frame, lit, prim z, override cases. Table for 5 scenes (title, toad, yard, kmr_20, battle). Go or no-go for R1 and R5. Under 1 KB.
2. PO3b (sonnet): after PO2, S28 and X02. New shader program, attribute layout, vertex and index rings, `gpuvtx` switch (default off). GPU path only for the plain case (unlit, no override, one matrix id). All other cases use the CPU path. Add `--extra` to golden.py. A/B: golden off = 0 diff, golden on within TOL.
3. PO3c (sonnet): colour overrides and `G_ZS_PRIM` as uniforms. Golden on, battle and pause scenes.
4. PO3d (sonnet): fixed light in the shader. Golden on, toad and battle.
5. PO3e (sonnet): GPU culling with per target winding, skip CPU reject on the GPU path. Golden on, yard, pause1 to 4, kmr_20 near wall.
6. PO3f (haiku, needs the user): O3DS run with `push-o`, `phase phasecsv`, switch off and on, `pull-o`, `phase_report.py`. Table: phase 3, 4c, FrameBegin, draws, command buffer %. If unclear, relaunch on sonnet.
7. PO3g (sonnet): `gpuvtx` default on, `nogpuvtx` kept for A/B. Shrink `VERT_SLOTS` by the measured peak. Write the 3dsx size and the `mem` line. Full golden and the normal chain.
