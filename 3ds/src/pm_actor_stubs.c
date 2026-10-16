/* Old-style definitions so these can satisfy the linker without repeating
 * every retail prototype. They return 0 and write nothing. */

#define STUB(name) int name() { return 0; }

STUB(check_input_open_menus)
STUB(check_input_status_bar)
STUB(func_800F0C9C)
STUB(handle_floor_behavior)
STUB(load_battle)
STUB(set_battle_formation)
STUB(set_battle_stage)
STUB(start_rumble)

int cymbal_crush_header;
int flip_card_1_header;
int flip_card_2_header;
int flip_card_3_header;
int flutter_down_header;
int horizontal_pipe_curl_header;
int shiver_header;
int shock_header;
int shuffle_cards_header;
int spirit_capture_header;
int startle_header;
int tutankoopa_gather_header;
int tutankoopa_swirl_1_header;
int tutankoopa_swirl_2_header;
int unfurl_header;
int unused_1_header;
int unused_2_header;
int unused_3_header;
int vertical_pipe_curl_header;
int get_in_bed_header;
/* Partner system is not ported. Real enter_exit.c and sfx_control.c reference these. */
