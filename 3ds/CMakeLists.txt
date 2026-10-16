cmake_minimum_required(VERSION 3.13)
project(pm_3ds C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

set(DECOMP "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_compile_options(-O2 -fsigned-char -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Dnullptr=NULL>)
add_compile_definitions(
    TARGET_3DS
    VERSION_US
    F3DEX_GBI_2
    _LANGUAGE_C
    _MIPS_SZLONG=32
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/stub_include
    ${DECOMP}/include
    ${DECOMP}/src
)

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
    ${DECOMP}/src/enemy_ai.c
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
    ${DECOMP}/src/vars_access.c
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
)
set(GAME_TU ${GAME_SRC} "${CMAKE_CURRENT_SOURCE_DIR}/src/game_stubs.c")
set_source_files_properties(${GAME_TU} PROPERTIES COMPILE_OPTIONS "-w;-Wno-error=implicit-function-declaration;-Wno-return-mismatch;-Wno-int-conversion;-Wno-incompatible-pointer-types")

ctr_add_shader_library(pm_shader shaders/pm.v.pica)
dkp_add_embedded_binary_library(pm_shader_lib pm_shader)

set(PORT_SRC
    src/pm_main.c
    src/pm_mem.c
    src/pm_log.c
    src/pm_input.c
    src/pm_gpu.c
    src/pm_tex.c
    src/pm_gbi.c
    src/pm_gu.c
    src/pm_nusys.c
    src/pm_os.c
    src/pm_audio.c
    src/pm_later.c
    src/pm_overlay.c
    src/pm_fx.c
    src/pm_map.c
    src/pm_title.c
    src/pm_evt_stub.c
    src/pm_msg.c
    src/pm_actor_link.c
    src/pm_kmr_stubs.c
    src/pm_shadow.c
    src/pm_actor_stubs.c
    src/game_stubs.c
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

add_executable(pm_3ds
    ${PORT_SRC}
    ${GAME_SRC}
    ${PM_BUILD_ID_H}
)
target_include_directories(pm_3ds PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_options(pm_3ds PRIVATE -Wl,--gc-sections)
target_link_libraries(pm_3ds pm_shader_lib citro3d ctru m)

ctr_create_3dsx(pm_3ds SMDH "${CMAKE_CURRENT_BINARY_DIR}/pm_3ds.smdh")
ctr_generate_smdh("${CMAKE_CURRENT_BINARY_DIR}/pm_3ds.smdh"
    NAME "Paper Mario"
    DESCRIPTION "N64 decomp on O3DS"
    AUTHOR "pmret"
)
