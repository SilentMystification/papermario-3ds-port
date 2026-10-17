#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "pm_port.h"
#include "pm_gpu.h"
#include "pm_gbi.h"
#include "pm_build_id.h"

extern void boot_main(void* data);

static u8 crash_stack[0x4000] __attribute__((aligned(8)));
static int netloaded;
static char args[256];

/* Stack records: bounds of the game and audio thread stacks, a fill pattern for the high-water mark.
 * A thread registers itself with the address of a local at its entry. The fill stops below the live frames. */
#define STACK_PAT 0xA5A5A5A5u
typedef struct { u32 lo, hi; const char* name; } StackRec;
static StackRec recs[2];

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
    pm_log("fpscr %s before %08x after %08x", who, before, fpscr_get());
}

static void crash_handler(ERRF_ExceptionInfo* excep, CpuRegisters* regs) {
    static const char* names[] = { "prefetch abort", "data abort", "undefined instruction", "VFP" };
    char buf[200];
    int n;
    const StackRec* st = NULL;
    for (int i = 0; i < 2; i++)
        if (recs[i].hi && regs->sp >= recs[i].lo && regs->sp <= recs[i].hi + 512) st = &recs[i];
    n = snprintf(buf, sizeof(buf),
                 "\n[CRASH] %s thread=%s (%lx) pc=%08lx lr=%08lx sp=%08lx far=%08lx fsr=%08lx cpsr=%08lx build #%d\n",
                 excep->type < 4 ? names[excep->type] : "?", st ? st->name : "other",
                 (unsigned long)threadGetCurrent(), (unsigned long)regs->pc, (unsigned long)regs->lr,
                 (unsigned long)regs->sp, (unsigned long)excep->far, (unsigned long)excep->fsr,
                 (unsigned long)regs->cpsr, PM_BUILD);
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
    svcBreak(USERBREAK_PANIC);
    for (;;) {}
}

/* R01: stop the program on purpose from a debug check (switch romstrict). Azahar never calls the guest handler on a real
 * fault, so a null write would lose the queued log lines. pm_crash_line flushes the queue, writes crash.txt, and the
 * [CRASH] prefix makes run_azahar.ps1 stop. svcBreak ends the process like the crash handler does. */
void pm_fatal_stop(const char* msg) {
    char buf[200];
    pm_log("FATAL: %s", msg);
    int n = snprintf(buf, sizeof(buf), "\n[CRASH] debug check: %s build #%d\n", msg, PM_BUILD);
    pm_crash_line(buf, n);
    svcBreak(USERBREAK_PANIC);
    for (;;) {}
}

/* Class E test: Azahar stops on a real CPU exception itself and never calls the guest handler, so the handler has a direct
 * entry. It gets made-up registers and the live sp. On hardware use the switches crashtest and crashtest_audio instead (real fault). */
void pm_crash_selftest(void) {
    ERRF_ExceptionInfo e;
    CpuRegisters r;
    memset(&e, 0, sizeof(e));
    memset(&r, 0, sizeof(r));
    e.type = ERRF_EXCEPTION_DATA_ABORT;
    e.fsr = 0x805;
    e.far = 4;
    for (int i = 0; i < 13; i++) r.r[i] = 0x1000u + (u32)i;
    r.sp = (u32)(uintptr_t)&e;
    r.lr = (u32)(uintptr_t)__builtin_return_address(0);
    r.pc = (u32)(uintptr_t)&pm_crash_selftest;
    r.cpsr = 0x60000010u;
    crash_handler(&e, &r);
}

void pm_crash_install(void* stack, unsigned size) {
    threadOnException(crash_handler, (u8*)stack + size, WRITE_DATA_TO_HANDLER_STACK);
}

unsigned pm_ticks(void) { return (unsigned)svcGetSystemTick(); }
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
/* A color image that is the screen (a frame buffer). Any other color image is memory the game reads back. */
int pm_is_cfb(const void* p) { return in_buf(p, gFrameBuf0) || in_buf(p, gFrameBuf1) || in_buf(p, gFrameBuf2); }
/* Frame buffer and saved-frame memory hold native u16 pixels (the game code reads them as u16). Textures
 * from ROM data hold big-endian bytes. pm_tex reads these two kinds differently. */
int pm_fb_native(const void* p) { return pm_is_cfb(p) || (nuGfxZBuffer && in_buf(p, nuGfxZBuffer)); }
/* D03: the z-buffer memory the pause code saves the last frame into. Allocated when first needed. */
void pm_zbuf_need(void) {
    if (!nuGfxZBuffer) {
        nuGfxZBuffer = (unsigned short*)malloc(CFB_PIXELS * 2);
        if (!nuGfxZBuffer) pm_log("D03: no memory for the %d KB z-buffer image", CFB_PIXELS * 2 / 1024);
        else pm_log("D03: z-buffer image allocated, %d KB", CFB_PIXELS * 2 / 1024);
    }
    if (nuGfxZBuffer) pm_tex_invalidate(nuGfxZBuffer, CFB_PIXELS * 2);
}

void pm_dump_scripts(void);
void pm_warp_tick(unsigned frame);
void pm_map_tick(void);
unsigned pm_uptime_ms(void);
static unsigned pm_frame_n;
unsigned pm_frame_count(void) { return pm_frame_n; }
/* D03: copy the last finished frame into the frame buffer that is not being drawn (the N64 "previous
 * color buffer"). The PICA color buffer holds that frame until C3D_FrameEnd of this frame runs the
 * command list, so any time during this frame's display list build or run is right. Once per frame. */
void pm_cfb_capture_prev(void) {
    static unsigned done = ~0u;
    unsigned i;
    if (done == pm_frame_n) return;
    done = pm_frame_n;
    for (i = 0; i < nuGfxCfbNum; i++) {
        if (nuGfxCfb[i] == nuGfxCfb_ptr) {
            unsigned short* prev = nuGfxCfb[(i + nuGfxCfbNum - 1) % nuGfxCfbNum];
            unsigned t0 = pm_ticks();
            static unsigned nlog;
            pm_gpu_read_frame(prev);
            if (nlog++ < 6) pm_log("D03: last frame copied to a frame buffer in %u us (ARM11 tick, Azahar timing)", (unsigned)((pm_ticks() - t0) / 268u));
            pm_tex_invalidate(prev, CFB_PIXELS * 2);
            return;
        }
    }
}
void pm_frame_loop(void) {
    unsigned frame = 0;
    pm_uptime_ms(); /* start the clock */
    nuGfxCfb = cfb_list;
    unsigned presents = 0;
    int ever = 0;
    int have_fps = 0;
    u64 step_start = 0;
    u64 fps_mark = 0;
    float fps = 0.f;
    float frame_ms = 0.f;
    const u64 tick_hz = SYSCLOCK_ARM11;
    float stat_ms = 0.f;
    unsigned stat_n = 0;
    u64 stat_t0 = 0;
    while (aptMainLoop()) {
        u64 now = svcGetSystemTick();
        /* The retrace builds a display list only every other call. Presenting
         * the idle call shows the other framebuffer, which was never drawn. */
        int draw = D_80073E0A != 0 || !ever;
        pm_log_poll();
        pm_pad_scan();
        if (!pm_debug_has("noaudio")) {
            PH_PUSH(PH_AUDIO);
            for (unsigned n = pm_audio_retraces(); n; n--) { pm_audio_tick(); PH_ADD(C_ATICK, 1); }
            PH_POP();
        }
        if (!draw) {
            pm_gfx_retrace();
            /* Pad to 30Hz when the drawn frame was quick. A slow frame already
             * missed its vblank, so waiting again only adds another stall. */
            if (!pm_debug_has("fast") && tick_hz && now - step_start < (tick_hz / 60ull) * 2ull) {
                PH_PUSH(PH_IDLE);
                gspWaitForVBlank();
                PH_POP();
            }
            continue;
        }
        step_start = now;
        nuGfxCfb_ptr = nuGfxCfb[frame % nuGfxCfbNum];
        pm_gpu_begin(1);
        pm_gfx_retrace();
        if (!pm_gpu_drew()) {
            if (!ever) pm_gbi_fallback();
        } else {
            ever = 1;
        }
        pm_gpu_end();
        pm_ph_frame_end();
        frame++;
        pm_frame_n = frame;
        pm_warp_tick(frame);
        pm_map_tick();
        presents++;
        now = svcGetSystemTick();
        frame_ms = (float)(now - step_start) * 1000.f / (float)tick_hz;
        if (!fps_mark) fps_mark = now;
        if (now - fps_mark >= tick_hz) {
            fps = (float)presents * (float)tick_hz / (float)(now - fps_mark);
            presents = 0;
            fps_mark = now;
            have_fps = 1;
        }
        if (frame % 3600 == 0) {
            pm_mem_log("tick");
            pm_log("log writer core %d heartbeat %u dropped %u", pm_log_writer_core(), pm_log_writer_hb(), pm_log_dropped());
        }
        if (pm_debug_has("logflood")) for (int i = 0; i < 10000; i++) pm_log("flood %d of frame %u", i, frame);
        if (pm_debug_has("logstat")) {
            if (!stat_t0) stat_t0 = now;
            stat_ms += frame_ms;
            if (++stat_n == 300) {
                pm_log_stat(stat_ms / 300.f, 300, (float)(now - stat_t0) / (float)tick_hz);
                if (frame % 1200 == 0) pm_log_top();
                stat_ms = 0.f;
                stat_n = 0;
                stat_t0 = now;
            }
        }
        if (frame == 300 && pm_debug_has("crashtest_call")) pm_crash_selftest();
        if (frame == 300 && pm_debug_has("crashtest")) __asm__ volatile(".word 0xe7f000f0"); /* class E: deliberate undefined instruction on the game thread */
        if (pm_debug_has("scripts") && frame % 120 == 0) pm_dump_scripts();
        {
            char top[40];
            char bot[40];
            if (have_fps) snprintf(top, sizeof(top), "FPS %4.1f  #%d", fps, PM_BUILD);
            else snprintf(top, sizeof(top), "FPS ...  #%d", PM_BUILD);
            if (pm_debug_has("stats") || pm_debug_has("fast"))
                snprintf(bot, sizeof(bot), "%5.1f %s", frame_ms, pm_gbi_stats());
            else
                snprintf(bot, sizeof(bot), "%5.1f ms", frame_ms);
            pm_gpu_hud(top, bot);
        }
    }
}

static void game_thread(void* arg) {
    (void)arg;
    pm_crash_install(crash_stack, sizeof(crash_stack));
    pm_stack_register(0, "game", 256 * 1024);
    char sw[256];
    memset(sw, 0, sizeof(sw));
    FILE* f = fopen("debug3ds.txt", "r");
    if (f) {
        fread(sw, 1, sizeof(sw) - 1, f);
        fclose(f);
    }
    int live = netloaded || strstr(sw, "livelog") != NULL;
    if (args[0]) {
        size_t n = strlen(sw);
        if (n && n + 1 < sizeof(sw)) sw[n++] = ' ';
        strncat(sw, args, sizeof(sw) - strlen(sw) - 1);
    }
    pm_log_set_switches(sw);
    pm_log_init(live);
    pm_log_start();
    pm_ph_init();
    pm_fpscr_setup("game");
    bool is_new = false;
    APT_CheckNew3DS(&is_new);
    if (is_new) osSetSpeedupEnable(true);
    unsigned ram = (unsigned)(osGetMemRegionSize(MEMREGION_APPLICATION) >> 20);
    pm_log("Paper Mario 3DS");
    pm_log("threads: game on core %d (svcGetProcessorID), log writer on core %d", (int)svcGetProcessorID(), pm_log_writer_core());
    pm_log("MODE = %s / 64MB cap (console region %uMB)", is_new ? "N3DS" : "O3DS", ram);
    if (sw[0] && sw[0] != '\n') pm_log("switches %s", sw);
    if (!pm_rom_init()) return; /* the bottom screen showed the reason */
    pm_mem_log("boot");
    pm_game_bind();
    pm_log("boot_main");
    boot_main(NULL);
    pm_log("boot_main returned");
}

int main(int argc, char** argv) {
    netloaded = __3dslink_host.s_addr != 0 || (argc > 0 && argv[0] && strncmp(argv[0], "3dslink:", 8) == 0);
    args[0] = 0;
    for (int i = 1; i < argc; i++) {
        strncat(args, argv[i], sizeof(args) - strlen(args) - 2);
        strncat(args, " ", sizeof(args) - strlen(args) - 1);
    }
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/PaperMario", 0777);
    chdir("sdmc:/3ds/PaperMario");
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    Thread t = threadCreate(game_thread, NULL, 256 * 1024, prio, 0, false);
    if (!t) return 1;
    threadJoin(t, U64_MAX);
    threadFree(t);
    pm_log_stop(); /* write what is left, then stop the writer thread */
    return 0;
}
