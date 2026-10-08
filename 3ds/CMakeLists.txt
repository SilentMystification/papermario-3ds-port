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

set(GAME_TU "${DECOMP}/src/main.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/game_stubs.c")
set_source_files_properties(${GAME_TU} PROPERTIES COMPILE_OPTIONS "-w")

ctr_add_shader_library(pm_shader shaders/pm.v.pica)
dkp_add_embedded_binary_library(pm_shader_lib pm_shader)

add_executable(pm_3ds
    src/pm_main.c
    src/pm_mem.c
    src/pm_log.c
    src/pm_input.c
    src/pm_gpu.c
    src/pm_tex.c
    src/pm_gbi.c
    src/pm_nusys.c
    src/pm_os.c
    src/pm_audio.c
    src/game_stubs.c
    ${DECOMP}/src/main.c
)
target_link_options(pm_3ds PRIVATE -Wl,--gc-sections)
target_link_libraries(pm_3ds pm_shader_lib citro3d ctru m)

ctr_create_3dsx(pm_3ds SMDH "${CMAKE_CURRENT_BINARY_DIR}/pm_3ds.smdh")
ctr_generate_smdh("${CMAKE_CURRENT_BINARY_DIR}/pm_3ds.smdh"
    NAME "Paper Mario"
    DESCRIPTION "N64 decomp on O3DS"
    AUTHOR "pmret"
)
