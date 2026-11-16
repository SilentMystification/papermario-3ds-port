#include "kkj_27.h"
#include "nd_gtab.h"

#include "world/common/atomic/TexturePan.inc.c"

NDN_DECL(N(setup_gfx_candle_lights));

EvtScript N(EVS_ExitWalk_kkj_14_2) = EVT_EXIT_WALK(60, kkj_27_ENTRY_0, "kkj_14", kkj_14_ENTRY_2);

EvtScript N(EVS_BindExitTriggers) = {
    BindTrigger(Ref(N(EVS_ExitWalk_kkj_14_2)), TRIGGER_FLOOR_ABOVE, COLLIDER_deilis, 1, 0)
    Return
    End
};

EvtScript N(D_80240778_B144E8) = {
    Call(GetEntryID, LVar0)
    Switch(LVar0)
        CaseEq(kkj_27_ENTRY_0)
            Set(LVar0, Ref(N(EVS_BindExitTriggers)))
            Exec(EnterWalk)
    EndSwitch
    Return
    End
};

EvtScript N(EVS_Main) = {
    Set(GB_WorldLocation, LOCATION_PEACHS_CASTLE)
    Call(SetSpriteShading, SHADING_KKJ_27)
    SetUP_CAMERA_NO_LEAD()
    Exec(N(EVS_SetupRotatingWall))
    Exec(N(EVS_SetupMusic))
    Exec(N(D_80240778_B144E8))
    Call(SetTexPanner, MODEL_o207, 0)
    Thread
        TEX_PAN_PARAMS_ID(TEX_PANNER_0)
        TEX_PAN_PARAMS_STEP(    0,    0,   40,  900)
        TEX_PAN_PARAMS_FREQ(    1,    1,    1,    1)
        TEX_PAN_PARAMS_INIT(    0,    0,    0,    0)
        Exec(N(EVS_UpdateTexturePan))
    EndThread
    Call(SetModelCustomGfx, MODEL_o207, CUSTOM_GFX_0, ENV_TINT_UNCHANGED)
    Call(SetCustomGfx, CUSTOM_GFX_0, Ref(NDN(N(setup_gfx_candle_lights))), nullptr)
    Call(SetTexPanner, MODEL_o209, 0)
    Thread
        TEX_PAN_PARAMS_ID(TEX_PANNER_0)
        TEX_PAN_PARAMS_STEP(    0,    0,   40,  900)
        TEX_PAN_PARAMS_FREQ(    1,    1,    1,    1)
        TEX_PAN_PARAMS_INIT(    0,    0,    0,    0)
        Exec(N(EVS_UpdateTexturePan))
    EndThread
    Call(SetModelCustomGfx, MODEL_o209, CUSTOM_GFX_0, ENV_TINT_UNCHANGED)
    Call(SetCustomGfx, CUSTOM_GFX_0, Ref(NDN(N(setup_gfx_candle_lights))), nullptr)
    Call(SetCamSpeed, CAM_DEFAULT, Float(3.0))
    Return
    End
};
