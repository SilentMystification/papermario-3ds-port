# Epic S6: entities, items, animators, shadows draw from records natively

## Goal
Slice S6 of the walker replacement, with the method that worked for map models (S3N) and sprites (S4): the producer writes a record, a debug check proves the native state equals the walker state at the same draw, the record draws natively, the old words go. Entity models are static display lists like map models, so reuse the native model path (nd_model.c: nd_model_state, the cache, nd_native_submit, the DLC2 static groups). Breadth first (user): a hard class becomes a named fallback, it does not stop the slice. This row file replaces plan_native_slices.md rows S6-1 to S6-4 (they used a force-include redirect).

## Start state
Branch nds1, main folder. Read docs/handoff/agent_s3n.md (STATE top, B4 to B10, the record layout and key list) and docs/handoff/plan_native_slices.md rows S6-1 to S6-4 for the anchors (entity_model.c G_PM_RM rewrite grep `rm_first`, draw_entity_model_B and _D with the vertex segment, Shadow.c gsSPPmTexInline, item_entity.c, animator.c appendGfx_animator).

## Rows (one commit each)
S6-1 RECORD + CHECK for entity models (src/entity_model.c appendGfx_entity_model, draw_entity_model_A to E, TARGET_3DS): one record per entity model draw (render mode list, texture, matrix, the display list and its segment base for B and D). Key the compiled list on (list, segment base) (plan risk R2: count NdObj per map). Debug switch `entcheck` (ndmcheck pattern). Gate: eq above 0, diff 0 at mac_01 (blocks, signs) and kmr_03 coin block scene (bench or a run with warp=kmr_03).
S6-2 SWITCH entity models native, delete the G_PM_RM rewrite in entity_model.c. Pictures right by eye.
S6-3 Shadows (src/entity/Shadow.c: delete the gsSPPmTexInline headers, plain words compile through the native list path) and item entities (src/item_entity.c). Pictures at mac_01 and a coin scene.
S6-4 Animators (src/animator.c: lists built per node for later become ranges, nd_range_*). Gate: battle_start picture (animated enemies) right by eye; named fallback if a node form is hard.

## Collision
Another code agent may work in this folder (row MENU2: pm_warp.c, pm_options*.c, pm_main.c, pm_input.c, menu functions of pm_gpu.c). Do not edit those. Keep edits buildable, commit quickly, pathspec commits only.

## Rules
`git status --short` before each edit and commit. Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. No toggles (debug switches allowed), no stubs, no git push, no console push, no attribution. Game source edits only inside `#ifdef TARGET_3DS`. No vertex shader change. Static memory add under 128 KB. Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S6 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e, never commit). Status line in 3ds/logs/perf/status_S6.txt before each Azahar start. Budget reset allowed. A design question: take the plan default, write it in docs/handoff/agent_s6.md, continue. Hand back after each row commit with the check line.
