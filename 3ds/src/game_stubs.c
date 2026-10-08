#include "common.h"
#include "pm_port.h"

GameStatus gGameStatus;
GameStatus* gGameStatusPtr;
u32 gRandSeed;
DisplayContext D_80164000[2];
DisplayContext* gDisplayContext;
Gfx* gMainGfxPos;
u16 gMatrixListPos;
s32 gCurrentDisplayContextIndex;
s32 ResetGameState;
u16* ResetSavedFrameImg;
s16 D_80073E08;
s16 D_80073E0A;
u8 ResetTilesImg[16];

void pm_game_bind(void) {
    gGameStatusPtr = &gGameStatus;
    gDisplayContext = &D_80164000[0];
    gMainGfxPos = gDisplayContext->mainGfx;
}

void crash_screen_init(void) {}
void is_debug_init(void) {}
void load_obfuscation_shims(void) {}
void shim_create_audio_system_obfuscated(void) {}
void shim_load_engine_data_obfuscated(void) {}
void step_game_loop(void) {}
void gfx_task_background(void) {}
void gfx_draw_frame(void) {}
void gfx_init_state(void) {}
