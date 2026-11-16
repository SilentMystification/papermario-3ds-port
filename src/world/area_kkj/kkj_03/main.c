#include "kkj_03.h"
#include "nd_gtab.h"

#include "world/common/atomic/TexturePan.inc.c"

NDN_DECL(N(setup_gfx_candle_lights));

EvtScript N(EVS_ExitDoors_kkj_02_1) = EVT_EXIT_DOUBLE_DOOR(kkj_03_ENTRY_0, "kkj_02", kkj_02_ENTRY_1,
    COLLIDER_deilit1, MODEL_door1, MODEL_door2);

EvtScript N(EVS_BindExitTriggers) = {
    BindTrigger(Ref(N(EVS_ExitDoors_kkj_02_1)), TRIGGER_WALL_PRESS_A, COLLIDER_deilit1, 1, 0)
    Return
    End
};

EvtScript N(EVS_EnterMap) = {
    Call(GetEntryID, LVar0)
    Switch(LVar0)
        CaseEq(kkj_03_ENTRY_0)
            Set(LVar2, MODEL_door1)
            Set(LVar3, MODEL_door2)
            ExecWait(EnterDoubleDoor)
            Exec(N(EVS_Scene_MeetingPeach))
        CaseEq(kkj_03_ENTRY_1)
            Exec(N(EVS_Scene_Ascending))
    EndSwitch
    Return
    End
};

EvtScript N(EVS_Main) = {
    Set(GB_WorldLocation, LOCATION_PEACH_CASTLE_GROUNDS)
    Call(SetSpriteShading, SHADING_NONE)
    SetUP_CAMERA_NO_LEAD()
    Call(MakeNpcs, false, Ref(N(DefaultNPCs)))
    Call(EnableGroup, MODEL_g156, false)
    Call(SetTexPanner, MODEL_o715, TEX_PANNER_0)
    Thread
        TEX_PAN_PARAMS_ID(TEX_PANNER_0)
        TEX_PAN_PARAMS_STEP(    0,    0,   40,  900)
        TEX_PAN_PARAMS_FREQ(    1,    1,    1,    1)
        TEX_PAN_PARAMS_INIT(    0,    0,    0,    0)
        Exec(N(EVS_UpdateTexturePan))
    EndThread
    Call(SetModelCustomGfx, MODEL_o715, CUSTOM_GFX_0, ENV_TINT_UNCHANGED)
    Call(SetCustomGfx, CUSTOM_GFX_0, (Bytecode)NDN(N(setup_gfx_candle_lights)), nullptr)
    Exec(N(EVS_SetupMusic))
    Call(UseDoorSounds, DOOR_SOUNDS_LARGE)
    Exec(N(EVS_BindExitTriggers))
    Exec(N(EVS_EnterMap))
    Wait(1)
    Return
    End
};
