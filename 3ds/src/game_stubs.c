#include "common.h"
#include "camera.h"
#include "fio.h"
#include "pm_port.h"

s32 ResetGameState;
u16* ResetSavedFrameImg;
s16 D_80073E08;
s16 D_80073E0A;
u8 ResetTilesImg[16];
extern PlayerData gPlayerData;
SaveGlobals gSaveGlobals;
extern PlayerStatus gPlayerStatus;
PlayerStatus* gPlayerStatusPtr = &gPlayerStatus;
BattleStatus gBattleStatus;
PartnerStatus gPartnerStatus;
SaveData gCurrentSaveFile;
u8 IntroMessageIdx;
u16 StarShrineLightBeamAlpha = 255;

s32 get_map_IDs_by_name(const char* mapName, s16* areaID, s16* mapID) {
    (void)mapName;
    if (areaID) *areaID = 0;
    if (mapID) *mapID = 0;
    return 0;
}
void set_map_transition_effect(ScreenTransition transition) { (void)transition; }
void sync_status_bar(void) {}

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
Vec3s gEntityColliderFaces[] = {
    { 4, 6, 5 }, { 4, 7, 6 },
    { 0, 3, 4 }, { 3, 7, 4 },
    { 3, 2, 7 }, { 2, 6, 7 },
    { 2, 1, 6 }, { 1, 5, 6 },
    { 1, 0, 5 }, { 0, 4, 5 },
    { 0, 1, 2 }, { 0, 2, 3 },
};

Vec3f gEntityColliderNormals[] = {
    {  0.0f,  1.0f,  0.0f }, {  0.0f,  1.0f,  0.0f },
    {  1.0f,  0.0f,  0.0f }, {  1.0f,  0.0f,  0.0f },
    {  0.0f,  0.0f, -1.0f }, {  0.0f,  0.0f, -1.0f },
    { -1.0f,  0.0f,  0.0f }, { -1.0f,  0.0f,  0.0f },
    {  0.0f,  0.0f,  1.0f }, {  0.0f,  0.0f,  1.0f },
    {  0.0f, -1.0f,  0.0f }, {  0.0f, -1.0f,  0.0f },
};
