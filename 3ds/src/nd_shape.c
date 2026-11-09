/* nd_shape.c: the map shape path (plan_s10_path group D, LS-1): the dlc group table (dlc_tab, the static-group compile, the box
 * class of the groups, the replay and the native ranges). Cut from the former pm_gbi.c without logic change (D12). nd_dlc.c compiles the lists. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
/* DLC1: the static list compiler and the group runner (S3F-4: the former dlc include, inlined here, same unit) */
/* DLC1: compile and replay of the map shape display lists (docs/plans/DLC.md, kind G). Included by the former pm_gbi.c after draw_tri, so it reads the statics of the interpreter.
 * `dlc` is always on,
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
int stdump_on(void) { return PMDL("stdump"); } /* FLK1 state dump per visible static segment, frames 200 to 205 */
int pm_gpu_rgb_scale_get(void);
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
/* SEAMFIX: the T-point table of the shape file (nd_seam.c), built at the first list compile after pm_dlc_gen changed, freed in dlc_reset */
static NdSeamTab seam_tab;
static unsigned seam_gen;
static int seam_ok;
extern char pm_shape_name[24];
__attribute__((cold, noinline)) static const NdSeamTab* seam_get(void) {
    if (seam_gen == pm_dlc_gen) return seam_ok ? &seam_tab : NULL;
    nd_seam_free(&seam_tab);
    seam_gen = pm_dlc_gen;
    unsigned t0 = pm_ticks();
    seam_ok = nd_seam_build(&seam_tab, gMapShapeData, 0x40000u) == 0;
    unsigned us = (pm_ticks() - t0) / 268u;
    if (!seam_ok) { pm_log_evt("seamfix map %s nomem us %u", pm_shape_name, us); return NULL; }
    pm_log_evt("seamfix map %s tp %u ins %u skip_mtx %u skip_ref %u lists %u tri %u bedge %u kb %u us %u", pm_shape_name, seam_tab.tp_all, seam_tab.ntp,
        seam_tab.skip_mtx, seam_tab.skip_ref, seam_tab.nlist, seam_tab.ntri, seam_tab.nbedge, (seam_tab.ntp * (unsigned)sizeof(NdTp) + seam_tab.nlist * (unsigned)sizeof(NdSeamList) + 1023u) >> 10, us);
    return &seam_tab;
}
__attribute__((cold, noinline)) static void seam_reset(void) { nd_seam_free(&seam_tab); seam_gen = 0; seam_ok = 0; }
__attribute__((cold, noinline)) int dlc_build(DlcEnt* e, const Gfx* l) {
    unsigned i;
    const NdSeamTab* st = seam_get();
    const NdSeamList* sl = st ? nd_seam_find(st, (u32)(uintptr_t)l) : NULL;
    for (i = 0; i < ND_DLC_MAXOPS; i++) { dlc_words[2 * i] = l[i].words.w0; dlc_words[2 * i + 1] = l[i].words.w1; if ((l[i].words.w0 >> 24) == G_ENDDL) break; }
    NdDlcEnv env = { nd_env_vtx, nd_env_valloc, nd_env_ialloc, sl && sl->cnt ? st->tp + sl->first : NULL, sl ? sl->cnt : 0u }; /* SEAMFIX: the T points of this list */
    int rc = nd_dlc_compile(&e->o, dlc_words, &env), pooled = 1;
    if (rc == NDR_FULL) { /* no room in the static areas: compile without the copies, every group runs through the ring (the source words: no T points) */
        NdDlcEnv env2 = { nd_env_vtx, NULL, NULL, NULL, 0 };
        dlc_c.sfull++;
        if (sl && sl->cnt) pm_log_evt("seamfix pool map %s list %08x skip %u", pm_shape_name, (u32)(uintptr_t)l, (unsigned)sl->cnt);
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
static unsigned seg_osc_ep; /* DARK: the epoch of the last colour image check of a static range */
__attribute__((noinline)) int dlc_seg(const unsigned short* idx, unsigned nt, unsigned gmi) {
    unsigned tss = ((unsigned)nd_ws.tex_ss << 16) | ((unsigned)nd_ws.tex_ts & 0xffffu);
    /* DARK: a static range draws into the colour image of its epoch, as draw_tri does. The darkness stencil quad (Gfx_DarknessStencilQuad,
     * a static list on the SGEN-1 entity path) draws into the 64 x 64 image in the z-buffer memory; without this check it drew on the screen. */
    if (seg_osc_ep != gv_epoch) { off_screen_check(); seg_osc_ep = gv_epoch; }
    if (ep_epoch != gv_epoch) gv_epoch_enter();
    dbg_seg = 0;
    if (ep_why >= 0) { dbg_seg = 1 + (unsigned)ep_why; return 0; }
    if (ep_cullboth) { n_out += (int)nt; return 1; }
    if (ep_t1f) { dbg_seg = 1 + GVR_TEX1; return 0; } /* T1F: a range cannot carry the per triangle window fold */
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
    seam_reset();
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
/* SBKW-M (diagnostic, switch dlchash): per frame 200 to 211, the lists the native ranges read, and how many of them have live Vtx bytes
 * that differ from the bytes compiled into their static copy (e->hash = dlc_hash at build). Logs the first two stale lists per frame with their first G_VTX source. */
extern void pm_log_prof_line(const char* fmt, ...);
static void sbkw_dlchash(const Gfx* l, const DlcEnt* e) {
    unsigned f = pm_frame_count();
    static unsigned lf_, nl_, ns_;
    if (f < 200u) return;
    if (f != lf_) { if (lf_ && lf_ <= 211u) pm_log_prof_line("dlch f%u lists %u stale %u", lf_, nl_, ns_); lf_ = f; nl_ = 0; ns_ = 0; }
    if (f > 211u) return;
    nl_++;
    u32 live = dlc_hash(l);
    if (live == e->hash) return;
    ns_++;
    if (ns_ > 2) return;
    u32 src = 0; u32 nv = 0;
    for (unsigned i = 0; i < 4096; i++) {
        u32 w0 = l[i].words.w0;
        if ((w0 >> 24) == G_VTX) { if (!src) src = l[i].words.w1; nv++; }
        if ((w0 >> 24) == G_ENDDL) break;
    }
    pm_log_prof_line("dlch f%u list %08x live %08x pool %08x vtx0 %08x nvtx %u ngrp %u", f, (u32)(uintptr_t)l, live, e->hash, src, nv, (unsigned)e->ngrp);
}

/* S4-1 (diagnostic): the refusal of pm_dlc_native_ranges by class (1 dlc off, 2 list range, 3 table, 4 refused build, 5 depth pass, 6 group without static data, 7 lit group, 8 room, 9 walker grange refuses, 10 matrix id) */
unsigned dnr_c[16];
int dnr(unsigned k) { dnr_c[k]++; return -1; }
void pm_gbi_dnr_take(unsigned* o, unsigned n) { for (unsigned i = 0; i < n && i < 16u; i++) { o[i] = dnr_c[i]; dnr_c[i] = 0; } }
uint64_t pm_dlc_vm; int pm_dlc_vm_ok;
int pm_dlc_vis_mask(const void* lv, uint64_t* out) {
    const Gfx* l = (const Gfx*)lv;
    if (!dlc_on) return -1;
    if (!is_host((u32)(uintptr_t)l)) { *out = 0; return 0; } /* as pm_dlc_native_ranges: no draw */
    if ((u32)l - (u32)gMapShapeData >= 0x40000u || ((u32)l & 7) || dlc_seen_gen != pm_dlc_gen) return -1;
    DlcEnt* e = dlc_find(l);
    if (!e || e->gen != pm_dlc_gen || e->refused || e->ngrp > 64u || img_role(0) == ROLE_DEPTH) return -1;
    const float(*m)[4] = combined;
    uint64_t vm = 0;
    for (unsigned k = 0; k < e->ngrp; k++) if (!dlc_box_out(&e->grp[k], m)) vm |= 1ull << k;
    *out = vm;
    return 0;
}
/* MAPX-2: the entry geometry bits the groups of a built static list keep (or of the group am masks: every other bit is set by the list before a
 * triangle). -1 when the list has no current entry (the model key then keeps the whole geometry word). */
int pm_dlc_geom_keep(const void* lv, uint32_t* keep) {
    const Gfx* l = (const Gfx*)lv;
    if (!dlc_on || !is_host((u32)(uintptr_t)l) || (u32)l - (u32)gMapShapeData >= 0x40000u || ((u32)l & 7) || dlc_seen_gen != pm_dlc_gen) return -1;
    DlcEnt* e = dlc_find(l);
    if (!e || e->gen != pm_dlc_gen || e->refused) return -1;
    uint32_t k = 0;
    for (unsigned g = 0; g < e->ngrp; g++) k |= e->grp[g].am;
    *keep = k;
    return 0;
}
int pm_dlc_native_ranges(const void* lv, uint32_t entry, NdRng* out, unsigned max) {
    const Gfx* l = (const Gfx*)lv;
    pm_dlc_vm_ok = 0;
    if (PMDL("dlchash")) { /* SBKW-M: calls per frame 200 to 211 and the list address of each (first four) */
        unsigned f = pm_frame_count(); static unsigned lf2, c2, n2;
        if (f >= 200u && f <= 211u) {
            if (f != lf2) { if (lf2) pm_log_prof_line("dlcn f%u calls %u", lf2, c2); lf2 = f; c2 = 0; n2 = 0; }
            c2++;
            if (n2 < 4u) { n2++; pm_log_prof_line("dlcn f%u l %08x host %d inmap %d gen %u %u", f, (u32)(uintptr_t)l, is_host((u32)(uintptr_t)l) ? 1 : 0, (u32)(uintptr_t)l - (u32)gMapShapeData < 0x40000u ? 1 : 0, pm_dlc_gen, dlc_on); }
        }
    }
    if (!dlc_on) return dnr(1);
    if (!is_host((u32)(uintptr_t)l)) { pm_dlc_vm = 0; pm_dlc_vm_ok = 1; return 0; } /* rec_dl: a list outside host memory is not run (gbi_cold_bad_next): no draw */
    if ((u32)l - (u32)gMapShapeData >= 0x40000u || ((u32)l & 7)) return dnr(2);
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    DlcEnt* e = dlc_find(l);
    if (!e) return dnr(3);
    if (PMDL("dlchash") && e->gen == pm_dlc_gen) sbkw_dlchash(l, e); /* SBKW-M: live Vtx bytes against the bytes of the static copy (frames 200 to 211) */
    if (e->gen != pm_dlc_gen) {
        dlc_free(e);
        e->gen = pm_dlc_gen; e->refused = 0;
        unsigned xt0 = pm_xf.on ? pm_ticks() : 0u;
        int r = dlc_build(e, l);
        if (pm_xf.on) { pm_xf.dlc_n++; pm_xf.dlc_t += pm_ticks() - xt0; } /* MAPX-1 */
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
    uint64_t vm = 0;
    for (unsigned k = 0; k < e->ngrp; k++) {
        const DlcGrp* g = &e->grp[k];
        if (dlc_box_out(g, m)) continue;
        if (k < 64u) vm |= 1ull << k;
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
                /* the walker's dlc_seg: G_CULL_BOTH drops the triangles (no draw) */
                if ((geom_at & G_CULL_BOTH) == G_CULL_BOTH) { t = end; continue; }
                if (n >= max) return -2;
                out[n].sd = g->sd + 3u * t; out[n].nidx = 3u * (end - t); out[n].geom = geom_at; n++;
                t = end;
            }
        }
    }
    pm_dlc_vm = vm; pm_dlc_vm_ok = e->ngrp <= 64u;
    return (int)n;
}

int dlc_try(const Gfx* l) {
    if ((u32)l - (u32)gMapShapeData >= 0x40000u || ((u32)l & 7)) return 0;
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    DlcEnt* e = dlc_find(l);
    if (!e) return 0;
    if (e->gen != pm_dlc_gen) {
        dlc_free(e);
        e->gen = pm_dlc_gen; e->refused = 0;
        unsigned xt0 = pm_xf.on ? pm_ticks() : 0u;
        int r = dlc_build(e, l);
        if (pm_xf.on) { pm_xf.dlc_n++; pm_xf.dlc_t += pm_ticks() - xt0; } /* MAPX-1 */
        e->hash = dlc_hash(l);
        dlc_c.compile++;
        if (r) { e->refused = (u8)r; dlc_c.refused[r < DLR_N ? r : DLR_LONG]++; }
    } else { dlc_c.hit++; }
    if (e->refused) return 0;
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
        if (pm_ph_cen) sp0_grp++;
        if (dlc_box_out(g, m)) {
            tri_since_vtx = g->tsv_exit;
            skipped_last = skipped_any = 1;
            dlc_c.skip++;
            if (pm_ph_cen) sp0_skip++;
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
    return 1;
}

/* S6-2c-2, S6-2c-3: entity model lists from the static pools. The record G_PM_ENT with G_PM_ENT_DRAW (entity_model.c) sits right before the model G_DL. The list is compiled once per
 * key (list address, vertex segment base): nd_ent_segments cuts it at its state ops, nd_ent_strip leaves the vertex loads and triangles, nd_dlc_compile copies them into the static
 * vertex and index pools (the area of the map shape groups, reset with pm_dlc_gen). The draw runs the state ops of each segment in place on the walker (same words, same order), then
 * draws the segment triangles from the pool with dlc_seg: the walker state at the segment is the state of the draw, no second copy of it. A segment the static path refuses
 * (lit, fog without GPU fog, zmark, decal with blend, ring full) runs its old words (the vertex loads before it, then its own ops). Content (the list words and the vertex span of a
 * vertex array) is hashed once per frame and key; a change recompiles, 4 changes make the key dynamic (walker). entcheck: compile and compare, the walker draws the old words. */
EntKey* ent_tab[ENT_TAB];
NdEntSeg ent_sbuf[ENT_SEGMAX];
/* S6-5: the list with the nested lists inlined. 0: a nested target is not host memory, too deep or too long. */
Gfx ent_flat_buf[ND_DLC_MAXOPS + 1];

__attribute__((cold, noinline)) void dlc_report(void) {
    pm_log_hw("dlc2 f%d tri %u opsrep %u | hashbad %u exitbad %u missing %u slotfix %u ringfail %u ents %u | static grp %u tri %u fail %u full %u batch %u kb %u skip %u grp %u xgrp %u", rm_frame, dlc_c.tri, dlc_c.opsrep, dlc_c.hashbad, dlc_c.exitbad, dlc_c.missing, dlc_c.slotfix, dlc_c.ringfail, dlc_nent, dlc_c.srep, dlc_c.stri, dlc_c.sfail, dlc_c.sfull, dlc_c.batch, pm_gpu_gstat_used() / 64u, dlc_c.skip, dlc_c.grp, dlc_c.xgrp);
    memset(&dlc_c, 0, sizeof dlc_c);
}








