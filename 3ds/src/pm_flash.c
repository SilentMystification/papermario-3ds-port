#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pm_port.h"

/* HW2: the game thread never touches the SD card for a save. flash_sync marks a dirty span of flash_mem. A thread on core 1
 * waits until no call came for 70 ms (the game writes all pages of a save in one burst), copies the span under flash_lock and
 * writes it with one fopen, one fwrite and one fclose. pm_flash_flush is the join: it writes what is still dirty on the
 * calling thread. Callers: HOME, sleep and exit hooks (pm_log.c cpu_hook), normal exit (pm_main.c). A write in flight holds
 * flash_wr_lock, so the join waits for it. Kept from the old code: the RAM copy is always complete, a write error is logged
 * and the save stays in RAM, fclose is checked (full SD card). Write order is the call order (one thread, one lock). */
static LightLock flash_lock, flash_wr_lock;
static u8* flash_mem;
static u32 flash_size;
static u32 flash_dirty_lo = 0xFFFFFFFFu, flash_dirty_hi;
static volatile u32 flash_stamp;
static Thread flash_th;
static LightEvent flash_ev;
static int flash_lock_ready, flash_th_state; /* 0 none, 1 running, -1 failed */

static void flash_write_dirty(const char* why) {
    static unsigned fails;
    LightLock_Lock(&flash_wr_lock);
    LightLock_Lock(&flash_lock);
    u32 lo = flash_dirty_lo, hi = flash_dirty_hi;
    u8* copy = NULL;
    if (lo < hi) {
        copy = (u8*)malloc(hi - lo);
        if (copy) memcpy(copy, flash_mem + lo, hi - lo);
        flash_dirty_lo = 0xFFFFFFFFu;
        flash_dirty_hi = 0;
    }
    LightLock_Unlock(&flash_lock);
    if (lo < hi) {
        u64 t0 = svcGetSystemTick();
        const u8* src = copy ? copy : flash_mem + lo; /* no memory for a copy: write from the RAM chip (a later call can only mark it dirty again) */
        FILE* f = fopen("save.flash", "r+b");
        if (!f) f = fopen("save.flash", "w+b");
        int ok = f != NULL && fseek(f, (long)lo, SEEK_SET) == 0 && fwrite(src, 1, hi - lo, f) == hi - lo;
        if (f && fclose(f) != 0) ok = 0;
        if (!ok && fails++ < 5) {
            pm_log_e("save.flash write FAILED (off %u len %u, SD full or missing?), save kept in RAM only", lo, hi - lo);
        } else if (ok) {
            pm_log("save.flash written: off %u len %u in %u ms (%s)", lo, hi - lo,
                   (unsigned)((svcGetSystemTick() - t0) / CPU_TICKS_PER_MSEC), why);
        }
        free(copy);
    }
    LightLock_Unlock(&flash_wr_lock);
}

static void flash_thread(void* arg) {
    (void)arg;
    for (;;) {
        LightEvent_Wait(&flash_ev);
        for (;;) {
            u32 s = flash_stamp;
            svcSleepThread(70000000LL);
            if (s == flash_stamp) break;
        }
        flash_write_dirty("core 1 idle");
    }
}

/* Join: everything marked dirty is on the SD card when this returns. Safe from any thread but the flash thread. */
void pm_flash_flush(const char* why) {
    if (!flash_lock_ready) return;
    flash_write_dirty(why);
}

void pm_flash_mark(unsigned off, unsigned len) {
    if (len == 0) return;
    pm_audio_drain(2); /* AU2: a save drains the mixer queue */
    LightLock_Lock(&flash_lock);
    if (off < flash_dirty_lo) flash_dirty_lo = off;
    if (off + len > flash_dirty_hi) flash_dirty_hi = off + len;
    flash_stamp++;
    LightLock_Unlock(&flash_lock);
    if (flash_th_state == 0) {
        LightEvent_Init(&flash_ev, RESET_ONESHOT);
        flash_th = threadCreate(flash_thread, NULL, 32 * 1024, 0x3F, 1, false);
        if (!flash_th) flash_th = threadCreate(flash_thread, NULL, 32 * 1024, 0x3F, -2, false);
        flash_th_state = flash_th ? 1 : -1;
        if (!flash_th) pm_log_w("save.flash: no flush thread, the game thread writes");
    }
    if (flash_th_state == 1) LightEvent_Signal(&flash_ev);
    else flash_write_dirty("no thread");
}


void pm_flash_attach(unsigned char* mem, unsigned size) {
    flash_mem = mem;
    flash_size = size;
    LightLock_Init(&flash_lock);
    LightLock_Init(&flash_wr_lock);
    flash_lock_ready = 1;
}
