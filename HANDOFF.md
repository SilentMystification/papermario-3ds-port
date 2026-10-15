# Handoff

When a feature or slice is finished, stop and tell the user. They start a new agent from this file. Do not keep working in the same chat.

A commit is not automatically a handoff. Each commit, decide whether the slice is done enough for a new agent. If it is, or if a handoff looks like the right next step for any other reason, bring it up and wait.

Replace this file. Do not append a history. Keep it short.

Plan: [Cutscene systems plan](file:///C:/Users/Andrew/.cursor/plans/cutscene_systems_plan_2097aefa.plan.md). Scene setup, actors, and Bowser are done. Effects are still stand-in quads. Audio is pending. Cook ROM assets only if a measured frame budget is missed.

## Now

Paper Mario O3DS port. The intro (`hos_05` entry 3) still plays in Azahar. The real `kmr_20_EVS_Main` now runs after the title (not committed).

Done this slice:
- Real code compiled: `collision.c`, `trigger.c`, `entity.c`, `entity_model.c`, `animator.c`, `vars_access.c` (real flags and bytes in `gCurrentSaveFile`), `7B440.c`, `7BB60.c`, `7E9D0.c`, `world/actions.c`, `world/action/*.c`, `evt/player_api.c`, `world/script_api/rooms.c`, the kmr_20 map files except `bed.c` and `records.c`, and some `entity/*.c` blocks and springs.
- `pm_map.c`: loads `<map>_hit` (byte-swaps the big-endian hit file, then real `load_hit_data`). `get_current_map_settings` returns `pm_settings`. `gAreas[area].maps[map].settings` points at it so `reset_player_status` reads the entry list. Calls `player_reset_data` for non-intro maps.
- `pm_title.c` `state_step_world` now calls `update_encounters`, `update_npcs`, `update_player`, `update_effects`, `iterate_models`, `update_cameras`.
- Generated headers: `3ds/tools/gen_msg_ids.py` (real message IDs), `gen_effect_ids.py` (real effect IDs), `gen_entity_rom.py` (entity model ROM offsets). `dedup_stubs.py` removes a port stub after the real file defines it (pipe the link log in).
- `src/entity.c` has `TARGET_3DS` branches: heap buffers, and animated meshes use the compiled C node data instead of the big-endian ROM copy.
- Debug switch `scripts` logs every live script each 120 frames (`pm_dump_scripts` in `pm_map.c`).

Result: with `fast autoa`, kmr_20 draws the house exterior from the script camera. `MakeNpcs` and `MakeEntities` finish. The begin-game scene scripts run (Parakarry `_npc_jump_to`).

Still stubs or missing: audio (all `SetMusic`, `PlaySound`, etc. are skip stubs in `pm_evt_stub.c`), item and popup APIs (`pm_kmr_stubs.c`), `fx_*` return 0 (`pm_later.c`), entity display lists and shadows draw nothing (`Entity_*_Render` are empty, `pm_shadow.c`), `bed.c` and `records.c` (need extracted textures), message boxes, status bar, HUD.

Faults still logged in Azahar (would fault on hardware): `spr_draw_player_sprite` about 29x (the player sprite is not set up), `update_player_shadow`, a few more under `appendGfx_player`.

Debug switches in `debug3ds.txt` or `-GameDebug`: `shots` saves a frame every 30 frames to `sdmc/3ds/PaperMario/shots/*.ppm` (240x400, rotate 90 degrees; `build3ds/sheet.py` makes contact sheets). Do not combine `shots` with `autoa` on the intro: A skips it. `3ds/run_azahar.ps1` deletes `debug3ds.txt` at the end of a run.

## Next

Boot a file and progress instead of sitting on the first frame. The port still fakes the file path: `pm_title.c` `state_step_file_select` jumps straight to kmr_20 entry 0 and `load_map_by_IDs` in `pm_map.c` replaces the real loader.

1. Fix the player: sprite and animation setup so `spr_draw_player_sprite` stops faulting, shadow, and `disable_player_input` / `enable_player_input` around the scene.
2. Let `kmr_20` scripts finish: check `evt 25/34/35` in the `scripts` log (blocked in `_npc_jump_to`), then verify input moves Mario (walk, jump, camera follow).
3. Use the real `state_map_transitions.c` / `state_file_select.c` flow: real `state_init_enter_world` and `state_step_enter_world`, file load into `gCurrentSaveFile` (needs `fio.c` and a save in SD), `set_map_transition_effect`, `GotoMap`, `get_map_IDs_by_name` (currently a stub returning 0, needs `gAreas` data or a port table).
4. Then the `hos_05` intro should hand off to the real file path instead of the stand-in.

## Do not

Do not push. One Azahar window; the run script kills only the process it started. Do not leave `fast` in `debug3ds.txt` (it skips the logos). `3ds/run_azahar.ps1` stops on `-Until`, a crash, log silence (`-Stall`), or `-Timeout`. Use `run_azahar.ps1` directly, not `go.ps1`, to see the unmapped-access report. The full intro needs about 150s. Log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt`. Window captures do not show the OpenGL screen. GPU work stays on the GPU. Paperboat (`reference/paperboat`) shares the game-side sprite and imgfx code. The Fast3D interpreter to compare against is `reference/soh-3ds/third_party/libultraship/src/fast/interpreter.cpp`.
