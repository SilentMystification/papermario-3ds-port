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

static unsigned dk_cmds, dk_bad, dk_epbad, dk_nlog, dk_bucket;
static unsigned dk_fbad; /* NST2A: the sum of the differing fields over all commands */

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
}

/* rec: the NdDs copy of core 1, then its ep word at word sizeof(NdDs) / 4 (nd_rec.c ds_rec). */
void nd_dschk_run(const uint32_t* rec) {
    const NdDs* theirs = (const NdDs*)(const void*)rec;
    const unsigned ep = rec[sizeof(NdDs) / 4u];
    const char* bad[8];
    char names[192];
    NdDs mine;
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
        pm_log_evt("dschk f%u cmds %u bad %u epbad %u fbad %u", pm_frame_n, dk_cmds, dk_bad, dk_epbad, dk_fbad);
    }
}

/* pm_fxtest.c: the end line at fxtest done (dschk switch only). */
void nd_dschk_end(void) {
    if (!pm_debug_has("dschk")) return;
    pm_log_evt("dschk end f%u cmds %u bad %u epbad %u fbad %u", pm_frame_n, dk_cmds, dk_bad, dk_epbad, dk_fbad);
}
