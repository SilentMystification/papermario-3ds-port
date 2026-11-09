/* CRASHALL part 2 host test: the crash page renders its lines into the RGBA16 buffer (glyph pixels 0xFFFF on the PC line). */
#include <stdio.h>
#include "pm_crash_page.h"

static int cnt[3]; /* all lit, lit on the PC line, lit below the text */
static void lit_px(void* c, int x, int y) {
    int* k = (int*)c;
    (void)x;
    k[0]++;
    if (y >= 12 + 2 * 13 && y < 12 + 2 * 13 + 7) k[1]++; /* the PC line: line 2 */
    if (y >= 12 + 4 * 13) k[2]++;                        /* below the last line: nothing */
}

int main(void) {
    const char* lines[] = { "ARM CRASH: data abort", "THREAD prefetch", "PC 00123456 LR 00ABCDEF SP 0F000000", "FAR 00000000 FSR 00000005 CPSR 60000010" };
    pm_crash_page_render(lit_px, cnt, lines, 4);
    int lit = cnt[0], lit_pc = cnt[1], lit_below = cnt[2];
    printf("crash page: glyph pixels %d, PC line %d, below the text %d\n", lit, lit_pc, lit_below);
    return (lit_pc >= 40 && lit_below == 0) ? 0 : 1;
}
