/* nd_geom.c: the geometry group (plan_s10_path group G, GM-2). G_VTX (vertex load to the ring) and G_TRI1/G_TRI2 (draw_tri):
 * the triangle path, its clip and cull helpers, the epoch and state checks. Cut from the former pm_gbi.c and nd_walk.c without logic change (D12).
 * fold_axis stays in the former pm_gbi.c (the window fold of the rectangle path, used by nd_rect.c). */
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

/* GLOB: definitions moved here from the former pm_gbi.c (layout only). */
u8 vert_live[64];
int tri_since_vtx;
u8 vert_scr[64];
s16 vert_sx[64], vert_sy[64];
unsigned char vert_cb[64]; /* DIP1: clip bits of vert_p, made once at G_VTX */
float vert_p[64][4]; /* PO2: clip space x y z w of a vertex, transformed once when it is loaded (G_VTX) */
u8 vert_litf[64]; /* GV5: bit 0 = loaded lit, bit 1 = loaded lit with texgen, bit 2 = the lights changed since (the colour was made then): the CPU path only */
u8 vert_gvf[64]; /* GV1: bit 0 = vert_p valid, bit 1 = the ring copy's S, T and colour read (gv_xf, GM-3). 3 for every vertex made. */
unsigned gv_fb[GVR_N], gv_load_n, gv_tri_n;
unsigned gp1_ld_in, gp1_ld_out, gp1_ld_cross, gp1_vx_in, gp1_vx_out, gp1_vx_cross;
unsigned short vert_ring[64], vert_gmi[64];
unsigned vert_tss[64];
unsigned st_gpu_tri; /* triangles drawn by the raw path */
unsigned st_vx; /* PO2: vertices transformed at G_VTX, triangle corners read (the old code transformed one matrix product per corner) */
int pm_nolight = -1; /* F11-10: -1 = read the switch at first use, the Options row sets 0 or 1 */
unsigned st_lit, st_fogg, st_fogb, st_blc, st_tgen, st_tri; /* S25: draws that use lights, fog, blend colour, texgen */
int n_cmd, n_in, n_drop, n_out;

typedef NdGvP GvP;


int clip_bits4(const float* p) { /* the same bits as clip_bits, from a clip position x y z w */
    int b = 0;
    float w = p[3];
    if (p[0] < -w) b |= 1;
    if (p[0] > w) b |= 2;
    if (p[1] < -w) b |= 4;
    if (p[1] > w) b |= 8;
    if (p[2] > w) b |= 32;
    return b;
}


/* FLK1: screen area sign of a triangle from clip positions (x y z w, the vert_p layout). Both copies of the cull test use it. */
float cull_cross(const float* p1, const float* p2, const float* p3) {
    float ex1 = p1[0] * p2[3] - p2[0] * p1[3], ey1 = p1[1] * p2[3] - p2[1] * p1[3];
    float ex2 = p3[0] * p2[3] - p2[0] * p3[3], ey2 = p3[1] * p2[3] - p2[1] * p3[3];
    float cross = (ex1 * ey2 - ey1 * ex2) * (p1[3] * p3[3]);
    if ((p1[3] < 0) ^ (p2[3] < 0) ^ (p3[3] < 0)) cross = -cross;
    return cross;
}

void gv_params_calc(unsigned tss, GvP* o) { int tw = 1, th = 1; pm_tex_size(&tw, &th); pm_gbi_gv_at(tss, nd_ws.geom, tw, th, o); } /* S3N-3b: not static, the ndmcheck compare calls it */

/* S3N-3c: the draw parameters at geometry geo (the lighting, texgen and fog bits of the batch), the rest as the walker state */
void pm_gbi_gv_at(unsigned tss, uint32_t geo, int tw, int th, GvP* o) {
    NdGvIn in;
    unsigned st = nd_ws.use_t1 ? 1 : 0;
    unpack8(nd_ws.envc, in.en);
    unpack8(cur_plan.vtx, in.vc);
    in.tss = tss;
    in.lighting = (geo & G_LIGHTING) != 0; in.texgen = (geo & G_TEXTURE_GEN) != 0; in.fog = (geo & G_FOG) != 0;
    in.comb5 = comb_mode5; in.plan_active = plan_active;
    in.plan_vtx_ok = cur_plan.vtxname != PM_NM_SHADE && cur_plan.vtxname != PM_NM_NONE;
    in.alpha_shade = comb_alpha_shade;
    in.boost = dual_on && nd_ws.tst1.comb2_a == G_CCMUX_SHADE && nd_ws.tst1.comb2_b == G_CCMUX_ENVIRONMENT && nd_ws.tst1.comb2_c == G_CCMUX_COMBINED && nd_ws.tst1.comb2_d == G_CCMUX_COMBINED;
    in.primz = (nd_ws.omode_l & (1u << G_MDSFT_ZSRCSEL)) != 0; in.prim_z = nd_ws.prim_z;
    in.envc = nd_ws.envc; in.plan_vtx = cur_plan.vtx;
    in.tw = tw; in.th = th;
    in.shs = nd_ws.tst1.tile_shift[st][0]; in.sht = nd_ws.tst1.tile_shift[st][1];
    in.tex_on = nd_ws.tex_on;
    in.vp[0] = vp_sx; in.vp[1] = vp_sy; in.vp[2] = vp_tx; in.vp[3] = vp_ty;
    in.tile_x0 = nd_ws.tile_x0; in.tile_y0 = nd_ws.tile_y0; in.tile_fx = nd_ws.tile_fx; in.tile_fy = nd_ws.tile_fy;
    in.fogmix = fogmix_on && (geo & G_FOG);
    in.zc_ok = 0; in.za = 1.f; in.zb = 0.f;
    if (in.fogmix) in.zc_ok = pm_gpu_gv_zcoef(&in.za, &in.zb);
    in.fog_fm = fog_fm; in.fog_fo = fog_fo;
    in.lt = lt; in.lt_n = lt_n; in.lk = (const int8_t (*)[3])lk;
    nd_gv_params(&in, o);
}

unsigned tri_flags(void) {
    unsigned f = 0, bl = nd_ws.omode_l;
    int two = ((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) == 1;
    unsigned a1 = (bl >> 30) & 3, a2 = (bl >> 22) & 3, b1 = (bl >> 28) & 3, b2 = (bl >> 20) & 3;
    if (nd_ws.geom & G_LIGHTING) f |= 1;
    if (nd_ws.geom & G_FOG) f |= 2;
    if (nd_ws.geom & G_TEXTURE_GEN) f |= 4;
    if (two ? (b1 == 3 || b2 == 3 || a1 == 3 || a2 == 3) : (a1 == 3 || a2 == 3)) f |= 8;
    if (two ? (b1 == 2 || b2 == 2 || a1 == 2 || a2 == 2) : (a1 == 2 || a2 == 2)) f |= 16;
    return f;
}

void tri_stats_add(unsigned f, unsigned n) {
    st_tri += n;
    if (f & 1) st_lit += n;
    if (f & 2) st_fogg += n;
    if (f & 4) st_tgen += n;
    if (f & 8) st_fogb += n;
    if (f & 16) st_blc += n;
}

void tri_stats(void) { tri_stats_add(tri_flags(), 1); }

/* GV1: make what the CPU path reads from the ring copy of a vertex: the clip position with its exact reject bits, S and T, the colour. */
void gv_xf(int sl) {
    if (vert_gvf[sl] == 3) return;
    const Vtx* r = (const Vtx*)(pm_gpu_gring() + 16u * vert_ring[sl]);
    if (!(vert_gvf[sl] & 1)) {
        const float(*m)[4] = gm_tab[vert_gmi[sl]];
        float x = (float)r->v.ob[0], y = (float)r->v.ob[1], z = (float)r->v.ob[2];
        float* p = vert_p[sl];
        p[0] = m[0][0] * x + m[1][0] * y + m[2][0] * z + m[3][0];
        p[1] = m[0][1] * x + m[1][1] * y + m[2][1] * z + m[3][1];
        p[2] = m[0][2] * x + m[1][2] * y + m[2][2] * z + m[3][2];
        p[3] = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];
        vert_cb[sl] = (unsigned char)clip_bits4(p);
    }
    vert_gvf[sl] = 3;
}

/* GV5: the lights are about to change (G_MOVEMEM light, G_MW_NUMLIGHT). A lit vertex that is still in the ring has not seen its colour yet: make it now with the old lights and keep it on the CPU path. */
int lit_pending; /* a lit vertex was loaded to the ring since the last lit_freeze */
void lit_freeze(void) {
    if (!lit_pending) return; /* no vertex was loaded lit since the last freeze */
    lit_pending = 0;
    for (int s = 0; s < 64; s++) {
        if (!vert_live[s] || !(vert_litf[s] & 3) || (vert_litf[s] & 4) || vert_ring[s] == GV_NONE) continue;
        if (vert_gvf[s] != 3) gv_xf(s);
        vert_litf[s] |= 4;
    }
}


int gv_lit_ok(void) { if (pm_nolight < 0) pm_nolight = pm_debug_has("nolight"); return pm_gpu_gv5_ok() && !pm_nolight; }
int gv_fog_gpu_ok(void) { return pm_gpu_gv5_ok() && pm_gpu_gv_zcoef(NULL, NULL); }


/* GV1b: state epoch. gv_epoch goes up at every display list op that is not a vertex load, a triangle, a matrix op or a list jump. The decision that depends on state
 * only (fallback reason, cull mode, the combiner, texture and render sync, the draw parameters) is made once per epoch and per texture scale, not per triangle. */
unsigned gv_epoch = 1, ep_epoch; unsigned ep_osc, ep_fl, ep_n, ep_pepoch, ep_ptss, ep_open_gen, ep_open_epoch, ep_open_tss, ep_open_gmi;

int ep_why, ep_ct, ep_cullboth, ep_open, ep_t1f; /* ep_t1f: TEXEL1 reads an off-screen window: gv_tri folds the window per triangle (t1_fold_tri) */

GvP ep_p;


void ep_flush_stats(void) { if (ep_n) { tri_stats_add(ep_fl, ep_n); ep_n = 0; } }

/* what the state alone says: a reason to leave the GPU path (or -1), the cull type, CULL_BOTH */
int gv_state_why(int* ct, int* both) {
    unsigned cg = nd_ws.geom & G_CULL_BOTH;
    *ct = cg == G_CULL_FRONT ? 1 : cg == G_CULL_BACK ? 2 : 0;
    *both = cg == G_CULL_BOTH;
    if ((nd_ws.geom & G_LIGHTING) && !gv_lit_ok()) return GVR_LIT;
    if (fogmix_on && (nd_ws.geom & G_FOG) && !gv_fog_gpu_ok()) return GVR_FOG;
    if (img_role(0) == ROLE_DEPTH) return GVR_ZMARK;
    return -1;
}

/* the first triangle (or DLC2 range) after a state change */
void gv_epoch_enter(void) {
    ep_flush_stats();
    ep_why = gv_state_why(&ep_ct, &ep_cullboth);
    ep_t1f = 0;
    if (ep_why < 0 && !ep_cullboth) {
        ep_fl = tri_flags();
        ensure_tile(nd_ws.use_t1 ? 1 : 0);
        dual_bind();
        sync_render();
        /* TEXEL1: the GPU reads tile 1 through the second texture unit and the uv map of dual_bind (uvx). An off-screen TEXEL1 needs the window fold of t1_fold per draw: gv_tri does it per triangle (t1_fold_tri, ep_t1f). */
        if (plan_active && cur_plan.uses_tex1) {
            if (!nd_ws.td[0].valid || !nd_ws.td[1].valid) ep_why = GVR_TEX1;
            else if (pm_tex_aux_off_idx() >= 0) ep_t1f = 1;
        }
    }
    ep_epoch = gv_epoch;
    ep_pepoch = 0;
}

/* GV1: one triangle of ring vertices. Returns 1 when it is done (rejected or sent to the GPU). Returns 0 when the CPU path must take it (reason in gv_fb). */
int gv_tri(const int* idx) {
    int a = idx[0], b = idx[1], c = idx[2];
    int why = -1;
    if ((vert_cb[a] & vert_cb[b] & vert_cb[c]) & 0x2f) { n_out++; PH_ADD(C_GVREJ, 1); GP_LAP(GPL_EARLY); return 1; }
    if ((vert_cb[a] | vert_cb[b] | vert_cb[c]) & 0x2f) {
        if (!(vert_gvf[a] & 1) && vert_cb[a]) gv_xf(a);
        if (!(vert_gvf[b] & 1) && vert_cb[b]) gv_xf(b);
        if (!(vert_gvf[c] & 1) && vert_cb[c]) gv_xf(c);
        if ((vert_cb[a] & vert_cb[b] & vert_cb[c]) & 0x2f) { n_out++; PH_ADD(C_GVREJ, 1); GP_LAP(GPL_EARLY); return 1; }
    }
    if (vert_scr[a] | vert_scr[b] | vert_scr[c]) why = GVR_SCR;
    else if (vert_tss[a] != vert_tss[b] || vert_tss[a] != vert_tss[c]) why = GVR_TSS;
    else if ((vert_litf[a] | vert_litf[b] | vert_litf[c]) || (nd_ws.geom & G_LIGHTING)) { /* GV5: the vertices were loaded with the lighting state of now, and the lights did not change since (the RSP lights at the load) */
        unsigned want = (nd_ws.geom & G_LIGHTING) ? 1u | ((nd_ws.geom & G_TEXTURE_GEN) ? 2u : 0u) : 0u;
        if (((vert_litf[a] | vert_litf[b] | vert_litf[c]) & 4) || vert_litf[a] != want || vert_litf[b] != want || vert_litf[c] != want) why = GVR_LIT;
    }
    if (why < 0) {
        if (ep_epoch != gv_epoch) gv_epoch_enter(); /* the first triangle after a state change */
        why = ep_why;
    }
    if (why < 0) {
        unsigned tss = vert_tss[a];
        if (ep_cullboth) { n_out++; GP_LAP(GPL_EARLY); return 1; }
        if (ep_pepoch != gv_epoch || ep_ptss != tss) {
            gv_params_calc(tss, &ep_p);
            pm_gpu_set_rgb_scale(ep_p.boost ? 2 : 1);
            ep_pepoch = gv_epoch; ep_ptss = tss;
        }
        if (ep_t1f) {
            if ((vert_litf[a] | vert_litf[b] | vert_litf[c]) & 2) { gv_fb[GVR_TEX1]++; PH_ADD(C_GVFB, 1); return 0; } /* a texgen vertex keeps the refusal */
            unsigned rt[3] = { vert_ring[a], vert_ring[b], vert_ring[c] };
            t1_fold_tri(rt, tss); /* a changed window map flushes the open batch (flush_gen), a new batch opens below */
        }
        ep_n++;
        if (ep_open && ep_open_gen == pm_gpu_flush_gen() && ep_open_epoch == gv_epoch && ep_open_tss == tss && pm_gpu_gappend((unsigned)vert_ring[a], (unsigned)vert_ring[b], (unsigned)vert_ring[c], vert_gmi[a], vert_gmi[b], vert_gmi[c])) {
            gp3_t0_surv(); n_in++; st_gpu_tri++; gv_tri_n++; PH_ADD(C_GVTRI, 1); GP_LAP(GPL_EMIT); return 1;
        }
        unsigned gg[3] = { vert_gmi[a], vert_gmi[b], vert_gmi[c] }, ri[3] = { vert_ring[a], vert_ring[b], vert_ring[c] }, ro[3];
        unsigned mvm = 0; /* TRI-2: a moved copy (edge vertex) is not the ring slot of the CPU path */
        if (pm_gpu_gtrip((const float(*)[4][4])gm_tab, gg, ep_p.vp, ep_p.uvso, ep_p.cm, ep_p.ca, ep_p.zm, ep_ct, ep_p.fxon ? &ep_p.fx : NULL, ri, ro, &mvm)) {
            if (!(mvm & 1u)) vert_ring[a] = (unsigned short)ro[0];
            if (!(mvm & 2u)) vert_ring[b] = (unsigned short)ro[1];
            if (!(mvm & 4u)) vert_ring[c] = (unsigned short)ro[2];
            ep_open = 1; ep_open_gen = pm_gpu_flush_gen(); ep_open_epoch = gv_epoch; ep_open_tss = tss;
            gp3_t0_surv(); n_in++; st_gpu_tri++; gv_tri_n++; PH_ADD(C_GVTRI, 1); GP_LAP(GPL_EMIT); return 1;
        }
        ep_n--; ep_open = 0;
        why = GVR_REFUSE;
    }
    gv_fb[why]++;
    PH_ADD(C_GVFB, 1);
    return 0;
}

/* PF1: software prefetch (PLD) of the display list, list targets and vertex data. The ARM11 of the O3DS has no hardware prefetcher and a small L1. Azahar has no cache model: no change there. */
unsigned tri_drop[2]; /* DEL: triangles dropped at the draw_tri drop site, printed as tridrop in rect_log */
/* DEL: one tridrop line per rect_log: triangles dropped since the last line (tri_drop, reset here) and the gv_fb reasons as deltas (gv_fb is not reset: other readers). */
void nd_tridrop_log(unsigned f) {
    static unsigned gvfb_snap[GVR_N];
    pm_log_d("tridrop f%u none %u scr %u tss %u lit %u fog %u zmark %u tex1 %u refuse %u t1fold %u", f, tri_drop[0], gv_fb[GVR_SCR] - gvfb_snap[GVR_SCR], gv_fb[GVR_TSS] - gvfb_snap[GVR_TSS], gv_fb[GVR_LIT] - gvfb_snap[GVR_LIT], gv_fb[GVR_FOG] - gvfb_snap[GVR_FOG], gv_fb[GVR_ZMARK] - gvfb_snap[GVR_ZMARK], gv_fb[GVR_TEX1] - gvfb_snap[GVR_TEX1], gv_fb[GVR_REFUSE] - gvfb_snap[GVR_REFUSE], t1fold_n);
    t1fold_n = 0;
    memcpy(gvfb_snap, gv_fb, sizeof gvfb_snap);
    tri_drop[0] = tri_drop[1] = 0;
}
int tri_prep_ok; /* PG4: off_screen_check and box_fold_sync ran for this op */
void pm_gpu_cen_mark(unsigned bit); /* CEN-1a (pm_gpu.c) */
void draw_tri(unsigned w);

void draw_tri(unsigned w) {
    int i0 = (int)((w >> 16) & 0xff) / 2;
    int i1 = (int)((w >> 8) & 0xff) / 2;
    int i2 = (int)(w & 0xff) / 2;
    int idx[3];
    GP_START();
    gp_ntri++;
    n_cmd++;
    tri_since_vtx = 1;
    if (i0 < 0 || i1 < 0 || i2 < 0 || i0 > 63 || i1 > 63 || i2 > 63) {
        n_drop++;
        return;
    }
    if (!vert_live[i0] || !vert_live[i1] || !vert_live[i2]) {
        n_drop++;
        if (dlc_dirty) dlc_c.slotfix++; /* DLC1: a vertex that the interpreter would have kept live after a replayed list */
        return;
    }
    idx[0] = i0;
    idx[1] = i1;
    idx[2] = i2;
    if (!tri_prep_ok) { if (ep_osc != gv_epoch) { off_screen_check(); box_fold_sync(); ep_osc = gv_epoch; } tri_prep_ok = 1; } /* PG4: the second triangle of a G_TRI2 sees the same state */
    { /* GV1: ring vertices go to the GPU with no CPU transform. The rest reads the ring copy. */
        if (vert_ring[i0] != GV_NONE && vert_ring[i1] != GV_NONE && vert_ring[i2] != GV_NONE && gv_tri(idx)) return;
    }
    /* TRI-4: no CPU triangle path. A triangle the ring did not take is dropped and counted (DEL): tri_drop[0] a vertex not on the ring, tri_drop[1] all on the ring and gv_tri refused (gv_fb names why) */
    tri_drop[(vert_ring[i0] == GV_NONE || vert_ring[i1] == GV_NONE || vert_ring[i2] == GV_NONE) ? 0 : 1]++;
    n_drop++;
}

void nd_geom_vtx(u32 w0, u32 w1, const Gfx* list) { /* G_VTX: the vertex load (cut from nd_walk.c, GM-2) */
    unsigned gv_t0 = gp_on() ? pm_ticks() : 0;
    unsigned n = (w0 >> 12) & 0xff;
    unsigned v0n = (w0 >> 1) & 0x7f;
    unsigned v0 = (v0n >= n) ? v0n - n : 0;
    Vtx* src = (Vtx*)ptr_of(w1);
    if (src) __builtin_prefetch(src + n); /* H4: the next vertex block */
    if (src && !is_host((u32)src)) {
        gbi_cold_bad_vtx(w1, w0, (u32)list);
        src = NULL;
    }
    if (src && n <= 64) {
        for (unsigned pk = 0; pk < n * 16u; pk += 32u) __builtin_prefetch((const char*)src + pk);
        if (v0 >= 64) n = 0; /* S30: a vertex load that starts past the buffer (garbage list) copied 4 GB */
        else if (v0 + n > 64) n = 64 - v0;
        PH_ADD(C_VTXLOAD, n);
        /* A load at slot 0 starts a new mesh. Later loads in the same
         * list append above it and keep the earlier slots. */
        if (v0 == 0 && tri_since_vtx) memset(vert_live, 0, sizeof vert_live);
        tri_since_vtx = 0;
        int gring0 = -1;
        unsigned gmi = GV_NONE;
        int gv = 0;
        const int lit_ld = (nd_ws.geom & G_LIGHTING) != 0;
        if (n) { /* GV1: a load goes to the ring as it is (GV5: also a lit load, the GPU lights it). A full ring takes the old code. */
            if (lit_ld && !gv_lit_ok()) gv_fb[GVR_LITLOAD]++;
            else {
                gmi = gv_matrix_id(lit_ld);
                if (gmi != GV_NONE) gring0 = pm_gpu_gvtx_load(src, n);
                if (gring0 >= 0) gv = 1; else { gmi = GV_NONE; gv_fb[GVR_RING]++; }
            }
        }
        if (gv) { gv_load_n += n; PH_ADD(C_GVLOAD, n); }
        int gp1_cls = GP1_CROSS;
        unsigned char gp1_p = 0;
        s16 gp1_lo[3] = { 32767, 32767, 32767 }, gp1_hi[3] = { -32768, -32768, -32768 };
        if (gv) {
            for (unsigned j = 0; j < n; j++)
                for (int q = 0; q < 3; q++) {
                    s16 o = src[j].v.ob[q];
                    if (o < gp1_lo[q]) gp1_lo[q] = o;
                    if (o > gp1_hi[q]) gp1_hi[q] = o;
                }
            gp1_cls = gp1_box_class(gp1_lo, gp1_hi, combined, &gp1_p);
            if (gp1_cls == GP1_IN) { gp1_ld_in++; gp1_vx_in += n; }
            else if (gp1_cls == GP1_OUT) { gp1_ld_out++; gp1_vx_out += n; }
            else { gp1_ld_cross++; gp1_vx_cross += n; }
        }
        for (unsigned i = 0; i < n; i++) {
            vert_ring[v0 + i] = gring0 >= 0 ? (unsigned short)(gring0 + (int)i) : GV_NONE;
            vert_gmi[v0 + i] = (unsigned short)gmi;
            vert_tss[v0 + i] = ((unsigned)nd_ws.tex_ss << 16) | ((unsigned)nd_ws.tex_ts & 0xffffu);
            vert_live[v0 + i] = 1;
            vert_scr[v0 + i] = 0;
            vert_litf[v0 + i] = (u8)(gv && lit_ld ? 1 | ((nd_ws.geom & G_TEXTURE_GEN) ? 2 : 0) : 0);
            if (gv && lit_ld) lit_pending = 1;
            if (gv && gp1_cls == GP1_IN) {
                vert_cb[v0 + i] = 0;
                vert_gvf[v0 + i] = 0;
                continue;
            }
            if (gv && gp1_cls == GP1_OUT) {
                vert_cb[v0 + i] = gp1_p;
                vert_gvf[v0 + i] = 0;
                continue;
            }
            if (gv) { /* GP1 cross: per-vertex product and clip bits */
                const float(*m)[4] = combined;
                float x = (float)src[i].v.ob[0], y = (float)src[i].v.ob[1], z = (float)src[i].v.ob[2];
                float* p = vert_p[v0 + i];
                p[0] = m[0][0] * x + m[1][0] * y + m[2][0] * z + m[3][0];
                p[1] = m[0][1] * x + m[1][1] * y + m[2][1] * z + m[3][1];
                p[2] = m[0][2] * x + m[1][2] * y + m[2][2] * z + m[3][2];
                p[3] = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];
                vert_cb[v0 + i] = (unsigned char)clip_bits4(p);
                vert_gvf[v0 + i] = 1;
                st_vx++;
                continue;
            }
            vert_gvf[v0 + i] = 3;
        }
    }
    if (gp_on()) gp_acc[7] += pm_ticks() - gv_t0;
}
