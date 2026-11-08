/* CORE2-2 census (switch c2haz): which game memory read by the walk of frame N does the game rewrite before the walk of frame N+1?
 * A walker thread on N3DS core 2 (plan_core2.md) walks frame N while the game thread runs the logic and the list build of frame N+1,
 * so every such range is a hazard: it must be double-buffered, copied into the task, or written only after the walker is done with it.
 *
 * Record: every pointer the walker resolves (ptr_of, nd_walk.c: G_VTX, G_MTX, viewport, lights, G_SETTIMG, segments, G_DL and the
 * native readers) goes into a table with the FNV-1a of its first C2_LEN bytes and the op that resolved it.
 * Check: at the first walked task of the next drawn frame (pm_gbi_run, after the logic and the list build of that frame) every entry is
 * hashed again. A changed entry is a hazard: one "c2haz new" line per address (the first 256 addresses), with the op. A summary line every
 * 60 checked frames. A frame whose scene (area, map, mode) differs from the recorded one is not checked (a map load rewrites everything;
 * the threaded walker drains the queue there).
 * Producer records (G_PM_REC, G_PM_SPR, G_PM_ENT) are scanned for host addresses (nd_c2_rec).
 * Limits: only the first C2_LEN bytes are hashed (a buffer rewritten past that is missed); reads of the native caches (model, entity,
 * texture) after their first build are not seen. Cold file, debug only. */
#include <string.h>
#include "pm_port.h"

int is_host(unsigned p); /* nd_walk.c */
extern unsigned pm_gbi_cur_op;
void pm_ph_scene_get(int* mode, int* area, int* map); /* pm_phase.c */

#define C2_N 4096u /* table entries (power of two) */
#define C2_LEN 64u /* bytes hashed per pointer */
#define C2_SEEN 256u /* addresses reported */
typedef struct { unsigned p, h; unsigned short op, used; } C2Ent;
static C2Ent c2_tab[C2_N];
static unsigned c2_n, c2_over, c2_frame = ~0u, c2_sc[3], c2_seen[C2_SEEN], c2_seen_n;
static unsigned c2_frames, c2_checked, c2_changed, c2_new;
int nd_c2_on = -1;

static unsigned c2_hash(const unsigned char* p) {
    unsigned h = 2166136261u;
    for (unsigned i = 0; i < C2_LEN; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}

__attribute__((cold, noinline)) void nd_c2_note(const void* ptr) {
    unsigned p = (unsigned)(uintptr_t)ptr;
    if (!p || !is_host(p) || !is_host(p + C2_LEN - 1)) return;
    unsigned i = ((p >> 3) * 2654435761u) >> 20; /* 12 bits: C2_N */
    for (unsigned k = 0; k < 16; k++, i = (i + 1) & (C2_N - 1)) {
        C2Ent* e = &c2_tab[i];
        if (e->used && e->p == p) return; /* first read of the frame wins: the walk reads it as it was then */
        if (!e->used) {
            e->used = 1; e->p = p; e->op = (unsigned short)pm_gbi_cur_op; e->h = c2_hash((const unsigned char*)ptr);
            c2_n++;
            return;
        }
    }
    c2_over++;
}

static int c2_seen_add(unsigned p) {
    for (unsigned i = 0; i < c2_seen_n; i++) if (c2_seen[i] == p) return 0;
    if (c2_seen_n < C2_SEEN) c2_seen[c2_seen_n++] = p;
    return 1;
}

/* Producer records (G_PM_REC, G_PM_SPR, G_PM_ENT): the walker reads their pointer words without ptr_of. Every word that is a host
 * address is noted (raster, palette, matrix, vertices, lists). */
__attribute__((cold, noinline)) void nd_c2_rec(const void* rec, unsigned words) {
    const unsigned* w = (const unsigned*)rec;
    for (unsigned i = 0; i < words * 2u; i++) if (w[i] >= 0x00100000u && is_host(w[i])) nd_c2_note((const void*)(uintptr_t)w[i]);
}

/* The first walked task of a drawn frame: check the entries of the last recorded frame, then record this one. */
__attribute__((cold, noinline)) void nd_c2_task(const void* list) {
    unsigned f = pm_frame_count();
    if (f == c2_frame) { nd_c2_note(list); return; }
    int sc[3];
    pm_ph_scene_get(&sc[0], &sc[1], &sc[2]);
    if (c2_frame != ~0u && (unsigned)sc[0] == c2_sc[0] && (unsigned)sc[1] == c2_sc[1] && (unsigned)sc[2] == c2_sc[2]) {
        unsigned ch = 0;
        for (unsigned i = 0; i < C2_N; i++) {
            C2Ent* e = &c2_tab[i];
            if (!e->used) continue;
            c2_checked++;
            if (c2_hash((const unsigned char*)(uintptr_t)e->p) == e->h) continue;
            ch++;
            if (c2_seen_add(e->p)) { c2_new++; pm_log("c2haz new f%u p %08x op %02x", f, e->p, e->op); }
        }
        c2_changed += ch;
        if (++c2_frames % 60u == 0)
            pm_log("c2haz sum f%u frames %u entries %u changed %u per frame, addresses %u, overflow %u", f, c2_frames, c2_n, c2_changed / 60u, c2_seen_n, c2_over);
        if (c2_frames % 60u == 0) c2_changed = 0;
    }
    memset(c2_tab, 0, sizeof c2_tab);
    c2_n = 0;
    c2_frame = f;
    c2_sc[0] = (unsigned)sc[0]; c2_sc[1] = (unsigned)sc[1]; c2_sc[2] = (unsigned)sc[2];
    nd_c2_note(list); /* the task list itself (main or background list) */
}

int nd_c2_init(void) { if (nd_c2_on < 0) nd_c2_on = pm_debug_has("c2haz"); return nd_c2_on; }
