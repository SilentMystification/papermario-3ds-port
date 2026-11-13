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

#ifdef NTAB_V2
/* ---- CONV2C: the host compare of the v2 tables (nd_gtab.h NdNTab). Compiled with -DNTAB_V2 by 3ds/tests/host.d/conv2c.sh, linked with nd_ntab_idx.c, nd_decide.c,
 * nd_tmpl.c, nd_texwin.c and pm_comb.c. For every v2 table the v1 table of the same list is walked as the v1 path walks it (the NdGDef words and the unit ops, the
 * calls of the callee tables), on the reset walker state (all zero: a self-contained list needs no other), by a state model written here from the walker's setters
 * (op_setcombine, set_tile, set_loadtile, set_loadblock, set_timg, set_loadtlut, set_tilesize; tex_win0 of nd_texwin.c is the one shared function). The model gives, per
 * triangle: the vertex array and the three absolute indices, the pipeline description (nd_pso_build output bytes and the description bytes), the tile 0 bind
 * descriptor; and the transform and colour op sequences. The table is replayed the same way. Any difference counts in bad.
 * --fault: one NDN_COL value + 1 in the compare's copy (the compare must then report bad above 0). ---- */
#include <stdlib.h>
#include "nd_int.h"

unsigned short* nuGfxZBuffer;
uint32_t f32tof24(float vf) { /* host copy of libctru f32tof24, as nd_decide_test.c */
    union { float f; uint32_t i; } v;
    v.f = vf;
    uint32_t sign = v.i >> 31, mant = (v.i & 0x7FFFFFu) >> 7;
    int exp = (int)((v.i >> 23) & 0xFF);
    if (exp == 0) return sign << 23;
    exp = exp - 127 + 63;
    if (exp < 0) return sign << 23;
    if (exp > 0x7F) return sign << 23 | 0x7Fu << 16;
    return mant | (uint32_t)exp << 16 | sign << 23;
}
void unpack8(unsigned c, float o[4]) {
    o[0] = (float)((c >> 24) & 255u) / 255.f; o[1] = (float)((c >> 16) & 255u) / 255.f;
    o[2] = (float)((c >> 8) & 255u) / 255.f;  o[3] = (float)(c & 255u) / 255.f;
}

extern const NdNTab* const ndn_all[];
extern const unsigned ndn_n, ndn_inherit, ndn_unsup, ndn_ndesc;
extern const NdPsoDesc* const ndn_desc[];

typedef struct { const void* base; unsigned i[3]; NdPsoDesc d; int tex; NdBindDesc b; unsigned nx, nc; int tex1; NdBindDesc b1; } TriEv;
typedef struct { unsigned a, b; uint32_t v; } XcEv;
#define EVMAX 16384u
static TriEv mtri[EVMAX], vtri[EVMAX];
static XcEv mxf[EVMAX], mcol[EVMAX], vxf[EVMAX], vcol[EVMAX];
static unsigned nmtri, nvtri, nmxf, nmcol, nvxf, nvcol;

typedef struct {
    uint32_t cw0, cw1, omh, oml, geom, plod;
    unsigned tex_on, has_comb;
    NdTst t; NdTexM m;
    const void* timg; const void* tlut; int tlut_n;
    int td_valid; NdBindDesc td;
    int td1_valid; NdBindDesc td1;
} MSt;
static MSt ms;
static struct { const void* base; unsigned idx; int ok; } mslot[64];
static int m_over;

static const GSym* m_vsym(uint32_t p) {
    for (unsigned i = 0; i < gconv_nsym; i++)
        if (gconv_sym[i].kind == GS_VTX && p >= (uintptr_t)gconv_sym[i].addr && p < (uintptr_t)gconv_sym[i].end) return &gconv_sym[i];
    return NULL;
}
/* the walker's combine rules (op_setcombine and box_prod, as nd_decide_test.c walker_comb) */
static NdPsoDesc m_desc(const MSt* s) {
    NdPsoDesc d;
    memset(&d, 0, sizeof d);
    d.cw0 = s->cw0; d.cw1 = s->cw1; d.omh = s->omh; d.oml = s->oml; d.geom = s->geom; d.t1_valid = (uint8_t)s->td1_valid; d.tex_on = (uint8_t)s->tex_on; d.comb_set = (uint8_t)s->has_comb; d.plod = (uint8_t)s->plod;
    const uint32_t w0 = s->cw0, w1 = s->cw1;
    uint32_t t[16];
    t[0] = (w0 >> 20) & 0xf;  t[2] = (w0 >> 15) & 0x1f; t[1] = (w1 >> 28) & 0xf; t[3] = (w1 >> 15) & 7;
    t[4] = (w0 >> 12) & 7;    t[6] = (w0 >> 9) & 7;     t[5] = (w1 >> 12) & 7;   t[7] = (w1 >> 9) & 7;
    t[8] = (w0 >> 5) & 0xf;   t[10] = w0 & 0x1f;        t[9] = (w1 >> 24) & 0xf; t[11] = (w1 >> 6) & 7;
    t[12] = (w1 >> 21) & 7;   t[14] = (w1 >> 18) & 7;   t[13] = (w1 >> 3) & 7;   t[15] = w1 & 7;
    const int t0 = t[0] == 1 || t[1] == 1 || t[3] == 1 || t[2] == 1 || t[2] == 8 || t[4] == 1 || t[5] == 1 || t[6] == 1 || t[7] == 1;
    const int t1 = t[0] == 2 || t[1] == 2 || t[3] == 2 || t[2] == 2 || t[2] == 9 || t[4] == 2 || t[5] == 2 || t[6] == 2 || t[7] == 2;
    int c2 = 0;
    for (int i = 8; i < 16; i++) if (t[i] == 1 || t[i] == 2) c2 = 1;
    if (t[10] == 8 || t[10] == 9) c2 = 1;
    d.use_t1 = (uint8_t)(t1 && !t0 && !c2);
    if (d.use_t1) { /* the rename of tile 1 to tile 0 changes t[], so box_prod reads the renamed fields */
        for (int i = 0; i < 8; i++) if (t[i] == 2) t[i] = 1;
        if (t[2] == 9) t[2] = 8;
    }
    d.box_prod = (uint8_t)(t[0] == 1 && t[1] >= 7 && t[2] == 2 && t[3] == 7 && t[4] == 7 && t[5] == 7 && t[6] == 7 && t[7] == 2 &&
                           t[8] == 3 && t[9] == 5 && t[10] == 0 && t[11] == 5 && t[12] == 7 && t[13] == 7 && t[14] == 7 && t[15] == 0);
    d.comb_alpha_shade = (uint8_t)(((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 || ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4);
    return d;
}
static void m_xf(unsigned a, uint32_t v) { if (nmxf < EVMAX) { mxf[nmxf].a = a; mxf[nmxf].v = v; mxf[nmxf].b = 0; nmxf++; } else m_over = 1; }
static void m_col(unsigned a, unsigned b, uint32_t v) { if (nmcol < EVMAX) { mcol[nmcol].a = a; mcol[nmcol].b = b; mcol[nmcol].v = v; nmcol++; } else m_over = 1; }
static void m_tilesize(unsigned tile, int uls, int ult, int lrs, int lrt) {
    ms.t.tile_sz[tile][0] = uls; ms.t.tile_sz[tile][1] = ult; ms.t.tile_sz[tile][2] = lrs; ms.t.tile_sz[tile][3] = lrt;
    if (tile == 1) {
        ms.td1_valid = 0;
        NdWin w1;
        tex_win1(&ms.t, &ms.m, ms.timg, uls, ult, lrs, lrt, &w1);
        if (w1.src && w1.w > 0 && w1.h > 0) {
            const unsigned pi = ms.t.tile_pal[1] & 15u;
            NdBindDesc* d = &ms.td1;
            memset(d, 0, sizeof *d);
            d->src = w1.src; d->pal = ms.m.tlut_pal[pi] ? ms.m.tlut_pal[pi] : ms.tlut; d->pal_n = ms.tlut_n; d->fmt = ms.t.tile_fmt[1];
            d->siz = ms.t.tile_siz[1]; d->stride = w1.stride; d->x0 = w1.x0; d->y0 = ult >> 2; d->w = w1.w; d->h = w1.h;
            memcpy(d->wrap, ms.t.tile_wrap[1], sizeof d->wrap);
            memcpy(d->sz, ms.t.tile_sz[1], sizeof d->sz);
            ms.td1_valid = 1;
        }
        return;
    }
    if (tile != 0) return;
    ms.td_valid = 0;
    NdWin w;
    tex_win0(&ms.t, &ms.m, ms.timg, uls, ult, lrs, lrt, &w);
    if (!(w.src && w.w > 0 && w.h > 0 && (w.h > 1 || (unsigned)(uintptr_t)ms.timg - (unsigned)(uintptr_t)nuGfxZBuffer >= 320u * 240u * 2u))) return;
    const unsigned pi = ms.t.tile_pal[0] & 15u;
    NdBindDesc* d = &ms.td;
    memset(d, 0, sizeof *d);
    d->src = w.src; d->pal = ms.m.tlut_pal[pi] ? ms.m.tlut_pal[pi] : ms.tlut; d->pal_n = ms.tlut_n;
    d->fmt = ms.t.tile_fmt[0]; d->siz = ms.t.tile_siz[0]; d->stride = w.stride; d->x0 = uls >> 2; d->y0 = ult >> 2; d->w = w.w; d->h = w.h;
    memcpy(d->wrap, ms.t.tile_wrap[0], sizeof d->wrap);
    memcpy(d->sz, ms.t.tile_sz[0], sizeof d->sz);
    ms.td_valid = 1;
}
static void m_unit(const uint32_t* u) {
    const uint32_t w0 = u[0], w1 = u[1], a = w0 & 0xffffffu;
    switch (w0 >> 24) {
    case NDO_OMH: { const uint32_t m = nd_op_om_mask(a); ms.omh = (ms.omh & ~m) | (w1 & m); break; }
    case NDO_OML: { const uint32_t m = nd_op_om_mask(a); ms.oml = (ms.oml & ~m) | (w1 & m); break; }
    case NDO_GEOM: ms.geom = (ms.geom & a) | w1; break;
    case NDO_TEXTURE: ms.tex_on = a & 1u; break;
    case NDO_COMB: ms.cw0 = NDO_COMB_W0(a); ms.cw1 = w1; ms.has_comb = 1; break;
    case NDO_PRIM: ms.plod = a & 0xffffu; m_col(0, a & 0xffffu, w1); break;
    case NDO_ENV: m_col(1, 0, w1); break;
    case NDO_FOG: m_col(2, 0, w1); break;
    case NDO_BLEND: m_col(3, 0, w1); break;
    case NDO_MTX: m_xf(a & 0xffu, w1); break;
    case NDO_POPMTX: m_xf(0xffu, w1); break;
    case NDO_TIMG:
        ms.timg = (const void*)(uintptr_t)w1; ms.t.timg_fmt = (a >> 21) & 7u; ms.t.timg_siz = (a >> 19) & 3u; ms.t.timg_stride = (int)(a & 0xfffu) + 1; ms.m.ld_valid = 0;
        break;
    case NDO_TILE: {
        const unsigned tile = (w1 >> 24) & 7u;
        ms.t.tile_fmt[tile] = (a >> 21) & 7u; ms.t.tile_siz[tile] = (a >> 19) & 3u; ms.t.tile_tmem[tile] = a & 0x1ffu; ms.t.tile_pal[tile] = (w1 >> 20) & 15u;
        ms.t.tile_wrap[tile][0] = (w1 >> 8) & 3u; ms.t.tile_wrap[tile][1] = (w1 >> 18) & 3u; ms.t.tile_wrap[tile][2] = (w1 >> 4) & 15u; ms.t.tile_wrap[tile][3] = (w1 >> 14) & 15u;
        ms.t.tile_shift[tile][0] = w1 & 15u; ms.t.tile_shift[tile][1] = (w1 >> 10) & 15u;
        break;
    }
    case NDO_LOADBLOCK: {
        const unsigned tile = (w1 >> 24) & 7u, tm = ms.t.tile_tmem[tile], texels = (w1 >> 12) & 0xfffu;
        const unsigned words = ((texels + 1u) << ms.t.tile_siz[tile]) >> 4;
        for (unsigned k = 0; k < 4; k++)
            if (ms.t.ldrec[k].tmem >= tm && (ms.t.ldrec[k].tmem == tm || ms.t.ldrec[k].tmem < tm + words)) ms.t.ldrec[k].img = NULL;
        unsigned k = 0;
        while (k < 4 && !(ms.t.blk[k].img && ms.t.blk[k].tmem == tm)) k++;
        if (k == 4) k = ms.m.blk_n++ & 3u;
        ms.t.blk[k].tmem = tm; ms.t.blk[k].img = ms.timg; ms.t.blk[k].bytes = ((texels + 1u) << ms.t.tile_siz[tile]) >> 1;
        break;
    }
    case NDO_LOADTILE: {
        const unsigned tile = (w1 >> 24) & 7u;
        const int uls = (int)((a >> 12) & 0xfffu), ult = (int)(a & 0xfffu), lrs = (int)((w1 >> 12) & 0xfffu), lrt = (int)(w1 & 0xfffu);
        ms.m.ld_valid = 1; ms.m.ld_x = uls >> 2; ms.m.ld_y = ult >> 2; ms.m.ld_h = ((lrt - ult) >> 2) + 1; ms.m.ld_w = ((lrs - uls) >> 2) + 1;
        const unsigned tm = ms.t.tile_tmem[tile];
        unsigned k = 0;
        while (k < 4 && !(ms.t.ldrec[k].img && ms.t.ldrec[k].tmem == tm)) k++;
        if (k == 4) k = ms.m.ldrec_n++ & 3u;
        ms.t.ldrec[k].tmem = tm; ms.t.ldrec[k].img = ms.timg; ms.t.ldrec[k].stride = ms.t.timg_stride; ms.t.ldrec[k].x = uls >> 2; ms.t.ldrec[k].y = ult >> 2;
        break;
    }
    case NDO_LOADTLUT: {
        const unsigned tile = (w1 >> 24) & 7u, slot = (ms.t.tile_tmem[tile] - 256u) >> 4;
        ms.tlut = ms.timg; ms.tlut_n = (int)((w1 >> 14) & 0x3ffu) + 1;
        if (slot < 16u) ms.m.tlut_pal[slot] = ms.timg;
        break;
    }
    case NDO_TILESIZE: m_tilesize((w1 >> 24) & 7u, (int)((a >> 12) & 0xfffu), (int)(a & 0xfffu), (int)((w1 >> 12) & 0xfffu), (int)(w1 & 0xfffu)); break;
    default: break; /* no-op units; a list with another unit has no v2 table */
    }
}
static int m_isstatic(uint32_t w0) {
    const unsigned id = w0 >> 24;
    return id == NDO_OMH || id == NDO_OML || id == NDO_GEOM || id == NDO_TEXTURE || id == NDO_COMB;
}
static void m_tri(unsigned a, unsigned b, unsigned c) {
    if (nmtri >= EVMAX) { m_over = 1; return; }
    TriEv* e = &mtri[nmtri++];
    memset(e, 0, sizeof *e);
    const unsigned s[3] = { a, b, c };
    for (int j = 0; j < 3; j++) {
        if (s[j] >= 64u || !mslot[s[j]].ok) { m_over = 1; return; }
        if (j && mslot[s[j]].base != mslot[a].base) { m_over = 1; return; }
        e->i[j] = mslot[s[j]].idx;
    }
    e->base = mslot[a].base;
    e->d = m_desc(&ms);
    e->tex = ms.td_valid && ms.tex_on;
    if (e->tex) e->b = ms.td;
    e->tex1 = ms.td1_valid && ms.tex_on;
    if (e->tex1) e->b1 = ms.td1;
    e->nx = nmxf; e->nc = nmcol;
}
/* the v1 table as the v1 path walks it; 1 when a branch call ended the walk */
static int m_walk(const NdGTab* t) {
    for (unsigned i = 0; i < t->nop; i++) {
        const NdGOp* o = &t->op[i];
        switch (o->k) {
        case NDG_END: return 0;
        case NDG_DEF: {
            const NdSDef* d = &((const NdGDef*)o->p)->d;
            ms.omh = (ms.omh & ~d->omh_m) | d->omh_v; ms.oml = (ms.oml & ~d->oml_m) | d->oml_v;
            if (d->has_geom) ms.geom = (ms.geom & d->geom_c) | d->geom_s;
            if (d->has_tex) ms.tex_on = d->tex_a & 1u;
            if (d->has_comb) { ms.cw0 = d->cw0; ms.cw1 = d->cw1; ms.has_comb = 1; }
            const uint32_t* u = (const uint32_t*)d->u;
            for (unsigned j = 0; j < d->n; j++) if (u[2u * j] != ND_NOP_A && !m_isstatic(u[2u * j])) m_unit(u + 2u * j);
            break;
        }
        case NDG_UNIT: m_unit((const uint32_t*)o->p); break;
        case NDG_VTX: {
            const GSym* s = m_vsym((uint32_t)(uintptr_t)o->p);
            if (!s) { m_over = 1; break; }
            const unsigned first = (unsigned)((uintptr_t)o->p - (uintptr_t)s->addr) / 16u;
            for (unsigned j = 0; j < o->a; j++) { mslot[o->b + j].base = s->addr; mslot[o->b + j].idx = first + j; mslot[o->b + j].ok = 1; }
            break;
        }
        case NDG_TRI: m_tri(o->a, o->b, o->c); break;
        case NDG_CALL: {
            const int term = m_walk((const NdGTab*)o->p);
            if (o->a || term) return 1;
            break;
        }
        default: m_over = 1; break;
        }
    }
    return 0;
}

static void v_replay(const NdNTab* t, int fault) {
    int pso = -1;
    const NdBindDesc* bind = NULL;
    const NdBindDesc* bind1 = NULL;
    static int fault_done;
    for (const NdNOp* o = t->ops;; o++) {
        if (o->op == NDN_END) break;
        switch (o->op) {
        case NDN_PSO: pso = o->b; break;
        case NDN_TEX: if (o->a) bind1 = (const NdBindDesc*)o->p; else bind = (const NdBindDesc*)o->p; break;
        case NDN_XF: if (nvxf < EVMAX) { vxf[nvxf].a = o->a; vxf[nvxf].v = o->p ? (uint32_t)(uintptr_t)o->p : o->v; vxf[nvxf].b = 0; nvxf++; } break;
        case NDN_COL:
            if (nvcol < EVMAX) {
                vcol[nvcol].a = o->a; vcol[nvcol].b = o->b; vcol[nvcol].v = o->v;
                if (fault && !fault_done) { vcol[nvcol].v += 1u; fault_done = 1; }
                nvcol++;
            }
            break;
        case NDN_DRAW:
            for (unsigned j = 0; j + 3u <= o->b; j += 3u) {
                if (nvtri >= EVMAX) break;
                TriEv* e = &vtri[nvtri++];
                memset(e, 0, sizeof *e);
                e->base = o->p;
                for (int q = 0; q < 3; q++) e->i[q] = t->idx[o->idx_pos + j + (unsigned)q];
                if (pso >= 0 && (unsigned)pso < ndn_ndesc) e->d = *ndn_desc[pso];
                else e->d.cw0 = 0xdeadbeefu;
                e->tex = bind != NULL;
                if (bind) e->b = *bind;
                e->tex1 = bind1 != NULL;
                if (bind1) e->b1 = *bind1;
                e->nx = nvxf; e->nc = nvcol;
            }
            break;
        default: break;
        }
    }
}

static unsigned v2_bad, v2_draws;
static void v2_fail(const char* what, const char* list, unsigned k) {
    if (v2_bad < 40u) printf("FAIL ntab %s #%u: %s\n", list, k, what);
    v2_bad++;
}
static int pso_same(const NdPsoDesc* a, const NdPsoDesc* b) {
    static NdPsoOut oa, ob;
    memset(&oa, 0, sizeof oa); memset(&ob, 0, sizeof ob);
    nd_pso_build(a, &oa);
    nd_pso_build(b, &ob);
    if (memcmp(&oa, &ob, sizeof oa)) return 0;
    return !memcmp(a, b, sizeof *a);
}
/* CONV2D: the unit stream of a table (its first op, NDN_UNIT) is the depth-0 words of the list from k = 0: NOPs and the end left out, the
 * walk stops after a branch call (the call stays a unit) */
static void v2_units(const NdNTab* t, const char* name) {
    const GSym* g = NULL;
    for (unsigned s = 0; s < gconv_nsym; s++) if (gconv_sym[s].kind == GS_GFX && gconv_sym[s].addr == t->list) g = &gconv_sym[s];
    if (!g || t->nops < 2u || t->ops[0].op != NDN_UNIT) { v2_fail("unit op first", name, 0); return; }
    const NdNOp* u = &t->ops[0];
    const uint32_t* L = (const uint32_t*)g->addr;
    const uint32_t* U = (const uint32_t*)u->p;
    const unsigned nl = (unsigned)(((uintptr_t)g->end - (uintptr_t)g->addr) / 8u);
    unsigned n = 0;
    for (unsigned k = 0; k < nl; k++) {
        const uint32_t w0 = L[2u * k], w1 = L[2u * k + 1u];
        if (w0 == ND_NOP_A) continue;
        if ((w0 >> 24) == NDO_END) break;
        if (n >= u->b) { v2_fail("unit stream", name, n); return; }
        if (U[2u * n] != w0 || U[2u * n + 1u] != w1) { v2_fail("unit stream", name, n); return; }
        n++;
        if ((w0 >> 24) == NDO_CALL && (w0 & 1u)) break;
    }
    if (n != u->b) v2_fail("unit stream count", name, n);
}
/* CONV2D2: the device index entry of a table's list (nd_gtab.h NdNDev): n is the word pairs from unit 0 to the stop, NOPs counted */
static unsigned v2_dev_bad;
static void v2_dev(const NdNTab* t, const char* name) {
    const GSym* g = NULL;
    for (unsigned s = 0; s < gconv_nsym; s++) if (gconv_sym[s].kind == GS_GFX && gconv_sym[s].addr == t->list) g = &gconv_sym[s];
    const NdNDev* dv = NULL;
    for (unsigned i = 0; i < ndn_dev_n; i++) if (ndn_dev_idx[i].list == t->list) dv = &ndn_dev_idx[i];
    if (!dv || !g) { v2_dev_bad++; v2_fail("device index entry", name, 0); return; }
    const uint32_t* L = (const uint32_t*)g->addr;
    const unsigned nl = (unsigned)(((uintptr_t)g->end - (uintptr_t)g->addr) / 8u);
    unsigned ks = nl;
    for (unsigned k = 0; k < nl; k++) {
        const uint32_t w0 = L[2u * k];
        if (w0 == ND_NOP_A) continue;
        if ((w0 >> 24) == NDO_END) { ks = k; break; }
        if ((w0 >> 24) == NDO_CALL && (w0 & 1u)) { ks = k + 1u; break; }
    }
    if (dv->n != ks) { v2_dev_bad++; v2_fail("device index count", name, ks); }
}
static void v2_compare(int fault) {
    unsigned tables = 0;
    for (unsigned n = 0; n < ndn_n; n++) {
        const NdNTab* t = ndn_all[n];
        const char* name = "?";
        const NdGTab* g1 = NULL;
        for (unsigned s = 0; s < gconv_nsym; s++) if (gconv_sym[s].kind == GS_GFX && gconv_sym[s].addr == t->list) name = gconv_sym[s].name;
        for (unsigned i = 0; i < nd_gtab_n; i++) if (nd_gtab_idx[i].list == t->list) g1 = nd_gtab_idx[i].tab;
        if (!g1) { v2_fail("no v1 table for the list", name, 0); continue; }
        memset(&ms, 0, sizeof ms); memset(mslot, 0, sizeof mslot);
        nmtri = nvtri = nmxf = nmcol = nvxf = nvcol = 0; m_over = 0;
        m_walk(g1);
        v_replay(t, fault);
        v2_units(t, name);
        v2_dev(t, name);
        tables++;
        if (m_over) v2_fail("the model could not follow the v1 table", name, 0);
        if (nmtri != nvtri) { v2_fail("triangle count", name, nmtri); continue; }
        if (nmxf != nvxf || nmcol != nvcol) v2_fail("transform or colour op count", name, nmcol);
        for (unsigned i = 0; i < nmxf && i < nvxf; i++) if (mxf[i].a != vxf[i].a || mxf[i].v != vxf[i].v) { printf("  xf model a %x v %x table a %x v %x (n %u/%u)\n", mxf[i].a, mxf[i].v, vxf[i].a, vxf[i].v, nmxf, nvxf); v2_fail("transform op", name, i); }
        for (unsigned i = 0; i < nmcol && i < nvcol; i++) if (mcol[i].a != vcol[i].a || mcol[i].b != vcol[i].b || mcol[i].v != vcol[i].v) v2_fail("colour op", name, i);
        for (unsigned i = 0; i < nmtri; i++) {
            const TriEv *m = &mtri[i], *v = &vtri[i];
            v2_draws++;
            if (m->base != v->base || m->i[0] != v->i[0] || m->i[1] != v->i[1] || m->i[2] != v->i[2]) v2_fail("triangle vertex triple", name, i);
            if (!pso_same(&m->d, &v->d)) v2_fail("pso output", name, i);
            if (m->tex && (!v->tex || memcmp(&m->b, &v->b, sizeof m->b))) v2_fail("bind descriptor", name, i);
            if (m->tex1 && (!v->tex1 || memcmp(&m->b1, &v->b1, sizeof m->b1))) v2_fail("tile 1 bind descriptor", name, i);
            if (m->nx != v->nx || m->nc != v->nc) v2_fail("transform or colour position", name, i);
        }
    }
    printf("ntab tables %u draws %u bad %u\n", tables, v2_draws, v2_bad);
    printf("ntab dev entries %u bad %u\n", ndn_dev_n, v2_dev_bad);
    printf("ntab inherit %u unsup %u descs %u\n", ndn_inherit, ndn_unsup, ndn_ndesc);
}
#endif

#ifdef NTAB_V2
int main(int argc, char** argv) {
    const int fault = argc > 1 && !strcmp(argv[1], "--fault");
#else
int main(void) {
#endif
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
#ifdef NTAB_V2
    v2_compare(fault);
    return bad || v2_bad ? 1 : 0;
#else
    return bad ? 1 : 0;
#endif
}
