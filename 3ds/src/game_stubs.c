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
BattleStatus gBattleStatus;
PartnerStatus gPartnerStatus;
SaveData gCurrentSaveFile;
u8 IntroMessageIdx;

s32 get_map_IDs_by_name(const char* mapName, s16* areaID, s16* mapID) {
    /* Only maps load_map_by_IDs can load. ponytail: replace with the real gAreas walk once the loader is generic. */
    static const struct { const char* name; s16 area, map; } maps[] = {
        { "hos_05", AREA_HOS, 5 }, { "kmr_02", AREA_KMR, 1 }, { "kmr_20", AREA_KMR, 11 }, { "osr_00", AREA_OSR, 0 }, { "kkj_00", AREA_KKJ, 0 },
    };
    s32 i;
    /* Unknown name keeps the current map so callers never read unset IDs. */
    *areaID = gGameStatusPtr->areaID;
    *mapID = gGameStatusPtr->mapID;
    for (i = 0; i < 5; i++) {
        if (strcmp(maps[i].name, mapName) == 0) {
            *areaID = maps[i].area;
            *mapID = maps[i].map;
            return 1;
        }
    }
    return 0;
}

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
void create_audio_system(void);
void shim_create_audio_system_obfuscated(void) { create_audio_system(); }

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
