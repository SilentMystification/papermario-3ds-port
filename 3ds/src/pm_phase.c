/* PT01: per-frame phase timer (measurement only, port layer). Switch `phase`. See pm_port.h.
 * Reads the ARM11 tick through pm_ticks (268111856 Hz). One drawn frame = from the end of the previous drawn frame to the
 * end of this one, so the phases add up to the wall time. Output goes through pm_log (non-blocking queue):
 *   every ~2 s: wall/work, per phase avg/p95/max, counters avg/max, frames over 34 ms (frame numbers), 3 worst frames.
 *   switch `phasecsv`: one line per drawn frame (analysis runs only). */
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

static const char* const pname[PH_N] = { "other", "logic", "dl", "gbi", "gpu", "tex", "audio", "rom", "yay0", "fbegin", "fend", "idle", "gcache", "gdraw", "g_vtx", "g_tri", "g_mtx", "g_op", "fbread", "g_tev", "g_bind", "g_state" };
static const char* const cname[C_N] = { "tri", "triout", "vtxload", "vtxslot", "draw", "flush", "texcall", "texmiss", "texKB", "romn", "romKB", "yayn", "yayKB", "dbgcall", "atick", "gbiops", "ajob", "gvload", "gvtri", "gvrej", "gvfb", "gvxf", "flcall", "flkb", "rawd", "rawcall" };

static int stk[16], depth, cur;
static unsigned last, frame_t0, acc[PH_N];
static int csv, csv_from, csv_to = 0x7fffffff;
static unsigned frame_no;

typedef struct { float ms[PH_N]; float wall; unsigned c[C_N]; unsigned f; } Rec;
static Rec spike_rec;
/* PT15: one line for a step over 50 ms: the phases of the step above 2 ms (only with the phase switch), texture misses and ROM reads. */
void pm_ph_spike(unsigned frame, float ms, unsigned vb, unsigned ticks) {
    char b[200];
    int n = 0;
    if (pm_ph_on) {
        for (int k = 0; k < PH_N && n < (int)sizeof b - 24; k++)
            if (k != PH_IDLE && spike_rec.ms[k] >= 2.f) n += snprintf(b + n, sizeof b - n, " %s %.1f", pname[k], spike_rec.ms[k]);
    }
    b[n] = 0;
    pm_log_hw("spike f %u ms %.1f vb %u ticks %u texmiss %u/%uKB rom %u/%uB ph%s", frame, ms, vb, ticks, spike_rec.c[C_TEXMISS], spike_rec.c[C_TEXBYTE] >> 10, spike_rec.c[C_ROMN], spike_rec.c[C_ROMB], pm_ph_on ? b : " off");
}
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
static const char* const ab_words[] = { "base", "gpuvtx", "nodr1", "nokick", "notx1", "phasegbi", "gv1", "nogv1", "raw1", "noraw1", "nofl1" };
static int ab_st[BK_N], ab_n, ab_every, ab_cur, ab_steps, ab_skip, ab_seq;
static int sc_mode = -1, sc_area = -1, sc_map = -1; /* the scene of the open buckets */
static int cur_mode = -1, cur_area = -1, cur_map = -1; /* set by the game hooks */
unsigned pm_ph_gpu_us[2];
unsigned pm_ph_cmd_pm;

void pm_ph_scene_mode(int mode) { cur_mode = mode; }
void pm_ph_scene_map(int area, int map) { cur_area = area; cur_map = map; }
void pm_ph_scene_get(int* mode, int* area, int* map) { *mode = cur_mode; *area = cur_area; *map = cur_map; }
void pm_ph_gpu_note(void) {
    if (!pm_ph_on) return;
    pm_ph_gpu_us[0] = (unsigned)(C3D_GetDrawingTime() * 1000.f);
    pm_ph_gpu_us[1] = (unsigned)(C3D_GetProcessingTime() * 1000.f);
}

int pm_ph_abtog_has(const char* w) {
    char v[96];
    if (!pm_debug_value("abtog", v, sizeof v)) return 0;
    char* c = strchr(v, ':');
    if (c) *c = 0;
    const char* p = v;
    size_t n = strlen(w);
    while ((p = strstr(p, w)) != NULL) {
        if ((p == v || p[-1] == ',') && (p[n] == 0 || p[n] == ',')) return 1;
        p += n;
    }
    return 0;
}

static void ab_apply(int st, int announce) {
    /* switch off the words of the old state, then the new one. The generation counter makes every PMDL cache see it. */
    for (int i = 1; i < (int)(sizeof ab_words / sizeof *ab_words); i++) pm_log_switch(ab_words[i], 0);
    if (st > 0) pm_log_switch(ab_words[st], 1);
    pm_ph_fine = pm_ph_on && (st == 5 || pm_debug_has("phasegbi"));
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

void pm_ph_init(void) {
    char rv[40];
    ps_on = pm_debug_has("phsum");
    pm_ph_on = pm_debug_has("phase") || ps_on;
    csv = pm_debug_has("phasecsv");
    if (pm_debug_value("phasecsv", rv, sizeof rv)) { /* HWM1: phasecsv=<from>:<to>, pc lines only for these drawn steps */
        csv = 1;
        csv_from = atoi(rv);
        const char* c = strchr(rv, ':');
        if (c) csv_to = atoi(c + 1);
    }
    pm_ph_fine = pm_ph_on && pm_debug_has("phasegbi");
    if (!pm_ph_on) return;
    ps_init();
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

void pm_ph_frame_end(void) {
    if (!pm_ph_on) return;
    unsigned now = pm_ticks();
    Rec* r = &win[nwin];
    acc[cur] += now - last;
    last = now;
    r->wall = (float)(now - frame_t0) / TICKS_MS;
    for (int k = 0; k < PH_N; k++) { r->ms[k] = (float)acc[k] / TICKS_MS; acc[k] = 0; }
    memcpy(r->c, pm_ph_cnt, sizeof r->c);
    memset(pm_ph_cnt, 0, sizeof pm_ph_cnt);
    r->f = ++frame_no;
    spike_rec = *r; /* PT15: the spike line in pm_main.c reads the phases of the step that just ended */
    frame_t0 = now;
    if (ps_on) ps_step(r);
    if (csv && r->f >= (unsigned)csv_from && r->f <= (unsigned)csv_to) {
        pm_log_d("pc %u w %d i %d lo %d dl %d gb %d gp %d tx %d au %d ro %d y %d fb %d fe %d ot %d gc %d gd %d gv %d gt %d gm %d go %d | %u %u %u %u %u %u %u %u %u %u %u %u %u %u", r->f, (int)(r->wall * 10.f),
               (int)(r->ms[PH_IDLE] * 10.f), (int)(r->ms[PH_LOGIC] * 10.f), (int)(r->ms[PH_DL] * 10.f), (int)(r->ms[PH_GBI] * 10.f),
               (int)(r->ms[PH_GPU] * 10.f), (int)(r->ms[PH_TEX] * 10.f), (int)(r->ms[PH_AUDIO] * 10.f), (int)(r->ms[PH_ROM] * 10.f),
               (int)(r->ms[PH_YAY0] * 10.f), (int)(r->ms[PH_FBEGIN] * 10.f), (int)(r->ms[PH_FEND] * 10.f), (int)(r->ms[PH_OTHER] * 10.f),
               (int)(r->ms[PH_GCACHE] * 10.f), (int)(r->ms[PH_GDRAW] * 10.f), (int)(r->ms[PH_GVTX] * 10.f), (int)(r->ms[PH_GTRI] * 10.f), (int)(r->ms[PH_GMTX] * 10.f), (int)(r->ms[PH_GOP] * 10.f), r->c[C_TRIOUT], r->c[C_VTXSLOT], r->c[C_DRAW], r->c[C_TEXMISS], r->c[C_TEXBYTE], r->c[C_ROMN], r->c[C_ROMB], r->c[C_ATICK], r->c[C_FLUSH], r->c[C_DBG], r->c[C_CMD], r->c[C_AJOB], r->c[C_RAWD], r->c[C_RAWCALL]);
        pm_log_hw("pcx %u rd %d tev %d bind %d st %d gpu %u %u cmd %u flushcalls %u flushkb %u", r->f, (int)(r->ms[PH_FBREAD] * 10.f), (int)(r->ms[PH_GTEV] * 10.f), (int)(r->ms[PH_GBIND] * 10.f),
                  (int)(r->ms[PH_GST] * 10.f), pm_ph_gpu_us[0], pm_ph_gpu_us[1], pm_ph_cmd_pm, r->c[C_FLCALL], r->c[C_FLKB]);
    }
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
    __real_step_game_loop();
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
