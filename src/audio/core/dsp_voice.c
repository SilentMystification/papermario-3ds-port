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
    u32 gen;
    float rate;
    float mix[12];
    float curL, curR;
} DspV;
static DspV dv[PM_DSPA_VOICES];
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

void au_dsp_block(void) {
    AuSynDriver* drv = gSynDriverPtr;
    int nv = 0, starts = 0, stops = 0, ends = 0, i;
    if (drv == nullptr) return;

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

    for (i = 0; i < PM_DSPA_VOICES; i++) {
        AuPVoice* pv = &drv->pvoices[i];
        AuEnvMixer* em = &pv->envMixer;
        AuLoadFilter* dec = &pv->decoder;
        DspV* v = &dv[i];
        Instrument* ins = dec->instrument;
        float rate, mix[12];
        s16 cL, cR;
        s32 dAfter;
        int b = pv->busID;

        if (em->motion != AL_PLAYING || ins == nullptr) {
            if (v->on) { pm_dspa_voice_stop(i); v->on = 0; stops++; }
            continue;
        }
        if (ins->type != AL_ADPCM_WAVE) { if (v->on) { pm_dspa_voice_stop(i); v->on = 0; } continue; }
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

        if (!v->on || v->gen != au2_vgen_get(i)) {
            DspaSample s;
            s.key = (unsigned)(uintptr_t)ins->wavData;
            s.bytes = ins->wavDataLength;
            s.rom = pm_rom_ptr(s.key, s.bytes);
            s.book = ins->predictor;
            s.bookBytes = ins->codebookSize;
            s.lstate = (const short*)ins->loopState;
            s.loopStart = ins->loopStart; s.loopEnd = ins->loopEnd; s.loopCount = ins->loopCount;
            if (v->on) pm_dspa_voice_stop(i);
            pm_dspa_voice_start(i, &s, rate, mix);
            v->on = 1; v->gen = au2_vgen_get(i); starts++;
            memcpy(v->mix, mix, sizeof mix); v->rate = rate;
        } else {
            int changed = rate != v->rate, k;
            for (k = 0; k < 12 && !changed; k++) if (mix[k] != v->mix[k]) changed = 1;
            if (changed) { pm_dspa_voice_update(i, rate, mix); memcpy(v->mix, mix, sizeof mix); v->rate = rate; }
        }

        if (pm_dspa_voice_ended(i)) {
            /* sample ran out: same state reset as the end of au_pull_voice, and the same stop record */
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
            pm_dspa_voice_stop(i);
            v->on = 0;
            ends++;
        }
    }
    pm_dspa_block_done(nv, starts, stops, ends, 0);
}
