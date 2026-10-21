# GV1 plan: full GPU geometry path on the O3DS

Written 2026-10-20 by GV1PLAN. Read-only plan. No build, no Azahar start, no console access.
Code basis: git HEAD 42f40d643 (3ds/src/pm_gbi.c, pm_gpu.c, 3ds/shaders/pm.v.pica, pm3.v.pica). Line numbers are HEAD lines. The G1 agent edits pm_gbi.c, pm_gpu.c and pm_tex.c now (rows HWM1, TX1, DR1, KICK1). Section 7 lists the G1 changes that this plan needs.
Inputs: docs/plans/hw_rerank.md (rows GV1, DR1, KICK1, HWM1), docs/plans/PO3.md (R1 to R7), 3ds/logs/perf/po3a_azahar_845.txt, docs/plans/H06a.md (section "PO3 and PO2"), docs/plans/H06b.md, docs/plans/intro_dip.md, docs/plans/V18_V19_azahar.md, docs/MEMORY_BUDGET.md.
Tags: M = measured, R = Azahar value times the HW ratio, E = estimate from code, G = guess.

## 0. Facts that set the design

- HW: gb 36.03 ms per world step, 8.9 us per gbi op, 63 us per draw (gp + gd), the GPU idles 75 to 85 percent of a step (hw_rerank.md 1.3, 2.1).
- Intro dip window (pc 1702 on): 1808 triangle commands, 290 drawn, 3168 vertex loads, 185 to 195 draws, 5679 gbi ops. 84 percent of the triangles are rejected after the transform (intro_dip.md 1).
- po3a (build 845, full cycle): 20.7 matrix ids per frame (max 204 in the intro), 0.0 percent mixed-id triangles, 0.1 percent lit, 0 percent texgen, 0 screen vertices, 0 fog, 85.8 percent plain, 87.5 percent plain with alpha 255 allowed, 1681 vertex loads per frame on average.
- The partial path PO3b and PO3c (switch gpuvtx) gave no Azahar gain (dipb_gv_off.txt and dipb_gv_on.txt: gb 25.05 and 25.07, gd 1.98 and 2.44). Cause in the HEAD code:
  - G_VTX still transforms every vertex, makes its clip bits, its colour and its S T (pm_gbi.c:1989 to 2010), and adds a 16 B ring copy (pm_gbi.c:1987).
  - A GPU triangle still runs the reject, the CPU cull with 8 multiplies, the n_keep loop (1229 to 1261), the stat block (1265 to 1275), ensure_tile, dual_bind (1277 to 1280), sync_render (1291) and gpu_emit. gpu_emit calls pm_tex_size and does 4 divides per triangle (1166 to 1185). pm_gpu_gtri does 6 memcmp per triangle (pm_gpu.c:1218).
  - One matrix per batch: pm_gpu_gtri flushes at each matrix change (pm_gpu.c:1218). The CPU path uses the identity matrix and does not break at a matrix change. So the raw path has more draws.

### 0.1 Two HEAD faults found by this plan (rows, no user decision)

- GV0a (HW fault of the existing gpuvtx path): the vertex ring `gring` and the index ring `gidx` (linearAlloc, pm_gpu.c:741 and 742) are never written back from the data cache. The only GSPGPU_FlushDataCache of the frame is pm_gpu.c:1127, and it covers the CPU vertex pool only. Azahar has no cache model, so the fault does not show there. On the O3DS the gpuvtx path can read old ring data (wrong or missing triangles). The Push 3 plan of hw_rerank.md section 4 has the state `gpuvtx` in abtog. Its picture and its timing are not valid until this is fixed. Fix: flush `gring` (gring_n x 16 B) and `gidx` (gidx_n x 2 B) next to line 1127, and at every split (next row). About 6 lines. Send to the live G1 agent with KICK1 (rule 10).
- GV0b (HW fault of the grab split): split_frame (pm_gpu.c:185 to 192) and the grab split at pm_gpu.c:1004 call C3D_FrameSplit. The GPU then runs draws that read the vertex pool before the flush at pm_gpu.c:1127. The vertices written since the frame start can still be in the data cache. Frames with a grab (B2, b7 grab counts) can show old vertices on the O3DS. Fix: flush the vertex pool range (and the two rings) written since the last split before each C3D_FrameSplit. This is the same code as the KICK1 split flush. About 10 lines. Send to the live G1 agent with KICK1.

## 1. Target design

### 1.1 Principle

The CPU decodes the display list, keeps the RDP and RSP state, binds textures and builds the combiner. For the common case it does not transform, clip, cull or emit vertices. Per vertex at G_VTX: a 16 B copy, a 2 B palette index store and a bounding box update (integer). Per triangle: the op decode, the live check, one AND of box bits, one check of a cached decision, 3 index stores.
The CPU path stays for rare cases. It reads the ring copy of the vertex (a snapshot at load time, H06a cause 1), not the game Vtx.

### 1.2 What runs on the PICA vertex shader (new program pm4.v.pica, from pm3.v.pica)

| Part | GPU form | Exact? |
|---|---|---|
| Position | `pos = M[k] * (ob.x, ob.y, ob.z, 1)`, k = palette index from the Vtx flag field of the ring copy (v0.w, short). `mova a0.x` then 4 dp4 against `pal[a0.x + 0..3]`. M[k] = post x game (below) | Float24 product instead of fp32 product then float24 (section 3.3) |
| Matrix palette | 21 entries x 4 vec4 = 84 float uniforms. Other uniforms: uvx, uvso, cmul, cadd, zmix, consts = 6. Total 90 of 96 | Exact (same matrices) |
| Viewport, wide mode, tilt, off-screen y flip, depth map | Folded into the post matrix of rebuild_3d (pm_gpu.c:94 to 118): view (vp_sx, vp_sy, vp_tx, vp_ty, 1.25 for wide and not ui_ortho, 40 px margin, off_active flip), z' = 0.5 z + 0.5 w, then `projection` (OrthoTilt) or `off_proj`. The CPU composes post x game in fp32 once per palette entry at the flush, because off_screen_check can change the target between G_VTX and the triangle | Exact (the CPU path uses the same post matrix) |
| N64 viewport time | The CPU path applies the viewport current at the triangle (pm_gbi.c:1311, rebuild_3d at set_mvp). GV1 keeps that: the post matrix is the one at the triangle, the batch breaks when it changes (as now, pm_gpu.c:1218 g_vp) | Exact against the CPU path |
| UV | `uv = tc * uvso.xy + uvso.zw` (pm3.v.pica). uvso from tex_ss, tex_ts, the tile shift, tile_x0, tile_y0 and the texture size, once per state epoch (1.4). H06b1 (texture off) keeps its offset form (pm_gbi.c:1181 to 1185) | Exact (same formula as vert_uv, pm_gbi.c:611 to 624) |
| UV of TEXEL1 | `uvx` as now | Exact |
| Colour | `clr = cn * (1/255) * cmul + cadd`. ENV (mode 5), plan constant, alpha 255, boost (pm_gbi.c:1186 to 1189) | Exact in the cases PO3c covers. The colour clamp of pack_color is the output clamp of the PICA |
| G_ZS_PRIM | `z = z * zmix.x + w * zmix.y` (pm3.v.pica) | Exact |
| Lighting | Not on the GPU. 0.1 percent of the triangles are lit (po3a). A lit load uses the CPU path (1.5) | n/a |
| Texgen | Not on the GPU (0 percent). CPU path | n/a |
| Fog (F08) | Not on the GPU in the first slices. po3a shows fog 0. Option GV1e: `a = clamp(z * rcp(w) * fm + fo)` | GV1e only if a counter shows use |
| Far plane, x and y planes | PICA clipper. The CPU path already sends clip positions through the same clipper (pm.v.pica, identity game matrix), so this part does not change | Exact |
| Trivial reject | Box reject on the CPU per G_VTX (1.3), the rest is clipped by the PICA | Exact: the box bits are a subset of the vertex bits, and the PICA clipper removes the rest with no visible pixel |
| Back and front cull | C3D_CullFace per batch from geom & G_CULL_BOTH and the winding sign of the post matrix. G_CULL_BOTH: the CPU drops the triangle (no draw) | Equal in theory (1.6). Test V02 yard cliff |
| Near plane | A load whose box reaches z < -w or w < 1 takes the CPU path (clip_near, pm_gbi.c:731). All other loads are in front of the near plane and need no clip | Exact (the CPU clip code runs for every triangle that can need it) |

Geometry shader: the PICA200 can run a geometry shader on one of its 4 shader units (citro3d `shaderProgramSetGsh`). Then 3 units are left for the vertex shader, the GS has its own 96 float uniforms, and the output of a GS is the input of the fixed clipper. GV1 does not need a GS: the cull and the clip are fixed function, the palette fits the vertex shader. Do not use a GS. Reason: it adds a program switch per draw and cuts the vertex rate.

### 1.3 Box reject (replaces the per-vertex clip bits of DIP1)

At G_VTX: integer min and max of ob x, y, z over the n vertices (6 compares per vertex). Transform the 8 corners of the box with `combined` (32 multiply-adds each, 256 per load, against 16 x n now). Box bits = AND of the 8 corner clip_bits4 (pm_gbi.c:1143). If all 8 corners are off one side, every vertex is off that side. Store the box bits in vert_cb of each slot of the load.
Triangle: `vert_cb[i0] & vert_cb[i1] & vert_cb[i2]` as now (pm_gbi.c:1237). A triangle that the box test keeps and the old vertex test rejects goes to the GPU, and the PICA clips it away. The picture is equal. Only the counters change (3.2).
Cost on the intro dip: 191 loads x 256 = 48896 multiply-adds against 3168 x 16 = 50688 now. That is equal. So the per-vertex test is cheaper when a load has fewer than 16 vertices. Rule: n < 16 uses the vertex transform for the bits (as now, but no colour, no S T, no vert_p store), n >= 16 uses the box. The box gives fewer rejects: GV1a measures `gvrej` against the old reject count.
Near flag: box bit 64 = some corner has w < 1 or z < -w. A load with bit 64 is a near load (1.5).

### 1.4 State epoch (per-triangle work to index only)

A counter `gv_epoch` goes up at every op that can change a triangle decision: G_SETCOMBINE, G_SETOTHERMODE_L and _H, G_GEOMETRYMODE, G_TEXTURE, G_SETTILE, G_SETTILESIZE, G_LOADBLOCK, G_LOADTILE, G_LOADTLUT, G_SETTIMG, G_SETCIMG, G_SETZIMG, G_SETPRIMCOLOR, G_SETENVCOLOR, G_SETFOGCOLOR, G_SETBLENDCOLOR, G_MOVEWORD (fog, segment), G_MOVEMEM (viewport, light), G_MTX, G_POPMTX, G_SETSCISSOR, the G_TEXRECT and fill ops (they bind other state), and the frame start.
The first triangle after an epoch change runs the full prep: off_screen_check, box_fold_sync, comb_audit, the stat block, ensure_tile, dual_bind, sync_render, and the decision (eligible for GPU, uvso, cmul, cadd, zmix, cull mode, the fallback reason). It stores the decision with the epoch. A triangle with the same epoch uses the stored decision. The stat block counts per triangle stay (an add of the stored flags).
This also takes the part of O3r that is in draw_tri (hw_rerank.md rank 6).
Risk: an op that changes state and is not in the list. Mitigation: switch `gv1check` (3.2) runs the full prep on every triangle and counts the triangles where the decision differs from the stored one (`gvstale`). The gate is gvstale 0.

### 1.5 CPU fallback (the CPU path stays for these)

Per load (decided at G_VTX, from the state at the load, as the RSP does):
- Lit (G_LIGHTING) or texgen: the load runs the old code (vert_color, vert_st, vert_p) at once. Reason `lit`.
- Ring full (gring_n + n > GRING_VTX): old code at once. Reason `ring`.
Per triangle (lazy transform of the 3 slots from the ring copy and the palette matrix of the slot, flag `vert_pv`):
- Near load (box bit 64): `near`.
- Mixed palette slots that are not in one draw palette: never a fallback. The palette holds the matrices of the 3 slots (1.7). `mixed` stays 0.
- Screen vertex (G_MODIFYVTX, vert_scr): `scr`.
- G_ZS_PRIM with the depth image role (B3, ROLE_DEPTH, r_zmark): `zmark`. GV1d moves it to the GPU (pass 2 is zmix x 0, y zn).
- Decal two-pass (V16b, r_decal && r_blend && decal2_on): `decal2`. GV1d moves it to the GPU (two DrawElements with the same indices, the stencil state of pm_gpu.c:582 to 595).
- Plan reads TEXEL1 and the aux texture is an off-screen slot (t1_fold needs the uv range, pm_gbi.c:1315, 1110): `tex1off`. When the aux texture is not a slot, t1_fold returns at once, so the GPU path is exact.
- Wide mode and orthographic UI (F07-7, pm_gpu.c:1214, 1364): `wideui`. The edge vertex move needs the CPU.
- Fog with fogmix_on: `fog`. GV1e moves it if the counter shows use.
- Debug switches drawlog, ddverb, depthview, rmlog, xtrace, po3a: `dbg`.
Fallback counter `gvfb` with one sub-count per reason in the `gv1` log line.

### 1.6 Cull on the GPU

The CPU rule (pm_gbi.c:1242 to 1253 and 1327 to 1340): cross = 2D cross of the homogeneous edges, sign flipped by the parity of negative w. G_CULL_FRONT culls cross <= 0, G_CULL_BACK culls cross >= 0. This is the homogeneous orientation test (sign of det[x y w] of the 3 corners). It gives the winding of the visible part after the projection. The PICA culls after the clip in window space, so the results are equal for every triangle with a visible part. A zero-area triangle has no pixels on either path.
Winding sign s = sign(vp_sx) x sign(vp_sy) x (off_active ? -1 : 1) x the sign of the OrthoTilt determinant. With s > 0, N64 back = GPU_CULL_BACK_CCW, else GPU_CULL_FRONT_CCW. GV1a finds the right sign by gv1check: it computes the CPU cull for each GPU triangle and the window-space winding of the post-matrix result, and counts the disagreements (`gvcull`). The gate is gvcull 0.
sh_cull (pm_gpu.c:281) is GPU_CULL_NONE for every CPU batch (pm_gpu.c:539). The GPU batch sets its mode. The CPU path keeps NONE.

### 1.7 Matrix palette and batching

- gm_tab (pm_gbi.c:250, 256 entries per frame) stays the frame table of the game matrices, one entry per distinct `combined` at a G_VTX.
- Each open batch has a palette of up to 21 gm_tab indices. A triangle whose 3 slot matrices are in the palette, or fit in the free entries, joins the batch. Else the batch is drawn and a new palette starts.
- The palette index k is written into the flag field (bytes 6 and 7) of the ring copy at G_VTX. The slot keeps its gm_tab index. When a slot joins a batch whose palette has its matrix at another index, the vertex must be copied again in the ring with the new k (16 B, rare: only after a palette flush between the load and the triangle). Counter `gvrecopy`.
- Upload at the flush: k entries, each composed post x game (64 multiply-adds), 4 C3D_FVUnifSet per entry. Use the DR1 uniform shadow (section 7) so an entry that is equal to the last upload at that index is not sent again.
- More than 21 matrices in one material run: the batch flushes and the next starts. Measure `gvpalmax` and `gvpalflush`. 20.7 ids per frame on average (po3a), so one material run seldom needs more than 21.
- Batch keys (a change draws the open batch): palette full, post matrix (viewport, target, wide flag), uvso, cmul, cadd, zmix, cull mode, and every flush cause that exists now (texture bind, combiner, render mode, scissor). The key is the epoch decision of 1.4, so the compare is one integer, not 6 memcmp.
- Draw count target: equal to or lower than the CPU path. The CPU path does not break at a matrix change. With the palette, GV1 does not either. Draws of rejected material runs: a run whose triangles are all box-rejected makes no draw (as now). A run that the box keeps and the old vertex test rejects makes a draw that the CPU path did not make. Counter `gvxdraw` (draws with no triangle that the old test keeps, from gv1check). This is the main draw risk of the intro dip (207 G_SETCOMBINE per frame).

## 2. Data flow and memory

| Item | Size | Now | GV1 |
|---|---|---|---|
| Vtx ring `gring` | GRING_VTX 16384 x 16 B = 256 KB, linear (pm_gpu.c:70, 741) | exists | Keep. 1681 loads per frame on average, 3168 in the dip. Peak per frame: new counter `gvloadmax` in the `mem` line. With fps60 (F06) each drawn frame resets the ring at pm_gpu_begin (pm_gpu.c:1088). Full ring: the load takes the old path (exact) |
| Index ring `gidx` | GRING_IDX 65536 x 2 B = 128 KB (pm_gpu.c:742) | exists | Keep. Worst case 2599 triangle commands per frame (PO3.md 5) = 7797 indices with no reject |
| Palette | 84 float uniforms | none | Uploaded per draw, only changed entries |
| Per-draw uniforms | uvso, cmul, cadd, zmix, uvx | exist | Shadow compare (DR1) |
| CPU vertex pool | VERT_SLOTS 32768 x 40 B = 1280 KB | exists | GV1f: shrink to 8192 after the measure of `verts_max` with gv1 on (-960 KB, PO3.md 5) |
| Command buffer | 4 MB (C3D_Init, pm_gpu.c:695) | exists | Per draw: about 200 B of state plus 70 B per uploaded matrix. 150 draws x (200 + 3 x 70) = 62 KB, 1.5 percent. Watch `command buffer %` in the `mem` line |
| Code | about 6 to 8 KB C, under 0.5 KB shader | | |

The rings are already allocated at boot (pm_gpu.c:741 and 742), also with gpuvtx off. So GV1 adds no new linear memory before GV1f, and GV1f frees 960 KB. Pool margin 2,712 KB (MEMORY_BUDGET.md 2). No memory risk.

Cache flush: `gring`, `gidx` and the CPU pool must be written back before the GPU reads them: at pm_gpu_end (pm_gpu.c:1125 to 1129) and at each C3D_FrameSplit (KICK1 and the grab splits). See GV0a and GV0b.

Per-draw cost: 63 us on HW (M). Rules that keep draws few: the palette (no break at a matrix change), the epoch key (no break when state ops repeat equal values, the O5 and O3 compare work of intro_dip.md 6), one BufInfo per frame for the ring (DR1 item 1, the indices are absolute ring indices, so DrawElements needs no new BufInfo), no draw for a fully box-rejected run.

## 3. Exactness

### 3.1 Picture gate

- `python 3ds/tools/golden.py --extra gv1 <scenes>` (golden.py:43 and 73 have --extra). TOL 8, PCT 0.5, battle 3.0 (golden.py:19, 20). No new tolerance.
- `golden.py` without the switch must stay 0 differences (gv1 default off until GV1f).
- Extra shot checks (shots with shotwin, compare gv1 against base by golden.py --extra or a pair of shots): V02 yard cliff skirt (cull), V18 sanctuary pan (shotwin=3490:40), V19 Sushie dive (demoscene=5 preset=toad shotwin=225:30), kmr_20 door (near wall), pause1 to pause4 (off-screen targets, y flip winding), battle (decal and shadow), title (wide UI quads with wide on).

### 3.2 Counters

triout (n_in + n_cut, pm_gbi.c:1547) and vtxslot count CPU work. With gv1 on they change by design. New counters (C_ ids in pm_port.h:61, so the pc and phs lines carry them):

| Counter | Meaning | Rule |
|---|---|---|
| gvload | vertices copied to the ring | equals vtxload minus the old-path loads (lit, ring) |
| gvtri | triangles sent to the GPU (indices / 3) | |
| gvrej | triangles rejected by the box bits | |
| gvfb | triangles on the CPU path, with sub-counts per reason (1.5) | |
| gvxf | lazy CPU transforms of slots | under 5 percent of gvload in world scenes |
| gvpal | matrices uploaded, gvpalmax, gvpalflush | |
| gvrecopy | ring copies for a palette index change | |
| tri | triangle commands | equal to base (exact) |
| tri balance | tri = drop + gvrej + gvtri + cpu-path triangles (out, in, cut, drop) | exact every frame |

Check switch `gv1check` (Azahar only, costs time): for every GPU triangle it also runs the old transform, reject and cull. Counters:
- `gvtriout`: triangles the old rule keeps among the GPU triangles. gvtriout + CPU-path triout must equal base triout per frame (0 frames differ against a base run with the same switches and fixedseed).
- `gvcull`: GPU cull mode disagrees with the CPU cull (1.6). Must be 0.
- `gvstale`: the stored epoch decision differs from a full prep (1.4). Must be 0.
- `gvxdraw`: draws with no triangle that the old rule keeps (draw risk of 1.7).
GPU-side counters: the PICA has no primitive or fragment counter that citro3d reads. The GPU-side equivalent is the submit count (gvtri, draw, gvpal) plus the picture compare. On HW, the GPU time shows only in fb and the on-time histogram (HWM1).
statehash: must be equal (render state does not reach game logic). Exception to watch: F03 depth query (pm_depth_query, pm_gbi.c:23) reads the GPU depth. If statehash differs, list the F03 queries of the window.

### 3.3 Risk list

| # | Risk | Effect | Test in Azahar | Mitigation |
|---|---|---|---|---|
| G1 | Depth precision. The CPU path rounds clip x y z w to float24 at the attribute load and then multiplies by the simple post matrix. GV1 multiplies s16 object coordinates (exact in float24) by a float24 copy of post x game. Error per term about 2^-17 of the largest term: about 0.03 world units for coordinates of 2000 (E). At d 500 that is about 1e-6 in z', under the float24 z' step 7.6e-6 near 1 (V18_V19_azahar.md) | Z fighting where two near-coplanar surfaces come from different matrices | depthview=3 shots of yard and toad, gv1 against base. golden 10 scenes | Decal2 stays on the CPU until GV1d. If a scene fails: compose post x game in fp32 per vertex is not possible, so keep that model on the CPU by a per-matrix fallback |
| G2 | Decal bias (pm_gpu.c:236 to 244, 0.001 default). Same bias on both paths. A float24 error of 1e-6 is 1000 times under the bias | Small | battle (shadow decals), V19 shots | none needed |
| G3 | V16b decal stencil two-pass | Missing overlay if the GPU batch skips pass 1 | battle Thunder Rage scene (V16b_azahar.md) after GV1d | CPU until GV1d. GV1d draws the same indices twice with the same stencil state |
| G4 | V18 and V19 flicker | New flicker if the rounding differs between frames of a pan | V18 shotwin=3490:40, V19 shotwin=225:30, gv1 against base, all frames | Same as G1 |
| G5 | Z buffer behaviour (ZB, B3 depth image role, X01 prim z, the darkness X02 and F03 readback, PO3.md R7) | Depth images wrong | intro (darkness), pause1, battle start swirl (X01) | ROLE_DEPTH and zmark stay on the CPU until GV1d. zmix covers prim z |
| G6 | Wide mode (F07). The 1.25 x scale is in the post matrix. UI quads move edge vertices on the CPU | UI bars do not reach the edge | title and fileselect with the wide option on | wideui fallback (1.5) |
| G7 | Cull winding per target | Missing walls or pause pages | yard (V02), pause1 to 4, gvcull 0 | gv1check |
| G8 | Near plane | Different picture near the camera | kmr_20 door, V27 Sushie dive, gvfb near count | Near loads take the CPU clip (exact). A later test can let the PICA clip them (open question Q1) |
| G9 | Azahar against the PICA (PO3.md R4): shader float rules, the clipper, mova | Golden passes in Azahar, HW differs | none | HW shots by the user (Q4) |
| G10 | Draw count rises (box keeps runs that the old test rejected) | +63 us per draw on HW | gvxdraw, draw counter of the intro dip window | n < 16 uses the vertex bits. If gvxdraw is over 10 per step: a per-run deferred draw (a run with no triangle inside the screen box is dropped). Plan that only if measured |
| G11 | Vtx written by the game after G_VTX (H06a cause 1) | none: the ring copy is the snapshot, and the CPU fallback reads the ring copy | yard, toad (draw_box, imgfx, msg_draw users) | Never key a buffer by the Vtx address |
| G12 | Slot rules (vert_live, tri_since_vtx, FX3f, H06a cause 2) | Wrong triangles | aura effect (FX3f scene), golden | Keep the slot tables as now. The ring index per slot is per load |
| G13 | Matrix at the load (H06a cause 4) | Wrong place after a G_MTX between G_VTX and the triangle | golden | The palette entry is the gm_tab index of the load, never the current matrix |

## 4. Slices

Rule for all slices: switch `gv1` (default off) turns on the new path, `nogv1` turns it off. gpuvtx stays as it is until GV1f removes it. Each slice: one sonnet code agent, the gate below, at most 8 Azahar starts. Azahar metric: gb ms per world step and the instruction proxy o3ds_est_ms (hw_rerank.md 5), from a fixed window. Use the intro dip window (pc 1650 to 1850, `fast preset=intro` or the full cycle phasecsv range) and the yard preset.
HW metric (HWM1 lines): `phs <mode> <map> st <k> ...` with the abtog state gv1 against base: gb, gp, gd, draw, the new gv counters, cpu p50 and p95, ok and late counts.

| Slice | Content | Size (changed lines) | Gate | Azahar test | Azahar expected | HW expected (O3DS ms per world step) |
|---|---|---|---|---|---|---|
| GV0 | GV0a and GV0b flushes (section 0.1). Goes to the live G1 agent with KICK1 | 15 | golden title, pause1, battle 0 differences | no new run (bundle with the KICK1 runs) | 0 | 0 (fault fix, needed for any HW gpuvtx or gv1 number) |
| GV1a | G_VTX for unlit loads: ring copy, box or vertex bits (1.3), no vert_color, vert_st, vert_p per vertex. Lazy CPU transform from the ring copy for the fallback triangles. GPU triangle: no CPU cull, no n_keep, no sample. C3D_CullFace per batch (1.6). Counters gvload, gvtri, gvrej, gvfb, gvxf. Switch gv1check with gvtriout, gvcull | 150 to 200 | gv1check: gvtriout balance 0 frames differ, gvcull 0. golden --extra gv1 yard toad battle within TOL. statehash equal. Work cut: Azahar gb in the dip window lower by 1.0 ms or more, `st_vx` (PO2 line) under 10 percent of vtxload with gv1 | 1 run dip window with gv1check, 1 run without, 1 base run, golden 3 scenes | gb -1.0 to -1.6 ms (G_VTX 2.3 ms in the dip, intro_dip.md 3) | 2 to 4 |
| GV1b | State epoch (1.4): the full prep only after a state change, one stored decision, pm_gpu_gtri append by key (no memcmp). Counter gvstale in gv1check | 100 to 150 | gvstale 0, the GV1a gates hold, Azahar gb lower by 0.8 ms or more than GV1a | 2 runs plus golden pause1 battle title | gb -0.8 to -1.5 ms | 2.5 to 4.5 |
| GV1c | Matrix palette (1.7): pm4.v.pica with mova, flag field index, upload of changed entries, gvpal counters, gvrecopy | 120 to 160 plus 25 shader | draws per step with gv1 equal to or lower than base in yard, toad and the dip window. gvxdraw reported. Golden 10 scenes within TOL (the one full golden of GV1) | 2 runs plus golden 10 | gd back to base or lower (dipb: +0.46 ms now) | 1.5 to 3 (the draws of the partial path, 20 to 40 draws x 63 us) |
| HW push 4 | User run (section 5). Stop point | 0 | gb lower by 6 ms or more per world step | none | | sum GV1a to GV1c: 6 to 11.5 |
| GV1d | decal2 two-pass and zmark pass 2 on DrawElements, tex1 when not a slot | 60 to 90 | battle, pause1, intro golden within TOL, gvfb decal2 zmark 0 | 2 runs plus golden 3 | gb -0.1 to -0.3 | 0.3 to 1 |
| GV1e | Optional: fog in the shader. Only if a counter shows gvfb fog over 2 percent of the triangles of any scene, else close the slice | 40 plus 10 shader | the F08 scene shots within TOL | 1 run | 0 to 0.2 | 0 to 0.7 |
| GV1f | gv1 default on, nogv1 kept, gpuvtx code removed, VERT_SLOTS by the measured verts_max, 3dsx size and mem line | 40 to 80 | full golden once, smoke yard toad, statehash equal | full golden | | 0 (memory -960 KB) |

Total expected: 8 to 14 ms per world step on the O3DS (hw_rerank.md rank 1), 6 to 11.5 of it from GV1a to GV1c.
Why GV1a is a real cut and not a moved draw: it removes the per-vertex transform (16 multiply-adds), vert_color, vert_st (2 multiplies, a divide by 65536 each) and clip_bits4 for about 1681 vertices per frame, and the 8-multiply CPU cull and the n_keep loop for every GPU triangle. Azahar counts these instructions (hw_rerank.md 0), so gb must fall in Azahar. The added work is a 2 B store and 6 integer compares per vertex.

Stop rules:
- A slice whose Azahar gb gain is under half of its Azahar expected value: stop. Do not start the next slice. Report the gbiprof sections of the slice (switch gbiprof) and ask for a plan update.
- A slice that raises draws per step by more than 5 against base: stop before HW. Fix the batching first (G10).
- HW push 4 shows gb lower by less than 4 ms per world step: stop GV1d and later. Do the unit-cost fit of hw_rerank.md 5 step 2 with the push 4 phs lines before more code. The earlier lesson: a partial path can be slower on HW than the CPU path. So no slice goes default on before push 4.
- Budget: 8 Azahar starts per slice. GV1c has the one full golden of GV1.

## 5. Hardware measurement plan (pulled log only)

After GV1c and after HWM1, DR1, KICK1, GV0 are on main. One push (push 4). It needs the user (O3DS on, no button press during the run).
```
export O=<ip> B=<build>; sh 3ds/push_3ds.sh --no-build --ip $O -t 420 audiostat pacestat logstat phase phsum noaudit loglevel=debug fixedseed statehash=10 phasecsv=1650:1850 abtog=base,gv1,gpuvtx:30
python3 -u 3ds/tools/ftppull.py $O o build3ds; cp build3ds/hw_o3ds_log.txt 3ds/logs/perf/hw_${B}_GV1_pull.txt
python3 3ds/tools/phs_report.py 3ds/logs/perf/hw_${B}_GV1_pull.txt > 3ds/logs/perf/hw_${B}_GV1_table.txt
```
Read: per scene, state gv1 against base: gb, gp + gd per draw, draw, gvtri, gvfb by reason, gvpal, cpu p50 and p95, on-time share at cpu 24 to 35 ms. The `gv1` line (every 300 drawn steps): `gv1 f<frame> load <n> tri <n> rej <n> fb <n> (lit near scr zmark decal2 tex1off wideui fog dbg ring) xf <n> pal <n> palmax <n> recopy <n>`. The `mem` line: command buffer percent, gvloadmax.
phasecsv=1650:1850 keeps the pc lines of the intro dip window only. gv1check is not in the HW run (it costs time).
Pass: gb lower by 8 ms or more per world step (hw_rerank.md row GV1), draws per step equal to or lower than base, 0 `GPU LIMIT NEAR` lines.

## 6. Open questions for the user (defaults)

- Q1: Near plane. Default: loads near the camera take the CPU clip (exact, slower). Option: let the PICA clip them after an HW shot test of kmr_20 and V27.
- Q2: Tolerance. Default: the golden TOL 8 and PCT 0.5 stay for gv1. No looser value.
- Q3: AGENT_RULES.md line 41 says that optimization code starts only after S28. The G1 agent codes TX1 and DR1 now. Default: GV1 code follows the same exception and starts after the G1 rows commit. Confirm.
- Q4: HW pictures. ftppull.py pulls the log and the crash dumps, not shots. Default: the user looks at yard, toad, battle and a pause page on the O3DS in push 4 and answers yes or no. Option: add the shots folder to ftppull.py (about 10 lines).
- Q5: Lighting and fog on the GPU. Default: no (0.1 and 0 percent). They stay on the CPU path.

## 7. Dependencies on the G1 rows (code read from HEAD)

| G1 row | GV1 needs | Conflict |
|---|---|---|
| HWM1 | phs lines, abtog with a state `gv1` (the allowed list in hw_rerank.md row HWM1 is base, gpuvtx, nodr1, nokick, notx1, phasegbi: it must also accept gv1, or any switch name), phasecsv=<from>:<to>, phs_report.py columns for the new C_ counters | pm_phase.c, pm_port.h:61 C_ enum. GV1a adds C_ ids after HWM1 commits |
| DR1 item 1 | one BufInfo per frame: the ring draw at pm_gpu.c:553 to 556 uses it. Indices are absolute, so no per-draw BufInfo | pm_gpu.c flush_impl 320 to 327 and 551 to 574: GV1 edits the same lines. GV1a starts after DR1 commits |
| DR1 item 2 | uniform shadow compare: the palette upload uses it | pm_gpu.c 319 to 340 |
| DR1 item 3 | none (texture rectangles and fills) | none |
| KICK1 | the flush of the range written since the last split must cover gring and gidx (GV0a, GV0b) | pm_gpu.c:1125 to 1148, split_frame 185 to 192 |
| TX1 | none. GV1 calls pm_tex_size and pm_tex_bind only (same API) | pm_tex.c: none |
Order: HWM1, TX1, DR1, KICK1 (with GV0) commit first. GV1a starts on the next main. All GV1 code edits pm_gbi.c, pm_gpu.c, pm_gpu.h, a new shader and CMakeLists.txt (shader list), so the GV1 rows run in sequence with the G1 rows, not in parallel.

## 8. Result notes (GV1 agent, 2026-10-21, Azahar intro dip window pc 1702 to 1850, noaudit, ms per step)

- GV1a (box reject for loads of 16 or more vertices, full prep per triangle): gb 12.7 (base) to 13.6, triout 290 to 568, draws 150.6 to 156.6. Cause: the box test and the GPU cull let 278 more triangles reach the full prep (ensure_tile, dual_bind, sync_render, stats, gpu_emit).
- GV1b (state epoch, 3 index stores per triangle): gb 11.5, gd 2.0 to 2.7. Pictures equal, gvstale 0.
- GV1c (palette): palmax is 1 in the dip window, so the palette removes no draw there. The box reject made gb worse than the per-vertex reject bits (12.4 against 11.9 with the palette). Default is now per-vertex bits (switch gv1box turns the box on). A slow path per triangle with first-use vertices (gappend refused them) cost 0.9 ms. gappend now assigns the palette index itself. Result: gb 11.2, gp 1.2, gd 2.6, triout 389, draws 152.6 (+2).
- Experiment gv1raw (no reject bits, only a near test per load): gb 14.6, triout 1075, draws 192.6 (+42). A reject that costs a few instructions per vertex saves a full state prep and a draw for every run that is off screen. The floor without reject is higher than base.
- Weighted by the hardware ratios (gb 3.5, gp 3.7, gd 1.9): base 52.7, GV1b 50.2, GV1c final 48.6 (about -4 ms in the dip window).
- Golden yard and toad with gv1 showed a wrong picture: the first cull sign was inverted (the dip window has no culled surfaces, so it could not show it). The sign is flipped now. Not yet verified.

## 9. GV1d stop report (GV1-FULL agent, 2026-10-21, same window, Azahar, noaudit, ms per step)
- Measured now: base gb 13.26, gp 1.21, gd 2.01, triout 290, draws 150.6. gv1 gb 11.25, gp 1.25, gd 2.60, triout 389, draws 152.6. Difference gb -2.0 ms. (The base value moves between runs: 12.7 and 13.26.)
- The GV1d changes are in the code already: absolute ring indices, no flush at a vertex reload, matrix change without flush (palette), state prep once per epoch. gbiprof flush table of the window (calls per step): gtrip flush 1.34 and all empty; no flush from a CPU triangle; the draws come from G_SETTILE of tile 0 (147.9 calls, 105 drawn), set_plan 69.9 (11 drawn), set_replace 8, set_combine 5.7, setscissor 4.7, G_SETOTHERMODE_H 6.3. A draw needs one texture or one combiner state: DrawElements cannot merge them. The window has 660 triangles in about 130 drawn batches (5 triangles per draw).
- The +2 draws of gv1 against base are runs where every triangle is culled by the GPU: the base path culls on the CPU and drops the run. The fallback counters in the window: tex1 1.7 per step, refuse 0.7 per step, near/scr/lit/fog 0.
- CPU per triangle (gbiprof laps, timer reads inflate them by about 2.3 times): early 0.9 ms (reject check), emit 0.5 ms, gvtx 0.9 ms per step for 660 triangles and 1150 vertices: about 1.4 us, 0.8 us per triangle and 0.8 us per vertex. The geometry part is about 9 percent of gbi in this window. The op table puts 5.8 ms of 26 (inflated) in G_SETTILE and G_SETTILESIZE: the flush and draw inside them.
- Result: a further batching step cannot go below base draws, so the stop rule of GV1d applies (draws not below base). Remaining possible gain of GV1e..g in this window is below 1 ms. Per-draw cost (gp 8 us per draw, gd) is the lever, not per-triangle cost.

## 10. FL1 (texture slab, one flush per command list submit)
- pm_tex.c: all texture images live in one linear slab (4.75 MB, SLAB_BYTES). A free list (best fit, merge on free) replaces linearAlloc and C3D_TexDelete per texture. A texture header is copied from one citro3d prototype per format. A miss records its span. pm_tex_flush_pending (called from gpu_flush_written and before the grab split) runs one GSPGPU_FlushDataCache per merged span (gap 64 KB). The per-texture C3D_TexFlush is gone. Switch nofl1 gives the old path. Switch fl1log logs the first 400 uploads with a byte hash.
- New counters in the pcx line and phc: flushcalls and flushkb (all GSPGPU_FlushDataCache calls of the frame: vertices, ring, index, textures).
- Azahar, toad warp preset, 360 steps: flush calls 1145 (nofl1) to 717, largest step 120 to 4. tx phase sum 86.6 ms to 59.6 ms (Azahar shows only the CPU part).
- Pictures: golden yard 0.00, toad 0.00, battle 2.56 (same as before). pause1 0.40 with fl1 against 0.06 with nofl1. Cause found with fl1log: the texture 512x8 from a source of 320x6 (img 0x328deee0) has different bytes in the two runs while the upload order is equal. The source memory differs with the heap layout. tx1check shows slab bytes equal to the old path. No FL1 fault.
