/* LB4: hang watchdog and clean process exit after a crash. See docs/plans/crash_exit.md.
 * The monitor thread runs on core 1 at the lowest priority. It polls every 100 ms (the START+SELECT escape) and runs the stall logic every
 * 10th poll (1 s). It uses no lock of the game threads. */
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pm_port.h"
#include "pm_thread.h"
#include "pm_build.h"

extern volatile int pm_io_quiet; /* pm_log.c: set while the app is suspended or asleep */
extern volatile unsigned pm_wd_quiet;

volatile unsigned pm_wd_prog;  /* the game loop adds 1 per iteration and per drawn step */
volatile unsigned pm_wd_evt[4]; /* the last script step: Evt, first line, next line, opcode */
volatile unsigned pm_wd_sub; /* the last numbered call of step_game_loop (src/main_loop.c PM_WDSUB) */
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
    pm_flash_crash_flush(); /* SAVESAFE: the save in RAM reaches save.flash.tmp and the rename, no malloc, under trylock */
    /* The close fsync can block on a sleeping SD card: wait 1 s at most, then end the process (the file may lack its last bytes). */
    pm_step_timed(PM_STEP_EXIT, close_step, NULL, 1000);
    svcExitProcess();
    for (;;) {}
}

static unsigned now_ms(void) { return (unsigned)(svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000ull)); }

/* SLP2: a note from the monitor thread. It appends to crash.txt directly: no log drain (that would end the queued log), no crash
 * state, no fsync. The line is written before anything can block.
 * SLEEP1: while the app sleeps or is suspended (HOME), no note touches the SD card. Notes go to a RAM ring; pm_wd_flush_notes writes
 * them after the wake or the HOME close, from a bounded step. A sleeping system module cannot take an FS call. */
#define WD_RING 2048
static LightLock wd_lock;
static char wd_ring[WD_RING];
static unsigned wd_ring_n, wd_ring_dropped;
volatile unsigned pm_wd_quiet; /* SLEEP1: set on HOME suspend, cleared on restore. Notes stay in RAM while set. */

static void wd_note(const char* buf, int n) {
    if (pm_io_quiet || pm_wd_asleep || pm_wd_quiet || !aptIsActive()) {
        LightLock_Lock(&wd_lock);
        if (wd_ring_n + (unsigned)n <= WD_RING) { memcpy(wd_ring + wd_ring_n, buf, (size_t)n); wd_ring_n += (unsigned)n; }
        else wd_ring_dropped++;
        LightLock_Unlock(&wd_lock);
        return;
    }
    FILE* f = fopen("crash.txt", "a");
    if (!f) return;
    fwrite(buf, 1, (size_t)n, f);
    fclose(f);
}

/* SLEEP1: writes the RAM notes to crash.txt. Call from a bounded step (it touches the SD card), after the gate is open. */
void pm_wd_flush_notes(void) {
    static char out[WD_RING + 80];
    unsigned n, dropped;
    LightLock_Lock(&wd_lock);
    n = wd_ring_n;
    memcpy(out, wd_ring, n);
    dropped = wd_ring_dropped;
    wd_ring_n = 0;
    wd_ring_dropped = 0;
    LightLock_Unlock(&wd_lock);
    if (dropped) n += (unsigned)snprintf(out + n, sizeof out - n, "watchdog: %u notes dropped while quiet (ring full)\n", dropped);
    if (!n) return;
    FILE* f = fopen("crash.txt", "a");
    if (!f) return;
    fwrite(out, 1, n, f);
    fclose(f);
}

/* ESCAPE: START+SELECT held 1 s (10 polls) returns to the HOME Menu, also while the game thread hangs. The poll reads the HID shared memory
 * (the words hidScanInput reads), not hidScanInput, which would write the game thread's key state. Switch noescape turns it off. */
extern int pm_gx_stuck(void);
extern int pm_gpu_hang_dump(char* b, int n);
static unsigned esc_n;
static unsigned wd_gpu_noted; /* the stall limit passed with the GPU stuck: noted once, the process stays */

static void wd_escape(void) {
    int stuck = pm_gx_stuck();
    char buf[400];
    int n = snprintf(buf, sizeof buf, "escape: START+SELECT 1 s, phase %u steps %u stuck %d ", pm_wd_phase, pm_wd_prog, stuck);
    n += pm_gpu_hang_dump(buf + n, (int)sizeof buf - n - 2);
    buf[n++] = '\n';
    buf[n] = 0;
    pm_crash_line(buf, n);
    if (!stuck) pm_exit_process(); /* the clean exit: back to the HOME Menu */
    APT_HardwareResetAsync(); /* GPU stuck: the console reboots to the HOME Menu, no power button (GSPGPU_ResetGpuCore may block here) */
    svcSleepThread(5000000000LL);
    svcExitProcess(); /* last resort */
    for (;;) {}
}

static void escape_poll(unsigned stalled) {
    if (PMDL("noescape") || !hidSharedMem) { esc_n = 0; return; }
    u32 id = hidSharedMem[4] & 7u;
    u32 k = hidSharedMem[10 + id * 4];
    int held = (k & (KEY_START | KEY_SELECT)) == (KEY_START | KEY_SELECT);
    if (!held && stalled >= 3 && PMDL("wdesc")) held = 1; /* test seam: the stall alone counts as the combo after 3 s */
    if (!held) { esc_n = 0; return; }
    if (++esc_n < 10u) return;
    esc_n = 0;
    wd_escape();
}

static void wd_main(void* arg) {
    (void)arg;
    unsigned last = pm_wd_prog, stalled = 0, asleep_s = 0, inact_s = 0, tk = 0;
    for (;;) {
        svcSleepThread(100000000LL);
        escape_poll(stalled);
        if (++tk % 10u) continue; /* the stall logic below runs once per second */
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
        if (stalled >= 2 && p != last) { char sb[80]; int sn = snprintf(sb, sizeof sb, "watchdog: the game thread moved again after %us\n", stalled + 1); wd_note(sb, sn); }
        if (!limit_s || !pm_wd_armed || p != last || pm_wd_hold || !aptIsActive()) { last = p; stalled = 0; wd_gpu_noted = 0; continue; }
        if (stalled >= 2 && !wd_gpu_noted) { /* a stall note every second from the third: crash.txt shows whether the game thread comes back before the limit */
            char sb[200];
            int sn = snprintf(sb, sizeof sb, "watchdog: stall %us phase %u call %u evt %08x op %08x\n", stalled + 1, pm_wd_phase, pm_wd_sub, pm_wd_evt[0], pm_wd_evt[3]);
            wd_note(sb, sn);
        }
        if (++stalled < limit_s) continue;
        if (wd_gpu_noted) continue; /* a stuck GPU: the process stays, only the START+SELECT escape ends it */
        char buf[200];
        int n = snprintf(buf, sizeof(buf), "\nwatchdog: no progress for %u s, game thread last phase %u (%s), steps %u, build #%s\n",
                         stalled, pm_wd_phase, pm_wd_phase_name(pm_wd_phase), pm_frame_count(), PM_BUILD_STR);
        pm_crash_line(buf, n); /* flushes the log queue first, so the last log lines are in log.txt */
        { extern unsigned pm_gbi_cur_op; extern unsigned nd_native_draws, nd_native_refused; extern const char* pm_gpu_last_flush(int*); int fl = 0; const char* ff = pm_gpu_last_flush(&fl);
          n = snprintf(buf, sizeof(buf), "watchdog: step_game_loop call %u, walker op %02x, last flush %s:%d, native draws %u refused %u\n", pm_wd_sub, pm_gbi_cur_op, ff ? ff : "?", fl, nd_native_draws, nd_native_refused); pm_crash_line(buf, n); }
        n = snprintf(buf, sizeof(buf), "watchdog: last script step evt %08x first %08x next %08x op %08x\n", pm_wd_evt[0], pm_wd_evt[1], pm_wd_evt[2], pm_wd_evt[3]); pm_crash_line(buf, n);
        { char hb[320]; int hn = snprintf(hb, sizeof hb, "watchdog: "); /* ESCAPE: the GX queue in flight */
          hn += pm_gpu_hang_dump(hb + hn, (int)sizeof hb - hn - 2); hb[hn++] = '\n'; hb[hn] = 0; pm_crash_line(hb, hn); }
        if (pm_gx_stuck()) {
            wd_gpu_noted = 1;
            n = snprintf(buf, sizeof(buf), "watchdog: GPU stuck, the process stays; hold START+SELECT 1 s to reboot\n");
            pm_crash_line(buf, n);
            continue;
        }
        n = snprintf(buf, sizeof(buf), "watchdog: exiting the process\n");
        pm_crash_line(buf, n);
        pm_exit_process();
    }
}

void pm_wd_start(void) {
    if (wd_th) return;
    LightLock_Init(&wd_lock);
    wd_th = pm_thread_create(wd_main, NULL, 8 * 1024, 0x3F, 1, "wdog");
    pm_log_w("watchdog: %s%u s, monitor thread %s", limit_s ? "" : "off, hang limit ", limit_s, wd_th ? "on core 1" : "NOT STARTED");
}
