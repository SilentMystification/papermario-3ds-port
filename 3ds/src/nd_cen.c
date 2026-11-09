/* nd_cen.c: the census of the walker (plan_s10_path group K, WK-2). Cut from the former pm_gbi.c without logic change (D12): the op
 * census (os_*), the producer census (pk_*, pm_gbi_prod_mark), the model census (mdc_*), the window and frame end hooks.
 * Cold except os_op_hook and mdc_op, which the walk calls per op under phase. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"

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

/* W1: sampled time per op class. Phase on only. op_n is exact (one increment per op). One tick pair per 64 ops, outer list only.
 * P1: producer census. Marks come from pm_interp_open (gMainGfxPos at the open). The cursor moves forward at sp == 0 in the
 * main list. Each op counts to the producer under the cursor. Nested lists count to the calling producer. */
enum { OS_VTX, OS_MTX, OS_TRI, OS_TEX, OS_CC, OS_GEO, OS_DL, OS_DLC, OS_SYNC, OS_OTH, OS_N };
const char* const os_nm[OS_N] = { "vtx", "mtx", "tri", "tex", "cc", "geo", "dl", "dlc", "sync", "oth" };
#define OS_DLC_OP 0xFF /* pseudo op: a G_DL that dlc_try answered */
unsigned os_op_n[256], os_nn[256], os_tk[256], os_sn[256], os_fl[256];
unsigned os_fr, os_off, os_nest, os_open, os_op, os_pk, os_t0, os_fl0;
unsigned os_svc_last; /* CAP1: pm_svc_n at the last opsplit line (pt field); pm_gbi_win_reset sets it */
PkMark pmk[PK_MARKS]; /* the marks of the list being walked (copied at its walk start) */
int pmk_n, pmx_main, pmx_next;
PkMark pmk_all[PK_MARKS]; /* WCEN3: marks of all lists not yet walked; a frame start drops its context's old marks */
int pmk_all_n;
unsigned pmx_cur, pk_key[PK_SLOTS], pk_set[PK_SLOTS], pk_ops[PK_SLOTS], pk_tk[PK_SLOTS], pk_sn[PK_SLOTS];
unsigned pk_wd[PK_SLOTS], pk_nd[PK_SLOTS]; /* WCEN3: GPU draws per producer, walker (wd) and native (nd) */
#define CW_NW 16 /* CEN-1a: why codes (pm_gpu.c CW_*) */
unsigned pm_gpu_cen_why(void); const char* pm_gpu_cen_name(unsigned c);
unsigned pk_wdw[PK_SLOTS][CW_NW]; /* CEN-1a: walker draws per producer by why code */
unsigned pk_rec[PK_SLOTS], pk_nat[PK_SLOTS]; /* WCEN2: per producer, record headers run (pk_rec) and records drawn natively (pk_nat) */

unsigned pk_slot(unsigned a) {
    unsigned h = ((a * 2654435761u) >> 16) % PK_PRE;
    for (int i = 0; i < PK_PRE; i++, h = (h + 1) % PK_PRE) {
        if (!pk_set[h]) { pk_set[h] = 1; pk_key[h] = a; return h; }
        if (pk_key[h] == a) return h;
    }
    return PK_OTHER;
}
/* NDREF1 switch ndref=<frame>: the frame whose draw stream pm_gpu.c writes (sdmc:/3ds/PaperMario/ndref.bin). -1 when the switch is off. */
extern int ndref_on;
void ndref_tag(unsigned kind, unsigned id, unsigned extra);
int pm_gbi_ndref_frame(void) {
    char v[16];
    return pm_debug_value("ndref", v, sizeof v) ? atoi(v) : -1;
}
void pm_gbi_draw_note(int native) { /* WCEN3: one GPU draw of the walker (native 0) or a native draw, counted to the producer under the cursor (phase mode) */
    if (!pm_ph_cen) return;
    if (native) pk_nd[pmx_cur]++; else { pk_wd[pmx_cur]++; unsigned w_ = pm_gpu_cen_why(); pk_wdw[pmx_cur][w_ < CW_NW ? w_ : 0]++; }
}
void pm_gbi_prod_mark(unsigned a) { /* called by pm_interp_open under phase */
    const Gfx* p = gMainGfxPos;
    if (ndref_on) ndref_tag(2, a, 0); /* NDREF1: producer tag in the reference stream */
    if (pmk_all_n >= PK_MARKS) pmk_all_n = 0; /* DIRECT D9: no main list, the marks are not consumed by a main walk */
    if (pmk_all_n < PK_MARKS) { pmk_all[pmk_all_n].p = p; pmk_all[pmk_all_n].s = pk_slot(a); pmk_all_n++; }
}

static unsigned os_flsum(void) { unsigned s = 0; for (int i = 0; i < 6; i++) s += pm_fl_us[i]; return s; }
/* W1 calibration: one tick read costs about os_cal ticks, and each sample window holds one read. Measured at window start. */
static unsigned os_cal;
static void os_calibrate(void) {
    unsigned long long sum = 0;
    for (int i = 0; i < 16; i++) { unsigned a = pm_ticks(); unsigned b = pm_ticks(); sum += b - a; }
    os_cal = (unsigned)(sum / 16);
}
static unsigned os_win(unsigned t) { return t - os_t0 > os_cal ? t - os_t0 - os_cal : 0; }
void os_close(void) {
    unsigned t = pm_ticks();
    unsigned w = os_win(t);
    os_tk[os_op] += w; os_sn[os_op]++; os_fl[os_op] += os_flsum() - os_fl0;
    pk_tk[os_pk] += w; pk_sn[os_pk]++;
    os_open = 0;
}
void os_dlc_hit(void) { /* a G_DL answered by dlc_try: count it as the dlc pseudo op */
    unsigned* n = os_nest > 1 ? os_nn : os_op_n;
    n[G_DL]--; n[OS_DLC_OP]++;
    if (os_open && os_op == G_DL) os_op = OS_DLC_OP;
}
static int os_class_of(unsigned op) {
    switch (op) {
    case OS_DLC_OP: return OS_DLC;
    case G_VTX: case G_MODIFYVTX: return OS_VTX;
    case G_MTX: case G_POPMTX: return OS_MTX;
    case G_TRI1: case G_TRI2: return OS_TRI;
    case G_SETTIMG: case G_SETTILE: case G_SETTILESIZE: case G_LOADTILE: case G_LOADBLOCK: case G_LOADTLUT: case G_TEXTURE: return OS_TEX;
    case G_SETCOMBINE: case G_SETPRIMCOLOR: case G_SETENVCOLOR: case G_SETOTHERMODE_H: case G_SETOTHERMODE_L: case G_SETFOGCOLOR: return OS_CC;
    case G_GEOMETRYMODE: case G_MOVEWORD: case G_MOVEMEM: case G_SETSCISSOR: return OS_GEO;
    case G_DL: case G_ENDDL: return OS_DL;
    case G_RDPPIPESYNC: case G_RDPTILESYNC: case G_RDPLOADSYNC: case G_RDPFULLSYNC: case G_NOOP: return OS_SYNC;
    default: return OS_OTH;
    }
}
/* MV (a): the module that runs each op (the handler of its dispatch arm in nd_walk.c). Read by os_report as the mod column. Updated in each move row:
 * an arm that moves changes its entry. MOD_PMGBI counts the ops whose handler is still in the former pm_gbi.c; MOD_REC the G_PM_REC arm (rec_run, nd_recs.c). */
enum { MOD_WALK, MOD_SET, MOD_TEX, MOD_RECT, MOD_GEOM, MOD_MTX, MOD_TGT, MOD_ENT, MOD_PMGBI, MOD_REC, MOD_OTH, MOD_N };
const char* const mod_nm[MOD_N] = { "walk", "set", "tex", "rect", "geom", "mtx", "tgt", "ent", "pmgbi", "rec", "oth" };
static const uint8_t op_module[256] = {
    [0 ... 255] = MOD_OTH,
    [G_ENDDL] = MOD_WALK, [G_DL] = MOD_WALK, [G_NOOP] = MOD_WALK, [G_SPNOOP] = MOD_WALK, [G_SETZIMG] = MOD_WALK,
    [G_RDPFULLSYNC] = MOD_WALK, [G_RDPTILESYNC] = MOD_WALK, [G_RDPPIPESYNC] = MOD_WALK, [G_RDPLOADSYNC] = MOD_WALK,
    [G_RDPHALF_1] = MOD_WALK, [G_RDPHALF_2] = MOD_WALK, [G_PM_SPR] = MOD_WALK, [G_MODIFYVTX] = MOD_WALK,
    [G_SETCONVERT] = MOD_WALK, [G_SETKEYR] = MOD_WALK, [G_SETKEYGB] = MOD_WALK,
    [G_PM_REC] = MOD_REC, [G_PM_ENT] = MOD_ENT,
    [G_VTX] = MOD_GEOM, [G_TRI1] = MOD_GEOM, [G_TRI2] = MOD_GEOM,
    [G_MTX] = MOD_MTX, [G_MOVEMEM] = MOD_MTX, [G_POPMTX] = MOD_MTX,
    [G_SETSCISSOR] = MOD_SET, [G_GEOMETRYMODE] = MOD_SET, [G_MOVEWORD] = MOD_SET, [G_SETFOGCOLOR] = MOD_SET, [G_SETBLENDCOLOR] = MOD_SET,
    [G_SETENVCOLOR] = MOD_SET, [G_SETPRIMDEPTH] = MOD_SET, [G_SETFILLCOLOR] = MOD_SET,
    [G_TEXTURE] = MOD_TEX, [G_SETOTHERMODE_H] = MOD_TEX, [G_SETOTHERMODE_L] = MOD_TEX, [G_SETCOMBINE] = MOD_TEX, [G_SETPRIMCOLOR] = MOD_TEX,
    [G_SETTIMG] = MOD_TEX, [G_SETTILE] = MOD_TEX, [G_LOADBLOCK] = MOD_TEX, [G_LOADTILE] = MOD_TEX, [G_LOADTLUT] = MOD_TEX, [G_SETTILESIZE] = MOD_TEX,
    [G_FILLRECT] = MOD_RECT, [G_TEXRECT] = MOD_RECT, [G_TEXRECTFLIP] = MOD_RECT,
    [G_SETCIMG] = MOD_TGT,
};
/* P1 name: a label string when the key reads as printable text, else the hex key (a function address, resolved with the .map file). */
static void pk_name(unsigned s, char* b, int n) {
    if (s == PK_PRE) { snprintf(b, n, "pre"); return; }
    if (s == PK_BG) { snprintf(b, n, "bg"); return; }
    if (s == PK_OTHER) { snprintf(b, n, "other"); return; }
    unsigned a = pk_key[s];
    const char* p = (const char*)(uintptr_t)a;
    int ok = a && is_host(a) && a >= 0x00100000u;
    for (int i = 0; ok && i < 40; i++) { if (!p[i]) break; if (p[i] < 32 || p[i] > 126) ok = 0; else if (i == 39) ok = 0; }
    if (ok && p[0]) snprintf(b, n, "%.40s", p); else snprintf(b, n, "%08x", a);
}
/* FB-3 pm5draw (log only): the producer under the cursor, as pk_name names it */
void pm_gbi_cur_prod_name(char* b, int n) { pk_name(pmx_cur, b, n); }
void os_op_hook(const Gfx* at, int sp, unsigned op) { /* one call per op, phase on */
    if (os_nest == 1) {
        if (os_open) os_close();
        if (pmx_main && sp == 0) while (pmx_next < pmk_n && at >= pmk[pmx_next].p) pmx_cur = pmk[pmx_next++].s;
    }
    if (os_nest > 1) os_nn[op]++; /* inside a dlc replay: its time is in the outer G_DL sample, so it is not a class term */
    else os_op_n[op]++;
    pk_ops[pmx_cur]++;
    if (os_nest == 1 && ((n_ops + os_off) & 63u) == 0) { os_open = 1; os_op = op; os_pk = pmx_cur; os_fl0 = os_flsum(); os_t0 = pm_ticks(); }
}

/* MDL1: model census, phase on only, counts only. appendGfx_model marks the model ID at its entry (gMainGfxPos). The cursor
 * moves as the producer census does (main list, sp == 0). Draw = a run of tri ops (any other op ends the run). Static = the hash
 * of the G_MTX words of the model is equal to the hash of the last frame. Table 128 slots, mark list 1024 entries. */
#define MDC_MARKS 1024
#define MDC_SLOTS 128
typedef struct {
    u16 id;
    u8 used, pan, fog, has_prev;
    unsigned ops, draws, tris, binds, closes, tex_sum, frames, stat; /* window sums */
    unsigned seen, ntx, tx[8], prev_h, cur_h; /* frame state */
} mdc_t;
mdc_t mdc[MDC_SLOTS];
struct { const Gfx* p; int s; u8 plain; } mdcm[MDC_MARKS];
int mdcm_n, mdcm_next, mdcm_cur = -1, mdcm_over, mdc_prev_tri, mdcm_last = -1;
unsigned mdc_tot_ops, mdc_tot_draws, mdc_tot_tris, mdc_tot_binds, mdc_none;

static __attribute__((cold, noinline)) int mdc_slot(unsigned id) {
    unsigned h = (id * 2654435761u) >> 25;
    for (int i = 0; i < MDC_SLOTS; i++, h = (h + 1) & (MDC_SLOTS - 1)) {
        if (!mdc[h].used) { memset(&mdc[h], 0, sizeof mdc[h]); mdc[h].used = 1; mdc[h].id = (u16)id; mdc[h].cur_h = 2166136261u; return (int)h; }
        if (mdc[h].id == id) return (int)h;
    }
    return -1;
}
/* T2b: one pseudo ID per task function pointer (first-sight order, 32 entries, 0xF100 + index). A full table gives 0xF0FF. */
#define MDC_FNS 32
const void* mdc_fn[MDC_FNS];
int mdc_fn_n;
static __attribute__((cold, noinline)) unsigned mdc_fn_id(const void* fn) {
    for (int i = 0; i < mdc_fn_n; i++) if (mdc_fn[i] == fn) return 0xF100u + (unsigned)i;
    if (mdc_fn_n >= MDC_FNS) return 0xF0FFu;
    mdc_fn[mdc_fn_n] = fn;
    return 0xF100u + (unsigned)mdc_fn_n++;
}
unsigned pm_gbi_task_id(const void* fn) { return mdc_fn_id(fn); }
static const void* mdc_fn_of(unsigned id) { return id >= 0xF100u && id - 0xF100u < (unsigned)mdc_fn_n ? mdc_fn[id - 0xF100u] : NULL; }
/* T2b: a non-empty close (pm_gpu_flush_at) counts on the model or task the op stream is in. */
void pm_gbi_census_close(void) { int cur = pmx_main ? mdcm_cur : -1; if (cur >= 0) mdc[cur].closes++; }

void pm_gbi_mdl_mark(unsigned id, int pan) { /* called by appendGfx_model under phase */
    const Gfx* p = gMainGfxPos;
    if (ndref_on) ndref_tag(3, id, (unsigned)(pan != 0)); /* NDREF1: model tag in the reference stream */
    if (mdcm_n && p < mdcm[mdcm_n - 1].p) mdcm_n = 0; /* the main list starts again */
    int s = mdc_slot(id);
    if (s >= 0) { mdc[s].seen = 1; mdc[s].pan = (u8)(pan != 0); }
    else mdcm_over++;
    mdcm_last = -1;
    if (mdcm_n < MDC_MARKS) { mdcm[mdcm_n].p = p; mdcm[mdcm_n].s = s; mdcm[mdcm_n].plain = 0; mdcm_last = mdcm_n; mdcm_n++; }
    else mdcm_over++;
}

/* SPLIT0C: split of the walk by kind, phase only. A period has a kind: 0 rest, 1 texture op (tex_op, tb_ops), 2 replayed
 * list (dlc_try). A tick is read at the group begin and end and at each kind change, never per op (a tick read costs 1.3 us in
 * Azahar). sp0_T is in system ticks (268 per us). The sp0c line prints us per frame over the phase window (300 frames). */
unsigned sp0_t, sp0_cur;
unsigned long long sp0_T[3], sp0_F; /* SPLIT0C2: flush ticks inside the tex kind, a subset of sp0_T[1] */
static __attribute__((noinline)) void sp0_set(unsigned k) {
    unsigned n = pm_ticks();
    sp0_T[sp0_cur] += n - sp0_t;
    sp0_t = n; sp0_cur = k;
}
static inline void sp0_kind(unsigned k) { if (k != sp0_cur) sp0_set(k); }
static inline int sp0_tex(unsigned op) {
    switch (op) {
    case G_TEXTURE: case G_SETOTHERMODE_H: case G_SETOTHERMODE_L: case G_SETCOMBINE: case G_SETPRIMCOLOR:
    case G_SETTIMG: case G_SETTILE: case G_LOADBLOCK: case G_LOADTILE: case G_LOADTLUT: case G_SETTILESIZE:
        return 1;
    default: return 0;
    }
}
unsigned long long pm_gbi_sp0_dlc(void) { return sp0_T[2]; } /* PF2: read by pm_phase.c pf_take, dlc group ticks */
void pm_pmu_sp0(unsigned out[3]); /* pm_phase.c, CAP2 */
/* S3-2 measurement: ticks and calls of tex_op per op code, phase on only (line sp0x of sp0_report) */
unsigned long long tx_t[256], tx_f[256]; unsigned tx_n[256]; /* owner: the former pm_gbi.c (sp0 report); tex_op in nd_tex.c adds to them */
unsigned long long pm_gpu_fl_acc(void); /* pm_gpu.c */
static __attribute__((cold, noinline)) void sp0_report(void) {
    unsigned pmv[3];
    { /* the six tex ops with the most time: op code, us per frame, calls per frame (window 300 frames) */
        char b[160]; int n = 0;
        for (int r = 0; r < 6; r++) {
            int best = -1; unsigned long long bt = 0;
            for (int k = 0; k < 256; k++) if (tx_t[k] > bt) { bt = tx_t[k]; best = k; }
            if (best < 0) break;
            n += snprintf(b + n, sizeof b - n, "%02x %u+%u/%u ", best, (unsigned)(tx_t[best] / 268u / 300u), (unsigned)(tx_f[best] / 268u / 300u), tx_n[best] / 300u);
            tx_t[best] = 0;
        }
        memset(tx_t, 0, sizeof tx_t); memset(tx_f, 0, sizeof tx_f); memset(tx_n, 0, sizeof tx_n);
        pm_log_hw("sp0x f%d tex ops own+close us/calls per frame: %s", rm_frame, b);
    }
    pm_pmu_sp0(pmv); /* CAP2: one read per window, not per op */
    pm_log_hw("sp0c f%d tex %u dlc %u rest %u flush %u skip %u grp %u pmu_ic %u pmu_dc %u pmu_cyc %u", rm_frame,
              (unsigned)(sp0_T[1] / 268u / 300u), (unsigned)(sp0_T[2] / 268u / 300u), (unsigned)(sp0_T[0] / 268u / 300u), (unsigned)(sp0_F / 268u / 300u), sp0_skip, sp0_grp, pmv[0], pmv[1], pmv[2]);
    memset(sp0_T, 0, sizeof sp0_T); sp0_F = 0; sp0_skip = sp0_grp = 0;
}
/* MR0: plain static model census, phase on only, cold. A plain model has no fog change, no tint, no custom gfx, no panner and
 * no billboard (set by appendGfx_model through pm_gbi_mr0_plain). The walk cuts time into periods. A period has a kind: 0 ops,
 * 1 bind (texture op), 2 dlc (a G_DL that dlc_try answers). A tick ends a period when the kind changes or the model changes.
 * Flush time (os_flsum) is cut out of each kind. Ticks are cheap: one per bind or dlc boundary and one per model change. */
unsigned mr0_t, mr0_fl0, mr0_plain_n;
int mr0_kind, mr0_plain, mr0_dlc_hit;
unsigned long long mr0_T[3], mr0_F[3]; /* T in system ticks, F in us */
void pm_gbi_mr0_plain(int plain) { /* appendGfx_model, under phase, right after its mark */
    if (mdcm_last >= 0 && plain) { mdcm[mdcm_last].plain = 1; mr0_plain_n++; }
}
static __attribute__((cold, noinline)) void mr0_tick(int kind_new, int plain_new) {
    unsigned now = pm_ticks(), fl = os_flsum();
    if (mr0_plain) {
        int k = mr0_kind;
        if (k == 2 && !mr0_dlc_hit) k = 0; /* a G_DL that nested a list is ops, not dlc */
        mr0_T[k] += now - mr0_t;
        mr0_F[k] += fl >= mr0_fl0 ? fl - mr0_fl0 : fl; /* pm_fl_us is cleared by the phase report: a reset counts from 0 */
    }
    mr0_t = now; mr0_fl0 = fl; mr0_kind = kind_new; mr0_plain = plain_new; mr0_dlc_hit = 0;
}
static __attribute__((cold, noinline)) void mr0_op(unsigned op, int adv, int plain) {
    int kn = op == G_DL ? 2 : 0;
    if (!adv && kn == 0 && mr0_kind == 0) return;
    mr0_tick(kn, plain);
}
/* MS0: removable closes, phase only, an upper bound. A run is consecutive plain opaque model marks (non-model ops between them
 * are not checked). A stable sort by texture inside a run leaves one close per distinct texture: removable = changes + 1 - distinct. */
int ms0_in, ms0_last = -1, ms0_nd;
unsigned ms0_d[64];
unsigned long long ms0_rem, ms0_chg, ms0_ctot, ms0_opm, ms0_runs;
static __attribute__((cold, noinline)) void ms0_end(void) {
    if (!ms0_in) return;
    int r = (int)ms0_nd > 0 ? (int)ms0_chg + 1 - (int)ms0_nd : 0;
    if (r > 0) ms0_rem += (unsigned)r;
    ms0_ctot += ms0_chg;
    ms0_in = 0; ms0_nd = 0; ms0_chg = 0; ms0_last = -1;
}
void pm_gbi_ms0_mark(int opaque, unsigned tex) { /* appendGfx_model, under phase: opaque = plain and RENDER_MODE_SURFACE_OPA */
    if (!opaque) { ms0_end(); return; }
    ms0_opm++;
    if (!ms0_in) { ms0_in = 1; ms0_runs++; }
    unsigned j = 0;
    while ((int)j < ms0_nd && ms0_d[j] != tex) j++;
    if ((int)j == ms0_nd && ms0_nd < 64) ms0_d[ms0_nd++] = tex;
    if (ms0_last >= 0 && ms0_last != (int)tex) ms0_chg++;
    ms0_last = (int)tex;
}
/* the main walk ends: the time after it (logic, other producers) is not a model period */
void mr0_walk_end(void) {
    if (mr0_plain || mr0_kind) mr0_tick(0, 0);
}
void mdc_op(const Gfx* at, int sp, unsigned op, u32 w0, u32 w1) {
    int adv = 0;
    if (os_nest == 1 && pmx_main && sp == 0) {
        while (mdcm_next < mdcm_n && at >= mdcm[mdcm_next].p) { mdcm_cur = mdcm[mdcm_next++].s; mdc_prev_tri = 0; adv = 1; }
    }
    int cur = pmx_main ? mdcm_cur : -1;
    mr0_op(op, adv, cur >= 0 && mdcm[mdcm_next - 1].plain);
    unsigned is_tri = op == G_TRI1 || op == G_TRI2, was_tri = (unsigned)mdc_prev_tri, ntri = op == G_TRI2 ? 2u : 1u;
    mdc_prev_tri = (int)is_tri;
    mdc_tot_ops++;
    if (is_tri) { mdc_tot_tris += ntri; if (!was_tri) mdc_tot_draws++; }
    if (cur < 0) { mdc_none++; return; }
    mdc_t* m = &mdc[cur];
    m->ops++;
    if (is_tri) { m->tris += ntri; if (!was_tri) m->draws++; }
    switch (op) {
    case G_TRI1: case G_TRI2: break;
    case G_SETFOGCOLOR: m->fog = 1; break;
    case G_MTX: {
        if (!is_host((u32)(uintptr_t)ptr_of(w1))) break;
        u32 v[16];
        memcpy(v, ptr_of(w1), sizeof(v));
        unsigned h = m->cur_h ^ w0;
        for (int i = 0; i < 16; i++) h = (h ^ v[i]) * 16777619u;
        m->cur_h = h;
        break;
    }
    default: break;
    }
}
static __attribute__((cold, noinline)) void mdc_frame_end(void) {
    for (int s = 0; s < MDC_SLOTS; s++) {
        mdc_t* m = &mdc[s];
        if (!m->used) continue;
        if (m->seen) {
            m->frames++;
            m->tex_sum += m->ntx;
            if (m->has_prev && m->cur_h == m->prev_h) m->stat++;
            m->prev_h = m->cur_h;
            m->has_prev = 1;
        } else {
            m->has_prev = 0;
        }
        m->seen = 0; m->ntx = 0; m->cur_h = 2166136261u;
    }
    mdcm_n = 0; mdcm_next = 0; mdcm_cur = -1;
    ms0_end();
}
static __attribute__((cold, noinline)) void mdc_report(void) {
    unsigned long long tot_o = 0, tot_d = 0, tot_t = 0, tot_b = 0;
    unsigned mdl_n = 0, mdl_st = 0;
    static unsigned char picked[MDC_SLOTS];
    memset(picked, 0, sizeof picked);
    for (int s = 0; s < MDC_SLOTS; s++) {
        mdc_t* m = &mdc[s];
        if (!m->used || !m->frames) continue;
        mdl_n++;
        if (m->frames > 1 && (unsigned long long)m->stat * 10 >= (unsigned long long)(m->frames - 1) * 9) mdl_st++;
        tot_o += m->ops; tot_d += m->draws; tot_t += m->tris; tot_b += m->binds;
    }
    pm_log_hw("mdlsum f%d tot ops %u draws %u tris %u binds %u | models %u static %u | unattr ops %u over %u", rm_frame,
        (unsigned)(mdc_tot_ops / 300), (unsigned)(mdc_tot_draws / 300), (unsigned)(mdc_tot_tris / 300), (unsigned)(mdc_tot_binds / 300),
        mdl_n, mdl_st, (unsigned)(mdc_none / 300), mdcm_over);
    for (int r = 0; r < 12; r++) {
        int best = -1; unsigned bv = 0;
        for (int s = 0; s < MDC_SLOTS; s++) if (!picked[s] && mdc[s].used && mdc[s].ops > bv) { bv = mdc[s].ops; best = s; }
        if (best < 0) break;
        picked[best] = 1;
        mdc_t* m = &mdc[best];
        unsigned tx = m->frames ? m->tex_sum / m->frames : 0;
        unsigned key = m->id >= 0xF100u ? (unsigned)(uintptr_t)mdc_fn_of(m->id) : (unsigned)m->id;
        pm_log_hw("mdlsum f%d r%d %s %08x draws %u ops %u tris %u binds %u closes %u tex %u static %u/%u pan %u fog %u", rm_frame, r + 1,
            m->id >= 0xF100u ? "fn" : m->id >= 0xF000 ? "task" : "id", key,
            (unsigned)(m->draws / 300), (unsigned)(m->ops / 300), (unsigned)(m->tris / 300), (unsigned)(m->binds / 300), (unsigned)(m->closes / 300), tx,
            m->frames > 1 ? m->stat : 0u, m->frames > 1 ? m->frames - 1 : 0u, (unsigned)m->pan, (unsigned)m->fog);
    }
    for (int s = 0; s < MDC_SLOTS; s++) {
        mdc[s].ops = mdc[s].draws = mdc[s].tris = mdc[s].binds = mdc[s].closes = mdc[s].tex_sum = mdc[s].frames = mdc[s].stat = 0;
        mdc[s].fog = 0;
    }
    { /* MR0: plain model census, us per model. Ticks run at 268 MHz (TICKS_MS 268112). F is pm_fl_us: already x8 for the 1-in-8 sample. */
        unsigned long long n = mr0_plain_n ? mr0_plain_n : 1;
        unsigned long long ops = mr0_T[0] / 268u, bind = mr0_T[1] / 268u, dlc = mr0_T[2] / 268u;
        unsigned long long fo = mr0_F[0] + mr0_F[1] + mr0_F[2];
        unsigned long long o = ops > mr0_F[0] ? ops - mr0_F[0] : 0, b = bind > mr0_F[1] ? bind - mr0_F[1] : 0;
        unsigned long long d = dlc > mr0_F[2] ? dlc - mr0_F[2] : 0;
        pm_log_hw("mr0 f%d plain %u per frame | per model us: ops %u bind %u close %u outside dlc %u dlc %u | console x2.86 outside dlc %u",
            rm_frame, (unsigned)(mr0_plain_n / 300u), (unsigned)(o / n), (unsigned)(b / n), (unsigned)(fo / n),
            (unsigned)((o + b + fo) / n), (unsigned)(d / n), (unsigned)((o + b + fo) * 286u / 100u / n));
    }
    memset(mr0_T, 0, sizeof mr0_T); memset(mr0_F, 0, sizeof mr0_F); mr0_plain_n = 0;
    pm_log_hw("ms0 f%d opaque plain models %u per frame, runs %u per frame, removable closes %u per frame (texture changes %u per frame)",
        rm_frame, (unsigned)(ms0_opm / 300u), (unsigned)(ms0_runs / 300u), (unsigned)(ms0_rem / 300u), (unsigned)(ms0_ctot / 300u));
    ms0_rem = ms0_chg = ms0_ctot = ms0_opm = ms0_runs = 0;
    mdc_tot_ops = mdc_tot_draws = mdc_tot_tris = mdc_tot_binds = mdc_none = 0;
    for (int s = 0; s < MDC_SLOTS; s++) mdc[s].used = 0; /* each window allocates its own slots; the table filled over several maps before (unattr ops) */
    mdcm_over = 0;
}

void pm_pos_text(char* out); /* pm_route.c */
static void os_win_clear(void) {
    memset(os_op_n, 0, sizeof os_op_n); memset(os_nn, 0, sizeof os_nn); memset(os_tk, 0, sizeof os_tk); memset(os_sn, 0, sizeof os_sn); memset(os_fl, 0, sizeof os_fl);
    memset(pk_ops, 0, sizeof pk_ops); memset(pk_tk, 0, sizeof pk_tk); memset(pk_sn, 0, sizeof pk_sn);
    memset(pk_rec, 0, sizeof pk_rec); memset(pk_nat, 0, sizeof pk_nat); memset(pk_wd, 0, sizeof pk_wd); memset(pk_nd, 0, sizeof pk_nd); memset(pk_wdw, 0, sizeof pk_wdw);
    gm_steps = 0;
}
const Gfx* pre_w0; const Gfx* pre_mk0; int pre_mkn; /* WCEN2 diag: the last outer walk's list start, its first mark and mark count */
static void os_report(void) {
    static unsigned long long est[256], estc[256], pke[PK_SLOTS];
    unsigned long long cls_tk[OS_N] = { 0 }, cls_n[OS_N] = { 0 }, mod_n[MOD_N] = { 0 }, tot_n = 0, tot_fl = 0, tot_sn = 0, psum = 0;
    for (int op = 0; op < 256; op++) {
        est[op] = os_sn[op] ? (unsigned long long)os_tk[op] * os_op_n[op] / os_sn[op] : 0;
        tot_n += os_op_n[op] + os_nn[op]; tot_fl += os_fl[op]; tot_sn += os_sn[op];
        cls_tk[os_class_of(op)] += est[op]; cls_n[os_class_of(op)] += os_op_n[op];
        mod_n[op_module[op]] += os_op_n[op];
    }
    char c[480]; int k = 0;
    for (int i = 0; i < OS_N; i++) k += snprintf(c + k, sizeof c - k, " %s %u/%u", os_nm[i], (unsigned)(cls_tk[i] / 268 / 300), (unsigned)(cls_n[i] / 300));
    char t[160]; int m = snprintf(t, sizeof t, "top");
    memcpy(estc, est, sizeof est);
    for (int r = 0; r < 6; r++) {
        int best = -1; unsigned long long bv = 0;
        for (int op = 0; op < 256; op++) if (estc[op] > bv) { bv = estc[op]; best = op; }
        if (best < 0) break;
        m += snprintf(t + m, sizeof t - m, " %02x:%u", best, (unsigned)(bv / 268 / 300));
        estc[best] = 0;
    }
    for (int s = 0; s < PK_SLOTS; s++) {
        pke[s] = pk_sn[s] ? (unsigned long long)pk_tk[s] * pk_ops[s] / pk_sn[s] : 0;
        psum += pk_ops[s];
    }
    /* W2: all pm_ticks reads per frame over the 300 frames (tk is the op sampler only) */
    unsigned svc_now = pm_svc_n;
    pm_log_hw("opsplit f%d ops %u |%s | %s | fl %u | tk %u | pt %u | gms %u gmn %u | rec %u", rm_frame, (unsigned)(tot_n / 300), c, t, (unsigned)(tot_fl / 300), (unsigned)(2 * tot_sn / 300), (svc_now - os_svc_last) / 300u, gm_steps / 300, gm_n, rec_n);
    os_svc_last = svc_now;
    { char md[200]; int q = snprintf(md, sizeof md, "modsplit f%d", rm_frame); /* MV (a): module column, own line (the opsplit line is cut at its length) */
      for (int i = 0; i < MOD_N; i++) q += snprintf(md + q, sizeof md - q, " %s %u", mod_nm[i], (unsigned)(mod_n[i] / 300));
      pm_log_hw("%s", md); }
    rec_n = 0;
    { char p[40]; pm_pos_text(p); pm_log_hw("pos f%d %s", rm_frame, p); } /* CAP1: Mario, one line per window */
    pm_log_hw("prod f%d sum %u ops %u", rm_frame, (unsigned)(psum / 300), (unsigned)(tot_n / 300));
    for (int r = 0; r < 15; r++) {
        int best = -1; unsigned long long bv = 0;
        for (int s = 0; s < PK_SLOTS; s++) if (pk_ops[s] > bv) { bv = pk_ops[s]; best = s; }
        if (best < 0) break;
        char nm[48];
        pk_name((unsigned)best, nm, sizeof nm);
        pm_log_hw("prod f%d %s ops %u us %u rec %u nat %u wd %u nd %u", rm_frame, nm, (unsigned)(bv / 300), (unsigned)(pke[best] / 268 / 300), (unsigned)(pk_rec[best] / 300), (unsigned)(pk_nat[best] / 300), (unsigned)pk_wd[best], (unsigned)pk_nd[best]); /* wd, nd: window totals */
        pk_ops[best] = 0;
    }
pm_log_hw("pre f%d marks %d", rm_frame, pre_mkn); /* WCEN2 diag */
    { /* CEN-1a: walker draws per producer by why code (window totals; the why counts of a line add up to its wd). The end line is the until line. */
      unsigned long long wt = 0, nt = 0;
      for (int s = 0; s < PK_SLOTS; s++) {
          wt += pk_wd[s]; nt += pk_nd[s];
          if (!pk_wd[s]) continue;
          char nm[48], w[200]; int m = 0;
          pk_name((unsigned)s, nm, sizeof nm);
          for (int c = 0; c < CW_NW; c++) if (pk_wdw[s][c]) m += snprintf(w + m, sizeof w - m, " %s:%u", pm_gpu_cen_name((unsigned)c), pk_wdw[s][c]);
          pm_log_hw("wcen4 f%d %s wd %u why%s", rm_frame, nm, pk_wd[s], w);
      }
      pm_log_hw("wcen4 end f%d wd %u nd %u", rm_frame, (unsigned)wt, (unsigned)nt);
    }
    os_win_clear();
}

/* CAP1: a map load starts a new window. pm_ph_scene_map calls this. os_fr counts from 0 again, so the next report is 300 frames later. */
void pm_gbi_win_reset(void) {
    os_fr = 0;
    os_win_clear();
    os_svc_last = pm_svc_n;
    memset(mr0_T, 0, sizeof mr0_T); memset(mr0_F, 0, sizeof mr0_F); mr0_plain_n = 0;
    ms0_rem = ms0_chg = ms0_ctot = ms0_opm = ms0_runs = 0;
    memset(sp0_T, 0, sizeof sp0_T); sp0_skip = sp0_grp = 0; /* SPLIT0C */
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
    if (pm_ph_cen) { if (os_fr == 0) os_calibrate(); mdc_frame_end(); if (++os_fr >= 300) { os_report(); mdc_report(); sp0_report(); os_fr = 0; } pmk_n = 0; }
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
        static const char* nm[8] = { "validity,off_screen,box_fold,comb_audit", "ensure_tile", "dual_bind", "reject,cull", "sync_render", "gpu_emit", "old path", "G_VTX" };
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
        (void)nm;
        memset(gp_acc, 0, sizeof gp_acc); gp_ntri = gp_frames = 0;
    }
    nd_mv_sp = 0; /* PT70: the RSP matrix stack starts empty in every task. A push without pop in the message draw leaked one level per frame, then the 10 level cap dropped the pad push of the blue pipe and its pop left the wrong matrix (the sunk pipe showed up while a text box waited) */
    if (gm_n) memset(gm_idx, 0, sizeof gm_idx);
    gm_n = 0; gm_last = GV_NONE; pm_gpu_gv_reset();
    memset(vert_ring, 0xff, sizeof vert_ring);
    memset(vert_gvf, 3, sizeof vert_gvf);
    n_cmd = n_in = n_cut = n_drop = n_out = n_keep = 0;
    n_splits = n_grabs = 0;
    have_sample = 0;
    rm_frame++;
    os_off = ((unsigned)rm_frame * 37u) & 63u; /* W1: the sample phase changes each frame */
}

const char* pm_gbi_stats(void) { return stats_line; }


/* ---- MV-9: frame census helpers, cut from the former pm_gbi.c (no change): flush class, phase streak timers, gp3 gate ---- */
/* PI2: batch class of the op that closes a batch. 0 TEX, 1 CC, 2 GEO, 3 RECT, 4 TRI, 5 other. */
int pm_gbi_flush_class(void) {
    switch (pm_gbi_cur_op) {
    case G_SETTIMG: case G_SETTILE: case G_SETTILESIZE: case G_LOADTILE: case G_LOADBLOCK: case G_LOADTLUT: case G_TEXTURE: return 0;
    case G_SETCOMBINE: case G_SETPRIMCOLOR: case G_SETENVCOLOR: case G_SETOTHERMODE_L: case G_SETOTHERMODE_H: case G_SETFOGCOLOR: return 1;
    case G_MTX: case G_POPMTX: case G_GEOMETRYMODE: case G_MOVEWORD: case G_MOVEMEM: return 2;
    case G_TEXRECT: case G_TEXRECTFLIP: case G_FILLRECT: return 3;
    case G_TRI1: case G_TRI2: return 4;
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
    if (op == G_TRI1 || op == G_TRI2) {
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
    if (op == G_SETTIMG || op == G_SETTILE || op == G_SETTILESIZE || op == G_LOADTILE || op == G_LOADBLOCK || op == G_LOADTLUT || op == G_TEXTURE ) {
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
