/* nd_gtab_test.c: the host compare of the native list tables (S7CONV design note 8, nd_host). Linked with the host objects of the converted
 * set (gconv_in.o: the lists and the symbol map), the generated tables (nd_gfx_tab.c), nd_gtab.c and nd_scomp.c (ND_SCOMP_HOST). For every
 * table: (a) the old reader (parts_ok and the nd_part_draw list walk as they were before S7CONV) against the table's parts flag, loads and
 * triangles; (b) the ops cover every unit of the list once in order; (c) each def equals nd_scomp_analyse of its units, with the inherited
 * mask rule; (d) nd_gtab_find gives each table. Any difference fails. */
#include <stdio.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_op.h"
#include "nd_scomp.h"
#include "nd_gtab.h"
#include "nd_gconv.h"

static unsigned bad;
static const char* cur;
static void fail(const char* what, unsigned k) {
    if (bad < 40u) printf("FAIL gtab %s unit %u: %s\n", cur, k, what);
    bad++;
}
static int mapped_vtx(uint32_t p) {
    for (unsigned i = 0; i < gconv_nsym; i++)
        if (gconv_sym[i].kind == GS_VTX && p >= (uintptr_t)gconv_sym[i].addr && p < (uintptr_t)gconv_sym[i].end) return 1;
    return 0;
}

/* ---- the old reader (nd_rec.c parts_ok and nd_geom.c nd_part_draw before S7CONV) ---- */
static int tri_ok(uint32_t t, unsigned n) {
    return ((t >> 16) & 0xffu) >> 1 < n && ((t >> 8) & 0xffu) >> 1 < n && (t & 0xffu) >> 1 < n;
}
static int parts_ok(const uint32_t* l) {
    if ((l[0] >> 24) != NDO_VTX) return 0;
    const unsigned n = (l[0] >> 12) & 0xffu;
    if (n < 1u || n > 64u || ((l[0] >> 1) & 0x7fu) != n) return 0;
    unsigned have = n;
    for (unsigned k = 1u; k <= 64u; k++) {
        const uint32_t w0 = l[2u * k], w1 = l[2u * k + 1u];
        if ((w0 >> 24) == NDO_END) return 1;
        if (w0 == ND_NOP_A) continue;
        if ((w0 >> 24) == NDO_VTX) {
            const unsigned n2 = (w0 >> 12) & 0xffu, e2 = (w0 >> 1) & 0x7fu;
            if (n2 < 1u || e2 > 64u || n2 > e2 || e2 - n2 > have) return 0;
            if (e2 > have) have = e2;
            continue;
        }
        if ((w0 >> 24) == NDO_TRI1) { if (!tri_ok(w0 & 0xffffffu, have)) return 0; continue; }
        if ((w0 >> 24) == NDO_TRI2) { if (!tri_ok(w0 & 0xffffffu, have) || !tri_ok(w1, have)) return 0; continue; }
        return 0;
    }
    return 0;
}
typedef struct { uint32_t src; unsigned n, v0; unsigned t[3]; int is_tri; } Draw;
/* The nd_part_draw walk: the loads (with the source checks of its first pass) and the triangle triples in order; -1 = the walk refuses. */
static int old_walk(const uint32_t* u, Draw* d, unsigned* nd, unsigned* tot) {
    const unsigned n = (u[0] >> 12) & 0xffu;
    if (!mapped_vtx(u[1]) || n < 1u || n > 64u) return -1;
    *tot = n;
    for (const uint32_t* q = u + 2; (q[0] >> 24) != NDO_END; q += 2) {
        if ((q[0] >> 24) != NDO_VTX) continue;
        const unsigned n2 = (q[0] >> 12) & 0xffu;
        if (!mapped_vtx(q[1]) || n2 < 1u || n2 > 64u) return -1;
        *tot += n2;
    }
    unsigned m = 0;
    d[m].is_tri = 0; d[m].src = u[1]; d[m].n = n; d[m].v0 = 0; m++;
    for (const uint32_t* q = u + 2; (q[0] >> 24) != NDO_END; q += 2) {
        if (q[0] == ND_NOP_A) continue;
        if ((q[0] >> 24) == NDO_VTX) {
            const unsigned n2 = (q[0] >> 12) & 0xffu, v0 = ((q[0] >> 1) & 0x7fu) - n2;
            d[m].is_tri = 0; d[m].src = q[1]; d[m].n = n2; d[m].v0 = v0; m++;
            continue;
        }
        const int two = (q[0] >> 24) == NDO_TRI2;
        for (int t = 0; t <= two; t++) {
            const uint32_t w = t ? q[1] : (q[0] & 0xffffffu);
            d[m].is_tri = 1; d[m].t[0] = ((w >> 16) & 0xffu) >> 1; d[m].t[1] = ((w >> 8) & 0xffu) >> 1; d[m].t[2] = (w & 0xffu) >> 1; m++;
        }
    }
    *nd = m;
    return 0;
}

static uint32_t inherit_rule(const NdSDef* d) { /* design note 6 */
    const uint32_t* u = (const uint32_t*)d->u;
    uint32_t m = NDG_IN_KEY;
    unsigned first = d->n;
    for (unsigned k = 0; k < d->n; k++) if (u[2u * k] != ND_NOP_A && !(d->vmask >> k & 1u)) { first = k; break; }
    int prim = 0, env = 0;
    for (unsigned k = 0; k < first; k++)
        if (d->vmask >> k & 1u) { prim |= (u[2u * k] >> 24) == NDO_PRIM; env |= (u[2u * k] >> 24) == NDO_ENV; }
    const uint32_t cyc = 3u << G_MDSFT_CYCLETYPE;
    if (!d->has_comb) m |= NDG_IN_COMB;
    if ((d->omh_m & cyc) != cyc) m |= NDG_IN_CYC;
    if (!d->has_tex) m |= NDG_IN_TEXON;
    if (!prim) m |= NDG_IN_PRIM;
    if (!env) m |= NDG_IN_ENV;
    return m;
}

static unsigned n_parts, n_defs, n_ops, n_draws;
static void check_tab(const char* name, const NdGTab* t, unsigned nu) {
    const uint32_t* L = t->list;
    cur = name;
    /* (b) round trip */
    unsigned k = 0, i = 0, nvtx = 0, ntri = 0, ended = 0;
    while (i < t->nop) {
        const NdGOp* o = &t->op[i];
        if (k < nu && L[2u * k] == ND_NOP_A) { k++; continue; } /* a def never starts with a sync (the tool skips it first) */
        if (o->k == NDG_END) {
            if (i != t->nop - 1u) fail("END is not the last op", k);
            if (k < nu && (L[2u * k] >> 24) != NDO_END) fail("END op on another unit", k);
            ended = 1; i++; break;
        }
        if (k >= nu) { fail("op after the list end", k); return; }
        const uint32_t w0 = L[2u * k], w1 = L[2u * k + 1u];
        const unsigned id = w0 >> 24;
        switch (o->k) {
        case NDG_DEF: {
            const NdGDef* g = (const NdGDef*)o->p;
            if (g->k0 != k || g->d.u != (const void*)(L + 2u * k) || g->d.n != o->a) fail("def range", k);
            /* (c) the def against nd_scomp_analyse */
            NdSDef d = g->d;
            d.st = 0;
            if (!nd_scomp_analyse(&d)) fail("def refused by nd_scomp_analyse", k);
            d.st = 1; d.id = g->d.id;
            if (memcmp(&d, &g->d, sizeof d)) fail("def fields differ from nd_scomp_analyse", k);
            if (g->inherit != inherit_rule(&g->d)) fail("def inherit mask", k);
            if (g->ctx != (NDG_CTX_T1V | NDG_CTX_DUAL | NDG_CTX_VPRIM)) fail("def ctx", k);
            for (unsigned j = 0; j < g->d.n; j++) {
                const unsigned jd = L[2u * (k + j)] >> 24;
                if (jd == NDO_VTX || jd == NDO_TRI1 || jd == NDO_TRI2 || jd == NDO_CALL || jd == NDO_END) fail("draw unit inside a def", k + j);
            }
            printf("gtab def %s+%u n %u inherit 0x%02x ctx 0x%x\n", name, k, g->d.n, g->inherit, g->ctx);
            n_defs++;
            k += o->a; i++;
            break;
        }
        case NDG_UNIT:
            if (o->p != (const void*)(L + 2u * k)) fail("unit pointer", k);
            k++; i++;
            break;
        case NDG_VTX:
            if (id != NDO_VTX || (uint32_t)(uintptr_t)o->p != w1 || o->a != ((w0 >> 12) & 0xffu) || o->a + o->b != ((w0 >> 1) & 0x7fu)) fail("vertex load", k);
            nvtx += o->a; k++; i++;
            break;
        case NDG_TRI:
            if (id != NDO_TRI1 && id != NDO_TRI2) { fail("triangle op on another unit", k); k++; i++; break; }
            for (int s = 0; s <= (id == NDO_TRI2); s++, i++) {
                const uint32_t w = s ? w1 : (w0 & 0xffffffu);
                const NdGOp* q = &t->op[i];
                if (i >= t->nop || q->k != NDG_TRI || q->a != ((w >> 16) & 0xffu) >> 1 || q->b != ((w >> 8) & 0xffu) >> 1 || q->c != (w & 0xffu) >> 1) fail("triangle", k);
                ntri++;
            }
            k++;
            break;
        case NDG_CALL:
            if (id != NDO_CALL || ((const NdGTab*)o->p)->list != (const uint32_t*)(uintptr_t)w1 || o->a != ((w0 & 0xffffffu) == 1u)) fail("call", k);
            k++; i++;
            break;
        default: fail("unknown op", k); k++; i++; break;
        }
    }
    if (!ended || i != t->nop) fail("ops do not end with END", k);
    if (nvtx != t->nvtx || ntri != t->ntri) fail("nvtx or ntri", k);
    n_ops += t->nop;
    /* (a) the old reader against the table */
    const int old_parts = parts_ok(L);
    Draw d[256];
    unsigned nd = 0, tot = 0;
    const int walk = old_parts ? old_walk(L, d, &nd, &tot) : -1;
    const int old_ok = old_parts && walk == 0;
    if (old_ok != (int)(t->flags & NDG_F_PARTS)) fail("parts verdict differs from parts_ok and the nd_part_draw checks", 0);
    if (old_ok && (t->flags & NDG_F_PARTS)) {
        if (tot != t->nvtx) fail("vertex total", 0);
        unsigned j = 0;
        for (unsigned m = 0; m < t->nop && t->op[m].k != NDG_END; m++, j++) {
            const NdGOp* o = &t->op[m];
            if (j >= nd) { fail("table has more draws than the old walk", m); break; }
            if (o->k == NDG_VTX ? (d[j].is_tri || d[j].src != (uint32_t)(uintptr_t)o->p || d[j].n != o->a || d[j].v0 != o->b)
                                : (o->k != NDG_TRI || !d[j].is_tri || d[j].t[0] != o->a || d[j].t[1] != o->b || d[j].t[2] != o->c)) fail("draw differs from the old walk", m);
        }
        if (j != nd) fail("draw count differs from the old walk", j);
        n_draws += nd; n_parts++;
    }
    /* (d) the lookup */
    if (nd_gtab_find(L) != t) fail("nd_gtab_find", 0);
}

int main(void) {
    unsigned lists = 0;
    for (unsigned s = 0; s < gconv_nsym; s++) {
        const GSym* g = &gconv_sym[s];
        if (g->kind != GS_GFX) continue;
        const NdGTab* t = NULL;
        for (unsigned i = 0; i < nd_gtab_n; i++) if (nd_gtab_idx[i].list == g->addr) t = nd_gtab_idx[i].tab;
        cur = g->name;
        if (!t) { fail("no table", 0); continue; }
        check_tab(g->name, t, (unsigned)(((uintptr_t)g->end - (uintptr_t)g->addr) / 8u));
        lists++;
    }
    if (lists != nd_gtab_n) { cur = "all"; fail("table count differs from the Gfx symbols", lists); }
    static const uint32_t not_a_list[2];
    if (nd_gtab_find(not_a_list)) { cur = "all"; fail("nd_gtab_find of a non-list", 0); }
    printf("gtab lists %u parts %u draws %u defs %u ops %u bad %u\n", lists, n_parts, n_draws, n_defs, n_ops, bad);
    return bad ? 1 : 0;
}
