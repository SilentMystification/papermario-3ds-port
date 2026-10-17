/* PT01: per-frame phase timer (measurement only, port layer). Switch `phase`. See pm_port.h.
 * Reads the ARM11 tick through pm_ticks (268111856 Hz). One drawn frame = from the end of the previous drawn frame to the
 * end of this one, so the phases add up to the wall time. Output goes through pm_log (non-blocking queue):
 *   every ~2 s: wall/work, per phase avg/p95/max, counters avg/max, frames over 34 ms (frame numbers), 3 worst frames.
 *   switch `phasecsv`: one line per drawn frame (analysis runs only). */
#include <stdio.h>
#include <string.h>
#include "pm_port.h"

#define TICKS_MS 268112.0f
#define MAXW 240
#define OVER_MS 34.0f

int pm_ph_on;
int pm_ph_fine;
unsigned pm_ph_cnt[C_N];

static const char* const pname[PH_N] = { "other", "logic", "dl", "gbi", "gpu", "tex", "audio", "rom", "yay0", "fbegin", "fend", "idle", "gcache", "gdraw", "g_vtx", "g_tri", "g_mtx", "g_op", "fbread" };
static const char* const cname[C_N] = { "tri", "triout", "vtxload", "vtxslot", "draw", "flush", "texcall", "texmiss", "texKB", "romn", "romKB", "yayn", "yayKB", "dbgcall", "atick", "gbiops" };

static int stk[16], depth, cur;
static unsigned last, frame_t0, acc[PH_N];
static int csv;
static unsigned frame_no;

typedef struct { float ms[PH_N]; float wall; unsigned c[C_N]; unsigned f; } Rec;
static Rec win[MAXW];
static int nwin;
static unsigned win_t0;

void pm_ph_init(void) {
    pm_ph_on = pm_debug_has("phase");
    csv = pm_debug_has("phasecsv");
    pm_ph_fine = pm_ph_on && pm_debug_has("phasegbi");
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
    pm_log("phase: on. pm_debug_has costs %.3f us per call (hit %d). A tick read costs %.3f us (x %u)", (double)dt / 2000.0 / 268.112, hit,
           (double)dt2 / 2000.0 / 268.112, x & 1);
    last = frame_t0 = win_t0 = pm_ticks();
    memset(pm_ph_cnt, 0, sizeof pm_ph_cnt);
}

void pm_ph_push(int id) {
    unsigned now = pm_ticks();
    acc[cur] += now - last;
    last = now;
    if (depth < 16) stk[depth++] = cur;
    cur = id;
}

void pm_ph_pop(void) {
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
    pm_log("phase W f%u-%u n %d span %.0f ms | wall avg/p95/max %.1f/%.1f/%.1f | cpu (wall-idle-fbegin) %.1f/%.1f/%.1f | wall over34 %u, cpu over33 %u",
           win[0].f, win[n - 1].f, n, (double)(now - win_t0) / TICKS_MS, wa, wp, wm, a, p, m, over, overcpu);
    for (int g = 0; g < (PH_N + 4) / 5; g++) {
        len = snprintf(line, sizeof line, "phase ms avg/p95/max:");
        for (int k = g * 5; k < g * 5 + 5 && k < PH_N; k++) {
            for (int i = 0; i < n; i++) col[i] = win[i].ms[k];
            stat3(col, n, &a, &p, &m);
            len += snprintf(line + len, sizeof line - (size_t)len, " %s %.1f/%.1f/%.1f", pname[k], a, p, m);
        }
        pm_log("%s", line);
    }
    for (int g = 0; g < 2; g++) {
        len = snprintf(line, sizeof line, "phase count avg/max per frame:");
        for (int k = g ? 8 : 0; k < (g ? C_N : 8); k++) {
            unsigned long long s = 0;
            unsigned mx = 0;
            for (int i = 0; i < n; i++) { s += win[i].c[k]; if (win[i].c[k] > mx) mx = win[i].c[k]; }
            len += snprintf(line + len, sizeof line - (size_t)len, " %s %u/%u", cname[k], (unsigned)(s / (unsigned)n), mx);
        }
        pm_log("%s", line);
    }
    if (over) {
        len = snprintf(line, sizeof line, "phase over frames:");
        for (int i = 0, k = 0; i < n && k < 24; i++)
            if (win[i].wall > OVER_MS) { len += snprintf(line + len, sizeof line - (size_t)len, " %u(%.0f)", win[i].f, win[i].wall); k++; }
        pm_log("%s", line);
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
            pm_log("%s", line);
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
    frame_t0 = now;
    if (csv) {
        pm_log("pc %u w %d i %d lo %d dl %d gb %d gp %d tx %d au %d ro %d y %d fb %d fe %d ot %d gc %d gd %d gv %d gt %d gm %d go %d | %u %u %u %u %u %u %u %u", r->f, (int)(r->wall * 10.f),
               (int)(r->ms[PH_IDLE] * 10.f), (int)(r->ms[PH_LOGIC] * 10.f), (int)(r->ms[PH_DL] * 10.f), (int)(r->ms[PH_GBI] * 10.f),
               (int)(r->ms[PH_GPU] * 10.f), (int)(r->ms[PH_TEX] * 10.f), (int)(r->ms[PH_AUDIO] * 10.f), (int)(r->ms[PH_ROM] * 10.f),
               (int)(r->ms[PH_YAY0] * 10.f), (int)(r->ms[PH_FBEGIN] * 10.f), (int)(r->ms[PH_FEND] * 10.f), (int)(r->ms[PH_OTHER] * 10.f),
               (int)(r->ms[PH_GCACHE] * 10.f), (int)(r->ms[PH_GDRAW] * 10.f), (int)(r->ms[PH_GVTX] * 10.f), (int)(r->ms[PH_GTRI] * 10.f), (int)(r->ms[PH_GMTX] * 10.f), (int)(r->ms[PH_GOP] * 10.f), r->c[C_TRIOUT], r->c[C_VTXSLOT], r->c[C_DRAW], r->c[C_TEXMISS], r->c[C_TEXBYTE], r->c[C_ROMN], r->c[C_ROMB], r->c[C_ATICK]);
    }
    if (++nwin >= MAXW || (now - win_t0) >= 536223712u /* 2 s */) summarize(now);
}

/* Linker wraps (CMakeLists.txt): the game functions keep their code. These only start and stop a phase around them. */
extern void __real_step_game_loop(void);
extern void __real_gfx_task_background(void);
extern void __real_gfx_draw_frame(void);

void __wrap_step_game_loop(void) {
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
