/* MAPX-1: the transition line. One `xfer` line per door (room change, same map) or map change, printed with pm_log_prof_line (prof mode
 * and logging at warn or more). A map change starts at set_game_mode(GAME_MODE_CHANGE_MAP) (the fade out) and ends 30 steps after the
 * first step that draws the new map (GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME clear after the load). A door starts when its room state leaves
 * idle (rooms.c SetDoorState) and ends 30 steps after it is idle again.
 *   xfer <door|map> <from>-><to> f<steps> max <ms> o34 <n> | fade <ms> load <ms> rom <n>/<KB> yay0 <n>/<KB>/<ms> inv <n> au <ms> init <ms>
 *        | dlc <n>/<ms> mdl <miss>/<clr> ent <n> tex <n>/<KB>/<ms>
 * max: the longest step (work ticks, all steps). o34: steps over 34 ms, the load step not counted. fade: the longest step that is neither
 * the load step nor the first drawn step. load: the load_map_by_IDs call. init: the first drawn step of the new map. The stage counters
 * are in pm_xf (pm_port.h); the stage sites read a tick only while a transition is open. Cold code: once per step one load and a branch. */
#include "common.h"
#include "game_modes.h"
#include "map.h"
#include "pm_port.h"
#include <stdio.h>
#include <string.h>

PmXf pm_xf;
unsigned nd_mc_clears(void); /* nd_model.c */
void pm_log_prof_line(const char* fmt, ...); /* pm_log.c */

enum { XF_NONE, XF_DOOR, XF_MAP };
#define XF_TAIL 30u      /* steps after the end event */
#define XF_LIMIT 1800u   /* a transition that never ends prints after this many steps */
#define T34 9115808u     /* 34 ms in ticks */
static int xf_kind, xf_loaded, xf_drawn;
static unsigned xf_steps, xf_tail, xf_max, xf_fade, xf_o34, xf_init, xf_clr0;
static char xf_from[24], xf_to[24];
static int xf_last_area = -1, xf_last_map = -1; /* the map loaded last: the from of the next map change */

static void map_name(char* o, unsigned n, int a, int m) {
    if (a >= 0 && a < (int)ARRAY_COUNT(gAreas) && m >= 0 && m < gAreas[a].mapCount) snprintf(o, n, "%s", gAreas[a].maps[m].id);
    else snprintf(o, n, "%d:%d", a, m);
}
/* ticks to tenths of a ms: t / 26811.2 (cold, once per printed field) */
static unsigned ms10(unsigned t) { return (unsigned)((unsigned long long)t * 10u / 268112u); }
#define MS(t) ms10(t) / 10u, ms10(t) % 10u

__attribute__((cold, noinline)) static void xf_print(void) {
    unsigned clr = nd_mc_clears() - xf_clr0;
#define XF_REST "%s->%s f%u max %u.%u o34 %u | fade %u.%u load %u.%u rom %u/%u yay0 %u/%u/%u.%u inv %u au %u.%u init %u.%u | dlc %u/%u.%u mdl %u/%u ent %u tex %u/%u/%u.%u"
    pm_log_prof_line(xf_kind == XF_MAP ? "xfer map " XF_REST : "xfer door " XF_REST, xf_from, xf_to, xf_steps, MS(xf_max), xf_o34, MS(xf_fade), MS(pm_xf.load_t),
                     pm_xf.rom_n, pm_xf.rom_b >> 10, pm_xf.yay_n, pm_xf.yay_b >> 10, MS(pm_xf.yay_t), pm_xf.inv_n, MS(pm_xf.au_t), MS(xf_init),
                     pm_xf.dlc_n, MS(pm_xf.dlc_t), pm_xf.mdl_miss, clr, pm_xf.ent_n, pm_xf.tex_n, pm_xf.tex_b >> 10, MS(pm_xf.tex_t));
    xf_kind = XF_NONE;
    pm_xf.on = 0;
}
__attribute__((cold, noinline)) static void xf_begin(int kind) {
    if (xf_kind != XF_NONE) xf_print(); /* a door still in its tail: print it first */
    memset(&pm_xf, 0, sizeof pm_xf);
    pm_xf.on = 1;
    xf_kind = kind;
    xf_loaded = xf_drawn = 0;
    xf_steps = xf_tail = xf_max = xf_fade = xf_o34 = xf_init = 0;
    xf_clr0 = nd_mc_clears();
}
void pm_xfer_map_begin(void) {
    xf_begin(XF_MAP);
    map_name(xf_from, sizeof xf_from, xf_last_area, xf_last_map);
    snprintf(xf_to, sizeof xf_to, "?");
}
void pm_xfer_map_load(int area, int map) {
    xf_last_area = area; xf_last_map = map;
    if (xf_kind != XF_MAP) return; /* a load outside a map change (enter world, a battle return): no line */
    map_name(xf_to, sizeof xf_to, area, map);
    pm_xf.load_step = 1;
    xf_loaded = 1;
}
void pm_xfer_door(int state) {
    if (state != 0) {
        if (xf_kind == XF_MAP) return;
        char m[16];
        map_name(m, sizeof m, gGameStatusPtr->areaID, gGameStatusPtr->mapID);
        xf_begin(XF_DOOR);
        snprintf(xf_from, sizeof xf_from, "%s:%s", m, state == 1 ? "out" : "in");
        snprintf(xf_to, sizeof xf_to, "%s:%s", m, state == 1 ? "in" : "out");
    } else if (xf_kind == XF_DOOR && !xf_tail) xf_tail = XF_TAIL;
}
void pm_xfer_step(unsigned long long t0, unsigned long long t1) {
    if (xf_kind == XF_NONE) return;
    unsigned w = (unsigned)(t1 - t0);
    int ld = pm_xf.load_step;
    pm_xf.load_step = 0;
    xf_steps++;
    if (w > xf_max) xf_max = w;
    int init = 0;
    if (xf_kind == XF_MAP && xf_loaded && !xf_drawn && !ld && !(gOverrideFlags & GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME)) {
        xf_drawn = 1; init = 1; xf_init = w; xf_tail = XF_TAIL; /* the first step that draws the new map */
    }
    if (!ld && w > T34) xf_o34++;
    if (!ld && !init && w > xf_fade) xf_fade = w;
    if ((xf_tail && --xf_tail == 0) || xf_steps >= XF_LIMIT) xf_print();
}
