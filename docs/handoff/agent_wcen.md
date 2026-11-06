# agent_wcen: walker producer census after S3, S4, S6

## STATE
WCEN (2026-11-06, commit a7efce2c1): 5 scenes, walker still 8.3 to 15.3 ms gbi per frame (Azahar, direction only), 439 to 1006 ops per frame. Top fallbacks: model.c (S3), entity_model.c, item_entity.c (S6), npc.c, sprite.c (S4).
WCEN2 (census code, pm_gbi.c and main_loop.c mark): top 15 printed; per producer rec and nat columns (native records vs walker work); non-main lists keyed by address. mac_01 and iwa_10 tables below are WCEN2. The other scene tables are WCEN (old labels: one bg slot).
OPEN: "pre" (45 to 48 percent of ops) is not resolved. The gfx_draw_frame mark gets 0 ops; the mark table is empty at walk start in mac_01 and holds 254 marks in iwa_10 (walk starts at mainGfx[0] in both). Marks and walk appear to be out of step. Next: keep the marks with the list until its walk ends (code change, not made here).

## Method
- Census: pm_gbi.c os_report (P1), phase switch, one 300-frame window per run (scene frames 0 to 300; battle: battle from scene frame 100). Walker ops and sampled us per producer, printed as `prod f<n> <name> ops <n> us <n>`. us is the average per frame.
- Producer key: a label string when printable (FI_OPEN("label") in the source), else the address of a render task appendGfx callback (model.c:5187 and 5201, FI_OPEN(task->appendGfx, ...)). The address is resolved with build3ds/pm_3ds.map (build of 02:30, the 3dsx the runs used).
- Scenes: the spot positions of spot_mac01_pic, spot_mac00_pic, spot_sbk56_pic, spot_iwa10_pic; battle as battle_start. Scenarios wcen_* in azt_scenarios.json, benches wcen_* in bench_defs.json.
- Azahar timing, direction only (test_catalog B3). Not console numbers.
- Build: bench run starts build3ds/pm_3ds.3dsx (bench run has no lane option). The lane build wcen was made, not used by the runs.

## Slice owners (plan_native_slices.md)
S2 frame state (cam_main.c, background_gfx.c). S3 models (model.c). S4 sprites and NPCs (sprite.c, npc.c). S5 HUD and text. S6 entities, items, animators (entity_model.c, item_entity.c, animator.c). S7 world and battle. S8 effects (src/effects/*.c, effects.c). "none" = the file is in no slice file list.

## Scene tables (ops and us per frame; WCEN top 8, WCEN2 top 15 with rec and nat)

### mac_01 (pos 0 0 200), WCEN2 (lane wcen, WCEN2 build, window 0 to 300): 492 ops/frame
Columns: ops per frame, us per frame, rec = record headers run (G_PM_REC, G_PM_SPR, G_PM_ENT), nat = records drawn natively, drew = ops - rec (walker work).
| # | producer | function | file | owner | ops | us | rec | nat | drew |
|---|---|---|---|---|---|---|---|---|---|
| 1 | pre | unresolved (see Gaps) | main list | unmapped | 225 | 4500 | 57 | 52 | 168 |
| 2 | 0017be58 | appendGfx_model | src/model.c | S3 fallback | 136 | 3547 | 49 | 46 | 87 |
| 3 | 001ebdd0 | appendGfx_npc | src/npc.c:835 | S4 fallback | 41 | 444 | 3 | 2 | 38 |
| 4 | 00244568 | appendGfx_item_entity | src/item_entity.c | S6 fallback | 32 | 224 | 1 | 1 | 31 |
| 5 | 015fa3d8 | non-main list, D_80164000+0x124460 | background list (inferred) | S2 | 19 | 98 | 0 | 0 | 19 |
| 6 | 015561a8 | non-main list, D_80164000+0x80230 | background list (inferred) | S2 | 19 | 147 | 0 | 0 | 19 |
| 7 | camera_render | render_frame | src/cam_main.c:145 | S2 | 8 | 34 | 0 | 0 | 8 |
| 8 | sprite_component | spr_draw_component | src/sprite.c:499 | S4 fallback | 4 | 174 | 0 | 0 | 4 |
| 9 | 0017f9ec | appendGfx_model_group | src/model.c | S3 fallback | 2 | 180 | 2 | 2 | 0 |
| 10 | model_matrix | mdl_update_transform_matrices (mark only) | src/model.c:2991 | S3 | 0 | 1 | 0 | 0 | 0 |
| 11 | camera_persp | camera mark, 0 ops | src/cam_main.c | S2 | 0 | 0 | 0 | 0 | 0 |
| 12 | gfx_draw_frame | frame start mark, 0 ops | src/main_loop.c:208 | S2 | 0 | 2 | 0 | 0 | 0 |
| 13 | 01250ea8 | data address key (pm_tex_epoch_v), 0 ops | none | none | 0 | 12 | 0 | 0 | 0 |

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

### iwa_10 (pos -586 1 530), WCEN2 (lane wcen, window 0 to 300): 1006 ops/frame
| # | producer | function | file | owner | ops | us | rec | nat | drew |
|---|---|---|---|---|---|---|---|---|---|
| 1 | pre | unresolved (see Gaps) | main list | unmapped | 480 | 9287 | 58 | 40 | 422 |
| 2 | 0022ef10 | appendGfx_entity_model | src/entity_model.c:347 | S6 fallback | 240 | 2291 | 14 | 6 | 226 |
| 3 | 001ebdd0 | appendGfx_npc | src/npc.c:835 | S4 fallback | 93 | 1168 | 8 | 4 | 85 |
| 4 | 0017be58 | appendGfx_model | src/model.c | S3 fallback | 84 | 7628 | 33 | 30 | 51 |
| 5 | sprite_component | spr_draw_component | src/sprite.c:499 | S4 fallback | 27 | 710 | 3 | 3 | 24 |
| 6 | 015fa3d8 | non-main list, D_80164000+0x124460 | background list (inferred) | S2 | 18 | 109 | 0 | 0 | 18 |
| 7 | 015561a8 | non-main list, D_80164000+0x80230 | background list (inferred) | S2 | 18 | 107 | 0 | 0 | 18 |
| 8 | WorkerBackUI | render_workers_backUI | src/worker.c:181 | none | 11 | 134 | 0 | 0 | 11 |
| 9 | 0017f9ec | appendGfx_model_group | src/model.c | S3 fallback | 9 | 1748 | 0 | 0 | 9 |
| 10 | camera_render | render_frame | src/cam_main.c:145 | S2 | 8 | 48 | 0 | 0 | 8 |
| 11 | 001dd178 | stars_shimmer_appendGfx | src/effects/stars_shimmer.c:282 | S8 | 7 | 168 | 0 | 0 | 7 |
| 12 | gfx_window_root | window root mark | src/main_loop.c | S9 | 1 | 6 | 0 | 0 | 1 |
| 13 | WorkerFrontUI | render_workers_frontUI | src/worker.c | none | 0 | 3 | 0 | 0 | 0 |
| 14 | model_matrix | mdl_update_transform_matrices (mark only) | src/model.c:2991 | S3 | 0 | 1 | 0 | 0 | 0 |
| 15 | camera_persp | camera mark, 0 ops | src/cam_main.c | S2 | 0 | 3 | 0 | 0 | 0 |

Native draws at iwa_10 (WCEN run, old build): ndm f300 native 15224, fb 2862 (S3); entnat f300 draws 5545, native 4788, fb 757 (S6).

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

## Gaps
- "pre" (see STATE): unlabelled. Diag line `pre f<n> walk0 <ops> mk0 <ops> marks <n>` in the census log (pm_gbi.c os_report).
- Non-main lists: keys 015fa3d8 and 015561a8 are addresses in D_80164000 (the display context array, offsets 0x124460 and 0x80230 in the lane map). Two background lists, one per display context (inferred from gfx_task_background; not checked against the struct).
- rec and nat count headers and native draws only. The ops that a native record skips are not visited by the walker, so they are not in ops (not counted, not measured).
- One 300-frame window per run. The battle window holds 200 battle frames plus the battle start.
- Shots (build3ds/azt/shots/wcen_*/ and wcen2_*) were not viewed by this agent. The subject lines (pos, warp) are in the logs.
