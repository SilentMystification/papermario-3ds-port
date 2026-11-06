# agent_wcen: walker producer census after S3, S4, S6

## STATE
WCEN (2026-11-06, a7efce2c1): 5 scenes, walker 8.3 to 15.3 ms gbi per frame (Azahar, direction only).
WCEN2 (b74996266): top 15, rec and nat, non-main list keys, frame-start mark.
WCEN3: the mark table belongs to the list walked (per-list copy; a frame start drops its context's old marks). pre is 0 ops at mac_01 (gate below 30: pass). Adds wd (walker GPU draws) and nd (native GPU draws) per producer. Tables for mac_01 and iwa_10 are WCEN3; other scenes are WCEN (old labels, pre unresolved there).
Walker work is wd, not ops: the iwa_10 model producer has 1790 walker draws (about 6 per frame) against 16359 native draws.

## Method
- Census: pm_gbi.c os_report (P1), phase switch, one 300-frame window per run (scene frames 0 to 300; battle: battle from scene frame 100). Walker ops and sampled us per producer, printed as `prod f<n> <name> ops <n> us <n>`. us is the average per frame.
- Producer key: a label string when printable (FI_OPEN("label") in the source), else the address of a render task appendGfx callback (model.c:5187 and 5201, FI_OPEN(task->appendGfx, ...)). The address is resolved with build3ds/pm_3ds.map (build of 02:30, the 3dsx the runs used).
- Scenes: the spot positions of spot_mac01_pic, spot_mac00_pic, spot_sbk56_pic, spot_iwa10_pic; battle as battle_start. Scenarios wcen_* in azt_scenarios.json, benches wcen_* in bench_defs.json.
- Azahar timing, direction only (test_catalog B3). Not console numbers.
- Build: bench run starts build3ds/pm_3ds.3dsx (bench run has no lane option). The lane build wcen was made, not used by the runs.

## Slice owners (plan_native_slices.md)
S2 frame state (cam_main.c, background_gfx.c). S3 models (model.c). S4 sprites and NPCs (sprite.c, npc.c). S5 HUD and text. S6 entities, items, animators (entity_model.c, item_entity.c, animator.c). S7 world and battle. S8 effects (src/effects/*.c, effects.c). "none" = the file is in no slice file list.

## Scene tables (ops and us per frame; WCEN top 8, WCEN2 top 15 with rec and nat)

### mac_01 (pos 0 0 200), WCEN3 (lane wcen, window 0 to 300): 492 ops/frame
Columns: ops and us per frame. rec = record headers run, nat = records drawn natively (per frame). wd = walker GPU draws, nd = native GPU draws, both window totals (300 frames).
| # | producer | function | file | owner | ops | us | rec | nat | wd | nd |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 0022f108 | appendGfx_entity_model | src/entity_model.c:347 | S6 fallback | 163 | 1813 | 15 | 6 | 1 | 4251 |
| 2 | 0017c050 | appendGfx_model | src/model.c:1376 | S3 fallback | 84 | 4488 | 84 | 84 | 1 | 15080 |
| 3 | 00244760 | appendGfx_item_entity | src/item_entity.c | S6 fallback | 64 | 437 | 2 | 2 | 0 | 578 |
| 4 | WorkerBackUI | render_workers_backUI | src/worker.c:181 | none | 43 | 573 | 0 | 0 | 96 | 111 |
| 5 | sprite_component | spr_draw_component | src/sprite.c:499 | S4 fallback | 40 | 963 | 5 | 5 | 0 | 2313 |
| 6 | 015fe3d8 | non-main list, D_80164000+0x124460 | background list (inferred) | S2 | 19 | 89 | 0 | 0 | 145 | 0 |
| 7 | 0155a1a8 | non-main list, D_80164000+0x80230 | background list (inferred) | S2 | 19 | 147 | 0 | 0 | 144 | 0 |
| 8 | 001e2d28 | walking_dust_appendGfx | src/effects/walking_dust.c:95 | S8 | 17 | 143 | 0 | 0 | 126 | 96 |
| 9 | camera_render | render_frame | src/cam_main.c:145 | S2 | 16 | 92 | 0 | 0 | 289 | 0 |
| 10 | gfx_window_root | window root mark | src/main_loop.c | S9 | 7 | 32 | 0 | 0 | 0 | 0 |
| 11 | 0017fbe4 | appendGfx_model_group | src/model.c | S3 fallback | 5 | 336 | 5 | 5 | 0 | 1729 |
| 12 | WorkerFrontUI | render_workers_frontUI | src/worker.c | none | 4 | 21 | 0 | 0 | 0 | 0 |
| 13 | gfx_game_mode_front_ui | game mode front UI mark | src/main_loop.c:303 | S9 | 0 | 1 | 0 | 0 | 21 | 0 |
| 14 | model_matrix | mdl_update_transform_matrices (mark only) | src/model.c:2991 | S3 | 0 | 2 | 0 | 0 | 0 | 0 |
| 15 | camera_persp | camera mark | src/cam_main.c | S2 | 0 | 7 | 0 | 0 | 0 | 0 |

The pre bucket is 0 ops at this scene (gate below 30 ops: pass). Sum of printed ops is 481 of 492 (per-row integer division).

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

### iwa_10 (pos -586 1 530), WCEN3 (lane wcen, window 0 to 300): 1006 ops/frame
| # | producer | function | file | owner | ops | us | rec | nat | wd | nd |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 0022f108 | appendGfx_entity_model | src/entity_model.c:347 | S6 fallback | 618 | 5825 | 41 | 17 | 928 | 9435 |
| 2 | 0017c050 | appendGfx_model | src/model.c:1376 | S3 fallback | 160 | 16487 | 65 | 56 | 1790 | 16359 |
| 3 | sprite_component | spr_draw_component | src/sprite.c:499 | S4 fallback | 66 | 1686 | 9 | 9 | 171 | 4225 |
| 4 | WorkerBackUI | render_workers_backUI | src/worker.c:181 | none | 43 | 557 | 0 | 0 | 2 | 159 |
| 5 | 001dd370 | stars_shimmer_appendGfx | src/effects/stars_shimmer.c:282 | S8 | 28 | 352 | 0 | 0 | 49 | 155 |
| 6 | 0017fbe4 | appendGfx_model_group | src/model.c | S3 fallback | 19 | 3417 | 1 | 0 | 0 | 264 |
| 7 | 0155a1a8 | non-main list, D_80164000+0x80230 | background list (inferred) | S2 | 18 | 108 | 0 | 0 | 142 | 0 |
| 8 | 015fe3d8 | non-main list, D_80164000+0x124460 | background list (inferred) | S2 | 18 | 109 | 0 | 0 | 141 | 0 |
| 9 | camera_render | render_frame | src/cam_main.c:145 | S2 | 16 | 90 | 0 | 0 | 283 | 0 |
| 10 | gfx_window_root | window root mark | src/main_loop.c | S9 | 7 | 28 | 0 | 0 | 5 | 5 |
| 11 | WorkerFrontUI | render_workers_frontUI | src/worker.c | none | 3 | 106 | 0 | 0 | 0 | 0 |
| 12 | gfx_game_mode_front_ui | game mode front UI mark | src/main_loop.c:303 | S9 | 0 | 1 | 0 | 0 | 11 | 0 |
| 13 | model_matrix | mdl_update_transform_matrices (mark only) | src/model.c:2991 | S3 | 0 | 2 | 0 | 0 | 0 | 0 |
| 14 | camera_persp | camera mark | src/cam_main.c | S2 | 0 | 7 | 0 | 0 | 0 | 0 |
| 15 | gfx_draw_frame | frame start mark | src/main_loop.c:208 | S2 | 0 | 4 | 0 | 0 | 0 | 0 |

Top 8 by walker draws (wd, window totals), mac_01: camera_render 289; 015fe3d8 145; 0155a1a8 144; walking_dust 126; WorkerBackUI 96; gfx_game_mode_front_ui 21; entity_model 1 and appendGfx_model 1 (tie).
Top 8 by walker draws, iwa_10: appendGfx_model 1790; entity_model 928; camera_render 283; sprite_component 171; 0155a1a8 142; 015fe3d8 141; stars_shimmer 49; gfx_game_mode_front_ui 11.

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
1. WCEN (old labels) had "pre" at 45 to 48 percent of ops. That was the mark timing: the mark table was cleared before the walk. WCEN3 fixes it: pre is 0 ops at mac_01 and absent from the top 15 at iwa_10. The WCEN scene tables above keep the old labels.
2. Mapped walker producers still on the walker (fallbacks): model.c (appendGfx_model, appendGfx_model_group, model_matrix) S3; entity_model.c, item_entity.c, animator.c S6; sprite.c and npc.c S4.
3. Not in any slice file list: worker.c render_workers_backUI (WorkerBackUI, 2 to 96 walker draws in the window). It needs a slice (S5 or S9) in the plan.
4. S8 effects still on the walker: sun, walking_dust, stars_shimmer, effect_renderUI (14 to 28 ops per frame each).
5. camera_render (cam_main.c render_frame, S2): 8 ops per frame at every scene.

## Gaps
- WCEN3 table of mac_01 and iwa_10 only. Other scenes not re-run (rule: two scenes per row).
- wd and nd are window totals (300 frames), not per frame. ops and us are per frame.
- Non-main lists keyed by address in D_80164000 (display context array): two background lists, one per context (inferred from gfx_task_background).
- rec and nat count headers and native draws only. Ops skipped by a native record are not visited by the walker, so they are not in ops.
- Producer attribution of a walker draw is the op under the cursor when the draw is flushed. A batch flushed at a later op goes to that op's producer.
- Shots (build3ds/azt/shots/wcen*_) not viewed by this agent. Subject lines (pos, warp) are in the logs.
