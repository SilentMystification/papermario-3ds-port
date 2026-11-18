/* nd_dschk.c: the dschk debug switch (NST3, design_nstate.md section 5). Core 1 records, before each command, the NdDs copy and the
 * ep bit (nd_rec.c rec_take, PMDL("dschk")). This runs on core 0 at that command: it fills an NdDs from nd_ws and the setter globals,
 * compares the two (nd_ds_cmp, draw-time fields zeroed), and compares the ep bit with nd_st_epoch (raised by the state ops). Logs:
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
/* NST2F: core 0 takes the state of a direct draw (nd_direct_bind, nd_direct_end, nd_glyph_draw): the draw writes the core 0 state at draw time
 * and its fallback runs the units, so core 0 reads the core 1 copy from here on. Clears box_folded (the copy is unfolded). */
static void dir_adopt(const NdDs* c) {
    nd_ws.omode_h = c->w.omode_h; nd_ws.omode_l = c->w.omode_l; nd_ws.om_kh = c->w.om_kh; nd_ws.om_kl = c->w.om_kl;
    nd_ws.comb_w0 = c->w.comb_w0; nd_ws.comb_w1 = c->w.comb_w1;
    nd_ws.tst1 = c->w.tst1; nd_ws.use_t1 = c->w.use_t1;
    memcpy(nd_ws.td, c->w.td, sizeof nd_ws.td); /* NDR0B: the texture descriptors */
    nd_ws.prim = c->w.prim; nd_ws.prim_lod = c->w.prim_lod; nd_ws.envc = c->w.envc;
    nd_ws.timg = c->w.timg; nd_ws.tlut = c->w.tlut; nd_ws.tlut_n = c->w.tlut_n;
    nd_tm = c->tm;
    comb_set = c->comb_set; box_prod = c->box_prod; comb_alpha_shade = c->comb_alpha_shade;
    dual_on = c->dual_on;
    box_folded = 0;
}
/* NST2F: counts one adopt and the fields core 0 differs by before it adopts (the fill is core 0 as it stands). */
static void adopt_count(const NdDs* c) {
    NdDs m;
    dschk_fill(&m);
    dk_adiff += (unsigned)nd_ds_cmp(c, &m, NULL, 0);
    dk_adopt++;
}
/* rec: the NdDs copy of core 1, then its ep word at word sizeof(NdDs) / 4 (nd_rec.c ds_rec). */
void nd_dschk_run(const uint32_t* rec) {
    if (nd_ds_rpl) return; /* NST2D: core 1 took this copy inside a range */
    const NdDs* theirs = (const NdDs*)(const void*)rec;
    const unsigned ep = rec[sizeof(NdDs) / 4u];
    const char* bad[8];
    char names[192];
    NdDs mine;
    if (nd_dschk_hit) { adopt_count(theirs); hit_adopt(theirs); nd_dschk_hit = 0; }
    if (nd_dschk_dir) { adopt_count(theirs); dir_adopt(theirs); nd_dschk_dir = 0; }
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
    if ((nd_st_epoch != 0) != (ep != 0)) dk_epbad++;
    nd_st_epoch = 0;
    if (pm_frame_n / 100 != dk_bucket) {
        dk_bucket = pm_frame_n / 100;
        pm_log_evt("dschk f%u cmds %u bad %u epbad %u fbad %u adopt %u adiff %u", pm_frame_n, dk_cmds, dk_bad, dk_epbad, dk_fbad, dk_adopt, dk_adiff);
    }
}

/* pm_fxtest.c: the end line at fxtest done (dschk switch only). */
void nd_dschk_end(void) {
    if (!pm_debug_has("dschk")) return;
    pm_log_evt("dschk end f%u cmds %u bad %u epbad %u fbad %u adopt %u adiff %u", pm_frame_n, dk_cmds, dk_bad, dk_epbad, dk_fbad, dk_adopt, dk_adiff);
    pm_log_evt("dschk lg hw %u over %u", nd_ds_lg_hw, nd_ds_lg_over); /* NST2D */
}
