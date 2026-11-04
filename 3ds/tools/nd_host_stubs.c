/* nd_host_stubs.c: host replacements for the platform calls the native draw code makes (citro3d GPUCMD, linearAlloc,
 * svc tick, pm_tex slot words). Game logic is not stubbed. A 32-bit host build: a pointer is a 32-bit address. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ND_CMD_MAX (1u << 20)
uint32_t nd_cmd[ND_CMD_MAX]; /* GPUCMD_AddRawCommands appends here */
uint32_t nd_cmd_n;

void GPUCMD_AddRawCommands(const uint32_t* cmd, uint32_t size) {
    if (nd_cmd_n + size > ND_CMD_MAX) return;
    memcpy(nd_cmd + nd_cmd_n, cmd, size * 4u);
    nd_cmd_n += size;
}

void* linearAlloc(size_t size) { void* p = NULL; return posix_memalign(&p, 32, size) == 0 ? p : NULL; }
void linearFree(void* p) { free(p); }

static uint64_t nd_tick;
uint64_t svcGetSystemTick(void) { return nd_tick += 4; } /* each read advances 4 ticks (the test clock) */

/* pm_tex slot words: a host table, set by the test. Returns 0 for a slot that is not live. */
uint32_t nd_slot_words[16][4];
uint8_t nd_slot_live[16];
int pm_tex_slot_words(int i, uint32_t* out) {
    if (i < 0 || i >= 16 || !nd_slot_live[i]) return 0;
    memcpy(out, nd_slot_words[i], 16);
    return 1;
}
