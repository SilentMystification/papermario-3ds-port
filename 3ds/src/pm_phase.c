/* PT01: per-frame phase timer (measurement only, port layer). Switch `phase`. See pm_port.h.
 * Reads the ARM11 tick through pm_ticks (268111856 Hz). One drawn frame = from the end of the previous drawn frame to the
 * end of this one, so the phases add up to the wall time. Output goes through pm_log (non-blocking queue):
 *   every ~2 s: wall/work, per phase avg/p95/max, counters avg/max, frames over 34 ms (frame numbers), 3 worst frames. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <citro3d.h>
#include "pm_port.h"

#define TICKS_MS 268112.0f
#define MAXW 240
#define OVER_MS 34.0f

int pm_ph_on;
int pm_ph_fine;
unsigned pm_ph_cnt[C_N];
static unsigned prev_cnt[C_N]; /* counters of the last finished frame: pm_ph_cnt is zero again at the next frame begin */
unsigned pm_ph_prev(int k) { return prev_cnt[k]; }
unsigned pm_rs_cnt[RS_N];
unsigned pm_cs_cnt[CS_N];

static const char* const pname[PH_N] = { "other", "logic", "dl", "gbi", "gpu", "tex", "audio", "rom", "yay0", "fbegin", "fend", "idle", "gcache", "gdraw", "g_vtx", "g_tri", "g_mtx", "g_op", "fbread", "g_tev", "g_bind", "g_state" };
static const char* const cname[C_N] = { "tri", "triout", "vtxload", "vtxslot", "draw", "flush", "texcall", "texmiss", "texKB", "romn", "romKB", "yayn", "yayKB", "dbgcall", "atick", "gbiops", "ajob", "gvload", "gvtri", "gvrej", "gvfb", "gvxf", "flcall", "flkb", "rawd", "rawcall" };

static int stk[16], depth, cur;
static unsigned last, frame_t0, acc[PH_N];
static unsigned frame_no;

typedef struct { float ms[PH_N]; float wall; unsigned c[C_N]; unsigned f; } Rec;
static Rec win[MAXW];
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
static Bucket bk[BK_N];
static int ps_on;
static const char* const ab_words[] = { "base", "phasegbi" }; /* rule 19: a state is added here only for one console comparison and taken out again after it */
static int ab_st[BK_N], ab_n, ab_every, ab_cur, ab_steps, ab_skip, ab_seq;
static int sc_mode = -1, sc_area = -1, sc_map = -1; /* the scene of the open buckets */
static int cur_mode = -1, cur_area = -1, cur_map = -1; /* set by the game hooks */
unsigned pm_ph_gpu_us[2];
unsigned pm_ph_cmd_pm;
unsigned pm_ph_kick_n, pm_ph_kick_us, pm_ph_draws, pm_ph_vb, pm_ph_iv, pm_ph_hud_us; /* PI4, PI5: per frame, reset in pm_ph_frame_end */

void pm_ph_scene_mode(int mode) { cur_mode = mode; }
void pm_ph_scene_map(int area, int map) { cur_area = area; cur_map = map; }
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
    Bucket* b = &bk[i];
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
        Bucket* b = &bk[si];
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

static void fs_frame_end(void) {
    unsigned now = pm_ticks();
    fs_sum[fs_cur] += now - fs_last;
    fs_last = now;
    fs_cur = FS_REST;
    unsigned w = now - fs_t0;
    fs_t0 = now;
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
    if (++fs_n < 300) return;
    /* the rest part: wall minus the sum of the other parts, by construction of the chain (every tick belongs to one part) */
    pm_log_hw("fsplit f%u logic %.1f dl %.1f gbi %.1f flush %.1f gwait %.1f swap %.1f audio %.1f rest %.1f wall %.1f max %.1f over50 %u svc %.1f",
              fs_frame, FSMS(FS_LOGIC), FSMS(FS_DL), FSMS(FS_GBI), FSMS(FS_FLUSH), FSMS(FS_GWAIT), FSMS(FS_SWAP), FSMS(FS_AUDIO), FSMS(FS_REST),
              (float)fs_wall / (float)fs_n / TICKS_MS, (float)fs_max / TICKS_MS, fs_over, (float)(pm_svc_n - fs_svc0) / (float)fs_n);
    memset(fs_sum, 0, sizeof fs_sum);
    fs_wall = 0;
    fs_max = fs_over = fs_n = 0;
    fs_svc0 = pm_svc_n;
}

/* Phase timing needs the switch (ph_want) and the log (pm_log_file). Logging off: pm_ph_on is 0, so no tick is read and no line is formed. */
static int ph_want, dcs_want;
int pm_dcs_on; /* DS0: switch dcsum. Only the sampled draw and flush timers and the dcsum line; no other phase site */
void pm_ph_log_gate(int on) {
    pm_ph_on = on && ph_want;
    pm_dcs_on = on && dcs_want;
    pm_ph_fine = pm_ph_on && pm_debug_has("phasegbi");
    if (on && fs_want && !pm_fs_on) fs_arm();
    pm_fs_on = on && fs_want;
}

void pm_ph_init(void) {
    char rv[40];
    ps_on = pm_debug_has("phsum");
    ph_want = pm_debug_has("phase") || ps_on;
    pm_ph_on = ph_want && pm_log_file();
    dcs_want = pm_debug_has("dcsum");
    pm_dcs_on = dcs_want && pm_log_file();
    ps_init(); /* only parses the abtog switch, so it runs even while logging is off */
    pm_ph_fine = pm_ph_on && pm_debug_has("phasegbi");
    fs_want = pm_debug_has("fsplit");
    pm_fs_on = fs_want && pm_log_file();
    if (pm_fs_on) fs_arm();
    if (!pm_ph_on) return;
    /* cost of the switch lookup that the game path makes per call (strstr over the switch string) */
    unsigned t0 = pm_ticks();
    int hit = 0;
    for (int i = 0; i < 2000; i++) hit += pm_debug_has("rmlog");
    unsigned dt = pm_ticks() - t0;
    /* cost of one tick read */
    unsigned t1 = pm_ticks();
    unsigned x = 0;
    for (int i = 0; i < 2000; i++) x += pm_ticks();
    unsigned dt2 = pm_ticks() - t1;
    pm_log_d("phase: on. pm_debug_has costs %.3f us per call (hit %d). A tick read costs %.3f us (x %u)", (double)dt / 2000.0 / 268.112, hit,
           (double)dt2 / 2000.0 / 268.112, x & 1);
    last = frame_t0 = win_t0 = pm_ticks();
    memset(pm_ph_cnt, 0, sizeof pm_ph_cnt);
    memset(pm_rs_cnt, 0, sizeof pm_rs_cnt);
    memset(pm_cs_cnt, 0, sizeof pm_cs_cnt);
    pm_gbi_sync_ticks = 0;
    pm_gbi_gv_ticks = 0;
    pm_gbi_tex_ticks = 0;
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
    for (int i = 0; i < n; i++) col[i] = win[i].wall;
    stat3(col, n, &wa, &wp, &wm);
    for (int i = 0; i < n; i++) col[i] = win[i].wall - win[i].ms[PH_IDLE] - win[i].ms[PH_FBEGIN];
    stat3(col, n, &a, &p, &m);
    for (int i = 0; i < n; i++) if (win[i].wall > OVER_MS) over++;
    for (int i = 0; i < n; i++) if (win[i].wall - win[i].ms[PH_IDLE] - win[i].ms[PH_FBEGIN] > 33.3f) overcpu++;
    pm_log_d("phase W f%u-%u n %d span %.0f ms | wall avg/p95/max %.1f/%.1f/%.1f | cpu (wall-idle-fbegin) %.1f/%.1f/%.1f | wall over34 %u, cpu over33 %u",
           win[0].f, win[n - 1].f, n, (double)(now - win_t0) / TICKS_MS, wa, wp, wm, a, p, m, over, overcpu);
    for (int g = 0; g < (PH_N + 4) / 5; g++) {
        len = snprintf(line, sizeof line, "phase ms avg/p95/max:");
        for (int k = g * 5; k < g * 5 + 5 && k < PH_N; k++) {
            for (int i = 0; i < n; i++) col[i] = win[i].ms[k];
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
            for (int i = 0; i < n; i++) { s += win[i].c[k]; if (win[i].c[k] > mx) mx = win[i].c[k]; }
            len += snprintf(line + len, sizeof line - (size_t)len, " %s %u/%u", cname[k], (unsigned)(s / (unsigned)n), mx);
        }
        pm_log_d("%s", line);
    }
    if (over) {
        len = snprintf(line, sizeof line, "phase over frames:");
        for (int i = 0, k = 0; i < n && k < 24; i++)
            if (win[i].wall > OVER_MS) { len += snprintf(line + len, sizeof line - (size_t)len, " %u(%.0f)", win[i].f, win[i].wall); k++; }
        pm_log_d("%s", line);
        /* the three worst frames, with every phase over 0.5 ms and every counter */
        for (int r = 0; r < 3; r++) {
            int w = -1;
            float best = OVER_MS;
            for (int i = 0; i < n; i++) if (win[i].wall > best) { best = win[i].wall; w = i; }
            if (w < 0) break;
            len = snprintf(line, sizeof line, "phase worst f%u wall %.1f:", win[w].f, win[w].wall);
            for (int k = 0; k < PH_N; k++)
                if (win[w].ms[k] >= 0.5f) len += snprintf(line + len, sizeof line - (size_t)len, " %s %.1f", pname[k], win[w].ms[k]);
            len += snprintf(line + len, sizeof line - (size_t)len, " |");
            for (int k = 0; k < C_N; k++)
                if (win[w].c[k]) len += snprintf(line + len, sizeof line - (size_t)len, " %s %u", cname[k], win[w].c[k]);
            pm_log_d("%s", line);
            win[w].wall = 0.f; /* out of the next pick, the window is reset below */
        }
    }
    nwin = 0;
    win_t0 = now;
}

/* UI: event line for a game menu change (state_pause.c, state_file_select.c, pause_partners.c). Phase gate: no line without the phase switch. */
void pm_ph_ui(const char* what, int v) {
    if (pm_ph_on) pm_log_hw("ui: %s v%d f%u", what, v, frame_no);
}

void pm_ph_frame_end(void) {
    if (pm_fs_on) fs_frame_end();
    if (!pm_ph_on) return;
    unsigned now = pm_ticks();
    Rec* r = &win[nwin];
    acc[cur] += now - last;
    last = now;
    r->wall = (float)(now - frame_t0) / TICKS_MS;
    for (int k = 0; k < PH_N; k++) { r->ms[k] = (float)acc[k] / TICKS_MS; acc[k] = 0; }
    memcpy(r->c, pm_ph_cnt, sizeof r->c);
    memcpy(prev_cnt, pm_ph_cnt, sizeof prev_cnt);
    memset(pm_ph_cnt, 0, sizeof pm_ph_cnt);
    r->f = ++frame_no;
    frame_t0 = now;
    pm_ph_kick_n = pm_ph_kick_us = pm_ph_hud_us = 0;
    memset(pm_fl_n, 0, sizeof pm_fl_n); memset(pm_fl_e, 0, sizeof pm_fl_e); memset(pm_fl_us, 0, sizeof pm_fl_us);
    pm_ec_n = pm_ec_svc = 0;
    memset(pm_rs_cnt, 0, sizeof pm_rs_cnt);
    memset(pm_cs_cnt, 0, sizeof pm_cs_cnt);
    pm_gbi_sync_ticks = 0;
    pm_gbi_gv_ticks = 0;
    pm_gbi_tex_ticks = 0;
    if (ps_on) ps_step(r);
    if (++nwin >= MAXW || (now - win_t0) >= 536223712u /* 2 s */) summarize(now);
}

/* Linker wraps (CMakeLists.txt): the game functions keep their code. These only start and stop a phase around them. */
extern void __real_step_game_loop(void);
extern void __real_gfx_task_background(void);
extern void __real_gfx_draw_frame(void);

unsigned pm_step_calls; /* PC01: game logic steps, counted always (not only with the phase switch) */
void __wrap_step_game_loop(void) {
    pm_audio_join_at(2); /* AU1d: game logic may touch the audio state */
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
