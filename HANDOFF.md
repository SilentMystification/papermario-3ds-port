# Handoff

When a feature or slice is finished, stop and tell the user. They start a new agent from this file. Do not keep working in the same chat.

A commit is not automatically a handoff. Each commit, decide whether the slice is done enough for a new agent. If it is, or if a handoff looks like the right next step for any other reason, bring it up and wait.

Replace this file. Do not append a history. Keep it short.

Plan: [Cutscene systems plan](file:///C:/Users/Andrew/.cursor/plans/cutscene_systems_plan_2097aefa.plan.md). Scene setup, actors, and Bowser are done. Effects are still stand-in quads. Audio is pending. Cook ROM assets only if a measured frame budget is missed.

## Now

Paper Mario O3DS port. The intro (`hos_05` entry 3) plays in Azahar. After the title, the real `kmr_20_EVS_Main` runs and the Luigi scene now plays to its end.

This round:
- Root cause of the NPC fall (old issues 1 and 2): `npc_collision.c` was not built. `npc_raycast_down_*` and `npc_test_move_*` were zero stubs, so NPCs had no floor, fell below y=-2000, and `update_npcs` zeroed `moveSpeed`. `3ds/CMakeLists.txt` now builds `npc_collision.c` and the stubs are gone. Luigi now walks on floor 25 and 9, and Mario reaches (70, 30, -110).
- `player` debug switch now also logs each NPC (`npcN x y z spd floor flags`).
- The game stops logging at about frame 299 (after about 20 s). The script is at `npc_intro.c:389`: `GotoMapSpecial osr_00` right after Luigi's letter. `get_map_IDs_by_name` is a stub (`game_stubs.c`) and the real map transition is not wired, so the loop hangs. Not yet confirmed where it hangs. Text boxes do not draw, so the user sees "Luigi reads the letter, then freeze".

## Open issues (carry over until fixed)

1. Hang at `GotoMapSpecial osr_00` (above). Needs the real transition path (see Next 3), or a stand-in that ends the scene.
2. Lighting looks wrong on Luigi and Parakarry, so probably all character (sprite) lighting. Check sprite shading (`pm_shade`, `gSpriteShadingProfile`).
4. Text box transitions do not happen. Parakarry's first text box does not show.
4. Transparency half works: translucent models show the background layer but not other geometry behind them. Likely draw order or depth write.
5. The Mario logo does not show on the house in the intro cutscene.

Known gaps: audio (all `SetMusic`, `PlaySound` are skip stubs in `pm_evt_stub.c`), item and popup APIs (`pm_kmr_stubs.c`), `fx_*` return 0 (`pm_later.c`), entity display lists and shadows draw nothing (`Entity_*_Render` are empty, `pm_shadow.c`), `bed.c` and `records.c` (need extracted textures), message boxes, status bar, HUD.

Debug switches in `debug3ds.txt` or `-Debug "..."`: `fast`, `autoa`, `shots` (frame every 30 to `sdmc/3ds/PaperMario/shots/*.ppm`, `build3ds/sheet.py step start count out.png` makes sheets), `scripts` (live script dump every 120 frames), `player`. Use `fast autoa shots` to reach kmr_20 with frames. Do not use `shots` with `autoa` to watch the intro. gdb: `-Gdb -GdbScript <file>`. Per-frame gdb `commands` logging did not work. Use game-side logging.

## Next

Boot a file and progress instead of sitting on the first frame. The port still fakes the file path: `pm_title.c` `state_step_file_select` jumps straight to kmr_20 entry 0 and `load_map_by_IDs` in `pm_map.c` replaces the real loader.

1. Work the open issues above, starting with 1 and 2 (they block the scene).
2. Verify input moves Mario (walk, jump, camera follow).
3. Use the real `state_map_transitions.c` / `state_file_select.c` flow: real `state_init_enter_world` and `state_step_enter_world`, file load into `gCurrentSaveFile` (needs `fio.c` and a save in SD), `set_map_transition_effect`, `GotoMap`, `get_map_IDs_by_name` (currently a stub returning 0).
4. Then the `hos_05` intro should hand off to the real file path instead of the stand-in.

## Do not

Do not push. One Azahar window; the run script kills only the process it started. Do not leave `fast` in `debug3ds.txt` (it skips the logos). `3ds/run_azahar.ps1` stops on `-Until`, a crash, log silence (`-Stall`), or `-Timeout`. Use `run_azahar.ps1` directly, not `go.ps1`, to see the unmapped-access report. The full intro needs about 150s. Log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt`. Window captures do not show the OpenGL screen. GPU work stays on the GPU. Paperboat (`reference/paperboat`) shares the game-side sprite and imgfx code. The Fast3D interpreter to compare against is `reference/soh-3ds/third_party/libultraship/src/fast/interpreter.cpp`.
