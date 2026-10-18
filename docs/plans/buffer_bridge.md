# Buffer bridge: one model for N64 memory images on the PICA

Read-only planning, 2026-10-18. No build. No Azahar run. No source edit.
Inputs: docs/plans/render_gaps.md (13 cases), docs/plans/X02_darkness_notes.md, docs/plans/F03_F08.md, docs/plans/PO3.md, docs/MEMORY_BUDGET.md, the uncommitted X02 diff (pm_gbi.c, pm_gpu.c, pm_tex.c, pm_main.c, pm_gpu.h, pm_tex.h), reference/soh-3ds (`CopyFramebuffer`, `depth_snapshot_3ds.h`), reference/sm64-3ds (shadow map render target).
Status words: EST = estimate. HW = only hardware can measure. UNV = by code reading only.

Goal: one rule set in the port layer that decides, for each N64 image address, where its pixels live (PICA render target, PICA depth buffer, CPU memory) and how a read of that address gets them. No per-effect code. The decomp code stays unchanged. No stubs.

## 0. Fault found in the working tree (X02, read only)

| Item | Detail | Action |
|---|---|---|
| pm_gpu.c lines 259-267 (working tree) | The body of the removed `tmpnofill` check stays without its `if` line. It forces black RGB with texture alpha on every draw. Its closing brace ends `flush()` early. Expect a build error or wrong pictures. | The X02 agent deletes lines 259-267 before its test. UNV: no build was run. |

## 1. Image model

One table entry per N64 image that the port sees this frame. Key: (address range, pixel size). Fields: address, bytes, siz (1 = 8 bit, 2 = 16 bit, 3 = 32 bit), stride w, height h, role, frame of last GPU write.

The X02 slot table (`offs[]` in pm_gpu.c, `offt[]` in pm_tex.c) already has address, siz, w, h. The bridge adds one field (role) and one decision function `img_role()` in pm_gbi.c. No new file. No new abstraction layer.

### 1.1 Kinds of image memory

| Kind | Addresses | Size | Note |
|---|---|---|---|
| Current frame buffer | `nuGfxCfb_ptr` (one of gFrameBuf0..2) | 320 x 240 x 16 bit, 150 KB | The PICA main target holds its real pixels. N64 memory is stale. |
| Previous frame buffer | the other `nuGfxCfb[i]` | same | Filled by `pm_cfb_capture_prev` (CPU Morton read). |
| Z-buffer image | `nuGfxZBuffer` (heap, allocated on first use by `pm_zbuf_need`) | 150 KB | Used as depth (cases 4, 5, 6), as a colour image (cases 3, 7) and as CPU scratch (case 12). |
| Game images | `DepthCopyBuffer`, `SpriteShadingPalette`, draw_box window buffers, `D_800B3AE0` (flame), `ResetSavedFrameImg` | small | Each case has one role (table 1.2). |

### 1.2 Roles: the one decision

`img_role()` runs at `G_SETCIMG` (with the render mode at the first draw) and at `G_SETTIMG` / texture load. First matching row wins.

| # | Condition | Role | Where the pixels live | Who handles it | Cases |
|---|---|---|---|---|---|
| R1 | cimg = `DepthCopyBuffer` | DEPTHQ | CPU word, one pixel of PICA depth | F03 design (pm_depth.c) | 4 |
| R2 | cimg in a frame buffer, render mode VISCVG | CPUFILL | CPU memory (full coverage value) | D03 (exists) | 10 |
| R3 | cimg = current frame buffer | MAIN | PICA main target | exists | all normal draws |
| R4 | cimg = z-buffer image, siz 2, FILL cycle or render mode with Z_CMP or Z_UPD | DEPTH | PICA main depth buffer | new (slice B3) | 5, 6 |
| R5 | cimg is a TLUT palette target (tile in TMEM upper half) | CPUPAL | CPU memory | S15 `rect_to_image` (exists) | 2 |
| R6 | any other cimg (game buffer, z-buffer as colour, 8 or 32 bit) | SLOT | PICA off-screen render texture keyed by (address, siz) | X02 / V05 slots (exist) | 1, 3, 7, 8 |
| T1 | timg inside a SLOT image written this frame, same siz | bind the slot texture | VRAM | X02 `load_inner` (exists) | 1, 3, 7, 8 |
| T2 | timg inside the current frame buffer | GRAB | VRAM copy of the main target | new (slice B2) | 13 |
| T3 | timg inside the previous frame buffer | PREV | CPU memory, filled once per frame | X01 / D03 (exists) | 11 |
| T4 | timg in z-buffer image, no slot this frame | CPU memory | heap | D03 (exists) | 12 |
| T5 | any other timg | normal texture | CPU memory, pm_tex cache | exists | - |

Today the port has no T2. The `G_SETTIMG` case calls `pm_cfb_capture_prev()` for any frame buffer address (pm_gbi.c:1654). So a read of the current frame gets last frame in the other buffer, and the current buffer stays stale (render_gaps case 13). The bridge splits T2 from T3.

Today R4 does not exist. `pm_fb_native()` returns true for the z-buffer image, so a 16 bit colour image on the z-buffer draws on the screen (case 5: invisible by luck, alpha 0; case 6: wrong). R6 with the z-buffer as colour (case 7) also does not exist yet.

Case 9 (soft reset) has no 3DS path. It is out of scope (YAGNI). If it is reached, R6 gives it a slot.

### 1.3 Which images need a PICA target

| Image | Needs a PICA target | Reason |
|---|---|---|
| Current frame buffer | yes (main target, exists) | all scene draws |
| Z-buffer as depth | no new target (main depth buffer) | DEPTH role |
| Z-buffer as colour (darkness 32 and 8 bit, underwater copy) | yes, slot | a later draw samples it |
| draw_box windows, flame scratch | yes, slot | a later draw samples it |
| Current-frame grab | yes, one grab slot | the PICA cannot sample its active target |
| DepthCopyBuffer, palette, coverage fill, previous frame buffer | no | CPU reads or writes them |

## 2. Render to image, use as texture (X02 base)

### What X02 already gives

| Part | Where | Bridge use |
|---|---|---|
| Off-screen slot keyed by (address, siz), RGBA8 render texture in VRAM | pm_gpu.c `pm_gpu_off_begin` | role SLOT for every R6 image |
| Slot size 64 x 64 (16 KB) for stride up to 64, else 512 x 256 (512 KB) | same | memory control |
| Two slots on one address (32 bit and 8 bit darkness) | key includes siz | z-buffer image as 3 images (32, 16, 8 bit) |
| 8 bit image write: R to A stage, replace blend | `flush` (`off_i8`) | flame I8 (case 8) |
| Texture lookup by address range and siz, origin offset | pm_tex.c `load_inner` | role T1 |
| `offdump=<frame>` dumps every slot | pm_gpu.c `off_dump` | test evidence for every slice |
| z-buffer allocated before the display list names it | pm_main.c `__wrap_appendGfx_darkness_stencil` | keeps the address valid. Bridge moves the call to one place (B1) |

### What X02 must not break (and the bridge must not break)

| Feature | Row | Check |
|---|---|---|
| draw_box rotating windows (16 bit game buffers, 512 x 256 slot) | V05 | pause page flip shot or `offdump` in pause |
| Shading palette CPU render (`rect_to_image` runs before `off_screen_check`) | S15 | golden `toad` 0 diff |
| VISCVG coverage fill and pause blur on z-buffer CPU memory | D03 | golden `pause1` 0 diff |
| Last-frame capture for swirl, blur | X01 | golden `battle` 0 diff, battle start swirl |
| Prim depth fills (G_ZS_PRIM) | X01 | transitions, golden `title` |
| Golden frames | G01g | `golden.py toad yard battle title` 0 diff unless the slice says why |

The order of checks in `G_TEXRECT` stays: DEPTHQ (F03) first, then `rect_to_image` (S15), then the slot or main draw.

## 3. Mid-frame grab of the current frame buffer (case 13)

Users: water_splash (about 10 battle sites), motion_blur_flame, underwater, bulb_glow, effect_utils, flo_10 waterfx, mac_01 crystal ball, kgr_02 unused.

Mechanism (soh-3ds `CopyFramebuffer`, platform/3ds/source/gfx_citro3d.cpp:3227):

| Step | Action |
|---|---|
| 1 | T2 matches at the texture load. `flush()` the pending batch. |
| 2 | Find the game x range from the load tile (uls, lrs). The main target is rotated: game x maps to target row `fy = 40 + x`. One 8-row tile strip is 240 x 8 x 4 = 7,680 bytes and holds 8 game columns. A game x range is one contiguous byte range. |
| 3 | `C3D_FrameSplit(0)`, then `GX_TextureCopy` of those strips from `target->frameBuf.colorBuf` into a grab scratch texture (256 x 512 RGBA8, VRAM). Line 480, gap 32 (16 byte units, 256 - 240 = 16 pixels gap). The split orders the copy after every draw so far. |
| 4 | Draw one rotated quad from the scratch into an upright SLOT keyed by (current frame buffer address, siz 2). Every consumer then uses the existing T1 path (texrects and triangles alike). |
| 5 | Reuse: if no draw went to the main target since the last grab and the range is covered, bind the same slot. No new copy. |
| 6 | Cap: `GRAB_MAX` grabs per frame (start 8, a calibration value). Over the cap: reuse the last grab and log once. HW decides the value. |

Why an upright slot (step 4) and not a uv swap in the draw: the uv swap must go into the texrect path, the triangle path and later the PO3 shader. The upright slot does not depend on how vertices are transformed (section 9). Cost: one quad and one 512 KB slot.

Rejected: CPU read of the frame mid-frame. With `C3D_FRAME_SYNCDRAW` the GPU has not drawn this frame yet when the interpreter runs. A CPU read gets the last frame. A correct CPU read needs a GPU wait per grab (a stall per splash).

Check (B2 step 1): the citro3d GX queue must run the split command list and the copy in order, at `C3D_FrameEnd`. If the queue starts at the split, `pm_cfb_capture_prev` (CPU read of the last frame) must run before the first split of the frame. UNV in Azahar, HW for timing.

## 4. Depth (cases 4, 5, 6)

| Case | Role | Mechanism |
|---|---|---|
| 4 bulb_glow `is_point_visible` | DEPTHQ | F03 design as written: one pixel of the last finished PICA depth buffer (D24S8, Morton), encoded to the N64 float format, written to `DepthCopyBuffer[id]`. No change. |
| 5 z-buffer clear fill (world, battle, HUD camera) | DEPTH | Fill into the z-buffer image = depth write on the main target in the fill rectangle: colour write off, depth test always, depth = decode of the fill word as N64 z (0xFFFC = far). One quad, no split. |
| 6 flo_10 water, mac_01 crystal ball: model drawn into the z-buffer image with Z_CMP, then decal copy with prim depth 32734 | DEPTH | Model draw = depth write only (colour write off) with depth test on, depth = the value the combiner writes as colour, as a constant per draw (same path as the X01 `G_ZS_PRIM` fill: `pm_gpu_fill_rect_z`). The later decal uses the existing prim depth test. |

Rejected for case 6: a stencil mask. The N64 compare is a depth compare against the scene depth too, not an inside or outside test. The 8 bit stencil of the target stays free.

B3 step 1 is a log, not code: for each draw with cimg = z-buffer image, log the cycle type, render mode, combiner and prim colour in flo_10, mac_01 and toad. The R4 condition (FILL or Z_CMP or Z_UPD) and the case 6 depth value are confirmed from that log before code. If the case 6 combiner output is not a constant, stop and report (no guess).

Case 5 risk: today the port does not clear depth between the world, battle and HUD cameras by this fill. After B3 it does. A golden change in `battle` or `toad` is then expected where the HUD was hidden by world depth. Each diff needs one explanation.

## 5. CPU reads of image memory

| Reader | Image | Copy back? | Mechanism |
|---|---|---|---|
| `is_point_visible`, `is_model_center_visible` | DepthCopyBuffer | yes, one 16 bit word per query (max 16 per frame) | F03 |
| Pause blur filter (background_gfx.c:342) | previous frame buffer, z-buffer image | yes, 150 KB once per pause open | D03 `pm_cfb_capture_prev` (keep) |
| `draw_prev_frame_buffer` (swirl, blur, dizzy_dial, firework_rocket) | previous frame buffer | yes today (CPU 150 KB per active frame) | keep for correctness. A GX copy into a slot is an optimization (after S28). |
| Shading palette | CPU-written 16 x 1 image | no copy, CPU owns it | S15 |
| Darkness, flame scratch, draw_box windows, grab, underwater copy | SLOT images | no. Stay on the GPU | no CPU reader exists in src (render_gaps sections A and B) |
| Coverage fill | frame buffer | CPU write only | D03 |

Rule: an image copies back to CPU memory only when a CPU reader is in this table. A new CPU reader needs a new row here. Slot images keep stale N64 memory by design.

## 6. Memory cost

The VRAM pool (6 MB on the O3DS) is not in docs/MEMORY_BUDGET.md. The heap and linear numbers are.

| Item | Pool | KB | Status |
|---|---|---|---|
| Main target colour 240 x 400 RGBA8 + depth D24S8 | VRAM | 750 | exists |
| Slots: OFF_N = 4, worst case 4 x 512 x 256 RGBA8 | VRAM | 2,048 | exists (V05, X02) |
| Darkness slots 2 x 64 x 64 | VRAM | 32 | X02, inside OFF_N |
| Grab scratch 256 x 512 RGBA8 | VRAM | 512 | new (B2) |
| Grab upright slot | VRAM | 0 extra | uses one of OFF_N. If OFF_N must grow to 5: +512 |
| VRAM total, worst case | VRAM | 3,310 to 3,822 of 6,144 | EST |
| z-buffer image | heap | 150 | exists (D03, X02) |
| Previous frame buffer copy | game BSS (gFrameBuf) | 0 new | exists |
| Code, all bridge slices | 3dsx | about 8 (1 to 2 per slice) | EST, each slice under the 10 KB rule |
| Linear pool | linear | 0 new | grab copy is VRAM to VRAM |

Heap spare (6,657 KB EST, MEMORY_BUDGET) changes by about -8 KB code only. Add one VRAM line to MEMORY_BUDGET.md in B1 (not in this commit). HW: VRAM fragmentation after many map changes (slots are made again when the size changes, X02).

## 7. PICA limits

| Limit | Effect on the bridge |
|---|---|
| Render target colour formats: RGBA8, RGB8, RGBA5551, RGB565, RGBA4. Depth: 16, 24, 24S8 | Slots use RGBA8. 8 bit images use the R to A stage (X02). 32 bit images keep full RGBA8. |
| Render texture size: power of 2, 8 to 1024 per side | 64 x 64 and 512 x 256 slots. The 320 x 240 game image fits 512 x 256. |
| A target cannot be sampled while it is the draw target | grab needs a copy (section 3) |
| `GX_TextureCopy`: raw tiled copy, 16 byte units, line and gap | strip copy of the rotated main target, no format change |
| `GX_DisplayTransfer`: tiled to linear, format change, scale, vertical flip, no rotation | not used by the bridge. Useful later for a CPU read of the previous frame (optimization) |
| `C3D_FrameSplit`: ends the command list mid-frame | one per grab. Cost HW |
| Depth buffer is not a sampleable depth texture | F03 reads one pixel on the CPU. No depth texture needed by any case |
| `C3D_RenderTargetClear` mid-frame (slot clear on first use) | GX memory fill. Ordering with the command list: UNV (X02 uses it today) |
| Texenv: 6 stages. X02 I8 stage, V06 mix stage and F08 fog stage all need `env_hi < 6` | a 6-stage combiner into an I8 slot loses the R to A stage. B1 logs this count. |
| Batch breaks | each SETCIMG change (`flush` + `C3D_FrameDrawOn`), each slot bind, each grab (flush + split). PO3 R1 already warns that draws per frame rise (120 to 208 today). B1 adds `splits` and `slots` counters to the `stats` line. |

## 8. Slice list (in order)

MODEL RULE: this plan was `opus` (planning). Code slices in the graphics layer use `sonnet`. Table and run slices use `haiku`. All B slices are CORRECTNESS work, allowed before S28. The optimization items (GX copy for the previous frame, grab strip size tuning past the cap, VRAM shrink) wait for S28 (OPT rule).

| Slice | Model | Kind | Work | Narrow test | Replaces or uses rows |
|---|---|---|---|---|---|
| B0 | sonnet | correctness | Finish X02 as its row says. Delete the stray block (section 0). Plain circle and Watt tests from X02_darkness_notes. | `warp=kgr_01:0 partner=1`, `offdump`, one log line per slot. golden toad yard battle 0 diff. | X02 (is the row) |
| B1 | sonnet | correctness, no picture change | `img_role()` in pm_gbi.c with table 1.2. Move today's checks (`pm_fb_native`, `pm_is_cfb`, `cimg_siz != 2`, VISCVG, `rect_to_image` guard) behind it. One `pm_zbuf_need()` call site at the first z-buffer use. `imglog` switch: one line per SETCIMG and SETTIMG with role, address kind, siz, stride, render mode. `stats` counters: splits, slots, grabs. Add the VRAM line to MEMORY_BUDGET.md. | golden toad yard battle title pause1: 0 diff. `imglog` table for kgr_01, flo_10, mac_01, one battle with water_splash. | none replaced. Feeds B2 to B5. Confirms V10 is not a memory image case (one `imglog` line in the Strategies menu). |
| B2 | sonnet | correctness | Role T2 grab: split T2 from T3 at `G_SETTIMG`, strip `GX_TextureCopy`, upright slot, reuse, `GRAB_MAX`. Step 1: confirm queue order (section 3). | `warp=flo_10:<entry>` (agent confirms the entry in pm_warp.c), `offdump` of the grab slot shows the scene. Log: grabs per frame. golden toad yard battle 0 diff. | new row for render_gaps case 13 |
| B3 | sonnet | correctness | Role DEPTH: case 5 fill as depth write, case 6 model as depth write. Step 1 is the log in section 4. | `warp=mac_01:<entry>` crystal ball and flo_10: `drawlog` shows the decal draw passes in the model area only. golden 4 scenes, each diff explained. | new row for cases 5 and 6 |
| B4 | sonnet | correctness | F03 as designed (role DEPTHQ). | F03 A/B check (host test `test_zenc.c`, hos_05 `F03 query` lines). | F03 (is the row) |
| B5 | sonnet | correctness | Underwater (case 7): z-buffer as colour goes to a slot by B1, the source comes from B2. Add `G_MODIFYVTX` to the opcode switch (GBI opcode, not a memory image, but the effect needs it). | Sushie battle move or `fx_underwater` site (agent finds a warp or preset, no new stand-in). `offdump` of the z-buffer slot, log of modified vertices. | new row for case 7 |
| B6 | sonnet | correctness | Flame scratch I8 (case 8) and ring_blast: chroma key (`G_SETKEYR`, `G_SETKEYGB`) to an alpha test or a combiner key. The I8 slot comes from X02. | isk battle stage with the flame effect, `offdump` of the I8 slot, one shot. | new row for case 8 |
| B7 | haiku (needs the user, after S28) | measure | O3DS run: phase times, grabs and splits per frame, VRAM `mem` line, swirl capture time, F03 query time. Table only. If unclear: relaunch on sonnet. | `push-o`, `pull-o`, `phase_report.py`. | none |

Rows that the bridge does not replace: V10 (box pattern combiner, `box_pattern_rect`, not a memory image, by reading). F08 (fog). Case 9 (no path).

Order notes: B0 before everything (file conflict). B1 before B2, B3, B5. B4 can run directly after B0 (F03 design is ready and independent). B5 after B2. Only one build agent at a time (graphics layer).

## 9. Risk from the vertex work (PO2, PO3)

PO2 moves the transform to `G_VTX` on the CPU. PO3 moves it into the PICA vertex shader. Some bridge parts depend on how vertices are transformed. They can break or need rework at those slices.

| Bridge part | Depends on | What can break | Re-check |
|---|---|---|---|
| F03 depth sense and scale (B4) | depth map (`C3D_DepthMap`, GREATER, near = large) and the clip z range | PO3 can fold the PICA depth range into the matrix (soh-3ds does). The sense or scale of D24 then changes. The glow is always hidden or always shown. | F03 step-1 log of d24 for the hos_05 lamp, plus `test_zenc.c` |
| DEPTH role writes (B3) and the X01 prim depth | `G_ZS_PRIM` depth `z = zn * w` on the CPU (draw_tri) | PO3c moves it into a uniform `zmix`. A wrong value hides the case 6 decal or the case 5 clear. | flo_10 and mac_01 `drawlog` of B3, golden battle (HUD camera) |
| 3D draws into slots (case 6 model, crystal ball, underwater mesh) | `rebuild_3d` picks `off_proj` (no tilt) or `projection` (tilt) | PO3 composes `combined` with viewport and tilt in one uniform. If it uses the screen tilt for a slot, the slot image is rotated 90 degrees. | `offdump` of one 3D draw into a slot (mac_01) |
| Slot and grab texture size in the uv | `vert_uv` calls `pm_tex_size` (64 or 512) per corner | PO3 uv uniform `s = 1/(32 tw)` per batch. A slot bind must break the batch and update the uniform. | `offdump` plus one shot of water_splash, golden pause1 (V05 windows) |
| Grab split (B2) | `flush()` writes all pending vertices before the split | PO3 vertex and index rings must be in the batch before `C3D_FrameSplit`. The cache flush stays before `C3D_FrameEnd`. | grab slot `offdump` on the GPU path (`gpuvtx` on and off) |
| G_MODIFYVTX (B5) | vertices kept in loaded form until the triangle | PO2 transforms at G_VTX: a later screen xy or uv change must patch the transformed vertex. PO3: a modified vertex must take the CPU path. | underwater `offdump`, log of modified vertices on both paths |
| Last-frame reads (F03, `pm_cfb_capture_prev`) | `C3D_FrameBegin(SYNCDRAW)`: GPU idle during the interpret | If PO2 or PO3 make the frame begin asynchronous, the CPU reads a half-drawn frame. | F03 query log and swirl golden `battle` after the change |
| Batch count | breaks from slots, grabs, SETCIMG | PO3 R1 adds breaks per matrix id. Both add up. Command buffer and draws per frame can reach the limit (`GPU LIMIT NEAR` log). | `stats` line: draws, splits, slots, command buffer % in flo_10 and a water_splash battle |

Re-check per PO slice (add to each PO row):

| PO slice | Re-check to run |
|---|---|
| PO2 | B5 underwater log (G_MODIFYVTX on transformed vertices). F03 query log if frame begin changes. golden toad yard battle. |
| PO3a | none (counters only). Add `splits` and `slots` to the PO3a table. |
| PO3b | `gpuvtx` on: F03 d24 log at hos_05, `offdump` of a 3D draw into a slot (mac_01), grab slot (flo_10), `stats` draws and splits. |
| PO3c | B3 depth writes (flo_10, mac_01 `drawlog`), X01 prim depth fills (title), golden battle. |
| PO3d | darkness (kgr_01) and water_splash shots: lit draws into slots and grabs. |
| PO3e | culling per target winding: slots have no tilt, so the winding differs from the screen. `offdump` of a 3D draw into a slot, golden pause1. |
| PO3f | B7 table again on the O3DS with `gpuvtx` on and off. |
| PO3g | full golden plus every bridge narrow test above, once. |

## 10. Only hardware can measure

| Item | Why |
|---|---|
| `GX_TextureCopy` time per strip and per full frame | Azahar has no VRAM or GX bandwidth model |
| `C3D_FrameSplit` cost per grab, best `GRAB_MAX` | GPU and GSP timing |
| `pm_cfb_capture_prev` CPU read of VRAM (150 KB) | VRAM read speed from the ARM11 |
| VRAM fragmentation after many map changes | allocator state over a long run |
| Real depth precision on the bulb glow edge | PICA D24 rounding differs from Azahar |
| GX queue order for the split, copy and slot clear | Azahar can show the order, not the stall |

## 11. Report

Slices: 8 (B0 to B7), 7 correctness (B0 to B6), 1 measure (B7). X02 gives the slot table keyed by (address, siz), sized slots, 8 bit write, slot texture lookup and `offdump`. Biggest risk: the grab path (B2) and its ordering with `C3D_FrameSplit`, plus the PO3 rework list in section 9. Memory: VRAM +512 KB (3.3 to 3.8 MB of 6 MB worst case), heap 0, code about 8 KB.
