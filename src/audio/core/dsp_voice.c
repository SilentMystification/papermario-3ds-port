#include "common.h"
#include "audio.h"
#include "audio/core.h"
#include "pm_dspaud.h"

/* AUDDSP A3: the render thread runs this once per AUC_BLOCK instead of the RSP mixer.
 * The shadow voices (AuPVoice, filled by the au_syn_*_x calls) hold volume, pan, fxMix, ramp, pitch and sample.
 * This code turns them into ndsp channel parameters. See docs/plans/AUD_DSP.md section 6. */

#define AUEQPOWER_LENGTH 128
extern s16 AuEqPower[128];
extern s16 _getRate(f64 vol, f64 tgt, s32 count, u16* ratel);
extern s32 SMALL_ROOM_PARAMS[], BIG_ROOM_PARAMS[], ECHO_PARAMS[], CHORUS_PARAMS[], NULL_PARAMS[];
extern s32* AU_FX_CUSTOM_PARAMS[];
extern u8 AuDelayedBusID;
extern s16 AuDelayedChannel;
extern s32 AuDelayCount;
u32 au2_vgen_get(int idx);
void au2_stop_record(int voiceIdx);

typedef struct {
    u8 on;
    u8 chp;                 /* ndsp channel + 1, 0 = no channel */
    u32 gen;
    u32 ncgen;              /* census: generation already counted as no channel */
    float rate;
    float mix[12];
    float curL, curR;
    u8 retry;
} DspV;
/* AUDV: the engine has 24 voices (system.c num_pvoice) and ndsp has 23 voice channels (channel 23 returns the effects). A voice takes
 * the lowest free channel when it starts and gives it back when it stops or ends, so the voice index does not decide the channel.
 * Before, voice i played on channel i and voice 23 (sound effect player 7, the first one the free-player search takes) had none. */
#define AU_DSP_NV 24
#define CH_ALL ((1u << PM_DSPA_VOICES) - 1u)
static DspV dv[AU_DSP_NV];
static u32 ch_used;
static unsigned dv_epoch;
/* a start that gets no cache entry is retried on the next block; 16 blocks (about 90 ms) outlasts the 60 ms eviction guard */
#define AU2_START_RETRY 16
static int fx_type[2] = { -1, -1 };
static const s32* fx_ptr[2];
static int dl_bus = -1, dl_side, dl_blocks;
static u16 gv_last = 0xFFFF;
static u8 warned_bus, warned_type;

static s16 getvol(s16 ivol, s32 samples, s16 ratem, u16 ratel) {
    s32 t;
    samples >>= 3;
    if (samples == 0) return ivol;
    t = (ratel * samples) >> 16;
    t += ratem * samples;
    return ivol + t;
}

static const s32* fx_params(u8 type) {
    switch (type) {
        case AU_FX_SMALLROOM: return SMALL_ROOM_PARAMS;
        case AU_FX_BIGROOM: case AU_FX_OTHER_BIGROOM: return BIG_ROOM_PARAMS;
        case AU_FX_ECHO: return ECHO_PARAMS;
        case AU_FX_CHORUS: return CHORUS_PARAMS;
        case AU_FX_CUSTOM_0: return AU_FX_CUSTOM_PARAMS[0];
        case AU_FX_CUSTOM_1: return AU_FX_CUSTOM_PARAMS[1];
        case AU_FX_CUSTOM_2: return AU_FX_CUSTOM_PARAMS[2];
        case AU_FX_CUSTOM_3: return AU_FX_CUSTOM_PARAMS[3];
        default: return NULL_PARAMS;
    }
}

unsigned au2_blk_backlog(void);

static void v_release(DspV* v) {
    if (v->chp) { ch_used &= ~(1u << (v->chp - 1)); v->chp = 0; }
}

/* the sample ran out (or cannot play): same state reset as the end of au_pull_voice, and the same stop record */
static void v_end(AuPVoice* pv, Instrument* ins, int i) {
    AuEnvMixer* em = &pv->envMixer;
    AuLoadFilter* dec = &pv->decoder;
    em->motion = AL_STOPPED;
    em->dirty = true;
    em->volume = 1;
    pv->resampler.delta = 0.0f;
    pv->resampler.first = true;
    dec->lastsam = 0;
    dec->first = 1;
    dec->sample = 0;
    dec->memin = (s32)ins->wavData;
    dec->loop.count = ins->loopCount;
    au2_stop_record(i);
}

void au_dsp_block(void) {
    AuSynDriver* drv = gSynDriverPtr;
    int nv = 0, starts = 0, stops = 0, ends = 0, i;
    int nvoice;
    if (drv == nullptr) return;
    nvoice = drv->num_pvoice < AU_DSP_NV ? drv->num_pvoice : AU_DSP_NV;
    if (dv_epoch != pm_dspa_epoch) { /* pm_dspa_rearm cleared every channel (sleep): the playing voices start again on new channels */
        dv_epoch = pm_dspa_epoch;
        memset(dv, 0, sizeof dv);
        ch_used = 0;
    }

    /* effects and channel delay */
    for (i = 0; i < 2; i++) {
        AuFxBus* b = &drv->fxBus[i];
        const s32* p = b->curEffectType == AU_FX_NONE ? nullptr : fx_params(b->curEffectType);
        if (fx_type[i] != b->curEffectType || fx_ptr[i] != p) {
            fx_type[i] = b->curEffectType;
            fx_ptr[i] = p;
            pm_dspa_fx_set(i, (const int*)p, b->curEffectType);
        }
    }
    {
        int bus = -1, side = 0, blocks = 0;
        if (AuDelayedChannel == AU_DELAY_CHANNEL_LEFT) side = 1;
        else if (AuDelayedChannel == AU_DELAY_CHANNEL_RIGHT) side = 2;
        if (side && AuDelayCount >= 2 && AuDelayedBusID < 2) { bus = AuDelayedBusID; blocks = AuDelayCount - 1; } else side = 0;
        if (bus != dl_bus || side != dl_side || blocks != dl_blocks) {
            dl_bus = bus; dl_side = side; dl_blocks = blocks;
            pm_dspa_delay_set(bus < 0 ? 0 : bus, side, blocks);
        }
    }
    {
        u16 gv = AuUseGlobalVolume ? AuGlobalVolume : 0x7FFF;
        if (gv != gv_last) { gv_last = gv; pm_dspa_master((float)gv / 32767.0f); }
    }

    for (i = 0; i < nvoice; i++) {
        AuPVoice* pv = &drv->pvoices[i];
        AuEnvMixer* em = &pv->envMixer;
        AuLoadFilter* dec = &pv->decoder;
        DspV* v = &dv[i];
        Instrument* ins = dec->instrument;
        float rate, mix[12];
        s16 cL, cR;
        s32 dAfter;
        int b = pv->busID, c;

        if (em->motion != AL_PLAYING || ins == nullptr) {
            if (v->on) { pm_dspa_voice_stop(v->chp - 1); v->on = 0; stops++; }
            v_release(v);
            continue;
        }
        if (ins->type != AL_ADPCM_WAVE) { if (v->on) { pm_dspa_voice_stop(v->chp - 1); v->on = 0; } v_release(v); if (!v->on && v->gen != au2_vgen_get(i)) { v->gen = au2_vgen_get(i); pm_dspa_vcount(DSPA_VC_NONADPCM, (unsigned)(uintptr_t)ins->wavData); } continue; }
        nv++;

        /* envelope: same ramp data as the RSP mixer (cvol, ltgt, rate, delta) */
        if (em->dirty) {
            em->dirty = false;
            if (!AuSynUseStereo) {
                em->ltgt = (em->volume * AuEqPower[AUEQPOWER_LENGTH / 2]) >> 15;
                em->rtgt = (em->volume * AuEqPower[AUEQPOWER_LENGTH / 2]) >> 15;
            } else {
                em->ltgt = (em->volume * AuEqPower[em->pan]) >> 15;
                em->rtgt = (em->volume * AuEqPower[AUEQPOWER_LENGTH - em->pan - 1]) >> 15;
            }
            em->lratm = _getRate(em->cvolL, em->ltgt, em->segEnd, &em->lratl);
            em->rratm = _getRate(em->cvolR, em->rtgt, em->segEnd, &em->rratl);
        }
        em->delta += AUDIO_SAMPLES;
        if (em->segEnd < em->delta) em->delta = em->segEnd;
        dAfter = em->delta;
        if (dAfter >= em->segEnd) {
            cL = em->ltgt; cR = em->rtgt;
        } else {
            cL = getvol(em->cvolL, dAfter, em->lratm, em->lratl);
            cR = getvol(em->cvolR, dAfter, em->rratm, em->rratl);
        }
        if (cL < 0) cL = 0;
        if (cR < 0) cR = 0;

        {
            float gl = (float)cL / 32768.0f, gr = (float)cR / 32768.0f;
            float dry = (float)em->dryamt / 32768.0f, wet = (float)em->wetamt / 32768.0f;
            float bus = 1.0f;
            int k;
            for (k = 0; k < 12; k++) mix[k] = 0.0f;
            if (b < 4) bus = (float)drv->fxBus[b].gain / 32768.0f;
            if (b >= 2 && b != 0xFF && !warned_bus) { warned_bus = 1; pm_dspa_log_once("bus %d is not mapped (only 0 and 1), voice plays dry", b); }
            if (b < 2 && dl_side && b == dl_bus) {
                /* channel delay on this bus: dry goes to the back channels of the aux, the CPU delays one side */
                mix[6 + 4 * b] = gl * dry * bus; mix[7 + 4 * b] = gr * dry * bus;
            } else {
                mix[0] = gl * dry * bus; mix[1] = gr * dry * bus;
            }
            if (b < 2) { mix[4 + 4 * b] = gl * wet * bus; mix[5 + 4 * b] = gr * wet * bus; }
        }
        rate = pv->resampler.ratio * 32000.0f;
        if (rate < 100.0f) rate = 100.0f;

        if (!v->chp) {
            u32 fr = ~ch_used & CH_ALL;
            if (!fr) { /* all 23 channels play: this voice waits for one */
                u32 g = au2_vgen_get(i);
                if (v->ncgen != g) { v->ncgen = g; pm_dspa_vcount(DSPA_VC_NOCHAN, (unsigned)(uintptr_t)ins->wavData); }
                continue;
            }
            v->chp = (u8)(__builtin_ctz(fr) + 1);
            ch_used |= 1u << (v->chp - 1);
            v->on = 0;
        }
        c = v->chp - 1;

        if (!v->on || v->gen != au2_vgen_get(i)) {
            DspaSample s;
            s.key = (unsigned)(uintptr_t)ins->wavData;
            s.bytes = ins->wavDataLength;
            s.rom = pm_rom_ptr(s.key, s.bytes);
            s.book = ins->predictor;
            s.bookBytes = ins->codebookSize;
            s.lstate = (const short*)ins->loopState;
            s.loopStart = ins->loopStart; s.loopEnd = ins->loopEnd; s.loopCount = ins->loopCount;
            {
                u32 g = au2_vgen_get(i);
                if (v->gen != g) v->retry = 0;
                if (v->on) pm_dspa_voice_stop(c);
                v->on = 0;
                if (pm_dspa_voice_start(c, &s, rate, mix)) {
                    v->on = 1; v->gen = g; v->retry = 0; starts++;
                    memcpy(v->mix, mix, sizeof mix); v->rate = rate;
                } else if (++v->retry > AU2_START_RETRY) {
                    /* give up: the sample cannot be put in the cache. End the voice as if its sample ran out, so the engine frees it
                     * (before, it stayed silent and held its voice until the next start) */
                    v->retry = 0;
                    v_end(pv, ins, i);
                    v_release(v);
                    pm_dspa_vcount(DSPA_VC_GIVEUP, (unsigned)(uintptr_t)ins->wavData);
                    continue;
                }
            }
        } else {
            int changed = rate != v->rate, k;
            for (k = 0; k < 12 && !changed; k++) if (mix[k] != v->mix[k]) changed = 1;
            if (changed) { pm_dspa_voice_update(c, rate, mix); memcpy(v->mix, mix, sizeof mix); v->rate = rate; }
        }

        if (pm_dspa_voice_ended(c)) {
            v_end(pv, ins, i);
            pm_dspa_voice_stop(c);
            v->on = 0;
            v_release(v);
            ends++;
        }
    }
    pm_dspa_block_done(nv, starts, stops, ends, 0);
}

/* BTLWIPE F4: the samples a song can start, for the pre-decode (pm_prefetch.c thread, read only). A static scan of the BGM file (raw big-endian
 * bytes in the resident ROM image): every phrase of its compositions, every track of a phrase, each track stream read like
 * au_bgm_player_update_playing reads it (delays, notes, commands with SeqCmdArgCounts arguments), the Detour and Branch targets too. The
 * instruments it can select: UseInstrument (F5: the BGM instrument table below 0x80, the global PRG table above), OverridePatch (E8), and the
 * drums its drum track notes name (PER below pitch 72, the BGM drum table above). Each resolves through bankSets like au_get_instrument; the
 * aux bank sets (0 and 7) are skipped (the song load replaces them). A sample is built as the voice start builds it (au_dsp_block), so the
 * cache key and layout are the same. A misread byte costs one extra decode at most: every offset and index is bounded. Returns the count. */
extern s8 SeqCmdArgCounts[];
static unsigned be16(const u8* p) { return ((unsigned)p[0] << 8) | p[1]; }
typedef struct { AuGlobals* g; const u8* b; unsigned len; DspaSample* out; int n, max; } SongScan;
static void song_add(SongScan* sc, unsigned bankPatch) {
    AuGlobals* g = sc->g;
    unsigned bank = bankPatch >> 8, set = (bank & 0x70) >> 4;
    if (sc->n >= sc->max || set == BANK_SET_IDX_0 || set == BANK_SET_IDX_7 || !g->bankSets[set]) return;
    Instrument* ins = (*g->bankSets[set])[bankPatch & 0xFF];
    if (!ins || ins->type != AL_ADPCM_WAVE) return;
    unsigned key = (unsigned)(uintptr_t)ins->wavData;
    for (int k = 0; k < sc->n; k++) if (sc->out[k].key == key) return;
    DspaSample* s = &sc->out[sc->n];
    s->key = key;
    s->bytes = ins->wavDataLength;
    s->rom = pm_rom_ptr(s->key, s->bytes);
    s->book = ins->predictor;
    s->bookBytes = ins->codebookSize;
    s->lstate = (const short*)ins->loopState;
    s->loopStart = ins->loopStart; s->loopEnd = ins->loopEnd; s->loopCount = ins->loopCount;
    if (s->rom) sc->n++;
}
static void song_use(SongScan* sc, unsigned idx) { /* F5 UseInstrument */
    const u8* b = sc->b;
    unsigned iOff = be16(b + 0x20) << 2, iCnt = be16(b + 0x22);
    if (idx < BGM_MAX_INSTRUMNETS) {
        if (iOff && idx < iCnt && iOff + 8 * idx + 2 <= sc->len) song_add(sc, be16(b + iOff + 8 * idx));
        else song_add(sc, sc->g->defaultPRGEntry.bankPatch);
    } else if (idx - BGM_MAX_INSTRUMNETS < PRG_MAX_COUNT && sc->g->dataPRG) song_add(sc, sc->g->dataPRG[idx - BGM_MAX_INSTRUMNETS].bankPatch);
}
static void song_drum(SongScan* sc, unsigned pitch) {
    const u8* b = sc->b;
    unsigned dOff = be16(b + 0x1C) << 2, dCnt = be16(b + 0x1E);
    if (pitch < 72) { if (sc->g->dataPER) song_add(sc, sc->g->dataPER->drums[pitch].bankPatch); }
    else if (dOff && pitch - 72 < dCnt && dOff + 12 * (pitch - 72) + 2 <= sc->len) song_add(sc, be16(b + dOff + 12 * (pitch - 72)));
}
#define SS_Q 48
int au_song_samples(s32 songID, DspaSample* out, int max) {
    AuGlobals* g = gSoundGlobals;
    SBNFileEntry fe;
    if (!g || songID < 0 || songID >= g->songListLength) return 0;
    if (au_fetch_SBN_file(g->songList[songID].bgmFileIndex, AU_FMT_BGM, &fe) != AU_RESULT_OK) return 0;
    SongScan sc = { g, NULL, fe.data & 0xFFFFFF, out, 0, max };
    const u8* b = sc.b = pm_rom_ptr((unsigned)fe.offset, sc.len);
    if (!b || sc.len < 0x24) return 0;
    unsigned qo[SS_Q], ql[SS_Q], qd[SS_Q], nq = 0; /* streams to read: offset in the file, byte limit (0 none), drum track */
    for (unsigned c = 0; c < 4; c++) { /* compositions: PLAY_PHRASE commands (top nibble 1) until COMP_END */
        unsigned co = be16(b + 0x14 + 2 * c) << 2;
        if (!co) continue;
        for (unsigned p = co; p + 4 <= sc.len && p < co + 4096u; p += 4) {
            unsigned cmd = (unsigned)be16(b + p) << 16 | be16(b + p + 2);
            if (cmd == BGM_COMP_END) break;
            if ((cmd >> 12) != (BGM_COMP_PLAY_PHRASE << 16)) continue;
            unsigned ph = co + ((cmd & 0xFFFF) << 2);
            for (unsigned t = 0; t < 16 && ph + 4 * t + 4 <= sc.len; t++) {
                unsigned ti = (unsigned)be16(b + ph + 4 * t) << 16 | be16(b + ph + 4 * t + 2), to = ti >> 16;
                if (!to || (ti & 0x100)) continue;
                unsigned so = ph + to, k;
                for (k = 0; k < nq && qo[k] != so; k++) {}
                if (k == nq && nq < SS_Q) { qo[nq] = so; ql[nq] = 0; qd[nq] = (ti >> 7) & 1; nq++; }
            }
        }
    }
    for (unsigned q = 0; q < nq; q++) {
        unsigned p = qo[q], end = ql[q] ? qo[q] + ql[q] : sc.len, drum = qd[q];
        if (end > sc.len) end = sc.len;
        for (unsigned steps = 0; p < end && steps < 8192u; steps++) {
            unsigned op = b[p++];
            if (op < 0x80) {
                if (op == 0) break;
                if (op >= 0x78) p++;
                continue;
            }
            if (op < 0xD4) { /* note: velocity, length (two bytes from 0xC0) */
                if (drum) song_drum(&sc, op & 0x7F);
                p++;
                if (p < end && b[p] >= 0xC0) p++;
                p++;
                continue;
            }
            if (op < 0xE0) break; /* not an opcode the player reads */
            unsigned na = (unsigned)SeqCmdArgCounts[op - 0xE0];
            if (p + na > end) break;
            if (op == 0xF5) song_use(&sc, b[p]);
            else if (op == 0xE8) song_add(&sc, (unsigned)b[p] << 8 | b[p + 1]);
            else if ((op == 0xFE || op == 0xFC) && nq < SS_Q) {
                unsigned to = be16(b + p);
                if (op == 0xFE) { qo[nq] = to; ql[nq] = b[p + 2]; qd[nq] = drum; nq++; } /* Detour: length bytes at offset */
                else for (unsigned e = 0; e < b[p + 2] && to + 3 * e + 3 <= sc.len && nq < SS_Q; e++) { /* Branch: table of offset and drum flag */
                    qo[nq] = be16(b + to + 3 * e); ql[nq] = 0; qd[nq] = b[to + 3 * e + 2] != 0; nq++;
                }
            }
            p += na;
        }
    }
    return sc.n;
}
