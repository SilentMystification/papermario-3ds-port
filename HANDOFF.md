# Handoff

When a feature or slice is finished, stop and tell the user. They start a new agent from this file. Do not keep working in the same chat.

A commit is not automatically a handoff. Each commit, decide whether the slice is done enough for a new agent. If it is, or if a handoff looks like the right next step for any other reason, bring it up and wait.

Replace this file. Do not append a history. Keep it short.

Plan: [Cutscene systems plan](file:///C:/Users/Andrew/.cursor/plans/cutscene_systems_plan_2097aefa.plan.md). Scene setup, actors, and Bowser are done. Effects are still stand-in quads. Audio is pending. Cook ROM assets only if a measured frame budget is missed.

## Now

Paper Mario O3DS port. The intro (`hos_05` entry 3) plays in Azahar. After the title, the real `kmr_20_EVS_Main` runs and the Luigi scene now plays to its end.

This round:
- Map transition fixed. After `GotoMap`, the second kmr_20 load drew garbage display lists and the Azahar run reported unmapped accesses. Cause: `load_map_by_IDs` (`pm_map.c`) never called `spr_init_sprites`. imgfx and sprite state kept pointers into the general heap that the loader resets. The loader now calls `spr_init_sprites(PLAYER_SPRITES_MARIO_WORLD)` (not for the intro map) and also `clear_animator_list`, `clear_entity_models`, `clear_encounter_status`, `clear_effect_data`, like the real loader.
- `GotoMap` now runs a port-side `state_step_change_map` (`pm_title.c`): wait 10 frames, load the map, wait for the main script, back to world. `state_init_world` skips its own load after that. No fade, because `screen_overlays.c` is not built. The real `state_map_transitions.c` needs it.
- `imgfx_mesh_anim_update` (`src/imgfx.c`) returns if `heap_malloc` fails.
- `fast autoa` run in Azahar ends exit 0 with no unmapped-access warning.

## Open issues (carry over until fixed)

1. kkj_00 is not ported. `get_map_IDs_by_name` keeps the current map on an unknown name, so the chain reloads kmr_20.
2. Lighting looks wrong on Luigi and Parakarry (sprite shading). `SetSpriteShading` is a skip stub, so shading stays off and sprites draw flat. Tried the real `sprite_shading.c` + `demo_api.c` with `--defsym sprite_shading_profiles_ROM_START=0x315B80` (data +0x1D0). Shading ran but characters turned dark. `appendGfx_shading_palette` renders a 16x1 palette into a fake framebuffer and does a TLUT double lookup, which `pm_gbi.c` cannot emulate. Reverted. Fix needs a port-side shader path: compute shadow/highlight colors on the CPU and tint the sprite.
3. Text box transitions do not happen. Parakarry's first text box does not show.
4. Transparency half works: translucent models show the background layer but not other geometry behind them. Likely draw order or depth write.
5. The Mario logo does not show on the house in the intro cutscene.

Known gaps: audio (all `SetMusic`, `PlaySound` are skip stubs in `pm_evt_stub.c`), item and popup APIs (`pm_kmr_stubs.c`), `fx_*` return 0 (`pm_later.c`), entity display lists and shadows draw nothing (`Entity_*_Render` are empty, `pm_shadow.c`), `bed.c` and `records.c` (need extracted textures), message boxes, status bar, HUD.

Debug switches in `debug3ds.txt` or `-Debug "..."`: `fast`, `autoa`, `shots` (frame every 30 to `sdmc/3ds/PaperMario/shots/*.ppm`, `build3ds/sheet.py step start count out.png` makes sheets), `scripts` (live script dump every 120 frames), `player`. Use `fast autoa shots` to reach kmr_20 with frames. Do not use `shots` with `autoa` to watch the intro. gdb: `-Gdb -GdbScript <file>`. Per-frame gdb `commands` logging did not work. Use game-side logging.

## Next

Boot a file and progress instead of sitting on the first frame. The port still fakes the file path: `pm_title.c` `state_step_file_select` jumps straight to kmr_20 entry 0 and `load_map_by_IDs` in `pm_map.c` replaces the real loader.

1. Work the open issues above, starting with 1 and 2.
2. Verify input moves Mario (walk, jump, camera follow).
3. Use the real `state_map_transitions.c` / `state_file_select.c` flow: real `state_init_enter_world` and `state_step_enter_world`, file load into `gCurrentSaveFile` (needs `fio.c` and a save in SD), `set_map_transition_effect`, `GotoMap`, `get_map_IDs_by_name` (currently a stub returning 0).
4. Then the `hos_05` intro should hand off to the real file path instead of the stand-in.

## Do not

Do not push. One Azahar window; the run script kills only the process it started. Do not leave `fast` in `debug3ds.txt` (it skips the logos). `3ds/run_azahar.ps1` stops on `-Until`, a crash, log silence (`-Stall`), or `-Timeout`. Use `run_azahar.ps1` directly, not `go.ps1`, to see the unmapped-access report. The full intro needs about 150s. Log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt`. Window captures do not show the OpenGL screen. GPU work stays on the GPU. Paperboat (`reference/paperboat`) shares the game-side sprite and imgfx code. The Fast3D interpreter to compare against is `reference/soh-3ds/third_party/libultraship/src/fast/interpreter.cpp`.
