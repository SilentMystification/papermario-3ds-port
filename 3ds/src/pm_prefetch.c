/* MAPX-4: the map prefetch. A map change knows its target at CHANGE_MAP_INIT (set_game_mode(GAME_MODE_CHANGE_MAP), pm_map.c), about 15 steps
 * before the load. A core-1 thread (lowest priority, below the audio threads) fills two side buffers allocated from the heap when the job is
 * posted (skipped when the allocation fails): slot 0 reads romfs:/shape/<map>_shape.nds (MAPROMFS: no Yay0 decode of a shape), slot 1 Yay0-decodes
 * <map>_hit from the asset bytes in the RAM ROM image. decode_yay0 (pm_map.c) takes a buffer whose source bytes and size match (and, for the shape,
 * the name): it waits for the job (it is done long before the load), copies the bytes and does the fix-ups and invalidation as before. The bytes are
 * the same as the game-thread load or decode of the same source, so the result is the same; a job that failed or never ran leaves decode_yay0 to load or decode on the game thread.
 * A taken or failed buffer is freed by decode_yay0's take; a buffer never taken is freed at the next post.
 * The game thread touches a slot only after the job has ended (pf_wait), the worker only between post and its end signal.
 * BTLWIPE F4: the same thread pre-decodes the samples of the battle song (pm_prefetch_song, posted by bgm_push_battle_song at the start of the
 * star wipe) into the DSP sample cache (pm_dspa_predecode), so the voice starts of the new song about 11 steps later find their samples and
 * the render thread does not decode on core 1 while the wipe runs. A posted map job runs first, between two samples. */
#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "pm_port.h"
#include "pm_thread.h"
#include "pm_dspaud.h"

int pm_yay0_raw(const unsigned char* src, unsigned src_len, unsigned char* dst, unsigned dst_len); /* pm_map.c */
int pm_shape_load_nds(unsigned char* buf, unsigned size, const char* name); /* pm_map.c */
void pm_log_prof_line(const char* fmt, ...);

#define PF_N 2
static const unsigned pf_cap[PF_N] = { 0x40000u, 0x20000u }; /* shape (the shape buffer size), hit */
static unsigned char* pf_buf[PF_N]; /* heap, game thread only: set in post, freed in take or the next post */
typedef struct { char name[24]; const unsigned char* src; unsigned clen, dec; volatile int st; } PfSlot; /* st: 0 empty, 1 posted, 2 ready, 3 failed */
static PfSlot pf[PF_N];
static Thread pf_th;
static LightEvent pf_go, pf_done;
static volatile int pf_busy; /* a posted job not yet ended */
static volatile int pf_song = -1; /* BTLWIPE F4: a posted song pre-decode (song id), -1 none */
int au_song_samples(int songID, DspaSample* out, int max); /* dsp_voice.c */
void pm_dspa_predecode(const DspaSample* list, int n); /* pm_dspaud.c */
#define PF_SMP 64
static DspaSample pf_smp[PF_SMP]; /* worker only */

static void pf_maps(void) { /* the posted map job, if any (pf_busy is set after its slots, cleared after they ran) */
    if (!__atomic_load_n(&pf_busy, __ATOMIC_ACQUIRE)) return;
    for (int i = 0; i < PF_N; i++) {
        PfSlot* s = &pf[i];
        if (s->st != 1) continue;
        s->st = (i == 0 ? pm_shape_load_nds(pf_buf[i], s->dec, s->name) : pm_yay0_raw(s->src, 0x100000u, pf_buf[i], s->dec)) ? 2 : 3; /* MAPROMFS: slot 0 reads the .nds file */
    }
    __atomic_store_n(&pf_busy, 0, __ATOMIC_RELEASE);
    LightEvent_Signal(&pf_done);
}
#define PFF_N 3 /* MTX2: the .nds header (16 B), the .seam, the .dlc of one shape */
static char pff_path[PFF_N][48]; static unsigned char* pff_buf[PFF_N]; static unsigned pff_len[PFF_N]; static volatile int pff_busy; static LightEvent pff_done; /* MTX2 */
static void pff_run(void) { if (!__atomic_load_n(&pff_busy, __ATOMIC_ACQUIRE)) return; /* worker: MTX2 */
    for (int i = 0; i < PFF_N; i++) { FILE* f = fopen(pff_path[i], "rb"); long n = -1; if (f && fseek(f, 0, SEEK_END) == 0) n = ftell(f); if (i == 0 && n > 16) n = 16;
        if (f && n > 0 && fseek(f, 0, SEEK_SET) == 0 && (pff_buf[i] = malloc((size_t)n)) && fread(pff_buf[i], 1, (size_t)n, f) == (size_t)n) pff_len[i] = (unsigned)n;
        else { free(pff_buf[i]); pff_buf[i] = NULL; pff_len[i] = 0; } if (f) fclose(f); }
    __atomic_store_n(&pff_busy, 0, __ATOMIC_RELEASE); LightEvent_Signal(&pff_done); }
static void pff_wait(void) { while (__atomic_load_n(&pff_busy, __ATOMIC_ACQUIRE)) LightEvent_Wait(&pff_done); }
void pm_prefetch_files_release(void) { pff_wait(); for (int i = 0; i < PFF_N; i++) { free(pff_buf[i]); pff_buf[i] = NULL; pff_len[i] = 0; pff_path[i][0] = 0; } }
static int pf_start(void); void pm_prefetch_files(const char* shape) { static const char* const ext[PFF_N] = { "nds", "seam", "dlc" }; pm_prefetch_files_release(); if (!pf_start()) return;
    for (int i = 0; i < PFF_N; i++) snprintf(pff_path[i], sizeof pff_path[i], "romfs:/shape/%s.%s", shape, ext[i]);
    LightEvent_Clear(&pff_done); __atomic_store_n(&pff_busy, 1, __ATOMIC_RELEASE); LightEvent_Signal(&pf_go); }
FILE* pm_pf_fopen(const char* path, const char* mode) { /* a posted path from memory (waits for the job), else the file */
    for (int i = 0; i < PFF_N; i++) if (pff_path[i][0] && strcmp(path, pff_path[i]) == 0) { pff_wait(); if (pff_buf[i]) { pm_xf.pff_n++; return fmemopen(pff_buf[i], pff_len[i], mode); } break; }
    return fopen(path, mode); }
static void pf_main(void* arg) {
    (void)arg;
    for (;;) {
        LightEvent_Wait(&pf_go);
        pf_maps();
        pff_run(); /* MTX2 */
        int song = __atomic_exchange_n(&pf_song, -1, __ATOMIC_ACQ_REL);
        if (song < 0) continue;
        int n = au_song_samples(song, pf_smp, PF_SMP);
        for (int k = 0; k < n; k++) {
            pf_maps();
            pm_dspa_predecode(&pf_smp[k], 1);
        }
    }
}
static int pf_start(void) { /* 0 when the thread cannot run (the caller does the work itself) */
    if (!pf_th) {
        LightEvent_Init(&pf_go, RESET_ONESHOT);
        LightEvent_Init(&pf_done, RESET_STICKY);
        LightEvent_Init(&pff_done, RESET_STICKY); /* MTX2 */
        pf_th = pm_thread_create(pf_main, NULL, 16 * 1024, 0x3F, 1, "prefetch"); /* MAPROMFS: stdio and FS calls on this thread now */
        if (!pf_th) { pf_th = (Thread)1; pm_log_w("prefetch: no core-1 thread, the game thread decodes"); }
    }
    return pf_th != (Thread)1;
}
/* Logic thread, bgm_push_battle_song: pre-decode the samples of songID. A song still pending is replaced. */
void pm_prefetch_song(int songID) {
    if (songID < 0 || !pm_dspa_enabled || !pf_start()) return;
    __atomic_store_n(&pf_song, songID, __ATOMIC_RELEASE);
    LightEvent_Signal(&pf_go);
}
static void pf_wait(void) {
    while (__atomic_load_n(&pf_busy, __ATOMIC_ACQUIRE)) LightEvent_Wait(&pf_done);
}
static void pf_release(int i) {
    free(pf_buf[i]);
    pf_buf[i] = NULL;
    pf[i].st = 0;
}
/* Game thread, at the start of a map change: names and sources of the two assets (src: their bytes in the ROM image, NULL when not resident). */
void pm_prefetch_post(const char* shape, const unsigned char* ssrc, unsigned scl, unsigned sdec, const char* hit, const unsigned char* hsrc, unsigned hcl, unsigned hdec) {
    pf_wait();
    for (int i = 0; i < PF_N; i++) pf_release(i);
    if (!pf_start()) return;
    const char* nm[PF_N] = { shape, hit };
    const unsigned char* sr[PF_N] = { ssrc, hsrc };
    const unsigned dc[PF_N] = { sdec, hdec }, cl[PF_N] = { scl, hcl };
    int any = 0;
    for (int i = 0; i < PF_N; i++) {
        PfSlot* s = &pf[i];
        if (!sr[i] || !dc[i] || dc[i] > pf_cap[i]) continue;
        pf_buf[i] = memalign(16, dc[i]);
        if (!pf_buf[i]) { pm_log_w("prefetch: no heap for %s, the game thread decodes it", nm[i]); continue; }
        snprintf(s->name, sizeof s->name, "%s", nm[i]);
        s->src = sr[i]; s->clen = cl[i]; s->dec = dc[i]; s->st = 1; any = 1;
    }
    if (!any) return;
    LightEvent_Clear(&pf_done);
    __atomic_store_n(&pf_busy, 1, __ATOMIC_RELEASE);
    LightEvent_Signal(&pf_go);
}
/* Game thread, in decode_yay0: 1 when a slot loaded the same source (size and the compressed bytes equal: the shape load in world.c calls
 * load_asset_by_name inside its own file, so no asset name reaches decode_yay0 for it) and its dec bytes were copied to dst. nds: the shape
 * name for a shape load (slot 0 holds the .nds bytes of that name only), NULL otherwise (slot 1 only). */
int pm_prefetch_take(const void* src, void* dst, unsigned dec, const char* nds) {
    if (!pf_th || pf_th == (Thread)1) return 0;
    for (int i = 0; i < PF_N; i++) {
        PfSlot* s = &pf[i];
        if ((i == 0) != (nds != NULL) || (nds && strcmp(s->name, nds) != 0)) continue; /* MAPROMFS: slot 0 holds .nds bytes, only a shape takes it */
        if (s->st == 0 || s->dec != dec || memcmp(src, s->src, s->clen) != 0) continue;
        pf_wait();
        int ok = s->st == 2;
        if (ok) memcpy(dst, pf_buf[i], dec);
        pf_release(i);
        return ok;
    }
    return 0;
}
