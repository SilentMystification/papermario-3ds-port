/* LS-3: the model producer record (G_PM_REC, M4-2 / S3F-2 / S3F-4): rec_run and its record ops, moved out of the former pm_gbi.c unchanged
 * in source order. Target-only: the native model path (pm_gpu_*) and the walker handlers of nd_int.h. */
#include <string.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_int.h"
#include "nd_op.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
#include "nd_fs.h"
#include "nd_msnapw.h"
#include "nd_snap.h" /* NWS4C: nd_ap */

void nd_mdl_alt(const uint32_t* ab); /* NAT5B: nd_walk.c */
void nd_range_run(const uint32_t* a); /* nd_walk.c: DIRECT D10a, a custom gfx range of the frame stream */
void pm_gpu_set_r13_t1(int bound);
/* M4-2 / S3F-2 / S3F-4: producer record of a model (model.c appendGfx_model, TARGET_3DS), G_PM_REC_WORDS (11) words, R0 to R10. R0: w0 = G_PM_REC << 24 |
 * render mode index << 16 | (texture list index & 0x7f) << 8 (count byte 0); w1 = the bind list. R1: w0 G_MTX flags, w1 header w1. R2: w0 numLights,
 * w1 finalMtx. R3: w0 flags (bit 1 fog, 2 prim, 4 env, 8 blend, 16 shroud render mode, 32 shroud, 64 depth fog, 256 IGNORE_MATRIX), w1 lights group.
 * R4: w0 fog colour word, w1 combine entry (0 when none). R5: w1 fog position word. R6: w0 prim, w1 env. R7: w0 blend, w1 shroud c2.
 * R8: billboard matrix op (0 when none). R9, R10: panner tile size ops (0 when none; typed by nd_rec_model). R13: the typed ops after the record.
 * rec_apply sets the walker state from them in the old word order: lights, bind list (nd_bind_run), combine (rec_op), render mode list
 * (the list runs as G_DL), fog (rec_fog), matrix (nd_xf_load), panner tiles, billboard. rec_op runs one op through the skip test and handler of the walker. */
static unsigned rng_seen; /* NATRS: bit 0 pre, bit 1 post: the first run of each kind logs rngrun once (coverage proof) */
static void rec_range(const uint32_t* a, unsigned k) { /* NATRS: an R11 custom gfx range; the skip state reset of the old nd_range_run list start */
    nd_sr_forget(); /* NWS1 */
    if (__builtin_expect(!(rng_seen & k), 0)) { rng_seen |= k; pm_log_evt("rngrun f%u %s", pm_frame_count(), k == 1u ? "pre" : "post"); }
    nd_range_run(a);
}
/* S3N-6: the matrix ops of a record (R1, R2 load, R8 billboard), the first part of rec_apply. A cache hit runs them before it decides; a
 * miss after that skips them (mdone) and runs the rest of rec_apply in the old order. */
extern int nd_mtx_static; /* SEAM2 */
static __attribute__((noinline)) void rec_matrix(const NdW2* q) {
    u32 f = q[3].words.w0;
    if (!(f & 256u)) nd_xf_load(q[1].words.w0, q[2].words.w1, q[1].words.w1);
    else nd_mtx_static = 1;
    if (q[8].words.w0) nd_xf_load(q[8].words.w0, q[8].words.w1, q[1].words.w1);
}
static void hit_apply(const NdW2* q, const NdMEnt* e);
/* MCOPY1: the lights of a record (R2, R3), the movemem words of nd_ds_model in its order, run as nd_mvm_run runs them. */
static __attribute__((noinline)) void rec_lights(const NdW2* q) {
    if (!q[3].words.w1 || q[2].words.w0 > 7u) return;
    const unsigned n = q[2].words.w0 ? q[2].words.w0 : 1u;
    const Lightsn* L = (const Lightsn*)(uintptr_t)q[3].words.w1;
    nd_light_count(NUML(n));
    for (unsigned i = 1; i <= n + 1u; i++) {
        const u32 w0 = (((u32)sizeof(Light) - 1u) / 8u) << 19 | (3u * i + 3u) << 8 | G_MV_LIGHT;
        const u32 w1 = (u32)(uintptr_t)(i <= n ? (const void*)&L->l[i - 1u] : (const void*)&L->a);
        if (lt_mem_same(w0, w1)) continue;
        gv_epoch++; nd_mv_mem(w0, w1);
    }
}
static unsigned mm_fr = ~0u;
static __attribute__((cold, noinline)) void mmiss_note(void) { const unsigned f = pm_frame_count(); if (f != mm_fr) { mm_fr = f; pm_log_evt("mmiss f%u", f); } } /* the miss path takes the hit state: marker of the expected-diff gate */
static __attribute__((noinline)) void rec_apply(const NdW2* q, int mdone, const NdMEnt* e) {
    u32 f = q[3].words.w0; /* S3F-2b: the flags of R3 (see model.c appendGfx_model) */
    gv_epoch++; mmiss_note(); hit_apply(q, e); rec_lights(q); nd_sr_forget(); /* MCOPY1: the hit state, the lights; the tile blocks come from the model snap */
    if (!(f & 256u) && !mdone) nd_xf_load(q[1].words.w0, q[2].words.w1, q[1].words.w1); /* S3F-1a: the matrix load of the model, the same function as G_MTX (256: IGNORE_MATRIX) */
    else if (f & 256u) nd_mtx_static = 1; /* SEAM2: IGNORE_MATRIX, no model matrix */
    if (q[11].words.w0) rec_range((const uint32_t*)(uintptr_t)q[11].words.w0, 1u); /* S3G-1: custom gfx pre, after the matrix as the old order (D10a: a stream range) NATRS */
    if (q[8].words.w0 && !mdone) nd_xf_load(q[8].words.w0, q[8].words.w1, q[1].words.w1); /* S3F-2c: billboard matrix op (MUL, no push) */
}
/* S3F-4: the record is the whole state of the model (the walker model path is gone, no old words follow it). */
extern int pm_gpu_ndm_on(void); /* S3N-3b: switch ndmcheck */
extern void pm_gpu_ndm_begin(const uint32_t* rec, const uint32_t* modew, const uint32_t* combw, const void* gfx, unsigned texidx, const void* geo, const NdMKey* mk);
extern void pm_gpu_model_key(const uint32_t* rec, NdMKey* k); /* S3N-6 */
extern void pm_gpu_ndm_end(void);
extern int pm_gpu_model_native(const uint32_t* rec, const uint32_t* modew, const uint32_t* combw, const void* gfx, unsigned texidx, const void* geo, const NdMKey* mk); /* S3N-4: the switch */
extern int pm_gpu_model_hit(const NdMKey* k, NdMEnt* e);
extern int nd_r11_state(uint32_t addr, const uint32_t** nr, uint32_t* cw, int* has_cb, uint32_t* mo, unsigned* nmo); /* MDL-5f; POND: nr */
int rec_nat_last; /* WCEN2: 1 when the last rec_run drew natively */
extern int mfr_on; extern unsigned mfr_idx; void pm_gpu_mfr_rec(uint32_t geo, int hit, int nat, unsigned idx, const NdMKey* mk); /* TIK19 debug (switch mfr) */
/* HC6 step 1 (plan_hotcode 5.4 premise 1): a hit leaves the scalar walker state a miss leaves (rec_apply, in its op order). Before, a hit kept the
 * walker state of before the record (omode, combine, prim, env, fog, blend, texture scale, fog factors), a miss the record's: the state after NDC_MODEL
 * depended on hit or miss. The copy writes values only (no GPU call): the hit's native draws set their own GPU state, and sc_ok 0 and nd_sr_forget
 * make the walker send its combine and render state again. The texture filter bits of othermode H stay (set_om sets the filter on a change of them:
 * the GPU filter is the walker's). Tile, load, light and matrix state is not scalar and not copied.
 * HC6 step 3: the change is made once per cache entry (hit_delta, cold, at its first hit; nd_mc_hd) and applied by hd_apply. A record with more R13
 * units than the key hashes (aux_hash: 8) makes it at every hit. */
static void hd_op(NdHitD* d, const uint32_t* t) {
    const uint32_t a = t[0] & 0xffffffu, b = t[1];
    switch (t[0] >> 24) {
    case ND_FS_OMH: { const uint32_t m = nd_op_om_mask(a), mv = m & ~(3u << G_MDSFT_TEXTFILT); d->omh_v = (d->omh_v & ~mv) | (b & mv); d->omh_m |= mv; d->kh |= m; break; }
    case ND_FS_OML: { const uint32_t m = nd_op_om_mask(a); d->oml_v = (d->oml_v & ~m) | (b & m); d->oml_m |= m; d->kl |= m; break; }
    case ND_FS_GEOM: d->g_and &= a; d->g_or = (d->g_or & a) | b; break;
    case ND_FS_PRIM: d->prim = b; d->plod = a & 0xffffu; d->fl |= HD_PRIM; break;
    case ND_FS_ENV: d->env = b; d->fl |= HD_ENV; break;
    case ND_FS_FOG: d->fog = b; d->fl |= HD_FOG; break;
    case ND_FS_BLEND: d->blend = b; d->fl |= HD_BLEND; break;
    case ND_FS_TEXTURE: d->tex = (uint8_t)a; d->tss = b; d->fl |= HD_TEX; break;
    case ND_FS_COMB: d->cw0 = ND_FS_COMB_W0(a); d->cw1 = b; d->fl |= HD_COMB; break;
    case ND_FS_MW: if ((a >> 16) == ND_FS_MW_FOG) { d->fm = (s16)(b >> 16); d->fo = (s16)b; d->fl |= HD_FOGF; } break;
    default: break;
    }
}
static void hd_run(NdHitD* d, const uint32_t* u, unsigned n) { for (unsigned k = 0; k < n; k++, u += 2) hd_op(d, u); }
static void hd_t(NdHitD* d, unsigned id, uint32_t a, uint32_t b) { const uint32_t t[2] = { ND_FS_W0(id, a), b }; hd_op(d, t); }
static __attribute__((cold, noinline)) void hit_delta(const NdW2* q, NdHitD* d) {
    const u32 f = q[3].words.w0;
    memset(d, 0, sizeof *d);
    d->g_and = 0xFFFFFFFFu;
    if (f & 4u) hd_t(d, ND_FS_PRIM, 0, q[6].words.w0);
    if (f & 8u) hd_t(d, ND_FS_ENV, 0, q[6].words.w1);
    if (f & 64u) { hd_t(d, ND_FS_FOG, 0, q[4].words.w0); hd_t(d, ND_FS_MW, ND_FS_MW_FOG << 16, q[5].words.w1); }
    if (q[13].words.w0) hd_run(d, (const uint32_t*)(q + G_PM_REC_WORDS), q[13].words.w0);
    else if (q->words.w1) { const uint32_t* u = (const uint32_t*)(uintptr_t)q->words.w1; for (; (u[0] >> 24) != ND_FS_END; u += 2) hd_op(d, u); }
    if (q[4].words.w1) hd_op(d, (const uint32_t*)(uintptr_t)q[4].words.w1);
    { const unsigned i = (q->words.w0 >> 16) & 0xffu; if (i < ND_RM_N) hd_run(d, nd_rm_tab[i], nd_rm_n[i]); }
    if (f & 32u) {
        hd_t(d, ND_FS_OML, ND_FS_A_RENDERMODE, (u32)(PM_RM_SHROUD) | q[7].words.w1);
        hd_t(d, ND_FS_FOG, 0, q[4].words.w0);
        if (f & 16u) hd_t(d, ND_FS_BLEND, 0, q[7].words.w0);
        hd_t(d, ND_FS_MW, ND_FS_MW_FOG << 16, q[5].words.w1);
    } else if (f & 2u) { hd_t(d, ND_FS_FOG, 0, q[4].words.w0); hd_t(d, ND_FS_MW, ND_FS_MW_FOG << 16, q[5].words.w1); }
    d->fl |= HD_OK;
}
/* HC6 census switch hitst (cold): per field, the hits whose old rule (keep the walker state) left a value other than the miss's. */
enum { HS_OMH, HS_OML, HS_GEOM, HS_PRIM, HS_ENV, HS_FOG, HS_BLEND, HS_TEX, HS_COMB, HS_FOGF, HS_N };
static unsigned hs_hits, hs_n[HS_N], hs_f0, hs_fl;
static __attribute__((cold, noinline)) void hitst_note(const NdHitD* d) {
    hs_hits++;
    hs_n[HS_OMH] += ((nd_ap->in.omode_h & ~d->omh_m) | d->omh_v) != nd_ap->in.omode_h; /* NWS4C */
    hs_n[HS_OML] += ((nd_ap->oml & ~d->oml_m) | d->oml_v) != nd_ap->oml; /* NWS4C */
    hs_n[HS_GEOM] += ((nd_ap->geom & d->g_and) | d->g_or) != nd_ap->geom;
    hs_n[HS_PRIM] += (d->fl & HD_PRIM) && (d->prim != nd_ap->in.prim || d->plod != nd_ap->plod); /* NWS4C */
    hs_n[HS_ENV] += (d->fl & HD_ENV) && d->env != nd_ap->in.env; /* NWS4C */
    hs_n[HS_FOG] += (d->fl & HD_FOG) && d->fog != nd_ap->fogc; /* NWS4C */
    hs_n[HS_BLEND] += (d->fl & HD_BLEND) && d->blend != nd_ap->blendc; /* NWS4C */
    hs_n[HS_TEX] += (d->fl & HD_TEX) && (d->tex != (uint8_t)nd_c0.tex_on || d->tss != nd_ap->tss);
    hs_n[HS_COMB] += (d->fl & HD_COMB) && (d->cw0 != nd_ap->in.w0 || d->cw1 != nd_ap->in.w1); /* NWS4C */
    hs_n[HS_FOGF] += (d->fl & HD_FOGF) && (d->fm != nd_c0.fog_fm || d->fo != nd_c0.fog_fo);
    const unsigned fr = pm_frame_count();
    if (fr != hs_fl) {
        hs_fl = fr;
        if (fr >= hs_f0 + 300u) {
            pm_log("hitst f%u frames %u hits %u stale omh %u oml %u geom %u prim %u env %u fog %u blend %u tex %u comb %u fogf %u", fr, fr - hs_f0, hs_hits,
                   hs_n[HS_OMH], hs_n[HS_OML], hs_n[HS_GEOM], hs_n[HS_PRIM], hs_n[HS_ENV], hs_n[HS_FOG], hs_n[HS_BLEND], hs_n[HS_TEX], hs_n[HS_COMB], hs_n[HS_FOGF]);
            hs_f0 = fr; hs_hits = 0;
            memset(hs_n, 0, sizeof hs_n);
        }
    }
}
/* the combine of a hit, as rec_top runs it (nd_unit_same COMB, nd_comb_set) */
static __attribute__((noinline)) void hd_comb(const NdHitD* d) {
    if (d->cw0 != nd_c0.comb_w0 || d->cw1 != nd_c0.comb_w1) { pm_gpu_flush(); nd_c0.comb_w0 = d->cw0; nd_c0.comb_w1 = d->cw1; } /* NWS4D: POND 3a flush and the memo */
}
static __attribute__((noinline)) void hd_rare(const NdHitD* d) {
    const unsigned fl = d->fl;
    if (fl & HD_PRIM) { nd_c0.prim = d->prim; nd_c0.prim_lod = d->plod; }
    if (fl & HD_ENV) nd_c0.envc = d->env;
    if (fl & HD_TEX) nd_c0.tex_on = d->tex;
    if (fl & HD_FOGF) { nd_c0.fog_fm = d->fm; nd_c0.fog_fo = d->fo; }
}
static void hit_apply(const NdW2* q, const NdMEnt* e) {
    NdHitD dl, * d = &dl;
    if (e && q[13].words.w0 <= 8u) { d = nd_mc_hd(e); if (!d->fl) hit_delta(q, d); }
    else hit_delta(q, d);
    if (__builtin_expect(PMDL("hitst"), 0)) hitst_note(d);
    if (d->fl & HD_COMB) hd_comb(d);
    nd_c0.omode_h = (nd_c0.omode_h & ~d->omh_m) | d->omh_v;
    if (d->fl & (HD_PRIM | HD_ENV | HD_FOG | HD_BLEND | HD_TEX | HD_FOGF)) hd_rare(d);
}
/* HC6 step 2 debug switch mkchk: per record, the entry key A + key B found against the entry the old compare (full key words) finds. */
NdMEnt* nd_mc_find_old(const NdMKey* k);
static unsigned mk_recs, mk_diff, mk_f0, mk_fl;
static __attribute__((cold, noinline)) void mkchk_note(const NdMKey* k, const NdMEnt* e) {
    mk_recs++;
    if (nd_mc_find_old(k) != e) mk_diff++;
    const unsigned fr = pm_frame_count();
    if (fr != mk_fl) {
        mk_fl = fr;
        if (fr >= mk_f0 + 300u) { pm_log("mkchk f%u frames %u recs %u diff %u", fr, fr - mk_f0, mk_recs, mk_diff); mk_f0 = fr; mk_recs = mk_diff = 0; }
    }
}
void pm_km_note(const uint32_t* rec, const NdMKey* k, int cls); /* KEYMISS census (pm_keymiss.c, switch keymiss) */
/* HC6 step 3: everything of a record but the hit: the miss (the full key, the diagnostics, rec_apply, the native draw or the walker geometry), the
 * warm budget skip, a failed hit (tried: the matrix ran and pm_gpu_model_hit said no) and the debug switches that need the full key (full0). */
static __attribute__((cold, noinline)) unsigned rec_miss(const NdW2* q, NdMKey* mkp, NdMEnt* mce, int full0, int tried, unsigned mi0) {
    int cached = mce != NULL;
    int full = full0;
    if (!cached && !full) { const uint32_t h_ = mkp->h; pm_gpu_model_key((const uint32_t*)q, mkp); mkp->h = h_; full = 1; } /* HC6 step 2: the miss path reads the full key */
    if (!cached) { pm_xf.mdl_miss++; if (!q[11].words.w0) { if (pm_xf.diag) pm_xf.wh[nd_mc_xdiag(mkp)]++; nd_mc_miss(mkp); extern void pm_gpu_model_miss_diag(const NdMKey* k); pm_gpu_model_miss_diag(mkp); } } /* S3N-7: the miss class (diagnostic); STARFX: a pre-list record is rebuilt (counted) but has no key to class */
    if (__builtin_expect(PMDL("keymiss"), 0) && q[12].words.w0 && (!cached || tried)) pm_km_note((const uint32_t*)q, mkp, cached ? 2 : q[11].words.w0 ? 1 : 0);
    int ndm = pm_gpu_ndm_on();
    int nat = 0, mdone = 0, hit = 0;
    /* MAPX-3: a miss over the warm budget of a covered frame (pm_xfer_skip) is run as an empty hit: its matrix ops, no state, no draw; it builds next frame */
    const int wskip = !cached && !ndm && pm_xf.on && q[12].words.w0 && !q[11].words.w0 && pm_xfer_skip(0);
    if (cached || wskip) { if (!tried) rec_matrix(q); mdone = 1; } /* S3N-6: a cached key: its matrix first, the hit test reads the matrix */
    /* MDL-5f: a record with a custom gfx pre list (state-only, drawn natively) never takes the hit path: rec_apply runs the pre list (its state op); the hit path skips rec_apply. The post list runs in both paths (rec_run). */
    if (wskip || (!tried && !ndm && mdone && q[12].words.w0 && !q[11].words.w0 && pm_gpu_model_hit(mkp, mce))) {
        /* S3N-6 hit: the record state is the cached one (rec_apply is not run; its matrix ops ran above). The sticky shadows are cleared as after a native draw.
         * POND (A1): a record with no geometry (q[12] 0, the water copy of model.c mdl_draw_hidden_panel_surface) never takes the hit path: its state is its whole effect, so rec_apply must run. */
        gv_epoch++;
        nat = 1; hit = !wskip;
        hit_apply(q, mce); /* HC6 step 1: the walker state a miss leaves */
        nd_sr_forget(); /* NWS1 */
    } else {
    if (!full) { const uint32_t h_ = mkp->h; pm_gpu_model_key((const uint32_t*)q, mkp); mkp->h = h_; } /* HC6 step 2: a cached key whose hit failed: the store reads the full key */
    rec_apply(q, mdone, mce);
    { /* MDL-1b v2: once per record after rec_aux, before the ndm and model branches. SBKW: every record, not only R13 texture-only ones: the walker's
       * tile 1 step (nd_tile_bind(1) of a plan that reads TEXEL1) runs for all records, so the native combine decision sees td[1].valid (sbk_56 water) */
        nd_msnap_dec((const uint32_t*)q); /* NAT2K: the decision words of this record (the walker set them in rec_apply) */
        const uint32_t* qw = (const uint32_t*)q; uint32_t cw_[4]; int cb_ = 0; int t1 = 0;
        if (!qw[26] || (!PMDL("r13off") && nd_r13_texonly(qw + NDMC_RW, qw[26], cw_, &cb_))) t1 = pm_gbi_bind_tile1(qw);
        pm_gpu_set_r13_t1(t1);
    }
    nd_msnap_use((const uint32_t*)q); /* NREC1B: model_draw reads this snap */
    if (q[13].words.w0 || q->words.w1 || q[9].words.w0 || q[10].words.w0) { nd_bsrc = nd_msnap_bq((const uint32_t*)q); nd_tile_bind(0); nd_bsrc = NULL; } /* MCOPY1: the uv origin of tile 0 (census b8) */
    /* MDL-5f: the walker order is record combine, render mode list, pre list (rec_apply), so a state-only pre list's SETCOMBINE and SETOTHERMODE
     * ops win over the record's words. The native draw gets them in the same order: its combine replaces cwp, the render mode list is copied
     * without its SETCOMBINE (when the pre list has one) and its SETOTHERMODE ops are followed by the pre list's ops, then ENDDL. */
    /* POND: a pre list with a nested model record (the mac_00 water copy, nd_model.c nd_r11_walk): that record set the draw state last, so the draw
     * takes its words (flags, prim, bind, R13 words, render mode list, combine); the geometry stays this record's. */
    const NdW2* qs = q;
    const uint32_t* mwp;
    const uint32_t* cwp;
    uint32_t mcomp[2 * 24], cwr[2];
    int hcb = 0; unsigned nmo = 0; uint32_t mo[8];
    if (q[11].words.w0) {
        const uint32_t* nr = NULL;
        nd_r11_state((uint32_t)q[11].words.w0, &nr, cwr, &hcb, mo, &nmo);
        if (nr) qs = (const NdW2*)nr;
    }
    mwp = nd_rm_tab[(qs->words.w0 >> 16) & (ND_RM_N - 1u)]; /* HC4b: typed render mode ops (nd_mode_compile) */
    cwp = qs[4].words.w1 ? (const uint32_t*)(uintptr_t)qs[4].words.w1 : NULL;
    if (hcb) cwp = cwr;
    /* MERLEE: a shroud record sets PM_RM_SHROUD after its render mode list (rec_apply order), so the native draw gets that word after the list ops too;
     * else the list's G_RM_PASS first cycle replaced it and the native models never darkened. */
    const int shr = (qs[3].words.w0 & 32u) != 0;
    if (hcb || nmo || shr) {
        unsigned k = 0;
        for (unsigned i = 0; i < 16u && (mwp[2 * i] >> 24) != ND_FS_END; i++) {
            if (hcb && (mwp[2 * i] >> 24) == ND_FS_COMB) continue; /* the pre list's combine replaces it */
            mcomp[2 * k] = mwp[2 * i]; mcomp[2 * k + 1] = mwp[2 * i + 1]; k++;
        }
        if (shr) { mcomp[2 * k] = ND_FS_W0(ND_FS_OML, ND_FS_A_RENDERMODE); mcomp[2 * k + 1] = (u32)(PM_RM_SHROUD) | qs[7].words.w1; k++; }
        for (unsigned j = 0; j < nmo; j++, k++) { mcomp[2 * k] = mo[2 * j]; mcomp[2 * k + 1] = mo[2 * j + 1]; }
        mcomp[2 * k] = ND_FS_W0(ND_FS_END, 0); mcomp[2 * k + 1] = 0;
        mwp = mcomp;
    }
    if (ndm) pm_gpu_ndm_begin((const uint32_t*)qs, mwp,
        qs[4].words.w1 ? (const uint32_t*)(uintptr_t)qs[4].words.w1 : NULL, (const void*)(uintptr_t)qs->words.w1, (qs->words.w0 >> 8) & 0x7fu, (const void*)(uintptr_t)q[12].words.w0, mkp);
    else if (pm_gpu_model_native((const uint32_t*)qs, mwp, /* NATRS */
        cwp, (const void*)(uintptr_t)qs->words.w1, (qs->words.w0 >> 8) & 0x7fu, (const void*)(uintptr_t)q[12].words.w0, mkp) >= 0) {
        /* S3N-4: the record drew natively (nd_model_draw): no walker geometry. The native draws set the TEV and the texture unit, so the walker
         * sends its combine and render mode again at its next op (the shadows of those ops are cleared). */
        nat = 1;
        nd_sr_forget(); /* NWS1 */
    }
    }
    if (q[12].words.w0 == 0 && PMDL("waterlog") && pm_frame_count() >= 200u && pm_frame_count() <= 201u) { /* POND-M1: copy record with no geometry; hit taken or not, the combine after the decision */
        unsigned tss_, geo_, cw0_, cw1_; pm_gbi_sticky(&tss_, &geo_, &cw0_, &cw1_);
        pm_log("wrec f%u copy hit %d nat %d cw %08x %08x", pm_frame_count(), hit, nat, cw0_, cw1_);
    }
    if (q[11].words.w1) rec_range((const uint32_t*)(uintptr_t)q[11].words.w1, 2u); /* S3G-1: custom gfx post (D10a: a stream range) NATRS */
    if (q[12].words.w1) nd_xf_popb(64u);            /* S3G-1: gSPPopMatrix(G_MTX_MODELVIEW), one matrix */
    if (ndm) pm_gpu_ndm_end();
    rec_nat_last = nat;
    if (__builtin_expect(mfr_on, 0)) pm_gpu_mfr_rec((uint32_t)q[12].words.w0, hit, nat, mfr_idx - mi0, mkp);
    return G_PM_REC_WORDS - 1 + q[13].words.w0; /* R1 to R12: matrix, lights, fog, combine, prim, env, blend, shroud, billboard, panner, custom gfx, geometry, pop; then the R13 words */
}
/* HC6 step 3: the record path. A hit (an entry of key A + key B, its matrix, pm_gpu_model_hit) runs here; everything else in rec_miss (cold). */
unsigned rec_run(const NdW2* q) {
    const unsigned mi0 = mfr_idx;
    NdMKey mk;
    /* HC6 step 2: the lookup takes key A (the payload, core 1) and key B (pm_gpu_model_keyb); the full key (the store, the miss diagnostics, ndm, mfr)
     * is built before rec_apply only when the hit is not taken (from the walker state before the record is applied, as before). */
    const int mkc = PMDL("mkchk"), full0 = mkc || mfr_on || pm_gpu_ndm_on();
    NDT_B(tk); /* PROF2: the key class (key A, key B and the record address memo) */
    if (__builtin_expect(full0, 0)) pm_gpu_model_key((const uint32_t*)q, &mk); /* S3N-6: the cache key, from the walker state before the record is applied */
    else pm_gpu_model_keyb((const uint32_t*)q, &mk);
    /* STARFX: a record with a custom gfx pre list never takes the hit path (MDL-5f, below), so it is not looked up and not stored (model_draw):
     * its list pointer is a key word that moves with the frame's display list, and every rebuild stored its draws again into the pool (the hos_01
     * glowing stars filled the 512-draw pool and cleared the whole cache about every 50 frames). */
    NdMEnt* mce = q[11].words.w0 ? NULL : nd_mc_find_rec(&mk); /* PERF1-4a: the record address memo, else hash and probe */
    NDT_E(tk, NDT_MKEY);
    if (__builtin_expect(mkc, 0) && !q[11].words.w0) mkchk_note(&mk, mce);
    /* POND (A1): a record with no geometry (q[12] 0) never takes the hit path: its state is its whole effect, so rec_apply must run. */
    if (full0 || !mce || !q[12].words.w0) return rec_miss(q, &mk, mce, full0, 0, mi0);
    NDT_B(tm); /* PROF2 */ rec_matrix(q); /* S3N-6: a cached key: its matrix first, the hit test reads the matrix */ NDT_E(tm, NDT_MMTX);
    NDT_B(th); const int hit_ = pm_gpu_model_hit(&mk, mce); NDT_E(th, NDT_MHIT); /* PROF2 */
    if (!hit_) return rec_miss(q, &mk, mce, 0, 1, mi0);
    /* S3N-6 hit: rec_apply is not run (its matrix ops ran above); the walker state is the miss's (hit_apply); the sticky shadows are cleared as after a native draw */
    gv_epoch++;
    NDT_B(tha); hit_apply(q, mce); NDT_E(tha, NDT_MHIT); /* PROF2 */
    nd_sr_forget(); /* NWS1 */
    if (q[11].words.w1) rec_range((const uint32_t*)(uintptr_t)q[11].words.w1, 2u); /* S3G-1: custom gfx post (D10a: a stream range) NATRS */
    if (q[12].words.w1) nd_xf_popb(64u);            /* S3G-1: gSPPopMatrix(G_MTX_MODELVIEW), one matrix */
    rec_nat_last = 1;
    return G_PM_REC_WORDS - 1 + q[13].words.w0;
}

