/* MAPX-1: the transition line. One `xfer` line per door (room change, same map) or map change, printed with pm_log_prof_line (prof mode
 * and logging at warn or more). A map change starts at set_game_mode(GAME_MODE_CHANGE_MAP) (the fade out) and ends 30 steps after the
 * first step that draws the new map (GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME clear after the load). A door starts when its room state leaves
 * idle (rooms.c SetDoorState) and ends 30 steps after it is idle again.
 *   xfer <door|map> <from>-><to> f<steps> max <ms> o34 <n> | fade <ms> load <ms> rom <n>/<KB> yay0 <n>/<KB>/<ms> inv <n> au <ms> init <ms> pre <n>
 *   xfer2 <door|map> n<seq> | dlc <n>/<ms> mdl <miss>/<clr> ent <n> tex <n>/<KB>/<ms> warm <covered frames>/<skipped misses>
 *   xfer3 battle wmax <ms> wo34 <n> song <ms> bks <n>/<ms> dec <n>/<ms>/<maxms> dr <n>/<ms> capcpu <n> pdec <built>/<in>/<drop>
 * A battle entry (kind battle, <map>->btl:<id>) starts at the battle song push and ends 30 steps after the first drawn battle step; its
 * load is load_battle_section. wmax, wo34: the steps before the load (the star wipe). song: snd_song_load. bks: instrument bank loads.
 * dec: VADPCM decodes (render thread), sum and longest. dr: walk drains that waited. capcpu: CPU reads of the colour buffer. pdec: samples
 * pre-decoded on core 1 (pm_prefetch_song): built, put in the cache, dropped.
 * max: the longest step (work ticks, all steps). o34: steps over 34 ms, the load step not counted. fade: the longest step that is neither
 * the load step nor the first drawn step. load: the load_map_by_IDs call. init: the first drawn step of the new map. The stage counters
 * are in pm_xf (pm_port.h); the stage sites read a tick only while a transition is open. Cold code: once per step one load and a branch. */
#include "common.h"
#include "game_modes.h"
#include "map.h"
#include "pm_port.h"
#include <stdio.h>
#include <string.h>
unsigned pm_frame_count(void);

PmXf pm_xf;
unsigned nd_mc_clears(void); /* nd_model.c */
unsigned nd_mc_reps(void); /* nd_model.c, MAPX-2 */
unsigned nd_mc_trims(void); /* nd_model.c, MAPX-2 */
void pm_log_prof_line(const char* fmt, ...); /* pm_log.c */

enum { XF_NONE, XF_DOOR, XF_MAP, XF_BTL };
static const char* const xf_kname[] = { "none", "door", "map", "battle" };
void pm_dspa_dec_mark(unsigned* n, unsigned* us);                /* pm_dspaud.c, BTLWIPE */
void pm_dspa_dec_read(unsigned* n, unsigned* us, unsigned* max); /* pm_dspaud.c, BTLWIPE */
void pm_dspa_pre_read(unsigned* built, unsigned* in, unsigned* drop); /* pm_dspaud.c, BTLWIPE F4 */
#define XF_TAIL 30u      /* steps after the end event */
#define XF_LIMIT 1800u   /* a transition that never ends prints after this many steps */
#define T34 9115808u     /* 34 ms in ticks */
static int xf_kind, xf_loaded, xf_drawn;
static unsigned xf_seq; /* transitions printed since boot (the n field) */
static unsigned xf_steps, xf_tail, xf_max, xf_fade, xf_o34, xf_init, xf_clr0, xf_rep0, xf_trim0;
static unsigned xf_wmax, xf_wo34, xf_dec_n0, xf_dec_us0, xf_pre0[3]; /* BTLWIPE: the steps before the load (the star wipe): longest, over 34 ms; decode counts at the start */
static char xf_from[24], xf_to[24];
static int xf_last_area = -1, xf_last_map = -1; /* the map loaded last: the from of the next map change */

static void map_name(char* o, unsigned n, int a, int m) {
    if (a >= 0 && a < (int)ARRAY_COUNT(gAreas) && m >= 0 && m < gAreas[a].mapCount) snprintf(o, n, "%s", gAreas[a].maps[m].id);
    else snprintf(o, n, "%d:%d", a, m);
}
/* ticks to "ms.tenth" (t / 26811.2; cold, once per printed field). The log slot takes 16 argument words and 160 bytes of %s text, so the line is two
 * lines (xfer, xfer2) and each ms value is a short string. */
static const char* msf(char* o, unsigned t) {
    unsigned v = (unsigned)((unsigned long long)t * 10u / 268112u);
    snprintf(o, 12, "%u.%u", v / 10u, v % 10u);
    return o;
}

__attribute__((cold, noinline)) static void xf_print(void) {
    unsigned clr = nd_mc_clears() - xf_clr0;
    char m[9][12];
#define XF_L1 "%s->%s f%u max %s o34 %u | fade %s load %s rom %u/%u yay0 %u/%u/%s inv %u au %s init %s pre %u"
#define XF_L2 " | dlc %u/%s mdl %u/%u ent %u tex %u/%u/%s warm %u/%u"
    const char* kn = xf_kname[xf_kind];
    pm_log_prof_line(xf_kind == XF_MAP ? "xfer map " XF_L1 : xf_kind == XF_BTL ? "xfer battle " XF_L1 : "xfer door " XF_L1, xf_from, xf_to, xf_steps, msf(m[0], xf_max), xf_o34, msf(m[1], xf_fade),
                     msf(m[2], pm_xf.load_t), pm_xf.rom_n, pm_xf.rom_b >> 10, pm_xf.yay_n, pm_xf.yay_b >> 10, msf(m[3], pm_xf.yay_t), pm_xf.inv_n,
                     msf(m[4], pm_xf.au_t), msf(m[5], xf_init), pm_xf.yay_pre);
    pm_log_prof_line("xfer2 %s n%u" XF_L2, kn, ++xf_seq, pm_xf.dlc_n, msf(m[6], pm_xf.dlc_t), pm_xf.mdl_miss, clr,
                     pm_xf.ent_n, pm_xf.tex_n, pm_xf.tex_b >> 10, msf(m[7], pm_xf.tex_t), pm_xf.warm_f, pm_xf.warm_skip);
    if (xf_kind == XF_BTL) { /* BTLWIPE: the wipe steps and the audio and drain stages of a battle entry */
        unsigned dn, dus, dmax, pb, pi, pd;
        pm_dspa_dec_read(&dn, &dus, &dmax);
        pm_dspa_pre_read(&pb, &pi, &pd);
        dus -= xf_dec_us0;
        pm_log_prof_line("xfer3 battle wmax %s wo34 %u song %s bks %u/%s dec %u/%u.%u/%u.%u dr %u/%s capcpu %u pdec %u/%u/%u", msf(m[0], xf_wmax), xf_wo34,
                         msf(m[1], pm_xf.song_t), pm_xf.bk_n, msf(m[2], pm_xf.bk_t), dn - xf_dec_n0, dus / 1000u, (dus % 1000u) / 100u, dmax / 1000u,
                         (dmax % 1000u) / 100u, pm_xf.dr_n, msf(m[3], pm_xf.dr_t), pm_xf.cap_cpu,
                         pb - xf_pre0[0], pi - xf_pre0[1], pd - xf_pre0[2]);
    }
    if (pm_xf.diag) { /* MAPX-2 switch xferdiag: the 6 key words that most misses differ in first (92: no stored entry of the model) */
        char b[160]; unsigned n = 0, used[93] = { 0 };
        for (int r = 0; r < 6; r++) {
            unsigned bj = 93u, bc = 0;
            for (unsigned j = 0; j < 93u; j++) if (!used[j] && pm_xf.wh[j] > bc) { bc = pm_xf.wh[j]; bj = j; }
            if (bj == 93u) break;
            used[bj] = 1;
            { extern uint32_t nd_mc_xbits[92]; n += (unsigned)snprintf(b + n, sizeof b - n, " w%u:%u:%x", bj, bc, bj < 92u ? (unsigned)nd_mc_xbits[bj] : 0u); if (bj < 92u) nd_mc_xbits[bj] = 0; }
        }
        b[n] = 0;
        pm_log_prof_line("xferk %s miss %u rep %u trim %u first%s", kn, pm_xf.mdl_miss, nd_mc_reps() - xf_rep0, nd_mc_trims() - xf_trim0, b);
    }
    xf_kind = XF_NONE;
    pm_xf.on = 0;
}
__attribute__((cold, noinline)) static void xf_begin(int kind) {
    if (xf_kind != XF_NONE) xf_print(); /* a door still in its tail: print it first */
    memset(&pm_xf, 0, sizeof pm_xf);
    pm_xf.on = 1;
    pm_xf.diag = pm_debug_has("xferdiag");
    xf_kind = kind;
    xf_loaded = xf_drawn = 0;
    xf_steps = xf_tail = xf_max = xf_fade = xf_o34 = xf_init = xf_wmax = xf_wo34 = 0;
    pm_dspa_dec_mark(&xf_dec_n0, &xf_dec_us0);
    pm_dspa_pre_read(&xf_pre0[0], &xf_pre0[1], &xf_pre0[2]);
    xf_clr0 = nd_mc_clears(); xf_rep0 = nd_mc_reps(); xf_trim0 = nd_mc_trims();
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
/* BTLWIPE: a battle entry. Starts at the battle song push of PRE_BATTLE_INIT (the star wipe starts there), the load is load_battle_section,
 * and it ends 30 steps after the first step that draws the battle. */
void pm_xfer_btl_begin(void) {
    xf_begin(XF_BTL);
    map_name(xf_from, sizeof xf_from, gGameStatusPtr->areaID, gGameStatusPtr->mapID);
    snprintf(xf_to, sizeof xf_to, "btl:?");
}
void pm_xfer_btl_load(unsigned ticks) {
    if (xf_kind != XF_BTL) return;
    snprintf(xf_to, sizeof xf_to, "btl:%04x", (unsigned)gCurrentBattleID & 0xFFFFu);
    pm_xf.load_t += ticks;
    pm_xf.load_step = 1;
    xf_loaded = 1;
}
void pm_xfer_door(int state) {
    if (state != 0) {
        if (xf_kind == XF_MAP || xf_kind == XF_BTL) return;
        char m[16];
        map_name(m, sizeof m, gGameStatusPtr->areaID, gGameStatusPtr->mapID);
        xf_begin(XF_DOOR);
        snprintf(xf_from, sizeof xf_from, "%s:%s", m, state == 1 ? "out" : "in");
        snprintf(xf_to, sizeof xf_to, "%s:%s", m, state == 1 ? "in" : "out");
    } else if (xf_kind == XF_DOOR && !xf_tail) xf_tail = XF_TAIL;
}
/* MAPX-3: warm frames. After the map load the first drawn frames lie under the opaque front overlay of the fade (the frame that ends AWAIT_MAIN
 * and the first FADE_IN frame: type screen or viewport colour, progress 255). There the walk builds the model cache with a count budget per frame;
 * a model record miss over the budget draws nothing (it is not seen) and builds in the next frame. The first frame with the overlay below 255 builds
 * the rest. Before, the whole first frame of the new map built every list, model and texture (one long frame); now that work spreads over the
 * covered frames. Deterministic: a count, no time. Called on a miss only. */
#define XF_WARM_MDL 64u /* model record misses built per covered frame */
static unsigned xw_f = ~0u, xw_n; static int xw_cov;
int pm_xfer_skip(int kind) {
    (void)kind;
    if (xf_kind != XF_MAP || !xf_loaded) return 0;
    unsigned f = pm_frame_count();
    if (f != xw_f) {
        u8 t; f32 z;
        xw_f = f; xw_n = 0;
        get_screen_overlay_params(SCREEN_LAYER_FRONT, &t, &z);
        xw_cov = (t == OVERLAY_SCREEN_COLOR || t == OVERLAY_VIEWPORT_COLOR) && z >= 255.0f;
        if (xw_cov) pm_xf.warm_f++;
    }
    if (!xw_cov) return 0;
    if (++xw_n <= XF_WARM_MDL) return 0;
    pm_xf.warm_skip++;
    return 1;
}
void pm_xfer_step(unsigned long long t0, unsigned long long t1) {
    if (xf_kind == XF_NONE) return;
    unsigned w = (unsigned)(t1 - t0);
    int ld = pm_xf.load_step;
    pm_xf.load_step = 0;
    xf_steps++;
    if (w > xf_max) xf_max = w;
    int init = 0;
    if (xf_kind == XF_BTL && !xf_loaded && !ld) { /* the star wipe steps */
        if (w > xf_wmax) xf_wmax = w;
        if (w > T34) xf_wo34++;
    }
    if ((xf_kind == XF_MAP || xf_kind == XF_BTL) && xf_loaded && !xf_drawn && !ld && !(gOverrideFlags & GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME)) {
        xf_drawn = 1; init = 1; xf_init = w; xf_tail = XF_TAIL; /* the first step that draws the new map */
    }
    if (!ld && w > T34) xf_o34++;
    if (!ld && !init && w > xf_fade) xf_fade = w;
    if ((xf_tail && --xf_tail == 0) || xf_steps >= XF_LIMIT) xf_print();
}
