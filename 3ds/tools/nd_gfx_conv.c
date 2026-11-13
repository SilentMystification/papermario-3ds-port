/* nd_gfx_conv.c: the gfx table tool (NATIVEAPI S7, row S7CONV design note). Host C (docker pm3ds-host:1, gcc -m32, the device defines),
 * linked with the host objects of the converted gfx sources and their symbol map (gen_gfx_syms.py, nd_gconv.h), and with the pure part of
 * nd_scomp.c (ND_SCOMP_HOST). Writes the native tables (nd_gtab.h) of every Gfx list of the set to stdout as C (3ds/src/nd_gfx_tab.c) and
 * one `gconv def` line per state def plus a summary to stderr. A pointer word is written as symbol + offset (the same link address on the
 * device); a pointer outside the map stays in its unit (NDG_UNIT, read at run time). Exit 1 on a list it cannot convert. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_op.h"
#include "nd_scomp.h"
#include "nd_gtab.h"
#include "nd_gconv.h"

static const GSym* sym_of(uintptr_t p, int kind, unsigned* off) {
    for (unsigned i = 0; i < gconv_nsym; i++) {
        const GSym* s = &gconv_sym[i];
        if (kind && s->kind != kind) continue;
        if (p >= (uintptr_t)s->addr && p < (uintptr_t)s->end) { *off = (unsigned)(p - (uintptr_t)s->addr); return s; }
    }
    return NULL;
}
static int is_draw(unsigned id) { return id == NDO_VTX || id == NDO_TRI1 || id == NDO_TRI2 || id == NDO_CALL || id == NDO_END; }

/* The inherited inputs of def d (design note 6): the nd_scomp_key groups it does not set before its decision. */
static uint32_t def_inherit(const NdSDef* d) {
    const uint32_t* u = (const uint32_t*)d->u;
    uint32_t m = NDG_IN_KEY;
    unsigned first = d->n;
    for (unsigned k = 0; k < d->n; k++) {
        if (u[2u * k] == ND_NOP_A || (d->vmask >> k & 1u)) continue;
        first = k;
        break;
    }
    int prim = 0, env = 0;
    for (unsigned k = 0; k < first; k++) {
        if (!(d->vmask >> k & 1u)) continue;
        if ((u[2u * k] >> 24) == NDO_PRIM) prim = 1;
        if ((u[2u * k] >> 24) == NDO_ENV) env = 1;
    }
    const uint32_t cyc = 3u << G_MDSFT_CYCLETYPE;
    if (!d->has_comb) m |= NDG_IN_COMB;
    if ((d->omh_m & cyc) != cyc) m |= NDG_IN_CYC;
    if (!d->has_tex) m |= NDG_IN_TEXON;
    if (!prim) m |= NDG_IN_PRIM;
    if (!env) m |= NDG_IN_ENV;
    return m;
}

typedef struct { NdGOp op; unsigned k, n; const GSym* s; unsigned off; NdSDef d; uint32_t inh; } Op;
static Op ops[4096];
static unsigned n_lists, n_parts, n_defs, n_ops, n_bad;

static void conv(const GSym* g) {
    const uint32_t* L = (const uint32_t*)g->addr;
    const unsigned nu = (unsigned)(((uintptr_t)g->end - (uintptr_t)g->addr) / 8u);
    unsigned no = 0, k = 0, nvtx = 0, ntri = 0, k_end = nu, defi = 0;
    while (k < nu) {
        if (no + 2u >= sizeof ops / sizeof ops[0]) { fprintf(stderr, "gconv: %s: too many ops\n", g->name); exit(1); }
        Op* o = &ops[no];
        memset(o, 0, sizeof *o);
        o->k = k; o->n = 1;
        const uint32_t w0 = L[2u * k], w1 = L[2u * k + 1u];
        const unsigned id = w0 >> 24;
        if (w0 == ND_NOP_A) { k++; continue; }
        if (is_draw(id)) {
            if (id == NDO_END) { o->op.k = NDG_END; no++; k_end = k; break; }
            if (id == NDO_TRI1 || id == NDO_TRI2) {
                for (int t = 0; t <= (id == NDO_TRI2); t++) {
                    const uint32_t w = t ? w1 : (w0 & 0xffffffu);
                    Op* q = &ops[no++];
                    memset(q, 0, sizeof *q);
                    q->k = k; q->n = 1;
                    q->op.k = NDG_TRI; q->op.a = (uint8_t)(((w >> 16) & 0xffu) >> 1); q->op.b = (uint8_t)(((w >> 8) & 0xffu) >> 1); q->op.c = (uint8_t)((w & 0xffu) >> 1);
                    ntri++;
                }
                k++;
                continue;
            }
            if (id == NDO_VTX) {
                const unsigned n = (w0 >> 12) & 0xffu, e = (w0 >> 1) & 0x7fu;
                o->s = sym_of(w1, GS_VTX, &o->off);
                if (o->s && n <= e) { o->op.k = NDG_VTX; o->op.a = (uint8_t)n; o->op.b = (uint8_t)(e - n); nvtx += n; }
                else o->op.k = NDG_UNIT;
                no++; k++;
                continue;
            }
            /* NDO_CALL: a list of the set at its start */
            o->s = sym_of(w1, GS_GFX, &o->off);
            if (o->s && o->off == 0u) { o->op.k = NDG_CALL; o->op.a = (uint8_t)((w0 & 0xffffffu) == 1u); }
            else o->op.k = NDG_UNIT;
            no++; k++;
            continue;
        }
        unsigned best = 0;
        for (unsigned n = 1; n <= 32u && k + n <= nu; n++) {
            if (is_draw(L[2u * (k + n - 1u)] >> 24)) break;
            NdSDef d;
            memset(&d, 0, sizeof d);
            d.u = L + 2u * k; d.n = (uint8_t)n;
            if (nd_scomp_analyse(&d)) best = n;
        }
        if (best) {
            o->op.k = NDG_DEF; o->op.a = (uint8_t)best; o->n = best;
            o->d.u = L + 2u * k; o->d.n = (uint8_t)best;
            nd_scomp_analyse(&o->d);
            o->d.st = 1; o->d.id = (uint8_t)(n_defs + defi);
            o->inh = def_inherit(&o->d);
            defi++;
            no++; k += best;
            continue;
        }
        o->op.k = NDG_UNIT;
        no++; k++;
    }
    if (!no || ops[no - 1u].op.k != NDG_END) { /* no END unit: the readers stop at the array end */
        memset(&ops[no], 0, sizeof ops[no]); ops[no].op.k = NDG_END; ops[no].k = nu; no++;
    }
    /* NDG_F_PARTS from the ops (the parts_ok rule, design note 3) */
    int parts = no >= 2u && ops[0].op.k == NDG_VTX && ops[0].k == 0u && ops[0].op.b == 0u && ops[0].op.a >= 1u && ops[0].op.a <= 64u && k_end <= 64u;
    unsigned have = parts ? ops[0].op.a : 0u;
    for (unsigned i = 1; parts && i < no; i++) {
        const NdGOp* p = &ops[i].op;
        if (p->k == NDG_END) break;
        if (p->k == NDG_VTX) {
            if (p->a < 1u || p->b + p->a > 64u || p->b > have) parts = 0;
            else if (p->b + p->a > have) have = p->b + p->a;
        } else if (p->k == NDG_TRI) {
            if (p->a >= have || p->b >= have || p->c >= have) parts = 0;
        } else parts = 0;
    }
    /* output */
    for (unsigned i = 0; i < no; i++) {
        const Op* o = &ops[i];
        if (o->op.k != NDG_DEF) continue;
        const NdSDef* d = &o->d;
        printf("static const NdGDef ndgd_%s_%u = { { .u = &%s[%u], .n = %u, .st = 1, .id = %u, .nv = %u, .pre = %u, .has_geom = %u, .has_tex = %u, .has_comb = %u, "
               ".last_op = %u, .vmask = 0x%xu, .omh_m = 0x%xu, .omh_v = 0x%xu, .oml_m = 0x%xu, .oml_v = 0x%xu, .geom_c = 0x%xu, .geom_s = 0x%xu, "
               ".tex_a = 0x%xu, .tex_b = 0x%xu, .cw0 = 0x%xu, .cw1 = 0x%xu, .nsync = %u, .sync_k = { %u, %u, %u, %u } }, 0x%xu, 0x%xu, %u };\n",
               g->name, o->k, g->name, o->k, d->n, d->id, d->nv, d->pre, d->has_geom, d->has_tex, d->has_comb, d->last_op, d->vmask, d->omh_m, d->omh_v,
               d->oml_m, d->oml_v, d->geom_c, d->geom_s, d->tex_a, d->tex_b, d->cw0, d->cw1, d->nsync, d->sync_k[0], d->sync_k[1], d->sync_k[2], d->sync_k[3], o->inh, NDG_CTX_T1V | NDG_CTX_DUAL | NDG_CTX_VPRIM, o->k);
        fprintf(stderr, "gconv def %s+%u n %u inherit 0x%02x ctx 0x%x\n", g->name, o->k, d->n, o->inh, NDG_CTX_T1V | NDG_CTX_DUAL | NDG_CTX_VPRIM);
    }
    printf("static const NdGOp ndgo_%s[%u] = {\n", g->name, no);
    for (unsigned i = 0; i < no; i++) {
        const Op* o = &ops[i];
        const NdGOp* p = &o->op;
        printf("    { %u, %u, %u, %u, ", p->k, p->a, p->b, p->c);
        switch (p->k) {
        case NDG_VTX: printf("(const char*)%s + %u", o->s->name, o->off); break;
        case NDG_DEF: printf("&ndgd_%s_%u", g->name, o->k); break;
        case NDG_UNIT: printf("&%s[%u]", g->name, o->k); break;
        case NDG_CALL: printf("&ndg_%s", o->s->name); break;
        default: printf("0"); break;
        }
        printf(" },\n");
    }
    printf("};\nconst NdGTab ndg_%s = { (const uint32_t*)%s, ndgo_%s, %u, %u, %u, %u };\n", g->name, g->name, g->name, no, parts ? NDG_F_PARTS : 0u, nvtx, ntri);
    n_lists++; n_parts += (unsigned)parts; n_defs += defi; n_ops += no;
}

/* ---- CONV2C: the v2 tables (nd_gtab.h NdNOp / NdNTab) of the self-contained lists. One walk over the typed units of a list, with the calls inlined (a
 * called list runs on the state of its call point, and the state it leaves stays), tracks the state groups of an NdPsoDesc, the tile state of tile 0
 * (the walker's set_tile / set_loadtile / set_loadblock / set_timg / set_loadtlut / set_tilesize rules, tex_win0 of nd_texwin.c), the matrices and the
 * colours. A list that reaches a triangle before it has set a combine, the cycle bits of other mode H and G_TEXTURE inherits state from its caller:
 * it gets no table (ntab_inherit.txt). A list with a unit the v2 form has no op for (lights, scissor, rectangles, convert and key words, tile 1, an
 * address outside the set) gets none either (ntab_unsup.txt, the reason). There is never a v1 fallback inside a v2 table. ---- */
#include "nd_int.h"
#include "nd_vslot.h"

unsigned short* nuGfxZBuffer;

#define V2_MAXOPS 4096u
#define V2_MAXIDX 8192u
#define V2_MAXDESC 8192u
#define V2_MAXTAB 2048u
#define V2_MAXBIND 64u
#define V2_MAXVS 2048u
typedef struct { uint8_t op, a; uint16_t b; uint32_t v; char p[160]; uint32_t idx_pos; } COp;
typedef struct {
    uint32_t cw0, cw1, omh, oml, geom, key_c, key_s, conv_k4, omh_set, plod;
    unsigned has_comb, has_tex, tex_on;
    NdTst t; NdTexM m;
    const void* timg; int timg_ok;
    const void* tlut; int tlut_n;
    int td_valid; NdBindDesc td;
    int td1_valid; NdBindDesc td1;
} V2St;
typedef struct {
    const GSym* top;
    V2St st;
    COp ops[V2_MAXOPS]; unsigned nops;
    uint16_t idx[V2_MAXIDX]; unsigned nidx;
    NdBindDesc bind[V2_MAXBIND]; unsigned nbind;
    NdVsOp vs[V2_MAXVS]; unsigned nvs;     /* the loads so far (all segments) */
    NdVsOp seg[V2_MAXVS]; unsigned nseg, seg_tri; /* the ops of the open segment */
    int emitted_pso, emitted_bind, emitted_bind1;
    char unsup[160];
    int inh; char inh_groups[64], inh_first[160];
} V2C;
static NdPsoDesc v2_desc[V2_MAXDESC];
static unsigned v2_ndesc;
static const GSym* v2_tab[V2_MAXTAB];
static V2C* cx;
static unsigned v2_n_tab, v2_n_inh, v2_n_unsup, v2_n_draw;

static void v2_unsup(const char* what, unsigned k) {
    if (cx->unsup[0]) return;
    snprintf(cx->unsup, sizeof cx->unsup, "%s (unit %u of %s)", what, k, cx->top->name);
}
static const char* v2_opname(unsigned id) {
    switch (id) {
    case NDO_VTX: return "VTX"; case NDO_TRI1: return "TRI1"; case NDO_TRI2: return "TRI2"; case NDO_MOVEMEM: return "MOVEMEM"; case NDO_MW: return "MOVEWORD";
    case NDO_RARE: return "RARE (convert or key)"; case NDO_RECT: return "RECT"; case NDO_SCISSOR: return "SCISSOR"; case NDO_FILL: return "FILLCOLOR";
    case NDO_PRIMZ: return "PRIMDEPTH"; case NDO_MISC: return "MISC"; default: return "op";
    }
}
/* the address expression of an address inside a symbol of the kind: NAME + byte offset */
static int v2_sym_expr(char* out, size_t n, uintptr_t p, int kind) {
    unsigned off;
    const GSym* s = sym_of(p, kind, &off);
    if (!s) return 0;
    snprintf(out, n, "(const char*)%s + %u", s->name, off);
    return 1;
}

/* the pipeline description of the state now (the walker's rules of op_setcombine: use_t1, box_prod, alpha shade are functions of the combine words) */
static NdPsoDesc v2_make_desc(const V2St* s) {
    NdPsoDesc d;
    memset(&d, 0, sizeof d);
    d.cw0 = s->cw0; d.cw1 = s->cw1; d.omh = s->omh; d.oml = s->oml; d.geom = s->geom; d.key_c = s->key_c; d.key_s = s->key_s; d.conv_k4 = s->conv_k4;
    d.tex_on = (uint8_t)s->tex_on;
    d.t1_valid = (uint8_t)s->td1_valid;
    const uint32_t w0 = s->cw0, w1 = s->cw1;
    uint32_t f[16];
    f[0] = (w0 >> 20) & 0xf;  f[1] = (w1 >> 28) & 0xf;  f[2] = (w0 >> 15) & 0x1f; f[3] = (w1 >> 15) & 7;
    f[4] = (w0 >> 12) & 7;    f[5] = (w1 >> 12) & 7;    f[6] = (w0 >> 9) & 7;      f[7] = (w1 >> 9) & 7;
    f[8] = (w0 >> 5) & 0xf;   f[9] = (w1 >> 24) & 0xf;  f[10] = w0 & 0x1f;         f[11] = (w1 >> 6) & 7;
    f[12] = (w1 >> 21) & 7;   f[13] = (w1 >> 3) & 7;    f[14] = (w1 >> 18) & 7;    f[15] = w1 & 7;
    int t0 = 0, t1 = 0, c2 = 0;
    for (unsigned i = 0; i < 8; i++) { t0 |= f[i] == 1; t1 |= f[i] == 2; }
    t0 |= f[2] == 8; t1 |= f[2] == 9;
    for (unsigned i = 8; i < 16; i++) c2 |= f[i] == 1 || f[i] == 2;
    c2 |= f[10] == 8 || f[10] == 9;
    d.use_t1 = (uint8_t)(t1 && !t0 && !c2);
    d.box_prod = (uint8_t)(f[0] == 1 && f[1] >= 7 && f[2] == 2 && f[3] == 7 && f[4] == 7 && f[5] == 7 && f[6] == 7 && f[7] == 2 && f[8] == 3 && f[9] == 5 &&
                           f[10] == 0 && f[11] == 5 && f[12] == 7 && f[13] == 7 && f[14] == 7 && f[15] == 0);
    d.comb_set = (uint8_t)s->has_comb;
    d.comb_alpha_shade = (uint8_t)(((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 || ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4);
    d.plod = (uint8_t)(s->plod & 0xffu);
    return d;
}
static unsigned v2_desc_id(const NdPsoDesc* d) {
    for (unsigned i = 0; i < v2_ndesc; i++) if (!memcmp(&v2_desc[i], d, sizeof *d)) return i;
    if (v2_ndesc >= V2_MAXDESC) { fprintf(stderr, "gconv: too many pso descriptions\n"); exit(1); }
    v2_desc[v2_ndesc] = *d;
    return v2_ndesc++;
}

static void v2_push(unsigned op, unsigned a, unsigned b, uint32_t v, const char* p, uint32_t idx_pos) {
    if (cx->nops >= V2_MAXOPS) { v2_unsup("too many v2 ops", 0); return; }
    COp* o = &cx->ops[cx->nops++];
    o->op = (uint8_t)op; o->a = (uint8_t)a; o->b = (uint16_t)b; o->v = v; o->idx_pos = idx_pos;
    snprintf(o->p, sizeof o->p, "%s", p ? p : "0");
}
/* the loads of the closed segment are state of the next ones: they join cx->vs */
static void v2_commit_loads(void) {
    for (unsigned i = 0; i < cx->nseg; i++) {
        if (cx->seg[i].kind != ND_VS_VTX) continue;
        if (cx->nvs >= V2_MAXVS) { v2_unsup("too many vertex loads", 0); return; }
        cx->vs[cx->nvs++] = cx->seg[i];
    }
}
/* resolve the open segment into NDN_DRAW ops: the loads before it are replayed (slots are state), the draws are the triangles of the segment */
static void v2_flush(void) {
    if (!cx->seg_tri) { v2_commit_loads(); cx->nseg = 0; return; }
    static NdVsOp all[2u * V2_MAXVS];
    static NdVsDraw dr[V2_MAXVS];
    static uint16_t idx[V2_MAXIDX];
    memcpy(all, cx->vs, cx->nvs * sizeof all[0]);
    memcpy(all + cx->nvs, cx->seg, cx->nseg * sizeof all[0]);
    unsigned ndr = 0, nidx = 0;
    const int r = nd_vslot_resolve(all, cx->nvs + cx->nseg, idx, V2_MAXIDX, dr, V2_MAXVS, &ndr, &nidx);
    v2_commit_loads();
    cx->nseg = 0; cx->seg_tri = 0;
    if (r) { v2_unsup("vertex slots (nd_vslot_resolve)", (unsigned)(-r - 1)); return; }
    for (unsigned i = 0; i < ndr; i++) {
        if (cx->nidx + dr[i].nidx > V2_MAXIDX) { v2_unsup("too many indices", 0); break; }
        char p[160];
        snprintf(p, sizeof p, "(const void*)%s", gconv_sym[dr[i].arr].name);
        v2_push(NDN_DRAW, 0, dr[i].nidx, 0, p, cx->nidx);
        memcpy(&cx->idx[cx->nidx], &idx[dr[i].idx0], dr[i].nidx * sizeof idx[0]);
        cx->nidx += dr[i].nidx;
    }
}
static void v2_seg_add(const NdVsOp* o) {
    if (cx->nseg >= V2_MAXVS) { v2_unsup("too many vertex ops", 0); return; }
    cx->seg[cx->nseg++] = *o;
}
static void v2_state_op(unsigned op, unsigned a, unsigned b, uint32_t v, const char* p) {
    v2_flush();
    v2_push(op, a, b, v, p, 0);
}

/* the tile state rules of nd_tex_set.c, on the tracker state */
static void v2_set_tile(V2St* s, unsigned tile, unsigned fmt, unsigned siz, unsigned tmem, unsigned pal, const unsigned* wr, unsigned sh0, unsigned sh1) {
    s->t.tile_fmt[tile] = fmt; s->t.tile_siz[tile] = siz; s->t.tile_tmem[tile] = tmem; s->t.tile_pal[tile] = pal;
    for (int i = 0; i < 4; i++) s->t.tile_wrap[tile][i] = wr[i];
    s->t.tile_shift[tile][0] = sh0; s->t.tile_shift[tile][1] = sh1;
}
static void v2_loadblock(V2St* s, unsigned tile, unsigned texels) {
    const unsigned tm = s->t.tile_tmem[tile];
    const unsigned words = ((texels + 1u) << s->t.tile_siz[tile]) >> 4;
    for (unsigned k = 0; k < 4; k++)
        if (s->t.ldrec[k].tmem >= tm && (s->t.ldrec[k].tmem == tm || s->t.ldrec[k].tmem < tm + words)) s->t.ldrec[k].img = NULL;
    unsigned k;
    for (k = 0; k < 4 && !(s->t.blk[k].img && s->t.blk[k].tmem == tm); k++) {}
    if (k == 4) k = s->m.blk_n++ & 3u;
    s->t.blk[k].tmem = tm; s->t.blk[k].img = s->timg; s->t.blk[k].bytes = ((texels + 1u) << s->t.tile_siz[tile]) >> 1;
}
static void v2_loadtile(V2St* s, unsigned tile, int uls, int ult, int lrs, int lrt) {
    s->m.ld_valid = 1; s->m.ld_x = uls >> 2;
    const unsigned tm = s->t.tile_tmem[tile];
    unsigned k;
    for (k = 0; k < 4 && !(s->t.ldrec[k].img && s->t.ldrec[k].tmem == tm); k++) {}
    if (k == 4) k = s->m.ldrec_n++ & 3u;
    s->t.ldrec[k].tmem = tm; s->t.ldrec[k].img = s->timg; s->t.ldrec[k].stride = s->t.timg_stride; s->t.ldrec[k].x = uls >> 2; s->t.ldrec[k].y = ult >> 2;
    s->m.ld_y = ult >> 2; s->m.ld_h = ((lrt - ult) >> 2) + 1; s->m.ld_w = ((lrs - uls) >> 2) + 1;
}
static void v2_tilesize(V2St* s, unsigned k, unsigned tile, int uls, int ult, int lrs, int lrt) {
    if (((lrs - uls) >> 2) + 1 <= 0 || ((lrt - ult) >> 2) + 1 <= 0) { v2_unsup("tile size window not positive", k); return; }
    s->t.tile_sz[tile][0] = uls; s->t.tile_sz[tile][1] = ult; s->t.tile_sz[tile][2] = lrs; s->t.tile_sz[tile][3] = lrt;
    if (tile == 1) {
        s->td1_valid = 0;
        if (!s->timg_ok) { v2_unsup("texture image outside the set", k); return; }
        NdWin wn;
        tex_win1(&s->t, &s->m, s->timg, uls, ult, lrs, lrt, &wn);
        if (wn.src && wn.w > 0 && wn.h > 0) {
            const unsigned pi = s->t.tile_pal[1] & 15u;
            NdBindDesc* d = &s->td1;
            memset(d, 0, sizeof *d);
            d->src = wn.src; d->pal = s->m.tlut_pal[pi] ? s->m.tlut_pal[pi] : s->tlut; d->pal_n = s->tlut_n; d->fmt = s->t.tile_fmt[1];
            d->siz = s->t.tile_siz[1]; d->stride = wn.stride; d->x0 = wn.x0; d->y0 = ult >> 2; d->w = wn.w; d->h = wn.h;
            for (int i = 0; i < 4; i++) { d->wrap[i] = s->t.tile_wrap[1][i]; d->sz[i] = s->t.tile_sz[1][i]; }
            s->td1_valid = 1;
        }
        return;
    }
    if (tile != 0) return;
    s->td_valid = 0;
    if (!s->timg_ok) { v2_unsup("texture image outside the set", k); return; }
    NdWin wn;
    tex_win0(&s->t, &s->m, s->timg, uls, ult, lrs, lrt, &wn);
    if (wn.src && wn.w > 0 && wn.h > 0 && (wn.h > 1 || (unsigned)(uintptr_t)s->timg - (unsigned)(uintptr_t)nuGfxZBuffer >= 320u * 240u * 2u)) {
        const void* pal = s->t.tile_pal[0] < 16u && s->m.tlut_pal[s->t.tile_pal[0] & 15] ? s->m.tlut_pal[s->t.tile_pal[0] & 15] : s->tlut;
        NdBindDesc* d = &s->td;
        memset(d, 0, sizeof *d);
        d->src = wn.src; d->pal = pal; d->pal_n = s->tlut_n; d->fmt = s->t.tile_fmt[0]; d->siz = s->t.tile_siz[0];
        d->stride = wn.stride; d->x0 = uls >> 2; d->y0 = ult >> 2; d->w = wn.w; d->h = wn.h;
        for (int i = 0; i < 4; i++) { d->wrap[i] = s->t.tile_wrap[0][i]; d->sz[i] = s->t.tile_sz[0][i]; }
        s->td_valid = 1;
    }
}

static void v2_inherit_check(unsigned k, const GSym* g, unsigned id) {
    const V2St* s = &cx->st;
    char gr[64] = "";
    if (!s->has_comb) strcat(gr, "comb ");
    if ((s->omh_set & (3u << G_MDSFT_CYCLETYPE)) != (3u << G_MDSFT_CYCLETYPE)) strcat(gr, "cycle ");
    if (!s->has_tex) strcat(gr, "texon ");
    if (gr[0] && !cx->inh) {
        cx->inh = 1;
        snprintf(cx->inh_groups, sizeof cx->inh_groups, "%s", gr);
        snprintf(cx->inh_first, sizeof cx->inh_first, "unit %u of %s (%s)", k, g->name, v2_opname(id));
    }
}

/* returns 1 when a branch call ended the walk (the rest of the calling list does not run) */
static int v2_walk(const GSym* g, unsigned k0, unsigned depth) {
    const uint32_t* L = (const uint32_t*)g->addr;
    const unsigned nu = (unsigned)(((uintptr_t)g->end - (uintptr_t)g->addr) / 8u);
    V2St* s = &cx->st;
    for (unsigned k = k0; k < nu; k++) {
        const uint32_t w0 = L[2u * k], w1 = L[2u * k + 1u], a = w0 & 0xffffffu;
        const unsigned id = w0 >> 24;
        if (w0 == ND_NOP_A) continue;
        switch (id) {
        case NDO_END: return 0;
        case NDO_OMH: { const uint32_t m = nd_op_om_mask(a); s->omh = (s->omh & ~m) | (w1 & m); s->omh_set |= m; break; }
        case NDO_OML: { const uint32_t m = nd_op_om_mask(a); s->oml = (s->oml & ~m) | (w1 & m); break; }
        case NDO_GEOM: s->geom = (s->geom & a) | w1; break;
        case NDO_TEXTURE: s->tex_on = a & 1u; s->has_tex = 1; break;
        case NDO_COMB: s->cw0 = NDO_COMB_W0(a); s->cw1 = w1; s->has_comb = 1; break;
        case NDO_PRIM: s->plod = a & 0xffffu; v2_state_op(NDN_COL, 0, a & 0xffffu, w1, NULL); break;
        case NDO_ENV: v2_state_op(NDN_COL, 1, 0, w1, NULL); break;
        case NDO_FOG: v2_state_op(NDN_COL, 2, 0, w1, NULL); break;
        case NDO_BLEND: v2_state_op(NDN_COL, 3, 0, w1, NULL); break;
        case NDO_MTX: /* a segmented address stays a value (an input of the caller); a host address of a matrix object has no symbol in the map: no v2 form */
            if ((w1 >> 24) >= 16u) { v2_unsup("matrix address outside the symbol map", k); break; }
            v2_state_op(NDN_XF, a & 0xffu, 0, w1, NULL);
            break;
        case NDO_POPMTX: v2_state_op(NDN_XF, 0xffu, 0, w1, NULL); break;
        case NDO_MW: if ((a >> 16) != NDO_MW_NONE) v2_unsup(v2_opname(id), k); break;
        case NDO_FILL: case NDO_PRIMZ: case NDO_SCISSOR: case NDO_MOVEMEM: case NDO_RARE: case NDO_RECT: case NDO_MISC:
            v2_unsup(v2_opname(id), k);
            break;
        case NDO_TIMG: {
            unsigned off;
            s->timg = (const void*)(uintptr_t)w1;
            s->timg_ok = !w1 || sym_of(w1, GS_IMG, &off) != NULL;
            s->t.timg_fmt = (a >> 21) & 7u; s->t.timg_siz = (a >> 19) & 3u; s->t.timg_stride = (int)(a & 0xfffu) + 1;
            s->m.ld_valid = 0;
            break;
        }
        case NDO_TILE: {
            const unsigned wr[4] = { (w1 >> 8) & 3u, (w1 >> 18) & 3u, (w1 >> 4) & 15u, (w1 >> 14) & 15u };
            v2_set_tile(s, (w1 >> 24) & 7u, (a >> 21) & 7u, (a >> 19) & 3u, a & 0x1ffu, (w1 >> 20) & 15u, wr, w1 & 15u, (w1 >> 10) & 15u);
            break;
        }
        case NDO_LOADBLOCK: v2_loadblock(s, (w1 >> 24) & 7u, (w1 >> 12) & 0xfffu); break;
        case NDO_LOADTILE: v2_loadtile(s, (w1 >> 24) & 7u, (int)((a >> 12) & 0xfffu), (int)(a & 0xfffu), (int)((w1 >> 12) & 0xfffu), (int)(w1 & 0xfffu)); break;
        case NDO_LOADTLUT: {
            const unsigned tile = (w1 >> 24) & 7u, cnt = (w1 >> 14) & 0x3ffu;
            s->tlut = s->timg; s->tlut_n = (int)cnt + 1;
            const unsigned slot = (s->t.tile_tmem[tile] - 256u) >> 4;
            if (slot < 16u) s->m.tlut_pal[slot] = s->timg;
            break;
        }
        case NDO_TILESIZE: v2_tilesize(s, k, (w1 >> 24) & 7u, (int)((a >> 12) & 0xfffu), (int)(a & 0xfffu), (int)((w1 >> 12) & 0xfffu), (int)(w1 & 0xfffu)); break;
        case NDO_VTX: {
            const unsigned n = (a >> 12) & 0xffu, e = (a >> 1) & 0x7fu;
            unsigned off;
            const GSym* vs = sym_of(w1, GS_VTX, &off);
            if (!vs || n > e || off % 16u) { v2_unsup("vertex load outside the set", k); break; }
            NdVsOp o;
            memset(&o, 0, sizeof o);
            o.kind = ND_VS_VTX; o.n = (uint8_t)n; o.v0 = (uint8_t)(e - n); o.arr = (uint32_t)(vs - gconv_sym); o.first = off / 16u;
            v2_seg_add(&o);
            break;
        }
        case NDO_TRI1: case NDO_TRI2: {
            v2_inherit_check(k, g, id);
            NdVsOp o;
            memset(&o, 0, sizeof o);
            o.kind = id == NDO_TRI1 ? ND_VS_TRI1 : ND_VS_TRI2;
            for (int t = 0; t <= (id == NDO_TRI2); t++) {
                const uint32_t w = t ? w1 : a;
                o.s[3 * t] = (uint8_t)(((w >> 16) & 0xffu) >> 1); o.s[3 * t + 1] = (uint8_t)(((w >> 8) & 0xffu) >> 1); o.s[3 * t + 2] = (uint8_t)((w & 0xffu) >> 1);
            }
            const NdPsoDesc d = v2_make_desc(s);
            const int pid = (int)v2_desc_id(&d);
            if (pid != cx->emitted_pso) { v2_flush(); v2_push(NDN_PSO, 0, (unsigned)pid, 0, NULL, 0); cx->emitted_pso = pid; }
            if (s->td_valid && s->tex_on) {
                unsigned i;
                for (i = 0; i < cx->nbind && memcmp(&cx->bind[i], &s->td, sizeof s->td); i++) {}
                if (i == cx->nbind) {
                    if (cx->nbind >= V2_MAXBIND) { v2_unsup("too many binds", k); break; }
                    cx->bind[cx->nbind++] = s->td;
                }
                if ((int)i != cx->emitted_bind) {
                    v2_flush();
                    char p[160];
                    snprintf(p, sizeof p, "&ndnb_%s_%u", cx->top->name, i);
                    v2_push(NDN_TEX, 0, 0, 0, p, 0);
                    cx->emitted_bind = (int)i;
                }
            }
            if (s->td1_valid && s->tex_on) {
                unsigned i;
                for (i = 0; i < cx->nbind && memcmp(&cx->bind[i], &s->td1, sizeof s->td1); i++) {}
                if (i == cx->nbind) {
                    if (cx->nbind >= V2_MAXBIND) { v2_unsup("too many binds", k); break; }
                    cx->bind[cx->nbind++] = s->td1;
                }
                if ((int)i != cx->emitted_bind1) {
                    v2_flush();
                    char p[160];
                    snprintf(p, sizeof p, "&ndnb_%s_%u", cx->top->name, i);
                    v2_push(NDN_TEX, 1, 0, 0, p, 0);
                    cx->emitted_bind1 = (int)i;
                }
            }
            v2_seg_add(&o);
            cx->seg_tri = 1;
            break;
        }
        case NDO_CALL: {
            unsigned off;
            const GSym* cs = sym_of(w1, GS_GFX, &off);
            if (!cs || off % 8u || depth >= 8u) { v2_unsup("call target outside the set or too deep", k); break; }
            const int term = v2_walk(cs, off / 8u, depth + 1u);
            if ((a & 1u) || term) return 1; /* G_DL_NOPUSH: the callee's end ends this list too */
            break;
        }
        default: v2_unsup("unknown unit", k); break;
        }
    }
    return 0;
}

static void v2_emit_table(const V2C* c) {
    const char* n = c->top->name;
    v2_n_draw += c->nidx / 3u;
    for (unsigned i = 0; i < c->nbind; i++) {
        const NdBindDesc* d = &c->bind[i];
        char src[160], pal[160];
        if (!v2_sym_expr(src, sizeof src, (uintptr_t)d->src, GS_IMG)) { fprintf(stderr, "gconv: %s: bind source outside the set\n", n); exit(1); }
        if (d->pal) { if (!v2_sym_expr(pal, sizeof pal, (uintptr_t)d->pal, GS_IMG)) { fprintf(stderr, "gconv: %s: palette outside the set\n", n); exit(1); } }
        else strcpy(pal, "0");
        printf("static const NdBindDesc ndnb_%s_%u = { %s, %s, %d, %uu, %uu, %d, %d, %d, %d, %d, { %uu, %uu, %uu, %uu }, { %d, %d, %d, %d } };\n", n, i, src, pal, d->pal_n, d->fmt, d->siz,
               d->stride, d->x0, d->y0, d->w, d->h, d->wrap[0], d->wrap[1], d->wrap[2], d->wrap[3], d->sz[0], d->sz[1], d->sz[2], d->sz[3]);
    }
    printf("static const uint16_t ndni_%s[%u] = {", n, c->nidx ? c->nidx : 1u);
    for (unsigned i = 0; i < c->nidx; i++) printf("%s%u", i ? "," : "", c->idx[i]);
    if (!c->nidx) printf("0");
    printf("};\nstatic const NdNOp ndno_%s[%u] = {\n", n, c->nops + 1u);
    for (unsigned i = 0; i < c->nops; i++) {
        const COp* o = &c->ops[i];
        printf("    { %u, %u, %u, 0x%xu, %s, %u },\n", o->op, o->a, o->b, o->v, o->p, o->idx_pos);
    }
    printf("    { %u, 0, 0, 0, 0, 0 },\n};\nconst NdNTab ndn_%s = { (const void*)%s, 0, %u, ndno_%s, ndni_%s };\n", (unsigned)NDN_END, n, n, c->nops + 1u, n, n);
}

static V2C v2c_mem;
static void v2_convert_all(const char* descs_path, const char* inh_path, const char* uns_path, const char* idx_path) {
    FILE* fi = fopen(inh_path, "w");
    FILE* fu = fopen(uns_path, "w");
    if (!fi || !fu) { fprintf(stderr, "gconv: cannot write the ntab logs\n"); exit(1); }
    printf("/* CONV2C: native tables v2 of the self-contained lists (nd_gtab.h NdNTab; 3ds/tools/nd_gfx_conv.c). */\n#include <stdint.h>\n");
    for (unsigned i = 0; i < gconv_nsym; i++) if (gconv_sym[i].kind == GS_IMG) printf("extern char %s[];\n", gconv_sym[i].name);
    cx = &v2c_mem;
    for (unsigned i = 0; i < gconv_nsym; i++) {
        const GSym* g = &gconv_sym[i];
        if (g->kind != GS_GFX) continue;
        memset(cx, 0, sizeof *cx);
        cx->top = g; cx->emitted_pso = -1; cx->emitted_bind = -1; cx->emitted_bind1 = -1;
        v2_walk(g, 0, 0);
        v2_flush();
        if (cx->inh) {
            fprintf(fi, "%s: inherits %sfirst draw %s\n", g->name, cx->inh_groups, cx->inh_first);
            v2_n_inh++;
            continue;
        }
        if (cx->unsup[0]) { fprintf(fu, "%s: %s\n", g->name, cx->unsup); v2_n_unsup++; continue; }
        if (v2_n_tab >= V2_MAXTAB) { fprintf(stderr, "gconv: too many v2 tables\n"); exit(1); }
        v2_emit_table(cx);
        v2_tab[v2_n_tab++] = g;
    }
    fclose(fi); fclose(fu);
    /* the pso descriptions: the file gen_pso_tab.py of NPSO3 scans for the ids (the id of a description is its line order) */
    FILE* fd = fopen(descs_path, "w");
    if (!fd) { fprintf(stderr, "gconv: cannot write %s\n", descs_path); exit(1); }
    fprintf(fd, "/* Generated by 3ds/tools/nd_gfx_conv.c (CONV2C, never committed): the pipeline descriptions of the v2 tables. NDN_PSO b = the line number below. */\n");
    fprintf(fd, "#include \"nd_api.h\"\n#pragma GCC diagnostic ignored \"-Wunused-const-variable\"\n");
    for (unsigned i = 0; i < v2_ndesc; i++) {
        const NdPsoDesc* d = &v2_desc[i];
        fprintf(fd, "ND_PSO_DESC(ndn_pso_%u, .cw0 = 0x%xu, .cw1 = 0x%xu, .omh = 0x%xu, .oml = 0x%xu, .geom = 0x%xu, .key_c = 0x%xu, .key_s = 0x%xu, .conv_k4 = 0x%xu, "
                 ".tex_on = %u, .use_t1 = %u, .t1_valid = %u, .dual_on = %u, .box_prod = %u, .box_folded = %u, .comb_set = %u, .comb_alpha_shade = %u, "
                 ".leg_prev = %u, .vprim = %u, .zimg = %u, .plod = %u);\n",
                 i, d->cw0, d->cw1, d->omh, d->oml, d->geom, d->key_c, d->key_s, d->conv_k4, d->tex_on, d->use_t1, d->t1_valid, d->dual_on, d->box_prod, d->box_folded,
                 d->comb_set, d->comb_alpha_shade, d->leg_prev, d->vprim, d->zimg, d->plod);
    }
    fclose(fd);
    /* the index of the host compare (nd_gtab_test.c, never compiled for the device) */
    FILE* fx = fopen(idx_path, "w");
    if (!fx) { fprintf(stderr, "gconv: cannot write %s\n", idx_path); exit(1); }
    fprintf(fx, "/* Generated by 3ds/tools/nd_gfx_conv.c (CONV2C): the index of the v2 tables for the host compare. */\n#include \"nd_gtab.h\"\n#include \"nd_ntab_descs.c\"\n");
    for (unsigned i = 0; i < v2_n_tab; i++) fprintf(fx, "extern const NdNTab ndn_%s;\n", v2_tab[i]->name);
    fprintf(fx, "const NdNTab* const ndn_all[%u] = {\n", v2_n_tab ? v2_n_tab : 1u);
    for (unsigned i = 0; i < v2_n_tab; i++) fprintf(fx, "    &ndn_%s,\n", v2_tab[i]->name);
    if (!v2_n_tab) fprintf(fx, "    0\n");
    fprintf(fx, "};\nconst unsigned ndn_n = %uu;\nconst NdPsoDesc* const ndn_desc[%u] = {\n", v2_n_tab, v2_ndesc ? v2_ndesc : 1u);
    for (unsigned i = 0; i < v2_ndesc; i++) fprintf(fx, "    &ndn_pso_%u,\n", i);
    if (!v2_ndesc) fprintf(fx, "    0\n");
    fprintf(fx, "};\nconst unsigned ndn_ndesc = %uu;\nconst unsigned ndn_inherit = %uu, ndn_unsup = %uu;\n", v2_ndesc, v2_n_inh, v2_n_unsup);
    fclose(fx);
    fprintf(stderr, "gconv ntab tables %u inherit %u unsup %u descs %u triangles %u\n", v2_n_tab, v2_n_inh, v2_n_unsup, v2_ndesc, v2_n_draw);
}

int main(int argc, char** argv) {
    printf("/* Generated by 3ds/tools/nd_gfx_conv.c (S7CONV, never committed): native tables of the static gfx lists (nd_gtab.h). */\n");
    printf("#include \"common.h\"\n#include \"nd_gtab.h\"\n");
    for (unsigned i = 0; i < gconv_nsym; i++) {
        const GSym* s = &gconv_sym[i];
        if (s->kind == GS_GFX) printf("extern Gfx %s[];\nextern const NdGTab ndg_%s;\n", s->name, s->name);
        else if (s->kind == GS_VTX) printf("extern Vtx %s[];\n", s->name);
    }
    for (unsigned i = 0; i < gconv_nsym; i++) if (gconv_sym[i].kind == GS_GFX) conv(&gconv_sym[i]);
    printf("NdGIdx nd_gtab_idx[%u] = {\n", n_lists ? n_lists : 1u);
    for (unsigned i = 0; i < gconv_nsym; i++) if (gconv_sym[i].kind == GS_GFX) printf("    { %s, &ndg_%s },\n", gconv_sym[i].name, gconv_sym[i].name);
    printf("};\nconst unsigned nd_gtab_n = %uu;\n", n_lists);
    if (argc < 5) { fprintf(stderr, "usage: nd_gfx_conv <nd_ntab_descs.c> <ntab_inherit.txt> <ntab_unsup.txt> <nd_ntab_idx.c>\n"); return 2; }
    v2_convert_all(argv[1], argv[2], argv[3], argv[4]);
    fprintf(stderr, "gconv lists %u parts %u defs %u ops %u bad %u\n", n_lists, n_parts, n_defs, n_ops, n_bad);
    return n_bad ? 1 : 0;
}
