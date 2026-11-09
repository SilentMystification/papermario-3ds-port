#include <string.h>
#include "nu/nusys.h"
#include "pm_port.h"
#include "pm_gpu.h"
#include "nd.h"

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
static Gfx* key_list[MAX_KEY];
static u32 key_size[MAX_KEY];
static int nkey;
uint64_t svcGetSystemTick(void);
void pm_pf_nd_add(unsigned long long ticks, unsigned legacy); /* pm_phase.c: S0-6 window sums (legacy 1 = one walk) */

/* S10-2: one walk of the task's words (pm_gbi_run). The phase line keeps its format: the walk time goes to pm_pf_nd_add as one legacy span. */
static void gbi_walk(Gfx* gfxList, u32 n) {
    uint64_t t0 = pm_ph_on ? svcGetSystemTick() : 0;
    pm_gbi_run(gfxList, n);
    if (pm_ph_on) pm_pf_nd_add(svcGetSystemTick() - t0, 1);
}

/* PI1: walk one task (walker side). */
static void gbi_task_w(Gfx* gfxList, u32 n) {
    if (pm_fs_on) pm_fs_tasks++; /* WMODEL: render tasks per frame, fsplit only */
    pm_gpu_begin_wait(); /* LB1: every gbi task (also the key replay of pm_interp_key_draw) waits for the GPU tail of the last frame */
    FS_MARK(FS_GBI); /* LB1b: the walk after the wait is gbi work, not rest (fsplit label only) */
    nd_frame_begin();
    gbi_walk(gfxList, n);
    if (pm_gpu_stereo()) { pm_gpu_eye(1); gbi_walk(gfxList, n); pm_gpu_eye(0); } /* STEREO: the same task for the right eye */
}

/* CORE2C-2: the walker side of one queued frame (nd_wq.c): the background task, then the main task. */
void pm_frame_walk(const PmFrame* f) {
    pm_walk_cfb = f->cfb;
    if (f->bg.ptr) gbi_task_w((Gfx*)f->bg.ptr, f->bg.n);
    if (f->main.ptr) {
        pm_interp_repl = (f->flags & PMF_REPL) != 0;
        gbi_task_w((Gfx*)f->main.ptr, f->main.n);
        pm_interp_repl = 0;
    }
}

/* The frame the game builds. LB1b: the background task (no swap flag) is recorded and goes with the swap task, after the main list is
 * built, so the main list build overlaps the GPU tail wait of the last frame. pm_gfx_bg_flush sends a background task that no swap
 * took (a reset tile task, or a step with no draw) at the end of the retrace call. */
static PmFrame fr;
static void fr_push(void) {
    fr.cfb = nuGfxCfb_ptr;
    pm_wq_frame(&fr);
    memset(&fr, 0, sizeof fr);
}

void nuGfxTaskStart(Gfx* gfxList, u32 gfxListSize, u32 ucode, u32 flag) {
    (void)ucode;
    int swap = (flag & NU_SC_SWAPBUFFER) != 0;
    if (swap) { FS_MARK(FS_AUDIO); pm_audio_step(); } /* AU1b: the game logic and the display list of this step are done: audio ticks here */
    FS_MARK(FS_GBI);
    PH_PUSH(PH_GBI);
    /* F06-2: the draw iteration shows the t = 0.5 pass of the swap task. pm_interp_key_draw replays every task of the step without
     * replacements in the idle iteration. The background task of the step comes before the swap task. */
    if (pm_interp_on > 0 && nkey < MAX_KEY) { key_list[nkey] = gfxList; key_size[nkey++] = gfxListSize; }
    if (!swap) {
        if (fr.bg.ptr) fr_push(); /* a second background task: the first goes alone */
        fr.bg.ptr = gfxList; fr.bg.n = gfxListSize;
    } else {
        fr.main.ptr = gfxList; fr.main.n = gfxListSize;
        if (pm_interp_on > 0) {
            if (pm_interp_skip) { pm_interp_skip_count(); nkey = 0; } /* F06-5: behind, draw the key frame now */
            else { pm_interp_lerp(0.5f); fr.flags |= PMF_REPL; }
        }
        fr_push();
    }
    PH_POP();
    FS_MARK(swap ? FS_REST : FS_DL);
    if (swap) pm_interp_stop(); /* the background task has no swap flag: the record goes on until the main task */
}

void pm_gfx_bg_flush(void) { if (fr.bg.ptr) fr_push(); }

void pm_interp_drop_keys(void) { nkey = 0; } /* F11-10 */

/* F06-2: the key frame of the last step, drawn without replacements. Returns 0 when the step built no list. */
static void wq_key_draw(void* a) { /* CORE2-3a: the key frame (begin, tasks, end) is one walker-side message */
    (void)a;
    pm_fs_q = 1; /* FS2: a key frame is not a drawn frame: no fsplit mark */
    pm_gpu_begin(1);
    PH_PUSH(PH_GBI);
    for (int i = 0; i < nkey; i++) gbi_task_w(key_list[i], key_size[i]);
    PH_POP();
    nkey = 0;
    pm_gpu_end();
    pm_fs_q = 0;
}
int pm_interp_key_draw(void) {
    if (!nkey) return 0;
    pm_wq_post(wq_key_draw, NULL, 0);
    return 1;
}

/* FS1: the same call with gfxTaskNum 3. gfxRetrace_Callback (src/main.c) then runs step_game_loop and skips gfx_task_background and gfx_draw_frame,
 * as it does on the N64 when 3 graphics tasks are pending. It sets D_80073E08 for the game. */
void pm_gfx_retrace_skip(void) {
    if (display_on && gfx_func) gfx_func(3);
    pm_gfx_bg_flush();
}

void pm_gfx_retrace(void) {
    if (display_on && gfx_func) gfx_func(0);
    pm_gfx_bg_flush();
}
