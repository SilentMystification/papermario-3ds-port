#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "pm_port.h"
#include "pm_gpu.h"
#include "pm_gbi.h"

extern void boot_main(void* data);

static u8 crash_stack[0x4000] __attribute__((aligned(8)));
static int netloaded;
static char args[256];

static void crash_handler(ERRF_ExceptionInfo* excep, CpuRegisters* regs) {
    static const char* names[] = { "prefetch abort", "data abort", "undefined instruction", "VFP" };
    char buf[160];
    int n = snprintf(buf, sizeof(buf),
                     "\n[CRASH] %s thread=%lx pc=%08lx lr=%08lx sp=%08lx far=%08lx\n",
                     excep->type < 4 ? names[excep->type] : "?",
                     (unsigned long)threadGetCurrent(),
                     (unsigned long)regs->pc, (unsigned long)regs->lr,
                     (unsigned long)regs->sp, (unsigned long)excep->far);
    pm_crash_line(buf, n);
    const u32* sp = (const u32*)(regs->sp & ~3u);
    n = snprintf(buf, sizeof(buf), "[CRASH] stack");
    for (int i = 0, found = 0; i < 1024 && found < 48; i++) {
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
    svcBreak(USERBREAK_PANIC);
    for (;;) {}
}

unsigned pm_ticks(void) { return (unsigned)svcGetSystemTick(); }

/* gfxRetrace_Callback flips D_80073E0A and builds a display list only when it
 * lands on 0. The other retrace leaves the N64 framebuffer alone. Clearing
 * and presenting on that retrace flashes black between the logo frames. */
extern s16 D_80073E0A;
/* The intro camera waits until the displayed color buffer changes, which NuSys
 * does on every swap. Two tokens stand in for that pair of framebuffers. */
extern unsigned short* nuGfxCfb_ptr;
static unsigned short cfb_token[2];

void pm_frame_loop(void) {
    unsigned frame = 0;
    int ever = 0;
    while (aptMainLoop()) {
        pm_log_poll();
        pm_pad_scan();
        int draw = D_80073E0A != 0 || !ever;
        nuGfxCfb_ptr = &cfb_token[frame & 1u];
        pm_gpu_begin(draw);
        pm_gfx_retrace();
        if (!pm_gpu_drew()) {
            if (!ever) pm_gbi_fallback();
        } else {
            ever = 1;
        }
        pm_gpu_end();
        if ((++frame % 60u) == 0) pm_log("frame %u", frame);
    }
}

static void game_thread(void* arg) {
    (void)arg;
    threadOnException(crash_handler, crash_stack + sizeof(crash_stack), WRITE_DATA_TO_HANDLER_STACK);
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
    bool is_new = false;
    APT_CheckNew3DS(&is_new);
    if (is_new) osSetSpeedupEnable(true);
    unsigned ram = (unsigned)(osGetMemRegionSize(MEMREGION_APPLICATION) >> 20);
    pm_log("Paper Mario 3DS");
    pm_log("MODE = %s / 64MB cap (console region %uMB)", is_new ? "N3DS" : "O3DS", ram);
    if (sw[0] && sw[0] != '\n') pm_log("switches %s", sw);
    pm_rom_init();
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
    return 0;
}
