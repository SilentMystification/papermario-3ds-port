#include <string.h>
#include "nu/nusys.h"
#include "pm_port.h"
#include "pm_gpu.h"
#include "nd.h"
#include "nd_cmd.h"

static NUGfxFunc gfx_func;

/* CORE2-3: viewport copies for the list (gbi_custom.h PM_VP). One pool per display context; a pool is reused when its context is built
 * again, two frames later, after the walk of the frame that used it. Full pool: the original pointer (one warning). */
#define PM_VP_POOL 32
extern s32 gCurrentDisplayContextIndex;
Vp* pm_vp_snap(const Vp* v) {
    static Vp pool[2][PM_VP_POOL];
    static unsigned n, last_f = ~0u;
    static int warned;
    unsigned f = pm_frame_count();
    if (f != last_f) { last_f = f; n = 0; }
    if (n >= PM_VP_POOL) { if (!warned) { warned = 1; pm_log_w("vp pool full (%u viewports in one frame): original pointer", n); } return (Vp*)v; }
    Vp* c = &pool[gCurrentDisplayContextIndex & 1][n++];
    *c = *v;
    return c;
}
static int display_on;

void nuGfxInitEX2(void) {
    pm_gpu_init();
    pm_boot_mark(PMB_GFX, 0); /* BOOT-1 */
    pm_log_console_ready(); /* CPULIM-UI: the bottom console exists now */
    pm_gbi_init();
    pm_audio_init();
    pm_boot_mark(PMB_AUDIO, 0); /* BOOT-1 */
}

void nuGfxDisplayOff(void) { display_on = 0; }
void nuGfxDisplayOn(void) { display_on = 1; }
void nuGfxFuncSet(NUGfxFunc func) { gfx_func = func; }
void nuGfxPreNMIFuncSet(NUGfxPreNMIFunc func) { (void)func; }
void nuGfxTaskAllEndWait(void) {}

void nuGfxSetCfb(u16** framebuf, u32 framebufnum) {
    (void)framebuf;
    (void)framebufnum;
}

/* N64 serial interface message queue. The 3DS has no SI and no rumble pak, so the rumble messages go nowhere. */
s32 nuSiSendMesg(NUScMsg mesg, void* dataPtr) { return 0; }
/* nusys BSS array, placed by symbol_addrs on the N64. nuContRmbMgrInit is not built, so it stays zeroed (state, mode). */
NUContRmbCtl nuContRmbCtl[NU_CONT_MAXCONTROLLERS];

u8 nuContInit(void) { return 1; }

void nuContDataGet(OSContPad* data, u32 padno) {
    (void)padno;
    if (!data) return;
    unsigned short b = 0, t = 0;
    signed char x = 0, y = 0;
    pm_pad_read(&b, &x, &y, &t);
    data->button = b;
    data->stick_x = x;
    data->stick_y = y;
    (void)t;
}

void nuContDataGetEx(NUContData* data, u32 padno) {
    (void)padno;
    if (!data) return;
    unsigned short b = 0, t = 0;
    signed char x = 0, y = 0;
    pm_pad_read(&b, &x, &y, &t);
    data->button = b;
    data->stick_x = x;
    data->stick_y = y;
    data->trigger = t;
    data->errno = 0;
}

void nuContDataGetExAll(NUContData* data) {
    if (!data) return;
    nuContDataGetEx(&data[0], 0);
    for (int i = 1; i < 4; i++) {
        data[i].button = 0;
        data[i].stick_x = 0;
        data[i].stick_y = 0;
        data[i].trigger = 0;
        data[i].errno = 0;
    }
}

#define MAX_KEY 4
static const uint32_t* key_list[MAX_KEY]; /* DIRECT: the task streams of the step */
static u32 key_size[MAX_KEY]; /* their list bytes (log only) */
static int nkey;
uint64_t svcGetSystemTick(void);
void pm_pf_nd_add(unsigned long long ticks, unsigned legacy); /* pm_phase.c: S0-6 window sums (legacy 1 = one walk) */

/* INTERP1: walk us of the last t = 0.5 pass [0], key frame [1], plain frame [2] */
unsigned pm_iw_us[3];
static unsigned long long iw_tk; /* INTERP1: ticks inside nd_stream_run of the pass being timed */

/* S10-2, DIRECT D0: one replay of the task stream (nd_rec.c). The phase line keeps its format: the walk time goes to pm_pf_nd_add as one legacy span. */
static void gbi_walk(const uint32_t* s) {
    uint64_t t0 = pm_ph_on ? svcGetSystemTick() : 0;
    const uint64_t f0 = pm_cnt_on ? svcGetSystemTick() : 0; /* FXSPLIT */
    const uint64_t w0 = svcGetSystemTick(); /* INTERP1 */
    nd_stream_run(s);
    iw_tk += svcGetSystemTick() - w0; /* INTERP1 */
    if (pm_ph_on) pm_pf_nd_add(svcGetSystemTick() - t0, 1);
    if (pm_cnt_on) pm_fx_walk_us += (unsigned)((svcGetSystemTick() - f0) / 268u); /* FXSPLIT */
}

/* PI1: walk one task (walker side). */
static void gbi_task_w(const uint32_t* s) {
    if (pm_fs_on) pm_fs_tasks++; /* WMODEL: render tasks per frame, fsplit only */
    pm_gpu_begin_wait(); /* LB1: every gbi task (also the key replay) waits for the GPU tail of the last frame */
    FS_MARK(FS_GBI); /* LB1b: the walk after the wait is gbi work, not rest (fsplit label only) */
    nd_frame_begin();
    gbi_walk(s); /* STEREO: one walk; the right eye replays the GPU entries at the frame end (pm_gpu.c sr_replay) */
}

/* CORE2C-2: the walker side of one queued frame (nd_wq.c): the background task, then the main task. */
extern int pm_wide; /* pm_gpu.c, game side */
/* INTROSNAP debug: the first 8 walked frames whose build-time wide (PMF_WIDE) differs from the live game value. The walker reads the
 * live pm_wide; __wrap_set_game_mode drains the walk queue before pm_wide changes, so the count should stay 0. Cold: runs only on a mismatch. */
static __attribute__((cold, noinline)) void wide_walk_note(int built) {
    static unsigned n;
    if (n >= 8) return;
    n++;
    pm_log("wide: walk f%u built %d live %d", pm_frame_count(), built, pm_wide);
}
/* INTERP2M: the kind of the frame being built, read at its frame end (pm_gpu.c icmp_frame): 0 plain, 1 L (t = 0.5), 2 K (key). Not cleared
 * in the walk: the frame end of a step is a later walker message than its walk (pm_main.c wq_step_end). */
int pm_icmp_kind;
int pm_tw_on; /* INTERPB1: 1 during the whole walk of an L frame (background and main task): twin matrices are made (nd_mtx_set.c) */
void nd_twin_frame_begin(int kind);
void pm_frame_walk(const PmFrame* f) {
    pm_icmp_kind = (f->flags & PMF_REPL) ? 1 : 0; /* INTERP2M */
    pm_tw_on = pm_icmp_kind; /* INTERPB1 */
    nd_twin_frame_begin(pm_icmp_kind);
    iw_tk = 0; /* INTERP1 */
    if (__builtin_expect(((f->flags & PMF_WIDE) != 0) != (pm_wide != 0), 0)) wide_walk_note((f->flags & PMF_WIDE) != 0);
    pm_walk_cfb = f->cfb;
    if (f->bg.ptr) gbi_task_w((const uint32_t*)f->bg.ptr);
    if (f->main.ptr) {
        pm_interp_repl = (f->flags & PMF_REPL) != 0;
        gbi_task_w((const uint32_t*)f->main.ptr);
        pm_interp_repl = 0;
    }
    pm_iw_us[(f->flags & PMF_REPL) ? 0 : 2] = (unsigned)(iw_tk / 268u); /* INTERP1 */
    pm_tw_on = 0; nd_kp_on = 0; /* INTERPB2: the patches stay in the list until the frame end */
}

/* The frame the game builds. LB1b: the background task (no swap flag) is recorded and goes with the swap task, after the main list is
 * built, so the main list build overlaps the GPU tail wait of the last frame. pm_gfx_bg_flush sends a background task that no swap
 * took (a reset tile task, or a step with no draw) at the end of the retrace call. */
static PmFrame fr;
static void fr_push(void) {
    fr.cfb = nuGfxCfb_ptr;
    if (pm_wide) fr.flags |= PMF_WIDE; /* INTROSNAP debug: the wide state the tasks were built with (wide_walk_note) */
    pm_wq_frame(&fr);
    memset(&fr, 0, sizeof fr);
}

void nuGfxTaskStart(Gfx* gfxList, u32 gfxListSize, u32 ucode, u32 flag) {
    (void)ucode;
    int swap = (flag & NU_SC_SWAPBUFFER) != 0;
    if (swap) { FS_MARK(FS_AUDIO); pm_audio_step(); } /* AU1b: the game logic and the display list of this step are done: audio ticks here */
    FS_MARK(FS_GBI);
    PH_PUSH(PH_DRAW);
    /* F06-2: the draw iteration shows the t = 0.5 pass of the swap task. the key frame replays every task of the step without
     * replacements in the idle iteration. The background task of the step comes before the swap task. */
    (void)gfxList; /* DIRECT D9: the task is its stream (the game writes no task list) */
    const uint32_t* st = nd_rec_close();
    if (pm_interp_on > 0 && nkey < MAX_KEY) { key_list[nkey] = st; key_size[nkey++] = gfxListSize; }
    if (!swap) {
        if (fr.bg.ptr) fr_push(); /* a second background task: the first goes alone */
        fr.bg.ptr = (void*)st; fr.bg.n = gfxListSize;
    } else {
        fr.main.ptr = (void*)st; fr.main.n = gfxListSize;
        if (pm_interp_on > 0) {
            if (pm_interp_skip) { pm_interp_skip_count(); nkey = 0; } /* F06-5: behind, draw the key frame now */
            else { pm_wq_frame_slot(); pm_interp_lerp(0.5f); fr.flags |= PMF_REPL; } /* the walker reads rtab: the frames before are walked first */
        }
        fr_push();
    }
    PH_POP();
    FS_MARK(swap ? FS_REST : FS_DL);
    if (swap) pm_interp_stop(); /* the background task has no swap flag: the record goes on until the main task */
}

void pm_gfx_bg_flush(void) { if (fr.bg.ptr) fr_push(); }

void pm_lg_key_walked(void); /* pm_logic.c */
void pm_interp_drop_keys(void) { nkey = 0; } /* F11-10 */

/* F06-2: the key frame of a step, drawn without replacements. The logic thread takes the task list at the step end (the list is its own),
 * the walker walks the copy: one walker-side message (begin, tasks, end). Returns 0 when the step built no list. */
int pm_interp_key_take(PmKey* k) {
    int n = nkey;
    for (int i = 0; i < MAX_KEY; i++) {
        k->t[i].ptr = i < n ? (void*)key_list[i] : NULL;
        k->t[i].n = i < n ? key_size[i] : 0u;
    }
    nkey = 0;
    return n != 0;
}
void pm_key_walk(const PmKey* k) { /* walker side */
    pm_icmp_kind = 2; /* INTERP2M */
    nd_twin_frame_begin(2); /* INTERPB1 */
    pm_fs_q = 1; /* FS2: a key frame is not a drawn frame: no fsplit mark */
    pm_gpu_begin(1);
    PH_PUSH(PH_DRAW);
    iw_tk = 0; /* INTERP1 */
    for (int i = 0; i < MAX_KEY && k->t[i].ptr; i++) gbi_task_w((const uint32_t*)k->t[i].ptr);
    pm_iw_us[1] = (unsigned)(iw_tk / 268u); /* INTERP1 */
    PH_POP();
    pm_gpu_end();
    pm_fs_q = 0;
    pm_lg_key_walked();
}

/* FS1: the same call with gfxTaskNum 3. gfxRetrace_Callback (src/main.c) then runs step_game_loop and skips gfx_task_background and gfx_draw_frame,
 * as it does on the N64 when 3 graphics tasks are pending. It sets D_80073E08 for the game. */
extern s16 D_80073E0A;
/* CORE2C-3: the frame loop decides the next pass from the retrace flag. gfxRetrace_Callback flips it first (src/main.c; the reset branch
 * never runs on the 3DS: nuGfxPreNMIFuncSet keeps no callback), so the value after this call is known before the step runs. */
static void retrace_call(s32 n) {
    int on = display_on && gfx_func;
    pm_lg_publish_d(on ? !D_80073E0A : D_80073E0A != 0);
    if (on) gfx_func(n);
    pm_gfx_bg_flush();
}
void pm_gfx_retrace_skip(void) { retrace_call(3); }

void pm_gfx_retrace(void) { retrace_call(0); }
