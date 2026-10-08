#include "common.h"
#include "camera.h"
#include "fio.h"
#include "pm_port.h"

DisplayContext* gDisplayContext;
Gfx* gMainGfxPos;
s32 ResetGameState;
u16* ResetSavedFrameImg;
s16 D_80073E08;
s16 D_80073E0A;
u8 ResetTilesImg[16];
u16* nuGfxCfb_ptr;
PlayerData gPlayerData;
SaveGlobals gSaveGlobals;
Camera gCameras[4];
s32 gCurrentCameraID;

void pm_game_bind(void) {
    gGameStatusPtr = &gGameStatus;
    gDisplayContext = &D_80164000[0];
    gMainGfxPos = gDisplayContext->mainGfx;
}

void crash_screen_init(void) {}
void is_debug_init(void) {}
void load_obfuscation_shims(void) {}
void shim_create_audio_system_obfuscated(void) {}

void load_engine_data(void);
void shim_load_engine_data_obfuscated(void) { load_engine_data(); }

void gfx_init_state(void) {}
void gfx_draw_background(void) {}

void create_cameras(void) {
    int i;
    for (i = 0; i < 4; i++) gCameras[i].flags = CAMERA_FLAG_DISABLED;
}

void set_cam_viewport(s16 id, s16 x, s16 y, s16 width, s16 height) {
    if (id < 0 || id >= 4) return;
    gCameras[id].viewportStartX = x;
    gCameras[id].viewportStartY = y;
    gCameras[id].viewportW = width;
    gCameras[id].viewportH = height;
}
