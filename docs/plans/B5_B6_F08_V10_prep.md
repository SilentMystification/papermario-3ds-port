# Prep: B5, B6, F08, V10 (read-only, 2026-10-18)

No source edit. No build. No Azahar run. Line numbers come from the working tree of 2026-10-18 (F03 edits in progress). They will move.
Status words: UNV = by code reading only. HW = only hardware can measure.
Sources: 3ds/AGENT_RULES.md, docs/plans/buffer_bridge.md, docs/plans/F03_F08.md, docs/plans/render_gaps.md, 3ds/SLICES.md rows, src/effects/underwater.c, flame.c, ring_blast.c, src/draw_box.c.

## Shared rules for all four rows
- One build agent at a time. All four rows edit pm_gbi.c (and F08 also pm_gpu.c).
- Run F03 first (it edits `case G_TEXRECT` and adds pm_depth.c). Then do the rows in the order below.
- Each Azahar start costs about 17 s. Use `-Turbo -Smoke -FaultStop`. At most 3 build and run rounds per row.
- Every row: append a line to 3ds/PROGRESS.log, move the done row to docs/SLICES_DONE.md, at most 3 lines in HANDOFF.md, commit own files only.

## Order and overlaps

| Order | Row | Reason |
|---|---|---|
| 1 | V10 | Smallest. Touches only `box_pattern_rect` and its caller. No dependency. |
| 2 | B6 | Touches `G_SETKEYR`/`G_SETKEYGB` (new cases) and the combiner (`cc_rgb_in`, plan). Needs the X02 I8 slot (exists in the tree). |
| 3 | F08 | Touches `G_MOVEWORD`, `xform_vtx`, `sync_render`, pm_gpu.c texenv build. Needs V06 mix stage (done part). |
| 4 | B5 | Needs B1 (`img_role`, z-buffer colour image to slot) and B2 (current frame grab). Do not start before both are done. |

Overlap table.

| Pair | Shared place | Effect |
|---|---|---|
| B5 and F08 | `xform_vtx`, `draw_tri` | B5 patches the screen xy after `xform_vtx`. F08 patches vertex alpha inside `xform_vtx`. Different fields. Apply F08 first. Then B5 patches x and y only. |
| B6 and F08 | `sync_combine`, `sync_render`, pm_gpu.c texenv build | B6 changes the combiner input. F08 adds a texenv stage after the V06 mix stage. Both need `env_hi < 6`. Run F08 and B6 in separate slices. Check the stage count in both. |
| V10 and F03 | `case G_TEXRECT` | F03 branch is at the top (`if (cimg == DepthCopyBuffer)`). V10 edits only the `box_pattern_rect` call and the function. No line overlap. |
| V10 and B1/B2 | `case G_TEXRECT` order | B1 moves the cimg checks behind `img_role()`. Keep the order DEPTHQ, `rect_to_image`, `off_screen_check`, `box_pattern_rect`. |
| B5 and B3 | z-buffer image | B3 role DEPTH (R4) must not capture the underwater z-buffer colour image. R4 needs FILL cycle or Z_CMP or Z_UPD. The underwater render mode has `ZMODE_OPA` with no Z_CMP. Check this in B1 `imglog`. |
| PO2 and PO3 | `G_VTX`, `xform_vtx` | OPT rows, after S28. B5 must leave a re-check line for PO2 (section 9 of buffer_bridge.md). |

## V10 Strategies and level-up box checkerboard

### Facts (UNV)
- draw_box (src/draw_box.c:497-762) loads tile 0 (`bgImage`, line 503-531) and tile 1 (`cornersImage`, line 533-726). Each corner idx 0..3 loads tile 1 (`gDPLoadMultiTile`) and draws one quad.
- Quad draw has two forms. If `quads != nullptr` (3D and rotating windows) the corner is drawn with `gSPVertex` and `gSP2Triangles` (line 703-707, 757-758). Else it uses `gSPScisTextureRectangle` (line 711-720, 760).
- `box_pattern_rect` (pm_gbi.c:1297) runs only from `case G_TEXRECT` (line 1630). It does the tile 0 times tile 1 product on the CPU. The triangle path (`draw_tri`) has no such code.
- `box_pat` is set in `sync_combine` (pm_gbi.c:306-318) only when the combiner is `(PRIM - ENV) * TEXEL0 + ENV` and the alpha reads TEXEL1 only.
- `box_pattern_rect` returns 0 (the checker texture then draws as a normal rect) when: `!box_pat`, `!td[0].valid`, `!td[1].valid`, `!td[0].src`, `!td[1].src`, or `tile_texel` fails for `(fmt*4+siz)` not in the switch at pm_gbi.c:1268-1283. Missing case examples: fmt 0 siz 3 (RGBA32), fmt 2 siz 2, IA16 with the pal check. A failed `pm_tex_load_rgba8` also returns 0.
- The checker is tile 0 (ui/box/bg_tile.png, I4 0/F pattern, draw_box.c:189-196). A white and brown checker on screen means tile 0 is drawn alone with the PRIM/ENV colours. So the product path did not run for those draws.
- The Strategies list and the level-up box are in battle. The main window works in other scenes (pause, dialog). The new fact in battle is a draw_box call with different flags.

### Likely cause (UNV, rank order)
1. The draw uses the `quads` triangle path (a rotating or 3D window, flags `DRAW_FLAG_ROTATING` or similar in the battle submenu code). Then `draw_tri` binds tile 0 only (`ensure_tile`, `dual_bind`) and the product is never built. Check: `ddverb`/`xt` log shows `G_TRI2` and no texrect for that window.
2. `box_pattern_rect` returns 0 at the guard because `td[1]` is not valid at the first corner quad, or `tile_texel` returns 0 for the corner format of this window (`cornersFmt`/`cornersBitDepth` of the battle menu image differs from the pause box).
3. A different window style with `bgImage` and no `cornersImage` (draw_box.c:727, `else if (bgImage != nullptr)`). Then `td[1]` is stale from an earlier window and `box_pat` is set. A stale `td[1]` gives a wrong product. A failed guard gives the checker.

### Exact change (after step 1 confirms the branch)
1. `3ds/src/pm_gbi.c`, `case G_TEXRECT` (about line 1630) and `box_pattern_rect` (line 1297-1345): add one `xt_on()` log line at each `return 0` of the guard line 1302-1303 and at line 1333. Print `box_pat`, `td[0].valid`, `td[1].valid`, `td[0].fmt`, `td[0].siz`, `td[1].fmt`, `td[1].siz`. Use the existing `xt` switch. No new switch.
2. Fix by branch.
   - Cause 1: add the same product to the triangle path. In `draw_tri`, before `ensure_tile(...)`, if `box_pat` and `box_pattern_tri` applies: compute the screen bounding box of the four vertices (an axis-aligned quad, 2 triangles) and call `box_pattern_rect`-like code with the quad tc as `hs`/`hd`. Smallest form: in `case G_TRI2`, if `box_pat` and the 6 indices are `0,3,1,0,2,3`, derive `xl,yl,xh,yh` from `verts[0]` and `verts[3]` (`ob` x/y after the matrix gives screen xy for the 2D case, use `xform_vtx` then divide by w) and `hs`/`hd` from `tc[]` of vertex 0 and 3. Call `box_pattern_rect`. Extract the quad logic into a helper `box_pattern_quad(float x0, float y0, float x1, float y1, float s0, float t0, float dsdx, float dtdy)` and call it from both places. Do not copy the loop.
   - Cause 2: add the missing `(fmt*4+siz)` case to `tile_texel` (line 1268-1283) for the format the log names. Decode as the decomp format table does.
   - Cause 3: reset `td[1].valid` when `G_SETTILE`/`G_LOADTILE` for tile 1 is not set since the last `G_SETCOMBINE`. Or skip `box_pat` if `td[1]` was not loaded after the last window start. Confirm from the log first.
3. Do not add a guard that hides the fault. If the log shows a root cause in tile state, fix the tile state.

### Test (one command)
`powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop warp=kmr_09:1 story=ch0_goombario partner=1 shots` with A taps for the tutorial text up to Strategies (hunt text from the V10 row in docs/HISTORY.md line 1379). Add `xt=<frame range>` for the log. Done check: the Strategies list is readable in a shot. Then `python 3ds/tools/golden.py toad yard battle pause1` must show 0 diff, except `battle` if it shows the menu (explain by eye).

Memory: 0 to 1 KB code. Not covered: other draw_box styles (dialog, name entry).

## F08 fog remainder

Design is complete in docs/plans/F03_F08.md (section "F08 fog remainder"). Exact change:

| File | Function and place | Change |
|---|---|---|
| 3ds/src/pm_gbi.c | statics near `fogc` (line 376) | add `static int fog_fm, fog_fo;` |
| 3ds/src/pm_gbi.c | `case G_MOVEWORD` (line 1850-1863) | before the `index != G_MW_SEGMENT` ignore log, add `if (index == G_MW_FOG) { fog_fm = (s16)(w1 >> 16); fog_fo = (s16)w1; break; }`. `G_MW_FOG` is index 0x08 (log text `gbi ignore moveword index 08`). |
| 3ds/src/pm_gbi.c | `xform_vtx` (line 478-492) | after the transform: `if (geom & G_FOG) { float winv = o->w > 0.f ? 1.f / o->w : 32767.f; float f = o->z * winv * fog_fm + fog_fo; o->a = f < 0.f ? 0.f : f > 255.f ? 255.f : f; }`. `lerp_clip` already interpolates `a`. |
| 3ds/src/pm_gbi.c | `sync_render` mix block (line 379-385) | new branch for blender cycle 1 `p == 3 (FOG), a == 2 (SHADE), mm == 0 (IN), b == 0 (1MA)`: `pm_gpu_set_fogmix(1, fogc)`. Keep the V06 branch. Else `pm_gpu_set_fogmix(0, 0)`. Switch `nofog` (via `pm_debug_has("nofog")`) forces 0. |
| 3ds/src/pm_gpu.c | next to `pm_gpu_set_mix` (about line 846) | new `pm_gpu_set_fogmix(int on, unsigned rgba)`. Statics `r_fogmix`, `r_fogc`. Reset where `r_mix` resets. |
| 3ds/src/pm_gpu.c | texenv build after the V06 mix stage (about line 247-258) | new stage when `r_fogmix`: `C3D_TexEnvSrc(e, C3D_RGB, GPU_CONSTANT, GPU_PREVIOUS, GPU_PRIMARY_COLOR); C3D_TexEnvOpRgb(e, SRC_COLOR, SRC_COLOR, SRC_ALPHA); C3D_TexEnvFunc(e, C3D_RGB, GPU_INTERPOLATE);` alpha channel `REPLACE PREVIOUS`. Guard `env_hi < 6`. Set the constant colour to the fog rgb. |
| 3ds/include/pm_gpu.h | prototypes | add `void pm_gpu_set_fogmix(int on, unsigned rgba);` |
| 3ds/src/pm_gbi.c | `G_VTX` log or `ddverb` line | add `fog fm fo` to the existing `ddverb` line (no new switch). |

Step 1 (log, no code): at the first `G_FOG` draw log `geom & G_FOG` at `G_VTX` time and at triangle time. If they differ, move the alpha write from `xform_vtx` to the point where `geom` is read at `G_VTX`.
Step 2 (log): the `comb total` line count of fog class combiners with 6 stages. If not 0, use the PICA fog LUT for those draws (plan section "Alternative").

Test (one command): `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop warp=mim_11:0 shots` (confirm the mim_11 entry in 3ds/src/pm_warp.c first, it has no preset in the grep). Run once with `nofog` for the A/B shot. Done check: the far trees and NPC fade to the fog colour. The `stats` line `fogblend` count rises. Then `python 3ds/tools/golden.py toad yard battle title` must show 0 diff (if `battle` differs, check an animated model with fog by eye).

Risks: fog-class combiners that read SHADE alpha as object alpha now match the RDP, so those pictures change. PICA alpha interpolation is perspective-correct, the RSP value is screen linear (small, accepted by sm64-3ds). Memory: about 0.6 KB code, 3dsx about +1 KB.

## B6 flame scratch I8 and ring_blast chroma key

### Facts (UNV)
- flame.c:198-199 sends `gDPSetKeyR(keyCenter, colorScale.r, 0)` and `gDPSetKeyGB(keyCenter, colorScale.g, 0, keyCenter, colorScale.b, 0)`. ring_blast.c:120-121 does the same with `var_f4 * 211`, `255`, `216`.
- The port ignores both opcodes (falls to `default: note(op)`, pm_gbi.c:1874).
- The combiner PM_CC_4B (include/macros.h:519) is `PRIMITIVE, CENTER, COMBINED, ENVIRONMENT` in cycle 2 RGB: `(PRIM - CENTER) * COMBINED + ENV`. So the key is not an alpha test. CENTER (mux value 6 for input b) is a constant colour (key centre r, g, b). The key scale and width are not read by this combiner.
- `cc_rgb_in` (pm_gbi.c:1100-1111) returns 0 for sel 6 in input b (`default`). The mux decode has no KEY constant in the plan builder. So the row text "alpha test or combiner key" is wrong: use a combiner constant.

### Exact change
1. `3ds/src/pm_gbi.c`: add statics `static unsigned keyc;` (RGB centre, packed like `envc`).
2. `case G_SETKEYR` (opcode 0xeb): `w1 = [wR:12 | cR:8 | sR:8]`. Set `keyc` red byte = `(w1 >> 8) & 0xff`. `break`.
3. `case G_SETKEYGB` (opcode 0xea): `w1 = [cG:8 | sG:8 | cB:8 | sB:8]`. Set green byte = `(w1 >> 24) & 0xff`, blue byte = `(w1 >> 8) & 0xff`. `break`. (Layout from reference/soh-3ds/patches/libultraship-3ds.patch line 1970-1990.)
4. Combiner: where the cycle plan feeds a constant for mux 6 in RGB input b (`cc_rgb_in` `case 6`, plus the plan builder at pm_gbi.c:750-780 and the matching texenv source in pm_gpu.c), return the key centre colour. If the plan builder has no free constant register, check the stage count first (`comb total` log). Likely route: `pm_gpu_set_env`-style call `pm_gpu_set_key(keyc)` and use `GPU_CONSTANT` with a second constant buffer, or fold CENTER into PRIM minus CENTER on the CPU when `comb2_a == PRIM` and `comb2_b == CENTER` (both constants): `K = PRIM - CENTER` per channel as the constant, then `K * COMBINED + ENV`. The fold needs no new stage. Do the fold.
5. Both opcodes also appear in ring_blast with `PM_CC_4B`. The same code serves both. Do not add a ring_blast special case.
6. Check the opcode value names in the port gbi.h (`G_SETKEYR`, `G_SETKEYGB`) before the `case`. They exist in reference/paperboat/include/PR/gbi.h:192-193.

### Test (one command)
`powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop warp=isk_04:<entry> offdump=<frame> shots` (the isk_04 to isk_08 stages show the flame. The agent finds the entry in pm_warp.c or adds a preset in the run script, no new stand-in). Done check: `offdump` of the I8 slot shows the flame mask, one shot shows the flame with colour from PRIM/ENV, not black or flat. Then `python 3ds/tools/golden.py toad yard battle` 0 diff.

Memory: about 0.3 KB code. Not covered: ring_blast in a battle (only code shared). Overlap: none with F08 if the fold is on the CPU (no new texenv stage).

## B5 underwater (case 7) and G_MODIFYVTX

Dependency: B1 (`img_role()`, R6 z-buffer as colour goes to a slot keyed by (address, siz 2)) and B2 (current frame grab, T2). Do not start before both are done. If B1 or B2 is open, write `BLOCKED: B1/B2 open` in HANDOFF.md and stop.

### Facts (UNV)
- underwater.c:167 sets the colour image to `nuGfxZBuffer`. Lines 171-185: 40 strips of 6 lines from `nuGfxCfb_ptr` (the current frame, T2 read) are copied into it. Line 187 sets the colour image back to the frame buffer.
- Line 191 sets the texture image to `nuGfxZBuffer` (T1 read, siz 2). Lines 203-281: 12 x 18 cells, each with `gDPLoadTile` from the z-buffer, 4 `gSPModifyVertex(..., G_MWO_POINT_XYSCREEN, val)` and a `gSP2Triangles`. The vertices live in `D_09000428_3B9E98` (16 vertices loaded at line 201).
- Value layout: `val = (x << 0x12) | (y * 4 + dy)`. High 16 bits = screen x in 10.2 fixed (`x << 2 << 16`). Low 16 bits = screen y in 10.2 fixed. `where = 0x18` (G_MWO_POINT_XYSCREEN, gbi.h:1329). `w0 = op(0x02)<<24 | where<<16 | vtx*2`. `w1 = val`.
- Port: no `case G_MODIFYVTX`. Falls to `default` (note). `G_VTX` keeps vertices in `verts[]` (object coords) and `vert_m[]` (matrix at load). `draw_tri` calls `xform_vtx(vert_m[i], &verts[i], &poly[i])` at triangle time (pm_gbi.c:881-883). A screen-space vertex needs no matrix.

### Exact change
1. `3ds/src/pm_gbi.c`: add statics near `vert_live` (line 148): `static u8 vert_scr[64]; static s16 vert_sx[64], vert_sy[64];`.
2. `case G_VTX` (line 1396-1421): after `vert_live[v0+i] = 1;` add `vert_scr[v0 + i] = 0;` so a reload clears the override.
3. New `case G_MODIFYVTX:` (opcode 0x02) next to `case G_VTX`: `unsigned where = (w0 >> 16) & 0xff; unsigned vi = ((w0 >> 1) & 0x7fff);` (vertex index = `w0 & 0xffff` / 2 in F3DEX2: check gbi.h:2288-2294 for the exact shift). If `vi < 64 && where == G_MWO_POINT_XYSCREEN`: `vert_sx[vi] = (s16)(w1 >> 16); vert_sy[vi] = (s16)(w1 & 0xffff); vert_scr[vi] = 1;`. Other `where` values: log once with the existing `note` style. Count modified vertices (`st_modv`) for the log line (done check asks for the log of modified vertices). Log the first 8 and the count per frame.
4. `draw_tri` (line 881-883) and the clip path: after `xform_vtx` for index `idx[i]`, `if (vert_scr[idx[i]]) { screen x = vert_sx / 4.f, y = vert_sy / 4.f; set poly[i] to clip space so the existing viewport mapping gives that pixel }`. Compute clip coordinates from the current viewport the same way the viewport code maps `x/w`. Set `w = 1`, `z` from the RSP rule (`G_MWO_POINT_XYSCREEN` keeps z). Use `z` of the transformed vertex, so call the normal transform first and replace only `x` and `y` (and `w` stays). Then `x_clip = ((sx - vp_cx) / vp_sx) * w`, with `vp_cx`, `vp_sx` the current viewport centre and scale (read them from the `G_MOVEMEM` viewport code at pm_gbi.c:1444-1460). Do not touch the lights or the fog alpha.
5. Texture source: with B1 role R6, the z-buffer colour image (siz 2, 320 stride) is a slot. With B2 the strips from `nuGfxCfb_ptr` fill that slot (T2). The `G_LOADTILE` at line 246-250 then binds the slot by T1 (`load_inner`). No extra B5 code for the source if B1 and B2 work. If the slot is empty (black), B5 reports BLOCKED on B2.
6. `rect_to_image` (S15) must not claim the z-buffer strips. Check order in `case G_TEXRECT`: DEPTHQ, `rect_to_image`, slot draw.

### Test (one command)
`powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop offdump=<frame> shots` with a Sushie battle or the script call `fx_underwater` (render_gaps case 7: partner/sushie.c:336, fx_api.c). The agent finds the warp or preset in pm_warp.c or the battle presets. No new stand-in. Done check: `offdump` of the z-buffer slot shows the scene, the log shows modified vertices (count 4 per cell, 864 per frame for 216 cells), one shot shows a warped mesh. Then `python 3ds/tools/golden.py toad yard battle` 0 diff.

Risks: PO2 and PO3 move the vertex transform (buffer_bridge.md section 9). Leave the re-check line in the PO2 row: the underwater log with G_MODIFYVTX on transformed vertices. HW: GPU cost of 216 cells with a texture bind each (batch breaks). Memory: about 0.5 KB code.

## Not checked in this prep
- No source line was run or compiled.
- The V10 cause is unconfirmed. Step 1 (the `xt` log) decides between causes 1, 2 and 3.
- The F3DEX2 shift for the vertex index in `G_MODIFYVTX` needs a look at gbi.h:2288-2300 before the `case` is written.
- The mim_11 and isk entries were not found in pm_warp.c by grep (only `yard` and `toad` presets matched). The slice agent confirms them.
