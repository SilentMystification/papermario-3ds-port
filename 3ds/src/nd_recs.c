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

unsigned rec_n; /* M4-2: records run in the current opsplit window */
void nd_range_run(const uint32_t* a); /* nd_walk.c: DIRECT D10a, a custom gfx range of the frame stream */
void pm_gpu_set_r13_t1(int bound);
/* M4-2 / S3F-2 / S3F-4: producer record of a model (model.c appendGfx_model, TARGET_3DS), G_PM_REC_WORDS (11) words, R0 to R10. R0: w0 = G_PM_REC << 24 |
 * render mode index << 16 | (texture list index & 0x7f) << 8 (count byte 0); w1 = the bind list. R1: w0 G_MTX flags, w1 header w1. R2: w0 numLights,
 * w1 finalMtx. R3: w0 flags (bit 1 fog, 2 prim, 4 env, 8 blend, 16 shroud render mode, 32 shroud, 64 depth fog, 256 IGNORE_MATRIX), w1 lights group.
 * R4: w0 fog colour word, w1 combine entry (0 when none). R5: w1 fog position word. R6: w0 prim, w1 env. R7: w0 blend, w1 shroud c2.
 * R8: billboard matrix op (0 when none). R9, R10: panner tile size ops (0 when none; typed by nd_rec_model). R13: the typed ops after the record.
 * rec_apply sets the walker state from them in the old word order: lights, bind list (nd_bind_run), combine (rec_op), render mode list
 * (the list runs as G_DL), fog (rec_fog), matrix (load_mtx), panner tiles, billboard. rec_op runs one op through the skip test and handler of the walker. */
/* HC4b: one typed record op through the skip test and the handler of the walker (nd_set.c). */
static __attribute__((noinline)) void rec_top(const uint32_t t[2]) {
    if (nd_op_same(t)) return;
    gv_epoch++; /* GV1b, as the walker */
    nd_op_apply(t);
}
static void rec_t(unsigned id, uint32_t a, uint32_t b) {
    const uint32_t t[2] = { NDO_W0(id, a), b };
    rec_top(t);
}
/* S3F-2: the fog of a record: the colour word, then the position word (the MOVEWORD header is built with the gbi macro, no division here). */
static void rec_fogpos(u32 pos) { rec_t(NDO_MW, NDO_MW_FOG << 16, pos); }
static __attribute__((noinline)) void rec_fog(u32 col, u32 pos) {
    rec_t(NDO_FOG, 0, col);
    rec_fogpos(pos);
}
/* HC4b (W3): the render mode list of index i as the typed ops of the render mode table (nd_rec.c, built by the recorder), run as the list ran
 * (nd_list_run: the list start rules, then the ops; dlc_try never took a render mode list: it is not map shape data). */
static __attribute__((noinline)) void rec_rm(unsigned i) {
    if (__builtin_expect(nd_lc_frame != -1, 0)) nd_lstcen(nd_rm_tab[i], 0u);
    nd_ops_run(nd_rm_tab[i], nd_rm_n[i], 0);
}
/* S3G-1: a model list of a record (the render mode list, the geometry): run as G_DL does (dlc_try first), else on the list executor
 * (nd_list_run, D10c) to its G_ENDDL. */
static __attribute__((noinline)) void rec_dl(const Gfx* l) {
    if (!l) return;
    if (!is_host((u32)(uintptr_t)l)) { gbi_cold_bad_next((u32)(uintptr_t)l, (u32)G_DL << 24, 0); return; }
    if (dlc_on && dlc_try(l)) { /* DLC1: a compiled list, as G_DL */
        if (pm_ph_cen) { os_dlc_hit(); mr0_dlc_hit = 1; }
        return;
    }
    if (__builtin_expect(nd_lc_frame != -1, 0)) nd_lstcen(l, 0u);
    nd_list_run(l);
}
/* S3G-2: the R13 ops after a record (HC4c: typed by nd_rec_model): n units run in place (D10b: a flat range on the op executor). */
static __attribute__((noinline)) void rec_aux(const Gfx* l, unsigned n) {
    nd_ops_run(l, n, 0);
}
/* S3N-6: the matrix ops of a record (R1, R2 load, R8 billboard), the first part of rec_apply. A cache hit runs them before it decides; a
 * miss after that skips them (mdone) and runs the rest of rec_apply in the old order. */
extern int nd_mtx_static; /* SEAM2 */
static __attribute__((noinline)) void rec_matrix(const Gfx* q) {
    u32 f = q[3].words.w0;
    if (!(f & 256u)) load_mtx(q[1].words.w0, q[2].words.w1, q[1].words.w1);
    else nd_mtx_static = 1;
    if (q[8].words.w0) load_mtx(q[8].words.w0, q[8].words.w1, q[1].words.w1);
}
static __attribute__((noinline)) void rec_apply(const Gfx* q, int mdone) {
    u32 f = q[3].words.w0; /* S3F-2b: the flags of R3 (see model.c appendGfx_model) */
    rec_n++;
    gv_epoch++; /* GV1b: the texture and render mode words it replaces counted here */
    if (f & 4u) rec_t(NDO_PRIM, 0, q[6].words.w0); /* depth and remap: the old words before the lights */
    if (f & 8u) rec_t(NDO_ENV, 0, q[6].words.w1);
    if (f & 64u) rec_fog(q[4].words.w0, q[5].words.w1);
    if (q[3].words.w1) rec_lights((const Lightsn*)(uintptr_t)q[3].words.w1, q[2].words.w0); /* S3F-1b: lights, as the old word order */
    u32 w0 = q->words.w0;
    const Gfx* tg = (const Gfx*)(uintptr_t)q->words.w1;
    if (q[13].words.w0) rec_aux(q + G_PM_REC_WORDS, q[13].words.w0); /* S3G-2: the typed R13 ops (special tile list, untextured ops) run in place of the bind */
    else if (tg) nd_bind_run((const uint32_t*)tg); /* HC4c: the typed bind list */
    if (q[4].words.w1) { /* S3F-2: the combine word, after the bind as in the old order */
        const Gfx* e = (const Gfx*)(uintptr_t)q[4].words.w1;
        rec_top((const uint32_t*)e); /* HC4e4: SolidCombineModes, AlphaTestCombineModes are typed (gs forms) */
    }
    rec_rm((w0 >> 16) & 0xffu); /* ST-4, HC4b: the render mode list (typed table) */
    if (f & 32u) { /* S3F-2b: shroud: render mode, fog colour, blend colour, fog position (the old order) */
        rec_t(NDO_OML, NDO_A_RENDERMODE, (u32)(PM_RM_SHROUD) | q[7].words.w1);
        rec_t(NDO_FOG, 0, q[4].words.w0);
        if (f & 16u) rec_t(NDO_BLEND, 0, q[7].words.w0);
        rec_fogpos(q[5].words.w1);
    } else if (f & 2u) {
        rec_fog(q[4].words.w0, q[5].words.w1); /* S3F-2: fog after the render mode, as the old word order */
    }
    if (!(f & 256u) && !mdone) load_mtx(q[1].words.w0, q[2].words.w1, q[1].words.w1); /* S3F-1a: the matrix load of the model, the same function as G_MTX (256: IGNORE_MATRIX) */
    else if (f & 256u) nd_mtx_static = 1; /* SEAM2: IGNORE_MATRIX, no model matrix */
    if (q[11].words.w0) nd_range_run((const uint32_t*)(uintptr_t)q[11].words.w0); /* S3G-1: custom gfx pre, after the matrix as the old order (D10a: a stream range) */
    if (q[9].words.w0) rec_top((const uint32_t*)&q[9]); /* S3F-2c: panner tile size, main then aux, as the old order (HC4c: typed by nd_rec_model) */
    if (q[10].words.w0) rec_top((const uint32_t*)&q[10]);
    if (q[8].words.w0 && !mdone) load_mtx(q[8].words.w0, q[8].words.w1, q[1].words.w1); /* S3F-2c: billboard matrix op (MUL, no push) */
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
static int nmn_sw(void) { static int v = -1; if (v < 0) v = pm_debug_has("nomodelnat") ? 1 : 0; return v; } /* MDL-5: switch nomodelnat, the walker geometry of every model record (the byte reference) */
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
    case NDO_OMH: { const uint32_t m = nd_op_om_mask(a), mv = m & ~(3u << G_MDSFT_TEXTFILT); d->omh_v = (d->omh_v & ~mv) | (b & mv); d->omh_m |= mv; d->kh |= m; break; }
    case NDO_OML: { const uint32_t m = nd_op_om_mask(a); d->oml_v = (d->oml_v & ~m) | (b & m); d->oml_m |= m; d->kl |= m; break; }
    case NDO_GEOM: d->g_and &= a; d->g_or = (d->g_or & a) | b; break;
    case NDO_PRIM: d->prim = b; d->plod = a & 0xffffu; d->fl |= HD_PRIM; break;
    case NDO_ENV: d->env = b; d->fl |= HD_ENV; break;
    case NDO_FOG: d->fog = b; d->fl |= HD_FOG; break;
    case NDO_BLEND: d->blend = b; d->fl |= HD_BLEND; break;
    case NDO_TEXTURE: d->tex = (uint8_t)a; d->tss = b; d->fl |= HD_TEX; break;
    case NDO_COMB: d->cw0 = NDO_COMB_W0(a); d->cw1 = b; d->fl |= HD_COMB; break;
    case NDO_MW: if ((a >> 16) == NDO_MW_FOG) { d->fm = (s16)(b >> 16); d->fo = (s16)b; d->fl |= HD_FOGF; } break;
    default: break;
    }
}
static void hd_run(NdHitD* d, const uint32_t* u, unsigned n) { for (unsigned k = 0; k < n; k++, u += 2) hd_op(d, u); }
static void hd_t(NdHitD* d, unsigned id, uint32_t a, uint32_t b) { const uint32_t t[2] = { NDO_W0(id, a), b }; hd_op(d, t); }
static __attribute__((cold, noinline)) void hit_delta(const Gfx* q, NdHitD* d) {
    const u32 f = q[3].words.w0;
    memset(d, 0, sizeof *d);
    d->g_and = 0xFFFFFFFFu;
    if (f & 4u) hd_t(d, NDO_PRIM, 0, q[6].words.w0);
    if (f & 8u) hd_t(d, NDO_ENV, 0, q[6].words.w1);
    if (f & 64u) { hd_t(d, NDO_FOG, 0, q[4].words.w0); hd_t(d, NDO_MW, NDO_MW_FOG << 16, q[5].words.w1); }
    if (q[13].words.w0) hd_run(d, (const uint32_t*)(q + G_PM_REC_WORDS), q[13].words.w0);
    else if (q->words.w1) { const uint32_t* u = (const uint32_t*)(uintptr_t)q->words.w1; for (; (u[0] >> 24) != NDO_END; u += 2) hd_op(d, u); }
    if (q[4].words.w1) hd_op(d, (const uint32_t*)(uintptr_t)q[4].words.w1);
    { const unsigned i = (q->words.w0 >> 16) & 0xffu; hd_run(d, nd_rm_tab[i], nd_rm_n[i]); }
    if (f & 32u) {
        hd_t(d, NDO_OML, NDO_A_RENDERMODE, (u32)(PM_RM_SHROUD) | q[7].words.w1);
        hd_t(d, NDO_FOG, 0, q[4].words.w0);
        if (f & 16u) hd_t(d, NDO_BLEND, 0, q[7].words.w0);
        hd_t(d, NDO_MW, NDO_MW_FOG << 16, q[5].words.w1);
    } else if (f & 2u) { hd_t(d, NDO_FOG, 0, q[4].words.w0); hd_t(d, NDO_MW, NDO_MW_FOG << 16, q[5].words.w1); }
    d->fl |= HD_OK;
}
/* HC6 census switch hitst (cold): per field, the hits whose old rule (keep the walker state) left a value other than the miss's. */
enum { HS_OMH, HS_OML, HS_GEOM, HS_PRIM, HS_ENV, HS_FOG, HS_BLEND, HS_TEX, HS_COMB, HS_FOGF, HS_N };
static unsigned hs_hits, hs_n[HS_N], hs_f0, hs_fl;
static __attribute__((cold, noinline)) void hitst_note(const NdHitD* d) {
    hs_hits++;
    hs_n[HS_OMH] += ((nd_ws.omode_h & ~d->omh_m) | d->omh_v) != nd_ws.omode_h;
    hs_n[HS_OML] += ((nd_ws.omode_l & ~d->oml_m) | d->oml_v) != nd_ws.omode_l;
    hs_n[HS_GEOM] += ((nd_ws.geom & d->g_and) | d->g_or) != nd_ws.geom;
    hs_n[HS_PRIM] += (d->fl & HD_PRIM) && (d->prim != nd_ws.prim || d->plod != nd_ws.prim_lod);
    hs_n[HS_ENV] += (d->fl & HD_ENV) && d->env != nd_ws.envc;
    hs_n[HS_FOG] += (d->fl & HD_FOG) && d->fog != fogc;
    hs_n[HS_BLEND] += (d->fl & HD_BLEND) && d->blend != blendc;
    hs_n[HS_TEX] += (d->fl & HD_TEX) && (d->tex != (uint8_t)nd_ws.tex_on || d->tss != ((uint32_t)nd_ws.tex_ss << 16 | (nd_ws.tex_ts & 0xffffu)));
    hs_n[HS_COMB] += (d->fl & HD_COMB) && (d->cw0 != nd_ws.comb_w0 || d->cw1 != nd_ws.comb_w1);
    hs_n[HS_FOGF] += (d->fl & HD_FOGF) && (d->fm != fog_fm || d->fo != fog_fo);
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
/* the combine of a hit, as rec_top runs it (nd_op_same COMB, op_setcombine) */
static __attribute__((noinline)) void hd_comb(const NdHitD* d) {
    if (!(comb_set && d->cw0 == nd_ws.comb_w0 && d->cw1 == nd_ws.comb_w1 && box_folded == 0 && cmb_gen_seen == comb_gen)) op_setcombine(d->cw0, d->cw1);
}
static __attribute__((noinline)) void hd_rare(const NdHitD* d) {
    const unsigned fl = d->fl;
    if (fl & HD_PRIM) { nd_ws.prim = d->prim; nd_ws.prim_lod = d->plod; }
    if (fl & HD_ENV) nd_ws.envc = d->env;
    if (fl & HD_FOG) fogc = d->fog;
    if (fl & HD_BLEND) blendc = d->blend;
    if (fl & HD_TEX) { nd_ws.tex_on = d->tex; nd_ws.tex_ss = d->tss >> 16; nd_ws.tex_ts = d->tss & 0xffffu; }
    if (fl & HD_FOGF) { fog_fm = d->fm; fog_fo = d->fo; }
}
static void hit_apply(const Gfx* q, const NdMEnt* e) {
    NdHitD dl, * d = &dl;
    if (e && q[13].words.w0 <= 8u) { d = nd_mc_hd(e); if (!d->fl) hit_delta(q, d); }
    else hit_delta(q, d);
    if (__builtin_expect(PMDL("hitst"), 0)) hitst_note(d);
    if (d->fl & HD_COMB) hd_comb(d);
    nd_ws.omode_h = (nd_ws.omode_h & ~d->omh_m) | d->omh_v; nd_ws.omode_l = (nd_ws.omode_l & ~d->oml_m) | d->oml_v;
    nd_ws.om_kh |= d->kh; nd_ws.om_kl |= d->kl; nd_ws.geom = (nd_ws.geom & d->g_and) | d->g_or;
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
static __attribute__((cold, noinline)) unsigned rec_miss(const Gfx* q, NdMKey* mkp, NdMEnt* mce, int full0, int tried, unsigned mi0) {
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
        rec_n++;
        gv_epoch++;
        nat = 1; hit = !wskip;
        hit_apply(q, mce); /* HC6 step 1: the walker state a miss leaves */
        sc_ok = 0; nd_sr_forget();
    } else {
    if (!full) { const uint32_t h_ = mkp->h; pm_gpu_model_key((const uint32_t*)q, mkp); mkp->h = h_; } /* HC6 step 2: a cached key whose hit failed: the store reads the full key */
    rec_apply(q, mdone);
    { /* MDL-1b v2: once per record after rec_aux, before the ndm and model branches. SBKW: every record, not only R13 texture-only ones: the walker's
       * tile 1 step (ensure_tile(1) of a plan that reads TEXEL1) runs for all records, so the native combine decision sees td[1].valid (sbk_56 water) */
        const uint32_t* qw = (const uint32_t*)q; uint32_t cw_[4]; int cb_ = 0; int t1 = 0;
        if (!qw[26] || (!PMDL("r13off") && nd_r13_texonly(qw + NDMC_RW, qw[26], cw_, &cb_))) t1 = pm_gbi_bind_tile1();
        pm_gpu_set_r13_t1(t1);
    }
    /* MDL-5f: the walker order is record combine, render mode list, pre list (rec_apply), so a state-only pre list's SETCOMBINE and SETOTHERMODE
     * ops win over the record's words. The native draw gets them in the same order: its combine replaces cwp, the render mode list is copied
     * without its SETCOMBINE (when the pre list has one) and its SETOTHERMODE ops are followed by the pre list's ops, then ENDDL. */
    /* POND: a pre list with a nested model record (the mac_00 water copy, nd_model.c nd_r11_walk): that record set the draw state last, so the draw
     * takes its words (flags, prim, bind, R13 words, render mode list, combine); the geometry stays this record's. */
    const Gfx* qs = q;
    const uint32_t* mwp;
    const uint32_t* cwp;
    uint32_t mcomp[2 * 24], cwr[2];
    int hcb = 0; unsigned nmo = 0; uint32_t mo[8];
    if (q[11].words.w0) {
        const uint32_t* nr = NULL;
        nd_r11_state((uint32_t)q[11].words.w0, &nr, cwr, &hcb, mo, &nmo);
        if (nr) qs = (const Gfx*)nr;
    }
    mwp = nd_rm_tab[(qs->words.w0 >> 16) & 0xffu]; /* HC4b: typed render mode ops (nd_mode_compile) */
    cwp = qs[4].words.w1 ? (const uint32_t*)(uintptr_t)qs[4].words.w1 : NULL;
    if (hcb) cwp = cwr;
    /* MERLEE: a shroud record sets PM_RM_SHROUD after its render mode list (rec_apply order), so the native draw gets that word after the list ops too;
     * else the list's G_RM_PASS first cycle replaced it and the native models never darkened. */
    const int shr = (qs[3].words.w0 & 32u) != 0;
    if (hcb || nmo || shr) {
        unsigned k = 0;
        for (unsigned i = 0; i < 16u && (mwp[2 * i] >> 24) != NDO_END; i++) {
            if (hcb && (mwp[2 * i] >> 24) == NDO_COMB) continue; /* the pre list's combine replaces it */
            mcomp[2 * k] = mwp[2 * i]; mcomp[2 * k + 1] = mwp[2 * i + 1]; k++;
        }
        if (shr) { mcomp[2 * k] = NDO_W0(NDO_OML, NDO_A_RENDERMODE); mcomp[2 * k + 1] = (u32)(PM_RM_SHROUD) | qs[7].words.w1; k++; }
        for (unsigned j = 0; j < nmo; j++, k++) { mcomp[2 * k] = mo[2 * j]; mcomp[2 * k + 1] = mo[2 * j + 1]; }
        mcomp[2 * k] = NDO_W0(NDO_END, 0); mcomp[2 * k + 1] = 0;
        mwp = mcomp;
    }
    if (ndm) pm_gpu_ndm_begin((const uint32_t*)qs, mwp,
        qs[4].words.w1 ? (const uint32_t*)(uintptr_t)qs[4].words.w1 : NULL, (const void*)(uintptr_t)qs->words.w1, (qs->words.w0 >> 8) & 0x7fu, (const void*)(uintptr_t)q[12].words.w0, mkp);
    else if (!nmn_sw() && pm_gpu_model_native((const uint32_t*)qs, mwp,
        cwp, (const void*)(uintptr_t)qs->words.w1, (qs->words.w0 >> 8) & 0x7fu, (const void*)(uintptr_t)q[12].words.w0, mkp) >= 0) {
        /* S3N-4: the record drew natively (nd_model_draw): no walker geometry. The native draws set the TEV and the texture unit, so the walker
         * sends its combine and render mode again at its next op (the shadows of those ops are cleared). */
        nat = 1;
        sc_ok = 0; nd_sr_forget();
    }
    }
    if (q[12].words.w0 == 0 && PMDL("waterlog") && pm_frame_count() >= 200u && pm_frame_count() <= 201u) { /* POND-M1: copy record with no geometry; hit taken or not, the combine after the decision */
        unsigned tss_, geo_, cw0_, cw1_; pm_gbi_sticky(&tss_, &geo_, &cw0_, &cw1_);
        pm_log("wrec f%u copy hit %d nat %d cw %08x %08x", pm_frame_count(), hit, nat, cw0_, cw1_);
    }
    if (!nat && (ndm || nmn_sw())) rec_dl((const Gfx*)(uintptr_t)q[12].words.w0); /* S3G-1: geometry (0 when none) */
    if (q[11].words.w1) nd_range_run((const uint32_t*)(uintptr_t)q[11].words.w1); /* S3G-1: custom gfx post (D10a: a stream range) */
    if (q[12].words.w1) gbi_popmtx(64u);            /* S3G-1: gSPPopMatrix(G_MTX_MODELVIEW), one matrix */
    if (ndm) pm_gpu_ndm_end();
    rec_nat_last = nat;
    if (__builtin_expect(mfr_on, 0)) pm_gpu_mfr_rec((uint32_t)q[12].words.w0, hit, nat, mfr_idx - mi0, mkp);
    return G_PM_REC_WORDS - 1 + q[13].words.w0; /* R1 to R12: matrix, lights, fog, combine, prim, env, blend, shroud, billboard, panner, custom gfx, geometry, pop; then the R13 words */
}
/* HC6 step 3: the record path. A hit (an entry of key A + key B, its matrix, pm_gpu_model_hit) runs here; everything else in rec_miss (cold). */
unsigned rec_run(const Gfx* q) {
    const unsigned mi0 = mfr_idx;
    NdMKey mk;
    /* HC6 step 2: the lookup takes key A (the payload, core 1) and key B (pm_gpu_model_keyb); the full key (the store, the miss diagnostics, ndm, mfr)
     * is built before rec_apply only when the hit is not taken (from the walker state before the record is applied, as before). */
    const int mkc = PMDL("mkchk"), full0 = mkc || mfr_on || pm_gpu_ndm_on();
    if (__builtin_expect(full0, 0)) pm_gpu_model_key((const uint32_t*)q, &mk); /* S3N-6: the cache key, from the walker state before the record is applied */
    else pm_gpu_model_keyb((const uint32_t*)q, &mk);
    /* STARFX: a record with a custom gfx pre list never takes the hit path (MDL-5f, below), so it is not looked up and not stored (model_draw):
     * its list pointer is a key word that moves with the frame's display list, and every rebuild stored its draws again into the pool (the hos_01
     * glowing stars filled the 512-draw pool and cleared the whole cache about every 50 frames). */
    NdMEnt* mce = q[11].words.w0 ? NULL : nd_mc_find_rec(&mk); /* PERF1-4a: the record address memo, else hash and probe */
    if (__builtin_expect(mkc, 0) && !q[11].words.w0) mkchk_note(&mk, mce);
    /* POND (A1): a record with no geometry (q[12] 0) never takes the hit path: its state is its whole effect, so rec_apply must run. */
    if (full0 || !mce || !q[12].words.w0) return rec_miss(q, &mk, mce, full0, 0, mi0);
    rec_matrix(q); /* S3N-6: a cached key: its matrix first, the hit test reads the matrix */
    if (!pm_gpu_model_hit(&mk, mce)) return rec_miss(q, &mk, mce, 0, 1, mi0);
    /* S3N-6 hit: rec_apply is not run (its matrix ops ran above); the walker state is the miss's (hit_apply); the sticky shadows are cleared as after a native draw */
    rec_n++;
    gv_epoch++;
    hit_apply(q, mce);
    sc_ok = 0; nd_sr_forget();
    if (q[11].words.w1) nd_range_run((const uint32_t*)(uintptr_t)q[11].words.w1); /* S3G-1: custom gfx post (D10a: a stream range) */
    if (q[12].words.w1) gbi_popmtx(64u);            /* S3G-1: gSPPopMatrix(G_MTX_MODELVIEW), one matrix */
    rec_nat_last = 1;
    return G_PM_REC_WORDS - 1 + q[13].words.w0;
}

