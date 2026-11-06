# agent_wcen: walker producer census after S3, S4, S6

## STATE
WCEN (row done, 2026-11-06): 5 scenes measured, one Azahar start each (benches wcen_mac01, wcen_mac00, wcen_sbk56, wcen_iwa10, wcen_battle; 5 of 8 starts). The walker still runs 8.3 to 15.3 ms gbi per frame (Azahar, phase on) and 439 to 1006 ops per frame at the spots. Top walker producers are the S3, S4 and S6 fallbacks (model.c, sprite.c, entity_model.c, item_entity.c, animator.c, npc.c) plus two unmapped buckets, pre and bg, which hold about 50 percent of the ops. The census prints 8 producers, not 15 (see Gaps). No code change.

## Method
- Census: pm_gbi.c os_report (P1), phase switch, one 300-frame window per run (scene frames 0 to 300; battle: battle from scene frame 100). Walker ops and sampled us per producer, printed as `prod f<n> <name> ops <n> us <n>`. us is the average per frame.
- Producer key: a label string when printable (FI_OPEN("label") in the source), else the address of a render task appendGfx callback (model.c:5187 and 5201, FI_OPEN(task->appendGfx, ...)). The address is resolved with build3ds/pm_3ds.map (build of 02:30, the 3dsx the runs used).
- Scenes: the spot positions of spot_mac01_pic, spot_mac00_pic, spot_sbk56_pic, spot_iwa10_pic; battle as battle_start. Scenarios wcen_* in azt_scenarios.json, benches wcen_* in bench_defs.json.
- Azahar timing, direction only (test_catalog B3). Not console numbers.
- Build: bench run starts build3ds/pm_3ds.3dsx (bench run has no lane option). The lane build wcen was made, not used by the runs.

## Slice owners (plan_native_slices.md)
S2 frame state (cam_main.c, background_gfx.c). S3 models (model.c). S4 sprites and NPCs (sprite.c, npc.c). S5 HUD and text. S6 entities, items, animators (entity_model.c, item_entity.c, animator.c). S7 world and battle. S8 effects (src/effects/*.c, effects.c). "none" = the file is in no slice file list.

## Scene tables (top 8 of the census; ops and us per frame)

### mac_01 (pos 0 0 200): 491 ops/frame, walker gbi 9049 us/frame
| # | producer | function | file:line | owner | ops | us |
|---|---|---|---|---|---|---|
| 1 | pre | none: ops before the first mark | main list | unmapped | 225 | 4504 |
| 2 | 0017b370 | appendGfx_model | src/model.c:1376 | S3 fallback | 131 | 3450 |
| 3 | sprite_component | spr_draw_component | src/sprite.c:499 | S4 fallback | 46 | 642 |
| 4 | bg | none: ops of a non-main list, no mark | - | unmapped | 39 | 258 |
| 5 | 00243a80 | appendGfx_item_entity | src/item_entity.c | S6 fallback | 32 | 225 |
| 6 | camera_render | render_frame | src/cam_main.c:118 | S2 | 8 | 34 |
| 7 | 001eb2e8 | appendGfx_npc | src/npc.c:835 | S4 fallback | 3 | 56 |
| 8 | 0017ef04 | appendGfx_model_group | src/model.c | S3 fallback | 2 | 179 |

### mac_00 (pos -135 0 85): 439 ops/frame, walker gbi 9459 us/frame
| # | producer | function | file:line | owner | ops | us |
|---|---|---|---|---|---|---|
| 1 | pre | none | main list | unmapped | 198 | 4454 |
| 2 | 0017b370 | appendGfx_model | src/model.c:1376 | S3 fallback | 176 | 5557 |
| 3 | bg | none | - | unmapped | 38 | 253 |
| 4 | 00243a80 | appendGfx_item_entity | src/item_entity.c | S6 fallback | 10 | 76 |
| 5 | camera_render | render_frame | src/cam_main.c:118 | S2 | 8 | 60 |
| 6 | sprite_component | spr_draw_component | src/sprite.c:499 | S4 fallback | 3 | 139 |
| 7 | 0017ef04 | appendGfx_model_group | src/model.c | S3 fallback | 3 | 160 |
| 8 | model_matrix | mdl_update_transform_matrices (mark only) | src/model.c:2991 | S3 | 0 | 1 |

### sbk_56 (pos 20 0 440): 627 ops/frame, walker gbi 8329 us/frame
| # | producer | function | file:line | owner | ops | us |
|---|---|---|---|---|---|---|
| 1 | pre | none | main list | unmapped | 293 | 4195 |
| 2 | 0022e428 | appendGfx_entity_model | src/entity_model.c:347 | S6 fallback | 164 | 1398 |
| 3 | bg | none | - | unmapped | 39 | 248 |
| 4 | 001dee10 | sun_appendGfx | src/effects/sun.c:157 | S8 | 28 | 450 |
| 5 | WorkerBackUI | render_workers_backUI | src/worker.c:181 | none | 22 | 294 |
| 6 | 001e2048 | walking_dust_appendGfx | src/effects/walking_dust.c:95 | S8 | 19 | 153 |
| 7 | 0017b370 | appendGfx_model | src/model.c:1376 | S3 fallback | 17 | 1251 |
| 8 | 001dc690 | stars_shimmer_appendGfx | src/effects/stars_shimmer.c:282 | S8 | 14 | 133 |

### iwa_10 (pos -586 1 530): 1006 ops/frame, walker gbi 15327 us/frame
| # | producer | function | file:line | owner | ops | us |
|---|---|---|---|---|---|---|
| 1 | pre | none | main list | unmapped | 481 | 9196 |
| 2 | 0022e428 | appendGfx_entity_model | src/entity_model.c:347 | S6 fallback | 240 | 2229 |
| 3 | sprite_component | spr_draw_component | src/sprite.c:499 | S4 fallback | 121 | 1918 |
| 4 | 0017b370 | appendGfx_model | src/model.c:1376 | S3 fallback | 84 | 7596 |
| 5 | bg | none | - | unmapped | 38 | 227 |
| 6 | WorkerBackUI | render_workers_backUI | src/worker.c:181 | none | 11 | 134 |
| 7 | 0017ef04 | appendGfx_model_group | src/model.c | S3 fallback | 9 | 1747 |
| 8 | camera_render | render_frame | src/cam_main.c:118 | S2 | 8 | 48 |

Native draws at iwa_10 (same run): ndm f300 native 15224, fb 2862 (S3 model draws); entnat f300 draws 5545, native 4788, fb 757 (S6). The other scenes did not log these lines.

### battle (kmr_03, battle from scene frame 100; window 0 to 300): 698 ops/frame, walker gbi 11428 us/frame
| # | producer | function | file:line | owner | ops | us |
|---|---|---|---|---|---|---|
| 1 | pre | none | main list | unmapped | 328 | 5486 |
| 2 | 0022e428 | appendGfx_entity_model | src/entity_model.c:347 | S6 fallback | 110 | 1718 |
| 3 | 00243a80 | appendGfx_item_entity | src/item_entity.c | S6 fallback | 75 | 452 |
| 4 | 0017b370 | appendGfx_model | src/model.c:1376 | S3 fallback | 38 | 2420 |
| 5 | 00231358 | appendGfx_animator | src/animator.c:779 | S6 fallback | 37 | 820 |
| 6 | bg | none | - | unmapped | 37 | 3069 |
| 7 | effect_renderUI | effect UI render | src/effects.c:165 | S8 | 17 | 303 |
| 8 | WorkerBackUI | render_workers_backUI | src/worker.c:181 | none | 16 | 190 |

Per scene totals (walker ops per frame; walker gbi us per frame = sum of the opsplit class column): mac_01 491 / 9049; mac_00 439 / 9459; sbk_56 627 / 8329; iwa_10 1006 / 15327; battle 698 / 11428. The top 8 hold 94 to 99 percent of the ops at each scene. Walker share of total gbi time is not measured by this census.

## Findings
1. The largest bucket is "pre" (45 to 48 percent of the ops at every scene). It is not a producer name. The mark cursor moves only at sp == 0 in the main list, so this bucket is the ops before the cursor reaches a mark. Its real producer is not known from this census.
2. Mapped walker producers still on the walker (fallbacks): model.c (appendGfx_model, appendGfx_model_group, model_matrix) S3; entity_model.c, item_entity.c, animator.c S6; sprite.c and npc.c S4.
3. Not in any slice file list: worker.c render_workers_backUI (WorkerBackUI, 11 to 22 ops per frame). It needs a slice (S5 or S9) in the plan.
4. S8 effects still on the walker: sun, walking_dust, stars_shimmer, effect_renderUI (14 to 28 ops per frame each).
5. camera_render (cam_main.c render_frame, S2): 8 ops per frame at every scene.

## Gaps (census limits; no code change in WCEN)
- The census prints 8 producers per window (pm_gbi.c os_report, loop `r < 8`), not 15. Printing 15 is a one-line change in pm_gbi.c (not done here).
- "pre" and "bg" have no label. Marks at the start of the main list and at each non-main list would name them. Code change (pm_gbi.c and the gfx entry in main_loop.c), not done here.
- One 300-frame window per run. The battle window holds 200 battle frames plus the battle start.
- The mac_01 log file was overwritten by a later run and not kept. Its numbers come from the run output and its opsplit and pos lines, read before the overwrite.
- The shots (build3ds/azt/shots/wcen_*/) were not viewed by this agent. The subject lines (pos, warp, battle formation) are in the logs.
