/* Old-style definitions so these can satisfy the linker without repeating
 * every retail prototype. They return 0 and write nothing. */

#define STUB(name) int name() { return 0; }

STUB(load_battle)
STUB(set_battle_formation)
STUB(set_battle_stage)

/* Partner system is not ported. Real enter_exit.c and sfx_control.c reference these. */
