#include "common.h"
#include "map.h"
#include "world/actions.h"

/* Symbols the intro's NPC and player code reference before their real systems
 * are linked. Collision, partners, and battle stay empty so the cutscene can
 * run. */

static SpriteShadingProfile pm_shade;

SpriteShadingProfile* gSpriteShadingProfile = &pm_shade;
PartnerAnimations gPartnerAnimations[12];
Npc* wPartnerNpc;
s32 WorldTattleInteractionID = -1;

EvtScript EVS_NpcHitRecoil = {
    Return
    End
};

s32 integer_log(s32 number, u32 base) {
    s32 ret = 1;
    if (base < 2) return 1;
    while (number > (s32)base) {
        number /= (s32)base;
        ret++;
    }
    return ret;
}
