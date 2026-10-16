#include "common.h"
#include "map.h"
#include "world/actions.h"

/* Symbols the intro's NPC and player code reference before their real systems
 * are linked. Collision, partners, and battle stay empty so the cutscene can
 * run. */


PartnerAnimations gPartnerAnimations[12];
Npc* wPartnerNpc;
s32 WorldTattleInteractionID = -1;

EvtScript EVS_NpcHitRecoil = {
    Return
    End
};

