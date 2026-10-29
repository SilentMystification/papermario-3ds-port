# CPU vs GPU work in the display-list path (yard)

Sources: console_numbers.md (WORLD, 2179 frames), console_walk_split.md (2718 frames), diag_frame_budget.md (O3DS 1227, steady frames). Counts per frame from build3ds/hw_pull/hw_o3ds_log.txt are averages over 549 logged frames with work, not yard only [M]. [M] measured, [I] inferred, [U] unmeasured.

| # | Item | Where (file:line) | Runs per | Count per yard frame | CPU ms per yard frame | Where it runs |
|---|---|---|---|---|---|---|
| 1 | MVP transform (GV path) | pm5.v.pica:41-48 | vertex | VTXLOAD [U] | 0 | GPU |
| 2 | Lighting, fog, texgen, uv scale | pm5.v.pica:77-137 | vertex | [U] | 0 | GPU |
| 3 | Clip reject bits, back-face cull (GV path) | pm_gbi.c:1442-1470, pm_gpu.c:402 (C3D_CullFace) | triangle | GVREJ [U] | 0 | GPU cull, CPU reject bits |
| 4 | Vertex copy to ring | pm_gpu.c:2020-2026, pm_gbi.c:2624 | vertex load | VTXLOAD [U] | [U] | CPU memcpy 16 B/vertex |
| 5 | Lit/texgen colour on CPU (fallback) | pm_gbi.c:666-703, 1327-1343 | vertex (fallback) | [U] | [U] | CPU |
| 6 | Vertex transform on CPU (GP1 cross, old path) | pm_gbi.c:2663-2668, 2671-2690 | vertex | [U] | [U] | CPU, 16 mul-add + clip bits |
| 7 | UV scale on CPU (fallback) | pm_gbi.c:711-724 | vertex (fallback) | [U] | [U] | CPU, 2 divides per vertex |
| 8 | Matrix load: unpack, mul, upload | pm_gbi.c:1990-2026, 376-398 | G_MTX | [U] | [I] 5-10 us per G_MTX | CPU, 2 mul + 16 divides |
| 9 | gv_matrix_id memcmp scan | pm_gbi.c:1367-1397 | G_VTX load | [U] | [U] | CPU, up to 256 x 64 B memcmp |
| 10 | Palette build (pcomp) | pm_gpu.c:802, 827 | palette change | PMUL 1.05 [M] | [I] under 0.1 | CPU |
| 11 | Palette index scan per vertex | pm_gpu.c:1814 | vertex per triangle | [U] | [U] | CPU linear scan of pal_n |
| 12 | Triangle decision and batch | pm_gbi.c:1442-1530, pm_gpu.c:1808 | triangle | draw calls 39.2 [M], raw 4.6 [M] | [U] | CPU decide, GPU raster |
| 13 | Vertex buffer write (CPU path) | pm_gpu.c:1083-1108 (40 B per vertex) | vertex | [U] | [U] | CPU |
| 14 | Cache flush | pm_gpu.c:261-266 | flush | [U] | [U] | GSP call |
| 15 | Texture hit lookup | pm_tex.c:925-980 (palette hash per load) | texture op | hits 86 [M] | 1.4 [M] (16 us/hit) | CPU |
| 16 | Texture miss: expand, tile, upload | pm_tex.c:315-371, 1128-1135; pm_tex_tx1.h:54, 83, 130 | miss | misses 1.6 [M] | 1.6 [M] (1.0 ms/miss) | CPU |
| 17 | Texture source hash on miss | pm_tex.c:767-771, 1227 | miss | with miss | [U] | CPU FNV per byte |
| 18 | Flush closes at tex ops | pm_gbi.c:50-60, pm_gpu.c:2099 | tex op | closes 377, empty 207 [M] | 8.4 [M] (7.2 O3DS) | CPU, not GPU work |
| 19 | Combiner decide, self-check | pm_gbi.c:959-1017 (24 samples per new key) | new combine key | [U] | [U] | CPU, cached 128 keys |
| 20 | Combiner sync and signature | pm_gbi.c:1019-1066 | sync | sync resolve | 2.99 [M] (1.3 O3DS), overlaps | CPU |
| 21 | TEV rebuild | pm_gpu.c:474-630, 882-884 | TEV key change | TEVR 6.8 [M] | [U] | CPU sets C3D_TexEnv |
| 22 | DLC/PDL state hash per G_DL | pm_gbi.c:2542-2550, pm_dlc.inc.c:440, 934 | G_DL class 2 or 3 | replay 5.7 [M], ops 73.9 [M] | [U] | CPU |
| 23 | Walk residual (VTX, MTX, geometry, dispatch) | pm_gbi.c:2531-3100 | op | ops 4545 [M] | 13.8 [M arithmetic] (18.5 O3DS [I]) | CPU, no counter |

Triangle streak (gv) 2.55 ms yard [M]. Log+HUD 1.4 ms [M]. Vertex and matrix counts per frame are [U]: C_VTXLOAD, C_GVTRI, C_TRI exist (3ds/include/pm_port.h) but the hardware log does not print them.

## Answers
1. Transform: GPU when the GV path runs. gpu_vtx_on = ring allocated (pm_gpu.c:1170). No switch check at pm_gbi.c:2619. The comment at pm_gbi.c:292 says gv1 is default off. That comment looks stale [I]. CPU applies MVP only in fallback (items 4-7).
2. Lighting, fog, texgen, uv scale, clip: GPU in pm5 (items 1-2). CPU only when a triangle falls back (GVR_LIT, GVR_FOG, GVR_TEX1, GVR_ZMARK, GVR_DBG, gv_state_why pm_gbi.c:1400-1410).
3. Triangle setup: GPU raster. CPU does reject bits (item 3) and the cull cross product only on the CPU path (pm_gbi.c:1800-1806).
4. Texture decode: CPU (items 15-17). CI and RGBA16 expand on CPU (pm_tex.c:315). tile swizzle is CPU (tiled16, pm_tex.c:303). Per miss, not per frame.
5. TEV: sync_combine skips by signature (pm_gbi.c:1045-1066), memo at 1019-1044, decide per new key (959), TEV rebuild per key change (pm_gpu.c:474). Per close the TevKey memcmp runs (pm_gpu.c:882).
6. Vertex buffer: GV path copies 16 B per vertex plus a box scan of 3 compares per vertex (pm_gbi.c:2632-2650). CPU path writes 40 B per vertex (pm_gpu.c:1083). Bytes per frame [U].
7. Matrix stack: load_mtx and upload_mvp multiply twice per G_MTX (pm_gbi.c:1990-2026). gv_matrix_id scans newest first (pm_gbi.c:1379).

## Top 5 movable to GPU, by CPU ms
0. (not GPU) Flush closes at tex ops, 8.4 ms yard. 207 of 281 closes are empty [M]. Fix: skip empty closes on CPU. Ranked first by ms only.
1. Texture miss convert and tile, 1.6 ms yard. PICA200: GX texture copy or display transfer (format and tile in one pass) for non-CI formats. CI stays on CPU. Native path exists (pm_tex.c:1131). Risk: transfer sync cost, format gaps.
2. Vertex fallback, items 5-7, [U] ms. PICA200: extend pm5 to cover TEXEL1 fold and ZMARK. Risk: medium. The per-draw fold (pm_gbi.c:1194, t1_fold) is not per vertex.
3. Matrix combine per G_MTX, item 8, [I] 5-10 us each. PICA200: separate MV and projection uniforms, dp4 chain in pm5. Risk: uniform bank. pm4.v.pica already uses pal[84] of 96 vec4 constants.
4. Combiner decide and TEV rebuild, items 19-21, sync 2.99 ms yard (overlaps). PICA200: TEV stages already used. Precompute TEV per combine key at display-list compile (PDL path). Risk: low.
5. Palette index scan, item 11, [U] ms. PICA200: store palette index per vertex at load (already done for ring: pm_gpu.c:1826). Risk: low.

## Next step
Add per-class op timing (console_walk_split.md design, sampled every 64 ops) to split the 13.8 ms (yard) and 18.5 ms (O3DS) residual. No change to the GPU path until that split exists.
