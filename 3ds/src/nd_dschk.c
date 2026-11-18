/* nd_dschk.c: the dschk debug switch (NST3, design_nstate.md section 5). Core 1 records, before each command, the NdDs copy and the
 * ep bit (nd_rec.c rec_take, PMDL("dschk")). This runs on core 0 at that command: it fills an NdDs from nd_ws and the setter globals,
 * compares the two (nd_ds_cmp, draw-time fields zeroed), and compares the ep bit with the gv_epoch move since the last compare (NST2I: every core 0 batch cut). Logs:
 * `dschk bad f<n> <names>` (the first 8 bad records), `dschk f<n> cmds <c> bad <b> epbad <e>` every 100 frames, `dschk end f<n> ...`. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_int.h"
#include "nd_ds.h"
#include "pm_port.h"

extern unsigned nd_st_epoch; /* nd_walk.c: the state epoch count, reset here at each command */
extern unsigned nd_st_why; /* nd_walk.c: the source of the first epoch raise (NST2H) */
extern unsigned gv_epoch; /* nd_geom.c: the batch cut count (NST2I) */
extern unsigned char nd_cmd_last; /* nd_rec.c: the op of the last command core 0 ran (NST2I) */
static unsigned dk_gv = 1; /* NST2I: gv_epoch at the last compare */
extern unsigned pm_frame_n;  /* pm_main.c: the logic step counter */
extern unsigned conv_k4, keyc, keys; /* nd_comb.c, nd_set.c: the core 0 setter globals of NdDs */
extern int dual_on, box_prod, comb_set, comb_alpha_shade;
extern int sci_x0, sci_y0, sci_x1, sci_y1, fog_fm, fog_fo, fogmix_on;
extern unsigned fogc, blendc;
extern unsigned lt_n;      /* nd_mtx_set.c */
extern int8_t lk[2][3];
extern float vp_sx, vp_sy, vp_sz, vp_tx, vp_ty, vp_tz;
extern int box_folded;    /* nd_comb.c: the box fold state of core 0 */
extern unsigned box_save[4]; /* nd_comb.c: the unfolded comb_a..comb_d while folded */

static unsigned dk_cmds, dk_bad, dk_epbad, dk_nlog, dk_bucket;
static unsigned dk_fbad; /* NST2A: the sum of the differing fields over all commands */
static unsigned dk_adopt, dk_adiff; /* NST2F: adopts (hit or direct) and the fields they changed in core 0 */
unsigned char nd_dschk_dir; /* NST2F: set by nd_direct_end and nd_glyph_draw; the next record is adopted (dir_adopt) */
static unsigned dk_sclbad, dk_sigbad, dk_sclog; /* NST2J: core 1 sc_lost and sc_dirty against core 0 (sc_ok, sc_sig) */

/* The core 0 side of NdDs: nd_ws, then the globals of the setters (the same values the NdDs fields hold on core 1). */
static void dschk_fill(NdDs* d) {
    memcpy(&d->w, &nd_ws, sizeof d->w);
    d->conv_k4 = conv_k4; d->keyc = keyc; d->keys = keys;
    d->dual_on = dual_on; d->box_prod = box_prod; d->comb_set = comb_set; d->comb_alpha_shade = comb_alpha_shade;
    d->sci_x0 = sci_x0; d->sci_y0 = sci_y0; d->sci_x1 = sci_x1; d->sci_y1 = sci_y1;
    d->fog_fm = fog_fm; d->fog_fo = fog_fo; d->fogmix_on = fogmix_on;
    d->fogc = fogc; d->blendc = blendc;
    memcpy(d->lt, lt, sizeof d->lt);
    d->lt_n = lt_n;
    memcpy(d->lk, lk, sizeof d->lk);
    d->vp_sx = vp_sx; d->vp_sy = vp_sy; d->vp_sz = vp_sz; d->vp_tx = vp_tx; d->vp_ty = vp_ty; d->vp_tz = vp_tz;
    d->tm = nd_tm;
    if (box_folded) { /* NST2F: the unfold of box_fold_sync (nd_comb.c), core 1 holds these values */
        d->w.tst1.comb_a = box_save[0]; d->w.tst1.comb_b = box_save[1]; d->w.tst1.comb_c = box_save[2]; d->w.tst1.comb_d = box_save[3];
        d->w.tst1.comb_aa = 7; d->w.tst1.comb_ab = 7; d->w.tst1.comb_ac = 7; d->w.tst1.comb_ad = 2;
        d->w.use_t1 = 0;
    }
}

unsigned char nd_dschk_fb; /* NST2G: set after pm_gbi_fallback; the next record is adopted (fb_adopt) */
unsigned char nd_dschk_hit; /* NST2b: set by hit_apply (nd_recs.c) when core 0 took a hit */
/* NST2b: core 0 takes the state of the core 1 record after a hit (the hit path skips the lights, tile and load units, the panner sizes
 * and the texture filter bits). Copies the fields the hit path does not write. */
static void hit_adopt(const NdDs* c) {
    nd_ws.timg = c->w.timg; nd_ws.tlut = c->w.tlut; nd_ws.tlut_n = c->w.tlut_n;
    nd_ws.tst1 = c->w.tst1;
    memcpy(nd_ws.td, c->w.td, sizeof nd_ws.td); /* NDR0B: the texture descriptors */
    nd_ws.omode_h = (nd_ws.omode_h & ~(3u << G_MDSFT_TEXTFILT)) | (c->w.omode_h & (3u << G_MDSFT_TEXTFILT));
    nd_tm = c->tm;
    memcpy(lt, c->lt, sizeof lt);
    lt_n = c->lt_n;
    memcpy(lk, c->lk, sizeof lk);
    dual_on = c->dual_on;
}
/* NST2G: core 0 takes the fill and the mode words of core 1 after the boot fallback (its units run on core 0 only). */
static void fb_adopt(const NdDs* c) { nd_ws.omode_h = c->w.omode_h; nd_ws.om_kh = c->w.om_kh; nd_ws.fill = c->w.fill; }
/* NST2F: core 0 takes the state of a direct draw (nd_direct_bind, nd_direct_end, nd_glyph_draw): the draw writes the core 0 state at draw time
 * and its fallback runs the units, so core 0 reads the core 1 copy from here on. */
static void dir_adopt(const NdDs* c) {
    nd_ws.omode_h = c->w.omode_h; nd_ws.omode_l = c->w.omode_l; nd_ws.om_kh = c->w.om_kh; nd_ws.om_kl = c->w.om_kl;
    nd_ws.comb_w0 = c->w.comb_w0; nd_ws.comb_w1 = c->w.comb_w1;
    {   /* NST2G: a folded core 0 keeps its folded combine; box_save takes the core 1 (unfolded) values, so dschk_fill and the unfold give them */
        const NdTst k = nd_ws.tst1; const int ku = nd_ws.use_t1;
        nd_ws.tst1 = c->w.tst1; nd_ws.use_t1 = c->w.use_t1;
        if (box_folded) {
            box_save[0] = c->w.tst1.comb_a; box_save[1] = c->w.tst1.comb_b; box_save[2] = c->w.tst1.comb_c; box_save[3] = c->w.tst1.comb_d;
            nd_ws.tst1.comb_a = k.comb_a; nd_ws.tst1.comb_b = k.comb_b; nd_ws.tst1.comb_c = k.comb_c; nd_ws.tst1.comb_d = k.comb_d;
            nd_ws.tst1.comb_aa = k.comb_aa; nd_ws.tst1.comb_ab = k.comb_ab; nd_ws.tst1.comb_ac = k.comb_ac; nd_ws.tst1.comb_ad = k.comb_ad;
            nd_ws.use_t1 = ku;
        }
    }
    memcpy(nd_ws.td, c->w.td, sizeof nd_ws.td); /* NDR0B: the texture descriptors */
    nd_ws.prim = c->w.prim; nd_ws.prim_lod = c->w.prim_lod; nd_ws.envc = c->w.envc;
    nd_ws.timg = c->w.timg; nd_ws.tlut = c->w.tlut; nd_ws.tlut_n = c->w.tlut_n;
    nd_tm = c->tm;
    comb_set = c->comb_set; box_prod = c->box_prod; comb_alpha_shade = c->comb_alpha_shade;
    dual_on = c->dual_on;
}
/* NST2F: counts one adopt and the fields core 0 differs by before it adopts (the fill is core 0 as it stands). */
static void adopt_count(const NdDs* c) {
    NdDs m;
    dschk_fill(&m);
    dk_adiff += (unsigned)nd_ds_cmp(c, &m, NULL, 0);
    dk_adopt++;
}
static uint32_t ek_key[32], ek_n[32]; static unsigned ek_used, ek_other, ek_nlog;
static void ep_note(uint32_t key) {   /* NST2H: one epbad command, key = side << 16 | site or rule << 8 | unit id */
    if (ek_nlog < 16u) { ek_nlog++; pm_log_evt("dschk epbad f%u k %x", pm_frame_n, (unsigned)key); }
    for (unsigned i = 0; i < ek_used; i++) if (ek_key[i] == key) { ek_n[i]++; return; }
    if (ek_used < 32u) { ek_key[ek_used] = key; ek_n[ek_used++] = 1u; } else ek_other++;
}
static void ep_print(void) {   /* NST2H: the counts by key, 8 per line, then the overflow */
    char s[256]; unsigned k = 0; s[0] = 0;
    for (unsigned i = 0; i < ek_used; i++) {
        k += (unsigned)snprintf(s + k, sizeof s - k, " %x=%u", (unsigned)ek_key[i], (unsigned)ek_n[i]);
        if ((i & 7u) == 7u || i + 1u == ek_used) { pm_log_evt("dschk epsrc f%u%s", pm_frame_n, s); k = 0; s[0] = 0; }
    }
    pm_log_evt("dschk epsrc f%u other %u", pm_frame_n, ek_other);
}
/* rec: the NdDs copy of core 1, then its ep word at word sizeof(NdDs) / 4 (nd_rec.c ds_rec). */
void nd_dschk_run(const uint32_t* rec) {
    if (nd_ds_rpl) return; /* NST2D: core 1 took this copy inside a range */
    const NdDs* theirs = (const NdDs*)(const void*)rec;
    const unsigned epw = rec[sizeof(NdDs) / 4u], ep = epw & 1u;
    const char* bad[8];
    char names[192];
    NdDs mine;
    if (nd_dschk_hit) { adopt_count(theirs); hit_adopt(theirs); nd_dschk_hit = 0; }
    if (nd_dschk_dir) { adopt_count(theirs); dir_adopt(theirs); nd_dschk_dir = 0; }
    if (nd_dschk_fb) { adopt_count(theirs); fb_adopt(theirs); nd_dschk_fb = 0; }
    dschk_fill(&mine);
    dk_cmds++;
    const int nb = nd_ds_cmp(theirs, &mine, bad, 8);
    dk_fbad += (unsigned)nb;
    if (nb) {
        dk_bad++;
        if (dk_nlog < 8) {
            unsigned k = 0;
            names[0] = 0;
            for (int i = 0; i < nb && i < 8; i++) k += (unsigned)snprintf(names + k, sizeof names - k, " %s", bad[i]);
            dk_nlog++;
            pm_log_evt("dschk bad f%u%s", pm_frame_n, names);
        }
    }
    {   const int e0 = gv_epoch != dk_gv;   /* NST2I: every core 0 batch cut, not only the 4 nd_st_epoch sites */
        dk_gv = gv_epoch;
        if (e0 != (ep != 0)) {
            dk_epbad++;
            if (e0) ep_note(0x10000u | (nd_st_epoch ? (nd_st_why & 0xffffu) : (0xff00u | nd_cmd_last)));   /* site ff: the command op that ran */
            else ep_note(0x20000u | (((epw >> 16) & 0xffu) == 9u ? (0x900u | nd_cmd_last) : ((epw >> 8) & 0xffffu)));
        }
    }
    nd_st_epoch = 0; nd_st_why = 0;
    { const int l0 = !sc_ok, d0 = sc_ok && sc_sig != sc_sig_now(), l1 = theirs->sc_lost != 0, d1 = theirs->sc_dirty != 0; /* NST2J */
      if (l0 != l1 || (!l0 && d0 != d1)) { if (l0 != l1) dk_sclbad++; else dk_sigbad++;
          if (dk_sclog < 16u) { dk_sclog++; pm_log_evt("dschk scl f%u op %x c0 %d %d c1 %d %d", pm_frame_n, (unsigned)nd_cmd_last, l0, d0, l1, d1); } } }
    if (pm_frame_n / 100 != dk_bucket) {
        dk_bucket = pm_frame_n / 100;
        ep_print();
        pm_log_evt("dschk f%u cmds %u bad %u epbad %u fbad %u adopt %u adiff %u sclbad %u sigbad %u", pm_frame_n, dk_cmds, dk_bad, dk_epbad, dk_fbad, dk_adopt, dk_adiff, dk_sclbad, dk_sigbad);
    }
}

/* pm_fxtest.c: the end line at fxtest done (dschk switch only). */
void nd_dschk_end(void) {
    if (!pm_debug_has("dschk")) return;
    ep_print();
    pm_log_evt("dschk end f%u cmds %u bad %u epbad %u fbad %u adopt %u adiff %u sclbad %u sigbad %u", pm_frame_n, dk_cmds, dk_bad, dk_epbad, dk_fbad, dk_adopt, dk_adiff, dk_sclbad, dk_sigbad);
    pm_log_evt("dschk lg hw %u over %u", nd_ds_lg_hw, nd_ds_lg_over); /* NST2D */
}
