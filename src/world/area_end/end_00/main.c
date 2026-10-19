#include "end_00.h"

extern void parade_wide_setup(s32 which); /* F07-6 */
API_CALLABLE(N(WideCopies)) {
    parade_wide_setup(0);
    return ApiStatus_DONE2;
}

EvtScript N(EVS_Main) = {
    Call(SetCamPerspective, CAM_DEFAULT, CAM_UPDATE_FROM_ZONE, 35, 16, 4096)
    Call(SetCamViewport, CAM_DEFAULT, 15, 28, 290, 128)
    Call(N(WideCopies))
    Call(SetCamBGColor, CAM_DEFAULT, 0, 0, 0)
    Call(SetCamEnabled, CAM_DEFAULT, true)
    Call(SetCamLeadPlayer, CAM_DEFAULT, false)
    Call(FadeOutMusic, 0, 500)
    Call(ClearAmbientSounds, 250)
    Call(EnableWorldStatusBar, false)
    Wait(30)
    Exec(N(EVS_ManageParade))
    Return
    End
};
