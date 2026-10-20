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

/* The N64 audio manager is a thread that blocks on a message queue. Run it as a real thread,
 * but hand control back and forth so that only one of the two threads runs at a time.
 * The game thread resumes it once per retrace and waits until it blocks again. */
static Thread au_thread;
static LightSemaphore au_run, au_idle;
static void (*au_entry)(void*);
static void* au_arg;
static unsigned long long au_last;

static u8 au_crash_stack[0x4000] __attribute__((aligned(8)));

static void au_main(void* unused) {
    (void)unused;
    pm_crash_install(au_crash_stack, sizeof(au_crash_stack)); /* the engine runs here: a fault needs its own trace */
    pm_stack_register(1, "audio", 64 * 1024);
    pm_fpscr_setup("audio");
    pm_log_d("threads: audio on core %d (svcGetProcessorID)", (int)svcGetProcessorID());
    au_entry(au_arg);
}

void pm_au_thread_start(void (*entry)(void*), void* arg) {
    if (au_thread) return;
    au_entry = entry;
    au_arg = arg;
    LightSemaphore_Init(&au_run, 0, 1);
    LightSemaphore_Init(&au_idle, 0, 1);
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    au_thread = threadCreate(au_main, NULL, 64 * 1024, prio, -2, false);
    if (!au_thread) {
        pm_log_e("audio thread failed\n");
        return;
    }
    LightSemaphore_Acquire(&au_idle, 1);
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
    LightSemaphore_Release(&au_idle, 1);
    LightSemaphore_Acquire(&au_run, 1);
    return 1;
}

void pm_au_thread_resume(void) {
    LightSemaphore_Release(&au_run, 1);
    LightSemaphore_Acquire(&au_idle, 1);
}

/* Number of 60 Hz retraces that passed since the last call, at most 6. */
unsigned pm_audio_retraces(void) {
    unsigned long long now = svcGetSystemTick();
    unsigned long long per = SYSCLOCK_ARM11 / 60ull;
    if (!au_last) au_last = now;
    unsigned long long n = (now - au_last) / per;
    au_last += n * per;
    if (n > 6) { /* AU1b: one call per drawn step now, so the cap is 6 (it was 3 for each of the two iterations of a step) */
        n = 6;
        au_last = now;
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
