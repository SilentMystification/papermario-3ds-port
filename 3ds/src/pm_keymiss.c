/* KEYMISS: census of the native model cache misses by key word (debug switch keymiss). Debug only: reads the cache key and the record words,
 * changes no draw. Words 0-27 of the key are record words (R0 w0 to R13 w1), 28-91 are walker words. Owners are the record (geo = rec[24], r0 = rec[0]).
 * A key miss (cls 0), a pre-list record miss (cls 1) and a refused hit (cls 2) are counted per owner; for cls 0 and 1 the full key (k->w) is compared
 * with the owner's last key and each differing key word is counted. For cls 2 k->w is not complete (rec_run builds only the compared words) and is never read. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"
#include "pm_port.h"

#define KM_N 32u
#define KM_W (NDMC_RW + NDMC_SW)

typedef struct KmOwn { uint32_t geo, r0, f; uint32_t w[KM_W]; uint16_t c[KM_W]; unsigned miss, fr, pre, ref; uint8_t used, hk; } KmOwn;

static KmOwn* km;
static unsigned km_g[KM_W], km_miss, km_new, km_pre, km_ref, km_f0, km_next = 300u;

static void km_print(unsigned f) {
    pm_log_evt("keymiss f%u frames %u miss %u new %u pre %u ref %u", f, f - km_f0, km_miss, km_new, km_pre, km_ref);
    char b[160]; unsigned p = 0; b[0] = 0;
    for (unsigned j = 0; j < KM_W && p < 140u; j++) if (km_g[j]) p += (unsigned)snprintf(b + p, sizeof b - p, " %u:%u", j, km_g[j]);
    pm_log("keymissw f%u%s", f, b);
    for (int n = 0; n < 8; n++) {
        KmOwn* best = NULL;
        for (unsigned i = 0; i < KM_N; i++) {
            KmOwn* o = &km[i];
            if (o->used && o->miss + o->ref > 0u && (!best || o->miss + o->ref > best->miss + best->ref)) best = o;
        }
        if (!best) break;
        uint16_t tc[KM_W];
        memcpy(tc, best->c, sizeof tc);
        char b2[64]; unsigned p2 = 0; b2[0] = 0;
        for (int t = 0; t < 4; t++) {
            unsigned jm = 0;
            for (unsigned j = 1; j < KM_W; j++) if (tc[j] > tc[jm]) jm = j;
            if (!tc[jm]) break;
            p2 += (unsigned)snprintf(b2 + p2, sizeof b2 - p2, " %u:%u", jm, (unsigned)tc[jm]);
            tc[jm] = 0;
        }
        pm_log("keymisso f%u geo %08x r0 %08x miss %u fr %u pre %u ref %u w%s", f, best->geo, best->r0, best->miss, best->fr, best->pre, best->ref, b2);
        best->miss = 0; best->ref = 0;
    }
    memset(km_g, 0, sizeof km_g);
    km_miss = km_new = km_pre = km_ref = 0;
    for (unsigned i = 0; i < KM_N; i++) {
        KmOwn* o = &km[i];
        memset(o->c, 0, sizeof o->c);
        o->miss = o->fr = o->pre = o->ref = 0;
    }
    km_f0 = f;
}

__attribute__((cold, noinline)) void pm_km_note(const uint32_t* rec, const NdMKey* k, int cls) {
    const unsigned f = pm_frame_count();
    if (!km) { km = calloc(KM_N, sizeof *km); if (!km) return; km_f0 = f; }
    if (f >= km_next) { km_print(f); while (km_next <= f) km_next += 300u; }
    const uint32_t geo = rec[24], r0 = rec[0];
    const unsigned h = ((geo >> 2) ^ (geo >> 11) ^ r0) & (KM_N - 1u);
    KmOwn* o = NULL;
    for (unsigned i = 0; i < KM_N; i++) {
        KmOwn* e = &km[(h + i) & (KM_N - 1u)];
        if (!e->used || (e->geo == geo && e->r0 == r0)) { o = e; break; }
    }
    if (!o) return;
    if (!o->used) { o->used = 1; o->geo = geo; o->r0 = r0; km_new++; }
    if (o->f != f) { o->fr++; o->f = f; }
    if (cls == 2) { o->ref++; km_ref++; return; }
    km_miss++; o->miss++;
    if (cls == 1) { o->pre++; km_pre++; }
    if (o->hk) for (unsigned j = 0; j < KM_W; j++) if (o->w[j] != k->w[j]) { o->c[j]++; km_g[j]++; }
    memcpy(o->w, k->w, sizeof o->w);
    o->hk = 1;
}
