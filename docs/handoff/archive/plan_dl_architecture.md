# plan_dl_architecture: fixing the N64 display-list cost on 3DS (O3DS yard 58 ms)
(Opus plan, saved by the orchestrator. Tags: [M] measured, [C] read from code at HEAD 19b8d4a5a, [I] inference.)

## 1. Answer to the user's question
- The mismatch is real, but it is not mainly "GPU work done on the CPU". The N64 RDP ran fine-grained state ops in hardware at no cost: TMEM loads, tile descriptors, othermode bits, pipe syncs. Here every one of those ops goes through a general interpreter on a 268 MHz ARM11 with 16 KB L1I and no L2: about 2040 cycles per op [M diag_frame_budget, I].
- The output side already fits the hardware. The GV path transforms and lights on the GPU, TEV takes the combiner, VRAM holds the texture slab, the KICK1 split works [C review_render_pipeline §3]. The cost sits before the output: re-reading about 4545 to 6000 ops every frame at op granularity.
- The fix is to translate at the producer's level (a texture handle, a model, a sprite quad), not op by op, so most ops are never decoded. This also replaces address-keyed caching, which has failed three times (R3 +18 ms, D1 0 calls, M1 hitx 0 because the heap gen changes every frame [M diag_dlr]).

## 2. Facts used
- Walk 34.9 ms (60%). The tex streak is 14.9 ms of it: closes 7.2 ms (6.5 ms in 67 real closes at 97 us each), own work 6.7 ms, hit lookup 1.0 ms. 18.5 ms of the walk has no timer [M diag_frame_budget, diag_dc_console].
- Ops per frame by class (Azahar): main list 2626, class 2 TextureHeap 1647 (1553 state), class 3 render modes 605, class 7 987. Class 1 map geometry is already replayed by dlc: 68 lists, 1120 ops [M plan_dl_replay §1].
- Class 2 lists are the model texture setup. appendGfx_model emits `gSPDisplayList(textureHandle->gfx)` for each model, every frame [C src/model.c:1523,1527]. ModelRenderModes are class 3 [C model.c:1636].
- Producer hooks exist: FI_OPEN(task->appendGfx, arg) wraps every render task [C model.c:4837,4854]; FI_OPEN labels wrap each gfx_draw_frame stage [C main_loop.c:191-260]. Both reach pm_interp_open [C 3ds/src/pm_interp.c:59].
- W0 is done (2dfa2c3e1). W1 is not built.
- The dispatch prologue runs on every op: two streak calls, the gp_on/dd_on/pm_ph_fine tests and an 8-way compare. The G_VTX case inlines the pipelog, po3a and mtxlog blocks [C pm_gbi.c:2489-2514, 2590-2614]. Consistent with I-cache pressure.

## 3. Options (O3DS console ms; ranges [I] until P1/W1)
| Opt | What | Saves | Lines | Risk | Measure first |
|---|---|---|---|---|---|
| A | W1 to W6 (plan_walk_cost) | 3 to 10 (upper end only if W6 and W3 both hit) | about 250 | low | W1 opsplit |
| B | General cache: DL pointer + content gen, reuse converted vertices and state | 1 to 4 (class 2 hit ops 501/frame at 58.7%, hitx 0: ceiling about 2.5 ms; classes 4-6 need matrix-aware geometry replay, +0 to 2) | 300 to 600 | high (3 earlier attempts failed; one unread key field = silently wrong texture) | census of class 4-6 accept rate (D7 spec) |
| C | Native ops for hot producers: the producer emits one port op pointing to a pre-resolved record instead of GBI sequences | 4 to 12 (per producer = ops removed x about 6 us; model texture setup alone has about 1550 ops: 3 to 8 ms; a second producer 1 to 4) | 60 to 100 per producer | medium; one producer per row; key is producer identity, not heap address | P1 producer census |
| D | Walk on N3DS core 2, one frame of latency | O3DS 0; N3DS 3 to 6 | 300+ | high (races on the gfx pool, input latency) | N3DS phase split |
| E | Game-side cuts: cull off-screen actors before appendGfx; effects at a lower rate | culling 0 to 4; lower effect rate changes the picture (user call) | 30 to 80 | low for a conservative sphere cull | P1 field: ops of producers whose vertex loads are all GP1_OUT |
| F | Move more CPU work to the PICA200 | 0.5 to 2 | 50 to 150 | medium | docs/handoff/diag_cpu_gpu_work.md |

F, the user's suspicion (assumption, the inventory is pending): transform, lighting and texgen are already on the GPU on the GV path. Still on the CPU: the per-load AABB and GP1 class, the per-vertex transform for "cross" loads (xdup 44.6/frame [M console_walk_split]), and the CPU fallback for decals, z-mark and wide UI. Texture decode runs only on a miss (1.6/frame), a spike not a steady load; the PICA has no palette formats so CI textures are expanded on the CPU. The 28.8 us draw cost per close is citro3d emitting the command buffer. F is small unless the inventory finds steady per-vertex CPU work above 1 ms.

## 4. Recommendation: C steered by measurement, with the cheap A rows alongside
One code agent, rows in order. Each row 30 to 100 lines, one Azahar quantity plus the yard picture (Mario, textured ground, shadow), console push at the end of the epic.
1. W1 (about 50 lines, plan_walk_cost): sampled op-class time. Splits the 18.5 ms with no timer. Azahar: class sum within 15% of taskpl. Console: the `opsplit f` line.
2. P1 producer census (about 40 lines, pm_interp.c + pm_gbi.c, only under `phase`): at pm_interp_open store {key a, b, offset of gMainGfxPos in mainGfx}, up to 256 marks per frame (bg task gets one label "bg"). In pm_gbi_run at sp==0 move a cursor forward to the mark holding the top-level pc (marks in order, no search). Every op: `prod_ops[cur]++`; nested G_DL targets count for the calling producer. In the W1 1-in-64 sample add ticks to `prod_tk[cur]`. Extra field for E: ops of producers whose vertex loads were all GP1_OUT. Every 300 frames print `prod f` with the top 8 by ops (names from the 3dsx .map file). Azahar: sum of prod_ops equals taskpl ops +-1%. Console: the `prod f` line.
3. C1 native texture bind (about 90 lines): when a model texture loads (mdl_load_texture path) run the handle's gfx once through the interpreter and store the exit tile/TIMG/TLUT state plus the slot as a TexBind record on the TextureHandle. appendGfx_model emits one port op that F3DEX2 leaves unused (pick from gbi.h) with w1 = the TexBind record, in place of `gSPDisplayList(textureHandle->gfx)`. The handler applies the record and binds through ensure_tile. If the record is invalid (handle reloaded) it runs the original list (fallback for an invalid record, not a toggle). Azahar: class 2 ops per frame (dlsrc) down by 1000 or more. Console: walk down 2 ms or more. Extra checks: pause menu text and the file select flip.
4. W4 + W2 (about 60 lines): dispatch flag table and 1-in-8 phase instruments, only if the W1 gates hold. Azahar: main walk.
5. C2: the next producer from P1 (likely spr_appendGfx_component sprite quads, hud_element or msg glyphs). One native quad op: texture record, 4 vertices, matrix id. Replaces about 15 ops per quad. Azahar: that producer's prod_ops.
6. W6 / W3: only if W1 shows more than 5 us per op left, or gms above 2000.

## 5. Stop rule
- After P1: if no producer other than class 1 is at 800 ops per frame or more, drop C and run A in plan_walk_cost order.
- A C row that saves less than 1.5 ms on the console is reverted, and C stops.
- Two rows in a row under 1 ms on the console end the epic.
- A wrong picture (texture, palette, text) reverts the row. No third try on the same producer.
- Expected after W1, P1, C1, W4/W2 and C2: about 58 ms to 40 to 50 ms on O3DS [I]. 33.4 ms on O3DS is unlikely from any single option.

## 6. Decisions the user must make
1. Allow C to change game producers under src/ (model.c, sprite.c, hud_element.c). The port's src then differs from the decomp at those call sites. A and B stay inside 3ds/src but have a lower ceiling.
2. Choose the O3DS target: a stable 20 fps (50 ms pacing) is reachable. 30 fps likely needs C plus E. If 30 fps is needed only on N3DS, fund D after C.
3. E, part 2: lower-rate effects change the picture (flicker). Accept or reject. Culling alone keeps the picture the same.

Files: pm_gbi.c (pm_gbi_run 2449-2560: W1, P1 cursor, native op handler; ensure_tile), pm_interp.c (pm_interp_open 59: P1 marks), src/model.c (appendGfx_model 1369, texture DL emit 1523/1527, execute_render_tasks FI_OPEN 4837/4854), pm_dlc.inc.c (pdl_lazy_tile and pso capture: reuse for TexBind; dead pdl_try/pdl_try_m to delete later), src/main_loop.c (gfx_draw_frame FI_OPEN stages 191-260: producer labels).
