#include "pm_build.h"
#ifndef PM_GIT_HASH
#define PM_GIT_HASH "unknown"
#endif
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "pm_port.h"
#include "pm_thread.h"
#include "pm_crash_page.h"
unsigned pm_log_late_con(void); /* pm_log.c: B1 check */
unsigned pm_log_con_copies(void), pm_log_con_nomenu(void), pm_log_late_drop(void);
/* O6: a switch word read once. Not for the words the options panel changes at run time (nofog, nolight). */
#include "pm_gpu.h"
#include "pm_dspaud.h"
#include "pm_romload.h"
#include "nd.h"


extern void boot_main(void* data);
const char* pm_wd_phase_name(unsigned p);

static int crash_taken; /* CRASHALL: the first crash handler ends the process; another thread that faults waits here */
volatile int pm_crash_exit; /* CRASHEXIT: a crash ended its thread; main exits clean */
extern volatile int pm_exit_request;
static char args[256];

/* Stack records: bounds of the game and audio thread stacks, a fill pattern for the high-water mark.
 * A thread registers itself with the address of a local at its entry. The fill stops below the live frames. */
#define STACK_PAT 0xA5A5A5A5u
typedef struct { u32 lo, hi; const char* name; } StackRec;
static StackRec recs[4]; /* game, audio, render, logic */

void pm_stack_register(int id, const char* name, unsigned size) {
    volatile u32 here;
    u32 top = (u32)&here;
    StackRec* r = &recs[id];
    r->name = name;
    r->hi = top;
    r->lo = (top - size + 512) & ~3u; /* 512 bytes of slack: the real top is a little above this frame */
    for (volatile u32* p = (volatile u32*)r->lo; (u32)p < top - 256; p++) *p = STACK_PAT;
}

unsigned pm_stack_used(int id, unsigned* size) {
    const StackRec* r = &recs[id];
    if (!r->hi) return 0;
    const u32* p = (const u32*)r->lo;
    while ((u32)p < r->hi && *p == STACK_PAT) p++;
    if (size) *size = r->hi - r->lo;
    return r->hi - (u32)p;
}

static unsigned fpscr_get(void) {
    unsigned v;
    __asm__ volatile("vmrs %0, fpscr" : "=r"(v));
    return v;
}

/* N64 libultra runs the game with FPCSR flush-to-zero set (FS), round to nearest, no FPU traps.
 * So: FZ (bit 24) on, DN (bit 25) on (a NaN result is one fixed quiet NaN, nothing depends on payloads),
 * RMode (bits 23:22) = 0 nearest, trap enables (bits 12:8, bit 15) off, sticky flags cleared. */
void pm_fpscr_setup(const char* who) {
    unsigned before = fpscr_get();
    unsigned v = (before | (1u << 24) | (1u << 25)) & ~((3u << 22) | (0x1Fu << 8) | (1u << 15) | 0x9Fu);
    __asm__ volatile("vmsr fpscr, %0" : : "r"(v));
    pm_log_d("fpscr %s before %08x after %08x", who, before, fpscr_get());
}

struct CrashPx { u8* fb; u16 pw; int x0; };
static void crash_px_set(void* c, int x, int y) {
    const struct CrashPx* p = (const struct CrashPx*)c;
    u8* d = p->fb + ((size_t)(x + p->x0) * p->pw + (p->pw - 1 - y)) * 3; /* BGR, column-major: the layout pm_gpu_shot_top_displayed_w reads */
    d[0] = d[1] = d[2] = 0xFF;
}

/* The page is drawn into the top screen's back buffer (no page buffer): clear it, the lit pixels by the CPU, flush, swap. */
static void crash_page_draw(const char* const* lines, int n) {
    u16 pw = 0, lw = 0; /* pw: physical width (one column of pixels), lw: the logical width of the top screen */
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &pw, &lw);
    if (!fb) return;
    memset(fb, 0, (size_t)pw * lw * 3);
    struct CrashPx c = { fb, pw, ((int)lw - PM_CRASH_PAGE_W) / 2 };
    pm_crash_page_render(crash_px_set, &c, lines, n);
    GSPGPU_FlushDataCache(fb, (u32)pw * lw * 3);
    gfxScreenSwapBuffers(GFX_TOP, false);
}

/* A or START ends the wait, or 60 s. The crash tick is refreshed here: the watchdog's 3 s limit is for a handler that hangs. */
static void crash_page_wait(void) {
    u64 t0 = svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000ull);
    for (;;) {
        hidScanInput();
        if (hidKeysDown() & (KEY_A | KEY_START)) return;
        u64 now = svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000ull);
        if (now - t0 >= 60000ull) return;
        pm_wd_crash_tick = (unsigned)now | 1u;
        svcSleepThread(16000000LL);
    }
}

/* CRASHALL part 2: the ARM crash page (pm_crash_page.c renders it). CPU writes only, no lock. */
static void crash_page_show(const char* type, const char* tname, const ERRF_ExceptionInfo* excep, const CpuRegisters* regs, const StackRec* st) {
    static char pg[PM_CRASH_PAGE_LINES][56];
    const char* lines[PM_CRASH_PAGE_LINES];
    int n = 0, nw = 0;
    u32 words[10];
    snprintf(pg[n++], 56, "ARM CRASH: %s", type);
    snprintf(pg[n++], 56, "THREAD %s", tname);
    snprintf(pg[n++], 56, "PC %08lX LR %08lX SP %08lX", (unsigned long)regs->pc, (unsigned long)regs->lr, (unsigned long)regs->sp);
    snprintf(pg[n++], 56, "FAR %08lX FSR %08lX CPSR %08lX", (unsigned long)excep->far, (unsigned long)excep->fsr, (unsigned long)regs->cpsr);
    for (int i = 0; i < 13; i += 4) {
        char* b = pg[n++];
        int k = 0;
        for (int j = i; j < i + 4 && j < 13; j++) k += snprintf(b + k, 56 - (size_t)k, "R%d %08lX ", j, (unsigned long)regs->r[j]);
    }
    if (st && regs->sp >= st->lo && regs->sp < st->hi) {
        const u32* sp = (const u32*)(regs->sp & ~3u);
        const u32* end = (const u32*)st->hi;
        for (int i = 0; sp + i < end && i < 2048 && nw < 10; i++) {
            u32 v = sp[i];
            if (v >= 0x00100000u && v < 0x01000000u) words[nw++] = v;
        }
    }
    snprintf(pg[n++], 56, "%s", nw ? "CODE-LIKE STACK WORDS" : "NO CODE-LIKE STACK WORDS");
    for (int i = 0; i < nw; i += 5) {
        char* b = pg[n++];
        int k = 0;
        for (int j = i; j < nw && j < i + 5; j++) k += snprintf(b + k, 56 - (size_t)k, "%08lX ", (unsigned long)words[j]);
    }
    snprintf(pg[n++], 56, "BUILD #%s", PM_BUILD_STR);
    snprintf(pg[n++], 56, "LOOP PHASE %u (%s) FRAME %u", pm_wd_phase, pm_wd_phase_name(pm_wd_phase), pm_frame_count());
    snprintf(pg[n++], 56, "A OR START: EXIT (60 S)");
    for (int i = 0; i < n; i++) lines[i] = pg[i];
    crash_page_draw(lines, n);
    crash_page_wait();
}

static void crash_handler(ERRF_ExceptionInfo* excep, CpuRegisters* regs) {
    static const char* names[] = { "prefetch abort", "data abort", "undefined instruction", "VFP" };
    char buf[200];
    int n;
    if (__atomic_exchange_n(&crash_taken, 1, __ATOMIC_ACQ_REL)) for (;;) svcSleepThread(1000000000LL); /* CRASHALL: one dump; the first handler ends the process */
    pm_wd_crash_tick = (unsigned)(svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000ull)) | 1u; /* LB4: the monitor thread ends the process if this handler does not finish in 3 s */
    const StackRec* st = NULL;
    for (int i = 0; i < (int)(sizeof recs / sizeof recs[0]); i++) /* BTLDBG: every registered stack (game, audio, render, logic) */
        if (recs[i].hi && regs->sp >= recs[i].lo && regs->sp <= recs[i].hi + 512) st = &recs[i];
    StackRec tst_;
    if (!st) { /* STACKANY */
        u32 lo_, hi_;
        if (pm_thread_stack_current(&lo_, &hi_) && regs->sp >= lo_ && regs->sp < hi_) {
            tst_.lo = lo_; tst_.hi = hi_; tst_.name = "thread";
            st = &tst_;
        }
    }
    const char* tname = pm_thread_name_current();
    if (!tname) tname = st ? st->name : "other";
    n = snprintf(buf, sizeof(buf),
                 "\n[CRASH] %s thread=%s (%lx) pc=%08lx lr=%08lx sp=%08lx far=%08lx fsr=%08lx cpsr=%08lx build #%s\n",
                 excep->type < 4 ? names[excep->type] : "?", tname,
                 (unsigned long)threadGetCurrent(), (unsigned long)regs->pc, (unsigned long)regs->lr,
                 (unsigned long)regs->sp, (unsigned long)excep->far, (unsigned long)excep->fsr,
                 (unsigned long)regs->cpsr, PM_BUILD_STR);
    pm_crash_line(buf, n);
    for (int i = 0; i < 13; i += 4) {
        n = snprintf(buf, sizeof(buf), "[CRASH] ");
        for (int j = i; j < i + 4 && j < 13; j++) n += snprintf(buf + n, sizeof(buf) - (size_t)n, "r%d=%08lx ", j, (unsigned long)regs->r[j]);
        buf[n++] = '\n';
        pm_crash_line(buf, n);
    }
    n = pm_mem_crash_line(buf, sizeof(buf));
    pm_crash_line(buf, n);
    /* Stack walk: only inside the recorded stack bounds, at most 2048 words. */
    if (st && regs->sp >= st->lo && regs->sp < st->hi) {
        n = snprintf(buf, sizeof(buf), "[CRASH] stack used %lu of %lu, code-like words:",
                     (unsigned long)(st->hi - regs->sp), (unsigned long)(st->hi - st->lo));
        buf[n++] = '\n';
        pm_crash_line(buf, n);
        const u32* sp = (const u32*)(regs->sp & ~3u);
        const u32* end = (const u32*)st->hi;
        n = snprintf(buf, sizeof(buf), "[CRASH] stack");
        for (int i = 0, found = 0; sp + i < end && i < 2048 && found < 48; i++) {
            u32 v = sp[i];
            if (v < 0x00100000u || v >= 0x01000000u) continue;
            n += snprintf(buf + n, sizeof(buf) - (size_t)n, " %08lx", (unsigned long)v);
            if (++found % 12 == 0) {
                buf[n++] = '\n';
                pm_crash_line(buf, n);
                n = snprintf(buf, sizeof(buf), "[CRASH] stack");
            }
        }
        buf[n++] = '\n';
        pm_crash_line(buf, n);
    } else {
        n = snprintf(buf, sizeof(buf), "[CRASH] sp %08lx is outside the known thread stacks, no stack walk\n", (unsigned long)regs->sp);
        pm_crash_line(buf, n);
    }
    n = snprintf(buf, sizeof(buf), "[CRASH] last game loop phase %u (%s), steps %u\n[CRASH] exiting the process\n",
                 pm_wd_phase, pm_wd_phase_name(pm_wd_phase), pm_frame_count());
    pm_crash_line(buf, n);
    crash_page_show(excep->type < 4 ? names[excep->type] : "?", tname, excep, regs, st);
    /* LB4: svcBreak(USERBREAK_PANIC) left the console frozen. End the process: the system returns to the menu. */
    if (threadGetCurrent() == NULL) pm_exit_process(); /* the main thread: no thread to end */
    pm_crash_exit = 1; pm_exit_request = 1; /* CRASHEXIT: the game loop ends, main runs pm_shutdown and returns */
    if (pm_opt.esc_mode != 0) aptClearChainloader(); /* esc mode quit or home: the close goes to the HOME Menu */
    pm_wd_crash_tick = (unsigned)(svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000ull)) | 1u; /* 3 s for the clean exit, then the watchdog */
    threadExit(1);
}

/* R01: stop the program on purpose from a debug check (switch romstrict). Azahar never calls the guest handler on a real
 * fault, so a null write would lose the queued log lines. pm_crash_line flushes the queue, writes crash.txt, and the
 * [CRASH] prefix makes run_azahar.ps1 stop. svcBreak ends the process like the crash handler does. */
void pm_fatal_stop(const char* msg) {
    char buf[200];
    pm_log_e("FATAL: %s", msg);
    int n = snprintf(buf, sizeof(buf), "\n[CRASH] debug check: %s build #%s\n", msg, PM_BUILD_STR);
    pm_crash_line(buf, n);
    pm_crash_exit = 1; pm_exit_request = 1; if (threadGetCurrent()) threadExit(1); pm_exit_process();
    for (;;) {}
}

void pm_crash_install(void* stack, unsigned size) {
    threadOnException(crash_handler, (u8*)stack + size, WRITE_DATA_TO_HANDLER_STACK);
}

u64 pm_loop_tick; /* JT: the tick the main loop read in this pass (pm_gpu_end reads it, no svc call of its own) */
unsigned pm_svc_n; /* EC2: svc tick reads, counted for the empty close count */
unsigned pm_ticks(void) { pm_svc_n++; return (unsigned)svcGetSystemTick(); }
/* milliseconds since the first call (64 bit tick, the 32 bit pm_ticks wraps every 16 s) */
unsigned pm_uptime_ms(void) {
    static u64 t0;
    u64 t = svcGetSystemTick();
    if (!t0) t0 = t;
    return (unsigned)((t - t0) / (SYSCLOCK_ARM11 / 1000ull));
}

/* gfxRetrace_Callback flips D_80073E0A and builds a display list only when it
 * lands on 0. The other retrace leaves the N64 framebuffer alone. Clearing
 * and presenting on that retrace flashes black between the logo frames. */
extern s16 D_80073E0A;
/* The intro camera waits until the displayed color buffer changes, which NuSys does on every
 * swap. The list holds the real frame buffers gFrameBuf0..2 (heaps3.c). The GPU does not draw
 * into them. Code that reads the last frame (pause screen, screen transitions) gets it from
 * pm_cfb_capture_prev: a copy of the PICA color buffer in N64 RGBA5551, made only when asked. */
extern unsigned short* nuGfxCfb_ptr;
extern unsigned short** nuGfxCfb;
extern unsigned short* nuGfxZBuffer;
extern unsigned short gFrameBuf0[], gFrameBuf1[], gFrameBuf2[];
void pm_gpu_read_frame(unsigned short* dst);
void pm_tex_invalidate(const void* ptr, unsigned size);
#define CFB_PIXELS (320 * 240)
static unsigned short* cfb_list[3] = { gFrameBuf0, gFrameBuf1, gFrameBuf2 };
unsigned nuGfxCfbNum = 2;
static int in_buf(const void* p, const unsigned short* b) {
    return (const unsigned short*)p >= b && (const unsigned short*)p < b + CFB_PIXELS;
}
/* A color image that is the screen (a frame buffer). Any other color image is memory the game reads back.
 * MAGI: only gFrameBuf0 and gFrameBuf1. nuGfxCfbNum is 2 and nuGfxSetCfb is empty, so gFrameBuf2 is never drawn or shown on the port;
 * it is the battle heap (heap_battleHead: actors, the battle HUD cache, run-time data). With gFrameBuf2 here, every 16 bit texture
 * from the battle heap was taken for an older frame (nd_tex_set.c set_timg: pm_cfb_capture_prev, a full frame copy and a frame split
 * per load) and read as native u16 pixels instead of its own bytes. */
int pm_is_cfb(const void* p) { return in_buf(p, gFrameBuf0) || in_buf(p, gFrameBuf1); }
/* B2: the frame buffer the game draws into now (the T2 grab source). Any other frame buffer holds an older frame (T3). */
extern int pm_wide;
/* F07-7: with wide a window may start up to 40 pixels before the buffer (game x -40 of row 0, effects in the gutters) */
void* pm_walk_cfb; /* CORE2C-2: set by pm_frame_walk from the frame (the walker reads this, the game side nuGfxCfb_ptr) */
int pm_is_cur_cfb(const void* p) { const void* c = pm_walk_cfb; return c && (in_buf(p, c) || (pm_wide && in_buf((const u8*)p + 80, c))); }
/* Frame buffer and saved-frame memory hold native u16 pixels (the game code reads them as u16). Textures
 * from ROM data hold big-endian bytes. pm_tex reads these two kinds differently. */
int pm_fb_native(const void* p) { return pm_is_cfb(p) || (nuGfxZBuffer && in_buf(p, nuGfxZBuffer)); }
/* PAUSEC: the saved picture (150 KB) is reserved once at boot, after the ROM is in, when the heap is whole. On the O3DS XL the commit is
 * full after the map loads (crash log: mem heap sbrk-used 26868 of 29512 KB, commit 65536 of 65536), so a first allocation at the pause
 * can fail there. A failed reservation logs an error; pm_zbuf_need retries at the pause, and the filter is skipped if it still fails. */
void pm_zbuf_reserve(void) {
    if (nuGfxZBuffer) return;
    nuGfxZBuffer = (unsigned short*)malloc(CFB_PIXELS * 2);
    if (nuGfxZBuffer) pm_log("D03: z-buffer image reserved at boot, %d KB", CFB_PIXELS * 2 / 1024);
    else pm_log_e("D03: no memory for the %d KB z-buffer image at boot (the pause draws over the live frame)", CFB_PIXELS * 2 / 1024);
    { /* PAUSEDR: the reserve result on the prof line, so a console prof run shows whether the grey pause picture can be saved */
        extern unsigned pm_heap_free_kb(void);
        extern void pm_log_prof_line(const char* fmt, ...);
        pm_log_prof_line("prof zbuf reserved %d heap_free %u KB", nuGfxZBuffer ? 1 : 0, pm_heap_free_kb());
    }
}
/* D03: the z-buffer memory the pause code saves the last frame into. Allocated at boot (pm_zbuf_reserve) or when first needed. */
/* PAUSEC: the logic thread may need the image before the walk that would allocate it (the pause filter, the darkness overlay). It then
 * waits for the walker to be idle, so the allocation and the walker's own call (img_role) never race. A failed allocation logs once; the
 * caller sees nuGfxZBuffer NULL and skips the picture. */
void pm_zbuf_need(void) {
    if (!nuGfxZBuffer) {
        static int fail_logged;
        if (pm_wq_on_logic()) pm_walk_drain("zbuf");
        if (!nuGfxZBuffer) nuGfxZBuffer = (unsigned short*)malloc(CFB_PIXELS * 2);
        if (!nuGfxZBuffer) { if (!fail_logged) { fail_logged = 1; pm_log_e("D03: no memory for the %d KB z-buffer image", CFB_PIXELS * 2 / 1024); } }
        else pm_log("D03: z-buffer image allocated, %d KB", CFB_PIXELS * 2 / 1024);
    }
    if (nuGfxZBuffer) pm_tex_invalidate(nuGfxZBuffer, CFB_PIXELS * 2);
}

/* X02: the darkness overlay draws into the z-buffer memory (a 64 x 64 image). The game owns that memory from boot, the port
 * allocates it on first use (see pm_zbuf_need), so the colour image address is valid before the display list names it. */
void pm_x02_log(int isWorld, int alpha, int x, int y);
void __real_appendGfx_darkness_stencil(int isWorld, int posX, int posY, float alpha, float progress);
void __wrap_appendGfx_darkness_stencil(int isWorld, int posX, int posY, float alpha, float progress) {
    pm_x02_log(isWorld, (int)alpha, posX, posY);
    pm_zbuf_need();
    __real_appendGfx_darkness_stencil(isWorld, posX, posY, alpha, progress);
}

void pm_dump_scripts(void);
void pm_warp_tick(unsigned frame);
unsigned pm_uptime_ms(void);
unsigned pm_frame_n; /* the logic step counter (set at the end of each step) */
extern unsigned pm_walk_frame; /* nd_wq.c: the frame of the entry the walker runs */
/* CORE2C-3: walker-side code sees the frame of the entry it runs (the frame the logic had when it posted it), other code the step counter. */
unsigned pm_frame_count(void) { return pm_walk_side && !pm_wq_on_logic() ? pm_walk_frame : pm_frame_n; }
/* D03: copy the last finished frame into the frame buffer that is not being drawn (the N64 "previous
 * color buffer"). The PICA color buffer holds that frame until C3D_FrameEnd of this frame runs the
 * command list, so any time during this frame's draw recording or run is right. Once per frame. */
PmB7 pm_b7;
/* PT12: a texture read of the old frame (G_SETTIMG): a GPU copy into the slot of the old frame buffer, no CPU pixels. The CPU readback is the
 * fallback when the GPU copy is not possible this frame. The pause picture is a texture read too (PAUSEOPEN, background_gfx.c). */
int pm_gpu_grab_prev(const void* prev_cfb);
extern short get_game_mode(void);
static int pm_game_mode_now(void) { return (int)get_game_mode(); }
/* BTRANS caplog (switch caplog): one line per capture call. path gpu2 = copy made at pm_gpu_begin, cpu = CPU read of the live colour buffer
 * (the swirl fallback, the case the row names), done = already taken this frame. split, kick and clear show whether this frame already
 * queued its KICK1 split or clear before the read. */
void pm_gpu_cap_state(int* o);
int pm_tex_failed(void);
int pm_enc_wipe(void);
static void cap_note(const char* path) {
    static int on = -1;
    int st[8];
    if (on < 0) on = pm_debug_has("caplog");
    if (!on) return;
    pm_gpu_cap_state(st);
    pm_log_d("caplog f%u cpu 0 repl %d interp %d path %s prev_pre %d grab_n %d split %d kick %d clear %d draws %d off_fail %d tex_fail %d tgt %d wipe %d why %s",
             pm_frame_count(), pm_interp_repl, pm_interp_on, path, st[0], st[1], st[2], st[3], st[4], st[5], st[6], pm_tex_failed(), st[7], pm_enc_wipe(),
             !st[0] ? "prev0" : st[1] ? "grabn" : "setup");
}
/* PAUSEOPEN: the last frame for a texture read (set_timg). The GPU copy when this drawn frame has it, else the CPU readback of the frame buffer
 * that is not drawn now. The pause no longer reads the pixels on the CPU (ROLE_CPUFILL only writes the coverage). */
void pm_cfb_capture_prev(void) {
    static unsigned done = ~0u;
    unsigned i;
    /* GAMEOVER: the GPU copy is asked in every drawn frame, the t = 0.5 pass (pm_interp_repl) too: pm_gpu_grab_prev knows whether this drawn
     * frame has its slot. A guard on pm_frame_n (one game step, two drawn frames with fps60) left the t = 0.5 pass without the slot: its blur
     * strips read the stale frame buffer memory, every second picture differed (the game over flicker). */
    for (i = 0; i < nuGfxCfbNum; i++) {
        if (nuGfxCfb[i] == pm_walk_cfb) {
            unsigned t0 = pm_ph_on ? pm_ticks() : 0;
            if (pm_gpu_grab_prev(nuGfxCfb[(i + nuGfxCfbNum - 1) % nuGfxCfbNum]) == 2) {
                if (pm_ph_on) { unsigned dt = pm_ticks() - t0; pm_b7.cap_n++; pm_b7.cap_ticks += dt; if (dt > pm_b7.cap_max) pm_b7.cap_max = dt; }
                cap_note("gpu2");
                return;
            }
            break;
        }
    }
    if (done == pm_frame_count()) { cap_note("done"); return; }
    done = pm_frame_count();
    for (i = 0; i < nuGfxCfbNum; i++) {
        /* PT78: the read takes the frame from the buffer that is not the drawn one, whatever the parity of the drawn step (a step skipped by FS1 moves the
         * draw to the other parity: the old rule, the previous buffer of the drawn one, wrote the draw target and the read got a stale frame). */
        if (nuGfxCfb[i] == pm_walk_cfb) {
            unsigned short* prev = nuGfxCfb[(i + nuGfxCfbNum - 1) % nuGfxCfbNum];
            unsigned t0 = pm_ph_on ? pm_ticks() : 0;
            static unsigned nlog;
            { /* BTRANS: a split this frame (KICK1 or a grab split) put part of this frame in the colour buffer: no CPU read, the slot keeps its last frame */
                int st[8];
                pm_gpu_cap_state(st);
                if (st[2] || st[3]) { cap_note("skip"); return; }
            }
            cap_note("cpu");
            if (pm_xf.on) pm_xf.cap_cpu++; /* BTLWIPE: the capcpu field of the xfer line */
            pm_gpu_read_frame(prev);
            if (pm_ph_on) { unsigned dt = pm_ticks() - t0; pm_b7.cap_n++; pm_b7.cap_ticks += dt; if (dt > pm_b7.cap_max) pm_b7.cap_max = dt; }
            if (pm_ph_on && nlog++ < 6) pm_log("D03: last frame copied to a frame buffer in %u us (ARM11 tick, Azahar timing)", (unsigned)((pm_ticks() - t0) / 268u));
            pm_tex_invalidate(prev, CFB_PIXELS * 2);
            return;
        }
    }
}
/* PC01: once per second (switch `pacestat`): game steps, drawn frames, vblanks and audio ticks since the last line. */
extern unsigned pm_step_calls;
static unsigned pace_ticks;
/* AU1b: the audio ticks of a step run at the kick point (entry of the main nuGfxTaskStart, after the game logic of the step), not at the top of
 * the iteration. pm_audio_step is called once per drawn step. The main loop calls it after pm_gfx_retrace when the step sent no main task. */
static int au_kicked;
unsigned pm_audio_last_n; /* DET: ticks of the last audio step, the detlog line reads and clears it */
/* DET: switch det = the frame schedule is a function of the game steps only: no FS1 skip, no interp skip, 2 audio ticks a step. */
static int pm_det_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("det"); return v; }
void pm_audio_step(void) {
    if (au_kicked) return;
    au_kicked = 1;
    /* AUDDROP: switch aulag=<ms> sleeps every game step (reproduces the O3DS step length on Azahar, not under det) */
    static int aulag = -1;
    if (aulag < 0) { char kv[16]; aulag = pm_debug_value("aulag", kv, sizeof kv) > 0 ? atoi(kv) : 0; }
    if (aulag > 0 && !pm_det_on()) svcSleepThread((s64)aulag * 1000000LL);
    unsigned n = pm_det_on() ? 2u : pm_audio_retraces(); /* DET: 2 a step */
    if (!n) pm_aud_zero_step(); /* AUDDROP census: a step with no audio tick */
    pm_audio_last_n = n;
    PH_ADD(C_ATICK, n);
    pace_ticks += n;
    pm_audio_kick(n); /* AU1d: runs on core 1 until the next join */
}
static unsigned pace_iv[4]; /* vblanks between two drawn iterations: 1, 2, 3, 4 or more */
/* FS1 */
#define FS1_MAX_RUN 3 /* FS1b: 3 skips cover a stall of 7 vblanks */
static unsigned fs1_steps, fs1_skipped, fs1_logged, fs1_last_iv, fs1_run, fs1_force;
static int fs1_off, fs1_on_log;
void pm_fskip_set(int on) { fs1_off = !on || pm_debug_has("fs1off"); } /* options panel row 9: takes effect at the next step */
/* PC01: vblanks per game step. 2 = 30 fps. A later 60 fps option (F06) changes only this value. Needs more work
 * than a changed number for 1: FrameBegin(SYNCDRAW) then has to stop waiting for the vblank (not done, not in scope). */
#define PM_VBLANKS_PER_STEP 2
unsigned pm_pace_drawn; /* PT24: all steps with a draw since the start (the pace line prints the difference) */
static u64 pace_wk; static unsigned pace_wkn; /* PROF2-6: work ticks (step start to frame end, no idle) and count of the drawn frames since the last pace line */
static void pace_stat(u64 now, u64 hz, unsigned frame) {
    static u64 t0;
    static unsigned s0, f0, v0, a0, d0, i0[4];
    unsigned v = C3D_FrameCounter(0);
    if (!t0) { t0 = now; s0 = pm_step_calls; f0 = frame; v0 = v; a0 = pace_ticks; d0 = pm_pace_drawn; for (int i = 0; i < 4; i++) i0[i] = pace_iv[i]; return; }
    if (now - t0 < 5u * hz) return; /* WARPLOG2: one pace line per 5 s */
    float wall = (float)(now - t0) * 1000.f / (float)hz;
    pm_log_d("pace: wall %.1f ms steps %u iterations %u drawn %u vblanks %u audio ticks %u  (steps/s %.2f) intervals 1:%u 2:%u 3:%u 4+:%u total 1:%u 2:%u 3:%u 4+:%u late steps (3+, over 33.3 ms) %u of %u", wall, pm_step_calls - s0,
           frame - f0, pm_pace_drawn - d0, v - v0, pace_ticks - a0, (float)(pm_step_calls - s0) * 1000.f / wall, pace_iv[0] - i0[0],
           pace_iv[1] - i0[1], pace_iv[2] - i0[2], pace_iv[3] - i0[3], pace_iv[0], pace_iv[1], pace_iv[2], pace_iv[3], pace_iv[2] + pace_iv[3], pace_iv[0] + pace_iv[1] + pace_iv[2] + pace_iv[3]);
    { unsigned wk = pace_wkn ? (unsigned)(pace_wk * 100ull / ((u64)pace_wkn * (hz / 1000ull))) : 0u; /* PROF2-6: mean work ms x 100 of a drawn frame, the prof-off side of the prof self-check (prof wk) */
      pm_log_d("pacewk f%u wk %u.%02u drawn %u", frame, wk / 100u, wk % 100u, pace_wkn); pace_wk = 0; pace_wkn = 0; }
    for (int i = 0; i < 4; i++) i0[i] = pace_iv[i];
    t0 = now; s0 = pm_step_calls; f0 = frame; v0 = v; a0 = pace_ticks; d0 = pm_pace_drawn;
}
unsigned pm_tex_evict_n(void);
unsigned pm_gpu_noise_state(void);
extern unsigned gRandSeed;
/* DET: detlog switch. One line per game step (frame) with the draw sequence and the inputs of the schedule. */
static unsigned det_keys, det_draws, det_iv;
/* HANG1: gspWaitForVBlank has no timeout, so a GSP that stops sending vblanks blocks the game thread with no line. This poll
 * waits at most 40 ms (200 x 0.2 ms) for the citro3d vblank counter to change. */
static int vblank_wait_timed(void) {
    u32 c0 = C3D_FrameCounter(0);
    for (int i = 0; i < 200; i++) {
        if (C3D_FrameCounter(0) != c0) return 1;
        svcSleepThread(200000ll);
    }
    return 0;
}
extern volatile int pm_exit_request; /* PT28 */
extern unsigned pm_ph_iv, pm_ph_hud_us; /* PI4, PI5 */
extern int pm_ph_on;
void pm_boot_title_check(void); /* BOOT-1: prints the boot ms line at the first title-screen step */
/* CORE2-3a: the end of a drawn frame on the walker side: the boot fallback picture when the step drew nothing, the BOOT-1 mark, the
 * frame end and present. lp_ever: a frame drew (the frame loop draws every pass until then; with the logic thread it sees it one
 * step late, boot only). */
static int lp_ever;
static void wq_frame_end(void) {
    if (!pm_gpu_drew()) {
        if (!lp_ever) pm_gbi_fallback();
    } else {
        if (!lp_ever) pm_boot_mark(PMB_DRAW, 0); /* BOOT-1 */
        lp_ever = 1;
    }
    pm_gpu_end();
}
/* CORE2C-3: the end entry of a step (walker side): the frame end of a drawn step, then the count pm_lg_show waits for. */
void pm_lg_end_walked(void);
void pm_lg_key_due(void);
typedef struct { int drew, key; } StepEnd;
static void wq_step_end(void* a) { const StepEnd* e = (const StepEnd*)a; if (e->drew) wq_frame_end(); if (e->key) pm_lg_key_due(); pm_lg_end_walked(); }
void pm_fxtest_tick(unsigned);
/* CORE2C-3: the body of one frame loop command (pm_logic.c), on the logic thread or inline. LG_IDLE: the retrace call of an idle pass.
 * LG_STEP: frame begin, the step (step_game_loop, list build, frame push), the audio ticks, the step end; LG_SKIP: the FS1 step with no
 * draw. Then the step tail: the frame number, warp and tour, fxtest, the script dump. */
void pm_step_run(const LgCmd* c) {
    if (c->kind == LG_IDLE) { pm_gfx_retrace(); return; }
    int drew = c->kind == LG_STEP;
    pm_interp_skip = (c->flags & LGF_ISKIP) != 0;
    nuGfxCfb_ptr = (unsigned short*)c->cfb;
    pm_pad_step(&c->pad);
    au_kicked = 0;
    if (drew) {
        pm_gpu_begin(1);
        pm_gfx_retrace();
        pm_xfer_hidden(); /* DOORHITCH: hidden models of a new map, after this frame's walk entries */
    } else {
        pm_gfx_retrace_skip();
        FS_MARK(FS_REST); /* FS2: the skipped step logic is not a drawn part of the next frame */
    }
    pm_audio_step(); /* AU1b: no main task in this step (or a skipped step): the ticks run now */
    PmKey k;
    StepEnd se = { drew, 0 };
    if (drew && pm_interp_on > 0) se.key = pm_interp_key_take(&k) && !(c->flags & LGF_NOKEY); /* the take always clears the list */
    pm_wq_post(wq_step_end, &se, sizeof se);
    pm_frame_n = c->frame;
    pm_warp_tick(c->frame);
    pm_fxtest_tick(c->frame); /* FX1a, switch fxtest */
    if (pm_debug_has("scripts") && c->frame % 120u == 0) pm_dump_scripts();
    if (se.key) pm_wq_key(&k); /* the key frame goes after the step tail: the logic of the next step runs while it walks */
}
/* INTERP1: interpolation stays on only while the walks of a step fit (core 0 walks twice a step with fps60). Heavy: the last walk or the
 * GPU drawing time of the last frame is over 13 ms; calm again after 30 steps in a row under 10 ms. INTERPB3: the walk limit is 26 ms (calm under 20 ms), one walk a step. Heavy: one walk a step (ISKIP). */
extern unsigned pm_iw_us[3]; /* pm_nusys.c */
extern unsigned pm_kc_n[2], pm_kc_why[5]; /* INTERPB3: copies shown, key walks; copies refused by cause */
static int interp_heavy(void) {
    static int heavy, calm;
    static unsigned n, nh, sl, sk, sp, sg;
    unsigned w = heavy ? pm_iw_us[2] : (pm_iw_us[0] > pm_iw_us[1] ? pm_iw_us[0] : pm_iw_us[1]);
    unsigned g = (unsigned)(C3D_GetDrawingTime() * 1000.f);
    if (w > 26000u || g > 13000u) { heavy = 1; calm = 0; } /* INTERPB3: one walk a step (the key frame is the K copy): the walk limit is 26 ms, calm under 20 ms; two GPU frames a step: the GPU limit stays 13 ms */
    else if (heavy && w < 20000u && g < 10000u) { if (++calm >= 30) heavy = 0; }
    else calm = 0;
    n++; nh += heavy; sl += pm_iw_us[0]; sk += pm_iw_us[1]; sp += pm_iw_us[2]; sg += g;
    if (n == 300) {
        pm_log_hw("interp f%u steps %u heavy %u lerp_us %u key_us %u plain_us %u gpu_us %u kcopy %u kwalk %u why %u %u %u %u %u", pm_frame_count(), n, nh, sl / n, sk / n, sp / n, sg / n, pm_kc_n[0], pm_kc_n[1], pm_kc_why[0], pm_kc_why[1], pm_kc_why[2], pm_kc_why[3], pm_kc_why[4]);
        pm_kc_n[0] = pm_kc_n[1] = 0; pm_kc_why[0] = pm_kc_why[1] = pm_kc_why[2] = pm_kc_why[3] = pm_kc_why[4] = 0;
        n = nh = sl = sk = sp = sg = 0;
    }
    return heavy;
}
/* CORE2C-3: the serial mode: the timers and census that read one stack or table from both sides, and det. */
static int lp_serial(void) { return pm_det_on() || pm_ph_on || pm_ph_cen || pm_fs_on || pm_dcs_on; }
void pm_frame_loop(void) {
    unsigned frame = 0;
    pm_uptime_ms(); /* start the clock */
    { extern unsigned gRandSeed; pm_log("rng start seed %08x%s", gRandSeed, pm_debug_has("fixedseed") ? " (fixedseed)" : ""); } /* R-SEED */
    nuGfxCfb = cfb_list;
    pm_lg_start(); /* CORE2C-3: the logic thread (core 1 on the O3DS, core 2 on the N3DS) */
    unsigned presents = 0, drawn = 0; /* PT24: presents = logic steps, drawn = steps with a draw */
    int have_fps = 0;
    unsigned fps_vb0 = 0, late_n = 0; /* PT15 */
    unsigned sp_n = 0; /* WARPLOG2: steps over 50 ms in the 300-frame logstat line */
    float sp_max = 0.f;
    int last_step_skipped = 0; /* PT87 */
    int late_pct = 0;
    u64 step_start = 0;
    unsigned step_vb = 0; /* vblank counter when the last drawn iteration got past FrameBegin */
    int step_vb_ok = 0;
    int menu_frozen = 0; /* MN1: the Select menu is open and the game is paused */
    int fa_state = 0; /* FADV: 0 off, 1 hold (no game step, no draw, the picture stays), 2 step (one game step runs, then 1 again) */
    extern int pm_fa_get(void); extern int pm_pad_take_step(void);
    u64 fps_mark = 0;
    float fps = 0.f, fps_draw = 0.f; /* PT24: steps/s (internal) and drawn frames/s (external) */
    /* HUD2: bottom screen memory rows, refreshed at pm_opt.hud_ms (pm_mem_hud). */
    extern void pm_mem_hud(char out[3][40]);
    char mem[3][40] = {{0}};
    float frame_ms = 0.f;
    const u64 tick_hz = SYSCLOCK_ARM11;
    float stat_ms = 0.f;
    unsigned stat_n = 0;
    u64 stat_t0 = 0;
    int pace_on = 0; /* PROFROW: read each frame below, the Profiling row can switch it */
    const int pos_dbg = pm_debug_has("poshud"); /* POSHUD: the poshud switch, same as the option */
    { /* FS1 switches: fs1off, fs1force=<n> (skip every n-th step) */
        char fv[16];
        fs1_off = pm_debug_has("fs1off") || !pm_opt.fskip;
        fs1_force = pm_debug_value("fs1force", fv, sizeof fv) ? (unsigned)atoi(fv) : 0;
        fs1_on_log = pm_log_level() >= 3;
    }
    /* PT88: the game step schedule is time, not the vblank count. A step is due every 1/29.92 s (sched_next). The idle pass waits until the next step is due, a pass
     * draws unless a whole step or more is owed, then only its draw is skipped. The frame begin no longer waits for a vblank (pm_gpu_begin), so a heavy scene
     * (cpu 55 ms and 11 ms of vblank wait) draws as often as the CPU allows, 18 a second instead of 15. With fps60 the old vblank schedule stays (two draws a step). */
    u64 sched_next = 0;
    const u64 step_period = tick_hz * 1000ull / 29920ull;
    while (aptMainLoop() && !pm_exit_request) {
        pm_wd_prog++;
        pm_wd_armed = 1;
        PM_WD_PHASE(1);
        pm_rom_stream_poll(); /* BOOT: a failed ROM chunk read shows the fatal screen (one load once the stream is done) */
        u64 now = svcGetSystemTick();
        pm_loop_tick = now;
        /* The retrace builds a display list only every other call. Presenting
         * the idle call shows the other framebuffer, which was never drawn. */
        int draw = pm_lg_draw_flag() != 0 || !lp_ever; /* CORE2C-3: the flag after the last command (it waits for the step before) */
        static int prev_draw; const int lastdraw = prev_draw; prev_draw = draw; /* GREY: draw flag of the previous pass */
        static int top_after_due; static unsigned top_after_k;
        PM_WD_PHASE(2);
        pm_log_poll();
        pm_esc_take(); /* ESCAPE2: the START+SELECT request of the watchdog */
        if (pm_menu_open) pm_lg_quiesce(); /* CORE2C-3: the menu reads and writes game state in the pad scan: the logic is idle */
        pm_pad_scan();
        if (pm_menu_open) { /* MN1: the Select menu pauses the game: no step, no draw, no audio tick. The menu text is presented here. */
            pm_lg_quiesce(); /* CORE2C-3: the open press came in this scan */
            if (!menu_frozen) { /* MN2, PAUSE AUDIO */
                extern unsigned pm_gx_qpend(void); extern void pm_gpu_menu_dirty_set(void);
                menu_frozen = 1; pm_log("menu: game paused at frame %u", frame);
                pm_log("menu: grey qpend %u lastdraw %d frame %u", pm_gx_qpend(), lastdraw, frame);
                pm_gpu_menu_dirty_set(); pm_gpu_menu_present(); pm_audio_mute(1); /* GREY: a forced present waits for the queued game frame (FrameBegin SYNCDRAW), then greys after its own transfer */
                top_after_due = 1;
            }
            gspWaitForVBlank();
            if (top_after_due) { /* GREY: the displayed top buffer one vblank after the pause */
                extern void pm_gpu_shot_top_displayed(const char* name);
                char tn[24];
                top_after_due = 0;
                if (pm_debug_has("shotgrey")) { snprintf(tn, sizeof tn, "top_after%04u", top_after_k++); pm_gpu_shot_top_displayed(tn); }
            }
            pm_gpu_menu_present();
            { /* B1 repro: shotfrozen=<n> takes the bottom screen on the n-th frozen pass (bot_fz<n>), then the event line */
                static int fz_want = -2;
                static unsigned fz_n;
                if (fz_want == -2) {
                    char fv[16];
                    int k = -1;
                    if (pm_debug_value("shotfrozen", fv, sizeof fv)) sscanf(fv, "%d", &k);
                    fz_want = k;
                }
                if (fz_want > 0 && ++fz_n == (unsigned)fz_want) {
                    char bname[32];
                    snprintf(bname, sizeof bname, "bot_fz%04u", fz_n);
                    pm_gpu_shot_bottom(bname);
                    pm_log_evt("menu: frozen shot %u late %u drop %u copies %u nomenu %u", fz_n, pm_log_late_con(), pm_log_late_drop(), pm_log_con_copies(), pm_log_con_nomenu());
                }
            }
            continue;
        }
        if (menu_frozen) { /* MN1: resume: the schedule starts again, no catch-up steps */
            menu_frozen = 0;
            sched_next = 0;
            step_vb_ok = 0;
            fa_state = pm_fa_get() ? 1 : 0; /* FADV: the row decides at the close */
            pm_audio_mute(fa_state); /* PAUSE AUDIO: the sound goes on from the paused buffer (before the resumed line: the until line); a frame advance hold stays silent */
            pm_log("menu: game resumed at frame %u", frame);
        }
        if (fa_state == 1) { /* FADV hold: no game step and no draw, the picture on the top screen stays */
            pm_lg_quiesce(); /* CORE2C-3: the last step is shown */
            if (!pm_pad_take_step()) { gspWaitForVBlank(); continue; }
            fa_state = 2; /* R: one game step, with no catch-up and no wait (sched_next and step_vb_ok as at a resume) */
            sched_next = 0; step_vb_ok = 0;
        }
        pace_on = PMDL("pacestat");
        if (pace_on) pace_stat(now, tick_hz, frame);
        if (!draw) {
            { LgCmd ic = { LG_IDLE, frame, NULL, { 0, 0, 0, 0, 0 } }; pm_lg_hand(&ic, lp_serial()); }
            /* F06-2: key frame of the last step, drawn and presented here (FrameBegin waits for the vblank) */
            if (pm_interp_on > 0) { /* PT87: the key frame is a draw, not a game step (it was counted as a step: the HUD showed x/x) */
                unsigned kn = pm_lg_show_keys();
                if (kn) { drawn += kn; pm_pace_drawn += kn; det_keys += kn; continue; }
            }
            /* PC01: keep PM_VBLANKS_PER_STEP vblanks between game steps. The counter is citro3d's own vblank
             * counter. The next drawn iteration waits for one more vblank in C3D_FrameBegin(SYNCDRAW), so this
             * waits until the counter is PM_VBLANKS_PER_STEP - 1 past the last drawn iteration. A slow frame is
             * already past it: no wait and no extra step to catch up. `fast` stays uncapped (debug). */
            if (!PMDL("fast") && step_vb_ok && pm_interp_on <= 0 && sched_next) { /* PT88: wait until the next step is due (sleep, then a short spin) */
                PM_WD_PHASE(3);
                PH_PUSH(PH_IDLE);
                for (;;) {
                    u64 t = svcGetSystemTick();
                    if (t >= sched_next) break;
                    u64 rem = sched_next - t;
                    if (rem > tick_hz / 500ull) pm_lg_sleep_ns((s64)((rem - tick_hz / 1000ull) * 1000000000ull / tick_hz)); /* leave 1 ms */
                    else if (rem > tick_hz / 5000ull) pm_lg_sleep_ns(100000ll);
                    else break;
                }
                PH_POP();
            } else if (!PMDL("fast") && step_vb_ok && pm_interp_on > 0) {
                PM_WD_PHASE(3);
                int guard = PM_VBLANKS_PER_STEP + 1;
                PH_PUSH(PH_IDLE);
                while ((int)(C3D_FrameCounter(0) - step_vb) < PM_VBLANKS_PER_STEP - 1 && guard--) {
                    if (vblank_wait_timed()) continue;
                    static int vb_logs; /* HANG1: the first 20 timeouts only */
                    if (vb_logs++ < 20) pm_log_w("gpu: no vblank for 40 ms in the idle wait (counter %u, last drawn %u)", (unsigned)C3D_FrameCounter(0), step_vb);
                    break;
                }
                PH_POP();
            }
            continue;
        }
        step_start = now;
        if (pm_interp_on > 0) sched_next = 0; else if (!sched_next) sched_next = now;
        void* cfb = nuGfxCfb[frame % nuGfxCfbNum]; /* the logic step sets nuGfxCfb_ptr from the command */
        /* FS1: N64 frame skip as a safety net. Skip the draw of this step only when the machine is really late: the last drawn step took more than
         * PM_VBLANKS_PER_STEP vblanks and this step starts when that many vblanks have already passed since the last drawn one (on time it starts after 1). At most FS1_MAX_RUN skips in a row.
         * The game logic runs as always (gfxTaskNum 3 in gfxRetrace_Callback), FrameBegin and FrameEnd are not called, the picture on screen stays. */
        int fs_skip = 0, fs_force_skip = 0;
        if (lp_ever && step_vb_ok && !fs1_off && !PMDL("fast") && !pm_det_on()) { /* PT87: also with fps60: a late step skips its draws (the key frame too), the game logic keeps 30 steps a second. Not with fast (uncapped debug runs). */
            fs1_steps++;
            /* FS1b: skip the draw whenever a whole step of the schedule has passed since the last drawn or skipped step (the step is at least one vblank late).
             * A skip pays two vblanks (step_vb += 2), so the draws stay on the schedule and the game logic runs at the full rate. */
            if (pm_interp_on <= 0) fs_skip = fs_force_skip || (now >= sched_next + step_period) || (fs1_force && fs1_steps % fs1_force == 0); /* PT88: a whole step or more is owed */
            else fs_skip = fs_force_skip || C3D_FrameCounter(0) - step_vb >= PM_VBLANKS_PER_STEP || (fs1_force && fs1_steps % fs1_force == 0);
            if (fs_skip && fs1_run >= FS1_MAX_RUN) fs_skip = 0;
        }
        last_step_skipped = fs_skip;
        LgCmd lc = { fs_skip ? LG_SKIP : LG_STEP, frame + 1u, cfb, { 0, 0, 0, 0, 0 } };
        pm_pad_latch(&lc.pad); /* CORE2C-3: the pad of this pass, read by the game once in the step */
        if (fa_state != 0) lc.flags |= LGF_NOKEY; /* FADV: a hold or step draws no key frame */
        if (fs_skip) {
            fs1_run++;
            fs1_skipped++;
            det_iv = step_vb_ok ? C3D_FrameCounter(0) - step_vb : 0u; /* DET: the vblanks of a skipped step */
            step_vb += PM_VBLANKS_PER_STEP; /* the skipped step used its share of the schedule */
            PM_WD_PHASE(5);
            pm_lg_hand(&lc, lp_serial());
            pm_lg_show(); /* the step before, when it drew */
        } else {
        fs1_run = 0;
        drawn++; pm_pace_drawn++; det_draws++;
        PM_WD_PHASE(4);
        {
            unsigned vb = C3D_FrameCounter(0);
            if (step_vb_ok) {
                unsigned iv = vb - step_vb;
                pm_ph_iv = iv; /* PI4 */
                pace_iv[iv < 1 ? 0 : iv > 4 ? 3 : iv - 1]++;
                if (!pm_det_on() && (iv > PM_VBLANKS_PER_STEP || (pm_interp_on > 0 && interp_heavy()))) lc.flags |= LGF_ISKIP; /* F06-5; DET: the key path is never taken on the vblank count. INTERP1: heavy walks skip the key frame */
                fs1_last_iv = iv;
                if (iv >= 3) late_n++; /* PT15 */
            }
            det_iv = step_vb_ok ? vb - step_vb : 0u; /* DET */
            step_vb = vb;
            step_vb_ok = 1;
        }
        PM_WD_PHASE(5);
        pm_lg_hand(&lc, lp_serial()); /* CORE2C-3: the step (logic thread, or inline in the serial mode) */
        PM_WD_PHASE(7);
        pm_lg_show(); /* walk and present every finished step: the one before (logic thread) or this one (serial) */
        pm_log_frame_tick();
        pm_boot_title_check(); /* BOOT-1 */
        pm_ph_frame_end();
        }
        if (fs1_on_log && (fs1_steps % 300) == 0 && fs1_steps != fs1_logged) { fs1_logged = fs1_steps; pm_log_hw("fs1 skipped %u of %u steps", fs1_skipped, fs1_steps); }
        if (pm_det_on() && frame % 120u == 0) { /* CORE2-1: the audio hash after the job of this step (join first: the job may still run) */
            pm_audio_join_at(0);
            extern unsigned pm_au2_hash(unsigned* n); unsigned an, ah = pm_au2_hash(&an);
            pm_log_evt("audiohash f%u cmds %u fnv %08x", frame, an, ah);
        }
        if (PMDL("detlog")) { /* DET: one line per game step: the draw sequence (key and lerp frames), the vblanks since the last step, the audio ticks, the RNG and noise state, texture evictions */
            pm_log_d("det step %u keys %u draws %u iv %u skip %d interp_skip %d aud %u rng %08x noise %08x evict %u", frame, det_keys, det_draws, det_iv, fs_skip, pm_interp_skip, pm_audio_last_n, gRandSeed, pm_gpu_noise_state(), pm_tex_evict_n());
            det_keys = det_draws = 0; pm_audio_last_n = 0;
        }
        frame++;
        if (fa_state == 2) { fa_state = 1; pm_log("fa: step frame %u", frame); } /* FADV: the step is done, hold again */
        if (pm_interp_on <= 0 && sched_next) { /* PT88: one step is used. After a long stall (a load) do not run to catch up: keep at most 3 steps owed. */
            u64 t = svcGetSystemTick();
            sched_next += step_period;
            if (t > sched_next + 3 * step_period) sched_next = t - 3 * step_period;
        }
        pm_wd_prog++;
        PM_WD_PHASE(8);
        PM_WD_PHASE(9);
        presents++;
        now = svcGetSystemTick();
        frame_ms = (float)(now - step_start) * 1000.f / (float)tick_hz;
        pm_xf.step_smp = pm_ph_on; /* DOORHITCH: a prof sample step (pm_pf_frame turns the phase timers off) */
        if (pm_pf_on && !last_step_skipped) pm_pf_frame(step_start, now); /* PF1: switch prof */
        { extern void pm_fspk_frame(unsigned, float); pm_fspk_frame(frame, frame_ms); } /* MAGI: switch fspk */
        pm_xfer_step(step_start, now); /* MAPX-1: the xfer line of a door or map change */
        if (pace_on && !last_step_skipped) { pace_wk += now - step_start; pace_wkn++; } /* PROF2-6: pacestat work time of a drawn frame */
        if (frame_ms > 50.f) { sp_n++; if (frame_ms > sp_max) sp_max = frame_ms; }
        { /* PT15: steps per second against the retrace (steps * 60 / vblanks, 120 vblanks) and the share of steps that took 3 or more vblanks */
            unsigned vbn = C3D_FrameCounter(0);
            if (!fps_vb0) fps_vb0 = vbn;
            if (vbn - fps_vb0 >= 60) { /* PT87: a window of one second (was two) */
                fps = (float)presents * 60.f / (float)(vbn - fps_vb0);
                fps_draw = (float)drawn * 60.f / (float)(vbn - fps_vb0);
                                late_pct = drawn ? (int)(100u * late_n / drawn) : 0; /* late_n counts drawn steps only */
                presents = 0;
                drawn = 0;
                late_n = 0;
                fps_vb0 = vbn;
                fps_mark = now;
                have_fps = 1;
            }
        }
        { /* HUDINT: the bottom memory rows refresh every pm_opt.hud_ms (options.ini hudint, default 1 s). The step is in ticks, set once per refresh. */
            static u64 mem_mark, mem_step;
            if (!mem_mark || now - mem_mark >= mem_step) {
                pm_mem_hud(mem);
                mem_mark = now;
                mem_step = (u64)pm_opt.hud_ms * tick_hz / 1000u;
            }
        }
        pm_log_lvtest(frame);
        if (frame % 300 == 0) pm_rom_drop_stat(); /* ROMKEEP: count of ROM reads in a dropped range, per 300 frames */
        if (frame % 60 == 0) pm_heaphi_line(frame); /* MEM-B: heap and pool high-water, per 60 frames (a short scene ends before 300) */
        if (frame % 3600 == 0) {
            pm_mem_log("tick");
            pm_log_d("log writer core %d heartbeat %u dropped %u", pm_log_writer_core(), pm_log_writer_hb(), pm_log_dropped());
        }
        if (PMDL("logstat")) {
            if (!stat_t0) stat_t0 = now;
            stat_ms += frame_ms;
            if (++stat_n == 300) {
                pm_log_stat(stat_ms / 300.f, 300, (float)(now - stat_t0) / (float)tick_hz, sp_n, sp_max);
                sp_n = 0; sp_max = 0.f;
                stat_ms = 0.f;
                stat_n = 0;
                stat_t0 = now;
            }
        }
        { /* CPULIM-UI test aid, HUDSHOT: shotbottom=<first frame>:<count> dumps the bottom screen framebuffer (240x320 RGB565 as the libctru console sets it, rotated) once per frame of the window, as shots/bot_<frame>.ppm in the shots folder */
            static int bw0 = -2, bw1;
            if (bw0 == -2) {
                char bv[24];
                bw0 = -1;
                if (pm_debug_value("shotbottom", bv, sizeof bv) && sscanf(bv, "%d:%d", &bw0, &bw1) == 2) bw1 += bw0;
                else bw0 = -1;
            }
            if (bw0 >= 0 && (int)frame >= bw0 && (int)frame < bw1) {
                char bname[32];
                snprintf(bname, sizeof bname, "bot_%04u", frame);
                pm_gpu_shot_bottom(bname); /* MN0: the writer is pm_gpu_shot_bottom */
                pm_log_evt("shotbottom bot_%04u frame %u", frame, frame); /* CFG-2: test-event line, the until of the panel scenarios (the file alone is not logged) */
            }
        }
        PM_WD_PHASE(10);
        if (frame == 60 && pm_debug_has("c2ownprobe")) { /* CORE2-3a positive control of c2own: one owned entry called from the game side; the c2own line must print */
            int st[8];
            pm_gpu_cap_state(st);
            pm_log_evt("c2ownprobe: pm_gpu_cap_state called from the game side at frame %u", frame);
        }
        {
            char top[48];
            char stats[40] = "";
            /* BOTLAYOUT: HUD row 1 = fps, frame time, late% (or the MAPHUD text for 2 s after a map change). Row 4 = the stats string (stats option only).
             * Footer row 28 = the map, row 29 = apt (the CPU limit the game set, ? when the last APT call failed), 3DS model, build. */
            char lim[8], sys[40];
            if (pm_cpu_state < 0) snprintf(lim, sizeof lim, "%d%%?", pm_cpu_game_value); else snprintf(lim, sizeof lim, "%d%%", pm_cpu_game_value);
            if (have_fps) snprintf(top, sizeof(top), "%.0f/%d FPS late%d%% %5.1f ms", fps_draw, pm_interp_on > 0 ? 60 : 30, late_pct, frame_ms);
            else snprintf(top, sizeof(top), "-/- FPS %5.1f ms", frame_ms);
            const char* stat_p = NULL; /* the stats row exists only with the stats option */
            if (pm_opt.dbg_stats || pm_debug_has("stats") || PMDL("fast")) { snprintf(stats, sizeof stats, "%s", pm_gbi_stats()); stat_p = stats; }
            snprintf(sys, sizeof sys, "apt: %s %s #%s", lim, pm_new3ds ? "N3DS" : "O3DS", PM_BUILD_STR);
            { /* MAPHUD: 2 s of steps after a map change. The step runs with the HUD off too (the map info line and the footer map). */
                char mapb[3][40]; /* MAPHUD: the map block, HUD rows 4 to 6: the change text for 2 s, else the current map */
                pm_map_hud_step(frame, 2u * (pm_interp_on > 0 ? 60u : 30u), mapb);
                if (pm_flash_warn) snprintf(sys, sizeof sys, "Save not written to SD (retrying)"); /* SAVESAFE: a failed save write shows here until the retry works */
                pm_gpu_footer(pm_map_now(), sys);
                if (pm_opt.dbg_hud) { /* PI5: the HUD update is timed as hud us */
                    unsigned h0 = pm_ph_on ? pm_ticks() : 0;
                    char pos[40]; /* POSHUD: Mario's row, every frame (not the hudint refresh) */
                    if (pm_opt.pos_hud || pos_dbg) pm_pos_text(pos);
                    pm_gpu_hud(top, stat_p, mem, (pm_opt.pos_hud || pos_dbg) ? pos : NULL, mapb);
                    if (pm_ph_on) pm_ph_hud_us += (pm_ticks() - h0) / 268u;
                }
            }
        }
    }
    pm_lg_stop(); /* CORE2C-3: the logic thread ends at a step boundary, the queue is run */
}

void pm_log_prof_apply(int boot_line); /* PROF4, pm_log.c */
void pm_options_prof_boot(void); /* PROFROW, pm_options.c */

/* BOOT-1: boot timing. pm_boot_mark is cold (no log call: the log is not running yet at the first marks). The line is printed
 * once, at the first title-screen frame, from the stored ticks. All values are ms after main() except the ROM read (ms, MB/s). */
static u64 pmb_tick[PMB_N];
static unsigned pmb_set, pmb_arg[PMB_N], pmb_rc[PMB_N], pmb_dc[PMB_N];
static unsigned pmb_au[4]; /* BOOT2 auread at the first draw: n bytes max chunks */
__attribute__((cold, noinline)) void pm_boot_mark(int id, unsigned arg) {
    if (id < 0 || id >= PMB_N || (pmb_set >> id & 1u)) return;
    pmb_tick[id] = svcGetSystemTick();
    pmb_arg[id] = arg;
    pmb_rc[id] = pm_rom_order_n();
    pmb_dc[id] = pm_rom_direct_n();
    if (id == PMB_DRAW) pm_rs_auread_get(&pmb_au[0], &pmb_au[1], &pmb_au[2], &pmb_au[3]);
    pmb_set |= 1u << id;
}
static unsigned pmb_ms(int id) { return (unsigned)((pmb_tick[id] - pmb_tick[PMB_MAIN]) / (SYSCLOCK_ARM11 / 1000ull)); }
static void pm_boot_report(void);
void pm_boot_title_check(void) {
    if (pmb_set >> PMB_TITLE & 1u) return;
    int m = pm_game_mode_now(); /* first step after the logos: GAME_MODE_TITLE_SCREEN (2, src/game_modes.h) or the intro (16); a no-warp boot skips the title */
    if (m == 2 || m == 16) { pm_boot_mark(PMB_TITLE, 0); pm_boot_report(); }
}
/* rom: ms from the ROM open to the stream start (header check, allocation). MB/s: the stream rate (0 while it runs). rdone: ms after
 * main when the whole image was in RAM (0: not yet at the title). waits, wms: accesses that blocked on a missing chunk and their ms. */
static void pm_boot_report(void) {
    unsigned rom_ms = (unsigned)((pmb_tick[PMB_ROM_READ] - pmb_tick[PMB_ROM_OPEN]) / (SYSCLOCK_ARM11 / 1000ull));
    unsigned waits, wms, demand;
    u64 done = pm_rom_stream_info(&waits, &wms, &demand);
    unsigned sms = done ? (unsigned)((done - pmb_tick[PMB_ROM_READ]) / (SYSCLOCK_ARM11 / 1000ull)) : 0u;
    unsigned rdone = done ? (unsigned)((done - pmb_tick[PMB_MAIN]) / (SYSCLOCK_ARM11 / 1000ull)) : 0u;
    float mbs = sms ? (float)pmb_arg[PMB_ROM_READ] / 1048576.f / ((float)sms / 1000.f) : 0.f;
    pm_log_w("boot ms main %u log %u opt %u rom %u (%.1f MB/s) gfx %u audio %u first %u title %u rdone %u waits %u wms %u",
             pmb_ms(PMB_THREAD), pmb_ms(PMB_LOG), pmb_ms(PMB_OPT), rom_ms, mbs, pmb_ms(PMB_GFX), pmb_ms(PMB_AUDIO), pmb_ms(PMB_DRAW), pmb_ms(PMB_TITLE),
             rdone, waits, wms);
    if (pmb_set >> PMB_DRAW & 1u) pm_log_w("auread n %u bytes %u max %u chunks %u", pmb_au[0], pmb_au[1], pmb_au[2], pmb_au[3]); /* BOOT2: at the first draw */
    if (pmb_rc[PMB_TITLE]) pm_log_w("romorder chunks first %u title %u of %u direct first %u title %u", pmb_rc[PMB_DRAW], pmb_rc[PMB_TITLE], 320u, pmb_dc[PMB_DRAW], pmb_dc[PMB_TITLE]); /* PM_ROM_FULL >> 17 */
}

static void game_thread(void* arg) {
    (void)arg;
    pm_boot_mark(PMB_THREAD, 0);
    pm_stack_register(0, "game", 256 * 1024);
    char sw[256]; /* H06d: a bigger array here stops the boot (FX3a, H06d: _svfprintf_r PC 0), so the limit stays and a cut is handled */
    memset(sw, 0, sizeof(sw));
    /* CFG-1: the launch arguments come first, then debug3ds.txt. Every switch read takes the first match (pm_debug_value, debug_scan),
     * so a push flag wins over the same word in debug3ds.txt. */
    if (args[0]) strncpy(sw, args, sizeof(sw) - 1);
    size_t base = strlen(sw);
    int sep = base && base + 1 < sizeof(sw);
    if (sep) sw[base++] = ' ';
    int sw_cut = 0;
    FILE* f = fopen("debug3ds.txt", "r");
    if (f) {
        size_t cap = sizeof(sw) - 1 - base;
        size_t got = fread(sw + base, 1, cap, f);
        sw[base + got] = 0;
        sw_cut = got == cap && fgetc(f) != EOF;
        fclose(f);
        if (sw_cut) { /* H06d: never keep half a word (tour=...iwa_01 became iwa_0): cut back to the last space of the file text */
            char* sp = strrchr(sw + base, ' ');
            if (sp) *sp = 0;
        }
    }
    if (sep && !sw[base]) sw[base - 1] = 0; /* no file text: drop the separator */
    /* MEM-B item 9: the live log (socket, 17492) is opt-in: the livelog word from debug3ds.txt or a launch argument (push_3ds.sh passes it).
     * A netload alone no longer starts it. Read after the args are appended, so the argument word counts. */
    int live = strstr(sw, "livelog") != NULL;
    pm_log_set_switches(sw);
    pm_log_init(live);
    pm_log_start();
    pm_boot_mark(PMB_LOG, 0);
    pm_log_live_status();
    pm_log_bench();
    pm_wd_start();
    pm_fpscr_setup("game");
    bool is_new = false;
    APT_CheckNew3DS(&is_new);
    pm_new3ds = is_new;
    if (is_new) osSetSpeedupEnable(true);
    unsigned ram = (unsigned)(osGetMemRegionSize(MEMREGION_APPLICATION) >> 20);
    pm_log_w("Paper Mario 3DS");
    pm_log_d("threads: game on core %d (svcGetProcessorID), log writer on core %d", (int)svcGetProcessorID(), pm_log_writer_core());
    pm_log_w("build %s git %s loglevel %d", PM_BUILD_STR, PM_GIT_HASH, pm_log_level());
    pm_log_w("MODE = %s / 64MB cap (console region %uMB)", is_new ? "N3DS" : "O3DS", ram);
    if (sw[0] && sw[0] != '\n') pm_log("switches %s", sw);
    if (sw_cut) pm_log_e("switches: debug3ds.txt is longer than 255 bytes. The last word and the rest are cut. Use tour=@file for a long map list.");
    pm_options_init();
    pm_boot_mark(PMB_OPT, 0);
    pm_log_prof_apply(1); /* PROF4: re-applies after pm_options_init set the levels, and prints the boot line */
    pm_ph_init(); /* after the log gate: phase, prof, pmu and fsplit read the final pm_log_file() */
    pm_options_prof_boot(); /* PROFROW: the launch word or the saved Profiling row */
    if (!pm_rom_init()) return; /* the bottom screen showed the reason */
    pm_mem_log("boot");
    pm_zbuf_reserve(); /* PAUSEC: the pause picture, while the heap is whole (after the ROM, so the ROM check is unchanged) */
    pm_game_bind();
    pm_log("boot_main");
    boot_main(NULL);
    pm_log("boot_main returned");
}

/* HW7: libctru exit frees the heap, and the GSP event thread (stack in that heap) still runs unless gfxExit stops it. Dump 37 on the
 * O3DS was that crash at HOME close. Reverse order of the init: flash, audio job, ndsp, log writer, citro3d, gfx, soc. One call (guard).
 * Called after the game thread ended, that is after aptMainLoop returned false (also for the APT exit request from HOME). */

static void wq_fini(void* a) { (void)a; C3D_Fini(); }
static void pm_shutdown(void) {
    static int done;
    if (done) return;
    done = 1;
    pm_rom_stream_stop(); /* BOOT: the loader ends before the services it reads through go away */
    if (pm_crash_exit) pm_flash_crash_flush(); else pm_flash_flush("exit");
    pm_audio_join_at(0);
    pm_cpu_limit_exit(); /* CPULIM-UI: core 1 limit back to 30 */
    pm_log("exit: audio job done, saving flushed, shutting down ndsp, citro3d, gfx, soc");
    pm_audio_shutdown();
    if (pm_crash_exit) pm_log_crash_close(); else pm_log_stop(); /* CRASHEXIT: the crash flush holds the drain lock */ /* write what is left, then stop the writer thread. The last log line. */
    pm_wq_call(wq_fini, NULL, 0); /* CORE2-3a: the walker owns citro3d */
    gfxExit();
    pm_log_soc_exit();
}

int main(int argc, char** argv) {
    pm_boot_mark(PMB_MAIN, 0); /* BOOT-1 */
    pm_thread_adopt("main");
    args[0] = 0;
    for (int i = 1; i < argc; i++) {
        strncat(args, argv[i], sizeof(args) - strlen(args) - 2);
        strncat(args, " ", sizeof(args) - strlen(args) - 1);
    }
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/PaperMario", 0777);
    chdir("sdmc:/3ds/PaperMario");
    /* HC4f2: romfs:/shape/<name>.nds (build.sh gen_shapes, the image of $OUT/romfs) is read by the shape loader in pm_map.c. */
    { Result rfs = romfsInit(); if (rfs) pm_log_evt("romfs init fail %08lx", (unsigned long)rfs); }
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    Thread t = pm_thread_create(game_thread, NULL, 256 * 1024, prio, 0, "game");
    if (!t) return 1;
    threadJoin(t, U64_MAX);
    threadFree(t);
    pm_shutdown();
    return 0;
}
