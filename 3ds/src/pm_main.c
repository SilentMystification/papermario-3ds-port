#include "pm_build.h"
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "pm_port.h"
/* O6: a switch word read once. Not for the words the options panel changes at run time (nofog, nolight). */
#define PMD(word) ({ static int v_ = -1; if (v_ < 0) v_ = pm_debug_has(word); v_; })
#include "pm_gpu.h"
#include "pm_gbi.h"


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
    pm_log_d("fpscr %s before %08x after %08x", who, before, fpscr_get());
}

static void crash_handler(ERRF_ExceptionInfo* excep, CpuRegisters* regs) {
    static const char* names[] = { "prefetch abort", "data abort", "undefined instruction", "VFP" };
    char buf[200];
    int n;
    const StackRec* st = NULL;
    for (int i = 0; i < 2; i++)
        if (recs[i].hi && regs->sp >= recs[i].lo && regs->sp <= recs[i].hi + 512) st = &recs[i];
    n = snprintf(buf, sizeof(buf),
                 "\n[CRASH] %s thread=%s (%lx) pc=%08lx lr=%08lx sp=%08lx far=%08lx fsr=%08lx cpsr=%08lx build #%s\n",
                 excep->type < 4 ? names[excep->type] : "?", st ? st->name : "other",
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
    svcBreak(USERBREAK_PANIC);
    for (;;) {}
}

/* R01: stop the program on purpose from a debug check (switch romstrict). Azahar never calls the guest handler on a real
 * fault, so a null write would lose the queued log lines. pm_crash_line flushes the queue, writes crash.txt, and the
 * [CRASH] prefix makes run_azahar.ps1 stop. svcBreak ends the process like the crash handler does. */
void pm_fatal_stop(const char* msg) {
    char buf[200];
    pm_log_e("FATAL: %s", msg);
    int n = snprintf(buf, sizeof(buf), "\n[CRASH] debug check: %s build #%s\n", msg, PM_BUILD_STR);
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
/* B2: the frame buffer the game draws into now (the T2 grab source). Any other frame buffer holds an older frame (T3). */
extern int pm_wide;
/* F07-7: with wide a window may start up to 40 pixels before the buffer (game x -40 of row 0, effects in the gutters) */
int pm_is_cur_cfb(const void* p) { return nuGfxCfb_ptr && (in_buf(p, nuGfxCfb_ptr) || (pm_wide && in_buf((const u8*)p + 80, nuGfxCfb_ptr))); }
/* Frame buffer and saved-frame memory hold native u16 pixels (the game code reads them as u16). Textures
 * from ROM data hold big-endian bytes. pm_tex reads these two kinds differently. */
int pm_fb_native(const void* p) { return pm_is_cfb(p) || (nuGfxZBuffer && in_buf(p, nuGfxZBuffer)); }
/* D03: the z-buffer memory the pause code saves the last frame into. Allocated when first needed. */
void pm_zbuf_need(void) {
    if (!nuGfxZBuffer) {
        nuGfxZBuffer = (unsigned short*)malloc(CFB_PIXELS * 2);
        if (!nuGfxZBuffer) pm_log_e("D03: no memory for the %d KB z-buffer image", CFB_PIXELS * 2 / 1024);
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
static unsigned pm_frame_n;
unsigned pm_frame_count(void) { return pm_frame_n; }
/* D03: copy the last finished frame into the frame buffer that is not being drawn (the N64 "previous
 * color buffer"). The PICA color buffer holds that frame until C3D_FrameEnd of this frame runs the
 * command list, so any time during this frame's display list build or run is right. Once per frame. */
PmB7 pm_b7;
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
            { unsigned dt = pm_ticks() - t0; pm_b7.cap_n++; pm_b7.cap_ticks += dt; if (dt > pm_b7.cap_max) pm_b7.cap_max = dt; }
            if (nlog++ < 6) pm_log("D03: last frame copied to a frame buffer in %u us (ARM11 tick, Azahar timing)", (unsigned)((pm_ticks() - t0) / 268u));
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
void pm_audio_step(void) {
    if (au_kicked) return;
    au_kicked = 1;
    if (PMD("noaudio")) return;
    unsigned n = pm_audio_lock() ? 2u : pm_audio_retraces();
    PH_ADD(C_ATICK, n);
    pace_ticks += n;
    pm_audio_kick(n); /* AU1d: runs on core 1 until the next join */
}
static unsigned pace_iv[4]; /* vblanks between two drawn iterations: 1, 2, 3, 4 or more */
/* PC01: vblanks per game step. 2 = 30 fps. A later 60 fps option (F06) changes only this value. Needs more work
 * than a changed number for 1: FrameBegin(SYNCDRAW) then has to stop waiting for the vblank (not done, not in scope). */
#define PM_VBLANKS_PER_STEP 2
static void pace_stat(u64 now, u64 hz, unsigned frame) {
    static u64 t0;
    static unsigned s0, f0, v0, a0, i0[4];
    unsigned v = C3D_FrameCounter(0);
    if (!t0) { t0 = now; s0 = pm_step_calls; f0 = frame; v0 = v; a0 = pace_ticks; for (int i = 0; i < 4; i++) i0[i] = pace_iv[i]; return; }
    if (now - t0 < hz) return;
    float wall = (float)(now - t0) * 1000.f / (float)hz;
    pm_log_d("pace: wall %.1f ms steps %u drawn %u vblanks %u audio ticks %u  (steps/s %.2f) intervals 1:%u 2:%u 3:%u 4+:%u total 1:%u 2:%u 3:%u 4+:%u late steps (3+, over 33.3 ms) %u of %u", wall, pm_step_calls - s0,
           frame - f0, v - v0, pace_ticks - a0, (float)(pm_step_calls - s0) * 1000.f / wall, pace_iv[0] - i0[0],
           pace_iv[1] - i0[1], pace_iv[2] - i0[2], pace_iv[3] - i0[3], pace_iv[0], pace_iv[1], pace_iv[2], pace_iv[3], pace_iv[2] + pace_iv[3], pace_iv[0] + pace_iv[1] + pace_iv[2] + pace_iv[3]);
    for (int i = 0; i < 4; i++) i0[i] = pace_iv[i];
    t0 = now; s0 = pm_step_calls; f0 = frame; v0 = v; a0 = pace_ticks;
}
int pm_interp_key_draw(void);
void pm_frame_loop(void) {
    unsigned frame = 0;
    pm_uptime_ms(); /* start the clock */
    { extern unsigned gRandSeed; pm_log("rng start seed %08x%s", gRandSeed, pm_debug_has("fixedseed") ? " (fixedseed)" : ""); } /* R-SEED */
    nuGfxCfb = cfb_list;
    unsigned presents = 0;
    int ever = 0;
    int have_fps = 0;
    u64 step_start = 0;
    unsigned step_vb = 0; /* vblank counter when the last drawn iteration got past FrameBegin */
    int step_vb_ok = 0;
    u64 fps_mark = 0;
    float fps = 0.f;
    float frame_ms = 0.f;
    const u64 tick_hz = SYSCLOCK_ARM11;
    float stat_ms = 0.f;
    unsigned stat_n = 0;
    u64 stat_t0 = 0;
    const int pace_on = pm_debug_has("pacestat");
    while (aptMainLoop()) {
        u64 now = svcGetSystemTick();
        /* The retrace builds a display list only every other call. Presenting
         * the idle call shows the other framebuffer, which was never drawn. */
        int draw = D_80073E0A != 0 || !ever;
        pm_log_poll();
        pm_pad_scan();
        if (pace_on) pace_stat(now, tick_hz, frame);
        if (!draw) {
            pm_gfx_retrace();
            /* F06-2: key frame of the last step, drawn and presented here (FrameBegin waits for the vblank) */
            if (pm_interp_on > 0 && pm_interp_key_draw()) {
                presents++;
                continue;
            }
            /* PC01: keep PM_VBLANKS_PER_STEP vblanks between game steps. The counter is citro3d's own vblank
             * counter. The next drawn iteration waits for one more vblank in C3D_FrameBegin(SYNCDRAW), so this
             * waits until the counter is PM_VBLANKS_PER_STEP - 1 past the last drawn iteration. A slow frame is
             * already past it: no wait and no extra step to catch up. `fast` stays uncapped (debug). */
            if (!PMD("fast") && step_vb_ok) {
                int guard = PM_VBLANKS_PER_STEP + 1;
                PH_PUSH(PH_IDLE);
                while ((int)(C3D_FrameCounter(0) - step_vb) < PM_VBLANKS_PER_STEP - 1 && guard--) gspWaitForVBlank();
                PH_POP();
            }
            continue;
        }
        step_start = now;
        nuGfxCfb_ptr = nuGfxCfb[frame % nuGfxCfbNum];
        pm_gpu_begin(1);
        {
            unsigned vb = C3D_FrameCounter(0);
            if (step_vb_ok) {
                unsigned iv = vb - step_vb;
                pace_iv[iv < 1 ? 0 : iv > 4 ? 3 : iv - 1]++;
                pm_interp_skip = iv > PM_VBLANKS_PER_STEP; /* F06-5 */
            }
            step_vb = vb;
            step_vb_ok = 1;
        }
        au_kicked = 0;
        pm_gfx_retrace();
        pm_audio_step(); /* AU1b: no main task in this step: run the ticks now */
        if (!pm_gpu_drew()) {
            if (!ever) pm_gbi_fallback();
        } else {
            ever = 1;
        }
        pm_gpu_end();
        pm_ph_frame_end();
        frame++;
        pm_frame_n = frame;
        pm_audio_join_at(3); /* AU1d: warp and fxtest can call game code */
        pm_warp_tick(frame);
        { extern void pm_fxtest_tick(unsigned); pm_fxtest_tick(frame); } /* FX1a, switch fxtest */
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
            pm_log_d("log writer core %d heartbeat %u dropped %u", pm_log_writer_core(), pm_log_writer_hb(), pm_log_dropped());
        }
        if (pm_debug_has("logflood")) for (int i = 0; i < 10000; i++) pm_log_d("flood %d of frame %u", i, frame);
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
        if (pm_debug_has("shotbottom") && frame == 400) { /* CPULIM-UI test aid: dump the bottom screen framebuffer (240x320 RGB565 as the libctru console sets it, rotated) as shotbottom.ppm */
            u16 bw, bh;
            u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &bw, &bh);
            FILE* sf = fopen("shotbottom.ppm", "wb");
            if (sf && fb) {
                fprintf(sf, "P6\n%u %u\n255\n", (unsigned)bh, (unsigned)bw);
                for (unsigned y = 0; y < bw; y++) /* screen row y, screen column x: the buffer is column major, bottom row first */
                    for (unsigned x = 0; x < bh; x++) { const u8* px = fb + ((size_t)x * bw + (bw - 1 - y)) * 2; unsigned c = px[0] | (px[1] << 8); fputc(((c >> 11) & 31) * 255 / 31, sf); fputc(((c >> 5) & 63) * 255 / 63, sf); fputc((c & 31) * 255 / 31, sf); }
                pm_log_w("shotbottom: wrote shotbottom.ppm %ux%u", (unsigned)bh, (unsigned)bw);
            }
            if (sf) fclose(sf);
        }
        if (frame == 300 && pm_debug_has("crashtest_call")) pm_crash_selftest();
        if (frame == 300 && pm_debug_has("crashtest")) __asm__ volatile(".word 0xe7f000f0"); /* class E: deliberate undefined instruction on the game thread */
        if (pm_debug_has("scripts") && frame % 120 == 0) pm_dump_scripts();
        {
            char top[40];
            char bot[40];
            /* CPULIM-UI: line 1 = fps, app core 1 time limit the game set (? when the last APT call failed), 3DS mode, build */
            char lim[8];
            if (pm_cpu_state < 0) snprintf(lim, sizeof lim, "L%d?", pm_cpu_game_value); else snprintf(lim, sizeof lim, "L%d", pm_cpu_game_value);
            if (have_fps) snprintf(top, sizeof(top), "FPS %4.1f %s %s #%s", fps, lim, pm_new3ds ? "N3DS" : "O3DS", PM_BUILD_STR);
            else snprintf(top, sizeof(top), "FPS ... %s %s #%s", lim, pm_new3ds ? "N3DS" : "O3DS", PM_BUILD_STR);
            if (pm_opt.dbg_stats || pm_debug_has("stats") || PMD("fast"))
                snprintf(bot, sizeof(bot), "%5.1f %s", frame_ms, pm_gbi_stats());
            else
                snprintf(bot, sizeof(bot), "%5.1f ms", frame_ms);
            if (pm_opt.dbg_hud) pm_gpu_hud(top, bot);
        }
    }
}

static void game_thread(void* arg) {
    (void)arg;
    pm_crash_install(crash_stack, sizeof(crash_stack));
    pm_stack_register(0, "game", 256 * 1024);
    char sw[256]; /* H06d: a bigger array here stops the boot (FX3a, H06d: _svfprintf_r PC 0), so the limit stays and a cut is handled */
    memset(sw, 0, sizeof(sw));
    FILE* f = fopen("debug3ds.txt", "r");
    int sw_cut = 0;
    if (f) {
        size_t got = fread(sw, 1, sizeof(sw) - 1, f);
        sw_cut = got == sizeof(sw) - 1 && fgetc(f) != EOF;
        fclose(f);
        if (sw_cut) { /* H06d: never keep half a word (tour=...iwa_01 became iwa_0): cut back to the last space */
            char* sp = strrchr(sw, ' ');
            if (sp) *sp = 0;
        }
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
    pm_new3ds = is_new;
    if (is_new) osSetSpeedupEnable(true);
    unsigned ram = (unsigned)(osGetMemRegionSize(MEMREGION_APPLICATION) >> 20);
    pm_log_w("Paper Mario 3DS");
    pm_log_d("threads: game on core %d (svcGetProcessorID), log writer on core %d", (int)svcGetProcessorID(), pm_log_writer_core());
    pm_log_w("build %s", PM_BUILD_STR);
    pm_log_w("MODE = %s / 64MB cap (console region %uMB)", is_new ? "N3DS" : "O3DS", ram);
    if (sw[0] && sw[0] != '\n') pm_log("switches %s", sw);
    if (sw_cut) pm_log_e("switches: debug3ds.txt is longer than 255 bytes. The last word and the rest are cut. Use tour=@file for a long map list.");
    pm_options_init();
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
    pm_cpu_limit_exit(); /* CPULIM-UI: core 1 limit back to 30 */
    pm_log_stop(); /* write what is left, then stop the writer thread */
    return 0;
}
