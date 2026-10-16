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
4. Transparency half works: translucent models show the background layer but not other geometry behind them. Likely draw order or depth write.
5. The Mario logo does not show on the house in the intro cutscene.
6. FIXED by S13 (real curtains.c): the red cover is gone in the intro (seen in shots).
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
