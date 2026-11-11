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

void nd_shadow_reset(NdShadow* s) { memset(s, 0, offsetof(NdShadow, pal)); s->tmpl = -1; } /* PERF2: the palette copies are not cleared (pal_valid 0 says they are not held) */

/* the colour word of TEV stage i is register 0xC3 + 8 i (0xF3 + 8 (i - 4) for the stages 4 and 5) */
static int col_stage(unsigned r) {
    if (r >= 0xC3 && r <= 0xDB && ((r - 0xC3) & 7) == 0) return (int)((r - 0xC3) >> 3);
    if (r == 0xF3) return 4;
    if (r == 0xFB) return 5;
    return -1;
}

/* HC6 step 4: emit_core with off != NULL also records the word offsets of the per-frame values (nd_emit_full); nd_emit_draw passes NULL (the
 * compiler drops the recording). */
#define OFF(f, v) do { if (off) off->f = (uint16_t)(v); } while (0)
static inline __attribute__((always_inline)) unsigned emit_core(uint32_t* out, NdShadow* sh, const NdDraw* d, NdBlkOff* off) {
    uint32_t* o = out;
    if (off) { memset(off, 0xff, sizeof *off); }
    /* template words: a register is written again only when its value changed. A stage colour comes from the draw. */
    if (d->tmpl >= 0) {
        const NdTmplWord* w;
        unsigned nw = nd_tmpl_words(d->tmpl, &w);
        int same = sh->tmpl == d->tmpl && sh->col_valid && !memcmp(sh->col, d->tev_col, sizeof sh->col) && sh->buf_col == d->buf_col && sh->has_buf == d->has_buf;
        if (!same) {
            for (unsigned i = 0; i < nw; i++) {
                uint32_t val = w[i].val;
                int cs = col_stage(w[i].reg);
                if (cs >= 0 && val != 0xFFFFFFFFu) { val = d->tev_col[cs]; OFF(col[cs], o - out); } /* a colour source sets this stage colour */
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
        unsigned en = (d->u[0].dim ? 1u : 0u) | (d->u[1].dim ? 2u : 0u) | (d->u[2].dim ? 4u : 0u);
        if (en && (!sh->cfg_valid || sh->cfg != en)) {
            cfg_cleared = 1;
            o = one(o, 0x80, 0xB, 0x11000u | en);
            o = one(o, 0x80, 0x4, 0x10000u);
            o = one(o, 0x8B, 0xF, 1);
            sh->cfg = en; sh->cfg_valid = 1;
        }
    }
    int unit_chg = 0;
    /* libctru gpu/registers.h: unit 0 border 0x81 (dim 0x82, param 0x83, lod 0x84, addr 0x85), type 0x8E; unit 1 border 0x91, type 0x96; unit 2 border 0x99 (dim 0x9A .. addr 0x9D), type 0x9E (FB-2) */
    static const unsigned blk_reg[3] = { 0x81u, 0x91u, 0x99u }, fmt_reg[3] = { 0x8Eu, 0x96u, 0x9Eu };
    for (unsigned u = 0; u < 3; u++) {
        const NdUnit* t = &d->u[u];
        if (!t->dim) continue;
        if (!sh->u_on[u] || t->force || memcmp(&sh->u[u], t, sizeof *t) != 0) {
            uint32_t b[5] = { 0, t->dim, t->param, t->lod, t->addr };
            if (u == 0) OFF(u0, o - out);
            o = cmd(o, blk_reg[u], 0xF, b, 5, 1);
            if (u == 0) OFF(ty, o - out);
            o = one(o, fmt_reg[u], 0xF, t->type);
            sh->u[u] = *t; sh->u_on[u] = 1; unit_chg = 1;
        }
    }
    if (unit_chg && !cfg_cleared) o = one(o, 0x80, 0x4, 0x10000u); /* citro3d clears the texture cache at every bind: a slab that was freed and reused holds new texels under an old address */
    /* palette entries and the per draw uniforms */
    for (unsigned k = 0; k < d->npal && k < ND_NPAL && !d->no_uni; k++) {
        if (!(sh->pal_valid >> k & 1u) || memcmp(sh->pal[k], d->pal[k], sizeof sh->pal[k]) != 0) {
            if (k == 0) OFF(pal, o - out + 2);
            o = uni(o, d->loc_pal + 4u * k, d->pal[k], 4);
            memcpy(sh->pal[k], d->pal[k], sizeof sh->pal[k]); sh->pal_valid |= 1u << k;
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
    if (d->has_nzo) { /* FB-2: the noise offset uniform (the walker sets it at every noise draw) */
        float v[4];
        rev4(v, d->nzo);
        if (!sh->nzo_valid || memcmp(sh->nzo, v, sizeof v) != 0) { o = uni(o, d->loc_nzo, v, 1); memcpy(sh->nzo, v, sizeof v); sh->nzo_valid = 1; }
    }
    /* the draw: vertex attribute buffer setup stays with citro3d; the index address, count and the trigger */
    o = one(o, 0x25E, 0x2, 0x300);
    o = one(o, 0x25F, 0xF, 1);
    OFF(idx, o - out);
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
#undef OFF
unsigned nd_emit_draw(uint32_t* out, NdShadow* sh, const NdDraw* d) { return emit_core(out, sh, d, NULL); }
/* HC6 step 4: the words of draw d with the full state (a reset shadow: no redundant-state filter) and the offsets of its per-frame values. */
unsigned nd_emit_full(uint32_t* out, const NdDraw* d, NdBlkOff* off) {
    NdShadow s;
    nd_shadow_reset(&s);
    return emit_core(out, &s, d, off);
}
/* HC6 step 4 (switch blkchk): the register writes of span a (old path) whose last value is not the last value of the same register in span b (the block).
 * A uniform data word (0x2C1) is keyed by the location of its 0x2C0 write and its index; a masked write by register and mask. */
static unsigned dec(const uint32_t* w, unsigned n, uint32_t* key, uint32_t* val, unsigned max) {
    unsigned i = 0, m = 0, loc = 0, idx = 0;
    while (i + 1u < n && m < max) {
        const uint32_t h = w[i + 1];
        const unsigned reg = h & 0xFFFFu, mask = (h >> 16) & 0xFu, cnt = ((h >> 20) & 0xFFu) + 1u, consec = h >> 31;
        for (unsigned j = 0; j < cnt && m < max; j++) {
            const uint32_t v = j ? w[i + 1 + j] : w[i];
            const unsigned r = consec ? reg + j : reg;
            if (r == 0x2C0u) { loc = v & 0xFFu; idx = 0; }
            key[m] = r == 0x2C1u ? 0x100000u | loc << 8 | (idx++ & 0xFFu) : r | (uint32_t)mask << 16;
            val[m++] = v;
        }
        i += 2u + (cnt - 1u) + ((cnt & 1u) ? 0u : 1u);
    }
    return m;
}
uint32_t nd_emit_dk, nd_emit_da, nd_emit_db; /* blkchk: the first differing key of the last nd_emit_cmp, its old value and its block value (0xDEADBEEF: absent) */
unsigned nd_emit_cmp(const uint32_t* a, unsigned na, const uint32_t* b, unsigned nb) {
    static uint32_t ka[1024], va[1024], kb[1024], vb[1024];
    const unsigned ma = dec(a, na, ka, va, 1024u), mb = dec(b, nb, kb, vb, 1024u);
    unsigned bad = 0;
    for (unsigned i = 0; i < ma; i++) {
        int later = 0;
        for (unsigned j = i + 1u; j < ma; j++) if (ka[j] == ka[i]) { later = 1; break; }
        if (later) continue;
        unsigned j = mb;
        while (j > 0u && kb[j - 1u] != ka[i]) j--;
        if (!j || vb[j - 1u] != va[i]) { if (!bad) { nd_emit_dk = ka[i]; nd_emit_da = va[i]; nd_emit_db = j ? vb[j - 1u] : 0xDEADBEEFu; } bad++; }
    }
    return bad;
}
