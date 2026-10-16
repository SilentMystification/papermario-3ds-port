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
- `state_step_change_map` in `3ds/src/pm_title.c` replaces `src/state_map_transitions.c`. `screen_overlays.c` and `screen_render_util.c` now build (S02). `3ds/src/pm_overlay.c` keeps only the curtain functions (S13 replaces them).
- `3ds/src/pm_later.c` stubs most `state_*` functions (battle, pause, demo, game over, language select) and `fx_*`.
- `3ds/src/pm_evt_stub.c` has skip stubs: `SpeakToPlayer`, `ShowMessageAtScreenPos`, `ShowMessageAtWorldPos`, `SetSpriteShading`, `ShowEmote`, `AddKeyItem`, `DropItemEntity`, `ShowGotItem`, `hos_05_EVS_Starship_Depart`.
- `3ds/src/pm_kmr_stubs.c` has audio and item stubs. `bgm_set_song` and `play_ambient_sounds` return 1 so scripts do not block.
- `3ds/src/pm_actor_stubs.c` has `fx_*`, `make_item_entity`, `partner_clear_player_tracking`, `partner_get_enter_map_script`, `partner_is_flying`, and more.
- `3ds/src/pm_shadow.c` and the empty `Entity_*_Render` functions draw nothing.

Not audited: `pm_fx.c`, `pm_shadow.c`, and the other stub files. Audit them first.

RESOLVED by E01 (run `3ds/tools/extract_assets.ps1`; see E01 section). Was: S02 needs `src/screen_overlays.c`. It uses `INCLUDE_IMG` for 4 stencil PNGs (`ui/stencil/{star,mario,sharp_circle,blurry_circle}.png.bin` under `ver/us/build/`) and `#include "vtx/stencil1.vtx.inc.c"`. Both are splat/asset-extraction outputs. The repo has no `ver/us/build`, no `src/vtx`, no `assets`, and no ROM. The port has no build-time asset extraction (same cause as `bed.c` and `records.c`). Any hand-made texture or vertex data is a stand-in. Needed first: a build-time extraction step for these assets, or a user decision.

## E01 result: asset extraction

Run `powershell -File 3ds/tools/extract_assets.ps1 [-Rom path]` from a clean tree. It checks the ROM SHA1 against `ver/us/checksum.sha1`, copies it to `ver/us/baserom.z64`, makes a venv in `build3ds/.venv_e01` (repo requirements + pigment64 + pyinstaller), runs the repo splat extraction, writes `build.ninja`, then builds only the `e01_assets` target (generated headers and `.png.bin`/`.pal.bin`). No N64 compile. Needs Python 3 and ninja on PATH.
- Outputs (all git-ignored): `assets/us/` (PNGs, `vtx/*.vtx.inc.c`, `*.gfx.inc.c`, bins), `ver/us/build/` (`.png.bin`, `include/` generated headers: `item_enum.h`, `move_enum.h`, `battle/actor_types.h`, `sprite/player.h`, `sprite/npc/*.h`, `sprite/sprite_shading_profiles.h`, `message_ids.h`, `*.png.h`), `build.ninja`.
- The Rust `pigment64` and `crunch64` CLIs are not installed. `3ds/tools/pigment64_shim.py` calls the repo's `tools/build/img/build.py`. `crunch64_shim.py` calls the crunch64 pip package (wrapped to an exe for `sprites.py`). One expected failure: `crash_screen/font.png.bin` (format i1 unsupported by build.py). Nothing else needs it.
- The 3DS CMake does not yet have `-Iassets/us -Iver/us/build/include`. Add both (`INCLUDE_IMG` also reads `ver/us/build/...bin` relative to the repo root). With those flags `screen_overlays.c`, `kmr_20/bed.c`, `kmr_20/records.c`, and an area file that uses `ACTOR_LEVEL_*`/`SHADING_*` pass `arm-none-eabi-gcc -fsyntax-only`. The enum headers for the 441 S01 failures now exist; a full recompile of those was not run.

## S02 result

Real `src/screen_overlays.c` and `src/screen_render_util.c` build. CMake has `-Iassets/us`, `-Iver/us/build/include`, `-DVERSION=us`, `-Wa,-I<repo>`. `3ds/stub_include/include_asset.h` is a copy of the repo header with `%object` (ARM gas). Removed: overlay functions in `pm_overlay.c`, `set_map_transition_effect`, `_render_transition_stencil`, `func_80138198`. Added `guTranslate` to `pm_gu.c` and `nuGfxCfb`/`nuGfxCfbNum` (2 tokens) in `pm_main.c`. The tokens hold no pixels, so `draw_prev_frame_buffer_*` shows no previous frame. `fast autoa` check: no unmapped-access warning, kmr_20 reached. The fade was not checked by eye.

## S03 result

BLOCKED: S03 needs `draw_status_ui` from `src/inventory.c` (unported status bar system, slices S05 and S06). Real `src/state_map_transitions.c` calls `draw_status_ui` in `state_drawUI_change_map` and `state_drawUI_enter_world`. `inventory.c` compiles once `3ds/stub_include/move_enum.h` (an empty shadow of the generated header) is deleted. It then needs `hud_element_set_script`, `hud_element_set_render_pos`, `hud_element_clear_flags`, `hud_element_draw_next`, `TimesHudScript`, `DigitHudScripts`, `SlashHudScript`, `SPStarHudScripts`, `SPIncrementHudScripts` (`src/hud_element.c`, S05) and `basic_hidden_window_update` (`src/windows.c`, S05). It also duplicates stubs for `clear_player_data`, `open_status_bar_slowly`, `close_status_bar`, `setup_status_bar_for_world`, `enable_status_bar_input`, `disable_status_bar_input`, `status_bar_respond_to_changes`, `sync_status_bar`, `is_ability_active`, `subtract_hp`. Also needed: real `nuContRmbForceStopEnd` (`src/os/nusys/nucontrmbforcestopend.c` calls `nuSiSendMesg`, not in the port; `pm_nusys.c` only has a no-op `nuContRmbForceStop`). Other than that, state_map_transitions.c links with only the stubs from pm_later.c and pm_title.c removed. Suggested order: do S05 and S06 first, then S03. When doing S03, `state_init_world` in `pm_title.c` needs a guard so it does not reload the map after the real `change_map` (use a flag set by file select). No code was changed. Tree reverted.

## S04 result

BLOCKED: S04 needs real files from S05 and others. `src/item_entity.c` and `src/evt/item_api.c` compile and the stub duplicates are removed by `dedup_stubs.py` (11 symbols). The link then fails on: `gHudElementCacheTableRaster`, `gHudElementCacheTablePalette`, `gHudElementCacheSize`, `gHudElementCacheBuffer` (`src/hud_element.c` and `src/bss/engine2_pre_bss.c`, S05), `SparkleScript_Coin` (`src/global_hud_scripts.c`, not in CMake), `gItemTable` and `gItemEntityScripts` (`ver/us/build/include/item_data.inc.c`, included by `src/5B320.c`, not in CMake), and `func_801491E4` (`src/sprite_shading.c`, which has the open issue 2 problem with `appendGfx_shading_palette`). That is 4 real files, not one. Suggested order: S05 first, then add `global_hud_scripts.c` and `5B320.c`, then decide on `sprite_shading.c`. No code was changed. Tree reverted.

## S11 result

Done. Real `src/effects.c`, `src/effect_utils.c`, `src/evt/fx_api.c`, all 127 `src/effects/*.c` and 112 `src/effects/gfx/*.c` build and link. `pm_fx.c`, its `PlayEffect` stand-in, and all `fx_*` stubs are gone. Static image grew from 6.69 MB to 7.43 MB (3dsx 0.9 MB to 1.8 MB). .bss is unchanged at 5.6 MB. The 64 MB FCRAM split in `pm_mem.c` is untouched. `fast autoa` reaches kmr_20, no unmapped access.
- `3ds/tools/gen_effect_ld.py` (run after `extract_assets.ps1`, which now also runs `tools/build/effects.py`) writes `stub_include/effect_ld_addrs.h` (effect ROM/VRAM ranges empty, gfx start null, so no DMA), `3ds/effects.cmake` (source list from build.ninja) and `src/effect_loads.c` (C form of the splat asm `fx_<name>`: `load_effect(id)` then `<name>_main`).
- `effect_shims.yaml` names become `--defsym shim_X=X` aliases. `pm_os.c` has no-op `osMapTLB`, `osUnmapTLB`, `osUnmapTLBAll` and 1-byte `gEffectDataBuffer`, `gEffectGlobals`.
- `OS_K0_TO_PHYSICAL` is a no-op under `TARGET_3DS`: the 0x8 top nibble made Azahar reject the 3dsx ("Generic load error", relative to the loader's 4-bit relocation subtype check).
- Not checked by eye: how the effects draw. Known gap: `flame` and a few effects render into scratch buffers `D_800B32E0` and `D_800B3AE0` (render-to-texture). `pm_gbi.c` does not emulate that. Effects that need `draw_box`, `draw_msg` or audio still call the port stubs (S12, S14).

## S12 result

Done. Real `src/audio/*.c` (ambience, bgm_control, bgm_player, load_banks, mseq_player, sfx_control, sfx_player, snd_interface) and `src/audio/core/*.c` (engine, pull_voice, reverb, syn_driver, system, tables, voice) plus `src/heaps.c` build and link. Removed stubs: `bgm_*`, `play_ambient_sounds`, `reset_ambient_sounds`, `update_ambient_sounds`, `snd_*`, `sfx_*`, `gCurrentDoorSounds`, `BattleEntityHeapBottom`, the 1-byte `gEffectDataBuffer` and `gEffectGlobals`, and the empty `shim_create_audio_system_obfuscated` (it now calls the real `create_audio_system`). `sfx_play_sound_at_npc` now resolves. `fast autoa` reaches kmr_20 with no unmapped-access warning (120 s run). The log shows `audio posts N peak P` every 120 posts. Peak is 10000 to 18000 while the intro and kmr_20 music play, so the engine makes real PCM and `pm_audio_post` queues it to ndsp. No ear check.
- RSP microcode: the engine builds `Acmd` lists for the RSP audio ucode. The 3DS has no RSP. `3ds/src/pm_rsp_audio.c` (from `reference/paperboat/src/port/audio/mixer.c`, a C version of the ucode ops: ADPCM decode, resample, envelope mix, mix, interleave) runs each `aXxx` op at the call site. `3ds/stub_include/PR/abi.h` and `n_abi.h` wrap the real headers and redirect the macros (`pkt` is still evaluated, so the command count stays right). No game logic. `n_aspMainTextStart` and `n_aspMainDataStart` are empty arrays because only their addresses are stored.
- Audio thread: `nuAuMgr` runs as a real libctru thread (`pm_au_thread_*` in `pm_audio.c`) but hands control back and forth, so only one thread runs at a time. `pm_audio_tick` (`pm_os.c`, called from `pm_frame_loop` once per elapsed 60 Hz, at most 3) wakes it, then plays the scheduler and RSP part: completes each audio task and sends its reply. `pm_os.c` also has `nusched`, `nuScAddClient`, `osCreateThread`, `osStartThread`, `osSetIntMask`, `osEPiStartDma` (calls `nuPiReadRom`), and a blocking `osRecvMesg` for the audio thread only. `osAiGetLength` reports the bytes still queued in ndsp, so the engine picks its frame size as on the N64. `-Debug noaudio` skips the tick.
- Endian: ROM audio data is big-endian. Each structure is swapped once right after its `au_read_rom` (`au_swap16/32` under `TARGET_3DS` in `engine.c`, `load_banks.c`): SBN header and file list, INIT lists, SEF header, BGM header/drums/instruments, MSEQ header and track settings, PER, PRG, BK header, instruments (once per distinct offset), predictors, loop states, envelope offsets. In-stream reads are swapped at the read: BGM composition and phrase words (`AU_BE32`), SEF index words (`AU_BE16` in `au_sfx_try_sound`), `SeqArgs` fields in `bgm_player.c`. `AmbVoiceStateInfo` and `SoundPlayChange` in `audio.h` have reversed field order under `TARGET_3DS`, because the code packs them with big-endian shifts.
- `-fno-short-enums`: arm-none-eabi makes enums 1 byte. `snd_song_load` returns a song name in an `AuResult`, which was cut to 0x20 and no song loaded. `CMakeLists.txt` now builds every file except those that include libctru or citro3d (`pm_main, pm_mem, pm_log, pm_input, pm_gpu, pm_tex, pm_audio`) with `-fno-short-enums`.
- Latent bug fixed: `heap_collisionHead` and `heap_battleHead` in `pm_later.c` had no 16-byte alignment. `_heap_create` needs it. Adding `heaps.c` moved them to a misaligned address, `collision_heap_malloc` returned NULL and `load_hit_data` ran on null. Now `aligned(16)`.
- CPU cost (Azahar only, the O3DS at 192.168.1.168 was off, so NOT measured on hardware): `audio cost avg 3000 us (up to 4000 us) max 30000 us per 60 Hz tick`, about 18% to 24% of a 16.7 ms retrace, at 32 kHz and 24 voices, scalar C (the ARM11 has no NEON). Azahar timing is not cycle exact, so real hardware may be slower. The `audio cost` line is in the game log. Next step: run `/push-o` and read that line. If the average is above about 5 ms per tick, write `BLOCKED:` and ask the user before lowering quality or sample rate.
- `3ds/tools/dedup_stubs.py` now also edits `pm_msg.c`.
- Not done: other audio-related game APIs may still stub (S14, S09). The `fast autoa` check does not prove sound effects play. Reverb and the SEF tables were not checked by ear.

## S14 result

Done. Real `src/evt/msg_api.c`, `draw_box.c`, `draw_img_util.c`, `speech_bubble.c`, `inspect_icon.c`, `i_spy.c`, `pulse_stone.c` build and link. Removed stubs: `SpeakToPlayer`, `ShowMessageAtScreenPos`, `ShowMessageAtWorldPos`, `appendGfx_speech_bubble`, `appendGfx_interact_prompt`, `appendGfx_ispy_icon`, `appendGfx_pulse_stone_icon`, `interact_inspect_setup`, `interact_speech_setup`, `ispy_notification_setup`, `pulse_stone_notification_setup`, `draw_box`, `draw_ci_image_with_clipping`, `draw_image_with_clipping`, `integer_log`. `pm_msg.c` is deleted. `fast autoa` (120 s) reaches kmr_20, no unmapped access. Not checked by eye: Parakarry text box, box art and font drawing (pm_gbi.c features not checked). S09 now needs only `fx_*` (S11, done) and `sfx_play_sound_at_npc` (S12, done): retry it.

## S09 and C01 result

Done. Real `src/world/partners.c`, `src/world_use_item.c`, 11 `src/world/partner/*.c`, `src/state_map_transitions.c` and `src/os/nusys/nucontrmbforcestopend.c` build and link. Removed: `partner_*`, `gPartnerStatus`, `gPartnerAnimations`, `wPartnerNpc` and related stubs (12 symbols), `state_*_change_map` in `pm_title.c`, `state_*_enter_world/game_over/enter_demo` in `pm_later.c`. S03 is covered by this (the real `state_map_transitions.c`).
- `world_partner_*` ROM/VRAM and `world_use_item_ROM_*` are `extern Addr` in `ld_addrs.h`, set by `--defsym` to `gPartnerStatus` (an in-image address, so START == END and `dma_copy` copies nothing). Value 1 made 3dsxtool fail ("Relocation to invalid address"). `partners.c` gets `-include common.h`.
- `nuSiSendMesg` is a no-op port shim in `pm_nusys.c` (no SI or rumble pak on the 3DS).
- `pm_map.c load_map_by_IDs` now calls `init_hud_element_list`, `hud_element_clear_cache`, `init_item_entity_list` and `initialize_status_bar` (the real `draw_status_ui` crashed without them). It sets `pm_map_loaded`, which `state_init_world` in `pm_title.c` uses to skip a second load. The title clears it before entering the world.
- `fast autoa` (170 s): kmr_20, osr_00, kmr_20 chain, no unmapped access. kkj_00 is still not reached (open issue 1, unchanged). Fade and status bar were not checked by eye.

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

Known gaps: item and popup APIs, entity display lists and shadows, `bed.c` and `records.c` (need extracted textures), message boxes, status bar, HUD.

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
