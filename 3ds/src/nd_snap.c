/* nd_snap.c: the snapchk switch (NDR0A, design_nstate.md section 6, stage S4). Core 1 (nd_snap_mesh, from nd_mesh_tri1/2 in nd_geo.c) takes the
 * combine inputs of nd_ds and the table decision of nd_pso_find_i, and records them as an NDC_NAPI run. Core 0 (nd_snapchk_run) copies the
 * record and arms nd_snap_arm; the next triangle draw (nd_snapchk_tri, from draw_tri in nd_geom.c) compares it with the walker inputs (behind PMDL("snapchk")), and applies its combine (NDR0E2).
 * Logs: `snapchk bad f<n> <names>` (the first 8 bad triangles), `snapchk f<n> recs <r> cmp <c> bad <b> fbad <s> miss <m>` every 100 frames. */
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
void comb_in_walker(NdCombIn* in); /* nd_comb.c: the walker inputs of the combine (core 0) */
extern int sl_const, ls_use, ls_cyc, ls_dual; extern unsigned sl_cc; /* NDR0E1: nd_comb.c */
int pm_gpu_comb_now(void); const PmPlan* pm_gpu_plan_now(void); unsigned pm_gpu_const_now(void); unsigned pm_gpu_env_now(void);

/* ---- core 1 ---- */
static int sn_leg, sn_mode5, sn_pat;
static NdSnapChk sn_last;
static unsigned sn_cmds = ~0u;
static NdTgt sn_tgt;
/* the target record of set_cimg (nd_target.c:87-99) without the GPU calls */
void nd_snap_tgt(uint32_t w0, uint32_t w1) {
    void* nc = (void*)(uintptr_t)w1;
    sn_tgt.img = nc; sn_tgt.siz = (uint8_t)((w0 >> 12) & 3u); sn_tgt.stride = (uint16_t)((w0 & 0xfffu) + 1u);
    sn_tgt.kind = (uint8_t)((nc == NULL ? NDK_NULL : 0u) | (w1 == 0u ? NDK_ZERO : 0u) | (nc == (void*)DepthCopyBuffer ? NDK_DEPTHQ : 0u) |
        (pm_is_cfb(nc) ? NDK_CFB : 0u) |
        ((nuGfxZBuffer && (const unsigned short*)nc >= nuGfxZBuffer && (const unsigned short*)nc < nuGfxZBuffer + 320 * 240) ? NDK_INZ : 0u) |
        (pm_fb_native(nc) ? NDK_NATIVE : 0u));
}

static int snap_build(const NdDs* s, int zc, NdSnapChk* rp) { /* NDR0E2: the snap is taken at every mesh triangle and texture rectangle; the compare stays behind PMDL("snapchk") */
    NdSnapChk r;
    NdCombOut o;
    memset(&r, 0, sizeof r);
    memset(&o, 0, sizeof o);
    nd_snap_in(s, sn_leg, sn_pat, &r.in);
    nd_snap_rnd(s, &r);
    r.role = nd_tgt_role(sn_tgt.kind, sn_tgt.siz, 0, s->w.omode_l, s->w.omode_h, zc);
    r.mixk = nd_snap_mix(s->w.omode_l, s->fogc, s->blendc, pm_nofog, &r.mixc); /* NDR0F1: the mix of sync_render */
    nd_render_state(s->w.omode_h, s->w.omode_l, r.role == ROLE_DEPTH, &r.rs); /* NDR0F1: the render state of sync_render */
    nd_snap_bind(&s->w.td[0], &s->w.tst1, 0, &r.b[0]); nd_snap_bind(&s->w.td[1], &s->w.tst1, 1, &r.b[1]); /* NDR0G1 */
    if (sn_cmds == nd_rec_cmds() && !memcmp(&r.in, &sn_last.in, sizeof r.in) && !memcmp(&r.role, &sn_last.role, 5u * 4u) && !memcmp(&r.rs, &sn_last.rs, sizeof r.rs) && r.mixk == sn_last.mixk && r.mixc == sn_last.mixc && !memcmp(r.b, sn_last.b, sizeof r.b)) return 0; /* no new command, same inputs */
    r.key = nd_pso_find_i(&r.in, 0u, &o);
    r.plan_on = -1;
    if (r.key >= 0) nd_pso_fix(&o, &r.in, sn_mode5);
    else nd_comb_core1(&r.in, 0u, sn_mode5, &o); /* NDR0D: a table miss decides on core 1 with its own table (nd_decide.c) */
    if (o.plan_on) pm_comb_colours_k(&o.plan, r.in.prim, r.in.env, r.in.prim_lo8 & 0xffu, r.in.key_c, r.in.key_s, r.in.conv_k4 & 255u);
    r.o = o;
    sn_leg = o.leg; sn_mode5 = o.mode5; sn_pat = o.pat;
    r.plan_on = o.plan_on;
    r.leg = sn_leg; r.mode5 = sn_mode5; r.pat = sn_pat;
    sn_last = r;
    *rp = r;
    return 1;
}
static void snap_take(const NdDs* s, int zc) { NdSnapChk r; if (!snap_build(s, zc, &r)) return; nd_rec_st(nd_snapchk_run, &r, sizeof r / 8u); /* NDR0E2: a state op (no dschk record, shadow stays) */ sn_cmds = nd_rec_cmds(); }
#define LS_MAX 16u
static NdSnapChk ls_s[LS_MAX]; static uint32_t ls_f[LS_MAX]; static unsigned ls_n, ls_ord, ls_over; static unsigned char ls_chg;
static unsigned char ls_ovl; /* NREC2: this list has more ranges than LS_MAX */
void nd_lsc_begin(void) { ls_n = 0; ls_ord = 0; ls_chg = 1; ls_ovl = 0; sn_cmds = ~0u; nd_lsc_on = 1; }
void nd_lsc_unit(uint32_t w0) { const unsigned id = w0 >> 24; if (id == ND_FS_TRI1 || id == ND_FS_TRI2) { if (ls_chg) { ls_chg = 0; if (ls_n >= LS_MAX) { ls_over++; ls_ovl = 1; } else if (snap_build(&nd_ds, 0, &ls_s[ls_n])) { ls_f[ls_n++] = ls_ord; sn_cmds = nd_rec_cmds(); } } ls_ord += id == ND_FS_TRI2 ? 2u : 1u; } else if (id != ND_FS_VTX && id != ND_FS_MTX && id != ND_FS_POPMTX && w0 != ND_FS_W0(ND_FS_MISC, ND_FS_MISC_NOP << 16)) ls_chg = 1; }
void nd_lsc_end_at(uint32_t* dst) { nd_lsc_on = 0; sn_cmds = ~0u; if (!ls_n || ls_ovl) return; /* NREC2: dst is the record word of the table; an over list keeps the walker binds */
    const unsigned h = ND_LS_HDR(ls_n); uint32_t* b = (uint32_t*)nd_rec_data_st(h + ls_n * (unsigned)sizeof ls_s[0]); b[0] = ls_n; b[1] = ls_ord; memcpy(b + 2, ls_f, 4u * ls_n); memcpy((char*)b + h, ls_s, ls_n * sizeof ls_s[0]); *dst = (uint32_t)(uintptr_t)b; }
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
void nd_snap_rect(void) { snap_take(&nd_ds, sn_tgt.siz == 2 && (sn_tgt.kind & (NDK_ZERO | NDK_INZ)) && !(nd_ds.w.omode_l & (1u << G_MDSFT_ZSRCSEL))); } /* NDR1C: zcol of nd_rect.c:236 */

/* ---- core 0 ---- */
unsigned char nd_snap_arm;
unsigned char nd_snap_live; /* NDR1C: a snap is armed for this rectangle run; a command clears it */
unsigned char nd_snap_rpend; /* NDR0F1: set by draw_tri for its own triangle, cleared after gv_tri and by nd_snapchk_rnd */
static NdSnapChk sc_cur;
static unsigned sc_recs, sc_cmp, sc_bad, sc_fbad, sc_miss, sc_nlog, sc_bucket;
static int sc_nb;
static const char* sc_nm[8];
static NdSnapBind sc_eb[2];
static unsigned sc_wst, sc_bfr = ~0u; /* NDR0G2: walker tiles not the snap (data), last bindsnap frame */
unsigned pm_frame_count(void);
int pm_drec_now(void);
extern unsigned char nd_cmd_last; /* nd_rec.c:181 */

static void sc_add(const char* s) {
    if (sc_nb < 8) sc_nm[sc_nb] = s;
    sc_nb++;
}

/* NDR0G2: the walker tile blocks when e is NULL, else e. */
static const NdSnapBind* sc_wb(const NdSnapBind* e, NdSnapBind* w) {
    if (e) return e;
    for (int n = 0; n < 2; n++) nd_snap_bind(&nd_ws.td[n], &nd_ws.tst1, n, &w[n]);
    return w;
}

/* NDR0G2: the count of a bad triangle (the first 8 with names), tag = the compare that found it. */
static void sc_count(const char* tag) {
    char names[192];
    if (sc_nb) {
        sc_bad++;
        sc_fbad += (unsigned)sc_nb;
        if (sc_nlog < 8) {
            unsigned k = 0;
            names[0] = 0;
            for (int i = 0; i < sc_nb && i < 8; i++) k += (unsigned)snprintf(names + k, sizeof names - k, " %s", sc_nm[i]);
            sc_nlog++;
            pm_log_evt("snapchk bad f%u %s%s", pm_frame_n, tag, names);
        }
    }
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
    sc_recs++;
}

#define SC_CMP(f, name) do { if (sc_cur.in.f != mine.f) sc_add(name); } while (0)

static void sc_compare(void) {
    NdCombIn mine;
    char names[192];
    comb_in_walker(&mine);
    sc_cmp++;
    sc_nb = 0;
    SC_CMP(w0, "w0"); SC_CMP(w1, "w1"); SC_CMP(omode_h, "omh"); SC_CMP(prim, "prim"); SC_CMP(env, "env");
    SC_CMP(prim_lo8, "plo8"); SC_CMP(key_c, "keyc"); SC_CMP(key_s, "keys"); SC_CMP(conv_k4, "cvk4");
    SC_CMP(tex_on, "tex_on"); SC_CMP(use_t1, "use_t1"); SC_CMP(t1_valid, "t1_valid"); SC_CMP(dual_on, "dual_on");
    SC_CMP(box_prod, "box_prod"); SC_CMP(box_folded, "box_folded"); SC_CMP(comb_set, "comb_set");
    SC_CMP(comb_alpha_shade, "comb_alpha_shade");
    if (sc_cur.role != nd_img_role(0)) sc_add("role");
    if (sc_cur.oml != nd_ws.omode_l) sc_add("oml");
    if (sc_cur.geom != nd_ws.geom) sc_add("geom");
    if (sc_cur.fogc != fogc) sc_add("fogc");
    if (sc_cur.blendc != blendc) sc_add("blendc");
    if (memcmp(sc_cur.in.tst, mine.tst, sizeof mine.tst)) sc_add("tst");
    if (sc_cur.leg != leg_mode) sc_add("leg");
    if (sc_cur.mode5 != comb_mode5) sc_add("mode5");
    if (sc_cur.pat != box_pat) sc_add("pat");
    if (sc_cur.plan_on != plan_active) sc_add("plan_on");
    {   /* NDR0E1: the core 0 combine state and the GPU state against the snap decision */
        const NdCombOut* o = &sc_cur.o;
        if (plan_active && o->plan_on && memcmp(&cur_plan, &o->plan, sizeof o->plan)) sc_add("cplan");
        if (sl_const != o->cc_on || (o->cc_on && sl_cc != o->cc)) sc_add("sl");
        if (ls_use != o->use || ls_cyc != o->cyc || ls_dual != o->dual) sc_add("ls");
        if (pm_gpu_comb_now() != (o->plan_on ? 8 : o->leg)) sc_add("gcomb");
        else if (o->plan_on && memcmp(pm_gpu_plan_now(), &o->plan, sizeof o->plan)) sc_add("gplan");
        if (o->cc_on && pm_gpu_const_now() != o->cc) sc_add("gconst");
        if (o->env_on && pm_gpu_env_now() != sc_cur.in.env) sc_add("genv");
    }
    if (nd_snap_bnew(NULL)) sc_wst++; /* NDR0G2: stale walker tiles (data, not bad) */
    if (sc_cur.key < 0) sc_miss++;
    if (sc_nb) {
        sc_bad++;
        sc_fbad += (unsigned)sc_nb;
        if (sc_nlog < 8) {
            unsigned k = 0;
            names[0] = 0;
            for (int i = 0; i < sc_nb && i < 8; i++) k += (unsigned)snprintf(names + k, sizeof names - k, " %s", sc_nm[i]);
            sc_nlog++;
            pm_log_evt("snapchk bad f%u%s last %u", pm_frame_n, names, (unsigned)nd_cmd_last);
        }
    }
}
void nd_snapchk_rnd(void) {   /* NDR0F1: after sync_render of the triangle epoch (nd_geom.c gv_epoch_enter) */
    nd_snap_rpend = 0;
    if (!PMDL("snapchk")) return; /* NDR0F1B: draw_tri sets the flag in every run */
    sc_nb = 0;
    NdRender w; unsigned wc; /* NDR0F2: the walker decision, as sync_render makes it */
    const int wk = nd_snap_mix(nd_ws.omode_l, fogc, blendc, pm_nofog, &wc);
    nd_render_state(nd_ws.omode_h, nd_ws.omode_l, nd_img_role(0) == ROLE_DEPTH, &w);
    if (memcmp(&sc_cur.rs, &w, sizeof w)) sc_add("rs");
    if (sc_cur.mixk != wk || sc_cur.mixc != wc) sc_add("mix");
    sc_count("rnd");
}

/* NDR0G2: the bind compare of the armed snapshot at the triangle epoch: the bits of nd_snap_bdiff against the walker tile blocks of the open epoch. */
void nd_snapchk_bind(void) {
    NdSnapBind w[2];
    if (!PMDL("snapchk")) return; /* NDR0G2: the switch off returns */
    const unsigned m = nd_snap_bdiff(sc_wb(nd_ep_bs, w), sc_cur.b);
    sc_nb = 0;
    if (m & 1u) sc_add("b0td");
    if (m & 2u) sc_add("b0tile");
    if (m & 4u) sc_add("b1td");
    if (m & 8u) sc_add("b1tile");
    sc_count("bind");
}

void nd_snap_render(void) { /* NDR0F2: the render state and mix of the armed snapshot, at the triangle epoch (nd_geom.c gv_epoch_enter) */
    const unsigned sg = sc_cur.oml ^ (sc_cur.in.omode_h * 2654435761u) ^ sc_cur.geom ^ (unsigned)sc_cur.role;
    nd_snapchk_rnd();
    if (nd_sr_same(sg)) return;
    nd_render_apply(&sc_cur.rs, sc_cur.mixk, sc_cur.mixc, sc_cur.in.omode_h, sc_cur.oml, sg);
}
void nd_snapchk_tri(void) {   /* NDR0E2: a mesh triangle: the compare (switch snapchk), then the combine of the snap */
    nd_snap_arm = 0;
    if (PMDL("snapchk")) sc_compare();
    nd_snap_comb();
}
/* NDR1C: the combine of the armed snap (nd_rect.c, rectangles; nd_snapchk_tri for triangles). */
void nd_snap_comb(void) { nd_comb_apply(&sc_cur.o, sc_cur.in.w0, sc_cur.in.w1, sc_cur.in.use_t1, sc_cur.in.env); }
const NdSnapBind* nd_snap_cur_b(void) { return sc_cur.b; } /* NDR1C */
/* NDR1C: the render state log of a rectangle (drec runs): one line per frame that differs from the walker. */
void nd_snap_rlog(void) {
    static unsigned sc_rfr = ~0u;
    if (!pm_drec_now() || pm_frame_count() == sc_rfr) return;
    NdRender w; unsigned wc;
    const int wk = nd_snap_mix(nd_ws.omode_l, fogc, blendc, pm_nofog, &wc);
    nd_render_state(nd_ws.omode_h, nd_ws.omode_l, nd_img_role(0) == ROLE_DEPTH, &w);
    if (memcmp(&sc_cur.rs, &w, sizeof w) || sc_cur.mixk != wk || sc_cur.mixc != wc || sc_cur.role != nd_img_role(0) || nd_snap_bnew(NULL)) {
        sc_rfr = pm_frame_count();
        pm_log_d("rectsnap f%u", sc_rfr);
    }
}
void nd_snapchk_rect(void) { nd_snap_arm = 0; if (PMDL("snapchk")) sc_compare(); }   /* NDR0E2: a texture rectangle: compare only */
/* NCONV1D1: core 0 compares each list range with the walker at its first triangle (nd_lsr_tri); the counters go to the lsnap line. */
unsigned char nd_ls_on; unsigned nd_ls_ord; static const uint32_t* lr_b; static unsigned char lr_p1; static unsigned lr_cur, lr_nx, lr_lists, lr_snaps, lr_cmp, lr_c1, lr_tmis, lr_bst;
void nd_lsr_begin(const uint32_t* b) { lr_b = b; nd_ls_on = b != NULL; nd_ls_ord = 0; lr_cur = ~0u; lr_nx = 0; if (b) { lr_lists++; lr_snaps += b[0]; } }
void nd_lsr_end(void) { if (nd_ls_on && nd_ls_ord != lr_b[1]) lr_tmis++; nd_ls_on = 0; }
void nd_lsr_seg(unsigned ord) { lr_p1 = 1; nd_lsr_tri(ord); lr_p1 = 0; }
void nd_lsr_tri(unsigned ord) {
    if (ord < lr_nx) return;
    const unsigned r = nd_snap_lsr(lr_b, ord), n = lr_b[0];
    lr_nx = r + 1u < n ? lr_b[2u + r + 1u] : ~0u;
    if (r == lr_cur) return;
    lr_cur = r;
    memcpy(&sc_cur, (const char*)lr_b + ND_LS_HDR(n) + r * sizeof sc_cur, sizeof sc_cur);
    nd_snap_arm = 1; lr_cmp++; lr_c1 += lr_p1; /* NCONV1D2: the next draw step consumes the range snap (nd_snapchk_tri) */
}
void nd_lsr_bst(void) { if (PMDL("snapchk") && ep_epoch == gv_epoch && nd_snap_bnew(nd_ep_bs)) lr_bst++; } /* NCONV1D2: a list triangle whose epoch binds other tile words than its range snap */
const NdCombOut* nd_snap_dir_o(void) { return nd_snap_live ? &sc_cur.o : NULL; } /* NDR2D: the decision of the live snap */

/* NDR0A: the 100-frame counter line, once per frame stream end (nd_rec.c stream_run NDC_END); behind PMDL. */
void nd_snapchk_tick(void) {
    if (!PMDL("snapchk")) return;
    if (pm_frame_n / 100 != sc_bucket) {
        sc_bucket = pm_frame_n / 100;
        pm_log_evt("snapchk f%u recs %u cmp %u bad %u fbad %u miss %u wst %u", pm_frame_n, sc_recs, sc_cmp, sc_bad, sc_fbad, sc_miss, sc_wst);
        pm_log_evt("lsnap f%u lists %u snaps %u cmp %u p1 %u bst %u tmis %u over %u", pm_frame_n, lr_lists, lr_snaps, lr_cmp, lr_c1, lr_bst, lr_tmis, ls_over);
    }
}
/* NDR4: the snap of a sprite record (pm_gpu_spr_build reads it). core 0. */
static unsigned sc_sfr = ~0u;
int nd_snap_spr(uint32_t o[9]) { /* NDR4: env prim omh oml geom cw0 cw1 mixk mixc of the live snap; 0 when none */
    if (!nd_snap_live) return 0;
    o[0] = sc_cur.in.env; o[1] = sc_cur.in.prim; o[2] = sc_cur.in.omode_h; o[3] = sc_cur.oml; o[4] = sc_cur.geom;
    o[5] = sc_cur.in.w0; o[6] = sc_cur.in.w1; o[7] = (uint32_t)sc_cur.mixk; o[8] = sc_cur.mixc;
    if (pm_drec_now() && pm_frame_count() != sc_sfr && (o[0] != nd_ws.envc || o[1] != nd_ws.prim || o[2] != nd_ws.omode_h || o[3] != nd_ws.omode_l ||
        o[4] != nd_ws.geom || o[5] != nd_ws.comb_w0 || o[6] != nd_ws.comb_w1 || sc_cur.fogc != fogc || sc_cur.blendc != blendc)) { sc_sfr = pm_frame_count(); pm_log_d("sprsnap f%u", sc_sfr); }
    return 1;
}
