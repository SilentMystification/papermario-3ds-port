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

int main(void) {
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
    fprintf(stderr, "gconv lists %u parts %u defs %u ops %u bad %u\n", n_lists, n_parts, n_defs, n_ops, n_bad);
    return n_bad ? 1 : 0;
}
