#include <3ds.h>
#include <string.h>

/* O3DS application FCRAM, all of it. N3DS and Azahar can offer more; this
 * build does not take it.
 *   4MB  N64 base RAM. Left free in the newlib heap for the game.
 *  40MB  Cartridge image, one resident copy.
 *  20MB  This port: linearAlloc pool (GPU command buffer, vertices, texture
 *        scratch, audio) plus the netload socket buffer.
 */
#define PM_FCRAM (64u * 1024u * 1024u)
#define PM_OURS  (20u * 1024u * 1024u)
#define PM_SOC   0x20000u

extern char* fake_heap_start;
extern char* fake_heap_end;
extern u32 __ctru_heap, __ctru_heap_size;
extern u32 __ctru_linear_heap, __ctru_linear_heap_size;

static u8* soc_buf;
static u32 soc_size;

static int started_by_netload(void) {
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

void __system_allocateHeaps(void) {
    Handle reslimit = 0;
    if (R_FAILED(svcGetResourceLimit(&reslimit, CUR_PROCESS_HANDLE))) svcBreak(USERBREAK_PANIC);
    s64 max_commit = 0, cur_commit = 0;
    ResourceLimitType type = RESLIMIT_COMMIT;
    svcGetResourceLimitLimitValues(&max_commit, reslimit, &type, 1);
    svcGetResourceLimitCurrentValues(&cur_commit, reslimit, &type, 1);
    svcCloseHandle(reslimit);

    u32 avail = (u32)(max_commit - cur_commit) & ~0xFFFu;
    if (avail > PM_FCRAM) avail = PM_FCRAM;

    u32 soc = started_by_netload() ? PM_SOC : 0;
    if (avail <= soc + PM_OURS) svcBreak(USERBREAK_PANIC);
    avail -= soc;
    u32 ours = PM_OURS - soc;

    u32 base = 0;
    if (R_FAILED(svcControlMemory(&base, 0, 0, avail, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE)))
        svcBreak(USERBREAK_PANIC);

    __ctru_linear_heap = base;
    __ctru_linear_heap_size = ours;
    __ctru_heap = base + ours;
    __ctru_heap_size = avail - ours;

    if (soc) {
        u32 soc_base = 0;
        if (R_SUCCEEDED(svcControlMemory(&soc_base, 0, 0, soc, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE))) {
            soc_buf = (u8*)soc_base;
            soc_size = soc;
        }
    }

    mappableInit(OS_MAP_AREA_BEGIN, OS_MAP_AREA_END);
    fake_heap_start = (char*)__ctru_heap;
    fake_heap_end = fake_heap_start + __ctru_heap_size;
}

void* pm_soc_buffer(unsigned* size) {
    if (size) *size = soc_size;
    return soc_buf;
}

unsigned pm_heap_bytes(void) { return __ctru_heap_size; }
unsigned pm_linear_bytes(void) { return __ctru_linear_heap_size; }
