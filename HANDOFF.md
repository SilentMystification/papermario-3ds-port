# Handoff

## RULE: NO STUBS WITHOUT ASKING

Do not add a stub. Do not write a stand-in for decomp code. The user dislikes stubs, and they cause bugs. This port is not a recreation of the game. Build the real decomp file.

If a stub looks like the only option, stop. Ask the user for permission. Name the unported system that forces it. Wait for the answer.

## TOP PRIORITY: remove the stubs

The next pass removes existing stubs and stand-ins. Do this before any new feature or open issue.

Method: build the real decomp file. Use `3ds/tools/dedup_stubs.py` on the link log to drop stubs that the real file now defines. Let unresolved symbols guide the work. Ask the user before keeping any stub.

Known stubs and stand-ins to remove:
- `load_map_by_IDs` in `3ds/src/pm_map.c` replaces the real loader in `src/world/world.c`. It hard-codes shape names, texture names, settings, main script, and camera per map.
- `get_map_IDs_by_name` in `3ds/src/game_stubs.c` is a hand-written list of 5 maps. This is why the pipe in kmr_20 reloads kmr_20 in place of Toad Town. Use the real `gAreas` table and lookup from `src/world/world.c`. That needs the `src/world/area_*` maps built. S01 measured this (see the next section).
- `load_background("kmr_bg")` in `3ds/src/pm_title.c` ignores the map's `bgName`.
- `state_step_file_select` and `state_init_world` in `3ds/src/pm_title.c` skip the real file select and enter-world flow.
- `state_step_change_map` in `3ds/src/pm_title.c` replaces `src/state_map_transitions.c`. It has no fade because `screen_overlays.c` is not built. `3ds/src/pm_overlay.c` replaces `screen_overlays.c`.
- `set_map_transition_effect` in `3ds/src/game_stubs.c` is empty.
- `3ds/src/pm_later.c` stubs most `state_*` functions (battle, pause, demo, game over, language select) and `fx_*`.
- `3ds/src/pm_evt_stub.c` has skip stubs: `SpeakToPlayer`, `ShowMessageAtScreenPos`, `ShowMessageAtWorldPos`, `SetSpriteShading`, `ShowEmote`, `AddKeyItem`, `DropItemEntity`, `ShowGotItem`, `hos_05_EVS_Starship_Depart`.
- `3ds/src/pm_kmr_stubs.c` has audio and item stubs. `bgm_set_song` and `play_ambient_sounds` return 1 so scripts do not block.
- `3ds/src/pm_actor_stubs.c` has `fx_*`, `make_item_entity`, `partner_clear_player_tracking`, `partner_get_enter_map_script`, `partner_is_flying`, and more.
- `3ds/src/pm_shadow.c` and the empty `Entity_*_Render` functions draw nothing.

Not audited: `pm_fx.c`, `pm_shadow.c`, and the other stub files. Audit them first.

## Now

Paper Mario O3DS port. The intro (`hos_05` entry 3) plays in Azahar. After the title, the real `kmr_20_EVS_Main` runs. The Luigi scene plays, then osr_00, then kmr_20 again.

State of the tree at this handoff:
- Committed `e7d46af0e`: map reload fix. `load_map_by_IDs` now calls `spr_init_sprites` (not for the intro map) and clears animators, entity models, encounters, and effects. A port-side `state_step_change_map` runs `GotoMap`. Azahar `fast autoa` ends with no unmapped-access warning.
- Uncommitted, in the next commit: kkj_00 sources are in `3ds/CMakeLists.txt` and `load_map_by_IDs` has a kkj_00 branch (the same hard-coded pattern, to be removed). The real `enter_exit.c`, `audio_api.c`, `enemy_ai.c`, and `25AF0.c` now build and replace their stubs. `msg_api.c` stays out because there is no message box system.
- kkj_00 does not load in the intro chain yet. The `fast autoa` run ends in kmr_20 after the third load. The log does not show which `GotoMap` fires before that. Log `get_map_IDs_by_name` calls to find out.
- A pipe in kmr_20 runs `GotoMap mac_00`. mac_00 is not in the name table. The map reloads kmr_20 and Mario exits his own door.

## S01 result: all area_* maps (measured, no code change)

- 28 areas, 2,401 `.c` files, 9.6 MB of `.c` source (10.0 MB with headers).
- Compiled with the CMake flags (-O2, devkitARM, `-c` only): 1,960 files built, 441 failed. The failures are missing generated enums (`ACTOR_LEVEL_*`, `SHADING_*`, `ITEM_FIRST_KEY`, `NPC_SELF`). Those 441 files are 4.0 MB of source. The built files are 6.3 MB of source.
- Built files: .text 538 KB, .data 2,865 KB (the map data is non-const, so it is .data), .rodata 10 KB, .bss 212 KB.
- Extrapolated to all files (x1.64 by source size): .text about 0.9 MB, .data about 4.7 MB, .bss about 0.35 MB. Total about 6 MB. Current 3dsx is 745 KB.
- RAM: `pm_mem.c` splits the 64 MB O3DS FCRAM as 4 MB N64 heap + 40 MB cart image + 20 MB linear. Nothing is free. 6 MB of static image does not fit.
- DECISION: does NOT fit. Do not link all maps. S08 must link only the maps in use, or load map data from the ROM at run time. Linking a subset needs a `gAreas` built from only those maps. The real `gAreas` in `world.c` references all of them.

## Open issues (carry over until fixed)

1. kkj_00 is not reached by the intro chain. See above. Toad Town (mac_00) and every other map also fail to load. The generic map table fixes both.
2. Lighting looks wrong on Luigi and Parakarry (sprite shading). `SetSpriteShading` is a skip stub, so shading stays off and sprites draw flat. Tried the real `sprite_shading.c` + `demo_api.c` with `--defsym sprite_shading_profiles_ROM_START=0x315B80` (data +0x1D0). Shading ran but characters turned dark. `appendGfx_shading_palette` renders a 16x1 palette into a fake framebuffer and does a TLUT double lookup, which `pm_gbi.c` cannot emulate. Reverted. Fix needs a port-side shader path: compute shadow/highlight colors on the CPU and tint the sprite.
3. Text box transitions do not happen. Parakarry's first text box does not show.
4. Transparency half works: translucent models show the background layer but not other geometry behind them. Likely draw order or depth write.
5. The Mario logo does not show on the house in the intro cutscene.
6. The red outline stays on screen after it should go. It blocks a large part of the view.
7. Mario has transparency problems around his sprite. The background layer shows in place of the scene geometry behind him. Likely the same cause as issue 4.

Known gaps: audio, item and popup APIs, entity display lists and shadows, `bed.c` and `records.c` (need extracted textures), message boxes, status bar, HUD.

Debug switches in `debug3ds.txt` or `-Debug "..."`: `fast`, `autoa`, `shots` (frame every 30 to `sdmc/3ds/PaperMario/shots/*.ppm`, `build3ds/sheet.py step start count out.png` makes sheets), `scripts` (live script dump every 120 frames), `player`. Use `fast autoa shots` to reach kmr_20 with frames. Do not use `shots` with `autoa` to watch the intro. gdb: `-Gdb -GdbScript <file>`. Per-frame gdb `commands` logging did not work, and an async `interrupt` in a gdb script did not stop the target. Use game-side logging.

## Next

After the stub cleanup: boot a file and progress instead of sitting on the first frame. Use the real `state_map_transitions.c` / `state_file_select.c` flow, file load into `gCurrentSaveFile` (needs `fio.c` and a save in SD), and the real `GotoMap`. Then work open issues 2 to 7. Then verify input moves Mario.

## Next after the stub list is empty (performance)

Do not start before `3ds/SLICES.md` is done. Order:
1. Add a per-frame phase timer: game logic, GBI transform and clip, GPU submit, texture load. Measure on real O3DS (`/push-o`), not Azahar.
2. If transform and clip dominate, transform each vertex once at `G_VTX`. `draw_tri` in `pm_gbi.c` now transforms per triangle corner. CPU only. Small change.
3. Then move the transform to the GPU: raw `Vtx` as short attributes, `u8` index list, `C3D_DrawElements`, combined matrix in the `projection[4]` uniform of `shaders/pm.v.pica`. Risks: hardware near-plane clipping (unverified), float24 precision, lighting and fog vertices, the `note_door` hack, `Vtx` endianness, small batches (one flush per matrix change).
4. Last, optional: pre-baked native assets. Offline PC bake tool, SD cache, fall back to runtime conversion on a miss. Textures first (`pm_tex.c` cache), static-model vertex buffers second. Keep the N64 data as the source of truth.

## Slice workflow

`3ds/SLICES.md` is the queue. One subagent does one slice with a fresh context. The orchestrator reads only the first open line and any `BLOCKED:` line here.

## Do not

Do not push. Do not stub (see the rule at the top). One Azahar window; the run script kills only the process it started. Do not leave `fast` in `debug3ds.txt` (it skips the logos). `3ds/run_azahar.ps1` stops on `-Until`, a crash, log silence (`-Stall`), or `-Timeout`. Use `run_azahar.ps1` directly, not `go.ps1`, to see the unmapped-access report. `go.ps1` skips the build when the 3dsx is newer than all sources, even after a `CMakeLists.txt` change. Touch a source file to force a build. The scene after the third kmr_20 load is idle, so the log goes quiet. That is not a hang. The full intro needs about 150s. Log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt`. Window captures do not show the OpenGL screen. GPU work stays on the GPU. Paperboat (`reference/paperboat`) shares the game-side sprite and imgfx code, and runs the unmodified decomp loader. The Fast3D interpreter to compare against is `reference/soh-3ds/third_party/libultraship/src/fast/interpreter.cpp`.
