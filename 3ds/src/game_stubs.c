#include "common.h"
#include "camera.h"
#include "fio.h"
#include "pm_port.h"

s32 ResetGameState;
u16* ResetSavedFrameImg;
s16 D_80073E08;
s16 D_80073E0A;
u8 ResetTilesImg[16];
PlayerData gPlayerData;
SaveGlobals gSaveGlobals;
PlayerStatus gPlayerStatus;
PlayerStatus* gPlayerStatusPtr = &gPlayerStatus;
BattleStatus gBattleStatus;
PartnerStatus gPartnerStatus;
CollisionData gZoneCollisionData;
u8 IntroMessageIdx;

void pm_game_bind(void) {
    gGameStatusPtr = &gGameStatus;
    gDisplayContext = &D_80164000[0];
    gMainGfxPos = gDisplayContext->mainGfx;
    init_worker_list();
    init_script_list();
}

void crash_screen_init(void) {}
void is_debug_init(void) {}
void load_obfuscation_shims(void) {}
void shim_create_audio_system_obfuscated(void) {}

void load_engine_data(void);
void shim_load_engine_data_obfuscated(void) { load_engine_data(); }

void gfx_init_state(void) {}
void pm_draw_loaded_background(void);
void gfx_draw_background(void) { pm_draw_loaded_background(); }
