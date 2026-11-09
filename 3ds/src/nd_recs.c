/* LS-3: the model producer record (G_PM_REC, M4-2 / S3F-2 / S3F-4): rec_run and its record ops, moved out of the former pm_gbi.c unchanged
 * in source order. Target-only: the native model path (pm_gpu_*) and the walker handlers of nd_int.h. */
#include <string.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"

unsigned rec_n; /* M4-2: records run in the current opsplit window */
void pm_gpu_set_r13_t1(int bound);
/* M4-2 / S3F-2 / S3F-4: producer record of a model (model.c appendGfx_model, TARGET_3DS), G_PM_REC_WORDS (11) words, R0 to R10. R0: w0 = G_PM_REC << 24 |
 * render mode index << 16 | (texture list index & 0x7f) << 8 (count byte 0); w1 = the bind list. R1: w0 G_MTX flags, w1 header w1. R2: w0 numLights,
 * w1 finalMtx. R3: w0 flags (bit 1 fog, 2 prim, 4 env, 8 blend, 16 shroud render mode, 32 shroud, 64 depth fog, 256 IGNORE_MATRIX), w1 lights group.
 * R4: w0 fog colour word, w1 combine entry (0 when none). R5: w1 fog position word. R6: w0 prim, w1 env. R7: w0 blend, w1 shroud c2.
 * R8: billboard matrix op (0 when none). R9, R10: panner tile size ops (0 when none).
 * rec_apply sets the walker state from them in the old word order: lights, bind list (tb_ops), combine (rec_op), render mode list
 * (the list runs as G_DL), fog (rec_fog), matrix (load_mtx), panner tiles, billboard. rec_op runs one op through the skip test and handler of the walker. */
extern Gfx* ModelRenderModes[];
/* S3F-2: one record op through the skip test and the handler of the walker (the ops the record writes). */
static __attribute__((noinline)) void rec_op(u32 w0, u32 w1) {
    unsigned op = w0 >> 24;
    if (g1_skip[op]) return;
    if (g2_same(op, w0, w1)) { g2_note(op, w0); return; }
    gv_epoch++; /* GV1b, as the walker */
    switch (op) {
    case G_MOVEWORD: gbi_moveword(w0, w1); break;
    case G_SETFOGCOLOR: set_fogcolor(w1); break;
    case G_SETBLENDCOLOR: set_blendcolor(w1); break;
    case G_SETENVCOLOR: gbi_setenv(w1); break;
    case G_SETPRIMCOLOR: case G_SETCOMBINE: case G_SETOTHERMODE_L: case G_SETOTHERMODE_H: case G_SETTILESIZE: tex_op(w0, w1); break;
    default: pm_log("rec: op %u is not a record op f%u", op, pm_frame_count()); break;
    }
}
/* S3F-2: the fog of a record: the colour word, then the position word (the MOVEWORD header is built with the gbi macro, no division here). */
static __attribute__((noinline)) void rec_fogpos(u32 pos) {
    Gfx t;
    gMoveWd(&t, G_MW_FOG, G_MWO_FOG, pos);
    rec_op(t.words.w0, t.words.w1);
}
static __attribute__((noinline)) void rec_fog(u32 col, u32 pos) {
    rec_op((u32)G_SETFOGCOLOR << 24, col);
    rec_fogpos(pos);
}
/* S3G-1: a model list of a record (custom gfx pre or post, geometry): run as G_DL does (dlc_try first). The list runs nested
 * (pm_gbi_run) and returns at its G_ENDDL, as the walker's G_DL push did. */
static __attribute__((noinline)) void rec_dl(const Gfx* l) {
    if (!l) return;
    if (!is_host((u32)(uintptr_t)l)) { gbi_cold_bad_next((u32)(uintptr_t)l, (u32)G_DL << 24, 0); return; }
    if (dlc_on && dlc_try(l)) { /* DLC1: a compiled list, as G_DL */
        if (pm_ph_cen) { os_dlc_hit(); mr0_dlc_hit = 1; }
        return;
    }
    pm_gbi_run((void*)l, 0);
}
/* S3G-2: the words after a record: n words run in place (no ENDDL needed, pm_gbi_run stops at its end pointer). */
static __attribute__((noinline)) void rec_aux(const Gfx* l, unsigned n) {
    pm_gbi_run((void*)l, n * 8u);
}
/* S3N-6: the matrix ops of a record (R1, R2 load, R8 billboard), the first part of rec_apply. A cache hit runs them before it decides; a
 * miss after that skips them (mdone) and runs the rest of rec_apply in the old order. */
static __attribute__((noinline)) void rec_matrix(const Gfx* q) {
    u32 f = q[3].words.w0;
    if (!(f & 256u)) load_mtx(q[1].words.w0, q[2].words.w1, q[1].words.w1);
    if (q[8].words.w0) load_mtx(q[8].words.w0, q[8].words.w1, q[1].words.w1);
}
static __attribute__((noinline)) void rec_apply(const Gfx* q, int mdone) {
    u32 f = q[3].words.w0; /* S3F-2b: the flags of R3 (see model.c appendGfx_model) */
    rec_n++;
    gv_epoch++; /* GV1b: the texture and render mode words it replaces counted here */
    if (f & 4u) rec_op((u32)G_SETPRIMCOLOR << 24, q[6].words.w0); /* depth and remap: the old words before the lights */
    if (f & 8u) rec_op((u32)G_SETENVCOLOR << 24, q[6].words.w1);
    if (f & 64u) rec_fog(q[4].words.w0, q[5].words.w1);
    if (q[3].words.w1) rec_lights((const Lightsn*)(uintptr_t)q[3].words.w1, q[2].words.w0); /* S3F-1b: lights, as the old word order */
    u32 w0 = q->words.w0;
    const Gfx* tg = (const Gfx*)(uintptr_t)q->words.w1;
    if (q[13].words.w0) rec_aux(q + G_PM_REC_WORDS, q[13].words.w0); /* S3G-2: the words after the record (special tile list, untextured ops) run in place of the bind */
    else if (tg) tb_ops(tg);
    if (q[4].words.w1) { /* S3F-2: the combine word, after the bind as in the old order */
        const Gfx* e = (const Gfx*)(uintptr_t)q[4].words.w1;
        rec_op(e->words.w0, e->words.w1);
    }
    rec_dl(ModelRenderModes[(w0 >> 16) & 0xffu]); /* ST-4: the list runs as G_DL (the walker handlers) */
    if (f & 32u) { /* S3F-2b: shroud: render mode word, fog colour, blend colour, fog position (the old order) */
        Gfx t;
        gDPSetRenderMode(&t, PM_RM_SHROUD, q[7].words.w1);
        rec_op(t.words.w0, t.words.w1);
        rec_op((u32)G_SETFOGCOLOR << 24, q[4].words.w0);
        if (f & 16u) rec_op((u32)G_SETBLENDCOLOR << 24, q[7].words.w0);
        rec_fogpos(q[5].words.w1);
    } else if (f & 2u) {
        rec_fog(q[4].words.w0, q[5].words.w1); /* S3F-2: fog after the render mode, as the old word order */
    }
    if (!(f & 256u) && !mdone) load_mtx(q[1].words.w0, q[2].words.w1, q[1].words.w1); /* S3F-1a: the matrix load of the model, the same function as G_MTX (256: IGNORE_MATRIX) */
    rec_dl((const Gfx*)(uintptr_t)q[11].words.w0); /* S3G-1: custom gfx pre, after the matrix as the old order */
    if (q[9].words.w0) rec_op(q[9].words.w0, q[9].words.w1); /* S3F-2c: panner tile size, main then aux, as the old order */
    if (q[10].words.w0) rec_op(q[10].words.w0, q[10].words.w1);
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
unsigned rec_run(const Gfx* q) {
    const unsigned mi0 = mfr_idx;
    NdMKey mk;
    pm_gpu_model_key((const uint32_t*)q, &mk); /* S3N-6: the cache key, from the walker state before the record is applied */
    NdMEnt* mce = nd_mc_find_rec(&mk); /* PERF1-4a: the record address memo, else hash and probe */
    int cached = mce != NULL;
    if (!cached) { nd_mc_miss(&mk); extern void pm_gpu_model_miss_diag(const NdMKey* k); pm_gpu_model_miss_diag(&mk); } /* S3N-7: the miss class (diagnostic) */
    int ndm = pm_gpu_ndm_on();
    int nat = 0, mdone = 0, hit = 0;
    if (cached) { rec_matrix(q); mdone = 1; } /* S3N-6: a cached key: its matrix first, the hit test reads the matrix */
    /* MDL-5f: a record with a custom gfx pre list (state-only, drawn natively) never takes the hit path: rec_apply runs the pre list (its state op); the hit path skips rec_apply. The post list runs in both paths (rec_run). */
    if (!ndm && mdone && q[12].words.w0 && !q[11].words.w0 && pm_gpu_model_hit(&mk, mce)) {
        /* S3N-6 hit: the record state is the cached one (rec_apply is not run; its matrix ops ran above). The sticky shadows are cleared as after a native draw.
         * POND (A1): a record with no geometry (q[12] 0, the water copy of model.c mdl_draw_hidden_panel_surface) never takes the hit path: its state is its whole effect, so rec_apply must run. */
        rec_n++;
        gv_epoch++;
        nat = 1; hit = 1;
        sc_ok = 0; nd_sr_forget();
    } else {
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
    mwp = (const uint32_t*)ModelRenderModes[(qs->words.w0 >> 16) & 0xffu];
    cwp = qs[4].words.w1 ? (const uint32_t*)(uintptr_t)qs[4].words.w1 : NULL;
    if (hcb) cwp = cwr;
    /* MERLEE: a shroud record sets PM_RM_SHROUD after its render mode list (rec_apply order), so the native draw gets that word after the list ops too;
     * else the list's G_RM_PASS first cycle replaced it and the native models never darkened. */
    const int shr = (qs[3].words.w0 & 32u) != 0;
    if (hcb || nmo || shr) {
        unsigned k = 0;
        for (unsigned i = 0; i < 16u && (mwp[2 * i] >> 24) != 0xdfu; i++) {
            if (hcb && (mwp[2 * i] >> 24) == 0xfcu) continue; /* G_SETCOMBINE: the pre list's combine replaces it */
            mcomp[2 * k] = mwp[2 * i]; mcomp[2 * k + 1] = mwp[2 * i + 1]; k++;
        }
        if (shr) { Gfx t; gDPSetRenderMode(&t, PM_RM_SHROUD, qs[7].words.w1); mcomp[2 * k] = t.words.w0; mcomp[2 * k + 1] = t.words.w1; k++; }
        for (unsigned j = 0; j < nmo; j++, k++) { mcomp[2 * k] = mo[2 * j]; mcomp[2 * k + 1] = mo[2 * j + 1]; }
        mcomp[2 * k] = 0xdfu << 24; mcomp[2 * k + 1] = 0;
        mwp = mcomp;
    }
    if (ndm) pm_gpu_ndm_begin((const uint32_t*)qs, mwp,
        qs[4].words.w1 ? (const uint32_t*)(uintptr_t)qs[4].words.w1 : NULL, (const void*)(uintptr_t)qs->words.w1, (qs->words.w0 >> 8) & 0x7fu, (const void*)(uintptr_t)q[12].words.w0, &mk);
    else if (!nmn_sw() && pm_gpu_model_native((const uint32_t*)qs, mwp,
        cwp, (const void*)(uintptr_t)qs->words.w1, (qs->words.w0 >> 8) & 0x7fu, (const void*)(uintptr_t)q[12].words.w0, &mk) >= 0) {
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
    rec_dl((const Gfx*)(uintptr_t)q[11].words.w1);  /* S3G-1: custom gfx post */
    if (q[12].words.w1) gbi_popmtx(64u);            /* S3G-1: gSPPopMatrix(G_MTX_MODELVIEW), one matrix */
    if (ndm) pm_gpu_ndm_end();
    rec_nat_last = nat;
    if (__builtin_expect(mfr_on, 0)) pm_gpu_mfr_rec((uint32_t)q[12].words.w0, hit, nat, mfr_idx - mi0, &mk);
    return G_PM_REC_WORDS - 1 + q[13].words.w0; /* R1 to R12: matrix, lights, fog, combine, prim, env, blend, shroud, billboard, panner, custom gfx, geometry, pop; then the R13 words */
}
