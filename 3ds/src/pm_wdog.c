/* LB4: hang watchdog and clean process exit after a crash. See docs/plans/crash_exit.md.
 * The monitor thread runs on core 1 at the lowest priority and sleeps 1 s. It uses no lock of the game threads. */
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pm_port.h"
#include "pm_build.h"

volatile unsigned pm_wd_prog;  /* the game loop adds 1 per iteration and per drawn step */
volatile unsigned pm_wd_phase; /* the last stage the game loop entered, see names below */
volatile unsigned pm_wd_hold;  /* more than 0: a save runs, the game thread may wait for it */
volatile unsigned pm_wd_armed; /* 1 after the first game loop iteration (boot and ROM load are not watched) */
volatile unsigned pm_wd_asleep, pm_wd_sleep_tick, pm_wd_wake_frames, pm_wd_slept; /* SLP2, see pm_port.h */
volatile unsigned pm_wd_crash_tick; /* system tick (low word, ms) when a crash handler started, 0 if none */

static const char* const phase_names[] = {
    "none", "loop top", "log poll and pad scan", "idle wait for vblank", "gpu begin (frame begin)",
    "game step and display list build (gfx retrace)", "audio step", "gpu end (gbi run, submit, frame end)",
    "audio join", "warp and fxtest tick", "hud and test switches",
};
const char* pm_wd_phase_name(unsigned p) { return p < sizeof(phase_names) / sizeof(phase_names[0]) ? phase_names[p] : "?"; }

static unsigned limit_s = 10;
static Thread wd_th;

/* Wait for a running save to end (at most 1 s), close the log file, end the process. No APT, GSP or ndsp call.
 * svcExitProcess stops every thread of the process: audio, flash, render and log writer. */
static void close_step(void* unused) {
    (void)unused;
    pm_log_crash_close();
}

void pm_exit_process(void) {
    for (int i = 0; i < 100 && pm_wd_hold; i++) svcSleepThread(10000000LL);
    /* The close fsync can block on a sleeping SD card: wait 1 s at most, then end the process (the file may lack its last bytes). */
    pm_step_timed(PM_STEP_EXIT, close_step, NULL, 1000);
    svcExitProcess();
    for (;;) {}
}

static unsigned now_ms(void) { return (unsigned)(svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000ull)); }

/* SLP2: a note from the monitor thread. It appends to crash.txt directly: no log drain (that would end the queued log), no crash
 * state, no fsync. The line is written before anything can block. */
static void wd_note(const char* buf, int n) {
    FILE* f = fopen("crash.txt", "a");
    if (!f) return;
    fwrite(buf, 1, (size_t)n, f);
    fclose(f);
}

static void wd_main(void* arg) {
    (void)arg;
    unsigned last = pm_wd_prog, stalled = 0, asleep_s = 0, inact_s = 0;
    for (;;) {
        svcSleepThread(1000000000LL);
        unsigned ct = pm_wd_crash_tick;
        if (ct) { /* a crash handler runs. If it does not end the process in 3 s, it hung or faulted again: end it here. */
            if ((unsigned)(now_ms() - ct) >= 3000u) {
                static const char m[] = "watchdog: the crash handler did not finish in 3 s, ending the process\n";
                pm_crash_line(m, (int)sizeof(m) - 1);
                pm_exit_process();
            }
            continue;
        }
        unsigned p = pm_wd_prog;
        if (pm_wd_asleep) { /* SLP2: a long sleep is not a hang. Note it every 10 s, never end the process: the wake may come later. */
            last = p; stalled = 0; inact_s = 0;
            if (++asleep_s % 10 == 0) {
                char buf[160];
                int n = snprintf(buf, sizeof(buf), "watchdog: asleep %u s (since tick %u ms), no wake yet, game thread phase %u (%s)\n",
                                 asleep_s, pm_wd_sleep_tick, pm_wd_phase, pm_wd_phase_name(pm_wd_phase));
                wd_note(buf, n);
            }
            continue;
        }
        asleep_s = 0;
        /* A suspended app (HOME menu) and a running save are not hangs. The tick does not stop in them, so reset the count. SLP2: the
         * inactive state is still noted every 10 s, so a stuck HOME menu or wake shows in crash.txt. */
        if (pm_wd_armed && !aptIsActive()) {
            if (++inact_s % 10 == 0) {
                char buf[160];
                int n = snprintf(buf, sizeof(buf), "watchdog: app not active for %u s, game thread phase %u (%s)\n",
                                 inact_s, pm_wd_phase, pm_wd_phase_name(pm_wd_phase));
                wd_note(buf, n);
            }
        } else inact_s = 0;
        if (!limit_s || !pm_wd_armed || p != last || pm_wd_hold || !aptIsActive()) { last = p; stalled = 0; continue; }
        if (++stalled < limit_s) continue;
        char buf[200];
        int n = snprintf(buf, sizeof(buf), "\nwatchdog: no progress for %u s, game thread last phase %u (%s), steps %u, build #%s\n",
                         stalled, pm_wd_phase, pm_wd_phase_name(pm_wd_phase), pm_frame_count(), PM_BUILD_STR);
        pm_crash_line(buf, n); /* flushes the log queue first, so the last log lines are in log.txt */
        n = snprintf(buf, sizeof(buf), "watchdog: exiting the process\n");
        pm_crash_line(buf, n);
        pm_exit_process();
    }
}

void pm_wd_start(void) {
    char v[16];
    if (pm_debug_value("wdog", v, sizeof v)) limit_s = (unsigned)atoi(v);
    if (wd_th) return;
    wd_th = threadCreate(wd_main, NULL, 8 * 1024, 0x3F, 1, false);
    pm_log_w("watchdog: %s%u s, monitor thread %s", limit_s ? "" : "off, hang limit ", limit_s, wd_th ? "on core 1" : "NOT STARTED");
}
