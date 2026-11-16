#include "kkj_15.h"
#include "nd_gtab.h"

#include "world/common/atomic/TexturePan.inc.c"
#include "../common/RestoreFromPeachState.inc.c"

NDN_DECL(N(setup_gfx_candle_lights));

EvtScript N(EVS_EndPeachChapter0) = {
    Set(LVar0, GB_KKJ_LastPartner)
    Call(N(RestoreFromPeachState))
    Call(PlaySound, SOUND_SLIDE_WHISTLE_OUT)
    Call(GotoMapSpecial, Ref("kmr_10"), kmr_10_ENTRY_0, TRANSITION_END_PEACH_INTERLUDE)
    Wait(100)
}; //@bug script not properly terminated

EvtScript N(EVS_EndPeachChapter1) = {
    Set(LVar0, GB_KKJ_LastPartner)
    Call(N(RestoreFromPeachState))
    Call(PlaySound, SOUND_SLIDE_WHISTLE_OUT)
    Call(GotoMapSpecial, Ref("trd_00"), trd_00_ENTRY_5, TRANSITION_END_PEACH_INTERLUDE)
    Wait(100)
}; //@bug script not properly terminated

EvtScript N(EVS_EndPeachChapter3) = {
    Set(LVar0, GB_KKJ_LastPartner)
    Call(N(RestoreFromPeachState))
    Call(PlaySound, SOUND_SLIDE_WHISTLE_OUT)
    Call(GotoMapSpecial, Ref("arn_07"), arn_07_ENTRY_3, TRANSITION_END_PEACH_INTERLUDE)
    Wait(100)
}; //@bug script not properly terminated

EvtScript N(EVS_ExitDoor_kkj_11_3) = {
    SetGroup(EVT_GROUP_EXIT_MAP)
    Call(DisablePlayerInput, true)
    Set(LVar0, kkj_15_ENTRY_0)
    Set(LVar1, COLLIDER_tte)
    Set(LVar2, MODEL_o2)
    Set(LVar3, DOOR_SWING_IN)
    Exec(ExitSingleDoor)
    Wait(17)
    Call(GotoMap, Ref("kkj_11"), kkj_11_ENTRY_3)
    Wait(100)
    Return
    End
};

EvtScript N(EVS_BindExitTriggers) = {
    BindTrigger(Ref(N(EVS_ExitDoor_kkj_11_3)), TRIGGER_WALL_PRESS_A, COLLIDER_tte, 1, 0)
    Return
    End
};

EvtScript N(EVS_EnterMap) = {
    Call(GetEntryID, LVar0)
    Switch(LVar0)
        CaseEq(kkj_15_ENTRY_0)
            Set(LVar2, MODEL_o2)
            Set(LVar3, DOOR_SWING_IN)
            ExecWait(EnterSingleDoor)
            Exec(N(EVS_BindExitTriggers))
        CaseEq(kkj_15_ENTRY_1)
            Switch(GB_StoryProgress)
                CaseEq(STORY_CH1_BEGAN_PEACH_MISSION)
                    Exec(N(EVS_EnterRotatingWall))
                    Exec(N(EVS_SetupChapter1))
                CaseEq(STORY_CH2_BEGAN_PEACH_MISSION)
                    Exec(N(EVS_EnterRotatingWall))
                    Exec(N(EVS_SetupChapter2))
                CaseEq(STORY_CH3_BEGAN_PEACH_MISSION)
                    Exec(N(EVS_Scene_Chapter3))
                CaseDefault
                    Exec(N(EVS_EnterRotatingWall))
                    Exec(N(EVS_BindExitTriggers))
            EndSwitch
        CaseEq(kkj_15_ENTRY_2)
            Exec(N(EVS_Scene_Chapter0_BowserAndKammy))
    EndSwitch
    Return
    End
};

EvtScript N(EVS_Main) = {
    Set(GB_WorldLocation, LOCATION_PEACHS_CASTLE)
    Call(SetSpriteShading, SHADING_NONE)
    EVT_SETUP_CAMERA_DEFAULT()
    Call(GetEntryID, LVar0)
    Switch(LVar0)
        CaseEq(kkj_15_ENTRY_2)
            Set(GB_StoryProgress, STORY_CH0_KAMMY_RETURNED_TO_BOWSER)
    EndSwitch
    Switch(GB_StoryProgress)
        CaseEq(STORY_CH0_KAMMY_RETURNED_TO_BOWSER)
            Call(MakeNpcs, false, Ref(N(Chapter0NPCs)))
        CaseEq(STORY_CH1_BEGAN_PEACH_MISSION)
            Call(MakeNpcs, false, Ref(N(Chapter1NPCs)))
        CaseEq(STORY_CH3_BEGAN_PEACH_MISSION)
            Call(MakeNpcs, false, Ref(N(Chapter3NPCs)))
    EndSwitch
    IfNe(GB_StoryProgress, STORY_CH1_BEGAN_PEACH_MISSION)
        Call(EnableModel, MODEL_o76, false)
        Call(ModifyColliderFlags, MODIFY_COLLIDER_FLAGS_SET_BITS, COLLIDER_o76, COLLIDER_FLAGS_UPPER_MASK)
    EndIf
    Call(ModifyColliderFlags, MODIFY_COLLIDER_FLAGS_SET_BITS, COLLIDER_g25, COLLIDER_FLAGS_UPPER_MASK)
    Call(SetTexPanner, MODEL_o151, TEX_PANNER_0)
    Thread
        TEX_PAN_PARAMS_ID(TEX_PANNER_0)
        TEX_PAN_PARAMS_STEP(    0,    0,   40,  900)
        TEX_PAN_PARAMS_FREQ(    1,    1,    1,    1)
        TEX_PAN_PARAMS_INIT(    0,    0,    0,    0)
        Exec(N(EVS_UpdateTexturePan))
    EndThread
    Call(SetModelCustomGfx, MODEL_o151, CUSTOM_GFX_0, ENV_TINT_UNCHANGED)
    Call(SetCustomGfx, CUSTOM_GFX_0, Ref(NDN(N(setup_gfx_candle_lights))), nullptr)
    Call(SetTexPanner, MODEL_o152, TEX_PANNER_0)
    Thread
        TEX_PAN_PARAMS_ID(TEX_PANNER_0)
        TEX_PAN_PARAMS_STEP(    0,    0,   40,  900)
        TEX_PAN_PARAMS_FREQ(    1,    1,    1,    1)
        TEX_PAN_PARAMS_INIT(    0,    0,    0,    0)
        Exec(N(EVS_UpdateTexturePan))
    EndThread
    Call(SetModelCustomGfx, MODEL_o152, CUSTOM_GFX_0, ENV_TINT_UNCHANGED)
    Call(SetCustomGfx, CUSTOM_GFX_0, Ref(NDN(N(setup_gfx_candle_lights))), nullptr)
    Call(UseDoorSounds, DOOR_SOUNDS_BASIC)
    Exec(N(EVS_SetupMusic))
    Call(GetEntryID, LVar0)
    IfEq(LVar0, kkj_15_ENTRY_1)
        IfEq(GB_StoryProgress, STORY_CH3_BEGAN_PEACH_MISSION)
            Call(StopSound, SOUND_KKJ_ROTATING_WALL)
        Else
            Call(PlaySoundAt, SOUND_KKJ_ROTATING_WALL, SOUND_SPACE_DEFAULT, 50, 5, -200)
        EndIf
        Wait(10)
    EndIf
    Exec(N(EVS_EnterMap))
    Wait(1)
    Return
    End
};
