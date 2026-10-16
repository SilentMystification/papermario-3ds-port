#include "common.h"
#include "map.h"
#include "world/actions.h"

/* Symbols the intro's NPC and player code reference before their real systems
 * are linked. Collision, partners, and battle stay empty so the cutscene can
 * run. */



EvtScript EVS_NpcHitRecoil = {
    Return
    End
};

