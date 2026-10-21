#ifndef PM_DSPAUD_H
#define PM_DSPAUD_H
/* AUDDSP A3: audio mixing on the 3DS DSP (switch dspaudio). See docs/plans/AUD_DSP.md. Plain C. */
#include <stdint.h>

extern volatile int pm_dspa_enabled;     /* 1 after pm_dspa_init: the AUC_BLOCK command drives ndsp, not the mixer */
#define PM_DSPA_VOICES 23                /* ndsp channels 0 to 22 are voices, channel 23 returns the effects */

typedef struct {
    unsigned key;                        /* ROM address of the sample data */
    const unsigned char* rom;            /* resident ROM bytes (VADPCM, 9 bytes per 16 samples) */
    unsigned bytes;                      /* rounded to 9 */
    const short* book;                   /* codebook, 8 predictors of 2 x 8 s16 */
    unsigned bookBytes;
    const short* lstate;                 /* loop state, 16 s16, or NULL */
    int loopStart, loopEnd, loopCount;   /* loopEnd 0 = no loop, loopCount -1 = endless */
} DspaSample;

int pm_dspa_init(void);                                           /* after ndspInit; 0 = failed */
void pm_dspa_shutdown(void);
void pm_dspa_pace(void);                                          /* render thread: wait until the next block is due */
void pm_dspa_voice_start(int ch, const DspaSample* s, float rate, const float mix[12]);   /* 0 = sample not available: ch stays silent */
void pm_dspa_voice_update(int ch, float rate, const float mix[12]);
void pm_dspa_voice_stop(int ch);
int pm_dspa_voice_ended(int ch);                                  /* all wave buffers of the channel are done */
void pm_dspa_master(float vol);
/* effects: bus 0 and 1. params = N64 parameter table (sections, length, 8 values per section) or NULL. */
void pm_dspa_fx_set(int bus, const int* params, int type);
/* channel delay: side 0 none, 1 left, 2 right, blocks of 184 samples */
void pm_dspa_delay_set(int bus, int side, int blocks);
void pm_dspa_block_done(int voices, int starts, int stops, int ends, unsigned costTicks);
unsigned pm_dspa_backlog_blocks(void);                            /* blocks emitted by control and not yet processed */
void pm_dspa_stats_log(void);
void pm_dspa_log_once(const char* fmt, ...);

/* decomp side (src/audio/core/dsp_voice.c) */
void au_dsp_block(void);                                          /* one 184 sample block, render thread */
unsigned au2_blk_backlog(void);
const unsigned char* pm_rom_ptr(unsigned addr, unsigned len);
#endif
