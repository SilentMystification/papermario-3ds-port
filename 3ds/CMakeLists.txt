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
    ${DECOMP}/src/evt/f8f60_len_1560.c
    ${DECOMP}/src/world/area_hos/hos_05/intro.c
    ${DECOMP}/src/world/area_hos/hos_05/main.c
    ${DECOMP}/src/world/area_hos/hos_05/misc.c
    ${DECOMP}/src/world/area_hos/hos_05/model_fx.c
    ${DECOMP}/src/world/area_hos/hos_05/music.c
    ${DECOMP}/src/world/area_hos/hos_05/narrator.c
    ${DECOMP}/src/world/area_hos/hos_05/npc.c
)
set(GAME_TU ${GAME_SRC} "${CMAKE_CURRENT_SOURCE_DIR}/src/game_stubs.c")
set_source_files_properties(${GAME_TU} PROPERTIES COMPILE_OPTIONS "-w;-Wno-error=implicit-function-declaration")

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
    src/pm_map.c
    src/pm_title.c
    src/pm_evt_stub.c
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
