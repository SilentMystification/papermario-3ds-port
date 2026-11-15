#include "common.h"
#include "nd_gtab.h"
#include "script_api/battle.h"
#include "effects.h"
#include "entity.h"
#include "ld_addrs.h"
#include "sprite/player.h"
#include "include_asset.h"

#define NAMESPACE battle_item_egg_missile

#include "battle/common/move/ItemRefund.inc.c"

API_CALLABLE(N(func_802A123C_71CF1C)) {
    Bytecode* args = script->ptrReadPos;
    s32 x = evt_get_variable(script, *args++);
    s32 y = evt_get_variable(script, *args++);
    s32 z = evt_get_variable(script, *args++);

    fx_smoke_ring(0, x, y, z);
    fx_explosion(0, x, y + 20, z);

    return ApiStatus_DONE2;
}

#include "battle/common/move/UseItem.inc.c"

static s32 _pad = 0;

#include "battle/move/item/egg_missile1.png.h"
INCLUDE_IMG("battle/move/item/egg_missile1.png", battle_item_egg_missile1_png);
INCLUDE_PAL("battle/move/item/egg_missile1.pal", battle_item_egg_missile1_pal);

#include "battle/move/item/egg_missile2.png.h"
INCLUDE_IMG("battle/move/item/egg_missile2.png", battle_item_egg_missile2_png);
INCLUDE_PAL("battle/move/item/egg_missile2.pal", battle_item_egg_missile2_pal);

#include "battle/move/item/egg_missile3.png.h"
INCLUDE_IMG("battle/move/item/egg_missile3.png", battle_item_egg_missile3_png);
INCLUDE_PAL("battle/move/item/egg_missile3.pal", battle_item_egg_missile3_pal);

#include "battle/move/item/egg_missile4.png.h"
INCLUDE_IMG("battle/move/item/egg_missile4.png", battle_item_egg_missile4_png);
INCLUDE_PAL("battle/move/item/egg_missile4.pal", battle_item_egg_missile4_pal);

Vtx N(EggMissileVtx)[] = {
    { .v = {{ -16, -16, 0 }, false, { 0,    0    }, { 0, 0, 0, 255 }}},
    { .v = {{ 15,  -16, 0 }, false, { 1024, 0    }, { 0, 0, 0, 255 }}},
    { .v = {{ 15,  15,  0 }, false, { 1024, 1024 }, { 0, 0, 0, 255 }}},
    { .v = {{ -16, 15,  0 }, false, { 0,    1024 }, { 0, 0, 0, 255 }}},
};

NDN_DECL(N(Frame1Gfx));

NDN_DECL(N(Frame2Gfx));

NDN_DECL(N(Frame3Gfx));

NDN_DECL(N(Frame4Gfx));

EntityModelScript N(EMS_EggMissile) = {
    ems_SetRenderMode(RENDER_MODE_ALPHATEST)
    ems_Draw(NDN(N(Frame1Gfx)), 2)
    ems_Draw(NDN(N(Frame2Gfx)), 2)
    ems_Draw(NDN(N(Frame3Gfx)), 2)
    ems_Restart
    ems_End
};

EntityModelScript unusedModelScript = STANDARD_ENTITY_MODEL_SCRIPT(NDN(N(Frame4Gfx)), RENDER_MODE_ALPHATEST);

EvtScript N(EVS_UseItem) = {
    SetConst(LVarA, ITEM_EGG_MISSILE)
    ExecWait(N(UseItemWithEffect))
    Call(UseBattleCamPreset, BTL_CAM_VIEW_ENEMIES)
    Call(MoveBattleCamOver, 15)
    Call(SetAnimation, ACTOR_PLAYER, 0, ANIM_Mario1_Throw)
    Call(PlaySound, SOUND_THROW)
    Wait(3)
    Call(CreateVirtualEntity, LVarA, Ref(N(EMS_EggMissile)))
    Call(GetActorPos, ACTOR_PLAYER, LVar0, LVar1, LVar2)
    Add(LVar0, 20)
    Add(LVar1, 42)
    Add(LVar2, 5)
    Call(SetVirtualEntityPosition, LVarA, LVar0, LVar1, LVar2)
    Call(InitTargetIterator)
    Call(SetGoalToTarget, ACTOR_SELF)
    Call(GetGoalPos, ACTOR_SELF, LVar0, LVar1, LVar2)
    Thread
        Set(LVar0, 0)
        Loop(18)
            Add(LVar0, 60)
            Call(SetVirtualEntityRotation, LVarA, 0, 0, LVar0)
            Wait(1)
        EndLoop
    EndThread
    Call(SetVirtualEntityJumpGravity, LVarA, Float(1.0))
    Add(LVar2, 5)
    Call(VirtualEntityJumpTo, LVarA, LVar0, LVar1, LVar2, 18)
    Call(DeleteVirtualEntity, LVarA)
    Call(PlaySound, SOUND_EGG_MISSILE_BLAST)
    Call(N(func_802A123C_71CF1C), LVar0, LVar1, LVar2)
    Thread
        Call(StartRumble, BTL_RUMBLE_HIT_EXTREME)
        Call(ShakeCam, CAM_BATTLE, 0, 2, Float(0.75))
        Call(ShakeCam, CAM_BATTLE, 0, 5, Float(1.5))
        Call(ShakeCam, CAM_BATTLE, 0, 4, Float(1.2))
        Call(ShakeCam, CAM_BATTLE, 0, 2, Float(0.45))
    EndThread
    Call(GetItemPower, ITEM_EGG_MISSILE, LVar0, LVar1)
    Call(ItemDamageEnemy, LVar0, DAMAGE_TYPE_FIRE | DAMAGE_TYPE_BLAST | DAMAGE_TYPE_IGNORE_DEFENSE | DAMAGE_TYPE_NO_CONTACT, 0, LVar0, BS_FLAGS1_TRIGGER_EVENTS)
    ExecWait(N(PlayerGoHome))
    Return
    End
};
