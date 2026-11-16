#include "kkj_18.h"
#include "nd_gtab.h"

#include "world/common/atomic/TexturePan.inc.c"
#include "../common/RestoreFromPeachState.inc.c"

NDN_DECL(N(setup_gfx_candle_lights));

EvtScript N(EVS_EndPeachChapter4) = {
    Set(LVar0, GB_KKJ_LastPartner)
    Call(N(RestoreFromPeachState))
    Call(PlaySound, SOUND_SLIDE_WHISTLE_OUT)
    Call(GotoMapSpecial, Ref("mac_04"), mac_04_ENTRY_5, TRANSITION_END_PEACH_INTERLUDE)
    Wait(100)
};

EvtScript N(EVS_UpdateClockPendulum) = {
    Loop(0)
        Call(MakeLerp, -8, 8, 30, EASING_LINEAR)
        Loop(0)
            Call(UpdateLerp)
            Call(RotateGroup, MODEL_g13, LVar0, 0, 0, 1)
            Wait(1)
            IfEq(LVar1, 0)
                BreakLoop
            EndIf
        EndLoop
        Call(MakeLerp, 8, -8, 30, EASING_LINEAR)
        Loop(0)
            Call(UpdateLerp)
            Call(RotateGroup, MODEL_g13, LVar0, 0, 0, 1)
            Wait(1)
            IfEq(LVar1, 0)
                BreakLoop
            EndIf
        EndLoop
    EndLoop
    Return
    End
};

EvtScript N(EVS_ExitDoor_0) = {
    SetGroup(EVT_GROUP_EXIT_MAP)
    Call(DisablePlayerInput, true)
    Set(LVar0, kkj_18_ENTRY_0)
    Set(LVar1, COLLIDER_ttw)
    Set(LVar2, MODEL_o9)
    Set(LVar3, DOOR_SWING_OUT)
    Exec(ExitSingleDoor)
    Wait(17)
    IfEq(AB_KKJ_CompletedBakeStep, CAKE_TYPE_NONE)
        Call(GotoMap, Ref("kkj_11"), kkj_11_ENTRY_6)
    Else
        Call(GotoMap, Ref("kkj_19"), kkj_19_ENTRY_0)
    EndIf
    Wait(100)
    Return
    End
};

EvtScript N(EVS_EnterMap) = {
    Call(DisablePlayerInput, true)
    IfNe(AF_KKJ_FinishedBakingCake, false)
        Call(SetPlayerPos, -227, 0, -35)
        Call(SetNpcPos, NPC_PARTNER, -227, 0, -35)
        Wait(30)
        Call(PlaySoundAtCollider, COLLIDER_ttw, SOUND_BASIC_DOOR_OPEN, 0)
        Call(MakeLerp, 0, 80, 10, EASING_LINEAR)
        Loop(0)
            Call(UpdateLerp)
            Call(RotateModel, MODEL_o9, LVar0, 0, -1, 0)
            Wait(1)
            IfEq(LVar1, 0)
                BreakLoop
            EndIf
        EndLoop
        Call(SetPlayerPos, -187, 0, -35)
        Call(SetNpcPos, NPC_PARTNER, -187, 0, -35)
    EndIf
    Set(LVar0, 0)
    Set(LVar2, MODEL_o9)
    Set(LVar3, DOOR_SWING_OUT)
    ExecWait(EnterSingleDoor)
    Call(DisablePlayerInput, false)
    BindTrigger(Ref(N(EVS_ExitDoor_0)), TRIGGER_WALL_PRESS_A, COLLIDER_ttw, 1, 0)
    Return
    End
};

EvtScript N(EVS_Main) = {
    Set(GB_WorldLocation, LOCATION_PEACHS_CASTLE)
    Call(SetSpriteShading, SHADING_NONE)
    EVT_SETUP_CAMERA_DEFAULT()
    IfEq(GB_StoryProgress, STORY_CH4_BEGAN_PEACH_MISSION)
        Call(MakeNpcs, false, Ref(N(DefaultNPCs)))
    EndIf
    Exec(N(EVS_UpdateClockPendulum))
    Call(SetTexPanner, MODEL_o56, TEX_PANNER_0)
    Thread
        TEX_PAN_PARAMS_ID(TEX_PANNER_0)
        TEX_PAN_PARAMS_STEP(    0,    0,   40,  900)
        TEX_PAN_PARAMS_FREQ(    1,    1,    1,    1)
        TEX_PAN_PARAMS_INIT(    0,    0,    0,    0)
        Exec(N(EVS_UpdateTexturePan))
    EndThread
    Call(SetModelCustomGfx, MODEL_o56, CUSTOM_GFX_0, ENV_TINT_UNCHANGED)
    Call(SetCustomGfx, CUSTOM_GFX_0, (Bytecode)NDN(N(setup_gfx_candle_lights)), nullptr)
    Exec(N(EVS_SetupMusic))
    Call(UseDoorSounds, DOOR_SOUNDS_BASIC)
    Exec(N(EVS_EnterMap))
    Wait(1)
    Return
    End
};

MAP_RODATA_PAD(1, exits);
