# Slice queue

One slice per subagent. Format: `[ ] id | real decomp files | done check`.
Rule: build the real decomp file. Never write a stub. If a stub looks like the only option, write `BLOCKED: <reason>` in HANDOFF.md and stop.
Common done check: build passes, no new unresolved symbol, `fast autoa` run reaches kmr_20 with no unmapped-access warning, stub lines for replaced symbols are deleted from the port files.

[x] S01 | Measure only. Size and RAM of building all src/world/area_* for world.c (gAreas). Write the numbers to HANDOFF.md. No code change. | numbers recorded, decision stated (fits O3DS or not)
[x] E01 | Asset extraction. ROM: C:\Users\Andrew\AppData\Roaming\Azahar\sdmc\3ds\PaperMario\papermario.z64 (40 MB, copy to ver/us/baserom.z64, keep it out of git). Run the repo splat extraction (configure, tools/splat_ext, ver/us/splat.yaml) to produce assets, src/vtx, ver/us/build outputs the decomp needs (PNGs, vtx.inc.c, bins). Check ROM against ver/us/checksum.sha1. Make a repeatable script 3ds/tools/extract_assets.ps1 or .sh. Extracted files stay untracked (see tools/precommit_check_no_assets.sh). No hand-made data. | extraction script runs from clean, src/screen_overlays.c INCLUDE_IMG paths resolve, bed.c and records.c includes resolve
[x] S02 | src/screen_overlays.c replaces pm_overlay.c overlay functions, set_map_transition_effect, _render_transition_stencil, func_80138198 | common check, fade appears on map change
[ ] S03 | src/state_map_transitions.c replaces state_*_change_map and enter_world/game_over/enter_demo states in pm_title.c and pm_later.c | common check
[ ] S04 | src/item_entity.c and src/evt/item_api.c replace item entity stubs, AddKeyItem, DropItemEntity, ShowGotItem | common check
[ ] S05 | src/windows.c, src/hud_element.c replace window and hud stubs | common check
[ ] S06 | src/inventory.c, src/status_icons.c replace status bar, clear_player_data, is_ability_active, subtract_hp, merlee message stubs | common check
[ ] S07 | src/rumble.c, src/evt/virtual_entity.c, src/background.c, src/background_gfx.c replace their stubs | common check
[ ] S08 | src/world/world.c with gAreas and all area_* maps. Delete load_map_by_IDs and get_map_IDs_by_name. Depends on S01 decision. | common check, pipe in kmr_20 loads mac_00
[ ] S09 | src/world/partners.c and src/world/partner/*.c replace partner_* stubs, gPartnerAnimations, wPartnerNpc | common check
[ ] S10 | src/world/surfaces.c, src/world/menus.c, src/8a860_len_3f30.c replace floor, menu, popup stubs | common check
[ ] S11 | src/effects.c and src/effects/*.c replace pm_fx.c and all fx_* stubs | common check, intro effects draw
[ ] S12 | src/audio/*.c (bgm_control, ambience, sfx_control, snd_interface) with an ndsp backend. Ask user if a port-side synth is needed. | common check, music plays in Azahar
[ ] S13 | Extract assets from the ROM per repo tooling for bed.c, records.c, curtains.c, entity/Shadow.c, message box art. Build those real files. | files build, EVS_SetupBed and EVS_Inspect_Records come from decomp
[ ] S14 | src/evt/msg_api.c, src/draw_box.c, src/draw_img_util.c, src/speech_bubble.c, src/inspect_icon.c, src/i_spy.c, src/pulse_stone.c replace message and prompt stubs | common check, Parakarry text box shows
[ ] S15 | src/evt/demo_api.c, src/sprite_shading.c. Needs a port-side shader path (see HANDOFF issue 2). Ask user before any port-side code. | common check, no dark characters
[ ] S16 | Real hos_05 EVS_Starship_Depart and EVS_EnterStarship replace the hand-written scripts in pm_evt_stub.c | common check
[ ] S17 | src/state_file_select.c, src/state_world.c, src/fio.c replace state_step_file_select, state_init_world, fio stubs, plus real state_title_screen.c replaces title drawing in pm_title.c | common check, file select reached
[ ] S18 | src/state_pause.c, state_demo.c, state_game_over.c, state_language_select.c replace remaining state stubs in pm_later.c | common check
[ ] S19 | src/battle/battle.c and the battle tree replace load_battle, set_battle_*, reset_battle_status, state_*_battle, heap stubs | common check
[ ] S20 | Final audit: grep every port file for stubs and stand-ins. List what remains. Ask user about each remaining port shim. | list written to HANDOFF.md
