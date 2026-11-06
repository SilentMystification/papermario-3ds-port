# Rows S6-2 to S6-4: entity models draw natively (texture ops in the list, vertex segments)

## Start state
Branch nds1, main folder. S6-1 done (ca6633a4c: G_PM_ENT record, entcheck eq 2041 / 4302 diff 0 at mac_01 / kmr_03). Census (6266da774, entcls): every entity draw is class tex (texture load words inside the model list) or vtxseg (B and D draws with a vertex segment); static 0. Read docs/handoff/agent_s6.md (STATE, census), docs/handoff/row_S6.md (rows and gates), docs/handoff/agent_s3n.md (B4 handle desc, B9 list ops with nd_words).

## Decisions (plan defaults, orchestrator)
- tex class: plan_native_system.md 2.9: the static list compiler (nd_dlc) handles the texture ops of the list: SETTIMG, SETTILE, SETTILESIZE, LOADBLOCK, LOADTILE, LOADTLUT. Decode them once per compiled list into a texture desc per segment (the B4 NdTexDesc form; nd_words over the ops gives timg, tile words, tsz, TLUT), keyed with the segment. At draw: nd_tex_get(desc) and the draw-time unit words. Put the decode in 3ds/src/nd_dlc.c and nd_model.c. Do NOT edit 3ds/src/nd_tex.c or pm_tex.c (row TEXSTALE works there now); call only their existing functions.
- vtxseg class: plan risk R2: compile per (list, vertex segment base) when the vertex array of that segment does not change between frames (check: hash the segment's vertex bytes on 2 frames; equal = static). Count NdObj per map in the entcls line. A segment whose vertices change per frame stays on the walker (named fallback `ent_dynvtx`).

## Rows (one commit each)
S6-2a tex decode in nd_dlc + entcheck compare of the unit words: gate entcheck diff 0, eq above 0, the tex class count drops to native-ready.
S6-2b vtxseg static check and compile per (list, seg base): gate entcheck diff 0, NdObj per map logged.
S6-2c switch: entity records draw natively (pm_gbi.c record arm, the nd_model draw path), refused draws run the old words; delete the G_PM_RM rewrite in entity_model.c (grep `rm_first`). Pictures right by eye at mac_01 and kmr_03.
S6-3 shadows (src/entity/Shadow.c gsSPPmTexInline headers, plain words through the compiler) and item entities (src/item_entity.c). Pictures at mac_01 and a coin scene.
S6-4 animators (src/animator.c, lists built per node become ranges). battle_start picture right by eye; a hard node form is a named fallback.

## Collision and rules
Other agents in this folder: TEXSTALE (pm_tex.c, nd_tex.c), MENU4 (pm_options*.c, pm_warp.c, pm_main.c, pm_input.c, menu functions of pm_gpu.c). Do not edit their files. Keep edits buildable, commit quickly, pathspec commits, own hunks only; JSON files stay valid. If git says index.lock exists, wait 10 s and retry. Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S6 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e, never commit). Status line in 3ds/logs/perf/status_S6.txt before each start. The budget never stops a row: reset and continue. Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. No toggles (debug switches allowed), no stubs, no git push, no console push, no attribution. Game source only inside `#ifdef TARGET_3DS`. Breadth first: a hard class is a named fallback in agent_s6.md. Update agent_s6.md STATE after each commit. Hand back only after S6-4.
