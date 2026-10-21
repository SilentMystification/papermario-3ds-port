#include <3ds.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pm_port.h"
#include "pm_dspaud.h"

/* AUDDSP A3: the N64 voices play on ndsp channels 0 to 22 as mono PCM16. The CPU decodes the VADPCM once, when a sample first plays,
 * into a cache in linear memory. Channel 23 plays the effect return. See docs/plans/AUD_DSP.md. */
volatile int pm_dspa_enabled;

#define BLK_TICKS 1541641ull          /* 184 samples at 32000 Hz in ARM11 ticks (268111856 Hz) */
#define MAXBUF 8
#define MAXENT 256
#define FXR_BUFS 8
#define FXFRAME 160

typedef struct {
    unsigned key;
    s16* mem;
    unsigned bytes, stamp_ms;
    int pins;
    s16 *pre, *body, *tail;
    unsigned npre, nbody, ntail;
    int loopCount, looped;
} Ent;
static Ent ents[MAXENT];
static unsigned cache_budget = 1536u * 1024u, cache_used, cache_peak;
static unsigned c_hit, c_miss, c_evict, c_fail, c_late, dec_us_sum, dec_us_max, dec_n;

typedef struct { Ent* e; ndspWaveBuf wb[MAXBUF]; int nb; int active; } Chan;
static Chan chn[PM_DSPA_VOICES];

static unsigned st_blocks, st_voice_sum, st_voice_max, st_starts, st_stops, st_ends, st_cost_sum, st_cost_max, st_late, st_bl_sum, st_bl_max, st_sets, st_set_us;
static unsigned long long next_due;
static unsigned long long t_blk0;

static unsigned now_ms(void) { return (unsigned)(svcGetSystemTick() / 268112ull); }
static unsigned tk_us(unsigned long long t) { return (unsigned)(t / 268ull); }

void pm_dspa_log_once(const char* fmt, ...) {
    char b[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    pm_log_w("dspaudio: %s", b);
}

/* ---------- VADPCM decode (same arithmetic as aADPCMdecImpl, scalar) ---------- */
static inline s16 clamp16(int v) { return v < -32768 ? -32768 : v > 32767 ? 32767 : (s16)v; }
static void vadpcm(const unsigned char* in, int nframes, const s16* book, s16 h2, s16 h1, s16* out) {
    for (int f = 0; f < nframes; f++) {
        int shift = in[0] >> 4, ti = in[0] & 0xF;
        const s16* tbl0;
        const s16* tbl1;
        if (ti > 7) ti = 7;
        tbl0 = book + ti * 16;
        tbl1 = tbl0 + 8;
        in++;
        for (int half = 0; half < 2; half++) {
            s16 ins[8];
            for (int j = 0; j < 4; j++) {
                ins[j * 2] = (s16)((((int)(*in >> 4) << 28) >> 28) << shift);
                ins[j * 2 + 1] = (s16)((((int)(*in & 0xF) << 28) >> 28) << shift);
                in++;
            }
            for (int j = 0; j < 8; j++) {
                int acc = tbl0[j] * h2 + tbl1[j] * h1 + (ins[j] << 11);
                for (int k = 0; k < j; k++) acc += tbl1[(j - k) - 1] * ins[k];
                acc >>= 11;
                s16 o = clamp16(acc);
                *out++ = o;
                h2 = h1;
                h1 = o;
            }
        }
    }
}

static void ent_free(Ent* e) {
    if (e->mem) linearFree(e->mem);
    cache_used -= e->bytes;
    memset(e, 0, sizeof *e);
}

static Ent* cache_get(const DspaSample* s) {
    Ent* free_e = NULL;
    unsigned now = now_ms();
    for (int i = 0; i < MAXENT; i++) {
        if (ents[i].mem && ents[i].key == s->key) { ents[i].stamp_ms = now; c_hit++; return &ents[i]; }
        if (!ents[i].mem && !free_e) free_e = &ents[i];
    }
    if (!s->rom || !s->book || s->bytes < 9) { c_fail++; return NULL; }
    c_miss++;
    unsigned long long t0 = svcGetSystemTick();
    int nframes = s->bytes / 9, N = nframes * 16;
    int ls = s->loopStart, le = s->loopEnd, endless = s->loopCount == -1;
    int looped = le != 0 && s->lstate != NULL && le > ls && le <= N && ls >= 0;
    unsigned npre, nbody = 0, ntail = 0;
    s16* lin = (s16*)malloc((size_t)N * 2);
    if (!lin) { c_fail++; return NULL; }
    vadpcm(s->rom, nframes, s->book, 0, 0, lin);
    if (looped) { npre = ls; nbody = le - ls; ntail = endless ? 0 : N - le; }
    else npre = N;
    unsigned total = (npre + nbody + ntail) * 2;
    unsigned need = (total + 127) & ~127u;
    if (need > cache_budget) { free(lin); c_fail++; return NULL; }
    /* evict the oldest unpinned entries until the sample fits (entries used in the last 60 ms stay: the DSP may still read them) */
    while (cache_used + need > cache_budget || !free_e) {
        Ent* old = NULL;
        for (int i = 0; i < MAXENT; i++)
            if (ents[i].mem && ents[i].pins == 0 && now - ents[i].stamp_ms > 60 && (!old || ents[i].stamp_ms < old->stamp_ms)) old = &ents[i];
        if (!old) { free(lin); c_fail++; return NULL; }
        ent_free(old);
        c_evict++;
        free_e = old;
    }
    s16* mem = (s16*)linearMemAlign(need, 128);
    if (!mem) { free(lin); c_fail++; return NULL; }
    Ent* e = free_e;
    memset(e, 0, sizeof *e);
    e->mem = mem; e->key = s->key; e->bytes = need; e->stamp_ms = now;
    e->loopCount = s->loopCount; e->looped = looped;
    e->pre = mem; e->npre = npre;
    memcpy(e->pre, lin, (size_t)npre * 2);
    if (looped) {
        e->body = mem + npre; e->nbody = nbody;
        int k = 16 - (ls & 15);
        if ((unsigned)k > nbody) k = nbody;
        memcpy(e->body, s->lstate + (ls & 15), (size_t)k * 2);
        int rest = nbody - k;
        if (rest > 0) {
            int f0 = (ls >> 4) + 1, nf = (rest + 15) / 16;
            s16* tmp = (s16*)malloc((size_t)nf * 32);
            if (tmp) {
                memset(tmp, 0, (size_t)nf * 32);
                if (f0 + nf > nframes) nf = nframes > f0 ? nframes - f0 : 0;
                if (nf > 0) vadpcm(s->rom + f0 * 9, nf, s->book, s->lstate[14], s->lstate[15], tmp);
                memcpy(e->body + k, tmp, (size_t)rest * 2);
                free(tmp);
            } else memset(e->body + k, 0, (size_t)rest * 2);
        }
        if (ntail) { e->tail = e->body + nbody; e->ntail = ntail; memcpy(e->tail, lin + le, (size_t)ntail * 2); }
    }
    free(lin);
    DSP_FlushDataCache(mem, total);
    cache_used += need;
    if (cache_used > cache_peak) cache_peak = cache_used;
    unsigned us = tk_us(svcGetSystemTick() - t0);
    dec_n++; dec_us_sum += us; if (us > dec_us_max) dec_us_max = us;
    return e;
}

/* ---------- channels ---------- */
static void chan_add(int ch, s16* data, unsigned n, int loop) {
    Chan* c = &chn[ch];
    if (!n || c->nb >= MAXBUF) return;
    ndspWaveBuf* w = &c->wb[c->nb++];
    memset(w, 0, sizeof *w);
    w->data_pcm16 = data;
    w->nsamples = n;
    w->looping = loop;
    w->status = NDSP_WBUF_FREE;
    ndspChnWaveBufAdd(ch, w);
}

void pm_dspa_voice_start(int ch, const DspaSample* s, float rate, const float mix[12]) {
    Chan* c = &chn[ch];
    Ent* e = cache_get(s);
    if (c->e) { c->e->pins--; c->e = NULL; }
    ndspChnWaveBufClear(ch);
    c->nb = 0;
    c->active = 0;
    if (!e) return;
    e->pins++;
    c->e = e;
    ndspChnSetFormat(ch, NDSP_FORMAT_MONO_PCM16);
    ndspChnSetInterp(ch, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(ch, rate);
    ndspChnSetMix(ch, (float*)mix);
    if (!e->looped) chan_add(ch, e->pre, e->npre, 0);
    else {
        chan_add(ch, e->pre, e->npre, 0);
        if (e->loopCount == -1) chan_add(ch, e->body, e->nbody, 1);
        else {
            int reps = e->loopCount + 1;
            if (reps > 4) reps = 4;
            for (int i = 0; i < reps; i++) chan_add(ch, e->body, e->nbody, 0);
            chan_add(ch, e->tail, e->ntail, 0);
        }
    }
    c->active = c->nb > 0;
}

void pm_dspa_voice_update(int ch, float rate, const float mix[12]) {
    if (!chn[ch].active) return;
    unsigned long long t = svcGetSystemTick();
    ndspChnSetRate(ch, rate);
    ndspChnSetMix(ch, (float*)mix);
    st_sets++;
    st_set_us += tk_us(svcGetSystemTick() - t);
}

void pm_dspa_voice_stop(int ch) {
    Chan* c = &chn[ch];
    ndspChnWaveBufClear(ch);
    if (c->e) { c->e->pins--; c->e->stamp_ms = now_ms(); c->e = NULL; }
    c->nb = 0;
    c->active = 0;
}

int pm_dspa_voice_ended(int ch) {
    Chan* c = &chn[ch];
    if (!c->active || c->nb == 0) return 0;
    for (int i = 0; i < c->nb; i++) {
        if (c->wb[i].looping) return 0;
        if (c->wb[i].status != NDSP_WBUF_DONE) return 0;
    }
    return 1;
}

void pm_dspa_master(float vol) { ndspSetMasterVol(vol); }

/* ---------- effects: CPU delay network on the aux outputs ---------- */
typedef struct { int in, out; float fb, ff, gain, lpa, lpy; int lp; } Tap;
typedef struct {
    int n, len, w;
    Tap t[4];
    float* buf;
} Fx;
static Fx fxs[2][2];                  /* [bus][channel] */
static volatile int fx_gen[2], fx_have[2];
static const int* fx_cfg[2];
static int fx_type_cfg[2];
static LightLock fx_lock;
#define DLY_MAX 4096
static float dly_dry[2][DLY_MAX], dly_wet[2][DLY_MAX];
static int dly_w[2], dly_bus_cfg = -1, dly_side_cfg, dly_n_cfg;
static float acc[2][FXFRAME];
static int acc_got;
static ndspWaveBuf fxw[FXR_BUFS];
static s16* fxmem;
static unsigned ax_n[2], ax_us_sum, ax_us_max, ax_core, fx_emitted, fx_dropped, fx_lastflip;
static volatile unsigned fx_started;

static void fx_build(int bus) {
    const int* p = fx_cfg[bus];
    for (int c = 0; c < 2; c++) {
        Fx* f = &fxs[bus][c];
        if (f->buf) { free(f->buf); f->buf = NULL; }
        f->n = 0;
        if (!p || p[0] <= 0 || p[0] > 4) continue;
        int len = (p[1] + 2) * 188;
        f->buf = (float*)calloc((size_t)len, sizeof(float));
        if (!f->buf) continue;
        f->len = len; f->w = 0; f->n = p[0];
        for (int i = 0; i < f->n; i++) {
            const int* q = p + 2 + i * 8;
            Tap* t = &f->t[i];
            t->in = q[0] * 188;
            t->out = q[1] * 188;
            t->fb = (float)(s16)q[2] / 32768.0f;
            t->ff = (float)(s16)q[3] / 32768.0f;
            t->gain = (float)(s16)q[4] / 32768.0f;
            t->lp = q[7] != 0;
            t->lpa = t->lp ? (float)(q[7] >> 1) / 16384.0f : 0.0f;
            t->lpy = 0.0f;
        }
    }
    if (p && p[0] > 0) {
        for (int i = 0; i < p[0] && i < 4; i++) if (p[2 + i * 8 + 5]) { pm_dspa_log_once("effect type %d uses chorus or flange modulation: not modelled, static delay", fx_type_cfg[bus]); break; }
    }
}

void pm_dspa_fx_set(int bus, const int* params, int type) {
    if (bus < 0 || bus > 1) return;
    LightLock_Lock(&fx_lock);
    fx_cfg[bus] = params;
    fx_type_cfg[bus] = type;
    fx_gen[bus]++;
    LightLock_Unlock(&fx_lock);
    pm_log("dspaudio: bus %d effect type %d (%d sections)", bus, type, params ? params[0] : 0);
}

void pm_dspa_delay_set(int bus, int side, int blocks) {
    LightLock_Lock(&fx_lock);
    dly_bus_cfg = side ? bus : -1;
    dly_side_cfg = side;
    dly_n_cfg = blocks * 188;
    if (dly_n_cfg > DLY_MAX - 1) dly_n_cfg = DLY_MAX - 1;
    memset(dly_dry, 0, sizeof dly_dry);
    memset(dly_wet, 0, sizeof dly_wet);
    LightLock_Unlock(&fx_lock);
    pm_log("dspaudio: channel delay bus %d side %d blocks %d", bus, side, blocks);
}

static void fx_emit(void) {
    int slot = -1;
    for (int i = 0; i < FXR_BUFS; i++) if (fxw[i].status != NDSP_WBUF_QUEUED && fxw[i].status != NDSP_WBUF_PLAYING) { slot = i; break; }
    if (slot < 0) { fx_dropped++; return; }
    s16* o = fxmem + slot * FXFRAME * 2;
    for (int i = 0; i < FXFRAME; i++) {
        o[i * 2] = clamp16((int)acc[0][i]);
        o[i * 2 + 1] = clamp16((int)acc[1][i]);
    }
    DSP_FlushDataCache(o, FXFRAME * 4);
    memset(&fxw[slot], 0, sizeof fxw[slot]);
    fxw[slot].data_pcm16 = o;
    fxw[slot].nsamples = FXFRAME;
    ndspChnWaveBufAdd(23, &fxw[slot]);
    fx_emitted++;
}

static void aux_frame(int bus, int n, void* smp[4]) {
    unsigned long long t0 = svcGetSystemTick();
    ax_core = (unsigned)svcGetProcessorID();
    ax_n[bus]++;
    if (n > FXFRAME) n = FXFRAME;
    LightLock_Lock(&fx_lock);
    if (fx_have[bus] != fx_gen[bus]) { fx_have[bus] = fx_gen[bus]; fx_build(bus); }
    int dl_on = dly_bus_cfg == bus && dly_side_cfg != 0;
    int dn = dly_n_cfg;
    if (acc_got & (1 << bus)) { fx_emit(); acc_got = 0; memset(acc, 0, sizeof acc); }
    for (int c = 0; c < 2; c++) {
        s16* in = (s16*)smp[c];
        s16* dry = (s16*)smp[2 + c];
        Fx* f = &fxs[bus][c];
        for (int i = 0; i < n; i++) {
            float x = in ? (float)in[i] : 0.0f;
            float wet = 0.0f;
            if (f->n && f->buf) {
                int w = f->w;
                f->buf[w] = x;
                for (int k = 0; k < f->n; k++) {
                    Tap* t = &f->t[k];
                    int ip = w - t->in; if (ip < 0) ip += f->len;
                    int op = w - t->out; if (op < 0) op += f->len;
                    float a = f->buf[ip], o = f->buf[op];
                    if (t->ff != 0.0f) o += t->ff * a;
                    if (t->lp) { t->lpy = (1.0f - t->lpa) * o + t->lpa * t->lpy; o = t->lpy; }
                    f->buf[op] = o;
                    if (t->fb != 0.0f) { a += t->fb * o; f->buf[ip] = a; }
                    wet += t->gain * o;
                }
                if (++f->w >= f->len) f->w = 0;
            }
            float d = dry ? (float)dry[i] : 0.0f;
            if (dl_on && dn > 0 && c == dly_side_cfg - 1) {
                int w = dly_w[c], r = w - dn; if (r < 0) r += DLY_MAX;
                dly_dry[c][w] = d; dly_wet[c][w] = wet;
                d = dly_dry[c][r]; wet = dly_wet[c][r];
                dly_w[c] = (w + 1) % DLY_MAX;
            }
            acc[c][i] += wet + d;
        }
        if (in) memset(in, 0, (size_t)n * 2);
        if (dry) memset(dry, 0, (size_t)n * 2);
    }
    acc_got |= 1 << bus;
    if (acc_got == 3) { fx_emit(); acc_got = 0; memset(acc, 0, sizeof acc); }
    LightLock_Unlock(&fx_lock);
    unsigned us = tk_us(svcGetSystemTick() - t0);
    ax_us_sum += us; if (us > ax_us_max) ax_us_max = us;
}
static void aux0_cb(void* d, int n, void* smp[4]) { (void)d; aux_frame(0, n, smp); }
static void aux1_cb(void* d, int n, void* smp[4]) { (void)d; aux_frame(1, n, smp); }

int pm_dspa_init(void) {
    const char* kb = NULL;
    (void)kb;
    char kv[16];
    unsigned want = pm_debug_value("dspcache", kv, sizeof kv) > 0 ? (unsigned)atoi(kv) : 1536u;
    if (want < 256) want = 256;
    cache_budget = want * 1024u;
    LightLock_Init(&fx_lock);
    fxmem = (s16*)linearMemAlign(FXR_BUFS * FXFRAME * 4, 128);
    if (!fxmem) { pm_log_e("dspaudio: linearAlloc for the effect return failed"); return 0; }
    memset(fxmem, 0, FXR_BUFS * FXFRAME * 4);
    for (int c = 0; c < 24; c++) ndspChnReset(c);
    for (int c = 0; c < PM_DSPA_VOICES; c++) {
        ndspChnSetFormat(c, NDSP_FORMAT_MONO_PCM16);
        ndspChnSetInterp(c, NDSP_INTERP_POLYPHASE);
    }
    ndspChnSetFormat(23, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetRate(23, 32728.0f);
    ndspChnSetInterp(23, NDSP_INTERP_NONE);
    {
        float m[12] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        ndspChnSetMix(23, m);
    }
    ndspAuxSetEnable(0, true);
    ndspAuxSetEnable(1, true);
    ndspAuxSetCallback(0, aux0_cb, NULL);
    ndspAuxSetCallback(1, aux1_cb, NULL);
    ndspSetMasterVol(1.0f);
    pm_dspa_enabled = 1;
    pm_log("dspaudio: on, 23 voices on ndsp channels 0 to 22, effect return on channel 23, cache %u KB", cache_budget / 1024);
    return 1;
}

void pm_dspa_shutdown(void) {
    if (!pm_dspa_enabled) return;
    pm_dspa_enabled = 0;
    ndspAuxSetCallback(0, NULL, NULL);
    ndspAuxSetCallback(1, NULL, NULL);
    for (int c = 0; c < 24; c++) ndspChnWaveBufClear(c);
}

/* ---------- timeline ---------- */
void pm_dspa_pace(void) {
    unsigned long long now = svcGetSystemTick();
    t_blk0 = now;
    if (!next_due || now > next_due + 6 * BLK_TICKS) { if (next_due) st_late++; next_due = now; }
    else if (now < next_due) {
        unsigned long long dt = next_due - now;
        svcSleepThread((s64)(dt * 1000ull / 268ull));
    }
    next_due += BLK_TICKS;
    t_blk0 = svcGetSystemTick();
}

void pm_dspa_block_done(int voices, int starts, int stops, int ends, unsigned costTicks) {
    (void)costTicks;
    unsigned us = tk_us(svcGetSystemTick() - t_blk0);
    unsigned bl = au2_blk_backlog();
    st_blocks++;
    st_voice_sum += voices; if ((unsigned)voices > st_voice_max) st_voice_max = voices;
    st_starts += starts; st_stops += stops; st_ends += ends;
    st_cost_sum += us; if (us > st_cost_max) st_cost_max = us;
    st_bl_sum += bl; if (bl > st_bl_max) st_bl_max = bl;
}

unsigned pm_dspa_backlog_blocks(void) { return au2_blk_backlog(); }

void pm_dspa_stats_log(void) {
    if (!pm_dspa_enabled || !st_blocks) return;
    unsigned b = st_blocks;
    pm_log_d("audio dsp: blocks %u voices avg x100 %u max %u starts %u stops %u ends %u; block cost avg %u max %u us (decode included), %u late resyncs, backlog blocks avg x10 %u max %u; ndsp param writes %u avg %u us",
             b, st_voice_sum * 100 / b, st_voice_max, st_starts, st_stops, st_ends, st_cost_sum / b, st_cost_max, st_late, st_bl_sum * 10 / b, st_bl_max,
             st_sets, st_sets ? st_set_us * 100 / st_sets : 0);
    pm_log_d("audio dsp: cache hits %u misses %u evictions %u failures %u, bytes %u KB peak %u KB of %u KB, decodes %u avg %u us max %u us; ndsp frames %u dropped %u",
             c_hit, c_miss, c_evict, c_fail, cache_used / 1024, cache_peak / 1024, cache_budget / 1024, dec_n, dec_n ? dec_us_sum / dec_n : 0, dec_us_max,
             ndspGetFrameCount(), ndspGetDroppedFrames());
    pm_log_d("audio dsp: aux callbacks bus0 %u bus1 %u core %u cost avg %u max %u us, effect frames queued %u dropped %u",
             ax_n[0], ax_n[1], ax_core, (ax_n[0] + ax_n[1]) ? ax_us_sum / (ax_n[0] + ax_n[1]) : 0, ax_us_max, fx_emitted, fx_dropped);
    st_blocks = st_voice_sum = st_voice_max = st_starts = st_stops = st_ends = st_cost_sum = st_cost_max = st_late = st_bl_sum = st_bl_max = st_sets = st_set_us = 0;
    c_hit = c_miss = c_evict = c_fail = dec_n = dec_us_sum = dec_us_max = 0;
    ax_us_sum = ax_us_max = 0; ax_n[0] = ax_n[1] = 0;
}
