# DLC plan: display-list compile and cache

Written 2026-10-21 by the DLC planner (read only, no source edit, no Azahar start, no console contact).
Inputs: docs/plans/hw_p4_findings.md (build 970, O3DS), hw_p3_findings.md (build 926 phasegbi split), arch_review.md 2a to 2c and 3.3, GV1.md 8 to 11, GV1_handoff.md, bench_notes.md B4 and B6, route_notes.md, MEMORY_BUDGET.md. Code read at HEAD 17433cbd0: 3ds/src/pm_gbi.c, 3ds/src/pm_gpu.c, 3ds/src/pm_tex.c, src/model.c.
Data source letters: M = measured on the console, A = measured in Azahar, S = counted offline from the ROM assets (this plan), E = estimate from code and numbers, G = guess.

## 0. Summary

- Static share per Toad Town frame (S + E): 85 to 92 percent of the 1440 triangles and 2400 vertex loads come from the map shape lists (pm_shape_buf). 70 to 80 percent of the 7000 gbi ops come from static lists (shape lists, texture lists in TextureHeap, ModelRenderModes lists). The `dlsrc` counter (bench_notes.md B6) does not exist yet. Slice DLC0 adds it (section 1.4).
- The cost is in the ops, not only in the triangles: 7000 ops per drawn frame, of which only about 1100 are G_VTX and triangle ops. DLC must compile the state lists too (DLC-S), not only the geometry lists (DLC-G).
- Replay design: at a G_DL whose target is a compiled static list, test one box per vertex chunk, copy the index ranges of the chunks in view to the frame index ring (indices into a static vertex area that is filled once per map), apply the compiled exit state of the list, and merge ranges with equal state block into one draw.
- Expected gain in town (E): gbi 39 ms to 14 to 25 ms (DLC-G 6 to 10 ms, DLC-S 8 to 15 ms more), plus gp+gd 8.3 ms to about 3 to 4 ms with DLC-P. CPU part 63.6 to 66.9 ms to about 35 to 46 ms: 20 drawn frames per second in town. 30 per second only at the best end.
- First slice (DLC1): compile the map shape geometry lists only (ops G_VTX, G_TRI1, G_TRI2, G_GEOMETRYMODE, syncs, G_ENDDL), box test per chunk, feed the ranges to the existing GV1 ring. Switch `dlc`, default off. Gate: gbiops in the toad preset falls by 900 or more per frame, golden toad and yard 0.00.

## 1. What is static in a typical frame

### 1.1 How the frame list is built

- The game builds one main list per step into gDisplayContext->mainGfx (gMainGfxPos, 3ds/src/game_stubs.c:11 to 12). pm_nusys.c:86 and :89 give it to pm_gbi_run (3ds/src/pm_gbi.c:2050). The main list is per frame by design. DLC never caches it.
- Map models: appendGfx_model (src/model.c:1369) writes per model and per frame a preamble into the main list: tint colours (1445 to 1456), gSPDisplayList of the texture list `textureHandle->gfx` (1510, 1514), gSPDisplayList of `ModelRenderModes[i]` (1623 to 1924), fog colour and fog position (1752, 1753, 1820, 1896), gSPMatrix of `model->finalMtx` (1930, 1941, only without MODEL_FLAG_IGNORE_MATRIX), tex panner gDPSetTileSize (1966 to 1974), billboard matrix (1981), gSPDisplayList of the geometry list `modelNode->displayData->displayList` (1992), gSPPopMatrix (2004), gDPPipeSync (2007).
- The texture list is built once per texture handle at map load into TextureHeap (src/model.c:2079 to 2084, make_texture_gfx at 3163). The images come in by dma_copy (2059, 2071).
- ModelRenderModes (src/model.c:175) points to lists in .data. No game code writes them (to be checked by `dlccheck`).
- A model with an identity blueprint matrix gets MODEL_FLAG_IGNORE_MATRIX (src/model.c:2596): no gSPMatrix, the geometry uses the camera matrix. Most static map models are of this kind, so most map draws share one matrix.
- render_models (src/model.c:2818) culls only small models (MODEL_FLAG_DO_BOUNDS_CULLING, all sizes under 100, src/model.c:2634) and keeps a model when one box corner is in view. Large models (ground, walls) are always drawn. This is why 68 to 83 percent of the triangles read are rejected later.
- Dynamic Vtx (H06a Cause 1, 16 files): all writers write heap copies (mdl_get_copied_vertices, src/model.c:4189: rowf_and_rhuff.inc.c:57, pra_29 bridge.c:33, hos_03 model_fx.c:20), imgfx and sprite buffers, msg, HUD and draw_box buffers. HiddenPanel (src/entity/HiddenPanel.c:34) writes an entity list and its Vtx in the entity heap. tst_13 writes N(shockwave_gfx) in .data. No writer targets pm_shape_buf (E, `dlccheck` confirms).
- Models with MODEL_FLAG_HAS_LOCAL_VERTEX_COPY do not call the shape list (src/model.c:1991). They draw the heap copy through custom gfx (1952, 1999). These are dynamic by class.

### 1.2 Offline count of the map shape lists (S)

Script: walk the model tree of the shape file (ShapeFileHeader, include/model.h:173), follow each model display list, simulate the port slot rule (vert_live, tri_since_vtx, pm_gbi.c:2133). Files: assets/us/mapfs/geom/*_shape.bin (uncompressed, base 0x80210000).

| Map | models | ops in geometry lists | triangles | vertex loads (sum of n) | G_VTX ops | state ops in the lists | cross-list slot reads | lit lists | G_MTX in the lists |
|---|---|---|---|---|---|---|---|---|---|
| mac_00 | 174 | 2019 | 2040 | 3348 | 283 (n avg 11.8, max 32) | 356, all G_GEOMETRYMODE | 0 | 0 | 0 |
| mac_01 | 148 | 2024 | 2210 | 3754 | 308 (n avg 12.2, max 32) | 297, all G_GEOMETRYMODE | 0 | 1 | 0 |
| kmr_02 | 137 | 1895 | 2121 | 3409 | | 280 | 0 | 0 | 0 |

- Triangles per model: mac_00 p50 8, p90 26, max 84. mac_01 p50 6, p90 33, max 336. Models of 16 or more triangles hold 55 percent (mac_00) and 76 percent (mac_01) of the triangles.
- No geometry list has a G_DL out of the shape buffer, a G_MODIFYVTX, a G_MTX or a state op other than G_GEOMETRYMODE. No triangle reads a slot that its own list did not load. So the compile of map geometry lists is simple and exact.

### 1.3 Share per Toad Town frame (console numbers M, split E)

Console 970, per drawn frame (phc lines of 3ds/logs/perf/hw_pull4/hw_o3ds_log.txt, line 593: 1:1 window, n 300): tri 1440, triout 242, vtxload 2397, gbiops 6994, draw 78. hw_p4 section 2: tri 1381 to 1422, vtxload 2248 to 2362, ops 6650 to 7290, draws 92 to 105.
(The task text gives gbiops 1350 per frame. The console phc field `gbiops` gives 6650 to 7290. This plan uses the console value.)

- Triangles: the map has 2040 (mac_00) and 2210 (mac_01) triangles. The frame reads 1381 to 1440. The vertex to triangle ratio of the frame (2397 / 1440 = 1.66) equals the ratio of the map lists (1.64 and 1.70). Sprites are quads (ratio 2.0) and text and HUD use texture rectangles. Estimate: dynamic triangles (NPC and Mario sprites, shadows, HUD quads, effects) 100 to 220 per frame. Static share of triangles read: 85 to 92 percent (E). Static share of vertex loads: 85 to 92 percent (E).
- Ops: per drawn map model about 6 to 10 main-list ops (dynamic), 16 to 30 texture-list ops (static, TextureHeap), 5 to 10 render-mode ops (static, .data), 11.6 geometry ops (static, shape). With 100 to 130 drawn models: 4500 to 6500 model ops, of which 80 to 85 percent are in static lists. Sprites, entities, HUD and effects add 1000 to 1500 dynamic ops. Static share of ops: 70 to 80 percent (E).
- Geometry ops are about 1100 of the 7000 ops (G_VTX about 200, G_TRI2 about 650, G_GEOMETRYMODE about 240). The other 5900 ops are state and flow ops. The console split of build 926 (hw_p3 section 2) puts gt+gv at 13.2 ms of 35 ms gb and "gb self" at about 16 to 24 ms. In build 970 the state ops and the per-run state prep are the larger part.

Decision of arch_review 3.3 (static triangle share 70 percent or more): pass by estimate. DLC0 measures it before DLC1 is merged as default.

### 1.4 The `dlsrc` counter (slice DLC0)

Switch `dlsrc` (zero cost without it: one load and one branch per site). Region classes by address (range compares in this order):

| id | class | range | static? |
|---|---|---|---|
| 0 | main | gDisplayContext buffers (D_80164000, game_stubs.c:11), sizeof(DisplayContext) x 2 | no |
| 1 | shape | pm_shape_buf (gMapShapeData), 3ds/src/pm_map.c:35, 0x40000 | yes |
| 2 | texheap | TextureHeap (src/texture_memory.c:6) | yes |
| 3 | rodata | .rodata and the ModelRenderModes lists (linker symbols of .rodata, and the range of the lists that ModelRenderModes points to) | yes |
| 4 | entity | pm_world_entity_heap (3ds/src/pm_buffers.c:8, 0x18000) | mixed |
| 5 | genheap | heap_generalHead (src/heaps3.c:6) | mixed |
| 6 | sprheap | heap_spriteHead (src/heaps3.c:7) | no |
| 7 | other | the rest of .data, .bss, newlib heap, linear | no |

Sites (pm_gbi.c line numbers at HEAD):
- Class of pc: compute `dls_pc` at list entry (2054), after a G_DL jump (2108 to 2112) and after a return (2098). Not per op.
- Ops: at 2077 (`n_ops++`): `dls_ops[dls_pc][kind]++`. kind 0 = geometry (G_VTX, G_TRI1, G_TRI2, G_MTX, G_POPMTX, G_GEOMETRYMODE, G_MODIFYVTX), 1 = flow (G_DL, G_ENDDL), 2 = state (the rest).
- Triangles: at 1371 (`n_cmd++` in draw_tri): `dls_tri[dls_pc]++` and `dls_triv[vert_reg[i0]]++` (class of the Vtx source of the first corner).
- Vertices: at 2129 (`PH_ADD(C_VTXLOAD, n)`): `dls_vtx[class(src)] += n`, and `vert_reg[v0 + i] = class(src)` in the loop at 2206.
- `dl`: distinct G_DL targets per frame (open-address set of 512, reset in pm_gbi_end_frame, 1686).
- `stable`: for every G_DL target in a static class, FNV-1a over the list words up to G_ENDDL and over the Vtx bytes of its G_VTX ops, kept in a 1024-entry table by address. Stable = equal to the value of 30 drawn frames before.
- Line every 300 drawn steps with the phs scene tag: `dlsrc <mode> <area:map> ops m s t r e g p o (geo/flow/state each) tri pc m s t r e g p o tri vs m s t r e g p o vtx m s t r e g p o dl <n> stable <pct>`.
- Size: about 150 lines in pm_gbi.c. Test: 1 Azahar start (toad and yard presets in one tour, 600 steps). Console: add `dlsrc` to the next HW push.

## 2. Compiled form and replay

### 2.1 Entry kinds and keys

- Key = target address of the G_DL plus the generation of every region it reads (one u16 per region class, section 3.1). The content hash (FNV-1a 32 of the list words and of the Vtx bytes) is stored at compile. It is checked only in `dlccheck` mode and at the first reuse after a generation change of a "mixed" region (entity, genheap).
- Lookup: open-address table of 2048 entries by address (multiplicative hash, 2 probes average). Cost about 0.3 to 0.5 us (E).
- Kind G (geometry list): only G_VTX, G_TRI1, G_TRI2, G_GEOMETRYMODE, G_RDPPIPESYNC and the other syncs, G_NOOP, G_ENDDL, and G_DL into lists of the same kind (flattened). All map shape lists are kind G (1.2).
- Kind S (state list): only state ops (G_SETTIMG, G_SETTILE, G_LOADBLOCK, G_LOADTILE, G_LOADTLUT, G_SETTILESIZE, G_SETCOMBINE, G_SETOTHERMODE_H and _L, G_GEOMETRYMODE, G_TEXTURE, G_SETPRIMCOLOR, G_SETENVCOLOR, G_SETFOGCOLOR, G_SETBLENDCOLOR, G_MOVEWORD of fog, syncs). Texture lists and render-mode lists are kind S.
- Kind M (mixed: state ops between geometry, G_MTX and G_POPMTX inside): entity and animator lists. Slice DLC4.
- Refused (interpreted every time, counted by reason): G_MODIFYVTX, G_TEXRECT, G_TEXRECTFLIP, G_FILLRECT, G_SETCIMG, G_SETZIMG, G_MOVEMEM (lights, viewport), G_MOVEWORD of a segment, a G_DL into a non-static class, a triangle that reads a slot its own list did not load, a list longer than 4096 ops, a target in class main, sprheap or other.

### 2.2 Kind G: what is compiled

- Vertex slots: the compiler runs the exact slot code of G_VTX (pm_gbi.c:2117 to 2134: n, v0 from w0, the S30 clamp at 64, `vert_live` clear at a slot 0 load after a triangle) and of draw_tri (pm_gbi.c:1362 to 1380: index / 2, the 0 to 63 range check, the live check that drops a triangle). Each slot holds the Vtx address it was loaded from. Each triangle becomes 3 absolute vertex indices. A dropped triangle is dropped at compile (same counter n_drop at replay). The compiled indices are therefore the exact vertices the interpreter reads.
- Vertex data: the 16 B Vtx in the GV1 ring layout (pm4.v.pica reads ob, flag, tc, cn). The flag field (bytes 6 and 7) is the palette index. DLC1: the replay copies each chunk Vtx block to the frame ring (pm_gpu_gvtx_load) and writes the flag. DLC2: a static vertex area of linear memory that follows the ring in the same attribute buffer (indices 16384 to 65535, up to 49152 vertices, 768 KB). The compiler copies the Vtx once (one GSPGPU_FlushDataCache for the entry). All vertices of the static area carry palette index 0.
- Chunks: one chunk per G_VTX load (map average 11.8 vertices, max 32). Per chunk: an object-space box (6 x s16), the first index and the index count of the triangles that the compiler assigns to this chunk (a triangle belongs to the chunk of its newest slot), the geometry mode masks at this point.
- Ranges: a chunk run with the same geometry mode (G_GEOMETRYMODE ops split ranges). Per range: and-mask and or-mask from the list start (the result at replay is `(entry_geom & and) | or`), the cull type follows from it as in gv_state_why (pm_gbi.c:1269).
- Exit state: the final geometry mode masks, the final slot map (64 Vtx addresses and the live bits), tri_since_vtx.
- Memory per map (S counts): Vtx 3348 to 3754 x 16 B = 54 to 60 KB, indices 2040 to 2210 x 3 x 2 B = 12 to 13 KB, chunks 283 to 308 x 24 B = 7 KB, entries 174 x 48 B = 8 KB, exit slot maps 174 x 136 B = 24 KB (or shared for lists that end with the same loads). About 110 KB per map.

### 2.3 Kind G: replay

At pm_gbi.c:2100 (case G_DL), after `next` is resolved, with switch `dlc`:
1. Lookup. A miss on a static class: compile (budget 1.5 ms of compile per frame, then interpret until the next frame). A refused entry: interpret.
2. Entry state check, once per list: the refusals of gv_state_why (lighting, texgen, fog mix, depth image role, TEXEL1 plan, debug switches, pm_gbi.c:1273 to 1277) evaluated with the geometry mode of each range. A refusal: interpret the list as today (the entry stays).
3. Box test per chunk with `combined` (pm_gbi.c:2168): plane-wise bound of the box against the 5 planes of clip_bits4 (pm_gbi.c:1155: x < -w, x > w, y < -w, y > w, z > w). For a plane, the minimum of a linear function over the box is 3 min-of-two products plus a constant: about 25 multiply-adds per chunk, 0.3 to 0.5 us (E). Whole box outside one plane: the chunk is skipped. This is exact: every triangle of the chunk then has all 3 vertices outside that plane, so the interpreter rejects all of them too (pm_gbi.c:1285). A model box (union of its chunks) is tested first and skips the whole list.
4. Near: the minimum of w over the box at or under 0.05 (the gv1raw test, pm_gbi.c:2172 to 2174). Then the chunk takes the GV1 path: the replay runs the interpreter on the list (DLC1), or on the chunk with the stored slot map (DLC2). Counted as `near`.
5. Draw: per range in view, the epoch state prep runs as in gv_tri (pm_gbi.c:1290 to 1301 and 1333 to 1336: ensure_tile, dual_bind, sync_render, gv_params_calc for the texture scale). Then one append of the index ranges of the chunks in view to the frame index ring (memcpy of u16 indices, plus a base add in DLC1). New pm_gpu call: `pm_gpu_grange(const u16* idx, unsigned n, unsigned base, const float m[4][4], int cull)`. It opens or extends a batch under the same rules as pm_gpu_gtrip (pm_gpu.c:1487 to 1490).
6. Exit: set `geom` to the exit masks, call pm_gpu_set_mode if it changed, store the exit slot map lazily (a flag `dlc_slots = entry`). If a later interpreted triangle reads a slot while `dlc_slots` is set, the replay first loads the slots of the entry through the normal G_VTX code with the matrix of the replay (counter `slotfix`, expected 0 in maps: 1.2 shows 0 cross reads).
7. G_DL_NOPUSH (a branch) to a compiled list: after the replay, do what G_ENDDL does (pm_gbi.c:2091 to 2098).

CPU cost per list: lookup 0.5 us, state check 0.5 us, model box 0.5 us, chunk boxes 0.4 us each, index copy 0.1 us per 10 triangles (DLC2), range prep only when the epoch changed. Zero work per rejected triangle, about 1 store per 3 indices in view. Per frame in town (E): 120 lists x 1.5 us + 250 chunks x 0.4 us + index copy 0.1 ms = 0.4 ms, plus the range prep that also runs today.

### 2.4 Kind S: compiled state lists (DLC3)

- Prerequisite DLC3a: gather the state that the state ops write in pm_gbi.c (tile_shift, the tile descriptors, TMEM map, timg, tlut, comb_w0/comb_w1 and the decoded combiner, othermode H and L, geom, tex_on, tex_ss, tex_ts, prim, env, fog, blend, conv_k4, keyc, keys, prim_z) into one struct `GbiState`. Mechanical change, about 300 lines, no behaviour change, golden 0.00.
- Compile: snapshot GbiState, run the existing op code for the list with draws disabled (a state ops pass, no triangles in kind S), snapshot again. The delta is the list of changed fields (offset, size, bytes). The side effects of a state op that are not state are recorded separately: a flush of the open batch (only when the delta changes a field the open batch reads), and the texture loads (G_SETTILESIZE calls pm_tex_load today, post_po3_path.md A2 and A3).
- Texture references: the delta holds the texture key (N64 image address, format, size, tile values, TLUT address), never a pm_tex slot pointer. The texture is resolved at draw time through pm_tex. A cache of (key to slot) per entry is valid while the slot generation is equal (new field: one u32 generation per slot, bumped at drop and at pm_tex_invalidate of that slot, pm_tex.c:510 to 544). The FL1 slab can evict and re-upload: the generation catches it.
- Replay: compare the delta id with the id of the last applied delta (equal: no work, common when consecutive models share a texture), else copy the fields (10 to 40 stores) and set `gv_epoch++`.
- State block id: the epoch prep (ensure_tile, dual_bind, sync_render, combiner plan) is keyed by a 64-bit hash of the GbiState fields it reads plus the texture slot ids. A table of 512 entries holds the results (plan id, TevKey, texture binds, uvso inputs). Hit: no prep, only a compare. This is the 2b block of arch_review.
- Values set by game code per frame (prim and env tint at src/model.c:1445 to 1456, fog at 1752 and 1896, tile size panning at 1966 to 1974) are main-list ops. The interpreter applies them after the delta, so the delta never hides them. Colour constants stay out of the state block key: they are uniforms or texenv constant writes per draw (1 to 3 register writes).

### 2.5 Batches and draws across lists

- Merge rule: consecutive ranges in list order (no interpreted draw between them) with equal state block id, equal matrix and the same vertex buffer go into one DrawElements. Order is kept, so blend order and decal order do not change.
- Matrix: static vertices carry palette index 0. A batch that holds static ranges has the range matrix in palette entry 0. A static range with another matrix starts a new batch. IGNORE_MATRIX models all use the camera matrix, so most map ranges can merge.
- Draw count (E): today 78 to 105 draws in town. Ranges whose chunks are all off screen give no draw (as today). Ranges whose box is partly in view but whose triangles are all off screen give a draw that the GPU culls (today the CPU drops it): +5 to +15 draws (gv1raw had +42 without any reject, GV1.md 8). Consecutive merges of models with the same texture: -10 to -30 percent. Net: 70 to 95 draws.
- Draw cost: 79 us per draw today (gp+gd, M). DLC-P (DLC5) builds the register packet of each state block once (the RAW1 part that is not done, GV1.md 11) and copies it per draw: target 20 to 30 us per draw (B1 r0 and r1 measure the floor). Gain 3.5 to 5 ms at 90 draws.

## 3. Invalidation and safety

### 3.1 Region generations (the decomp memory model)

| class | written when | hook (bumps the generation of the class) |
|---|---|---|
| shape | map load and battle stage load (load_asset_by_name, Yay0 decode into the buffer, the big-endian fix pass) | pm_map.c:375 (next to `pm_tex_invalidate(dst, dec)`), the `load_asset_by_name` wrap, and every nuPiReadRom or dma_copy whose destination overlaps the range (pm_os.c:386, next to pm_tex_invalidate) |
| texheap | mdl_load_all_textures, dma_copy of images (src/model.c:2059, 2071), make_texture_gfx (2081) | the same dma_copy and ROM read hook, plus the start of mdl_load_all_textures (src/model.c:2364, TARGET_3DS line) |
| rodata | never | none. `dlccheck` hashes at every replay to prove it |
| entity | entity create and delete, entity model load, HiddenPanel writes | entity.c alloc and free of pm_world_entity_heap blocks (TARGET_3DS lines), the ROM read hook. Content hash at first reuse after any bump |
| genheap | animator and model copy loads, heap_malloc and heap_free | DLC4 only: content hash at first reuse after a bump of the block that holds the list (heap block header address as a sub key) |

- The hook is one compare per ROM read and per Yay0 decode: `if (dst < hi && dst + n > lo) gen[class]++`. A bump does not free anything. An entry with an old generation is a miss and recompiles. At a map load all shape entries fall out: the LRU (3.4) frees them.
- A list in a static class that calls a list in a dynamic class (G_DL into main, sprheap, other): refused (reason `dynchild`). Measured 0 for map shape lists (1.2).
- G_MODIFYVTX: refused at compile. At replay, a G_MODIFYVTX in the main list after a replayed list touches the exit slot map: the lazy slot load of 2.3 step 6 runs first (vert_scr then applies as today).
- Segments: a static list with a segment address (w1 >> 24 under 16) stores the segbase values it used. Replay compares them (one compare per used segment). Map lists hold host pointers after the load fix (pm_map.c), so this is rare.

### 3.2 Exactness rules

- Slot rules: the compiler is the same code as the interpreter (2.2). Refuse any triangle that reads a slot not loaded inside the compiled unit. Exit slot map restored lazily.
- Lighting and texgen: refuse at replay when the range geometry mode has G_LIGHTING or G_TEXTURE_GEN (the GV1 rule, pm_gbi.c:1273). Lit vertices keep the CPU colour path. mac_01 has 1 lit list (1.2).
- Fog: refuse at replay when fogmix_on and G_FOG (pm_gbi.c:1274), as GV1 does. A fog colour or fog range op in the main list changes no compiled data.
- Geometry mode and cull: per range from the masks and the entry mode. The PICA cull mode comes from the winding sign of the post matrix (GV1_handoff.md: ccw = (gv_wind > 0) == gv1flip). Same function as gv1.
- Depth image role (zmark), TEXEL1 plan, decal2 and wide UI refusals of pm_gpu_gtrip: the range takes the interpreter.
- Wide mode: the box test uses the same 5 planes as clip_bits4, so the reject set is a subset of the interpreter reject set. render_models keeps its own wide cull (src/model.c:2835).
- Float24: the GPU transforms as in GV1 today. No new transform rule.

### 3.3 Check mode `dlccheck` and gates

- `dlccheck=1`: at every replay, hash the list words and the Vtx bytes and compare with the compile hash (counter `hashbad`). Compare the exit state with an interpreted run on a GbiState copy (counter `exitbad`).
- `dlccheck=2` (Azahar only, slow): run the interpreter on the same list into a capture buffer (pm_gpu emit disabled). Rule: every triangle that the interpreter emits must be in a replayed range with the same state block key (counter `missing`). Every replayed triangle that the interpreter did not emit must have its 3 vertices outside one plane or be culled by the cull mode (counter `extra_bad`). Gate: `hashbad`, `exitbad`, `missing`, `extra_bad` all 0 on toad, yard, battle, pause1, V16b, V18, V19.
- Hash gates: statehash equal with and without `dlc` on the route (the render does not change the game state, route_report.py --compare). Golden 10 scenes 0.00 against the `nodlc` result before the default turns on.
- `dlc` line every 300 drawn frames: hits, misses, compiles, compile ms, refused by reason (lit, fog, zmark, tex1, dbg, dynchild, modvtx, slot, near, long), lists replayed, chunks tested, chunks rejected, ranges drawn, triangles replayed, slotfix, cache KB, evictions.

### 3.4 Memory

- Toad Town: about 110 KB per map for kind G (2.2), plus kind S deltas (150 texture lists x 64 to 200 B = 10 to 30 KB), plus state blocks (512 x 96 B = 48 KB). Entity and animator lists (DLC4): 50 to 150 KB (G).
- Budget: static vertex area 768 KB linear (after the 256 KB ring, from the 2.5 MB that MEM1 freed, pm_gpu.c:892). Entries, chunks, indices and deltas in the newlib heap with a cap of 512 KB. The audio PCM cache (1.5 MB, AUD_DSP.md) and the FL1 slab (4.75 MB) are not touched.
- LRU by bytes over entries, with an age stamp per use. At a map change the shape class bumps and its entries become free at the next compile. The static vertex area is a free list of extents (the FL1 slab code, pm_tex.c:295, can be reused).

## 4. Expected gain on the console

Basis (M): town gbi 38.8 to 40.4 ms, 7000 ops, 1440 triangles, 2400 vertex loads, 78 to 105 draws, gp+gd 7.2 to 8.3 ms. 926 phasegbi: gt+gv 13.2 ms of 35 ms. 970 per op: 5.8 us average.

| Part | today (ms per drawn frame) | after (E) | saved (E) |
|---|---|---|---|
| Static geometry ops (DLC-G): 85 to 92 percent of 10 to 13 ms of triangle and vertex work | 10 to 13 | 1.5 to 3 (replay 0.4 ms, range prep, the dynamic share) | 6 to 10 |
| Static state ops (DLC-S): 3500 to 4500 ops at 2 to 3 us, plus the epoch prep per draw (60 to 100 us x 90) | 14 to 20 | 4 to 7 (delta apply, cache misses, colour writes) | 8 to 15 |
| Dynamic ops (main preamble, sprites, HUD, entities) | 8 to 10 | 8 to 10 | 0 |
| gbi sum | 39 | 14 to 25 | 14 to 25 |
| Draws (DLC-P): 90 draws at 79 us | 7.2 to 8.3 | 2 to 3 | 4 to 5 |

- Town: CPU part 63.6 to 66.9 ms to 35 to 46 ms. Under 50 ms gives 20 drawn per second (today 12.8). Under 33.3 ms needs the best end and a cut in logic or `ot`.
- Battle: no console data in build 970. Battle stage lists are shape lists (kind G and S). Actors are animator models (kind M, DLC4) and sprites (dynamic). Estimate 5 to 12 ms (G) after DLC3, more after DLC4.
- Menus (pause, file select): texture rectangles and HUD quads, dynamic. Estimate 0 to 1 ms. DLC-P helps the draw count there.
- Honest range: the low end assumes refusals for 15 percent of the static lists, an epoch prep that still runs per draw, and no draw merge. The high end assumes the state block cache hits 90 percent.
- Stop rules: (1) DLC0 static triangle share under 50 percent in town: stop, no DLC. (2) DLC1 gbiops cut under 600 per frame in the toad preset: stop and re-read the dlsrc data. (3) DLC2 console gb saving under 4 ms in town (route town_walk and plaza_walk, or the hand-play windows): stop DLC-G work, go to DLC3 only if the dlsrc state share is 40 percent or more. (4) DLC3 console gb not under 25 ms in town: stop before DLC4.

## 5. Slices

All slices: switch `dlc` (default off until DLC2 passes on the console), `nodlc` turns it off, `dlccheck` as 3.3, abtog state `dlc`. Lane A (main build tree), one code agent. Code in a new file 3ds/src/pm_dlc.c, hooks in pm_gbi.c (G_DL case and end of frame), pm_gpu.c (grange, static area), pm_os.c and pm_map.c (generation hooks).

| Slice | Content | Size (lines) | Azahar work gate | Pictures and checks | Starts |
|---|---|---|---|---|---|
| DLC0 | `dlsrc` counter (1.4). Fix the four `(null)` names in pm_phase.c:21 (cname has 22 names, C_N is 26) in the same commit | 150 | dlsrc line in toad and yard. Expect static tri share 85 percent or more | none (counter only) | 1 |
| DLC1 | Kind G for class shape. Compile, chunk boxes, replay into the GV1 ring (per-frame Vtx copy of the chunks in view, base add per index), lazy exit slot map, generation hooks for shape, `dlc` line, `dlccheck=1` | 450 | toad preset 300 steps standing: gbiops per frame falls by 900 or more (geometry ops of the map lists not interpreted), dlc hits over 95 percent of shape G_DL after the first frame, chunks rejected 50 percent or more, hashbad 0, exitbad 0 | golden toad and yard 0.00 with `--extra dlc` | 3 |
| DLC2 | Static vertex area after the ring (768 KB), zero work per triangle (index memcpy only), batch merge across lists, near chunk fallback per chunk, `dlccheck=2` | 300 | gvload per frame falls by 85 percent of vtxload in toad, dlccheck=2 counters 0 | golden toad, yard, battle, pause1. Shots of V16b, V18 pan, V19 dive (30 shots, 0.00) and wide mode on toad | 4 |
| HW-A | Console push: route play1_nosave and hand play in mac_00 and mac_01 with `phase phsum dlsrc abtog=base,dlc:30` | 0 | | phs gb, gp, gd per state. Gate: gb dlc lower than base by 6 ms or more in town | console |
| DLC3a | GbiState struct (2.4), no behaviour change | 300 | gbiops equal to before | golden toad, yard 0.00 | 1 |
| DLC3b | Kind S for texheap and rodata, texture slot generations, state block cache | 500 | gbiops per frame in toad falls by a further 3000 or more, state block hit rate 85 percent or more | golden 10 scenes 0.00 against nodlc, dlccheck=2 on 7 scenes | 4 |
| DLC4 | Kind M for entity and genheap (entity and animator lists, G_MTX inside to the palette, content hash at first reuse) | 400 | battle: dlc hits on animator lists, gbiops falls | golden battle, pause1, V19 | 3 |
| DLC5 (DLC-P) | Pre-built register packets per state block, copy per draw | 500 | rawcall per draw 1.0, draws per frame not higher than base | golden 10 scenes | 3 |
| HW-B | Console push with the full route (one full golden before S28 as the rules say) | 0 | | ms per drawn frame in the route town segments, phs, pcx flush lines | console |

Not part of DLC: logic cost (8 ms), FS1 pacing, the HUD counter fix (hw_p4 6.2).

## 6. Risks and open questions (with defaults)

1. The static share is lower than estimated (entities and sprites heavier in other towns). Default: DLC0 decides (stop rule 1).
2. The epoch prep per range is the real cost and DLC-G alone gains little (as GV1d found: per-draw cost is the lever). Default: DLC2 HW gate at 6 ms, else go to DLC3 at once.
3. Box partly in view gives more draws and more GPU triangles. GPU has 25 ms of room (8 ms used). Default: accept up to +15 draws. Split chunks over 32 vertices is not needed (max 32).
4. Texture load order changes in DLC3b (the texture is resolved at the draw, not at G_SETTILESIZE). A list whose geometry is all off screen then loads no texture. Pictures equal, texstat counts change. Default: accept, note it in the dlc line.
5. A game write into a static class that no hook sees (a decomp memcpy into TextureHeap or .data). Default: `dlccheck=1` on every golden scene and the route in Azahar before default on. A found writer gets a hook or its class becomes mixed.
6. Battle stage load into the shape buffer while the world map stays loaded (battle returns). Default: the generation hook bumps on both loads. A recompile after a battle costs about 2 to 5 ms once (G).
7. Compile spikes at map entry (174 lists). Default: 1.5 ms compile budget per frame. The map entry frames are already 117 to 217 ms (hw_p4 section 4), so the first 2 to 4 frames compile.
8. 64 slots in the port against 32 in F3DEX2. Default: the compiler copies the port rule (64), so it equals the interpreter. Map loads never pass 32 (1.2).
9. Two command buffers (arch_review stage 3 says they go with DLC-G). Default: not in DLC1 or DLC2. The static vertex area is written only at compile, so the single buffer is safe. Revisit with DLC5.
10. Lane: DLC1 to DLC3 touch pm_gbi.c, which the graphics agent (VIS-3) also edits now (the working tree has uncommitted pm_gbi.c changes). Default: DLC starts after VIS-3 merges, in lane A.

## 7. Results (DLC code agent)

### DLC0 (switch `dlsrc`, tool 3ds/tools/dlsrc_report.py, Azahar route play1, logs 3ds/logs/dlc/d0_route.txt)
Two lines per 300 drawn frames: `dlsrc` (ops by class, geometry, flow, state) and `dlsrc2` (triangles, triangles by vertex class, vertex loads, distinct G_DL targets, stable targets). Class "other" holds lists that no range names (not main, shape, TextureHeap, model lists, entity, general or sprite heap).
Per drawn frame, Toad Town walk (mac_00): ops 6249, of which main 2185, shape 1157, texheap 1551, rodata 581, other 774. Static ops (shape, texheap, rodata) 53 percent, not the 70 to 80 percent of the estimate. Triangles 1266, 93 percent from static lists. Vertex loads 2106, 92 percent static. 137 distinct G_DL targets per frame. All static targets stay equal over 30 frames (1122 of 1122).
Other scenes (static share of ops, triangles, vertex loads): plaza 47, 84, 84 percent. Battle (3 windows) 40 to 45, 93 to 98, 89 to 96 percent. Field 44, 88, 86 percent.
Reading: DLC-G (shape geometry) covers the triangles and vertex loads (stop rule 1 does not apply: 93 percent is over 50). The main list is 35 percent of the ops and "other" 12 percent, so DLC-S can save at most about half of the ops.

### DLC1 (switch `dlc`, default off, `nodlc`, `dlccheck`; code 3ds/src/pm_dlc.inc.c, included by pm_gbi.c)
Design as built (differs from the plan text in four points):
- The unit is a group, not a G_VTX chunk. A group starts at a G_VTX at slot 0 after triangles (the interpreter clears the slot live bits there), so a group reads only the vertices that it loads. One box per group (object coordinates). The box test uses the five planes of clip_bits4 with a relative margin of 1e-5, so a skipped group has no triangle that the interpreter keeps. `dlccheck` proves it per skipped group (counter `missing`).
- A group in view is replayed with the interpreter code (G_VTX copy to the GV1 ring, draw_tri) and no CPU transform: vert_cb is 0, vert_gvf is 0, as in the `gv1box` path. The GPU clips the triangles that the CPU would reject by vertex. A group that touches the near plane, has G_LIGHTING, or does not fit the ring (room for its vertices plus 3 copies per triangle) runs through pm_gbi_run on its op range (counters near, lit, ring, mat).
- Geometry mode is kept by cumulative masks per group. The exit mask is applied at the end. `pm_gpu_set_mode` runs only when the mode changes.
- Stale test: counter pm_dlc_gen. It goes up in pm_tex_invalidate (every ROM read and Yay0 decode that overlaps gMapShapeData) and after fix_shape in decode_yay0. A change drops every entry. `dlccheck` hashes the list words and the Vtx bytes at each replay (`hashbad`) and checks the exit mode and tri_since_vtx (`exitbad`). `slotfix` counts a triangle dropped for a dead slot after a replay that skipped its last group.
Lines: `dlc` (hit, comp, refusals, lists, groups, skip, replay, near, lit, ring, mat) and `dlc2` (tri, opsrep, hashbad, exitbad, missing, slotfix, ringfail, entries) every 300 drawn frames.
Azahar results (route play1, same switches with and without `dlc`, statehash equal in 9 samples, route end equal):
- Town walk (241 steps): gbiops per step 7138 to 5848 (-1290), triangles to the draw 1441 to 532, vertex loads 2398 to 897, draws 81.4 to 82.4, gbi 10.47 to 8.67 ms (x3.5 hardware ratio: -6.3 ms). Plaza walk: gbiops 6793 to 5722, gbi 11.00 to 9.41 ms. Field walk gbi 6.59 to 5.62. Battle gbi 5.90 to 5.48.
- Toad preset, 300 frames: 94 lists, 126 groups per frame. 76 percent skipped, 23 percent replayed, 0.9 percent run by the interpreter (near). hashbad, exitbad, missing, slotfix, ringfail all 0 in the toad preset, the full cycle and the route (dlccheck).
- Pictures against the run without `dlc`: golden toad 0.00, yard 0.00, battle 2.56 (same as without), pause1 0.06 (same), V19 dive 30 shots 0 px, V18 pan 40 shots 0 px, wide toad 6 shots 0 px. Statehash full cycle f1800 53090ae9, f4000 d95fd1a4, f8000 82add753 equal.
- Not tested: V16b scene alone (golden battle only), a console run.

### After DLC1: what is left in the gbi phase (Azahar, toad preset, switch gbiprof, per drawn frame, timer floor about 1.7 us per op)
- Ops per frame 5547 (base) to 4434 (dlc). Time by op, inflated by the timer: base 23.0 ms, dlc 17.1 ms. G_DL with replay 2.9 ms (of which about 1.1 ms is the replay, the rest is the timer). The state ops stay: G_SETTILESIZE 172 calls at 9.4 us above the floor (1.6 ms: 2.3 us before the load, 3.7 us in pm_tex_load, rest flush), G_SETCOMBINE 186 at 5.0 us (0.9 ms), G_LOADTLUT 118 at 6.2 us (0.7 ms, includes the batch flush when the bound palette changes), G_SETTILE 458 at 2.0 us (0.9 ms), G_SETOTHERMODE_H 454 at 1.2 us.
- A CI texture list is 17 ops: SETTIMG, TILESYNC, SETTILE, LOADSYNC, LOADTLUT, PIPESYNC, SETCOMBINE, TEXTURE, SETTIMG, SETTILE, LOADSYNC, LOADBLOCK, PIPESYNC, SETTILE, SETTILESIZE, SETOTHERMODE_H, ENDDL. A render mode list is 5 ops (OTHERMODE_L, OTHERMODE_H, GEOMETRYMODE twice, ENDDL). 90 texture lists and about 100 render mode lists per frame. The same texture list twice in a row: 11.6 percent of the calls, within the last 3 lists 30 percent.
- Texture lists cost about 30 us each (2.7 ms of 8.7 ms gbi in Azahar). The plan (2.4) assumed 3500 to 4500 state ops at 2 to 3 us. Measured: 1551 texture ops and 581 render mode ops per frame at 1.5 us average, and a part of that cost is real work (batch flush, pm_tex_load), which a compiled list must still do. Expected gain of DLC3b is therefore 1.5 to 2 ms in Azahar (5 to 7 ms on the console), not 8 to 15 ms.
- DLC2 (static vertex area): the replay is about 1.1 ms in Azahar, so DLC2 can save about 0.5 ms (about 2 ms on the console). Not built.
- Reading for the decision: stop rule 2 and 3 of section 4 are checked in Azahar only. The console gate HW-A (section 8.0d of hw_batch.md) decides DLC2, DLC3a, DLC3b, DLC5.

### DLC2 (lane B, switch `dlc`, `nodlc2` turns it off, abtog state `dlc1` = dlc without DLC2; code pm_dlc.inc.c, pm_gpu.c)
Design as built: a compiled group copies its Vtx once into a static vertex area (16384 vertices, 256 KB, behind the GV1 ring in the same buffer, indices 16384 and up, flag 0 = palette entry 0) and keeps its triangle indices (dead-slot triangles dropped at compile, the position of each in-group geometry op kept). A group in view (not near, not lit, no texgen, no fog mix) runs `dlc_static_group`: per segment between geometry ops one `pm_gpu_grange` append (memcpy of u16 indices, no per-vertex or per-triangle interpreter work). The batch palette must hold the group matrix at entry 0, else a new batch starts. The state of the epoch (ensure_tile, dual_bind, sync_render, params) comes from `gv_epoch_enter`, split out of `gv_tri`. The area is reset with the shape-buffer generation (`pm_gpu_gstat_reset`, after FrameBegin SYNCDRAW). A group without room, a lit/near group, or a refusal of the raw path runs the DLC1 path.
Azahar (toad preset, abtog base,dlc1,dlc, 300 steps per state, 3ds/logs/perf/dlc2_abtog_toad.txt): gbi 11.11 (base), 9.10 (DLC1), 8.51 ms (DLC2). cpu50 17.3, 15.3, 14.7 ms. DLC2 against DLC1: -0.59 ms (x3.5 = about -2 ms on the console). dlc2 line: static grp 8694 of 8695 replayed groups, fail 0, full 0, 45 KB static area. Draws 91, 90, 90.
Checks: statehash equal with and without `nodlc2` (10 of 10 samples, dlccheck on: hashbad, exitbad, missing, slotfix, ringfail 0). Golden toad 0.00, yard 0.00 with `--extra dlc`.

### DLC3a and DLC3b (lane B): kind S measured, no gain, code not kept
Built and measured, then dropped. DLC3a gathered the gbi state into a struct (S3St: tiles, tile sizes, timg, loads, tlut, combine, texture, othermode H and L, geometry mode) and grouped the ops of a state list (texture list, render mode list) by the groups they write and read. DLC3b skipped a state list when the groups had the values at the end of its last run in the same frame and the texture epoch was equal. A version counter variant gave the same result.
Azahar toad preset, 300 frames, `dlccheck` on: 87614 G_DL targets, 3205 distinct state lists parsed (the rest are shape lists and lists with other ops: G_VTX 27652, G_MOVEMEM 544, G_DL 539, G_SETSCISSOR 526). Skip candidates: 0. Every repeat call of a list found other values: render mode OML 8689, OMH 11323 and 5214, geometry mode 11541, texture image 4802, combine 801. The scene alternates render modes and textures from model to model, so no list sees the state it left. Back-to-back repeats (11.6 percent in the plan) are not enough, because the lists between them change the groups.
Reading: the gain of 1.5 to 2 ms (Azahar) that the plan expected for DLC3b does not exist for an elision design. The cost of a texture list is the work inside its ops (pm_tex_load, batch flush, combiner sync), which a skip cannot remove. Stop rule: no DLC3 code merged. The attempt is not in the tree. A real saving needs a cheaper SETTILESIZE/SETCOMBINE path (row PERDRAW), not a skip.

### PERDRAW (lane B): per-draw cost, profile only, no code change
Azahar toad preset, dlc on, `phase phasegbi`, 300 steps, 73 draws per step (3ds/logs/perf, row PERDRAW): gp 0.63 ms (g_tev 0.24, g_bind 0.25, g_state 0.21), gd 1.10 ms (C3D_DrawElements and the buffer info), gcache 0.13 ms. Per draw (Azahar): gp 8.6 us, gd 15 us. On the console (x3.5) gd is about 53 us of the 79 us.
`gbiprof` flush table: 986 flush calls per step, 923 empty (return at the first test), 63 drawn. The empty calls come from pm_gpu_set_mode (232), gbi lines 2980, 2803, 2754 (162 each), set_plan (67), set_combine (56). They cost no draw.
Reading: the state caches (tev key, sh_* setters, texture bind, palette registers) already skip every repeat. What stays is the work inside citro3d's C3D_DrawElements (context update, command write) for each real draw. No change in this code gives a measurable saving without a hardware profile. The only lever left is fewer real draws (63 to 73), which needs one batch across texture changes (an atlas or a texture array): not a small change. No code was changed for PERDRAW. Hardware step: `phase phasegbi` on the console shows whether gd is the C3D_DrawElements call or the buffer info (hw_batch.md 8.0e, state `phasegbi` of abtog).
