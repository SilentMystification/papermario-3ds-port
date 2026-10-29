#include "pm_pdl.h"
/* DLC1: compile and replay of the map shape display lists (docs/plans/DLC.md, kind G). Included by pm_gbi.c after draw_tri, so it reads the statics of the interpreter.
 * Switch `dlc` (default off, `nodlc` turns it off), `dlccheck` (hash, exit state and the reject proof of every skipped group, counters in the `dlc` line).
 * A shape list is cut into groups: a group starts at a G_VTX at slot 0 after triangles, so a group reads only the vertices that it loads itself. The compiler keeps the box of the
 * object coordinates of each group. At replay a group whose box lies outside one clip plane is skipped: the interpreter rejects every triangle of it as well. A group in view runs
 * the same code as the interpreter (G_VTX copy to the GV1 ring, draw_tri) without the CPU transform of the vertices. A group that is near the camera plane, lit, or does not
 * fit the ring runs through the interpreter itself (pm_gbi_run on the op range of the group). */
unsigned pm_dlc_gen = 1; /* bumped when the shape buffer is written (pm_tex_invalidate, decode_yay0) */

typedef struct { u32 op0, op1, am, om; s16 lo[3], hi[3]; u16 nvtx, ntri, goff, ng, sn, nbat; u8 tsv_exit; u16* sd; u16* bat_q; } DlcGrp; /* C3: sd = indices then nbat batch end counts; bat_q[gop index before each batch] */
typedef struct { u32 addr, gen, hash, nops, exit_am, exit_om; u16 ngrp; u8 refused; DlcGrp* grp; u32 (*gop)[2]; } DlcEnt;
enum { DLR_OK, DLR_OP, DLR_SLOT, DLR_VTX, DLR_LONG, DLR_N };
#define DLC_TAB 2048
#define DLC_MAXOPS 4096
static DlcEnt* dlc_tab[DLC_TAB];
static unsigned dlc_nent;
static int dlc_on, dlc_depth;
static struct {
    unsigned frames, hit, compile, refused[DLR_N], full, lists, grp, skip, replay, i_near, i_lit, i_ring, i_mat, tri, opsrep, hashbad, exitbad, missing, slotfix, ringfail, srep, sfail, sfull, sfb, stri, batch;
} dlc_c;
static unsigned dlc_dirty; /* the last replay left vert_live cleared although the interpreter would have left slots live */
static const Gfx dlc_end[1] = { { { (u32)G_ENDDL << 24, 0 } } };

static int dlc_enabled(void) {
    return !dd_on() && !xt_on() && !rmlog_on() && !po3a_on();
}
static int dlcchk_on(void) { return PMDL("dlccheck"); }
static void dlc_free(DlcEnt* e) {
    for (unsigned k = 0; k < e->ngrp; k++) { free(e->grp[k].sd); free(e->grp[k].bat_q); }
    free(e->grp); free(e->gop); e->grp = NULL; e->gop = NULL; e->ngrp = 0;
}

#define DLC_GPU_GEOM(g) ((g) & (G_ZBUFFER | G_LIGHTING))

static u32 dlc_geom_at(const DlcEnt* e, const DlcGrp* g, u32 entry, unsigned sg) {
    u32 x = (entry & g->am) | g->om;
    for (unsigned i = 0; i < sg; i++) {
        const u32* op = e->gop[g->goff + i];
        x = (x & op[0]) | op[1];
    }
    return x;
}

/* C3: adjacent gop segments share the same draw geom for every list entry (PSO inputs unchanged). */
static int dlc_gop_same_geom(const DlcEnt* e, const DlcGrp* g, unsigned sg) {
    u32 a0 = dlc_geom_at(e, g, 0, sg);
    u32 a1 = dlc_geom_at(e, g, 0xffffffffu, sg);
    u32 b0 = dlc_geom_at(e, g, 0, sg + 1u);
    u32 b1 = dlc_geom_at(e, g, 0xffffffffu, sg + 1u);
    return a0 == a1 && b0 == b1 && a0 == b0;
}

/* C3: record draw ranges at compile; merge segments that share PSO geom (ends + gop index before each range). */
static void dlc_batch_compile(DlcGrp* g, const DlcEnt* e) {
    unsigned short* ends = g->sd + 3u * g->ntri;
    unsigned sn = g->sn, ng = g->ng;
    g->nbat = 0;
    free(g->bat_q);
    g->bat_q = NULL;
    if (!sn) return;
    if (!ng) {
        g->nbat = 1;
        g->bat_q = (u16*)malloc(sizeof(u16));
        if (!g->bat_q) { g->nbat = 0; return; }
        g->bat_q[0] = 0;
        ends[0] = (u16)sn;
        return;
    }
    g->nbat = (u16)(ng + 1u);
    g->bat_q = (u16*)malloc((ng + 1u) * sizeof(u16));
    if (!g->bat_q) { g->nbat = 0; return; }
    unsigned nb = 0, sg = 0;
    while (sg <= ng) {
        unsigned sr = sg;
        while (sr < ng && dlc_gop_same_geom(e, g, sr)) sr++;
        g->bat_q[nb] = (u16)sg;
        ends[nb] = ends[sr];
        nb++;
        sg = sr + 1u;
    }
    ends[nb - 1u] = (u16)sn;
    g->nbat = (u16)nb;
}

static DlcGrp dlc_sg[1024];
static u32 dlc_sgop[2048][2];
/* Cut the list into groups. Fills e. Returns the reason to refuse (DLR_OK when compiled). */
static int dlc_compile(DlcEnt* e, const Gfx* l) {
    unsigned ng = 0, ngop = 0, i;
    u32 cam = 0xffffffffu, com = 0;
    unsigned char live[64];
    int tsv = -1, in = 0;
    DlcGrp* g = NULL;
    memset(live, 2, sizeof live);
    for (i = 0; i < DLC_MAXOPS; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        unsigned op = w0 >> 24;
        if (op == G_ENDDL) break;
        switch (op) {
        case G_VTX: {
            unsigned n = (w0 >> 12) & 0xff, v0n = (w0 >> 1) & 0x7f, v0 = v0n >= n ? v0n - n : 0;
            const Vtx* src = (const Vtx*)ptr_of(w1);
            if (!src || (u32)src - (u32)gMapShapeData >= 0x40000u || n == 0 || n > 64 || v0 >= 64) return DLR_VTX;
            if (v0 + n > 64) n = 64 - v0;
            if (!in || (v0 == 0 && tsv == 1)) {
                if (g) { g->op1 = i; g->tsv_exit = (u8)(tsv == 1); }
                if (ng >= 1024) return DLR_LONG;
                g = &dlc_sg[ng++];
                memset(g, 0, sizeof *g);
                g->op0 = i; g->am = cam; g->om = com; g->goff = (u16)ngop;
                for (int q = 0; q < 3; q++) { g->lo[q] = 32767; g->hi[q] = -32768; }
                if (in) memset(live, 0, sizeof live);
                in = 1;
            }
            tsv = 0;
            for (unsigned k = 0; k < n; k++) {
                for (int q = 0; q < 3; q++) { s16 o = src[k].v.ob[q]; if (o < g->lo[q]) g->lo[q] = o; if (o > g->hi[q]) g->hi[q] = o; }
                live[v0 + k] = 1;
            }
            g->nvtx += (u16)n;
            break;
        }
        case G_TRI1:
        case G_TRI2: {
            if (!in) return DLR_SLOT;
            for (int t = 0; t < (op == G_TRI2 ? 2 : 1); t++) {
                u32 w = t ? w1 : w0;
                int ix[3] = { (int)((w >> 16) & 0xff) / 2, (int)((w >> 8) & 0xff) / 2, (int)(w & 0xff) / 2 };
                for (int k = 0; k < 3; k++) if (ix[k] <= 63 && live[ix[k]] == 2) return DLR_SLOT; /* a slot that the list did not load */
                g->ntri++;
            }
            tsv = 1;
            break;
        }
        case G_GEOMETRYMODE: {
            u32 am = w0 & 0xffffffu;
            cam &= am; com = (com & am) | w1;
            if (in) { if (ngop >= 2048) return DLR_LONG; dlc_sgop[ngop][0] = am; dlc_sgop[ngop][1] = w1; ngop++; g->ng++; }
            break;
        }
        case G_NOOP: case G_SPNOOP: case G_RDPFULLSYNC: case G_RDPTILESYNC: case G_RDPPIPESYNC: case G_RDPLOADSYNC:
            break;
        default:
            return DLR_OP;
        }
    }
    if (i >= DLC_MAXOPS) return DLR_LONG;
    if (g) { g->op1 = i; g->tsv_exit = (u8)(tsv == 1); }
    e->ngrp = (u16)ng; e->nops = i + 1; e->exit_am = cam; e->exit_om = com;
    e->grp = ng ? (DlcGrp*)malloc(ng * sizeof(DlcGrp)) : NULL;
    e->gop = ngop ? (u32(*)[2])malloc(ngop * 8) : NULL;
    if ((ng && !e->grp) || (ngop && !e->gop)) { dlc_free(e); return DLR_LONG; }
    if (ng) memcpy(e->grp, dlc_sg, ng * sizeof(DlcGrp));
    if (ngop) memcpy(e->gop, dlc_sgop, ngop * 8);
    return DLR_OK;
}

static void dlc_set_geom(u32 g);
static unsigned dlc_gmi(void);
/* DLC2: copy the Vtx of each group once into the static vertex area (flag 0) and keep the triangle indices. The slot rules are those of dlc_compile and of the interpreter
 * (a group starts with all slots dead, a triangle on a dead slot is dropped). A group without room keeps sd NULL and runs through the ring. */
static void dlc_static_build(DlcEnt* e, const Gfx* l) {
    unsigned char live[64];
    unsigned short smap[64];
    for (unsigned k = 0; k < e->ngrp; k++) {
        DlcGrp* g = &e->grp[k];
        unsigned char* dst = NULL;
        int first = pm_gpu_gstat_alloc(g->nvtx, &dst);
        if (first < 0) { dlc_c.sfull++; continue; }
        unsigned short* sd = (unsigned short*)malloc((3u * g->ntri + g->ng + 1u) * 2u);
        if (!sd) { dlc_c.sfull++; continue; }
        memset(live, 0, sizeof live);
        unsigned nv = 0, nt = 0, ngo = 0;
        for (u32 j = g->op0; j < g->op1; j++) {
            u32 w0 = l[j].words.w0, w1 = l[j].words.w1;
            switch (w0 >> 24) {
            case G_VTX: {
                unsigned n = (w0 >> 12) & 0xff, v0n = (w0 >> 1) & 0x7f, v0 = v0n >= n ? v0n - n : 0;
                const Vtx* src = (const Vtx*)ptr_of(w1);
                if (v0 + n > 64) n = 64 - v0;
                memcpy(dst + 16u * nv, src, 16u * n);
                for (unsigned i = 0; i < n; i++) {
                    *(unsigned short*)(dst + 16u * (nv + i) + 6) = 0;
                    smap[v0 + i] = (unsigned short)(first + nv + i);
                    live[v0 + i] = 1;
                }
                nv += n;
                break;
            }
            case G_TRI1:
            case G_TRI2:
                for (int t = 0; t < ((w0 >> 24) == G_TRI2 ? 2 : 1); t++) {
                    u32 w = t ? w1 : w0;
                    int ix[3] = { (int)((w >> 16) & 0xff) / 2, (int)((w >> 8) & 0xff) / 2, (int)(w & 0xff) / 2 };
                    if (ix[0] > 63 || ix[1] > 63 || ix[2] > 63 || !live[ix[0]] || !live[ix[1]] || !live[ix[2]]) continue;
                    for (int q = 0; q < 3; q++) sd[3 * nt + q] = smap[ix[q]];
                    nt++;
                }
                break;
            case G_GEOMETRYMODE:
                sd[3u * g->ntri + ngo++] = (unsigned short)nt;
                break;
            default:
                break;
            }
        }
        if (nv != g->nvtx || ngo != g->ng) { free(sd); dlc_c.sfull++; continue; } /* cannot happen: the same walk as dlc_compile */
        pm_gpu_gstat_commit((unsigned)first, nv);
        g->sn = (u16)nt; g->sd = sd;
        dlc_batch_compile(g, e);
    }
}

/* DLC2: one range of static triangles with the state of the current epoch. Returns 0 when the raw path refuses (nothing was queued). */
static int dlc_seg(const unsigned short* idx, unsigned nt, unsigned gmi) {
    unsigned tss = ((unsigned)tex_ss << 16) | ((unsigned)tex_ts & 0xffffu);
    if (ep_epoch != gv_epoch) gv_epoch_enter();
    if (ep_why >= 0) return 0;
    if (ep_cullboth) { n_out += (int)nt; return 1; }
    if (ep_pepoch != gv_epoch || ep_ptss != tss) {
        gv_params_calc(tss, &ep_p);
        pm_gpu_set_rgb_scale(ep_p.boost ? 2 : 1);
        ep_pepoch = gv_epoch; ep_ptss = tss;
    }
    int gr = pm_gpu_grange((const float(*)[4][4])gm_tab, gmi, ep_p.vp, ep_p.uvso, ep_p.cm, ep_p.ca, ep_p.zm, ep_ct, ep_p.fxon ? &ep_p.fx : NULL, idx, 3u * nt);
    if (!gr) { ep_open = 0; return 0; }
    if (gr == 2) dlc_c.batch++;
    ep_n += nt;
    ep_open = 1; ep_open_gen = pm_gpu_flush_gen(); ep_open_epoch = gv_epoch; ep_open_tss = tss;
    n_cmd += (int)nt; gp_ntri += nt; n_in += (int)nt; st_gpu_tri += nt; gv_tri_n += nt; PH_ADD(C_GVTRI, nt);
    dlc_c.stri += nt;
    return 1;
}

/* DLC2/C3: a group in view from the static area. Returns 0 when nothing was queued and the group must run through the ring. */
static int dlc_static_group(const DlcEnt* e, const DlcGrp* g, unsigned gmi, u32 gg) {
    u32 bad = (gv_lit_ok() ? 0 : G_LIGHTING) | (fogmix_on && !gv_fog_gpu_ok() ? G_FOG : 0), x = geom;
    const u32 lm = G_LIGHTING | G_TEXTURE_GEN, x0 = x & lm; /* the RSP lights at the vertex load: the lighting state must not change inside the group, else the interpreter runs it */
    if (x & bad) return 0;
    for (unsigned q = 0; q < g->ng; q++) { x = (x & e->gop[g->goff + q][0]) | e->gop[g->goff + q][1]; if ((x & bad) || (x & lm) != x0) return 0; }
    if (!g->nbat || !g->bat_q || !pm_gpu_gidx_room(3u * g->sn)) return 0;
    const unsigned short* ends = g->sd + 3u * g->ntri;
    unsigned t = 0, gop = 0;
    int drawn = 0;
    u32 geom_at = gg;
    for (unsigned b = 0; b < g->nbat; b++) {
        while (gop < g->bat_q[b])
            geom_at = (geom_at & e->gop[g->goff + gop][0]) | e->gop[g->goff + gop++][1];
        unsigned end = ends[b];
        if (end > t) {
            dlc_set_geom(geom_at);
            if (!dlc_seg(g->sd + 3u * t, end - t, gmi)) { if (!drawn) return 0; dlc_c.sfail++; return 1; }
            drawn = 1;
            t = end;
        }
    }
    if (drawn && g->nbat < g->ng + 1u) dlc_c.batch += g->ng + 1u - g->nbat;
    return drawn;
}

static DlcEnt* dlc_find(const Gfx* l) {
    u32 a = (u32)l;
    unsigned k = ((a >> 3) * 2654435761u) >> 21 & (DLC_TAB - 1);
    for (unsigned n = 0; n < DLC_TAB; n++, k = (k + 1) & (DLC_TAB - 1)) {
        DlcEnt* e = dlc_tab[k];
        if (!e) {
            if (dlc_nent >= DLC_TAB * 3 / 4) { dlc_c.full++; return NULL; }
            e = (DlcEnt*)calloc(1, sizeof *e);
            if (!e) return NULL;
            e->addr = a; dlc_tab[k] = e; dlc_nent++;
            return e;
        }
        if (e->addr == a) return e;
    }
    return NULL;
}

static void dlc_set_geom(u32 g) {
    if (g == geom) return;
    geom = g;
    pm_gpu_set_mode((geom & G_ZBUFFER) != 0, (geom & G_LIGHTING) != 0);
    gv_epoch++;
}

/* The matrix id of `combined` (the same search as the G_VTX code), GV_NONE when the table is full. */
static unsigned dlc_gmi(void) { return gv_matrix_id(0); }

/* Box test against the clip planes of clip_bits4. Returns 1 when every vertex of the box is outside one plane (by a margin for the float error of the interpreter). */
static int dlc_box_out(const DlcGrp* g, const float (*m)[4]) {
    static const struct { signed char a, b; } pl[5] = { { 0, 1 }, { 0, -1 }, { 1, 1 }, { 1, -1 }, { 2, -1 } }; /* coordinate, sign of w: x + w < 0, x - w > 0, y + w < 0, y - w > 0, z - w > 0 */
    float lo[3] = { g->lo[0], g->lo[1], g->lo[2] }, hi[3] = { g->hi[0], g->hi[1], g->hi[2] };
    int out = 0;
    for (int p = 0; p < 5; p++) {
        float k = m[3][pl[p].a] + (float)pl[p].b * m[3][3], mn = k, mx = k, mag = k < 0 ? -k : k;
        for (int q = 0; q < 3; q++) {
            float c = m[q][pl[p].a] + (float)pl[p].b * m[q][3], x = c * lo[q], y = c * hi[q];
            if (x < y) { mn += x; mx += y; } else { mn += y; mx += x; }
            mag += (x < 0 ? -x : x) > (y < 0 ? -y : y) ? (x < 0 ? -x : x) : (y < 0 ? -y : y);
        }
        float eps = mag * 1e-5f + 1e-3f;
        if (pl[p].b > 0 ? mx < -eps : mn > eps) out = 1;
    }
    return out;
}

enum { GP1_IN, GP1_OUT, GP1_CROSS };

/* AND of clip_bits4 on the 8 corners of an object-space box (subset P for GP1 out loads). */
static unsigned char gp1_box_subset(const s16 lo[3], const s16 hi[3], const float (*m)[4]) {
    unsigned bits = 0x3f;
    for (int i = 0; i < 8; i++) {
        float x = (float)((i & 1) ? hi[0] : lo[0]), y = (float)((i & 2) ? hi[1] : lo[1]), z = (float)((i & 4) ? hi[2] : lo[2]);
        float p[4];
        for (int c = 0; c < 4; c++) p[c] = m[0][c] * x + m[1][c] * y + m[2][c] * z + m[3][c];
        bits &= (unsigned)clip_bits4(p);
    }
    return (unsigned char)bits;
}

/* 1 when every corner is inside all clip planes of clip_bits4. */
static int gp1_box_in(const s16 lo[3], const s16 hi[3], const float (*m)[4]) {
    static const struct { signed char a, b; } pl[5] = { { 0, 1 }, { 0, -1 }, { 1, 1 }, { 1, -1 }, { 2, -1 } };
    for (int p = 0; p < 5; p++) {
        float k = m[3][pl[p].a] + (float)pl[p].b * m[3][3], mn = k, mx = k, mag = k < 0 ? -k : k;
        for (int q = 0; q < 3; q++) {
            float c = m[q][pl[p].a] + (float)pl[p].b * m[q][3], x = c * (float)lo[q], y = c * (float)hi[q];
            if (x < y) { mn += x; mx += y; } else { mn += y; mx += x; }
            mag += (x < 0 ? -x : x) > (y < 0 ? -y : y) ? (x < 0 ? -x : x) : (y < 0 ? -y : y);
        }
        float eps = mag * 1e-5f + 1e-3f;
        if (pl[p].b > 0 ? mn < -eps : mx > eps) return 0;
    }
    return 1;
}

/* GP1: exact hybrid box per non-DLC G_VTX load. */
static int gp1_box_class(const s16 lo[3], const s16 hi[3], const float (*m)[4], unsigned char* pbits) {
    DlcGrp g;
    g.lo[0] = lo[0]; g.lo[1] = lo[1]; g.lo[2] = lo[2];
    g.hi[0] = hi[0]; g.hi[1] = hi[1]; g.hi[2] = hi[2];
    int out = dlc_box_out(&g, m);
    if (gp1_box_in(lo, hi, m)) return GP1_IN;
    if (out) {
        *pbits = gp1_box_subset(lo, hi, m);
        return GP1_OUT;
    }
    return GP1_CROSS;
}

/* dlccheck: the interpreter rejects every triangle of a skipped group (all three clip bits share one plane). Counts the triangles that it would not reject. */
static void dlc_verify_skip(const Gfx* l, const DlcGrp* g, const float (*m)[4]) {
    unsigned char cb[64], live[64] = { 0 };
    int tsv = 1;
    for (u32 j = g->op0; j < g->op1; j++) {
        u32 w0 = l[j].words.w0, w1 = l[j].words.w1;
        unsigned op = w0 >> 24;
        if (op == G_VTX) {
            unsigned n = (w0 >> 12) & 0xff, v0n = (w0 >> 1) & 0x7f, v0 = v0n >= n ? v0n - n : 0;
            const Vtx* src = (const Vtx*)ptr_of(w1);
            if (v0 + n > 64) n = 64 - v0;
            if (v0 == 0 && tsv) memset(live, 0, sizeof live);
            tsv = 0;
            for (unsigned k = 0; k < n; k++) {
                float x = src[k].v.ob[0], y = src[k].v.ob[1], z = src[k].v.ob[2], p[4];
                for (int q = 0; q < 4; q++) p[q] = m[0][q] * x + m[1][q] * y + m[2][q] * z + m[3][q];
                cb[v0 + k] = (unsigned char)clip_bits4(p); live[v0 + k] = 1;
            }
        } else if (op == G_TRI1 || op == G_TRI2) {
            tsv = 1;
            for (int t = 0; t < (op == G_TRI2 ? 2 : 1); t++) {
                u32 w = t ? w1 : w0;
                int a = (int)((w >> 16) & 0xff) / 2, b = (int)((w >> 8) & 0xff) / 2, c = (int)(w & 0xff) / 2;
                if (a > 63 || b > 63 || c > 63 || !live[a] || !live[b] || !live[c]) continue;
                if (!(cb[a] & cb[b] & cb[c] & 0x2f)) dlc_c.missing++;
            }
        }
    }
}

/* One group in view: the interpreter code for G_VTX (GV1 ring copy, no CPU transform) and draw_tri. Returns 0 on success. */
static void dlc_run_group(const Gfx* l, const DlcGrp* g, unsigned gmi) {
    unsigned tss = ((unsigned)tex_ss << 16) | ((unsigned)tex_ts & 0xffffu);
    for (u32 j = g->op0; j < g->op1; j++) {
        u32 w0 = l[j].words.w0, w1 = l[j].words.w1;
        switch (w0 >> 24) {
        case G_VTX: {
            RS_ADD(RS_DLC, 1);
            unsigned n = (w0 >> 12) & 0xff, v0n = (w0 >> 1) & 0x7f, v0 = v0n >= n ? v0n - n : 0;
            const Vtx* src = (const Vtx*)ptr_of(w1);
            if (v0 + n > 64) n = 64 - v0;
            PH_ADD(C_VTXLOAD, n);
            if (v0 == 0 && tri_since_vtx) memset(vert_live, 0, sizeof vert_live);
            tri_since_vtx = 0;
            int r = pm_gpu_gvtx_load(src, n);
            if (r < 0) { dlc_c.ringfail++; gv_fb[GVR_RING]++; break; } /* cannot happen: the room test counted every vertex and every copy */
            gv_load_n += n; PH_ADD(C_GVLOAD, n);
            for (unsigned i = 0; i < n; i++) {
                unsigned s = v0 + i;
                vert_ring[s] = (unsigned short)(r + (int)i);
                vert_gmi[s] = (unsigned short)gmi;
                vert_tss[s] = tss;
                vert_mid[s] = 0;
                vert_live[s] = 1;
                vert_scr[s] = 0;
                vert_gvf[s] = 0;
                vert_cb[s] = 0;
            }
            break;
        }
        case G_TRI1:
            RS_ADD(RS_DLC, 1);
            tri_prep_ok = 0;
            draw_tri(w0);
            break;
        case G_TRI2:
            RS_ADD(RS_DLC, 2);
            tri_prep_ok = 0;
            draw_tri(w0);
            draw_tri(w1);
            break;
        case G_GEOMETRYMODE:
            geom = (geom & (w0 & 0xffffffu)) | w1;
            pm_gpu_set_mode((geom & G_ZBUFFER) != 0, (geom & G_LIGHTING) != 0);
            gv_epoch++;
            break;
        default:
            break;
        }
    }
}

/* the shape buffer was written: every entry is stale */
static void dlc_reset(void) {
    for (unsigned k = 0; k < DLC_TAB; k++) if (dlc_tab[k]) { dlc_free(dlc_tab[k]); free(dlc_tab[k]); dlc_tab[k] = NULL; }
    dlc_nent = 0;
    pm_gpu_gstat_reset();
}
static unsigned dlc_seen_gen;
/* A G_DL to l. Returns 1 when the list is done (replayed), 0 when the interpreter must run it. */
static int dlc_try(const Gfx* l) {
    if ((u32)l - (u32)gMapShapeData >= 0x40000u || ((u32)l & 7)) return 0;
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    DlcEnt* e = dlc_find(l);
    if (!e) return 0;
    int chk = dlcchk_on();
    if (e->gen != pm_dlc_gen) {
        dlc_free(e);
        e->gen = pm_dlc_gen; e->refused = 0;
        int r = dlc_compile(e, l);
        e->hash = dls_hash(l);
        dlc_c.compile++;
        RS_ADD(RS_DLCM, 1);
        if (r) { e->refused = (u8)r; dlc_c.refused[r]++; }
        else dlc_static_build(e, l);
    } else { dlc_c.hit++; RS_ADD(RS_DLCH, 1); }
    if (e->refused) return 0;
    if (chk && dls_hash(l) != e->hash) { dlc_c.hashbad++; e->gen = 0; return 0; }
    const float(*m)[4] = combined;
    u32 entry = geom;
    unsigned gmi = GV_NONE;
    int gmi_ready = 0, skipped_last = 0, skipped_any = 0;
    dlc_c.lists++;
    dlc_c.opsrep += e->nops;
    RS_ADD(RS_DLCOPS, e->nops);
    tb_probe(RS_TBDL, l, e->nops);
    if (e->ngrp) { __builtin_prefetch(e->grp); __builtin_prefetch(e->gop); }
    for (unsigned k = 0; k < e->ngrp; k++) {
        const DlcGrp* g = &e->grp[k];
        if (k + 1 < e->ngrp) { __builtin_prefetch(g + 1); __builtin_prefetch(g[1].sd); }
        dlc_c.grp++;
        RS_ADD(RS_DLCG, 1);
        if (dlc_box_out(g, m)) {
            if (chk) dlc_verify_skip(l, g, m);
            tri_since_vtx = g->tsv_exit;
            skipped_last = skipped_any = 1;
            dlc_c.skip++;
            RS_ADD(RS_DLCSKIP, 1);
            continue;
        }
        skipped_last = 0;
        u32 gg = (entry & g->am) | g->om;
        int why = 0;
        int lit = 0; /* GV5: a lit group is replayed from the static area (the GPU lights it) or runs through the interpreter */
        if (!why) {
            u32 x = gg;
            lit = (x & G_LIGHTING) != 0;
            for (unsigned q = 0; q < g->ng; q++) { x = (x & e->gop[g->goff + q][0]) | e->gop[g->goff + q][1]; if (x & G_LIGHTING) lit = 1; }
            if (lit && !(g->sd && gv_lit_ok())) why = 2;
        }
        if (!why && !pm_gpu_gv_room(g->nvtx + 3u * g->ntri)) why = 3;
        if (!why) {
            if (lit) gmi = gv_matrix_id(1); /* the normal matrix is part of the entry */
            else if (!gmi_ready) { gmi = dlc_gmi(); gmi_ready = 1; }
            if (gmi == GV_NONE) why = 4;
        }
        dlc_set_geom(gg);
        if (!why && g->sd && dlc_static_group(e, g, gmi, gg)) {
            dlc_c.replay++; dlc_c.srep++; dlc_c.tri += g->ntri;
            tri_since_vtx = g->tsv_exit;
            skipped_last = skipped_any = 1;
            continue;
        }
        if (!why && lit) why = 2; /* the static group was not taken */
        if (why) {
            if (why == 2) dlc_c.i_lit++; else if (why == 3) dlc_c.i_ring++; else if (why == 4) dlc_c.i_mat++;
            dlc_depth++;
            pm_gbi_run((void*)(l + g->op0), (g->op1 - g->op0) * 8u);
            dlc_depth--;
            continue;
        }
        dlc_c.replay++; dlc_c.tri += g->ntri;
        dlc_run_group(l, g, gmi);
    }
    dlc_set_geom((entry & e->exit_am) | e->exit_om);
    if (skipped_last) memset(vert_live, 0, sizeof vert_live);
    if (skipped_any) dlc_dirty = 1;
    if (chk) { /* the exit state against a plain walk of the list */
        u32 g2 = entry;
        int anyg = 0;
        for (unsigned i = 0; i < DLC_MAXOPS; i++) {
            u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
            if ((w0 >> 24) == G_ENDDL) break;
            if ((w0 >> 24) == G_GEOMETRYMODE) g2 = (g2 & (w0 & 0xffffffu)) | w1;
            if ((w0 >> 24) == G_VTX) anyg = 1;
        }
        if (g2 != geom) dlc_c.exitbad++;
        if (anyg && e->ngrp && tri_since_vtx != e->grp[e->ngrp - 1].tsv_exit) dlc_c.exitbad++;
    }
    return 1;
}

static void dlc_report(void) {
    pm_log_hw("dlc2 f%d tri %u opsrep %u | hashbad %u exitbad %u missing %u slotfix %u ringfail %u ents %u | static grp %u tri %u fail %u full %u batch %u kb %u", rm_frame, dlc_c.tri, dlc_c.opsrep, dlc_c.hashbad, dlc_c.exitbad, dlc_c.missing, dlc_c.slotfix, dlc_c.ringfail, dlc_nent, dlc_c.srep, dlc_c.stri, dlc_c.sfail, dlc_c.sfull, dlc_c.batch, pm_gpu_gstat_used() / 64u);
    memset(&dlc_c, 0, sizeof dlc_c);
}

/* C0: profile static state lists (texheap and rodata). Switch pdl0. One pdl0 line every 300 drawn frames. */
enum {
    GBIG_TILE = 1u, GBIG_TILESZ = 2u, GBIG_TMEM = 4u, GBIG_TIMG = 8u, GBIG_TLUT = 16u, GBIG_COMB = 32u, GBIG_OMH = 64u, GBIG_OML = 128u,
    GBIG_GEOM = 256u, GBIG_TEX = 512u, GBIG_PRIM = 1024u, GBIG_ENV = 2048u, GBIG_FOG = 4096u, GBIG_BLEND = 8192u, GBIG_KEYS = 16384u,
    GBIG_PRIMZ = 32768u, GBIG_SCISSOR = 65536u, GBIG_LIGHT = 131072u, GBIG_CIMG = 262144u
};
/* D1: interpreter globals by GBIG group. gbi_pack copies the fields of the groups in mask, in table order, into dst and returns the byte count. */
typedef struct { void* p; u16 n; u32 grp; } GbiFld;
static const GbiFld gbi_fld[] = {
    { &timg, sizeof timg, GBIG_TIMG }, { &timg_fmt, sizeof timg_fmt, GBIG_TIMG }, { &timg_siz, sizeof timg_siz, GBIG_TIMG },
    { &timg_stride, sizeof timg_stride, GBIG_TIMG }, { &ld_valid, sizeof ld_valid, GBIG_TIMG }, { &ld_x, sizeof ld_x, GBIG_TIMG },
    { &ld_y, sizeof ld_y, GBIG_TIMG }, { &ld_h, sizeof ld_h, GBIG_TIMG }, { &ld_w, sizeof ld_w, GBIG_TIMG },
    { tile_fmt, sizeof tile_fmt, GBIG_TILE }, { tile_siz, sizeof tile_siz, GBIG_TILE }, { tile_tmem, sizeof tile_tmem, GBIG_TILE },
    { tile_pal, sizeof tile_pal, GBIG_TILE }, { tile_wrap, sizeof tile_wrap, GBIG_TILE }, { tile_shift, sizeof tile_shift, GBIG_TILE },
    { ldrec, sizeof ldrec, GBIG_TMEM }, { &ldrec_n, sizeof ldrec_n, GBIG_TMEM }, { blk, sizeof blk, GBIG_TMEM }, { &blk_n, sizeof blk_n, GBIG_TMEM },
    { &tlut, sizeof tlut, GBIG_TLUT }, { &tlut_n, sizeof tlut_n, GBIG_TLUT }, { tlut_pal, sizeof tlut_pal, GBIG_TLUT },
    { tile_sz, sizeof tile_sz, GBIG_TILESZ }, { td, sizeof td, GBIG_TILESZ }, { &m1, sizeof m1, GBIG_TILESZ }, { &tile_x0, sizeof tile_x0, GBIG_TILESZ },
    { &tile_y0, sizeof tile_y0, GBIG_TILESZ }, { &tile_fx, sizeof tile_fx, GBIG_TILESZ }, { &tile_fy, sizeof tile_fy, GBIG_TILESZ }, { &cur_tile, sizeof cur_tile, GBIG_TILESZ },
    { &comb_w0, sizeof comb_w0, GBIG_COMB }, { &comb_w1, sizeof comb_w1, GBIG_COMB },
    { &omode_h, sizeof omode_h, GBIG_OMH }, { &omode_l, sizeof omode_l, GBIG_OML }, { &geom, sizeof geom, GBIG_GEOM },
    { &tex_on, sizeof tex_on, GBIG_TEX }, { &tex_ss, sizeof tex_ss, GBIG_TEX }, { &tex_ts, sizeof tex_ts, GBIG_TEX },
    { &prim, sizeof prim, GBIG_PRIM }, { &envc, sizeof envc, GBIG_ENV }, { &fogc, sizeof fogc, GBIG_FOG }, { &fog_fm, sizeof fog_fm, GBIG_FOG },
    { &fog_fo, sizeof fog_fo, GBIG_FOG }, { &fogmix_on, sizeof fogmix_on, GBIG_FOG }, { &blendc, sizeof blendc, GBIG_BLEND },
    { &keyc, sizeof keyc, GBIG_KEYS }, { &keys, sizeof keys, GBIG_KEYS }, { &conv_k4, sizeof conv_k4, GBIG_KEYS }, { &prim_z, sizeof prim_z, GBIG_PRIMZ },
    { &sci_x0, sizeof sci_x0, GBIG_SCISSOR }, { &sci_y0, sizeof sci_y0, GBIG_SCISSOR }, { &sci_x1, sizeof sci_x1, GBIG_SCISSOR }, { &sci_y1, sizeof sci_y1, GBIG_SCISSOR },
};
static unsigned gbi_pack(unsigned mask, u8* dst) {
    unsigned n = 0;
    for (unsigned i = 0; i < sizeof gbi_fld / sizeof gbi_fld[0]; i++)
        if (gbi_fld[i].grp & mask) { memcpy(dst + n, gbi_fld[i].p, gbi_fld[i].n); n += gbi_fld[i].n; }
    return n;
}

static int pdl0_on(void) { return PMDL("pdl0"); }
static struct { unsigned calls, rep_ok, rss_sum, sin_new, lists; } pdl0_c;
static unsigned pdl0_frames;
#define PDL_META 512
static struct { u32 addr; u8 kind; unsigned readset; } pdl_meta[PDL_META];
#define PDL_KEY 2048
static struct { u32 addr, state_in, exit_h; } pdl_prv[PDL_KEY], pdl_cur[PDL_KEY];
static struct { u32 addr, state_in; } pdl_uniq[4096];
static unsigned pdl_uniq_n;
static struct { u32 addr, state_in; } pdl_stk[12];
static int pdl_sp;

static void pdl_op_rw(unsigned op, unsigned* rd, unsigned* wr) {
    *rd = *wr = 0;
    switch (op) {
    case G_SETTIMG: *wr = GBIG_TIMG; break;
    case G_SETTILE: *wr = GBIG_TILE; break;
    case G_LOADBLOCK: case G_LOADTILE: *rd = GBIG_TIMG | GBIG_TILE | GBIG_TMEM; *wr = GBIG_TMEM; break;
    case G_LOADTLUT: *rd = GBIG_TIMG | GBIG_TILE | GBIG_TLUT | GBIG_TILESZ; *wr = GBIG_TLUT; break;
    case G_SETTILESIZE: *rd = GBIG_TIMG | GBIG_TILE | GBIG_TMEM | GBIG_TLUT; *wr = GBIG_TILESZ | GBIG_TILE; break;
    case G_SETCOMBINE: *wr = GBIG_COMB; break;
    case G_SETOTHERMODE_H: *wr = GBIG_OMH; break;
    case G_SETOTHERMODE_L: *wr = GBIG_OML; break;
    case G_GEOMETRYMODE: *wr = GBIG_GEOM; break;
    case G_TEXTURE: *wr = GBIG_TEX; break;
    case G_SETPRIMCOLOR: *wr = GBIG_PRIM; break;
    case G_SETENVCOLOR: *wr = GBIG_ENV; break;
    case G_SETFOGCOLOR: *wr = GBIG_FOG; break;
    case G_SETBLENDCOLOR: *wr = GBIG_BLEND; break;
    case G_SETCONVERT: case G_SETKEYR: case G_SETKEYGB: *wr = GBIG_KEYS; break;
    case G_SETPRIMDEPTH: *wr = GBIG_PRIMZ; break;
    case G_SETSCISSOR: *wr = GBIG_SCISSOR; break;
    case G_SETCIMG: *wr = GBIG_CIMG; break;
    case G_MOVEWORD: *wr = GBIG_FOG; break;
    default: break;
    }
}

static int pdl_kind_s(const Gfx* l) {
    u32 a = (u32)l;
    unsigned k = (a >> 3) * 2654435761u >> 23 & (PDL_META - 1);
    for (unsigned n = 0; n < PDL_META; n++, k = (k + 1) & (PDL_META - 1)) {
        if (pdl_meta[k].addr == a) return pdl_meta[k].kind == 1;
        if (!pdl_meta[k].addr) break;
    }
    int geo = 0;
    unsigned written = 0, readset = 0;
    for (unsigned i = 0; i < 4096; i++) {
        u32 w0 = l[i].words.w0;
        unsigned op = w0 >> 24;
        if (op == G_ENDDL) break;
        if (op == G_VTX || op == G_TRI1 || op == G_TRI2 || op == G_MODIFYVTX || op == G_MTX) { geo = 1; break; }
        unsigned rd, wr; pdl_op_rw(op, &rd, &wr);
        readset |= rd & ~written;
        written |= wr;
    }
    for (unsigned n = 0; n < PDL_META; n++, k = (k + 1) & (PDL_META - 1)) {
        if (!pdl_meta[k].addr || pdl_meta[k].addr == a) {
            pdl_meta[k].addr = a; pdl_meta[k].kind = geo ? 2 : 1; pdl_meta[k].readset = geo ? 0 : readset;
            break;
        }
    }
    return !geo;
}

static unsigned pdl_readset_of(const Gfx* l) {
    u32 a = (u32)l;
    unsigned k = (a >> 3) * 2654435761u >> 23 & (PDL_META - 1);
    for (unsigned n = 0; n < PDL_META; n++, k = (k + 1) & (PDL_META - 1)) if (pdl_meta[k].addr == a) return pdl_meta[k].readset;
    return 0;
}

static void pdl_exit_comb_from_list(GbiState* ex, const Gfx* l, unsigned n) {
    for (unsigned i = 0; i < n; i++) {
        if ((l[i].words.w0 >> 24) == G_SETCOMBINE) {
            ex->comb_w0 = l[i].words.w0;
            ex->comb_w1 = l[i].words.w1;
        }
    }
}

static u32 gbi_state_in_hash(const GbiState* s, unsigned readset) {
    if (!readset) return gbi_state_hash(s);
    u32 h = 2166136261u;
    if (readset & GBIG_TILE) h = (h ^ gbi_state_fnv(s->tile_tmem, sizeof s->tile_tmem)) * 16777619u;
    if (readset & GBIG_TILESZ) { h = (h ^ (unsigned)s->tile_x0 ^ (unsigned)s->tile_y0) * 16777619u; h = (h ^ (unsigned)s->tile_fx) * 16777619u; }
    if (readset & GBIG_TMEM) h = (h ^ gbi_state_fnv(s->ldrec, sizeof s->ldrec)) * 16777619u;
    if (readset & GBIG_TIMG) h = (h ^ (unsigned)(uintptr_t)s->timg) * 16777619u;
    if (readset & GBIG_TLUT) h = (h ^ (unsigned)(uintptr_t)s->tlut) * 16777619u;
    if (readset & GBIG_COMB) h = (h ^ s->comb_w0 ^ s->comb_w1) * 16777619u;
    if (readset & GBIG_OMH) h = (h ^ s->omode_h) * 16777619u;
    if (readset & GBIG_OML) h = (h ^ s->omode_l) * 16777619u;
    if (readset & GBIG_GEOM) h = (h ^ s->geom) * 16777619u;
    if (readset & GBIG_TEX) h = (h ^ s->tex_ss ^ s->tex_ts) * 16777619u;
    if (readset & GBIG_PRIM) h = (h ^ s->prim) * 16777619u;
    if (readset & GBIG_ENV) h = (h ^ s->envc) * 16777619u;
    if (readset & GBIG_FOG) h = (h ^ s->fogc ^ (unsigned)s->fog_fm) * 16777619u;
    if (readset & GBIG_BLEND) h = (h ^ s->blendc) * 16777619u;
    if (readset & GBIG_KEYS) h = (h ^ s->keyc ^ s->keys ^ s->conv_k4) * 16777619u;
    if (readset & GBIG_PRIMZ) h = (h ^ s->prim_z) * 16777619u;
    if (readset & GBIG_SCISSOR) h = (h ^ (unsigned)s->sci_x0 ^ (unsigned)s->sci_y0) * 16777619u;
    if (readset & GBIG_LIGHT) h = (h ^ gbi_state_fnv(s->lt, sizeof s->lt)) * 16777619u;
    if (readset & GBIG_CIMG) h = (h ^ (unsigned)(uintptr_t)s->cimg) * 16777619u;
    return h;
}

static int pdl_prv_hit(u32 addr, u32 state_in, u32 exit_h) {
    unsigned k = ((addr >> 3) ^ state_in) * 2654435761u >> 21 & (PDL_KEY - 1);
    for (unsigned n = 0; n < PDL_KEY; n++, k = (k + 1) & (PDL_KEY - 1)) {
        if (pdl_prv[k].addr == addr && pdl_prv[k].state_in == state_in) return pdl_prv[k].exit_h == exit_h;
        if (!pdl_prv[k].addr) break;
    }
    return 0;
}

static void pdl_cur_put(u32 addr, u32 state_in, u32 exit_h) {
    unsigned k = ((addr >> 3) ^ state_in) * 2654435761u >> 21 & (PDL_KEY - 1);
    for (unsigned n = 0; n < PDL_KEY; n++, k = (k + 1) & (PDL_KEY - 1)) {
        if (!pdl_cur[k].addr || (pdl_cur[k].addr == addr && pdl_cur[k].state_in == state_in)) {
            pdl_cur[k].addr = addr; pdl_cur[k].state_in = state_in; pdl_cur[k].exit_h = exit_h;
            return;
        }
    }
}

static GbiState pdl0_gl_enter_st;
static GbiState pdl0_gl_leave_st;

static void pdl0_gl_enter(const Gfx* l) {
    if (!l || !is_host((u32)l)) return;
    if (!dls_rd_lo) dls_init_ranges();
    unsigned c = dls_class((u32)l);
    if (c != 2 && c != 3) return;
    if (!pdl_kind_s(l)) return;
    gbi_state_get(&pdl0_gl_enter_st);
    unsigned rs = pdl_readset_of(l);
    u32 sin = gbi_state_in_hash(&pdl0_gl_enter_st, rs);
    unsigned rss = 0;
    for (unsigned b = rs; b; b &= b - 1u) rss++;
    if (pdl_sp < 12) { pdl_stk[pdl_sp].addr = (u32)l; pdl_stk[pdl_sp].state_in = sin; pdl_sp++; }
    pdl0_c.calls++; pdl0_c.rss_sum += rss;
}

static void pdl0_gl_leave(void) {
    if (!pdl_sp) return;
    u32 addr = pdl_stk[pdl_sp - 1].addr, sin = pdl_stk[pdl_sp - 1].state_in;
    pdl_sp--;
    gbi_state_get(&pdl0_gl_leave_st);
    u32 ex = gbi_state_hash(&pdl0_gl_leave_st);
    if (pdl_prv_hit(addr, sin, ex)) pdl0_c.rep_ok++;
    pdl_cur_put(addr, sin, ex);
    unsigned j;
    for (j = 0; j < pdl_uniq_n; j++) if (pdl_uniq[j].addr == addr && pdl_uniq[j].state_in == sin) break;
    if (j == pdl_uniq_n && pdl_uniq_n < 4096u) { pdl_uniq[pdl_uniq_n].addr = addr; pdl_uniq[pdl_uniq_n].state_in = sin; pdl_uniq_n++; pdl0_c.sin_new++; }
    pdl0_c.lists++;
}

static void pdl0_frame_end(void) {
    memcpy(pdl_prv, pdl_cur, sizeof pdl_cur);
    memset(pdl_cur, 0, sizeof pdl_cur);
    pdl_uniq_n = 0;
    pdl_sp = 0;
}

static void pdl0_report(void) {
    unsigned rep = pdl0_c.calls ? pdl0_c.rep_ok * 100u / pdl0_c.calls : 0;
    unsigned rss = pdl0_c.calls ? pdl0_c.rss_sum / pdl0_c.calls : 0;
    unsigned sin = pdl0_c.lists ? pdl0_c.sin_new / pdl0_c.lists : 0;
    pm_log_hw("pdl0 f%d n %u calls %u lists %u rss %u sin %u rep %u%%", rm_frame, pdl0_frames, pdl0_c.calls, pdl0_c.lists, rss, sin, rep);
    memset(&pdl0_c, 0, sizeof pdl0_c);
    pdl0_frames = 0;
}

/* C1/C2: compile and replay static state lists (TextureHeap and rodata). No separate switch: runs with dlc_enabled(). */

typedef struct {
    u8 valid;
    int why, ct, cullboth, fl;
    GvP p;
} PsoRec;

#define PSO_N 512
#define PDL_TAB 1024
typedef struct {
    u32 addr, heap_gen, state_in;
    unsigned nops;
    u8 refused;
    u16 pso_id;
    int tex_slot;
    unsigned tex_gen;
    GbiState exit;
} PdlEnt;

static PsoRec pso_tab[PSO_N];
static unsigned pso_n = 1;
static PdlEnt* pdl_tab[PDL_TAB];
static unsigned pdl_nent;
static int pdl_depth;
static struct { unsigned hit, comp, replay, refuse, full, exitbad, drawbad, texbad; } pdl_c;

static unsigned pdl_cls[8]; /* dl1: pdl_try calls per dls_class (before the class gate) */

static int pdlchk_on(void) { return PMDL("pdlcheck"); }

static unsigned pdl_nops(const Gfx* l) {
    for (unsigned i = 0; i < DLC_MAXOPS; i++)
        if ((l[i].words.w0 >> 24) == G_ENDDL) return i + 1u;
    return DLC_MAXOPS;
}

static int pdl_refuse_ops(const Gfx* l) {
    if (!pdl_kind_s(l)) return 1;
    for (unsigned i = 0; i < DLC_MAXOPS; i++) {
        unsigned op = l[i].words.w0 >> 24;
        if (op == G_ENDDL) return 0;
        if (op == G_DL || op == G_MODIFYVTX || op == G_SETCIMG || op == G_SETZIMG || op == G_FILLRECT || op == G_TEXRECT || op == G_TEXRECTFLIP || op == G_MTX || op == G_MOVEMEM || (op == G_MOVEWORD && ((l[i].words.w0 >> 16) & 0xff) != G_MW_FOG))
            return 1;
        unsigned rd, wr; pdl_op_rw(op, &rd, &wr);
        if (!rd && !wr && op != G_NOOP && op != G_SPNOOP && op != G_RDPFULLSYNC && op != G_RDPTILESYNC && op != G_RDPPIPESYNC && op != G_RDPLOADSYNC)
            return 1;
    }
    return 1;
}

static PdlEnt* pdl_find(u32 a) {
    unsigned k = ((a >> 3) * 2654435761u) >> 21 & (PDL_TAB - 1);
    for (unsigned n = 0; n < PDL_TAB; n++, k = (k + 1) & (PDL_TAB - 1)) {
        PdlEnt* e = pdl_tab[k];
        if (!e) {
            if (pdl_nent >= PDL_TAB * 3 / 4) { pdl_c.full++; return NULL; }
            e = (PdlEnt*)calloc(1, sizeof *e);
            if (!e) return NULL;
            e->addr = a; pdl_tab[k] = e; pdl_nent++;
            return e;
        }
        if (e->addr == a) return e;
    }
    return NULL;
}

static u32 pdl_heap_gen(u32 a) {
    switch (dls_class(a)) {
    case 2: return pm_pdl_heap_gen;
    case 4: return pm_pdl_entity_gen;
    case 5: return pm_pdl_genheap_gen;
    case 6: return pm_pdl_sprite_gen;
    default: return 0;
    }
}

static unsigned pdl_pso_capture(void) {
    unsigned id = pso_n++;
    if (id >= PSO_N) id = PSO_N - 1;
    PsoRec* r = &pso_tab[id];
    r->valid = 1;
    r->why = ep_why; r->ct = ep_ct; r->cullboth = ep_cullboth; r->fl = ep_fl;
    r->p = ep_p;
    pm_pso_bake(id);
    return id;
}

unsigned pm_pdl_flush_pso(void) { return pdl_pso_cur ? pdl_pso_cur : 0; }

static void pdl_pso_use(void) {
    if (!pdl_pso_cur || pdl_pso_cur >= PSO_N || !pso_tab[pdl_pso_cur].valid) return;
    ep_flush_stats();
    PsoRec* r = &pso_tab[pdl_pso_cur];
    ep_why = r->why; ep_ct = r->ct; ep_cullboth = r->cullboth; ep_fl = r->fl;
    ep_p = r->p;
    ep_pepoch = 0;
    pdl_ep_pso = pdl_pso_cur;
}

#define PDL_TEX_OFF (-2) /* replay: pm_tex_bind for an off-screen tile 0 texture */

static int pdl_lazy_tile(int n) {
    if (n != 0) return 0;
    if (td[0].valid) return 0; /* SETTILESIZE/GP3: pm_tex_load from td[0], not a stale PDL cache slot */
    if (pdl_tex_slot == PDL_TEX_OFF) {
        if (pm_tex_cur_idx() < 0) pm_gpu_flush();
        if (!pm_tex_bind()) return 0;
        cur_tile = 0;
        pdl_tex_slot = -1;
        return 1;
    }
    if (pdl_tex_slot < 0) return 0;
    if (pm_tex_slot_gen(pdl_tex_slot) != pdl_tex_gen) { pdl_tex_slot = -1; pdl_c.texbad++; return 0; }
    if (pm_tex_cur_idx() != pdl_tex_slot) pm_gpu_flush();
    if (!pm_tex_bind_slot(pdl_tex_slot)) return 0;
    cur_tile = 0;
    pdl_tex_slot = -1;
    return 1;
}

static GbiState pdl_comp_gbi_st;
static GbiState pdl_try_gbi_st;
static GbiState pdl_try_gbi_chk;
static PdlEnt* pdl_comp_ent;
static const Gfx* pdl_comp_list;
static int pdl_comp_sp;
static int pdl_comp_nopush;

/* pdl key: the replay applies the full exit state, so the full entry state and the segment table must match */
static u32 pdl_key(const GbiState* s) { return gbi_state_hash(s) ^ gbi_state_fnv(segbase, sizeof segbase) * 31u; }

static void pdl_compile_finish(void) {
    PdlEnt* e = pdl_comp_ent;
    if (!e) return;
    const Gfx* lst = pdl_comp_list;
    unsigned nops = e->nops;
    pdl_depth--;
    pdl_compile_one = 0;
    pdl_comp_ent = NULL;
    pdl_comp_list = NULL;
    pdl_comp_sp = -1;
    gbi_state_get(&e->exit);
    if (lst) pdl_exit_comb_from_list(&e->exit, lst, nops);
    if (pm_tex_off_idx() >= 0) {
        e->tex_slot = PDL_TEX_OFF;
        e->tex_gen = 0;
    } else {
        e->tex_slot = pm_tex_cur_idx();
        e->tex_gen = pm_tex_slot_gen(e->tex_slot);
    }
    gv_epoch++;
    gv_epoch_enter();
    e->pso_id = (u16)pdl_pso_capture();
    pdl_c.comp++;
}

static void pdl_compile_begin(PdlEnt* e, const Gfx* l) {
    e->nops = pdl_nops(l);
    gbi_state_get(&pdl_comp_gbi_st);
    e->state_in = pdl_key(&pdl_comp_gbi_st);
    pdl_compile_one = 1;
    pdl_depth++;
    pdl_comp_ent = e;
    pdl_comp_list = l;
    pdl_comp_sp = -1;
    pdl_comp_nopush = 0;
}

static void pdl_comp_arm(int sp, int nopush) {
    if (!pdl_compile_one || !pdl_comp_ent) return;
    pdl_comp_sp = sp;
    pdl_comp_nopush = nopush;
}

static int pdl_comp_done(int* sp, Gfx** pc) {
    if (!pdl_compile_one || !pdl_comp_ent || !pdl_comp_list || pdl_comp_sp < 0) return 0;
    Gfx* endl = (*pc) - 1;
    const Gfx* l = pdl_comp_list;
    unsigned n = pdl_comp_ent->nops;
    if (endl < l || endl >= l + n || (endl->words.w0 >> 24) != G_ENDDL) return 0;
    if (pdl_comp_nopush) {
        pdl_compile_finish();
        *pc = (Gfx*)dlc_end;
        return 1;
    }
    if (*sp == pdl_comp_sp + 1) pdl_compile_finish();
    return 0;
}

static int pdl_try(const Gfx* l) {
    if (!l || !is_host((u32)l) || pdl_depth) return 0;
    if (!dls_rd_lo) dls_init_ranges();
    unsigned c = dls_class((u32)l);
    pdl_cls[c & 7]++;
    if (c != 2 && c != 3) return 0;
    if (pdl_refuse_ops(l)) { pdl_c.refuse++; return 0; }
    PdlEnt* e = pdl_find((u32)l);
    if (!e) return 0;
    u32 hgen = pdl_heap_gen((u32)l);
    gbi_state_get(&pdl_try_gbi_st);
    u32 sin = pdl_key(&pdl_try_gbi_st);
    if (!e->nops || e->refused || e->heap_gen != hgen || e->state_in != sin) {
        e->heap_gen = hgen; e->refused = 0;
        if (pdl_refuse_ops(l)) { e->refused = 1; pdl_c.refuse++; return 0; }
        pdl_compile_begin(e, l);
        return 0;
    } else pdl_c.hit++;
    gbi_state_apply(&e->exit);
    comb_w0 = e->exit.comb_w0;
    comb_w1 = e->exit.comb_w1;
    sync_combine();
    if (pdlchk_on()) {
        gbi_state_get(&pdl_try_gbi_chk);
        if (gbi_state_hash(&pdl_try_gbi_chk) != gbi_state_hash(&e->exit)) pdl_c.exitbad++;
    }
    gv_epoch++;
    pdl_pso_cur = e->pso_id;
    pdl_ep_pso = 0;
    pdl_tex_slot = e->tex_slot;
    pdl_tex_gen = e->tex_gen;
    tb_probe(RS_TBPD, l, pdl_nops(l));
    pdl_c.replay++;
    return 1;
}

/* C4: kind M (entity, genheap, sprite heap): PSO segments for state-only runs, RUN slices for geometry (MTX and colours read from the list each replay). */
enum { PDLM_PSO = 1, PDLM_RUN = 2, PDLM_PARAM = 3 };
typedef struct { u8 tag; u16 u, v; } PdlMIns;
typedef struct {
    u32 addr, heap_gen, state_in, content_hash;
    unsigned nops, nins;
    u8 refused;
    int tex_slot;
    unsigned tex_gen;
    GbiState exit;
    PdlMIns* ins;
} PdlMEnt;

#define PDL_M_TAB 512
static PdlMEnt* pdl_m_tab[PDL_M_TAB];
static unsigned pdl_m_nent;
static struct { unsigned hit, comp, replay, refuse, full; } pdl_m_c;
static GbiState pdl_m_gbi_st;
static GbiState pdl_try_m_gbi_st;
static GbiState pdl_try_m_gbi_chk;

static int pdl_m_state_only(unsigned op) {
    if (op == G_GEOMETRYMODE || op == G_VTX || op == G_TRI1 || op == G_TRI2 || op == G_MTX || op == G_POPMTX) return 0;
    if (op == G_DL || op == G_ENDDL) return 0;
    if (op == G_SETPRIMCOLOR || op == G_SETENVCOLOR || op == G_SETFOGCOLOR || op == G_SETBLENDCOLOR) return 0;
    if (op == G_NOOP || op == G_SPNOOP || op == G_RDPFULLSYNC || op == G_RDPTILESYNC || op == G_RDPPIPESYNC || op == G_RDPLOADSYNC) return 1;
    unsigned rd, wr;
    pdl_op_rw(op, &rd, &wr);
    return rd || wr;
}

static int pdl_m_class(unsigned c) { return c == 4 || c == 5 || c == 6 || c == 7; }

static int pdl_kind_m(const Gfx* l) {
    u32 a = (u32)l;
    unsigned c = dls_class(a);
    if (!pdl_m_class(c)) return 0;
    int geo = 0, state = 0, child = 0;
    for (unsigned i = 0; i < DLC_MAXOPS; i++) {
        unsigned op = l[i].words.w0 >> 24;
        if (op == G_ENDDL) break;
        if (op == G_VTX || op == G_TRI1 || op == G_TRI2 || op == G_MTX || op == G_POPMTX) geo = 1;
        if (op == G_DL) child = 1;
        if (pdl_m_state_only(op)) state = 1;
        if (op == G_SETPRIMCOLOR || op == G_SETENVCOLOR || op == G_SETFOGCOLOR || op == G_SETBLENDCOLOR) state = 1;
        if (op == G_MODIFYVTX || op == G_TEXRECT || op == G_TEXRECTFLIP || op == G_FILLRECT || op == G_SETCIMG || op == G_SETZIMG || op == G_MOVEMEM) return 0;
        if (op == G_MOVEWORD && ((l[i].words.w0 >> 16) & 0xff) != G_MW_FOG) return 0;
    }
    if (!geo) return 0;
    if (state || child) return 1;
    /* mesh animator: genheap patch lists are vtx/tri only (state is on the main list) */
    return c == 5;
}

static int pdl_m_refuse(const Gfx* l) {
    if (!pdl_kind_m(l)) return 1;
    for (unsigned i = 0; i < DLC_MAXOPS; i++) {
        unsigned op = l[i].words.w0 >> 24;
        if (op == G_ENDDL) return 0;
        if (op == G_MODIFYVTX || op == G_TEXRECT || op == G_TEXRECTFLIP || op == G_FILLRECT || op == G_SETCIMG || op == G_SETZIMG || op == G_MOVEMEM) return 1;
        if (op == G_MOVEWORD && ((l[i].words.w0 >> 16) & 0xff) != G_MW_FOG) return 1;
    }
    return 1;
}

static PdlMEnt* pdl_m_find(u32 a) {
    unsigned k = ((a >> 3) * 2654435761u) >> 21 & (PDL_M_TAB - 1);
    for (unsigned n = 0; n < PDL_M_TAB; n++, k = (k + 1) & (PDL_M_TAB - 1)) {
        PdlMEnt* e = pdl_m_tab[k];
        if (!e) {
            if (pdl_m_nent >= PDL_M_TAB * 3 / 4) { pdl_m_c.full++; return NULL; }
            e = (PdlMEnt*)calloc(1, sizeof *e);
            if (!e) return NULL;
            e->addr = a; pdl_m_tab[k] = e; pdl_m_nent++;
            return e;
        }
        if (e->addr == a) return e;
    }
    return NULL;
}

static void pdl_m_free_ins(PdlMEnt* e) {
    free(e->ins);
    e->ins = NULL;
    e->nins = 0;
}

static int pdl_m_ins_add(PdlMEnt* e, u8 tag, u16 u, u16 v) {
    PdlMIns* q = (PdlMIns*)realloc(e->ins, (e->nins + 1u) * sizeof(PdlMIns));
    if (!q) return 0;
    e->ins = q;
    e->ins[e->nins].tag = tag;
    e->ins[e->nins].u = u;
    e->ins[e->nins].v = v;
    e->nins++;
    return 1;
}

static void pdl_m_compile(PdlMEnt* e, const Gfx* l) {
    pdl_m_free_ins(e);
    e->nops = pdl_nops(l);
    e->content_hash = dls_hash(l);
    gbi_state_get(&pdl_m_gbi_st);
    e->state_in = gbi_state_in_hash(&pdl_m_gbi_st, 0);
    pdl_compile_one = 1;
    unsigned i = 0;
    while (i < e->nops) {
        unsigned op = l[i].words.w0 >> 24;
        if (pdl_m_state_only(op)) {
            unsigned j = i;
            while (j < e->nops && pdl_m_state_only(l[j].words.w0 >> 24)) j++;
            pm_gbi_run((void*)(l + i), (j - i) * 8u);
            if (!pdl_m_ins_add(e, PDLM_PSO, (u16)i, (u16)(j - 1u))) break;
            i = j;
        } else {
            unsigned j = i;
            while (j < e->nops && !pdl_m_state_only(l[j].words.w0 >> 24)) j++;
            if (j > i) {
                if (!pdl_m_ins_add(e, PDLM_RUN, (u16)i, (u16)(j - 1u))) break;
                pm_gbi_run((void*)(l + i), (j - i) * 8u);
            }
            i = j;
        }
    }
    pdl_compile_one = 0;
    gbi_state_get(&e->exit);
    if (pm_tex_off_idx() >= 0) {
        e->tex_slot = PDL_TEX_OFF;
        e->tex_gen = 0;
    } else {
        e->tex_slot = pm_tex_cur_idx();
        e->tex_gen = pm_tex_slot_gen(e->tex_slot);
    }
    gv_epoch++;
    gv_epoch_enter();
    pdl_m_c.comp++;
}

static int pdl_try_m(const Gfx* l) {
    if (!l || !is_host((u32)l) || pdl_depth || pdl_compile_one) return 0;
    if (!dls_rd_lo) dls_init_ranges();
    unsigned c = dls_class((u32)l);
    if (!pdl_m_class(c)) return 0;
    if (pdl_m_refuse(l)) { pdl_m_c.refuse++; return 0; }
    PdlMEnt* e = pdl_m_find((u32)l);
    if (!e) return 0;
    u32 hgen = pdl_heap_gen((u32)l);
    u32 hash = dls_hash(l);
    gbi_state_get(&pdl_try_m_gbi_st);
    u32 sin = gbi_state_in_hash(&pdl_try_m_gbi_st, 0);
    if (!e->nins || e->refused || e->heap_gen != hgen || e->state_in != sin || e->content_hash != hash) {
        e->heap_gen = hgen;
        e->refused = 0;
        e->content_hash = hash;
        if (pdl_m_refuse(l)) { e->refused = 1; pdl_m_c.refuse++; return 0; }
        pdl_m_compile(e, l);
        if (!e->nins) { e->refused = 1; return 0; }
    } else pdl_m_c.hit++;
    pdl_tex_slot = e->tex_slot;
    pdl_tex_gen = e->tex_gen;
    for (unsigned k = 0; k < e->nins; k++) {
        if (e->ins[k].tag == PDLM_PSO) {
            unsigned u = e->ins[k].u, v = e->ins[k].v;
            if (v >= e->nops || u > v) continue;
            pm_gbi_run((void*)(l + u), (v - u + 1u) * 8u);
            pdl_pso_cur = 0;
            pdl_ep_pso = 0;
        } else if (e->ins[k].tag == PDLM_RUN) {
            unsigned u = e->ins[k].u, v = e->ins[k].v;
            if (v >= e->nops || u > v) continue;
            pm_gbi_run((void*)(l + u), (v - u + 1u) * 8u);
        }
    }
    gbi_state_apply(&e->exit);
    if (pdlchk_on()) {
        gbi_state_get(&pdl_try_m_gbi_chk);
        if (gbi_state_hash(&pdl_try_m_gbi_chk) != gbi_state_hash(&e->exit)) pdl_c.exitbad++;
    }
    gv_epoch++;
    tb_probe(RS_TBPM, l, e->nops);
    pdl_m_c.replay++;
    pdl_c.replay++;
    return 1;
}

/* C5: main-list templates between G_DL calls (class 0). Skeleton hash with PARAM words masked; replay applies PARAM + PSO/RUN. */
#define PDL_TPL_N 256
typedef struct {
    u32 skel;
    unsigned nops, nins;
    PdlMIns* ins;
} PdlTplEnt;

static PdlTplEnt* pdl_tpl_tab[PDL_TPL_N];
static unsigned pdl_tpl_nent;
static struct { unsigned hit, comp, replay, fallback, seg, full; } pdl_tpl_c;
static int pdl_tpl_active;

static int pdl_tpl_param_op(unsigned op, u32 w0) {
    switch (op) {
    case G_MTX:
    case G_SETPRIMCOLOR:
    case G_SETENVCOLOR:
    case G_SETFOGCOLOR:
    case G_SETBLENDCOLOR:
    case G_SETTILESIZE:
        return 1;
    case G_MOVEWORD:
        return ((w0 >> 16) & 0xff) == G_MW_SEGMENT;
    default:
        return 0;
    }
}

static u32 pdl_tpl_skel(const Gfx* seg, unsigned n) {
    u32 h = 2166136261u;
    for (unsigned i = 0; i < n; i++) {
        u32 w0 = seg[i].words.w0, w1 = seg[i].words.w1;
        unsigned op = w0 >> 24;
        h = (h ^ w0) * 16777619u;
        switch (op) {
        case G_MTX:
        case G_SETPRIMCOLOR:
        case G_SETENVCOLOR:
        case G_SETFOGCOLOR:
        case G_SETBLENDCOLOR:
        case G_VTX:
            h = (h ^ 0u) * 16777619u;
            break;
        case G_SETTILESIZE:
            h = (h ^ (w0 & 0xfff00000u)) * 16777619u;
            h = (h ^ (w1 & 0xff000000u)) * 16777619u;
            break;
        case G_MOVEWORD:
            if (((w0 >> 16) & 0xff) == G_MW_SEGMENT) h = (h ^ 0u) * 16777619u;
            else h = (h ^ w1) * 16777619u;
            break;
        default:
            h = (h ^ w1) * 16777619u;
            break;
        }
    }
    return h;
}

static int pdl_tpl_seg_refuse(const Gfx* seg, unsigned n) {
    for (unsigned i = 0; i < n; i++) {
        unsigned op = seg[i].words.w0 >> 24;
        if (op == G_DL || op == G_VTX || op == G_TRI1 || op == G_TRI2 || op == G_MODIFYVTX || op == G_TEXRECT || op == G_TEXRECTFLIP || op == G_FILLRECT || op == G_SETCIMG || op == G_SETZIMG || op == G_MOVEMEM)
            return 1;
        if (op == G_MOVEWORD && ((seg[i].words.w0 >> 16) & 0xff) != G_MW_SEGMENT && ((seg[i].words.w0 >> 16) & 0xff) != G_MW_FOG) return 1;
    }
    return 0;
}

static PdlTplEnt* pdl_tpl_find(u32 sk) {
    unsigned k = sk >> 16 ^ sk;
    k = ((k >> 3) * 2654435761u) >> 21 & (PDL_TPL_N - 1);
    for (unsigned t = 0; t < PDL_TPL_N; t++, k = (k + 1) & (PDL_TPL_N - 1)) {
        PdlTplEnt* e = pdl_tpl_tab[k];
        if (!e) {
            if (pdl_tpl_nent >= PDL_TPL_N * 3 / 4) { pdl_tpl_c.full++; return NULL; }
            e = (PdlTplEnt*)calloc(1, sizeof *e);
            if (!e) return NULL;
            e->skel = sk;
            pdl_tpl_tab[k] = e;
            pdl_tpl_nent++;
            return e;
        }
        if (e->skel == sk) return e;
    }
    return NULL;
}

static void pdl_tpl_free_ins(PdlTplEnt* e) {
    free(e->ins);
    e->ins = NULL;
    e->nins = 0;
}

static int pdl_tpl_ins_add(PdlTplEnt* e, u8 tag, u16 u, u16 v) {
    PdlMIns* q = (PdlMIns*)realloc(e->ins, (e->nins + 1u) * sizeof(PdlMIns));
    if (!q) return 0;
    e->ins = q;
    e->ins[e->nins].tag = tag;
    e->ins[e->nins].u = u;
    e->ins[e->nins].v = v;
    e->nins++;
    return 1;
}

static int pdl_tpl_is_param_idx(const Gfx* seg, unsigned n, unsigned i) {
    return i < n && pdl_tpl_param_op(seg[i].words.w0 >> 24, seg[i].words.w0);
}

static void pdl_tpl_compile(PdlTplEnt* e, const Gfx* seg, unsigned n) {
    pdl_tpl_free_ins(e);
    e->nops = n;
    e->skel = pdl_tpl_skel(seg, n);
    pdl_compile_one = 1;
    unsigned i = 0;
    while (i < n) {
        while (i < n && pdl_tpl_is_param_idx(seg, n, i)) {
            if (!pdl_tpl_ins_add(e, PDLM_PARAM, (u16)i, (u16)(seg[i].words.w0 >> 24))) goto tpl_done;
            pm_gbi_run((void*)(seg + i), 8u); /* C5: PSO after this block must see the same state as replay */
            i++;
        }
        if (i >= n) break;
        unsigned op = seg[i].words.w0 >> 24;
        if (pdl_m_state_only(op)) {
            unsigned j = i;
            while (j < n && !pdl_tpl_is_param_idx(seg, n, j) && pdl_m_state_only(seg[j].words.w0 >> 24)) j++;
            pm_gbi_run((void*)(seg + i), (j - i) * 8u);
            if (!pdl_tpl_ins_add(e, PDLM_PSO, (u16)pdl_pso_capture(), 0)) break;
            i = j;
        } else {
            unsigned j = i;
            while (j < n && !pdl_tpl_is_param_idx(seg, n, j) && !pdl_m_state_only(seg[j].words.w0 >> 24)) j++;
            if (j > i) {
                if (!pdl_tpl_ins_add(e, PDLM_RUN, (u16)i, (u16)(j - 1u))) break;
                pm_gbi_run((void*)(seg + i), (j - i) * 8u);
            }
            i = j;
        }
    }
tpl_done:
    pdl_compile_one = 0;
    pdl_tpl_c.comp++;
}

static void pdl_tpl_replay(PdlTplEnt* e, const Gfx* seg) {
    pdl_tex_slot = -1;
    for (unsigned k = 0; k < e->nins; k++) {
        if (e->ins[k].tag == PDLM_PARAM) {
            unsigned u = e->ins[k].u;
            if (u < e->nops) pm_gbi_run((void*)(seg + u), 8u);
        } else if (e->ins[k].tag == PDLM_PSO) {
            pdl_pso_cur = e->ins[k].u;
            pdl_ep_pso = 0;
        } else if (e->ins[k].tag == PDLM_RUN) {
            unsigned u = e->ins[k].u, v = e->ins[k].v;
            if (v < e->nops && u <= v) pm_gbi_run((void*)(seg + u), (v - u + 1u) * 8u);
        }
    }
    pdl_tpl_c.replay++;
}

static void pdl_tpl_segment(const Gfx* seg, unsigned n) {
    if (!n) return;
    pdl_tpl_c.seg++;
    tb_probe(RS_TBTS, seg, n);
    if (pdl_tpl_seg_refuse(seg, n)) {
        pdl_tpl_c.fallback++;
        pm_gbi_run((void*)seg, n * 8u);
        return;
    }
    u32 sk = pdl_tpl_skel(seg, n);
    PdlTplEnt* e = pdl_tpl_find(sk);
    if (!e) {
        pm_gbi_run((void*)seg, n * 8u);
        return;
    }
    if (!e->nins || e->nops != n || e->skel != sk) pdl_tpl_compile(e, seg, n);
    else pdl_tpl_c.hit++;
    if (e->nins) pdl_tpl_replay(e, seg);
    else pm_gbi_run((void*)seg, n * 8u);
}

static int pdl_tpl_gdl(Gfx** pc, Gfx* stack[12], int* sp, const Gfx* list_end) {
    Gfx* cmd = *pc - 1;
    u32 w0 = cmd->words.w0, w1 = cmd->words.w1;
    Gfx* next = (Gfx*)ptr_of(w1);
    if (next && !is_host((u32)next)) return 1;
    if (pdl0_on()) pdl0_gl_enter(next);
    if (dlc_on && dlc_try(next)) {
        if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) *pc = (Gfx*)dlc_end;
        return 1;
    }
    if (!pdl_compile_one && dlc_on && pdl_try(next)) {
        RS_ADD(RS_PDL, 1);
        if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) *pc = (Gfx*)dlc_end;
        return 1;
    }
    if (pdl_compile_one && pdl_comp_sp < 0) pdl_comp_arm(*sp, ((w0 >> 16) & 0xff) == G_DL_NOPUSH);
    if (!pdl_compile_one && dlc_on && pdl_try_m(next)) {
        RS_ADD(RS_PDLM, 1);
        if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) *pc = (Gfx*)dlc_end;
        return 1;
    }
    if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) {
        *pc = next;
    } else if (*sp < 12) {
        stack[(*sp)++] = *pc;
        *pc = next;
    }
    return 1;
}

static int pdl_tpl_drive(void* list, unsigned nbytes) {
    if (!dlc_on || pdl_compile_one || dlc_depth || pdl_depth || pdl_tpl_active) return 0;
    if (!nbytes || nbytes < 8) return 0;
    Gfx* pc = (Gfx*)list;
    Gfx* end = pc + nbytes / 8u;
    Gfx* stack[12];
    int sp = 0;
    unsigned guard = 0;
    pdl_tpl_active = 1;
    while (pc < end && guard++ < 200000u) {
        if (sp == 0) {
            Gfx* q = pc;
            while (q < end) {
                unsigned o = q->words.w0 >> 24;
                if (o == G_DL || o == G_ENDDL) break;
                q++;
            }
            if (q > pc) {
                pdl_tpl_segment(pc, (unsigned)(q - pc));
                pc = q;
            }
            if (pc >= end) break;
            unsigned op = pc->words.w0 >> 24;
            if (op == G_ENDDL) { pdl_tpl_active = 0; return 1; }
            if (op == G_DL) {
                pc++;
                if (!pdl_tpl_gdl(&pc, stack, &sp, end)) { pdl_tpl_active = 0; return 0; }
                continue;
            }
            break;
        }
        unsigned op = pc->words.w0 >> 24;
        if (op == G_ENDDL) {
            if (pdl0_on()) pdl0_gl_leave();
            pc = stack[--sp];
            continue;
        }
        if (op == G_DL) {
            pc++;
            if (!pdl_tpl_gdl(&pc, stack, &sp, end)) { pdl_tpl_active = 0; return 0; }
            continue;
        }
        pm_gbi_run(pc, 8u);
        pc++;
    }
    pdl_tpl_active = 0;
    return sp == 0;
}

static void pdl_report(void) {
    memset(pdl_cls, 0, sizeof pdl_cls);
    memset(&pdl_c, 0, sizeof pdl_c);
    memset(&pdl_m_c, 0, sizeof pdl_m_c);
    memset(&pdl_tpl_c, 0, sizeof pdl_tpl_c);
}
