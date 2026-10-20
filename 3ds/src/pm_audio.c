#include <3ds.h>
#include <string.h>
#include <unistd.h>
#include "pm_port.h"

#define CAP 8192
#define SLOTS 8

static int ready;
static unsigned rate = 32000;

/* AU1a: switch audiohash = FNV-1a of every PCM buffer that pm_audio_post receives (before the full queue drop), one log line per 120 posts
 * and one at smoke end. Switch audiolock = exactly 2 ticks per drawn step and osAiGetLength 0, so the hash does not depend on the clock. */
int pm_audio_lock(void) { static int v = -1; if (v < 0) v = pm_debug_has("audiolock"); return v; }
static unsigned au_hash = 2166136261u, au_posts, au_dropped, au_underrun;
void pm_audio_hash_log(void) {
    if (pm_debug_has("audiohash")) pm_log("audiohash posts %u fnv %08x dropped %u underrun %u (final)", au_posts, au_hash, au_dropped, au_underrun);
}
static ndspWaveBuf wb[SLOTS];
static s16* data[SLOTS];

/* HW7: stop the DSP use before libctru frees the heap. The audio thread stays blocked on its event and never runs again. */
void pm_audio_shutdown(void) {
    if (!ready) return;
    ready = 0;
    pm_exit_mark("audio: reset");
    ndspChnReset(0);
    pm_exit_mark("audio: exit");
    ndspExit();
}

void pm_audio_init(void) {
    static int tried;
    if (ready || tried) return; /* fail once, log once, run silent: no ndspInit retry on every audio tick */
    tried = 1;
    int fw = access("sdmc:/3ds/dspfirm.cdc", F_OK) == 0;
    (fw ? pm_log_w : pm_log_e)("dspfirm.cdc %s", fw ? "found" : "MISSING (sdmc:/3ds/dspfirm.cdc): no sound unless the DSP firmware is dumped");
    Result rc = ndspInit();
    if (R_FAILED(rc)) {
        pm_log_e("ndsp init failed rc=%08lx, audio is silent", (unsigned long)rc);
        return;
    }
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(0);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetRate(0, (float)rate);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    for (int i = 0; i < SLOTS; i++) {
        data[i] = (s16*)linearAlloc(CAP);
        memset(&wb[i], 0, sizeof(wb[i]));
        wb[i].status = NDSP_WBUF_DONE;
        if (!data[i]) {
            pm_log_e("audio linearAlloc %d of %d failed, audio is silent", i, SLOTS);
            for (int j = 0; j < i; j++) linearFree(data[j]);
            ndspExit();
            return;
        }
    }
    ready = 1;
    pm_log("ndsp ready %u Hz\n", rate);
}

void pm_audio_post(const void* pcm, unsigned bytes, unsigned hz) {
    if (hz && hz != rate) {
        rate = hz;
        if (ready) ndspChnSetRate(0, (float)rate);
    }
    if (!pcm || bytes < 4) return;
    if (pm_debug_has("audiohash")) {
        const unsigned char* q = (const unsigned char*)pcm;
        for (unsigned i = 0; i < bytes; i++) au_hash = (au_hash ^ q[i]) * 16777619u;
        if (++au_posts % 120 == 0) pm_log("audiohash posts %u fnv %08x dropped %u underrun %u", au_posts, au_hash, au_dropped, au_underrun);
    }
    if (!ready) pm_audio_init();
    if (!ready) return;
    {
        /* Log once per 120 posts: how many were queued and the loudest sample, to check the output in an emulator. */
        static unsigned posts;
        static int peak;
        const s16* p = (const s16*)pcm;
        for (unsigned i = 0; i < bytes / 2; i++) {
            int v = p[i] < 0 ? -p[i] : p[i];
            if (v > peak) peak = v;
        }
        if (++posts % 120 == 0) {
            if (pm_debug_has("audiostat")) pm_log_d("audio posts %u peak %d\n", posts, peak);
            peak = 0;
        }
    }
    int slot = -1;
    for (int i = 0; i < SLOTS; i++) {
        if (wb[i].status != NDSP_WBUF_QUEUED && wb[i].status != NDSP_WBUF_PLAYING) {
            slot = i;
            break;
        }
    }
    if (slot < 0) { au_dropped++; return; }
    if (pm_audio_pending() == 0) au_underrun++; /* nothing queued: the DSP ran dry before this post */
    if (bytes > CAP) bytes = CAP;
    bytes &= ~3u;
    memcpy(data[slot], pcm, bytes);
    DSP_FlushDataCache(data[slot], bytes);
    memset(&wb[slot], 0, sizeof(wb[slot]));
    wb[slot].data_vaddr = data[slot];
    wb[slot].nsamples = bytes / 4;
    wb[slot].status = NDSP_WBUF_FREE;
    ndspChnWaveBufAdd(0, &wb[slot]);
}

/* AU1c/AU1d: the N64 audio manager (nuAuMgr) is a thread that blocks on a message queue. It runs as a real thread on core 1.
 * The game thread does two things only: pm_audio_kick(n) (start n 60 Hz ticks) and pm_audio_join() (wait until they are done).
 * The audio thread plays the driver role in pm_au_thread_yield: when nuAuMgr blocks it answers the audio tasks it submitted,
 * sends the next retrace message while ticks are left, and when nothing is left it signals "done" and sleeps until the next kick.
 * Game code and the audio thread never run at the same time: kick at the main nuGfxTaskStart, join at the next entry of
 * pm_gfx_retrace, step_game_loop and pm_warp_tick. Same code and same order as the lockstep version, so the PCM hash is equal. */
static Thread au_thread;
static LightEvent au_start, au_done;
static volatile int au_ticks_left, au_busy;
static int au_core = -2;
static void (*au_entry)(void*);
static void* au_arg;
static unsigned long long au_last;
static unsigned au_pr_n, au_pr_sum, au_pr_min = 0xFFFFFFFFu, au_pr_max, au_pr_on = 2;
static unsigned au_w_n, au_w_sum, au_w_max, au_j_sum, au_j_max, au_j_t0, au_g_sum;

static u8 au_crash_stack[0x4000] __attribute__((aligned(8)));

int pm_on_audio(void) { return au_thread && threadGetCurrent() == au_thread; }

static void au_main(void* unused) {
    (void)unused;
    pm_crash_install(au_crash_stack, sizeof(au_crash_stack)); /* the engine runs here: a fault needs its own trace */
    pm_stack_register(1, "audio", 64 * 1024);
    pm_fpscr_setup("audio");
    au_core = (int)svcGetProcessorID();
    pm_log_d("threads: audio on core %d (svcGetProcessorID)", au_core);
    au_entry(au_arg);
}

void pm_au_thread_start(void (*entry)(void*), void* arg) {
    if (au_thread) return;
    au_entry = entry;
    au_arg = arg;
    LightEvent_Init(&au_start, RESET_ONESHOT);
    LightEvent_Init(&au_done, RESET_STICKY);
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    /* core 1 (system core) works after APT_SetAppCpuTimeLimit in pm_log_start. If it fails, core 0 (same protocol, runs when the game thread waits). */
    if (!pm_debug_has("audiocore0")) au_thread = threadCreate(au_main, NULL, 64 * 1024, prio, 1, false);
    if (!au_thread) {
        pm_log_w("audio thread on core 1 not created, using core 0");
        au_thread = threadCreate(au_main, NULL, 64 * 1024, prio, -2, false);
    }
    if (!au_thread) {
        pm_log_e("audio thread failed\n");
        return;
    }
    LightEvent_Wait(&au_done); /* nuAuMgr ran its init and blocked for the first time */
    pm_log("audio manager thread ready");
}

int pm_au_thread_started(void) { return au_thread != NULL; }

/* Called by the audio thread when it waits on an empty queue. Returns 0 on any other thread. */
int pm_au_thread_yield(void) {
    if (!au_thread || threadGetCurrent() != au_thread) return 0;
    {
        static unsigned yields;
        if (yields == 299 && pm_debug_has("crashtest_audio_call")) pm_crash_selftest();
        if (++yields == 300 && pm_debug_has("crashtest_audio")) __asm__ volatile(".word 0xe7f000f0"); /* class E: deliberate undefined instruction on the audio thread */
    }
    for (;;) {
        if (pm_audio_serve()) return 1; /* an audio task was answered: nuAuMgr can go on */
        pm_audio_tick_end();
        if (au_ticks_left > 0) {
            au_ticks_left--;
            pm_audio_retrace_send();
            return 1;
        }
        if (au_busy) {
            unsigned dt = pm_ticks() - au_j_t0;
            if (pm_ph_on) __atomic_fetch_add(&pm_ph_cnt[C_AJOB], dt / 268u, __ATOMIC_RELAXED); /* PG0: kick to done of the job in us, a pc field (core 1 wall time, an upper bound of its CPU time) */
            au_j_sum += dt;
            if (dt > au_j_max) au_j_max = dt;
            au_busy = 0;
        }
        LightEvent_Signal(&au_done);
        /* AU1f core share probe: every 16th job, after "done", the audio thread runs a fixed loop and times it. The shortest time is
         * the speed of a free core. A longer average means core 1 gives the audio thread less than the whole core (APT limit, system
         * modules). Switch audiostat or coreprobe. 100000 loop steps are about 1.5 ms. */
        if (au_pr_on == 2) au_pr_on = (pm_debug_has("audiostat") || pm_debug_has("coreprobe")) ? 1 : 0;
        if (au_pr_on) {
            static unsigned jobs;
            if ((++jobs & 15) == 0) {
                volatile unsigned x = 0;
                unsigned t0 = pm_ticks();
                for (unsigned i = 0; i < 100000; i++) x += i * 3u;
                unsigned dt = pm_ticks() - t0;
                au_pr_n++;
                au_pr_sum += dt;
                if (dt < au_pr_min) au_pr_min = dt;
                if (dt > au_pr_max) au_pr_max = dt;
            }
        }
        LightEvent_Wait(&au_start);
    }
}

/* Game thread: start n ticks on the audio thread. */
void pm_audio_kick(unsigned n) {
    if (!au_thread || !n) return;
    pm_audio_join_at(0);
    au_ticks_left = (int)n;
    au_busy = 1;
    au_j_t0 = pm_ticks();
    LightEvent_Clear(&au_done);
    LightEvent_Signal(&au_start);
}

/* Game thread: wait until the audio thread is idle. Free when it is idle already. */
static unsigned au_site_n[4], au_site_sum[4];
void pm_audio_join_at(int site) {
    if (!au_busy) return;
    unsigned t0 = pm_ticks();
    au_g_sum += t0 - au_j_t0; /* core 0 work between kick and join */
    PH_PUSH(PH_AUDIO);
    LightEvent_Wait(&au_done);
    PH_POP();
    unsigned dt = pm_ticks() - t0;
    au_site_n[site & 3]++; au_site_sum[site & 3] += dt;
    au_w_sum += dt;
    if (dt > au_w_max) au_w_max = dt;
    if (++au_w_n == (pm_debug_has("audiostat") ? 60u : 600u)) { /* L01: per 2 s (30 joins per second) with audiostat */
        pm_log_d("audio wait avg %u us max %u us (core 0 join), job avg %u us max %u us, kick to join avg %u us, audio thread core %d",
                 (unsigned)((unsigned long long)au_w_sum * 1000 / au_w_n / 268112), (unsigned)((unsigned long long)au_w_max * 1000 / 268112),
                 (unsigned)((unsigned long long)au_j_sum * 1000 / au_w_n / 268112), (unsigned)((unsigned long long)au_j_max * 1000 / 268112), (unsigned)((unsigned long long)au_g_sum * 1000 / au_w_n / 268112), au_core);
        if (au_pr_n) pm_log_d("audio core probe (100000 steps, %u probes): min %u us avg %u us max %u us. avg / min above 1.0 means core %d gives the audio thread less than the whole core",
                              au_pr_n, (unsigned)((unsigned long long)au_pr_min * 1000 / 268112), (unsigned)((unsigned long long)au_pr_sum * 1000 / au_pr_n / 268112),
                              (unsigned)((unsigned long long)au_pr_max * 1000 / 268112), au_core);
        au_pr_n = au_pr_sum = au_pr_max = 0; /* min stays: it is the free core speed */
        pm_log_d("audio join sites (count, avg us): kick %u %u, retrace %u %u, step %u %u, main %u %u", au_site_n[0], au_site_n[0] ? au_site_sum[0] / au_site_n[0] / 268 : 0, au_site_n[1], au_site_n[1] ? au_site_sum[1] / au_site_n[1] / 268 : 0, au_site_n[2], au_site_n[2] ? au_site_sum[2] / au_site_n[2] / 268 : 0, au_site_n[3], au_site_n[3] ? au_site_sum[3] / au_site_n[3] / 268 : 0);
        au_w_n = au_w_sum = au_w_max = au_j_sum = au_j_max = au_g_sum = 0;
    }
}

/* Number of 60 Hz retraces that passed since the last call, at most 6. */
unsigned pm_audio_retraces(void) {
    unsigned long long now = svcGetSystemTick();
    unsigned long long per = SYSCLOCK_ARM11 / 60ull;
    if (!au_last) au_last = now;
    /* P20: n = (now - au_last) / per without a u64 divide. n is capped at 6, so count up to 7 steps. Same n, same au_last. */
    unsigned long long d = now - au_last, n = 0;
    if (d >= 7ull * per) { /* n >= 7: cap */
        n = 6;
        au_last = now;
    } else {
        while (d >= per) { d -= per; n++; }
        au_last += n * per;
    }
    return (unsigned)n;
}

/* Bytes of PCM that were posted and not played yet. This is what osAiGetLength reports. */
unsigned pm_audio_pending(void) {
    unsigned n = 0;
    if (!ready || pm_audio_lock()) return 0;
    for (int i = 0; i < SLOTS; i++) {
        if (wb[i].status == NDSP_WBUF_QUEUED || wb[i].status == NDSP_WBUF_PLAYING) n += wb[i].nsamples;
    }
    unsigned pos = ndspChnGetSamplePos(0);
    n = n > pos ? n - pos : 0;
    return n * 4;
}
