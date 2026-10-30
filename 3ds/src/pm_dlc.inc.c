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
static PM_HOTD int dlc_on; static int dlc_depth;
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

static __attribute__((noinline)) u32 dlc_geom_at(const DlcEnt* e, const DlcGrp* g, u32 entry, unsigned sg) {
    u32 x = (entry & g->am) | g->om;
    for (unsigned i = 0; i < sg; i++) {
        const u32* op = e->gop[g->goff + i];
        x = (x & op[0]) | op[1];
    }
    return x;
}

/* C3: adjacent gop segments share the same draw geom for every list entry (PSO inputs unchanged). */
static __attribute__((noinline)) int dlc_gop_same_geom(const DlcEnt* e, const DlcGrp* g, unsigned sg) {
    u32 a0 = dlc_geom_at(e, g, 0, sg);
    u32 a1 = dlc_geom_at(e, g, 0xffffffffu, sg);
    u32 b0 = dlc_geom_at(e, g, 0, sg + 1u);
    u32 b1 = dlc_geom_at(e, g, 0xffffffffu, sg + 1u);
    return a0 == a1 && b0 == b1 && a0 == b0;
}

/* C3: record draw ranges at compile; merge segments that share PSO geom (ends + gop index before each range). */
static __attribute__((cold, noinline)) void dlc_batch_compile(DlcGrp* g, const DlcEnt* e) {
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
static __attribute__((cold, noinline)) int dlc_compile(DlcEnt* e, const Gfx* l) {
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
static __attribute__((cold, noinline)) void dlc_static_build(DlcEnt* e, const Gfx* l) {
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
static __attribute__((noinline)) int dlc_seg(const unsigned short* idx, unsigned nt, unsigned gmi) {
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
static __attribute__((noinline)) int dlc_static_group(const DlcEnt* e, const DlcGrp* g, unsigned gmi, u32 gg) {
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

static __attribute__((noinline)) DlcEnt* dlc_find(const Gfx* l) {
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

static __attribute__((noinline)) void dlc_set_geom(u32 g) {
    if (g == geom) return;
    geom = g;
    pm_gpu_set_mode((geom & G_ZBUFFER) != 0, (geom & G_LIGHTING) != 0);
    gv_epoch++;
}

/* The matrix id of `combined` (the same search as the G_VTX code), GV_NONE when the table is full. */
static __attribute__((noinline)) unsigned dlc_gmi(void) { return gv_matrix_id(0); }

/* Box test against the clip planes of clip_bits4. Returns 1 when every vertex of the box is outside one plane (by a margin for the float error of the interpreter). */
static __attribute__((noinline)) int dlc_box_out(const DlcGrp* g, const float (*m)[4]) {
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
static __attribute__((noinline)) unsigned char gp1_box_subset(const s16 lo[3], const s16 hi[3], const float (*m)[4]) {
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
static __attribute__((noinline)) int gp1_box_in(const s16 lo[3], const s16 hi[3], const float (*m)[4]) {
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
static __attribute__((noinline)) int gp1_box_class(const s16 lo[3], const s16 hi[3], const float (*m)[4], unsigned char* pbits) {
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
static __attribute__((cold, noinline)) void dlc_verify_skip(const Gfx* l, const DlcGrp* g, const float (*m)[4]) {
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
static __attribute__((noinline)) void dlc_run_group(const Gfx* l, const DlcGrp* g, unsigned gmi) {
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
static __attribute__((cold, noinline)) void dlc_reset(void) {
    for (unsigned k = 0; k < DLC_TAB; k++) if (dlc_tab[k]) { dlc_free(dlc_tab[k]); free(dlc_tab[k]); dlc_tab[k] = NULL; }
    dlc_nent = 0;
    pm_gpu_gstat_reset();
}
static unsigned dlc_seen_gen;
/* A G_DL to l. Returns 1 when the list is done (replayed), 0 when the interpreter must run it. */
static __attribute__((noinline)) int dlc_try(const Gfx* l) {
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

static __attribute__((cold, noinline)) void dlc_report(void) {
    pm_log_hw("dlc2 f%d tri %u opsrep %u | hashbad %u exitbad %u missing %u slotfix %u ringfail %u ents %u | static grp %u tri %u fail %u full %u batch %u kb %u", rm_frame, dlc_c.tri, dlc_c.opsrep, dlc_c.hashbad, dlc_c.exitbad, dlc_c.missing, dlc_c.slotfix, dlc_c.ringfail, dlc_nent, dlc_c.srep, dlc_c.stri, dlc_c.sfail, dlc_c.sfull, dlc_c.batch, pm_gpu_gstat_used() / 64u);
    memset(&dlc_c, 0, sizeof dlc_c);
}
