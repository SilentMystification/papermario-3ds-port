/* nd_snap.c: the snap record (NDR0A, design_nstate.md section 6, stage S4). Core 1 (nd_snap_mesh, from nd_mesh_tri1/2 in nd_geo.c) takes the
 * combine inputs of nd_ds and the table decision of nd_pso_find_i, and records them as an NDC_NAPI run. Core 0 (nd_snapchk_run) copies the
 * record and arms nd_snap_arm; the next triangle draw (nd_snapchk_tri, from draw_tri in nd_geom.c) applies its combine (NDR0E2).
 * DEL3A: the compares and the snapchk log lines are deleted; the snap is applied only. */
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_int.h"
#include "nd_ds.h"
#include "nd_api.h"
#include "nd_snap.h"
#include "nd_tgt.h"
#include "pm_port.h"

extern unsigned pm_frame_n; /* pm_main.c: the logic step counter */
extern unsigned fogc, blendc; /* nd_set.c:41 */
extern int pm_nofog; /* NDR0F1: the option (pm_options.c), read only */

/* ---- core 1 ---- */
static int sn_leg, sn_mode5, sn_pat;
static NdSnapChk sn_last;
static unsigned sn_cmds = ~0u;
void nd_snap_task(void) { sn_cmds = ~0u; } /* NAT2J: a task stream starts with its own snap */
static NdTgt sn_tgt;
static unsigned sn_vprim; /* NREC4: 1 while a particle table is taken */
unsigned nd_snb, nd_snn, nd_snm; /* NAT1: core 1 snap_build calls, new snaps, table misses */
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

static int snap_build0(const NdDs* s, int zc, NdSnapChk* rp) { /* NDR0E2: the snap is taken at every mesh triangle and texture rectangle */
    NdSnapChk r;
    NdCombOut o;
    memset(&r, 0, sizeof r);
    memset(&o, 0, sizeof o);
    nd_snb++; /* NAT1 */
    nd_snap_in(s, sn_leg, sn_pat, &r.in);
    nd_snap_rnd(s, &r);
    r.role = nd_tgt_role(sn_tgt.kind, sn_tgt.siz, 0, s->w.omode_l, s->w.omode_h, zc);
    r.mixk = nd_snap_mix(s->w.omode_l, s->fogc, s->blendc, pm_nofog, &r.mixc); /* NDR0F1: the mix of nd_rnd_sync */
    r.fill = s->w.fill; r.primz = s->w.prim_z; /* NAT4H */
    nd_render_state(s->w.omode_h, s->w.omode_l, r.role == ROLE_DEPTH, &r.rs); /* NDR0F1: the render state of nd_rnd_sync */
    nd_snap_bind(&s->w.td[0], &s->w.tst1, 0, &r.b[0]); nd_snap_bind(&s->w.td[1], &s->w.tst1, 1, &r.b[1]); /* NDR0G1 */
    if (sn_cmds == nd_rec_cmds() && !memcmp(&r.in, &sn_last.in, sizeof r.in) && !memcmp(&r.role, &sn_last.role, 6u * 4u) && !memcmp(&r.rs, &sn_last.rs, sizeof r.rs) && r.plod == sn_last.plod && r.mixk == sn_last.mixk && r.mixc == sn_last.mixc && !memcmp(r.b, sn_last.b, sizeof r.b) && r.fill == sn_last.fill && r.primz == sn_last.primz) return 0; /* no new command, same inputs; NAT4H fill and prim_z */
    nd_snn++; /* NAT1 */
    r.key = nd_pso_find_i(&r.in, sn_vprim, &o);
    r.plan_on = -1;
    if (r.key >= 0) nd_pso_fix(&o, &r.in, sn_mode5);
    else { nd_snm++; nd_comb_core1(&r.in, sn_vprim, sn_mode5, &o); } /* NDR0D: a table miss decides on core 1 with its own table (nd_decide.c) */
    if (o.plan_on) pm_comb_colours_k(&o.plan, r.in.prim, r.in.env, r.in.prim_lo8 & 0xffu, r.in.key_c, r.in.key_s, r.in.conv_k4 & 255u);
    r.o = o;
    sn_leg = o.leg; sn_mode5 = o.mode5; sn_pat = o.pat;
    r.plan_on = o.plan_on;
    r.leg = sn_leg; r.mode5 = sn_mode5; r.pat = sn_pat;
    sn_last = r;
    *rp = r;
    return 1;
}
static int snap_build(const NdDs* s, int zc, NdSnapChk* rp) { NDT_B(nt8); const int r = snap_build0(s, zc, rp); NDT_E(nt8, NDT_SNAP); return r; } /* NAT8A */
void nd_snapcp_run(const uint32_t* rec);
static void snap_take_r(const NdDs* s, int zc, NdNapiRun run) { NdSnapChk r; if (!snap_build(s, zc, &r)) return; nd_rec_st(run, &r, sizeof r / 8u); /* NDR0E2: a state op (no dschk record, shadow stays); NWS4A: run */ sn_cmds = nd_rec_cmds(); }
static void snap_take(const NdDs* s, int zc) { snap_take_r(s, zc, nd_snapchk_run); } /* NWS4A */
const NdCombOut* nd_snap_now(void) { NdSnapChk t; (void)snap_build(&nd_ds, 0, &t); sn_cmds = ~0u; return &sn_last.o; } /* NAT2K: the decision of nd_ds now; the next snap_take records again */
#define LS_SMAX 256u /* NAT2D: unique snaps, each in its own stream block */
#define LS_RMAX 256u /* NAT2D: ranges */
static NdSnapChk ls_t; static const NdSnapChk* ls_p[LS_SMAX]; static uint32_t ls_h[LS_SMAX], ls_f[LS_RMAX]; static unsigned ls_n, ls_sn, ls_ord; static unsigned char ls_chg;
_Static_assert(ND_LS_HDR(LS_RMAX + LS_SMAX) + ND_LS_DSB <= 32744u, "NAT2D: a table fits one nd_rec_data_st block");
static unsigned ls_find0(void) { /* NAT2D: the index of a snap equal to ls_t (FNV-1a 32 first), else a new stream block */
    const unsigned char* q = (const unsigned char*)&ls_t; uint32_t h = 2166136261u;
    for (unsigned i = 0; i < sizeof ls_t; i++) h = (h ^ q[i]) * 16777619u;
    for (unsigned i = 0; i < ls_sn; i++) if (ls_h[i] == h && !memcmp(ls_p[i], &ls_t, sizeof ls_t)) return i;
    if (ls_sn >= LS_SMAX) return ~0u;
    nd_lsb_n++; NdSnapChk* p = (NdSnapChk*)nd_rec_data_st(sizeof ls_t); *p = ls_t; ls_p[ls_sn] = p; ls_h[ls_sn] = h; return ls_sn++;
}
static unsigned ls_find(void) { nd_lsf_n++; NDT_B(nt8); const unsigned r = ls_find0(); NDT_E(nt8, NDT_LSC); return r; } /* NAT8A */
static unsigned char ls_ovl; /* NREC2: this list has more ranges than LS_MAX */
static unsigned char ls_ep; void nd_lsc_ep(void) { ls_ep = 1; } /* NCONV2C */
void nd_lsc_begin(void) { ls_n = 0; ls_sn = 0; ls_ord = 0; ls_chg = 1; ls_ovl = 0; ls_ep = 0; sn_cmds = ~0u; nd_lsc_on = 1; }
void nd_lsc_begin_v(unsigned v) { nd_lsc_begin(); sn_vprim = v; } /* NREC4 */
void nd_snap_vprim(unsigned v) { sn_vprim = v; sn_cmds = ~0u; } /* NAT5E: the particle plan of the next snaps; the next snap is new */
void nd_lsc_unit(uint32_t w0) { const unsigned id = w0 >> 24; if (id == ND_FS_TRI1 || id == ND_FS_TRI2) { if (ls_chg) { ls_chg = 0; if (ls_n >= LS_RMAX || ls_ord > 0xffffu) ls_ovl = 1; else if (snap_build(&nd_ds, 0, &ls_t)) { const unsigned k = ls_find(); if (k == ~0u) ls_ovl = 1; else { ls_f[ls_n++] = ls_ord | k << 16 | (ls_ep ? ND_LS_EP : 0u); sn_cmds = nd_rec_cmds(); } } else if (ls_ep && ls_n) { ls_f[ls_n] = ls_ord | (ls_f[ls_n - 1u] & 0x3fff0000u) | ND_LS_EP | ND_LS_DUP; ls_n++; } ls_ep = 0; } ls_ord += id == ND_FS_TRI2 ? 2u : 1u; } else if (id != ND_FS_VTX && id != ND_FS_MTX && id != ND_FS_POPMTX && w0 != ND_FS_W0(ND_FS_MISC, ND_FS_MISC_NOP << 16)) ls_chg = 1; }
void nd_lsc_tris(unsigned n) { if (!n) return; nd_lsc_unit(ND_FS_W0(ND_FS_TRI1, 0)); ls_ord += n - 1u; } /* NREC4: n list triangles, one snap test */
unsigned pm_frame_count(void); static __attribute__((cold, noinline)) void ls_over_note(void) { static unsigned n; if (n++ < 16u) pm_log_evt("lsover f%u ranges %u ord %u", pm_frame_count(), ls_n, ls_ord); } /* NAT2G */
static void lsc_end0(uint32_t* dst) { if (ls_ovl) ls_over_note(); nd_lsc_on = 0; sn_cmds = ~0u; sn_vprim = 0; if (!ls_n || ls_ovl) return; /* NREC2: dst is the record word of the table; an over list keeps the walker binds */
    const unsigned h = ND_LS_HDR(ls_n + ls_sn); uint32_t* b = (uint32_t*)nd_rec_data_st(h + ND_LS_DSB); b[0] = ls_n; b[1] = ls_sn; memcpy(b + 2, ls_f, 4u * ls_n);
    for (unsigned i = 0; i < ls_sn; i++) b[2u + ls_n + i] = (uint32_t)(uintptr_t)ls_p[i]; memcpy((char*)b + h, &nd_ds, sizeof nd_ds); /* NCONV2D: the end state */ *dst = (uint32_t)(uintptr_t)b; } /* NAT2D */
void nd_lsc_end_at(uint32_t* dst) { nd_lse_n++; NDT_B(nt8); lsc_end0(dst); NDT_E(nt8, NDT_LSC); } /* NAT8A */
void nd_lsc_end(void) { nd_lsc_end_at(nd_rec_last_pay() + 4); }
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
const NdSnapChk* const nd_ap = &sc_cur; /* NWS4B: the applied snap, read by core 0 */
static __attribute__((cold, noinline)) void ws_log(void) { static unsigned fr = ~0u; if (pm_frame_count() != fr) { fr = pm_frame_count(); pm_log_d("wsnap f%u", fr); } } /* NWS4B */
uint32_t nd_apw(int k) { const NdSnapChk* a = &sc_cur; switch (k) { case NDAPW_W0: return a->in.w0; case NDAPW_W1: return a->in.w1; case NDAPW_OH: return a->in.omode_h; case NDAPW_OL: return a->oml; case NDAPW_PRIM: return a->in.prim; case NDAPW_PLOD: return a->plod; case NDAPW_ENV: return a->in.env; case NDAPW_FOGC: return a->fogc; case NDAPW_DUAL: return (uint32_t)a->in.dual_on; default: return (uint32_t)a->in.use_t1; } } /* NWS4B: pm_gpu.c does not include nd_snap.h */
static __attribute__((cold, noinline)) void mws_cold(void) { static unsigned fr = ~0u; if (pm_frame_count() != fr) { fr = pm_frame_count(); pm_log_d("mwsnap f%u", fr); } } /* NWS4C */
void nd_mws_log(void) { mws_cold(); } /* NWS4C */
void nd_apw_gvc(NdGvC* c) { c->w0 = sc_cur.in.w0; c->w1 = sc_cur.in.w1; c->env = sc_cur.in.env; c->oml = sc_cur.oml; c->primz = sc_cur.primz; c->use_t1 = sc_cur.in.use_t1; c->dual_on = sc_cur.in.dual_on; } /* NWS4C */
void nd_wsnap_chk(void) { const NdCombIn* i = &sc_cur.in; const NdTst* t = &nd_ws.tst1; extern int dual_on, box_folded; extern unsigned keyc, keys, conv_k4; /* NWS4B: the shadow (old reads) against the snap (new reads) */
    const uint32_t ts[16] = { t->comb_a, t->comb_b, t->comb_c, t->comb_d, t->comb_aa, t->comb_ab, t->comb_ac, t->comb_ad, t->comb2_a, t->comb2_b, t->comb2_c, t->comb2_d, t->comb2_aa, t->comb2_ab, t->comb2_ac, t->comb2_ad };
    if (nd_ws.comb_w0 != i->w0 || nd_ws.comb_w1 != i->w1 || nd_ws.omode_h != i->omode_h || nd_ws.omode_l != sc_cur.oml || nd_ws.prim != i->prim || nd_ws.prim_lod != sc_cur.plod || nd_ws.envc != i->env ||
        nd_ws.fill != sc_cur.fill || nd_ws.prim_z != sc_cur.primz || (uint32_t)nd_ws.use_t1 != (uint32_t)i->use_t1 || memcmp(ts, i->tst, sizeof ts) || keyc != i->key_c || keys != i->key_s || conv_k4 != i->conv_k4 ||
        fogc != sc_cur.fogc || blendc != sc_cur.blendc || dual_on != i->dual_on || box_folded != i->box_folded) ws_log(); }
static NdSnapBind sc_eb[2];
static unsigned sc_bfr = ~0u; /* NDR0G2: walker tiles not the snap (data), last bindsnap frame */
unsigned pm_frame_count(void);
int pm_drec_now(void);


/* NDR0G2: the walker tile blocks when e is NULL, else e. */
static const NdSnapBind* sc_wb(const NdSnapBind* e, NdSnapBind* w) {
    if (e) return e;
    for (int n = 0; n < 2; n++) nd_snap_bind(&nd_ws.td[n], &nd_ws.tst1, n, &w[n]);
    return w;
}


/* NDR0G2: 1 when the armed snapshot tile words differ from the blocks e (NULL = the walker tiles). */
int nd_snap_bnew(const NdSnapBind* e) {
    NdSnapBind w[2];
    return nd_snap_bdiff(sc_wb(e, w), sc_cur.b) != 0u;
}

/* NDR0G2: the blocks of the open epoch; the drec run logs the first epoch frame that binds other tile words. */
const NdSnapBind* nd_snap_eb(void) {
    memcpy(sc_eb, sc_cur.b, sizeof sc_eb);
    if (pm_drec_now() && pm_frame_count() != sc_bfr && nd_snap_bnew(NULL)) { sc_bfr = pm_frame_count(); pm_log_d("bindsnap f%u", sc_bfr); }
    return sc_eb;
}

void nd_snapchk_run(const uint32_t* rec) {
    memcpy(&sc_cur, rec, sizeof sc_cur);
    nd_snap_arm = 1;
    nd_snap_live = 1; /* NDR1C */
}
int pm_drec_now(void);
static __attribute__((cold, noinline)) void cps_log(void) { static unsigned fr = ~0u; if (pm_frame_count() != fr) { fr = pm_frame_count(); pm_log_d("cpsnap f%u", fr); } } /* NWS4A */
void nd_snapcp_run(const uint32_t* rec) { if (__builtin_expect(pm_drec_now(), 0) && memcmp(&sc_cur, rec, sizeof sc_cur) != 0) cps_log(); nd_snapchk_run(rec); } /* NWS4A, core 0: a copy snap; the drec marker of UD-N5 when it differs from the snap applied before */


void nd_snap_render(void) { /* NDR0F2: the render state and mix of the armed snapshot, at the triangle epoch (nd_geom.c gv_epoch_enter) */
    const unsigned sg = sc_cur.oml ^ (sc_cur.in.omode_h * 2654435761u) ^ sc_cur.geom ^ (unsigned)sc_cur.role;
    nd_snap_rpend = 0; /* DEL3A: the rnd compare is deleted */
    if (nd_sr_same(sg)) { nd_c0w &= ~2u; return; } /* NAT1B */
    nd_render_apply(&sc_cur.rs, sc_cur.mixk, sc_cur.mixc, sc_cur.in.omode_h, sc_cur.oml, sg);
    nd_c0w &= ~2u; /* NAT1B */
}
static unsigned char lr_geo; void nd_lsr_geo(void) { lr_geo = 1; } /* NCONV2D: the open list runs ls_run (nd_walk.c) */
extern int box_folded, dual_on; extern unsigned keyc, keys, conv_k4; /* nd_comb.c */
void pm_comb_set_k4(unsigned k4); void pm_gpu_set_prim(unsigned rgba, unsigned lod); void pm_gpu_set_env(unsigned rgba); void pm_gpu_flush(void); /* pm_gpu.h */
static void ls_gpu(uint32_t w0, uint32_t w1, int cset, uint32_t prim, uint32_t plod, uint32_t env, uint32_t k4) { /* NCONV2D: the GPU calls of the skipped units */
    if (nd_ws.comb_w0 != w0 || nd_ws.comb_w1 != w1 || (cset && !nd_ws.comb_w0)) pm_gpu_flush(); /* NWS2 */ if (nd_ws.prim != prim || nd_ws.prim_lod != plod) pm_gpu_set_prim(prim, plod);
    if (nd_ws.envc != env) pm_gpu_set_env(env); if (conv_k4 != k4) pm_comb_set_k4(k4); nd_ws.prim_lod = plod; }
static void ds_take(const NdDs* c) { /* NCONV2D: list end: the fields of the skipped units from the core 1 end state (dschk: core 1 equals core 0 at each command) */
    ls_gpu(c->w.comb_w0, c->w.comb_w1, c->comb_set, c->w.prim, c->w.prim_lod, c->w.envc, c->conv_k4);
    nd_ws.omode_l = c->w.omode_l; nd_ws.om_kl = c->w.om_kl; nd_ws.prim = c->w.prim; nd_ws.envc = c->w.envc; nd_ws.comb_w0 = c->w.comb_w0; nd_ws.comb_w1 = c->w.comb_w1;
    {   /* NST2G: a folded core 0 keeps its folded combine; box_save takes the core 1 (unfolded) values, so dschk_fill and the unfold give them */
        const NdTst k = nd_ws.tst1; const int ku = nd_ws.use_t1;
        nd_ws.tst1 = c->w.tst1; nd_ws.use_t1 = c->w.use_t1;
        if (box_folded) {
            nd_ws.tst1.comb_a = k.comb_a; nd_ws.tst1.comb_b = k.comb_b; nd_ws.tst1.comb_c = k.comb_c; nd_ws.tst1.comb_d = k.comb_d;
            nd_ws.tst1.comb_aa = k.comb_aa; nd_ws.tst1.comb_ab = k.comb_ab; nd_ws.tst1.comb_ac = k.comb_ac; nd_ws.tst1.comb_ad = k.comb_ad;
            nd_ws.use_t1 = ku;
        }
    }
    fogc = c->fogc; blendc = c->blendc; keyc = c->keyc; keys = c->keys; conv_k4 = c->conv_k4; dual_on = c->dual_on; } /* NWS2 */
/* NCONV2B: the walker takes the combine and render inputs of the consumed snap (the 1:1 fields of comb_in_walker, nd_comb.c:200).
 * DEL3A: the compare that named each field is deleted. geom and tss stay core 0 state. */
static void sc_adopt(void) {
    extern unsigned keyc, keys, conv_k4; /* nd_comb.c:24-25 */
    extern int dual_on, box_folded; /* nd_comb.c:26-32 */
    const NdCombIn* i = &sc_cur.in;
    ls_gpu(i->w0, i->w1, i->comb_set, i->prim, sc_cur.plod, i->env, i->conv_k4); /* NCONV2D; NAT4D: also for stream snaps */
    nd_ws.comb_w0 = i->w0; nd_ws.comb_w1 = i->w1; nd_ws.omode_h = i->omode_h; nd_ws.prim = i->prim; nd_ws.envc = i->env;
    keyc = i->key_c; keys = i->key_s; conv_k4 = i->conv_k4;
    { NdTst* t = &nd_ws.tst1; const uint32_t* s = i->tst; t->comb_a = s[0]; t->comb_b = s[1]; t->comb_c = s[2]; t->comb_d = s[3]; t->comb_aa = s[4]; t->comb_ab = s[5]; t->comb_ac = s[6]; t->comb_ad = s[7]; t->comb2_a = s[8]; t->comb2_b = s[9]; t->comb2_c = s[10]; t->comb2_d = s[11]; t->comb2_aa = s[12]; t->comb2_ab = s[13]; t->comb2_ac = s[14]; t->comb2_ad = s[15]; } /* NWS2 */
    nd_ws.tex_on = i->tex_on; nd_ws.use_t1 = i->use_t1; dual_on = i->dual_on; box_folded = i->box_folded;
    nd_ws.omode_l = sc_cur.oml; fogc = sc_cur.fogc; blendc = sc_cur.blendc;
    nd_ws.fill = sc_cur.fill; nd_ws.prim_z = sc_cur.primz; /* NAT4H */
}
void nd_snapchk_tri(void) {   /* NDR0E2: a mesh triangle: the compare (switch snapchk), then the combine of the snap */
    nd_snap_arm = 0;
    nd_snap_comb();
    sc_adopt(); /* NCONV2B */
}
/* NDR1C: the combine of the armed snap (nd_rect.c, rectangles; nd_snapchk_tri for triangles). */
void nd_snap_comb(void) { nd_comb_apply(&sc_cur.o, sc_cur.in.w0, sc_cur.in.w1, sc_cur.in.use_t1, sc_cur.in.env); nd_c0w &= ~1u; /* NAT1B */ }
const NdSnapBind* nd_snap_cur_b(void) { return sc_cur.b; } /* NDR1C */
const NdSnapChk* nd_snap_cur(void) { return &sc_cur; } /* NCS1: the snap core 0 applied last (debug readers) */
/* NDR1C: the render state log of a rectangle (drec runs): one line per frame that differs from the walker. */
void nd_snap_rlog(void) { ND_WCHK(); /* NWS4B */
    static unsigned sc_rfr = ~0u;
    if (!pm_drec_now() || pm_frame_count() == sc_rfr) return;
    NdRender w; unsigned wc;
    const int wk = nd_snap_mix(nd_ap->oml, nd_ap->fogc, nd_ap->blendc, pm_nofog, &wc); /* NWS4B */
    nd_render_state(nd_ap->in.omode_h, nd_ap->oml, nd_img_role(0) == ROLE_DEPTH, &w); /* NWS4B */
    if (memcmp(&sc_cur.rs, &w, sizeof w) || sc_cur.mixk != wk || sc_cur.mixc != wc || sc_cur.role != nd_img_role(0) || nd_snap_bnew(NULL)) {
        sc_rfr = pm_frame_count();
        pm_log_d("rectsnap f%u", sc_rfr);
    }
}
/* NCONV1D1: core 0 compares each list range with the walker at its first triangle (nd_lsr_tri); the counters go to the lsnap line. */
unsigned char nd_ls_on; unsigned nd_ls_ord; static const uint32_t* lr_b; static unsigned lr_cur, lr_nx;
void nd_lsr_begin(const uint32_t* b) { lr_b = b; nd_ls_on = b != NULL; nd_ls_ord = 0; lr_cur = ~0u; lr_nx = 0; lr_geo = 0; }
void nd_lsr_end(void) { if (nd_ls_on && lr_geo) ds_take((const NdDs*)(const void*)((const char*)lr_b + ND_LS_HDR(lr_b[0] + lr_b[1]))); lr_geo = 0; nd_ls_on = 0; } /* NCONV2D */
void nd_lsr_seg(unsigned ord) { nd_lsr_tri(ord); }
void nd_lsr_tri(unsigned ord) {
    if (ord < lr_nx) return;
    const unsigned r = nd_snap_lsr(lr_b, ord), n = lr_b[0];
    lr_nx = r + 1u < n ? ND_LS_ORD(lr_b[2u + r + 1u]) : ~0u;
    if (r == lr_cur) return;
    lr_cur = r;
    if (lr_geo && (lr_b[2u + r] & ND_LS_EP)) gv_epoch++; /* NCONV2D: the cut of the skipped units */
    if (lr_b[2u + r] & ND_LS_DUP) return; /* NCONV2C: an ep-only range: the armed snap stays */
    memcpy(&sc_cur, (const void*)(uintptr_t)lr_b[2u + n + ND_LS_SNAP(lr_b[2u + r])], sizeof sc_cur); /* NAT2D: the snap block of the range */
    nd_snap_arm = 1; /* NCONV1D2: the next draw step consumes the range snap (nd_snapchk_tri) */
}
const NdCombOut* nd_snap_dir_o(void) { return nd_snap_live ? &sc_cur.o : NULL; } /* NDR2D: the decision of the live snap */

/* NDR4: the snap of a sprite record (pm_gpu_spr_build reads it). core 0. */
static unsigned sc_sfr = ~0u;
int nd_snap_spr(uint32_t o[9]) { ND_WCHK(); /* NDR4: env prim omh oml geom cw0 cw1 mixk mixc of the live snap; 0 when none */ /* NWS4B */
    if (!nd_snap_live) return 0;
    o[0] = sc_cur.in.env; o[1] = sc_cur.in.prim; o[2] = sc_cur.in.omode_h; o[3] = sc_cur.oml; o[4] = sc_cur.geom;
    o[5] = sc_cur.in.w0; o[6] = sc_cur.in.w1; o[7] = (uint32_t)sc_cur.mixk; o[8] = sc_cur.mixc;
    if (pm_drec_now() && pm_frame_count() != sc_sfr && (o[0] != nd_ap->in.env || o[1] != nd_ap->in.prim || o[2] != nd_ap->in.omode_h || o[3] != nd_ap->oml || /* NWS4B */
        o[4] != nd_ws.geom || o[5] != nd_ap->in.w0 || o[6] != nd_ap->in.w1 || sc_cur.fogc != nd_ap->fogc || sc_cur.blendc != nd_ap->blendc)) { sc_sfr = pm_frame_count(); pm_log_d("sprsnap f%u", sc_sfr); } /* NWS4B */
    return 1;
}

/* NAT2E: one debug line per drec frame in which a particle instance snap gives another combine than the walker has now (UD-N2 expected diff) */
void nd_snap_plog(void) {
    static unsigned ps_fr = ~0u;
    if (!pm_drec_now() || pm_frame_count() == ps_fr) return;
    const NdCombOut* o = &sc_cur.o;
    if (o->plan_on != plan_active || (o->plan_on ? memcmp(&o->plan, &cur_plan, sizeof cur_plan) != 0 : o->leg != leg_mode)) {
        ps_fr = pm_frame_count();
        pm_log_d("partsnap f%u", ps_fr);
    }
}
/* NAT1D: 1 when a walker decision differs from the last snap core 0 took (sc_cur) */
unsigned nd_cd_w, nd_cd_s, nd_cd_off, nd_cd_x; /* NAT1E: walker and snap words of the last compare (plan_on << 8 | leg), the first differing plan byte (0xffff none) */
int nd_snap_cdiff(int plan_on, int leg, const PmPlan* plan, int mode5, int pat, int cc_on, unsigned cc, int env_on) { const NdCombOut* s = &sc_cur.o; /* NAT1E: the detail words below */
    nd_cd_w = (unsigned)plan_on << 8 | ((unsigned)leg & 0xffu); nd_cd_s = (unsigned)s->plan_on << 8 | ((unsigned)s->leg & 0xffu); nd_cd_off = 0xffffu; nd_cd_x = (unsigned)(mode5 != 0) | (unsigned)(pat != 0) << 1 | (unsigned)(cc_on != 0) << 2 | (unsigned)(env_on != 0) << 3 | (unsigned)(s->mode5 != 0) << 8 | (unsigned)(s->pat != 0) << 9 | (unsigned)(s->cc_on != 0) << 10 | (unsigned)(s->env_on != 0) << 11;
    if (plan_on && s->plan_on) { const unsigned char* a = (const unsigned char*)plan; const unsigned char* b = (const unsigned char*)&s->plan; for (unsigned i = 0; i < sizeof *plan; i++) if (a[i] != b[i]) { nd_cd_off = i; break; } }
    return s->plan_on != plan_on || (plan_on ? nd_cd_off != 0xffffu : s->leg != leg) || s->mode5 != mode5 || s->pat != pat || s->cc_on != cc_on || (cc_on && s->cc != cc) || s->env_on != env_on; } /* NAT1F */
unsigned nd_snap_primz(void) { return sc_cur.primz; } /* NAT4H: the sprite path */
unsigned nd_snap_fill(void) { return sc_cur.fill; } /* NAT4H: the z-buffer clear */
