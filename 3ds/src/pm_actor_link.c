#include "common.h"
#include "effects.h"
#include "map.h"
#include "world/actions.h"

/* Symbols the intro's NPC and player code reference before their real systems
 * are linked. Collision, partners, and battle stay empty so the cutscene can
 * run. Effect calls that the decoration code immediately dereferences point at
 * one inert instance until the effects slice replaces them. */

static u8 pm_fx_blob[0x100];
static EffectInstance pm_fx;
static SpriteShadingProfile pm_shade;

SpriteShadingProfile* gSpriteShadingProfile = &pm_shade;
CollisionStatus gCollisionStatus;
HiddenPanelsData gCurrentHiddenPanels;
PartnerAnimations gPartnerAnimations[12];
Npc* wPartnerNpc;
DisguiseAnims BasicPeachDisguiseAnims[4];
s32 WorldTattleInteractionID = -1;
s32 NpcHitQueryColliderID;
s32 PrevPlayerDirection;
s32 PrevPlayerCamRelativeYaw;
f32 PlayerNormalYaw;
f32 PlayerNormalPitch;
f32 D_800F7B48;
s32 D_800F7B4C;

EvtScript EVS_NpcHitRecoil = {
    Return
    End
};

static EffectInstance* pm_fx_instance(void) {
    pm_fx.data.aura = (AuraFXData*)pm_fx_blob;
    return &pm_fx;
}

void fx_aura(s32 type, f32 x, f32 y, f32 z, f32 scale, EffectInstance** out) {
    (void)type; (void)x; (void)y; (void)z; (void)scale;
    if (out) *out = pm_fx_instance();
}

void fx_stars_orbiting(s32 type, f32 x, f32 y, f32 z, f32 scale, s32 n, EffectInstance** out) {
    (void)type; (void)x; (void)y; (void)z; (void)scale; (void)n;
    if (out) *out = pm_fx_instance();
}

EffectInstance* fx_energy_orb_wave(s32 type, f32 x, f32 y, f32 z, f32 scale, s32 arg5) {
    (void)type; (void)x; (void)y; (void)z; (void)scale; (void)arg5;
    return pm_fx_instance();
}

s32 integer_log(s32 number, u32 base) {
    s32 ret = 1;
    if (base < 2) return 1;
    while (number > (s32)base) {
        number /= (s32)base;
        ret++;
    }
    return ret;
}

void remove_effect(EffectInstance* effect) {
    (void)effect;
}
