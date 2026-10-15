# Handoff

When a feature or slice is finished, stop and tell the user. They start a new agent from this file. Do not keep working in the same chat.

A commit is not automatically a handoff. Each commit, decide whether the slice is done enough for a new agent. If it is, or if a handoff looks like the right next step for any other reason, bring it up and wait.

Replace this file. Do not append a history. Keep it short.

Plan: [Cutscene systems plan](file:///C:/Users/Andrew/.cursor/plans/cutscene_systems_plan_2097aefa.plan.md). Scene setup, actors, and Bowser are done. Effects are still stand-in quads. Audio is pending. Cook ROM assets only if a measured frame budget is missed.

## Now

Paper Mario O3DS port. The intro (`hos_05` entry 3) plays end to end in Azahar.

Fixed this slice (not committed):
- Clown car: 4-bit imgfx strips ignored the LoadTile row offset (`pm_gbi.c` `G_LOADTILE`). The top rows drew twice.
- Real message code is compiled: `msg.c`, `msg_draw.c`, `msg_data.c`, plus `3ds/src/pm_msg.c` (blank box art, stub `draw_box`). The narrator text draws. Font ROM offsets are in `3ds/stub_include/charset_offsets.h`. `dma_load_msg` byte-swaps. `draw_msg` passes the ID, not the buffer pointer, to `get_msg_properties`.
- `MSG_Intro_*` IDs in `message_ids.h` were fake. Intro is message section 0, so ID = index.
- `render_curtains` resets the scissor before the narrator callback.
- Fire breath from `PlayEffect` now uses the script life (it never ended).

Not done: fire and lightning and the other effects are still colored quads (`pm_fx.c`). The real effect code is in `src/effects/` but its display lists live in the ROM effect overlay. The black-silhouette fade-in of Bowser is not verified. Message boxes, choices, and item icons in text draw nothing (box art is blank, `draw_box` is a stub).

Debug switches in `debug3ds.txt` or `-GameDebug`: `shots` saves a frame every 30 frames to `sdmc/3ds/PaperMario/shots/*.ppm` (240x400, rotate 90 degrees; `build3ds/sheet.py` makes contact sheets, crop hides the letterbox bars, so view full frames for text). Do not combine `shots` with `autoa`: A skips the intro. `3ds/run_azahar.ps1` deletes `debug3ds.txt` at the end of a run.

## Next

After the title, the port runs stand-ins. `pm_title.c` and `pm_map.c` replace the real title, file select, enter-world, and change-map code. kmr_20 loads as a static mesh. It has a fixed camera, no script, no player, no collision, and no HUD.

Make the real kmr_20 main script run. Known gaps:
- Flag and byte storage in `pm_evt_stub.c` always returns 0 and stores nothing. `create_trigger` returns NULL.
- Of 129 `Call()` names in kmr_20, 11 are stubs and 62 are not linked: player (`SetPlayerPos`, `PlayerJump`, `SetPlayerActionState`), audio, messages (`ShowChoice`, `CloseMessage`), entities (`MakeEntity`), and the kmr_20 and ToadHouse map code.
- Not compiled: collision, entity, trigger, msg, windows, hud, fio, audio, `src/world/action/`, pause, battle, `state_world.c`, `state_map_transitions.c`.
- `player_raycast_down` (from `update_npcs`) reads unmapped memory about 100 times per run. That would fault on hardware.

Start small: compile `collision.c`, `trigger.c`, `entity.c`, and real flag storage. Then run `kmr_20_EVS_Main`.

## Do not

Do not push. One Azahar window; the run script kills only the process it started. Do not leave `fast` in `debug3ds.txt` (it skips the logos). `3ds/run_azahar.ps1` stops on `-Until`, a crash, log silence (`-Stall`), or `-Timeout`. Use `run_azahar.ps1` directly, not `go.ps1`, to see the unmapped-access report. The full intro needs about 150s. Log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt`. Window captures do not show the OpenGL screen. GPU work stays on the GPU. Paperboat (`reference/paperboat`) shares the game-side sprite and imgfx code. The Fast3D interpreter to compare against is `reference/soh-3ds/third_party/libultraship/src/fast/interpreter.cpp`.
