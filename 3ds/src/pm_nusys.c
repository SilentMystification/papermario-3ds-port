#include "nu/nusys.h"
#include "pm_port.h"
#include "pm_gpu.h"
#include "pm_gbi.h"

static NUGfxFunc gfx_func;
static int display_on;

void nuGfxInitEX2(void) {
    pm_gpu_init();
    pm_log_console_ready(); /* CPULIM-UI: the bottom console exists now */
    pm_gbi_init();
    pm_audio_init();
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
/* PI1: walk one task and count it. kind 0 = background, 1 = main (swap), 2 = key replay. One tick pair per task, none per op. */
static void gbi_task(Gfx* gfxList, u32 n, int kind) {
    if (!pm_ph_on) { pm_gbi_run(gfxList, n); return; }
    unsigned o0 = pm_gbi_ops_now(), t0 = pm_ticks();
    pm_gbi_run(gfxList, n);
    pm_rs_cnt[RS_TK_BG + kind]++;
    pm_rs_cnt[RS_TUS_BG + kind] += (pm_ticks() - t0) / 268u;
    pm_rs_cnt[RS_TOPS] += pm_gbi_ops_now() - o0;
}

void nuGfxTaskStart(Gfx* gfxList, u32 gfxListSize, u32 ucode, u32 flag) {
    (void)ucode;
    int swap = (flag & NU_SC_SWAPBUFFER) != 0;
    if (swap) pm_audio_step(); /* AU1b: the game logic and the display list of this step are done: audio ticks here */
    PH_PUSH(PH_GBI);
    if (pm_interp_on > 0) {
        /* F06-2: the draw iteration shows the t = 0.5 pass of the swap task. pm_interp_key_draw replays every task of the
         * step without replacements in the idle iteration. The background task of the step comes before the swap task. */
        if (nkey < MAX_KEY) { key_list[nkey] = gfxList; key_size[nkey++] = gfxListSize; }
        if (swap && pm_interp_skip) { pm_interp_skip_count(); nkey = 0; } /* F06-5: behind, draw the key frame now */
        else if (swap) { pm_interp_lerp(0.5f); pm_interp_repl = 1; }
        gbi_task(gfxList, gfxListSize, swap ? 1 : 0);
        pm_interp_repl = 0;
    } else {
        gbi_task(gfxList, gfxListSize, swap ? 1 : 0);
    }
    PH_POP();
    if (swap) pm_interp_stop(); /* the background task has no swap flag: the record goes on until the main task */
}

void pm_interp_drop_keys(void) { nkey = 0; } /* F11-10 */

/* F06-2: the key frame of the last step, drawn without replacements. Returns 0 when the step built no list. */
int pm_interp_key_draw(void) {
    if (!nkey) return 0;
    pm_gpu_begin(1);
    PH_PUSH(PH_GBI);
    for (int i = 0; i < nkey; i++) gbi_task(key_list[i], key_size[i], 2);
    PH_POP();
    nkey = 0;
    pm_gpu_end();
    return 1;
}

/* FS1: the same call with gfxTaskNum 3. gfxRetrace_Callback (src/main.c) then runs step_game_loop and skips gfx_task_background and gfx_draw_frame,
 * as it does on the N64 when 3 graphics tasks are pending. It sets D_80073E08 for the game. */
void pm_gfx_retrace_skip(void) {
    pm_audio_join_at(1);
    if (display_on && gfx_func) gfx_func(3);
}

void pm_gfx_retrace(void) {
    pm_audio_join_at(1); /* AU1d: the game function below runs game code */
    if (display_on && gfx_func) gfx_func(0);
}
