#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pm_port.h"
#include "pm_thread.h"

/* HW2: the game thread never touches the SD card for a save. pm_flash_put and pm_flash_fill change flash_mem under flash_lock and
 * mark it dirty. A thread on core 1 waits until no call came for 70 ms (the game writes all pages of a save in one burst), then
 * writes the whole 128 KB image: a copy under flash_lock (one malloc, made before the lock), written to save.flash.tmp, fclose
 * checked, then renamed over save.flash (SAVESAFE: a cut write leaves the old file whole). No copy (malloc failed): the image is
 * written from flash_mem with flash_lock held, so the game waits for one write at most. pm_flash_flush is the join: it writes
 * what is still dirty on the calling thread. A failed write keeps the image dirty, shows pm_flash_warn on the footer and is
 * retried every 2 s. pm_flash_crash_flush (crash and watchdog exits) writes without malloc under trylock. Callers of the join:
 * HOME, sleep and exit hooks (pm_log.c cpu_hook), normal exit (pm_main.c), return to title (pm_warp.c). A write in flight holds
 * flash_wr_lock, so the join waits for it. The RAM copy is always complete. Write order is the call order (one thread, one lock). */
static LightLock flash_lock, flash_wr_lock;
static u8* flash_mem;
static u32 flash_size;
static volatile int flash_dirty;
static volatile u32 flash_stamp;
static Thread flash_th;
static LightEvent flash_ev;
static int flash_lock_ready, flash_th_state; /* 0 none, 1 running, -1 failed */
volatile int pm_flash_warn; /* SAVESAFE: 1 while the last image write failed (footer message, pm_gpu_footer) */

/* Write the whole image: save.flash.tmp first, then the rename. A rename over an old file that the SD refuses is done as remove
 * then rename. A complete tmp file that is left (rename failed) is newer than save.flash: flash_init loads it. */
/* Test switch saveerr: the first two image writes fail (retry path, footer message). */
static int flash_test_fail(void) {
    static int left = -1;
    if (left < 0) left = pm_debug_has("saveerr") ? 2 : 0;
    if (left > 0) {
        left--;
        return 1;
    }
    return 0;
}

static int flash_file_put(const u8* img, u32 n) {
    if (flash_test_fail()) return 0;
    FILE* f = fopen("save.flash.tmp", "wb");
    if (!f) return 0;
    int ok = fwrite(img, 1, n, f) == n;
    if (fclose(f) != 0) ok = 0;
    if (!ok) {
        remove("save.flash.tmp");
        return 0;
    }
    if (rename("save.flash.tmp", "save.flash") != 0) {
        remove("save.flash");
        if (rename("save.flash.tmp", "save.flash") != 0) return 0;
    }
    return 1;
}

static void flash_fail(void) {
    LightLock_Lock(&flash_lock);
    flash_dirty = 1; /* the RAM image is still the newest save: write it again */
    pm_flash_warn = 1;
    LightLock_Unlock(&flash_lock);
    if (flash_th_state == 1) LightEvent_Signal(&flash_ev); /* the flush thread retries after 2 s */
}

/* Returns -1 when nothing was dirty, 1 when the image is on the SD card, 0 on a failed write (the image stays dirty). */
static int flash_write_dirty(const char* why) {
    if (!flash_dirty) return -1;
    LightLock_Lock(&flash_wr_lock);
    u8* copy = (u8*)malloc(flash_size); /* before flash_lock: the game does not wait for an allocation */
    LightLock_Lock(&flash_lock);
    if (!flash_dirty) {
        LightLock_Unlock(&flash_lock);
        free(copy);
        LightLock_Unlock(&flash_wr_lock);
        return -1;
    }
    flash_dirty = 0;
    if (copy) {
        memcpy(copy, flash_mem, flash_size);
        LightLock_Unlock(&flash_lock);
    }
    __atomic_add_fetch(&pm_wd_hold, 1, __ATOMIC_RELAXED); /* LB4: the watchdog and the crash exit wait for a running save */
    u64 t0 = svcGetSystemTick();
    int ok = copy ? flash_file_put(copy, flash_size) : flash_file_put(flash_mem, flash_size); /* no copy: flash_lock is still held */
    if (!copy) LightLock_Unlock(&flash_lock);
    if (ok) {
        pm_flash_warn = 0;
        pm_log("save.flash written (atomic) %u ms (%s)", (unsigned)((svcGetSystemTick() - t0) / CPU_TICKS_PER_MSEC), why);
    } else {
        pm_log_e("save.flash write FAILED (SD full, missing or read-only?), save kept in RAM, retry in 2 s (%s)", why);
        flash_fail();
    }
    free(copy);
    __atomic_sub_fetch(&pm_wd_hold, 1, __ATOMIC_RELAXED);
    LightLock_Unlock(&flash_wr_lock);
    return ok;
}

extern volatile int pm_io_quiet; /* PT68: pm_log.c, set while the app is suspended or asleep */
static void flash_thread(void* arg) {
    (void)arg;
    for (;;) {
        LightEvent_Wait(&flash_ev);
        for (;;) {
            u32 s = flash_stamp;
            svcSleepThread(70000000LL);
            if (s == flash_stamp) break;
        }
        while (pm_io_quiet) svcSleepThread(50000000LL); /* PT68: no SD write while asleep, the span stays dirty */
        if (flash_write_dirty("core 1 idle") == 0) svcSleepThread(2000000000LL); /* SAVESAFE: the failure signalled the event; retry after 2 s */
    }
}

/* Join: everything marked dirty is on the SD card when this returns. Safe from any thread but the flash thread. */
void pm_flash_flush(const char* why) {
    if (!flash_lock_ready) return;
    flash_write_dirty(why);
}

/* Crash and watchdog exit (pm_exit_process): no malloc. Waits at most 1 s for the write lock and for flash_lock (a page write
 * in progress), then writes flash_mem. The lines go to crash.txt through pm_crash_line. */
static void flash_exit_note(const char* b) {
    pm_log("%s", b); /* the log queue; pm_crash_line drains it into the log file */
    pm_crash_line(b, (int)strlen(b));
}

void pm_flash_crash_flush(void) {
    char b[120];
    unsigned i;
    if (!flash_lock_ready || !flash_dirty) return;
    for (i = 0; i < 100 && LightLock_TryLock(&flash_wr_lock) != 0; i++) svcSleepThread(10000000LL);
    if (i == 100) {
        flash_exit_note("[CRASH] save.flash not written at exit: write lock held for 1 s\n");
        return;
    }
    for (i = 0; i < 100 && LightLock_TryLock(&flash_lock) != 0; i++) svcSleepThread(10000000LL);
    if (i == 100) {
        LightLock_Unlock(&flash_wr_lock);
        flash_exit_note("[CRASH] save.flash not written at exit: a save page write holds flash_lock for 1 s\n");
        return;
    }
    if (flash_dirty) {
        u64 t0 = svcGetSystemTick();
        int ok = flash_file_put(flash_mem, flash_size);
        if (ok) flash_dirty = 0;
        LightLock_Unlock(&flash_lock);
        LightLock_Unlock(&flash_wr_lock);
        if (ok) snprintf(b, sizeof b, "save.flash written at exit (atomic) %u ms\n", (unsigned)((svcGetSystemTick() - t0) / CPU_TICKS_PER_MSEC));
        else snprintf(b, sizeof b, "[CRASH] save.flash not written at exit (SD write failed)\n");
        flash_exit_note(b);
    } else {
        LightLock_Unlock(&flash_lock);
        LightLock_Unlock(&flash_wr_lock);
    }
}

static void flash_kick(void) {
    if (flash_th_state == 0) {
        LightEvent_Init(&flash_ev, RESET_ONESHOT);
        flash_th = pm_thread_create(flash_thread, NULL, 32 * 1024, 0x3F, 1, "flash");
        if (!flash_th) flash_th = pm_thread_create(flash_thread, NULL, 32 * 1024, 0x3F, -2, "flash");
        flash_th_state = flash_th ? 1 : -1;
        if (!flash_th) pm_log_w("save.flash: no flush thread, the save is written at the next join (HOME, sleep, exit)");
    }
    if (flash_th_state == 1) LightEvent_Signal(&flash_ev);
}

/* Marks the whole image dirty (new file, or a copy from the atomic path is due). */
void pm_flash_mark_all(void) {
    pm_audio_drain(2); /* AU2: a save drains the mixer queue */
    LightLock_Lock(&flash_lock);
    flash_dirty = 1;
    flash_stamp++;
    LightLock_Unlock(&flash_lock);
    flash_kick();
}

/* Test switch savecrash: the process ends through pm_exit_process (the crash and watchdog exit) 10 ms after the last save write, while
 * the save is still dirty (the flush thread waits 70 ms). Log: the RAM hash of the image, then the exit flush line. */
static void savecrash_thread(void* arg) {
    (void)arg;
    u32 s;
    u32 h = 0, k;
    do {
        s = flash_stamp;
        svcSleepThread(5000000LL);
    } while (s != flash_stamp);
    svcSleepThread(10000000LL);
    for (k = 0; k < flash_size; k++) h = h * 31u + flash_mem[k];
    pm_log("savecrash: exit 10 ms after the save, ram hash %08x", h);
    pm_exit_process();
}

static int savecrash_armed;

/* A page write from the osFlash calls: copy and mark in one lock, so the flush copy never sees a half page. */
void pm_flash_put(unsigned off, const void* src, unsigned len) {
    if (len == 0) return;
    pm_audio_drain(2); /* AU2: a save drains the mixer queue */
    LightLock_Lock(&flash_lock);
    memcpy(flash_mem + off, src, len);
    flash_dirty = 1;
    flash_stamp++;
    LightLock_Unlock(&flash_lock);
    if (!savecrash_armed && pm_debug_has("savecrash")) {
        savecrash_armed = 1;
        pm_thread_create(savecrash_thread, NULL, 16 * 1024, 0x3F, 1, "savecrash");
    }
    flash_kick();
}

/* A sector erase from the osFlash calls: same lock as pm_flash_put. */
void pm_flash_fill(unsigned off, unsigned len) {
    if (len == 0) return;
    pm_audio_drain(2); /* AU2: a save drains the mixer queue */
    LightLock_Lock(&flash_lock);
    memset(flash_mem + off, 0xFF, len);
    flash_dirty = 1;
    flash_stamp++;
    LightLock_Unlock(&flash_lock);
    flash_kick();
}

void pm_flash_attach(unsigned char* mem, unsigned size) {
    flash_mem = mem;
    flash_size = size;
    LightLock_Init(&flash_lock);
    LightLock_Init(&flash_wr_lock);
    flash_lock_ready = 1;
}
