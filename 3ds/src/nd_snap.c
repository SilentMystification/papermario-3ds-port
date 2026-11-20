/* nd_snap.c: the snap record (NDR0A, design_nstate.md section 6, stage S4). Core 1 (nd_snap_mesh, from nd_mesh_tri1/2 in nd_geo.c) takes the
 * combine inputs of nd_ds and the table decision of nd_pso_find_i, and records them as an NDC_NAPI run. Core 0 (nd_snapchk_run) copies the
 * record and arms nd_snap_arm; the next triangle draw (nd_snapchk_tri, from draw_tri in nd_geom.c) applies its combine (NDR0E2).
 * DEL3A: the compares and the snapchk log lines are deleted; the snap is applied only. */
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_int.h"
#include "nd_ds.h"
#include "nd_api.h"
#include "nd_snap.h"
#include "nd_tgt.h"
#include "pm_port.h"

extern unsigned pm_frame_n; /* pm_main.c: the logic step counter */
extern int pm_nofog; /* NDR0F1: the option (pm_options.c), read only */

/* ---- core 1 ---- */
static int sn_leg, sn_mode5, sn_pat;
unsigned nd_snb, nd_snn, nd_snm; /* NAT1: core 1 snap_build calls, new snaps, table misses */
static const NdSnapChk sn_zero; static const NdSnapChk* sn_lp = &sn_zero; /* PERF1: the last snap */
typedef struct { uint32_t h; int32_t m5; uint32_t vp; const NdSnapChk* p; } SiEnt; static SiEnt si_t[256]; static unsigned si_n, si_next; unsigned nd_si_new, nd_si_hit, nd_si_full;
typedef struct { uint32_t h; int32_t m5; uint32_t vp; int32_t key; NdCombIn in; NdCombOut o; } SdEnt; /* SNAPD: one decision */
#define SD_N 128u /* C1MIC: about 66 KB on the heap at first use (MDLBLK freed 256 KB); SNAPD: entries (a power of 2) */
static SdEnt* sd_t; static unsigned char sd_tried; unsigned nd_sd_new, nd_sd_hit;
static uint32_t si_hw(uint32_t h, const void* p, unsigned n) { const uint32_t* w = (const uint32_t*)p; for (unsigned i = 0; i < n / 4u; i++) h = (h ^ w[i]) * 16777619u; return h; }
static int si_eq(const NdSnapChk* a, const NdSnapChk* b) { return !memcmp(&a->in, &b->in, sizeof a->in) && !memcmp(&a->role, &b->role, 6u * 4u) && !memcmp(&a->plod, &b->plod, sizeof *a - offsetof(NdSnapChk, plod)); }
static __attribute__((cold, noinline)) void si_log(void) { si_next = pm_frame_n + 100u; pm_log_evt("snapi f%u new %u hit %u full %u snn %u dec %u dhit %u", pm_frame_n, nd_si_new, nd_si_hit, nd_si_full, nd_snn, nd_sd_new, nd_sd_hit); }
void nd_snap_ctx(void) { memset(si_t, 0, sizeof si_t); si_n = 0; sn_lp = &sn_zero; if (pm_frame_n >= si_next) si_log(); } /* PERF1: a context chain restarts */
static unsigned sn_cmds = ~0u;
void nd_snap_task(void) { sn_cmds = ~0u; } /* NAT2J: a task stream starts with its own snap */
static NdTgt sn_tgt;
static unsigned sn_vprim; /* NREC4: 1 while a particle table is taken */
unsigned nd_lsf_n, nd_lsb_n, nd_lse_n; /* NAT8A */ /* core 1 ls_find calls, new blocks, nd_lsc_end_at calls */
/* the target record of set_cimg (nd_target.c:87-99) without the GPU calls */
void nd_snap_tgt(uint32_t w0, uint32_t w1) {
    void* nc = (void*)(uintptr_t)w1;
    sn_tgt.img = nc; sn_tgt.siz = (uint8_t)((w0 >> 12) & 3u); sn_tgt.stride = (uint16_t)((w0 & 0xfffu) + 1u);
    sn_tgt.kind = (uint8_t)((nc == NULL ? NDK_NULL : 0u) | (w1 == 0u ? NDK_ZERO : 0u) | (nc == (void*)DepthCopyBuffer ? NDK_DEPTHQ : 0u) |
        (pm_is_cfb(nc) ? NDK_CFB : 0u) |
        ((nuGfxZBuffer && (const unsigned short*)nc >= nuGfxZBuffer && (const unsigned short*)nc < nuGfxZBuffer + 320 * 240) ? NDK_INZ : 0u) |
        (pm_fb_native(nc) ? NDK_NATIVE : 0u));
}

static const NdSnapChk* snap_build0(const NdDs* s, int zc) { /* NDR0E2: the snap is taken at every mesh triangle and texture rectangle */
    NdSnapChk r;
    memset(&r, 0, sizeof r);
    nd_snb++; /* NAT1 */
    nd_snap_in(s, sn_leg, sn_pat, &r.in);
    nd_snap_rnd(s, &r);
    r.role = nd_tgt_role(sn_tgt.kind, sn_tgt.siz, 0, s->w.omode_l, s->w.omode_h, zc);
    r.mixk = nd_snap_mix(s->w.omode_l, s->fogc, s->blendc, pm_nofog, &r.mixc); /* NDR0F1: the mix of nd_rnd_sync */
    r.fill = s->w.fill; r.primz = s->w.prim_z; /* NAT4H */ r.kh = s->w.om_kh; r.kl = s->w.om_kl; /* NC0A */
    nd_render_state(s->w.omode_h, s->w.omode_l, r.role == ROLE_DEPTH, &r.rs); /* NDR0F1: the render state of nd_rnd_sync */
    nd_snap_bind(&s->w.td[0], &s->w.tst1, 0, &r.b[0]); nd_snap_bind(&s->w.td[1], &s->w.tst1, 1, &r.b[1]); /* NDR0G1 */
    if (sn_cmds == nd_rec_cmds() && si_eq(&r, sn_lp)) return NULL;
    nd_snn++; /* NAT1 */
    const int m5 = sn_mode5; const uint32_t hi = si_hw(2166136261u ^ (uint32_t)m5 ^ sn_vprim << 8, &r.in, sizeof r.in);
    const uint32_t h = si_hw(si_hw(hi, &r.role, 24u), &r.plod, sizeof r - offsetof(NdSnapChk, plod)); /* SNAPD: hi is the decision key */
    unsigned i = h & 255u, k; for (k = 0; k < 8u && si_t[i].p; k++, i = (i + 1u) & 255u) if (si_t[i].h == h && si_t[i].m5 == m5 && si_t[i].vp == sn_vprim && si_eq(&r, si_t[i].p)) {
        const NdSnapChk* const q = si_t[i].p; nd_si_hit++; sn_leg = q->leg; sn_mode5 = q->mode5; sn_pat = q->pat; sn_lp = q; return q; }
    if (!sd_t && !sd_tried) { sd_tried = 1; sd_t = (SdEnt*)calloc(SD_N, sizeof *sd_t); if (sd_t) for (unsigned q = 0; q < SD_N; q++) sd_t[q].m5 = INT32_MIN; } /* SNAPD: NULL keeps the plain decision; INT32_MIN never matches an empty entry */
    SdEnt* const de = sd_t ? &sd_t[hi & (SD_N - 1u)] : NULL;
    r.plan_on = -1;
    if (de && de->h == hi && de->m5 == m5 && de->vp == sn_vprim && !memcmp(&de->in, &r.in, sizeof r.in)) { nd_sd_hit++; r.key = de->key; r.o = de->o; }
    else {
        r.key = nd_pso_find_i(&r.in, sn_vprim, &r.o);
        if (r.key >= 0) nd_pso_fix(&r.o, &r.in, sn_mode5);
        else { nd_snm++; nd_comb_core1(&r.in, sn_vprim, sn_mode5, &r.o); } /* NDR0D: a table miss decides on core 1 with its own table (nd_decide.c) */
        if (r.o.plan_on) pm_comb_colours_k(&r.o.plan, r.in.prim, r.in.env, r.in.prim_lo8 & 0xffu, r.in.key_c, r.in.key_s, r.in.conv_k4 & 255u);
        nd_sd_new++; if (de) { de->h = hi; de->m5 = m5; de->vp = sn_vprim; de->key = r.key; de->in = r.in; de->o = r.o; }
    }
    sn_leg = r.o.leg; sn_mode5 = r.o.mode5; sn_pat = r.o.pat;
    r.plan_on = r.o.plan_on;
    r.leg = sn_leg; r.mode5 = sn_mode5; r.pat = sn_pat;
    NdSnapChk* const p = (NdSnapChk*)nd_rec_data_st(sizeof r); *p = r; nd_si_new++; sn_lp = p;
    if (k < 8u && si_n < 192u) { si_t[i].h = h; si_t[i].m5 = m5; si_t[i].vp = sn_vprim; si_t[i].p = p; si_n++; } else nd_si_full++; return p;
}
static const NdSnapChk* snap_build(const NdDs* s, int zc) { NDT_B(nt8); const NdSnapChk* const r = snap_build0(s, zc); NDT_E(nt8, NDT_SNAP); return r; } /* NAT8A */
void nd_snapcp_run(const uint32_t* rec);
static void snap_take_r(const NdDs* s, int zc, NdNapiRun run) { const NdSnapChk* const p = snap_build(s, zc); if (!p) return; const uint32_t u[2] = { (uint32_t)(uintptr_t)p, 0u }; nd_rec_st(run, u, 1u); /* NDR0E2: a state op (no dschk record, shadow stays); NWS4A: run */ sn_cmds = nd_rec_cmds(); }
static void snap_take(const NdDs* s, int zc) { snap_take_r(s, zc, nd_snapchk_run); } /* NWS4A */
const NdCombOut* nd_snap_now(void) { (void)snap_build(&nd_ds, 0); sn_cmds = ~0u; return &sn_lp->o; } /* NAT2K: the decision of nd_ds now; the next snap_take records again */
#define LS_SMAX 256u /* NAT2D: unique snaps, each in its own stream block */
#define LS_RMAX 256u /* NAT2D: ranges */
static const NdSnapChk* ls_np; static const NdSnapChk* ls_p[LS_SMAX]; static uint32_t ls_f[LS_RMAX]; static unsigned ls_n, ls_sn, ls_ord; static unsigned char ls_chg;
_Static_assert(ND_LS_HDR(LS_RMAX + LS_SMAX) + ND_LS_DSB <= 32744u, "NAT2D: a table fits one nd_rec_data_st block");
static unsigned ls_find0(void) { for (unsigned i = 0; i < ls_sn; i++) if (ls_p[i] == ls_np) return i; if (ls_sn >= LS_SMAX) return ~0u; nd_lsb_n++; ls_p[ls_sn] = ls_np; return ls_sn++; } /* PERF1 */
static unsigned ls_find(void) { nd_lsf_n++; NDT_B(nt8); const unsigned r = ls_find0(); NDT_E(nt8, NDT_LSC); return r; } /* NAT8A */
static unsigned char ls_ovl; /* NREC2: this list has more ranges than LS_MAX */
static unsigned char ls_ep; void nd_lsc_ep(void) { ls_ep = 1; } /* NCONV2C */
void nd_lsc_begin(void) { ls_n = 0; ls_sn = 0; ls_ord = 0; ls_chg = 1; ls_ovl = 0; ls_ep = 0; sn_cmds = ~0u; nd_lsc_on = 1; }
void nd_lsc_begin_v(unsigned v) { nd_lsc_begin(); sn_vprim = v; } /* NREC4 */
void nd_snap_vprim(unsigned v) { sn_vprim = v; sn_cmds = ~0u; } /* NAT5E: the particle plan of the next snaps; the next snap is new */
void nd_lsc_unit(uint32_t w0) { const unsigned id = w0 >> 24; if (id == ND_FS_TRI1 || id == ND_FS_TRI2) { if (ls_chg) { ls_chg = 0; if (ls_n >= LS_RMAX || ls_ord > 0xffffu) ls_ovl = 1; else if ((ls_np = snap_build(&nd_ds, 0)) != NULL) { const unsigned k = ls_find(); if (k == ~0u) ls_ovl = 1; else { ls_f[ls_n++] = ls_ord | k << 16 | (ls_ep ? ND_LS_EP : 0u); sn_cmds = nd_rec_cmds(); } } else if (ls_ep && ls_n) { ls_f[ls_n] = ls_ord | (ls_f[ls_n - 1u] & 0x3fff0000u) | ND_LS_EP | ND_LS_DUP; ls_n++; } ls_ep = 0; } ls_ord += id == ND_FS_TRI2 ? 2u : 1u; } else if (id != ND_FS_VTX && id != ND_FS_MTX && id != ND_FS_POPMTX && w0 != ND_FS_W0(ND_FS_MISC, ND_FS_MISC_NOP << 16)) ls_chg = 1; }
void nd_lsc_tris(unsigned n) { if (!n) return; nd_lsc_unit(ND_FS_W0(ND_FS_TRI1, 0)); ls_ord += n - 1u; } /* NREC4: n list triangles, one snap test */
unsigned pm_frame_count(void); static __attribute__((cold, noinline)) void ls_over_note(void) { static unsigned n; if (n++ < 16u) pm_log_evt("lsover f%u ranges %u ord %u", pm_frame_count(), ls_n, ls_ord); } /* NAT2G */
static void lsc_end0(uint32_t* dst) { if (ls_ovl) ls_over_note(); nd_lsc_on = 0; sn_cmds = ~0u; sn_vprim = 0; if (!ls_n || ls_ovl) return; /* NREC2: dst is the record word of the table; an over list keeps the walker binds */
    const unsigned h = ND_LS_HDR(ls_n + ls_sn); uint32_t* b = (uint32_t*)nd_rec_data_st(h + ND_LS_DSB); b[0] = ls_n; b[1] = ls_sn; memcpy(b + 2, ls_f, 4u * ls_n);
    for (unsigned i = 0; i < ls_sn; i++) b[2u + ls_n + i] = (uint32_t)(uintptr_t)ls_p[i]; { uint32_t* const e = (uint32_t*)((char*)b + h); e[0] = nd_ds.w.comb_w0; e[1] = nd_ds.w.comb_w1; e[2] = (uint32_t)nd_ds.comb_set; e[3] = nd_ds.w.prim; e[4] = nd_ds.w.prim_lod; e[5] = nd_ds.w.envc; e[6] = nd_ds.conv_k4; e[7] = nd_ds.w.om_kl; } /* NCONV2D: the end state */ *dst = (uint32_t)(uintptr_t)b; } /* NAT2D */
void nd_lsc_end_at(uint32_t* dst) { nd_lse_n++; NDT_B(nt8); lsc_end0(dst); NDT_E(nt8, NDT_LSC); } /* NAT8A */
static NdDs sn_tmp; /* NDR2A, core 1: nd_ds with the pso words of a direct draw */
void nd_snap_pso(const NdPsoN64* p, uint32_t prim, uint32_t env) { /* NDR2A: the state words of nd_direct_bind, in its order */
    sn_tmp = nd_ds;
    sn_tmp.w.omode_h = (sn_tmp.w.omode_h & ~p->omh_m) | (p->omh & p->omh_m); sn_tmp.w.om_kh |= p->omh_m;
    sn_tmp.w.omode_l = (sn_tmp.w.omode_l & 7u) | (p->oml & ~7u); sn_tmp.w.om_kl |= ~7u;
    if (sn_tmp.w.comb_w0 != p->cw0 || sn_tmp.w.comb_w1 != p->cw1 || !sn_tmp.comb_set) nd_ds_apply(&sn_tmp, ND_FS_W0(ND_FS_COMB, p->cw0), p->cw1);
    sn_tmp.dual_on = 0; sn_tmp.w.prim = prim; sn_tmp.w.prim_lod = 0u; sn_tmp.w.envc = env;
    snap_take(&sn_tmp, 0);
}
void nd_snap_mesh(void) { snap_take(&nd_ds, 0); }
void nd_snap_rect(void) { const int zc = sn_tgt.siz == 2 && (sn_tgt.kind & (NDK_ZERO | NDK_INZ)) && !(nd_ds.w.omode_l & (1u << G_MDSFT_ZSRCSEL)); nd_ds.zslot |= (unsigned char)(zc != 0); /* NAT5T */ snap_take(&nd_ds, zc); } /* NDR1C: zcol of nd_rect.c:236 */
void nd_snap_alt(const NdDs* s, int rect) { const int zc = rect && sn_tgt.siz == 2 && (sn_tgt.kind & (NDK_ZERO | NDK_INZ)) && !(s->w.omode_l & (1u << G_MDSFT_ZSRCSEL)); /* NWS4A, core 1: the snap of a copy draw */
    sn_cmds = ~0u; snap_take_r(s, zc, nd_snapcp_run); sn_cmds = ~0u; } /* the memo of the main stream never counts a copy snap */

/* ---- core 0 ---- */
unsigned char nd_snap_arm;
unsigned char nd_snap_live; /* NDR1C: a snap is armed for this rectangle run; a command clears it */
unsigned char nd_snap_rpend; /* NDR0F1: set by draw_tri for its own triangle, cleared after nd_gv_tri and by nd_snap_render */
static NdSnapChk sc_cur;
const NdSnapChk* nd_ap = &sc_cur; /* PERF1: the applied snap, in the stream or the hold copy */
uint32_t nd_apw(int k) { const NdSnapChk* a = nd_ap; switch (k) { case NDAPW_W0: return a->in.w0; case NDAPW_W1: return a->in.w1; case NDAPW_OH: return a->in.omode_h; case NDAPW_OL: return a->oml; case NDAPW_PRIM: return a->in.prim; case NDAPW_PLOD: return a->plod; case NDAPW_ENV: return a->in.env; case NDAPW_FOGC: return a->fogc; case NDAPW_DUAL: return (uint32_t)a->in.dual_on; case NDAPW_KH: return a->kh; case NDAPW_KL: return a->kl; default: return (uint32_t)a->in.use_t1; } } /* NWS4B: pm_gpu.c does not include nd_snap.h */
void nd_apw_gvc(NdGvC* c) { c->w0 = nd_ap->in.w0; c->w1 = nd_ap->in.w1; c->env = nd_ap->in.env; c->oml = nd_ap->oml; c->primz = nd_ap->primz; c->use_t1 = nd_ap->in.use_t1; c->dual_on = nd_ap->in.dual_on; } /* NWS4C */
static NdSnapBind sc_eb[2];
static unsigned sc_bfr = ~0u; /* NDR0G2: walker tiles not the snap (data), last bindsnap frame */
unsigned pm_frame_count(void);
int pm_drec_now(void);


/* NDR0G2: the walker tile blocks when e is NULL, else e. */
static const NdSnapBind* sc_wb(const NdSnapBind* e, NdSnapBind* w) {
    if (e) return e;
    for (int n = 0; n < 2; n++) nd_snap_bind(&nd_c0.td[n], &nd_c0.tst1, n, &w[n]);
    return w;
}


/* NDR0G2: 1 when the armed snapshot tile words differ from the blocks e (NULL = the walker tiles). */
int nd_snap_bnew(const NdSnapBind* e) {
    NdSnapBind w[2];
    return nd_snap_bdiff(sc_wb(e, w), nd_ap->b) != 0u;
}

/* NDR0G2: the blocks of the open epoch; the drec run logs the first epoch frame that binds other tile words. */
const NdSnapBind* nd_snap_eb(void) {
    memcpy(sc_eb, nd_ap->b, sizeof sc_eb);
    if (pm_drec_now() && pm_frame_count() != sc_bfr && nd_snap_bnew(NULL)) { sc_bfr = pm_frame_count(); pm_log_d("bindsnap f%u", sc_bfr); }
    return sc_eb;
}

void nd_snapchk_run(const uint32_t* rec) {
    nd_ap = (const NdSnapChk*)(uintptr_t)rec[0]; /* PERF1 */
    nd_snap_arm = 1;
    nd_snap_live = 1; /* NDR1C */
}
int pm_drec_now(void);
static __attribute__((cold, noinline)) void cps_log(void) { static unsigned fr = ~0u; if (pm_frame_count() != fr) { fr = pm_frame_count(); pm_log_d("cpsnap f%u", fr); } } /* NWS4A */
void nd_snapcp_run(const uint32_t* rec) { if (__builtin_expect(pm_drec_now(), 0) && memcmp(nd_ap, (const void*)(uintptr_t)rec[0], sizeof *nd_ap) != 0) cps_log(); nd_snapchk_run(rec); } /* NWS4A, core 0: a copy snap; the drec marker of UD-N5 when it differs from the snap applied before */


void nd_snap_render(void) { /* NDR0F2: the render state and mix of the armed snapshot, at the triangle epoch (nd_geom.c gv_epoch_enter) */
    const unsigned sg = nd_ap->oml ^ (nd_ap->in.omode_h * 2654435761u) ^ nd_ap->geom ^ (unsigned)nd_ap->role;
    nd_snap_rpend = 0; /* DEL3A: the rnd compare is deleted */
    if (nd_sr_same(sg)) { nd_c0w &= ~2u; return; } /* NAT1B */
    nd_render_apply(&nd_ap->rs, nd_ap->mixk, nd_ap->mixc, nd_ap->in.omode_h, nd_ap->oml, sg);
    nd_c0w &= ~2u; /* NAT1B */
}
static unsigned char lr_geo; void nd_lsr_geo(void) { lr_geo = 1; } /* NCONV2D: the open list runs ls_run (nd_walk.c) */
void pm_comb_set_k4(unsigned k4); void pm_gpu_set_prim(unsigned rgba, unsigned lod); void pm_gpu_set_env(unsigned rgba); void pm_gpu_flush(void); /* pm_gpu.h */
static void ls_gpu(uint32_t w0, uint32_t w1, int cset, uint32_t prim, uint32_t plod, uint32_t env, uint32_t k4) { /* NCONV2D: the GPU calls of the skipped units */
    if (nd_c0.comb_w0 != w0 || nd_c0.comb_w1 != w1 || (cset && !nd_c0.comb_w0)) pm_gpu_flush(); /* NWS2 */ if (nd_c0.prim != prim || nd_c0.prim_lod != plod) pm_gpu_set_prim(prim, plod);
    if (nd_c0.envc != env) pm_gpu_set_env(env); if (nd_c0.conv_k4 != k4) pm_comb_set_k4(k4); nd_c0.prim_lod = plod; }
void nd_snapchk_tri(void) {   /* NDR0E2: a mesh triangle: the compare (switch snapchk), then the combine of the snap */
    nd_snap_arm = 0;
    nd_snap_comb();
    { /* NWS4D: the GPU memo of the applied words (the walker shadow is gone) */
        const NdCombIn* i = &nd_ap->in;
        ls_gpu(i->w0, i->w1, i->comb_set, i->prim, nd_ap->plod, i->env, i->conv_k4); /* NCONV2D; NAT4D: also for stream snaps */
        nd_c0.comb_w0 = i->w0; nd_c0.comb_w1 = i->w1; nd_c0.omode_h = i->omode_h; nd_c0.prim = i->prim; nd_c0.envc = i->env; nd_c0.conv_k4 = i->conv_k4; nd_c0.tex_on = i->tex_on;
    }
}
/* NDR1C: the combine of the armed snap (nd_rect.c, rectangles; nd_snapchk_tri for triangles). */
void nd_snap_comb(void) { nd_comb_apply(&nd_ap->o, nd_ap->in.w0, nd_ap->in.w1, nd_ap->in.use_t1, nd_ap->in.env); nd_c0w &= ~1u; /* NAT1B */ }
const NdSnapBind* nd_snap_cur_b(void) { return nd_ap->b; } /* NDR1C */
const NdSnapChk* nd_snap_cur(void) { return nd_ap; } /* NCS1: the snap core 0 applied last (debug readers) */
void nd_snap_hold(void) { if (nd_ap != &sc_cur) { sc_cur = *nd_ap; nd_ap = &sc_cur; } } /* PERF1, core 0: the walked chain may be rebuilt next */
/* NDR1C: the render state log of a rectangle (drec runs): one line per frame that differs from the walker. */
void nd_snap_rlog(void) { /* NWS4B */
    static unsigned sc_rfr = ~0u;
    if (!pm_drec_now() || pm_frame_count() == sc_rfr) return;
    NdRender w; unsigned wc;
    const int wk = nd_snap_mix(nd_ap->oml, nd_ap->fogc, nd_ap->blendc, pm_nofog, &wc); /* NWS4B */
    nd_render_state(nd_ap->in.omode_h, nd_ap->oml, ND_ROLE(0) == ROLE_DEPTH, &w); /* NWS4B */
    if (memcmp(&nd_ap->rs, &w, sizeof w) || nd_ap->mixk != wk || nd_ap->mixc != wc || nd_ap->role != ND_ROLE(0) || nd_snap_bnew(NULL)) {
        sc_rfr = pm_frame_count();
        pm_log_d("rectsnap f%u", sc_rfr);
    }
}
/* NCONV1D1: core 0 compares each list range with the walker at its first triangle (nd_lsr_tri); the counters go to the lsnap line. */
unsigned char nd_ls_on; unsigned nd_ls_ord; static const uint32_t* lr_b; static unsigned lr_cur, lr_nx;
void nd_lsr_begin(const uint32_t* b) { lr_b = b; nd_ls_on = b != NULL; nd_ls_ord = 0; lr_cur = ~0u; lr_nx = 0; lr_geo = 0; }
void nd_lsr_end(void) { /* NCONV2D: list end: the GPU memo of the skipped units from the core 1 end state (dschk: core 1 equals core 0 at each command) */
    if (nd_ls_on && lr_geo) {
        const uint32_t* const c = (const uint32_t*)(const void*)((const char*)lr_b + ND_LS_HDR(lr_b[0] + lr_b[1]));
        ls_gpu(c[0], c[1], (int)c[2], c[3], c[4], c[5], c[6]);
        nd_c0.prim = c[3]; nd_c0.envc = c[5]; nd_c0.comb_w0 = c[0]; nd_c0.comb_w1 = c[1]; nd_c0.conv_k4 = c[6];
    }
    lr_geo = 0; nd_ls_on = 0; } /* NWS4D */
void nd_lsr_seg(unsigned ord) { nd_lsr_tri(ord); }
void nd_lsr_tri(unsigned ord) {
    if (ord < lr_nx) return;
    const unsigned r = nd_snap_lsr(lr_b, ord), n = lr_b[0];
    lr_nx = r + 1u < n ? ND_LS_ORD(lr_b[2u + r + 1u]) : ~0u;
    if (r == lr_cur) return;
    lr_cur = r;
    if (lr_geo && (lr_b[2u + r] & ND_LS_EP)) gv_epoch++; /* NCONV2D: the cut of the skipped units */
    if (lr_b[2u + r] & ND_LS_DUP) return; /* NCONV2C: an ep-only range: the armed snap stays */
    nd_ap = (const NdSnapChk*)(uintptr_t)lr_b[2u + n + ND_LS_SNAP(lr_b[2u + r])]; /* NAT2D: the snap block of the range */
    nd_snap_arm = 1; /* NCONV1D2: the next draw step consumes the range snap (nd_snapchk_tri) */
}
const NdCombOut* nd_snap_dir_o(void) { return nd_snap_live ? &nd_ap->o : NULL; } /* NDR2D: the decision of the live snap */

/* NDR4: the snap of a sprite record (pm_gpu_spr_build reads it). core 0. */
static unsigned sc_sfr = ~0u;
int nd_snap_spr(uint32_t o[9]) { /* NDR4: env prim omh oml geom cw0 cw1 mixk mixc of the live snap; 0 when none */ /* NWS4B */
    if (!nd_snap_live) return 0;
    o[0] = nd_ap->in.env; o[1] = nd_ap->in.prim; o[2] = nd_ap->in.omode_h; o[3] = nd_ap->oml; o[4] = nd_ap->geom;
    o[5] = nd_ap->in.w0; o[6] = nd_ap->in.w1; o[7] = (uint32_t)nd_ap->mixk; o[8] = nd_ap->mixc;
    if (pm_drec_now() && pm_frame_count() != sc_sfr && (o[0] != nd_ap->in.env || o[1] != nd_ap->in.prim || o[2] != nd_ap->in.omode_h || o[3] != nd_ap->oml || /* NWS4B */
        o[5] != nd_ap->in.w0 || o[6] != nd_ap->in.w1 || nd_ap->fogc != nd_ap->fogc || nd_ap->blendc != nd_ap->blendc)) { sc_sfr = pm_frame_count(); pm_log_d("sprsnap f%u", sc_sfr); } /* NWS4B */
    return 1;
}

/* NAT2E: one debug line per drec frame in which a particle instance snap gives another combine than the walker has now (UD-N2 expected diff) */
void nd_snap_plog(void) {
    static unsigned ps_fr = ~0u;
    if (!pm_drec_now() || pm_frame_count() == ps_fr) return;
    const NdCombOut* o = &nd_ap->o;
    if (o->plan_on != plan_active || (o->plan_on ? memcmp(&o->plan, &cur_plan, sizeof cur_plan) != 0 : o->leg != leg_mode)) {
        ps_fr = pm_frame_count();
        pm_log_d("partsnap f%u", ps_fr);
    }
}
unsigned nd_snap_primz(void) { return nd_ap->primz; } /* NAT4H: the sprite path */
unsigned nd_snap_fill(void) { return nd_ap->fill; } /* NAT4H: the z-buffer clear */
