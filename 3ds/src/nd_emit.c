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
    memcpy(w, f, 4u * nvec * 4u);
    return cmd(o, 0x2C1, 0xF, w, 4 * nvec, 0);
}
static void rev4(float* o, const float* g) { o[0] = g[3]; o[1] = g[2]; o[2] = g[1]; o[3] = g[0]; } /* game x y z w to the memory order w z y x */

void nd_shadow_reset(NdShadow* s) { memset(s, 0, sizeof *s); s->tmpl = -1; }

/* the colour word of TEV stage i is register 0xC3 + 8 i (0xF3 + 8 (i - 4) for the stages 4 and 5) */
static int col_stage(unsigned r) {
    if (r >= 0xC3 && r <= 0xDB && ((r - 0xC3) & 7) == 0) return (int)((r - 0xC3) >> 3);
    if (r == 0xF3) return 4;
    if (r == 0xFB) return 5;
    return -1;
}

unsigned nd_emit_draw(uint32_t* out, NdShadow* sh, const NdDraw* d) {
    uint32_t* o = out;
    /* template words: a register is written again only when its value changed. A stage colour comes from the draw. */
    if (d->tmpl >= 0) {
        const NdTmplWord* w;
        unsigned nw = nd_tmpl_words(d->tmpl, &w);
        int same = sh->tmpl == d->tmpl && sh->col_valid && !memcmp(sh->col, d->tev_col, sizeof sh->col) && sh->buf_col == d->buf_col && sh->has_buf == d->has_buf;
        if (!same) {
            for (unsigned i = 0; i < nw; i++) {
                uint32_t val = w[i].val;
                int cs = col_stage(w[i].reg);
                if (cs >= 0 && val != 0xFFFFFFFFu) val = d->tev_col[cs]; /* a colour source sets this stage colour */
                else if (w[i].reg == 0xFD) { if (!d->has_buf) continue; val = d->buf_col; }
                if (sh->tmpl >= 0 && sh->tmpl_val[i] == val) continue;
                o = one(o, w[i].reg, 0xF, val);
                sh->tmpl_val[i] = val;
            }
            sh->tmpl = d->tmpl; memcpy(sh->col, d->tev_col, sizeof sh->col); sh->col_valid = 1; sh->buf_col = d->buf_col; sh->has_buf = d->has_buf;
        }
    }
    /* texture units: the enable word when the set changes, then the blocks that changed */
    int cfg_cleared = 0;
    {
        unsigned en = (d->u[0].dim ? 1u : 0u) | (d->u[1].dim ? 2u : 0u);
        if (en && (!sh->cfg_valid || sh->cfg != en)) {
            cfg_cleared = 1;
            o = one(o, 0x80, 0xB, 0x11000u | en);
            o = one(o, 0x80, 0x4, 0x10000u);
            o = one(o, 0x8B, 0xF, 1);
            sh->cfg = en; sh->cfg_valid = 1;
        }
    }
    int unit_chg = 0;
    for (unsigned u = 0; u < 2; u++) {
        const NdUnit* t = &d->u[u];
        unsigned base = 0x80u + 0x10u * u;
        if (!t->dim) continue;
        if (!sh->u_on[u] || t->force || memcmp(&sh->u[u], t, sizeof *t) != 0) {
            uint32_t b[5] = { 0, t->dim, t->param, t->lod, t->addr };
            o = cmd(o, base + 1, 0xF, b, 5, 1);
            o = one(o, u ? 0x96u : base + 0xE, 0xF, t->type); /* the format register: 0x8E unit 0, 0x96 unit 1 (SGEN-2) */
            sh->u[u] = *t; sh->u_on[u] = 1; unit_chg = 1;
        }
    }
    if (unit_chg && !cfg_cleared) o = one(o, 0x80, 0x4, 0x10000u); /* citro3d clears the texture cache at every bind: a slab that was freed and reused holds new texels under an old address */
    /* palette entries and the per draw uniforms */
    for (unsigned k = 0; k < d->npal && k < ND_NPAL && !d->no_uni; k++) {
        if (!sh->pal_valid[k] || memcmp(sh->pal[k], d->pal[k], sizeof sh->pal[k]) != 0) {
            o = uni(o, d->loc_pal + 4u * k, d->pal[k], 4);
            memcpy(sh->pal[k], d->pal[k], sizeof sh->pal[k]); sh->pal_valid[k] = 1;
        }
    }
    {
        float all[16], z[4] = { d->zmix[0], d->zmix[1], 0.f, 0.f };
        rev4(all, d->uvso); rev4(all + 4, d->cmul); rev4(all + 8, d->cadd); rev4(all + 12, z);
        if (!d->no_uni && (!sh->uni_valid || memcmp(sh->uni, all, sizeof all) != 0)) {
            o = uni(o, d->loc_var, all, 4);
            memcpy(sh->uni, all, sizeof all); sh->uni_valid = 1;
        }
    }
    if (d->has_uvx) { /* SGEN-2: the unit 1 uv map, its own uniform */
        float v[4];
        rev4(v, d->uvx);
        if (!sh->uvx_valid || memcmp(sh->uvx, v, sizeof v) != 0) { o = uni(o, d->loc_uvx, v, 1); memcpy(sh->uvx, v, sizeof v); sh->uvx_valid = 1; }
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
