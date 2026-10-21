#ifndef PM_AU2_H
#define PM_AU2_H

/* AU2: command queue between the audio control part (nuAuMgr, sequencers, au_syn_* calls) and the render part (mixer thread).
 * See docs/plans/AU2_design.md. Plain C, only stdint types: the decomp files and the port files include it. */
#include <stdint.h>

enum {
    AUC_NONE,
    AUC_STOP_VOICE,
    AUC_START_VOICE,
    AUC_START_PARAMS,
    AUC_SET_WAVETABLE,
    AUC_PITCH,
    AUC_MIXER,
    AUC_PAN_FXMIX,
    AUC_VOL_DELTA,
    AUC_PAN,
    AUC_FXMIX,
    AUC_PVOICE_BUS,
    AUC_BUS_VOL,
    AUC_BUS_EFFECT,
    AUC_BUS_FXPARAM,
    AUC_DELAY_TIME,
    AUC_DELAY_LEFT,
    AUC_DELAY_RIGHT,
    AUC_DELAY_OFF,
    AUC_DELAY_INIT,
    AUC_STEREO_SET,
    AUC_STEREO_REPAN,
    AUC_GVOL_USE,
    AUC_GVOL_SET,
    AUC_BLOCK,      /* render one 184-sample block into the PCM buffer at v0 */
    AUC_CLEAN_DMA,  /* nuAuCleanDMABuffers */
    AUC_POST,       /* pm_audio_post(v0, v1 bytes) */
    AUC_N
};

/* 20 bytes. Meaning of the fields per op: see au2_exec in src/audio/core/syn_driver.c. */
typedef struct AuCmd {
    uint8_t op, idx, b1, b2;
    int16_t s1, s2;
    int32_t v0, v1, v2;
} AuCmd;

#define AU2_SLOT_CMDS 256
#define AU2_SLOTS 8

/* 1 when the queue is active (render thread running). 0: every call runs at once on the calling thread (switch au2off, or before start). */
extern volatile int pm_au2_active;
void pm_au2_emit(const AuCmd* c);          /* control thread (or game thread while the audio thread is joined) */
int pm_au2_idle(void);                     /* 1: render thread idle and no open job. Control thread only. */
void pm_au2_commit(void);                  /* publish the open job */
void pm_au2_sync(int reason);              /* commit and wait until the render thread ran everything. Any thread but the render thread. */
uint32_t pm_au2_committed(void);           /* number of jobs committed so far */
void pm_au2_wait_jobs(uint32_t target);    /* wait until `target` jobs ran (a value of pm_au2_committed). Not on the render thread. */
int pm_au2_start(void);                    /* create the render thread; 0 = not started (au2off or failure) */
void pm_au2_stop(void);                    /* drain and end the render thread */
int pm_au2_on_render(void);                /* 1 on the render thread */
void pm_au2_stats_log(void);               /* called at each 300th control join */
void pm_au2_rom_lock(void);                /* LightLock around the ROM file read while the queue is active */
void pm_au2_rom_unlock(void);
void au2_exec(const AuCmd* c);             /* syn_driver.c: run one command (render thread, or inline) */
void au2_post_exec(void* pcm, unsigned bytes); /* pm_audio.c: AUC_POST body */
void au2_clean_dma_exec(void);             /* system.c */
void au2_stop_record(int voiceIdx);        /* syn_driver.c: render side, the sample of a voice ended. Control applies it two frames later (au2_apply_stops). */

enum { AU2_SYNC_BANK = 0, AU2_SYNC_MAP, AU2_SYNC_SAVE, AU2_SYNC_SHUTDOWN, AU2_SYNC_FULL, AU2_SYNC_N };

#endif
