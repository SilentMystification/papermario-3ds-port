/* Systems the logo slice does not run. Each one is replaced by its real file later. */
unsigned char heap_battleHead[0x25800] __attribute__((aligned(16)));
/* npc_iter_no_op is npc.c */
/* disable_player_input is 77480.c */
/* spr_render_init is sprite.c */
/* player_render_interact_prompts is 77480.c */
void crash_screen_set_draw_info() {}
/* clear_player_status is 77480.c */
/* spr_init_sprites is sprite.c */
/* clear_npcs is npc.c */
/* init_encounter_status is npc.c */
void reset_battle_status() {}
/* update_npcs, update_player, update_encounters are npc.c and 77480.c */
/* render_npcs is npc.c, render_player is 77480.c */
void state_drawUI_battle() {}
void state_drawUI_demo() {}
void state_drawUI_end_battle() {}
void state_init_battle() {}
void state_init_demo() {}
void state_init_end_battle() {}
void state_step_battle() {}
void state_step_demo() {}
void state_step_end_battle() {}
short FlowerGoalPositionX, FlowerGoalPositionY, FlowerGoalPositionZ;
