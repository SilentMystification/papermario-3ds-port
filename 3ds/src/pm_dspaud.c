#include <3ds.h>
#include "pm_linear.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pm_port.h"
#include "pm_dspaud.h"

/* AUDDSP A3: the 24 N64 voices play on ndsp channels 0 to 22 as mono PCM16 (dsp_voice.c gives a voice the lowest free channel at its start). The CPU decodes the VADPCM once, when a sample first plays,
 * into a cache in linear memory. Channel 23 plays the effect return. See docs/plans/AUD_DSP.md. */
volatile int pm_dspa_enabled;
volatile unsigned pm_dspa_epoch;

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
/* BTLWIPE: decodes since boot (count, us, never reset) and the longest one since pm_dspa_dec_mark, for the xfer3 line (pm_xfer.c). */
static volatile unsigned xd_n, xd_us, xd_max;
void pm_dspa_dec_mark(unsigned* n, unsigned* us) { *n = xd_n; *us = xd_us; xd_max = 0; }
void pm_dspa_dec_read(unsigned* n, unsigned* us, unsigned* max) { *n = xd_n; *us = xd_us; *max = xd_max; }

static volatile unsigned ax_total; /* aux callbacks since the start */
static volatile int fx_flip;       /* the first aux callback ran: the render thread sets the mixes again */
static unsigned ax_core;
static void aux0_cb(void* d, int n, void* smp[4]);
static void aux1_cb(void* d, int n, void* smp[4]);
/* census (pm_audcensus.c): t0 = tick at the start, blk = blocks since the start, exp = samples queued (endless: no expected length) */
typedef struct { Ent* e; ndspWaveBuf wb[MAXBUF]; int nb; int active; unsigned long long t0; float rate; unsigned blk, exp, id; int endless, ended; } Chan;
static Chan chn[PM_DSPA_VOICES];
static const unsigned char vc_ac[DSPA_VC_N] = { AC_NOCHAN, AC_GIVEUP, AC_NONADPCM };

static unsigned st_blocks, st_voice_sum, st_voice_max, st_starts, st_stops, st_ends, st_cost_sum, st_cost_max, st_late, st_bl_sum, st_bl_max, st_sets, st_set_us;
static unsigned vc_n[DSPA_VC_N], vc_loopcap, vc_win;
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

#include "pm_vadpcm.h"

static void ent_free(Ent* e) {
    if (e->mem) pm_linear_free(e->mem);
    cache_used -= e->bytes;
    memset(e, 0, sizeof *e);
}

/* PT85: a sound start is dropped when its sample cannot be put in the cache. One warning line per drop (first 40), with the reason. */
static Ent* sample_drop(const char* why, const DspaSample* s) {
    static unsigned n;
    c_fail++;
    if (n++ < 40) pm_log_w("dspaudio: sound start dropped (%s): key %08x bytes %d cache %u of %u KB", why, (unsigned)s->key, (int)s->bytes, cache_used / 1024, cache_budget / 1024);
    return NULL;
}
/* A free table entry after evicting the oldest unpinned entries not used in the last keep_ms until need more bytes fit, or NULL. Render
 * thread only (the owner of ents). keep_ms 60 for a sound start (the DSP may still read an entry used in the last 60 ms), 2000 for a
 * pre-decoded entry (BTLWIPE F4: it must not push out what plays now). */
static Ent* cache_room(unsigned need, unsigned keep_ms, unsigned now) {
    Ent* free_e = NULL;
    for (int i = 0; i < MAXENT && !free_e; i++) if (!ents[i].mem) free_e = &ents[i];
    while (cache_used + need > cache_budget || !free_e) {
        Ent* old = NULL;
        for (int i = 0; i < MAXENT; i++)
            if (ents[i].mem && ents[i].pins == 0 && now - ents[i].stamp_ms > keep_ms && (!old || ents[i].stamp_ms < old->stamp_ms)) old = &ents[i];
        if (!old) return NULL;
        ent_free(old);
        c_evict++;
        free_e = old;
    }
    return free_e;
}
/* The decode of one sample into a linear block laid out pre | body | tail (shared by the sound start and the pre-decode). out gets the block
 * and the layout (key, pins 0, stamp 0); the table and cache_used are not touched. owner = the render thread (the table owner): it makes room
 * (cache_room, 60 ms) between the decode and the linear allocation, as the sound start always did; the pre-decode thread allocates only.
 * Returns NULL or the reason of a failure. */
static const char* sample_build(const DspaSample* s, Ent* out, int owner, unsigned now) {
    int nframes = s->bytes / 9, N = nframes * 16;
    int ls = s->loopStart, le = s->loopEnd, endless = s->loopCount == -1;
    int looped = le != 0 && s->lstate != NULL && le > ls && le <= N && ls >= 0;
    unsigned npre, nbody = 0, ntail = 0;
    s16* lin = (s16*)malloc((size_t)N * 2);
    if (!lin) return "no heap for the decode";
    vadpcm(s->rom, nframes, s->book, 0, 0, lin);
    if (looped) { npre = ls; nbody = le - ls; ntail = endless ? 0 : N - le; }
    else npre = N;
    unsigned total = (npre + nbody + ntail) * 2;
    unsigned need = (total + 127) & ~127u;
    if (need > cache_budget) { free(lin); return "larger than the cache"; }
    if (owner && !cache_room(need, 60, now)) { free(lin); return "no cache entry can be evicted: all pinned or used in the last 60 ms"; }
    s16* mem = (s16*)pm_linear_memalign(need, 128);
    if (!mem) { free(lin); return "no linear memory"; }
    Ent* e = out;
    memset(e, 0, sizeof *e);
    e->mem = mem; e->key = s->key; e->bytes = need;
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
    return NULL;
}

/* BTLWIPE F4: pre-decoded samples. The pre-decode thread (pm_prefetch.c, core 1, lowest priority) builds entries and puts them in this ring;
 * the render thread moves them into the table at its next cache lookup. One producer, one consumer, no lock: the render thread never waits
 * for the low priority thread. */
#define PRE_N 32u
static Ent pre_ring[PRE_N];
static volatile unsigned pre_head, pre_tail; /* head: producer, tail: consumer */
static volatile unsigned pre_n_dec, pre_n_in, pre_n_drop; /* built, put in the table, dropped (already cached, or no room) */
static void pre_drain(unsigned now) {
    unsigned t = pre_tail, h = __atomic_load_n(&pre_head, __ATOMIC_ACQUIRE);
    while (t != h) {
        Ent* p = &pre_ring[t & (PRE_N - 1)];
        int dup = 0;
        for (int i = 0; i < MAXENT; i++) if (ents[i].mem && ents[i].key == p->key) { dup = 1; break; }
        Ent* e = dup ? NULL : cache_room(p->bytes, 2000, now);
        if (e) {
            *e = *p;
            e->stamp_ms = now;
            cache_used += e->bytes;
            if (cache_used > cache_peak) cache_peak = cache_used;
            pre_n_in++;
        } else { pm_linear_free(p->mem); pre_n_drop++; }
        p->mem = NULL;
        t++;
    }
    __atomic_store_n(&pre_tail, t, __ATOMIC_RELEASE);
}
/* Pre-decode thread: build the samples of list that are not in the cache. Stops when the ring is full. The table read is unlocked: a stale
 * answer costs one decode (the render thread drops a duplicate). */
void pm_dspa_predecode(const DspaSample* list, int n) {
    if (!pm_dspa_enabled) return;
    for (int k = 0; k < n; k++) {
        const DspaSample* s = &list[k];
        if (!s->rom || !s->book || s->bytes < 9) continue;
        int have = 0;
        for (int i = 0; i < MAXENT && !have; i++) if (ents[i].mem && ents[i].key == s->key) have = 1;
        for (unsigned t = pre_tail; t != pre_head && !have; t++) if (pre_ring[t & (PRE_N - 1)].key == s->key) have = 1;
        for (int j = 0; j < k && !have; j++) if (list[j].key == s->key) have = 1;
        if (have) continue;
        if (pre_head - __atomic_load_n(&pre_tail, __ATOMIC_ACQUIRE) >= PRE_N) return;
        Ent* p = &pre_ring[pre_head & (PRE_N - 1)];
        if (sample_build(s, p, 0, 0)) continue;
        pre_n_dec++;
        __atomic_store_n(&pre_head, pre_head + 1, __ATOMIC_RELEASE);
    }
}
void pm_dspa_pre_read(unsigned* built, unsigned* in, unsigned* drop) { *built = pre_n_dec; *in = pre_n_in; *drop = pre_n_drop; }

static Ent* cache_get(const DspaSample* s) {
    unsigned now = now_ms();
    if (pre_head != pre_tail) pre_drain(now);
    for (int i = 0; i < MAXENT; i++)
        if (ents[i].mem && ents[i].key == s->key) { ents[i].stamp_ms = now; c_hit++; return &ents[i]; }
    if (!s->rom || !s->book || s->bytes < 9) return sample_drop("no rom data, book or size", s);
    c_miss++;
    unsigned long long t0 = svcGetSystemTick();
    Ent d;
    const char* why = sample_build(s, &d, 1, now);
    if (why) return sample_drop(why, s);
    Ent* e = cache_room(d.bytes, 60, now); /* the room made in sample_build: a free entry and the bytes */
    if (!e) { pm_linear_free(d.mem); return sample_drop("no cache entry after the room was made", s); }
    *e = d;
    e->stamp_ms = now;
    cache_used += e->bytes;
    if (cache_used > cache_peak) cache_peak = cache_used;
    unsigned us = tk_us(svcGetSystemTick() - t0);
    dec_n++; dec_us_sum += us; if (us > dec_us_max) dec_us_max = us;
    if (us >= 2000u && pm_debug_has("audiostat")) /* AUDNAP: a sync decode in a voice start on the render thread (the stall suspect) */
        pm_log_d("audio decode stall: key %08x bytes %u us %u cache used %u KB", (unsigned)s->key, (unsigned)s->bytes, us, cache_used / 1024);
    xd_n++; xd_us += us; if (us > xd_max) xd_max = us;
    return e;
}

/* ---------- channels ---------- */
/* PT22: the aux callback never ran on the O3DS (build 970 log), so the wet part of every voice was lost. While no aux callback has run
 * the wet and the delay sends are added to the main mix. */
static float raw_mix[PM_DSPA_VOICES][12];
static void mix_out(const float in[12], float out[12]) {
    int fold = ax_total == 0;
    for (int k = 0; k < 12; k++) out[k] = in[k];
    if (fold) {
        { /* A5: the fold adds up to 5 sends to the dry gain and ndsp has no limiter. The user hears this mix and it sounds right: no behaviour change, one log line when a voice can pass 1.0 */
            static int warned;
            float l = in[0] + in[4] + in[6] + in[8] + in[10], r = in[1] + in[5] + in[7] + in[9] + in[11];
            if (!warned && (l > 1.0f || r > 1.0f)) { warned = 1; pm_log("dspaudio: the folded wet sends of a voice reach gain %.2f (left) %.2f (right), above 1.0, ndsp sums the voices without a limiter", l, r); }
        }
        out[0] = in[0] + in[4] + in[6] + in[8] + in[10];
        out[1] = in[1] + in[5] + in[7] + in[9] + in[11];
        for (int k = 4; k < 12; k++) out[k] = 0.0f;
    }
}
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
    if (loop) c->endless = 1;
    c->exp += n;
}

/* census: a voice cut before its end. UNHEARD: the DSP has not started its first buffer. SQUEEZED: cut within blk blocks of its start. */
static void cut_census(const Chan* c) {
    if (c->nb && c->wb[0].status == NDSP_WBUF_QUEUED) pm_aud_ev(AC_UNHEARD, c->id);
    else if (tk_us(svcGetSystemTick() - c->t0) < c->blk * 5750u / 2u) pm_aud_ev(AC_SQUEEZED, c->id);
}

int pm_dspa_voice_start(int ch, const DspaSample* s, float rate, const float mix[12]) {
    Chan* c = &chn[ch];
    Ent* e = cache_get(s);
    if (c->e) { c->e->pins--; c->e = NULL; }
    ndspChnWaveBufClear(ch);
    c->nb = 0;
    c->active = 0;
    if (!e) return 0;
    e->pins++;
    c->e = e;
    c->t0 = svcGetSystemTick(); c->rate = rate; c->blk = 0; c->exp = 0; c->id = s->key; c->endless = 0; c->ended = 0;
    ndspChnSetFormat(ch, NDSP_FORMAT_MONO_PCM16);
    ndspChnSetInterp(ch, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(ch, rate);
    memcpy(raw_mix[ch], mix, sizeof raw_mix[ch]);
    { float m2[12]; mix_out(mix, m2); ndspChnSetMix(ch, m2); }
    if (!e->looped) chan_add(ch, e->pre, e->npre, 0);
    else {
        chan_add(ch, e->pre, e->npre, 0);
        if (e->loopCount == -1) chan_add(ch, e->body, e->nbody, 1);
        else {
            int reps = e->loopCount + 1;
            if (reps > 4) { reps = 4; vc_loopcap++; pm_aud_ev(AC_LOOPCAP, s->key); }
            for (int i = 0; i < reps; i++) chan_add(ch, e->body, e->nbody, 0);
            chan_add(ch, e->tail, e->ntail, 0);
        }
    }
    c->active = c->nb > 0;
    return c->active;
}

void pm_dspa_voice_update(int ch, float rate, const float mix[12]) {
    if (!chn[ch].active) return;
    unsigned long long t = svcGetSystemTick();
    ndspChnSetRate(ch, rate);
    memcpy(raw_mix[ch], mix, sizeof raw_mix[ch]);
    { float m2[12]; mix_out(mix, m2); ndspChnSetMix(ch, m2); }
    st_sets++;
    st_set_us += tk_us(svcGetSystemTick() - t);
}

void pm_dspa_voice_stop(int ch) {
    Chan* c = &chn[ch];
    if (c->active && !c->ended) cut_census(c);
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
    c->ended = 1;
    /* census: the sample ran out in under half its length (rate in samples a second, exp in samples) */
    if (!c->endless && c->exp && (float)tk_us(svcGetSystemTick() - c->t0) * 2.0f < (float)c->exp * 1000000.0f / c->rate) pm_aud_ev(AC_EARLYEND, c->id);
    return 1;
}

void pm_dspa_vcount(int what, unsigned id) { if ((unsigned)what < DSPA_VC_N) { vc_n[what]++; pm_aud_ev(vc_ac[what], id); } }

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
static unsigned ax_n[2], ax_us_sum, ax_us_max, fx_emitted, fx_dropped, fx_lastflip;
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
    if (!ax_total++) fx_flip = 1;
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

/* Reset all 24 ndsp channels and set the format, interpolation, rate and mix of each. Used at init and after sleep. */
static void chan_setup(void) {
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
}

/* After sleep the channels may still hold the wave buffers of the voices played before sleep. Drop them and set the channels up again.
 * Game thread only, when the audio job and the render queue are idle (the sleep wake hook checks this). */
void pm_dspa_rearm(void) {
    if (!pm_dspa_enabled) return;
    for (int c = 0; c < PM_DSPA_VOICES; c++) {
        Chan* k = &chn[c];
        if (k->e) { k->e->pins--; k->e->stamp_ms = now_ms(); k->e = NULL; }
        k->nb = 0;
        k->active = 0;
    }
    for (int i = 0; i < FXR_BUFS; i++) fxw[i].status = NDSP_WBUF_FREE; /* the effect return buffers are dropped with the channel */
    chan_setup();
    pm_dspa_epoch++;
}

int pm_dspa_init(void) {
    const char* kb = NULL;
    (void)kb;
    char kv[16];
    unsigned want = pm_debug_value("dspcache", kv, sizeof kv) > 0 ? (unsigned)atoi(kv) : 1536u;
    if (want < 256) want = 256;
    cache_budget = want * 1024u;
    LightLock_Init(&fx_lock);
    fxmem = (s16*)pm_linear_memalign(FXR_BUFS * FXFRAME * 4, 128);
    if (!fxmem) { pm_log_e("dspaudio: linearAlloc for the effect return failed"); return 0; }
    memset(fxmem, 0, FXR_BUFS * FXFRAME * 4);
    chan_setup();
    for (int i = 0; i < 2; i++) {
        ndspAuxSetEnable(i, true);
        ndspAuxSetVolume(i, 1.0f); /* the libctru default is 0.0 */
    }
    ndspAuxSetCallback(0, aux0_cb, NULL); ndspAuxSetCallback(1, aux1_cb, NULL);
    pm_log("dspaudio: aux enabled %d %d volume %.2f %.2f", ndspAuxIsEnabled(0), ndspAuxIsEnabled(1), ndspAuxGetVolume(0), ndspAuxGetVolume(1));
    ndspSetMasterVol(1.0f);
    pm_dspa_enabled = 1;
    pm_log("dspaudio: on, 24 engine voices on 23 ndsp channels 0 to 22 (lowest free at start), effect return on channel 23, cache %u KB", cache_budget / 1024);
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
    if (!next_due || now > next_due + 6 * BLK_TICKS) { if (next_due) st_late++; next_due = now; pm_aud_run_end(); }
    else if (now < next_due) {
        unsigned long long dt = next_due - now;
        svcSleepThread((s64)(dt * 1000ull / 268ull));
        pm_aud_run_end();
    } else pm_aud_catchup(tk_us(now - next_due)); /* census: late, no sleep: the block runs now (catch-up) */
    next_due += BLK_TICKS;
    t_blk0 = svcGetSystemTick();
}

void pm_dspa_block_done(int voices, int starts, int stops, int ends, unsigned costTicks) {
    (void)costTicks;
    if (fx_flip) { /* PT22: the first aux callback ran: stop adding the wet part to the main mix */
        fx_flip = 0;
        pm_log("dspaudio: first aux callback ran (core %u), wet sends go to the effect path now", ax_core);
        for (int c = 0; c < PM_DSPA_VOICES; c++) if (chn[c].active) { float m2[12]; mix_out(raw_mix[c], m2); ndspChnSetMix(c, m2); }
    }
    unsigned us = tk_us(svcGetSystemTick() - t_blk0);
    unsigned bl = au2_blk_backlog();
    for (int c = 0; c < PM_DSPA_VOICES; c++) if (chn[c].active) chn[c].blk++; /* census: blocks since each start */
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
    if (pm_debug_has("audiostat")) {
        pm_aud_window_log(dec_us_max);
        pm_log_d("audio voices w%u: starts %u no channel %u cut %u (cache give-up %u, loop cap %u), non-adpcm %u, playing max %u",
                 ++vc_win, st_starts, vc_n[DSPA_VC_NOCHAN], vc_n[DSPA_VC_GIVEUP] + vc_loopcap, vc_n[DSPA_VC_GIVEUP], vc_loopcap, vc_n[DSPA_VC_NONADPCM], st_voice_max);
        memset(vc_n, 0, sizeof vc_n);
        vc_loopcap = 0;
    }
    st_blocks = st_voice_sum = st_voice_max = st_starts = st_stops = st_ends = st_cost_sum = st_cost_max = st_late = st_bl_sum = st_bl_max = st_sets = st_set_us = 0;
    c_hit = c_miss = c_evict = c_fail = dec_n = dec_us_sum = dec_us_max = 0;
    ax_us_sum = ax_us_max = 0; ax_n[0] = ax_n[1] = 0;
}
