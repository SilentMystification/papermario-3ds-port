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

STUB(SetSpriteShading)
STUB(SetMusic)
STUB(FadeInMusic)
STUB(PlaySound)
STUB(DisablePlayerInput)
STUB(PlaySoundAt)
STUB(PlaySoundAtPlayer)
STUB(GetPlayerPos)
STUB(GetPartnerInUse)
STUB(InterruptUsePartner)
STUB(func_802D2C14)
STUB(PlayerMoveTo)
STUB(PlayerFaceNpc)
STUB(SpeakToPlayer)
STUB(SetPlayerAnimation)
STUB(UseExitHeading)
STUB(ShowMessageAtScreenPos)

s32 get_msg_lines(s32 msgID) { (void)msgID; return 1; }
void draw_msg(s32 msgID, s32 posX, s32 posY, s32 opacity, s32 palette, u8 style) {
    (void)msgID; (void)posX; (void)posY; (void)opacity; (void)palette; (void)style;
}

EvtScript ExitWalk = {
    Return
    End
};

EvtScript EnterWalk = {
    Return
    End
};

EvtScript hos_05_EVS_Starship_Depart = {
    Return
    End
};

EvtScript hos_05_EVS_EnterStarship = {
    Return
    End
};
STUB(DisablePlayerPhysics)
STUB(DismissEffect)
STUB(PlayEffect_impl)
STUB(RemoveEffect)
s32 get_global_flag(s32 idx) { (void)idx; return 0; }
s32 get_area_flag(s32 idx) { (void)idx; return 0; }
s32 get_global_byte(s32 idx) { (void)idx; return 0; }
s32 get_area_byte(s32 idx) { (void)idx; return 0; }
s32 set_global_flag(s32 idx) { (void)idx; return 0; }
s32 set_area_flag(s32 idx) { (void)idx; return 0; }
s32 clear_global_flag(s32 idx) { (void)idx; return 0; }
s32 clear_area_flag(s32 idx) { (void)idx; return 0; }
s8 set_global_byte(s32 idx, s32 val) { (void)idx; (void)val; return 0; }
s8 set_area_byte(s32 idx, s32 val) { (void)idx; (void)val; return 0; }

Trigger* create_trigger(TriggerBlueprint* def) { (void)def; return NULL; }
void delete_trigger(Trigger* toDelete) { (void)toDelete; }
s32 is_another_trigger_bound(Trigger* trigger, EvtScript* script) { (void)trigger; (void)script; return 0; }
void sfx_play_sound(s32 id) { (void)id; }
void fx_fire_breath(void) {}
void render_animated_model(s32 animatorID, Mtx* rootTransform) { (void)animatorID; (void)rootTransform; }
void update_model_animator_with_transform(s32 animatorID, Mtx* mtx) { (void)animatorID; (void)mtx; }
void get_screen_overlay_params(s32 idx, u8* type, f32* zoom) {
    (void)idx;
    if (type) *type = 0;
    if (zoom) *zoom = 0.f;
}
