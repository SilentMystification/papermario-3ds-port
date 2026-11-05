/* nd_emit.c: the command words of one raw draw from native data (plan_native_slices S3 M3). Pure C: the host test replays the words into a register file and compares them
 * with the real command stream of the old path (nd_emit_test.c, ndref v4 kind 6). */
#include <stdint.h>
#include <string.h>
#include "nd.h"
_Static_assert(ND_TMPL_WORDS <= 64, "NdShadow holds the template words");

/* one command: value, header, extra values, a pad word when the count of words is odd (the list is 8 byte aligned) */
static uint32_t* cmd(uint32_t* o, unsigned reg, unsigned mask, const uint32_t* v, unsigned n, int consec) {
    *o++ = v[0];
    *o++ = reg | mask << 16 | (n - 1u) << 20 | (consec ? 0x80000000u : 0u);
    for (unsigned i = 1; i < n; i++) *o++ = v[i];
    if (!(n & 1u)) *o++ = 0; /* 1 + 1 + (n - 1) words: odd n gives an even count, even n needs the pad */
    return o;
}
static uint32_t* one(uint32_t* o, unsigned reg, unsigned mask, uint32_t v) { return cmd(o, reg, mask, &v, 1, 0); }

/* a float uniform upload: the location word (bit 31 = IEEE float32 words), then the words at the data port */
static uint32_t* uni(uint32_t* o, unsigned loc, const float* f, unsigned nvec) {
    uint32_t w[16];
    o = one(o, 0x2C0, 0xF, 0x80000000u | loc);
    for (unsigned v = 0; v < nvec; v++)
        for (unsigned c = 0; c < 4; c++) memcpy(&w[4 * v + c], &f[4 * v + c], 4);
    return cmd(o, 0x2C1, 0xF, w, 4 * nvec, 0);
}
static void rev4(float* o, const float* g) { o[0] = g[3]; o[1] = g[2]; o[2] = g[1]; o[3] = g[0]; } /* game x y z w to the memory order w z y x */

void nd_shadow_reset(NdShadow* s) { memset(s, 0, sizeof *s); s->tmpl = -1; }

unsigned nd_emit_draw(uint32_t* out, NdShadow* sh, const NdDraw* d) {
    uint32_t* o = out;
    /* template words: a register written again only when its value changed */
    if (d->tmpl >= 0) {
        const NdTmplWord* w;
        unsigned nw = nd_tmpl_words(d->tmpl, &w);
        if (sh->tmpl != d->tmpl) {
            for (unsigned i = 0; i < nw; i++) {
                if (sh->tmpl >= 0 && sh->tmpl_val[i] == w[i].val) continue;
                o = one(o, w[i].reg, 0xF, w[i].val);
                sh->tmpl_val[i] = w[i].val;
            }
            sh->tmpl = d->tmpl;
        }
    }
    /* texture units */
    for (unsigned u = 0; u < 2; u++) {
        const NdUnit* t = &d->u[u];
        unsigned base = 0x80u + 0x10u * u;
        if (!t->dim) continue;
        if (!sh->u_valid || memcmp(&sh->u[u], t, sizeof *t) != 0 || !sh->u_on[u]) {
            uint32_t b[5] = { 0, t->dim, t->param, t->lod, t->addr };
            o = cmd(o, base + 1, 0xF, b, 5, 1);
            o = one(o, base + 0xE, 0xF, t->type);
            sh->u[u] = *t; sh->u_on[u] = 1;
        }
    }
    sh->u_valid = 1;
    /* uniforms */
    if (!sh->pal_valid || memcmp(sh->pal, d->pal, sizeof d->pal) != 0) {
        o = uni(o, ND_UNI_PAL, d->pal, 4);
        memcpy(sh->pal, d->pal, sizeof d->pal); sh->pal_valid = 1;
    }
    {
        float all[16], z[4] = { d->zmix[0], d->zmix[1], 0.f, 0.f };
        rev4(all, d->uvso); rev4(all + 4, d->cmul); rev4(all + 8, d->cadd); rev4(all + 12, z);
        if (!sh->uni_valid || memcmp(sh->uni, all, sizeof all) != 0) {
            o = uni(o, ND_UNI_VAR, all, 4);
            memcpy(sh->uni, all, sizeof all); sh->uni_valid = 1;
        }
    }
    /* the draw: vertex attribute buffer setup stays with citro3d; the index address, count and the trigger */
    o = one(o, 0x25E, 0x2, 0x300);
    o = one(o, 0x25F, 0xF, 1);
    o = one(o, 0x227, 0xF, d->idx_word);
    o = one(o, 0x228, 0xF, d->nidx);
    o = one(o, 0x22A, 0xF, 0);
    o = one(o, 0x229, 0x2, 0x100);
    o = one(o, 0x253, 0x2, 0x100);
    o = one(o, 0x245, 0x1, 0);
    o = one(o, 0x22F, 0xF, 1);
    o = one(o, 0x245, 0x1, 1);
    o = one(o, 0x229, 0x2, 0);
    o = one(o, 0x253, 0x2, 0);
    o = one(o, 0x231, 0xF, 1);
    o = one(o, 0x25E, 0x8, 0);
    o = one(o, 0x25E, 0x8, 0);
    return (unsigned)(o - out);
}
