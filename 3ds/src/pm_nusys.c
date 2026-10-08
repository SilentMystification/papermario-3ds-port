#include "nu/nusys.h"
#include "pm_port.h"
#include "pm_gpu.h"
#include "pm_gbi.h"

static NUGfxFunc gfx_func;
static int display_on;

void nuGfxInitEX2(void) {
    pm_gpu_init();
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

void nuContRmbForceStop(void) {}

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

void nuGfxTaskStart(Gfx* gfxList, u32 gfxListSize, u32 ucode, u32 flag) {
    (void)ucode;
    (void)flag;
    pm_gbi_run(gfxList, gfxListSize);
}

void pm_gfx_retrace(void) {
    if (display_on && gfx_func) gfx_func(0);
}
