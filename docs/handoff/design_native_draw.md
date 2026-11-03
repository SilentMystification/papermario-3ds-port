# design_native_draw: phase 0 derivation for the native draw rewrite

Plan: `C:\Users\Andrew\.claude\plans\you-are-not-an-glittery-nautilus.md`. Step 1. Read-only on source. No Azahar start.

Tags:
- **[C file:line]** CONFIRMED by reading source or a tool output in this step.
- **[D doc]** taken from a handoff doc or log. Not re-measured here.
- **[I]** INFERRED. Arithmetic or reasoning from [C] and [D] facts. Not measured.

Glossary: L1I and L1D = ARM11 instruction and data cache (16 KB each). PICA = the 3DS GPU command unit. TEV = texture environment (combiner stages). GBI = graphics binary interface (the N64 display list format). fp24 = the 24-bit float of the PICA vertex shader. UC = the record (replay) work of the port. Units: ARM11 at 268 MHz. 1 us = 268 cycles.

---

## 0. Corrections to the plan

| Plan says | Tree says | Source |
|---|---|---|
| SEAM at pm_gpu.c 226 | SEAM comment at pm_gpu.c:228 (`Mtx_Multiply(&shader_3d, post, &game)`) | [C pm_gpu.c:228] |
| second walk at pm_gbi.c 441 | pm_gbi.c:441 is `unpack()`, not a walk. The second walk is `gbi_task` at pm_nusys.c:81 | [C pm_nusys.c:76, 81; pm_gbi.c:441] |
| eye shift location | `upload_mvp()` at pm_gbi.c:446 to 459 | [C pm_gbi.c:446-459] |
| flush_draw 900 to 1120 | flush_draw is pm_gpu.c:904 to 1133 | [C pm_gpu.c:904, 1133] |
| "pso_tab" template table | no `pso_tab` in the tree (grep: no match). The template key is `TevKey` (pm_gpu.c:494) and `PmPlan` (include/pm_gpu.h:70), built by `pm_comb_build` (pm_comb.c:393) | [C] |
| "GPU time (gdraw) 8.1 ms" and "GPU busy about 17 %" (plan_draw_cost.md:13) | `PH_GDRAW` is CPU time inside the draw call (pm_gpu.c:849, 1104). It is not GPU busy time. GPU busy time has never been measured. The 17 % figure is withdrawn | [C pm_gpu.c:849, 1104] |
| RecKey 64 B (plan_uc3.md:22) | agent_uc3d.md:21 says RecKey is 32 B aligned, about 96 B. Not re-checked here | [D agent_uc3d.md:21] |
| producers 3931 gSP/gDP lines | 3978 lines with regex `g[SD]P[A-Za-z0-9_]*\(` over src/. The plan figure used another regex | [C] |
| effects 1700 lines | 1727 lines in 127 .c files (128 entries in src/effects, one is not .c) | [C] |
| walker per-model code 55.5 KB | the named set in section 2.2 sums to 37.7 KB. The 55.5 KB figure is not reproduced | [C nm] |
| native 4.5 to 7 ms for 90 models | consistent with list build 4.4 ms plus emission 0.1 to 0.2 ms (section 2.4). Not an independent measurement | [D plan_dl_design.md section 1] [I] |

---

## 1. Census of the old path, per producer family

### 1.1 Producer size per family (static source counts)

Counts are `g[SD]P...(` call lines. Distinct CC and RM counts are distinct `G_CC_*` or `PM_CC*` and `G_RM_*` macro arguments in the family.

| Family | Files | gSP/gDP lines | Distinct CC names | Distinct RM names | Vertex calls | Primitive calls of note |
|---|---|---|---|---|---|---|
| static models, src/model.c | 1 | 150 | 31 | 37 | 0 | gSPDisplayList 8, gSPMatrix 8, gSPTextureRectangle 2, gSPPmRenderMode 1, gSPPmTexBind 1 [C] |
| sprites and NPCs, src/sprite.c | 1 | 39 | 6 | 3 | 3 | gSP2Triangles 3, gSPVertex 3 [C] |
| HUD, src/hud_element.c | 1 | 132 | 8 | 10 | 2 | gSP1Triangle 2, gSP2Triangles 1, gDPFillRectangle 1, gSPVertex 2 [C] |
| text and message boxes, src/msg_draw.c | 1 | 135 | 10 | 6 | 4 | gSPTextureRectangle 6, gSP2Triangles 4, gSPVertex 4, gDPFillRectangle 1 [C] |
| boxes, src/draw_box.c | 1 | 79 | 7 | 4 | 4 | gSP2Triangles 3, gSPVertex 4, gDPFillRectangle 1 [C] |
| entities, src/entity/ | 19 | 85 | 7 | 8 | 0 | no gSPVertex. Meshes come through the model path [C] |
| battle, src/battle/ | 39 | 62 | 7 | 8 | 0 | no gSPVertex [C] |
| effects, src/effects/ | 127 .c | 1727 | 25 | 13 | 18 calls in 14 files | gSPDisplayList 370, gSPMatrix 258, gDPSetPrimColor 158, gDPSetEnvColor 82, gDPSetTileSize 81 [C] |
| all producers, src/ | 246 | 3978 | 158 argument pairs | 103 names | | [C] |

### 1.2 Mesh kind per family

| Family | Mesh kind | Who builds the vertices | Evidence |
|---|---|---|---|
| plain models (G_PM_MODEL groups; model.c:1406, 1515, 2073) | **static**. Vtx arrays live in ROM or heap. Every static vertex has palette flag 0 | The interpreter copies each G_VTX into the GPU ring at run time (pm_gbi.c:3874, pm_dlc.inc.c:409 call `pm_gpu_gvtx_load`, defined at pm_gpu.c:2275). The data is not rebuilt | [C pm_gbi.c:3874, pm_dlc.inc.c:409, pm_gpu.c:2275] [D plan_uc3.md:42] |
| sprites and NPCs (G_PM_TEXI) | **dynamic**. Quads are built per call | sprite.c and sprite_component. 247 ops per frame in the Toad Town mac_01 view | [C sprite.c gSPVertex 3] [D plan_toad_model.md section 2] |
| HUD | **dynamic**, 2D quads | hud_element.c per call | [C] |
| text, boxes | **dynamic**, glyph rectangles (RDP rectangle ops and 2-triangle quads) | msg_draw.c and draw_box.c per call | [C] |
| entities, battle | **static** through the model path, plus dynamic sprite quads (item entities: 69 ops per frame in the mac_01 view) | model path and sprite path | [C no gSPVertex in entity or battle] [D plan_toad_model.md section 2] [I] |
| effects | **mixed**. 7 of the 18 vertex calls read static ROM arrays (`&D_...`). 11 read run-time buffers (vtxTemp, vtxBuffer, locals) | effects appendGfx functions | [C grep of src/effects] |

### 1.3 Draw counts per frame (Toad Town, build 1588, O3DS)

[D plan_dl_design.md section 1 table, windows f1513 to f2413]
- plain models: 90 per frame. Opaque plain models: 71 per frame, in 13 state runs. 15 removable closes. 47 texture changes among the 71 opaque draws.
- Texture changes: 47 of 71 opaque plain draws (66 %). So the texture is a per-model value, not a per-template value.
- G_PM_TEXI class (sprites and NPCs): 6.3 to 6.8 ms per frame.
- Walker ops per frame: 850 to 890 [D plan_dl_design.md section 1].
- Other top ops (phase on): e4 (G_TEXRECT, texture rectangle) 2.8 to 3.4 ms, 09 (G_PM_RM, render mode list) 2.0 to 2.1 ms [C include/gbi_custom.h:204].

No per-frame count exists in this step for effects, entities, battle, HUD or text. A per-family census is the first deliverable of step 2.

### 1.4 Render-state templates

The old path keys the draw state on this TevKey [C pm_gpu.c:494, set at 1041 to 1047]:
`combine, tex, tex1, i8_0, i8_1, rgb_scale, r_mix, draw_3d, rect_2d, dview, off_active, off_i8, r_memblend, r_replace, r_blend, prim, cconst, mixcol, plan_gen`.

Depth, alpha test, scissor, cull and blend are set on every close, outside TevKey, inside flush_draw [C pm_gpu.c:1062 depth test, 1071-1072 alpha test, 1087-1088 scissor, 1092 cull, 1094-1098 blend]. The program (pm4 or pm5, with lighting, texgen and fog) is chosen earlier in flush_draw by `gpal` and `g_use5`.

Design note [I]: `prim`, `cconst` and `mixcol` are colour values in the TevKey. Every distinct colour makes a new template unless the colour is patched as a record word. Section 3.2 recommends patching.

Static bound from source [C]: 158 distinct combine argument pairs (`gDPSetCombineMode` and `gsDPSetCombineMode`, unique). 103 distinct `G_RM_*` names. 6 format and size pairs in direct `gDPSetTile` calls: RGBA16, RGBA4, CI4, CI8, I8, IA8. The `pso_tab` table named in the plan does not exist.

Runtime count [I]: not measured. Toad Town opaque plain models run in 13 state runs per frame [D]. Their distinct count is 5 to 13. Whole-scene estimate 15 to 30 distinct templates, with colours patched. Step 2 measures it with the draw census logger.

---

## 2. Per-draw cycle and code-size budget

### 2.1 Measured split of one plain model (O3DS, Toad Town, phase on)

[D plan Evidence; plan_dl_design.md section 1 table]
- 230 us per model = 61,640 cycles per model.
- texture state (tex_op bind chain): 7.8 ms per frame = 38 % = 87 us = 23,300 cycles per model.
- geometry replay (dlc): 4.9 ms = 24 % = 54 us = 14,600 cycles.
- batch close (flush_draw plus citro3d): 4.8 ms = 23 % = 53 us = 14,200 cycles.
- rest (render mode, matrix, lights, combine): 3.2 ms = 15 % = 36 us = 9,600 cycles.

### 2.2 Code size (CONFIRMED with nm -S)

Run: `docker run --rm -v <build3ds>:/w devkitpro/devkitarm sh -c '/opt/devkitpro/devkitARM/bin/arm-none-eabi-nm -S --size-sort -C /w/pm_3ds.elf'`. Input: build3ds/pm_3ds.elf (file time Nov 3 01:42). Output: 43,510 symbols.

| Function | Bytes | Source |
|---|---|---|
| flush_draw | 11,504 | pm_gpu.c:904-1133 |
| tex_op | 5,616 | pm_gbi.c:2960-3250 |
| gbi_run_body | 5,600 | pm_gbi.c:3731-4055 |
| flush_tev_rebuild.constprop.0 | 2,788 | pm_gpu.c:621-776 |
| pm_gpu_gtrip | 2,544 | pm_gpu.c:2121 |
| gv_tri | 2,516 | pm_gbi.c:1496 |
| dlc_try | 1,748 | pm_dlc.inc.c (static) |
| pm_tex_bind | 1,352 | pm_tex.c |
| rec_try | 1,060 | pm_gbi.c (static) |
| mg_run.constprop.0 | 1,048 | pm_gbi.c:3672 |
| flush_draw_cpu | 1,520 | pm_gpu.c:845 |
| pm_gpu_rec_same | 712 | pm_gpu.c (UC2 section) |
| rec_done | 696 | pm_gbi.c (static) |
| mdc_op | 844 | pm_gbi.c |
| os_op_hook | 396 | pm_gbi.c |
| pm_gpu_eye | 380 | pm_gpu.c:1731 |
| pm_gbi_run | 248 | pm_gbi.c:3254 |
| pm_gpu_gvtx_load | 164 | pm_gpu.c:2275 |
| gbi_task | 132 | pm_nusys.c:76 |

Named set total: 37,712 bytes (37.7 KB). The plan figure of 55.5 KB is not reproduced. Per-object .text totals from the link map (build3ds/pm_3ds.map), summed per object [C, approximate]: pm_gbi.o 83.5 KB, pm_gpu.o 56.8 KB, model.o 40.2 KB, pm_tex.o 36.1 KB, msg_draw.o 23.3 KB, hud_element.o 19.5 KB, pm_comb.o 12.5 KB, sprite.o 9.4 KB, draw_box.o 9.3 KB.

The named set alone is 2.4 times the 16 KB L1I.

### 2.3 I-cache check (INFERRED)

Plan figures: 150,000 I-cache misses per frame and 41 to 44 % of cycles stalled on instruction fetch [D plan]. Frame wall time at phase on: 53.3 ms [D plan_dl_design.md section 1].
- Stall time: 0.42 x 53.3 ms = 22.4 ms = 6.0 M cycles.
- 6.0 M / 150,000 misses = 40 cycles per miss.
- hw_guidelines.md:7 gives 30 to 60 or more cycles per miss. 40 is inside that range. The stall share and the miss count agree.

### 2.4 Per-draw budget for 60 fps (INFERRED)

Frame budget 16.7 ms. With fps60 interpolation the walk gets about 11 ms [plan, honest feasibility note]. For 90 plain models: 11 ms / 90 = 122 us = 32,700 cycles per model. HEAD costs 230 us per model, 1.9 times this budget.

Native emission per draw (target):

| Component | Cycles (estimate) | Basis |
|---|---|---|
| read 16 B record (one record per half line) | 10 | sequential, PLD prefetch |
| bucket pass (counting sort by template id, two passes) | 20 | 90 records, one array |
| patch 16 palette words (two ldm/stm of 8 words) | 20 | 16 words |
| patch texture address, index address, draw count (6 words) | 10 | fixed offsets from the patch table |
| append about 32 words to the command buffer (one copy) | 90 | GPUCMD_AddRawCommands, 32 words |
| write-allocate misses on command and record lines (about 6 lines, 40 cycles each) | 240 | hw_guidelines.md:7 |
| loop and branch | 20 | |
| **total per draw** | **about 410** | about 1.5 us |

- 90 plain models: 90 x 410 = 37,000 cycles = 0.14 ms.
- Share of the 11 ms walk: about 1.3 %.
- Eye 2 (stereo) adds one memcpy of the command span (90 x 32 words = 2,880 words, 11.5 KB) plus 16 palette words per draw. Estimate 0.05 ms [I].
- Cross-check: list build (dl) is 4.4 ms for the whole frame [D plan_dl_design.md]. The native plain-model total is therefore at most 4.4 ms plus 0.14 ms. This agrees with the plan estimate of 4.5 to 7 ms [I].

Data working set in the emission loop [I]:
- record stream: 90 x 16 B = 1.4 KB
- palette matrices: 90 x 64 B = 5.8 KB per eye (two eyes: 11.5 KB)
- current bucket template block: 384 B, read once per bucket
- All hot data together is more than 16 KB if read as one set. The loop reads the streams in order, once each, with PLD. The live set per draw is about 2 KB.

### 2.5 Dynamic families (INFERRED)

- sprites and NPCs (G_PM_TEXI, 6.3 to 6.8 ms per frame [D]): dynamic vertices. Emission per quad is the same order as a model (about 400 cycles), plus the vertex build. Expected 0.3 to 0.6 ms for 40 to 70 quads. Count not measured.
- text and boxes: small quads, count not measured. Census needed.
- effects: count and mix not measured. Census needed. The dynamic path copies run-time vertices into a per-frame arena (section 3.6).

---

## 3. Per-family design

### 3.1 Record and template

Record, 16 bytes, two per 32-byte line [I]:

```c
typedef struct {
    u16 tmpl;      /* template id, 0..63 */
    u16 tex;       /* texture handle: TextureHandles index (< 128) */
    u16 mtx;       /* game matrix slot (pal_g index) */
    u16 nidx;      /* index count of this draw */
    u32 idx_off;   /* byte offset into the index arena */
    u32 vtx_base;  /* Vtx base address (static ROM or arena) */
} NRec;            /* 4 x u16 + 2 x u32 = 16 bytes */
```

Template block: 96 words (384 B), aligned 32 [I]. Built once per template from the flush state (section 3.3). It holds the words that do not change within a bucket: fragment ops, depth, blend, cull, TEV stages, unit config, attribute and buffer config, program.

Patch table per template [I]: entries `{u16 off; u8 kind}`. Kinds: 1 texture address unit 0, 2 texture address unit 1, 3 palette matrix (16 words), 4 index address (0x227 word), 5 draw count. The values come from the record fields.

### 3.2 Per-template and per-model words

Source of the words: the citro3d setters called by flush_draw, and the word-level diff of captured records [D].

Register facts with a source:
- 0x104 alpha test, 0x105 stencil test, 0x106 stencil op, 0x107 depth and colour mask [D diag_uc2_0x110.md:16-19].
- Texture address words 0x85 (unit 0), 0x95 (unit 1), 0x9D (unit 2) are class 1 "address" words [C pm_gpu.c:2571].
- Palette uniform words (u4_pal: 4 vec4 = 16 words per game matrix) are class 2 [C pm_gpu.c:174, 996].
- 0x227 is the index buffer config word. Its parameter 0 holds the index address minus the buffer base [D plan_uc3.md:50].
- Other fragment, depth and blend registers: set in the sh_* helpers (pm_gpu.c:531-566) [C]. Their register numbers are not listed in this step. Step 2 extracts them from a captured record.

Per template (constant within a bucket):
- depth and colour mask, depth map, early depth, alpha test, stencil, blend, cull, scissor [C pm_gpu.c:1062-1098]. Scissor is set from the display list per region. Treat it as template, patch when it differs.
- TEV stages: C3D_TexEnv, built by flush_tev_rebuild (pm_gpu.c:1050).
- texture unit config (type, format, size) for each bound unit, except the address word [I].
- attribute and buffer info (BufInfo_Add, pm_gpu.c:1109) [C].
- program bind (prog4 or prog5, C3D_BindProgram, inside flush_draw) [C].

Per model (changes every draw):
- palette matrix: 16 words per game matrix (class 2). Changes when the game matrix changes [C pm_gpu.c:996].
- texture address for unit 0, and unit 1 when used: words 0x85 and 0x95 (class 1) [C].
- index address word 0x227 and draw count [D plan_uc3.md:50].
- `prim`, `cconst`, `mixcol` colours, if patched and not keyed (section 1.4). Colour uniforms (cmul, cadd) are class 3 uniform words [C flush_draw uniform block].

Word-level diff evidence [D]:
- UC3M split at mac01t f307: oth 1384, adr 184, pal 0, uni 0 differing words [D agent_uc3b.md:25]. The "oth" class includes the 0x227 rewrites, which differ on purpose [D agent_uc3c.md:23]. The true other difference is not known from this count. The palette and uniform classes are masked.
- Per record: 0.77 KB average, about 190 words, 16 patch words [D plan_uc3.md:270]. The native per-draw command span is about 32 words (section 2.4). That cuts the copy about six times.

Is a bucket stable? [I] A bucket is stable when all of its records give the same template block. Per-model differences are then only the patched words above. The UC3 reccheck shows this for the replayed set (bad 0, miss 0 at yard_rec f348) [D agent_uc3d.md:42]. Step 2 extends the check to all families using the census.

### 3.3 Template build

Build once at init, and again if the pm_comb tables change:
1. pm_comb_build (pm_comb.c:393) makes the PmPlan for each combine word. Source has 158 pairs (section 1.4).
2. The state block is captured from the citro3d context after one draw of that template, with the UC capture path (pm_gpu_rec_begin at pm_gpu.c:2417, then the words between begin and end). This reuses the record machinery. Do not write a second register encoder.
3. Store each block in template memory, 384 B per template. For 64 templates: 24 KB. Only the current bucket's block is in the hot set.

### 3.4 Bucketing

- Opaque and depth tested (z_on, not r_decal): may be sorted by template id, then texture id. Safe for depth-tested opaque draws [plan seam rule 6].
- Alpha blended (r_blend, memblend) and decal (r_decal, GEQUAL with bias, pm_gpu.c:1062): keep game order. Coplanar decals depend on order.
- Alpha test (r_atest >= 0, pm_gpu.c:1071): writes depth, so it may be sorted as opaque. Confirm on the census.
- 2D and orthographic draws (rect_2d, ui_ortho): keep game order. They overlap like paint.

### 3.5 Emission loop (one loop, all eyes)

```
for each bucket (template t, in sorted order):
    copy template block t into the command span (one ldm/stm run, 96 words)
    for each record r in the bucket:
        patch palette words from pcomp[r.mtx]            (16 words)
        patch texture address units 0 and 1 from r.tex
        patch index address and draw count from r.idx_off, r.nidx
        append the span (about 32 words) to the GPU command buffer
    flush the written span once per split
```

Eye loop: `for (eye = 0; eye < eyes; eye++)`, with `eyes` = 1 in mono and 2 in 3D. Eye 2 copies the eye 1 span and patches palette words with pcomp_eye (section 4). No second walk.

### 3.6 Static and dynamic meshes

- Static mesh (plain models, entities through models): vtx_base points to the ROM Vtx array. No per-frame copy. This removes the G_VTX ring copy (pm_gpu_gvtx_load, pm_gpu.c:2275) for static meshes [plan, Vertices].
- Dynamic mesh (sprites, HUD, text, boxes, effects): the producer writes vertices into a per-frame arena (vtx_base = arena offset). One cache flush of the arena per split [hw_guidelines.md:8]. Arena size = the frame's dynamic vertex count (census needed).

---

## 4. Stereo 3D (one walk, two eyes)

HEAD [C]:
- gbi_task runs the walk once per eye: pm_nusys.c:81 `if (pm_gpu_stereo()) { pm_gpu_eye(1); pm_gbi_run(gfxList, n); pm_gpu_eye(0); }`. So 3D costs two walks.
- Stereo state: pm_gpu.c:98-106 (targetR, eye shift, conv).
- pm_gpu_stereo and pm_gpu_eye: pm_gpu.c:1730-1743.
- Per-frame eye select: pm_gpu.c:1934 (`eye_slider = osGet3DSliderState()`), 1936 (`stereo_frame = targetR && eye_slider > 0.02f`), 1940 (`pm_eye_e`).
- Eye shift for a perspective matrix: pm_gbi.c:446-459 (`upload_mvp`): x' = x + e x (w - conv).
- targetR is created at pm_gpu.c:1279.

Design:
- Build records, buckets and state blocks once per frame.
- Left eye: emit with eye 1 matrices.
- Right eye: memcpy the left span, patch 16 palette words per draw (pcomp_eye = post_eye x game, same product order), patch the target registers 0x110 to 0x11E once per eye.
- Ortho and UI draws: emit once, copy to both targets, patch only the target registers [plan].
- Mono: `eyes = 1`. The same loop runs. No separate path.
- GPU work doubles in 3D. CPU emission does not.

Gate: Toad Town at slider 1. Add `eye2 us` to the fsplit line next to the 2D gbi number.

---

## 5. Seam rules

Seams come from unequal arithmetic on shared edges. These rules keep the same arithmetic on the same inputs.

1. One matrix product per model, fp32 on the CPU, same order as today: pcomp_calc (pm_gpu.c:817-824, `Mtx_Multiply(&pcomp[gm], &gv_post, &game)`) and the SEAM product at pm_gpu.c:228 (`Mtx_Multiply(&shader_3d, post, &game)`). Upload the result. No second product.
2. One dp4 on the s16 object vertex. SEAM block at pm_gbi.c:1761 (a raw-path triangle takes its position from the GPU matrix). s16 values are exact in fp24.
3. No per-vertex arithmetic added in the shader. New scale or bias goes into the matrix.
4. Models sharing an edge with different matrices need matrices equal to fp24 rounding. Do not move viewport or tilt.
5. Keep the near clip (sh_user_clip, pm_gpu.c:557), the depth map (sh_depth_map, pm_gpu.c:536) and the decal bias unchanged.
6. Sorting changes no seam for opaque depth-tested draws.

Seam test per slice [plan]: draw each object in one solid colour on a contrasting clear colour, on HEAD and on the new build. Count background pixels inside the union of silhouettes. The new build must not add holes. Report the counts.

Host test before console [plan]: emulate the fp24 dp4 on shared-edge vertex pairs from the map data and check bit equality. No Azahar needed.

---

## 6. Slice plan and files

| Slice | Families | Files to change | Gate |
|---|---|---|---|
| 1 | static plain models (Toad Town) | new: 3ds/src/pm_native.c, 3ds/include/pm_native.h. Change: 3ds/src/model.c (record write at the G_PM_MODEL site), 3ds/src/pm_gbi.c (G_PM_MODEL group in mg_run for static plain models; old path deleted in the same commit), 3ds/src/pm_gpu.c (template capture, reuse of the rec capture, GPU busy probe), 3ds/src/pm_nusys.c (gbi_task eye loop), 3ds/ld/3dsx.ld.in (`.data.pmhot`) | census equal, reference scenes, seam count, console delta |
| 2 | sprites and NPCs (G_PM_TEXI) | src/sprite.c, src/sprite_shading.c, pm_native.c dynamic arena | as slice 1 |
| 3 | HUD, text, boxes | src/hud_element.c, src/msg_draw.c, src/draw_box.c | as slice 1 |
| 4 | entities and battle | src/entity/, src/entity_model.c, src/battle/ | as slice 1 |
| 5 | effects (dynamic path) | src/effects/*.c (127 files) | as slice 1 |
| 6 | delete the interpreter walker | pm_gbi.c (gbi_run_body, tex_op, mg_run, mdc_op, os_op_hook), pm_dlc.inc.c, UC record code (rec_try, rec_done, pm_gpu_rec_*), the flush_draw state path that templates do not use | build, grep for no dead references |

Functions to add in slice 1 (proposed names):
- `nat_tmpl_init()`: build template blocks from pm_comb and the flush state (pm_native.c).
- `nat_rec_put(const NRec*)`: producer write, inline in model.c under TARGET_3DS.
- `nat_frame_emit(int eyes)`: bucket pass and emission loop (pm_native.c, hot section).
- `nat_emit_draw(const NRec*, const u32* blk, const u16* patch, unsigned npatch)`: patch and append one draw.
- `nat_eye_copy(u32 span)`: eye 2 memcpy and palette patch.
- `nat_busy_probe()`: GPU busy probe (below).

Hot section: `.data.pmhot` (3dsx.ld.in:90) for nat_frame_emit, nat_emit_draw and nat_eye_copy. Target 4 to 8 KB of code [plan].

Data layout in slice 1 [I]:
- NRec array: 90 x 16 B = 1,440 B, 45 lines.
- Template blocks: 384 B each, aligned 32.
- Palette matrices: 64 B each (C3D_Mtx), aligned 32, two eyes.
- Index arena: u16 indices, linear [D plan_uc3.md section (a)].
- Command span: written in order, flushed once per split.

GPU busy-time probe (feasibility gate for 3D at 60 fps) [I]:
- HEAD has a wait counter: FS_GWAIT at pm_gpu.c:1918. C3D_FrameBegin with C3D_FRAME_SYNCDRAW (pm_gpu.c:1920) waits for the previous frame on the GPU. The `gwait` field is printed at pm_phase.c:225-226. The normal loop hides most of the wait behind CPU work (gwait 0.1 ms [D plan]).
- Probe, switch `gpubusy`: capture one frame's command buffer (or one pm_gpu_rec_play block). Each probe frame: replay that fixed buffer (pm_gpu_rec_play, pm_gpu.c:2522), call FrameEnd, then call FrameBegin with SYNCDRAW at once, with no game work in between. Time FrameBegin with svcGetSystemTick. The wait is the GPU busy time of that buffer. Take the median of 30 frames.
- Run twice: mono, and with two replays (one per target) for eyes = 2.
- Output line: `gpubusy f<n> median us <x> eyes <1|2>`.
- Azahar has no GPU timing model [plan]. The probe runs on the console only.
- Gate: 3D at 60 fps is possible only if 2 x per-eye busy time, plus CPU work, is under 16.7 ms.

---

## 7. Feasibility

CPU walk, 2D and 3D [I]:
- Plain models: native estimate 0.14 ms emission plus the producer share. The producer share is inside the list build (dl 4.4 ms for the whole frame [D]). Plain models at most about 4.6 ms against an 11 ms walk budget.
- All families: not yet shown. The dynamic families (sprites, HUD, text, effects) have no measured budget. Their per-draw emission is the same order (about 400 cycles). The walk total depends on their counts (census).
- 3D: the second walk is removed. The CPU cost of eye 2 is about 0.05 ms [I]. CPU is not the limit for 3D.

GPU [I]:
- GPU busy time has never been measured (section 0). The per-eye GPU time is not known from source. The probe in section 6 measures it.
- 60 fps in 3D is not established. It holds only if 2 x per-eye GPU busy time is under 16.7 ms, minus CPU work. If not, 60 fps in 3D is not reachable at any CPU cost.

Statement:
- 60 fps on the Old 3DS in 2D: possible on the CPU side, not yet shown. It needs the full walk under 11 ms. The dynamic families must confirm this. Audio spikes must stay under control (audio p95 11.8 ms in one phase window [D build3ds/hw_live_log_170.txt]).
- 60 fps on the Old 3DS in 3D: open. It needs the GPU busy probe (section 6) before later slices are judged.

---

## 8. Open items

- Census logger (step 2): per frame multiset of (template, texture id, triangle count), old path. Needed for the template count and the dynamic counts.
- Template count per scene: measured in step 2, not in this step.
- Register numbers of the sh_* setters (pm_gpu.c:531-566) and the 0x227 draw trigger: read from a captured record in step 2.
- Confirm 40 cycles per I-cache miss on the console. Section 2.3 is arithmetic on plan figures.
- RecKey size: 64 B (plan_uc3.md) or about 96 B (agent_uc3d.md). Settle before the record layout is final.
- Console time for the GPU probe (plan open item).
