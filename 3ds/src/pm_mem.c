#include <3ds.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include "pm_port.h"

/* O3DS application FCRAM, all of it. N3DS and Azahar can offer more; this
 * build does not take it.
 *   4MB  N64 base RAM. Left free in the newlib heap for the game.
 *  40MB  Cartridge image, one resident copy.
 *  14MB  This port: linearAlloc pool (GPU command buffer, vertices, texture
 *        scratch, audio) plus the netload socket buffer.
 * H03: the pool is the measured need, not a round number. Azahar, 4 maps: linear used minus the texture cache is a constant
 * 7528KB (vertices, command buffer, audio, scratch, render targets). The texture cache budget is 4096KB. Sum 11624KB. The pool
 * adds a margin of 2.7MB for fragmentation from texture churn. A full pool fails a texture, never a boot (pm_tex.c).
 */
#define PM_FCRAM (64u * 1024u * 1024u)
#define PM_OURS  (14u * 1024u * 1024u)
#define PM_LINEAR_FIXED_KB 4968u /* H03 measured 7528, MEM1 command buffer 4096 KB to 1536 KB: minus 2560 */
#define PM_SOC   0x20000u

extern char* fake_heap_start;
extern char* fake_heap_end;
extern u32 __ctru_heap, __ctru_heap_size;
extern u32 __ctru_linear_heap, __ctru_linear_heap_size;

static u8* soc_buf;
static s64 commit_max, commit_cur; /* M02 measurement: what the OS offered before the 64MB cap */
static u32 soc_size;
static Result soc_rc_alloc = -1;
unsigned pm_soc_alloc_rc(void) { return (unsigned)soc_rc_alloc; }

int pm_started_by_netload(void) {
    const char* p = envGetSystemArgList();
    if (!p) return 0;
    u32 argc = *(const u32*)p;
    const char* arg = p + 4;
    const char* last = NULL;
    for (u32 i = 0; i < argc; i++) {
        if (i == 0 && strncmp(arg, "3dslink:", 8) == 0) return 1;
        last = arg;
        arg += strlen(arg) + 1;
    }
    return last && argc > 1 && strlen(last) == 17 && strncmp(last + 8, "_3DSLINK_", 8) == 0;
}

/* Boot failure before the heap exists: leave a trace on the SD card, then break. The heap, stdio and the log are not up
 * yet, so this opens the SD archive by hand with stack buffers only. Best effort: if the SD is not reachable, only the break happens. */
static void boot_fail(const char* what, u32 a, u32 b, Result rc) {
    char msg[200];
    int n = snprintf(msg, sizeof(msg), "[BOOT FAIL] %s a=%08lx b=%08lx rc=%08lx\n", what, (unsigned long)a, (unsigned long)b, (unsigned long)rc);
    if (R_SUCCEEDED(srvInit())) {
        if (R_SUCCEEDED(fsInit())) {
            Handle fh;
            if (R_SUCCEEDED(FSUSER_OpenFileDirectly(&fh, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                                                    fsMakePath(PATH_ASCII, "/3ds/PaperMario/crash.txt"), FS_OPEN_WRITE | FS_OPEN_CREATE, 0))) {
                u64 size = 0;
                u32 wrote = 0;
                FSFILE_GetSize(fh, &size);
                FSFILE_Write(fh, &wrote, size, msg, (u32)n, FS_WRITE_FLUSH);
                FSFILE_Close(fh);
            }
            fsExit();
        }
        srvExit();
    }
    svcOutputDebugString(msg, n);
    svcBreak(USERBREAK_PANIC);
}

void __system_allocateHeaps(void) {
    Handle reslimit = 0;
    Result rc0 = svcGetResourceLimit(&reslimit, CUR_PROCESS_HANDLE);
    if (R_FAILED(rc0)) boot_fail("svcGetResourceLimit", 0, 0, rc0);
    s64 max_commit = 0, cur_commit = 0;
    ResourceLimitType type = RESLIMIT_COMMIT;
    svcGetResourceLimitLimitValues(&max_commit, reslimit, &type, 1);
    svcGetResourceLimitCurrentValues(&cur_commit, reslimit, &type, 1);
    svcCloseHandle(reslimit);
    commit_max = max_commit;
    commit_cur = cur_commit;

    u32 avail = (u32)(max_commit - cur_commit) & ~0xFFFu;
    if (avail > PM_FCRAM) avail = PM_FCRAM;

    /* HW1: socInit needs a memory block in the normal heap area (0x08000000 and up). The linear block is at 0x14000000 and
     * above, so socInit returns e0e01bf5 (invalid address) with a linear buffer. The old code called MEMOP_ALLOC with
     * address 0, which the kernel refuses. This port keeps its heap in the linear block, so the normal heap area is free:
     * allocate the socket buffer there at 0x08000000. The 128 KB come out of the commit before the linear block, as before.
     * Always reserved, not only after a netload, so the livelog switch works from the SD card too. */
    u32 soc = PM_SOC;
    if (avail <= soc + PM_OURS) boot_fail("commit too small", avail, soc + PM_OURS, 0);
    avail -= soc;
    u32 ours = PM_OURS - soc;

    u32 base = 0;
    Result rc1 = svcControlMemory(&base, 0, 0, avail, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE);
    if (R_FAILED(rc1)) boot_fail("linear alloc of the whole commit", avail, 0, rc1);

    __ctru_linear_heap = base;
    __ctru_linear_heap_size = ours;
    __ctru_heap = base + ours;
    __ctru_heap_size = avail - ours;

    u32 soc_base = 0;
    soc_rc_alloc = svcControlMemory(&soc_base, 0x08000000, 0, soc, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE);
    if (R_SUCCEEDED(soc_rc_alloc)) {
        soc_buf = (u8*)soc_base;
        soc_size = soc;
    }

    mappableInit(OS_MAP_AREA_BEGIN, OS_MAP_AREA_END);
    fake_heap_start = (char*)__ctru_heap;
    fake_heap_end = fake_heap_start + __ctru_heap_size;
}

/* HW7: after socExit. The 128 KB block is ours (MEMOP_ALLOC at 0x08000000). */
void pm_soc_free(void) {
    u32 out;
    if (soc_buf) svcControlMemory(&out, (u32)soc_buf, 0, soc_size, MEMOP_FREE, 0);
    soc_buf = NULL;
    soc_size = 0;
}

void* pm_soc_buffer(unsigned* size) {
    if (size) *size = soc_size;
    return soc_buf;
}

unsigned pm_heap_bytes(void) { return __ctru_heap_size; }
unsigned pm_linear_bytes(void) { return __ctru_linear_heap_size; }

unsigned pm_tex_used(void);
unsigned pm_tex_peak(void);
unsigned pm_tex_map_peak_take(unsigned* wmax);
unsigned pm_tex_budget(void);
unsigned pm_tex_fail_count(void);
static unsigned lin_peak; /* highest linear bytes in use seen at a log call */

/* M01 measurement: newlib heap in use, newlib heap free, linear pool free. All bytes read from the OS now. */
/* Memory line for the crash handler. No mallinfo and no linear heap query: the fault may sit inside malloc and hold its lock. */
int pm_mem_crash_line(char* buf, int cap) {
    s64 cur = 0;
    Handle rl = 0;
    ResourceLimitType type = RESLIMIT_COMMIT;
    if (R_SUCCEEDED(svcGetResourceLimit(&rl, CUR_PROCESS_HANDLE))) {
        svcGetResourceLimitCurrentValues(&cur, rl, &type, 1);
        svcCloseHandle(rl);
    }
    return snprintf(buf, (size_t)cap, "[CRASH] mem heap total %uKB sbrk-used %uKB linear pool %uKB commit used %uKB of %uKB\n",
                    pm_heap_bytes() >> 10, (unsigned)((char*)sbrk(0) - (char*)__ctru_heap) >> 10, pm_linear_bytes() >> 10,
                    (unsigned)(cur >> 10), (unsigned)(commit_max >> 10));
}

/* Bottom screen memory rows, 40 bytes each with the NUL. Called at 1 Hz from the game thread: mallinfo takes the malloc lock.
 * Row 0 is the game pool: linear pool plus newlib heap (pm_linear_bytes, pm_heap_bytes), in MB with one decimal. */
static unsigned tenth_mb(u32 bytes) { return (unsigned)((bytes * 10u + (1u << 19)) >> 20); }
void pm_mem_hud(char out[3][40]) {
    u32 lin_total = pm_linear_bytes(), heap_total = pm_heap_bytes();
    u32 lin_free = (u32)linearSpaceFree();
    u32 heap_used = (u32)mallinfo().uordblks;
    u32 total = lin_total + heap_total;
    u32 free_b = lin_free + (heap_total - heap_used); /* heap free: arena free plus sbrk-left */
    u32 used_b = total - free_b;
    unsigned vram_free = (unsigned)(vramSpaceFree() >> 10), vram_used = 6u * 1024u - vram_free; /* the PICA has 6 MB of VRAM */
    snprintf(out[0], 40, "MEM used %u.%u / %u.%u MB  free %u.%u MB",
             tenth_mb(used_b) / 10, tenth_mb(used_b) % 10, tenth_mb(total) / 10, tenth_mb(total) % 10,
             tenth_mb(free_b) / 10, tenth_mb(free_b) % 10);
    snprintf(out[1], 40, "linear %u/%uK heap %u/%uK", (lin_total - lin_free) >> 10, lin_total >> 10, heap_used >> 10, heap_total >> 10);
    snprintf(out[2], 40, "VRAM used %uK free %uK tex %uK", vram_used, vram_free, pm_tex_used() >> 10);
    static int hud_logged; /* one line per run: proves the 1 Hz refresh ran */
    if (!hud_logged++) pm_log_d("hud mem rows ready: %s | %s | %s", out[0], out[1], out[2]);
}

void pm_mem_log(const char* tag) {
    if (!pm_log_file()) return; /* logging off: no stack walk, no stats formatting */
    if (!strcmp(tag, "map") && pm_debug_has("texmax")) {
        unsigned wmax, pk = pm_tex_map_peak_take(&wmax);
        pm_log_d("texmax %u KB of %u KB before this map load, one frame used %u KB, now %u KB", pk >> 10, pm_tex_budget() >> 10, wmax >> 10, pm_tex_used() >> 10);
    }
    {
        unsigned gs = 0, as = 0;
        unsigned gu = pm_stack_used(0, &gs), au = pm_stack_used(1, &as);
        pm_log_d("mem %s stack high-water game %u of %u B, audio %u of %u B", tag, gu, gs, au, as);
        pm_gpu_log_stats(tag);
    }
    if (!strcmp(tag, "boot")) /* BS1: status line, shown at every log level */
        pm_log_w("memory: linear %uKB, heap %uKB, texture budget %uKB, FCRAM cap %uKB", pm_linear_bytes() >> 10, pm_heap_bytes() >> 10, pm_tex_budget() >> 10, PM_FCRAM >> 10);
    if (!strcmp(tag, "boot"))
        pm_log_d("mem commit limit %uKB used-by-image %uKB offered %uKB cap %uKB", (unsigned)(commit_max >> 10), (unsigned)(commit_cur >> 10),
               (unsigned)((commit_max - commit_cur) >> 10), PM_FCRAM >> 10);
    if (!strcmp(tag, "boot")) { /* SOC buffer: 0 when the 128 KB block was not allocated */
        unsigned sb = 0;
        pm_soc_buffer(&sb);
        pm_log_d("mem SOC buffer %uKB at 0x08000000", sb >> 10);
    }
    unsigned lin_used = pm_linear_bytes() - (unsigned)linearSpaceFree();
    if (strcmp(tag, "boot") && lin_used > lin_peak) lin_peak = lin_used;
    if (!strcmp(tag, "boot"))
        pm_log_d("mem split H03 linear pool %uKB = fixed need %uKB + tex budget %uKB + margin %uKB, heap %uKB", pm_linear_bytes() >> 10,
               PM_LINEAR_FIXED_KB, pm_tex_budget() >> 10, (pm_linear_bytes() >> 10) - PM_LINEAR_FIXED_KB - (pm_tex_budget() >> 10),
               pm_heap_bytes() >> 10);
    else
        pm_log_d("mem %s linear used %uKB peak %uKB texture failures %u", tag, lin_used >> 10, lin_peak >> 10, pm_tex_fail_count());
    pm_log_d("mem %s vram free %uKB of %uKB (B7-0)", tag, (unsigned)(vramSpaceFree() >> 10), (unsigned)(6u * 1024u)); /* the PICA has 6 MB of VRAM */
    struct mallinfo mi = mallinfo();
    pm_log_d("mem %s heap total %uKB used %uKB free-in-arena %uKB sbrk-left %uKB linear total %uKB free %uKB tex-cache %uKB peak %uKB", tag,
           pm_heap_bytes() >> 10, (unsigned)mi.uordblks >> 10, (unsigned)mi.fordblks >> 10,
           (unsigned)(fake_heap_end - (char*)__ctru_heap - mi.arena) >> 10, pm_linear_bytes() >> 10,
           (unsigned)linearSpaceFree() >> 10, pm_tex_used() >> 10, pm_tex_peak() >> 10);
}
