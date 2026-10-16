#include "common.h"
#include "camera.h"
#include "fio.h"
#include "pm_port.h"

extern PlayerData gPlayerData;
extern PlayerStatus gPlayerStatus;
BattleStatus gBattleStatus;

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

void shim_general_heap_create_obfuscated(void) { general_heap_create(); }
