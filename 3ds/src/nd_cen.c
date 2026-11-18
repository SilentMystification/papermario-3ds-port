/* nd_cen.c: the frame end hooks of the walker census (plan_s10_path group K, WK-2). DELCEN: the op, producer, model and
 * split census is deleted; this file keeps the frame end, the flush class and the streak timers. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"
#include "nd_fs.h"

/* PG0 / PERF-GBI counters (fed in prof mode, read by the census below every GP_PERIOD frames) */
unsigned gp_same[8], gp_set[8]; /* PG0: state op calls and calls that set the value it already had: 0 combine, 1 prim, 2 env, 3 tile 0, 4 tile size, 5 sync_combine calls, 6 sync_render calls */
__attribute__((cold, noinline)) void gp_note(int k, int same) { gp_set[k]++; if (same) gp_same[k]++; }
/* PERF-GBI (prof mode): cycle laps inside draw_tri, a tick read costs 1.3 us in Azahar so the totals are inflated, the ranking holds. One load and one branch when off. */
unsigned gp_acc[8], gp_last, gp_ntri, gp_frames;
unsigned gp_opt[256], gp_opn[256], gp_opt_last; int gp_prev_op = -1; /* time between two ops, charged to the first */
#define GP_PERIOD 300u
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"

/* GLOB: definitions moved here from the former pm_gbi.c (layout only). */
unsigned pm_gbi_sync_ticks, pm_gbi_gv_ticks, pm_gbi_tex_ticks;
int rm_frame;
u8 seen[256];
char stats_line[96];
unsigned op_cnt[256];
int n_cut, n_keep, sample_x, have_sample;
void note(unsigned op) {
    unsigned n = ++op_cnt[op];
    seen[op] = 1;
}

unsigned n_ops; /* PT01: display list ops run this frame */

/* NDREF1 switch ndref=<frame>: the frame whose draw stream pm_gpu.c writes (sdmc:/3ds/PaperMario/ndref.bin). -1 when the switch is off. */
extern int ndref_on;
void ndref_tag(unsigned kind, unsigned id, unsigned extra);
int pm_gbi_ndref_frame(void) {
    char v[16];
    return pm_debug_value("ndref", v, sizeof v) ? atoi(v) : -1;
}

void pm_gbi_end_frame(void) {
    /* One second of presented frames, summed. Identical seconds are not written. */
    static int frames, acc_t, acc_i, acc_c, acc_d, acc_o, acc_k, acc_x;
    static char prev[112];
    snprintf(stats_line, sizeof stats_line, "t%d i%d c%d d%d o%d k%d x%d sp%u sl%d gr%u",
             n_cmd, n_in, n_cut, n_drop, n_out, n_keep, have_sample ? sample_x : 0, n_splits, pm_gpu_off_count(), n_grabs);
    PH_ADD(C_CMD, n_ops);
    { static unsigned dlc_fr; if (dlc_on && ++dlc_fr >= 300) { dlc_fr = 0; dlc_report(); } }
    if (pm_ph_cen || pm_dcs_on) pm_gpu_dcsum_frame(rm_frame); /* DS0: dcsum runs without phase; it must clear pm_dc_c and pm_dc_us every frame */
    n_ops = 0;
    ep_flush_stats(); gv_epoch++; ep_open = 0;
    if (gp3_t0_pending) { gp3_t0_dead++; gp3_t0_pending = 0; }
    PH_ADD(C_TRI, n_cmd);
    PH_ADD(C_TRIOUT, n_in + n_cut);
    acc_t += n_cmd;
    acc_i += n_in;
    acc_c += n_cut;
    acc_d += n_drop;
    acc_o += n_out;
    acc_k += n_keep;
    if (have_sample) acc_x = sample_x;
    frames++;
    if (frames >= 30) {
        char line[112];
        snprintf(line, sizeof line, "t%d i%d c%d d%d o%d k%d x%d sp%u sl%d gr%u", acc_t, acc_i, acc_c, acc_d, acc_o, acc_k, acc_x, n_splits, pm_gpu_off_count(), n_grabs);
        if (strcmp(line, prev) != 0) {
            snprintf(prev, sizeof prev, "%s", line);
        }
        frames = acc_t = acc_i = acc_c = acc_d = acc_o = acc_k = 0;
    }
    if (gp_on() && ++gp_frames == GP_PERIOD) { /* PERF-GBI: per frame average in 0.1 ms units (ticks / 26811), tick reads inflate the sections */
        char b[200]; int n = 0;
        for (int k = 0; k < 8; k++) n += snprintf(b + n, sizeof b - n, "%s %u ", k == 3 ? "early" : k == 0 ? "A" : k == 1 ? "ens" : k == 2 ? "dual" : k == 4 ? "sync" : k == 5 ? "emit" : k == 6 ? "old" : "gvtx", gp_acc[k] / 26811 / 300);
        for (int r = 0; r < 20; r++) { /* the twenty most expensive op codes: per frame time in 0.1 ms and count */
            int best = -1; unsigned bt = 0;
            for (int k = 0; k < 256; k++) if (gp_opt[k] > bt) { bt = gp_opt[k]; best = k; }
            if (best < 0) break;
            gp_opt[best] = 0;
        }
        memset(gp_opt, 0, sizeof gp_opt); memset(gp_opn, 0, sizeof gp_opn);
        memset(gp_same, 0, sizeof gp_same); memset(gp_set, 0, sizeof gp_set);
        pm_gpu_gbiprof_report(GP_PERIOD);
        { extern void pm_debug_report(unsigned); pm_debug_report(GP_PERIOD); }
        { extern void pm_tex_prof_report(unsigned); pm_tex_prof_report(GP_PERIOD); }
        memset(gp_acc, 0, sizeof gp_acc); gp_ntri = gp_frames = 0;
    }
    nd_mv_sp = 0; /* PT70: the RSP matrix stack starts empty in every task. A push without pop in the message draw leaked one level per frame, then the 10 level cap dropped the pad push of the blue pipe and its pop left the wrong matrix (the sunk pipe showed up while a text box waited) */
    nd_twin_frame_end(); /* INTERPB1 */
    if (gm_n) memset(gm_idx, 0, 2u * PM_GM_N * sizeof gm_idx[0]);
    gm_n = 0; gm_last = GV_NONE; pm_gpu_gv_reset();
    memset(vert_ring, 0xff, sizeof vert_ring);
    memset(vert_gvf, 3, sizeof vert_gvf);
    n_cmd = n_in = n_cut = n_drop = n_out = n_keep = 0;
    n_splits = n_grabs = 0;
    have_sample = 0;
    rm_frame++;
}

const char* pm_gbi_stats(void) { return stats_line; }


/* ---- MV-9: frame census helpers, cut from the former pm_gbi.c (no change): flush class, phase streak timers, gp3 gate ---- */
/* PI2: batch class of the op that closes a batch. 0 TEX, 1 CC, 2 GEO, 3 RECT, 4 TRI, 5 other. */
int pm_gbi_flush_class(void) {
    switch (pm_gbi_cur_op) {
    case ND_FS_TIMG: case ND_FS_TILE: case ND_FS_TILESIZE: case ND_FS_LOADTILE: case ND_FS_LOADBLOCK: case ND_FS_LOADTLUT: case ND_FS_TEXTURE: return 0;
    case ND_FS_COMB: case ND_FS_PRIM: case ND_FS_ENV: case ND_FS_OML: case ND_FS_OMH: case ND_FS_FOG: return 1;
    case ND_FS_MTX: case ND_FS_POPMTX: case ND_FS_GEOM: case ND_FS_MW: case ND_FS_MOVEMEM: return 2;
    case ND_FS_RECT: return 3;
    case ND_FS_TRI1: case ND_FS_TRI2: return 4;
    default: return 5;
    }
}
/* W2: phase timers read the tick for one run in 8 and scale the sample by 8. A run is a streak of ops of one class. */
unsigned ph_seq;
int ph_pick(void) { return (++ph_seq & 7u) == 0; }
static int gv_run, gv_streak;
unsigned gv_streak_t0;
void gv_streak_flush(void) {
    gv_run = 0;
    if (!gv_streak) return;
    pm_gbi_gv_ticks += (pm_ticks() - gv_streak_t0) * 8u;
    gv_streak = 0;
}
void gv_streak_op(unsigned op) {
    if (op == ND_FS_TRI1 || op == ND_FS_TRI2) {
        if (pm_ph_cen && !gv_run) { gv_run = 1; if (ph_pick()) { gv_streak_t0 = pm_ticks(); gv_streak = 1; } }
    } else
        gv_streak_flush();
}
int tex_run, tex_streak;
unsigned tex_streak_t0;
void tex_streak_flush(void) {
    tex_run = 0;
    if (!tex_streak) return;
    pm_gbi_tex_ticks += (pm_ticks() - tex_streak_t0) * 8u;
    tex_streak = 0;
}
void tex_streak_op(unsigned op) {
    if (op == ND_FS_TIMG || op == ND_FS_TILE || op == ND_FS_TILESIZE || op == ND_FS_LOADTILE || op == ND_FS_LOADBLOCK || op == ND_FS_LOADTLUT || op == ND_FS_TEXTURE) {
        if (pm_ph_cen && !tex_run) { tex_run = 1; if (ph_pick()) { tex_streak_t0 = pm_ticks(); tex_streak = 1; } }
    } else
        tex_streak_flush();
}

/* GP3 gate: tile 0 G_SETTILESIZE with td[0] set vs no surviving tri or tile-0 texrect before the next one. */
unsigned gp3_t0_ld, gp3_t0_dead, gp3_t0_pending;
void gp3_t0_set(void) {
    if (gp3_t0_pending) gp3_t0_dead++;
    gp3_t0_ld++;
    gp3_t0_pending = 1;
}
void gp3_t0_surv(void) { gp3_t0_pending = 0; }
