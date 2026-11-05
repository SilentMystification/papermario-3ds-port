# plan_toad_model: model 00b7 and the model cost in Toad Town (mac_01)
Plan only. Rules: hw_guidelines.md. Console = Azahar x 2.86 (estimate). Tags: [C] code read, [M] measured (build3ds/lane-mops/last_log.txt f299, Azahar), [E] estimate.

## 1. What model 00b7 is (the user's question)
- 00b7 is MODEL_sign3, a signpost. It is not the roof. [C]
  - The census key is model->modelID, and modelID is the tree index (src/model.c:2636, `model->modelID = TreeIterPos`).
  - include/mapfs/mac_01_shape.h:23 has `#define MODEL_sign3 0xB7`. The map scripts use the same header, so the IDs agree.
- The spinning roof is MODEL_yane (0x39) and MODEL_off_yane (0x3C). The script is EVS_SpinRoof (src/world/area_mac/mac_01/rooms.c:178-191). It calls RotateGroup on both groups each frame, +3 degrees about Y. [C]
- The roof draws through render_transform_group (src/model.c:3225). That function writes the group G_MTX (line 3232), calls appendGfx_model for each child (line 3218), then writes POPMTX and PipeSync (lines 3238-3240). The roof vertices and child lists do not change. Only group->finalMtx changes. [C]
- The roof already replays a cached list with a new matrix:
  - pm_gpu_note_mvp puts the matrix in the triangle palette, so a new matrix does not close a draw (plan_model_ops 1).
  - dlc reports every map-shape group as static and replayed, with fail 0. A static group needs no CPU transform and no vertex copy (plan_draw_cost 1, item e). [M console]
  - Cost of the roof: about 2 extra ops (G_MTX and POPMTX) plus the child replays. appendGfx_model_group is 32 ops and 325 us per frame on Azahar [M], about 0.9 ms on the console [E]. No roof row pays.

## 2. Why 00b7 shows 10 draws and 373 ops
- The census gives every op to the last model mark (pm_gbi.c mdc_op, line 2148). Only appendGfx_model calls pm_gbi_mdl_mark (model.c:1427).
- So the ops of any producer that runs after a model and before the next mark go to that model. Examples: sprite_component (NPCs and the player), appendGfx_item_entity, group matrix ops.
- Arithmetic from the same window [M]:

| producer | ops per frame |
|---|---|
| sprite_component | 247 |
| appendGfx_item_entity (0x00225678, pm_3ds.map line 37021) | 69 |
| appendGfx_model_group | 32 |
| sum | 348 of 373 |

  - The rest is 25 ops. That is about the 7 to 25 ops of one model. So sign3 itself is a normal small model.
- The other values agree with this: binds 0 (sprites load textures without G_PM_TEX); static 0/263 (sprite and group matrices change each frame); 96 tris in 10 runs are sprite quads (map models replay through dlc, one op, so their tris do not show as tri ops).
- Result: no op in "00b7" can be removed as a model change. The real model ops are spread over about 107 models at about 7 ops each (732 ops).
- The census used a different view from the console:

| | Azahar mac01 scenario (post office) | console scene A |
|---|---|---|
| total ops per frame | 1165 | 2156 |
| appendGfx_model ops per frame | 755 | 1899 |

  - The tour10 view on Azahar gives 2045 ops (diag_ops2). Re-rank the models in a view that matches the console.

## 3. Per-model replay for the 80 static models
- This path exists already: pdl_try_m (3ds/src/pm_dlc.inc.c:1109) and the C5 templates (line 1156). In mac01, pdl_m reports "m hit 0 comp 0 replay 0 refuse 37". It refuses all 37 candidates. [M]
- Its replay calls pm_gbi_run on the same ops again (lines 1131-1143). With no refusals, it still removes only the dispatch.
- Ceiling of a new list per (model ID, matrix hash): dispatch only, 80 models x about 7 ops x about 2 us = about 1.1 ms console [E]. The bind body, the close and the dlc replay must still run, because the state between models changes (sprites, entities and custom gfx sit between models; plan_model_ops 4). Skipping the GPU state work is a raw GPUCMD block (DS5): high risk, up to 2.4 ms.
- Verdict: does not pay. Same risk class as the reverted retained draw.

## 4. Rows
| row | change | file, function | console ms saved, mac_01 | risk | Azahar check (azt mac01) | order |
|---|---|---|---|---|---|---|
| T0 | Fix the census. In execute_render_tasks, before each appendGfx call, mark pseudo-ID 0xF000 + kind: 1 render_transform_group, 2 appendGfx_model_group, 0xFF other. appendGfx_model marks itself at the same pointer, so its mark wins. After the loop, mark 0xF0FE (tail). TARGET_3DS guard, phase on only. | src/model.c execute_render_tasks (about line 4770); pm_gbi.c mdc_report (print IDs >= 0xF000 as "task") | 0 | none | 00b7 ops <= 30; 0xF0FF ops >= 300; tot ops 1165 unchanged; PPM byte-equal to build3ds/azt/shots/mac01/0000_1_2.ppm | 1 |
| T1 | Census in the console view. Add azt scenario mac01t: the tour10 warp and the camera position of scene A, until mdlsum. If the position cannot be copied, the orchestrator runs the T0 build on the console with phase on. | 3ds/tools/azt_scenarios.json | 0 | none | taskpl ops 1900 to 2200; mdlsum lines present; shots saved | 2 |
| T2 | Sprite census line: sprite draws, ops per draw, non-empty closes per sprite draw. Cold, noinline, phase only. | src/sprite.c spr_appendGfx_component (one TARGET_3DS mark); pm_gbi.c | 0 | none | line present 2 times; PPM byte-equal | 3 |
| T3 | Sprite path rows, only if T2 shows sprite closes >= 10 per frame: decide after T2. | src/sprite.c, pm_gbi.c | <= 1.8 total [E] | medium | PPM byte-equal on mac01 and yard | 4 |
| T4 | DS4 (TexBind record) and the bind body: about 55 us x 88 binds = 4.8 ms console. The largest per-model term that T0 to T1 cannot move. | pm_gbi.c tb_run | 1 to 3 [E] | medium | as in plan_draw_cost DS4 | 5 |

## 5. Rows that do not pay
- Any 00b7 row (merge its sub-draws, cache its vertex block, reorder inside it): 00b7 is a counting error (section 2).
- Roof matrix-only cache: dlc already replays it with a new palette matrix. Gain is about 0.
- Per-model packed list for the static models: dispatch only, <= 1.1 ms, high risk (section 3).
- Texture-pan vertex cache: pan 0 and fog 0 for all top models. Nothing to cache.
- Reorder inside a model to remove closes: not proved safe at equal Z (decals, alpha, zmark). <= 0.7 ms (plan_draw_cost d).
- Skip-if-equal state (R1 to R3): the repeat counts are below each gate (agent_modelops).

## 6. Hardware
- T0 and T2 marks are one store each, under pm_ph_on. The census code is cold and noinline (rules 1, 10).
- The mark table stays 1024 x 8 bytes, and only the census reads it. No division in hot code (rule 4).

## 7. First 3 rows for one Haiku code agent
1. T0 (about 15 lines in src/model.c, guarded, and 5 lines in pm_gbi.c). 2. T1 (scenario JSON only). 3. T2 (about 20 lines).
- One Azahar start per row. Revert a row that changes one picture byte.
- Projected mac_01 after T0 to T2: 60.6 ms, no change [E]. These rows are diagnostic.
- The 27 ms gap is not in one model. Its terms are about 88 models x (bind 55 us + close 101 us + dlc 67 us) = about 15 to 20 ms console [E], plus sprites <= 1.8 ms.
- Next work after T1: DS4/DS5 and plan_dl_architecture, ranked by the corrected per-model us.
