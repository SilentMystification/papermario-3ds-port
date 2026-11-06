/* ROUTE: closed loop walking for the padscript token go_<x>_<z>_<scans> (pm_input.c). The route script gives a world target,
 * this function gives the stick that walks Mario to it. The input depends only on the game state, so a run repeats exactly. */
#include "common.h"
#include <stdio.h>
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

/* POSHUD: bottom screen row for Mario: position and targetYaw as whole numbers (rounded half away from zero, integer printf only). */
#define POS_R(v) ((int)((v) + ((v) < 0.f ? -0.5f : 0.5f)))
void pm_pos_text(char* out) {
    snprintf(out, 40, "Mario x %d y %d z %d yaw %d", POS_R(gPlayerStatus.pos.x), POS_R(gPlayerStatus.pos.y), POS_R(gPlayerStatus.pos.z), POS_R(gPlayerStatus.targetYaw));
}

/* MAPHUD: the 3 row map block (HUD rows 4 to 6). Always filled. During `hold` steps after a change (returns 1):
 * row 4 the old map id, then the change on the same line ("<old>     <old> -> <new>"), row 5 the old area and map (no arrow), row 6 blank.
 * Otherwise (returns 0): row 4 the current map id, row 5 its area and map, row 6 blank.
 * BOTLAYOUT: pm_map_now is the footer map row, "Map: <id> (area A, map M)", always. Per step: two loads and compares. */
static char map_now[40];
const char* pm_map_now(void) { return map_now; }
extern s32 gCurrentBattleID;
extern s32 gCurrentStageID;
const char* pm_battle_hud_text(int formation, int stage, char* out, unsigned n); /* BT3, pm_warp.c */
int pm_map_hud_step(unsigned step, unsigned hold, char (*rows)[40]) {
    static char hold_blk[3][40], idle_blk[3][40];
    static int a_prev = -1, m_prev = -1;
    static unsigned until;
    static char from[24] = "none"; /* BOTLAYOUT: the map id before the change, for the info line */
    static s32 bt_f = -1, bt_s = -1;
    int a, m;
    if (get_game_mode() == GAME_MODE_BATTLE) { /* BT3: the footer row is the battle line, built on a change of the formation or stage */
        if (gCurrentBattleID != bt_f || gCurrentStageID != bt_s) {
            bt_f = gCurrentBattleID; bt_s = gCurrentStageID;
            pm_battle_hud_text(bt_f, bt_s, map_now, sizeof map_now);
            pm_log_d("maphud: battle %s", map_now);
        }
    } else if (bt_f != -1) { bt_f = -1; a_prev = -1; } /* back in the world: the map line is built again (the map change path) */
    pm_route_mapid(&a, &m);
    if (a != a_prev || m != m_prev) {
        const char* id = "none";
        char oa[20] = "", na[20] = "";
        if (a_prev >= 0) snprintf(oa, sizeof oa, "area %d map %d", a_prev, m_prev);
        a_prev = a;
        m_prev = m;
        until = step + hold;
        if (a >= 0 && a < ARRAY_COUNT(gAreas) && gAreas[a].maps && m >= 0 && m < gAreas[a].mapCount) id = gAreas[a].maps[m].id;
        pm_log_i("map change: %s -> %s (area %d map %d)", from, id, a, m);
        if (a >= 0) snprintf(na, sizeof na, "area %d map %d", a, m);
        snprintf(hold_blk[0], 40, "%-11.11s%.12s -> %.12s", from, from, id);
        snprintf(hold_blk[1], 40, "%.39s", oa);
        hold_blk[2][0] = 0;
        snprintf(idle_blk[0], 40, "%.39s", id);
        snprintf(idle_blk[1], 40, "%.39s", na);
        idle_blk[2][0] = 0;
        snprintf(from, sizeof from, "%s", id);
        snprintf(map_now, sizeof map_now, "Map: %s (area %d, map %d)", id, a, m);
        pm_log_d("maphud: %s area %d map %d step %u until %u", id, a, m, step, until);
    }
    if (until && step < until) { memcpy(rows, hold_blk, sizeof hold_blk); return 1; }
    until = 0;
    memcpy(rows, idle_blk, sizeof idle_blk);
    return 0;
}

/* PROF: where the player is, for the prof lines (pm_phase.c pf_report): map id string, story progress, battle id, Mario position. Read only. */
void pm_prof_where(const char** name, int* story, int* btl, int* entry, float pos[3]) {
    *entry = (int)gGameStatusPtr->entryID; /* the door or entry the player arrived by: tells the rooms of one map apart */
    pos[0] = gPlayerStatus.pos.x; pos[1] = gPlayerStatus.pos.y; pos[2] = gPlayerStatus.pos.z;
    *name = gAreas[gGameStatusPtr->areaID].maps[gGameStatusPtr->mapID].id;
    *story = (int)evt_get_variable(NULL, GB_StoryProgress);
    *btl = (int)gCurrentBattleID;
}
