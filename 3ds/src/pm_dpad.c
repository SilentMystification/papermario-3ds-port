/* UI1c: the test for free control in the world. The D-pad mode Multifunction (pm_input.c) uses it:
 * in the free overworld the D-pad is the C-stick, in every other state it is the analog stick and the C-stick. */
#include "common.h"
#include "functions.h"
#include "hud_element.h"
#include "game_modes.h"
#include "pm_port.h"

extern PopupMenu* gPopupMenu;
s32 pm_msg_active(void);

int pm_free_overworld(void) {
    if (get_game_mode() != GAME_MODE_WORLD) return 0;
    if (gPlayerStatus.flags & (PS_FLAG_INPUT_DISABLED | PS_FLAG_CUTSCENE_MOVEMENT | PS_FLAG_PAUSED | PS_FLAG_ENTERING_BATTLE | PS_FLAG_HAS_CONVERSATION_NPC)) return 0;
    if (pm_msg_active()) return 0;
    if (gPopupMenu != NULL && gPopupMenu->result == POPUP_RESULT_CHOOSING) return 0;
    return 1;
}
