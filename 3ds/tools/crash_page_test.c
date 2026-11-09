/* CRASHALL part 2 host test: the crash page renders its lines into the RGBA16 buffer (glyph pixels 0xFFFF on the PC line). */
#include <stdio.h>
#include "pm_crash_page.h"

static uint16_t fb[PM_CRASH_PAGE_W * PM_CRASH_PAGE_H];

int main(void) {
    const char* lines[] = { "ARM CRASH: data abort", "THREAD prefetch", "PC 00123456 LR 00ABCDEF SP 0F000000", "FAR 00000000 FSR 00000005 CPSR 60000010" };
    pm_crash_page_render(fb, lines, 4);
    int lit = 0, lit_pc = 0, lit_below = 0;
    for (int y = 0; y < PM_CRASH_PAGE_H; y++)
        for (int x = 0; x < PM_CRASH_PAGE_W; x++) {
            if (fb[y * PM_CRASH_PAGE_W + x] != 0xFFFF) continue;
            lit++;
            if (y >= 12 + 2 * 13 && y < 12 + 2 * 13 + 7) lit_pc++;   /* the PC line: line 2 */
            if (y >= 12 + 4 * 13) lit_below++;                        /* below the last line: nothing */
        }
    printf("crash page: glyph pixels %d, PC line %d, below the text %d\n", lit, lit_pc, lit_below);
    return (lit_pc >= 40 && lit_below == 0) ? 0 : 1;
}
