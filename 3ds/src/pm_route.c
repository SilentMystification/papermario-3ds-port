/* ROUTE: closed loop walking for the padscript token go_<x>_<z>_<scans> (pm_input.c). The route script gives a world target,
 * this function gives the stick that walks Mario to it. The input depends only on the game state, so a run repeats exactly. */
#include "common.h"
#include <string.h>
#include "functions.h"
#include "game_modes.h"
#include "map.h"
#include "fio.h"
#include "battle/battle.h"
#include "pm_port.h"
extern unsigned pm_frame_count(void);
extern unsigned pm_uptime_ms(void);

/* Returns 1 when Mario is within 24 units of (tx, tz). Else sets the stick (-80 to 80) toward the target, turned by the camera yaw as player_input_to_move_vector does. */
int pm_route_aim(int tx, int tz, int* sx, int* sy) {
    float dx = (float)tx - gPlayerStatus.pos.x;
    float dz = (float)tz - gPlayerStatus.pos.z;
    static int ltx = 0x7fffffff, ltz, still;
    static float lx, lz;
    if (tx != ltx || tz != ltz) { ltx = tx; ltz = tz; still = 0; lx = gPlayerStatus.pos.x; lz = gPlayerStatus.pos.z; }
    /* a leg ends when Mario does not move for 90 scans (a wall): the next token of the route goes on */
    if (__builtin_fabsf(gPlayerStatus.pos.x - lx) + __builtin_fabsf(gPlayerStatus.pos.z - lz) < 1.0f) still++; else still = 0;
    lx = gPlayerStatus.pos.x; lz = gPlayerStatus.pos.z;
    if (still >= 90) { still = 0; ltx = 0x7fffffff; *sx = *sy = 0; pm_log("route: leg blocked at %d %d", (int)lx, (int)lz); return 1; }
    if (dx * dx + dz * dz < 24.0f * 24.0f) { ltx = 0x7fffffff; *sx = *sy = 0; return 1; }
    float world = __builtin_atan2f(dx, -dz) * 57.29578f;      /* movement is dx = sin(a), dz = -cos(a) */
    float phi = (world - gCameras[CAM_DEFAULT].curYaw) * 0.01745329f; /* stick angle: game angle = atan2(stickX, stickY) + camera yaw */
    *sx = (int)(80.0f * __builtin_sinf(phi));
    *sy = (int)(80.0f * __builtin_cosf(phi));
    return 0;
}

/* ROUTE: padscript token goto_<map>:<entry>. The same real map change as one step of the tour switch (pm_warp.c tour_step): the exit script is skipped,
 * so the code does what the exit scripts do before GAME_MODE_CHANGE_MAP. */
void pm_route_goto(const char* name, int entry) {
    s16 areaID, mapID;
    if (!get_map_IDs_by_name(name, &areaID, &mapID)) { pm_log_w("route: map %s not in gAreas", name); return; }
    set_curtain_scale_goal(2.0f);
    set_curtain_draw_callback(NULL);
    bgm_set_song(1, AU_SONG_NONE, 0, 250, VOL_LEVEL_FULL);
    gGameStatusPtr->areaID = areaID;
    gGameStatusPtr->mapID = mapID;
    gGameStatusPtr->entryID = (s16)entry;
    set_map_transition_effect(TRANSITION_STANDARD);
    set_game_mode(GAME_MODE_CHANGE_MAP);
}

/* ROUTE: the area and map of the game status, for the shot file names (pm_gpu.c dump_shot). */
void pm_route_mapid(int* area, int* map) { *area = gGameStatusPtr->areaID; *map = gGameStatusPtr->mapID; }

/* ROUTE: padscript token ev_<name>: waits for a game event, takes no scan. Logs `route: ev <name> f<step>` once it holds. */
static const char* ev_names[] = { "", "title", "file", "world", "map", "battle", "pause" };
int pm_route_ev_kind(const char* name) {
    for (int k = 1; k < 7; k++) if (strcmp(name, ev_names[k]) == 0) return k;
    return 0;
}
int pm_route_ev(int kind) {
    static int armed, left; /* left: the game left the overworld since the token began (map: the world again after a map change) */
    int m = get_game_mode();
    if (!armed) { armed = 1; left = 0; }
    if (m != GAME_MODE_WORLD) left = 1;
    int ok = kind == 1 ? m == GAME_MODE_TITLE_SCREEN : kind == 2 ? m == GAME_MODE_FILE_SELECT : kind == 3 ? m == GAME_MODE_WORLD :
             kind == 4 ? (left && m == GAME_MODE_WORLD) : kind == 5 ? m == GAME_MODE_BATTLE : m == GAME_MODE_PAUSE;
    if (!ok) return 0;
    armed = 0;
    pm_log("route: ev %s f%u t=%u ms", ev_names[kind], pm_frame_count(), pm_uptime_ms());
    return 1;
}

/* ROUTE: one state line every 30 scans while a route runs: route: st f<step> mode <game mode> map <area>:<map> bs <battle state> bs2 <battle sub state> hp coins */
void pm_route_note(void) {
    pm_log("route: st f%u mode %d map %d:%d bs %d bs2 %d hp %d coins %d", pm_frame_count(), (int)get_game_mode(), (int)gGameStatusPtr->areaID, (int)gGameStatusPtr->mapID,
           (int)gBattleState, (int)gBattleSubState, (int)gPlayerData.curHP, (int)gPlayerData.coins);
}

/* ROUTE: padscript token save_<slot>: the save of the save block (fio_save_game). The line gives the time of the write in ms. */
void pm_route_save(int slot) {
    extern unsigned pm_uptime_ms(void);
    unsigned t0 = pm_uptime_ms();
    fio_save_game(slot);
    pm_log("route: save slot %d f%u took %u ms", slot, pm_frame_count(), pm_uptime_ms() - t0);
}
