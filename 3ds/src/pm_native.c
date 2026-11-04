/* NDS1 phase B layout: see pm_native.h. Pure C, no citro3d. The emit loop is the hot part (hot section set in phase B).
 * ponytail: template lookup is a linear scan of 64 entries. Runs at build time only. Upgrade to a hash if producers look up per draw. */
#include <string.h>
#include "pm_native.h"

NatTmpl nat_tmpl[NAT_TMPL_MAX];
uint32_t nat_mtx[2][NAT_MTX_MAX][16] __attribute__((aligned(32)));
uint32_t nat_tex[NAT_TEX_MAX][4];
uint32_t nat_idx_base;

/* pm_gbi.c:504 and :1070: two = the cycle type is 2 CYCLE (1 << G_MDSFT_CYCLETYPE in omode_h, include/PR/gbi.h:579); t1only = the bound
 * texture is TEXEL1 (pm_gbi.c:632, 636). */
#define NAT_MDSFT_CYCLETYPE 20
static int nat_pm_comb(uint32_t cw0, uint32_t cw1, uint32_t oh, int t1only, PmPlan* p) {
    int two = ((oh >> NAT_MDSFT_CYCLETYPE) & 3) == 1;
    return pm_comb_build(cw0, cw1, two, t1only, p);
}

int nat_tmpl_add(uint32_t cw0, uint32_t cw1, uint32_t geom, uint32_t oh, uint32_t ol, int z, int decal, int blend, int atest,
    int t1only, PmPlan* plan_out, int* rc_out) {
    int id = -1;
    for (int i = 0; i < NAT_TMPL_MAX; i++) {
        NatTmpl* t = &nat_tmpl[i];
        if (t->used) {
            if (t->cw0 == cw0 && t->cw1 == cw1 && t->geom == geom && t->oh == oh && t->ol == ol && t->z == !!z &&
                t->decal == !!decal && t->blend == !!blend && t->atest == !!atest && t->t1only == !!t1only) {
                if (plan_out || rc_out) {
                    PmPlan p; int rc = nat_pm_comb(cw0, cw1, oh, t1only, &p);
                    if (plan_out) *plan_out = p;
                    if (rc_out) *rc_out = rc;
                }
                return i;
            }
        } else if (id < 0) id = i;
    }
    if (id < 0) return -1;
    NatTmpl* t = &nat_tmpl[id];
    PmPlan p; int rc = nat_pm_comb(cw0, cw1, oh, t1only, &p);
    if (plan_out) *plan_out = p;
    if (rc_out) *rc_out = rc;
    memset(t, 0, sizeof *t);
    t->cw0 = cw0; t->cw1 = cw1; t->geom = geom; t->oh = oh; t->ol = ol;
    t->z = !!z; t->decal = !!decal; t->blend = !!blend; t->atest = !!atest; t->t1only = !!t1only;
    /* Opaque depth-tested draws may be reordered. Blend, decal and 2D (z off) keep game order. Alpha test writes depth, so it may be reordered. */
    t->ordered = (blend || decal || !z) ? 1 : 0;
    t->used = 1;
    return id;
}

/* Per-draw commands. Palette: the last 0x2C1 write of 16 words (c 15) and the 0x2C0 index before it (the first 0x2C1 of a capture is the
 * same in every model, so it stays in the template). Others: unit 0 and unit 1 texture blocks (0x081, 0x091, c 4), index address and count
 * (0x227, 0x228, c 0), trigger (0x22F, c 0). goff: word offset in grp. */
static const struct { uint32_t reg, c, goff; } nat_kind[5] = {
    { 0x081, 4, 20 }, { 0x091, 4, 26 }, { 0x227, 0, 32 }, { 0x228, 0, 34 }, { 0x22F, 0, 36 },
};
#define NAT_GOFF_IDX 0   /* 0x2C0 index word */
#define NAT_GOFF_PAL 2   /* 0x2C1 palette command */

/* Returns blen, or -1 (does not split), or -3 (a draw of a multi-draw record has state words between its groups: the template
 * cannot be emitted once before the groups). Records with one draw keep the old rule (state words anywhere). */
int nat_tmpl_strip(unsigned id, const uint32_t* w, unsigned n) {
    if (id >= NAT_TMPL_MAX) return -1;
    NatTmpl* t = &nat_tmpl[id];
    /* pass 1: palette commands P (the first 0x2C1 of a capture is a state word, the last nd are the draws), per-draw kinds K (one per draw),
     * the index command before the first draw palette */
    unsigned P = 0, kc[5] = { 0, 0, 0, 0, 0 }, pal0 = ~0u, pidx0 = ~0u, i = 0;
    while (i + 1 < n) {
        uint32_t hd = w[i + 1], reg = hd & 0x3FFu, c = (hd >> 20) & 0xFFu;
        unsigned len = 2u + c + (c & 1u);
        if (i + len > n) return -1;
        if (reg == 0x2C1 && c == 15) P++;
        else for (int j = 0; j < 5; j++) if (reg == nat_kind[j].reg && c == nat_kind[j].c) kc[j]++;
        i += len;
    }
    if (i != n) return -1;
    unsigned nd = kc[2]; /* draws = index address commands. A texture or index count command is left out when the draw keeps the state: the group still carries it */
    if (nd == 0 || nd > P) return -1;
    if (kc[3] != nd || kc[4] != nd || kc[0] > nd || kc[1] > nd) return -1;
    unsigned first_draw = P - nd; /* ordinal of the first draw palette */
    /* pass 2: the draw group prototype (draw 0), state words to blk in order */
    unsigned found[5] = { 0, 0, 0, 0, 0 }, nb = 0, pal_seen = 0; i = 0;
    while (i + 1 < n) {
        uint32_t hd = w[i + 1], reg = hd & 0x3FFu, c = (hd >> 20) & 0xFFu;
        unsigned len = 2u + c + (c & 1u), nx = i + len;
        int is_pal = reg == 0x2C1 && c == 15;
        int next_draw_pal = nx + 1 < n && (w[nx + 1] & 0x3FFu) == 0x2C1u && ((w[nx + 1] >> 20) & 0xFFu) == 15u && pal_seen + 1 > first_draw;
        int is_draw_pal = is_pal && pal_seen >= first_draw;
        int k = -1;
        for (int j = 0; j < 5; j++) if (reg == nat_kind[j].reg && c == nat_kind[j].c) k = j;
        if (is_pal) {
            if (is_draw_pal) { if (pal0 == ~0u) pal0 = i; if (i == pal0) memcpy(t->grp + NAT_GOFF_PAL, w + i, len * sizeof(uint32_t)); }
            else { if (nb + len > NAT_TMPL_WORDS) return -1; memcpy(t->blk + nb, w + i, len * sizeof(uint32_t)); nb += len; }
            pal_seen++;
        } else if (next_draw_pal) {
            if (pidx0 == ~0u) { pidx0 = i; memcpy(t->grp + NAT_GOFF_IDX, w + i, len * sizeof(uint32_t)); }
        } else if (k >= 0) {
            if (!found[k]++) memcpy(t->grp + nat_kind[k].goff, w + i, len * sizeof(uint32_t));
        } else {
            if (nd > 1 && pidx0 != ~0u) return -3; /* state after the first draw: the template is not emitted once */
            if (nb + len > NAT_TMPL_WORDS) return -1;
            memcpy(t->blk + nb, w + i, len * sizeof(uint32_t));
            nb += len;
        }
        i += len;
    }
    for (int j = 0; j < 5; j++) if (!found[j]) return -1;
    t->blen = (uint16_t)nb;
    return (int)nb;
}

/* One LSD pass: stable counting sort of in[] by tex0 (by_tex) or by tmpl. Keys: tex0 < NAT_TEX_MAX, tmpl < NAT_TMPL_MAX. */
static void nat_pass(const NatRec* rec, const uint16_t* in, uint16_t* out, unsigned len, int by_tex) {
    unsigned cnt[NAT_TEX_MAX + 1];
    unsigned nb = by_tex ? NAT_TEX_MAX : NAT_TMPL_MAX;
    memset(cnt, 0, (nb + 1) * sizeof cnt[0]);
    for (unsigned k = 0; k < len; k++) {
        const NatRec* r = &rec[in[k]];
        cnt[(by_tex ? r->tex0 : r->tmpl) + 1]++;
    }
    for (unsigned b = 0; b < nb; b++) cnt[b + 1] += cnt[b];
    for (unsigned k = 0; k < len; k++) {
        const NatRec* r = &rec[in[k]];
        out[cnt[by_tex ? r->tex0 : r->tmpl]++] = in[k];
    }
}

unsigned nat_order(const NatRec* rec, unsigned n, uint16_t* ord) {
    uint16_t tmp[NAT_REC_MAX];
    unsigned i = 0;
    while (i < n) {
        if (nat_tmpl[rec[i].tmpl].ordered) { ord[i] = (uint16_t)i; i++; continue; }
        unsigned j = i;
        while (j < n && !nat_tmpl[rec[j].tmpl].ordered) j++;
        /* run [i, j): index list in game order, then tex0 pass, then tmpl pass (LSD, stable) */
        for (unsigned k = i; k < j; k++) ord[k] = (uint16_t)k;
        nat_pass(rec, ord + i, tmp, j - i, 1);
        nat_pass(rec, tmp, ord + i, j - i, 0);
        i = j;
    }
    return n;
}

/* Group layout (word offsets): 0x2C0 index 0 to 1, palette 2 to 19 (data 4 to 19), tex0 20 to 25 (data 22 to 25), tex1 26 to 31 (data 28 to 31),
 * index address 32 (p0), index count 34 (p0), trigger 36. */
unsigned nat_emit(const NatRec* rec, unsigned n, const uint16_t* ord, uint32_t* out, unsigned cap, unsigned eye,
    NatMtxLog* log, unsigned* nlog) {
    unsigned o = 0, nl = 0;
    int cur = -1;
    for (unsigned k = 0; k < n; k++) {
        const NatRec* r = &rec[ord[k]];
        const NatTmpl* t = &nat_tmpl[r->tmpl];
        if (r->tmpl != cur) {
            if (o + t->blen > cap) return 0;
            memcpy(out + o, t->blk, t->blen * sizeof(uint32_t));
            o += t->blen;
            cur = r->tmpl;
        }
        if (o + NAT_GRP_WORDS > cap) return 0;
        uint32_t* d = out + o;
        memcpy(d, t->grp, sizeof t->grp);
        memcpy(d + NAT_GOFF_PAL + 2, nat_mtx[eye][r->mtx], 16 * sizeof(uint32_t));
        log[nl].at = o + NAT_GOFF_PAL + 2; log[nl].slot = r->mtx; nl++;
        memcpy(d + 22, nat_tex[r->tex0], 4 * sizeof(uint32_t));
        memcpy(d + 28, nat_tex[r->tex1], 4 * sizeof(uint32_t));
        d[32] = nat_idx_base + r->idx_off;
        d[34] = r->nidx;
        o += NAT_GRP_WORDS;
    }
    *nlog = nl;
    return o;
}

void nat_eye_copy(uint32_t* dst, const uint32_t* src, unsigned nw, const NatMtxLog* log, unsigned nlog, unsigned eye) {
    memcpy(dst, src, nw * sizeof(uint32_t));
    for (unsigned k = 0; k < nlog; k++) memcpy(dst + log[k].at, nat_mtx[eye][log[k].slot], 16 * sizeof(uint32_t));
}

/* NatRec writer (N4 step 2b). Pure C. Walk as in nat_tmpl_strip: a command is hd = w[i + 1], its p0 is w[i]. */
unsigned nat_wr_why[6]; unsigned nat_wr_off, nat_wr_base; /* N4 step 1: last offset refused by case 2 */ /* N4 step 1: refusal counts of nat_rec_write (0 tid or gm, 1 length, 2 index offset, 3 nidx, 4 count) */
int nat_rec_write(NatRec* out, unsigned nd, const uint32_t* w, unsigned n, unsigned tid, unsigned gm, uint32_t tex, uint32_t base) {
    if (tid >= NAT_TMPL_MAX || gm >= NAT_MTX_MAX) { nat_wr_why[0]++; return -1; }
    unsigned ka = 0, kc = 0, i = 0;
    while (i + 1 < n) {
        uint32_t hd = w[i + 1], reg = hd & 0x3FFu, c = (hd >> 20) & 0xFFu;
        unsigned len = 2u + c + (c & 1u);
        if (i + len > n) { nat_wr_why[1]++; return -1; }
        if (!(hd >> 31) && c == 0 && reg == 0x227) {
            uint32_t off = w[i] - base;
            if (ka >= nd || off >= 0x20000u) { nat_wr_why[2]++; nat_wr_off = off; nat_wr_base = base; return -1; }
            out[ka++].idx_off = off;
        } else if (!(hd >> 31) && c == 0 && reg == 0x228) {
            if (kc >= nd || w[i] > 0xFFFFu) { nat_wr_why[3]++; return -1; }
            out[kc++].nidx = (uint16_t)w[i];
        }
        i += len;
    }
    if (ka != nd || kc != nd) { nat_wr_why[4]++; return -1; }
    for (unsigned k = 0; k < nd; k++) {
        out[k].tmpl = (uint8_t)tid; out[k].tex0 = (uint8_t)(tex & 0xFFu); out[k].tex1 = (uint8_t)((tex >> 16) & 0xFFu);
        out[k].flags = 0; out[k].mtx = (uint16_t)gm; out[k].vtx_base = 0;
    }
    return (int)nd;
}

/* E2: texture block id by content. Id 0 is the all-zero block; ids 1..NAT_TEX_MAX-1 are found by an equal block or taken by the first empty slot. Returns -1 when full. */
int nat_tex_id(const uint32_t* blk) {
    if (!(blk[0] | blk[1] | blk[2] | blk[3])) return 0;
    for (unsigned i = 1; i < NAT_TEX_MAX; i++) {
        uint32_t* t = nat_tex[i];
        if (!(t[0] | t[1] | t[2] | t[3])) { memcpy(t, blk, 4 * sizeof(uint32_t)); return (int)i; }
        if (memcmp(t, blk, 4 * sizeof(uint32_t)) == 0) return (int)i;
    }
    return -1;
}

/* PMOD P1: bake records. ponytail: bump pool, no free; the map generation resets it. Upgrade to per-DL free when maps grow past the pool. */
NatModel nat_model[NAT_MODEL_MAX];
unsigned nat_nmodel;
static uint8_t* nat_pool_base;
static unsigned nat_pool_size, nat_pool_used;

void nat_pool_set(void* base, unsigned bytes) { nat_pool_base = (uint8_t*)base; nat_pool_size = bytes; nat_pool_used = 0; }

void nat_model_reset(void) { nat_nmodel = 0; nat_pool_used = 0; }

const uint16_t* nat_pool_put(const uint16_t* idx, unsigned n) {
    unsigned bytes = (n * 2u + 31u) & ~31u;
    if (!nat_pool_base || nat_pool_used + bytes > nat_pool_size) return NULL;
    uint16_t* d = (uint16_t*)(nat_pool_base + nat_pool_used);
    memcpy(d, idx, n * sizeof(uint16_t));
    nat_pool_used += bytes;
    return d;
}

void nat_model_bake(uint32_t dl, unsigned nseg, unsigned nv, unsigned nidx, unsigned ok) {
    if (nat_nmodel >= NAT_MODEL_MAX) return;
    NatModel* m = &nat_model[nat_nmodel++];
    m->dl = dl; m->nseg = (uint16_t)nseg; m->nv = nv; m->nidx = nidx; m->ok = (uint8_t)(ok != 0);
}
