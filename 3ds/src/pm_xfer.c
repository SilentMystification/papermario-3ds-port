/* MAPX-1: the transition line. One `xfer` line per door (room change, same map) or map change, printed with pm_log_prof_line (prof mode
 * and logging at warn or more). A map change starts at set_game_mode(GAME_MODE_CHANGE_MAP) (the fade out) and ends 30 steps after the
 * first step that draws the new map (GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME clear after the load). A door starts when its room state leaves
 * idle (rooms.c SetDoorState) and ends 30 steps after it is idle again.
 *   xfer <door|map> <from>-><to> f<steps> max <ms> o34 <n> | fade <ms> load <ms> rom <n>/<KB> yay0 <n>/<KB>/<ms> inv <n> au <ms> init <ms> pre <n>
 *   xfer2 <door|map> n<seq> | dlc <n>/<ms> mdl <miss>/<clr> ent <n> tex <n>/<KB>/<ms> warm <covered frames>/<skipped misses>
 * max: the longest step (work ticks, all steps). o34: steps over 34 ms, the load step not counted. fade: the longest step that is neither
 * the load step nor the first drawn step. load: the load_map_by_IDs call. init: the first drawn step of the new map. The stage counters
 * are in pm_xf (pm_port.h); the stage sites read a tick only while a transition is open. Cold code: once per step one load and a branch. */
#include "common.h"
#include "game_modes.h"
#include "map.h"
#include "model.h"
#include "pm_port.h"
#include <stdio.h>
#include <string.h>
unsigned pm_frame_count(void);

PmXf pm_xf;
unsigned nd_mc_clears(void); /* nd_model.c */
unsigned nd_mc_reps(void); /* nd_model.c, MAPX-2 */
unsigned nd_mc_trims(void); /* nd_model.c, MAPX-2 */
void nd_mc_fill(unsigned* ent, unsigned* pool); /* nd_model.c, DOORHITCH */
void pm_log_prof_line(const char* fmt, ...); /* pm_log.c */

enum { XF_NONE, XF_DOOR, XF_MAP };
#define XF_TAIL 30u      /* steps after the end event */
#define XF_LIMIT 1800u   /* a transition that never ends prints after this many steps */
#define T34 9115808u     /* 34 ms in ticks */
static int xf_kind, xf_loaded, xf_drawn;
static unsigned xf_seq; /* transitions printed since boot (the n field) */
static unsigned xf_steps, xf_tail, xf_max, xf_fade, xf_o34, xf_init, xf_clr0, xf_rep0, xf_trim0;
static char xf_from[24], xf_to[24];
/* DOORHITCH: the stage counts of the longest step (xfer3 maxstep): model misses, refused hits, texture and dlc ticks, ROM reads, prof sample.
 * xs_p: the counters at the end of the step before; xs_m: the deltas of the longest step. ehi, phi: the most live cache entries and pool draws at a step end. */
typedef struct { unsigned mdl, rng, tex_t, dlc_t, rom, smp; } XfSnap;
static XfSnap xs_p, xs_m;
static unsigned xs_ehi, xs_phi;
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
    pm_log_prof_line(xf_kind == XF_MAP ? "xfer map " XF_L1 : "xfer door " XF_L1, xf_from, xf_to, xf_steps, msf(m[0], xf_max), xf_o34, msf(m[1], xf_fade),
                     msf(m[2], pm_xf.load_t), pm_xf.rom_n, pm_xf.rom_b >> 10, pm_xf.yay_n, pm_xf.yay_b >> 10, msf(m[3], pm_xf.yay_t), pm_xf.inv_n,
                     msf(m[4], pm_xf.au_t), msf(m[5], xf_init), pm_xf.yay_pre);
    pm_log_prof_line(xf_kind == XF_MAP ? "xfer2 map n%u" XF_L2 : "xfer2 door n%u" XF_L2, ++xf_seq, pm_xf.dlc_n, msf(m[6], pm_xf.dlc_t), pm_xf.mdl_miss, clr,
                     pm_xf.ent_n, pm_xf.tex_n, pm_xf.tex_b >> 10, msf(m[7], pm_xf.tex_t), pm_xf.warm_f, pm_xf.warm_skip);
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
        pm_log_prof_line("xferk %s miss %u rep %u trim %u first%s", xf_kind == XF_MAP ? "map" : "door", pm_xf.mdl_miss, nd_mc_reps() - xf_rep0, nd_mc_trims() - xf_trim0, b);
    }
    /* DOORHITCH: the longest step by stage, and the cache fill over the transition (after xferk: the test until-line) */
    pm_log_prof_line(xf_kind == XF_MAP ? "xfer3 map n%u | maxstep mdl %u rng %u tex %s dlc %s rom %u smp %u | rng %u trim %u phi %u ehi %u rdw %s hid %u/%u" : "xfer3 door n%u | maxstep mdl %u rng %u tex %s dlc %s rom %u smp %u | rng %u trim %u phi %u ehi %u rdw %s hid %u/%u",
                     xf_seq, xs_m.mdl, xs_m.rng, msf(m[6], xs_m.tex_t), msf(m[7], xs_m.dlc_t), xs_m.rom, xs_m.smp, pm_xf.mdl_rng, nd_mc_trims() - xf_trim0, xs_phi, xs_ehi, msf(m[8], pm_xf.rdw_t), pm_xf.hid_d, pm_xf.hid_t);
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
    xf_steps = xf_tail = xf_max = xf_fade = xf_o34 = xf_init = 0;
    xf_clr0 = nd_mc_clears(); xf_rep0 = nd_mc_reps(); xf_trim0 = nd_mc_trims();
    memset(&xs_p, 0, sizeof xs_p); memset(&xs_m, 0, sizeof xs_m); xs_ehi = xs_phi = 0;
}
void pm_xfer_map_begin(void) {
    xf_begin(XF_MAP);
    map_name(xf_from, sizeof xf_from, xf_last_area, xf_last_map);
    snprintf(xf_to, sizeof xf_to, "?");
}
static int xh_on; /* DOORHITCH step 3: armed by a map load (2 in the load step, then 1 until the first frame not covered) */
void pm_xfer_map_load(int area, int map) {
    xf_last_area = area; xf_last_map = map;
    xh_on = 2; /* 2: the load step itself (the map scripts have not hidden the room models yet) */
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
    {   /* DOORHITCH: the deltas of this step; the longest step keeps its own */
        XfSnap c = { pm_xf.mdl_miss, pm_xf.mdl_rng, pm_xf.tex_t, pm_xf.dlc_t, pm_xf.rom_n, 0 };
        if (w > xf_max) { xs_m.mdl = c.mdl - xs_p.mdl; xs_m.rng = c.rng - xs_p.rng; xs_m.tex_t = c.tex_t - xs_p.tex_t; xs_m.dlc_t = c.dlc_t - xs_p.dlc_t; xs_m.rom = c.rom - xs_p.rom; xs_m.smp = pm_xf.step_smp != 0; }
        xs_p = c;
        unsigned en, pn; nd_mc_fill(&en, &pn);
        if (en > xs_ehi) xs_ehi = en;
        if (pn > xs_phi) xs_phi = pn;
    }
    if (w > xf_max) xf_max = w;
    int init = 0;
    if (xf_kind == XF_MAP && xf_loaded && !xf_drawn && !ld && !(gOverrideFlags & GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME)) {
        xf_drawn = 1; init = 1; xf_init = w; xf_tail = XF_TAIL; /* the first step that draws the new map */
    }
    if (!ld && w > T34) xf_o34++;
    if (!ld && !init && w > xf_fade) xf_fade = w;
    if ((xf_tail && --xf_tail == 0) || xf_steps >= XF_LIMIT) xf_print();
}
/* DOORHITCH step 3: the first entry of a room. Its models (the house interior on kmr_20) are hidden by the room scripts after the load and first
 * drawn in the door, where their compiled lists and textures were built inside the door steps (O3DS kmr_20: 17 textures, 326 KB, 51 ms). After a
 * map load the logic thread posts each hidden model (list, texture handle) to the walker, which builds them after the frame's walk
 * (pm_gpu_warm_model): XH_STEP per frame under the covered frames of the load (the overlay at 255, as pm_xfer_skip), then one per drawn frame once
 * the map line has ended and no door is open, until a scan finds no new hidden model. The covered frames alone do not reach the room models:
 * kmr_20 creates its room (CreateMapRoom) in the second covered frame and hides the interior after it. Deterministic: a count, no time. */
#define XH_STEP 12u
static uint32_t xh_done[MAX_MODELS / 32]; static unsigned xh_n; /* models posted since the load: by index, the count */
static void wq_xh(void* a) { const uintptr_t* q = (const uintptr_t*)a; void pm_gpu_warm_model(const void* dl, int ti, const void* gfx); pm_gpu_warm_model((const void*)q[0], (int)q[1], (const void*)q[2]); }
void pm_xfer_hidden(void) {
    if (!xh_on) return;
    if (xh_on == 2 || (gOverrideFlags & GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME)) { xh_on = 1; memset(xh_done, 0, sizeof xh_done); xh_n = 0; return; }
    u8 t; f32 z;
    get_screen_overlay_params(SCREEN_LAYER_FRONT, &t, &z);
    const int cov = (t == OVERLAY_SCREEN_COLOR || t == OVERLAY_VIEWPORT_COLOR) && z >= 255.0f;
    if (!cov && xf_kind != XF_NONE) return; /* the fade-in of a map change or a door: those frames build their own models */
    extern TextureHandle TextureHandles[];
    const unsigned lim = cov ? XH_STEP : 1u;
    unsigned n = 0;
    for (unsigned i = 0; i < MAX_MODELS && n < lim; i++) {
        Model* m = (*gCurrentModels)[i];
        if (!m || !(m->flags & MODEL_FLAG_HIDDEN) || (m->flags & (MODEL_FLAG_INACTIVE | MODEL_FLAG_HAS_LOCAL_VERTEX_COPY))) continue;
        if (!m->modelNode || !m->modelNode->displayData || (xh_done[i >> 5] & (1u << (i & 31u)))) continue;
        xh_done[i >> 5] |= 1u << (i & 31u);
        uintptr_t q[3];
        q[0] = (uintptr_t)m->modelNode->displayData->displayList;
        q[1] = m->textureID ? (uintptr_t)(m->textureID + m->textureVariation) : (uintptr_t)-1;
        q[2] = m->textureID ? (uintptr_t)TextureHandles[m->textureID + m->textureVariation].gfx : 0u;
        pm_wq_post(wq_xh, q, sizeof q);
        n++;
    }
    xh_n += n;
    if (!n && !cov) { xh_on = 0; pm_log_d("xfer hidden f%u posted %u", pm_frame_count(), xh_n); }
}
