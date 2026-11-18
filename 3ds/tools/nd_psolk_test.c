/* nd_psolk_test.c: NPSO2D host compare of the pso table lookup. Usage: nd_psolk_test [--fault] <ndref.bin> ...
 * Grid: the states of the ndref captures (and the synthetic leg 4 states) with the walker flags, colours, box_pat bp, mode5_prev m5 and
 * leg_mode 0 or 4. Per point: ref = nd_comb_core on the walker inputs (the old path); got = nd_pso_build of the description made from the same
 * inputs, then nd_pso_fix with the walker's colours, bp and m5. Where nd_pso_in_eq holds, got must equal ref byte for byte.
 * Find: for each table id, the walker inputs of its description give nd_pso_find a table entry with the same comb bytes; with w1 ^ 1 it gives 0.
 * --fault: nd_pso_fix is not applied (leg 4 mode5, box_pat and cc of the walker must then differ: bad above 0).
 * Prints "psolk find <n> bad <b>", then "psolk entries <e> neq <q> bad <b>" last. Exit 1 on any bad. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"
#include "nd_api.h"

extern const NdPsoKey nd_pso_key[];                     /* nd_pso_tab.c */
extern const NdPsoSmall nd_pso_small[];                 /* nd_pso_tab.c */
extern const NdPlanRec nd_pso_plan[];                  /* nd_pso_tab.c */
extern const unsigned nd_pso_nkey;                      /* nd_pso_tab.c */
extern const unsigned nd_pso_n;                        /* nd_pso_tab.c */
#include "../tests/pso/fix_descs.c" /* the table descriptions: pso_fix_a is id 0, pso_fix_b id 1 (the one file of tests/pso) */

static void key_of(const NdPsoDesc* d, NdPsoKey* k) { /* the key of a description (gen_pso_main.c makes the same) */
    memset(k, 0, sizeof *k);
    k->cw0 = d->cw0; k->cw1 = d->cw1; k->omh = d->omh; k->key_c = d->key_c; k->key_s = d->key_s;
    k->conv_k4 = (uint8_t)d->conv_k4; k->plod = d->plod; k->vprim = d->vprim;
    k->flags = nd_pso_flags(d->tex_on, d->use_t1, d->t1_valid, d->dual_on, d->box_prod, d->box_folded, d->comb_set, d->comb_alpha_shade);
}

uint32_t f32tof24(float vf) { /* host copy of libctru f32tof24, as nd_decide_test.c has it */
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

void unpack8(unsigned c, float o[4]) { /* host copy, as nd_decide_test.c has it */
    o[0] = (float)((c >> 24) & 255u) / 255.f; o[1] = (float)((c >> 16) & 255u) / 255.f;
    o[2] = (float)((c >> 8) & 255u) / 255.f;  o[3] = (float)(c & 255u) / 255.f;
}

static uint32_t rd32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

typedef struct { uint32_t w0, w1, omh, oml; int zimg; } St;
static St* sts;
static unsigned nst;

static void add_state(St s) {
    for (unsigned i = 0; i < nst; i++) if (!memcmp(&sts[i], &s, sizeof s)) return;
    sts = realloc(sts, (nst + 1) * sizeof *sts);
    sts[nst++] = s;
}

/* the walker rules (nd_comb_set) for one G_SETCOMBINE, as nd_decide_test.c has them */
static void walker_comb(uint32_t w0, uint32_t w1, uint32_t t[16], int* use_t1, int* box_prod, int* ashade) {
    t[0] = (w0 >> 20) & 0xf;  t[2] = (w0 >> 15) & 0x1f; t[1] = (w1 >> 28) & 0xf; t[3] = (w1 >> 15) & 7;
    t[4] = (w0 >> 12) & 7;    t[6] = (w0 >> 9) & 7;     t[5] = (w1 >> 12) & 7;   t[7] = (w1 >> 9) & 7;
    t[8] = (w0 >> 5) & 0xf;   t[10] = w0 & 0x1f;        t[9] = (w1 >> 24) & 0xf; t[11] = (w1 >> 6) & 7;
    t[12] = (w1 >> 21) & 7;   t[14] = (w1 >> 18) & 7;   t[13] = (w1 >> 3) & 7;   t[15] = w1 & 7;
    int t0 = t[0] == 1 || t[1] == 1 || t[3] == 1 || t[2] == 1 || t[2] == 8 || t[4] == 1 || t[5] == 1 || t[6] == 1 || t[7] == 1;
    int t1 = t[0] == 2 || t[1] == 2 || t[3] == 2 || t[2] == 2 || t[2] == 9 || t[4] == 2 || t[5] == 2 || t[6] == 2 || t[7] == 2;
    int c2 = 0;
    for (int i = 8; i < 16; i++) if (t[i] == 1 || t[i] == 2) c2 = 1;
    if (t[10] == 8 || t[10] == 9) c2 = 1;
    *use_t1 = 0;
    if (t1 && !t0 && !c2) {
        *use_t1 = 1;
        if (t[0] == 2) t[0] = 1;
        if (t[1] == 2) t[1] = 1;
        if (t[3] == 2) t[3] = 1;
        if (t[2] == 2) t[2] = 1;
        if (t[2] == 9) t[2] = 8;
        if (t[4] == 2) t[4] = 1;
        if (t[5] == 2) t[5] = 1;
        if (t[6] == 2) t[6] = 1;
        if (t[7] == 2) t[7] = 1;
    }
    *box_prod = t[0] == 1 && t[1] >= 7 && t[2] == 2 && t[3] == 7 && t[4] == 7 && t[5] == 7 && t[6] == 7 && t[7] == 2 &&
                t[8] == 3 && t[9] == 5 && t[10] == 0 && t[11] == 5 && t[12] == 7 && t[13] == 7 && t[14] == 7 && t[15] == 0;
    *ashade = ((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 || ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4;
}

static unsigned n_ent, n_neq, n_bad;
static int fault;

/* one grid point: the walker inputs (in), the description (d) made from the same state, the old path (ref) and the table path (got) */
static void point(const St* s, int tex_on, int t1v, int dual, int cset, uint32_t kc, uint32_t ks, uint32_t k4, int vprim, unsigned plod, int folded,
                  int leg, int bp, int m5, uint32_t prim, uint32_t env) {
    uint32_t t[16];
    int use_t1, box_prod, ashade;
    walker_comb(s->w0, s->w1, t, &use_t1, &box_prod, &ashade);
    if (folded && !box_prod) return;
    if (folded) { t[0] = 3; t[1] = 5; t[2] = 1; t[3] = 5; t[4] = 7; t[5] = 7; t[6] = 7; t[7] = 1; use_t1 = 1; }
    NdCombIn in;
    memset(&in, 0, sizeof in);
    in.w0 = s->w0; in.w1 = s->w1; in.omode_h = s->omh; in.prim = prim; in.env = env; in.prim_lo8 = plod; in.key_c = kc; in.key_s = ks; in.conv_k4 = k4;
    memcpy(in.tst, t, sizeof t);
    in.leg_mode = leg; in.tex_on = tex_on; in.use_t1 = use_t1; in.t1_valid = t1v; in.dual_on = dual; in.box_prod = box_prod; in.box_pat = bp; in.box_folded = folded;
    in.comb_set = cset; in.comb_alpha_shade = ashade;
    NdPsoDesc d;
    memset(&d, 0, sizeof d);
    d.cw0 = s->w0; d.cw1 = s->w1; d.omh = s->omh; d.oml = s->oml; d.key_c = kc; d.key_s = ks; d.conv_k4 = k4;
    d.tex_on = (uint8_t)tex_on; d.use_t1 = (uint8_t)use_t1; d.t1_valid = (uint8_t)t1v; d.dual_on = (uint8_t)dual;
    d.box_prod = (uint8_t)box_prod; d.box_folded = (uint8_t)folded; d.comb_set = (uint8_t)cset; d.comb_alpha_shade = (uint8_t)ashade;
    d.leg_prev = (uint8_t)leg; d.vprim = (uint8_t)vprim; d.zimg = (uint8_t)s->zimg; d.plod = (uint8_t)plod;

    NdCombOut ref;
    pm_comb_set_k4(k4);
    nd_comb_core(&in, (unsigned)vprim, m5, &ref);
    NdPsoOut o;
    nd_pso_build(&d, &o);
    NdCombOut got = o.comb;
    if (!fault) nd_pso_fix(&got, &in, m5);
    NdPsoKey kd;
    key_of(&d, &kd);
    if (!nd_pso_in_eq(&kd, &in, (unsigned)vprim)) { n_neq++; return; }
    n_ent++;
    if (memcmp(&got, &ref, sizeof ref) != 0) {
        n_bad++;
        if (n_bad < 6) printf("BAD cw %08x %08x omh %08x tex %d t1v %d dual %d cset %d leg %d bp %d m5 %d: leg %d/%d mode5 %d/%d pat %d/%d cc %08x/%08x\n",
                              s->w0, s->w1, s->omh, tex_on, t1v, dual, cset, leg, bp, m5, got.leg, ref.leg, got.mode5, ref.mode5, got.pat, ref.pat, got.cc, ref.cc);
    }
}

/* the walker inputs of a key (the fields nd_pso_in_eq reads, the tst from the words) */
static void win_of_key(const NdPsoKey* key, NdCombIn* w) {
    memset(w, 0, sizeof *w);
    w->w0 = key->cw0; w->w1 = key->cw1; w->omode_h = key->omh; w->prim_lo8 = key->plod; w->key_c = key->key_c; w->key_s = key->key_s; w->conv_k4 = key->conv_k4;
    nd_comb_tst(key->cw0, key->cw1, (key->flags >> 1) & 1, (key->flags >> 5) & 1, w->tst);
    w->tex_on = key->flags & 1; w->use_t1 = (key->flags >> 1) & 1; w->t1_valid = (key->flags >> 2) & 1; w->dual_on = (key->flags >> 3) & 1;
    w->box_prod = (key->flags >> 4) & 1; w->box_folded = (key->flags >> 5) & 1; w->comb_set = (key->flags >> 6) & 1; w->comb_alpha_shade = (key->flags >> 7) & 1;
}

/* the find part: each table key finds an entry; each table id (the descriptions of tests/pso) finds the whole NdCombOut of nd_pso(id); w1 ^ 1 gives 0 */
static unsigned n_find, n_fbad;
static void find_key(unsigned k) {
    const NdPsoKey* key = &nd_pso_key[k];
    NdCombIn w;
    NdCombOut o;
    win_of_key(key, &w);
    n_find++;
    if (nd_pso_find(&w, key->vprim, &o) != 1) { n_fbad++; printf("BAD find key %u: no entry\n", k); return; }
    w.w1 ^= 1u;
    if (nd_pso_find(&w, key->vprim, &o)) { n_fbad++; printf("BAD find key %u: w1 ^ 1 hits\n", k); }
}
static void find_id(unsigned id, const NdPsoDesc* d) {
    NdPsoKey key;
    NdCombIn w;
    NdCombOut o;
    key_of(d, &key);
    win_of_key(&key, &w);
    n_find++;
    if (nd_pso_find(&w, d->vprim, &o) != 1 || memcmp(&o, &nd_pso(id)->comb, sizeof o) != 0) { n_fbad++; printf("BAD find id %u: comb differs\n", id); return; }
    w.w1 ^= 1u;
    if (nd_pso_find(&w, d->vprim, &o)) { n_fbad++; printf("BAD find id %u: w1 ^ 1 hits\n", id); }
}

int main(int argc, char** argv) {
    int a = 1;
    if (a < argc && !strcmp(argv[a], "--fault")) { fault = 1; a++; }
    if (a >= argc) { fprintf(stderr, "usage: nd_psolk_test [--fault] <ndref.bin> ...\n"); return 2; }
    for (; a < argc; a++) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) { printf("FAIL: cannot open %s\n", argv[a]); return 1; }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char* buf = malloc((size_t)sz);
        assert(buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz);
        fclose(f);
        assert(memcmp(buf, "NDR1", 4) == 0 && rd32(buf + 4) == 4);
        for (size_t pos = 20, end = 20 + rd32(buf + 16); pos < end;) {
            uint32_t kind = rd32(buf + pos), size = rd32(buf + pos + 4);
            const unsigned char* b = buf + pos + 8;
            if (kind == 4) { St s = { rd32(b + 32), rd32(b + 36), rd32(b + 48), rd32(b + 52), rd32(b + 56) != 0 }; add_state(s); }
            pos += 8 + size;
        }
        free(buf);
    }
    { /* the captures hold no constant colour combine (leg 4): the synthetic states of nd_decide_test.c, one cycle and two cycles */
        St c1 = { 0xFC000000u | 1u << 12 | 3u << 9, 3u << 15 | 7u << 12 | 7u << 9, 0u, 0x00552048u, 0 };
        St c2 = c1; c2.omh = 1u << 20; c2.w1 |= 7u << 21 | 7u << 18 | 7u << 3; c2.w0 |= 7u << 5;
        add_state(c1); add_state(c2);
    }
    static const uint32_t pe[4][2] = { { 0, 0 }, { 0xff8040c0u, 0x20a0e010u }, { 0x12345678u, 0xfedcba98u }, { 0x000000ffu, 0xffffff00u } };
    for (unsigned i = 0; i < nst; i++) {
        const St* s = &sts[i];
        for (int tex = 0; tex < 2; tex++) for (int t1v = 0; t1v < 2; t1v++) for (int dual = 0; dual < 2; dual++) for (int folded = 0; folded < 2; folded++)
        for (int v = 0; v < 3; v++) for (int cset = 0; cset < 2; cset++) for (int leg = 0; leg < 2; leg++) for (int bp = 0; bp < 2; bp++) for (int m5 = 0; m5 < 2; m5++) {
            const uint32_t kc = v == 0 ? 0 : 0x80402000u, ks = v == 0 ? 0 : 0x10203000u, k4 = v == 0 ? 0 : 0x55u;
            for (int p = 0; p < 4; p++)
                point(s, tex, t1v, dual, cset, kc, ks, k4, v == 2, v == 0 ? 0u : 0x7fu, folded, leg ? 4 : 0, bp, m5, pe[p][0], pe[p][1]);
        }
    }
    for (unsigned k = 0; k < nd_pso_nkey; k++) find_key(k);
    {
        const NdPsoDesc* ids[] = { &pso_fix_a, &pso_fix_b };
        if (nd_pso_n != 2) { n_fbad++; printf("BAD table: %u ids, the descriptions are 2\n", nd_pso_n); }
        for (unsigned id = 0; id < 2 && id < nd_pso_n; id++) find_id(id, ids[id]);
    }
    printf("psolk find %u bad %u\n", n_find, n_fbad);
    printf("psolk entries %u neq %u bad %u\n", n_ent, n_neq, n_bad);
    return n_bad || n_fbad ? 1 : 0;
}
