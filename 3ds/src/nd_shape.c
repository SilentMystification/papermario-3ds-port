/* nd_shape.c: the map shape path (plan_s10_path group D, LS-1): the dlc group table (dlc_tab, the static-group compile, the box
 * class of the groups, the replay and the native ranges). Cut from pm_gbi.c without logic change (D12). nd_dlc.c compiles the lists. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "pm_gbi.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
/* DLC1: the static list compiler and the group runner (S3F-4: the former dlc include, inlined here, same unit) */
/* DLC1: compile and replay of the map shape display lists (docs/plans/DLC.md, kind G). Included by pm_gbi.c after draw_tri, so it reads the statics of the interpreter.
 * `dlc` is always on (dlc_enabled turns it off under the dd, xt and rmlog tools), `dlccheck` (hash, exit state and the reject proof of every skipped group, counters in the `dlc` line).
 * A shape list is cut into groups: a group starts at a G_VTX at slot 0 after triangles, so a group reads only the vertices that it loads itself. The compiler keeps the box of the
 * object coordinates of each group. At replay a group whose box lies outside one clip plane is skipped: the interpreter rejects every triangle of it as well. A group in view runs
 * the same code as the interpreter (G_VTX copy to the GV1 ring, draw_tri) without the CPU transform of the vertices. A group that is near the camera plane, lit, or does not
 * fit the ring runs through the interpreter itself (pm_gbi_run on the op range of the group). */
unsigned pm_dlc_gen = 1; /* bumped when the shape buffer is written (pm_tex_invalidate, decode_yay0) */

DlcEnt* dlc_tab[DLC_TAB];
unsigned dlc_nent;
int dlc_on; int dlc_depth;
DlcCnt dlc_c;
unsigned sp0_grp, sp0_skip; /* SPLIT0C: groups and skips per phase window (sp0c line), counted under phase only */
unsigned dlc_dirty; /* the last replay left vert_live cleared although the interpreter would have left slots live */
const Gfx dlc_end[1] = { { { (u32)G_ENDDL << 24, 0 } } };

/* hash of a list (words up to G_ENDDL) and of the Vtx bytes of its G_VTX ops */
u32 dlc_hash(const Gfx* l) {
    u32 h = 2166136261u;
    for (unsigned i = 0; i < 4096; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        h = (h ^ w0) * 16777619u; h = (h ^ w1) * 16777619u;
        unsigned op = w0 >> 24;
        if (op == G_VTX) {
            unsigned n = (w0 >> 12) & 0xff;
            const unsigned char* v = (const unsigned char*)ptr_of(w1);
            if (v && is_host((u32)v) && n <= 64) for (unsigned k = 0; k < n * 16; k++) h = (h ^ v[k]) * 16777619u;
        }
        if (op == G_ENDDL) break;
    }
    return h;
}
int dlc_enabled(void) {
    return !dd_on() && !xt_on() && !rmlog_on();
}
int dlcchk_on(void) { return PMDL("dlccheck"); }
int behroute_on(void) { return PMDL("behroute"); } /* FLK1 debug switch behroute: a visible group with a box corner at w < 0.05 runs through the interpreter, not the static GPU segment */
__attribute__((noinline)) int dlc_behind(const DlcGrp* g, const float (*m)[4]) {
    for (int c = 0; c < 8; c++) {
        float x = (c & 1) ? g->hi[0] : g->lo[0], y = (c & 2) ? g->hi[1] : g->lo[1], z = (c & 4) ? g->hi[2] : g->lo[2];
        if (m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3] < 0.05f) return 1;
    }
    return 0;
}
int stdump_on(void) { return PMDL("stdump"); } /* FLK1 state dump per visible static segment, frames 200 to 205 */
int pm_gpu_rgb_scale_get(void);
int nobox_on(void) { return PMDL("nobox"); } /* FLK1 debug switch nobox: the group box skip of dlc_try is off */
int skipdump_on(void) { return PMDL("skipdump"); } /* FLK1 debug switch skipdump: at frame 200 each group of a list logs its box, screen box and skip result */
/* FLK1: the screen box of a group (pixels, pm_gbi vp_ scale) and the first clip plane that rejects it. */
__attribute__((cold, noinline)) void dlc_grp_dump(const DlcGrp* g, const float (*m)[4], unsigned k, unsigned gsk, unsigned gop) {
    static const struct { signed char a, b; } pl[5] = { { 0, 1 }, { 0, -1 }, { 1, 1 }, { 1, -1 }, { 2, -1 } };
    float smin[2] = { 1e9f, 1e9f }, smax[2] = { -1e9f, -1e9f };
    int behind = 0, rej = -1;
    for (int c = 0; c < 8; c++) {
        float x = (c & 1) ? g->hi[0] : g->lo[0], y = (c & 2) ? g->hi[1] : g->lo[1], z = (c & 4) ? g->hi[2] : g->lo[2];
        float p[4];
        for (int q = 0; q < 4; q++) p[q] = m[0][q] * x + m[1][q] * y + m[2][q] * z + m[3][q];
        if (p[3] <= 0) { behind = 1; continue; }
        float sx = p[0] / p[3] * vp_sx + vp_tx, sy = vp_ty - p[1] / p[3] * vp_sy;
        if (sx < smin[0]) smin[0] = sx; if (sx > smax[0]) smax[0] = sx;
        if (sy < smin[1]) smin[1] = sy; if (sy > smax[1]) smax[1] = sy;
    }
    for (int p = 0; p < 5 && rej < 0; p++) {
        int out = 1;
        for (int c = 0; c < 8 && out; c++) {
            float x = (c & 1) ? g->hi[0] : g->lo[0], y = (c & 2) ? g->hi[1] : g->lo[1], z = (c & 4) ? g->hi[2] : g->lo[2];
            float v = m[0][pl[p].a] * x + m[1][pl[p].a] * y + m[2][pl[p].a] * z + m[3][pl[p].a];
            float wv = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];
            float d = v + (float)pl[p].b * wv;
            if (pl[p].b > 0 ? !(d < 0) : !(d > 0)) out = 0;
        }
        if (out) rej = p;
    }
    static unsigned sd_n; if (++sd_n > 400) return;
    pm_log("skipg f%d k %u op0 %u ntri %u nvtx %u skip %u plane %d behind %d scr x %.0f %.0f y %.0f %.0f box %d %d %d %d %d %d", (int)pm_frame_count(), k, gop, g->ntri, g->nvtx, gsk, rej, behind, smin[0], smax[0], smin[1], smax[1], g->lo[0], g->lo[1], g->lo[2], g->hi[0], g->hi[1], g->hi[2]);
}
int noboxint_on(void) { return PMDL("noboxint"); } /* FLK1 debug switch noboxint: an out group runs through the interpreter instead of the skip */
void dlc_free(DlcEnt* e) {
    for (unsigned k = 0; k < e->ngrp; k++) { free(e->grp[k].ends); free(e->grp[k].bat_q); }
    free(e->grp); e->grp = NULL; e->gop = NULL; e->ngrp = 0;
    nd_dlc_free(&e->o); /* the vertex and index copies live in the static pools until the next pm_gpu_gstat_reset */
}


__attribute__((noinline)) u32 dlc_geom_at(const DlcEnt* e, const DlcGrp* g, u32 entry, unsigned sg) {
    u32 x = (entry & g->am) | g->om;
    for (unsigned i = 0; i < sg; i++) {
        const u32* op = e->gop[g->goff + i];
        x = (x & op[0]) | op[1];
    }
    return x;
}

/* C3: adjacent gop segments share the same draw geom for every list entry (PSO inputs unchanged). */
__attribute__((noinline)) int dlc_gop_same_geom(const DlcEnt* e, const DlcGrp* g, unsigned sg) {
    u32 a0 = dlc_geom_at(e, g, 0, sg);
    u32 a1 = dlc_geom_at(e, g, 0xffffffffu, sg);
    u32 b0 = dlc_geom_at(e, g, 0, sg + 1u);
    u32 b1 = dlc_geom_at(e, g, 0xffffffffu, sg + 1u);
    return a0 == a1 && b0 == b1 && a0 == b0;
}

/* C3: record draw ranges at compile; merge segments that share PSO geom (ends + gop index before each range). */
__attribute__((cold, noinline)) void dlc_batch_compile(DlcGrp* g, const DlcEnt* e) {
    unsigned sn = g->sn, ng = g->ng;
    g->nbat = 0;
    free(g->bat_q);
    g->bat_q = NULL;
    free(g->ends);
    g->ends = (u16*)malloc((ng + 1u) * sizeof(u16)); /* the triangle index before each geometry op (nd_dlc gat), then the group triangle count */
    if (!g->ends) return;
    unsigned short* ends = g->ends;
    for (unsigned i = 0; i < ng; i++) ends[i] = e->o.gat[g->goff + i];
    ends[ng] = (u16)sn;
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

void dlc_set_geom(u32 g);
unsigned dlc_gmi(void);
/* S3 M2: the static list compile is nd_dlc (moved from dlc_compile and dlc_static_build): groups, boxes, geometry ops, the Vtx copies in the static vertex area (flag 0)
 * and the triangle indices in the static index area. The DlcEnt keeps the NdObj and the per group view the replay loop reads. A group without pool room keeps sd NULL and
 * runs through the ring (dlc_run_group). */
unsigned short* pm_gpu_gidxs_alloc(unsigned n);
void pm_gpu_gidxs_commit(const unsigned short* p, unsigned n);
const void* nd_env_vtx(uint32_t w1, unsigned n) {
    const Vtx* src = (const Vtx*)ptr_of(w1);
    if (!src || (u32)src - (u32)gMapShapeData >= 0x40000u || n == 0 || n > 64) return NULL;
    return src;
}
uint8_t* nd_env_valloc(unsigned n, unsigned* first) {
    unsigned char* dst = NULL;
    int f = pm_gpu_gstat_alloc(n, &dst);
    if (f < 0) return NULL;
    *first = (unsigned)f;
    return dst;
}
uint16_t* nd_env_ialloc(unsigned n) { return pm_gpu_gidxs_alloc(n); }
u32 dlc_words[2 * (ND_DLC_MAXOPS + 1)];
__attribute__((cold, noinline)) int dlc_build(DlcEnt* e, const Gfx* l) {
    unsigned i;
    for (i = 0; i < ND_DLC_MAXOPS; i++) { dlc_words[2 * i] = l[i].words.w0; dlc_words[2 * i + 1] = l[i].words.w1; if ((l[i].words.w0 >> 24) == G_ENDDL) break; }
    NdDlcEnv env = { nd_env_vtx, nd_env_valloc, nd_env_ialloc };
    int rc = nd_dlc_compile(&e->o, dlc_words, &env), pooled = 1;
    if (rc == NDR_FULL) { /* no room in the static areas: compile without the copies, every group runs through the ring */
        NdDlcEnv env2 = { nd_env_vtx, NULL, NULL };
        dlc_c.sfull++;
        rc = nd_dlc_compile(&e->o, dlc_words, &env2);
        pooled = 0;
        if (rc == NDR_OK) { free(e->o.vtx); free(e->o.idx); e->o.vtx = NULL; e->o.idx = NULL; }
    }
    if (rc) return rc;
    e->nops = e->o.nops; e->exit_am = e->o.exit_am; e->exit_om = e->o.exit_om; e->ngrp = e->o.ngrp; e->gop = e->o.gop;
    e->grp = e->ngrp ? (DlcGrp*)calloc(e->ngrp, sizeof(DlcGrp)) : NULL;
    if (e->ngrp && !e->grp) { nd_dlc_free(&e->o); e->ngrp = 0; e->gop = NULL; return NDR_LONG; }
    for (unsigned k = 0; k < e->ngrp; k++) {
        DlcGrp* g = &e->grp[k]; const NdGrp* q = &e->o.grp[k];
        g->op0 = q->op0; g->op1 = q->op1; g->am = q->am; g->om = q->om; memcpy(g->lo, q->lo, 6); memcpy(g->hi, q->hi, 6);
        g->nvtx = q->nvtx; g->ntri = q->ntri; g->goff = (u16)q->gbase; g->ng = q->ng; g->tsv_exit = q->tsv_exit;
        if (pooled) { g->sn = q->ntri; g->sd = e->o.idx + q->ibase; dlc_batch_compile(g, e); }
    }
    if (pooled) {
        if (e->o.nvtx) pm_gpu_gstat_commit(e->o.vfirst, e->o.nvtx);
        if (e->o.nidx) pm_gpu_gidxs_commit(e->o.idx, e->o.nidx);
    }
    return 0;
}

unsigned dbg_dsg, dbg_seg; /* S3N-3c debug: the static refusal codes */
/* DLC2: one range of static triangles with the state of the current epoch. Returns 0 when the raw path refuses (nothing was queued). */
__attribute__((noinline)) int dlc_seg(const unsigned short* idx, unsigned nt, unsigned gmi) {
    unsigned tss = ((unsigned)nd_ws.tex_ss << 16) | ((unsigned)nd_ws.tex_ts & 0xffffu);
    if (ep_epoch != gv_epoch) gv_epoch_enter();
    dbg_seg = 0;
    if (ep_why >= 0) { dbg_seg = 1 + (unsigned)ep_why; return 0; }
    if (ep_cullboth) { n_out += (int)nt; return 1; }
    if (ep_pepoch != gv_epoch || ep_ptss != tss) {
        gv_params_calc(tss, &ep_p);
        pm_gpu_set_rgb_scale(ep_p.boost ? 2 : 1);
        ep_pepoch = gv_epoch; ep_ptss = tss;
    }
    int gr = pm_gpu_grange((const float(*)[4][4])gm_tab, gmi, ep_p.vp, ep_p.uvso, ep_p.cm, ep_p.ca, ep_p.zm, ep_ct, ep_p.fxon ? &ep_p.fx : NULL, idx, 3u * nt);
    if (!gr) { ep_open = 0; dbg_seg = 9; return 0; }
    if (gr == 2) dlc_c.batch++;
    ep_n += nt;
    ep_open = 1; ep_open_gen = pm_gpu_flush_gen(); ep_open_epoch = gv_epoch; ep_open_tss = tss;
    n_cmd += (int)nt; gp_ntri += nt; n_in += (int)nt; st_gpu_tri += nt; gv_tri_n += nt; PH_ADD(C_GVTRI, nt);
    dlc_c.stri += nt;
    return 1;
}

/* DLC2/C3: a group in view from the static area. Returns 0 when nothing was queued and the group must run through the ring. */
__attribute__((noinline)) int dlc_static_group(const DlcEnt* e, const DlcGrp* g, unsigned gmi, u32 gg) {
    u32 bad = (gv_lit_ok() ? 0 : G_LIGHTING) | (fogmix_on && !gv_fog_gpu_ok() ? G_FOG : 0), x = nd_ws.geom;
    const u32 lm = G_LIGHTING | G_TEXTURE_GEN, x0 = x & lm; /* the RSP lights at the vertex load: the lighting state must not change inside the group, else the interpreter runs it */
    dbg_dsg = 0;
    if (x & bad) { dbg_dsg = 1; return 0; }
    for (unsigned q = 0; q < g->ng; q++) { x = (x & e->gop[g->goff + q][0]) | e->gop[g->goff + q][1]; if ((x & bad) || (x & lm) != x0) { dbg_dsg = 2; return 0; } }
    if (!g->nbat || !g->bat_q || !pm_gpu_gidx_room(3u * g->sn)) { dbg_dsg = 3; return 0; }
    const unsigned short* ends = g->ends;
    unsigned t = 0, gop = 0;
    int drawn = 0;
    u32 geom_at = gg;
    for (unsigned b = 0; b < g->nbat; b++) {
        while (gop < g->bat_q[b])
            geom_at = (geom_at & e->gop[g->goff + gop][0]) | e->gop[g->goff + gop++][1];
        unsigned end = ends[b];
        if (end > t) {
            dlc_set_geom(geom_at);
            if (!dlc_seg(g->sd + 3u * t, end - t, gmi)) { if (!drawn) { dbg_dsg = 4; return 0; } dlc_c.sfail++; return 1; }
            drawn = 1;
            t = end;
        }
    }
    if (drawn && g->nbat < g->ng + 1u) dlc_c.batch += g->ng + 1u - g->nbat;
    return drawn;
}

__attribute__((noinline)) DlcEnt* dlc_find(const Gfx* l) {
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

__attribute__((noinline)) void dlc_set_geom(u32 g) {
    if (g == nd_ws.geom) return;
    nd_ws.geom = g;
    pm_gpu_set_mode((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0);
    gv_epoch++;
}

/* The matrix id of `combined` (the same search as the G_VTX code), GV_NONE when the table is full. */
__attribute__((noinline)) unsigned dlc_gmi(void) { return gv_matrix_id(0); }

/* Box test against the clip planes of clip_bits4. Returns 1 when every vertex of the box is outside one plane (by a margin for the float error of the interpreter). */
__attribute__((noinline)) int dlc_box_out(const DlcGrp* g, const float (*m)[4]) {
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


/* AND of clip_bits4 on the 8 corners of an object-space box (subset P for GP1 out loads). */
__attribute__((noinline)) unsigned char gp1_box_subset(const s16 lo[3], const s16 hi[3], const float (*m)[4]) {
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
__attribute__((noinline)) int gp1_box_in(const s16 lo[3], const s16 hi[3], const float (*m)[4]) {
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
int gp1_box_class(const s16 lo[3], const s16 hi[3], const float (*m)[4], unsigned char* pbits) {
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
__attribute__((cold, noinline)) void dlc_verify_skip(const Gfx* l, const DlcGrp* g, const float (*m)[4]) {
    unsigned char cb[64], live[64] = { 0 }, ever[64] = { 0 };
    int tsv = 1;
    for (u32 j = 0; j < g->op0; j++) { /* FLK1: the slots that an earlier group of this list loaded (the ring keeps them) */
        u32 w0 = l[j].words.w0;
        if ((w0 >> 24) == G_ENDDL) break;
        if ((w0 >> 24) == G_VTX) {
            unsigned n = (w0 >> 12) & 0xff, v0n = (w0 >> 1) & 0x7f, v0 = v0n >= n ? v0n - n : 0;
            for (unsigned k = 0; k < n && v0 + k < 64; k++) ever[v0 + k] = 1;
        }
    }
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
                if (a > 63 || b > 63 || c > 63) continue;
                if (!live[a] || !live[b] || !live[c]) { if (ever[a] || ever[b] || ever[c]) dlc_c.xgrp++; continue; }
                if (!(cb[a] & cb[b] & cb[c] & 0x2f)) dlc_c.missing++;
            }
        }
    }
}

/* One group in view: the interpreter code for G_VTX (GV1 ring copy, no CPU transform) and draw_tri. Returns 0 on success. */
__attribute__((noinline)) void dlc_run_group(const Gfx* l, const DlcGrp* g, unsigned gmi) {
    unsigned tss = ((unsigned)nd_ws.tex_ss << 16) | ((unsigned)nd_ws.tex_ts & 0xffffu);
    for (u32 j = g->op0; j < g->op1; j++) {
        u32 w0 = l[j].words.w0, w1 = l[j].words.w1;
        switch (w0 >> 24) {
        case G_VTX: {
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
                vert_live[s] = 1;
                vert_scr[s] = 0;
                vert_gvf[s] = 0;
                vert_cb[s] = 0;
            }
            break;
        }
        case G_TRI1:
            tri_prep_ok = 0;
            draw_tri(w0);
            break;
        case G_TRI2:
            tri_prep_ok = 0;
            draw_tri(w0);
            draw_tri(w1);
            break;
        case G_GEOMETRYMODE:
            set_geom(w0, w1);
            gv_epoch++;
            break;
        default:
            break;
        }
    }
}

/* the shape buffer was written: every entry is stale */
__attribute__((cold, noinline)) void dlc_reset(void) {
    for (unsigned k = 0; k < DLC_TAB; k++) if (dlc_tab[k]) { dlc_free(dlc_tab[k]); free(dlc_tab[k]); dlc_tab[k] = NULL; }
    dlc_nent = 0;
    pm_gpu_gstat_reset();
}
unsigned dlc_seen_gen;
/* A G_DL to l. Returns 1 when the list is done (replayed), 0 when the interpreter must run it. */

/* S3N-3c debug: the walker DLC counters (calls, groups, skips, static, ring, lit, material, static refusals) for the ndmcheck nraw log */
void pm_gbi_dlcc(unsigned* o) {
    o[0] = dlc_c.hit + dlc_c.compile; o[1] = dlc_c.grp; o[2] = dlc_c.skip; o[3] = dlc_c.srep; o[4] = dlc_c.i_ring; o[5] = dlc_c.i_mat; o[6] = dlc_c.i_lit; o[7] = dlc_c.sfail + dlc_c.replay;
    o[8] = dbg_dsg; o[9] = dbg_seg;
}
/* S3N-3c: the static ranges of a shape list for nd_model_draw. The groups that dlc_try replays statically (box test, no lit group, room in the pools), one range per batch as
 * dlc_static_group cuts them. No replay and no counters. Returns the count; -1 when dlc_try would not replay the list statically (the fallback); -2 when out is full. */
int pm_gpu_grange_would(const float m[4][4], unsigned nidx, const unsigned short* idx);
int pm_gpu_grange_why(const float m[4][4], unsigned nidx, const unsigned short* idx); /* S4-1 (diagnostic) */
int pm_gpu_grange_native_would(const float m[4][4], unsigned nidx, const unsigned short* idx); /* S5-1 (b); WIDE1-1: idx = the indices of the range, tested for edge vertices in wide mode */
/* S4-1 (diagnostic): the refusal of pm_dlc_native_ranges by class (1 dlc off or nobox, 2 list range, 3 table, 4 refused build, 5 depth pass, 6 group without static data, 7 lit group, 8 room, 9 walker grange refuses, 10 matrix id) */
unsigned dnr_c[16];
int dnr(unsigned k) { dnr_c[k]++; return -1; }
void pm_gbi_dnr_take(unsigned* o, unsigned n) { for (unsigned i = 0; i < n && i < 16u; i++) { o[i] = dnr_c[i]; dnr_c[i] = 0; } }
int pm_dlc_native_ranges(const void* lv, uint32_t entry, NdRng* out, unsigned max) {
    const Gfx* l = (const Gfx*)lv;
    if (!dlc_on || !dlc_enabled() || nobox_on() || noboxint_on()) return dnr(1);
    if (!is_host((u32)(uintptr_t)l)) return 0; /* rec_dl: a list outside host memory is not run (gbi_cold_bad_next): no draw */
    if ((u32)l - (u32)gMapShapeData >= 0x40000u || ((u32)l & 7)) return dnr(2);
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    DlcEnt* e = dlc_find(l);
    if (!e) return dnr(3);
    if (e->gen != pm_dlc_gen) {
        dlc_free(e);
        e->gen = pm_dlc_gen; e->refused = 0;
        int r = dlc_build(e, l);
        e->hash = dlc_hash(l);
        if (r) e->refused = (u8)r;
    }
    if (e->refused) return dnr(4);
    const float(*m)[4] = combined;
    if (img_role(0) == ROLE_DEPTH) return dnr(5); /* gv_state_why: the z mark pass (fallback) */
    const u32 lm = G_TEXTURE_GEN;
    const u32 bad = fogmix_on && !gv_fog_gpu_ok() ? G_FOG : 0;
    unsigned n = 0, gmi = GV_NONE;
    int gmi_ready = 0;
    for (unsigned k = 0; k < e->ngrp; k++) {
        const DlcGrp* g = &e->grp[k];
        if (dlc_box_out(g, m)) continue;
        const u32 gg = (entry & g->am) | g->om;
        if (!g->sd || !g->nbat || !g->bat_q) return dnr(6);
        if (gg & (G_LIGHTING | bad)) return dnr(7); /* lit groups run through the GPU lights path (S3N later row) */
        u32 x = gg;
        for (unsigned q = 0; q < g->ng; q++) {
            const u32* op = e->gop[g->goff + q];
            x = (x & op[0]) | op[1];
            if ((x & (G_LIGHTING | bad)) || (x & lm) != (gg & lm)) return dnr(7);
        }
        if (!pm_gpu_gidx_room(3u * g->sn) || !pm_gpu_gv_room(g->nvtx + 3u * g->ntri)) return dnr(8);
        if (!pm_gpu_grange_native_would(m, 3u * g->sn, g->sd)) { int w = pm_gpu_grange_why(m, 3u * g->sn, g->sd); return dnr(w ? 10 + w : 9); } /* S4-1: 11 gv or gpu off, 12 zmark, 13 decal and blend, 14 wide, 15 ring */ /* the walker's grange refuses: the group runs on the ring path (not a static draw) */
        if (!gmi_ready) { gmi = dlc_gmi(); gmi_ready = 1; }
        if (gmi == GV_NONE) return dnr(10);
        unsigned t = 0, go = 0;
        u32 geom_at = gg;
        for (unsigned b = 0; b < g->nbat; b++) {
            while (go < g->bat_q[b]) { const u32* op = e->gop[g->goff + go]; geom_at = (geom_at & op[0]) | op[1]; go++; }
            unsigned end = g->ends[b];
            if (end > t) {
                /* the walker's dlc_seg: G_CULL_BOTH drops the triangles (no draw), cull_off draws with no cull */
                const int off = cull_off();
                if (!off && (geom_at & G_CULL_BOTH) == G_CULL_BOTH) { t = end; continue; }
                if (n >= max) return -2;
                out[n].sd = g->sd + 3u * t; out[n].nidx = 3u * (end - t); out[n].geom = off ? (geom_at & ~G_CULL_BOTH) : geom_at; n++;
                t = end;
            }
        }
    }
    return (int)n;
}

int dlc_try(const Gfx* l) {
    if ((u32)l - (u32)gMapShapeData >= 0x40000u || ((u32)l & 7)) return 0;
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    DlcEnt* e = dlc_find(l);
    if (!e) return 0;
    int chk = dlcchk_on();
    if (e->gen != pm_dlc_gen) {
        dlc_free(e);
        e->gen = pm_dlc_gen; e->refused = 0;
        int r = dlc_build(e, l);
        e->hash = dlc_hash(l);
        dlc_c.compile++;
        if (r) { e->refused = (u8)r; dlc_c.refused[r < DLR_N ? r : DLR_LONG]++; }
    } else { dlc_c.hit++; }
    if (e->refused) return 0;
    if (chk && dlc_hash(l) != e->hash) { dlc_c.hashbad++; e->gen = 0; return 0; }
    const float(*m)[4] = combined;
    u32 entry = nd_ws.geom;
    unsigned gmi = GV_NONE;
    int gmi_ready = 0, skipped_last = 0, skipped_any = 0;
    dlc_c.lists++;
    dlc_c.opsrep += e->nops;
    if (e->ngrp) { __builtin_prefetch(e->grp); __builtin_prefetch(e->gop); }
    for (unsigned k = 0; k < e->ngrp; k++) {
        const DlcGrp* g = &e->grp[k];
        if (k + 1 < e->ngrp) { __builtin_prefetch(g + 1); __builtin_prefetch(g[1].sd); }
        dlc_c.grp++;
        if (pm_ph_on) sp0_grp++;
        if (skipdump_on() && pm_frame_count() >= 200 && pm_frame_count() < 206) dlc_grp_dump(g, m, k, dlc_box_out(g, m), (unsigned)g->op0);
        if (!nobox_on() && dlc_box_out(g, m)) {
            if (chk) dlc_verify_skip(l, g, m);
            if (noboxint_on()) {
                dlc_set_geom((entry & g->am) | g->om);
                dlc_depth++;
                pm_gbi_run((void*)(l + g->op0), (g->op1 - g->op0) * 8u);
                dlc_depth--;
                continue;
            }
            tri_since_vtx = g->tsv_exit;
            skipped_last = skipped_any = 1;
            dlc_c.skip++;
            if (pm_ph_on) sp0_skip++;
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
        if (!why && behroute_on() && dlc_behind(g, m)) why = 5;
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
        if (g2 != nd_ws.geom) dlc_c.exitbad++;
        if (anyg && e->ngrp && tri_since_vtx != e->grp[e->ngrp - 1].tsv_exit) dlc_c.exitbad++;
    }
    return 1;
}

/* S6-2c-2, S6-2c-3: entity model lists from the static pools. The record G_PM_ENT with G_PM_ENT_DRAW (entity_model.c) sits right before the model G_DL. The list is compiled once per
 * key (list address, vertex segment base): nd_ent_segments cuts it at its state ops, nd_ent_strip leaves the vertex loads and triangles, nd_dlc_compile copies them into the static
 * vertex and index pools (the area of the map shape groups, reset with pm_dlc_gen). The draw runs the state ops of each segment in place on the walker (same words, same order), then
 * draws the segment triangles from the pool with dlc_seg: the walker state at the segment is the state of the draw, no second copy of it. A segment the static path refuses
 * (lit, fog without GPU fog, zmark, decal with blend, ring full) runs its old words (the vertex loads before it, then its own ops). Content (the list words and the vertex span of a
 * vertex array) is hashed once per frame and key; a change recompiles, 4 changes make the key dynamic (walker). entcheck: compile and compare, the walker draws the old words. */
EntKey* ent_tab[ENT_TAB];
unsigned ent_nent, ent_gen_seen;
NdEntSeg ent_sbuf[ENT_SEGMAX];
unsigned ent_opc[256];
EntCount ent_c;
GdlCount gdl_c; /* SGEN-1: the generic G_DL path */
unsigned ent_skipgen; /* the G_DL after a draw record that ent_draw refused: no second try */
int entchk_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("entcheck"); return v; }
const void* ent_env_vtx(uint32_t w1, unsigned n) {
    const Vtx* src = (const Vtx*)ptr_of(w1);
    if (!src || !is_host((u32)src) || n == 0 || n > 64) return NULL;
    return src;
}
/* S6-5: the words of the list and of every nested list (G_DL), then the vertex span */
u32 ent_hash_walk(const Gfx* l, u32 h, int depth) {
    for (unsigned i = 0; i < ND_DLC_MAXOPS; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        h = (h ^ w0) * 16777619u; h = (h ^ w1) * 16777619u;
        if ((w0 >> 24) == G_ENDDL) return h;
        if ((w0 >> 24) == G_DL) {
            const Gfx* t = (const Gfx*)ptr_of(w1);
            if (t && is_host((u32)t) && depth < 5) h = ent_hash_walk(t, h, depth + 1);
            if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) return h;
        }
    }
    return h;
}
u32 ent_hash(const Gfx* l, const u8* vp, u32 vlen) {
    u32 h = ent_hash_walk(l, 2166136261u, 0);
    for (u32 i = 0; i < vlen; i += 4u) { u32 v; memcpy(&v, vp + i, 4); h = (h ^ v) * 16777619u; }
    return h;
}
/* S6-5: the list with the nested lists inlined. 0: a nested target is not host memory, too deep or too long. */
Gfx ent_flat_buf[ND_DLC_MAXOPS + 1];
int ent_flat(const Gfx* l, unsigned* n, int depth) {
    for (unsigned i = 0; i < ND_DLC_MAXOPS; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        unsigned op = w0 >> 24;
        if (op == G_ENDDL) return 1;
        if (op == G_DL) {
            const Gfx* t = (const Gfx*)ptr_of(w1);
            if (!t || !is_host((u32)t) || depth >= 5 || !ent_flat(t, n, depth + 1)) return 0;
            if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) return 1;
            continue;
        }
        if (*n >= ND_DLC_MAXOPS) return 0;
        ent_flat_buf[(*n)++] = l[i];
    }
    return 0;
}
/* the pool copies against the list: every triangle corner is the vertex the slot holds at the triangle (bytes 0 to 5 and 8 to 15; the flag word is the palette index). Per segment. */
void ent_refcheck(const EntKey* k, const Gfx* l) {
    const u8* slot[64];
    u8 bad[ENT_SEGMAX];
    unsigned gt = 0, s = 0;
    memset(slot, 0, sizeof slot);
    memset(bad, 0, sizeof bad);
    for (u32 i = 0; i < k->nops; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        unsigned op = w0 >> 24;
        if (op == G_VTX) {
            unsigned n = (w0 >> 12) & 0xff, e = (w0 >> 1) & 0x7f, v0 = e >= n ? e - n : 0;
            const u8* src = (const u8*)ent_env_vtx(w1, n);
            if (v0 + n > 64) n = 64 - v0;
            for (unsigned t = 0; t < n; t++) slot[v0 + t] = src ? src + 16u * t : NULL;
        } else if (op == G_TRI1 || op == G_TRI2) {
            for (int t = 0; t < (op == G_TRI2 ? 2 : 1); t++, gt++) {
                u32 w = t ? w1 : w0;
                int ix[3] = { (int)((w >> 16) & 0xff) / 2, (int)((w >> 8) & 0xff) / 2, (int)(w & 0xff) / 2 };
                while (s + 1 < k->nseg && gt >= k->seg[s].tri0 + k->seg[s].ntri) s++;
                for (int c = 0; c < 3; c++) {
                    const u8* p = ix[c] < 64 ? slot[ix[c]] : NULL;
                    const u8* q = k->vtx + 16u * (k->idx[3u * gt + (unsigned)c] - k->vfirst);
                    if (!p || memcmp(p, q, 6) != 0 || memcmp(p + 8, q + 8, 8) != 0) bad[s] = 1;
                }
            }
        }
    }
    for (unsigned q = 0; q < k->nseg; q++) {
        if (!k->seg[q].ntri) continue;
        ent_c.cseg++;
        if (bad[q] || gt != k->seg[k->nseg - 1].tri0 + k->seg[k->nseg - 1].ntri) ent_c.cdiff++; else ent_c.ceq++;
    }
}
void ent_compile(EntKey* k, const Gfx* l0) {
    unsigned ns = 0, nops = 0;
    u32 bad = 0, ntri = 0;
    const u8 *lo = NULL, *hi = NULL;
    NdObj o;
    free(k->seg); k->seg = NULL; k->nseg = 0;
    free(k->fw); k->fw = NULL;
    k->st = ENT_REF; k->why = 0; k->fr = pm_frame_count(); k->retry = k->fr + 300u;
    ent_c.comp++;
    if (!ent_flat(l0, &nops, 0)) { k->why = 5; return; }
    ent_flat_buf[nops].words.w0 = (u32)G_ENDDL << 24; ent_flat_buf[nops].words.w1 = 0;
    if (!k->vb) { for (unsigned i = 0; i < nops; i++) if ((ent_flat_buf[i].words.w0 >> 24) == G_VTX) { u32 a = ent_flat_buf[i].words.w1; if (!is_host(a) && (a >> 24) > 0 && (a >> 24) < 16u) { k->useseg = 1; break; } } if (k->useseg) { k->why = 9; return; } }
    k->fw = (Gfx*)malloc((nops + 1u) * sizeof(Gfx));
    if (!k->fw) { k->why = 5; return; }
    memcpy(k->fw, ent_flat_buf, (nops + 1u) * sizeof(Gfx));
    const Gfx* l = k->fw;
    const u32* w = (const u32*)l;
    int rc = nd_ent_segments(w, ent_sbuf, ENT_SEGMAX, &ns, &bad);
    if (rc != NDR_OK) { k->why = (u8)(rc + 1); if (rc == NDR_OP) ent_opc[bad & 0xffu]++; return; }
    if (ns == 0) { k->why = 8; return; }
    nd_ent_strip(w, dlc_words);
    for (unsigned s = 0; s < ns; s++) ntri += ent_sbuf[s].ntri;
    if (k->vb) for (unsigned i = 0; i < nops; i++) { /* the vertex span of the vertex array: hashed once per frame */
        if ((l[i].words.w0 >> 24) != G_VTX) continue;
        const u8* p = (const u8*)ptr_of(l[i].words.w1);
        unsigned n = (l[i].words.w0 >> 12) & 0xff;
        if (!p || !is_host((u32)p)) continue;
        if (!lo || p < lo) lo = p;
        if (!hi || p + 16u * n > hi) hi = p + 16u * n;
    }
    if (lo && (u32)(hi - lo) > 0x20000u) { k->why = 5; return; }
    NdDlcEnv env = { ent_env_vtx, nd_env_valloc, nd_env_ialloc };
    rc = nd_dlc_compile(&o, dlc_words, &env);
    if (rc != NDR_OK) { k->why = (u8)(rc + 1); return; }
    if (o.nidx != 3u * ntri) { nd_dlc_free(&o); k->why = 7; return; }
    k->seg = (EntSeg*)malloc(ns * sizeof(EntSeg));
    if (!k->seg) { nd_dlc_free(&o); k->why = 5; return; }
    for (unsigned s = 0; s < ns; s++) {
        k->seg[s].a0 = s ? ent_sbuf[s - 1].op1 : 0u; k->seg[s].op0 = ent_sbuf[s].op0; k->seg[s].op1 = ent_sbuf[s].op1;
        k->seg[s].tri0 = ent_sbuf[s].tri0; k->seg[s].ntri = ent_sbuf[s].ntri;
    }
    k->nseg = (u16)ns; k->nops = nops;
    k->idx = o.idx; k->vtx = o.vtx; k->vfirst = o.vfirst; k->nvtx = o.nvtx;
    if (o.nvtx) pm_gpu_gstat_commit(o.vfirst, o.nvtx);
    if (o.nidx) pm_gpu_gidxs_commit(o.idx, o.nidx);
    nd_dlc_free(&o); /* the groups and geometry ops; the pool copies stay */
    k->vp = lo; k->vlen = lo ? (u32)(hi - lo) & ~3u : 0u;
    k->hash = ent_hash(l0, k->vp, k->vlen);
    k->st = ENT_OK;
    if (entchk_on()) ent_refcheck(k, l);
}
void ent_reset(void) {
    for (unsigned i = 0; i < ENT_TAB; i++) if (ent_tab[i]) { free(ent_tab[i]->seg); free(ent_tab[i]->fw); free(ent_tab[i]); ent_tab[i] = NULL; }
    ent_nent = 0; ent_gen_seen = pm_dlc_gen;
}
EntKey* ent_lookup(const Gfx* l, u32 vb) {
    unsigned h = (((u32)l >> 3) * 2654435761u ^ vb * 40503u) >> 20 & (ENT_TAB - 1);
    for (unsigned n = 0; n < ENT_TAB; n++, h = (h + 1) & (ENT_TAB - 1)) {
        EntKey* k = ent_tab[h];
        if (!k) {
            if (ent_nent >= ENT_TAB * 3 / 4) return NULL;
            k = (EntKey*)calloc(1, sizeof *k);
            if (!k) return NULL;
            k->list = (u32)l; k->vb = vb; k->id = ent_nent;
            ent_tab[h] = k; ent_nent++;
            ent_compile(k, l);
            return k;
        }
        if (k->list == (u32)l && k->vb == vb) return k;
    }
    return NULL;
}

__attribute__((cold, noinline)) void dlc_report(void) {
    pm_log_hw("dlc2 f%d tri %u opsrep %u | hashbad %u exitbad %u missing %u slotfix %u ringfail %u ents %u | static grp %u tri %u fail %u full %u batch %u kb %u skip %u grp %u xgrp %u", rm_frame, dlc_c.tri, dlc_c.opsrep, dlc_c.hashbad, dlc_c.exitbad, dlc_c.missing, dlc_c.slotfix, dlc_c.ringfail, dlc_nent, dlc_c.srep, dlc_c.stri, dlc_c.sfail, dlc_c.sfull, dlc_c.batch, pm_gpu_gstat_used() / 64u, dlc_c.skip, dlc_c.grp, dlc_c.xgrp);
    memset(&dlc_c, 0, sizeof dlc_c);
}

extern unsigned char pm_ent_decal_ok; /* pm_gpu.c: pm_gpu_grange takes a decal with blend (two native passes at the close) */
unsigned ent_sw[16], ent_gw[8]; /* ent_gw: pm_gpu_grange_why of a grange refusal (1 gv or gpu off, 2 zmark, 3 decal and blend, 4 wide, 5 ring) */ /* segments the static path refused, by reason: 1 lit, 2 fog, 3 index room, 4 matrix table, 5 + dlc_seg code (1 + gv state why, 9 grange) */
__attribute__((noinline)) int ent_seg_native(const EntKey* k, const EntSeg* s) {
    extern unsigned char pm_ent_decal_ok;
    if (nd_ws.geom & G_LIGHTING) { ent_sw[1]++; return 0; }
    if (fogmix_on && (nd_ws.geom & G_FOG) && !gv_fog_gpu_ok()) { ent_sw[2]++; return 0; }
    if (!pm_gpu_gidx_room(3u * s->ntri)) { ent_sw[3]++; return 0; }
    pm_gpu_set_mode((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0);
    unsigned gmi = dlc_gmi();
    if (gmi == GV_NONE) { ent_sw[4]++; return 0; }
    pm_ent_decal_ok = 1;
    int r = dlc_seg(k->idx + 3u * s->tri0, s->ntri, gmi);
    pm_ent_decal_ok = 0;
    if (r) return 1;
    ent_sw[5 + (dbg_seg < 10 ? dbg_seg : 10)]++;
    if (dbg_seg == 9) { pm_ent_decal_ok = 1; ent_gw[pm_gpu_grange_why(gm_tab[gmi], 3u * s->ntri, k->idx + 3u * s->tri0) & 7]++; pm_ent_decal_ok = 0; }
    ((EntKey*)k)->nint++;
    return 0;
}


void ent_log(void) {
    static unsigned next = 300u;
    unsigned f = pm_frame_count();
    extern void pm_log_prof_line(const char* fmt, ...);
    if (f < next) return;
    next = (f / 300u + 1u) * 300u;
    pm_log_prof_line("entnat f%u draws %u native %u fb %u segs native %u interp %u keys %u comp %u dyn %u", f, ent_c.draws, ent_c.nat, ent_c.fb, ent_c.segn, ent_c.segi, ent_nent, ent_c.comp, ent_c.dyn);
    pm_log_prof_line("entnat why 1 table %u 2 op %u 3 slot %u 4 vtx %u 5 long %u 6 pool %u 7 count %u 8 none %u", ent_c.why[1], ent_c.why[2], ent_c.why[3], ent_c.why[4], ent_c.why[5], ent_c.why[6], ent_c.why[7], ent_c.why[8]);
    for (unsigned q = 0; q < 256u; q++) if (ent_opc[q]) pm_log_prof_line("entnat refused op %02x keys %u", q, ent_opc[q]);
    for (unsigned q = 0, n = 0; q < ENT_TAB && n < 16u; q++) if (ent_tab[q] && (ent_tab[q]->nint || ent_tab[q]->st != ENT_OK)) { const EntKey* k = ent_tab[q]; n++; pm_log_prof_line("entnat key list %08x vb %08x st %u why %u nseg %u nops %u draws %u seg %u int %u", k->list, k->vb, k->st, k->why, k->nseg, k->nops, k->ndraw, k->segid, k->nint); }
    pm_log_prof_line("entnat grange why 0 %u 1 %u 2 %u 3 %u 4 %u 5 %u", ent_gw[0], ent_gw[1], ent_gw[2], ent_gw[3], ent_gw[4], ent_gw[5]);
    pm_log_prof_line("entnat segwhy lit %u fog %u room %u mtx %u | dlc_seg code (0 none, 1 + gv why, 9 grange) 0:%u 1:%u 2:%u 3:%u 4:%u 5:%u 6:%u 7:%u 8:%u 9:%u", ent_sw[1], ent_sw[2], ent_sw[3], ent_sw[4], ent_sw[5], ent_sw[6], ent_sw[7], ent_sw[8], ent_sw[9], ent_sw[10], ent_sw[11], ent_sw[12], ent_sw[13], ent_sw[14]);
    { extern void pm_gpu_nraw_why(unsigned* o); unsigned nw[10]; pm_gpu_nraw_why(nw); pm_log_prof_line("nrawwhy f%u cmdbuf %u off %u grab %u zmark %u nozt %u tex1 %u noise %u unit0 %u tmpl %u", f, nw[1], nw[2], nw[3], nw[4], nw[5], nw[6], nw[7], nw[8], nw[9]); }
    pm_log_prof_line("gdl f%u lists %u native %u dyn %u refuse %u skip %u draws native %u walker %u", f, gdl_c.lists, gdl_c.nat, gdl_c.dyn, gdl_c.ref, gdl_c.skip, ent_c.segn, ent_c.segi);
    if (entchk_on()) pm_log_prof_line("entsegcmp f%u segs %u eq %u diff %u", f, ent_c.cseg, ent_c.ceq, ent_c.cdiff);
}

EntKey* ent_find(const Gfx* l, u32 vb) {
    unsigned h = (((u32)l >> 3) * 2654435761u ^ vb * 40503u) >> 20 & (ENT_TAB - 1);
    for (unsigned n = 0; n < ENT_TAB; n++, h = (h + 1) & (ENT_TAB - 1)) {
        EntKey* k = ent_tab[h];
        if (!k) return NULL;
        if (k->list == (u32)l && k->vb == vb) return k;
    }
    return NULL;
}

u32 ent_segkey(void) { u32 h = 2166136261u; for (unsigned i = 1; i < 16; i++) h = (h ^ nd_ws.segbase[i]) * 16777619u; return h | 1u; }

/* SGEN-1: every static list the walker meets (G_DL arm, no record). Lists in the display contexts (the frame heap) are dynamic by address: walker. 1: drawn. */
int ent_gdl(const Gfx* l) {
    if (!dlc_on || ((u32)l & 7u) || (u32)l - (u32)D_80164000 < 2u * sizeof(DisplayContext)) { gdl_c.skip++; return 0; }
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    if (ent_gen_seen != pm_dlc_gen) ent_reset();
    gdl_c.lists++;
    EntKey* k = ent_find(l, 0);
    if (!k) {
        if (ent_nent >= ENT_TAB * 3 / 4) { gdl_c.ref++; return 0; }
        k = (EntKey*)calloc(1, sizeof *k);
        if (!k) return 0;
        unsigned h = (((u32)l >> 3) * 2654435761u) >> 20 & (ENT_TAB - 1);
        while (ent_tab[h]) h = (h + 1) & (ENT_TAB - 1);
        k->list = (u32)l; k->vb = 0; k->id = ent_nent; ent_tab[h] = k; ent_nent++;
        ent_compile(k, l);
    }
    if (k->useseg) { /* the vertices sit behind segment addresses: the entry of these segment bases */
        u32 vb = ent_segkey();
        k = ent_find(l, vb);
        if (!k) {
            if (ent_nent >= ENT_TAB * 3 / 4) { gdl_c.ref++; return 0; }
            k = ent_lookup(l, vb);
            if (!k) { gdl_c.ref++; return 0; }
        }
    }
    if (k->st == ENT_DYN) { gdl_c.dyn++; return 0; }
    if (k->st == ENT_REF && k->why != 6) { gdl_c.ref++; return 0; }
    int r = ent_run(l, k, 0);
    if (r) gdl_c.nat++; else if (k->st == ENT_DYN) gdl_c.dyn++; else gdl_c.ref++;
    return r;
}

int ent_draw(const Gfx* rec) {
    const Gfx* l = (const Gfx*)ptr_of(rec[0].words.w1);
    const Gfx* nx = rec + G_PM_ENT_WORDS;
    if (!l || !dlc_on || !is_host((u32)l) || ((u32)l & 7u) || (nx->words.w0 >> 24) != G_DL || nx->words.w1 != rec[0].words.w1) return 0;
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    if (ent_gen_seen != pm_dlc_gen) ent_reset();
    ent_c.draws++;
    EntKey* k = ent_lookup(l, rec[1].words.w1);
    return ent_run(l, k, rec[1].words.w0 | (rec[2].words.w0 << 8));
}

int ent_run(const Gfx* l, EntKey* k, u32 segid) {
    const unsigned f = pm_frame_count();
    if (!k) { ent_c.why[1]++; ent_c.fb++; ent_log(); return 0; }
    if (k->st == ENT_OK && k->fr != f) { /* content, once per frame */
        k->fr = f;
        if (ent_hash(l, k->vp, k->vlen) != k->hash) {
            if (++k->nrec > 3) { k->st = ENT_DYN; ent_c.dyn++; }
            else { u8 nr = k->nrec; ent_compile(k, l); k->nrec = nr; }
        }
    } else if (k->st == ENT_REF && k->why == 6 && f >= k->retry) { ent_compile(k, l); }
    k->ndraw++; k->segid = segid;
    { static unsigned msk = 0xffffffffu; if (msk == 0xffffffffu) { char b[16]; msk = pm_debug_value("entskip", b, sizeof b) ? (unsigned)strtoul(b, NULL, 16) : 0u; } if (k->id < 32u && (msk >> k->id & 1u)) return 0; } /* debug switch entskip=<hex mask of key ids>: those keys run the walker (bisect aid) */
    if (k->st != ENT_OK) { ent_c.fb++; if (k->st == ENT_REF) ent_c.why[k->why < 12 ? k->why : 0]++; ent_log(); return 0; }
    if (entchk_on()) { ent_c.nat++; ent_log(); return 0; }
    for (unsigned s = 0; s < k->nseg; s++) {
        const EntSeg* e = &k->seg[s];
        const Gfx* fw = k->fw;
        if (e->op0 > e->a0) pm_gbi_run((void*)(fw + e->a0), (e->op0 - e->a0) * 8u);
        if (!e->ntri) continue; /* a state-only or vertex-only segment: the loads replay when a later segment needs them */
        if (e->ntri && ent_seg_native(k, e)) { ent_c.segn++; continue; }
        ent_c.segi++; /* the old words: the vertex loads before the segment, then its own ops */
        for (u32 i = 0; i < e->op0; i++) if ((fw[i].words.w0 >> 24) == G_VTX) pm_gbi_run((void*)(fw + i), 8u);
        if (e->op1 > e->op0) pm_gbi_run((void*)(fw + e->op0), (e->op1 - e->op0) * 8u);
    }
    { const EntSeg* e = &k->seg[k->nseg - 1]; if (k->nops > e->op1) pm_gbi_run((void*)(k->fw + e->op1), (k->nops - e->op1) * 8u); }
    tri_since_vtx = 1; memset(vert_live, 0, sizeof vert_live); dlc_dirty = 1; /* as the skipped groups of dlc_try */
    ent_c.nat++;
    ent_log();
    return 1;
}
