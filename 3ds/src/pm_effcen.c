/* EFFCEN: per-effect cost census (debug word effcen). Off: one cached word read per execute_render_tasks call, nothing else.
 * Per render task (model.c execute_render_tasks, TARGET_3DS): the Gfx words its appendGfx emits (gMainGfxPos delta) and the ticks
 * of that call. Keyed by the appendGfx function, which is the effect's render function (names: build3ds/pm_3ds.map).
 * Every 60 frames (pm_frame_count) one line: effcen f<n> top: <fn> e<effectID> f<active frames> t<tasks> ops<words> us<us> | ...
 * tasks, ops and us are per active frame of that key; the top 5 are ranked by the sum of ticks over the window.
 * Not in this census: walk-time cost (GPU draws, batch closes, walked ops). The walker runs after execute_render_tasks, so its
 * ops and closes stay in nd_cen.c (mdlsum lines). */
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "effects.h"
#include "pm_port.h"
#include "variables.h"

extern EffectInstance* gEffectInstances[96];

#define EC_KEYS 32
#define EC_TOP 5
#define EC_PERIOD 60u
#define EC_TICKS_US 268u

typedef struct { const void* fn; int eid; unsigned lastf, frames, tasks, words, ticks; } EcKey;
static EcKey ec_k[EC_KEYS];
static int ec_n, ec_over;
static unsigned ec_last_print = ~0u;

int pm_effcen_on(void) { return PMDL("effcen"); }
unsigned pm_effcen_t0(void) { return pm_ticks(); }

void pm_effcen_task(const void* fn, const void* arg, const Gfx* g0, unsigned t0) {
    unsigned f = pm_frame_count();
    unsigned ticks = pm_ticks() - t0;
    unsigned words = (unsigned)(gMainGfxPos - g0);
    int i, k;
    for (i = 0; i < ec_n && ec_k[i].fn != fn; i++) {}
    if (i == ec_n) {
        if (ec_n >= EC_KEYS) { ec_over++; return; }
        ec_k[i].fn = fn;
        ec_k[i].eid = -1;
        ec_k[i].lastf = ~0u;
        ec_k[i].frames = ec_k[i].tasks = ec_k[i].words = ec_k[i].ticks = 0;
        for (k = 0; k < 96; k++) { /* pointer match only: a live effect instance of this task */
            if (gEffectInstances[k] && (const void*)gEffectInstances[k] == arg) { ec_k[i].eid = gEffectInstances[k]->effectID; break; }
        }
        ec_n++;
    }
    if (ec_k[i].lastf != f) { ec_k[i].lastf = f; ec_k[i].frames++; }
    ec_k[i].tasks++;
    ec_k[i].words += words;
    ec_k[i].ticks += ticks;
}

void pm_effcen_frame(void) {
    unsigned f = pm_frame_count();
    char b[400];
    int n = 0, r, i;
    unsigned long long used = 0;
    if (f == ec_last_print || f % EC_PERIOD) return;
    ec_last_print = f;
    b[0] = 0;
    for (r = 0; r < EC_TOP; r++) {
        int best = -1;
        unsigned bt = 0;
        for (i = 0; i < ec_n; i++) {
            if (((used >> i) & 1u) == 0 && ec_k[i].ticks > bt) { bt = ec_k[i].ticks; best = i; }
        }
        if (best < 0) break;
        used |= 1ull << best;
        {
            EcKey* e = &ec_k[best];
            unsigned fr = e->frames ? e->frames : 1u;
            n += snprintf(b + n, sizeof b - n, "%08x e%d f%u t%u ops%u us%u | ", (unsigned)(uintptr_t)e->fn, e->eid, e->frames,
                          e->tasks / fr, e->words / fr, (e->ticks / EC_TICKS_US) / fr);
        }
    }
    pm_log_hw("effcen f%u top: %s keys %d over %d", f, b, ec_n, ec_over);
    ec_n = 0;
    ec_over = 0;
    memset(ec_k, 0, sizeof ec_k);
}
