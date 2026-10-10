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
CollisionData gCollisionData;
u8 IntroMessageIdx;
u16 StarShrineLightBeamAlpha = 255;

static Collider pm_colliders[32];
static Collider pm_zones[16];

void parent_collider_to_model(s16 colliderID, s16 modelIndex) { (void)colliderID; (void)modelIndex; }
void update_collider_transform(s16 colliderID) { (void)colliderID; }
s32 get_map_IDs_by_name(const char* mapName, s16* areaID, s16* mapID) {
    (void)mapName;
    if (areaID) *areaID = 0;
    if (mapID) *mapID = 0;
    return 0;
}
void set_map_transition_effect(ScreenTransition transition) { (void)transition; }
void sync_status_bar(void) {}

void pm_game_bind(void) {
    s32 i;
    for (i = 0; i < 32; i++) {
        pm_colliders[i].firstChild = -1;
        pm_colliders[i].nextSibling = -1;
    }
    for (i = 0; i < 16; i++) {
        pm_zones[i].firstChild = -1;
        pm_zones[i].nextSibling = -1;
    }
    gCollisionData.colliderList = pm_colliders;
    gCollisionData.numColliders = 32;
    gZoneCollisionData.colliderList = pm_zones;
    gZoneCollisionData.numColliders = 16;
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
