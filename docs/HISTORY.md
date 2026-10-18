# History (append-only archive)

This file holds the full old HANDOFF.md as of 2026-10-17, unchanged, in order: every slice result, the S20 audit, notes and research. Nothing was deleted. Append new detail at the end. Read it only with grep when a topic needs it. The rules that stay in force are repeated in HANDOFF.md.

---

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

## S08 result (slice S08, PARTIAL: loader done, mac_00 crashes after it loads)

Done in S08: real `get_map_IDs_by_name`, `load_map_by_IDs`, `load_asset_by_name`, `get_asset_offset`, `get_current_map_settings` and `gEntityCollider*` now come from `src/world/world.c`. CMake cuts `world.c` at `#define AREA(` into `build3ds/world_gen.c` (no edit of shared source). The table is `3ds/src/area_table.c` (port side, same AREA/MAP layout, real indices and mapCount). Real MAP() rows: hos_05, kmr_02, kmr_20, osr_00, kkj_00, mac_00. All other maps are `MAP_ID()` rows (id, bgName, reverb only, null settings). Map code is linked in, so the port MAP() leaves dmaStart null (no overlay DMA).
- Removed: port `load_map_by_IDs`, `pm_maps`, `pm_settings`, `gAreas[29]`, hard-coded kmr_20/osr_00/kkj_00 settings and cameras, port `load_asset_by_name`, port `get_current_map_settings`, port `get_map_IDs_by_name` (game_stubs.c), duplicate `gEntityCollider*`, stand-in `hos_05_EVS_Starship_Depart/EnterStarship`, stale shadow headers `stub_include/item_enum.h` and `message_ids.h` (the generated ones now apply).
- Real files added: hos_05 settings+star_ship, all kmr_02 and mac_00 files, `entity/model/Hammer1Block.c`, `BlueWarpPipe`, `SaveBlock`, `HiddenPanel` (+ model), `world/script_api/shops.c`, `fio.c`, `8a860_len_3f30.c` (popup menus).
- Endian shims (port layer, no game logic): `nuPiReadRom` swaps the u32 fields of the asset table (pm_os.c). `decode_yay0` converts a decoded map shape (when dst is gMapShapeData) and a `_hit` asset in place (pm_map.c, uses `--wrap=load_asset_by_name` to learn the asset name). `gMapShapeData` is a 256 KB buffer in pm_map.c (heaps.c renamed its 32 KB copy with `-DgMapShapeData=pm_unused_shape_data`).
- Guard (pm_map.c, `--wrap` on get_map_IDs_by_name, set_game_mode, load_map_by_IDs): GotoMap to a row with null settings (or an unknown name) logs `GotoMap refused: ...` and keeps the current map. Verified: `GotoMap refused: area 1 map 2 is not linked`.
- Flash shims in pm_os.c (`osFlash*`, `osInvalDCache`, `osWritebackDCache`): reads return 0xFF, writes do nothing. Needed by real `fio.c`.
- STAND-IN left: `SaveMetadata gSaveSlotMetadata[4]` storage in pm_title.c (real one is in `state_title_screen.c`). See 3ds/QUESTIONS.md.
- Port title init now clears NPC, script, worker, model, animator, effect and entity lists, because hos_05 -> hos_04 is refused (hos_04 is id-only), so no load cleans them.
- Known bug kept from real code: `*(s32*)mapConfig->unk_1C & 3` reads a big-endian packing, so the reverb mode is 0 on ARM.
- Size: .text 1.12 MB, .data 1.11 MB, .bss 6.74 MB (image 8.97 MB, was 7.43 MB at S11). 3dsx 2.29 MB.
- Check (`run_azahar.ps1 -Debug "fast autoa"`, NOT `go.ps1`): hos_05 intro, title, file 1, kmr_20 (area 0 map 11), osr_00 (area 23), kmr_20, then the pipe loads mac_00 (area 1 map 1, `spawn npcs 24`). kkj_00 is still not reached (issue 1, unchanged: nothing sends the chain there). A few seconds in mac_00 the game crashes: PC=0 in `render_effects_UI` (effects.c:171), or under gdb a divide by zero in `spr_appendGfx_component_flat` and a fault in `is_ability_active`. Different crashes per run point to memory corruption or a sprite/effect limit in the 24-NPC mac_00 scene. Not diagnosed. Next step: find who overwrites memory in mac_00 (watchpoint on `gPlayerData.equippedBadges`, sprite heap size, effect instance count).
- Unrelated detail: always pass `-Debug "fast autoa"` to run_azahar.ps1. The script deletes debug3ds.txt otherwise.

## S07 result

Done (slice S07). Real `src/rumble.c`, `src/evt/virtual_entity.c`, `src/background.c`, `src/background_gfx.c` and the real `src/os/nusys/nucontrmb{check,modeset,start,forcestop}.c` build and link. Removed: `poll_rumble`, `start_rumble`, `update_max_rumble_duration`, `clear_virtual_entity_list`, `init_virtual_entity_list`, `reset_background_settings`, `gfx_init_state`, `gfx_draw_background`, port `nuContRmbForceStop`, and in `pm_title.c` the port `load_background` and `pm_draw_loaded_background` (title now calls real `load_map_bg("title_bg")` and `set_background`).
- `load_map_by_IDs` (port) now calls `load_map_bg`, `reset_background_settings`, `set_background` in the real order. The bg names (`hos_bg`, `kmr_bg`, `nok_bg`) are hard-coded there until S08 (real `MapConfig.bgName`). The old `load_background("kmr_bg")` in `state_init_world` is gone.
- `gBackgroundImage` is a 256 KB buffer (`pm_kmr_stubs.c`, asm label). `load_map_bg` has a `TARGET_3DS` fix-up of the big-endian header and pointers. Fog palette code swaps with `BG_PAL`.
- Bug fixed in port `decode_yay0` (`pm_map.c`): its source bound cut off the literal bytes, so every call failed. First user was `load_map_bg`.
- Port shims: `nuContRmbCtl[]` (zeroed nusys BSS) in `pm_nusys.c`. `nuSiSendMesg` returns 0, so rumble messages go nowhere.
- Port `load_asset_by_name` was removed in S08.
- `fast autoa` (170 s): kmr_20 reached, no unmapped access, no hang. Intro hos_bg sky seen in shots. The kmr_20 background was not checked by eye (shots mode is too slow to reach it).

## Now

Paper Mario O3DS port. The intro (`hos_05` entry 3) plays in Azahar. After the title, the real `kmr_20_EVS_Main` runs. The Luigi scene plays, then osr_00, then kmr_20 again.

State of the tree at this handoff:
- Committed `e7d46af0e`: map reload fix. `load_map_by_IDs` now calls `spr_init_sprites` (not for the intro map) and clears animators, entity models, encounters, and effects. A port-side `state_step_change_map` runs `GotoMap`. Azahar `fast autoa` ends with no unmapped-access warning.
- Uncommitted, in the next commit: kkj_00 sources are in `3ds/CMakeLists.txt` and `load_map_by_IDs` has a kkj_00 branch (the same hard-coded pattern, to be removed). The real `enter_exit.c`, `audio_api.c`, `enemy_ai.c`, and `25AF0.c` now build and replace their stubs. `msg_api.c` stays out because there is no message box system.
- kkj_00 does not load in the intro chain yet. The `fast autoa` run ends in kmr_20 after the third load. The log does not show which `GotoMap` fires before that. Log `get_map_IDs_by_name` calls to find out.
- A pipe in kmr_20 runs `GotoMap mac_00`. mac_00 is not in the name table. The map reloads kmr_20 and Mario exits his own door.

## S13 result

Done (slice S13). Real `src/curtains.c`, `src/entity/Shadow.c`, `kmr_20/bed.c` and `kmr_20/records.c` build and link first try. Deleted: `3ds/src/pm_overlay.c` (all curtain stand-ins), `3ds/src/pm_shadow.c` (SHADOW() blueprints, `entity_Shadow_init`, `Entity_RenderNone`, `entity_can_collide_with_jumping_player`), `kmr_20_EVS_SetupBed` and `kmr_20_EVS_Inspect_Records` in `pm_kmr_stubs.c`. No `HES_Item_*` stand-in existed. `fast autoa` (130 s): kmr_20, osr_00, kmr_20, no unmapped access.
- Curtain (issue 6): `fast shots` (32 frames, sheet via `build3ds/sheet.py`) shows the real theater curtain valence at the top of the intro and the star ship and palace scenes fully visible. The curtain is not solid red. The red cover came from the port stand-in. Issue 6 is fixed by the real file. Only the valence and side walls were seen. Not checked: curtain close and open animation timing, Shadow textures drawn in kmr_20 (shadows not looked at).

## M01 result (slice M01: trim the ROM image, audit copies)

Step 1 done and checked. Steps 2 and 3 are partial (below).
- Step 1 (slice M01): `3ds/tools/gen_rom_remap.py` reads `ver/us/splat.yaml` and writes `3ds/src/rom_remap.h` (kept ranges `{rom_start,len,packed_off}`, dropped ranges with names). `pm_rom_init` (pm_os.c) reads only the kept ranges from the z64 into one packed block. `pi_read_raw` walks each read over original offsets, so run-time offsets still work. A byte in a dropped range logs `ROM READ IN DROPPED RANGE <addr>` and reads as zero. The SD streaming fallback uses the same walk (no packing, the file keeps original offsets, dropped bytes are zeroed too). Switch `romfull` keeps the full image (still logs would-be hits). Switch `romtrace` writes `romtrace_full.txt` or `romtrace_pack.txt` in sdmc:/3ds/PaperMario (address, size, FNV-1a of the bytes delivered). No game logic.
- Dropped (8,643,106 B): world map overlays mac_01..gv_01 0x800880..0xE20110 (6,420,624), dead maps 0xE9ECD0..0xF00000 (398,128), junk gap + copy_1E00000 0x1C84D30..0x1E40000 (1,815,248), copy_1942C40, copy_1B8220C, copy_27FEE22 (9,106). mac_00 (0x7EAD10..0x800880) stays.
- Not dropped: 0x315B80..0x800880 (5.16 MB). The M01 trace read 0 bytes of it, but one 140 s run is not proof (battle, pause, other maps not run). Follow-up row M02.
- Check (measured, Azahar, `fast autoa`, same map chain in all runs: hos_05, kmr_20, osr_00, kmr_20, mac_00): full image 57,004 reads (2,522 distinct), 0 in dropped ranges. Packed image 57,088 reads, 0 dropped-range log lines. The 904 distinct reads that are not 1280 B are identical (same address, size, checksum). The only differences are 1280 B BGM stream reads at timing-dependent offsets (about 100 of 2,500 distinct), and no read with the same address and size has a different checksum. `python 3ds/tools/rom_trace_check.py full.txt packed.txt` redoes the compare.
- Distinct ROM bytes read in the run: 1,369,102 (1.3 MB) of 33.3 MB kept: main/engine 244 KB, audio 463 KB, sprites 145 KB, mapfs 516 KB, maps 0.7 KB, block 0x315B80..0x800880 0 B, msg 0.6 KB.
- RAM (measured by `pm_mem_log`, Azahar, 64 MB cap): resident ROM 40,960 KB before, 32,519 KB after. Newlib heap (44 MB) used 41,229 KB before, 32,789 KB after at boot. Heap left 3,820 KB before, 12,264 KB after. Linear pool 20,480 KB (unchanged, free about 12 MB in game). Static image (.text+.data+.bss, measured with `arm-none-eabi-size`): 8.99 MB before, 8.84 MB after step 2 (.bss 6,744,096 B to 6,590,496 B).
- ESTIMATED, not measured: on a real O3DS the process image counts against the 64 MB, so the heap is about 34 MB and the resident cap (heap minus 4 MB) is about 30 MB. The packed image (31.8 MB) still does not fit and the port streams from SD there. Dropping more never-read ROM (M02) or streaming is needed. Not run on hardware.
- Step 2 audit (live copies, sizes; measured by `nm` and the trace, nothing removed unless noted):
  - ROM image resident 32.5 MB. Every asset is also copied into a game buffer by dma_copy, so the loaded copy is a second copy. This is the largest redundancy. Working set in the run was 1.3 MB. Option: stream the ROM from SD (needs a decision on SD latency, esp. BGM 1280 B reads every few frames), or keep resident only the hot ranges. Not applied.
  - Textures (pm_tex.c): the N64 source is the game's own heap data. The PICA copy is one `C3D_Tex` per cache slot, evicted at 4 MB budget (linear pool). A 512 KB linear scratch holds the expand and tile steps. There is no persistent second copy beyond that. No change.
  - Map data: linked map data (.data 1.12 MB) no longer has a second copy in the ROM image (the overlays are dropped, 6.4 MB). Done in step 1.
  - gMapShapeData 256 KB (pm_map.c) plus `pm_unused_shape_data` 32 KB (heaps.c copy, renamed, unused): 32 KB left, it needs an edit of shared `heaps.c`, not done.
  - `fb_mem` 150 KB (pm_os.c) was an unused fallback frame buffer. Removed (osViGetCurrentFramebuffer now returns the real `gFrameBuf0`, only the reset path reads it). Check: same map chain, non-streaming reads identical.
  - `gFrameBuf0..2` 450 KB (real heaps3.c): the port GPU path does not draw into them. Real state code (state_battle, file_select) uses them as scratch, so they stay.
  - `D_80164000` 1.3 MB (real DisplayContext x2), `heap_generalHead` 1.5 MB, `heap_spriteHead` 1 MB: real sizes, needed.
- Step 3 (numbers only, no map added): free heap after step 1 is 8.4 MB (measured). S01 estimated 6 MB of static image for all maps (.data 4.7, .text 0.9, .bss 0.35), ESTIMATED. The 441 enum failures from S01 should compile now (E01 headers exist), not retried. On Azahar (44 MB heap) the whole set would fit with about 2 MB left. On a real O3DS (estimated) it does not fit with a resident ROM, because static image grows the same pool. So: link more maps only together with M02 or SD streaming. Per map the static cost averages about 6 MB / 130 maps = 45 KB (estimated), so 10 to 20 more maps cost under 1 MB.
- The mac_00 crash from S08 still shows nothing new: this run reached mac_00 and ran to the end of the 170 s without a crash line.

## M02 result (slice M02: drop more never-read ROM, measure RAM)

All items below belong to slice M02. Done and checked on Azahar. Not run on hardware (see below).
- Bug found in the M01 generator (slice M01 code, fixed in M02): the line scanner of `gen_rom_remap.py` missed 121 splat segments. It now uses PyYAML (`pip install pyyaml`). The M01 drops come out the same. One M01 error: `entity_model_BombableRock` reads 0xE9D650..0xEA0900, but M01 dropped 0xE9ECD0..0xF00000. The drop now starts at 0xEA0900 (5.4 KB kept).
- New drops (5,720,208 B more, total 14,363,314 B): (a) the block 0x3169F0..0x800880 except `effect_gfx_*` (kept) and `sprite_shading_profiles` 0x315B80..0x3169F0 (kept, read through defsym): effect code, world_partner, battle_partner, battle_move, action_cmd, battle areas, world_model_anim, world_script_api, texture_memory, mac_00, items, level_up, starpoint, btl_states_menus. (b) resident code overlays engine1, engine2, evt, engine4, entity, pause, filemenu, battle_code (1.23 MB). `main` stays: the trace reads 8,544 B inside it.
- PROVEN: (1) trace: 0 reads in any dropped range in 3 runs (S17 trace re-checked against the new table, M02 full image, M02 packed). (2) `3ds/tools/rom_static_check.py`: all 77 named ROM ranges in ld_addrs.h, entity_model_offsets.h and the defsym list have no overlap with a dropped range, and icon/charset offset headers have no value inside one. (3) The ROM_START symbols of every dropped overlay are empty ((u8*)1, (u8*)0 or an alias of gPartnerStatus), so dma_copy gets length 0. On branch s19-full-battle the battle header has only empty ranges (read with `git show`, branch not touched).
- NOT proven: (1) battle, pause and other maps were not run (not reachable on main). Only the static symbol argument covers them. (2) `effect_gfx_*` (660 KB) is graphics data in a splat `code` segment. `effects.cmake` sets its ROM start to NULL, so effect graphics are never loaded (existing gap, `sharedData->graphics` stays NULL). It is kept for a future fix. See QUESTIONS.md. (3) Some dropped overlays have image data in `.data` subsegments (engine1, engine2, pause, filemenu, battle_code, level_up, starpoint). The port links it from the PNG sources. No ROM read, but if a later slice adds a read by real offset, it gets zeros and logs `ROM READ IN DROPPED RANGE`.
- Check (measured, Azahar, `fast autoa romtrace`, 200 s, full image vs packed): chain hos_05, title, kmr_20, osr_00, kmr_20, mac_00 (area 1 map 1), 0 unmapped, 0 crash, 0 dropped-range lines. Full 58,944 reads (2,891 distinct), packed 59,436. The only differences are 151 BGM reads of 1280 B at timing-dependent offsets. No read with the same address and size has a different checksum.
- RAM (measured by `pm_mem_log`, Azahar): resident ROM 32,519 KB (M01) to 26,933 KB. Heap used at map load 32,789 KB to 27,339 KB. Heap left 12,264 KB to 17,712 KB on Azahar (heap 44 MB). Linear pool 20,480 KB, free about 12.3 to 12.9 MB, texture cache peak 998 KB (new log field `tex-cache`, log line `mem tick` once per minute). Static image (arm-none-eabi-size, now): text 1,232,212 + data 1,289,880 + bss 6,755,840 = 9,277,932 B (grew from 8.84 MB through S15 to S18).
- O3DS layout (measured inputs, ESTIMATED result): the boot line `mem commit limit` shows Azahar offers 98,304 KB and 9,080 KB are used by the image. If the real O3DS limit is 64 MB: heap = 65,536 - 9,080 - 20,480 = 35,976 KB, resident cap = 31,880 KB. The packed image (26,933 KB) fits. Heap left at map load about 8,637 KB (35,976 - 27,339). At M01 the same estimate gave 3,187 KB left and the ROM did not fit. This holds only if the real limit is 64 MB. The first boot line on hardware shows the real value.
- Redundancy audit, step 2 (measured unless marked): resident ROM 26.9 MB still duplicates what dma_copy puts in game buffers. Working set in the run is 1.3 MB (distinct bytes read). Never-read kept bytes: 2.79 MB, of which code 2.45 MB (mostly `effect_gfx_*` and entity model gfx typed code), icon 0.2 MB, title bg 0.13 MB (read in the intro). Texture cache: 998 KB peak of the 4 MB budget, plus 512 KB scratch (linear), no persistent second copy. Audio: 8 x 8 KB linear buffers. No new removal beyond M01 (`fb_mem` 150 KB). Remaining 32 KB `pm_unused_shape_data` needs a shared heaps.c edit, not done.
- Hardware: the O3DS (192.168.1.168) answers ping, but `go.ps1 push o` failed (push_3ds.sh exit 1, no netload). Not retried. Nothing measured on hardware.
- Decision for the user (in QUESTIONS.md): stream the ROM from SD or keep it resident. With 26.9 MB resident the O3DS estimate fits with 8.6 MB spare.

## H02 result (slice H02: hardware-safety diagnostics, port layer only)

All items below belong to slice H02. Checked on Azahar only. Nothing ran on hardware.
- a: `pm_crash_install` puts the crash handler on the game thread and on the audio thread (own 16 KB handler stack each, `pm_audio.c au_main`).
- b: the handler prints thread name, pc, lr, sp, far, fsr, cpsr, build number, r0-r12, a memory line (no mallinfo, safe inside malloc) and a stack walk limited to the recorded stack bounds. Every line goes to log.txt and to crash.txt (append). `pm_log_init` renames the old log.txt to log.old at boot (no O_TRUNC loss of a crash trace). The run script deletes log.txt itself, so log.old appears only on other launches (tested with a copy of the script).
- c: both stacks are pre-filled (0xA5A5A5A5). The `mem` lines log `stack high-water game N of M B, audio N of M B`. Azahar: game 3100 of 261632 B, audio 1788 of 65024 B at the 4th map load.
- d: `pm_rom_init` checks the file, z64 magic (byte-swapped .v64/.n64 gets its own message), size 41943040, and FNV-1a of the first 64 KB (0x0244636C, measured on the ROM with SHA1 3837f44c... from ver/us/checksum.sha1). A full SHA1 of 40 MB was not tried on the ARM11 (it would take seconds). On failure the bottom screen shows the reason, START exits, the game does not run. Tested in Azahar: missing ROM and 1 MB file give the FATAL log line. The bottom screen text was not looked at by eye.
- e: `dspfirm.cdc found/MISSING` log line once. `ndspInit` fails once, logs the result code, audio stays silent, no retry per tick.
- f: checks for linearAlloc (audio buffers, vertex buffer), C3D_Init, save.flash writes (log and keep going). `mem` lines show GPU maxima per frame: draws, vertex slots (of 32768), command buffer percent, vertex drops. A `GPU LIMIT NEAR` warning appears above 90% or on a drop. Azahar autoa: draws up to 244, slots up to 8256, command buffer 3%, 0 drops.
- g: FPSCR logged. Azahar value before: 0x03000010 (FZ and DN already on, round to nearest). The port now sets FZ=1, DN=1, RMode=nearest, traps off, sticky flags cleared, in both threads. Reason: libultra runs the N64 FPU with flush-to-zero, round to nearest, no traps. On Azahar nothing changes except cleared flags. UNVERIFIED on hardware: the real initial FPSCR may differ (the first-boot line `fpscr game before ...` shows it).
- h: `boot_fail` in `pm_mem.c` writes `[BOOT FAIL] ...` to sdmc:/3ds/PaperMario/crash.txt by hand (srvInit, fsInit, no heap) before svcBreak. Tested with a temporary forced failure (not committed): the line reached crash.txt in Azahar.
- i: each vertex batch starts at a 16-byte address (costs at most one 40-byte slot per two batches).
- j: pad press edges are ORed across scans and cleared when the game reads. A press that ends before the read shows as held for that one read. Switch `taptest` injects a 1-scan A tap every 60 scans. Azahar: injected 30, seen 30.
- k: `pm_tex_invalidate` hides a slot used in the current frame from the cache and frees it at the next `pm_tex_begin_frame`. Not exercised by a test (V01 logged 0 in-frame invalidations). `fast autoa` shows no regression. The V01 background change was not re-checked by eye.
- Switches (class E): `crashtest` (undefined instruction on the game thread at frame 300), `crashtest_audio` (same on the audio thread). Azahar stops on a real CPU exception with its own dialog and never calls the guest handler, so these two only make sense on hardware. For Azahar use `crashtest_call` and `crashtest_audio_call`: they call the handler directly with made-up registers. Both wrote full crash.txt content with the right thread name (game, audio) and a stack walk inside the bounds.
- Check (Azahar, `fast autoa`, 230 s): kmr_20, osr_00, kmr_20, mac_00 reached, mac_00 stays up, 0 unmapped-access lines, 0 crash lines.
- Not testable without hardware: real faults reaching the handler, real FPSCR value, dspfirm.cdc absent case on the real DSP, SD write failures, GPU command buffer overflow.

## S01 result: all area_* maps (measured, no code change)

- 28 areas, 2,401 `.c` files, 9.6 MB of `.c` source (10.0 MB with headers).
- Compiled with the CMake flags (-O2, devkitARM, `-c` only): 1,960 files built, 441 failed. The failures are missing generated enums (`ACTOR_LEVEL_*`, `SHADING_*`, `ITEM_FIRST_KEY`, `NPC_SELF`). Those 441 files are 4.0 MB of source. The built files are 6.3 MB of source.
- Built files: .text 538 KB, .data 2,865 KB (the map data is non-const, so it is .data), .rodata 10 KB, .bss 212 KB.
- Extrapolated to all files (x1.64 by source size): .text about 0.9 MB, .data about 4.7 MB, .bss about 0.35 MB. Total about 6 MB. Current 3dsx is 745 KB.
- RAM: `pm_mem.c` splits the 64 MB O3DS FCRAM as 4 MB N64 heap + 40 MB cart image + 20 MB linear. Nothing is free. 6 MB of static image does not fit.
- DECISION: does NOT fit. Do not link all maps. S08 must link only the maps in use, or load map data from the ROM at run time. Linking a subset needs a `gAreas` built from only those maps. The real `gAreas` in `world.c` references all of them.

## L01 result (slice L01: non-blocking log, port layer only)

All items below belong to slice L01. Checked on Azahar only. Nothing ran on hardware.

Question: does logging cause the stutter? Measured before the change (Azahar, `fast autoa logstat`; the switch `logstat` prints a line per 300 drawn frames):
- Rate is low: 0.1 to 0.2 lines per frame, 1 to 6 lines per second, 175 lines in a 100 s autoa run. Normal run without `fast autoa`: 1 to 3 lines per second. The rate is NOT the problem.
- The cost per line is the problem. The old pm_log did write + fsync + socket + stdout in the calling thread. In Azahar one line cost about 12 ms (frame work was 1 to 1.5 ms lower per frame with `lognull`, which drops every line, at about 0.1 lines per frame). With `logflood` (10000 lines per frame) the old code wrote only 2340 lines in 60 s and frame 1 never ended. The real SD fsync cost is not known. Azahar cannot show it. A stutter of a few lines per second fits the numbers, but it is NOT proven on hardware.
- Noisiest call sites (count in a 100 s run): `stats %s` 39 (pm_gbi.c), `audio cost` 23 and `audio posts` 23 (every 2 s), `game mode` 11, mem lines 3 each. No per-frame line exists.

After the change (same scene, same run, `fast autoa logstat`): frame work 22.6, 34.3, 35.6, 42.4, 31.4, 29.8 ms (before: 24.8, 35.4, 36.2, 43.5, 34.6, 31.3 ms). That equals the `lognull` run. Wall time per frame 25.4, 41.1, 39.9, 47.9 ms (before: 28.2, 42.6, 41.5, 50.4 ms). With `logflood` (10000 lines per frame): wall 30.4, 47.3, 47.4 ms. So 10000 calls cost 4 to 7 ms per frame in Azahar (the queue is full, each call is one atomic add and a return). The frame time is NOT unchanged by 10000 calls per frame, but the game is not blocked either. The game reached frame 1061 in 70 s and the log has `N lines dropped` lines. Azahar runs both ARM cores on host threads that may share work, so some of the 4 to 7 ms may be writer work. Not separated.

What changed (3ds/src/pm_log.c and small edits):
- pm_log formats the line and copies it into one of 512 slots (184 bytes each, 96 KB in .bss). Lock-free: reserve with `__atomic_compare_exchange_n` on the head index, commit with a release store, the writer reads with acquire. Full queue: one atomic add on a drop counter and return, before any formatting. The writer logs `N lines dropped (queue full)`.
- One writer thread (libctru `threadCreate`, 32 KB stack, priority 0x3F) waits on a `LightEvent` with a 250 ms timeout. Producers signal it when the queue was empty or half full. It writes log.txt in batches, calls fsync about once per second, sends to the live socket, prints to stdout and accepts live-log clients. No busy wait.
- Core: `APT_SetAppCpuTimeLimit(30)`, then the thread on core 1 (libctru 2.7.0 `thread.h`: core 1 allows one thread after that call). If that fails, the thread goes on core 0 at the same low priority. The boot line `log: writer id 1 core 1 ... thread ran 1` proves the thread ran (a start flag, not the `LightEvent_WaitTimeout` result, which returned 1 although the event was set). The minute line `log writer core N heartbeat H dropped D` and the `logstat` line show the heartbeat. In Azahar the thread runs on core 1 (heartbeat about 4 per second). `svcGetProcessorID` returns a wrong number for the game thread in Azahar (for example 19 or 8020348). Trust it only on hardware.
- Before the writer exists (first boot lines), pm_log writes directly as before. A crash: `pm_crash_line` calls `crash_flush` once (takes the drain lock, writes the queue, fsync). Test `crashtest_call`: log.txt holds all queued lines and the 8 `[CRASH]` lines, crash.txt appends them. The log.txt to log.old rotation and crash.txt from H02 are unchanged. `pm_log_stop` (end of `main`) writes what is left.
- Console: stdout is the bottom screen console, which is not thread safe. The first test run gave 23465 unmapped accesses in `con_write` because the writer printed while the HUD changed the console window. Fix: `pm_con_lock` (a `LightLock` in pm_log.c) around the writer's print, the console setup and the FATAL screen. The HUD (`pm_gpu_hud`) uses `LightLock_TryLock` and skips one update when the writer is printing, so the game thread never waits for the writer.
- Noisy lines behind switches: the `stats` line only with switch `stats`. `audio posts` only with switch `audiostat`. `audio cost` every 20 s by default (S28 reads it), every 2 s with `audiostat`. Kept: `mem tick`, `load map`, `scan N`, the `rom_trace_check.py` inputs, `rmlog`, FATAL and crash lines.
- New switches: `logstat` (line per 300 drawn frames: work ms, wall ms, lines per frame and per second, writer heartbeat, drops, top 8 call sites every 1200 frames; it also goes to the Azahar debug log through `svcOutputDebugString`, read `%APPDATA%\Azahar\log\azahar_log.txt`), `lognull` (drop all lines except errors, for A/B), `logflood` (10000 lines per frame).

What Azahar does not show:
- The real SD write and fsync cost on hardware. The writer takes it now. An SD stall of more than the queue holds (512 lines) would drop lines in a busy scene.
- Core 1 behaviour on the O3DS (whether APT_SetAppCpuTimeLimit lets the thread run). The core 0 fallback exists but was not run.
- The live log. Azahar has no network buffer on this run path (`started_by_netload` is false, a forced test with a heap buffer gave `socInit e0e01bf5`). The socket send path and `livelog.py` were NOT run. Code read only. `livelog.py` reads `#seq` numbers. The number is taken after the queue slot, so the queue order equals the number order. Dropped lines make no gap, the writer adds one numbered `lines dropped` line. `go.ps1 pull o|n` and `ftppull.py` read log.txt over FTP. The file format is unchanged. The newest lines come about 250 ms late, and fsync runs once per second.
- The writer thread has no crash handler of its own (a fault there gets no trace).

## Port threading rule and note for the audio slice

Lighter checks (user rule 2026-10, applies to all later slices): after a fix the regression check is (1) the smoke check on the presets yard and toad with -Turbo -Smoke (about 16 s each), (2) shots of only the scenes the change plausibly touches, (3) NO 4-scene A/B and NO full tour, unless the slice touches the map change path, boot, title, file select or the intro. A by-eye A/B with a `nocull`-style switch stays for proving that the symptom itself is fixed.

Port threading and sync use libctru natives (threadCreate, LightLock, LightEvent, LightSemaphore, svc handles). No hand-rolled locks or sleeps in loops. For the later audio slice: the cooperative audio thread hand-off in `3ds/src/pm_os.c` (`pm_au_thread_yield`) and the `au_run` and `au_idle` semaphores in `3ds/src/pm_audio.c` use custom yield logic. Review them against this rule then. L01 did not change them.

## V02 paused (slice V02, paused for L01, no code changed)

Found so far (slice V02, Azahar only):
- `fast autoa shots freshsave` reaches the Mario house yard (kmr_20, shots 11 to 29) and the yard after the letter (shots 44 to 53). Sheet 3ds/logs/v02_s0.png (git-ignored). By eye in the default state: the bushes sit in front of the hazy hills and the tree trunk is in front of the haze. I did NOT see the wall over the bushes. The same shots with `rmoff` (shot 50) look the same, except a dark square under the Mario sprite and Luigi with `rmoff` (old always-write depth).
- `rmlog` kmr_20 draw states (frame 301 onward): first zb1 draws with oml 0c192078 and 00552078 (Z_CMP and Z_UPD on, depth w 400 to 1200), then a run of draws with geometry zb 0 and oml 00552048 or 00553048 (Z_CMP 0, Z_UPD 0, w 750 to 1100), then zb1 draws again. Draws with zb 0 and no Z_CMP skip the depth test in the port, so they cover earlier depth draws. The N64 does the same, so the draw order of the display list decides. Not yet shown: which of these draws is the panorama and which are the bushes.
- Not verified: the report may be a hardware only effect (depth precision on PICA, decal bias 0.001 below float24 resolution), or a different scene (mac_00 background, hos_05 house).
Next step: log per draw (model name or vertex address) in the zb 0 run of kmr_20 frame 301 and check whether the bushes are in that run. Test the decal bias (`C3D_DepthMap` in `pm_gpu.c` flush) with a larger value. Check mac_00 shots.
Commands: `powershell -File 3ds/run_azahar.ps1 -Debug "fast autoa freshsave rmlog" -Timeout 200 -Until "area 1 map 1"` (rmlog lines `rm f<frame>`, logged on frames where frame mod 30 is 1, only on a state change). A/B: add `rmoff`. Shots: `fast autoa shots freshsave`, then `python build3ds/sheet.py 1 0 54 out.png`.

## V02 result (slice V02, NOT REPRODUCED, no code changed)

Checked in slice V02 on Azahar, on current main (after S26, S27, V01, PO1, PT01):
- `fast autoa shots freshsave` (54 shots): kmr_20 yard before the letter (shots 11 to 29), the house interior, the letter, the yard after the letter (44 to 53). Shots 15 and 49 looked at full size (rotated, 2x). Bushes sit in front of the hazy hills. Tree trunk sits in front of the haze. No back wall over the bushes at any camera position.
- `fast shots` (106 shots): the hos_05 intro (star ship, palace, Bowser). No Mario house in the first 106 shots. The house scene in the intro comes later and was not reached with shots.
- `fast autoa shots freshsave` to 200 shots: mac_00 (Toad Town) idle view and osr_00. No bush or wall overlap seen.
- Not done: no per-draw `rmlog` of panorama and bush draws, because there was nothing to find. Depth state was not changed.
Question for the user is in 3ds/QUESTIONS.md. Possible remaining cause: a hardware only effect (depth precision on PICA) that Azahar does not show.

## Open issues (carry over until fixed)

1. kkj_00 is not reached by the intro chain. See above. Toad Town (mac_00) and every other map also fail to load. The generic map table fixes both.
2. FIXED by S15 (slice S15): sprite shading renders. See the S15 result section. No linked map enables shading yet (all call SHADING_NONE), so it shows only when a map with a real profile is linked.
3. Text box transitions do not happen. Parakarry's first text box does not show.
4. FIXED by S26 (slice S26): translucent draws wrote depth and hid geometry behind them. See the S26 result section.
5. The Mario logo does not show on the house in the intro cutscene.
6. FIXED by S13 (real curtains.c): the red cover is gone in the intro (seen in shots).
7. FIXED by S26 (slice S26): same cause as issue 4. Mario's sprite no longer has a black rectangle that hides the scene.

- V02 FIXED (see "V02 result 2" at the end, back-face culling): at Mario's house (hos_05 intro or the Mario house scene) the back wall of the panorama renders over the bushes in the background (a side of the map renders over level geometry). Hypothesis, unverified: draw order or depth state. S26 made depth compare (Z_CMP), depth update (Z_UPD), decal and blend follow G_SETOTHERMODE_L in pm_gbi.c and pm_gpu.c. The panorama model may use a render mode that the port now handles wrongly (depth compare off, depth write on, or a draw sorted after the bushes). The PICA depth range or depth bias (decal bias 0.001 is below float24 resolution) may also cause it. Method (slice V02): use the `rmlog` switch (S26) to log othermode, z mode and draw order for the panorama and bush draws in that scene. Compare with `rmoff`. Compare with the reference interpreter (reference/soh-3ds libultraship fast/interpreter.cpp). Shots before and after. Fix in the platform layer.
- V03 (user-reported, unverified): some text renders as white rectangles in the intro cutscene. Hypothesis, unverified: a regression from S26 (I4/I8 textures changed from L8 to LA8, blend and alpha test now follow the render mode, constant-colour combiner mode) or S27 (tile 1 binding, combine mode 5, edge padding of non-power-of-two textures). A glyph texture drawn with wrong alpha, wrong combiner or the wrong tile shows as a solid box. Also possible: the narrator or message font textures (charset, IA4/I4/CI4 with palette) not decoded for a size or format, or the text path (msg.c with draw_image_with_clipping, or narrator code in hos_05/narrator.c) uses a render mode the port handles wrong. Method (slice V03): identify which text (narrator, message box, titles). Log the render state per text draw with `rmlog`. Compare with `rmoff`. Check the texture format and decoded content (dump a glyph texture under 3ds/logs/). Shots before and after. Fix in the platform layer.

Known gaps: item and popup APIs, entity display lists and shadows, `bed.c` and `records.c` (need extracted textures), message boxes, status bar, HUD.

Debug switches in `debug3ds.txt` or `-Debug "..."`: `fast`, `autoa`, `shots` (frame every 30 to `sdmc/3ds/PaperMario/shots/*.ppm`, `build3ds/sheet.py step start count out.png` makes sheets), `scripts` (live script dump every 120 frames), `player`. Use `fast autoa shots` to reach kmr_20 with frames. Do not use `shots` with `autoa` to watch the intro. gdb: `-Gdb -GdbScript <file>`. Per-frame gdb `commands` logging did not work, and an async `interrupt` in a gdb script did not stop the target. Use game-side logging.

## V03 result (slice V03: white boxes behind narrator text, DONE)

Platform layer only (`pm_tex.c`), no game code changed.
- Reproduced (slice V03, Azahar, `fast shots`, 106 shots, needs `-Stall 120`): at the Bowser silhouette scene (shots 57 to 66) the narrator glyph quads draw as solid white boxes. Earlier narrator lines are fine. It is not S26 or S27: the render mode, combiner and palette at the glyph draws were identical in good and bad text (temporary texrect log: CI4 16x16, palette 16 entries, `oml 0f0a7008`, TEXEL0 decal).
- Cause: the log line `tex cache full 16x16`. `pm_tex.c` had 128 slots. The silhouette draws many background strips (296x6 and 264x7 CI8 tiles) in the same frame, so all 128 slots were in use this frame and could not be evicted. The glyph load failed, no texture was bound, and the quad drew white.
- Fix: `SLOTS` 128 to 512, and every scan stops at `slot_hi` (highest live slot + 1), so the cost stays that of the live count. After: 0 `cache full` lines, text reads correctly in shots 56 to 90 ("Ha ha ha! Yeah! I did!", "So I can, at long last, beat my archenemy, Mario.", "I'll take this Star Rod!").
- Seen by eye: intro sheet (28 frames) unchanged otherwise (storybook, palace, Kammy, Bowser). The olive square in shot 76 is still there (S25 combiner, known). `fast autoa` 6 loads, kmr_20, osr_00, kmr_20, mac_00 reached, 0 unmapped, 0 crash. Not checked by eye: file select, shadows, Mario sprites in new shots (not part of this change; only cache capacity changed).
- Limit: a frame with more than 512 distinct textures still fails the same way (the `tex cache full` line still logs once).

## Next

After the stub cleanup: boot a file and progress instead of sitting on the first frame. Use the real `state_map_transitions.c` / `state_file_select.c` flow, file load into `gCurrentSaveFile` (needs `fio.c` and a save in SD), and the real `GotoMap`. Then work open issues 2 to 7. Then verify input moves Mario.

## Next after the stub list is empty (performance)

Do not start before `3ds/SLICES.md` is done. Order:
1. Add a per-frame phase timer: game logic, GBI transform and clip, GPU submit, texture load. Measure on real O3DS (`/push-o`), not Azahar. DONE on Azahar in slice PT01 (switch `phase`, see the PT01 result). Hardware run still open.
2. If transform and clip dominate, transform each vertex once at `G_VTX`. `draw_tri` in `pm_gbi.c` now transforms per triangle corner. CPU only. Small change.
3. Then move the transform to the GPU: raw `Vtx` as short attributes, `u8` index list, `C3D_DrawElements`, combined matrix in the `projection[4]` uniform of `shaders/pm.v.pica`. Risks: hardware near-plane clipping (unverified), float24 precision, lighting and fog vertices, the `note_door` hack, `Vtx` endianness, small batches (one flush per matrix change).
4. Last, optional: pre-baked native assets. Offline PC bake tool, SD cache, fall back to runtime conversion on a miss. Textures first (`pm_tex.c` cache), static-model vertex buffers second. Keep the N64 data as the source of truth.

## Performance phase (after everything works)

Do not start before the slice list is done. User decisions (2026-10) are marked DECISION. Unverified facts are marked UNVERIFIED.
- DECISION: the target is the Old 3DS (O3DS) at full game speed. The N3DS is not the target. The game does not run at full speed in Azahar today, so general optimization is needed later. This is not measured yet. First add a per-frame phase timer (logic, GBI transform and clip, GPU submit, texture load, audio) and measure on real hardware.
- DECISION, workload split: Core 0 = game logic plus the CPU part of the RSP work. Core 1 = audio logic. PICA GPU = rasterization plus the RSP vertex work that suits it (transform in the vertex shader, indexed draws).
- DECISION, principle: use the proper hardware for each task. For audio, consider the 3DS hardware DSP through ndsp (per-voice mixing and resampling on the DSP) as an alternative to a faster CPU mixer (fixed point or ARMv6 SIMD, the ARM11 has no NEON). UNVERIFIED: N64 VADPCM differs from DSP ADPCM, so samples would need transcoding at load time.
- Facts to check before moving audio to core 1 (all UNVERIFIED): the O3DS app share of core 1 is capped (about 30% as recalled from general knowledge, verify). Today the audio thread may share the game core (cooperative, verify the core id in threadCreate). The port's osSendMesg/osRecvMesg, nuPiReadRom (shared FILE) and pm_log are not thread safe and would need locks. Lowering audio quality needs the user's approval.
- Audio and general optimization are separate later slices. Keep the list in the previous section (vertex transform once at G_VTX, GPU transform with indexed draws, native pre-baked assets last).
- ARM11 math audit (user request). Scan the game math and port math for operations that are slow on the ARM11 and replace them with cheaper equivalents where safe. Facts, general 3DS knowledge, UNVERIFIED until measured: no hardware integer divide (calls to __aeabi_idiv, __aeabi_uidiv, __aeabi_idivmod cost tens of cycles). VFP11 float divide and sqrt are slow and block the pipeline (about 15 to 20 cycles). No NEON, only VFP and ARMv6 integer SIMD. libm sinf, cosf, atan2f, powf, fmodf are slow (the decomp already uses sin_deg and cos_deg tables). Method: (1) list every call site of those routines in the linked binary with arm-none-eabi-objdump and nm. Rank them by how often they run using the per-frame phase timer and sampled stack traces, not by reading code. (2) Safe rewrites: shift for power-of-two divides, one reciprocal then multiplies for repeated divides by the same value, fixed point where precision allows. Start in the port layer (pm_gbi.c, pm_gpu.c, pm_tex.c, pm_rsp_audio.c). (3) Compare outputs before and after. The decomp is a matching decomp and its logic (physics, collision) relies on exact float results. A reciprocal multiply rounds differently than a divide. Do not use -ffast-math without a test (it changes NaN and denormal behaviour). DECISION: do not get stuck on core math that could break things, but experiment where possible. Guardrails: (a) each optimized path sits behind a compile-time or debug switch, original kept beside it, so A/B and revert are cheap. (b) Replay check: record a fixed input sequence (the autoa chain), log a hash of key game state every N frames (player position and velocity, NPC positions, script variables), run original and optimized math. Identical hashes mean no behaviour change. Drift shows which change and by how much. (c) Risky experiments go on their own local branch. Merge to main only if the replay check passes or the user approves a known tiny difference. Port-layer math (graphics, audio) needs only the visual or audio comparison. Game-logic math needs the replay check.

## Slice workflow

`3ds/SLICES.md` is the queue. One subagent does one slice with a fresh context. The orchestrator reads only the first open line and any `BLOCKED:` line here.

The orchestrator handoff, the agent prompt template and the slice queue order are in `3ds/ORCHESTRATOR.md`.

## Reference ports (slice S07 note)

Every agent may read the ports in `reference/` (see `reference/README.md`) to learn how a feature works. Read `reference/paperboat` first (Paper Mario PC port, runs the decomp loader). Use `reference/soh-3ds` and `reference/sm64-3ds` for 3DS GPU, audio and memory. Read to learn the method, then build the real decomp file here. Do not copy a hand-written stand-in from a reference port. If a stand-in is the only way to match, write `BLOCKED:` and ask.

## Known stand-ins added or recorded by S07

- S07, new: `load_asset_by_name` in `3ds/src/pm_map.c` is a port-side copy of the real `load_asset_by_name` in `src/world/world.c` (line 268). Reason: the real `background.c` `load_map_bg` calls it, and `world.c` is S08 (needs `gAreas`). The port copy uses the port's own `find_asset`, which byte-swaps the asset table. Paperboat does not use a stand-in. It builds the real `src/world/world.c` copy of the function. So the way to use the real one is to build `world.c` (S08). Remove the port copy in S08.
- S09, recorded by S07: the `world_partner_*` and `world_use_item_ROM_*` symbols are `--defsym` aliases that point at `gPartnerStatus`. This is a placeholder from S09. It is for CODE only (MIPS overlays that the ARM build links in place), so `dma_copy` copies nothing and nothing is read. Slice P01 checked this: no data symbol uses the placeholder (see the P01 result).

## Do not

Regression check (user decision 2026-10): feature testing uses the warp (preset or warp=map:entry) with -Turbo, plus padscript where input is needed. Before sign-off, a full pass through the real flow (fast autoa freshsave from boot) is done only if the feature needs it, meaning the slice touches boot, title, file select, the intro or the map change path. Features that do not need it sign off on the warp checks alone. Say in the slice result which kind of check was used.

Port threading and sync use libctru natives (threadCreate, LightLock, LightEvent, LightSemaphore, svc handles). No hand-rolled locks or sleeps in loops.

Tours: iterate on the failing set, full tour once at the end of a slice. Do not commit logs, traces or shots. Write them under `3ds/logs/` (ignored by git). Only `3ds/PROGRESS.log` is tracked.
Do not push. Do not stub (see the rule at the top). One Azahar window; the run script kills only the process it started. Do not leave `fast` in `debug3ds.txt` (it skips the logos). `3ds/run_azahar.ps1` stops on `-Until`, a crash, log silence (`-Stall`), or `-Timeout`. Use `run_azahar.ps1` directly, not `go.ps1`, to see the unmapped-access report. `go.ps1` skips the build when the 3dsx is newer than all sources, even after a `CMakeLists.txt` change. Touch a source file to force a build. The scene after the third kmr_20 load is idle, so the log goes quiet. That is not a hang. The full intro needs about 150s. Log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt`. Window captures do not show the OpenGL screen. GPU work stays on the GPU. Paperboat (`reference/paperboat`) shares the game-side sprite and imgfx code, and runs the unmodified decomp loader. The Fast3D interpreter to compare against is `reference/soh-3ds/third_party/libultraship/src/fast/interpreter.cpp`.

## S10 result

Done (slice S10). Real `src/world/surfaces.c` and `src/world/menus.c` build and link. `src/8a860_len_3f30.c` was already built in S08. Removed stand-ins: `handle_floor_behavior`, `func_800F0C9C`, `check_input_open_menus`, `check_input_status_bar` (pm_actor_stubs.c), `clear_world_menus` (pm_kmr_stubs.c). No stand-in for `create/hide/destroy_popup_menu` remained. The other stub files hold only symbols of unbuilt slices (battle, pause, entity render, state_*), so nothing else is replaceable now. `fast autoa` (200 s, slice S10 check): kmr_20, osr_00, kmr_20, mac_00 (area 1 map 1) loads and stays up, no unmapped-access warning.

## S15 result (slice S15: sprite shading)

Done (slice S15). Three causes, all fixed in the platform layer or by the same endian pattern as S12.
- Cause 1 (slice S15): no linked map enables shading. kmr_20, hos_05, osr_00, mac_00, kmr_02 and kkj_00 all call `SetSpriteShading, SHADING_NONE`, which returns early. So Luigi and Parakarry in the intro were NOT shaded and look normal in the before shots. The dark characters only appeared once a profile was forced on.
- Cause 2 (slice S15): `src/evt/demo_api.c` `SetSpriteShading` read two ROM words (`ShadingOffsetsBuffer`) without a byte swap, so the whole profile came out zero (all-black sprites). Added `__builtin_bswap32` under `TARGET_3DS` (same pattern as S12 audio).
- Cause 3 (slice S15): `pm_gbi.c` ignored `G_SETCIMG`, so the 16x1 palette rect of `appendGfx_shading_palette` drew on the screen and `SpriteShadingPalette` stayed unwritten. The sprite draw uses two tiles (tile 0 = shading palette, tile 1 = sprite palette, same 4-bit indices, shading tile shifted by the light offset, first combiner cycle TEXEL0*TEXEL1).
- What `pm_gbi.c` does now (platform layer, no game logic, the real display lists run unchanged): tracks the color image, TMEM address and palette per tile, the LOADTLUT palette slot, env color, scissor and both combiner cycles. A texrect into an off-screen color image sourced from a TLUT in TMEM runs in software with the real combiner inputs and writes RGBA5551 into the palette array (screen color images are unchanged, `pm_is_cfb` in `pm_main.c`). When tile 1 holds a 4-bit image and tile 0 is set with a different palette and the combiner is TEXEL0*TEXEL1, `load_shaded_sprite` builds the 16x16 result table from the combiner and `pm_tex_load_shaded` (pm_tex.c) bakes one RGBA8 texture. Cache key includes a hash of the table and the offset. The second cycle `(SHADE-ENV)*COMBINED+COMBINED` can exceed 1, so the vertex color carries (SHADE-ENV+1)/2 and `pm_gpu_set_rgb_scale(2)` sets the PICA texenv RGB scale to 2.
- Not emulated: bilinear filter of the shading lookup (nearest, offset rounded to a quarter texel), the `SPR_SHADING_FLAG_SET_VIEWPORT` extra pass (flag is "never set"), shaded imgfx meshes (`imgfx.c` paths untested), TEXEL0*TEXEL1 combines of other draws (restricted to TEXEL0 and TEXEL1 in a and c).
- Check (Azahar, slice S15): `fast autoa shots`, 152 frames from the unshaded intro/kmr_20/mac_00, pixel-identical to the before run (no regression). With a temporary hook (NOT committed) that ran the real `SetSpriteShading(SHADING_HOS_01)` every 30 frames, Luigi, Mario, Toad drew with the profile's blue tint, not dark and not black. Before the endian fix the same hook gave black silhouettes. Seen by eye on the sheet, not measured against an N64 capture. `fast autoa` (220 s): kmr_20, osr_00, kmr_20, mac_00 reached and stable, no unmapped access, no crash.
- Follow-up: link a map that has a real shading profile (hos_01, kmr_30) to see it in play.

## S16 result
Done. S08 had already removed the hand-written hos_05 scripts. S16 grep: no hos_05 script in 3ds/src. Real `star_ship.c` is in CMake and links. `3ds/src/pm_evt_stub.c` held only an unused `STUB` macro and `skip_call`, so S16 deleted it and its CMake entry. `fast autoa` run (S16): kmr_20 reached, mac_00 loads and stays up, no unmapped access, no crash line.

## S18 result (slice S18: PARTIAL, pause done, demo blocked on S19)

- Slice S18 built the real `src/state_pause.c`, `src/pause/pause_{main,tabs,stats,badges,items,partners,spirits,map,gfx,styles}.c`, `src/menu_hud_scripts.c` and `src/os/nusys/nupireadromoverlay.c`. Removed from `pm_later.c`: `state_init/step/drawUI_pause` and `state_init/step/drawUI_unpause`.
- Overlays are linked in, so `pause_*` and `ui_images_filemenu_pause_*` ROM ranges are `--defsym` aliases of `gPartnerStatus` (empty range, nothing copied). Same placeholder pattern as the partners, listed as a stand-in. Declared in `3ds/stub_include/ld_addrs.h`. Pause files get `-include common.h`.
- `pause_map.c` includes `world_map.inc.c`. Slice S18 made it with `python tools/build/world_map.py src/world_map.xml ver/us/build/include/world_map.inc.c` (git-ignored). `extract_assets.ps1` does not make it yet: add that command.
- `3ds/tools/dedup_stubs.py` now skips files that do not exist.
- Game over: already real since S09 (`state_map_transitions.c`), no stub left in `pm_later.c`.
- BLOCKED part (slice S18b): `src/state_demo.c` needs `load_demo_battle` (`src/battle/battle.c`, pulls the whole battle tree, S19). Demo stand-ins `state_init/step/drawUI_demo` stay in `pm_later.c`. Queued in QUESTIONS.md.
- Stand-ins left for S17: `state_*_language_select`, `state_*_exit_language_select`, `state_*_exit_file_select` in `pm_later.c`, plus `state_step_file_select` and `state_init_world` in `pm_title.c`. For S19: `state_*_battle`, `state_*_end_battle`, `reset_battle_status`. `state_drawUI_world` is also still a stand-in (`state_world.c`, S17).
- Check (slice S18, Azahar `fast autoa`, 220 s): kmr_20 reached (twice), mac_00 loaded and stayed up, 0 unmapped-access or crash lines. The pause screen was NOT seen: autoa never opens it. It is linked only. Overlay `pause_init` and the `D_80200000` aux cache symbol were not run. `D_80200000` resolved at link, so no stand-in was added for it.

## P01 result (slice P01: ROM-symbol placeholders)

Done, no code change except a CMake comment. Slice P01 classified all 47 `--defsym ... =gPartnerStatus` lines with `ver/us/splat.yaml` and the real users.
- Code (never read): 11 `world_partner_*` (ROM_START, ROM_END, VRAM), `world_use_item_ROM_*`, `pause_*` (ROM, VRAM, TEXT, DATA, RODATA, BSS). `dma_copy` and `nuPiReadRomOverlay` get length 0, so no read and `bzero` of 0 bytes.
- Data: only `ui_images_filemenu_pause_*` (splat `type: code`, `.data` images). The images are linked in place by `INCLUDE_IMG` in the real `src/menu_hud_scripts.c`, and the HUD scripts name them by symbol. `pause_init` and `filemenu_init` call `DMA_COPY_SEGMENT` on it with length 0. So the placeholder is correct. A real ROM offset would be wrong: `dma_copy` would copy ROM bytes over gPartnerStatus (the VRAM target). Data symbols read from ROM: 0. Data symbols fixed: 0.
- Why not a script: the partner symbols are used as `&symbol` (relocation), and 3dsxtool rejects a small absolute value there. The placeholder must be an in-image address. A generated list would give the same bytes.
- M01 remap: no range change needed, nothing reads these ranges. Check (Azahar, `fast autoa romtrace`, 230 s, slice P01): kmr_20, osr_00, kmr_20, mac_00 reached, 61,484 reads (3,168 distinct), 0 in dropped ranges (`rom_trace_check.py`), 0 unmapped or crash lines.
- The pause screen was not seen: no input script reaches it. Only the read path was checked.

## S19 result (slice S19: battle tree, DONE ON BRANCH `s19-full-battle`, NOW MERGED into main by slice M03, see the M03 result below)

All code for slices S19 and S18b is on the local branch `s19-full-battle` (one commit on top of main). Main only has the notes. Reason: the full battle tree is over the ceiling in the task (about 3 MB, about 150 files). Merge with `git merge s19-full-battle` when the question in 3ds/QUESTIONS.md is answered.
- What links (slice S19): the whole `src/battle` tree, 621 `.c` files (all `.inc.c` are included by those), plus `src/state_battle.c`, `src/state_demo.c`, `src/battle_ui_gfx.c`, `src/level_up.c`, `src/starpoint.c`, `src/entity/model/PowBlock.c`. That is 627 new files. List in `3ds/battle.cmake` (GLOB, same exclusions as paperboat for region files). No missing symbol. No duplicate symbol.
- Why all of it: real `src/battle/battle.c` has `gBattleAreas[]`. It names `b_area_<x>_Formations` and `_Stages` of all 47 battle areas. Every formation names its actors. So one real `battle.c` pulls every area, actor, stage and move. A smaller set needs a port-side `gBattleAreas` (like `area_table.c` for `gAreas`). That is a data stand-in, so it was not made. See the question.
- Size (measured, `arm-none-eabi-size` on the ELF): before (M01) .text 1.12 MB, .data 1.11 MB, .bss 6.59 MB, image 8.84 MB. After: .text 1,748,020, .data 4,453,400, .bss 6,542,448 B, image 12,743,868 B (12.15 MiB). Added about 3.9 MB. 3dsx 2.29 MB to 6.59 MB. Split of the 627 objects (text+data): core 29 files 0.5 MB, moves 76 files 0.54 MB, partners 11 files 0.15 MB, action commands 24 files 0.06 MB, battle areas 486 files 2.83 MB (about 100 KB each for the largest areas: flo2, trd_part_2, flo, kmr_part_3, tik, kpa2, kpa, mac).
- RAM (Azahar, `pm_mem_log`): heap used and sbrk-left are the same as at M01 (used 32,789 KB at boot, left 12,264 KB). Azahar has a fixed 44 MB heap, so the image does not show there. On a real O3DS the image counts against the 64 MB (estimated, not run on hardware): about 3.9 MB more.
- Removed stand-ins: `load_battle`, `set_battle_formation`, `set_battle_stage` (pm_actor_stubs.c), `reset_battle_status`, `state_init/step/drawUI_battle`, `state_*_end_battle` (pm_later.c), `heap_battleHead` (now `--defsym heap_battleHead=gFrameBuf2`, as in `ver/us/undefined_syms.txt`), `gBattleStatus` (game_stubs.c). `BattleEntityHeapBottom` was removed earlier (S12). Removed stale shadow headers in `3ds/stub_include`: `battle/actor_types.h`, `sprite/*` (player.h, npc/*.h, sprite_shading_profiles.h). The generated headers in `ver/us/build/include` now apply.
- Port-layer additions (no game logic): `3ds/tools/gen_battle_ld.py` writes `3ds/stub_include/battle_ld_addrs.h` (empty ROM/VRAM ranges, 159 names: battle_code, btl_states_menus, battle_partner_*, battle_move_* and the item, star power, level_up and starpoint segments, action_cmd_*, battle_area_*, world_model_anim_kzn_*). All battle code overlays are linked in, so `dma_copy(1,1,1)` copies nothing. `shim_battle_heap_create_obfuscated` in `game_stubs.c` calls the real `battle_heap_create` (same pattern as the existing general-heap shim). Debug switches in `pm_title.c state_step_world`: `battle` (load_battle after 600 world frames) and `demobattle` (enter the real demo state at scene 1). `pm_map.c`: switch `assets` logs each `load_asset_by_name`.
- GAP: kzn2 (Lava Piranha) has a `dmaTable` of `world_model_anim_kzn_NN` ranges. Those are code files `src/world/model_anim/kzn/*.c` that the real game loads over each other. They are not linked, the ranges are empty. The Lava Piranha animation is missing until those files are linked.
- Real decomp bug seen: stage `kmr_02` names the asset `kmr_bt02_shape`, which does not exist (the source says so). A battle on `BTL_KMR_1_STAGE_00` reads past the asset table. Do not use that stage in a test.
- Check (slice S19, Azahar): `fast autoa romtrace` 230 s: kmr_20, osr_00, kmr_20, mac_00 reached, 0 unmapped-access lines, 0 crash lines, 0 `ROM READ IN DROPPED RANGE` lines. The same with the `battle` switch: 0 dropped-range lines (the battle reads no dropped ROM).
- Battle seen by eye (`fast autoa battle shots`, sheet of the shots): the real battle starts. The kmr_03 stage draws, Mario and a Goomba, the name tag, the Jump command menu and the "2 STAR POINTS!" victory screen. autoa presses A, so the battle plays out. Bad draws: black and olive polygons during the attack animation and a few black bars. Those come from Fast3D features in `pm_gbi.c` that are not done yet (not diagnosed, so no feature name is known). The battle ends, then the game goes back to the map chain. With the debug hook, the log shows `assert i < MAX_NPCS` at battle start, and later an unmapped-access report in `update_encounters_neutral` (`get_npc_unsafe` of a world enemy). This comes from the hook: it calls `load_battle` without the real encounter state. A real encounter start was not run. Plain `fast autoa` (no hook) has 0 unmapped lines.
- Not checked: any other battle area, partner moves, items, star powers, level up screen, game over, flee. Only one Goomba battle was seen.

## S18b result (slice S18b: demo state, DONE ON BRANCH `s19-full-battle`)

Real `src/state_demo.c` links. The `state_init/step/drawUI_demo` stand-ins are removed from `pm_later.c`. Seen by eye (`fast autoa demobattle shots`): the demo battle scene starts (dig stage, Mario with hammer, Fuzzies, hammer first strike). The Fuzzy sprites look dark. After it, the demo goes to scene 2 (`isk_04`), which is not linked: `load refused: area 11 map 3 is not linked`. Then the screen stays black and `update_player_shadow` reports unmapped accesses (no player shadow after the refused load). The full demo needs those maps (`nok_12`, `isk_04`, `tik_03`, `trd_09`, `jan_04` and more). The real game enters the demo from the title timer. The port title does not do that, so only the debug switch reaches it.

## S17 result (slice S17: real title, file select, world state, fio, filemenu, DONE on main)

Slice S17 built the real `src/state_title_screen.c`, `src/state_file_select.c` (also the US language select and exit states), `src/state_world.c`, and `src/filemenu/filemenu_{common,main,yesno,info,createfile,gfx,msg,styles}.c` (`filemenu_selectlanguage.c` is PAL only, not built). `src/fio.c` was already built. Deleted: `3ds/src/pm_title.c` (all of it: port title, file select, `state_init_world`, `state_step_world`, logo and press-start drawing, `gSaveSlotMetadata` storage, the hand-made new-file setup, `pm_map_loaded`), the language select, exit language select, exit file select and `state_drawUI_world` stand-ins in `pm_later.c`, and `IntroMessageIdx` in `game_stubs.c`. The real new-file code (`filemenu_yesno.c` FM_CONFIRM_CREATE) now does the D01 setup (clear_player_data, clear_saved_variables, STORY_INTRO). `fio_init_flash` and `fio_load_globals` are the real ones.
- Port-layer pieces (slice S17, no game logic): (1) `osFlash*` in `pm_os.c` now back a 128 KB chip (128 byte pages, 16 KB sectors, erased = 0xFF) with `save.flash` in `sdmc:/3ds/PaperMario/`. Slot n sits in sector n, slots 6 and 7 are the globals. (2) `filemenu_*` ROM ranges are `--defsym` aliases of `gPartnerStatus` (empty range, the overlay is linked in place, same pattern as pause, see P01). (3) `decode_yay0` swaps the 4 header words of `title_data` (big-endian offsets read in place). (4) `pm_map.c` logs `game mode N` on each `set_game_mode`.
- Debug switch `freshsave`: start with an erased chip (new file run). Without it, an existing `save.flash` is loaded. Delete `save.flash` for the same result.
- `autoa` is unchanged (`pm_input.c`). It presses A every 4 s. The real new-file menu takes about 90 s of A presses: file 1, new file, 8 name letters (name `AAAAAAAA`), confirm. Then kmr_20 loads. With a save present, A on file 1 continues the game in 46 s.
- Check (slice S17, Azahar): `fast autoa freshsave` 240 s: hos_05, title, file select, kmr_20, osr_00, kmr_20, mac_00 (area 1 map 1, `spawn npcs 6`, same as before S17), 0 unmapped-access warnings, no crash. `fast autoa` with the save written by that run, 260 s: same chain, 0 warnings. `romtrace` full and packed compare: 63,296 and 63,404 reads, 0 in dropped ranges, only the BGM 1280 B timing differences.
- Save check: the new file wrote `save.flash` (131,072 B, `Mario Story 006` magic at sector 0). After restart the file select listed file 1 as `AAAAAAAA` (from the saved metadata) and the continue path loaded kmr_20 through `LOAD_FROM_FILE_SELECT`. NOT checked: a save from a save block in the world (the intro chain never reaches one), copy, erase, globals, a save with progress beyond the intro.
- Seen by eye (`fast autoa shots`): the real title (logo, PRESS START, star curtain), then the file select with the name entry window, then the file list. Rendering is wrong in places: during the name entry the keyboard shows pink arrows over every letter, and the file list windows overlap or are cut off (Stereo and Mono boxes clipped at the right edge, the files row is incomplete). Not diagnosed, probably `pm_gbi.c` or window scale gaps. The text and menus work (the name was typed, the confirm worked).
- Not run: the demo path of the title (`state_*_demo` are still empty stand-ins on main, see QUESTIONS.md), the PAL language select, the title menu when `contBitPattern & 1` (the real title draws PRESS START only if a controller bit is set: it showed in the shots, so the port reports controller 1).
- The `player` debug switch (per 10 frames position log) was in the deleted `state_step_world` and is gone.

## S20 audit (slice S20: final audit, no game code changed)

Scope: every file in `3ds/src`, `3ds/stub_include`, `3ds/include`, `3ds/CMakeLists.txt`, `3ds/effects.cmake`, `3ds/shaders`, `3ds/tools`. Also the `TARGET_3DS` edits in shared source (`git grep TARGET_3DS -- src include`). The branch `s19-full-battle` was read with `git diff main...s19-full-battle`. Nothing was checked out.
Method: every small file was read in full. For the large files (`pm_gbi.c`, `pm_os.c`, `pm_gpu.c`, `pm_tex.c`, `pm_audio.c`, `pm_rsp_audio.c`, `pm_mem.c`, `pm_log.c`) the entry points were read and grep looked for game symbols (`gPlayer*`, `gGameStatus`, `Npc`, `Evt`, `GAME_MODE`) and for hacks. No game symbol appears in them. No program was run in this slice. Rendering and issue states come from the notes above.

### Counts per class (one row = one item in the lists below)

| Class | Rows | Meaning |
|---|---|---|
| A | 17 | empty or skip stub, or zero placeholder, for decomp code or data |
| B | 7 | hand-written logic or data that replaces decomp code |
| C | 26 | port platform layer (legitimate) |
| D | 4 (+1 on the branch) | placeholder alias (legitimate) |
| E | 10 | debug switch or tooling |

Check of C and D for game logic: 2 items moved out of C into B (the `GotoMap` guard in `pm_map.c`, the `spriteIdx < 0` guard in `src/sprite.c`). 1 item moved to E (`note_door`). All other C and D items contain no game logic.

### Class A (replace with real code). Full table

| # | Where | Stands in for | Real file or system | Why it exists |
|---|---|---|---|---|
| A1 | `pm_actor_stubs.c:6-8` `load_battle`, `set_battle_formation`, `set_battle_stage` (return 0) | battle start | `src/battle/battle.c` | battle tree is only on the branch (Q1) |
| A2 | `pm_later.c:12` `reset_battle_status` (empty) | battle reset | `src/battle/battle.c` | same |
| A3 | `pm_later.c:15-23` `state_init/step/drawUI_battle`, `_demo`, `_end_battle` (9 empty functions) | battle and demo states | `src/state_battle.c`, `src/state_demo.c` | same. An idle title ends in a black screen after 8 s (Q4) |
| A4 | `pm_later.c:7` `crash_screen_set_draw_info` (empty) | crash screen draw info | `src/crash_screen.c` | the N64 crash screen draws into the N64 frame buffer. `pm_main.c` has its own crash handler (Q5) |
| A5 | `game_stubs.c:23-25` `crash_screen_init`, `is_debug_init`, `load_obfuscation_shims` (empty) | crash screen, IS-Viewer, anti-tamper shims | `src/crash_screen.c`, `src/is_debug.c`, `src/load_obfuscation_shims.c` | N64 only. May move to class C if the user agrees (Q5) |
| A6 | `pm_actor_stubs.c:10-29` 20 `int *_header` variables (cymbal_crush, flip_card_1..3, flutter_down, ...) | imgfx animation ROM offsets | `src/imgfx.c` (`ImgFXAnimOffsets` under `TARGET_3DS` already holds the real offsets) | they satisfy `extern Addr` lines that the 3DS table no longer uses. Probably dead. Delete them and link to confirm (S21) |
| A7 | `pm_kmr_stubs.c:15-16` `Entity_BrickBlock_Render`, `Entity_Hammer3Block_Render` (`gsSPEndDisplayList`) | block models | `src/entity/model/BrickBlock.c`, `Hammer3Block.c` | model files are not in CMake. These blocks draw nothing (S23) |
| A8 | `pm_kmr_stubs.c:19-25` `Entity_ScriptSpring_RenderPart1..7` (empty lists) | spring models | `src/entity/model/ScriptSpring_gfx.c` | same. Springs draw nothing (S23) |
| A9 | `pm_kmr_stubs.c:26-33` `Entity_Shattering{Brick,Hammer1,Hammer2,Hammer3}Block_Fragments{Render,Matrices}` (zero arrays) | shattering block fragments | `src/entity/model/Shattering*Block.c` | same. Fragments draw nothing (S23) |
| A10 | `pm_kmr_stubs.c:10-11` `Entity_BombableRock_Fragments{Matrices,Render}` (zero arrays) | rock fragments | `src/entity/model/BombableRock.c` | same (S23) |
| A11 | `pm_actor_link.c:11` `EVS_NpcHitRecoil` = `Return End` | NPC hit recoil script | `src/38F00.c` (166 lines, not in CMake) | the script was needed before `38F00.c` was linked. Build the file and delete `pm_actor_link.c` (S21) |
| A12 | `game_stubs.c:6-10` `ResetGameState`, `ResetSavedFrameImg`, `D_80073E08`, `D_80073E0A`, `ResetTilesImg[16]` | startup data and the reset tile image | `src/main_pre.c` (11 lines, not in CMake) | `main_pre.c` was never added. `ResetTilesImg` holds 16 zero bytes in place of `reset_tiles.png` (S21) |
| A13 | `game_stubs.c:13` `BattleStatus gBattleStatus` (zero) | battle status storage | `src/battle/` (real definition) | the branch removes it (A1) |
| A14 | `pm_later.c:2` `heap_battleHead[0x25800]` | battle heap | real address `gFrameBuf2` (`ver/us/undefined_syms.txt`) | the branch replaces it by `--defsym` (A1) |
| A15 | `pm_later.c:24` `heap_collisionHead[0x20000]` | collision heap | `src/heaps2.c` (`COLLISION_HEAP_SIZE` is 0x18000) | `heaps2.c` is not in CMake. The size differs. Storage only (S21) |
| A16 | `pm_later.c:25` `FlowerGoalPositionX/Y/Z` (`short`) | flower goal position | `src/entity/jan_iwa/Plants1.c` (`u16 BSS`) | file not built. Real code reads these (`use_spinning_flower.c`). The type differs (S21, or later with the jan_iwa entities) |
| A17 | `stub_include/effect_ld_addrs.h:385-608` 224 lines `effect_gfx_*_ROM_START/END` = `(u8*)0` | effect graphics (data, not code) | splat `effect_gfx_*` segments, 660 KB, kept in the packed ROM | gfx start is null, so `sharedData->graphics` stays NULL. Effects draw without graphics (S24, Q3) |

### Class B (hand-written logic or data). Full table

| # | Where | Stands in for | Real file or system | Why it exists |
|---|---|---|---|---|
| B1 | `area_table.c` (586 lines) | `gAreas` table in `src/world/world.c` | real table with all `area_*` maps | S08 user decision: 6 real `MAP()` rows, all others `MAP_ID()`. All maps do not fit (S01). The `.bgName`, `.songVariation`, `.sfxReverb` fields are copied by hand. No script checks them against `world.c` (S22). The header comment names `pm_map_guard.c`, which does not exist (the code is in `pm_map.c`) |
| B2 | `pm_map.c:496-541` `__wrap_get_map_IDs_by_name`, `__wrap_set_game_mode`, `__wrap_load_map_by_IDs`, `map_is_linked` | game flow for a `GotoMap` to a map that is not in the image | real `load_map_by_IDs` with every map linked | consequence of B1. It decides game flow (refuse and stay), so it is not a platform shim. Moved from C to B. It goes away with B1 |
| B3 | `stub_include/sprite/npc/*.h` (233 files) and `sprite/player.h` | generated `ver/us/build/include/sprite/npc/*.h` | the generated headers (E01) | `stub_include` comes before `ver/us/build/include` in `include_directories`, so these shadow the real ones. `Goomba.h` has 398 lines, the generated one 421 (the `SPR_IMG_*` lines are missing). `player.h` is identical in content. The branch deletes them |
| B4 | `stub_include/sprite/sprite_shading_profiles.h` (5 lines) | generated header (97 lines) | generated header | only `SHADING_NONE` exists. `SetSpriteShading(SHADING_HOS_01)` cannot compile on main. The branch deletes it |
| B5 | `stub_include/battle/actor_types.h` (7 lines) | generated header (435 lines) | generated header | only 3 `ACTOR_LEVEL_*` names. The branch deletes it |
| B6 | `stub_include/charset/{letter_content_1,peach_letter,postcard}.png.h` | generated `ver/us/build/include/charset/*.png.h` | the generated headers | image sizes typed by hand from splat. `postcard.png.h` only includes `peach_letter.png.h` (both are 150x105 in splat, so the values match). They shadow the generated ones |
| B7 | `src/sprite.c:874` `#ifdef TARGET_3DS if (spriteIdx < 0) return 0;` (shared source) | decomp behavior for a bad sprite index | real code, which reads an array at a negative index | guard added in an early slice to hide bad data. It changes game code. Find the cause of the bad index, then remove the guard (S22) |

### Class C (platform layer, no game logic). Short list

1. `pm_main.c` boot, frame loop, 2 color-image tokens `nuGfxCfb` (`pm_is_cfb`), crash handler.
2. `pm_mem.c` FCRAM split (4 MB base, resident ROM, 20 MB linear pool).
3. `pm_os.c` `os*` calls, `bcopy`, message queues, threads, TLB no-ops, `osVi*`, `osAi*`.
4. `pm_os.c` ROM: `pm_rom_init`, `pi_read_raw`, `nuPiReadRom` (swaps the asset table), `rom_remap.h` (generated by `gen_rom_remap.py`).
5. `pm_os.c` flash: `osFlash*` on `save.flash`.
6. `pm_nusys.c` `nuGfx*`, `nuCont*` (reads the 3DS pad), `nuSiSendMesg` no-op, `nuContRmbCtl[]` storage.
7. `pm_gu.c` libultra `gu*` matrix functions (fixed-point pack like the RSP).
8. `pm_gbi.c` Fast3D interpreter (platform layer, runs the real display lists). Gaps are in the next section.
9. `pm_gpu.c` PICA200 state, batches, Z buffer, HUD text.
10. `pm_tex.c` texture decode, tile, cache (4 MB budget).
11. `pm_audio.c` ndsp output and the audio thread hand-over.
12. `pm_rsp_audio.c` C version of the RSP audio microcode ops (from paperboat).
13. `pm_input.c` pad mapping (the `autoa` part is E).
14. `pm_map.c` big-endian fixes: `fix_shape`, `swap_hit_file`, `decode_yay0`, `find_asset`, `gMapShapeData` 256 KB, `--wrap` of `load_asset_by_name`. They read decomp structure layouts but hold no game rule.
15. `pm_kmr_stubs.c:35,37` `gBackgroundImage` buffer (real address 0x80200000) and `pm_world_entity_heap` (real N64 address, `#ifdef` in `src/entity.c`).
16. `game_stubs.c:15-32` `pm_game_bind`, `shim_*_obfuscated` (call the real functions).
17. `effect_loads.c` generated C form of the splat asm (`load_effect(id)` then `<name>_main`).
18. `shaders/pm.v.pica` one matrix multiply.
19. `stub_include/ld_addrs.h` real ROM offsets (`imgfx_data`, `logos`, `title_bg_1`, `charset`), `entity_model_offsets.h` (generated), `charset_offsets.h` (typed by hand from splat, should be generated), `include_asset.h` (ARM `%object`), `PR/abi.h`, `PR/n_abi.h`.
20. `CMakeLists.txt` flags (`-fno-short-enums`, `-fsigned-char`, `-fwrapv`), `--wrap` list, 2 `sprite_shading_profiles_*` real ROM offsets, `world.c` cut at `#define AREA(`.
21. `3ds/include/*.h` port headers.
22. Shared source `TARGET_3DS` edits, 36 places in 15 files (`background.c`, `charset.h`, `demo_api.c`, `imgfx.c`, `main.c`, `model.c`, `msg.c`, `entity.c`, `sprite.c`, `hos_05/intro.c`, `macros.h`, `common_structs.h`, `os_convert.h`, `os_libc.h`, `include_asm.h`). Almost all are byte swaps and address macros. Two are more: B7 above, and `entity.c:1064` (animated entity models use the compiled C node data in place of the ROM copy: data path only, no rule).
23. `imgfx.c` `ImgFXAnimOffsets` (offsets copied from splat `imgfx_data`).
24. `entity.c:15-21` `WORLD_ENTITY_HEAP_*` macros.
25. `pm_gbi.c:151-160` fixed light direction for `G_LIGHTING` vertices (RSP lighting approximation, no game logic, but a gap: the display list lights are ignored).
26. `3ds/build.sh`, `go.ps1`, `push_3ds.sh`, `bump_build.cmake` build glue (no game content).

### Class D (placeholder alias). Short list

1. `CMakeLists.txt` 47 `--defsym X=gPartnerStatus` (`world_partner_*`, `world_use_item_*`, `pause_*`, `filemenu_*`, `ui_images_filemenu_pause_*`). They name code overlays that are linked in place. `ui_images_filemenu_pause_*` names image data, but the images are linked in place by `INCLUDE_IMG` (P01), so the empty range is correct. No game logic.
2. `stub_include/ld_addrs.h` code overlay ranges `((u8*)1)` (engine1, engine2, engine4, evt, entity*, world_script_api, world_action_* and others). Code only. `rom_static_check.py` (M02) showed no overlap with a dropped range.
3. `stub_include/effect_ld_addrs.h` effect code ranges `((u8*)1)` (3 per effect). Code only. The 224 `effect_gfx_*` lines are NOT code and are class A (A17).
4. `effect_shims.yaml` `--defsym shim_X=X` (aliases of real functions).
5. On the branch: `battle_ld_addrs.h` (159 empty ranges for battle code and the item and move overlays) and `--defsym heap_battleHead=gFrameBuf2` (real address).

### Class E (debug switch or tooling). Short list

1. `pm_log.c` switch parser and log functions.
2. `pm_input.c:50-57` `autoa`.
3. `pm_gbi.c` `pm_gbi_fallback` (red, green, blue boot test triangle, switch `nofallback`), `pm_gbi_stats`, the `gbi skip` note.
4. `pm_gbi.c:258-280,433-440` `note_door`: probe of the house door vertex with fixed object coordinates (240, 30, -80). It only writes a log line. It has no game logic, but it holds a map coordinate. Delete it (S22). Moved from C to E.
5. `pm_gpu.c:249` `shots`.
6. `pm_map.c:483-494` `pm_dump_scripts` (`scripts`).
7. `pm_os.c` `romfull`, `romtrace`.
8. `pm_main.c` FPS and ms HUD, `stats`, `fast`.
9. `3ds/tools/*` (14 files): `extract_assets.*`, `gen_*`, `rom_*_check.py`, `dedup_stubs.py`, `livelog.py`, `ftppull.py`, shims, `c01.sh`. `dedup_stubs.py` lists `pm_evt_stub.c` (deleted), harmless. `c01.sh` is a C01 build helper, probably dead.
10. `3ds/run_azahar.ps1` and the pull and push helpers.

### Branch `s19-full-battle` (read only)

One commit (`7fd566684`) on `088a06e2e`. Main is 4 commits ahead (S17, M02 and notes). Diff `main...s19-full-battle`: 250 files, +690 and -37,614 lines. It adds only these files under `3ds`: `battle.cmake` (GLOB of `src/battle/*.c` minus `.inc.c` and region files, plus `state_battle.c`, `state_demo.c`, `level_up.c`, `starpoint.c`, `PowBlock.c`, `battle_ui_gfx.c`, 627 files in all), `stub_include/battle_ld_addrs.h` (592 lines), `tools/gen_battle_ld.py`, and small edits. The image grows 3.9 MB (12.15 MiB in all). The 3dsx is 6.59 MB.
- It removes: A1, A2, A3, A13, A14, B3, B4, B5.
- It adds: debug switches `battle` and `demobattle` in `pm_title.c`, switch `assets` in `pm_map.c`, `shim_battle_heap_create_obfuscated`, `--defsym heap_battleHead=gFrameBuf2`.
- Merge into main: `git merge-tree` reports conflicts in `PROGRESS.log`, `QUESTIONS.md`, `SLICES.md`, `HANDOFF.md`, `game_stubs.c`, `pm_later.c`, and `pm_title.c` (modified on the branch, deleted on main). The two debug hooks need a new home (one call in `pm_main.c` or the real `state_world.c` flow). Rebase the branch first, then merge.
- Known on the branch: kzn2 `dmaTable` ranges are empty (Lava Piranha animation missing), stage `kmr_02` names a missing asset (decomp bug), `assert i < MAX_NPCS` with the debug hook only, attack animations draw black and olive polygons, Fuzzy sprites look dark.

### Open rendering gaps (from the notes, plus what the audit found in `pm_gbi.c`)

1. Fast3D features not handled in `pm_gbi.c`. The opcode switch handles `G_VTX`, `G_TRI1/2`, `G_MTX`, `G_POPMTX`, `G_MOVEMEM` (viewport only), `G_GEOMETRYMODE`, `G_TEXTURE`, `G_SETOTHERMODE_H/L`, `G_SETCOMBINE`, prim and env color, `G_SETCIMG`, fill, `G_FILLRECT`, `G_TEXRECT`, tile and TLUT loads, and `G_MOVEWORD` (segments). It ignores `G_SETZIMG`. Every other opcode logs `gbi skip <op>` once. By reading the code, these are missing: fog (`G_FOG`, fog color), display list lights (a fixed light is used), `G_SETBLENDCOLOR`, blender and alpha-compare modes, `G_SETKEYGB`, `G_SETCONVERT`, texture generation, `G_LINE3D`, `G_BRANCH_Z`, `G_CULLDIV`, LOD and mipmap, the bilinear filter of the shading lookup, and render-to-texture except the shading palette. Not measured: which of these cause the black and olive polygons (S25 finds out with the `gbi skip` log in a battle).
2. `effect_gfx_*` never loaded (A17). Effects that need graphics data draw nothing or draw wrong. `flame` and a few effects render into `D_800B32E0` and `D_800B3AE0` (render-to-texture, not emulated).
3. Attack animations show black and olive polygons (battle branch). Not diagnosed.
4. FIXED by S27 (slice S27): pink arrows over the name entry keyboard, file list windows with black bodies. See the S27 result.
5. FIXED by S26 (slice S26): transparency, issues 4 and 7.
6. Text box transitions, the Parakarry box (issue 3), the Mario logo on the house (issue 5): not checked by eye after S14. State is unknown.
7. Shading: the second pass `SPR_SHADING_FLAG_SET_VIEWPORT` and shaded imgfx meshes are untested.
8. Not seen by eye yet: pause screen (S18), Shadow textures, kmr_20 background. Entity blocks, springs and rocks draw nothing (A7 to A10).

### Open issues from this file and their state now

| # | Issue | State |
|---|---|---|
| 1 | kkj_00 not reached by the intro chain | Open. The chain ends in kmr_20, osr_00, kmr_20, mac_00. Nothing sends the game to kkj_00 |
| 2 | Sprite shading | Fixed (S15). No linked map uses a real profile, so it is not seen in play |
| 3 | Text box transitions, Parakarry text box | Unknown. Real message files are built (S14). Not seen by eye |
| 4 | Half transparency | Open |
| 5 | Mario logo on the house | Unknown. Not re-checked |
| 6 | Red curtain | Fixed (S13) |
| 7 | Mario sprite transparency | Open, same cause as 4 |
| - | mac_00 crash (S08) | Root cause found by D01 (new file setup). No crash in runs since |
| - | O3DS RAM limit | Estimated only. Not run on hardware. `push o` failed (M02) |
| - | Audio CPU cost | Measured on Azahar only (about 3 ms per tick) |
| - | Demo path of the title | Dead black screen on an idle title on main (Q4) |
| - | Save from a save block, copy, erase | Not checked |
| - | Battle: only one Goomba battle seen, no real encounter start | Open (S19d) |

### Ranked next 10 slices (rows are in `3ds/SLICES.md`)

Ranking rule: remove stubs first (user priority), then rendering faults that block checking, then measurement.
1. S19a, merge or trim the battle tree (Q1). It removes 8 rows (A1, A2, A3, A13, A14, B3, B4, B5) at once. Blocked on the user.
2. S21, build `main_pre.c`, `heaps2.c`, `38F00.c`. It deletes A6, A11, A12, A15. Small and certain (the real files exist, 11, 7 and 166 lines).
3. S22, remove stale shadow headers on main (B3 to B6, only if the branch is not merged first), delete `note_door` (E), check `area_table.c` against `world.c` by script (B1), find the cause of the `spriteIdx < 0` guard (B7).
4. S23, link the entity model files (A7 to A10). Blocks, springs and rocks draw nothing today. Real files, no stand-in.
5. S24, load effect graphics (A17, Q3). It needs the real `effect_gfx` ROM ranges from splat in place of 0. Check the `flame` render-to-texture case.
6. S25, Fast3D gaps: log `gbi skip` in a battle, fix the missing feature (was S19c), then lights and fog.
7. S26, transparency (issues 4 and 7): draw order and depth write.
8. S27, file select render faults (pink arrows, overlapping windows).
9. S28, hardware run on O3DS and N3DS: real memory limit, audio cost, FPS. It settles Q2.
10. S29, real encounter start, kkj_00 chain, title demo timer (S19d, issue 1, Q4). Needs S19a first.
Q5 (crash screen shims) and Q6 (link more maps, for example hos_04) have no slice until the user answers.

## S21 result

Done (slice S21). Real `src/main_pre.c`, `src/heaps2.c`, `src/38F00.c` are in CMake and link first try. Deleted in S21: `ResetGameState`, `ResetSavedFrameImg`, `D_80073E08`, `D_80073E0A`, `ResetTilesImg[16]` (game_stubs.c, A12), `heap_collisionHead[0x20000]` (pm_later.c, A15), `pm_actor_link.c` with its CMake line (A11), the 20 `*_header` ints in pm_actor_stubs.c (A6, the link needs none of them). Audit rows A6, A11, A12, A15 are closed. `ResetTilesImg` now holds the real `reset_tiles.png`.
- A16 (`FlowerGoalPosition*` in pm_later.c) stays. `Plants1.c` is a jan_iwa entity file. Build it with those entities (S23).
- Check (S21, Azahar `fast autoa`, 220 s): kmr_20, osr_00, kmr_20, mac_00 reached and stable. 0 unmapped-access lines, 0 crash lines, 0 `ROM READ IN DROPPED RANGE` lines. `GotoMap refused` for area 1 map 2 is the known B2 guard.

## S22 result (slice S22, partly done)
- S22 removed `3ds/stub_include/sprite/` (all `npc/*.h`, `player.h`, `sprite_shading_profiles.h`), `battle/actor_types.h` and `charset/*.png.h`. Each one was a stub or a short copy that shadowed the generated header in `ver/us/build/include`. The generated ones apply now. Build passes.
- S22 kept in `stub_include`: `PR/abi.h`, `PR/n_abi.h`, `include_asset.h` (ARM gas `%object`), `charset_offsets.h`, `effect_ld_addrs.h`, `entity_model_offsets.h`, `ld_addrs.h` (port ROM offsets for the packed image). No generated twin has the same content, except `ld_addrs.h`, which shadows a generated one by design (packed image).
- S22 guard `spriteIdx < 0` in `src/sprite.c` (commit 02e12317b, "Fix intro crash from real update_player"): added when story state was wrong (see D01). S22 logged the guard in the chain hos_05, kmr_20, osr_00, kmr_20, mac_00: 0 hits. S22 restored `src/sprite.c` to the pre-guard text. `fast autoa romtrace` (200 s): same chain, 0 unmapped, 0 crash, 0 dropped-range lines.
- Not done in S22: `note_door` in `pm_gbi.c` (E), the `area_table.c` check script (B1) and the `large_quads` edit in `src/sprite.c`. Slice S22b did them (next section).

## S22b result (slice S22b, DONE)
- S22b removed `note_door` from `pm_gbi.c` (commit 2e3cb7411 "3D Rendering!!!" added it). It was a debug probe for one vertex near the kmr_20 door. It only wrote a `door ...` log line. It changed no drawing and no game state, so no root cause hides behind it. It was a leftover from a depth and near-plane check.
- S22b removed the `large_quads` edit from `src/sprite.c` (commit b19d274e8). It was an experiment for the clown car. That commit says it did not make Bowser show. Without it, `spr_get_quad_for_size` returns null for large pieces and the real `imgfx_appendGfx_component` draws them (real `imgfx.c` is linked). `src/sprite.c` has no port edit left in this function.
- S22b added `3ds/tools/check_area_table.py`. It compares the 28 map arrays (421 rows, ids, `.bgName`, `.songVariation`, `.sfxReverb`) and the `AREA()` rows of `area_table.c` with `src/world/world.c`. Result: 0 differences. A test edit of one field gave 1 difference and exit 1. The script does not check `.init` and `.settings` (the port links 6 real maps).
- S22b fixed the stale comment in `area_table.c` (it named `pm_map_guard.c`).
- Check (S22b, Azahar `fast autoa romtrace`, 240 s): chain hos_05, kmr_20, osr_00, kmr_20, mac_00 reached. 0 unmapped, 0 crash, 0 dropped-range lines. `GotoMap refused` for area 1 map 2 is the known B2 guard.
- Check by eye (S22b, `fast shots`, 83 frames, sheet): storybook, star ship, palace, Kammy on the broom, the clown car with Bowser (frames 75, 78, 81) draw. Not checked: pixel compare with the earlier build, the kmr_20 door area.

## S23 result (slice S23, DONE)
- Slice S23 added to CMake: `src/entity/model/{BrickBlock,Hammer3Block,ScriptSpring_gfx,ShatteringBrickBlock,ShatteringHammer1Block,ShatteringHammer2Block,ShatteringHammer3Block,BombableRock}.c`. Link passed first try, no duplicate symbol. Slice S23 deleted from `pm_kmr_stubs.c`: `Entity_BrickBlock_Render`, `Entity_Hammer3Block_Render`, `Entity_ScriptSpring_RenderPart1..7`, the 4 `Entity_Shattering*Block_Fragments{Render,Matrices}` pairs, `Entity_BombableRock_Fragment{Matrices,Render}`. Audit rows A7, A8, A9, A10 are closed.
- Slice S23 also built `src/entity/jan_iwa/Plants1.c` with its 5 model files (`CymbalPlant_anim/gfx`, `PinkFlower_anim/gfx`, `SpinningFlower`). These are entity files, not an area. Link passed. Slice S23 deleted `FlowerGoalPositionX/Y/Z` from `pm_later.c`. Audit row A16 is closed.
- Check (S23, Azahar `fast autoa romtrace`, 240 s, run after each build): kmr_20, osr_00, kmr_20, mac_00 reached. 0 unmapped, 0 crash, 0 `ROM READ IN DROPPED RANGE` lines. `rom_static_check.py`: 77 ranges, 0 overlap with a dropped range. `GotoMap refused` for area 1 map 2 is the known B2 guard.
- By eye (S23, `fast autoa shots`, 124 frames): the linked maps with a block or spring is kmr_20 only (spring and bricks in the basement). The shots show the kmr_20 outside and the room above. No block or spring was in view. So the drawing of blocks, springs and rocks is NOT checked by eye. The linked code is only proven to link and not to crash.

## S24 result (slice S24: effect graphics, Q3, A17)

Done (slice S24). `3ds/tools/gen_effect_ld.py` now reads `ver/us/splat.yaml` and writes the real `effect_gfx_*_ROM_START/END` (112 effects, 659,552 B in total, end = next larger segment start) in `3ds/stub_include/effect_ld_addrs.h`. The 224 `(u8*)0` placeholders (A17) are gone. A script check shows no range overlaps a M02 drop in `rom_remap.h` (M02 keeps them already), so no remap change. `src/effects.c` is unchanged: `dma_copy` goes through `nuPiReadRom` and the packed walk.
- Check (slice S24, Azahar, `fast autoa romtrace`, 230 s): hos_05, kmr_20, osr_00, kmr_20, mac_00 reached, 0 `DROPPED` lines, 0 unmapped lines. The trace shows real reads of effect graphics: `effect_gfx_landing_dust` 77,472 B and `effect_gfx_sun` 2,560 B. `gSPSegment(9, graphics)` now points at a heap block with the real bytes.
- Finding (slice S24, by reading the code): the effect Gfx lists, Vtx arrays and images in `src/effects/gfx/*.c` are C and PNG data linked into the 3dsx, so DL words are host pointers and do not use segment 9. `sharedData->graphics` was NULL but nothing read through it. So the ROM block is a faithful copy that no draw call needs yet. Effects drew the same before and after by this argument. A before and after pixel compare was NOT made.
- Seen by eye (slice S24, `fast shots`, 106 frames, intro): storybook, ship, palace, Kammy, Bowser and the clown car draw, with sparkles and light effects. Not seen: any flame. Bad draws in frames 70 to 90 (Bowser fight at the Star Rod): black squares around the sparkles, one olive square with a black blob (frames 74 to 76), black bars. The effect that makes each one is not identified. They look like alpha/blender gaps (CLD or XLU render modes), so they belong to S25 and S26. The `gbi skip` log in this run: `f3`, `04` (G_BRANCH_Z), `02` (G_MODIFYVTX).
- Render-to-texture, exact list (by reading the code): only `flame` writes its own scratch images (`D_800B3AE0` and `D_800B32E0`, 32x64 I8, two-cycle combiner, tile loads from the same buffer, 6 texrects). `underwater` writes into `nuGfxZBuffer` as RGBA16 and back to the frame buffer. The missing feature for both: texrect and tile load into a non-screen colour image in formats other than the TLUT case of S15 (`rect_to_image` handles only a TMEM palette). `flame` is used only in unlinked maps (kzn2 battles, isk stages), so it cannot be run or seen now. I wrote no fix: it needs a 2-cycle I8 software rasteriser, which is not small, and I cannot test it. Do it after those maps link.

## S26 result (slice S26: transparency, issues 4 and 7)

Done (slice S26). Platform layer only (`pm_gbi.c`, `pm_gpu.c`, `pm_tex.c`), no game code changed.
- Root cause 1 (slice S26): `pm_gbi.c` read `G_SETOTHERMODE_L` but ignored it. `pm_gpu.c` always wrote depth, always blended by source alpha and never alpha tested. A translucent mode (XLU, no `Z_UPD`) therefore wrote depth, and the quad of a sprite wrote depth over its transparent texels. Later draws behind them failed the depth test. The measured render modes (log switch `rmlog`) match: XLU surface `0c1849d8` has `Z_CMP`, no `Z_UPD`.
- Fix 1 (slice S26): `sync_render()` in `pm_gbi.c` reads the mode as libultraship `DeriveTriState` does: depth test from `Z_CMP`, depth write from `Z_UPD`, decal (`ZMODE_DEC`) passes equal depth with a 0.001 depth bias toward the eye, blend only when the blender mixes with the frame buffer by (1 - alpha), texture edge (`CVG_X_ALPHA`) alpha test 0x30 (blend off) or 0x08 (blend on), `G_AC_THRESHOLD` alpha test 0x08. `pm_gpu_set_render()` carries it to the PICA state. Debug switch `rmoff` restores the old always-write, always-blend state for an A/B check.
- Root cause 2 (slice S26): the I4 and I8 formats used PICA L8, whose alpha is 1. The RDP gives an I texel an alpha equal to its intensity. Effect sprites (sparkles) drew their whole quad: black squares and black bars (the Star Rod fight, S24 note). Fix: `pm_tex.c` uses LA8 for I formats with alpha = intensity.
- Root cause 3 (slice S26): a combiner whose colour is a constant (shadows: black) but whose alpha reads TEXEL0 never bound the texture, so shadows were solid squares (white, then black). Fix: combine mode 4 in `pm_gbi.c` `const_rgb_tex_alpha()` (1-cycle and 2-cycle forms, colour constant, alpha = constant x TEXEL0 alpha; the form `(T1-T0)*E+T0` uses TEXEL0 for both because tile 1 is not loaded). The same slice added combine mode 3 (texture colour, vertex alpha) when the alpha combiner does not read a texel. Found on the way: the PICA constant colour register is 0xAABBGGRR. `pm_gpu.c` passed 0xRRGGBBAA, so prim colour in mode 1 had red and blue swapped. Fixed with `abgr()`.
- Seen by eye (slice S26, Azahar, `fast shots` and `fast autoa shots`, A/B with `rmoff`):
  - Intro, Star Rod fight (shots 70 to 82): with the old state Bowser's clown car is missing at shot 70 and a dark bar sits across shots 74 and 82. With the fix both are right. Before the slice there were black squares around the sparkles and black bars. After: no black squares, no black bars.
  - Toad Town and the yard (autoa shots 76 to 96): before, Mario and Luigi had a black rectangle that hid the window, sign and house wall behind them. After: gone. The soft round shadows under Mario and Luigi now show.
- Not fixed, by eye (slice S26): the olive square with a dark blob (intro shot 74) and the black sparkle streaks (they should be white or yellow). Cause by the log: the combiner is `(ENV-PRIM)*TEXEL0+PRIM` and `(1-TEXEL0)*PRIM+TEXEL0`, a general combiner. `pm_gbi.c` only models texture x shade, a constant, and the modes above. This belongs to S25 (combiner). The sky in kmr_20 and mac_00 stays black in some shots (not S26).
- Check (slice S26): `fast autoa`, 261 s: hos_05, kmr_20, osr_00, kmr_20, mac_00 reached, mac_00 stays up, 0 unmapped lines, no crash. The `fast shots` run ends with a crash after the intro (title screen, `mdl_update_transform_matrices`), same as before S26.
- Not tested: blend colour (`G_SETBLENDCOLOR`), fog, noise, the exact `ZMODE_INTER` rule, 2-cycle modes whose second blender cycle differs from the first.


## S27 result (slice S27: file select render faults, DONE)

Platform layer only (`pm_gbi.c`, `pm_gpu.c`, `pm_tex.c`), no game code changed.
- Evidence (slice S27, texrect and tile log, since removed): `draw_box` (window frames) draws the window body with 4 quadrant texrects. Its combiner is 2-cycle, `(PRIM - ENV) * TEXEL1 + ENV`, alpha = TEXEL1 alpha. TEXEL1 is tile 1, loaded by `gDPLoadMultiTile` (window image, clamp). `pm_gbi.c` only tracked tile 0 and `pm_gpu_tex_rect` forced "texture only". So each body used whatever tile 0 held last: the 8x8 hud tile of the name-entry underscore (HES_FilenameSpace), mirrored over the whole window. That is the pink arrows over the keyboard (the 16 px period is the 8x8 tile mirrored) and the black or curtain-like window bodies elsewhere.
- Fix (slice S27): (1) `pm_gbi.c` keeps a descriptor of tile 0 and tile 1 (source, format, size, palette, wrap). If the combiner reads TEXEL1 and never TEXEL0, TEXEL1 is renamed TEXEL0 and `ensure_tile(1)` binds tile 1 before the draw (triangles and texrects); `ensure_tile(0)` restores tile 0. Cycle 2 must not read a texel for this rule. (2) New combine mode 5 in `pm_gpu.c`: `(PRIM - ENV) * TEXEL + ENV` as a PICA INTERPOLATE (constant = PRIM, ENV in the vertex colour), alpha = texel alpha. Texrects keep mode 5 (they forced mode 2 before). (3) `pm_tex.c` pads a texture to its power-of-two size with the edge texel, so a clamped read past the image gets the edge (the window image is 16x7 in a 16x8 texture).
- Hypothesis of the coordinator (texture cache key collision in `pm_tex.c`): REJECTED by evidence. The log showed no cache problem: the rects drew the correct, current tile 0 (the 8x8 hud tile). The wrong part was the tile choice. (No checksum compare was run: the tile log was enough.)
- Seen by eye (slice S27, Azahar, `fast autoa shots freshsave`, 115 shots), before and after:
  - Before: pink arrows on a 16 px grid over the whole keyboard (all cells, not only the current one), name box and file windows with black bodies, Stereo and Mono boxes drawn as black slabs.
  - After: the keyboard window is beige with the letters, one caret arrow under the current name position, the name box is a dark blue box with a gold edge, the 4 file windows are beige and grey with numbers 1 to 4, the Stereo and Mono boxes and the three red buttons look right.
- User items (slice S27): (1) arrow under the current character on ALL grid cells: fixed, same cause. (2) wrong textures on the file windows and the curtain texture in the name box: fixed, same cause. (3) starry sky behind the file select and the house cutscene: by eye the sky is the real file select stage (blue starry stage between the curtains, shots 18 to 60) and the real night yard behind Mario's house (shots 72 to 84, a night scene). The castle scene (shot 108) and the house inside (shots 90 to 102) show no sky. I did not see a layer that is not redrawn. No change made. If the user saw sky somewhere else, tell me the place.
- Check (slice S27): `fast autoa` 280 s: kmr_20, hos_05, kmr_20, mac_00 reached (area 1 map 1), mac_00 stays up, 0 unmapped lines, no crash, no dropped-range line.
- Not done: the second cycle texel use (TEXEL0 in cycle 2 means the next tile), more than 2 tiles, the filled body style of other windows not seen in this run (pause menu: not checked by eye).

### New open issues reported by the user during slice S27 (all unverified, rows in 3ds/SLICES.md)
- V01: background image squished at the left and right sides, stretched in the middle. Not checked.
- G01: loading zone in mac_00 does not trigger after the pipe. Hypothesis: the target map is an id-only row (`GotoMap refused`, Q6 and S30). Not checked.
- G02: FIXED (see the G02 result section).
- Also queued by the coordinator: S32 input speed, S33 gitignore cleanup (no detail given to S27).

- Open issue V01 (recorded during slice S32, not worked in S32): the user also reports that the WRONG background shows in game (the intro cutscene or title background in a later scene). User-reported, unverified. Joins the squished background report in V01. Hypothesis, not verified: one fixed buffer (gBackgroundImage) plus a texture cache keyed on address and size returns a stale picture. Test steps are in the V01 row of 3ds/SLICES.md.

Done (slice S32). `autoa` in `pm_input.c` now presses A on 2 of every 4 scans (was 1 scan in 240), and `fast` skips the 30 Hz `gspWaitForVBlank` pad wait in `pm_main.c`. The log line `load map` now shows `scan N` (pad scan count). Before (`fast autoa freshsave`, old pattern): kmr_20 at scan 3928, 87 s. After: scan 562, 31 s. With a save present: kmr_20 at scan 410. Chain hos_05, kmr_20, osr_00, kmr_20, mac_00 reached, mac_00 stays up, 0 unmapped lines, no crash (150 s and 100 s runs). `fast autoa shots freshsave` shows the real title, file select, name entry (AAAAAAAA) and file list. Not changed: Azahar speed limit (not needed).

## V01 result (slice V01: background squished and wrong background, DONE)

Platform layer only (`pm_gbi.c`, `pm_map.c`), no game code changed. Two separate root causes.
- Bug 1 (squished background), cause: `src/background.c` draws the background with CI8 tile loads and COPY-mode texrects with dsdx 4096 (4.0). In COPY mode the RDP writes 4 pixels per cycle, so 4.0 means 1 texel per pixel. `pm_gbi.c` used 4.0 as 4 texels per pixel and ignored the inclusive lower right corner. Log (slice V01, temporary): rect x 12..307, tw 512, s0 0, s1 2.305 (should be 0.578). So the 296 px image was squeezed into about a quarter of the width and the rest was edge colour (white). Fix: in COPY mode dsdx /= 4 and width + 1 (as libultraship `GfxDpTextureRectangle`). Seen by eye: before, title and first kmr_20 show a thin strip of the picture at the left, white elsewhere. After, the title art fills the width, kmr_20 has the blue sky with clouds and hills.
- Bug 2 (wrong background), cause: texture cache hit on a stale entry. `gBackgroundImage` is one fixed buffer, so the raster and palette of every map sit at the same addresses (0x84b1a0 + n*0x6f0, palette 0x84afa0, 296x6 CI8 tiles). `pm_tex.c` keys on address, size and tlut pointer, and `decode_yay0` wrote new data without telling the cache. Evidence (temporary checksum of the source bytes at insert and at hit): 40 of 40 logged hits had a different checksum, raster and palette both. Sequence in the log: title_bg decoded, then kmr_20 `kmr_bg` decoded into the same address, then stale hits. Fix: `decode_yay0` (`pm_map.c`) calls `pm_tex_invalidate(dst, dec)`. Check after: kmr_20 shows its own sky, mac_00 shows its own sky.
- Real path check: `load_map_by_IDs` (real world.c) runs `load_map_bg(wMapBgName)` then `set_background` on each load, and `decode_yay0` log shows the new asset name decoded for each map (`title_bg`, `kmr_bg`). The header was always new. Only the cache was stale.
- Starry sky question: hos_05 (`hos_bg`, set in `settings.c`) is meant to show the starry sky. Seen in `fast shots`: starry sky behind the star ship scene. The file select and title use `title_bg` (set in `state_title_screen.c`), a picture of the Mario world, not a sky. The earlier starry sky behind the file select in S27 shots is consistent with a stale `hos_bg` from the intro (same bug 2), but I did not reproduce it before the fix (autoa with a save and freshsave both skip the intro). After the fix the file select shows the title picture. The night yard behind Mario's house is the kmr_20 outside at night: not checked here.
- Coordinator hypothesis H01 (pm_tex_invalidate drops a slot that the current frame still uses): REJECTED for V01 by log. Temporary log of every invalidate with slot epoch and frame epoch, run hos/title/kmr_20/osr_00: 0 invalidations of a slot used in the current frame (INFRAME 1 count 0 of 39 drops). Game logic runs inside `pm_gpu_begin`..`pm_gpu_end` before the display list executes, so a decode comes before this frame's draws. The risk stays possible in theory (a texture drawn, then invalidated, in one frame). Not seen. `C3D_TexLoadImage` flush: not the cause, the right pictures show after the cache fix.
- Check (slice V01, Azahar): `fast autoa` 110 s: kmr_20, osr_00, kmr_20, mac_00 (area 1 map 1) reached, mac_00 stays up, 0 unmapped, 0 crash. Sheets: `3ds/logs/v01_b1.png` before, `v01_a1.png`, `v01_fr.png` (freshsave), `v01_i1.png` (intro) after (git-ignored).
- Not checked: other maps, the wave background, fog tinted palette, pause background fade, battle backgrounds.

## PT01 result (slice PT01: per-frame phase timer, measurement only, port layer)

All items below belong to slice PT01. No optimization was done. No game logic changed. All numbers come from Azahar. They are NOT cycle exact. The user says Azahar does not run at full speed. Nothing ran on the O3DS. The last section says what to re-measure first on the O3DS.

### What was added
- `3ds/src/pm_phase.c` and the macros in `3ds/include/pm_port.h` (`PH_PUSH`, `PH_POP`, `PH_ADD`: one load and one branch when the switch is off). Hooks: `--wrap` on `step_game_loop`, `gfx_draw_frame`, `gfx_task_background` (CMake link options, the game code is not edited). Small edits in `pm_main.c`, `pm_nusys.c`, `pm_gpu.c`, `pm_gbi.c`, `pm_tex.c`, `pm_os.c` (`pi_read_raw`), `pm_map.c` (`yay0_decode`), `pm_log.c` (call counter in `pm_debug_has`).
- Switches (runtime, they also work on hardware): `phase` (summary every 2 s through `pm_log`: wall, cpu, each phase avg/p95/max, counters avg/max, frame numbers of frames over 34 ms, the 3 worst frames with all phases). `phasecsv` (one log line per drawn frame, for analysis runs only: 30 to 60 lines per second, 0 lines dropped in a 330 s run). `phasegbi` (times G_VTX, triangle and G_MTX ops on their own, see the limits).
- `python 3ds/tools/phase_report.py log.txt` reads a `phasecsv` log. It prints the per scene table, the causes of frames over 34 ms and the gap histogram. The scene is the last `game mode` or `load map` line before the frame.
- Phases are exclusive: a nested phase takes its time from the phase that runs. One drawn frame runs from the end of the previous drawn frame to its own end, so the phases add up to the wall time. `cpu` = wall - idle - FrameBegin. FrameBegin is `C3D_FrameBegin(C3D_FRAME_SYNCDRAW)`. It waits for the previous frame on the GPU and for the screen transfer, and the transfer waits for the vblank. So FrameBegin is mostly a wait and is not CPU work. Idle is the `gspWaitForVBlank` pad of the normal run (the `fast` run has no pad).
- Timer cost: one `svcGetSystemTick` read costs 1.34 us in Azahar (log line `phase: on`). A frame makes about 900 reads. A run without `phase` (R4, `fast autoa logstat`) has wall 34.1 to 37.2 ms in mac_00. The same scene with `phase` has wall 38.9 ms. So the timer adds about 3 ms per frame (about 8%). All phase times are a little high. The ranking does not change.
- Logs (git-ignored, `3ds/logs/`): `pt01_r1_log.txt` (fast), `pt01_r2_log.txt` (normal), `pt01_r3_log.txt` (fast, phasegbi), `pt01_r4_log.txt` (no phase, logstat).
- Runs: R1 `fast autoa phase phasecsv` 230 s, 5271 drawn frames. R2 `autoa phase phasecsv` (normal, 30 Hz pad wait) 330 s, 6736 frames. R3 `fast autoa phase phasegbi phasecsv`. R4 `fast autoa logstat`. The map chain in all runs: logos, hos_05 intro, title, file select (continue), kmr_20, osr_00, kmr_20, mac_00. 0 unmapped and 0 crash lines.

### Table 1: phase time per scene, fast run R1 (ms, avg / p95 / max)

| phase | title (n=66) | file select (n=38) | hos_05 intro (n=48) | kmr_20 (n=1032) | mac_00 (n=3731) | osr_00 (n=37) |
|---|---|---|---|---|---|---|
| wall | 22.2 / 33.4 / 80.7 | 27.5 / 46.3 / 61.0 | 16.5 / 16.8 / 17.0 | 43.8 / 50.8 / 98.2 | 38.9 / 51.3 / 51.5 | 50.3 / 51.4 / 51.6 |
| cpu (wall - idle - FrameBegin) | 17.5 / 27.8 / 65.0 | 20.7 / 31.8 / 53.1 | 2.9 / 3.3 / 3.3 | 36.2 / 44.4 / 83.1 | 32.6 / 38.1 / 46.9 | 42.0 / 48.1 / 48.1 |
| 1 logic (step_game_loop) | 0.0 / 0.0 / 2.8 | 0.0 / 0.4 / 1.3 | 0.0 | 0.6 / 0.7 / 0.9 | 0.6 / 0.7 / 0.9 | 0.4 / 0.5 / 0.5 |
| 2 display list build (gfx_draw_frame) | 0.0 | 0.1 / 0.3 / 0.3 | 0.0 | 0.5 / 0.7 / 0.7 | 0.8 / 0.8 / 0.8 | 0.6 / 0.7 / 0.7 |
| 3 GBI interpret (self: walk, transform, clip, emit) | 1.8 / 2.1 / 2.2 | 2.9 / 5.9 / 6.1 | 0.6 / 0.7 / 0.8 | 15.3 / 20.8 / 21.4 | 11.1 / 11.4 / 12.7 | 14.8 / 15.2 / 15.2 |
| 4a GPU draw prep (state, uniform, texenv) | 0.6 / 0.8 / 0.8 | 0.7 / 1.0 / 1.0 | 0.0 | 0.7 / 0.8 / 1.0 | 0.6 / 0.7 / 0.8 | 1.2 / 1.4 / 1.4 |
| 4b `GSPGPU_FlushDataCache` per draw | 7.5 / 8.6 / 8.7 | 8.4 / 11.3 / 11.3 | 0.7 / 0.8 / 0.9 | 8.5 / 9.6 / 11.3 | 7.4 / 7.8 / 8.4 | 13.0 / 15.2 / 15.3 |
| 4c BufInfo + `C3D_DrawArrays` | 2.2 / 2.5 / 2.6 | 2.4 / 3.3 / 3.3 | 0.2 | 2.5 / 2.8 / 3.2 | 2.2 / 2.3 / 2.5 | 3.8 / 4.5 / 4.5 |
| 4d FrameEnd (submit) | 0.3 | 0.3 | 0.3 | 0.3 | 0.3 | 0.3 |
| 4e FrameBegin (wait, not CPU) | 4.7 / 15.7 / 16.5 | 6.8 / 15.5 / 15.8 | 13.6 / 14.4 / 14.7 | 7.6 / 15.4 / 16.7 | 6.2 / 15.9 / 16.7 | 8.3 / 12.3 / 13.4 |
| 5 texture convert + upload (`pm_tex.c` miss) | 1.3 / 0.0 / 42.8 | 0.6 / 0.4 / 20.1 | 0.0 | 0.2 / 1.2 / 43.9 | 0.7 / 1.2 / 4.2 | 0.0 / 0.3 / 0.3 |
| 6 audio tick (`pm_audio_tick`) | 2.9 / 13.0 / 13.0 | 4.4 / 9.8 / 9.9 | 0.5 / 0.5 / 0.9 | 6.8 / 15.3 / 19.2 | 8.0 / 13.4 / 21.6 | 7.0 / 9.7 / 9.8 |
| 7a ROM read (`pi_read_raw`) | 0.0 / 0.0 / 2.2 | 0.0 / 0.2 / 0.6 | 0.0 | 0.0 / 0.2 / 0.3 | 0.1 / 0.2 / 0.4 | 0.0 / 0.1 / 0.1 |
| 7b Yay0 decode | 0.1 / 0.0 / 9.3 | 0.0 / 0.0 / 0.9 | 0.0 | 0.0 | 0.0 | 0.0 |
| 8 idle (vblank pad) | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 |
| other (loop, pad, HUD, log poll) | 0.3 / 0.3 / 2.4 | 0.3 | 0.2 / 0.3 / 0.3 | 0.3 / 0.3 / 2.2 | 0.3 / 0.4 / 2.5 | 0.3 / 0.4 / 1.9 |
| triangles drawn avg / max | 46 / 53 | 99 / 231 | 36 / 53 | 862 / 1260 | 295 / 481 | 816 / 818 |
| vertex slots avg / max | 829 / 951 | 977 / 1383 | 157 / 207 | 3029 / 4344 | 1325 / 1881 | 3404 / 3618 |
| draws avg / max | 120 / 138 | 134 / 180 | 12 / 14 | 135 / 180 | 120 / 134 | 208 / 244 |
| texture uploads avg / max per frame | 1 / 58 | 1 / 45 | 0 / 3 | 0 / 40 | 0 / 3 | 0 / 4 |
| texture KB avg / max per frame | 11 / 464 | 4 / 142 | 0 / 6 | 1 / 286 | 4 / 26 | 0 / 1 |
| ROM reads avg / max per frame | 1 / 71 | 1 / 23 | 0 / 0 | 1 / 11 | 3 / 13 | 1 / 4 |
| audio ticks avg / max per frame | 1 / 3 | 1 / 3 | 1 / 2 | 2 / 4 | 2 / 4 | 3 / 4 |

Extra counters for a steady mac_00 frame (`phase count` lines): 1357 triangle commands, 291 drawn (79% culled after the transform), 2237 vertices loaded by G_VTX (4071 triangle corners are transformed), 5981 display list ops, 784 `flush` calls of which 120 draw, 162 texture cache lookups (hits), 600 `pm_debug_has` calls (1.9 us each when a switch string is set, 1.1 ms).

### Table 2: normal run R2 (30 Hz pad wait), wall and cpu only
| scene | wall avg / p95 / max | cpu avg / p95 / max | idle avg | frames at 2 / 3 / 4 vblanks |
|---|---|---|---|---|
| hos_05 intro | 33.3 / 33.4 / 33.5 | 3.7 / 3.7 / 4.7 | 13.5 | all at 2 |
| title | 48.3 / 50.2 / 97.5 | 22.6 / 27.7 / 65.6 | 12.4 | not counted |
| file select | 41.2 / 50.9 / 77.7 | 22.3 / 32.8 / 50.2 | 5.9 | not counted |
| kmr_20 | 47.7 / 50.8 / 98.1 | 36.5 / 44.3 / 83.3 | 0.5 | 151 / 878 / 3 |
| osr_00 | 50.7 / 51.6 / 66.7 | 41.9 / 49.1 / 50.8 | 0.0 | not counted |
| mac_00 | 46.3 / 51.4 / 67.3 | 34.2 / 38.8 / 45.9 | 0.8 | 1136 / 3721 / 29 |

Wall time is a multiple of 16.7 ms. A frame with cpu work of 33.3 ms or more takes 3 vblanks (50 ms). In Azahar kmr_20 and mac_00 run at about 21 fps, osr_00 at 20 fps. Frames over 34 ms wall: 5630 of 6736 (83.6%) in R2, 2920 of 5271 (55.4%) in the fast run R1. Frames with cpu work over 33.3 ms: 4072 (60.5%) in R2, 2039 (38.7%) in R1.

### Top 3 causes of frames over 33 ms (R2, from `phase_report.py`; a cause is the phase 3 ms or more above its scene median, else `base`)
1. Steady load, no spike (4387 of 5630 frames, 78%). The cpu work per frame is 34 to 42 ms in kmr_20, mac_00 and osr_00, so the frame misses the 2 vblank slot. The three biggest phases of a kmr_20 or mac_00 frame: GBI interpretation 11 to 15 ms, `GSPGPU_FlushDataCache` per draw 7.5 to 8.5 ms (osr_00 13 ms), audio tick 7 to 10 ms (2 to 3 ticks of 2.5 to 3.5 ms). Logic (0.6 ms) and display list build (0.5 to 0.8 ms) are small.
2. Audio catch-up (905 frames, mean +5.2 ms above median). The audio tick runs once per elapsed 60 Hz, at most 3 per loop pass. A frame that took 3 vblanks gets 3 or 4 ticks. A 2 vblank frame gets 2. So a slow frame gets more audio work, and that makes the next frame slower. Frames with 4 ticks (about 5% of mac_00 frames) show audio 14 to 21 ms.
3. GBI peaks (277 frames, +5.6 ms): frames with more triangles after a map change or camera move (kmr_20 up to 1260 drawn, 2599 commands in change_map). Rare but large: texture upload bursts (18 frames, mean +26 ms, up to 69 ms: 58 to 85 uploads, 280 to 460 KB in one frame, about 0.75 ms per upload) and map load ROM reads (3 frames of 449, 447 and 300 ms, see below).

### Stutter pattern (frame numbers from R2)
- It is not a GC-like hitch. The main pattern is steady. In mac_00 the wall alternates 33 ms and 50 ms (2 and 3 vblanks) frame by frame, runs `(2,1),(3,1),(2,1),(3,1)` from the start of the scene (frame 1862). The autocorrelation of the wall time peaks at lag 4, 6 and 2. The audio tick count alternates between 2 and 3, and cpu work is about 34 ms, close to the 33.3 ms limit, so the frame flips between 2 and 3 vblanks. That is a visible judder. In kmr_20 85% of the frames take 3 vblanks (50 ms), with runs of 79 and 86 frames.
- No spike has a fixed period. Audio spikes (4 ticks, 14 to 21 ms) occur 164 to 888 frames apart in mac_00 (frames 2340, 3228, 3956, 4120, 4844, 5264, 5616), not regular. Texture uploads: in mac_00 about 1 upload of 4 KB per frame in 71% of the frames (animated sprites or streamed textures), 0.7 ms on average, not a hitch.
- Map load hitches (one time per load): frame 523 (hos_05 to kmr_20) 449 ms: 15,643 ROM reads (1 MB, 66 B each) 327 ms, game logic 76 ms (script init), Yay0 12 ms. Frame 1523 (kmr_20 reload) 447 ms. Frame 1831 (mac_00) 300 ms: 9168 ROM reads 207 ms. Then a few frames of 100 to 140 ms with texture bursts (for example frames 533, 1528, 1837). Title frames 375 and 380: 98 and 90 ms, texture upload (35 and 58 uploads). File select frame 461: 78 ms. Boot to the first frame: 5.47 s (ROM load and init, before frame 1).

### Ranked optimization candidates (Azahar ms per frame, mac_00 or kmr_20)
1. One `GSPGPU_FlushDataCache` per frame, not per draw (`pm_gpu.c flush()`): 7.4 to 8.5 ms (kmr_20 8.5, osr_00 13.0, title 7.5 for only 46 triangles). That is 62 us per call in Azahar at 120 to 208 calls per frame. The GPU does not read the vertex buffer before `C3D_FrameEnd`, so one flush of the used range before FrameEnd is enough. Evidence: the draw counts are exact. The per call cost is Azahar's model of a GSP IPC call. UNVERIFIED on hardware. This is the first thing to check there.
2. GBI: 10.8 ms (mac_00) to 15 ms (kmr_20). 79% of the triangle commands are culled after a full per corner transform (mac_00: 1357 commands, 291 drawn, 4071 corner transforms for 2237 loaded vertices). Transform once per vertex at G_VTX, keep clip bits per vertex, reject a triangle by AND of three flags (this is performance step 2 in "Next after the stub list is empty"). Fine split (R3, the timer inflates it): triangle ops about half of the GBI time, G_VTX and G_MTX under 1 ms, the walk and the state ops (5981 ops per frame) the rest. Expected saving 2 to 5 ms. UNVERIFIED which part is memory bound on hardware.
3. Audio: 7 to 10 ms per frame (2 to 3 ticks of 2.5 to 3.5 ms, scalar C mixer, 24 voices). Move per voice mixing to the DSP (ndsp), or lower the cost per tick (quality changes need the user). Also cap the catch-up so that a slow frame does not add ticks.
4. Draw count: 120 to 208 draws per frame for 46 (title) to 862 triangles. The title has 120 draws for 46 triangles. Merge draws that share state. Cost of the per draw work: items 1 and 4c (2.2 to 3.8 ms for BufInfo + DrawArrays) and 4a (0.6 to 1.2 ms). Also 784 `flush` calls for 120 draws in mac_00 (cheap).
5. Pipeline: FrameBegin waits 5 to 8 ms in Azahar (vblank locked). With `C3D_FRAME_SYNCDRAW` the CPU cannot start frame N+1 while the GPU draws frame N. Double buffering the vertex buffer and the deferred texture free would let the CPU overlap. On hardware the GPU time may be small. Needs the hardware run first.
6. Texture miss cost: 0.75 ms per upload (scalar expand, then tile, then upload). Bursts of 40 to 85 uploads cost 40 to 70 ms on a map load. Single pass tiling or direct tile order writes. Lower priority (one time per map load).
7. Map load: 15.6k ROM reads of 66 B cost 327 ms (21 us per read). The per read path (`pi_read_raw`: drop table walk, `pm_tex_invalidate` over 128 slots, `pm_debug_has("romtrace")`) dominates. One time per load, 0.3 s.
8. `pm_debug_has`: 600 calls per frame, 1.1 ms in Azahar, only when a switch string is set (the function returns at once with no switches). Cache the lookups in static ints. Small.

### Limits of this measurement
- `phasegbi` (R3) puts two tick reads around each vertex, triangle and matrix op (about 900 ops per frame). Wall grew from 38.9 to 48.8 ms in mac_00. The split is approximate. State ops are not timed on their own because there are 5981 per frame.
- The 5.47 s boot time and the 449 ms map load include Azahar's own cost for service calls.
- Azahar GPU and GSP time is a model. FrameBegin, `GSPGPU_FlushDataCache` and `C3D_DrawArrays` costs especially need hardware.

### Must be re-measured on the O3DS
- Run `push-o` with `phase phasecsv` (in `debug3ds.txt` or as game args), then `pull-o`, then `python 3ds/tools/phase_report.py <log>`. First read the `phase W` lines. Compare cpu per frame with the 34 to 42 ms here.
- The real cost of `GSPGPU_FlushDataCache` per call (candidate 1), of FrameBegin (GPU time, candidate 5) and of the audio tick (the `audio cost` line said 3 ms per tick in Azahar, the real ARM11 has no NEON and may be slower).
- Azahar does not model cache misses or pipeline stalls. The real ARM11 (268 MHz, 16 KB caches) is likely slower in the GBI and texture loops. The tick read costs 1.34 us here. The real svc cost may differ, so the timer overhead on hardware is unknown (the `phase: on` line shows it).
- The 2 and 3 vblank judder depends on the real cpu time per frame. Expect it where cpu is near 33 ms.

## PO1 result (slice PO1: one vertex cache flush per frame, port layer)

All items belong to slice PO1. Numbers come from Azahar and are NOT the real hardware cost. Logs: `3ds/logs/po1_base_log.txt` (before), `po1_new2_log.txt` (after), both `fast autoa phase phasecsv`, same 170 s run.

### Flush audit (all cache calls in the port)
- `pm_gpu.c flush()`: `GSPGPU_FlushDataCache` once per draw (120 to 208 per frame). REMOVED. Now one call in `pm_gpu_end` over `vert_base` to `verts` (the range written this frame), before `C3D_FrameEnd`.
- `pm_tex.c`: `C3D_TexLoadImage` copies the tile data and flushes it once per texture upload. Already the minimum. Not changed.
- `pm_audio.c`: one `DSP_FlushDataCache` per audio buffer fill. Not per draw. Not changed.
- `C3D_FrameEnd(GX_CMDLIST_FLUSH)` flushes only the command buffer (the S-slice comment explains why). Not changed.
- Batch start is already rounded up to 16 bytes (H02 item i).

### Hazards checked
- Vertex range reused in one frame: no. `verts` only grows from `vert_base` in a frame and resets in `pm_gpu_begin`. A full buffer drops vertices (counted in `vert_drops`), it never wraps.
- Previous frame still on the GPU: `C3D_FrameBegin(C3D_FRAME_SYNCDRAW)` waits first, so the CPU does not write while the GPU reads.
- Texture rewritten in the same frame: slots are freed late (H02 item k) and `C3D_TexLoadImage` flushes at upload, so the GPU never reads unflushed texture data.
- Command buffer order: unchanged. The flush runs before `C3D_FrameEnd`, so before the GPU starts the list.

### Result (Azahar, fast autoa, 3 runs of 170 s)
| | before | after |
|---|---|---|
| vertex flushes per frame | 120 (mac_00), 135 (kmr_20), 208 (osr_00) | 1 |
| flush time per frame | 7.4 to 13.0 ms | under 0.1 ms |
| mac_00 cpu per frame avg / p95 / max | 32.4 / 38.0 / 45.4 ms | 23.7 / 28.2 / 32.6 ms |
| mac_00 wall avg / p95 / max | 38.3 / 51.3 / 51.4 ms | 33.4 / 34.7 / 38.0 ms |
| kmr_20 cpu avg / p95 | 36.2 / 44.4 ms | 26.3 / 35.1 ms |
| frames over 36 ms wall | 1501 of 3755 (40.0%) | 93 of 4482 (2.1%) |
| frames over 40 ms wall (a missed vblank slot) | 1493 (39.8%) | 81 (1.8%) |
| frames over 34 ms wall (the report threshold) | 2012 (53.6%) | 1421 (31.7%) |
The 34 ms row stays high because a 2 vblank frame takes 33.4 ms and its jitter reaches 34.7 ms. Use the 36 and 40 ms rows. Frames with cpu over 33.3 ms: 1513 (40.3%) before, 82 (1.8%) after. Audio ticks per frame after frame 1500: avg 2.3 and max 4 before, avg 2.0 and max 3 after, frames with 3 or more ticks 806 of 2574 before, 411 of 3243 after. Audio was not changed. The tick count falls because frames no longer run long.

### Visual check
`fast autoa shots` (162 shots, run with the change): logos, title, file select, hos_05 intro, kmr_20, mac_00 look right. No flicker, no corrupt polygons, no stale vertex garbage seen between shots. Shots show only 1 frame in 30, so a one frame glitch could be missed. The white fades are map transitions. `fast autoa`: kmr_20 reached, mac_00 stays up, 0 unmapped and 0 crash lines.

### Re-measure on the O3DS
- The 62 us per flush call is Azahar's model of a GSP IPC call. The real cost per call and the real cost of one flush of 40 to 130 KB (the vertex range, a cache write back of 32 byte lines on the ARM11) are unknown. The saving on hardware is probably smaller or different. Run `push-o` with `phase phasecsv`, then compare the `gc` column (now one call per frame) and the cpu per frame with the PT01 numbers.
- If the flush of the whole range is slow on hardware, flush only up to the last batch that changed.


## Warp (slice W01: warp, story presets, scripted pad, Azahar turbo)

All items belong to slice W01. Azahar only. Port-side debug tools (class E). No decomp function was replaced. Later agents: slices G01 and G02 and the F01 sweep SHOULD use the warp and `-Turbo` instead of the 30 s to 75 s `fast autoa freshsave` chain.

### Switches (game side, in `debug3ds.txt` or `-Debug "..."`, code in `3ds/src/pm_warp.c` and `pm_input.c`)
- `preset=<name>`: one row of the table below (story, map, entry, flag).
- `warp=<map>:<entry>` (example `warp=mac_00:4`): map name and entry id. Overrides the map of a preset. Only the 6 linked maps work (hos_05, kmr_02, kmr_20, osr_00, kkj_00, mac_00). An unlinked map logs `warp: map X not found` and the normal start runs.
- `story=<name|number>` (example `story=ch0_toad_town` or `story=-107`): GB_StoryProgress. Names: intro, ch0_wake_up, ch0_hammer, ch0_goombario, ch0_toad_town, ch1_start, ch1_kooper, ch2_start, epilogue (table `stories` in pm_warp.c, add rows from include/enums.h). Alone it sets the story and warps to kmr_20 entry 0.
- How it works: the wrapper `__wrap_set_game_mode` (pm_map.c) sees the real `set_game_mode(GAME_MODE_LOGOS)` at the end of `state_step_startup`. The hook then does what the real new-file menu does (`filemenu_yesno.c` FM_CONFIRM_CREATE): `clear_player_data`, `clear_saved_variables`, area/map/entry, `evt_set_variable(GB_StoryProgress)` (and one GF flag if the preset has one), the real `fio_serialize_state`, and enters the real `GAME_MODE_ENTER_WORLD`. That state loads the map with `LOAD_FROM_FILE_SELECT` (real `fio_deserialize_state` and `load_map_by_IDs`). So logos, intro, title and file select are skipped, and the map script runs from the real entry. Without the switches nothing changes (normal chain checked at the end of W01: same scans 562, 2316, 2562, 3178, 0 unmapped).
- Log lines: `warp: <map> entry <e> (area a map m) story s`, `warp: scene frame0 F scan S t=ms`, `warp: scene fN t= pos x y z inputDisabled 0|1 actionState N` (every 100 frames, every 2 frames when `padscript` or `padlog` is on), `warp: smoke done`. `t=` is emulated time (same in normal and turbo).
- `padscript=A,wait30,right*60` or `padscript=@file.txt` (file in the SD app folder, comma, space or newline between tokens, max 160 steps). A token is `wait<n>` or buttons joined by `+` with an optional `*<n>` scan count (default 1): `A B Z START L R dup ddown dleft dright` and the stick `up down left right` (80 of 80). Unit = pad scan. The game reads the pad every 2nd scan, so 2 scans = 1 game frame (30 Hz). With `warp`/`preset`/`story` the script starts on the first frame of the scene (`padscript: start at scan N`), else at scan 0. At the end the real pad is back. `padrand` is unchanged. Hold A 20 scans or more for a full jump: a tap of 3 scans is a hop (ACTION_STATE_HOP).
- Example (walk, jump, walk back): `fast preset=toad padscript=wait80,right*40,A*24,wait60,left*40,A*24,wait30`. `wait80` covers the pipe entry (input is on from about frame 36 in the scene).
- Helper scripts (git-ignored): `3ds/logs/w01_test.sh <preset>`, `3ds/logs/w01_test2.sh <name> "<switches>"` (turbo smoke with a walk and jump script, position range summary).

### Presets (all tested: turbo smoke, 300 frames in the scene, `-FaultStop` 0 unmapped, no assert, no hang, no crash)
| name | story | map:entry | note |
|---|---|---|---|
| intro | STORY_INTRO | kmr_20:0 | real bed scene of a new file. The scene disables input for 300+ frames (by design): Mario is NOT controllable here |
| yard | CH0_ARRIVED_AT_TOAD_TOWN (-107) | kmr_20:4 | Mario's yard, enters from the pipe, GF_KMR20_ReunitedWithLuigi set (no Luigi scene). Controllable from frame 34. Walked x 594 to 654 |
| toad | -107 | mac_00:4 | Toad Town from the pipe. Controllable from frame 36. Walked x -100 to -5 |
| toad_east | -107 | mac_00:1 | east edge (x 600). Walking right enters the zone to mac_01: `GotoMap refused: area 1 map 2 is not linked` (3 lines). Input is then disabled by the exit script and Mario walks off the map and falls (y -1043) |
| toad_west | -107 | mac_00:0 | west edge (x -387), zone to kmr_10 (not linked either) |
| osr | -107 | osr_00:1 | x 612 to 710 |
| kmr02 | CH0_WAKE_UP | kmr_02:0 | x -4 to 96 |
- Not in the table (reason): `hos_05` (warp=hos_05:3 reaches the scene, but it is the intro cutscene map, no control). `kkj_00` (entry 6 with story -107 gives the ending scene, Mario never gets input in 300 frames). Other maps are id-only rows, refused by the guard. Presets with partner, items or badges: none made, no linked map needs them (clear_player_data gives boots normal, no hammer). Other story points work with `story=<number>`, but only the rows above were run.
- Evidence for G01 (not diagnosed in W01): `toad_east` shows the east zone fires and the load is refused (Q6/S30), so it is not a trigger bug.
- Evidence for G02 (not diagnosed): in `toad`, `toad_west`, `kmr02` a held A (24 scans) gives ACTION_STATE_JUMP for about 2 game frames and Mario stays at y 0 (max y 0 in toad_west and kmr02). Use `padscript=...A*24...` and read `warp: scene f.. pos`.

### Azahar speed (run_azahar.ps1 `-Turbo`, `-Smoke`)
- Azahar config (`%APPDATA%\Azahar\config\qt-config.ini`, [Renderer]): `frame_limit` (percent, default 100), `turbo_limit` (200, hotkey only), `use_vsync`. `-Turbo` sets `frame_limit=1000` and `use_vsync=false` for the run only. The 4 original lines go to `build3ds/qt-config.turbo-backup` and are restored in the finally block, and at the next start if a run was killed. Checked after the runs: `frame_limit\default=true`, `frame_limit=100`, `use_vsync=true`. `-TurboLimit <percent>`: 5000 gave the same times, so emulation speed is the limit.
- Switch `turbo` is added to the game switches (the log shows `switches ... turbo`). The script prints `TURBO run: not valid for performance numbers`.
- `-Timeout` and `-Stall` are divided by `-TurboFactor` (default 3, min 25 s and 10 s). The script prints the wall time at each map load, `scene frame0` and `smoke done`. The game log reaches the SD card late, so these times are late by up to about 1 s.
- `-Smoke` adds `smoke` (needs warp, preset or story): the game logs `warp: smoke done` 300 frames after `scene frame0`. The script stops there. Exit 0 = ok, 1 = failed, 2 = unmapped access or crash symbols.
- Game logic is frame driven and Azahar's clock is emulated: same switches, normal versus turbo, gave the same log. `fast autoa freshsave`: map loads at scans 562, 2316, 2562, 3178 in both. The logs differ only in `switches ... turbo`, `audio cost` lines (1 to 17 us) and where the run ended. The `warp: scene f` lines of a `padscript` run are identical (`t=` differs by 1 ms).
- Speedup (wall seconds from script start, one run each, +-1 s): `fast autoa freshsave`: kmr_20 27.9 normal, 14.8 turbo. mac_00 73.9 normal, 23.9 turbo (3.1x). kmr_20 to mac_00 alone: 46 s to 9 s (5x). `fast preset=toad` first scene frame: 22.3 normal, 13.7 turbo. `-Smoke` complete: 32 s normal, 16 s turbo. About 13 s is fixed (Azahar start, ROM resident load) and turbo does not shorten it.
- Limits: in turbo the Azahar audio output runs ahead (no ear check, expect crackle or silence). The port timers (phase timer, audio cost, fps) read `svcGetSystemTick`, which Azahar emulates, so they matched the normal run. The host GPU work is not charged, so turbo logs are NOT valid for performance numbers. `-FaultStop` works the same in turbo.
- Commands: `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "fast preset=toad"` (16 s). Normal chain: `-Debug "fast autoa freshsave" -Stall 40` (the normal-speed kmr_20 intro is silent in the log for more than 10 s).
- Modal dialogs: run_azahar.ps1 must never leave a modal dialog open, an unattended run stalls on it. The popup the user saw during W01 came from one command of this slice: `azahar.exe --help` (Azahar opens a usage message box instead of printing). Never start azahar.exe with other arguments than the .3dsx path. The -Turbo override only edits 2 keys of the real config and has no first-run or telemetry key effect. The script now reports `modal dialog suspected` when no game log appears 30 s after the stall time, or when the log goes quiet before the first `game mode` line.

## F01 paused (slice F01: fault sweep, paused for W01)

Note from W01: the walk, jump, wall and loading-zone paths that this note calls not exercised can now be run with `preset=toad` and `padscript` (see the Warp section). Scripted `START` opens the pause.

All items belong to slice F01. Azahar only. Nothing ran on hardware.
- Capture: Azahar logs `HW.Memory <Error> core/memory.cpp:UnmappedAccess:579: unmapped Read32 @ 0x00000010 at PC 0x00105000`. The log filter in qt-config.ini is `*:Info Debug.Emulated:Debug`, so these lines already show. No change needed. Log file: `%APPDATA%\Azahar\log\azahar_log.txt`.
- Tools built (commit c701a4bef): `run_azahar.ps1 -FaultStop` stops at the first UnmappedAccess line (reason "unmapped access"). Every run writes `build3ds/last_unmapped.txt` (each distinct access, count, address, PC). Switch `faulttest` reads address 0x10 at scan 300 (tested: detected, PC 0x105000). Switch `padrand` is a seeded random walk (stick, B, Z, START taps). Helper scripts in 3ds/logs (git-ignored): `f01_sweep.sh name timeout "switches" [until]` copies `.unm` and `.log`; `f01_a2l.sh addr...` runs addr2line; `f01_seg.gdb`, `f01_tex.gdb` are gdb scripts for `-Gdb -GdbScript`.
- Fault 1 FIXED (commit c701a4bef). Idle title (`fast`, no A) crashed on the first title frame. Azahar shows "Undefined Instruction", gdb shows SIGSEGV at `mdl_update_transform_matrices` (model.c:2682), PC 0x125A80 (0x125BB8 in the dialog), r5 = 0x5083B0. Cause: `src/state_intro.c` port edit (hos_04 not linked) jumped to the title and skipped the INTRO_PART_NONE cleanup, so `wModelList` kept stale pointers into the freed heap. Fix: set `introPart = NONE` and `startupState = INTRO_LOAD_MAP`, so the real cleanup block runs next frame. After: idle `fast` 330 s, title, then the title timer starts the new game flow, 0 unmapped, no crash.
- Runs with 0 unmapped lines and no crash: `fast autoa freshsave` 250 s, `fast autoa` 250 s and 200 s (after the fix, kmr_20, osr_00, kmr_20, mac_00 reached), `fast` 60 s, `fast autoa padrand freshsave` 420 s, `fast autoa padrand` 420 s.
- Limit of the random walk: after the refused GotoMap (area 1 map 2, B2) in mac_00, player input stays disabled, so walls, jumping and pause were NOT exercised. `padrand` never opened the pause (no `game mode` line). Walking and pause need W01 (warp or story presets) or a free-roam map.
- OPEN, not fixed: `tex ptr 0000fd7e 4x1 fmt 0` (log line, once, in the hos_05 intro just before the refused hos_04 load, idle path only). `pm_tex.c load_inner` returns 0 for image pointers below 0x100000. gdb: `pm_gbi_run(list=0x3ff560 in heap_spriteHead, nbytes=320)` from `nuGfxTaskStart`. The list bytes are random data (0x0755ce39 ...) then 0xffffffff, so a whole frame list is read from freed or unwritten sprite heap. Not an Azahar unmapped access. Cause not found. The `ptr_of` fallback in pm_gbi.c (returns an unknown segment pointer as is) and the `is_host` checks in pm_gbi.c are port guards that hide bad display lists. Next step: find who passes that list (state change frame, the frame after clear_model_data or general_heap_create).
- Not done: branch s19-full-battle sweep (`battle`, `demobattle`). A worktree exists at `3ds/logs/wt_battle` (detached at 7fd566684, assets and ver/us copied, main's pm_input.c copied in, not built). The branch is based on old main (autoa 1 press in 240 scans), so it needs the main `pm_input.c`. Remove with `git worktree remove --force 3ds/logs/wt_battle`. Q4 `update_player_shadow` access: cause read from code only: `get_shadow_by_index(shadowID)` returns NULL after a cleared shadow list, then `shadow->pos` writes through NULL. Not run.
- Not done (static sweep): bounds checks of array indexes in port code, unaligned LDM/VLDR (Azahar does not fault on them, so only static review or UBSan alignment can find them), unchecked `heap_malloc` in imgfx.c (anim->vtxCount buffers at ~1297), `pi_read_raw` zero fill past ROM size without a log line. The 47 `gPartnerStatus` defsyms: P01 plus a grep show they are only dma_copy arguments, no call.


## G02 result (slice G02: Mario could not jump)

All items belong to slice G02. Azahar only. Fix in the build setup (`3ds/CMakeLists.txt`), no decomp file changed.
- Root cause: `src/7B440.c` (`clear_player_data` / `init_player_status`) reads walk, run and max jump speed by pointer arithmetic: `floatsTemp = &(&DefaultMoveSpeed)[-1]`, then 3 floats. The decomp puts `DefaultWalkSpeed`, `DefaultMoveSpeed`, `D_800F7B78` next to each other in `src/world/actions.c`. The port builds with `-fdata-sections` and links with `--gc-sections`, so each variable had its own section. The linker moved `DefaultMoveSpeed` next to `GravityParamsStartFall` and dropped the two unused neighbours. Result: walkSpeed = a garbage word (0), runSpeed = 4 (right by luck), maxJumpSpeed = `GravityParamsStartFall[0]` = 0.154. `phys_update_jump` caps the integrator at maxJumpSpeed, so the real jump speed 15.7 became 0.154 and Mario stayed on the ground.
- Fix: `set_property(SOURCE src/world/actions.c APPEND PROPERTY COMPILE_OPTIONS -fno-data-sections -fno-toplevel-reorder)`. The file keeps its data as one block in source order. Log after: `walk 200 run 400 maxJump 32000` (x100, x100, x1000), the decomp values 2, 4, 32.
- Evidence tool: the `warp: scene f..` line now also prints `vy` (gravityIntegrator[0] x100), `maxJump` (x1000), `walk` and `run` (x100).
- Y trace, `fast preset=yard padscript=wait80,A*24,wait60` (frames in the scene, y of Mario):
  - Before: f42 y 30 (vy 0.15), f44 y 30 (vy -1.04), then y 30. No lift. maxJump 0.154, walk 0.
  - After: f42 41, f44 56, f46 66, f48 67 (peak, 37 above the floor), f50 64, f52 57, f54 46, f56 30 (landed). maxJump 32, walk 2.
- `preset=toad` with the default script (walk, jump, walk back, jump): two full jumps (y 0 to 37, 0 to 11+), landing between them, walking works.
- Other code that reads a neighbour variable through a pointer may break the same way. Not searched. If a similar bug shows up, check the section layout with `nm` first.
- Normal-speed check (slice G02): `fast autoa` 260 s, maps kmr_20, osr_00, kmr_20, mac_00 (area 1 map 1) at scans 410, 2164, 2410, 3026. 0 unmapped lines.

## D02 result (slice D02: code that depends on variable layout)

All items belong to slice D02. Azahar only. Fixes are in the build setup (`3ds/CMakeLists.txt`) and one port buffer (`3ds/src/pm_map.c`). No decomp file changed.
- Check used: warp checks (preset yard, preset toad, turbo, smoke, `-FaultStop`) plus ONE full normal chain at the end, because the D_80200000 fix touches file select and pause.
- Method: (1) grep over the 513 built `src/` files (list from build.ninja) for `&(&X)[n]`, `*(&X +- n)`, `&X +- n`, `((T*)&X)[n]`, `&X < &Y`, macros with these, negative indexes, comments about adjacency. Only `7B440.c` (fixed in G02) matched. (2) a second build dir with `-Warray-bounds=2 -Wstringop-overflow=4 -flto -Wlto-type-mismatch` (all `-w` removed): 34 array-bounds, 38 stringop and 15 type mismatch warnings, each read. (3) `3ds/tools/check_data_layout.py nm.txt [--scan]` checks that the listed groups are adjacent in source order in `arm-none-eabi-nm -n -S pm_3ds.elf`, and repeats the grep. Add a row to `GROUPS` for each new case.
- Hits: 4 real layout dependencies. 3 broken in the binary, all fixed. 1 not reachable.
  1. `src/world/actions.c` (G02, still fixed, 3 floats).
  2. `src/evt/script_list.c` and `src/evt/demo_api.c` SetSpriteShading. BROKEN, silent. `ShadingOffsetsBuffer`, `D_802DBB64`, `PackedShadingData`, `D_802DB8B6C`, `D_802DBB70[0x100]` are declared 4 bytes each, but the code reads 8 bytes and DMAs 0x100 bytes over them. All BSS uses `section(".bss")` so gc does not split it, but gcc emits BSS in REVERSE source order. Before (nm): D_802DBB70, D_802DB8B6C, PackedShadingData (0x6de8f8), D_802DBB64, ShadingOffsetsBuffer (0x6de900), then `D_802DB830` (item_api PopupMenu, 0x330 bytes). So every SetSpriteShading call wrote 244 bytes into D_802DB830 and 4 bytes past ShadingOffsetsBuffer. Fix: `-fno-data-sections -fno-toplevel-reorder` for that file. After: source order, adjacent (checker ok). No visible change seen in the warp runs (the popup is set before it is used), so no before/after values.
  3. `D_80200000` (heaps.c, 0x4000 bytes) is the hud element aux cache base: `state_pause.c` passes capacity 0x38000, `state_file_select.c` 0x20000. On the N64 it is expansion RAM. Here the next symbol was `AuHeapBase` (audio heap, 0x56000). BROKEN, silent: the cache could overrun the audio heap in pause and file select. Fix: heaps.c gets `-DD_80200000=pm_unused_80200000` (same method as gMapShapeData) and `3ds/src/pm_map.c` defines `pm_hud_aux_cache[0x38000]` as `D_80200000`. Cost: +0x34000 bytes (208 KB) BSS. Open: if RAM is short on the O3DS, alias it to another buffer that is idle in pause and file select (not checked). `D_80204000`, `D_80207000`, `D_8020A000` are only used by the dead kzn lava piranha code, not built.
  4. `src/main.c` gfxRetrace_Callback writes `ResetFrameBufferArray[0..2]` over a single `u16*` (N64 reset button path, `ResetGameState`). Not reachable: nothing in the port sets `ResetGameState`. Left as is.
- Not a dependency (read): `GameModeIncomplete` vs `GameMode` (reads stay inside 0x14 bytes), `mac_00_NpcData_SharedTownsfolk[9]` (decomp @bug, no adjacency), `MsgCharsets[5]`, `BgmTicksRates[8]`, `AU_FX_CUSTOM_PARAMS[0]` (same or larger real size), `status_bar_is_ignoring_changes` s8/s32 return type, `heap_*Head` HeapNode vs u8[] (address use), port-side `gBackgroundImage`/`gMapShapeData` buffers (bigger than the decomp type), `fio.c` casts (sizeof of the struct), `draw_box.c` quads (already uses gBoxQuadBuffer). Class 5 (defsym tables): nothing new, see P01.
- Per-file list or global `-fno-data-sections -fno-toplevel-reorder` on all decomp files? Measured (3dsx, elf): base 2692992 B file, text 1264404, data 1360064, bss 6906656. Global: 2829808 B file (+136816, +5.1%), text +6952, data +123840, bss +40512, total RAM image about +171 KB. Global also needs 3 exceptions to link (`entity/Block.c`, `entity/SimpleSpring.c`, `entity/model/SimpleSpring_anim.c`: their data points at render functions of block models that are not linked, and gc used to drop it). Per-file (actions.c, script_list.c): 0 bytes. Decision: per-file list, because the scan found only these files and the global flag costs about 171 KB for no known gain. If a later slice links more decomp data with neighbour reads, run `check_data_layout.py --scan` first.
- Side finding, NOT D02 (also at the commit before D02): pause opened by `padscript` START in `preset=toad` writes to tiny addresses (`unmapped Write16 0x1 @ 0x209EA`, 66756 times) in `gfx_frame_filter_pass_1` (`src/background_gfx.c:64`) and `gfx_transfer_frame_to_depth` (`:271`). It looks like the frozen frame pointer is not mapped to a real buffer. Needs a slice (F01 area). Command: `fast preset=toad padscript=wait80,right*40,A*24,wait60,START*3,wait120,START*3,wait60`.
- Checks after the fixes: yard jump same as G02 (y 41, 56, 66, 67 peak, back to 30 at f56), toad smoke 0 unmapped, `fast autoa freshsave` maps at scans 562, 2316, 2562, 3178 (same as before), 0 unmapped lines.

## H04 result (slice H04: the HUD texture is not there, DONE, Azahar, port layer only)

All items belong to slice H04. Seen by eye in `fast shots preset=yard` (shot 12, frame 360 of the scene). Files: `3ds/src/pm_gbi.c`, `3ds/stub_include/ld_addrs.h`. No decomp file changed.
- What the user saw (before): the status bar at the top was a flat white block (stepped shape, no border, no gold). HP, FP, coin icons and digits were there. A cluster of red and blue pixels sat next to the coin icon.
- Hypotheses: (1) bar never shown: REJECTED, the bar slides in and out in kmr_20 (shots 7 and 12) and the HUD elements draw. In `preset=toad` the bar did not show in 340 frames (map behaviour, not checked further). (2) hud element textures do not decode: REJECTED for the icons and digits, they draw right. (3) ROM remap dropped a range: REJECTED, the icon segment 0x1CC310 is in `pm_rom_keep`, no `ROM READ IN DROPPED RANGE` line. (4) render state: CONFIRMED for the window body.
- Cause 1 (window body, white): `draw_box` draws the status bar window (WINDOW_STYLE_5 and 6) with 2 cycles. Cycle 1 is `TEXEL0 * TEXEL1` (tile 0 = flat background `bg_flat`, I4 all 0xF, tile 1 = corner image, IA8, clamp), alpha = TEXEL1 alpha. Cycle 2 is `(PRIM - ENV) * COMBINED + ENV`. S27 only renamed TEXEL1 to TEXEL0 when TEXEL0 is not read, so this form drew tile 0 alone: white. Evidence: a temporary texrect log (removed) showed `comb 1 f 2 7`, `t0 4/0`, `t1 3/1`, prim ebe677ff env 8e5a25ff, use_t1 0.
- Fix 1: `box_fold_sync` (called before each textured triangle and texrect). When the combine mode has that exact form and tile 0 is all white, the draw is folded to `(PRIM - ENV) * TEXEL1 + ENV` with tile 1 (exact, because TEXEL0 is 1.0). Not exact for window styles with the `bg_tile` pattern background (styles 3 and 23): they keep the old path (not changed, not checked by eye).
- Cause 2 (bands and wrong corners after fix 1): the tile 1 image offset came from SetTileSize uls and ult. `gDPLoadMultiTile` (draw_box corners) loads at 0,0 and uses SetTileSize only to place tiles 1 to 3 at the far side. So quadrants 1 to 3 read the corner image from a wrong offset. Fix 2: with a LoadTile in effect, the source offset is the LoadTile origin. Also a new tile 1 image forces a rebind (`cur_tile = -1`), the old code kept the previous quadrant's texture bound.
- Cause 3 (red and blue pixels): the coin sparkle (`HES_Item_CoinSparkleRandom`, `hs_SetIcon`, 8x8 CI4) reads its raster and palette from the icon segment with `nuPiReadRom((s32)icon_ROM_START + raster, ...)` (`src/hud_element.c`). The port header `3ds/stub_include/ld_addrs.h` had `icon_ROM_START` = 1, so it read ROM offset 0x30241 (boot data), not 0x1CC310 + 0x30240. Evidence: the palette words logged at the sparkle draw were `8c5801b0 8c340044 ...` (not 5551 colours). Fix 3: `icon_ROM_START` = 0x1CC310 (splat.yaml `[0x1CC310, pm_icons, icon]`). This also covers every other `hs_SetIcon` user (item icons in menus, `item_entity.c`, `msg.c` item icons): NOT checked by eye there.
- After: shot 12 shows a gold panel with a darker lower edge, rounded stepped shape, HP 10/10, FP 5/5, star points 0, coins 0, the coin icon with no junk. Toad walk with padscript (`-FaultStop`, 340 frames): rc 0, 0 unmapped. Full normal chain NOT run (the slice does not touch boot, title, file select, intro or the map change path), but `icon_ROM_START` also changes icon reads in file select and pause: not checked by eye.
- Not part of this report and not checked: pause menu HUD, battle HUD.

## S30 result (slice S30: link all maps, Q6)

All items belong to slice S30. Azahar only. Memory numbers for the O3DS are ESTIMATED from Azahar-measured inputs (64 MB limit assumed, as in M02).

### What changed
- `src/world/world.c` is built whole with its real `gAreas` (421 map rows). `3ds/src/area_table.c`, `3ds/tools/check_area_table.py` and the CMake cut of world.c are deleted (stand-in B1 is gone).
- `3ds/tools/gen_map_sources.py` writes `3ds/maps.cmake` (real sources of each map, taken from the decomp `build.ninja`, plus every `src/entity/**` and `src/world/script_api/**` file the maps need), `3ds/stub_include/map_rom_ranges.h` (all map `_ROM_START/_END/_VRAM` are null: map code and data are linked in, no overlay DMA) and `3ds/map_syms.lds` (for a map that is not linked: `<map>_settings = pm_unlinked_map`, a sentinel in pm_map.c). `3ds/maps_skip.txt` lists what stays unlinked and why. Run `python 3ds/tools/gen_map_sources.py` after editing it.
- All 2,142 map files (418 maps) compile with the CMake flags and no stub. The 441 files that failed in S01 compile now. Real fixes: the generated `ver/us/build/include/recipes.inc.c` was missing (run `ninja ver/us/build/include/recipes.inc.c`; `extract_assets.py` now lists `.inc.c` targets), the `fx_sun_undeclared` alias from `undefined_syms.txt` (link option), `src/world/area_dgb/smash_bridges.c` (data segment of dgb_01, linked in place).
- The port guard (B2) stays for unlinked maps: `GotoMap refused` now gives the control back (see below).

### Memory (arm-none-eabi-size of the ELF, boot log, Azahar)
- Before S30: image 9,531,124 B (text 1,264,404, data 1,360,064, bss 6,906,656), 3dsx 2,692,992 B.
- All 418 maps linked: text 2,379,964, data 5,758,944, bss 7,292,944 = 15,431,852 B (+5.9 MB), 3dsx 8,563,360 B, `used-by-image` 15,088 KB, estimated O3DS spare 65,536 - 20,480 - 15,088 - 27,370 = 2,598 KB. That is under the 4,500 KB ceiling, so not all areas are linked.
- Linked (267 maps in 22 areas, last build): text 1,941,924, data 4,191,480, bss 7,264,712 = 13,398,116 B, 3dsx 6,437,736 B (6.14 MB). Boot log: `mem commit limit ... used-by-image 13104KB`, `mem map heap used 27370KB sbrk-left 17676KB linear free 12953KB`, `mem tick heap used 27375KB sbrk-left 17676KB linear free 11828KB tex-cache 1124KB`. Estimated O3DS heap left = 65,536 - 20,480 - 13,104 - 27,375 = 4,577 KB (above 4,500 KB). Estimated spare after the battle merge (+3.9 MB): about 0.7 MB. Azahar `sbrk-left` does not change with the image (Azahar offers 98 MB), so the estimate comes from `used-by-image`.
- Compile time: the full compile of all map files plus entities took about 2 minutes on 32 cores (Docker volume). One area step (generate, build, link, size, Azahar smoke) took about 75 to 80 seconds of build plus 20 seconds of run.
- Per area, measured one by one (cumulative, order mac, kmr, hos, osr, kkj, tik, nok, trd, kgr, iwa, dro, sbk, isk, mim, obk, arn, dgb, omo, then tst, end, mgm, gv). The first row includes the entity files (about 0.5 MB, needed by any map) and the 6 maps linked before. kpa was measured and did NOT fit (spare 4,442 KB, 58 KB under the ceiling), so it was removed again: its row is not part of the final image (the rows after it start from omo). tst, end, mgm, gv are small and fit after kpa was dropped.

| area | maps linked in total | files | image KB (used-by-image) | added KB | est. O3DS spare KB |
|---|---|---|---|---|---|
| mac | 13 | 122 | 9980 |  | 7706 |
| kmr | 27 | 197 | 10168 | 188 | 7518 |
| hos | 35 | 254 | 10316 | 148 | 7370 |
| osr | 39 | 273 | 10356 | 40 | 7330 |
| kkj | 63 | 403 | 10668 | 312 | 7018 |
| tik | 85 | 521 | 10900 | 232 | 6786 |
| nok | 94 | 583 | 11148 | 248 | 6538 |
| trd | 105 | 632 | 11316 | 168 | 6370 |
| kgr | 107 | 639 | 11324 | 8 | 6362 |
| iwa | 114 | 679 | 11444 | 120 | 6242 |
| dro | 116 | 693 | 11536 | 92 | 6150 |
| sbk | 166 | 821 | 11724 | 188 | 5962 |
| isk | 183 | 908 | 11840 | 116 | 5846 |
| mim | 195 | 987 | 11980 | 140 | 5706 |
| obk | 204 | 1050 | 12136 | 156 | 5550 |
| arn | 215 | 1109 | 12236 | 100 | 5450 |
| dgb | 234 | 1212 | 12560 | 324 | 5126 |
| omo | 251 | 1319 | 12904 | 344 | 4782 |
| kpa | 301 | 1540 | 13244 | 340 | 4442 |
| tst | 260 | 1335 | 12924 | -320 | 4762 |
| end | 262 | 1355 | 13036 | 112 | 4650 |
| mgm | 266 | 1370 | 13092 | 56 | 4594 |
| gv | 267 | 1372 | 13096 | 4 | 4590 |

- Not linked, measured or estimated: kpa +340 KB (measured). kzn about 229, flo about 355, jan about 450, sam about 327, pra about 287 KB (ESTIMATED: the measured total of all 418 maps minus the linked ones, split by object size).

### Maps left id-only (154 maps, listed in `3ds/maps_skip.txt`)
- Areas kpa (50 maps, Bowser's Castle), kzn, flo, jan, sam, pra: memory ceiling only. Nothing else blocks them, except the three maps below.
- `kmr_22`, `kzn_19`: `common/lava_piranha/part2.inc.c` needs `world_model_anim_kzn_00..20_ROM_START/_END/_VRAM`, the ROM data of `src/world/model_anim/kzn/*.c`, DMA'd to one VRAM address. Needs slice S19b.
- `flo_11`: `main.c` calls `func_80044238` (src/encounter_api.c) which calls `func_80072BCC`, defined in `src/battle/battle.c` (battle tree, Q1).
- Question Q7 in `3ds/QUESTIONS.md` asks how to link the rest.

### Refused GotoMap no longer freezes Mario (task 4)
- Cause: the exit scripts (`EVT_EXIT_WALK` and the door and pipe variants) run `Exec(ExitWalk)`, which calls `DisablePlayerInput(true)` AFTER `GotoMap` returned. Nothing enables the input again, because a real map change clears every script. The first idea (enable the input inside the guard) was too early: the Exec'd script disabled it right after.
- Fix (port guard, `3ds/src/pm_map.c`, no decomp change): `pm_map_tick` (called once per drawn frame from pm_main.c). After a refusal it waits until the scripts of group EVT_GROUP_EXIT_MAP are gone (or 200 frames), then calls the game's own `enable_player_input`, `partner_enable_input`, `func_800E01DC`, `enable_status_bar_input`, clears `GLOBAL_OVERRIDES_40`, and puts Mario back where he stood 45 frames before (history of the frames with input on), idle. Without the move the exit zone fires again at once and Mario stays frozen in a loop.
- Test (temporary id-only row for mac_01, then removed): `fast preset=toad_east padscript=wait80,right*120,wait200,left*30,wait30` gives `GotoMap refused` at f46, `control back after 103 frames (exit scripts ended)`, `Mario back to 600 0 0 (was 686 0 0)`, inputDisabled 0, and Mario walks left (x 600 to 531). 0 unmapped.

### East zone of mac_00 (G01)
`fast preset=toad_east padscript=wait80,right*100,wait260,left*140,wait300` (turbo, fixed 50 s timeout): `load map area 1 map 2` (mac_01) at scan 198, Mario controllable at x -581, walks out of the west zone, `load map area 1 map 1` (mac_00) at scan 566, x 648 to 513. 0 unmapped. G01 is done: the cause was Q6 (mac_01 was id-only).

### Presets (turbo, -Smoke, -FaultStop)
toad, toad_east, toad_west, yard, kmr02, osr: all rc 0, 0 unmapped, 0 refusals.

### Two real bugs found by the link (fixed, not map specific)
- `ENTITY_ADDR(entity, type, data)` (include/entity.h) is `gfxBaseAddr + (data & 0xFFFF)`. On the N64 the symbol is an offset in the DMA'd model. Here the model data is linked in place, so the low 16 bits of a host address are layout dependent. With sbk linked, kmr_02 (SaveBlock) drew a display list from the wrong place in the entity heap: a garbage list, 600 KB of overwrite and unmapped writes at the end of the image. Fix: under `TARGET_3DS` the macro is `(type)(data)` (shared decomp header, `#ifdef` as in src/entity.c). Visual check of the save block: NOT done (logs only).
- `pm_gbi.c` G_VTX: a load that starts past slot 64 (`v0 >= 64`) made `n = 64 - v0` negative, so `memcpy` copied 4 GB. Now `n = 0`. A garbage display list can still be drawn. The guard hides bad lists (see the F01 notes).

### Map tour (tour switch, class E, `3ds/src/pm_warp.c`)
`warp=<first>:0 tour=@tour.txt` visits a list of maps through the real GotoMap path (40 frames each, `-Until "tour: done"`). `tour.txt` is in the SD app folder, `map:0` per line. Scripts (git-ignored): `3ds/logs/s30_tour.sh` (chunks of 40, restarts after a fault), result `3ds/logs/s30_tour_result.csv`. Entry 0 of each map, state of a new file (flags unset, no partner). Result: 266 maps visited, 251 without an unmapped access in 40 frames, 15 with. The tour does not detect a black screen or a wrong picture, only unmapped accesses, crash symbols and stalls. The map column can be one map late (the fault shows while the next map is requested): the function name tells the real map. No assert was seen.

| map (as listed by the tour) | result |
|---|---|
| mac_05 | FAIL: stopped after 19 s: unmapped access  77761x at PC 0x00127B40 mdl_local_gfx_update_vtx_pointers at /work/src/src/model.c:4088 |
| mac_06 | FAIL: stopped after 16 s: unmapped access  80898x at PC 0x00127B30 mdl_local_gfx_update_vtx_pointers at /work/src/src/model.c:4088 |
| tik_07 | FAIL: stopped after 16 s: unmapped access  40x at PC 0x00218CA4 tik_06_CheckDripCollisionWithNPC at /work/src/src/world/area_tik/tik_06/../common/DripVolu |
| tik_15 | FAIL: stopped after 16 s: unmapped access  42x at PC 0x0021EB44 tik_14_CheckDripCollisionWithNPC at /work/src/src/world/area_tik/tik_14/../common/DripVolu |
| kgr_01 | FAIL: stopped after 17 s: unmapped access  15410x at PC 0x00127B20 mdl_make_local_vertex_copy at /work/src/src/model.c:4117 |
| kmr_10 | FAIL: stopped after 17 s: unmapped access  152x at PC 0x001D5D70 SpeakToPlayer at /work/src/src/evt/msg_api.c:21 |
| isk_14 | FAIL: stopped after 27 s: unmapped access  56175x at PC 0x00130CD4 set_script_flags at /work/src/src/evt/script_list.c:950 |
| nok_01 | FAIL: stopped after 18 s: unmapped access  1x at PC 0x00248580 trd_06_InitializeFallingPartner at /work/src/src/world/area_trd/trd_06/falling.c:148 |
| hos_02 | FAIL: stopped after 20 s: unmapped access  98982x at PC 0x00127B30 mdl_local_gfx_update_vtx_pointers at /work/src/src/model.c:4088 |
| hos_04 | FAIL: stopped after 16 s: unmapped access  74446x at PC 0x00127B30 mdl_local_gfx_update_vtx_pointers at /work/src/src/model.c:4088 |
| kkj_28 | FAIL: stopped after 28 s: unmapped access  6x at PC 0x001B09CC partner_walking_enable at /work/src/src/world/partners.c:1136 |
| omo_12 | FAIL: stopped after 24 s: unmapped access  383x at PC 0x002957AC omo_11_UpdateRotatingPlatforms at /work/src/src/world/area_omo/omo_11/gizmos.c:185 |
| tst_03 | FAIL: stopped after 17 s: unmapped access  675x at PC 0x002985EC omo_16_SetNpcPosYaw at /work/src/3ds/../src/common/SetNpcPosYaw.inc.c:10 |
| tst_12 | FAIL: stopped after 17 s: unmapped access  28410x at PC 0x0015F24C floating_flower_main at /work/src/src/effects/floating_flower.c:31 (discriminator 1) |
| gv_01 | FAIL: stopped after 18 s: unmapped access  116545x at PC 0x0012FC9C find_script_labels at /work/src/src/evt/script_list.c:136 |

Causes (grouped, new rows in SLICES.md): `mdl_make_local_vertex_copy` / `mdl_local_gfx_update_vtx_pointers` (model.c 4088, 4117) in mac_05, mac_06, kgr_01, hos_02, hos_04 (row V04). Null reads in scripts, NPCs and partners (row F02): `find_script_labels` (gv_01, script_list.c:136), `set_script_flags` (isk_14, script_list.c:950), `SpeakToPlayer` (kmr_10, msg_api.c:21), `CheckDripCollisionWithNPC` of tik_06 and tik_14 (DripVolumes.inc.c:20, NPC missing), `trd_06_InitializeFallingPartner` (no partner in a new file), `partner_walking_enable` (kkj_28, no partner), `omo_11_UpdateRotatingPlatforms`, `omo_16_SetNpcPosYaw`, `floating_flower_main` (tst_12). Several may come from the tour state (new file, no partner, entry 0), not from the maps. Not diagnosed beyond the first fault.

### Full normal chain (slice S30 touches the map change path)
`fast autoa freshsave -FaultStop`, normal speed: kmr_20 (scan 562), osr_00 (2316), kmr_20 (2562), mac_00 (3178), 0 unmapped, same scans as before S30.

## R01 result (slice R01: ROM-address symbol audit, DONE, Azahar, port layer only)

All items belong to slice R01. No decomp file changed. Checked on Azahar with the warp (`-Turbo -Smoke`) and the map tour. The full normal chain was NOT run: the slice does not touch boot, title, file select, the intro or the map change path.
- Tool (slice R01): `python 3ds/tools/rom_symbol_audit.py` (repo root, needs PyYAML) lists every `*_ROM_START`, `*_ROM_END`, `*_VRAM` the build defines (macros in `3ds/stub_include/*.h` and `--defsym` lines in `3ds/CMakeLists.txt`) with its value, and compares it with `ver/us/splat.yaml`. Verdicts: REAL (START and END equal splat), CODE (placeholder, splat type code), DATA? and BAD (not reviewed, exit status 1), VERIFIED (reviewed by hand, list `R01_VERIFIED` in the script). `-a` also prints the map rows.
- Result on main: 787 symbol groups. CODE 596 (421 maps, 128 effect code, 31 world (partner, action, use_item), 4 entity code overlays, 11 other engine overlays, pause, filemenu), REAL 188 (112 `effect_gfx_*`, 71 `entity_model_*`, charset, logos, title_bg_1, icon, imgfx_data), VERIFIED 3, DATA? 0, BAD 0. The battle ranges (`battle_ld_addrs.h`) exist only on branch `s19-full-battle`, not checked here. Their ranges are empty by construction (P01 and M02 note).
- Placeholders that are CODE (verified, never read): every map `_ROM_START/_END/_VRAM` is NULL (S30, map code is linked in place), every `effect_<name>` code overlay is `(u8*)1` (`effect_ld_addrs.h`), `engine1/2/4`, `evt`, `entity*`, `world_script_api`, `world_action_*`, `font_width`, `speech_bubble`, `pulse_stone`, `i_spy`, `inspect_icon`, `effect_globals` (all `(u8*)1`), the 47 `gPartnerStatus` defsyms (P01: partners, `world_use_item`, pause, filemenu, `ui_images_filemenu_pause`). All have START == END, so `dma_copy` gets length 0 and reads nothing. Their `.data` images are linked in place from the PNG sources (a missing one would fail at link). `obfuscated_*` shims are port functions in `game_stubs.c`, no DMA.
- DATA symbols found with a placeholder: 0. Hand-typed values that were wrong but not read (fixed by the generator below): `charset_ROM_END` 0x123128 (real 0x131340), `imgfx_data_ROM_END` and `_VRAM` = 1 (real END 0x315B80, only START is used), `icon_ROM_END` missing, `title_bg_1_ROM_END` was 0x24B7F0 (all title images, real segment END 0x2238C0, unused). `icon_ROM_START` was the H04 bug.
- Fix at the root (slice R01): `3ds/tools/gen_data_rom.py` writes `3ds/stub_include/data_rom_addrs.h` (charset, logos, title_bg_1, icon, imgfx_data) from splat.yaml. The hand-typed lines are gone from `ld_addrs.h`. START values are identical to before (so no visible change is expected). To add a data segment: add its name to `STEMS` and run the script. The audit marks any data stem that has a placeholder value.
- Other checks (all equal to splat): `charset_offsets.h` 34 of 34 offsets, entity model ranges 71 of 71 (`BombableRock` END 0xEA0900 on purpose, M02), `sprite_shading_profiles_data_ROM_START` 0x315D50 = START + 0x1D0 (`undefined_syms.txt`), the literals `SPRITE_ROM_START` 0x1943000, `MSG_ROM_START` 0x1B83000, `ASSET_TABLE_ROM_START` 0x1E40000, `SBN_ROM_OFFSET` 0xF00000, `ASSET_TABLE_START` 0x1E40020 in pm_os.c. `icon_offsets.h` is the generated file of the decomp build. The M02 note that `effects.cmake` sets the effect graphics start to NULL is stale: `effect_gfx_*` have the real offsets (REAL above) and `effects.c` loads them.
- Permanent debug check (slice R01, `3ds/src/pm_os.c rom_suspect`): once per address, through the non-blocking log, a line `ROM READ AT TINY OFFSET`, `ROM READ IN DROPPED RANGE` or `ROM READ PAST END OF ROM`. With the switch `romstrict` the same read also logs `FATAL: romstrict ...` and stops the program (`pm_fatal_stop` in `pm_main.c`: log flush, `[CRASH] debug check` line, svcBreak; a null write lost the queued log lines under Azahar, so it was not used). `crash.txt` and the log hold the reason, the run script stops on `[CRASH]`. Tested with a temporary header (charset START 0, icon START in a dropped range, then restored): the 3 line types came out once per address, `romstrict` stopped the run at the first one.
- Limit of the tiny-offset test: the H04 bug read offset 0x30241 (placeholder 1 plus a raster offset), not a tiny offset. The check alone would not have caught it. The segment classifier catches that class: `python 3ds/tools/rom_trace_classify.py <romtrace files>` maps each read of a `romtrace` file to its splat segment and flags TINY, ODD (odd start offset) and CODE (data read inside a code segment).
- Trace check (measured): presets yard, toad, kmr02, osr (300 frames) and the S30 map tour of 266 maps (7 chunks, restarts after a fault), all with `romtrace`. 0 `ROM READ` lines in 22 logs. The classifier over all traces gives reads in 55 segments: charset, icon, imgfx_data, sprite_shading_profiles, effect gfx, entity models, audio, sprites, mapfs, msg. No TINY, no ODD. One CODE hit: `entity_model_HiddenPanel` (splat type code, the data is a model, real). Tour result: 250 maps without a fault, 16 with (same causes as S30: mdl_local_gfx_update_vtx_pointers, script null reads, partner null reads; the map names differ by one because the tour lags by one map). Rows V04 and F02 cover them.
- Seen by eye: nothing new. The START values did not change, so the item icons, letters and story pages read the same bytes as before. Pause and file select item icons were NOT looked at (pause with START faults, row D03). Not seen: message icons.
- H04 notes: the `bg_tile` windows (styles 3 and 23) and the toad status bar that never showed in 340 frames are not caused by a ROM symbol (no placeholder found, no unusual read). No row added.
- Files: `3ds/tools/rom_symbol_audit.py`, `gen_data_rom.py`, `rom_trace_classify.py`, `3ds/stub_include/data_rom_addrs.h`, `ld_addrs.h`, `3ds/src/pm_os.c`, `pm_main.c`, `3ds/include/pm_port.h`. Debug switch list: add `romstrict` next to `romtrace`.

## F02 result (slice F02, with V04: tour faults, DONE for the fixed groups, Azahar, no stub)

All items belong to slice F02 (V04 had the same kind of cause and is done here). Check used: warp (`-Turbo -Smoke -FaultStop`) per map, presets yard toad kmr02 osr intro toad_east (all 0 unmapped, 0 assert), the S30 map tour (266 maps), and the full normal chain `fast autoa freshsave` (maps at scans 562, 2316, 2562, 3178, same as before, 0 unmapped), because the NPC change touches the map change path and the intro. Gdb (`-Gdb -GdbScript`, `break find_script_labels if script->ptrNextLine == 0`) gave the call chain of each fault.
- Before: tour of 266 maps, 251 clean, 15 to 16 faulting (S30 and R01). After: with `partner=1` 264 clean, 2 faulting (iwa_01, tst_12, both explained below). Without a partner (the old tour state) 254 clean, 12 faulting: the partner users, iwa_01 and tst_12.
- Fix 1 (V04, commit c6aee52ab): `mdl_local_gfx_update_vtx_pointers` (src/model.c) read the display list word with `*(u64*)gfx >> 32`. That is the first word only on a big-endian CPU. On ARM it gave w1, so the loop never saw G_ENDDL, ran over the heap and wrote into the heap block headers (heap full, then NULL stores, tens of thousands of unmapped writes). 3DS branch: `nodeDlist->words.w0`. Fixes mac_05, mac_06, kgr_01 and the model copies of hos_02, hos_04.
- Fix 2 (commits 2d343c464 and 700960e31, src/encounter.c `create_encounters`): `if ((s32) npcData->init < EVT_LIMIT)` means "is a pointer" on the N64 (pointers are negative as s32). A host pointer is positive, so EVERY NPC init script in EVERY map was dropped. 3DS branch: init is a script when `(u32) init >= 0x100000` (image start). Some maps (machi 0x1F64, kzn_04 and trd_08 0x4003) hold a small number there that is not a pointer. This cleared the null reads in mac_05 (`EVS_SetupWhale` reads an animation that the whale init script sets), isk_14, kmr_10, kkj_28. WARNING for later slices: NPCs now run their init scripts, so behaviour in every map changed (it now matches the decomp). Nothing was checked by eye.
- Fix 3 (commit 35df9d1c7, 3ds/CMakeLists.txt): map files that end a script without End (decomp @bug "not terminated": gv_01, kmr_11, kkj_15, kkj_16, kkj_23, kkj_29 main.c). `find_script_labels` reads on into the next data until it finds End. With -fdata-sections the next bytes were zeros and the scan ran 8 MB past the image (gv_01). Now `-fno-data-sections -fno-toplevel-reorder` for these files (D02 method). The trd_06 `unterminated switch` is a run time script bug (not changed).
- Tool: warp switch `partner=<n>` (3ds/src/pm_warp.c, class E, PARTNER_* id, 1 = Goombario). Tour script copy with a map list: `LIST=<file> TAG=x SW="partner=1" bash 3ds/logs/f02_tour.sh` (git-ignored).
- Remaining faults, each with a documented cause (no port bug found):
  - Partner needed (tour state: no partner, entry 0): `partner_walking_enable` / `enable_partner_ai` (hos_02, hos_04, hos_06, kkj_27: `wPartner` is NULL with PARTNER_NONE and the map script calls `EnablePartnerAI`), `CheckDripCollisionWithNPC` (tik_01, tik_06, tik_14, tik_21: `get_npc_unsafe(NPC_PARTNER)` is NULL), `trd_06_InitializeFallingPartner`, `SpeakToPlayer` of kmr_11, `BringPartnerOut` (arn_08), `omo_11_UpdateRotatingPlatforms`, `omo_16_SetNpcPosYaw` (tst_03). The same code runs on the N64, so these maps expect a partner. All clean with `partner=1`. Not verified on the N64.
  - iwa_01 in the tour (`func_80240DA4_9087D4`, kmr_23/npc.c:413): the curtain draw callback that kmr_23 sets stays set when the tour leaves kmr_23 after 40 frames. `warp=iwa_01:0` alone is clean. Tour artifact (cause from reading the code, a normal exit from kmr_23 was not run).
  - tst_12 (`floating_flower_main`, floating_flower.c:31): cut test map. Its script uses `Loop(LVar5)`, and `evt_handle_loop` stores the raw argument, so the count is the variable id (about 30 million). It makes effects until the 96 effect slots are full, then `general_heap_malloc` returns NULL and the code writes through NULL. Same code on the N64. The map is unreachable in the game.
- Found while checking, not fixed, row F04: `resolve_npc` (src/evt/npc_api.c:11), player_api.c:403, WhaleAnim.inc.c:7, Bandit.inc.c:20 and `evt_find_label` (evt.c:2039) use `>= EVT_LIMIT` to tell an NPC id from an `Npc*`. A host pointer is positive, so it reads as an id. 7 maps use `GetNpcPointer`. No fault seen in the tour.

### F01 open item "tex ptr 0xfd7e" (slice F02, cause found, row F03)
- Found by F01 in `fast` (idle, no A, no freshsave), map hos_05 (area 5 map 5, the intro), about scan 3000, every run. Log line `tex ptr 0000fd7e 4x1 fmt 0`, printed once by `pm_tex.c load_inner`. It is not an unmapped access. It still appears after S30.
- Reproduce (about 40 s): `powershell -File 3ds/run_azahar.ps1 -Turbo -Timeout 150 -Until "load map area 5 map 4" -Debug "fast"`, then grep `tex ptr` in the output. No warp reproduction was tried (warp=hos_05:0 is the same scene).
- Cause (gdb hardware watchpoint on the display list slot plus a temporary G_SETTIMG log, both removed): the list holds `SETTIMG RGBA16 width 320 addr 0xFC80`, SETTILE, LOADTILE 4x1 at x 127, then `SETCIMG` to `DepthCopyBuffer`. That is the depth query in `is_point_visible` (src/model.c:4497, called by `bulb_glow_appendGfx`, the lamp in the hos_05 intro). It loads 4 pixels of the N64 z-buffer: `osVirtualToPhysical(&nuGfxZBuffer[outY * 320])`. `nuGfxZBuffer` is a zero BSS pointer that the port never sets, so the address is NULL + 101 * 640 = 0xFC80. The F01 idea (list read from unwritten sprite heap) was wrong for this line.
- Effect: `pm_tex` rejects the image, the copy into `DepthCopyBuffer` never runs, `DepthCopyBuffer` stays 0 and every depth query decodes depth 0. Visibility of glow effects comes from that. Not a data abort. Real fix needs a depth read back in the platform layer: row F03.
- Ruled out: bulb_glow's own `nuGfxCfb_ptr` loads (log never fired), `nuGfxCfb_ptr` NULL (set each frame in pm_main.c), ENTITY_ADDR, heap_spriteHead lists, 16 bit truncation of a host pointer.

## D03 result (slice D03: the pause menu, DONE, Azahar, port layer plus 2 small TARGET_3DS edits)

All items belong to slice D03. Azahar only. Nothing ran on hardware.

### Root cause of the fault
- `nuGfxZBuffer` was NULL and `nuGfxCfb[0..1]` were 2-byte tokens (S02). The pause code (`gfx_draw_background`, src/background_gfx.c) saves the last frame into the "z-buffer" and filters it on the CPU. Write at 66756 = (104 * 320 + 98) * 2 = NULL + pixel offset. Same kind as F03.

### Fix (what the pause screen reads now)
- `nuGfxCfb` lists the real `gFrameBuf0..2` (heaps3.c), `nuGfxCfbNum` 2. `nuGfxCfb_ptr = nuGfxCfb[frame % nuGfxCfbNum]` (`pm_main.c`).
- `pm_cfb_capture_prev` (pm_main.c) copies the last finished frame from the PICA colour buffer into the frame buffer that is not being drawn (the N64 "previous colour buffer"), N64 RGBA5551, native u16. `pm_gpu_read_frame` (pm_gpu.c) does the read (tiled RGBA8, 240 x 400 rotated, game x = long axis + 40, game y runs back). It runs only on request, once per drawn frame: (a) in `pm_gbi.c` `G_SETTIMG` when the source is a frame buffer (`draw_prev_frame_buffer`, firework, dizzy dial, OVERLAY_BLUR), (b) at the VISCVG fill of `BACKGROUND_RENDER_STATE_BEGIN_PAUSED`. The PICA colour buffer keeps frame N-1 until `C3D_FrameEnd` of frame N, so the read is valid during the whole display list build and run.
- VISCVG (pause begin) extracts per-pixel edge coverage into a frame buffer. The PICA has no coverage, so `G_FILLRECT` in VISCVG mode into a frame buffer writes the full-coverage value 0x8421 to that memory and draws nothing on the screen (it painted a gray rectangle before). The real filter then copies each pixel (`zBuffer = fb0 | 1`). The edge smoothing of the N64 is NOT reproduced (no coverage).
- `nuGfxZBuffer` is allocated (150 KB, malloc) at the first pause (`pm_zbuf_need`). Before that it stays NULL (F03 unchanged).
- `pm_tex` reads frame-buffer and saved-frame memory as native u16 (`pm_fb_native`); ROM textures are big-endian bytes.
- Measured (Azahar timing, NOT hardware): the read costs 8.7 ms once per pause open (log line `D03: last frame copied to a frame buffer in N us`, phase `fbread` in `phase` output, max 8.7 ms). A draw that reads the previous frame every frame (firework blur, OVERLAY_BLUR) pays 8.7 ms per frame. VRAM reads are uncached on hardware, so it may be slower there. UNVERIFIED on hardware.

### Pause menu faults found by eye, causes, results
Shots of the Stats, Items (key items and items lists), Spirits and Map pages (`fast preset=toad shots padscript=...R*4...`, sheet by `build3ds/sheet.py`).
1. Faint beige or blue checker over the whole screen, panels see-through. Cause: `draw_box` windows with the `bg_tile` pattern (combiner `(PRIM - ENV) * TEXEL0 + ENV`, tile 0 = I4 16 x 16 pattern, alpha from TEXEL1 = corner image). The port took the alpha from the pattern texture (0 or 1, so a transparent checker) and had no tile 1 border. Fix: `box_pattern_rect` (pm_gbi.c) composes TEXEL0 * TEXEL1 (colour) and TEXEL1 alpha on the CPU into an RGBA8 texture (128 x 128 chunks, cached by content in `pm_tex_load_rgba8`), then draws with `(PRIM - ENV) * texel + ENV` (combine mode 5). Combine mode 7 (same, alpha opaque) is the fallback when the format is not handled. The pattern is a real 2-tone checker in the game (beige on the stats page).
2. Scene behind the menu not darkened. Cause: `PM_CC_43` = `(TEXEL0 - PRIM) * PRIM_ALPHA + PRIM` was drawn as the plain texture (and `tex_on` is 0 for texture rectangles after `gSPTexture` was not called). Fix: combine mode 6 in pm_gpu.c, detected in `sync_combine` without `tex_on`.
3. Stats page text drawn many times as "Check here to see what Mario's current status is". Cause: `gPauseMessages` holds message IDs and pointers to message buffers (`gPauseMsg_*`). The N64 tells them apart by the sign (KSEG0 pointers are negative). A 3DS pointer is positive, so `draw_msg` ran `dma_load_msg` on an address (log lines `ROM READ PAST END OF ROM`). Fix (TARGET_3DS): `MSG_BUF_ID` / `MSG_BUF_PTR` in include/macros.h (bit 31 tag), the 6 sites in src/msg.c that cast `msgID` to a pointer, and `pause_get_menu_msg` tags values >= 0x2F0000 (largest ID is below 0x2F0000, last section 0x2E). A static initializer cannot hold the tagged address (3dsx "Generic load error"). Row F05: the same problem in shops.c and Credits.inc.c.
4. Quad one pixel too wide. Cause: `pm_gpu_tex_rect` takes an inclusive lower right pixel, G_TEXRECT in 1-cycle mode is exclusive. Every textured rectangle was stretched by one pixel. Visible as a zoom drift of a previous-frame blur drawn each frame. Fix: pass `xh/4 - 1`, `yh/4 - 1` outside COPY mode.
5. Padscript: `char v[40]` cut an inline `padscript=` after 39 characters (the START presses were dropped). Now 200.

Rounded corners: the composite carries the corner image alpha, so the box shape comes from the real corner image.

### Pages checked and not checked
- Checked by eye: Stats (name, level, HP, FP, BP, star points, coins, star pieces, play time, boots, hammer), Items tab (key items and items lists), Spirits (7 star spirit pictures in a framed box), Map, tab header, open and close with START, the world after closing (Mario walks: x -100 to -13 after `right*40`).
- Legitimately empty on a new file (do not report again): item list, key item list ("None"), badge list ("None"), partner page, Star Energy bar, star spirit slots without spirits. The "Badges" and "Party" sub-pages and the pause with items, badges or a partner were NOT reached (no switch gives a new file items). Quit/system page: not reached.
- Not done: edge smoothing of the saved frame (needs coverage), depth readback (F03). Shared design for F03: `nuGfxZBuffer` is the same 150 KB buffer (`pm_zbuf_need`). A depth read needs the PICA depth buffer (D24S8, tiled) copied to 16-bit N64 depth in the same lazy way: capture on the first `G_SETTIMG` whose address is inside `nuGfxZBuffer` when no pause is active, only the rows `is_point_visible` loads (4 pixels per call, `outY * 320`). It cannot be used while the pause holds the saved frame in the same buffer.

### Checks
- `fast preset=toad padscript=wait80,right*40,A*24,wait60,START*3,wait120,START*3,wait60 -Turbo -Smoke -FaultStop`: 0 unmapped, `game mode 10` (pause), 11, 4 (world). Page tour (L/R tabs): 0 unmapped, 0 `ROM READ PAST END`.
- Warp checks (preset=toad) for the pause. The rectangle fix and frame buffers touch all rendering, so the normal chain was also run: `fast autoa freshsave -Turbo`, map loads at scans 562, 2316, 2562, 3178, 0 unmapped (same as before).
- Test switch `prevtest` (pm_warp.c, class E): from scene frame 100 it sets the game's own OVERLAY_BLUR front overlay with the real `set_screen_overlay_params_front` and `set_screen_overlay_center`, so a previous-frame draw shows. Result: 6 captures logged, picture stable, no drift.
- Run files: 3ds/logs/d03_*.txt (git-ignored).

## P02 result (slice P02, with F04 and F05: N64 address and ID range assumptions, DONE, Azahar, no stub)

All items belong to slice P02. Azahar only. Nothing ran on hardware. Check used: warp tour (`-Turbo -FaultStop`, `partner=1 ptrcheck`), 7 presets with a walk script, 3 warps, and the full normal chain (the NPC and message tests touch the intro scripts).

### Evt value encoding (include/script_api/macros.h, src/evt/evt.c `evt_get_variable` and siblings)
| s32 value | meaning | 3DS host pointer (image 0x100000 to about 0xD00000, heap from 0x08000000, linear heap 0x14000000 and up, all positive) |
|---|---|---|
| <= -270000000 (EVT_LIMIT) | literal. N64 KSEG0 pointers (0x80000000 = -2147483648 and up) are here | never here |
| -270000000 to -250000000 | literal (EVT_IGNORE_ARG is -250000000) | never here |
| -250000000 to -220000000 | `Float()` fixed point, (value + 230000000) / 1024 | never here |
| -220000000 to -20000000 | variable references in bands of 20000000: array flag, array var, game byte, area byte, game flag, area flag, map flag, local flag, map var, local var | never here |
| above -20000000 (all positive values) | plain integer literal | HERE. A host pointer is read and written as an integer, which is right for a pointer |
- Result: the evt readers and writers (`evt_get_variable`, `evt_set_variable`, `evt_get_float_variable`, `evt_set_float_variable`, `evt_get_variable_index`) need no change. A pointer in a variable round trips.
- What breaks is the TEST written for N64 pointers: `value < EVT_LIMIT` (pointer) or `value >= EVT_LIMIT` (ID), `msgID < 0` (buffer), `(s32) p < 0`. On the 3DS those tests say "ID" or "number" for every pointer. S30 (ENTITY_ADDR), F02 (NpcData init) and D03 (msg buffers) were this one cause.

### Shared helpers (TARGET_3DS inside the macro, the N64 expansion is the old test)
- `EVT_IS_PTR(v)` (include/script_api/macros.h:49): 3DS `(s32)(v) >= 0x100000`, N64 `(s32)(v) < EVT_LIMIT`. Signed on purpose: negative IDs (NPC_SELF -1, NPC_PARTNER) are not pointers. A first unsigned version made mac_02 fault (found by the tour, fixed before the commit).
- `MSG_IS_BUF(id)` (include/macros.h:591): 3DS `(u32)(id) >= 0x2F0000`, N64 sign. It accepts a tagged buffer (`MSG_BUF_ID`, D03) and a raw pointer.
- `IS_ADDR(v)` (include/macros.h): 3DS `(s32)(v) >= 0x100000`, N64 `(s32)(v) < 0`.
- `MAP_TATTLE_IS_FUNC(v)` (include/script_api/macros.h:58): MapSettings.tattle is a message ID or a function. 3DS: address from 0x100000 outside 0x190000..0x1901FF (map tattle IDs are MESSAGE_ID(0x19, n), n up to 0x188).
- Known limits: (a) `MSG_IS_BUF` assumes no message buffer lives below 0x2F0000. Measured in this build: code ends at 0x2D0790, `.rodata` 0x2D1000, `.data` 0x2DD000. The 187 symbols below 0x2F0000 are textures, string literals and constants, no message buffer. If a later change puts a message array there, move the data or raise the bound. (b) `MAP_TATTLE_IS_FUNC` assumes no `get_tattle` function sits at 0x190000..0x1901FF. The two linked ones are at 0x239308 (sbk_02) and 0x23acc8 (sbk_30). Check `nm` when kpa, flo, jan, sam or mgm_02 are linked (S30b). (c) `EVT_IS_PTR` assumes NPC and label IDs stay below 0x100000.

### Edits to shared decomp files (all by the macros above, file and line after the P02 commit)
- include/script_api/macros.h:49 and :55 (macros). include/macros.h:591 (MSG_IS_BUF, IS_ADDR, PTRCHECK).
- src/evt/npc_api.c:11 `resolve_npc` (F04). src/evt/player_api.c:403 (F04). src/world/common/atomic/WhaleAnim.inc.c:7 (F04). src/world/common/enemy/Bandit.inc.c:20 (F04). src/evt/evt.c:2045 `evt_find_label` (F04).
- src/msg.c:1458, 1492, 1540, 1733, 2029: `msgID >= 0` and `< 0` became `MSG_IS_BUF` (F05; D03 had done the 6 casts). src/world/area_end/common/Credits.inc.c:781 (F05).
- src/filemenu/filemenu_msg.c:814 `filemenu_draw_message`: `(s32) message < 0` was the only path that drew a string on the non-JP build, so no file select string drawn through it could show on the 3DS. Now `IS_ADDR`.
- src/world/partner/goombario.c:406 and :425: `msgID < EVT_LIMIT` (map tattle function) now `MAP_TATTLE_IS_FUNC`.
- Debug hooks (no behaviour): `PTRCHECK` in src/npc.c:1029 and :1046 (get_npc_unsafe, get_npc_safe), src/msg.c:1405 (dma_load_msg), src/evt/evt.c:1863, :1947, :1997 (set, get float, set float).

### Hits and classes (static grep of the built sources: 1890 files in the CMake lists plus their .inc.c)
- Compares with EVT_LIMIT outside evt.c: 12 sites. Broken and fixed: resolve_npc, player_api.c:403, WhaleAnim, Bandit, goombario tattle x2 (6 here, plus encounter.c:2540 fixed in F02). Safe: goombario.c:379 (enemy tattle, only message IDs in NpcData, no function), `!= EVT_IGNORE_ARG` in actor_api.c and 1A5830.c (literal compare, battle tree, not built on main). Not built on main: `src/battle/190B20.c:1762` (`formation->home.index >= EVT_LIMIT`, branch s19-full-battle, not touched): use `!EVT_IS_PTR(...)` when that branch merges.
- Compares inside evt.c: the readers and writers and the debug print (lines 1248 to 1340): safe, see the table. `evt_find_label` was broken, fixed.
- Message ID versus buffer sign tests: msg.c 5 sites, Credits 781, filemenu_msg.c 814: broken, fixed. Callers that pass a raw buffer as an ID (`shops.c:125` and `:166`, `area_mgm/mgm_01/npc.c:670`, Quizmo `SetMessageText Ref(...)` x5, Credits `get_msg_properties((s32) line->message ...)` x3): were broken, now work without an edit because `MSG_IS_BUF` accepts raw pointers (test: `get_msg_width` of a raw buffer returns 8 in `ptrtest`).
- `0x80000000` and similar constants: `y == 0x80000000` (77480.c, Reflection.inc.c), `gSpinHistoryPosY`, TWEESTER_PATH_STOP, `sprite.h` flags: integer or float sentinels, safe. N64 VRAM defines (`entity_*_VRAM`, 0x802xxxxx): overlay names, ranges are empty (R01), safe. `osVirtualToPhysical`, `OS_K0_TO_PHYSICAL`, `VIRTUAL_TO_PHYSICAL`: no-op or port shim (S11, S02), safe. `IO_READ`: only in the obfuscated shims and `src/os`, replaced by the port.
- `& 0xFFFFFF` and `& 0xFFFF` on addresses: `animator.c` 83, 346, 1055, 1069 are guarded by `animationBuffer != NULL` and `baseAddr != NULL`, and both are NULL for every 3DS animator (the 3DS branch of `entity.c` passes NULL for animated entities): safe. `entity.c` 924 to 940 (`entity_swizzle_anim_pointers`) runs only on ROM data, the 3DS skips animated entities: safe. `effects.c` osMapTLB masks: no-op shim, safe. ENTITY_ADDR: fixed in S30.
- Casts of pointers to `s32` or `u32` stored in game state: `(s32)` of a pointer into evt variables (`fx_api.c`, `npc_api.c:65`, `evt.c:852` and `:1044`): 32-bit host, round trips, safe. No cast of a pointer into an s16, u16 or u8 field found. No `%p` use. No sign extension case (32-bit host).
- Found only by the dynamic check: nothing (see below).

### Dynamic check (switch `ptrcheck`, `pm_ptrcheck` in 3ds/src/pm_warp.c)
- `PTRCHECK` logs `ptrcheck: <site> value <hex> caller <addr>` once per (site, caller) when a value from 0x100000 to 0x1FFFFFFF reaches `get_npc_safe`, `get_npc_unsafe`, `dma_load_msg` (value above 0x2F0000), `evt_get_float_variable`, or the destination argument of `evt_set_variable` or `evt_set_float_variable`. `evt_get_variable` has no check: a pointer there is the normal case (see the table). Use `addr2line` on the caller.
- Control (switch `ptrtest`, scene frames 50 and 60): the 4 sites fire once each, `resolve_npc` returns the partner `Npc*` for its own pointer, `get_msg_width` of a raw message buffer returns 8.
- Result (turbo, `-FaultStop`): tour of 266 maps with `partner=1 ptrcheck` (40 frames each, entry 0): 264 clean, 2 faulting: iwa_02 (the kmr_23 curtain callback left set by the tour, F02 note) and tst_12 (cut map, F02 note). 0 `ptrcheck` lines. Presets yard, toad, kmr02, osr, toad_east, toad_west, intro with `partner=1 ptrcheck` and a walk and jump script: rc 0, 0 hits, 0 unmapped. Warps mac_01 (GetNpcPointer, Rowf and Rhuff), nok_02, nok_04 (GetNpcPointer and `(Npc*)` casts): 0 hits, 0 unmapped. Normal chain `fast autoa freshsave ptrcheck`: loads at scans 562, 2316, 2562, 3178 (same as before), 0 hits, 0 unmapped.
- Limit: the tour runs 40 idle frames per map, so scripts that need input (talk to an NPC, open a shop, tattle, credits) did not run. The static fixes cover those. Shop text, credits text and tattle were NOT run (no preset reaches them).

### By eye (shots, `fast preset=toad_west partner=1 shots padscript=...`)
- Toad Town west gate: Mario, Goombario follows and stops beside him, a Toad (red cap) walks in from the right edge and stands (moved by its init script). File select (`fast autoa shots freshsave`): "Select file to start:", "File 1", "NEW", "First Play", the letter grid and "Start game with File 1?" all draw. NOT compared with a build from before P02 or before F02 (no before shots were taken), so the shots show the state now, not the change.

### Open
- Hardware not run. The battle tree check of `190B20.c` waits for the merge. The `MSG_IS_BUF` and `MAP_TATTLE_IS_FUNC` bounds are layout assumptions (limits above).
- Slip noted: a `git checkout 3ds/PROGRESS.log` during the slice dropped the uncommitted lines of the log. They were written again by hand (same text).

## E02 result (slice E02: save block and entity check, DONE, Azahar, port layer only, no decomp code changed)

All items belong to slice E02. Azahar only. Check used: warp with `-Turbo -FaultStop`, padscript and shots, plus one file select load (the save touches the file select). No stub, no decomp edit.

### Tool changes (class E, `3ds/src/pm_warp.c`)
- `smokeframes=<n>`: length of the smoke run, default 300 (with `-Smoke`). Padscripts for a save need about 1000 frames.
- `loadslot=<n>`: the warp first calls the real `fio_fetch_saved_file_info` and `fio_load_game(n)` (what the file select does), then sets map, entry and story, and skips `clear_player_data` and `clear_saved_variables`. The file name and `saveCount` are then the real ones. Without it a warp starts with `saveCount` 0 and an empty name: the block save then gets count 1, the same as the old file in physical slot 0, and the load picks the OLD file (tie rule `LogicalSaveInfo.count < saveCount`). That is a warp artifact, not a game fault: the real flow loads `saveCount` first.
- Helper scripts (git-ignored): `3ds/logs/e02_run.sh <name> "<switches>" "<padscript>" <frames>`, `e02_mont.py`, `e02_maps.sh`.

### Save block (entity name `Entity_SavePoint`, `src/entity/SaveBlock.c`; 28 linked maps have one)
- Used: kmr_02 at (250, 60, 75). Command: `fast preset=kmr02 loadslot=0 shots smokeframes=1000 padscript=wait80,right*88,down*24,right*30,wait20,A*24,wait100,A*3,wait80,A*3,wait80,A*3,wait80,A*3,wait80,A*3,wait80,A*3,wait120,A*3,wait120,A*3,wait120` (Mario ends at x 255 z 82, a jump with A held 24 scans hits it from below). Rc 0, 0 unmapped.
- Seen by eye (shots): the block draws as a cube with a rainbow gradient and an S face. After the hit: input disabled, the message box with the tutorial text ("Hit this block to save your game progress." and 3 more pages, first hit only), then "Do you want to save?" with the choice box (Save, Don't save, hand cursor on Save), A selects Save, then "Save completed.", A closes it, input comes back (inputDisabled 0, HUD shows), the block is normal again. The 4 tutorial pages need 4 A presses (3 scans each, 80 frames apart).
- Save file (131,072 B, 8 sectors of 16 KB, written by the real `fio_save_game` through the `osFlash` shims): the save went to the next free physical sector (1), sector 0 (the old file) stays. SaveData is 0x1380 bytes, little endian. Checked with a script: magic `Mario Story 006`, `crc1` = sum of all 0x4E0 words = `~crc2`, saveSlot 0, saveCount 2 (old 1), area 0 map 1 entry 0 (kmr_02). Old sector: map 11 (kmr_20). Both checksums valid.
- Load: `fast autoa shots` (no freshsave, no warp, full chain logos, title, file select). The file select lists "File 1 AAAAAAAA, Level 01, Play Time 00:00", the confirm window shows, `load map area 0 map 1` at scan 410, Mario stands in kmr_02 beside the block. The load accepted the new checksum and chose the newer file.
- Not checked: erase and copy, a save with a partner or items, the save while the game is at story points after the intro, the second save over an existing slot of the same logical file more than once.

### Entities (shots, `warp=<map>:<entry> partner=1`, 150 frames each, padscript where noted)
| entity | map | result |
|---|---|---|
| SavePoint (save block) | kmr_02, kmr_04 | draws, reacts (above) |
| BrickBlock | kmr_04 (-230, 60, -140) | draws, hit from below: breaks into pieces and vanishes |
| YellowBlock (? block) | kmr_03, machi | draws (? face). Not hit: it stands on the ground (needs a hammer) |
| Hammer1Block (big block with eyes) | kmr_03 | draws. Not hit (needs a hammer) |
| SimpleSpring | kmr_07 | draws (blue and yellow coil). Not stepped on: a Goomba guard scene (real NPC talk, text draws) stops Mario at x 231 and my A presses missed its timing |
| BlueWarpPipe | kmr_02 | draws as a flat pale pad at (0, 0, 355). Not entered. I believe this is the closed state, NOT compared with a real shot |
| Signpost | nok_13 | draws (board on a post) |
| HiddenPanel, HiddenYellowBlock, MulticoinBlock, HeartBlock, ScriptSpring, BlueSwitch, Chest, Padlock | machi, tst_03, kmr_03, kmr_20 | seen in `machi` and `tst_03`: ? block, brick blocks, a chest like box and a colored grid box (`machi`, near the ? block, not identified), tst_03 shows hollow pale yellow frames (not identified). Not verified, no row made because I cannot tell the intended look from the shots |
- No fault in any of these warps (`-FaultStop`, 8 maps in the first batch plus 5 more, all rc 0, 0 unmapped). The ENTITY_ADDR change of S30 works for these entities: they were created, drawn and (save block, brick block) ran their scripts.
- Not reached: Heart Block (kmr_03 route blocked by the hammer block at (45, 0, 70)), springs and pipes by hand, hidden blocks, chests. Follow up: a later slice with `partner`, hammer and items presets can reach them.


## T02 note (slice T02, documentation only, not started)

Slice T02 is a new row in 3ds/SLICES.md, low priority. It makes `3ds/run_azahar.ps1` safe for the user's Azahar config. Five fault paths were found by a read-only review (restore skipped after a throw before the try, hard kill, overlapping runs, locked config file, missing backup or regex miss). The plan is a per-run config copy, a try/finally around the whole script, a named mutex, and a loud failure on a missing key. State at the time of writing: the user's qt-config.ini is clean (frame_limit=100, use_vsync=true). No code was changed for T02.

## PC01 and F06 notes (documentation only, not started)

PC01 (high priority, next after S25) and F06 (later) are new rows in 3ds/SLICES.md. PC01: the user reports that the frame rate feels uncapped up to 59 or 60. A read-only review (UNVERIFIED) found that `pm_frame_loop` in 3ds/src/pm_main.c waits for the next vblank only, so a game step can take less than 2/60 s while audio follows the wall clock. PC01 measures steps per second first, then enforces two counted vblanks per step with a target-rate parameter. F06 is the 30/60 fps toggle: render extra frames with interpolation, not logic at 60 Hz. No code was changed for these two rows.

## Open issues V05, V06, V07, V08 (documentation only, this round, not started)

Four user reports (2026-10) are rows in 3ds/SLICES.md after V02. All diagnoses are UNVERIFIED hypotheses. V05: artifacting when menus flip (pause pages, file select panels). V06: the outside ground disappears when entering buildings. V07: item entities flicker when the camera moves. V08: transparency on the save block (Entity_SavePoint) looks odd, with a possible regression from the S25 plan path (switch `nocomb` turns it off) or the nocolor blender. The user wants none of them left dangling this round. No code was changed for these rows.

## Widescreen and 60 fps research (documentation only, read-only review, related rows F06 and F07)

Source: a read-only review of reference/paperboat, reference/soh-3ds (it is a Mario Kart 64 port, not OoT) and reference/sm64-3ds. Claims come from the files unless marked inference. Nothing was changed in code.

### Widescreen (row F07)
- paperboat supports any aspect. The game stays 320x240 logically and the interpreter scales X. The projection aspect is overridden (src/cam_main.c:61-68, src/port/patches/CameraPatches.cpp:128-134) for CAM_DEFAULT and CAM_BATTLE, the vertical FOV stays. The viewport is rescaled (cam_fit_viewport) and the scissor widened (get_cam_scissor_x). Edge helpers: OTRGetDimensionFromLeftEdge, OTRGetDimensionFromRightEdge, OTRGetRectDimension, OTRGetScissorCoordX (Engine.cpp:1415-1481).
- UI is patched per element: inventory.c:901-1048 (status bar), hud_element.c:279 and 1876, msg.c:2274, msg_draw.c:301-303, draw_box.c:407-411, encounter.c:602, screen_render_util.c:255-269, screen_overlays.c:165, pause_map.c and pause_partners.c scissors. Backgrounds are tiled (BackgroundPatches.cpp). The title screen is not tiled. Curtains use k = aspect/(4/3) (curtains.c:82-83). Per-scene patches: parade backdrop (end_00, end_01), BulbGlow, framebuffer capture (pause, mirror), Underwater, DarknessStencil, star_spirits_energy.c:334.
- Game-side culling: paperboat model.c TEST_POINT_VISIBILITY uses wsCullX = aspect*3/4 (model.c:2845). This repo src/model.c:2804 still has `xComp >= -1.0f && xComp <= 1.0f`. This is the main cull to widen. sm64-3ds widens the same way (hScreenEdge *= aspect under -DWIDESCREEN, gfx_pc.c:907-921, clip_rej 1.2x guard band). The 3DS references use 400 px logical width. Their wide mode is 800x240 with 3D off.
- This repo: SCREEN_WIDTH 268 lines in 56 files, 320 270 lines, 240 540 lines, vfov 37 lines in 16 files. Biggest: background_gfx.c 33, hud_element.c 12, msg_draw.c 10, screen_overlays.c 10, screen_render_util.c 10, cam_main.c 7, curtains.c 5, draw_box.c 5. Letterbox viewports: set_cam_viewport(CAM_DEFAULT,12,28,296,184) at state_file_select.c:71 and 110, state_logos.c:99, state_title_screen.c:206. Port: pm_gpu.c px() = 40 + x, py() = 240 - y, scissor clamped 40..360 (lines 238-252) and 320/240 (614-615), colour frame buffer copy 320x240 (pm_main.c:165, pm_gpu.c:432-440). pm_gbi.c clip_bits works in clip space, so a wider viewport needs no change there.
- Proposal (inference): (1) make the viewport X scale and offset and the scissor aspect-aware in pm_gpu.c, and scale the G_MOVEMEM viewport X by about 1.25 for the default and battle cameras. (2) Projection aspect 400/viewport height for CAM_DEFAULT and CAM_BATTLE (cam_main.c:65) and widen the model.c cull. (3) Edge helpers get_dim_left(v) = -40 + v and get_dim_right(v) = 360 - v for right-edge UI. (4) Per-scene exceptions: title, parade backdrops, framebuffer captures (pm_cfb_capture_prev 320x240 would need 400x240). Risk order: silent pop-out at the edges, scissor and clamps in the port, capture size, per-scene visual bugs. Ship opt-in first. Both expected problems (periphery culling, misplaced assets) happened in paperboat and are patched in the files above.

### 60 fps (row F06)
- paperboat keeps the logic at 30 Hz and interpolates the rendering: VI ticker thread (src/port/os/VI.cpp:38-94), gfxRetrace_Callback flips D_80073E0A (src/main.c:131-140, same as this repo), option InterpolationFPS default 30, and src/port/interpolation/FrameInterpolation.cpp (744 lines). It records a tree each logic frame and interpolates the final MtxF matrices (lerping the inputs looked wrong). Special cases: 180 degree rotations (paper effect), camera cuts (FrameInterpolation_DontInterpolateCamera at cam_main.c:359), screen-anchored texture rectangles turned into quads (ScreenAnchorPatches.cpp), lava piranha vines. Audio stays at one frame per 60 Hz retrace (nuScCreateScheduler(0,1)), independent of the render rate.
- soh-3ds (Mario Kart): the 30 Hz simulation clock is exact, an optional 60 Hz midpoint present uses frame_interpolation_3ds.cpp, interpolation is dropped under overload (adaptive presentation), audio is pumped per logic tick with a high-water mark. sm64-3ds: 30 fps logic, patch_interpolations(), frame skip if CPU + GPU > 33.3 ms (ENABLE_N3DS_FRAMESKIP), no 60 fps target.
- This repo: the 30 Hz step sits at src/main.c:131-134 (D_80073E0A ^= 1), src/main_pre.c:7, src/main_loop.c:53-62 (frameCounter += 2), 3ds/src/pm_phase.c:170 (__wrap_step_game_loop), 3ds/src/pm_main.c:225-250 (pm_frame_loop), 3ds/src/pm_audio.c:144-155 and 3ds/src/pm_os.c:91 (audio is already decoupled, one audio frame per 60 Hz retrace).
- Proposal (inference): keep the logic at 30 Hz. A 60 fps path presents a second frame in the idle slot using interpolated matrices hooked into pm_gbi.c (G_MTX, mv_sp, upload_mvp). Large work (camera, actors, animator, HUD rects). Unlikely on the O3DS because the frame budget is the same CPU and GPU work. Copy the adaptive drop-to-30 policy of soh-3ds.

## Updates V02, X01, B01 (documentation only, this round, not started)

V02 has a new user report (2026-10): a BROWN WALL still draws over the FENCES and BUSHES in the background of Mario's house scene (kmr_20). The V02 row in 3ds/SLICES.md has the new hints, hypotheses and method. Two new rows: X01 (make every screen transition work: fades, star, circle and Mario-head wipes, battle enter and exit, curtains, pause blur, previous-frame blur, flashes; table of results goes in HANDOFF.md) and B01 (battle start: black background instead of the fuzzy effect and the star wipe, seen on branch s19-full-battle; depends on X01). All diagnoses are UNVERIFIED. No code was changed for these rows.

## S25 result (slice S25: the general Fast3D combiner and other gaps, DONE on main, Azahar only)

All items belong to slice S25. Platform layer only (`3ds/src/pm_gbi.c`, new `3ds/src/pm_comb.c`, `pm_gpu.c`, `pm_tex.c`, `shaders/pm.v.pica`). No decomp file changed. No stub. The branch s19-full-battle was not touched and no battle was run, so the battle part of the row text (attack animations) is NOT checked. Check kind: warp presets with `-Turbo -Smoke -FaultStop`, the intro (`fast shots`, 200 shots), the 266-map tour (`partner=1`, 40 frames per map), the file select chain (`fast autoa shots freshsave`), and `fast autoa` at the end (loads at scans 410, 2164, 2410, 3026: kmr_20 and mac_00 reached, 0 unmapped, no assert).

### What was wrong (measured)
- `pm_gbi.c` knew 7 combine modes (shade, prim, texture x shade, constant colour with texture alpha, two lerp forms). Every other combiner fell back to one of them. A numeric audit (the RDP equation on 24 random inputs against what the mode draws) showed that most draws of the intro used a combiner the old modes did not give: 4.1 M of 5.1 M draws in one intro run. Typical forms: `T0*SHADE` then `COMBINED*PRIM+ENV` (every sprite: Mario, NPCs, effects), `(ENV-PRIM)*T0+PRIM`, `(1-PRIM)*T0+PRIM`, `T1*T0` with alpha `T1A*T0A` (page dissolve, sparkles, cards), `(1-T0)*PRIM+T0` with alpha from TEXEL1.
- The olive square of intro shot 76 (also 77): a 32x32 I4 quad with render mode `50553378`, blender `G_BL_CLR_MEM` for both colour inputs. That blender writes the old frame buffer colour back, so the draw changes depth only. The port drew it as a coloured quad (shade colour olive). Fix: `pm_gpu_set_nocolor()`, the draw writes depth only, or nothing when depth is off.
- The black sparkle streaks (shots 71, 74, 75, 78): `(1-T0)*PRIM+T0` with alpha `(T1A-ENVA)*T1A+T0A`, reads two textures. TEXEL1 was drawn as TEXEL0. Now TEXEL1 is unit 1 of the PICA. The streaks are pink and white, the magic of the Star Rod fight.
- Sprites were dark (Mario, Luigi, Goombario, Toad in the yard, toad and kmr02 shots, `nocomb` A/B): the second cycle `COMBINED*PRIM+ENV` was ignored. With the plan path they have their colours.

### What was built
- `pm_comb.c`: lowers one G_SETCOMBINE (1 or 2 cycles, colour and alpha) to up to 6 PICA texenv stages. Forms: `D`, `A*C(+D)`, `(1-B)*C(+D)`, lerp (INTERPOLATE, B == D), `D - B*C`, `(A-B)*C` (SUBTRACT then MODULATE), general `(A-B)*C+D` (SUBTRACT then MULTIPLY_ADD, the PICA clamps A-B at 0). Constants: PRIM, ENV, PRIM_LOD_FRAC and the zero register go to the stage constant, the buffer colour (`GPU_PREVIOUS_BUFFER`, never updated) and the vertex colour (when the shade is not read). A cycle 2 `COMBINED` input works when it is read in the first sub-stage. `pm_comb_eval` runs the same stages on the CPU, so each plan is checked against the RDP equation before use.
- `pm_gbi.c` `decide()`: per combiner (cached by the words and the flags the legacy mode depends on), the legacy mode is kept when it is within 0.012 of the RDP result, else the plan is used when it is closer. Switch `nocomb` turns the plan path off (A/B). Mode 4 (constant colour with texture alpha) is exact by construction and always kept.
- Two textures: tile 1 is loaded into a second texture slot (`pm_tex_set_aux`), bound at unit 1. The vertex shader writes `texcoord1 = uv * uvx.xy + uvx.zw` (uniform `uvx`, set per draw from the two tile windows). `C3D_TexBind(1, NULL)` crashes, so unit 1 stays bound to the last TEXEL1 texture and no stage reads it.
- Also fixed on the way: the cycle type change (`G_SETOTHERMODE_H`) and the shaded-sprite flag now re-select the combine mode. A scrolled tile window with a negative start (12 bit two's complement, `SETTILESIZE`) left tile 1 unloaded. It is now read as size with start 0.
- Log lines (non-blocking log): `comb new f<frame> INEXACT|PLAN-OK ...` once per combiner and render mode with the equation, `comb count ...` at 1, 10, 100 ... draws with the frame range, `comb total` every 600 frames (draws, plan draws, inexact draws, counts of lit, fog, blend colour, texgen draws), `gbi skip <op> count` and `gbi ignore movemem|moveword index` at 1, 100, 10000 uses. Helper scripts (git-ignored): `3ds/logs/s25_agg.py`, `s25_tour.sh`, `s25_preset.sh`, `s25_sheet.py`.

### Counts in the 266-map tour (turbo, 40 frames per map, `partner=1`)
- 13.3 M draws in the last totals of the 9 chunks (an undercount, the total line is every 600 frames), 2.2 M of them through the plan path. 3 maps stopped with an unmapped access: kmr_23 (the curtain callback left set by the tour, known from F02), tst_12 (cut map, known from F02), and nok_01 with `au_syn_begin_audio_frame` (engine.c:488, audio; in the first tour the map was nok_03, the tour lags by one map). `warp=nok_01:0` and `warp=nok_03:0` alone ran clean (smoke 300 frames, 0 unmapped). So it is a tour transient. Not diagnosed. Not graphics.
- Combiners that are still not exact (draw counts are lower bounds, logged at powers of 10, summed over chunks):
  - `(T0-0)*SHADE+0 / (T0A-0)*SHADEA`, words `fc121824 ff33ffff` and `fc121e04 fffff3f8`, with `gSPTexture` off: about 2.7 K draws. The RDP reads texel (0,0). The port draws shade only (texture assumed white). Not changed.
  - `(T1-K4)*PRIMA+T0` (`fc251a47 77fcaa3e`) 2.3 K draws, plan error 0.27: K4 (a convert constant) reads 0.
  - `(T0-T1)*T0+1 / (1-T0A)*T..` (`fc10e5e0 230b1d52`) 2.2 K draws, error 0.04.
  - `(1-T0)*PRIM+T0` with the `T1A` alpha form (`fc6198c1 1114b83f`) 1.1 K draws, error 0.42 (generic SUBTRACT clamp).
  - `(0-ENV)*T0+1` (`fcf09a61 501374ff`) 1.1 K draws, error 0.33 (needs a negative A-B).
  - `(T0-ENV)*ENVA+PRIM` (`fc1616ce 5f11fe3f`) 0.2 K draws, error 0.31.
  - One form `(T1-CMB)*CMB+ENV` (intro, about 1 K draws) reads COMBINED in a late sub-stage: no plan, legacy mode. NOISE and KEY inputs read 0.
- Lights and fog (counted per drawn triangle): lit 7.9 K of 13.3 M (0.06 percent). The fixed light is kept. A correct light needs the normal matrix and the light directions of the RSP. It is not cheap and no visible need was seen. Fog: `G_FOG` plus a blender with the fog colour in 1.3 M draws, but only in two tour chunks (maps in the range of chunks 40 and 176), none in the intro, yard, toad town, file select. Not implemented: the RDP blends with the shade alpha as the fog factor, the PICA blends with the texenv alpha, so it needs a separate alpha path. `G_SETBLENDCOLOR` was used by 800 draws in the first 1200 frames of `fast autoa`, not handled. Texgen: 0 draws. Skipped opcodes seen: `f3` G_LOADBLOCK (textures come from the tile size, fine), `04` and `02` (BRANCH_Z, MODIFYVTX), `f8` and `f9` (fog and blend colour), `ee`, `ec`, `eb`, `ea` (not decoded).

### Seen by eye and not seen
- Seen (intro, `fast shots`, before and after sheets `3ds/logs/s25_before_70_82.png` and `s25_after3_70_82.png`): the olive square is gone (shots 76 and 77 show Bowser), the black streaks are gone (pink and white energy), the star spirit cards (shot 98) show their pictures, scene dissolves show as cross-fades. Background tints in shots 70 to 77 are darker or red (they were light blue before). By the combiner this is the colour fade of the scene, consistent with Bowser's rage. NOT compared with a real console.
- Seen (presets yard, toad, kmr02, A/B with `nocomb`): sprites are no longer dark. Sheet `3ds/logs/s25_cmp_presets.png`. Backgrounds, houses, windows, the pause menu (Stats and Items pages with the checker window), file select with name entry, the title, and the soft shadow under Mario look the same as before.
- Not seen: any battle (branch not built), hardware speed (the plan path uses up to 6 stages and one more texture unit, Azahar does not show the cost on the O3DS), fog and light maps by eye, the 3 faulting maps as pictures.

## PC01 result (slice PC01: exact 30 fps pacing, DONE, Azahar only, port layer only)

All items belong to slice PC01. Files: `3ds/src/pm_main.c` (`pm_frame_loop`), `3ds/src/pm_phase.c` (`pm_step_calls` counter in `__wrap_step_game_loop`). No game code changed. Numbers are from Azahar. Logs (git-ignored): `3ds/logs/pc01_*.txt`, summary script `3ds/logs/pc01_sum.sh`.

### What the measurement showed (before the change, normal speed)
- The user's report (uncapped up to 59 or 60) was NOT reproduced. The game never stepped faster than 30 per second. `C3D_FrameBegin(SYNCDRAW)` calls `C3D_FrameSync`, which waits for the next vblank. Together with the display transfer wait, a drawn iteration takes at least 2 vblanks even with the pad wait off (`fast`: 29.8 steps/s).
- The old idle wait was wrong in the other direction. After the drawn iteration, `gspWaitForVBlank` ran when the frame took under 33 ms. Then FrameBegin waited for one more vblank. A step took 2 or 3 vblanks: yard 23.5 steps/s (min 20, max 24), toad 23.9, audio ticks per step 2.5 to 2.6. Turbo run: 23.8 (same, the Azahar clock is emulated). `fast`: 29.8, audio ticks per step 2.0.
- Vblank counter: `C3D_FrameCounter(0)` (citro3d) counts 59.83 to 59.86 vblanks per second in Azahar. Audio ticks (wall clock, 1/60 s) run at 60.0 per second.

### Change
- Switch `pacestat`: one `pace:` line per second (wall ms, steps, drawn frames, vblanks, audio ticks, steps per second, histogram of vblanks between drawn iterations).
- The idle iteration waits (`gspWaitForVBlank` in a loop) until `C3D_FrameCounter(0)` is `PM_VBLANKS_PER_STEP - 1` past the vblank count at which the last drawn iteration got past FrameBegin. FrameBegin then waits for the last vblank. A slow frame is already past the target: no wait, no extra step. Guard: at most `PM_VBLANKS_PER_STEP + 1` waits. `fast` keeps no wait (uncapped debug mode). `PM_VBLANKS_PER_STEP` is 2. A 60 fps option (F06) must also stop FrameBegin from waiting for the vblank. That is not done and not in this slice.

### Result (normal speed, no `fast`, no turbo)
| run | steps/s | vblanks/s | audio ticks per step | intervals (vblanks between steps) |
|---|---|---|---|---|
| toad, 49 s | 29.91 (min 30, max 30) | 59.83 | 2.01 | 2: 1470, 3: 0 |
| yard, 49 s | 29.82 (min 29, max 30) | 59.85 | 2.01 | 2: 1458, 3: 9 |
| yard with `phase`, 49 s | 29.66 | 59.83 | 2.02 | 2: 1437, 3: 25 |
| yard `fast` | 29.78 | 59.86 | 2.01 | 2: 860, 3: 8 |
- 29.91 is the limit: 59.83 vblanks per second divided by 2. A value of exactly 30.00 per wall second needs a vblank rate of 60.0. The step is 2 vblanks in 99.4 to 100 percent of the steps. The 3 vblank steps are frames with cpu work near 34 ms in the yard (phase report: wall 33.4 ms avg, 4 of 60 frames over 34 ms in one window, max 50 ms once).
- Idle slack: the phase report shows cpu 25 to 27 ms of a 33.4 ms frame (yard), so about 7 ms per step is wait.
- Padscript walk (`preset=toad padscript=wait80,right*40,A*24,wait60,left*40,A*24,wait30 smoke`): the `warp: scene f.. pos` lines of the normal run and the `fast` run are identical frame by frame (153 lines compared, 0 diff). A step does not depend on the wait.
- Chains: `fast autoa freshsave` map loads at scans 562, 2316, 2562, 3178 (kmr_20, osr_00, kmr_20, mac_00), same as before, 0 unmapped. Normal `autoa freshsave` (no `fast`, logos, title, file select run): kmr_20, osr_00, kmr_20, mac_00 reached, 0 unmapped. `-Turbo -Smoke preset=toad`: smoke done, exit 0.
- Not changed: audio. It still follows the wall clock (1/60 s, `pm_audio_retraces`). It gives 2.01 ticks per step. On a console with a vblank rate of 59.83 Hz the wall clock gives 60.0 ticks per second, so the audio drift against the game stays near 0.3 percent. A tick count tied to the vblank counter is a possible later change.

### What Azahar timing cannot show
- The real vblank behaviour of the O3DS: the real screen refresh (about 59.83 Hz), the real GPU time per frame, and whether frames on the O3DS (slower ARM11) take more than 2 vblanks. If the cpu work is above about 33 ms the step takes 3 vblanks (20 fps), as in the PT01 numbers. The `pace:` line (`pacestat`) on the O3DS shows this at once: steps per second and the interval histogram. Run slice S28 with `pacestat phase`.
- Judder from 2 and 3 vblank frames, and the feel of the game speed by ear and eye. Nothing was heard or seen on a console.
- Real audio drift over minutes.

## F08 and V09 notes (documentation only, not started)

Two new rows in 3ds/SLICES.md. F08 (fog): S25 found that fog is not implemented. About 1.3M draws in 2 tour chunks use it (`G_SETFOGCOLOR`, the fog blender bits in othermode L, the RSP fog factor per vertex). The task is to decide with evidence which maps and scenes depend on it, then implement it in the platform layer (PICA fog table or a shader path). V09 (intro tints): S25 changed the background tints in the intro (shots 70 to 77) from light blue to dark or red. Nobody compared them with a console. The task is to check the hos_05 intro background and the screen tint and overlay colours in the intro scripts against the decomp and against paperboat, with an A/B using `nocomb`, and to check nok_01 (`au_syn_begin_audio_frame` unmapped access, seen only in the tour). No code was changed for these two rows.

## M03 result (slice M03: the battle merge, Q1 (a), DONE, Azahar only)

All items belong to slice M03. Azahar only. Nothing ran on hardware. Check used: warp checks, a battle sweep, one full normal chain and one full 266-map tour, because the merge touches the build, the title demo and the map change path.
- Merge (slice M03): branch `s19-full-battle` merged into main with `git merge` (history of main not rewritten). Backup tag `s19-pre-merge` points at the old branch tip 7fd566684. Conflicts: `pm_title.c` (deleted on main, kept deleted), `CMakeLists.txt` (both `--wrap` and `--defsym heap_battleHead` lines kept), `ld_addrs.h` (map, data and battle includes kept), `game_stubs.c` and `pm_later.c` (stand-in lines deleted on both sides), four notes files (main side kept, main already had the S19 notes). The tree links on the first build: 689 build steps, 0 new unresolved or duplicate symbol.
- Stand-ins deleted (S20 audit rows): A1 `load_battle`, `set_battle_formation`, `set_battle_stage`. A2 `reset_battle_status`. A3 the 9 `state_*_battle`, `state_*_end_battle`, `state_*_demo` stubs. A13 `gBattleStatus` (game_stubs.c). A14 `heap_battleHead` array (now `--defsym heap_battleHead=gFrameBuf2`, the real address from `ver/us/undefined_syms.txt`, not a stand-in). B3, B4, B5 the shadow headers in `stub_include` (sprite, battle/actor_types). `BattleEntityHeapBottom` was removed in S12. A grep over 3ds/src finds none of these names now.
- Debug switches moved to `3ds/src/pm_warp.c` (class E, no game logic, run from `pm_warp_tick`, so they need `warp`, `preset` or `story`): `battle` (default BTL_KMR_1_FORMATION_00 on BTL_KMR_1_STAGE_01), `battle=<formation>:<stage>` (a name from `src/battle/battle_names.h` and `stage_names.h`, or a number, table generated by `3ds/tools/gen_battle_names.py` into `3ds/stub_include/battle_name_table.h`), `battlef=<n>` (scene frame, default 100), `demobattle` (real demo state at scene 1). Example: `fast preset=yard partner=1 battle=BTL_HOS_FORMATION_00:BTL_HOS_STAGE_00 battlef=20 smokeframes=300`.
- The hook sets `gEncounterState = ENCOUNTER_STATE_NONE` before `load_battle`. Cause of the old fault (investigated, not guessed): during a battle `state_step_battle` calls `update_encounters`. The state stayed NEUTRAL, so `update_encounters_neutral` read the enemy list of the world map (`assert i < MAX_NPCS`, then unmapped reads, 872 at one PC). The real encounter code sets `ENCOUNTER_STATE_POST_BATTLE` in `update_encounters_pre_battle` (encounter.c:1445), which needs a real world enemy. So a debug battle skips the real encounter start and the real post battle fade. After the win the game mode goes back to world (modes 8, 9, 4) but the screen stays black (no fade in). That is a limit of the hook. The real start and end is row S19d.
- P02 style fix: `src/battle/190B20.c:1762` `formation->home.index >= EVT_LIMIT` is now `!EVT_IS_PTR(formation->home.index)`. Scan of `src/battle` (all `.c`) for the same class: other `EVT_LIMIT` uses 0. `EVT_IGNORE_ARG` compares (1A5830.c, actor_api.c, many `.inc.c` call sites) compare to a literal, safe. `(s32) animID >= 0` and `(s32) anim >= 0` (190B20.c:1080, :1122): AnimID is a sprite animation ID or -1, not a pointer, safe. `msgID` uses all go through `draw_msg`, `get_msg_width` and `msg_get_printer_for_msg` (already `MSG_IS_BUF`). `0x8xxxxxxx` constants: only the 4 `VINE_n_BASE` in `kzn2/actor/lava_piranha.c` (already a TARGET_3DS variable branch in the file). No cast of a pointer to s16 or u16 found. `osVirtualToPhysical` in 16C8E0.c: port shim, safe.
- D02 layout scan (`check_data_layout.py nm.txt --scan` on the new ELF): both known groups still `ok` (actions.c, script_list.c). The scan pattern list over the battle files, `state_battle.c`, `state_demo.c`, `level_up.c`, `starpoint.c`, `battle_ui_gfx.c`, `PowBlock.c`: 0 hits.
- R01 and M01/M02 ROM audit for the battle: `rom_static_check.py`: 185 named ROM ranges, 0 overlap a dropped range (77 before the merge). `rom_symbol_audit.py`: the 159 battle names in `battle_ld_addrs.h` are all empty ranges (START == END == 1, code overlays linked in). 4 show as DATA? (`fire_flower`, `fright_jar`, `snowman_doll`, `stop_watch`, splat `.data` segments): nothing in `src` or `include` uses their `_ROM_START`, so no reader. Dynamic: `romtrace` with the battle sweep below: 0 `ROM READ IN DROPPED RANGE` lines in every run. The M01/M02 remap keeps everything the battle reads (stage assets come from the asset table, sprites from the kept sprite ranges).
- Battle sweep (formation 00 and stage 00 of each of the 40 battle areas with `-Turbo -Smoke -FaultStop romtrace`, 260 frames, `preset=yard battlef=20`, result file 3ds/logs/m03_sweep.csv): 34 clean. Not clean, with causes: (1) KMR_2, KMR_3, ISK_1, ISK_2 stage 00 are the real decomp stages `kmr_02` and `isk_00`. They name a shape asset that does not exist (`@bug` comments in `battle/common/stage/area_kmr/kmr_02.inc.c` and `area_isk/isk_00.inc.c`), so `load_asset_by_name` runs past the asset table. N64 behaves the same. With stage 01 all four run clean (KMR_3 formation 00 needs a partner, see 2). (2) HOS formation 00 and KMR_3 formation 00 are tutorials where Goombario speaks. Without a partner `ActorSpeak` reads a NULL actor. With `partner=1` both run clean. (3) KZN2: `update_animated_models` unmapped reads. The `world_model_anim_kzn_*` ranges are empty (Lava Piranha animation data), row S19b. 0 asserts in all 40.
- Goomba battle played through (`preset=yard battle` and a padscript that taps A, shots): command menu with Jump, attack, damage number, the Goomba defeated, "STAR POINTS" screen with 2, then game mode 9 and 4. Seen by eye: Mario and the Goomba draw on the kmr_03 stage with HP, FP, star point HUD, and the jump animation draws. The black and olive polygons of the attack animation were NOT seen in the 24 shots of this run (S25 may have fixed them). Only the Goomba jump was played, so partner moves, items, star power and other enemies are not checked. After the win the screen is black (see the hook limit above, not B01 or X01). The battle start effect (B01) and transitions (X01) were not touched.
- Memory (measured on Azahar, estimates for the O3DS are Azahar based and NOT hardware): image (`arm-none-eabi-size`) text 2,539,964 + data 7,366,960 + bss 7,284,920 = 17,191,844 B (before the merge 13,398,116 B, so +3.79 MB). 3dsx 10,542,300 B (before 6,437,736, +4.1 MB). Boot log `mem commit limit 98304KB used-by-image 16808KB`, `mem map heap used 27371KB`, later ticks 27383 KB. Estimated O3DS spare (M02 model, 64 MB limit) = 65,536 - 20,480 linear - 16,808 image - 27,383 heap = about 865 KB. That is above the 0.5 MB limit of the task, so NO map was turned back to an id-only row and no battle code was dropped. It is far below the old 4,500 KB ceiling. Linking the remaining 154 maps (S30b, +2.0 MB measured in S30) would give about -1.1 MB: it does not fit. Question Q7 is updated.
- Checks: `fast autoa freshsave` chain loads at scans 562, 2316, 2562, 3178 (same as before the merge), 0 unmapped, no assert (the run went on to mac_01 and nok). Presets yard, toad (with pause START and the pause menu drawn by eye, D03), kmr02 (save block map, E02): rc 0, 0 unmapped. Yard jump peak y 37 above the ground (same as before). Title idle with `fast` (no autoa): the real title timer enters the demo. Scenes nok_12, the dig battle, isk_04, tik_03, trd_09 run. Then `load refused: area 17 map 4 is not linked` (jan_04) and the screen stays black, 0 unmapped, no crash. Demo maps that are not linked: jan_04 (area jan) and kzn_02 (area kzn). These are the next row (Q4 b). The other demo maps (nok_12, isk_04, tik_03, trd_09, mac_00, mac_03, nok_02, kkj_11, arn_08, kkj_26) are linked.
- 266-map tour (turbo, 40 frames per map, `partner=1 ptrcheck`, `-FaultStop`, chunks of 40): 263 ok, 3 stopped with an unmapped access. They are the same 3 as the S25 tour before the merge: kmr_23 curtain callback (shown as iwa_00, the tour lags one map), nok_01 `au_syn_begin_audio_frame` (engine.c:488), tst_12 (cut map). 0 `ptrcheck` lines. No new failure from the battle tree.
- Not checked: hardware, any battle after the first one by eye (only the sweep start is clean), the real encounter start (S19d), the battle start effect (B01), transitions (X01).

## S19b result (slice S19b and S19b+: Lava Piranha data, kmr_22, kzn_19, demo maps jan_04 and kzn_02, Azahar only)

All items belong to slices S19b and S19b+. Azahar only. O3DS numbers are ESTIMATED from Azahar inputs (64 MB limit assumed).
- How the data is real (S19b): the 37 `src/world/model_anim/kzn/*.c` files are NOT linked as ARM. The game copies each script with `dma_copy(ROM_START, ROM_END, VINE_n_BASE)`, so it needs ROM offsets. `3ds/tools/gen_data_rom.py` now writes the real ROM range of `world_model_anim_kzn_00..24` (`3ds/stub_include/data_rom_addrs.h`), `gen_battle_ld.py` no longer emits empty ranges for them. `gen_rom_remap.py` keeps the 173 KB (0x7B5960..0x7E0E80) in the resident ROM. The scripts are `s16[]` data. `nuPiReadRom` swaps each s16 of a read inside that range (`pm_rom_swap16` in `rom_remap.h`, port layer, no game logic).
- Root cause found by the first battle run (fault in `nuPiReadRom`): on US the vine buffers are the N64 addresses 0x80234000 and others (`#else` branch, SHIFT builds use real symbols). Fix: `#if defined(SHIFT) || defined(TARGET_3DS)` in `src/battle/area/kzn2/actor/lava_piranha.c` and `src/world/common/lava_piranha/part1.inc.c` (the real BSS arrays `Vine0Base..` and `D_80200000..`).
- Maps: `maps_skip.txt` gets `link:<map>` lines (new feature of `gen_map_sources.py`, a single map of a skipped area). kmr_22 is real (its skip line is gone). kzn_19, kzn_02 and jan_04 use `link:`. 271 of 421 maps are linked.
- Memory (Azahar, measured; O3DS estimated): ELF 17,191,868 to 17,316,388 B (+124,520 B), 3dsx 10,542,332 to 10,655,968 B, `used-by-image` 16,808 to 16,928 KB, heap used 27,383 to 27,554 KB (the ROM part is +173 KB). Estimated O3DS spare 65,536 - 20,480 - 16,928 - 27,554 = 574 KB (about 865 KB before). The limit is 500 KB. No step was dropped. No other map was linked (Q7 text updated).
- Seen by eye: kzn_19 (`warp=kzn_19:0`, 14 shots, cave room, Mario, HUD). Lava Piranha battle (`preset=yard partner=1 battle=BTL_KZN2_FORMATION_00:BTL_KZN2_STAGE_06 battlef=20`, `padscript` with A taps, 1500 frames, 0 unmapped, 0 assert): the Goombario tutorial, the piranha rises from the lava with animated vines and head, the command menu shows. Not played to the end (attacks, bud vines and death not checked). kmr_22 loads, 300 frames, 0 unmapped, but the card picture is garbled (new row V09). The vine animation path of the WORLD (`LoadAnimationFromTable` in kzn_19 and kmr_22 with the boss script) was not triggered: the warp states do not start the boss scene.
- Demo (S19b+): `fast` title idle, turbo, `-FaultStop`, 0 unmapped. The first demo loop plays all scenes by eye (shots): nok_12, dig battle, isk_04, tik_03, trd_09, jan_04 (Sushie, jungle water), battle, mac_00, kzn_02 (lava, Kooper), battle, mac_03, nok_02, battle, kkj_11 (Peach), arn_08, battle, nok_12, kkj_26, end, then game mode 2 (title) and 16 (intro, hos_05). No black screen, no refused load.
- NOT working: the loop does not repeat. The second hos_05 intro (after the demo) stays on a static starry picture for 360,000 frames (`scripts` dump: evt 832 stays at `hos_05_CamPanAcrossRoom`, intro.c:838). Cause (read in the code): `N(PanAcrossRoomTime)` is a writable static of the map. It reaches 170 in the first intro and the script ends on `== 170`. The map is linked in place, so the second load does not restore it. On the N64 the overlay DMA restores the map data. This affects any map with changing statics on re-entry. New row MS01 (HIGH, first named S33 by mistake, S33 is the gitignore slice). So S19b+ is `[~]`.
- New switch `shotskip=<n>` (class E, `pm_gpu.c`): no shot in the first n frames, for a later scene.
- Checks: full normal chain `fast autoa freshsave` (the map table changed): loads at scans 562, 2316, 2562, 3178 as before, 0 unmapped. Presets were not rerun (kzn_19, kmr_22, battle runs above).
- Not checked: hardware, the world vine animation, a full Lava Piranha fight, a second demo loop.

## MS01 result (slice MS01: map statics restored at map entry, Azahar only)

All items belong to slice MS01. Azahar only. Check used: demo loop, two 262-map tours, the normal chain, the pause preset, because the slice touches the map load path.
- Measured (link map, `3ds/tools/map_data_sizes.py`): linked maps own 3,008,102 B of .data (270 groups) plus 51,488 B shared in area_dgb, and 201,544 B of .bss (65 groups). Most map data is EvtScript arrays that never change.
- Important: an initialised zero (`s32 x = 0;`) is .bss with GCC but .data on the N64. `N(PanAcrossRoomTime)` is such a variable. So the N64 overlay copy reset it, and here only a .bss clear can do the same.
- Changers (debug switch `dataprobe`, `3ds/src/pm_mapdata.c`, input `ms_ranges.txt` made from the link map of the same build; runs: the 262-map tour twice with partner=1, one demo loop, the normal chain): 22 maps change something. .data: 21 symbols, 1,536 B, 353 B changed (hos_05 15 camera values, hos_01, kmr_21, mac_06, tst_13 shockwave vtx 1,456 B). .bss: 73 symbols, 33,579 B, 16,733 B changed (osr_00 and jan_04 `scenes_bss` 16 KB buffers, hos_05 intro state, DemoInitState flags in 7 demo maps, mac_05, mac_06, hos_02, kmr_00). The tour visits each map 40 frames only, so scripts that run later are not measured, only the demo and chain add them.
- Design: (a) a full pristine copy would cost 3.0 MB: no. (c) reading the 3dsx: not needed. (d) paperboat runs the real overlay loader on a PC, no equivalent. Chosen (b) variant: at the dma_copy step of load_map_by_IDs (a linked map gets the tag `PM_MAP_DMA_TAG` as dmaStart and dmaEnd in `map_rom_ranges.h`, so world.c calls dma_copy, and `--wrap=dma_copy` in `pm_map.c` calls `pm_mapdata_enter(gMapConfig->id)`, zero bytes are copied, as before): (1) the .data symbols listed in `3ds/map_restore.txt` are copied back from a copy taken at the first entry (about 1.5 KB of heap), (2) all .bss of the map is set to zero (no copy needed).
- Build: `3ds/tools/gen_map_restore.py` writes `3ds/ld/map_restore.ld` (one block per map, included by `3ds/ld/3dsx.ld.in`, a copy of the devkitPro 3dsx.ld with one INCLUDE line, found by gcc through `-B` because INSERT scripts do not work with the specs file) and `3ds/src/map_restore_table.c`. `gen_map_restore.py learn <map> <probe logs>` adds new changers to `map_restore.txt`. Build now also writes and copies `build3ds/pm_3ds.map`.
- Cost (Azahar): `used-by-image` 16,984 KB (S19b recorded 16,928 KB, +56 KB, of which the probe, table and restore code are about 4 KB, the rest is block alignment and other changes since S19b not isolated), heap at first map the same, 3dsx 10,908,772 B (was 10,655,968, +253 KB file, the .bss moved into the file). Estimated O3DS spare about 574 - 56 = 518 KB, still above 500 KB. Not hardware.
- Verified: the title demo loop with `fast`, turbo (NOT valid for timing): 19 full loops in 1000 s, hos_05 intro runs every time (`restored data 60 bss 848`), `dprobe: after` check 281 times with 0 bytes different from the first entry, 0 unmapped. Two tours of 262 maps (523 entries, partner=1): done, 0 unmapped, 0 diffs after restore. Normal chain `fast autoa freshsave`: loads at scans 562, 2316, 2562, 3178 as before, 0 unmapped. `preset=toad` with pause padscript and `preset=kmr02`: smoke done, rc 0.
- Not checked: normal speed (not run, turbo only), by-eye shots of the loop (the check was the log: 19 hos_05 loads, no hang), statics that change only in scenes the runs did not reach (add with `dataprobe` then `gen_map_restore.py learn`), hardware. Bss of a map that code outside the map sets before the load would be cleared: none seen.

## S19d and S29 result (slices S19d, S29 and the title demo check, Azahar only)

All items belong to slices S19d and S29. Azahar only. Check used: warp runs (-Turbo -Smoke -FaultStop, fixed length) and the full normal chain once at the end (the slices touch encounters, map change and the intro chain).
- S19d, real encounter (no debug hook). Map kmr_09 (`warp=kmr_09:0` or `:1 story=ch0_goombario partner=1`), Mario walks into a Goomba or Paragoomba with padscript. 0 unmapped, 0 assert in every run. By eye (shots): overworld, the enemy notices Mario (emote), the battle start wipe, the battle stage with HUD, the Goombario tutorial text, command menu, jump, damage, STAR POINTS screen, LEVEL UP screen (`sp=99`: pick FP, FP went 5 to 10), fade back to the map (game modes 8, 9, 4), the defeated enemy is gone, the pause menu opens after the battle (modes 10, 11, stats page shows the new values) and Mario walks out of the map. The screen no longer stays black after a win (M03 limit was the debug hook only). FLEE (Strategies, Run Away): Mario runs, fade, back in the map. LOSE (`hp=1`, Do Nothing): HP 0, Mario falls, GAME OVER with spotlight, title, file select, the file starts (modes 6, 2, 14, load kmr_20). The first fix was not needed in game code. No game code was changed.
- Not checked by eye: first strike by Mario (jumping on the enemy before contact), partner moves, items, star power, battle start effect detail (B01), transitions (X01), several enemy types.
- New faults found (rows V10, V11 in 3ds/SLICES.md): V10 the Strategies submenu and the level-up description box draw as a white and brown checkerboard, text not readable. V11 kmr_05 from entry 0: Mario stops at x about 75 to 88, so Goomba_01 there is not reachable (maybe real geometry, unchecked).
- Tool changes (class E, port layer, no game logic): `padscript` can take 450 steps (was 160) and a file of 3 KB (was 1 KB); debug switches `sp=<n>` (star points) and `hp=<n>` (Mario HP) in `3ds/src/pm_warp.c`, for warp runs. Size: 3dsx 10,908,960 to 10,909,052 B (+92 B), bss about +3 KB. Estimated O3DS spare about 515 KB (518 before), still above 500 KB. Not hardware.
- Test notes: enemy battles take about 1000 frames with A taps. A padscript file (`padscript=@name.txt` in the SD app folder) is needed for long scripts, the switch string holds 256 bytes. Timing of menus differs by one frame between runs, so use repeated stick taps. Helper scripts (git-ignored): 3ds/logs/s19d_run.sh, s19d_demo.sh, sheet.py.
- S29, issue 1 (kkj_00): NOT a port bug. The decomp decides it. osr_00 entry 3 (intro letter, from kmr_20) goes to kmr_20 entry 1 (`scenes.c:61`). kkj_00 entry 5 comes only from osr_00 entry 4 (`scenes.c:86`), and osr_00 entry 4 comes from mac_01 `EVS_Scene_IntroWalking` (mac_01/intro.c:58), which runs after mac_00. So the real chain is kmr_20, osr_00, kmr_20, mac_00, mac_01, osr_00, kkj_00. Full chain, turbo, log: load map at scans 562 (kmr_20), 2316 (osr_00), 2562 (kmr_20), 3178 (mac_00), 3818 (mac_01), 4222 (osr_00), 4472 (kkj_00, area 4 map 0). 0 unmapped. The old note only watched the first 4 loads. After kkj_00 entry 5 the intro scene gives control to Mario (Luigi talks), so a run with autoa idles there by design. Issue 1 is closed.
- Title demo: with `fast` the title timer starts the demo. A at scene 1 (scan 8500, world scene nok_12): fade, game mode 17 then 2 (title) then 16 (intro) with no freeze. A at scene 2 (scan 8700, battle scene): modes 9, 17, 2, 16, no freeze. Pressing A, B, Z or START during a demo sets the demo state to change-map (input.c:67). All 17 scenes were seen to play in the MS01 loop, the later scenes were not tested with A. By eye no fault in the title and intro frames after the exit. The earlier per-scene faults list is the S19b list (no new one seen).
- Full normal chain `fast autoa freshsave` after the last build: loads at scans 562, 2316, 2562, 3178, 3818, 4222, 4472, 0 unmapped, no assert.

## X01 result (slices X01 and B01: screen transitions, DONE, Azahar only, port layer plus one TARGET_3DS macro)

All items belong to slices X01 and B01. Azahar only. Check used: warp runs (-Turbo -Smoke -FaultStop, fixed length) and the full normal chain once at the end (the slice touches the map change path and the title). Shots are one per game frame, labelled with the game frame number (`f<n>` in the sheets).

### Causes found and fixed (root cause, platform layer, no game-specific logic)
1. Battle start swirl (B01). `OVERLAY_START_BATTLE` builds the swirl in the depth buffer: a stencil pass draws the rotating star shape with depth source `G_ZS_PRIM` and `G_SETPRIMDEPTH` 0 (colour unchanged), then a full screen black fill with prim depth 20 and Z_CMP fills everything that is not nearer. The port ignored `G_SETPRIMDEPTH` and the fill ignored depth (`rect_2d` skips the depth test). Result before: the screen went black where nothing was drawn (the sky and background), the tree and ground stayed, no star shape. That is the user report. Fix: `pm_gbi.c` keeps `prim_z` (15 bit, 0 near, 0x7fff far). With `G_ZS_PRIM` every triangle gets that depth (`z = (2 d - 1) w`). A fill rect with `G_ZS_PRIM`, Z_CMP and G_ZBUFFER goes to the new `pm_gpu_fill_rect_z` (`pm_gpu.c`, depth test on, depth d, same mapping as 3D). Switch `noprimz` turns it off for A/B checks.
2. Mario, star and circle wipes. Two causes. (a) The star and circle stencils load a 32 texel wide image into a 64 texel tile (`G_TX_MIRROR | G_TX_CLAMP`, mask 5). The RDP masks s before the texel address, so the part past the mask mirrors. `pm_tex.c` read 64 texels from a 32 texel row, so the star was garbled. Fix: `pm_tex_set_wrap` now keeps mask and mirror flags and `expand` reads the image through the mask (the tile becomes the 64 wide mirrored window, clamped at its edge). The cache key includes the mask. (b) The decomp macro `_SHIFTL(v, 16, 16)` casts the texture rectangle start s and t (negative floats, for example -144) with `(unsigned int)(v)`. On ARM that is 0 (saturating). On MIPS it wraps. The wipe centre was wrong (it closed to the top left corner). Fix: `include/PR/mbi.h`, under `TARGET_3DS` the macro goes through `(int)` first. The N64 expansion is the old text.
3. Black frame at pause open. The pause begin frame does not draw (the game skips `gfx_draw_frame`). The port cleared the target every frame and presented it, so one frame was black between the world and the pause page (f108 in `preset=toad`, START at scan 100). Fix: `pm_gpu.c` clears the target at `pm_gpu_end` and only if the game drew something that frame (a GX memory fill runs before the command list queued by `C3D_FrameEnd`). A frame the game does not draw keeps the last picture, as on the N64.

### Tools added (class E, debug only)
`shotwin=<first frame>:<n>` (one shot per game frame), `shotmode=<mode>:<n>` (n shots from the change to a game mode, for example `9:90` battle end, `6:100` game over, `2:100` title), `tourfx=<n>` (the `tour` map changes use ScreenTransition n), `xtrace=<first>:<n>` (log fill rects, texrects, render state in a frame window), `noprimz`. The log shows `shot <n> frame <f>` and `game mode <m> frame <f>`. Helper scripts (git-ignored): `3ds/logs/x01_run.sh`, `x01_run2.sh`, `x01_run3.sh`, `xsheet.py`.

### Table (result by eye, one shot per frame; shot number = game frame)
| transition | how triggered | result | shots |
|---|---|---|---|
| Map change fade out and in, TRANSITION_STANDARD (0) and TOY_TRAIN (1) | `warp=kmr_20:4 story=-107 tour=kmr_20:4 tourfx=0` (and 1) | ok (fade to black with dither, fade in) | sheet x01_fx0, f89 to f157 |
| End demo black (2), end demo white (3), game begin or end (4), outro end scene (5), chapter white (6), slow white (7), star card (14) | same, `tourfx=2 3 4 5 6 7 14` | ok (fast or slow black or white fades, outro starts light grey and fades to black as the code says) | x01_fx2 to fx14, f89 to f157 |
| Mario head wipe, ENTER_WORLD (8), MARIO_WHITE (9), MARIO_BLACK (10) | `tourfx=8 9 10`, also every map entry (`preset=toad`) | fixed (was off centre, now closes to the centre and opens from the centre, head shape) | fx8, f96 to f123 |
| Star wipe, AFTER_SAVE_PROMPT (11), END_PEACH_INTERLUDE (12), PEACH_CAPTURED (13) | `tourfx=11 12 13` | fixed (was a garbled shape, now a star around the centre; 12 has the 160 alpha star then the Mario head) | fx11, fx12, fx13, f107 to f157 |
| END_CHAPTER_INTERRUPTED (15) | `tourfx=15` | ok (white out, Mario head in) | fx15 |
| SLOW_BLUR_MOTION (16) | `tourfx=16` | not used by the game (no map script sets it, the enum comment says unused). It draws the previous frame blur box, no fault | fx16 |
| Battle enter, real encounter (swirl, black fill outside, previous frame blur) | `warp=kmr_09:0 story=ch0_goombario partner=1 padscript=wait80,right*130,wait200` (walk into the Goomba) | fixed (was: sky black first, no star shape). Now the dark rotating hole closes on the Mario and Goomba centre with the previous frame smear, then black, then the battle fades in | b3 sheet, f156 to f240 (wipe starts f159, game mode 8 at f197) |
| Battle enter, `battle` debug hook path | `warp=kmr_20:4 story=-107 battle` | ok by design: the hook sets ENCOUNTER_STATE_NONE, so the encounter code that draws the swirl does not run. Plain black then fade in (no swirl) | h1 sheet, f98 to f197 |
| Battle exit (win, back to the map) | `warp=kmr_09:0 ... sp=99 shotmode=9:90 padscript=@s19d_w7.txt` | ok (black, fade in to the map in about 12 frames, enemy gone) | e2 sheet, f1161 to f1231 (mode 9 at f1160, mode 4 at f1165) |
| Title fade in, title to file select | `fast autoa freshsave shotmode=2:100` | ok (white fade in, logo slides out, file windows fly in) | t2, f63 to f165 |
| Logos fade | no `fast`, `shots shotwin=0:200` | ok (N64 logo, Nintendo logo, Intelligent Systems logo fade in and out) | l1, f0 to f200 |
| Intro curtain valence and page fades | `fast shots shotwin=130:200` | ok (valence at the top, white page, night sky fade in, page wipe) | i2, f130 to f330 |
| Intro letterbox bars (kmr_20 viewport bars) | every kmr_20 run | ok (black bars from the camera viewport, picture crisp) | fx0 and all kmr_20 sheets |
| Pause open | `preset=toad padscript=wait100,START*2,...` | fixed (one black frame at f108 removed). The world holds 4 frames, then the page unrolls | p1, f104 to f121 |
| Pause close | same | ok (stats page to world, first world frame slightly dark) | p2, f200 to f230 |
| Game over (spotlight cone, GAME OVER letters, fade to title) | `warp=kmr_09:1 ... hp=1 shotmode=6:100 padscript=@s19d_wc.txt` | ok (letters appear one by one, cone and spot, then title) | g1, f1242 to f1450 |
| Previous frame blur (draw_prev_frame_buffer, OVERLAY_BLUR) | `preset=toad prevtest padscript=wait130,right*120` | ok (ghost trail of the moving scene inside the box) | v1, f148 to f159 |
| White and colour flashes | the white types 3, 6, 7, 9, 11, 12, 14, 15 above | ok | see above |
| Curtain open and close in the intro (curtains.c) | `fast shots` | ok for the valence. The curtain scale animation is only in map scripts that no linked scene reaches here | i1, i2 |
| Spotlight stencil (OVERLAY_VIEWPORT_SPOTLIGHT, sharp circle) | no caller in the decomp | not used by the game | none |
| World darkness and battle darkness (OVERLAY_WORLD_DARKNESS, OVERLAY_BATTLE_DARKNESS, dark rooms such as kgr_01, kgr_02) | `warp=kgr_01:0 partner=1`, `warp=omo_14:0` | WRONG: the whole screen is black (Mario and the map are not visible). Row X02 | d_kgr_01, d_omo_14, f0 to f150 |

### Row X02 (new): darkness stencil needs a render target in the z-buffer memory
`appendGfx_darkness_stencil` (src/screen_render_util.c) sets the colour image to `nuGfxZBuffer` (RGBA 32 bit 64x64, then I8 64x64). It draws three quads into it (a constant fill and two rotating blurry circle quads). It converts the RGBA32 image to I8 with four texrect strips. It loads that I8 image as a texture (mirror and clamp, mask 6, tile 128 over 64) and draws the full viewport texrect from it. Then it fills the z-buffer back. The port draws the quads onto the real screen and reads the texture from CPU memory that was never written, so the overlay is fully opaque. `xtrace=120:1` on kgr_01 shows it (texrect strips 0..64, fill 0 0 63 63, final texrect s -6848 dsdx 2048). A fix needs a generic render to image: `G_SETCIMG` to a non cfb address with a small scissor goes to a 64x64 PICA render target, and a texture read from the same address binds that target (with the I8 reinterpretation as a second target and the mask and clamp window). Estimated size: 2 targets of 64x64x4 B = 32 KB in VRAM (not the O3DS heap) and about 3 KB of code. Not done here: it is a slice of its own and it is not a transition. Affected maps: kgr_01, kgr_02 (linked), omo_12, omo_14, kpa_01, kpa_03, jan_11, kzn_08 (not linked) and the battle darkness actors (omo2, omo3, not linked).

### Seen and not fixed here
- V05 (pause page flip): the page unrolls as a skewed quad for 3 frames (f113 to f115) and the stats text is complete from the first full frame. Not a regression. Not changed.
- V10 (Strategies checkerboard): seen again in the e2 run (battle sheet f27, the window with the menu list). Not a transition fault. Cause not found here. The draw_box pattern path (`box_pattern_rect`, tile 0 pattern and tile 1 corner) is the suspect. The tile mask change does not touch it.
- V09 (kmr_22 rainbow stripes) is unchanged by the fixes above.

### Checks (nothing else regressed)
- Title, file select, pause, text windows, sprites and backgrounds in the sheets above look as before. A/B with `noprimz` (same switches, shots every 30 frames): the `preset=toad` and `toad_west` walks differ only in 5 pixel wide timing spots. The kmr_09 battle differs by timing only (emote and menu timing). Not seen by eye: mac_00 water (`water_gfx.c`), the mac_01 crystal ball and the battle attack result text (`attack_result_text.c`). They use `G_ZS_PRIM` too and now get the real prim depth.
- Full normal chain `fast autoa freshsave` (turbo, -FaultStop): loads at scans 562, 2316, 2562, 3178, 3818, 4222, 4472, 0 unmapped, no assert.
- Memory: 3dsx 10,909,052 to 10,912,156 B (+3,104 B, code), bss under 100 B. Estimated O3DS spare about 512 KB (515 before), above the 500 KB limit. Not hardware.

## V02 result 2 (slice V02: brown wall over the fence and bushes, FIXED, Azahar only, port layer only)

All items belong to slice V02. Azahar only. Check used: warp runs (-Turbo -Smoke -FaultStop, fixed length), plus the full normal chain once (the fix touches every triangle, so also the intro and map change path).

### Cause (slice V02)
- The yard of kmr_20 draws a white picket fence in front of tall bushes. Earlier V02 agents looked for a wall over the bushes but did not know the white fence exists. With the wall, the fence is missing entirely (nothing white in the yard). That is the user report.
- Per draw log (new switch `drawlog`) plus `ddonly` (draw only some chunks) showed it. The bushes and the fence are chunks #8 to #24 of model group g88. The brown wall is chunk #25, a separate group g98 (render task 2, mode SURFACE_OPA_NO_ZB, bounding box x -336 to 902, y -100 to 0, z -619 to 619). Its 6 triangles lie at y 0 down to -100, below the ground, around the yard edge: a cliff skirt that faces outward.
- Both groups use the no depth render mode (Z_CMP and Z_UPD off, so painter order). The game sorts them by model centre: g88 (180,118,-235) is farther than g98 (283,-50,0), so g98 draws last. On the N64 the skirt never shows, because the map sets G_CULL_BACK and the RSP drops the triangle that faces away. The port had no triangle culling at all (`C3D_CullFace(GPU_CULL_NONE)`, `G_CULL_*` ignored in pm_gbi.c). It drew the skirt from inside the yard, over the fence and bushes.
- Not the cause: the depth range and precision, decal bias, alpha test of the fence, render mode state (S26), model sort code in model.c (same C code as the N64). The sort order and the render modes in the port are as the game asks.

### Fix (slice V02)
`pm_gbi.c` `draw_tri`: after the clip reject, if the geometry mode has G_CULL_FRONT, G_CULL_BACK or both, take the sign of the screen area (same formula as libultraship `interpreter.cpp`, sign of `N*w1*w3`, flipped when one corner is behind the eye) and drop the triangle as the RSP does. Switch `nocull` turns it off (A/B). No game-specific logic. 3dsx size 10,912,156 to 10,915,188 B (+3,032 B, code), no new static data. Estimated O3DS spare stays about 510 KB.

### Tools added (class E, debug only, no memory cost beyond the code above)
- `drawlog=<first frame>:<n>`: one line per mesh chunk (a G_VTX at slot 0): `dd f<frame> #<index> top <main list address> vtx <vertex data address> dl <inner DL> <parent DL> oml omh zb img <texture address> fmt w x h prim tris box <screen box> w <min> <max>`. The shot of frame F shows the frame before it (GPU lag): use a drawlog window one frame earlier than the shot.
- `ddonly=<lo>:<hi>`: inside the drawlog window only chunks with index lo to hi draw (bisect). `ddverb`: one `ddv` line per triangle corner (object and clip coordinates).
- `depthview=<1|2|3>`: 3D fragments get a depth colour (1: red near to blue far, 2: one colour per draw, 3: repeating depth bands). Seen: z/w is above 0.98 for the whole yard (red everywhere in view 1), so the depth range is compressed to the far end, but the depth order of the yard is right. No depth fault was found.
- Helper scripts (git-ignored): `3ds/logs/v02_run.sh`, `v02_diff.py`, `v02_cmp.py`.

### Seen by eye (slice V02, A/B with `nocull`, one shot per frame, same switches)
- kmr_20 yard (`preset=yard`, a walk to the left, to the right, up and down, 300 frames, shots every 12th frame in sheet v02_sheet_y2): white picket fence and bushes visible from every camera position, no brown wall. Before: a brown wall in all of them (sheet v02_sheet_y1).
- Normal chain `fast autoa freshsave` (turbo, -FaultStop): loads at scans 562, 2316, 2562, 3178, 3818, 4222, 4472, 0 unmapped, no assert. The night view of the Mario house and the yard after the letter show the fence (shots 10, 15, 50). With `nocull` the brown wall is back in the same shots.
- Positions checked in the first search, before the cause was found: kmr_20 entry 4 yard (padscript walk both ways, up to the fence), kmr_20 intro night view with Luigi, the letter, the yard after the letter, house interior. Not checked: kmr_02 near the house, the demo scenes, the title idle loop (not needed after the root cause).
- Unchanged by the culling (differences are the same size as the run to run noise of two identical runs): Toad Town (`preset=toad`, 59 px at most), kmr_02 (about 650 px, the same with two runs without `nocull`), osr_00 (0 px), pause open and close (65 px at most), the real battle in kmr_09 and the `battle` hook in kmr_20 (stage, Mario, Goombas, menu, HUD look the same), the file select, intro text, windows.
- Not checked by eye: hardware (O3DS, N3DS), the intro maps hos_05, the title demo scenes, maps that are not linked. Mirror matrices (floor reflection, `mtxFlipY`) use the same test as the N64 and were not seen.


## V05 result (slice V05: menu flip artifacts, FIXED, Azahar only, port layer only)

All items belong to slice V05. Azahar only. Nothing ran on hardware. Check kind: warp (`-Turbo -Smoke -FaultStop`, fixed length, `shotwin` one shot per game frame) for the pause, the normal chain `fast autoa freshsave shots` for the file select.

### Still there on the current build? Yes (slice V05)
- Shots on consecutive frames of a pause page flip (`fast preset=toad shots shotwin=96:120 padscript=wait80,START*3,wait60,R*3,wait40,R*3,wait40,L*3,wait40`). Before: during each flip the page contents (text, icons, tab lists) drew flat in the top left corner of the screen, on top of the skewed page, and the skewed page body lacked its contents. File select flip: the panels (File 1 to 4) lost their text and one quarter of each panel background.
- A/B `nocull` (V02 back-face culling off): the same pictures. The culling did not change or fix it. S25 and X01 did not either.

### Cause (slice V05, two faults, evidence by `xtrace`, `drawlog`, `ddonly`)
1. draw_box renders the contents of a rotating window (flags DRAW_FLAG_ROTSCALE) into a game buffer: `gDPSetColorImage` to `mdl_get_next_texture_address` (log: `fillrect 0 0 288 153 ... cimg 0xdcc460`), draws the contents there with fill and texture rectangles, sets the colour image back to the frame buffer and draws the buffer as strips (6 rows each, `img 00dcc480 289x7`, 320 wide quads) through the rotation matrix. The port ignored the colour image: the contents drew on the screen at the window origin, and the strips read a buffer nobody had written. `ddonly` kept the flat rectangle with the strips switched off.
2. The scissor was applied late. `pm_gpu_set_scissor` changed the scissor while triangles were still queued. The last quarter of the window background (4th corner chunk) was queued when draw_box set the next pass's scissor (0, 0, width, height), and the batch drew with that smaller scissor. Found with `ddonly=10:10` (drew nothing) and the `ddverb` triangles (both triangles kept, so not culling).
3. Related: the render tile source window used the SetTileSize origin. The strip load is `LoadTextureTile` at row `idx * 6` followed by `SetTileSize` at (160 - width / 2, 0). The RDP reads the image from the LoadTile origin and uses SetTileSize only as the texel coordinate of the tile start.

### Fix (slice V05)
- `pm_gpu.c` `pm_gpu_off_begin` / `pm_gpu_off_end`: a colour image that is not a frame buffer and not the saved frame (`pm_fb_native`) becomes an off-screen render target: a 512 x 256 RGBA8 texture in VRAM (`C3D_TexInitVRAM`, `C3D_RenderTargetCreateFromTex`, no depth buffer), up to 4 per frame, allocated at first use, cleared when first used in a frame. The pass starts at the first fill, texture rectangle or triangle after `G_SETCIMG` (`off_screen_check` in `pm_gbi.c`) and ends when the colour image changes. 2D uses an unrotated ortho matrix, 3D (`rebuild_3d`) follows `off_active`. A fill in FILL cycle mode writes the pixel (no blend). Blending: colour source over, alpha source over destination (a covered pixel becomes opaque).
- `pm_tex.c`: a load of an RGBA16 window that lies inside an off-screen image of this frame binds the render texture. `pm_tex_origin` gives the window offset, `pm_gbi.c` (`ensure_tile`, `G_SETTILESIZE`) subtracts it from `tile_x0` and `tile_y0`, so the existing uv code is unchanged.
- `pm_gbi.c` G_SETTILESIZE tile 0: source window from the LoadTile origin when a LoadTile was seen for the image (`ld_valid`), same as tile 1 already did. No change for `gDPLoadTextureTile` (both origins are equal).
- `pm_gpu.c` `pm_gpu_set_scissor`: draws the queued batch before the scissor changes.
- Debug switches: `nooff` (old behaviour for A/B), `offdump` (first off-screen image of each frame as shots/off<frame>.ppm, 12 files). 3dsx 10,915,212 B (V02 commit build) to 10,918,572 B (+3,360 B code). Static data: about 0.6 KB (4 render target structs). VRAM: 512 KB per off-screen image in use, 4 images at most (the file select flip uses 4, none was dropped: no `V05:` log line). Estimated O3DS spare stays about 510 KB (main memory; the images are VRAM).

### Seen by eye (slice V05, shots on every frame)
- Pause page flips Stats to Items to Spirits to Map to Stats: the page rotates as a perspective quad with its contents (text, icons, star spirit pictures, map) rotating with it. Sheet 3ds/logs/v05_m1.png (git-ignored).
- File select flip (panels fly in and out): the four panels keep their text (NEW, File 1 to 4) and the full background while they turn. Sheet 3ds/logs/v05_p3.png.
- A/B against the V02 commit build (same switches, same scripts, per shot pixel difference above 24 levels in more than 50 pixels): pause open and flips differ (expected). The static pause pages (Stats, Items) are identical. Toad Town walk (200 shots): identical. File select chain `autoa freshsave shots` (200 shots, every 30th frame): no difference above 1500 pixels, only NPC animation timing in the castle scene. Battle hook (kmr_03, command menu, 240 shots): differences come from a different animation and camera time in two runs (the scene looks the same), same kind of noise as V02 recorded.
- Not checked: hardware, other languages of the pause, the pause with items or a partner, the other maps that use rotating windows (battle menu windows flipping), the intro.

### V06, V07, V08 on the current build (slice V05 quick check, nothing fixed)
- V06 (ground disappears when entering buildings): NOT REPRODUCED, NOT TESTED to the end. In `preset=toad` Mario reached the door of the house at the pipe (shots show "!" and the door opening), but the door did not start a map change in 480 frames and no ground fault is visible outside. The Mario house interior (kmr_20) was not reached. Still open.
- V07 (items flicker when the camera moves): NOT TESTED. kmr_03 has coins at (345, 205 to 255, -250) (`MakeItemEntity`, FIXED_NEVER_VANISH). The padscript walk from `warp=kmr_03:0` stopped at x 30 (blocked), the coins never came into view. Still open. Next agent: find a map where an item lies on the walkable ground and the camera moves.
- V08 (save block transparency): STILL THERE, and unchanged by `nocomb`. kmr_02 (`preset=kmr02 loadslot=0`, block hit by a jump): the block is a translucent rainbow cube with a white S, the wall behind shows through. `nocomb` gives the same picture (crop 3ds/logs/v05_v8z.png). Not compared with a real console, so it is not known whether this translucency is right. `nocull` A/B was not run for the block (V02 saw no change for entities). Still open.

## V06/V07 paused (slices V06 and V07, short note)
- V06: Toad Town houses are in-map rooms (src/world/script_api/rooms.c, CreateMapRoom in mac_00 rooms.c). Russ T door: padscript `wait80,right*38,up*60,up+A*4,up*20` (preset=toad). Exit: `wait200,down+right*30` (wait until the enter script is done, else the exit script returns).
- Found: entering fades the shrouded group (house, ground, bushes) with PM_RM_SHROUD (blend colour mixed by the fog colour alpha). The port ignored G_SETFOGCOLOR and G_SETBLENDCOLOR, so the ground stayed bright for 7 frames and then vanished in one frame (drawlog: 8 chunks gone at one frame, prim alpha falls 0xdf, 0x7f, 0x1f). At alpha 255 the game draws nothing, so the ground is gone inside the room by design. The background picture stays (bgColor only fills under it).
- Fix (uncommitted before this note, committed after): `pm_gbi.c` sync_render and the two opcodes, `pm_gpu.c` pm_gpu_set_mix (one extra TEV stage), switch `nomix` for A/B. Shots now show the ground darkening. Smoke yard and toad rc 0. Exit works (world returns).
- Not known: whether the user means this vanishing (by design) or a ground that stays missing. Asked in QUESTIONS.md. kmr_20 and kmr_02 doors (map change) not tested.
- Debug: warp log lines now end with `wall <curWall> push <pushingAgainstWall>`.
- V07: not started. Next: kmr_02 or mac_00 coins, drawlog item entity chunks per frame, `nocull` A/B.
- Commands: `3ds/logs/v06_run.sh <tag> "preset=toad shots shotwin=156:10 padscript=..."` (git-ignored helper), sheet script `3ds/logs/v05_sheet.py`.


---

## Original text of the open slice rows (moved from 3ds/SLICES.md on 2026-10-17, unchanged)

The short rows in 3ds/SLICES.md point here. Find a row with grep on its id.

[~] S18 | PARTIAL: pause is real (state_pause.c, src/pause/*, menu_hud_scripts.c, nupireadromoverlay.c). Demo and language select remain, see S18b and S17 | common check
[ ] S30b | Link the remaining areas kpa (Bowser's Castle), kzn, flo, jan, sam, pra (154 maps, about 2 MB more image, estimated spare then 2.6 MB). Needs the answer to Q7 | maps_skip.txt area lines removed, presets and tour clean
[ ] S19c | Fast3D gaps seen in the battle (black and olive polygons during attacks): find the missing pm_gbi.c feature, fix in the platform layer | attack animations draw
[ ] T02 | Low priority, no user action needed. Make `3ds/run_azahar.ps1` safe for the user's Azahar config. Findings of a read-only review: (1) Set-Turbo runs near line 116 but the try starts near line 176: a throw in between (Copy-Item at 120 with ErrorActionPreference Stop, Add-Type, Start-Process at 155, the Gdb setup) skips the restore. (2) A hard kill of PowerShell, a closed terminal or a reboot skips `finally`. (3) Overlapping runs race: the restore at line 109 of the second run deletes the backup and ends the turbo of the first run, and the whole-file Get-Content/Set-Content read-modify-write (lines 93, 107, Set-GdbStub 78 and 86) can write stale text. (4) A locked config file (emulator running) can make the restore throw and leave turbo values and the backup in place. (5) With no backup the restore does nothing, and a regex miss in Set-Turbo changes nothing silently. Fix: run Azahar with a PER-RUN copy of the config (portable or per-run user directory, or a config flag if Azahar has one: check) so the real qt-config.ini is never edited. Wrap the whole script body in try/finally. Take a named mutex so one run edits at a time. Fail loudly when a key is not found | the real qt-config.ini is never written, overlapping runs do not interfere
[ ] F08 | Fog is not implemented (S25 found it). About 1.3M draws in 2 tour chunks use it (G_SETFOGCOLOR and the fog blender bits in othermode L, the RSP fog factor per vertex; none in the intro, yard or toad). Decide with evidence which maps and scenes depend on it (the tour counts are in the S25 result), then implement it in the platform layer (PICA fog table or a shader path). Not started | the map list with fog is written down, the chosen method renders fog on one such map and does not change maps without fog
[ ] V09 | S25 changed the intro background tints (shots 70 to 77) from light blue to dark or red. Nobody could compare with a real console. Check against the decomp (the hos_05 intro background and the screen tint and overlay colours in the intro scripts) and against paperboat's behaviour whether the new colours are correct or a regression. Do an A/B with the `nocomb` switch. Also check nok_01 (`au_syn_begin_audio_frame` unmapped access, seen only in the tour). Not started | a written verdict per shot with the decomp values, and the nok_01 cause or a reason it is the tour only
[ ] F06 | LATER, big. 30/60 fps toggle (user wish). The game logic, physics, animation counters and script waits assume 30 Hz steps. So 60 fps means rendering extra frames between logic steps with interpolation of positions and camera between two logic states (hooked into model transforms or display lists), not running the logic at 60 Hz. Plan only after the hardware run shows the O3DS holds 30 fps with margin | plan in HANDOFF.md
[ ] F07 | LATER, after the hardware run (user wish): widescreen mode. Research is in HANDOFF.md section "Widescreen and 60 fps research". Plan (inference, opt-in first): aspect-aware viewport and scissor in pm_gpu.c, projection aspect 400/viewport height for CAM_DEFAULT and CAM_BATTLE, widen the model.c cull (src/model.c:2804 `xComp >= -1.0f && xComp <= 1.0f`), edge helpers for right-edge UI, per-scene exceptions (title, parade backdrops, framebuffer captures) | widescreen option shows 400 px wide scenes without pop-out at the edges
[~] F01 | Fault sweep: find accesses that Azahar absorbs (logs an 'unmapped' warning and returns 0) but a real 3DS would data-abort on. Dynamic: run many paths in Azahar and collect every unmapped read or write with PC and address, grouped by cause: idle title (8 s demo timer), a refused GotoMap, battle and demo battle (branch s19-full-battle, after merge), pause, save and load, jumping, walking into walls and loading zones in the linked maps, the intro on a fresh save. Static: null pointer reads, uninitialized function pointers, array indexes without bounds, big-endian struct overlays, plus the H01 list in HANDOFF.md. Fix root causes, no guards that hide the fault (the spriteIdx<0 guard from S22 is the example to avoid) | list of causes with PC and address, each fixed or recorded, `fast autoa` and the other paths run with no unmapped warning
[ ] S28 | Hardware run on O3DS and N3DS: first boot line (real memory limit), audio cost line, FPS. Settles Q2 and helps Q1 | numbers in HANDOFF.md
[ ] H03 | Replace the fixed memory split (4 MB N64 RAM, resident ROM, 20 MB linear pool in pm_mem.c, taken as all remaining commit) with sizes derived from measured need: resident assets, GPU linear pool peak (cmd 4 MB, verts 1.3 MB, tex up to 4 MB, scratch), N64 RAM block of 4 MB. Keep a few MB of commit headroom for later system calls. Log the numbers at boot. Needs the hardware boot line (S28) for the real memory limit. User view: all used assets must be resident, but blocks that are not needed should not be reserved | numbers logged at boot, no regression on `fast autoa`
[~] V06 | PAUSED: mac_00 house rooms reproduce it, blender mix added (see HANDOFF V06/V07 paused), user must confirm. User report (2026-10), UNVERIFIED diagnosis, this round: entering buildings makes the outside ground disappear. Hypotheses: model group visibility flags (the game hides or shows model groups on entering: EnableModel, SetGroupEnabled, model flags) stay wrong; the interior draws with a depth or blend mode that wipes the ground (S26 depth state: Z_CMP, Z_UPD, decal); the camera near or far plane; the interior and exterior share a texture or buffer. Test in a map with enterable buildings (mac_00 Toad Town houses, kmr_20 Mario house): warp, enter, shots, rmlog | cause named with evidence, fixed
[~] V07 | PAUSED: not started, see HANDOFF V06/V07 paused. User report (2026-10), UNVERIFIED diagnosis, this round: some item entities flicker in and out of existence when the camera moves. Hypotheses: near-plane clipping (clip_near, w > 0.05 in pm_gbi.c); triangle culling by clip bits (the near plane is not tested in clip_bits); float24 depth precision in the PICA shader; visibility or sort of item entities (render_item_entities, item_entity.c) using the camera; an intermittent texture load failure ('tex cache full' or another failure line in the log); the effect or entity draw skipped for a frame. Test: warp to a map with item entities (a coin or item on the ground), move the camera with padscript, shots on consecutive frames and a log of draws per frame for the item | cause named with evidence, fixed
[ ] V08 | User report (2026-10), UNVERIFIED diagnosis, this round: transparency on the save block looks odd. Entity: Entity_SavePoint (E02 verified the kmr_02 block draws and works: warp to kmr_02). Hypotheses: blend or alpha handling for translucent entity parts (S26 render mode: Z_UPD depth write on translucent faces, alpha compare, decal); the S25 pm_comb.c general combiner plan path and the nocolor blender added in S25 (a regression is possible: compare with and without the plan path, the switch `nocomb` turns the plan path off); ordering of translucent draws; an I4 or IA texture with the LA8 conversion from S26 giving wrong alpha; two-sided faces drawn without back-face handling. Method: shots of the save block close up (padscript walk), rmlog of the entity draws (othermode, combiner, texture formats), compare `rmoff` and `nocomb`, compare with reference/soh-3ds interpreter for the same combiner and blender | cause named with evidence, fixed
[ ] F03 | Depth query reads a NULL z-buffer: `is_point_visible` (src/model.c:4497) loads 4 pixels of `nuGfxZBuffer` (NULL in the port) into `DepthCopyBuffer`, which stays 0 (log `tex ptr 0000fd7e`, hos_05 intro lamp, bulb_glow). Give the platform layer a depth read back for the one pixel, or another real source for the depth value | `DepthCopyBuffer` holds real depth, no `tex ptr` line
[ ] V09 | Found in S19b, unverified cause: kmr_22 ("A Plea from the Stars" card, `warp=kmr_22:0 story=-107`) shows a garbled rainbow bar of vertical stripes above the title text. The text itself is right. Check the texture format of the card picture in `src/world/area_kmr/kmr_22` (CI or IA sizing, palette, tile size) in the Fast3D path | the card shows its picture
[ ] V10 | Found in S19d (by eye): in battle the Strategies submenu and the level-up description box draw as a white and brown checkerboard (cut-out pattern), the menu entries are not readable. The same pattern covers the bottom text box. Probably window panel texture or alpha/dither handling in pm_gbi.c (hunt: `warp=kmr_09:1 story=ch0_goombario partner=1 shots`, taps of A for the tutorial text, up to Strategies, A). Owner row B01/X01 area. | Strategies list readable in a shot
[ ] V11 | Found in S19d (unverified): kmr_05 from entry 0: Mario walking right stops at x about 75 to 88 (z -10 to -104) on y 0, the path to the first Goomba (x 305, y 50) is not reachable. May be real geometry (needs a ramp at another z) or a collision fault like G01. Check with the collision data. | Mario reaches Goomba_01 in kmr_05
[ ] X02 | Found in X01: the darkness overlay (OVERLAY_WORLD_DARKNESS, OVERLAY_BATTLE_DARKNESS, dark rooms kgr_01, kgr_02, omo_12, omo_14, kzn_08) draws the whole screen black. appendGfx_darkness_stencil renders into nuGfxZBuffer as a 64x64 RGBA32 colour image, converts it to I8 and reads it back as a texture. The port needs a generic render to image for a non cfb G_SETCIMG (64x64 PICA render target, texture bound from the same address, I8 reinterpretation, mask and clamp window), estimated 32 KB VRAM and 3 KB code, see HANDOFF "X01 result". No game specific stand-in. | `warp=kgr_01:0 partner=1` shows the map with a soft dark circle around Mario
[ ] G01g | NEXT round, after the handoff. Golden frames: an automatic pixel comparison of fixed scenes (title, file select, pause pages, toad, yard, a battle, the intro start). Save reference screenshots. A script runs the scenes with warp and turbo and compares pixels with a tolerance. It prints the scenes that differ and the size of the difference. Agents then look only at scenes that changed. Build it before the performance phase | script and references exist, one run prints a table


---

## Original QUESTIONS.md (moved on 2026-10-17, unchanged)

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

### V06 question (from slice V06)
- In the Toad Town houses (Russ T, shop, dojo, waterfront) the game hides the outside ground on purpose when you enter (shroud fade to nothing, the background picture stays). Do you mean this, or a ground that stays missing after you leave a house or in another building? Give the map, the building and a screenshot. Answer: (open)


---

## Original 3ds/ORCHESTRATOR.md (replaced on 2026-10-17, unchanged)

# Orchestrator handoff

Written 2026-10-17 for a new orchestrator chat. Read this file, then HANDOFF.md, 3ds/SLICES.md and 3ds/QUESTIONS.md.

## 1. Role
- The orchestrator only orchestrates. It never writes code. It never reads code or project source.
- Subagents do all work: code, scans, reads, queue edits, notes.
- Launch ONE slice agent at a time for anything that builds or runs Azahar. The build directory is shared. Only one Azahar window can run. Run the agent in the background.
- Read only the agent report and the last lines of 3ds/PROGRESS.log.
- Relay each report to the user in plain terms: what was done, the numbers, what was not verified, the next step. Then launch the next slice.
- Read-only research agents (Explore type) may run in parallel with a build agent.
- Post a status to the user after every report. Never leave the user without feedback during a long run.
- Start no subagent the user did not expect. Stop at a finished slice when the context grows long. Write a short HANDOFF.md note and ask the user to start a new chat.

## 2. User decisions (2026-10)
- All subagents use the default model. No tiers.
- Never push to git. No Claude or Anthropic attribution in commits or PRs.
- NO STUBS. Never write a stub or stand-in for decomp code or data without asking. A port-layer shim is allowed only for real N64 OS or hardware functions with no game logic.
- Fix root causes. Never hide a fault with a guard.
- Q1 (a): merge the whole real battle tree (branch s19-full-battle), everything resident, no stand-in table.
- Q2 (a): keep needed assets resident in RAM. No SD streaming during play. The ROM file itself need not stay resident.
- Q4: merge battle first. Then link the demo maps.
- Q5: the obfuscation shims stay ignored (empty). The crash screen functions only if the debug output can be used (slice S31 checks). Else leave them.
- Q6: link all maps. Measure first. The memory ceiling stopped at 267 of 421.
- The game runs at exactly 30 fps (PC01). A 30/60 toggle comes later (F06).
- The O3DS is the hardware target. It should reach full speed. Performance work comes after everything works.
- ARM11 math audit: experiments allowed. Game-logic math changes need a replay check and the user approval.
- Use libctru natives for threading and sync.
- The log is non-blocking on core 1 (L01).
- Workload split: core 0 = logic plus the CPU part of the RSP work. Core 1 = audio. PICA GPU = rasterization plus vertex work. Consider ndsp for audio mixing.
- Testing: feature tests use the warp with -Turbo -Smoke and a fixed length. Never an open wait. Tours iterate on the failing set. Run the full tour once at the end. Run the full normal chain only for slices that touch boot, title, file select, intro or the map change path.

## 3. How to run
- Debug switches go in `debug3ds.txt` or `-Debug "..."`. Never leave `fast` in debug3ds.txt.
- Switches: `fast` (skip logos and the 30 Hz pad wait), `autoa`, `freshsave`, `shots`, `padscript=`, `padrand`, `warp=<map>:<entry>`, `story=`, `preset=` (intro yard toad toad_east toad_west osr kmr02), `partner=`, `loadslot=`, `smoke`, `smokeframes=`, `tour=@file`, `turbo`, `rmlog`, `rmoff`, `phase` (`phasecsv`, `phasegbi`), `ptrcheck`, `romstrict`, `romtrace`, `logstat`, `lognull`, `logflood`, `audiostat`, `stats`, `scripts`, `faulttest`, `crashtest_call`.
- Main command: `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "fast preset=toad"`. Other options: `-Timeout`, `-Stall`, `-Until`, `-Gdb -GdbScript`, `-TurboFactor`. Exit 0 = ok, 1 = failed, 2 = unmapped access or crash.
- Normal chain: `-Debug "fast autoa freshsave" -Stall 40`. Use run_azahar.ps1, not go.ps1.
- Never start azahar.exe with other arguments than the .3dsx path (modal dialog).
- Hardware skills: `pull-n`, `pull-o` (pull log and crash dumps), `push-a` (Azahar), `push-n` (N3DS 192.168.1.223), `push-o` (O3DS 192.168.1.168).
- Progress log: each agent appends one line per step to 3ds/PROGRESS.log (`HH:MM ID text`). Logs, traces and shots go under 3ds/logs/ (git-ignored). Never commit logs. Do not run `git checkout` on tracked log files.

## 4. Agent prompt template
```
Repo: c:\Users\Andrew\Documents\GitHub\papermario-3ds-port. Branch main. Do not git push. Add no Claude or Anthropic attribution to commits. Commit locally in plain English. Commit only your own files.
Read HANDOFF.md (sections "Do not", "Warp" and the result of your slice) and 3ds/SLICES.md first.
Append one line per step to 3ds/PROGRESS.log (format: HH:MM <ID> text). Write logs and shots under 3ds/logs/. Never commit them.
Rules:
- No stubs. Never write a stand-in for decomp code or data. If a stub looks like the only option, write BLOCKED: <reason> in HANDOFF.md and stop.
- Fix root causes. No guard that hides a fault.
- Testing: warp with -Turbo -Smoke -FaultStop and a fixed length. Run the full normal chain only if the slice touches boot, title, file select, intro or the map change path.
- Threading and sync use libctru natives.
- Write notes and replies in ASD-STE100 English: short sentences, one instruction each, active voice, no semicolons, no praise.
Overnight mode: if you need a user answer, add the question to 3ds/QUESTIONS.md and keep working on what you can do.
Task: <slice id and text from SLICES.md>
Verification: <the done check from SLICES.md>. Say what you saw by eye and what you did not check.
Write a result section in HANDOFF.md and mark the row in SLICES.md.
Report (5 lines or fewer): slice id; last commit hash; result; what was and was not checked by eye; BLOCKED or not.
```

## 5. State (2026-10-17)
- RUNNING when this file was written: S25 (Fast3D gaps, combiner audit, pm_comb.c). Its row in SLICES.md still shows `[ ]`. PROGRESS.log shows its commit af71c887b at 02:30. It may still run. Read its report in the S25 note in HANDOFF.md and the S25 row in SLICES.md before you launch anything.
- Queue, in order:
1. S25: finish, read the report. The battle part of S25 needs the merge.
2. PC01 (HIGH): exact 30 fps pacing in pm_frame_loop. Measure steps per second first. No input needed.
3. S28: hardware run on the O3DS and N3DS (boot line, audio cost, FPS). Needs the user: O3DS on, dspfirm.cdc on the SD card.
4. H03: derive the memory split from measured need. Needs the S28 boot line.
5. S31: check whether the crash screen can use the debug output (Q5). No row exists yet in SLICES.md. Create it.
6. S30b: link kpa, kzn, flo, jan, sam, pra (154 maps). Needs Q7.
7. F01 remainder: fault sweep paths (battle after the merge, pause, save, zones). Row is `[~]`.
8. F03: depth read back for `is_point_visible` (NULL z-buffer). No input needed.
9. T02: run_azahar.ps1 safe config (per-run config copy, mutex, try/finally). Low priority.
10. V02: wall over bushes. Not reproduced. Needs the user details (section 6).
11. Battle merge: branch s19-full-battle (Q1 a). Rebase first. `pm_title.c` was deleted on main, so the `battle` and `demobattle` hooks need a new place. Needs the memory answer (Q7).
12. S19b: link src/world/model_anim/kzn/*.c. After the merge.
13. S19d: real encounter start. After the merge.
14. S29: kkj_00 chain, title demo timer. After the merge and the demo maps (Q4).
15. Remaining 154 maps (same as S30b if not done earlier).
16. Performance phase (HANDOFF "Performance phase"): PO2 vertex once at G_VTX, GPU transform, audio on core 1 or ndsp, ARM11 math audit, F06 30/60 toggle plan. Re-measure on the O3DS.
17. Final re-audit of all port files (like S20): list stand-ins, confirm each is gone or approved.

## 6. Open questions for the user (from 3ds/QUESTIONS.md)
- Q7: fit all maps plus the battle tree? All 418 maps leave about 2.6 MB (O3DS estimate). The battle tree needs about 3.9 MB. Options: stream rare map data, shrink the resident ROM, accept less spare, or wait for S28.
- PO2: transform each vertex once at G_VTX (CPU only, small change). Not in QUESTIONS.md yet. Ask the user.
- V02: where exactly does the back wall render over the bushes? Map, moment, camera angle, device. A photo helps if it is hardware only.
- Is the O3DS at 192.168.1.168 on, with dspfirm.cdc on the SD card, for S28?
- QUESTIONS.md still lists Q1, Q2, Q4, Q5 as open. Section 2 holds the user decisions. Ask an agent to update QUESTIONS.md.

## 7. Start prompt for the new chat
```
Read 3ds/ORCHESTRATOR.md, HANDOFF.md, 3ds/SLICES.md and 3ds/QUESTIONS.md. You are the orchestrator only. Do not read or write code. Subagents do all work. First check the running agent state: read the last 20 lines of 3ds/PROGRESS.log and the open rows in 3ds/SLICES.md. Tell me the status in plain terms. Then launch the next slice with the template in 3ds/ORCHESTRATOR.md, one build agent at a time, in the background.
```

## G01g golden frames (2026-10-17)
- New `3ds/tools/golden.py`. Each scene is one `run_azahar.ps1 -Turbo -FaultStop` run with `shots shotwin=<frame>:3` and `-Until "shot 2 frame"`. It compares shot 0000 with `3ds/golden/<scene>.png`. A pixel differs when a channel differs by more than 8. A scene fails above 0.5 percent of the pixels (battle: 3 percent).
- Scenes and absolute frames: title (`fast`, 400), fileselect (`fast padscript=wait200,START*6,wait400`, 500), intro (preset intro, 183), toad (251), yard (249), battle (preset toad battle, 300), pause1 to pause4 (preset toad, START then R*3 per page, frames 181, 226, 266, 306).
- Result: references saved from the build of 2026-10-17 11:43 (V06 blender mix). A second run: 9 scenes 0.00 percent, battle 0.38 to 1.06 percent (random effects, not exactly the same each run). Pause pages differ from each other by 46 percent of the pixels, so the padscript pages work. Seen by eye: no. The references are not compared with a console.
- Not checked: pause pages beyond 4, a real regression catch (no code was broken on purpose), O3DS or N3DS. Total size of references 536 KB (PNG, tracked).

## H03 memory split and graceful texture failure (2026-10-17)
- Measured in Azahar (maps of the normal chain, `mem map` lines): linear used 8395, 8696, 8736 KB. Minus the texture cache in use, the rest is a constant 7527 KB (vertices, command buffer, audio, scratch, render targets). Texture cache peak 1209 KB. Budget 4096 KB.
- Change: `PM_OURS` 20480 KB to 14336 KB in `pm_mem.c`. Need 7528 + 4096 = 11624 KB, margin 2712 KB for fragmentation. The heap grows by 6 MB (51200 KB on Azahar, and 6 MB more spare on a real O3DS). New boot line: `mem split H03 linear pool ... = fixed need + tex budget + margin, heap`. New line per map: `mem map linear used ... peak ... texture failures N`.
- Graceful failure: `pm_tex.c` sets `fail_last` when the cache is full or `C3D_TexInit` fails. `flush` in `pm_gpu.c` skips a batch that needs a texture when the last load failed. Before, the quad drew with the vertex colour only (white).
- Test: BUDGET set to 128 KB for one build. Log `tex cache full 296x6, draws using it are skipped`, no crash, smoke toad exit 0. BUDGET restored to 4 MB.
- Checked after: smoke yard and toad exit 0, normal chain `fast autoa freshsave` exit 0, 0 texture failures, 0 cache full lines. 3dsx 10,919,208 B to 10,919,784 B (+576 B).
- Not checked: pixel compare of the skipped draws (no shot), O3DS or N3DS (the margin is an Azahar estimate, fragmentation on real hardware is unknown), golden frames.

### V07 items flicker when the camera moves (result: not reproduced, no fix)
- Method. Temporary logs in `draw_item_entities`, `appendGfx_item_entity` and the `dd_end` of `pm_gbi.c` (all removed). New debug switch `testitem=<m|c|s>` in `pm_warp.c` (scene frame 30: a real `make_item_entity_nodelay` mushroom, coin or star piece 60 units beside the player, FIXED_NEVER_VANISH). Item chunks are the `gSPVertex` of `D_8014C5A0` (24 px) and `D_8014C5E0` (32 px) in `src/item_entity.c`.
- Maps with a moving camera: kmr_20 (yard, mushroom, coin, star piece), mac_00 (star piece), mac_02 (real odd key), kmr_02, kmr_04, hos_01 (sprite shading on, the `func_801491E4` path, prim 000000ff), nok_01, mac_01. 200 frame windows with padscript or padrand.
- Evidence. (1) Draw log: the item chunk is present in every frame (yard 230 of 230, mac_00 219 of 219, mac_02 230 of 230) with 2 triangles and a smooth screen box. (2) Pixels: contact sheets of the item region over 60 frames show no frame where the item is missing or broken. The star piece pixel count changes only with its own sparkle animation. (3) `nocull` A/B in the yard, 200 frames: tris, box and `w` identical in all 200 lines. The item display list clears G_CULL_BOTH, so culling is not a cause. (4) Item flags in the world: not hidden, cam bit 0 set. In mac_00 the test item gets ITEM_ENTITY_FLAG_HIDDEN from game logic after 60 frames (a real script, not a draw fault). (5) Gaps seen in sweeps (mac_01 frames 190 to 210, nok_01 271 to 276) are `log: N lines dropped (queue full)` and drop all chunks of those frames, not only the item.
- Ruled out: near plane clip (w 309 to 1043 at the item), clip bits, cull, tex cache (`texture failures 0`, constant `img` address), draw order (dist 0 sorts the same every frame), item visibility flags and render group.
- Not ruled out: an effect that exists only on hardware (cache flush of the vertex data, GPU command buffer), an item type that I did not spawn (tossed items, transparent HIDING items, item block items), the coin sparkle plane that lies in the same plane as the coin (z fight, coin looked stable in the yard sheet), maps with floor reflection. The user report has no map name.
- Memory: 3dsx 10,919,784 B before, 10,919,956 B after (+172 B, the `testitem` switch). Checks: smoke yard and toad exit 0, golden toad and yard 0.00 percent. Not checked: hardware, full tour, normal chain (no change to game or graphics code).

## V08 result (save block translucency, DONE, verdict RIGHT, no code change, 3dsx +0 B)

- Decomp: `entity_SaveBlock_setupGfx` (src/entity/SaveBlock.c lines 24 to 72) draws the star with G_RM_ZB_CLD_SURF and the block faces with G_RM_AA_XLU_SURF | Z_CMP. Both use PM_CC_01/PM_CC_02 (colour TEXEL0, alpha PRIM * TEXEL0) and prim alpha 128. The block is translucent by design (about 50 percent). reference/paperboat has the same code. reference/soh-3ds is an OoT port and has no equal entity.
- Port: the draw log shows combine `fcffb3ff ff2cfe7f` with render mode `00504b50` (blender mix with the frame buffer, XLU depth). The S25 plan path reports PLAN-OK with plan error 0 for it, so the combiner is exact.
- Evidence by eye (one crop, `fast preset=kmr02` with the E02 padscript, shot at frame 440): the wall and the star show through the S block. `nocomb` differs in 1.18 percent of the frame, the block looks the same. This matches the user note that `nocomb` changes nothing.
- Not checked: a real console, the exact look on hardware. Smoke on yard and toad: rc 0. Golden not run: no code was touched.

## QUESTIONS answered 2026-10-18

Old text moved from 3ds/QUESTIONS.md.

### V06. Which problem do you mean? (row V06)
- In the Toad Town houses (Russ T, shop, dojo, waterfront) the game hides the outside ground on purpose when you enter (shroud fade). Do you mean this, or a ground that stays missing after you leave or in another building? Give the map, the building and a screenshot.
- Answer: User answer 2026-10-18: the ground vanishing inside Toad Town buildings is CORRECT. The real fault: when Mario is in a building the outside must be BLACK, but the port shows the normal background there. Find in the decomp how the outside is hidden in building rooms (shroud, background, CreateMapRoom) and make the port draw black outside. Check mac_00 houses, kmr_20 and kmr_02 doors. (Row V06 in 3ds/SLICES.md, waits for X02.)

### V07. Where do the items flicker? (row V07)
- I could not reproduce the flicker in Azahar. Item draws and pixels were stable in 8 maps. Which map, which item, and which camera move show it? Did you see it only on the O3DS or N3DS?
- Answer: SHELVED (user decision 2026-10-18, the user reports it again if seen, do not work on it). (Row V07 in 3ds/SLICES.md.)

### T02 Phase 1 (2026-10-18). run_azahar.ps1 safe for the shared Azahar config
- A per-run copy of qt-config.ini is not possible. Azahar reads only %APPDATA%\Azahar\config\qt-config.ini and the rule allows no other argument. The script saves the file byte for byte to %TEMP%\pm_azahar_qt-config.orig before the first edit and copies it back in finally.
- Named mutex `Global\pm_azahar_run` serialises all runs on the PC. A second run prints a wait message and waits up to -LockWait s (default 900), then exits 3 and runs nothing. The lock stays held until the last read of the shared game log and emulator log, because the next run deletes them at its start.
- A hard kill skips finally. The script writes the PID to %TEMP%\pm_azahar_run.pid. The next run owns the lock, kills that Azahar if it still runs, and restores the saved config. An abandoned mutex is treated as acquired.
- A missing config key throws and the run stops (Set-CfgKeys). Only the [Debugging] keys may be inserted under their section. The old qt-config.turbo-backup file is gone: the byte copy replaces it. A stale build3ds/qt-config.turbo-backup from an old run may be deleted.
- Checks: normal -Turbo -Smoke yard run exit 0, config SHA256 same before and after. Two runs at once: second waited 21 s, got the lock, exit 0, both exit 0, hash same. Forced kill of a run: the config was left changed and Azahar ran. The next run killed it, restored the config, exit 0, hash same.
- Not checked: lock timeout exit 3, the missing key throw, -Gdb, -Interactive, golden.py, a run by the old script at the same time (it ignores the mutex).

## QUESTIONS answered 2026-10-18 (Q7, Q2)

Old text moved from 3ds/QUESTIONS.md.

### Q7. Memory: how to link the last 154 maps? (row S30b)
- Fact (Azahar numbers, O3DS values estimated, not hardware): image about 16.98 MB. Estimated O3DS spare about 518 KB against a limit of 500 KB. All maps plus the battle tree do not fit. Linking the other 154 maps (+2.0 MB) gives about -1.1 MB.
- Options: (a) stream the map data of rare areas. (b) stream the ROM from SD (Q2, saves up to 26 MB). (c) stay at 271 maps. (d) wait for the hardware run (S28).
- Recommendation: (d), then (b) if S28 shows a limit below 64 MB.
- Answer: User answer 2026-10-18: link ALL 421 maps. The -1.1 MB number used the old 20 MB linear pool and is wrong since H03 (pool 14 MB, heap gains 6,144 KB). Basis: docs/MEMORY_BUDGET.md. Spare 4,669 KB with all 421 maps (4,169 KB above the 500 KB reserve). Worst case 573 KB (if the separate 4 MB N64 block is extra). Estimate only, S28 settles it.

### Q2. Keep the ROM resident or stream it from SD? (from M02)
- Fact: resident ROM is about 27 MB. Not measured on hardware. Stays resident until S28.
- Answer: User answer 2026-10-18: keep ALL game data resident in RAM. No SD streaming. No duplicate copies.

### X02 darkness (B0 of buffer_bridge)
- Stray tmpnofill body removed from pm_gpu.c flush (build error). Logs: `X02: darkness draw to 64x64 slot`, `X02: overlay reads 64x64 slot` (pm_gpu.c, pm_tex.c), `X02: darkness world .. alpha` (pm_warp.c pm_x02_log). padscript token `cdown` added. Test tool 3ds/tools/x02_check.py (pixel stats). Watt: partner=6 plus padscript=wait40,cdown*4,wait200. omo_14 does call darkness (earlier note was wrong). kzn_08 is not linked.
### S30b: all 154 id-only maps linked (2026-10-18, lane B)
- Edit: 3ds/maps_skip.txt now has no skip line (area:kpa, kzn, flo, jan, sam, pra and flo_11 removed, link: lines not needed). gen_map_sources.py run (needs build.ninja in the root, copied from the main folder, git-ignored): linked 421, unlinked 0. map_syms.lds has no row. map_rom_ranges.h has all 421 maps with the DMA tag.
- Guard removed in 3ds/src/pm_map.c: `__wrap_get_map_IDs_by_name`, `map_is_linked`, `pm_unlinked_map`, `pm_map_tick` and its history and refuse code, the refusal in `__wrap_set_game_mode` and `__wrap_load_map_by_IDs`. The wraps of set_game_mode and load_map_by_IDs stay (warp hook, shotmode, mem log, data probe). The `--wrap=get_map_IDs_by_name` link flag is gone. pm_main.c no longer calls pm_map_tick.
- Size: 3dsx 10,919,956 to 13,011,156 B (+2,091,200 B). Boot line: commit limit 98304KB used-by-image 18944KB, heap total 51200KB, sbrk-left 23716KB (Azahar numbers). Map line at load: heap used 27543KB, linear peak 7527KB, texture failures 0.
- Checks (-Turbo -Smoke -FaultStop, 300 frames each, exit 0, no fault, no unmapped access): warp kpa_03, kzn_03, flo_03, jan_03, sam_03, pra_03 (entry 0), presets yard and toad.
- Not checked: the other 148 new maps, entry points other than 0, the exit scripts of the new maps, tour of all maps, hardware (S28), full chain, golden frames.

## F01 kmr_23 (lane B)
- Cause (docs/plans/F01_causes.md): the tour warp skips the exit script of kmr_23. The curtain draw callback stays set. `func_80240DA4_9087D4` runs on the next map with a NULL data pointer.
- Fix: 3ds/src/pm_warp.c `tour_step` calls `set_curtain_scale_goal(2.0f)` and `set_curtain_draw_callback(NULL)` before the map change. This is the same as `N(func_80240010_907A40)` in kmr_23/main.c. No guard in the decomp.
- Checks (-Turbo -Smoke -FaultStop): `warp=kmr_23:0 tour=iwa_01:0,iwa_00:0 smokeframes=200` rc 0, no unmapped access. Presets yard and toad rc 0.
- Not checked: the run before the fix (fault not reproduced for comparison), the full tour, the real exit with entry 4 (kzn_19), other maps that set a curtain callback, hardware, tst_12, nok_01.

## F01 tst_12 (lane B, 2026-10-18)
- Measured with temporary logs (removed): `Loop(LVar5)` logged val=-17. From LVar5 = 200 / LVarF + 1 this gives LVarF = -11 (integer). `floating_flower_main` ran more than 12000 times in one run (96 effect slots, then `general_heap_malloc` returns NULL, fault at floating_flower.c:32, 34720 hits at one PC).
- Cause: in `N(D_80240050_B1D340)` (src/world/area_tst/tst_12/main.c) LVarF holds the float -22.5 (DivF of 100.0 by -4.44, because the first Exec passes LVarC = 100 - 544 < 0). `Add(LVarF, 11)` reads LVarF through `evt_get_variable`, which converts the float to the integer -22, and stores the integer -11. `Div(LVar5, LVarF)` gives -18, +1 gives -17. `evt_handle_end_loop` counts a negative value down and never reaches 0. The loop runs about 2^31 times. `Wait(LVarF)` with -11 also never ends.
- This corrects the HISTORY 953 note and the F01_causes guess (Wait(0)). The Loop is the flood, not the Wait.
- The port evt.c code is the decomp code. The evaluation order is the same on the N64 by source reading (not run on an N64). So this is a decomp defect of a cut, unreachable map. No port fix without a guard, so none was made.
- Check: `warp=tst_12:0 -Turbo -Smoke -FaultStop` still exits 2 (expected). yard and toad exit 0. nok_01 not touched.

## F01 nok_01 (lane B, 2026-10-18)
- Fault: `voice->instrument` is -1 in `au_syn_begin_audio_frame` (engine.c:488). Source: BGM player B notes read `track->instrument` = NO_INSTRUMENT (no F5 ran on the track).
- Method: tour chunk trd_05 to nok_04 (39 maps, the list of s30_tour_list.txt lines 131 to 169, warp=trd_05:0, no -FaultStop). Temporary logs in engine.c and bgm_player.c (state, load, F5, hash of the first 0x400 bytes of dataBGM[1], read_rom). All removed.
- Evidence: trd_05 plays SONG_KOOPA_BROS_INTERLUDE (song id 81, file size 0x4e0) on player B (settings.c FadeInMusic 1). The tour leaves the map without the script that stops it. The hash of the B file is 255ed113 at init and a0dadaec after nok_01 loads. nok_01 radio.c calls snd_load_ambient(AMBIENT_RADIO). `au_ambient_load` reads the MSEQ files into `dataMSEQ[1]`. `dataMSEQ[1]` and `dataBGM[1]` are the same address (engine.c:178 and 180, decomp layout). So the song of player B is overwritten and its notes run without an instrument.
- Not a race, not zero fill, not a bad swap. The decomp layout is N64 identical. On the N64 the game stops player B before nok_01. The tour does not.
- Fix: 3ds/src/pm_warp.c `tour_step` calls `bgm_set_song(1, AU_SONG_NONE, 0, 250, VOL_LEVEL_FULL)` before each tour map change (harness only, no guard in audio code).
- Check: same chunk, tour done, no audio fault. `warp=nok_01:0`, yard, toad: rc 0. Not checked: a real play path from trd_05 into nok_01 (not reachable), other maps that leave a song on player B, hardware, the full tour. Other faults in the chunk: trd_06 falling.c:148 (InitializeFallingPartner), partners.c:1136 (partner_walking_enable).

## F01 trd_06 and partner_walking_enable (lane B, 2026-10-18)
- Faults: trd_06 `falling.c:148` (`InitializeFallingPartner`, `get_npc_unsafe(NPC_PARTNER)` is NULL) and `partners.c:1136` (`partner_walking_enable`, partner NULL).
- Method: narrow warps with -Turbo -Smoke -FaultStop. `warp=trd_06:0` (new file, no partner): rc 2 at falling.c:148. `warp=trd_06:0 partner=1`: rc 0. Chunk trd_05 to nok_04 (39 maps, tour=@tour.txt): `partner=0` rc 2 at falling.c:148, `partner=1` rc 0, tour done, no unmapped access.
- Cause: the tour starts from a new file, curPartner is PARTNER_NONE, so no partner NPC exists. The trd_06 scripts and the partner code need the partner NPC. The real game has a partner there (trd_06 comes after the partners are earned, the N64 code dereferences the NPC the same way). Tour-only fault, same class as kmr_23 and nok_01.
- Fix: 3ds/src/pm_warp.c, a tour uses partner 1 (Goombario) unless `partner=` is given. No guard in game code.
- Check: same chunk without a partner switch: rc 0, tour done. yard rc 0, toad rc 0. Not checked: the real exit and entry path from trd_05 into trd_06 with game scripts (story state not reachable by warp), partners other than Goombario, the full tour, hardware.


## Trimmed 2026-10-18 (docs/PARALLEL_PLAN.md)

This file was obsolete and was removed. Full text follows.

# Parallel plan (written 2026-10-18, read-only study, nothing was run)

Sources read: ORCHESTRATOR.md, HANDOFF.md, SLICES.md, INDEX.md, run_azahar.ps1, build.sh, go.ps1, qt-config.ini, the Azahar folder. Facts marked UNVERIFIED were not tested. X02 owns Azahar and the build now.

## Part A. Can several Azahar instances run?

Answer: YES in principle. Each instance needs its own user directory. The repo does not do this today. UNVERIFIED: no run was made.

### A1. Separate user directory per instance
| Item | Finding |
|---|---|
| Portable mode | Azahar (a Citra fork) uses a folder named `user` next to azahar.exe as its user directory, if the folder exists. This is Citra behaviour. UNVERIFIED for the installed version (first step of the setup tests it). The repo has no portable folder now. `C:\Program Files\Azahar` has no `user` folder. |
| Command line option | Not needed and not allowed. The rule says azahar.exe starts with only the .3dsx path. Portable mode needs no option. |
| Per-instance copy | Copy `C:\Program Files\Azahar` (178 MB) to `C:\azahar-lanes\<lane>\`. Add `user\config\qt-config.ini` (copy of the real one, edited), `user\nand` (177 MB copy, first boot needs it), `user\sysdata` (4 KB), `user\sdmc\3ds\PaperMario\papermario.z64` (40 MB, use a hard link: `mklink /H`), `user\shaders` (11 MB, optional, avoids a cold shader cache). Do NOT copy the 4.5 GB real sdmc. About 430 MB per lane. |
| Config keys to change per lane | `gdbstub_port` (24689 is fixed in the script), `nand_directory` and `sdmc_directory` (set to the lane paths, or leave them to portable mode), `use_custom_storage`. |

### A2. Shared paths in a run
| Path or resource | Where it is used | Shared? | Fix per lane |
|---|---|---|---|
| `%APPDATA%\Azahar\config\qt-config.ini` | run_azahar.ps1 `$cfg` (Set-Turbo, Set-GdbStub) | Shared. Overlap races (T02 cause 3). | Lane config in `user\config`. |
| `%APPDATA%\Azahar\sdmc\3ds\PaperMario` | `$sd`: 3dsx copy, debug3ds.txt, log.txt, shots, save.flash, crash.txt, ROM | Shared. One game log, one debug3ds.txt. | `$sd` under the lane `user\sdmc`. The game reads `sdmc:/` so it follows the lane. |
| `%APPDATA%\Azahar\log\azahar_log.txt` | `$emuLog`: FaultStop, exception, last_unmapped | Shared. Run start deletes it. | Lane `user\log`. |
| `build3ds/` and `pm_3ds.3dsx` | build.sh output, run_azahar.ps1 copies the 3dsx, last_log.txt, last_unmapped.txt, last_exception.txt, last_shot.png, last_gdb.txt, gdb_cmds.txt, qt-config.turbo-backup | Shared inside one tree. | A git worktree per lane gives each lane its own `build3ds/`. |
| Docker volume `pm3ds-work` (`/work/src`, `/work/build`) | build.sh rsync with `--delete`, then cmake. gdb and addr2line in run_azahar.ps1 read `/work/build/pm_3ds.elf` | Shared. Two builds destroy each other. | Volume per lane: `pm3ds-work-<lane>`. Change build.sh and the two `docker run` calls in run_azahar.ps1 to use `$PM_LANE`. |
| `build.sh` last step | copies the 3dsx to the real `%APPDATA%` sdmc | Shared. | Skip it when `PM_LANE` is set. run_azahar.ps1 copies the 3dsx anyway. |
| `debug3ds.txt` | written to `$sd`, removed in finally | Shared. | Follows `$sd`. |
| `3ds/logs/` | git-ignored, agent logs and traces | Per worktree if the lane has its own tree. | Use the lane tree. |
| `3ds/golden/*.png`, `3ds/tools/golden.py` | tracked references, calls run_azahar.ps1 | Tracked files. Only one lane may run `--update`. | golden.py inherits `PM_LANE` from the environment. |
| Capture by window | `-Capture`: `$p.MainWindowHandle` and PrintWindow. The dialog check uses the window list of the PID. | Per PID, not by title. | None. |
| Game shots (`shots` switch) | PPM files in `$sd\shots`, copied to `build3ds\shots` | Follows `$sd`. | None. |
| GDB port 24689 | Set-GdbStub | Shared. | Port per lane in the lane config. Pass it to gdb (`target remote winhost:<port>`). Rarely used. |
| Live log TCP 17492 | `pm_log.c` binds this port inside the game. Azahar guest sockets use the host network. | A second instance gets `bind 17492 failed`. The game logs the line and goes on. UNVERIFIED. | None needed. Ignore the line. |
| `PROGRESS.log`, `HANDOFF.md`, `SLICES.md`, `docs/*` | tracked | Merge conflicts only. | Rule: a lane appends only. The orchestrator merges. |
| Kill logic | Kill-Azahar uses the PID. | Safe already. | None. |

### A3. Does shot capture depend on one window or on focus?
- No. `-Capture` uses the PID of the launched instance, `PrintWindow` (works without focus) and `GetWindowRect`. The game shots come from the game as files. The script never uses a window title.
- Risk: PrintWindow of a minimised window gives a black frame. Keep the windows open and not minimised.

### A4. What N lanes need
1. A git worktree per lane: `git worktree add C:\pm-lanes\<lane> -b lane/<lane> main`. The existing `3ds/logs/wt_battle` worktree is old (detached). Remove it first or ignore it.
2. The extracted assets are git-ignored (`ver/us/build`, `src/vtx`, `assets`, `include/message_ids.h`). A new worktree does not have them. Copy or junction them from the main tree (`mklink /J`). This is the largest hidden cost. Check it first.
3. A Docker volume per lane (`pm3ds-work-<lane>`). The first build in a lane is a full build. The ninja build uses all 32 threads, so parallel builds slow each other.
4. An Azahar folder copy per lane with its own config, sdmc, nand, log (A1).
5. A lane variable `PM_LANE` read by build.sh, run_azahar.ps1, golden.py. Empty means the old behaviour.
6. Merge back: each lane commits on its branch. The orchestrator merges into main one lane at a time (`git merge --no-ff lane/<lane>`), builds once and runs the smoke check on `yard` and `toad`. The shared files (HANDOFF.md, PROGRESS.log, SLICES.md) conflict. Lane agents append only. The orchestrator fixes conflicts.
7. Generated tracked files (maps.cmake, map_syms.lds, stub_include) conflict if two lanes regenerate them. Only the S30b lane may run generators.

### A5. Fallback: one lock for the Azahar run
| | Per-lane Azahar (A4) | Lock only |
|---|---|---|
| Parallel edit and build | Yes, if build dirs and volumes are separate | Yes, only with a worktree and a volume per lane. Without them builds still clash. |
| Parallel Azahar run | Yes | No. Agents queue. A run is 16 to 60 s. |
| Change size | Large: 3 scripts, a setup script, an unverified portable mode | Small: a named mutex in run_azahar.ps1 (about 10 lines), part of T02 |
| Risk | Portable mode may not work. The lane config can drift. | Low |

Recommendation: do the lock FIRST (T02, small, safe). Add worktree, volume and build dir per lane next, because builds are the long step. Add the per-lane Azahar copy only if the queue for Azahar becomes the bottleneck. Most slices run Azahar for about 1 to 3 minutes in total, so the queue is short.

### A6. Cost of one instance
- Not measured in the repo or the logs. The logs only give guest numbers (audio 3 ms per tick, guest memory 98 MB offered). An Azahar run uses CPU for the emulator thread and the render thread. At `-Turbo` the speed limit is 1000 percent, so it takes as much CPU as it can get.
- Estimate, UNVERIFIED: 2 to 4 cores and 1 to 2 GB RAM per instance (process working set, window, shader cache). Measure with `Get-Process azahar | select WS,CPU` during one run.
- The PC: Ryzen 9 3950X, 16 cores, 32 threads, 64 GB RAM. Three or four instances fit. Docker builds also use all threads. Turbo runs from several lanes at the same time may change timing but not frame-driven results. No performance numbers from a parallel run are valid.

### A7. Changes in run_azahar.ps1 (relates to row T02)
| # | Change | T02 link |
|---|---|---|
| 1 | Named mutex `Global\pm_azahar_cfg` around any read or write of a config. Release in `finally`. | T02 named mutex |
| 2 | Resolve paths from `$env:PM_LANE`. Empty: the paths of today. Set: `$exe`, `$cfg`, `$sd`, `$emuLog` under `C:\azahar-lanes\<lane>`. | T02 per-run config |
| 3 | Use the lane config only. The real `qt-config.ini` is never opened when a lane is set. Later also for the default run (copy the real one to a temp user dir). | T02 core goal |
| 4 | Wrap the whole body in try/finally. Move Set-Turbo and Set-GdbStub inside the `try`. | T02 cause 1 |
| 5 | Remove the unconditional `Set-Turbo $false` at line 109 for a lane run (it deletes a running lane's backup). | T02 cause 3 |
| 6 | Fail loudly when a config key is missing (throw, do not skip). | T02 cause 5 |
| 7 | Docker calls (gdb, addr2line): volume `pm3ds-work-$PM_LANE`. Gdb port from the lane config. | new |
| 8 | Optional: when a lane has no config, copy from the real one and set `use_gdbstub`, `frame_limit`, `use_vsync` once. | new |
| 9 | While X02 runs, do not edit the file in place. Edit it after X02 ends, or write `3ds/run_azahar_lane.ps1` and merge later. | safety |

## Part B. Lookahead over open rows

Shelved: V09 intro tints and nok_01 (first V09 row). Optional (unconfirmed visual faults): V09 kmr_22, V10, V11.
Model tier by the MODEL RULE: opus only for plans, sonnet for code, shared code, graphics, memory, audio, haiku for mechanical work.

| Row | Files or subsystem (likely) | Azahar runs | User or hardware | Tier |
|---|---|---|---|---|
| S28 | no code. Hardware run, boot line, audio cost, fps | Not Azahar. O3DS and N3DS | YES (O3DS on, dspfirm.cdc) | sonnet (judgment on numbers) |
| S30b | `3ds/maps_skip.txt`, `maps.cmake`, `map_syms.lds`, `CMakeLists.txt`, `map_restore.*`, `src/world/area_*`, memory layout | yes, tours | YES, answer to Q7 | sonnet |
| V07 | `src/entity.c`, `pm_gbi.c`, `testitem` in `pm_warp.c` | yes | YES, exact scene or hardware | sonnet |
| V06 | `pm_gbi.c`, `pm_gpu.c` (fog, blender) | yes | YES, user must confirm | sonnet |
| X02 | `pm_gbi.c`, `pm_gpu.c`, `pm_tex.c` (render to image, 32 KB VRAM, 3 KB code) | yes (running now) | no | sonnet |
| S31 | `game_stubs.c`, `pm_later.c`, crash handler in `pm_main.c`, `crashtest*` switches | yes (`crashtest_call`) | partly: real crash screen needs hardware | sonnet (no-stub judgment) |
| F01 | kmr_23 `src/curtains.c`, tst_12 cut map, nok_01 `src/audio/core/engine.c` (the nok_01 part overlaps shelved V09), pause, save, zones | yes (tour) | no | sonnet |
| F03 | `src/model.c:4497`, depth read back in `pm_gpu.c` and `pm_gbi.c` | yes (`warp=hos_05`) | no | sonnet |
| T02 | `3ds/run_azahar.ps1`, `build.sh`, `golden.py` | one check run at the end | no | sonnet (script plus config safety) |
| F08 | `pm_gbi.c`, `pm_comb.c`, `pm_gpu.c`, shader `pm.v.pica` | yes, fog map | no | sonnet |
| PO2 | `pm_gbi.c` (vertex transform at G_VTX), `pm_phase.c` | yes (phase timer) | YES, answer in QUESTIONS.md | sonnet |
| PO3 | plan in `docs/`. Later `pm_gbi.c`, `pm_gpu.c`, `pm.v.pica` | plan: no. Later: yes | no | opus for the plan, sonnet later |
| AU1 | plan first. Later `pm_audio.c`, `pm_rsp_audio.c`, `pm_os.c`, `src/audio/core/engine.c` | later yes | YES for the O3DS cost number | opus plan, sonnet later |
| MA1 | many files (libm calls, divides). Replay hash in `pm_warp.c` | yes (replay check) | YES, approval for game-logic math | haiku for the listing, sonnet for changes |
| F06 | plan only, after S28 | no | needs S28 | opus |
| F07 | plan only, after S28 | no | needs S28 | opus |
| S18 | check run, move row to `docs/SLICES_DONE.md` | yes, one run | no | haiku |
| S19c | check one attack battle, move row | yes, one run | no | haiku |
| V09 kmr_22 (optional) | card texture in `src/world/area_kmr/kmr_22`, `pm_tex.c` | yes | no, but unconfirmed fault | sonnet |
| V10 (optional) | `draw_box` pattern path `box_pattern_rect`, `pm_gbi.c` | yes | no, unconfirmed | sonnet |
| V11 (optional) | kmr_05 collision or map data | yes | no, unconfirmed | sonnet |

### Batches without file conflicts
Conflict groups: GFX = `pm_gbi.c`, `pm_gpu.c`, `pm_tex.c`, `pm_comb.c`. AUDIO = `pm_audio.c`, `pm_rsp_audio.c`, `pm_os.c`, `engine.c`. MAPS = `maps.cmake`, `CMakeLists.txt`, generated files. TOOLS = `run_azahar.ps1`, `build.sh`, `golden.py`.

| Batch | Rows | Why they do not conflict | Needs |
|---|---|---|---|
| 0 (beside X02, now) | T02 (TOOLS, edit in a copy or after X02), PO3 plan (docs only), AU1 plan (docs only), MA1 listing (docs only) | No GFX file. No build. No Azahar run. Plans write one new file each in `docs/`. | no build, no Azahar |
| 1 (after X02 ends and lanes exist) | F01 (curtains, engine.c, tst_12), S31 (stubs, crash handler), S18 and S19c (docs plus one run each) | Different files. F01 touches AUDIO `engine.c`, so no AU1 coding in this batch. | one lane each |
| 2 (GFX chain, one lane at a time) | F03, then F08, then PO2 (needs the user answer), then optional V10, V09 kmr_22 | All touch `pm_gbi.c` or `pm_gpu.c`. They would conflict. | one lane |
| 3 (AUDIO) | AU1 code after its plan, after F01 nok_01 | Shares `engine.c` with F01. | S28 number helps |
| 4 (sequential, alone) | S30b | Changes memory layout and generated files. All other measurements and golden frames move. Needs Q7. | user |
| 5 (user or hardware) | S28, V06, V07 | Cannot run without the user. S28 first because it settles Q2 and Q7. | user |
| 6 (late) | MA1 changes, PO3 code, F06, F07 | Touch many files. MA1 needs replay hashes and approval. F06 and F07 plan after S28. | S28, user |

Rows that must stay sequential: S28 first in the hardware path. S30b after Q7 and alone. The GFX chain (X02, F03, F08, PO2, PO3 code) because of shared files. MA1 changes and PO3 code after the GFX chain. F06 and F07 after S28.

### What can start beside X02 now
- T02 (write the changes in `3ds/run_azahar_lane.ps1` or wait: X02 runs `run_azahar.ps1`). No Azahar run, no build.
- PO3 plan, AU1 plan (opus, docs only), MA1 listing (haiku, read-only listing into one doc).
- Read-only analysis for F01, F03, S31 (cause notes only, no build, no code edit).
- NOT now: anything that builds (the shared `pm3ds-work` volume and `build3ds/` belong to X02) or runs Azahar, and any edit in the GFX files.

## Recommended setup
1. Phase 1 (now): the lock. T02 adds the named mutex, try/finally, and loud key checks. No lanes yet.
2. Phase 2: worktree, Docker volume and build directory per lane (2 or 3 lanes). Lanes share one Azahar through the mutex. Runs queue.
3. Phase 3 (only if the queue hurts): per-lane Azahar folder with portable `user` directory (A1).
4. Merge: orchestrator merges one lane at a time, smoke check `yard` and `toad` after each merge.

## First steps for the build agent (haiku or sonnet, after X02 ends)
1. Read ORCHESTRATOR.md, this file. Check `git status`. Do not touch X02 files.
2. Test portable mode on a throw-away copy: `robocopy "C:\Program Files\Azahar" C:\azahar-lanes\t1 /E`, make `C:\azahar-lanes\t1\user`, copy `%APPDATA%\Azahar\config`, `nand`, `sysdata` into it, hard link the ROM into `user\sdmc\3ds\PaperMario`. Start `C:\azahar-lanes\t1\azahar.exe` with ONLY the 3dsx path (use a copy of the script with the lane paths). Check that `user\log\azahar_log.txt` and `user\sdmc\...\log.txt` appear and that `%APPDATA%\Azahar\log\azahar_log.txt` does not change. Write the result (works or not) in PROGRESS.log. If not, stop at Phase 2.
3. Add `PM_LANE` to `build.sh` (volume `pm3ds-work-$PM_LANE`, no copy to the real sdmc when set). Add the same volume name to the two `docker run` calls and the lane paths in `run_azahar.ps1` (A7 items 1 to 7).
4. Create lane worktrees: `git worktree add C:\pm-lanes\L1 -b lane/L1 main` and `L2`. Junction the git-ignored assets (`ver\us\build`, `src\vtx`, `assets`, `include\message_ids.h`) from the main tree. Check `include/message_ids.h` is regenerated or copied.
5. In each lane: `set PM_LANE=L1`, run `sh 3ds/build.sh`, then `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "fast preset=yard"`. Both lanes in parallel. Both must exit 0 and keep their own `build3ds/last_log.txt`.
6. Measure CPU and RAM during the parallel run (`Get-Process azahar | select Id,WS,CPU`). Write the numbers in this file (Part A6).
7. Write the lane rules in ORCHESTRATOR.md (one paragraph): lanes append only to shared docs, the orchestrator merges, only the S30b lane runs generators.
8. Commit locally in plain English. No push. No attribution.


## Trimmed 2026-10-18 (HANDOFF.md)

Full text of HANDOFF.md before the trim. Rules that 3ds/AGENT_RULES.md holds are kept here.

```
# Handoff (short, read in full by every agent)

RULE: Each slice adds at most 3 lines to this file. Put detail in docs/HISTORY.md (append at the end).
Pointers: docs/HISTORY.md (all old slice results), docs/SLICES_DONE.md (done rows), docs/INDEX.md (where things are), 3ds/ORCHESTRATOR.md (workflow, reading rules, model rule), 3ds/SLICES.md (open queue), 3ds/QUESTIONS.md (open questions).

## RULE: NO STUBS WITHOUT ASKING
- Do not add a stub. Do not write a stand-in for decomp code or data. Build the real decomp file.
- A port-layer shim is allowed only for real N64 OS or hardware functions that hold no game logic.
- If a stub looks like the only option, stop. Write `BLOCKED: <reason>` here. Name the unported system that forces it. Ask the user. Wait.
- Top priority: remove existing stubs (list at the end of this file).
- Fix root causes. Never hide a fault with a guard (the `spriteIdx < 0` guard of S22 was removed).

## Rules the project depends on (Do not)
- Do not push to git. Add no Claude or Anthropic attribution to commits or PRs. Commit locally, plain English, only your own files.
- Do not run `git checkout` on tracked files. Do not commit logs, traces or shots. They go under `3ds/logs/` (git-ignored). Only `3ds/PROGRESS.log` is tracked.
- Do not start `azahar.exe` with any argument except the .3dsx path (a modal dialog blocks the run). Use `3ds/run_azahar.ps1`, not `go.ps1`. Only one Azahar window can run.
- Do not leave `fast` in `debug3ds.txt` (it skips the logos). The build skips when the 3dsx is newer than all sources: touch a source file after a CMakeLists.txt change.
- Testing: feature tests use the warp (`preset=` or `warp=<map>:<entry>`) with `-Turbo -Smoke -FaultStop` and a fixed length. Never wait open ended. Add `padscript=` where input is needed.
- Lighter check rule (2026-10): after a fix run (1) the smoke check on presets `yard` and `toad` with `-Turbo -Smoke` (about 16 s each), (2) shots of only the scenes the change touches. No 4-scene A/B and no full tour, unless the slice touches the map change path, boot, title, file select or the intro. Then run the full normal chain `-Debug "fast autoa freshsave" -Stall 40`. A by-eye A/B with a `nocull` style switch stays allowed to prove that the symptom is fixed.
- NARROW TESTS (2026-10-18): test only what the change touches. Use one target scene or map, a fixed short length and one text check (log line, draw count or one pixel diff). Do not build large test setups. Do not skip a fault the narrow test finds: fix it. Say in the result which areas were not tested. Build or run checks per slice must be short.
- Tours: iterate on the failing set. Run the full tour once at the end of a slice. Say in the result which kind of check you used.
- Threading and sync use libctru natives (threadCreate, LightLock, LightEvent, LightSemaphore, svc handles). No hand-rolled locks. No sleep loops. The custom yield in `pm_os.c` (`pm_au_thread_yield`) and `pm_audio.c` is to be reviewed in the audio slice (AU1).
- Memory rule: the estimated O3DS spare is about 4.7 MB with all 421 maps (docs/MEMORY_BUDGET.md). Add at most about 10 KB of code or data per slice. Exception: S30b may add about 2 MB of static image (user approved). Write the size change (3dsx bytes) in the slice result.
- Screenshot rule: use screenshots only when really needed. Prefer text evidence (draw logs, depth view, checksums, pixel diffs, summary scripts). When a picture is needed, make ONE contact sheet.
- Log rule: each agent appends one line per step to `3ds/PROGRESS.log` (`HH:MM ID text`). Read logs with the last 40 lines only. Game log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt`. The log is non-blocking (L01). Never commit it.
- Notes and replies use ASD-STE100 English: short sentences, one instruction each, active voice, no semicolons, no praise.
- Reference ports in `reference/` (paperboat first, soh-3ds and sm64-3ds for 3DS GPU, audio, memory): read to learn the method. Never copy a hand-written stand-in.

## Current state (2026-10-17)
- Branch `main`. Tag `s19-pre-merge` keeps the old tip of branch `s19-full-battle` (merged by M03). No push.
- The game boots in Azahar: logos, title, file select, intro (hos_05), kmr_20, osr_00, mac_00, mac_01, kkj_00 (the real chain). The real battle tree is merged. Real encounters, level up, flee, game over and the title demo loop (17 scenes, repeats after MS01) run.
- All 421 maps are linked (S30b). 3ds/maps_skip.txt is empty. 6 new maps warped in Azahar, the other 148 not.
- Image about 16.98 MB. 3dsx about 10.9 MB. Estimated O3DS spare about 518 KB. This is an estimate from Azahar numbers (64 MB limit assumed, 20 MB linear pool, N64 RAM block 4 MB).
- The O3DS and the N3DS were never run. S28 is the first hardware run. All speed numbers are Azahar numbers and are not valid for the O3DS.
- The game runs at exactly 30 fps (PC01: 29.91 steps per second, 2 vblanks per step). 60 fps (F06) and widescreen (F07) come later.
- Fixed in the platform layer (details in docs/HISTORY.md): combiner (S25), transparency (S26), file select (S27), screen transitions (X01), brown wall by culling (V02), menu flip (V05), HUD (H04), pause (D03), V06 blender mix.
- Port layer files and tools: see docs/INDEX.md.
- Golden frames (G01g): `python 3ds/tools/golden.py [--update] [scene..]` runs 10 scenes (title, fileselect, intro, toad, yard, battle, pause1 to 4), prints a table of pixel differences against `3ds/golden/*.png`. About 25 s per scene. Run it after a graphics change. Update the references only after a change that you checked by eye.
- H03: linear pool 14 MB (was 20 MB), boot line `mem split H03`. A full texture cache skips the draw (no white quad). Detail in docs/HISTORY.md.
- V08 done: the save block is translucent by design (decomp XLU, prim alpha 128). The port draws it exact. No code change.
- Last commits: V05 (menu flip, scissor), V06 (fog colour mix).

## Debug switches (put in `debug3ds.txt` or `-Debug "..."`). One line per group, details in docs/INDEX.md
- Start: `fast autoa freshsave loadslot=<n>`. Warp: `warp=<map>:<entry> story=<name|n> preset=<name> partner=<n> sp=<n> hp=<n> battle[=<f>:<s>] battlef=<n> demobattle`.
- Run control: `smoke smokeframes=<n> tour=@file tourfx=<n> turbo padscript=<..|@file> padrand padlog taptest`.
- Pictures: `shots shotwin=<f>:<n> shotmode=<m>:<n> shotskip=<n> offdump`.
- Draw A/B and trace: `nocomb nocull nomix nooff noprimz rmoff rmlog drawlog=<f>:<n> ddonly=<lo>:<hi> ddverb depthview=<1|2|3> xtrace=<f>:<n> prevtest nofallback`.
- Checks: `ptrcheck ptrtest romstrict romtrace romfull dataprobe assets scripts faulttest crashtest crashtest_call crashtest_audio crashtest_audio_call`.
- Timing and log: `phase phasecsv phasegbi stats audiostat pacestat logstat lognull logflood noaudio`.
- Presets: intro yard toad toad_east toad_west osr kmr02.
- Run script `3ds/run_azahar.ps1`: `-Until -Timeout -Stall -Turbo -TurboFactor -TurboLimit -Smoke -FaultStop -Capture -Gdb -GdbScript -Interactive`. Exit 0 ok, 1 failed, 2 unmapped access or crash. Main command: `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "fast preset=toad"`.
- Build: `3ds/build.sh` and `3ds/go.ps1` (build glue). Hardware: skills `push-o`, `push-n`, `pull-o`, `pull-n`, `push-a`.

## Open issues (one line each, row id in 3ds/SLICES.md)
- V06 (queued, after X02): user confirmed the ground gap in buildings is correct. Real fault: the outside must be black in a building room, the port draws the normal background.
- V07 (SHELVED by user 2026-10-18): items flicker. Not reproduced in Azahar. The user reports it again if seen. Do not work on it.
- V09 (two rows): intro tints changed by S25 (and nok_01 tour fault). kmr_22 card shows a rainbow bar.
- V10: battle Strategies box and level-up box draw as a checkerboard.
- V09 intro tints and nok_01 shelved by the user, partial edits on branch shelf/v09-nok01.
- X02 done (B0 of docs/plans/buffer_bridge.md): darkness runs the real decomp function with 64x64 slots keyed by (address, pixel size). kzn_08 untested (not linked). Detail in docs/HISTORY.md.
- F03: `is_point_visible` reads a NULL z-buffer (hos_05 lamp, `tex ptr 0000fd7e`).
- F08: fog partly done by V06 (blend and fog colour mix). The per-vertex fog factor is open.
- S30b done: all 421 maps linked, 3dsx 13,011,156 B (+2.09 MB), GotoMap guard removed. Spare about 4.7 MB by estimate (docs/MEMORY_BUDGET.md).
- S31: crash screen and debug output check (Q5).
- T02 Phase 1 done: run_azahar.ps1 holds mutex `Global\pm_azahar_run` (-LockWait s, exit 3 on timeout), restores qt-config.ini byte for byte in finally, and recovers after a hard kill. Lanes (Phase 2, 3) are open in docs/PARALLEL_PLAN.md.
- F01 tst_12: cause measured (LVar5 = -17, LVarF = -11, endless Loop, N64-identical decomp defect of a cut map, no port fix). Skip tst_12 in tours. Detail in docs/HISTORY.md.
- Tour faults: kmr_23, nok_01, trd_06 (falling.c:148) and partners.c:1136 fixed by F01 (lane B, tour-only: a tour now uses partner 1). Detail in docs/HISTORY.md.
- Not verified by eye: partner moves, items and star power in battle. Pause with items or a partner. Hardware.
- F09 language select: future feature, not in the current batch.
- PO3 RISK (RSP to PICA port): effects that read back N64 memory images (X02, F03, water_splash, grabs) may need rework. Check each at every PO3 slice (docs/plans/PO3.md R7).
- Buffer bridge plan: docs/plans/buffer_bridge.md (B0 is X02)
- S28 only after all Azahar-verifiable rows are done (user decision 2026-10-18).

## Stand-ins still open (checked against docs/HISTORY.md, S20 audit A, B, C, D and later results)
- Placeholder aliases (class D, code only, no data read): 47 `--defsym X=gPartnerStatus` in `3ds/CMakeLists.txt` (`world_partner_*`, `world_use_item_*`, `pause_*`, `filemenu_*`). Code overlays linked in place. Verified in P01 and R01.
- `((u8*)1)` ROM ranges for linked code overlays in `3ds/stub_include/ld_addrs.h`, `effect_ld_addrs.h`, `battle_ld_addrs.h`. Code only.
- GotoMap guard: removed by S30b.
- Empty shims: `crash_screen_init`, `is_debug_init`, `load_obfuscation_shims` (`3ds/src/game_stubs.c`), `crash_screen_set_draw_info` (`3ds/src/pm_later.c`). User decision Q5: obfuscation shims stay empty. Row S31 checks the crash screen.
- Storage at real N64 addresses (class C, no game logic): `gBackgroundImage` and `pm_world_entity_heap` (`3ds/src/pm_kmr_stubs.c`).
- Gone (do not look for them): `area_table.c`, `pm_title.c`, `pm_evt_stub.c`, `pm_actor_link.c`, `pm_shadow.c`, `pm_fx.c`, battle stand-ins, effect graphics zeros (A17), shadow headers (B3 to B6), the `spriteIdx` guard, `note_door`.
- Not stand-ins but still approximate: fixed light direction for `G_LIGHTING` vertices (`pm_gbi.c`), no fog table (F08), no texture generation, LOD or `G_BRANCH_Z`.
- Lane B (worktree ..\papermario-3ds-port-laneB, branch lane-b, `PM_LANE=laneB`): see docs/LANES.md. Build parallel, run one at a time.

- S18 and S19c closed (lane B copy run via PM_LANE=s18chk scratch lane). poly_scan.py thresholds too loose (flags natural dirt): tune before reuse.

```

## Trimmed 2026-10-18 (3ds/SLICES.md)

Full text of 3ds/SLICES.md before the trim. Long rows moved to docs/plans/<id>.md (H05, V06, V07, V09, V12, PO2, PO3_row, F06, F07, F09, F10, MA1, AU1_row, B1, B2, B3, B5, B6, B7). Note line moved here: "Note: graphics rows: one at a time, X02 first."

```
# Slice queue (open rows only, plan for the next round)

Format: `[ ] id | task | done check`. `[~]` = partly done. Done rows are in docs/SLICES_DONE.md. The full original text of each row below is in docs/HISTORY.md (section "Original text of the open slice rows"): grep for the id.
Rule: build the real decomp file. Never write a stub. If a stub looks like the only option, write `BLOCKED: <reason>` in HANDOFF.md and stop.
Model tier per row: see the MODEL RULE in 3ds/ORCHESTRATOR.md.

[ ] H05 | HARDWARE-READINESS AUDIT (user decision 2026-10-18). Runs after every other Azahar-verifiable row is done and lane-b is merged, BEFORE S28. Goal: high confidence that the first hardware run works. Part A, game faults: full tour with -FaultStop, long chains (title, intro, chapter paths, battles, pause, save and load), every remaining in-game fault is listed and flagged for a fix (a fault that happens in the game is a row, not a note). Part B, Azahar versus hardware (read code, no guess): memory limit and the 64 MB assumption, linear and VRAM use, cache flush and invalidate before GPU or DMA use, buffer alignment, texture size and format limits, GPU command buffer and vertex limits, thread stacks, core use on the O3DS (app core and the syscore time limit), ARM11 unaligned access and float behaviour, N64 big-endian data handling, SD and file speed, 3dsx load size, watchdog or timeout paths, DSP firmware, log and crash path. Use fresh agents per area (one read-only agent per area, sonnet, plans and findings only), compare with reference/ ports, write docs/plans/H05_findings.md | a findings table (area, risk, evidence, fix row or accepted), every in-game fault has a fix row, the user reviews the table before S28
[ ] S28 | GATE (user decision 2026-10-18): start only when every other open row that Azahar can verify is done or shelved, H05 findings are fixed or accepted by the user, lane-b is merged into main, and one full tour and one golden run are clean. Prep: docs/plans/S28_prep.md. Hardware run on the O3DS (192.168.1.168) and the N3DS: boot line (real memory limit), audio cost, FPS. Needs the user: O3DS on, dspfirm.cdc on the SD card | numbers in HANDOFF.md, settles Q2 and Q7
[ ] V07 | SHELVED (user decision 2026-10-18, the user reports it again if seen, do not work on it) Item entities flicker when the camera moves. NOT REPRODUCED in Azahar (see docs/HISTORY.md V07): item draws and pixels are stable in 8 maps with a moving camera, `nocull` A/B identical. Needs the exact scene from the user (QUESTIONS.md) or a hardware run | cause named with evidence, fixed
[ ] V06 | User answer 2026-10-18: the ground vanishing inside Toad Town buildings is CORRECT. The real fault: when Mario is in a building the outside must be BLACK, but the port shows the normal background there. Find in the decomp how the outside is hidden in building rooms (shroud, background, CreateMapRoom) and make the port draw black outside. Check mac_00 houses, kmr_20 and kmr_02 doors. Note: it touches the graphics layer, so it waits for X02 to finish. | outside of a building room is black in a shot, ground and other scenes unchanged
[ ] V09 | SHELVED (user decision 2026-10-18, potential future problem, not confirmed by the user, take up only if the user notices it) (intro tints) S25 changed the hos_05 intro tints (shots 70 to 77) to dark or red. Check against the decomp and paperboat, A/B with `nocomb`. Also nok_01 `au_syn_begin_audio_frame` unmapped access (tour only). Partial nok_01 audio edits (src/audio/bgm_player.c, src/audio/core/engine.c) are saved on local branch shelf/v09-nok01 (commit made from a stash). Not merged. Not verified. | verdict per shot, nok_01 cause
[ ] V10 | Battle Strategies submenu and level-up box draw as a white and brown checkerboard (draw_box pattern path `box_pattern_rect` is the suspect) | Strategies list readable in a shot
[ ] V12 | USER-CONFIRMED 2026-10-18. The damage star in battle is wrong: it is zoomed in and clipped. Find the effect (damage star, hit number burst), compare size and clip with the decomp and render_gaps.md (scissor, texrect, scale). Graphics row: one at a time AND check the other battle effects for the same fault (the user asks: find the common cause, then test every effect that draws with the same path: hit bursts, status icons, number pops, attack effects, partner and item effects; list the effects tested and the ones not tested) | one battle hit shows the star at normal size, whole, in one shot or a pixel check, and a table of the other effects tested shows none zoomed or clipped
[ ] F03 | = slice B4 of the buffer bridge plan. `is_point_visible` (src/model.c:4497) reads a NULL z-buffer (log `tex ptr 0000fd7e`, hos_05 lamp). Give the platform layer a depth read back for one pixel | `DepthCopyBuffer` holds real depth, no `tex ptr` line
[ ] F08 | Fog remainder. V06 added the blend and fog colour mix. G_FOG per-vertex factor and the fog table are not done (about 1.3M draws in the tour use fog) | one fog map renders fog, maps without fog unchanged
[ ] PO2 | OPT: code work starts only after S28 (user decision 2026-10-18). Plans are done: docs/plans/PO3.md. Performance: transform each vertex once at G_VTX (CPU only). Needs the user answer (QUESTIONS.md) | phase timer shows less transform time, same pictures
[ ] PO3 | OPT: code work starts only after S28 (user decision 2026-10-18). Plans are done: docs/plans/PO3.md. Performance: GPU vertex transform (raw Vtx attributes, index list, matrix in the vertex shader). Plan first (opus allowed for the plan only) | plan in docs/, then a working A/B. RISK (RSP to PICA port): effects that render into or read back N64 memory images (z-buffer, colour image, framebuffer grabs: darkness X02, depth readback F03, water_splash and similar current-frame grabs, see docs/plans/render_gaps.md and docs/plans/buffer_bridge.md when it exists) may break or need rework when RSP vertex work moves to the PICA GPU (PO3, PO2). Check each such case again at every PO3 slice (A/B with golden frames and the render_gaps list)
[ ] AU1 | OPT: code work starts only after S28 (user decision 2026-10-18). Plans are done: docs/plans/AU1.md. Performance: audio on core 1 or the DSP (ndsp), with libctru natives. Plan first | audio cost per tick measured on the O3DS
[ ] MA1 | OPT: code work starts only after S28 (user decision 2026-10-18). Plans are done: docs/plans/MA1_list.md. Performance: ARM11 math audit (slow divides, sqrt, libm calls) with the replay check (hash of game state per N frames). Game-logic math changes need the replay check and the user approval | same hashes, lower cost
[ ] F06 | LATER, big. 30/60 fps toggle by interpolated extra frames (logic stays at 30 Hz). Plan only after the hardware run | plan in docs/
[ ] F07 | LATER. Widescreen mode (research in docs/HISTORY.md, section "Widescreen and 60 fps research"). Plan after the hardware run | plan in docs/
[ ] F09 | LATER, future feature. Language select. The US build never runs it (only a PAL call site exists, see docs/plans/S18_S19c_commands.md). Decide scope after the hardware run | plan in docs/
[ ] F10 | NICE TO HAVE, last. Fix the endless loop in tst_12 (decomp script D_80240050: Add(LVarF,11) reads float -22.5 as integer, Loop(LVar5) gets -17). Do after everything else, after F06 and F07. tst_12 stays linked and warpable for the future scene selector. Until then the tour list may skip tst_12 as a test setting only | warp=tst_12:0 -Turbo -Smoke -FaultStop exit 0, no flood. Decide: patch the game data or add a documented port rule (needs the user)

Note: graphics rows: one at a time, X02 first.
[ ] B1 | (sonnet) Correctness, no picture change. `img_role()` in pm_gbi.c with table 1.2. Move today's checks behind it. One `pm_zbuf_need()` call site. `imglog` switch. `stats` counters splits, slots, grabs. Add VRAM line to MEMORY_BUDGET.md | golden toad yard battle title pause1 0 diff. `imglog` table for kgr_01, flo_10, mac_01, one battle with water_splash
[ ] B2 | (sonnet) Correctness. Role T2 grab: split T2 from T3 at `G_SETTIMG`, strip `GX_TextureCopy`, upright slot, reuse, `GRAB_MAX`. Step 1: confirm queue order | `warp=flo_10:<entry>` (agent confirms the entry in pm_warp.c), `offdump` of the grab slot shows the scene. Log grabs per frame. golden toad yard battle 0 diff
[ ] B3 | (sonnet) Correctness. Role DEPTH: case 5 fill as depth write, case 6 model as depth write. Step 1 is the log in section 4 | `warp=mac_01:<entry>` crystal ball and flo_10: `drawlog` shows the decal draw passes in the model area only. Golden 4 scenes, each diff explained
[ ] B5 | (sonnet) Correctness. Underwater (case 7): z-buffer as colour goes to a slot (B1), source from B2. Add `G_MODIFYVTX` to the opcode switch | Sushie battle move or `fx_underwater` site (agent finds a warp or preset). `offdump` of the z-buffer slot, log of modified vertices
[ ] B6 | (sonnet) Correctness. Flame scratch I8 (case 8) and ring_blast: chroma key (`G_SETKEYR`, `G_SETKEYGB`) to an alpha test or combiner key. I8 slot from X02 | isk battle stage with the flame effect, `offdump` of the I8 slot, one shot
[ ] B7 | (haiku) after S28, O3DS measure run. Phase times, grabs and splits per frame, VRAM `mem` line, swirl capture time, F03 query time. Table only | `push-o`, `pull-o`, `phase_report.py`

```

## Trimmed 2026-10-18 (docs/INDEX.md)

Removed: the entry `src/vtx` (file does not exist). Changed: the lanes entry pointed to docs/PARALLEL_PLAN.md (archived here).

Before: `- Extracted, git-ignored: `ver/us/build/` (PNGs, `.png.bin`, headers, build.ninja), `src/vtx`, `assets/`.`
Before: `- Lanes (parallel build worktree, `PM_LANE`, volume, merge, removal, cost): `docs/LANES.md`. Plan: `docs/PARALLEL_PLAN.md`.`

## Trimmed 2026-10-18 (3ds/ORCHESTRATOR.md)

Stale text removed or updated. Before:

- Q6: link all maps. Measure first. 271 of 421 are linked, the rest does not fit (Q7).
- SMALL HANDOFF.md: at most about 150 lines. Each slice adds at most 3 lines to it. Detail goes to docs/HISTORY.md.

After: Q6 says all 421 maps are linked (S30b). HANDOFF.md limit is 55 lines.

## Trimmed 2026-10-18 (docs/ORCH_STATE.md)

No text removed. A decision row was added to section 4 (ALL functionality ported before S28, user decision 2026-10-18).

## B2 grab queue order (2026-10-18)
citro3d stops the GX queue in C3D_FrameBegin and runs it in C3D_FrameEnd. Calls queue in call order: C3D_RenderTargetClear, C3D_FrameSplit (command list part), GX_TextureCopy, next part. The end of frame clear of the main target is a queue entry too, so pm_gpu_grab queues it before the first split. A CPU read mid-frame sees the last frame because the queue has not run, so pm_cfb_capture_prev needs no change. Texture copy: 7680 byte strips (30 tiles, 8 target rows = 8 game columns) into a 256 x 512 texture with a 512 byte gap. PICA v = 0 samples the last memory row, so v = 1 - row / 512. Grab quad: game (x, y) reads column 239 - y, row 40 + x.

## B3 depth role (2026-10-18)
- `nuGfxZBuffer` is NULL in the world until the pause or the darkness overlay allocates it (D03). The game names the z-buffer image as address 0 (`setcimg zero 0x0`). `img_role()` returns ROLE_DEPTH for siz 2, address 0 (set by a G_SETCIMG with w1 = 0) or an address inside `nuGfxZBuffer`, and a FILL cycle or Z_CMP or Z_UPD.
- Case 5 (fill): `pm_gpu_fill_depth()` writes depth only (`r_nocol`, `r_zupd`, no `r_zcmp`), depth = fill word >> 1 over 32767 (0xFFFC gives 32766, far). `flush()` keeps a 2D batch with `rect_z` under `r_nocol`.
- Case 6 (model): step 1 log on flo_10 entry 2 (`imglog`): comb `f f 1f 3` (PRIM), prim f8f0f000, word ffbc, so the colour word is constant. Depth = word >> 1 = 32734, the same value as the decal prim depth. Triangles get `z = zn * w` like the X01 G_ZS_PRIM path, colour off, depth test always.
- Known gap: the N64 tests the model depth against the scene first and then writes the constant. The PICA tests and writes one value, so the model also writes where a nearer object hides it. Not seen on screen yet.
- Decal texrect (mac_01 crystal ball, G_ZS_PRIM): `pm_gpu_tex_rect_z()` tests the prim depth like the X01 fill. Not seen on screen: the crystal ball is clamped to x 4..35 and the model is off screen in mac_01 (Mario cannot reach the house, a `SetPlayerPos` hook did not move the camera).
- flo_10 `drawlog=39:1` (temp hook, removed): model chunks #58 #59 #61 (oml 0f0a4010, depth write) and decal chunks #62 #63 #65 have the same screen boxes (190 148 434 193, -22 111 298 252, 216 111 269 148).
- Golden: toad 0, yard 0, title 0, pause1 0.00, battle 0.86 then 1.05 in a second run (baseline 1.04 to 1.06, random effects). F03: hos_05 `fast` gives 8 `F03 query` lines, 0 `tex ptr`. 3dsx 13021120 B (+2768 vs GP1).

### T01b, R05, R13 (2026-10-18)
- T01b: warp=mac_01:0 story=ch1_kooper loadslot=0, padscript `wait80,up*40,left*140,right*45,up*60,wait5,A*2,wait200,right*60,wait10` then `A*2,wait24` x40. The fortune needs 5 coins and the save slot had 0: a temp hook in vert_color set 50 coins (removed). Temp log: `texgen vtx f626 ss 4096 ts 4096`, then 400 frames of texgen vertices. Shot 25 of a `shots` run: mirrorball as a colour tile sphere (disco ball). Overlay: kmr_09 encounter shot 5 shows the swirl hole with the scene inside.
- R05: `xtrace` does not print the z-buffer clear (the depth path breaks before the log). `imglog` shows `setcimg zero role3` then `cfb_cur role0` every frame, so the clear is a depth write. Case 9: nuGfxPreNMIFuncSet is a no-op (pm_nusys.c) and gfxRetrace_Callback has no caller, so the reset tile pattern never runs.
- R13: grab slot dumps (`offdump`) show the real scene for motion_blur_flame (mac_01 fortune) and water_splash (nok_11:1 story=-75 partner=1, A taps from x -127, grabs f779 to f814). Both draw black blobs in the shots. Theory, not proven: the 2-cycle combiner reads TEXEL0 and TEXEL1 in cycle 2 in the port order (pm_comb.c term_rgb), the RDP swaps them in cycle 2. bulb_glow: plain `fast` boot, intro grabs at f2094 to 2104, shot 70 shows lightning and glow over the scene. effect_utils func_E02000AC and kgr_02 unused.c have no caller in src.
- Tool notes: `build3ds/shots` is removed by the next run_azahar.ps1 of another agent, copy shots during the run. `shots` needs the key `shots`, `shotwin` alone does nothing.
