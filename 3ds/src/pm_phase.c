/* PT01: per-frame phase timer (measurement only, port layer). Switch `phase`. See pm_port.h.
 * Reads the ARM11 tick through pm_ticks (268111856 Hz). One drawn frame = from the end of the previous drawn frame to the
 * end of this one, so the phases add up to the wall time. Output goes through pm_log (non-blocking queue):
 *   every ~2 s: wall/work, per phase avg/p95/max, counters avg/max, frames over 34 ms (frame numbers), 3 worst frames. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <citro3d.h>
#include "pm_port.h"
#include "nd.h"

#define TICKS_MS 268112.0f
#define MAXW 240
#define OVER_MS 34.0f

int pm_ph_on;
int pm_ph_cen; /* PERF1: phase mode (pm_port.h) */
int pm_cen_lines; /* PROF2-3 (pm_port.h) */
static int cen_want; /* PROF2-3: switch census */
int pm_ph_fine;
unsigned pm_ph_cnt[C_N];
static unsigned prev_cnt[C_N]; /* counters of the last finished frame: pm_ph_cnt is zero again at the next frame begin */
unsigned pm_ph_prev(int k) { return prev_cnt[k]; }
unsigned pm_cs_cnt[CS_N];

static const char* const pname[PH_N] = { "other", "logic", "dl", "gbi", "gpu", "tex", "audio", "rom", "yay0", "fbegin", "fend", "idle", "gcache", "gdraw", "g_vtx", "g_tri", "g_mtx", "g_op", "fbread", "g_tev", "g_bind", "g_state" };
static const char* const cname[C_N] = { "tri", "triout", "vtxload", "vtxslot", "draw", "flush", "texcall", "texmiss", "texKB", "romn", "romKB", "yayn", "yayKB", "dbgcall", "atick", "gbiops", "ajob", "gvload", "gvtri", "gvrej", "gvfb", "gvxf", "flcall", "flkb", "rawd", "rawcall" };

static int stk[16], depth, cur;
static unsigned last, frame_t0, acc[PH_N];
static unsigned frame_no;

typedef struct { float ms[PH_N]; float wall; unsigned c[C_N]; unsigned f; } Rec;
static int nwin;
static unsigned win_t0;

/* ---- HWM1: phs lines (switch phsum) and the in-run A/B (switch abtog=<state,state,...>:<n>) ----
 * One bucket per A/B state holds the sums of the current scene (game mode, area, map). A bucket is written as three log lines
 * (phs: phase sums, phh: cpu histogram, phc: counter sums) when the scene changes or after 300 drawn steps in the bucket.
 * Units: phase sums in 0.1 ms, counters as sums. cpu = wall - idle - fbegin. The first 2 steps after a state change are not summed. */
#define BK_N 8
#define CPU_MAX 300
#define SKIP_STEPS 2
typedef struct {
    unsigned n, f0, f1;
    float ph[PH_N];
    unsigned long long c[C_N];
    float cpu[CPU_MAX];
    unsigned ok[14], late[14];
    float gpu_us[2];
    unsigned cmd_max;
} Bucket;
/* MEM-B v3: the census window and the A/B buckets are one cold calloc, made at init only when phase or phsum is on (0 bytes otherwise). */
typedef struct { Rec win[MAXW]; Bucket bk[BK_N]; } PmPhCold;
static PmPhCold* cold;
static void cold_need(void) { if (!cold) cold = calloc(1, sizeof *cold); } /* MEM-B v3: called where ph_want or ps_on becomes set */
static int ps_on;
static const char* const ab_words[] = { "base", "phasegbi" }; /* rule 19: a state is added here only for one console comparison and taken out again after it */
static int ab_st[BK_N], ab_n, ab_every, ab_cur, ab_steps, ab_skip, ab_seq;
static int sc_mode = -1, sc_area = -1, sc_map = -1; /* the scene of the open buckets */
static int cur_mode = -1, cur_area = -1, cur_map = -1; /* set by the game hooks */
static int pf_scene_seen; /* PROF2-6: 0 after a scene event (mode, map, window start) until the prof scene note. Zero-initialised (.bss): a .data word would move the initialised data of the image */
unsigned pm_ph_gpu_us[2];
unsigned pm_ph_cmd_pm;
unsigned pm_ph_kick_n, pm_ph_kick_us, pm_ph_draws, pm_ph_iv, pm_ph_hud_us; /* PI4, PI5: per frame, reset in pm_ph_frame_end */

void pm_ph_scene_mode(int mode) { cur_mode = mode; pf_scene_seen = 0; }
static void fs_win_reset(void);
void pm_gbi_win_reset(void);
/* CAP1: a map load starts the opsplit, mr0, ms0 and fsplit windows */
void pm_ph_scene_map(int area, int map) { cur_area = area; cur_map = map; pf_scene_seen = 0; pm_gbi_win_reset(); fs_win_reset(); }
void pm_ph_scene_get(int* mode, int* area, int* map) { *mode = cur_mode; *area = cur_area; *map = cur_map; }
void pm_ph_gpu_note(void) {
    if (!pm_ph_on) return;
    pm_ph_gpu_us[0] = (unsigned)(C3D_GetDrawingTime() * 1000.f);
    pm_ph_gpu_us[1] = (unsigned)(C3D_GetProcessingTime() * 1000.f);
}

static void ab_apply(int st, int announce) {
    /* switch off the words of the old state, then the new one. The generation counter makes every PMDL cache see it. */
    for (int i = 1; i < (int)(sizeof ab_words / sizeof *ab_words); i++) pm_log_switch(ab_words[i], 0);
    if (st > 0) pm_log_switch(ab_words[st], 1);
    pm_ph_fine = pm_ph_on && (st == 1 || pm_debug_has("phasegbi"));
    pm_walk_dbg_set();
    if (announce) pm_log_hw("abtog %d %s f%u", ab_seq, ab_words[st], frame_no);
    ab_seq++;
}

static void ps_init(void) {
    char v[96];
    if (!pm_debug_value("abtog", v, sizeof v)) return;
    char* c = strchr(v, ':');
    ab_every = c ? atoi(c + 1) : 30;
    if (ab_every < 4) ab_every = 4;
    if (c) *c = 0;
    for (char* t = strtok(v, ","); t && ab_n < BK_N; t = strtok(NULL, ",")) {
        for (int i = 0; i < (int)(sizeof ab_words / sizeof *ab_words); i++) if (!strcmp(t, ab_words[i])) { ab_st[ab_n++] = i; break; }
    }
    if (ab_n < 2) { ab_n = 0; ab_every = 0; return; }
    ps_on = 1;
    ab_apply(ab_st[0], 0);
    pm_log_hw("abtog %d %s f%u (start, %d states, every %d drawn steps)", 0, ab_words[ab_st[0]], 0u, ab_n, ab_every);
}

static int fcmp(const void* a, const void* b) { float x = *(const float*)a, y = *(const float*)b; return x < y ? -1 : x > y; }

static void bk_flush(int i) {
    Bucket* b = &cold->bk[i];
    if (!b->n) return;
    const char* nm = ab_words[ab_n ? ab_st[i] : 0];
    unsigned id = b->f0; /* the first frame number joins the three lines */
    qsort(b->cpu, b->n, sizeof(float), fcmp);
    float p50 = b->cpu[(b->n * 50 + 99) / 100 - 1], p95 = b->cpu[(b->n * 95 + 99) / 100 - 1];
    static const int ord[] = { PH_LOGIC, PH_DL, PH_GBI, PH_GPU, PH_GCACHE, PH_GDRAW, PH_TEX, PH_AUDIO, PH_ROM, PH_YAY0, PH_FBEGIN, PH_FEND, PH_OTHER, PH_GVTX, PH_GTRI, PH_GMTX, PH_GOP, PH_FBREAD, PH_GTEV, PH_GBIND, PH_GST };
    char line[256];
    int len = snprintf(line, sizeof line, "phs %d %d:%d %s n %u f%u-%u |", sc_mode, sc_area, sc_map, nm, b->n, b->f0, b->f1);
    for (unsigned k = 0; k < sizeof ord / sizeof *ord; k++) len += snprintf(line + len, sizeof line - (size_t)len, " %d", (int)(b->ph[ord[k]] * 10.f + 0.5f));
    snprintf(line + len, sizeof line - (size_t)len, " | %d %d", (int)(p50 * 10.f + 0.5f), (int)(p95 * 10.f + 0.5f));
    pm_log_hw("%s", line);
    len = snprintf(line, sizeof line, "phh %d %d:%d %s f%u |", sc_mode, sc_area, sc_map, nm, id);
    for (int k = 0; k < 14; k++) len += snprintf(line + len, sizeof line - (size_t)len, " %u", b->ok[k]);
    len += snprintf(line + len, sizeof line - (size_t)len, " |");
    for (int k = 0; k < 14; k++) len += snprintf(line + len, sizeof line - (size_t)len, " %u", b->late[k]);
    pm_log_hw("%s", line);
    len = snprintf(line, sizeof line, "phc %d %d:%d %s f%u |", sc_mode, sc_area, sc_map, nm, id);
    for (int k = 0; k < C_N; k++) len += snprintf(line + len, sizeof line - (size_t)len, " %llu", b->c[k]);
    snprintf(line + len, sizeof line - (size_t)len, " | gpu %u %u cmdmax %u", (unsigned)b->gpu_us[0], (unsigned)b->gpu_us[1], b->cmd_max);
    pm_log_hw("%s", line);
    memset(b, 0, sizeof *b);
}

static void ps_step(const Rec* r) {
    if (cur_mode != sc_mode || cur_area != sc_area || cur_map != sc_map) {
        for (int i = 0; i < BK_N; i++) bk_flush(i);
        sc_mode = cur_mode; sc_area = cur_area; sc_map = cur_map;
    }
    int si = ab_n ? ab_cur : 0;
    if (ab_skip > 0) ab_skip--;
    else {
        Bucket* b = &cold->bk[si];
        float cpu = r->wall - r->ms[PH_IDLE] - r->ms[PH_FBEGIN];
        int bin = cpu < 24.f ? 0 : cpu >= 36.f ? 13 : (int)cpu - 23;
        if (!b->n) b->f0 = r->f;
        b->f1 = r->f;
        b->cpu[b->n++] = cpu;
        if (r->wall <= 34.5f) b->ok[bin]++; else b->late[bin]++;
        for (int k = 0; k < PH_N; k++) b->ph[k] += r->ms[k];
        for (int k = 0; k < C_N; k++) b->c[k] += r->c[k];
        b->gpu_us[0] += (float)pm_ph_gpu_us[0];
        b->gpu_us[1] += (float)pm_ph_gpu_us[1];
        if (pm_ph_cmd_pm > b->cmd_max) b->cmd_max = pm_ph_cmd_pm;
        if (b->n >= CPU_MAX) bk_flush(si);
    }
    if (ab_n && ++ab_steps >= ab_every) {
        ab_steps = 0;
        ab_cur = (ab_cur + 1) % ab_n;
        ab_apply(ab_st[ab_cur], 1);
        ab_skip = SKIP_STEPS;
    }
}

/* FS2: switch fsplit. The open part takes the time since the last mark. A mark reads the tick once, so a drawn frame has 12 reads:
 * 11 marks in pm_gpu_begin, the step wrap, nuGfxTaskStart, pm_gpu_end and pm_main, plus the frame end. No per-op or per-draw read. */
int pm_fs_on, pm_fs_q;
static int fs_want, fs_first;
static int fs_cur = FS_REST;
static unsigned fs_last, fs_t0, fs_svc0, fs_max, fs_over, fs_n, fs_frame;
static unsigned long long fs_sum[FS_N], fs_wall;
static unsigned long long fs_tk_sum, fs_wm_sum; /* WMODEL: render tasks and world model list entries, summed over the window */
static unsigned fs_tk_max, fs_wm_max;
unsigned pm_fs_tasks;
/* WMODEL: non-null entries of the decomp world model list (wModelList, MAX_MODELS 256). Cold: once per frame, fsplit only. */
extern void* wModelList[256];
static __attribute__((cold, noinline)) unsigned fs_count_models(void) {
    unsigned n = 0;
    for (int i = 0; i < 256; i++) n += wModelList[i] != 0;
    return n;
}
extern unsigned pm_svc_n;
#define FS_OVER50 ((unsigned)(50.f * TICKS_MS)) /* 50 ms in ticks */
#define FSMS(k) ((float)fs_sum[k] / (float)fs_n / TICKS_MS)

void pm_fs_mark(int cat) {
    if (pm_fs_q) return;
    unsigned now = pm_ticks();
    fs_sum[fs_cur] += now - fs_last;
    fs_last = now;
    fs_cur = cat;
}

static void fs_arm(void) { /* the first frame end after arming only starts the clock: the boot time is not a frame */
    fs_last = fs_t0 = pm_ticks();
    fs_cur = FS_REST;
    fs_svc0 = pm_svc_n;
    fs_first = 1;
}

/* CAP1: clears the fsplit window sums. A map load (pm_ph_scene_map) and the window end both call it. */
static void fs_win_reset(void) {
    memset(fs_sum, 0, sizeof fs_sum);
    fs_wall = 0;
    fs_max = fs_over = fs_n = 0;
    fs_tk_sum = fs_wm_sum = 0;
    fs_tk_max = fs_wm_max = 0;
    fs_svc0 = pm_svc_n;
}

static void fs_frame_end(void) {
    unsigned now = pm_ticks();
    fs_sum[fs_cur] += now - fs_last;
    fs_last = now;
    fs_cur = FS_REST;
    unsigned w = now - fs_t0;
    fs_t0 = now;
    unsigned tk = pm_fs_tasks; /* WMODEL */
    pm_fs_tasks = 0;
    if (fs_first) { /* the first frame: no sum, no line */
        fs_first = 0;
        memset(fs_sum, 0, sizeof fs_sum);
        fs_svc0 = pm_svc_n;
        return;
    }
    fs_frame++;
    fs_wall += w;
    if (w > fs_max) fs_max = w;
    if (w > FS_OVER50) fs_over++;
    unsigned wm = fs_count_models(); /* WMODEL */
    fs_tk_sum += tk; fs_wm_sum += wm;
    if (tk > fs_tk_max) fs_tk_max = tk;
    if (wm > fs_wm_max) fs_wm_max = wm;
    if (++fs_n < 300) return;
    /* the rest part: wall minus the sum of the other parts, by construction of the chain (every tick belongs to one part) */
    pm_log_hw("fsplit f%u logic %.1f dl %.1f gbi %.1f flush %.1f gwait %.1f swap %.1f audio %.1f rest %.1f wall %.1f max %.1f over50 %u svc %.1f wm %.1f max %u tk %.2f max %u",
              fs_frame, FSMS(FS_LOGIC), FSMS(FS_DL), FSMS(FS_GBI), FSMS(FS_FLUSH), FSMS(FS_GWAIT), FSMS(FS_SWAP), FSMS(FS_AUDIO), FSMS(FS_REST),
              (float)fs_wall / (float)fs_n / TICKS_MS, (float)fs_max / TICKS_MS, fs_over, (float)(pm_svc_n - fs_svc0) / (float)fs_n,
              (float)fs_wm_sum / (float)fs_n, fs_wm_max, (float)fs_tk_sum / (float)fs_n, fs_tk_max);
    fs_win_reset();
}

/* CF0: PMU counters, switch pmu (log on). A window is 300 drawn frames. Window A counts CORE_INST_CACHE_MISS (PMN0) and
 * CORE_DATA_CACHE_READ_MISS (PMN1); window B counts CORE_MAIN_TLB_MISS and CORE_STALL_BY_LACK_OF_INST. The counters are read once
 * at the window end, so a frame costs no svc call. One line per window: per-frame counts, cycles, and the last svc result rc. */
int pm_pmu_on;
static int pmu_want, pmu_win, pmu_n;
static unsigned pmu_f;
static Result pmu_rc;
static void pmu_set_window(void) {
    u64 out;
    u32 ev0 = pmu_win ? PERFCOUNTEREVT_CORE_MAIN_TLB_MISS : PERFCOUNTEREVT_CORE_INST_CACHE_MISS;
    u32 ev1 = pmu_win ? PERFCOUNTEREVT_CORE_STALL_BY_LACK_OF_INST : PERFCOUNTEREVT_CORE_DATA_CACHE_READ_MISS;
    pmu_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_SET_EVENT, PERFCOUNTERREG_CORE_COUNT_REG_0, ev0);
    pmu_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_SET_EVENT, PERFCOUNTERREG_CORE_COUNT_REG_1, ev1);
    pmu_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_SET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_0, 0);
    pmu_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_SET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_1, 0);
    pmu_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_SET_VALUE, PERFCOUNTERREG_CORE_CYCLE_COUNTER, 0);
    pmu_n = 0;
}
static void pmu_init(void) {
    u64 out;
    pmu_want = pm_debug_has("pmu");
    pm_pmu_on = pmu_want && pm_log_file();
    if (!pm_pmu_on) return;
    pmu_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_ENABLE, 0, 0);
    pmu_set_window();
}
static void pmu_frame_end(void) {
    if (++pmu_n < 300) return;
    u64 c0, c1, cc;
    pmu_rc = svcControlPerformanceCounter(&c0, PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_0, 0);
    pmu_rc = svcControlPerformanceCounter(&c1, PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_1, 0);
    pmu_rc = svcControlPerformanceCounter(&cc, PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_CYCLE_COUNTER, 0);
    unsigned ev0 = pmu_win ? PERFCOUNTEREVT_CORE_MAIN_TLB_MISS : PERFCOUNTEREVT_CORE_INST_CACHE_MISS;
    unsigned ev1 = pmu_win ? PERFCOUNTEREVT_CORE_STALL_BY_LACK_OF_INST : PERFCOUNTEREVT_CORE_DATA_CACHE_READ_MISS;
    pmu_f += 300;
    pm_log_hw("pmu f%u ev %u %u per frame %u %u cyc %u rc %08x", pmu_f, ev0, ev1, (unsigned)(c0 / 300), (unsigned)(c1 / 300),
              (unsigned)(cc / 300), (unsigned)pmu_rc);
    pmu_win ^= 1;
    pmu_set_window();
}
/* CAP2: the counters at the sp0c window end, per frame since the last counter reset (pmu_n frames). Three reads per call, cold.
 * Window A holds INST_CACHE_MISS and DATA_CACHE_READ_MISS in out[0] and out[1]; window B holds MAIN_TLB_MISS and STALL in those slots. */
void pm_pmu_sp0(unsigned out[3]) {
    u64 c0 = 0, c1 = 0, cc = 0;
    out[0] = out[1] = out[2] = 0;
    if (!pm_pmu_on || pmu_n == 0) return;
    pmu_rc = svcControlPerformanceCounter(&c0, PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_0, 0);
    pmu_rc = svcControlPerformanceCounter(&c1, PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_1, 0);
    pmu_rc = svcControlPerformanceCounter(&cc, PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_CYCLE_COUNTER, 0);
    out[0] = (unsigned)(c0 / pmu_n); out[1] = (unsigned)(c1 / pmu_n); out[2] = (unsigned)(cc / pmu_n);
}

/* PF1, PF2: switch prof (clean-frame profile) or prof=<N> (sample every N drawn frames, 4 to 255, default 30). profstall picks counter set s (PF3).
 * pm_pf_frame runs once per drawn frame in the main loop with two ticks the loop already has. No svc call per frame. One line per 300 drawn frames
 * at warn level. PF2: one drawn frame in N is a sample. pm_pf_frame arms pm_ph_on for the next drawn frame, so that frame's phase timers run;
 * pm_ph_frame_end hands its record to pf_take and pm_pf_frame turns pm_ph_on off again. The PMU fields (PF3) print 0 until PF3. */
extern unsigned pm_step_calls;
extern unsigned pm_ph_iv;
unsigned long long pm_gbi_sp0_dlc(void); /* the former pm_gbi.c: dlc group ticks, summed in sampled frames only */
int pm_pf_on;
void pm_log_prof_line(const char* fmt, ...); /* PROF4, pm_log.c */
int pm_pf_arm; /* prof samples: prof on and phase off. the former pm_gbi.c skips its phase reports while it is set */
static int pf_want;
static unsigned pf_every = 30;
static unsigned pf_cd; /* clean drawn frames left before the next sample */
static int pf_samp; /* the drawn frame in progress is a sample */
static int pf_prev_s; /* the previous drawn frame was a sample: its wall belongs to the sample sums */
static char pf_set = 'm';
static unsigned long long pf_last, pf_wsum, pf_wmax, pf_wksum;
static unsigned long long pf_vb0;
static unsigned pf_st0, pf_total, pf_w, pf_wn, pf_wkn, pf_o34;
static unsigned pf_iv[4];
static unsigned pf_s, pf_sw_n; /* sampled frames (work), sampled walls */
static unsigned long long pf_sw_fts, pf_sw_wks; /* sampled wall and work sums, ticks */
static float pf_sms[3]; /* sampled gbi, tex, gpu: sums, ms */
static unsigned long long pf_dlc_t, pf_dlc_prev; /* dlc group ticks: window sum and the last reading */
static Result pf_rc; /* last svc result of the counter edges */
static u64 pf_base[3]; /* PF3: PMN0, PMN1, CCNT at the last sample end. Zero at init (the counters are reset there). */
static unsigned long long pf_pe[3]; /* PF3: clean-interval sums of PMN0, PMN1, CCNT in the window */
static unsigned pf_pfr, pf_run; /* PF3: clean frames summed in the window; clean frames since the last sample end */
unsigned pm_pf_late;
static unsigned pf_svc0, pf_csvc, pf_ssvc; /* PROF2-1: pm_svc_n at the last pm_pf_frame; window sums of the svc tick reads in clean and in sampled frames */
/* PF3: one svc per register. The counters are not reset here, so each edge is a plain read. */
static void pf_pmu_read(u64 out[3]) {
    pf_rc = svcControlPerformanceCounter(&out[0], PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_0, 0);
    pf_rc = svcControlPerformanceCounter(&out[1], PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_1, 0);
    pf_rc = svcControlPerformanceCounter(&out[2], PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_CYCLE_COUNTER, 0);
}
#define PF_OVER_T 9115808ull /* 34 ms in ticks: 34 x TICKS_MS */
void pm_gpu_pf_read(unsigned long long out[6]); /* pm_gpu.c: per frame counters and GPU busy ticks, read and reset */
static unsigned long long pf_nd_t; static unsigned pf_leg; /* S0-6: native emit ticks and LEGACY records of the window (pm_pf_nd_add, from S1) */
void pm_pf_nd_add(unsigned long long ticks, unsigned legacy) { pf_nd_t += ticks; pf_leg += legacy; }
/* PROF: the scene of the window. Every prof line of a window ends with it, so runs of one room or event compare by key (map, mode, story, battle).
 * A scene change inside a window drops the window and starts a new one, so chg stays 0 (kept in the line for older readers). */
void pm_prof_where(const char** name, int* story, int* btl, int* entry, float pos[3]); /* pm_route.c */
static int pf_sc[3] = { -1, -1, -1 };
static float pf_pos0[3]; /* Mario position at the first frame of the window */
/* CORE2-2 premise: on the N3DS, can the game create a thread on core 2 (the planned walker thread)? One short thread at prof start
 * stores its processor id; the line says created and the id, or refused. */
static volatile int c2_core = -9;
static void c2_main(void* arg) { (void)arg; c2_core = (int)svcGetProcessorID(); }
static void c2_probe(void) {
    bool n3 = false;
    APT_CheckNew3DS(&n3);
    if (!n3) { pm_log_prof_line("prof core2 probe: O3DS, no core 2"); return; }
    Thread t = threadCreate(c2_main, NULL, 4 * 1024, 0x30, 2, false);
    if (!t) { pm_log_prof_line("prof core2 probe: N3DS, thread on core 2 REFUSED"); return; }
    threadJoin(t, 1000000000LL);
    threadFree(t);
    pm_log_prof_line("prof core2 probe: N3DS, thread on core 2 created, ran on core %d", c2_core);
}
extern unsigned long long pm_au_pf_sum; /* CORE2-0, pm_audio.c: join wait ticks of the window, longest single wait */
extern unsigned pm_au_pf_max;
/* PROF: start a new window (after a report, or when the scene changes inside a window: a window holds one scene only, so chg is 0 on every printed line) */
static void pf_reset(void) {
    pm_au_pf_sum = 0; pm_au_pf_max = 0;
    { uint64_t ct[NDT_N]; pm_gpu_ndt_take(ct); } /* PERF1: the class timers start with the window (a tour window, a scene change) */
    pf_nd_t = 0; pf_leg = 0;
    pf_sc[0] = pf_sc[1] = pf_sc[2] = -1; /* the next window starts in the scene now */
    pf_pe[0] = pf_pe[1] = pf_pe[2] = 0;
    pf_pfr = 0;
    pf_csvc = pf_ssvc = 0;
    pf_wsum = pf_wmax = pf_wksum = 0;
    pf_wn = pf_wkn = pf_o34 = pf_w = 0;
    memset(pf_iv, 0, sizeof pf_iv);
    pf_s = pf_sw_n = 0;
    pf_sw_fts = pf_sw_wks = 0;
    pf_sms[0] = pf_sms[1] = pf_sms[2] = 0.f;
    pf_dlc_t = 0;
    pf_vb0 = C3D_FrameCounter(0);
    pf_st0 = pm_step_calls;
    pf_scene_seen = 0; /* PROF2-6: the next drawn frame notes the scene and the start position */
}
/* PROF: the harness placed Mario (pos= switch, pm_warp.c pm_pos_log): a new window starts at the placed position */
void pm_ph_scene_moved(void) { pf_reset(); }
/* TOUR1: tour mode (pm_warp.c map tour). The tour owns the window: no 300-frame report and no scene drop. pm_ph_window_open starts a window
 * after the load settles; pm_ph_window_close prints it (pf_report divides by pf_w and pf_s, so a short window is valid) before the next warp. */
static int pf_tour;
static char pf_tlabel[48];
void pm_ph_tour_mode(int on) { pf_tour = on; }
static void pf_scene_note(void) {
    if (pf_sc[0] == -1) {
        pf_sc[0] = cur_area; pf_sc[1] = cur_map; pf_sc[2] = cur_mode;
        pf_pos0[0] = pf_pos0[1] = pf_pos0[2] = 0.f;
        if (cur_area >= 0 && cur_map >= 0) { const char* nm; int story, btl, en; pm_prof_where(&nm, &story, &btl, &en, pf_pos0); }
    }
    else if (pf_sc[0] != cur_area || pf_sc[1] != cur_map || pf_sc[2] != cur_mode) { pf_reset(); pf_scene_note(); } /* the scene changed: drop the window, start again in the new scene */
}
/* PROF2-4: the report lines carry integers. A time is ms x 100 (printed %u.%02u) from u64 ticks, TICKS_MS 268112; no double. Each line is at
 * most 16 argument words with no refused conversion, so the log queue copies its words (pm_logfmt.h lf_scan) and the writer formats it:
 * the game thread makes no vsnprintf. Cold: once per window. */
#define PF_TPM 268112ull /* ticks per ms */
static unsigned pf_cs(unsigned long long ticks, unsigned n) { /* mean ms x 100 of n frames */
    return n ? (unsigned)(ticks * 100ull / ((unsigned long long)n * PF_TPM)) : 0u;
}
static unsigned pf_fcs(float ms, float n) { return ms > 0.f ? (unsigned)(ms * 100.f / n + 0.5f) : 0u; } /* a float ms sum (phase record) as ms x 100 */
static int pf_ri(float x) { return (int)(x < 0.f ? x - 0.5f : x + 0.5f); }
static const char* const pf_disp[8] = { "w0 s0 f30", "w0 s0 f60", "w0 s1 f30", "w0 s1 f60", "w1 s0 f30", "w1 s0 f60", "w1 s1 f30", "w1 s1 f60" };
/* The scene line: the static map name, positions rounded to integers, the display mode as one static string. chg is always 0 (a scene change
 * drops the window), kept in the text for older readers. */
static void pf_where_line(void) {
    const char* nm = "?"; int story = 0, btl = 0, en = 0; float pos[3] = { 0.f, 0.f, 0.f };
    if (cur_area >= 0 && cur_map >= 0) pm_prof_where(&nm, &story, &btl, &en, pos);
    extern int pm_wide; int pm_gpu_stereo_flag(void); /* disp: the display mode at print time (read only) */
    unsigned dm = (pm_wide ? 4u : 0u) | (pm_gpu_stereo_flag() ? 2u : 0u) | (pm_interp_on > 0 ? 1u : 0u);
    pm_log_prof_line("prof f%u w%u at %s %d:%d m%d sp%d b%d e%d chg0 pos0 %d %d %d pos %d %d %d disp %s", pf_total, pf_w, nm, cur_area, cur_map, cur_mode,
        story, btl, en, pf_ri(pf_pos0[0]), pf_ri(pf_pos0[1]), pf_ri(pf_pos0[2]), pf_ri(pos[0]), pf_ri(pos[1]), pf_ri(pos[2]), pf_disp[dm]);
}
static void pf_report(void) {
    unsigned vb = (unsigned)(C3D_FrameCounter(0) - pf_vb0);
    unsigned fps10 = vb ? pf_w * 600u / vb : 0u; /* fps x 10 */
    unsigned ft = pf_cs(pf_wsum, pf_wn), wk = pf_cs(pf_wksum, pf_wkn), fts = pf_cs(pf_sw_fts, pf_sw_n), wks = pf_cs(pf_sw_wks, pf_s);
    unsigned mx10 = (unsigned)(pf_wmax * 10ull / PF_TPM); /* max ms x 10 */
    int oh = (int)wks - (int)wk; /* ms x 100 that the sampled timers add to one sampled frame */
    unsigned oha = oh < 0 ? (unsigned)-oh : (unsigned)oh;
    unsigned amort = oh > 0 ? (unsigned)oh * 10u / pf_every : 0u; /* us per drawn frame: oh ms x 1000 / N */
    float sn = pf_s ? (float)pf_s : 1.f;
    unsigned e0 = pf_pfr ? (unsigned)(pf_pe[0] / pf_pfr) : 0u; /* PMN0 per clean frame */
    unsigned e1 = pf_pfr ? (unsigned)(pf_pe[1] / pf_pfr) : 0u; /* PMN1 per clean frame */
    unsigned cy = pf_pfr ? (unsigned)(pf_pe[2] / pf_pfr) : 0u; /* CCNT per clean frame */
    unsigned p0 = pf_pe[2] ? (unsigned)(pf_pe[0] * 1000ull / pf_pe[2]) : 0u; /* permille of cycles */
    unsigned p1 = pf_pe[2] ? (unsigned)(pf_pe[1] * 1000ull / pf_pe[2]) : 0u;
    unsigned gbi = pf_fcs(pf_sms[0], sn), tex = pf_fcs(pf_sms[1], sn), gpu = pf_fcs(pf_sms[2], sn), dlc = pf_cs(pf_dlc_t, pf_s);
    pm_log_prof_line("prof f%u w%u fps %u.%u | ft %u.%02u max %u.%u o34 %u wk %u.%02u", pf_total, pf_w, fps10 / 10u, fps10 % 10u,
        ft / 100u, ft % 100u, mx10 / 10u, mx10 % 10u, pf_o34, wk / 100u, wk % 100u);
    /* PROF2-1: the self-check csvc (svc tick reads of all clean frames of the window, target 0) and ssvc (per sampled frame) next to oh and amort */
    pm_log_prof_line("prof f%u w%u s%u fts %u.%02u wks %u.%02u oh %s%u.%02u amort %u csvc %u ssvc %u", pf_total, pf_w, pf_s, fts / 100u, fts % 100u,
        wks / 100u, wks % 100u, oh < 0 ? "-" : "", oha / 100u, oha % 100u, amort, pf_csvc, pf_s ? pf_ssvc / pf_s : 0u);
    pm_log_prof_line("prof f%u w%u ph gbi %u.%02u dlc %u.%02u tex %u.%02u gpu %u.%02u | st %u vb %u iv %u %u %u %u", pf_total, pf_w,
        gbi / 100u, gbi % 100u, dlc / 100u, dlc % 100u, tex / 100u, tex % 100u, gpu / 100u, gpu % 100u,
        pm_step_calls - pf_st0, vb, pf_iv[0], pf_iv[1], pf_iv[2], pf_iv[3]);
    /* PERF1: the counters on their own line */
    pm_log_prof_line("prof f%u w%u pmu %c %u %u cyc %u pm %u %u", pf_total, pf_w, pf_set, e0, e1, cy, p0, p1);
    { /* PERF1: ms per sampled frame of each producer class (exclusive, nd.h NDT_*); rest = gbi less the classes (the walker ops, state, other lists) */
        uint64_t ct[NDT_N]; pm_gpu_ndt_take(ct);
        unsigned cm[NDT_N], csum = 0;
        for (int k = 0; k < NDT_N; k++) { cm[k] = pf_cs(ct[k], pf_s); csum += cm[k]; }
        int rest = (int)gbi - (int)csum; unsigned ra = rest < 0 ? (unsigned)-rest : (unsigned)rest;
        pm_log_prof_line("prof f%u w%u cls rec %u.%02u ent %u.%02u spr %u.%02u dlc %u.%02u sub %u.%02u rest %s%u.%02u", pf_total, pf_w,
            cm[NDT_REC] / 100u, cm[NDT_REC] % 100u, cm[NDT_ENT] / 100u, cm[NDT_ENT] % 100u, cm[NDT_SPR] / 100u, cm[NDT_SPR] % 100u,
            cm[NDT_DLC] / 100u, cm[NDT_DLC] % 100u, cm[NDT_SUB] / 100u, cm[NDT_SUB] % 100u, rest < 0 ? "-" : "", ra / 100u, ra % 100u);
    }
    { /* S0-6: stage line. nd: native emit ms per sampled frame. lg: legacy walker (gbi) ms per sampled frame. dr, tc, tw, tri: per drawn frame. leg: LEGACY records per drawn frame. gb: GPU busy ms per sampled frame. */
        unsigned long long pv[6]; pm_gpu_pf_read(pv);
        unsigned pw = pf_w ? pf_w : 1u;
        unsigned nd = pf_cs(pf_nd_t, pf_s), gb = pf_cs(pv[4], (unsigned)pv[5]);
        pm_log_prof_line("prof f%u w%u nd %u.%02u lg %u.%02u dr %u tc %u tw %u tri %u leg %u gb %u.%02u", pf_total, pf_w, nd / 100u, nd % 100u,
            gbi / 100u, gbi % 100u, (unsigned)(pv[0] / pw), (unsigned)(pv[1] / pw), (unsigned)(pv[2] / pw), (unsigned)(pv[3] / pw), pf_leg / pw,
            gb / 100u, gb % 100u);
        /* S0-8: frame budget targets (docs/handoff/frame_budget.md, phase off ms) next to the measured nd, lg and gb of this window. */
        pm_log_prof_line("prof f%u w%u budget tgt logic 3.3 dl 4.4 nd 2.0 fe 1.5 au 0.5 cpu 11.7 slack 5.0 gb 14.0 frame 16.7 | meas nd %u.%02u lg %u.%02u gb %u.%02u",
            pf_total, pf_w, nd / 100u, nd % 100u, gbi / 100u, gbi % 100u, gb / 100u, gb % 100u);
        pf_where_line(); /* the scene line */
        /* U1: the writer heartbeat count (grows only while the queue runs). CORE2-0: aujoin_us = game thread audio join wait per sampled frame
         * (PROF2-1: timed in sampled frames only), aujoin_max = longest single join (us). */
        pm_log_prof_line("prof f%u w%u writer beats %u | aujoin_us %u aujoin_max %u", pf_total, pf_w, pm_log_writer_hb(),
            pf_s ? (unsigned)(pm_au_pf_sum * 1000ull / PF_TPM / pf_s) : 0u, (unsigned)((unsigned long long)pm_au_pf_max * 1000ull / PF_TPM));
    }
    pf_reset();
}
/* PF2: the next drawn frame is a sample. The phase stack starts clean, so its first push adds no time from before the sample. */
static void pf_arm(void) {
    u64 c[3];
    pf_pmu_read(c);
    for (int k = 0; k < 3; k++) pf_pe[k] += (unsigned)(c[k] - pf_base[k]); /* u32 delta: one interval is far below 2^32 cycles */
    pf_pfr += pf_run;
    pf_run = 0;
    last = frame_t0 = pm_ticks();
    memset(acc, 0, sizeof acc);
    depth = 0;
    cur = PH_OTHER;
    pf_samp = 1;
    cold_need(); /* SPRFIX: pm_ph_frame_end reads cold->win for a sampled frame; prof mode 1 never allocates cold otherwise (NULL read at pf_take) */
    pm_ph_on = 1;
    pm_walk_dbg_set();
}
/* PF2: one sampled frame, at its end. r is the frame record: it is not stored in the phase window. */
static void pf_take(const Rec* r) {
    unsigned long long d = pm_gbi_sp0_dlc();
    pf_dlc_t += d >= pf_dlc_prev ? d - pf_dlc_prev : d; /* a map load resets the counter: then d is the new value */
    pf_dlc_prev = d;
    pf_sms[0] += r->ms[PH_GBI]; pf_sms[1] += r->ms[PH_TEX]; pf_sms[2] += r->ms[PH_GPU];
}
void pm_pf_frame(unsigned long long t0, unsigned long long t1) {
    int cur_s = pf_samp; /* this drawn frame is a sample */
    unsigned sv = pm_svc_n; /* PROF2-1: the tick reads since the last call belong to this frame (a sample's arm read is in its own interval) */
    if (!pf_total) pf_svc0 = sv; /* the boot reads are not a frame */
    unsigned late = pm_pf_late; /* the GPU done read of the last sample (pm_gpu.c gb), made at the next frame begin */
    pm_pf_late = 0;
    if (cur_s) pf_ssvc += sv - pf_svc0; else { pf_csvc += sv - pf_svc0 - late; pf_ssvc += late; }
    pf_svc0 = sv;
    if (!pf_total) { pf_vb0 = C3D_FrameCounter(0); pf_st0 = pm_step_calls; }
    if (pf_last) { /* wall of the previous drawn frame: its start to this start */
        unsigned long long w = t0 - pf_last;
        if (pf_prev_s) { pf_sw_fts += w; pf_sw_n++; }
        else { pf_wsum += w; pf_wn++; if (w > pf_wmax) pf_wmax = w; if (w > PF_OVER_T) pf_o34++; }
    }
    pf_last = t0;
    pf_prev_s = cur_s;
    unsigned long long wk = t1 - t0;
    if (cur_s) { /* sample end: its work, then the phase timers off again */
        pf_s++; pf_sw_wks += wk;
        pf_samp = 0; pm_ph_on = 0; pf_cd = pf_every - 1;
        pm_walk_dbg_set();
        pf_pmu_read(pf_base); pf_run = 0;
    } else {
        pf_wksum += wk; pf_wkn++; pf_run++;
        if (pm_pf_arm && --pf_cd == 0) pf_arm();
    }
    unsigned iv = pm_ph_iv;
    pf_iv[iv < 1 ? 0 : iv > 4 ? 3 : iv - 1]++;
    pf_total++;
    pf_w++;
    if (pf_tour) return; /* TOUR1: the tour opens and closes the window */
    if (!pf_scene_seen) { pf_scene_seen = 1; pf_scene_note(); } /* PROF2-6: only after a scene event (pm_ph_scene_map, pm_ph_scene_mode, pf_reset), one flag test per frame */
    if (pf_w >= 300) pf_report();
}
void pm_ph_window_open(const char* label) {
    snprintf(pf_tlabel, sizeof pf_tlabel, "%s", label);
    if (!pm_pf_on) return;
    pf_reset();
    pf_scene_note(); /* the scene and the start position of this window */
}
void pm_ph_window_close(void) {
    if (!pm_pf_on || !pf_w) return;
    pm_log_prof_line("prof f%u w%u tour %s", pf_total, pf_w, pf_tlabel);
    pf_report();
}
/* PROFROW: the prof counter setup and the calibration line. Called at boot (pf_init) and when the Profiling row turns prof on. */
static void pf_start(void) {
    u64 out;
    pm_pmu_on = 0; /* prof owns the two counters: the pmu switch does not change their set */
    u32 ev0 = pf_set == 's' ? PERFCOUNTEREVT_CORE_STALL_BY_LACK_OF_INST : PERFCOUNTEREVT_CORE_INST_CACHE_MISS;
    pf_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_ENABLE, 0, 0);
    pf_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_SET_EVENT, PERFCOUNTERREG_CORE_COUNT_REG_0, ev0);
    pf_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_SET_EVENT, PERFCOUNTERREG_CORE_COUNT_REG_1, PERFCOUNTEREVT_CORE_DATA_CACHE_READ_MISS);
    pf_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_SET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_0, 0);
    pf_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_SET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_1, 0);
    pf_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_SET_VALUE, PERFCOUNTERREG_CORE_CYCLE_COUNTER, 0);
    unsigned t0 = pm_ticks(); /* cold, once at init: the mean tick read cost for the cal line */
    for (int i = 0; i < 200; i++) pm_ticks();
    unsigned dt = pm_ticks() - t0;
    unsigned p0 = pm_ticks();
    for (int i = 0; i < 50; i++) pf_rc = svcControlPerformanceCounter(&out, PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_0, 0);
    unsigned p1 = pm_ticks();
    unsigned tick_c = (unsigned)((unsigned long long)dt * 100000ull / (200ull * 268112ull)); /* us x 100 per read */
    unsigned pmu_c = (unsigned)((unsigned long long)(p1 - p0) * 100000ull / (50ull * 268112ull));
    unsigned base_c = 6u * pmu_c / pf_every; /* two 3-register reads per N frames */
    { static int c2_done; if (!c2_done) { c2_done = 1; c2_probe(); } } /* PERF1: the core-1 load probe is gone (it spun on the GSP and DSP core); the core 2 probe line stays */
    pm_log_prof_line("prof cal every %u set %c tick %u.%02u us pmu %u.%02u us base %u.%02u us rc %08x", pf_every, pf_set, tick_c / 100u, tick_c % 100u,
             pmu_c / 100u, pmu_c % 100u, base_c / 100u, base_c % 100u, (unsigned)pf_rc);
    pf_svc0 = pm_svc_n; /* PROF2-1: the svc count starts after the calibration reads */
}

int pm_debug_mode(const char* key); /* CFG-1, pm_log.c */
static void pf_init(void) {
    char v[16];
    pf_want = pm_debug_mode("prof") == 1; /* CFG-1: prof=off or prof=0 samples nothing */
    if (pm_debug_value("prof", v, sizeof v)) { unsigned n = (unsigned)atoi(v); if (n >= 4 && n <= 255) pf_every = n; }
    pf_set = pm_debug_has("profstall") ? 's' : 'm';
    pm_pf_on = pf_want && pm_log_file();
    pf_cd = pf_every - 1;
    if (pm_pf_on) pf_start();
}

/* Phase timing needs the switch (ph_want) and the log (pm_log_file). Logging off: pm_ph_on is 0, so no tick is read and no line is formed. */
static int ph_want, dcs_want;
int pm_dcs_on; /* DS0: switch dcsum. Only the sampled draw and flush timers and the dcsum line; no other phase site */
void pm_ph_log_gate(int on) {
    pm_ph_on = on && ph_want;
    pm_ph_cen = pm_ph_on;
    pm_cen_lines = on && (pm_ph_cen || cen_want);
    pm_pf_on = on && pf_want;
    pm_pf_arm = pm_pf_on && !ph_want;
    if (!on) pf_samp = 0;
    pm_dcs_on = on && dcs_want;
    pm_ph_fine = pm_ph_on && pm_debug_has("phasegbi");
    if (on && fs_want && !pm_fs_on) fs_arm();
    pm_fs_on = on && fs_want;
    pm_walk_dbg_set();
}

/* PROFROW: the phase timers start from now (boot, and a row change to phase). */
static void ph_start(void) {
    last = frame_t0 = win_t0 = pm_ticks();
    memset(pm_ph_cnt, 0, sizeof pm_ph_cnt);
    memset(pm_cs_cnt, 0, sizeof pm_cs_cnt);
    pm_gbi_sync_ticks = 0;
    pm_gbi_gv_ticks = 0;
    pm_gbi_tex_ticks = 0;
}

void pm_ph_init(void) {
    char rv[40];
    ps_on = pm_debug_has("phsum");
    ph_want = pm_debug_mode("phase") == 1 || ps_on; /* CFG-1: phase=off or phase=0 turns the phase timers off */
    pm_ph_on = ph_want && pm_log_file();
    pm_ph_cen = pm_ph_on;
    cen_want = pm_debug_has("census");
    pm_cen_lines = pm_log_file() && (pm_ph_cen || cen_want);
    dcs_want = pm_debug_has("dcsum");
    pm_dcs_on = dcs_want && pm_log_file();
    ps_init(); /* only parses the abtog switch, so it runs even while logging is off */
    if (ph_want || ps_on) cold_need();
    pm_ph_fine = pm_ph_on && pm_debug_has("phasegbi");
    fs_want = pm_debug_has("fsplit");
    pm_fs_on = fs_want && pm_log_file();
    if (pm_fs_on) fs_arm();
    pmu_init();
    pf_init();
    pm_pf_arm = pm_pf_on && !ph_want;
    pm_walk_dbg_set();
    if (!pm_ph_on) return;
    /* cost of the switch lookup that the game path makes per call (strstr over the switch string) */
    unsigned t0 = pm_ticks();
    int hit = 0;
    for (int i = 0; i < 2000; i++) hit += pm_debug_has("nofog");
    unsigned dt = pm_ticks() - t0;
    /* cost of one tick read */
    unsigned t1 = pm_ticks();
    unsigned x = 0;
    for (int i = 0; i < 2000; i++) x += pm_ticks();
    unsigned dt2 = pm_ticks() - t1;
    pm_log_d("phase: on. pm_debug_has costs %.3f us per call (hit %d). A tick read costs %.3f us (x %u)", (double)dt / 2000.0 / 268.112, hit,
           (double)dt2 / 2000.0 / 268.112, x & 1);
    ph_start();
}

void pm_ph_push(int id) {
    if (pm_on_audio()) return; /* AU1c: one phase stack, owned by the game thread */
    unsigned now = pm_ticks();
    acc[cur] += now - last;
    last = now;
    if (depth < 16) stk[depth++] = cur;
    cur = id;
}

void pm_ph_pop(void) {
    if (pm_on_audio()) return;
    unsigned now = pm_ticks();
    acc[cur] += now - last;
    last = now;
    cur = depth ? stk[--depth] : PH_OTHER;
}

static void sort(float* a, int n) {
    for (int i = 1; i < n; i++) {
        float v = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > v) { a[j + 1] = a[j]; j--; }
        a[j + 1] = v;
    }
}

static void stat3(const float* v, int n, float* avg, float* p95, float* mx) {
    float t[MAXW], s = 0.f;
    for (int i = 0; i < n; i++) { t[i] = v[i]; s += v[i]; }
    sort(t, n);
    *avg = s / (float)n;
    *p95 = t[(n * 95 + 99) / 100 - 1];
    *mx = t[n - 1];
}

static void summarize(unsigned now) {
    float col[MAXW], a, p, m, wa, wp, wm;
    int n = nwin;
    unsigned over = 0, overcpu = 0;
    char line[240];
    int len;
    for (int i = 0; i < n; i++) col[i] = cold->win[i].wall;
    stat3(col, n, &wa, &wp, &wm);
    for (int i = 0; i < n; i++) col[i] = cold->win[i].wall - cold->win[i].ms[PH_IDLE] - cold->win[i].ms[PH_FBEGIN];
    stat3(col, n, &a, &p, &m);
    for (int i = 0; i < n; i++) if (cold->win[i].wall > OVER_MS) over++;
    for (int i = 0; i < n; i++) if (cold->win[i].wall - cold->win[i].ms[PH_IDLE] - cold->win[i].ms[PH_FBEGIN] > 33.3f) overcpu++;
    pm_log_d("phase W f%u-%u n %d span %.0f ms | wall avg/p95/max %.1f/%.1f/%.1f | cpu (wall-idle-fbegin) %.1f/%.1f/%.1f | wall over34 %u, cpu over33 %u",
           cold->win[0].f, cold->win[n - 1].f, n, (double)(now - win_t0) / TICKS_MS, wa, wp, wm, a, p, m, over, overcpu);
    for (int g = 0; g < (PH_N + 4) / 5; g++) {
        len = snprintf(line, sizeof line, "phase ms avg/p95/max:");
        for (int k = g * 5; k < g * 5 + 5 && k < PH_N; k++) {
            for (int i = 0; i < n; i++) col[i] = cold->win[i].ms[k];
            stat3(col, n, &a, &p, &m);
            len += snprintf(line + len, sizeof line - (size_t)len, " %s %.1f/%.1f/%.1f", pname[k], a, p, m);
        }
        pm_log_d("%s", line);
    }
    for (int g = 0; g < 2; g++) {
        len = snprintf(line, sizeof line, "phase count avg/max per frame:");
        for (int k = g ? 8 : 0; k < (g ? C_N : 8); k++) {
            unsigned long long s = 0;
            unsigned mx = 0;
            for (int i = 0; i < n; i++) { s += cold->win[i].c[k]; if (cold->win[i].c[k] > mx) mx = cold->win[i].c[k]; }
            len += snprintf(line + len, sizeof line - (size_t)len, " %s %u/%u", cname[k], (unsigned)(s / (unsigned)n), mx);
        }
        pm_log_d("%s", line);
    }
    if (over) {
        len = snprintf(line, sizeof line, "phase over frames:");
        for (int i = 0, k = 0; i < n && k < 24; i++)
            if (cold->win[i].wall > OVER_MS) { len += snprintf(line + len, sizeof line - (size_t)len, " %u(%.0f)", cold->win[i].f, cold->win[i].wall); k++; }
        pm_log_d("%s", line);
        /* the three worst frames, with every phase over 0.5 ms and every counter */
        for (int r = 0; r < 3; r++) {
            int w = -1;
            float best = OVER_MS;
            for (int i = 0; i < n; i++) if (cold->win[i].wall > best) { best = cold->win[i].wall; w = i; }
            if (w < 0) break;
            len = snprintf(line, sizeof line, "phase worst f%u wall %.1f:", cold->win[w].f, cold->win[w].wall);
            for (int k = 0; k < PH_N; k++)
                if (cold->win[w].ms[k] >= 0.5f) len += snprintf(line + len, sizeof line - (size_t)len, " %s %.1f", pname[k], cold->win[w].ms[k]);
            len += snprintf(line + len, sizeof line - (size_t)len, " |");
            for (int k = 0; k < C_N; k++)
                if (cold->win[w].c[k]) len += snprintf(line + len, sizeof line - (size_t)len, " %s %u", cname[k], cold->win[w].c[k]);
            pm_log_d("%s", line);
            cold->win[w].wall = 0.f; /* out of the next pick, the window is reset below */
        }
    }
    nwin = 0;
    win_t0 = now;
}

/* UI: event line for a game menu change (state_pause.c, state_file_select.c, pause_partners.c). Phase gate: no line without the phase switch. */
void pm_ph_ui(const char* what, int v) {
    if (pm_ph_on) pm_log_hw("ui: %s v%d f%u", what, v, frame_no);
}

/* PAUSE5B: cold timing of one pause open, switch fsplit with phase (the counters need phase). pm_pc_begin runs when the pause
 * starts (state_pause.c, BEGIN_PAUSED). The window is PC_FR drawn frames. The steps add their ticks while it is open. One
 * line at the end (pcopen). Without fsplit every call returns at once. */
#define PC_FR 60
enum { PCS_ROM, PCS_INIT, PCS_SETUP, PCS_FILTER, PCS_N };
extern void pm_tex_map_state(unsigned* out_n, unsigned* out_ev);
static int pc_left, pc_mapch, pc_mcur_area, pc_mcur_map;
static unsigned pc_s[PCS_N], pc_last_t, pc_sum_t, pc_max_t, pc_max_f, pc_tex_n, pc_tex_kb, pc_ev0, pc_b0_t, pc_b0_n, pc_begin_f;
static unsigned pc_max_tex, pc_max_kb;
static float pc_ph[PH_N], pc_maxph[PH_N]; /* window sums and the worst frame, ms per phase */

unsigned pm_pc_t(void) { return pm_fs_on ? pm_ticks() : 0u; }
void pm_pc_add(int k, unsigned t0) { if (pc_left && t0) pc_s[k] += pm_ticks() - t0; }

void pm_pc_begin(void) {
    unsigned n = 0, ev = 0;
    if (!pm_fs_on || !pm_ph_on) return;
    memset(pc_s, 0, sizeof pc_s);
    memset(pc_ph, 0, sizeof pc_ph);
    memset(pc_maxph, 0, sizeof pc_maxph);
    pc_sum_t = pc_max_t = pc_max_f = pc_tex_n = pc_tex_kb = pc_mapch = pc_max_tex = pc_max_kb = 0;
    pc_last_t = pm_ticks();
    pc_b0_t = pm_b7.cap_ticks; pc_b0_n = pm_b7.cap_n;
    pc_begin_f = frame_no;
    pm_tex_map_state(&n, &ev);
    pc_ev0 = ev;
    pc_mcur_area = cur_area; pc_mcur_map = cur_map;
    pc_left = PC_FR;
}

static __attribute__((cold, noinline)) void pc_end(void) {
    unsigned n = 0, ev = 0;
    pm_tex_map_state(&n, &ev);
    pm_log_hw("pcopen f%u: %d frames, ms/frame avg %.2f max %.2f (f%u), window %.1f ms | tex loads %u (%u KB) | evictions %u | map changes %u | readback %.2f ms (%u calls) | filter %.2f ms | setup %.2f ms: rom %.2f ms, pause_init %.2f ms, rest %.2f ms",
              pc_begin_f, PC_FR, (float)pc_sum_t / PC_FR / TICKS_MS, (float)pc_max_t / TICKS_MS, pc_max_f, (float)pc_sum_t / TICKS_MS,
              pc_tex_n, pc_tex_kb, ev - pc_ev0, pc_mapch,
              (float)(pm_b7.cap_ticks - pc_b0_t) / TICKS_MS, pm_b7.cap_n - pc_b0_n,
              (float)pc_s[PCS_FILTER] / TICKS_MS, (float)pc_s[PCS_SETUP] / TICKS_MS, (float)pc_s[PCS_ROM] / TICKS_MS,
              (float)pc_s[PCS_INIT] / TICKS_MS, (float)(pc_s[PCS_SETUP] - pc_s[PCS_ROM] - pc_s[PCS_INIT]) / TICKS_MS);
    pm_log_hw("pcphase: worst f%u %.1f ms, tex loads %u (%u KB) | tex %.1f fbread %.1f gpu %.1f gbi %.1f dl %.1f other %.1f idle %.1f | sums ms: tex %.1f fbread %.1f gpu %.1f gbi %.1f dl %.1f other %.1f",
              pc_max_f, (float)pc_max_t / TICKS_MS, pc_max_tex, pc_max_kb,
              pc_maxph[PH_TEX], pc_maxph[PH_FBREAD], pc_maxph[PH_GPU], pc_maxph[PH_GBI], pc_maxph[PH_DL], pc_maxph[PH_OTHER], pc_maxph[PH_IDLE],
              pc_ph[PH_TEX], pc_ph[PH_FBREAD], pc_ph[PH_GPU], pc_ph[PH_GBI], pc_ph[PH_DL], pc_ph[PH_OTHER]);
    pm_log_hw("pcgbi: worst frame gbi parts ms: gvtx %.1f gtri %.1f gmtx %.1f gop %.1f gtev %.1f gbind %.1f gst %.1f (phasegbi only)",
              pc_maxph[PH_GVTX], pc_maxph[PH_GTRI], pc_maxph[PH_GMTX], pc_maxph[PH_GOP], pc_maxph[PH_GTEV], pc_maxph[PH_GBIND], pc_maxph[PH_GST]);
}

/* one drawn frame of the window: the frame record r (ms per phase, counters) is complete */
static __attribute__((cold, noinline)) void pc_frame(unsigned now, const Rec* r) {
    unsigned dt = now - pc_last_t;
    pc_last_t = now;
    pc_sum_t += dt;
    for (int k = 0; k < PH_N; k++) pc_ph[k] += r->ms[k];
    if (dt > pc_max_t) {
        pc_max_t = dt; pc_max_f = frame_no;
        for (int k = 0; k < PH_N; k++) pc_maxph[k] = r->ms[k];
        pc_max_tex = r->c[C_TEXMISS]; pc_max_kb = r->c[C_TEXBYTE];
    }
    pc_tex_n += r->c[C_TEXMISS];
    pc_tex_kb += r->c[C_TEXBYTE];
    if (cur_area != pc_mcur_area || cur_map != pc_mcur_map) { pc_mapch++; pc_mcur_area = cur_area; pc_mcur_map = cur_map; }
    if (--pc_left == 0) pc_end();
}

/* PROFROW: the Debug tab Profiling row (pm_options.c). Mode 0 off, 1 prof (prof lines only, as prof=30), 2 phase set. The change is applied at the
 * next drawn frame end (pm_ph_frame_end), so no phase push is open while a gate changes. Ceiling: the prof counters stay enabled after prof
 * mode (no svc call stops them); pm_pmu_on stays 0 after a prof row, so a launch pmu word is not restored. */
static int prof_pend = -1;
void pm_prof_row_set(int mode) { prof_pend = mode; }
void pm_log_prof_row(int mode); /* PROFROW, pm_log.c: LOG5 saves and restores the levels; Profiling turns logging off */
static void prof_row_apply(int m) {
    int pf_was = pm_pf_on;
    pm_log_prof_row(m);
    ph_want = m == 2 || ps_on;
    if (ph_want) cold_need();
    pf_want = m == 1;
    if (m == 1) pf_every = 30;
    pf_cd = pf_every - 1;
    pm_ph_log_gate(pm_log_file());
    if (!pm_pf_on) pf_samp = 0;
    if (pm_pf_on && !pf_was) { pf_reset(); pf_start(); }
    if (pm_ph_on) ph_start();
}

void pm_ph_frame_end(void) {
    if (prof_pend >= 0) { int m = prof_pend; prof_pend = -1; prof_row_apply(m); }
    if (pm_pmu_on) pmu_frame_end();
    if (pm_fs_on) fs_frame_end();
    if (!pm_ph_on) return;
    unsigned now = pm_ticks();
    Rec* r = &cold->win[nwin];
    acc[cur] += now - last;
    last = now;
    r->wall = (float)(now - frame_t0) / TICKS_MS;
    for (int k = 0; k < PH_N; k++) { r->ms[k] = (float)acc[k] / TICKS_MS; acc[k] = 0; }
    memcpy(r->c, pm_ph_cnt, sizeof r->c);
    if (pc_left) pc_frame(now, r);
    memcpy(prev_cnt, pm_ph_cnt, sizeof prev_cnt);
    memset(pm_ph_cnt, 0, sizeof pm_ph_cnt);
    r->f = ++frame_no;
    frame_t0 = now;
    pm_ph_kick_n = pm_ph_kick_us = pm_ph_hud_us = 0;
    memset(pm_fl_n, 0, sizeof pm_fl_n); memset(pm_fl_e, 0, sizeof pm_fl_e); memset(pm_fl_us, 0, sizeof pm_fl_us);
    pm_ec_n = pm_ec_svc = 0;
    memset(pm_cs_cnt, 0, sizeof pm_cs_cnt);
    pm_gbi_sync_ticks = 0;
    pm_gbi_gv_ticks = 0;
    pm_gbi_tex_ticks = 0;
    if (pf_samp) { pf_take(r); return; } /* PF2: a sampled frame is not a phase window row */
    if (ps_on) ps_step(r);
    if (++nwin >= MAXW || (now - win_t0) >= 536223712u /* 2 s */) summarize(now);
}

/* Linker wraps (CMakeLists.txt): the game functions keep their code. These only start and stop a phase around them. */
extern void __real_step_game_loop(void);
extern void __real_gfx_task_background(void);
extern void __real_gfx_draw_frame(void);

unsigned pm_step_calls; /* PC01: game logic steps, counted always (not only with the phase switch) */
void __wrap_step_game_loop(void) {
    pm_step_calls++;
    pm_interp_start();
    PH_PUSH(PH_LOGIC);
    FS_MARK(FS_LOGIC);
    __real_step_game_loop();
    FS_MARK(FS_DL);
    PH_POP();
}

void __wrap_gfx_task_background(void) {
    PH_PUSH(PH_DL);
    __real_gfx_task_background();
    PH_POP();
}

void __wrap_gfx_draw_frame(void) {
    PH_PUSH(PH_DL);
    __real_gfx_draw_frame();
    PH_POP();
}

/* MAGI fspk (debug tool, switch fspk): per-frame lines around a game event (pm_fspk_mark, e.g. the Magikoopa spell hit). A ring keeps
 * the last FSPK_PRE frames; the mark prints them, then FSPK_POST more frames, then "fspk: done". Line: frame, step ms (Azahar: direction
 * only), draws, triangles, TEV rebuilds, texture binds of the last finished frame, and the per-frame deltas of texture misses, uploaded
 * KB, entity key compiles, static G_DL lists, walker-drawn entity segments and model cache misses. Frames over 34 ms carry " o34". */
#define FSPK_PRE 16
#define FSPK_POST 48
typedef struct { unsigned f, dr, tri, tevr, tb, txm, txkb, entc, gdl, segi, mdlm; float ms; float fs[FS_N]; } FspkRec;
static unsigned long long fspk_fs0[FS_N]; /* fsplit sums at the last frame (switch fsplit): per-frame parts */
static FspkRec fspk_ring[FSPK_PRE];
static unsigned fspk_n, fspk_left, fspk_hit, fspk_o34, fspk_maxf;
static float fspk_max;
static int fspk_on = -1;
static unsigned fspk_prev[6];
extern unsigned pm_gpu_fr_last[4];
void pm_tex_tot(unsigned* miss, unsigned* up);
void nd_ent_stat(unsigned o[4]);
void pm_gpu_mdl_stat(unsigned* miss, unsigned* clears);
static void fspk_print(const FspkRec* r) {
    pm_log_evt("fspk f%u ms %.1f dr %u tri %u tevr %u tb %u txm %u txkb %u entc %u gdl %u segi %u mdlm %u%s", r->f, (double)r->ms, r->dr, r->tri, r->tevr, r->tb,
               r->txm, r->txkb, r->entc, r->gdl, r->segi, r->mdlm, r->ms > 34.f ? " o34" : "");
    if (pm_fs_on) pm_log_evt("fspk f%u split logic %.1f dl %.1f gbi %.1f flush %.1f gwait %.1f swap %.1f audio %.1f rest %.1f", r->f, (double)r->fs[FS_LOGIC], (double)r->fs[FS_DL],
                             (double)r->fs[FS_GBI], (double)r->fs[FS_FLUSH], (double)r->fs[FS_GWAIT], (double)r->fs[FS_SWAP], (double)r->fs[FS_AUDIO], (double)r->fs[FS_REST]);
}
void pm_fspk_mark(const char* what) {
    if (fspk_on <= 0 || fspk_left) return;
    fspk_hit = pm_frame_count();
    pm_log_evt("fspk: mark %s f%u", what, fspk_hit);
    unsigned k0 = fspk_n > FSPK_PRE ? fspk_n - FSPK_PRE : 0;
    for (unsigned k = k0; k < fspk_n; k++) fspk_print(&fspk_ring[k % FSPK_PRE]);
    fspk_o34 = 0; fspk_max = 0.f; fspk_maxf = 0;
    fspk_left = FSPK_POST;
    if (pm_debug_has("shots")) { extern void pm_gpu_shot_window(unsigned from, unsigned n); pm_gpu_shot_window(fspk_hit + 4, 6); } /* the effect at the hit, and the 16 bit battle-heap texture frames (hit + 4, hit + 8) */
}
void pm_fspk_frame(unsigned f, float ms) {
    if (fspk_on < 0) fspk_on = pm_debug_has("fspk");
    if (!fspk_on) return;
    unsigned c[6], e[4], mm, cl;
    pm_tex_tot(&c[0], &c[1]); nd_ent_stat(e); pm_gpu_mdl_stat(&mm, &cl);
    c[1] >>= 10; c[2] = e[0]; c[3] = e[1]; c[4] = e[3]; c[5] = mm;
    FspkRec r = { f, pm_gpu_fr_last[0], pm_gpu_fr_last[3], pm_gpu_fr_last[1], pm_gpu_fr_last[2],
                  c[0] - fspk_prev[0], c[1] - fspk_prev[1], c[2] - fspk_prev[2], c[3] - fspk_prev[3], c[4] - fspk_prev[4], c[5] - fspk_prev[5], ms };
    memcpy(fspk_prev, c, sizeof c);
    for (int k = 0; k < FS_N; k++) { /* the window reset clears fs_sum: then the sum itself is this frame's part */
        unsigned long long d = fs_sum[k] >= fspk_fs0[k] ? fs_sum[k] - fspk_fs0[k] : fs_sum[k];
        r.fs[k] = (float)((double)d / TICKS_MS); fspk_fs0[k] = fs_sum[k];
    }
    if (!fspk_left) { fspk_ring[fspk_n++ % FSPK_PRE] = r; return; }
    fspk_print(&r);
    if (ms > 34.f) fspk_o34++;
    if (ms > fspk_max) { fspk_max = ms; fspk_maxf = f; }
    if (--fspk_left == 0) { pm_log_evt("fspk: done hit f%u frames %u o34 %u max %.1f ms f%u", fspk_hit, FSPK_POST, fspk_o34, (double)fspk_max, fspk_maxf); fspk_n = 0; }
}
int pm_fspk_active(void) { return fspk_left != 0; } /* MAGI: inside the window after the mark (producer debug lines) */
