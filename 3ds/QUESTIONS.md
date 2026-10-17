# Questions for the user

Last tidied by slice S20. Answer by editing the "Answer" line or by telling the next agent. Order: most important first.

## Open questions (by importance)

### Q1. Battle tree: merge the whole tree, a smaller set, or wait? (from S19, S18b)
- Fact: the real battle tree (627 files, +3.9 MB image, 3dsx 6.59 MB) links and a battle plays. It sits on the local branch `s19-full-battle`, not on main. It is over the ceiling (3 MB, 150 files). Real `battle.c` names all 47 battle areas in `gBattleAreas`, so a smaller set needs a port-side table (a data stand-in like `area_table.c`).
- What a merge removes: 8 stand-in rows (A1, A2, A3, A13, A14, B3, B4, B5 in the S20 audit) and it enables S18b (real demo state).
- Options:
  - (a) Merge the whole tree. No new stand-in. +3.9 MB image. Risk on a real O3DS: the image counts against RAM. The M02 estimate (8.6 MB spare before the merge) gives about 4.7 MB spare after it, estimated, not measured.
  - (b) Allow a port-side `gBattleAreas` with chosen areas (kmr_part_1..3, mac, hos, kkj, dig: about 90 files, under 1 MB). It adds one data stand-in.
  - (c) Wait for a hardware run (S28) and then choose.
- Recommendation: (c) first, then (a). Run S28 (one push to the O3DS, read the first boot line). If the heap left is above 4 MB after the merge, take (a). It adds no stand-in. Take (b) only if the hardware run shows too little memory.
- Note for the merge: rebase first. `pm_title.c` was deleted on main, so the two debug hooks (`battle`, `demobattle`) need a new place.
- Answer: (user, 2026-10) (a), whole tree merged. Done in slice M03 (tag s19-pre-merge keeps the old branch tip).

### Q2. Stream the ROM from SD or keep it resident? (from M02)
- Fact: resident ROM is 26.9 MB. Estimated to fit the O3DS with 8.6 MB spare if the limit is 64 MB. Not measured on hardware (`push o` failed in M02). Streaming saves 26 MB but costs SD latency on the 1280 B BGM reads.
- Options: (a) stay resident, (b) stream from SD, (c) keep only hot ranges resident.
- Recommendation: (a) until the hardware run (S28) shows the real limit. Q1 depends on this answer.
- Answer: (open)

### Q3. Effect graphics: load them or drop them? (from M02, S20 item A17)
- Fact: `effects.cmake` sets every `effect_gfx_*` ROM start to NULL, so `sharedData->graphics` stays NULL. The data is 660 KB in the packed ROM. Effects that need it draw nothing or draw wrong. This is a data placeholder, not a code placeholder.
- Options: (a) load it (slice S24: use the real splat ranges, check `flame` render-to-texture), (b) drop the data and accept effects without graphics.
- Recommendation: (a). The rule is to remove stand-ins. Cost: 660 KB stays resident (already counted).
- Answer: (a) done in S24 (user asleep: the rule 'remove stand-ins' decides). Ranges are real, 660 KB is read when an effect loads.

### Q4. Idle title: demo timer (from S17, S18b)
- Fact: the real title has a 480 frame timer. If nobody presses A it enters GAME_MODE_DEMO or the intro. On main the demo states are empty stand-ins (A3), so an idle title ends in a black screen after 8 s. `autoa` and `fast` do not show it. With the branch, scene 1 of the demo battle plays. Scene 2 (`isk_04`) is not linked, so the demo goes black there. The full demo needs `nok_12`, `isk_04`, `tik_03`, `trd_09`, `jan_04` and more.
- Options: (a) leave until Q1 is merged, (b) after Q1, link the demo maps (cost: about 45 KB per map, estimated), (c) after Q1, leave the demo scenes that need unlinked maps. The real code then refuses the load and goes black.
- Recommendation: (a) now, then (b) for the demo maps only. A black screen after a refused load is a known fault (see `update_player_shadow` unmapped access).
- Update (S19b): the demo maps are linked (jan_04, kzn_02, plus kmr_22, kzn_19 and the Lava Piranha data). The first full demo loop runs (all 17 scenes by eye, 0 unmapped). The loop does not repeat: the intro after the demo hangs (row MS01, map .data not reset). Answer: (user) merge first, then link the demo maps: done.
- Answer: (user, 2026-10) merge first, then link the demo maps (b). Done in S19b.

### Q5. N64-only startup shims: keep as empty functions? (new, S20 items A4 and A5)
- Fact: `crash_screen_init`, `crash_screen_set_draw_info`, `is_debug_init`, `load_obfuscation_shims` are empty in the port. The real versions draw a crash screen into the N64 frame buffer, talk to an IS-Viewer cart, and write anti-tamper code. `pm_main.c` has its own crash handler.
- Options: (a) accept them as class C (platform, N64 only). They stay empty with a comment. (b) build the real files. `crash_screen.c` then needs a 3DS frame buffer path. `is_debug.c` and `load_obfuscation_shims.c` have no use on the 3DS.
- Recommendation: (a). The rule says ask before keeping a stub, so this is the ask.
- Answer: (open)

### Q6. Link more maps (hos_04 and others)? (from S08)
- Fact: the intro goes hos_05 to hos_04. `hos_04` is an id-only row, so the load is refused and the game goes to the title (same result as the real intro end, `state_intro.c` handles it). Only 6 maps are real rows (hos_05, kmr_02, kmr_20, osr_00, kkj_00, mac_00). Each real map costs about 45 KB (estimated). This also decides how long the stand-in B1 (`area_table.c`) and B2 (the `GotoMap` guard) stay.
- Options: (a) leave. (b) link hos_04 (8 files in `src/world/area_hos/hos_04`). (c) link all maps after S28 shows the real RAM limit. This removes B1 and B2.
- Recommendation: (a) now, (c) after S28 and Q2. If all maps fit, B1 and B2 go away.
- Answer: (user, S30) "scan and measure, I would like all maps to be linked". S30 linked 267 of 421 maps (ceiling reached, see Q7).

### Q7. How to link the last 154 maps (kpa, kzn, flo, jan, sam, pra, plus kmr_22, kzn_19, flo_11)? (from S30)
- Fact (measured in S30, Azahar inputs, O3DS values estimated): 267 maps linked, image 13.1 MB, estimated O3DS heap left 4.58 MB. All 418 compilable maps would give 15.1 MB and 2.6 MB left. The battle merge (Q1, +3.9 MB) is not counted: with 267 maps it leaves about 0.7 MB, with all maps it does not fit. kpa alone costs 340 KB. kmr_22 and kzn_19 need S19b (kzn model_anim data), flo_11 needs battle.c.
- Options: (a) stream the map data of rarely used areas (read it from the ROM on the first visit, needs a loader for the in-place map data, a big change). (b) shrink the resident ROM more (Q2, for example stream the ROM from SD, saves up to 26 MB). (c) accept less spare and link all (2.6 MB, no room for the battle tree). (d) stay at 267 and link the rest after the hardware run (S28) shows the real limit.
- Recommendation: (d) first. Then (b) if S28 shows less than 64 MB or if Q1 is merged.
- Update (M03, Azahar based estimate, not hardware): Q1 is merged. Image 17.19 MB, estimated O3DS spare about 865 KB (65,536 - 20,480 - 16,808 - 27,383 KB). Linking the 154 maps (+2.0 MB) gives about -1.1 MB: it does NOT fit without (a) or (b). The merge needs no map removed (spare is above 0.5 MB). Decision needed from the user: (b) stream the ROM from SD (Q2) or (a) stream map data, or stay at 267 maps.
- Update (S19b, Azahar based estimate, NOT hardware): 4 maps linked (kmr_22, kzn_19, jan_04, kzn_02) and 173 KB of Lava Piranha ROM data kept resident. Image 16,928 KB (+120 KB, ELF 17,316,388 B), heap used 27,554 KB (+171 KB), 3dsx 10,655,968 B. Estimated O3DS spare = 65,536 - 20,480 - 16,928 - 27,554 = 574 KB (was about 865 KB at the same tick). That is 74 KB above the 500 KB limit. Nothing more fits: one more map of about 45 KB is the last step. S30b (154 maps) is far out of reach without (a) or (b). Row MS01 (map .data restore) needs memory too and may not fit.
- Answer: (open)

## Answered or resolved

- gSaveSlotMetadata storage in `pm_title.c` (from S08): RESOLVED by S17. The real `state_title_screen.c` is built and the port file is deleted.
- Pause PAL overlays (`pause_gfx_*`) not built (from S18): US build only. No action.
- Demo stand-ins wait for S19 (from S18): merged into Q1 and Q4.
- S18b demo scenes with unlinked maps: merged into Q4.
- S17 idle title timer: merged into Q4.
- M02 effect graphics, M02 ROM streaming: moved to Q3 and Q2.
- S08 hos_04: moved to Q6.

## V02 (answered by slice V02, no user input needed)
Reproduced and fixed: the brown wall was a back-face-culled cliff skirt that the port drew (the port had no triangle culling). Please look at the kmr_20 yard on Azahar and on hardware. If a wall still shows anywhere else, give the map and the moment.
