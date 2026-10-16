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

## S01 result: all area_* maps (measured, no code change)

- 28 areas, 2,401 `.c` files, 9.6 MB of `.c` source (10.0 MB with headers).
- Compiled with the CMake flags (-O2, devkitARM, `-c` only): 1,960 files built, 441 failed. The failures are missing generated enums (`ACTOR_LEVEL_*`, `SHADING_*`, `ITEM_FIRST_KEY`, `NPC_SELF`). Those 441 files are 4.0 MB of source. The built files are 6.3 MB of source.
- Built files: .text 538 KB, .data 2,865 KB (the map data is non-const, so it is .data), .rodata 10 KB, .bss 212 KB.
- Extrapolated to all files (x1.64 by source size): .text about 0.9 MB, .data about 4.7 MB, .bss about 0.35 MB. Total about 6 MB. Current 3dsx is 745 KB.
- RAM: `pm_mem.c` splits the 64 MB O3DS FCRAM as 4 MB N64 heap + 40 MB cart image + 20 MB linear. Nothing is free. 6 MB of static image does not fit.
- DECISION: does NOT fit. Do not link all maps. S08 must link only the maps in use, or load map data from the ROM at run time. Linking a subset needs a `gAreas` built from only those maps. The real `gAreas` in `world.c` references all of them.

## Open issues (carry over until fixed)

1. kkj_00 is not reached by the intro chain. See above. Toad Town (mac_00) and every other map also fail to load. The generic map table fixes both.
2. FIXED by S15 (slice S15): sprite shading renders. See the S15 result section. No linked map enables shading yet (all call SHADING_NONE), so it shows only when a map with a real profile is linked.
3. Text box transitions do not happen. Parakarry's first text box does not show.
4. FIXED by S26 (slice S26): translucent draws wrote depth and hid geometry behind them. See the S26 result section.
5. The Mario logo does not show on the house in the intro cutscene.
6. FIXED by S13 (real curtains.c): the red cover is gone in the intro (seen in shots).
7. FIXED by S26 (slice S26): same cause as issue 4. Mario's sprite no longer has a black rectangle that hides the scene.

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

## Reference ports (slice S07 note)

Every agent may read the ports in `reference/` (see `reference/README.md`) to learn how a feature works. Read `reference/paperboat` first (Paper Mario PC port, runs the decomp loader). Use `reference/soh-3ds` and `reference/sm64-3ds` for 3DS GPU, audio and memory. Read to learn the method, then build the real decomp file here. Do not copy a hand-written stand-in from a reference port. If a stand-in is the only way to match, write `BLOCKED:` and ask.

## Known stand-ins added or recorded by S07

- S07, new: `load_asset_by_name` in `3ds/src/pm_map.c` is a port-side copy of the real `load_asset_by_name` in `src/world/world.c` (line 268). Reason: the real `background.c` `load_map_bg` calls it, and `world.c` is S08 (needs `gAreas`). The port copy uses the port's own `find_asset`, which byte-swaps the asset table. Paperboat does not use a stand-in. It builds the real `src/world/world.c` copy of the function. So the way to use the real one is to build `world.c` (S08). Remove the port copy in S08.
- S09, recorded by S07: the `world_partner_*` and `world_use_item_ROM_*` symbols are `--defsym` aliases that point at `gPartnerStatus`. This is a placeholder from S09. It is for CODE only (MIPS overlays that the ARM build links in place), so `dma_copy` copies nothing and nothing is read. Slice P01 checked this: no data symbol uses the placeholder (see the P01 result).

## Do not

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

## S19 result (slice S19: battle tree, DONE ON BRANCH `s19-full-battle`, NOT on main)

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
- G02: jump animation plays, Mario barely leaves the ground. Cause unknown. Not checked.
- Also queued by the coordinator: S32 input speed, S33 gitignore cleanup (no detail given to S27).

- Open issue V01 (recorded during slice S32, not worked in S32): the user also reports that the WRONG background shows in game (the intro cutscene or title background in a later scene). User-reported, unverified. Joins the squished background report in V01. Hypothesis, not verified: one fixed buffer (gBackgroundImage) plus a texture cache keyed on address and size returns a stale picture. Test steps are in the V01 row of 3ds/SLICES.md.

Done (slice S32). `autoa` in `pm_input.c` now presses A on 2 of every 4 scans (was 1 scan in 240), and `fast` skips the 30 Hz `gspWaitForVBlank` pad wait in `pm_main.c`. The log line `load map` now shows `scan N` (pad scan count). Before (`fast autoa freshsave`, old pattern): kmr_20 at scan 3928, 87 s. After: scan 562, 31 s. With a save present: kmr_20 at scan 410. Chain hos_05, kmr_20, osr_00, kmr_20, mac_00 reached, mac_00 stays up, 0 unmapped lines, no crash (150 s and 100 s runs). `fast autoa shots freshsave` shows the real title, file select, name entry (AAAAAAAA) and file list. Not changed: Azahar speed limit (not needed).
