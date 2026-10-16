# S19: real battle tree. Everything under src/battle except .inc.c (included by other files) and the region files.
file(GLOB_RECURSE BATTLE_SRC ${DECOMP}/src/battle/*.c)
file(GLOB_RECURSE BATTLE_INC ${DECOMP}/src/battle/*.inc.c)
list(REMOVE_ITEM BATTLE_SRC ${BATTLE_INC})
list(FILTER BATTLE_SRC EXCLUDE REGEX "/[^/]*_(jp|fr|es|de|en|en_de|pal|ique)\.c$")
list(APPEND BATTLE_SRC
    ${DECOMP}/src/state_battle.c
    ${DECOMP}/src/state_demo.c
    ${DECOMP}/src/level_up.c
    ${DECOMP}/src/starpoint.c
    ${DECOMP}/src/entity/model/PowBlock.c
    ${DECOMP}/src/battle_ui_gfx.c
)
