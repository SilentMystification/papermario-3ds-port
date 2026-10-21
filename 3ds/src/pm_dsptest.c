#include <3ds.h>
#include <math.h>
#include <string.h>
#include "pm_port.h"
#include "pm_dsptest.h"

/* AUDDSP A2: switch dsptest. 24 ndsp channels play looped PCM16 (0 to 11) and DSP-ADPCM (12 to 23) wave buffers for 10 s.
 * A thread on core 1 runs a 60 Hz loop. Per tick it calls ndspChnSetMix and ndspChnSetRate on every channel and restarts 4 channels
 * (ndspChnWaveBufClear + ndspChnWaveBufAdd). It logs the ARM11 time of each call class. The frame callback and the aux 0 callback
 * log their count, their core, their time and the sample values. Lines start with "dsptest:". */
#define NCH 24
#define PCMN 2048
#define ADFR 64
static volatile int running;
static s16* pcm;
static u8* adp;
static ndspAdpcmData adpd[NCH];
static ndspWaveBuf wbuf[NCH];
static volatile unsigned cb_n, cb_us_sum, cb_us_max, cb_core, cb_iv_max;
static volatile unsigned long long cb_last;
static volatile unsigned ax_n, ax_us_sum, ax_us_max, ax_core, ax_ns, ax_pk16, ax_pk32, ax_iv_max;
static volatile unsigned long long ax_last;
static volatile int ax_dump;

static unsigned us(unsigned long long t) { return (unsigned)(t / 268ull); }

static void frame_cb(void* d) {
    (void)d;
    unsigned long long t0 = svcGetSystemTick();
    if (cb_last) { unsigned iv = us(t0 - cb_last); if (iv > cb_iv_max) cb_iv_max = iv; }
    cb_last = t0;
    cb_core = (unsigned)svcGetProcessorID();
    cb_n++;
    unsigned c = us(svcGetSystemTick() - t0);
    cb_us_sum += c;
    if (c > cb_us_max) cb_us_max = c;
}

static void aux_cb(void* d, int n, void* smp[4]) {
    (void)d;
    unsigned long long t0 = svcGetSystemTick();
    if (ax_last) { unsigned iv = us(t0 - ax_last); if (iv > ax_iv_max) ax_iv_max = iv; }
    ax_last = t0;
    ax_core = (unsigned)svcGetProcessorID();
    ax_ns = (unsigned)n;
    ax_n++;
    for (int k = 0; k < 4; k++) {
        if (!smp[k]) continue;
        const s16* p = (const s16*)smp[k];
        const s32* q = (const s32*)smp[k];
        for (int i = 0; i < n; i++) { int v = p[i] < 0 ? -p[i] : p[i]; if ((unsigned)v > ax_pk16) ax_pk16 = v; }
        for (int i = 0; i < n; i++) { unsigned v = q[i] < 0 ? -q[i] : q[i]; if (v > ax_pk32) ax_pk32 = v; }
    }
    if (ax_dump < 2 && ax_n > 100) {
        ax_dump++;
        const s16* p = (const s16*)smp[0];
        pm_log("dsptest: aux0 first samples n=%d ptr %p %p %p %p s16[0..7] %d %d %d %d %d %d %d %d", n, smp[0], smp[1], smp[2], smp[3],
               p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
    }
    unsigned c = us(svcGetSystemTick() - t0);
    ax_us_sum += c;
    if (c > ax_us_max) ax_us_max = c;
}

typedef struct { unsigned n, sum, max; } Cost;
static void cost_add(Cost* c, unsigned long long t) { unsigned u = (unsigned)(t * 100ull / 268ull); c->n++; c->sum += u; if (u > c->max) c->max = u; }

static void test_thread(void* arg) {
    (void)arg;
    Cost mix = {0}, rate = {0}, clr = {0}, add = {0}, tick = {0};
    unsigned ticks = 0, wake_late = 0;
    unsigned long long t_start = svcGetSystemTick(), t_next = t_start;
    const unsigned long long T60 = 268111856ull / 60;
    while (running && svcGetSystemTick() - t_start < 10ull * 268111856ull) {
        unsigned long long tt0 = svcGetSystemTick();
        for (int c = 0; c < NCH; c++) {
            float m[12] = {0};
            float g = 0.2f + 0.1f * sinf((ticks + c * 7) * 0.05f);
            float pan = 0.5f + 0.5f * sinf((ticks + c * 3) * 0.03f);
            m[0] = g * (1.f - pan); m[1] = g * pan;
            m[4] = 0.3f * g; m[5] = 0.3f * g; /* aux 0 */
            unsigned long long t = svcGetSystemTick();
            ndspChnSetMix(c, m);
            cost_add(&mix, svcGetSystemTick() - t);
            t = svcGetSystemTick();
            ndspChnSetRate(c, 8000.f + 4000.f * (1.f + sinf((ticks + c) * 0.04f)) + c * 300.f);
            cost_add(&rate, svcGetSystemTick() - t);
        }
        for (int k = 0; k < 4; k++) {
            int c = (ticks * 4 + k) % NCH;
            unsigned long long t = svcGetSystemTick();
            ndspChnWaveBufClear(c);
            cost_add(&clr, svcGetSystemTick() - t);
            memset(&wbuf[c], 0, sizeof(wbuf[c]));
            if (c < 12) { wbuf[c].data_pcm16 = pcm; wbuf[c].nsamples = PCMN; }
            else { wbuf[c].data_adpcm = adp; wbuf[c].nsamples = ADFR * 14; wbuf[c].adpcm_data = &adpd[c]; }
            wbuf[c].looping = true;
            t = svcGetSystemTick();
            ndspChnWaveBufAdd(c, &wbuf[c]);
            cost_add(&add, svcGetSystemTick() - t);
        }
        cost_add(&tick, svcGetSystemTick() - tt0);
        ticks++;
        if (ticks % 120 == 0) {
            int playing = 0;
            for (int c = 0; c < NCH; c++) playing += ndspChnIsPlaying(c);
            pm_log("dsptest: t=%u ticks, playing %d of 24, per call us (x100 units /100): SetMix avg %u max %u, SetRate avg %u max %u, WaveBufClear avg %u max %u, WaveBufAdd avg %u max %u; one tick (24 mix + 24 rate + 4 restarts) avg %u max %u (us x100)",
                   ticks, playing, mix.sum / (mix.n ? mix.n : 1), mix.max, rate.sum / (rate.n ? rate.n : 1), rate.max,
                   clr.sum / (clr.n ? clr.n : 1), clr.max, add.sum / (add.n ? add.n : 1), add.max, tick.sum / (tick.n ? tick.n : 1), tick.max);
            pm_log("dsptest: frame callback n %u core %u cost avg %u max %u us, interval max %u us; aux0 callback n %u core %u nsamples %u cost avg %u max %u us, interval max %u us, peak as s16 %u as s32 %u; ndsp frames %u dropped %u",
                   cb_n, cb_core, cb_n ? cb_us_sum / cb_n : 0, cb_us_max, cb_iv_max, ax_n, ax_core, ax_ns, ax_n ? ax_us_sum / ax_n : 0, ax_us_max, ax_iv_max,
                   ax_pk16, ax_pk32, ndspGetFrameCount(), ndspGetDroppedFrames());
        }
        t_next += T60;
        unsigned long long now = svcGetSystemTick();
        if (now < t_next) svcSleepThread((s64)((t_next - now) * 1000ull / 268ull * 1000ull / 1000ull));
        else wake_late++;
    }
    pm_log("dsptest: done, %u ticks in 10 s, late wakes %u, control cost per tick (24 SetMix + 24 SetRate + 4 restarts) avg %u us x100 max %u us x100; per call us x100: SetMix %u SetRate %u Clear %u Add %u",
           ticks, wake_late, tick.sum / (tick.n ? tick.n : 1), tick.max, mix.sum / (mix.n ? mix.n : 1), rate.sum / (rate.n ? rate.n : 1), clr.sum / (clr.n ? clr.n : 1), add.sum / (add.n ? add.n : 1));
    for (int c = 0; c < NCH; c++) ndspChnWaveBufClear(c);
    ndspSetCallback(NULL, NULL);
    ndspAuxSetEnable(0, false);
    for (int c = 0; c < NCH; c++) ndspChnReset(c);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetRate(0, 32000.f);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    running = 0;
}

static int dt_started;
int pm_dsptest_active(void) { return running; }

void pm_dsptest_start(void) {
    if (dt_started) return;
    dt_started = 1;
    pcm = (s16*)linearAlloc(PCMN * 2);
    adp = (u8*)linearAlloc(ADFR * 8);
    if (!pcm || !adp) { pm_log_e("dsptest: linearAlloc failed"); return; }
    for (int i = 0; i < PCMN; i++) pcm[i] = (s16)(12000.f * sinf(6.2831853f * i / 128.f));
    /* DSP-ADPCM frames: header byte = predictor index 0 (coefficients 0) and scale 6; 14 nibbles make a square-ish wave */
    for (int f = 0; f < ADFR; f++) {
        adp[f * 8] = 0x06;
        for (int b = 1; b < 8; b++) adp[f * 8 + b] = (b < 4) ? 0x44 : 0xCC;
    }
    DSP_FlushDataCache(pcm, PCMN * 2);
    DSP_FlushDataCache(adp, ADFR * 8);
    u16 coefs[16] = {0};
    for (int c = 0; c < NCH; c++) {
        ndspChnReset(c);
        ndspChnSetInterp(c, NDSP_INTERP_POLYPHASE);
        ndspChnSetFormat(c, c < 12 ? NDSP_FORMAT_MONO_PCM16 : NDSP_FORMAT_MONO_ADPCM);
        ndspChnSetRate(c, 16000.f);
        if (c >= 12) ndspChnSetAdpcmCoefs(c, coefs);
        memset(&adpd[c], 0, sizeof(adpd[c]));
        adpd[c].index = 0x06;
        memset(&wbuf[c], 0, sizeof(wbuf[c]));
        if (c < 12) { wbuf[c].data_pcm16 = pcm; wbuf[c].nsamples = PCMN; }
        else { wbuf[c].data_adpcm = adp; wbuf[c].nsamples = ADFR * 14; wbuf[c].adpcm_data = &adpd[c]; }
        wbuf[c].looping = true;
        ndspChnWaveBufAdd(c, &wbuf[c]);
    }
    ndspSetCallback(frame_cb, NULL);
    ndspAuxSetEnable(0, true);
    ndspAuxSetCallback(0, aux_cb, NULL);
    running = 1;
    Thread t = threadCreate(test_thread, NULL, 0x4000, 0x30, 1, true);
    if (!t) { running = 0; pm_log_e("dsptest: threadCreate failed"); return; }
    pm_log("dsptest: started, 24 channels (0 to 11 PCM16 loop, 12 to 23 DSP-ADPCM loop), 10 s");
}
