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
- Answer: (open)

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
- Answer: (open)

### Q5. N64-only startup shims: keep as empty functions? (new, S20 items A4 and A5)
- Fact: `crash_screen_init`, `crash_screen_set_draw_info`, `is_debug_init`, `load_obfuscation_shims` are empty in the port. The real versions draw a crash screen into the N64 frame buffer, talk to an IS-Viewer cart, and write anti-tamper code. `pm_main.c` has its own crash handler.
- Options: (a) accept them as class C (platform, N64 only). They stay empty with a comment. (b) build the real files. `crash_screen.c` then needs a 3DS frame buffer path. `is_debug.c` and `load_obfuscation_shims.c` have no use on the 3DS.
- Recommendation: (a). The rule says ask before keeping a stub, so this is the ask.
- Answer: (open)

### Q6. Link more maps (hos_04 and others)? (from S08)
- Fact: the intro goes hos_05 to hos_04. `hos_04` is an id-only row, so the load is refused and the game goes to the title (same result as the real intro end, `state_intro.c` handles it). Only 6 maps are real rows (hos_05, kmr_02, kmr_20, osr_00, kkj_00, mac_00). Each real map costs about 45 KB (estimated). This also decides how long the stand-in B1 (`area_table.c`) and B2 (the `GotoMap` guard) stay.
- Options: (a) leave. (b) link hos_04 (8 files in `src/world/area_hos/hos_04`). (c) link all maps after S28 shows the real RAM limit. This removes B1 and B2.
- Recommendation: (a) now, (c) after S28 and Q2. If all maps fit, B1 and B2 go away.
- Answer: (open)

## Answered or resolved

- gSaveSlotMetadata storage in `pm_title.c` (from S08): RESOLVED by S17. The real `state_title_screen.c` is built and the port file is deleted.
- Pause PAL overlays (`pause_gfx_*`) not built (from S18): US build only. No action.
- Demo stand-ins wait for S19 (from S18): merged into Q1 and Q4.
- S18b demo scenes with unlinked maps: merged into Q4.
- S17 idle title timer: merged into Q4.
- M02 effect graphics, M02 ROM streaming: moved to Q3 and Q2.
- S08 hos_04: moved to Q6.

## V02 (not reproduced): where exactly does the wall cover the bushes?
Slice V02 found nothing in Azahar. Bushes draw in front of the hills in kmr_20 (yard, both camera positions), the hos_05 intro and mac_00. Please tell: the map, the moment (before or after the letter, intro or after the title), the camera angle, what is in front, and the device (Azahar, O3DS, N3DS). If it is hardware only, a photo helps.
