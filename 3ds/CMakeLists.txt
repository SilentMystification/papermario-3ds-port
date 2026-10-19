cmake_minimum_required(VERSION 3.13)
project(pm_3ds C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

set(DECOMP "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_compile_options(-O2 -fsigned-char -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Dnullptr=NULL> $<$<COMPILE_LANGUAGE:C>:-Wa,-I${DECOMP}>)
add_compile_definitions(
    TARGET_3DS
    VERSION_US
    VERSION=us
    F3DEX_GBI_2
    _LANGUAGE_C
    _MIPS_SZLONG=32
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/stub_include
    ${DECOMP}/include
    ${DECOMP}/src
    ${DECOMP}/assets/us
    ${DECOMP}/ver/us/build/include
)

include(effects.cmake)
include(battle.cmake)

set(GAME_SRC
    ${DECOMP}/src/main.c
    ${DECOMP}/src/main_loop.c
    ${DECOMP}/src/game_modes.c
    ${DECOMP}/src/game_states.c
    ${DECOMP}/src/state_startup.c
    ${DECOMP}/src/state_logos.c
    ${DECOMP}/src/startup_screen_fading.c
    ${DECOMP}/src/heap.c
    ${DECOMP}/src/heaps3.c
    ${DECOMP}/src/43F0.c
    ${DECOMP}/src/input.c
    ${DECOMP}/src/cam_main.c
    ${DECOMP}/src/cam_math.c
    ${DECOMP}/src/cam_mode_interp.c
    ${DECOMP}/src/screen_overlays.c
    ${DECOMP}/src/curtains.c
    ${DECOMP}/src/entity/Shadow.c
    ${DECOMP}/src/screen_render_util.c
    ${DECOMP}/src/cam_mode_minimal.c
    ${DECOMP}/src/cam_mode_no_interp.c
    ${DECOMP}/src/cam_mode_unused_ahead.c
    ${DECOMP}/src/cam_mode_unused_leading.c
    ${DECOMP}/src/cam_mode_unused_radial.c
    ${DECOMP}/src/cam_mode_zone_interp.c
    ${DECOMP}/src/state_intro.c
    ${DECOMP}/src/model.c
    ${DECOMP}/src/model_clear_render_tasks.c
    ${DECOMP}/src/texture_memory.c
    ${DECOMP}/src/os/guOrtho.c
    ${DECOMP}/src/os/lookathil.c
    ${DECOMP}/src/os/frustum.c
    ${DECOMP}/src/os/position.c
    ${DECOMP}/src/os/sqrtf.c
    ${DECOMP}/src/os/perspective.c
    ${DECOMP}/src/os/guRotate.c
    ${DECOMP}/src/os/lookatref.c
    ${DECOMP}/src/os/guMtxXFML.c
    ${DECOMP}/src/os/sins.c
    ${DECOMP}/src/os/coss.c
    ${DECOMP}/src/worker.c
    ${DECOMP}/src/evt/evt.c
    ${DECOMP}/src/evt/script_list.c
    ${DECOMP}/src/evt/cam_api.c
    ${DECOMP}/src/evt/model_api.c
    ${DECOMP}/src/evt/map_api.c
    ${DECOMP}/src/world/script_api/enter_exit.c
    ${DECOMP}/src/evt/audio_api.c
    ${DECOMP}/src/heaps.c
    ${DECOMP}/src/heaps2.c
    ${DECOMP}/src/main_pre.c
    ${DECOMP}/src/38F00.c
    ${DECOMP}/src/rumble.c
    ${DECOMP}/src/background.c
    ${DECOMP}/src/background_gfx.c
    ${DECOMP}/src/evt/virtual_entity.c
    ${DECOMP}/src/audio/ambience.c
    ${DECOMP}/src/audio/bgm_control.c
    ${DECOMP}/src/audio/bgm_player.c
    ${DECOMP}/src/audio/load_banks.c
    ${DECOMP}/src/audio/mseq_player.c
    ${DECOMP}/src/audio/sfx_control.c
    ${DECOMP}/src/audio/sfx_player.c
    ${DECOMP}/src/audio/snd_interface.c
    ${DECOMP}/src/audio/core/engine.c
    ${DECOMP}/src/audio/core/pull_voice.c
    ${DECOMP}/src/audio/core/reverb.c
    ${DECOMP}/src/audio/core/syn_driver.c
    ${DECOMP}/src/audio/core/system.c
    ${DECOMP}/src/audio/core/tables.c
    ${DECOMP}/src/audio/core/voice.c
    ${DECOMP}/src/enemy_ai.c
    ${DECOMP}/src/effects.c
    ${DECOMP}/src/effect_utils.c
    ${DECOMP}/src/evt/fx_api.c
    ${EFFECT_SRC}
    ${DECOMP}/src/25AF0.c
    ${DECOMP}/src/evt/f8f60_len_1560.c
    ${DECOMP}/src/evt/npc_api.c
    ${DECOMP}/src/encounter_api.c
    ${DECOMP}/src/npc.c
    ${DECOMP}/src/npc_collision.c
    ${DECOMP}/src/sprite.c
    ${DECOMP}/src/101b90_len_8f0.c
    ${DECOMP}/src/imgfx.c
    ${DECOMP}/src/msg.c
    ${DECOMP}/src/msg_draw.c
    ${DECOMP}/src/msg_data.c
    ${DECOMP}/src/77480.c
    ${DECOMP}/src/encounter.c
    ${DECOMP}/src/evt/player_api.c
    ${DECOMP}/src/world/script_api/rooms.c
    ${DECOMP}/src/entity/Block.c
    ${DECOMP}/src/entity/SimpleSpring.c
    ${DECOMP}/src/entity/model/SimpleSpring_anim.c
    ${DECOMP}/src/entity/ShatteringBlock.c
    ${DECOMP}/src/entity/ShatteringBlock_common.c
    ${DECOMP}/src/entity/model/ScriptSpring_anim.c
    ${DECOMP}/src/entity/default/BombableRock.c
    ${DECOMP}/src/collision.c
    ${DECOMP}/src/state_map_transitions.c
    ${DECOMP}/src/os/nusys/nucontrmbforcestopend.c
    ${DECOMP}/src/os/nusys/nucontrmbforcestop.c
    ${DECOMP}/src/os/nusys/nucontrmbcheck.c
    ${DECOMP}/src/os/nusys/nucontrmbmodeset.c
    ${DECOMP}/src/os/nusys/nucontrmbstart.c
    ${DECOMP}/src/world/partners.c
    ${DECOMP}/src/world_use_item.c
    ${DECOMP}/src/world/partner/goombario.c
    ${DECOMP}/src/world/partner/kooper.c
    ${DECOMP}/src/world/partner/bombette.c
    ${DECOMP}/src/world/partner/parakarry.c
    ${DECOMP}/src/world/partner/goompa.c
    ${DECOMP}/src/world/partner/watt.c
    ${DECOMP}/src/world/partner/sushie.c
    ${DECOMP}/src/world/partner/lakilester.c
    ${DECOMP}/src/world/partner/bow.c
    ${DECOMP}/src/world/partner/goombaria.c
    ${DECOMP}/src/world/partner/twink.c
    ${DECOMP}/src/world/actions.c
    ${DECOMP}/src/world/action/hammer.c
    ${DECOMP}/src/world/action/hit_fire.c
    ${DECOMP}/src/world/action/hit_lava.c
    ${DECOMP}/src/world/action/idle.c
    ${DECOMP}/src/world/action/jump.c
    ${DECOMP}/src/world/action/knockback.c
    ${DECOMP}/src/world/action/land.c
    ${DECOMP}/src/world/action/misc.c
    ${DECOMP}/src/world/action/slide.c
    ${DECOMP}/src/world/action/sneaky_parasol.c
    ${DECOMP}/src/world/action/spin.c
    ${DECOMP}/src/world/action/spin_jump.c
    ${DECOMP}/src/world/action/step_up.c
    ${DECOMP}/src/world/action/tornado_jump.c
    ${DECOMP}/src/world/action/use_munchlesia.c
    ${DECOMP}/src/world/action/use_spinning_flower.c
    ${DECOMP}/src/world/action/use_tweester.c
    ${DECOMP}/src/world/action/walk.c
    ${DECOMP}/src/7B440.c
    ${DECOMP}/src/7BB60.c
    ${DECOMP}/src/7E9D0.c
    ${DECOMP}/src/entity.c
    ${DECOMP}/src/entity_model.c
    ${DECOMP}/src/animator.c
    ${DECOMP}/src/trigger.c
    ${DECOMP}/src/hud_element.c
    ${DECOMP}/src/global_hud_scripts.c
    ${DECOMP}/src/5B320.c
    ${DECOMP}/src/msg_img.c
    ${DECOMP}/src/evt/msg_api.c
    ${DECOMP}/src/draw_box.c
    ${DECOMP}/src/draw_img_util.c
    ${DECOMP}/src/speech_bubble.c
    ${DECOMP}/src/inspect_icon.c
    ${DECOMP}/src/i_spy.c
    ${DECOMP}/src/pulse_stone.c
    ${DECOMP}/src/windows.c
    ${DECOMP}/src/status_star_shimmer.c
    ${DECOMP}/src/inventory.c
    ${DECOMP}/src/status_icons.c
    ${DECOMP}/src/item_entity.c
    ${DECOMP}/src/sprite_shading.c
    ${DECOMP}/src/evt/demo_api.c
    ${DECOMP}/src/evt/item_api.c
    ${DECOMP}/src/vars_access.c
    ${DECOMP}/src/world/area_kmr/kmr_20/bed.c
    ${DECOMP}/src/world/area_kmr/kmr_20/records.c
    ${DECOMP}/src/world/area_kmr/kmr_20/bushes.c
    ${DECOMP}/src/world/area_kmr/kmr_20/documents.c
    ${DECOMP}/src/world/area_kmr/kmr_20/entity.c
    ${DECOMP}/src/world/area_kmr/kmr_20/main.c
    ${DECOMP}/src/world/area_kmr/kmr_20/music.c
    ${DECOMP}/src/world/area_kmr/kmr_20/npc.c
    ${DECOMP}/src/world/area_kmr/kmr_20/npc_epilogue.c
    ${DECOMP}/src/world/area_kmr/kmr_20/npc_intro.c
    ${DECOMP}/src/world/area_kmr/kmr_20/npc_scenes.c
    ${DECOMP}/src/world/area_kmr/kmr_20/rooms.c
    ${DECOMP}/src/world/area_kmr/kmr_20/settings.c
    ${DECOMP}/src/world/area_kmr/kmr_20/trees.c
    ${DECOMP}/src/world/area_osr/osr_00/main.c
    ${DECOMP}/src/world/area_osr/osr_00/music.c
    ${DECOMP}/src/world/area_osr/osr_00/npc_intro.c
    ${DECOMP}/src/world/area_osr/osr_00/npc_epilogue.c
    ${DECOMP}/src/world/area_osr/osr_00/scenes.c
    ${DECOMP}/src/world/area_osr/osr_00/settings.c
    ${DECOMP}/src/world/area_kkj/kkj_00/main.c
    ${DECOMP}/src/world/area_kkj/kkj_00/music.c
    ${DECOMP}/src/world/area_kkj/kkj_00/npc_intro.c
    ${DECOMP}/src/world/area_kkj/kkj_00/npc_ending.c
    ${DECOMP}/src/world/area_kkj/kkj_00/scene_intro.c
    ${DECOMP}/src/world/area_kkj/kkj_00/scene_ending.c
    ${DECOMP}/src/world/area_kkj/kkj_00/settings.c
    ${DECOMP}/src/world/area_hos/hos_05/intro.c
    ${DECOMP}/src/world/area_hos/hos_05/main.c
    ${DECOMP}/src/world/area_hos/hos_05/misc.c
    ${DECOMP}/src/world/area_hos/hos_05/model_fx.c
    ${DECOMP}/src/world/area_hos/hos_05/music.c
    ${DECOMP}/src/world/area_hos/hos_05/narrator.c
    ${DECOMP}/src/world/area_hos/hos_05/npc.c
    ${DECOMP}/src/world/area_hos/hos_05/settings.c
    ${DECOMP}/src/world/area_hos/hos_05/star_ship.c
    ${DECOMP}/src/world/area_kmr/kmr_02/settings.c
    ${DECOMP}/src/world/area_kmr/kmr_02/music.c
    ${DECOMP}/src/world/area_kmr/kmr_02/main.c
    ${DECOMP}/src/world/area_kmr/kmr_02/npc.c
    ${DECOMP}/src/world/area_kmr/kmr_02/rooms_1.c
    ${DECOMP}/src/world/area_kmr/kmr_02/entity.c
    ${DECOMP}/src/world/area_kmr/kmr_02/rooms_2.c
    ${DECOMP}/src/world/area_kmr/kmr_02/kammy.c
    ${DECOMP}/src/world/area_kmr/kmr_02/foliage.c
    ${DECOMP}/src/world/area_kmr/kmr_02/images.c
    ${DECOMP}/src/world/area_mac/mac_00/settings.c
    ${DECOMP}/src/world/area_mac/mac_00/music.c
    ${DECOMP}/src/world/area_mac/mac_00/main.c
    ${DECOMP}/src/world/area_mac/mac_00/entity.c
    ${DECOMP}/src/world/area_mac/mac_00/rooms.c
    ${DECOMP}/src/world/area_mac/mac_00/npc.c
    ${DECOMP}/src/world/area_mac/mac_00/shop.c
    ${DECOMP}/src/world/area_mac/mac_00/music_mix.c
    ${DECOMP}/src/world/area_mac/mac_00/demo.c
    ${DECOMP}/src/world/area_mac/mac_00/water_gfx.c
    ${DECOMP}/src/world/area_mac/mac_00/shared_npc.c
    ${DECOMP}/src/world/area_mac/mac_00/intro_npc.c
    ${DECOMP}/src/world/area_mac/mac_00/intro.c
    ${DECOMP}/src/world/area_mac/mac_00/epilogue_npc.c
    ${DECOMP}/src/world/area_mac/mac_00/epilogue.c
    ${DECOMP}/src/world/world.c
    ${DECOMP}/src/entity/model/Hammer1Block.c
    ${DECOMP}/src/entity/model/BrickBlock.c
    ${DECOMP}/src/entity/model/Hammer3Block.c
    ${DECOMP}/src/entity/model/ScriptSpring_gfx.c
    ${DECOMP}/src/entity/model/ShatteringBrickBlock.c
    ${DECOMP}/src/entity/model/ShatteringHammer1Block.c
    ${DECOMP}/src/entity/model/ShatteringHammer2Block.c
    ${DECOMP}/src/entity/model/ShatteringHammer3Block.c
    ${DECOMP}/src/entity/model/BombableRock.c
    ${DECOMP}/src/entity/jan_iwa/Plants1.c
    ${DECOMP}/src/entity/model/CymbalPlant_anim.c
    ${DECOMP}/src/entity/model/CymbalPlant_gfx.c
    ${DECOMP}/src/entity/model/PinkFlower_anim.c
    ${DECOMP}/src/entity/model/PinkFlower_gfx.c
    ${DECOMP}/src/entity/model/SpinningFlower.c
    ${DECOMP}/src/entity/BlueWarpPipe.c
    ${DECOMP}/src/entity/model/BlueWarpPipe.c
    ${DECOMP}/src/entity/SaveBlock.c
    ${DECOMP}/src/entity/model/SaveBlock.c
    ${DECOMP}/src/entity/HiddenPanel.c
    ${DECOMP}/src/entity/model/HiddenPanel.c
    ${DECOMP}/src/world/script_api/shops.c
    ${DECOMP}/src/fio.c
    ${DECOMP}/src/8a860_len_3f30.c
    ${DECOMP}/src/world/surfaces.c
    ${DECOMP}/src/world/menus.c
    ${DECOMP}/src/state_pause.c
    ${DECOMP}/src/state_title_screen.c
    ${DECOMP}/src/state_file_select.c
    ${DECOMP}/src/state_world.c
    ${DECOMP}/src/filemenu/filemenu_common.c
    ${DECOMP}/src/filemenu/filemenu_main.c
    ${DECOMP}/src/filemenu/filemenu_yesno.c
    ${DECOMP}/src/filemenu/filemenu_info.c
    ${DECOMP}/src/filemenu/filemenu_options.c
    ${DECOMP}/src/filemenu/filemenu_createfile.c
    ${DECOMP}/src/filemenu/filemenu_gfx.c
    ${DECOMP}/src/filemenu/filemenu_msg.c
    ${DECOMP}/src/filemenu/filemenu_styles.c
    ${DECOMP}/src/menu_hud_scripts.c
    ${DECOMP}/src/os/nusys/nupireadromoverlay.c
    ${DECOMP}/src/pause/pause_main.c
    ${DECOMP}/src/pause/pause_tabs.c
    ${DECOMP}/src/pause/pause_stats.c
    ${DECOMP}/src/pause/pause_badges.c
    ${DECOMP}/src/pause/pause_items.c
    ${DECOMP}/src/pause/pause_partners.c
    ${DECOMP}/src/pause/pause_spirits.c
    ${DECOMP}/src/pause/pause_map.c
    ${DECOMP}/src/pause/pause_gfx.c
    ${DECOMP}/src/pause/pause_styles.c
    ${BATTLE_SRC}
)
# S30: every map. maps.cmake and map_syms.lds come from tools/gen_map_sources.py. The real world.c is built whole with its real gAreas.
# S30b: all maps are linked, no guard.
include(maps.cmake)
foreach(a ${MAP_AREAS})
    list(APPEND MAP_SRC ${MAP_SRC_${a}})
endforeach()
list(REMOVE_ITEM MAP_SRC ${GAME_SRC})
list(APPEND GAME_SRC ${MAP_SRC})
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/maps.cmake)
set(GAME_TU ${GAME_SRC} "${CMAKE_CURRENT_SOURCE_DIR}/src/game_stubs.c")
set_source_files_properties(${GAME_TU} PROPERTIES COMPILE_OPTIONS "-w;-Wno-error=implicit-function-declaration;-Wno-return-mismatch;-Wno-int-conversion;-Wno-incompatible-pointer-types")
# G02: world/actions.c holds f32 tables that 7B440.c reads through pointer arithmetic from DefaultMoveSpeed[-1] (walk, run, max jump).
# -fdata-sections plus --gc-sections gave each variable its own section: the layout broke and maxJumpSpeed read GravityParamsStartFall[0].
# Keep this file as one data block in source order.
set_property(SOURCE ${DECOMP}/src/world/actions.c APPEND PROPERTY COMPILE_OPTIONS -fno-data-sections -fno-toplevel-reorder)
# D02: demo_api.c SetSpriteShading reads ShadingOffsetsBuffer[2] and PackedShadingData[0x100] over the BSS variables that follow them in script_list.c
# (declared as 4 byte variables there). gcc emits BSS in reverse order, so the 256 byte DMA overwrote D_802DB830 (item_api PopupMenu).
set_property(SOURCE ${DECOMP}/src/evt/script_list.c APPEND PROPERTY COMPILE_OPTIONS -fno-data-sections -fno-toplevel-reorder)
# F02: these map files end a script without an End (decomp @bug, 'not terminated'). find_script_labels then reads on into the next data
# until it finds an End. In source order that is the next script of the file. With -fdata-sections the next bytes were BSS zeros and the scan ran
# to the end of the image (gv_01 EVS_DropLetter, unmapped reads over 8 MB). Keep the files as one data block in source order.
# C2H: jan_05, jan_08, jan_09 npc.c include MBush.inc.c, whose EVS_NpcAI_MBush ends in StartBattle with no End (decomp "fallthrough :(").
set_property(SOURCE ${DECOMP}/src/world/area_gv/gv_01/main.c ${DECOMP}/src/world/area_kmr/kmr_11/main.c
    ${DECOMP}/src/world/area_kkj/kkj_15/main.c ${DECOMP}/src/world/area_kkj/kkj_16/main.c
    ${DECOMP}/src/world/area_kkj/kkj_23/main.c ${DECOMP}/src/world/area_kkj/kkj_29/main.c
    ${DECOMP}/src/world/area_jan/jan_05/npc.c ${DECOMP}/src/world/area_jan/jan_08/npc.c ${DECOMP}/src/world/area_jan/jan_09/npc.c
    APPEND PROPERTY COMPILE_OPTIONS -fno-data-sections -fno-toplevel-reorder)
# V12c: effect gfx files read one array on into the next array of the file (snowfall D_09000C00_38DC70 is 16 raw bytes with no end, the state
# list D_09000C10_38DC80 follows. fright_jar indexes D_09004040_3C5BE0 as 22 vertex rows over the three vertex arrays after it). With -fdata-sections
# the next array was somewhere else: snowfall drew nothing, the jar drew a black wedge.
# 14 effect gfx files have raw byte lists (grep '^u8 D_09' src/effects/gfx). Only snowfall and fright_jar are done here: all 14 cost 18 KB (the 10 KB rule), see SLICES.md row V12d.
# Keep the file as one data block in source order, as in the ROM.
set(EFFECT_GFX_SRC ${DECOMP}/src/effects/gfx/snowfall.c ${DECOMP}/src/effects/gfx/fright_jar.c)
# V22: five more files where a gsSPVertex list loads more vertices than its array holds and the triangles index past it into the next array(s) (scan: docs/plans/dlist_noend.md).
# V12d: the 12 other files with a raw u8 D_09 list. V16V17: fire_breath (D_09000B88 loads 5 vertices and ends, the triangles are in D_09000BA8).
foreach(_e balloon effect_75 ending_decals fire_breath fire_flower flashing_box_shockwave huff_puff_breath ice_pillar lil_oink merlin_house_stars peach_star_beam purple_ring quizmo_assistant snowman_doll static_status sun throw_spiny tubba_heart_attack water_block)
    list(APPEND EFFECT_GFX_SRC ${DECOMP}/src/effects/gfx/${_e}.c)
endforeach()
set_property(SOURCE ${EFFECT_GFX_SRC} APPEND PROPERTY COMPILE_OPTIONS -fno-data-sections -fno-toplevel-reorder)
# heaps.c holds gMapShapeData as a 32 KB array. Real map shapes are up to 157 KB, so the port defines the real-size buffer
# (pm_map.c) and heaps.c gets its copy renamed.
set_property(SOURCE ${DECOMP}/src/heaps.c APPEND PROPERTY COMPILE_OPTIONS -DgMapShapeData=pm_unused_shape_data -DD_80200000=pm_unused_80200000)
set_property(SOURCE ${DECOMP}/src/world/partners.c APPEND PROPERTY COMPILE_OPTIONS -include common.h)
file(GLOB PAUSE_SRC ${DECOMP}/src/pause/pause_*.c)
set_property(SOURCE ${PAUSE_SRC} APPEND PROPERTY COMPILE_OPTIONS -include common.h)

ctr_add_shader_library(pm_shader shaders/pm.v.pica)
dkp_add_embedded_binary_library(pm_shader_lib pm_shader)

set(PORT_SRC
    src/pm_main.c
    src/pm_mem.c
    src/pm_log.c
    src/pm_phase.c
    src/pm_input.c
    src/pm_warp.c
    src/pm_options.c
    src/pm_fxtest.c
    src/pm_gpu.c
    src/pm_depth.c
    src/pm_tex.c
    src/pm_gbi.c
    src/pm_comb.c
    src/pm_gu.c
    src/pm_interp.c
    src/pm_nusys.c
    src/pm_os.c
    src/pm_audio.c
    src/pm_rsp_audio.c
    src/pm_later.c
    src/pm_map.c
    src/pm_mapdata.c
    src/map_restore_table.c
    src/pm_buffers.c
    src/game_stubs.c
    src/effect_loads.c
)
set(PM_BUILD_ID_H "${CMAKE_CURRENT_BINARY_DIR}/pm_build_id.h")
add_custom_command(
    OUTPUT "${PM_BUILD_ID_H}"
    COMMAND ${CMAKE_COMMAND}
        -DNUMBER_FILE=${CMAKE_CURRENT_BINARY_DIR}/build_number.txt
        -DOUT=${PM_BUILD_ID_H}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/bump_build.cmake
    DEPENDS ${PORT_SRC} ${GAME_SRC} shaders/pm.v.pica
    VERBATIM
)

file(STRINGS ${DECOMP}/src/effect_shims.yaml EFFECT_SHIM_LINES REGEX "^- shim_")
foreach(line ${EFFECT_SHIM_LINES})
    string(REGEX REPLACE "^- shim_" "" fn "${line}")
    if(NOT fn STREQUAL "NULL")
        list(APPEND EFFECT_SHIM_DEFSYMS "-Wl,--defsym,shim_${fn}=${fn}")
    endif()
endforeach()

# The decomp assumes 4-byte enums. arm-none-eabi-gcc makes enums as small as possible, so an AuResult that holds a
# song name (0x31303120) was cut to one byte. Files that include libctru or citro3d keep the default, because
# those libraries were built with it and their structures hold enums.
set(PM_NO_SHORT_ENUM_SRC ${PORT_SRC} ${GAME_SRC})
list(REMOVE_DUPLICATES PM_NO_SHORT_ENUM_SRC)
list(REMOVE_ITEM PM_NO_SHORT_ENUM_SRC src/pm_main.c src/pm_mem.c src/pm_log.c src/pm_input.c src/pm_gpu.c src/pm_tex.c src/pm_audio.c)
set_property(SOURCE ${PM_NO_SHORT_ENUM_SRC} APPEND PROPERTY COMPILE_OPTIONS -fno-short-enums)

add_executable(pm_3ds
    ${PORT_SRC}
    ${GAME_SRC}
    ${PM_BUILD_ID_H}
)
target_include_directories(pm_3ds PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
# MS01: 3dsx.ld of devkitPro with the map statics blocks (INCLUDE of ld/map_restore.ld, made by tools/gen_map_restore.py). gcc finds it
# through -B before its own copy: the 3dsx.specs link spec names 3dsx.ld.
set(MS_LD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ld)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ld/3dsx.ld.in ${CMAKE_BINARY_DIR}/ldscripts/3dsx.ld @ONLY)
target_link_options(pm_3ds PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/map_syms.lds -B${CMAKE_BINARY_DIR}/ldscripts/ -Wl,--gc-sections -Wl,-Map,${CMAKE_BINARY_DIR}/pm_3ds.map
    -Wl,--wrap=set_game_mode -Wl,--wrap=load_map_by_IDs -Wl,--wrap=dma_copy -Wl,--wrap=load_asset_by_name
    -Wl,--wrap=step_game_loop -Wl,--wrap=gfx_draw_frame -Wl,--wrap=gfx_task_background -Wl,--wrap=appendGfx_darkness_stencil
    -Wl,--defsym,heap_battleHead=gFrameBuf2
    -Wl,--defsym,sprite_shading_profiles_ROM_START=0x315B80
    -Wl,--defsym,sprite_shading_profiles_data_ROM_START=0x315D50
    # partner overlays are linked in. Each range is empty (START == END, in the image) so dma_copy copies nothing.
    -Wl,--defsym,world_partner_goombario_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_goombario_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_goombario_VRAM=gPartnerStatus
    -Wl,--defsym,world_partner_kooper_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_kooper_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_kooper_VRAM=gPartnerStatus
    -Wl,--defsym,world_partner_bombette_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_bombette_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_bombette_VRAM=gPartnerStatus
    -Wl,--defsym,world_partner_parakarry_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_parakarry_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_parakarry_VRAM=gPartnerStatus
    -Wl,--defsym,world_partner_goompa_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_goompa_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_goompa_VRAM=gPartnerStatus
    -Wl,--defsym,world_partner_watt_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_watt_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_watt_VRAM=gPartnerStatus
    -Wl,--defsym,world_partner_sushie_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_sushie_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_sushie_VRAM=gPartnerStatus
    -Wl,--defsym,world_partner_lakilester_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_lakilester_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_lakilester_VRAM=gPartnerStatus
    -Wl,--defsym,world_partner_bow_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_bow_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_bow_VRAM=gPartnerStatus
    -Wl,--defsym,world_partner_goombaria_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_goombaria_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_goombaria_VRAM=gPartnerStatus
    -Wl,--defsym,world_partner_twink_ROM_START=gPartnerStatus
    -Wl,--defsym,world_partner_twink_ROM_END=gPartnerStatus
    -Wl,--defsym,world_partner_twink_VRAM=gPartnerStatus
    -Wl,--defsym,world_use_item_ROM_START=gPartnerStatus
    -Wl,--defsym,world_use_item_ROM_END=gPartnerStatus
    # P01: pause overlay code and its image data (INCLUDE_IMG in menu_hud_scripts.c) are linked in place, so every range
    # is empty and nothing is read from the ROM. Do not give these a real ROM offset: dma_copy would then copy ROM bytes
    # over gPartnerStatus (the VRAM target).
    -Wl,--defsym,ui_images_filemenu_pause_ROM_START=gPartnerStatus
    -Wl,--defsym,ui_images_filemenu_pause_ROM_END=gPartnerStatus
    -Wl,--defsym,ui_images_filemenu_pause_VRAM=gPartnerStatus
    -Wl,--defsym,pause_ROM_START=gPartnerStatus
    -Wl,--defsym,pause_ROM_END=gPartnerStatus
    -Wl,--defsym,pause_VRAM=gPartnerStatus
    -Wl,--defsym,pause_TEXT_START=gPartnerStatus
    -Wl,--defsym,pause_TEXT_END=gPartnerStatus
    -Wl,--defsym,pause_DATA_START=gPartnerStatus
    -Wl,--defsym,pause_RODATA_END=gPartnerStatus
    -Wl,--defsym,pause_BSS_START=gPartnerStatus
    -Wl,--defsym,pause_BSS_END=gPartnerStatus
    # S17: filemenu overlay is linked in place (same as pause). Empty ranges, nothing is read from the ROM.
    -Wl,--defsym,filemenu_ROM_START=gPartnerStatus
    -Wl,--defsym,filemenu_ROM_END=gPartnerStatus
    -Wl,--defsym,filemenu_VRAM=gPartnerStatus
    -Wl,--defsym,filemenu_TEXT_START=gPartnerStatus
    -Wl,--defsym,filemenu_TEXT_END=gPartnerStatus
    -Wl,--defsym,filemenu_DATA_START=gPartnerStatus
    -Wl,--defsym,filemenu_RODATA_END=gPartnerStatus
    -Wl,--defsym,filemenu_BSS_START=gPartnerStatus
    -Wl,--defsym,filemenu_BSS_END=gPartnerStatus
    # S30: ver/us/undefined_syms.txt line 9 (real decomp alias, used by the flo maps)
    -Wl,--defsym,fx_sun_undeclared=fx_sun
    # effect_shims.yaml entries are asm jumps through effectFuncs in the N64 build. Here each one is an alias of the real function.
    ${EFFECT_SHIM_DEFSYMS})
set_target_properties(pm_3ds PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/map_syms.lds;${CMAKE_CURRENT_SOURCE_DIR}/ld/3dsx.ld.in;${CMAKE_CURRENT_SOURCE_DIR}/ld/map_restore.ld")
target_link_libraries(pm_3ds pm_shader_lib citro3d ctru m)

ctr_create_3dsx(pm_3ds SMDH "${CMAKE_CURRENT_BINARY_DIR}/pm_3ds.smdh")
ctr_generate_smdh("${CMAKE_CURRENT_BINARY_DIR}/pm_3ds.smdh"
    NAME "Paper Mario"
    DESCRIPTION "N64 decomp on O3DS"
    AUTHOR "pmret"
)
