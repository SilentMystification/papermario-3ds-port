#include "common.h"
#include "pm_port.h"

/* Calls the intro script reaches before their real systems exist.
 * Each one consumes the arguments the opcode already counted, then finishes. */

static ApiStatus skip_call(Evt* script, bool initial) {
    (void)initial;
    if (script->curArgc > 0) {
        script->ptrReadPos += script->curArgc;
        script->curArgc = 0;
    }
    return ApiStatus_DONE2;
}

#define STUB(name) ApiStatus name(Evt* script, bool initial) { return skip_call(script, initial); }

STUB(SpeakToPlayer)
STUB(ShowMessageAtScreenPos)

EvtScript hos_05_EVS_Starship_Depart = {
    Return
    End
};

EvtScript hos_05_EVS_EnterStarship = {
    Return
    End
};
void sfx_play_sound(s32 id) { (void)id; }
STUB(ShowEmote)
STUB(ShowMessageAtWorldPos)
