/* kccmd_test.c: host self-test of the K copy command start decode (nd_kccmd.h). Prints "kccmd ok 3/3" when every check passes. */
#include <stdio.h>
#include <stdint.h>
#include "nd_kccmd.h"

#define NW 768u /* KC_WORDS / 64 with KC_WORDS 48 K words */

/* The list: command A at 0 (one word, n 0), command B at 2 (one consecutive write of 4 params: header n 4, words 4..7), command C at 8
 * (n 0). Coff 10. The header word of each command is at start + 1. */
static uint32_t list[10];

static int check_starts(void) {
    uint32_t bm[NW];
    list[0] = 0x11111111u; list[1] = 0x00000001u;             /* A: id 1, n 0 */
    list[2] = 0x22222222u; list[3] = 0x00400002u;             /* B: id 2, n 4 */
    list[4] = 0x33333333u; list[5] = 0x33333333u; list[6] = 0x33333333u; list[7] = 0x33333333u;
    list[8] = 0x44444444u; list[9] = 0x00000003u;             /* C: id 3, n 0 */
    if (!nd_kccmd_decode(list, 10u, bm, NW)) return 0;
    for (unsigned off = 0; off < 10u; off++) {
        const int want = (off == 0u || off == 2u || off == 8u);
        if (nd_kccmd_is_start(bm, off) != want) return 0;
    }
    return 1;
}

static int check_param_refused(void) {
    uint32_t bm[NW];
    list[0] = 0x11111111u; list[1] = 0x00000001u;
    list[2] = 0x22222222u; list[3] = 0x00400002u;
    list[4] = 0x33333333u; list[5] = 0x33333333u; list[6] = 0x33333333u; list[7] = 0x33333333u;
    list[8] = 0x44444444u; list[9] = 0x00000003u;
    if (!nd_kccmd_decode(list, 10u, bm, NW)) return 0;
    if (nd_kccmd_is_start(bm, 4u)) return 0;   /* parameter word inside the consecutive write */
    if (nd_kccmd_is_start(bm, 6u)) return 0;   /* parameter word inside the consecutive write */
    if (nd_kccmd_is_start(bm, 3u)) return 0;   /* header word (odd) */
    if (nd_kccmd_is_start(bm, 9u)) return 0;   /* header word (odd) */
    return 1;
}

static int check_count_past_end_refused(void) {
    uint32_t bm[NW];
    list[0] = 0x11111111u; list[1] = 0x00000001u;
    list[2] = 0x22222222u; list[3] = 0x00400002u;
    list[4] = 0x33333333u; list[5] = 0x33333333u; list[6] = 0x33333333u; list[7] = 0x33333333u;
    list[8] = 0x44444444u; list[9] = 0x00500003u;             /* C: n 5: next 16 > coff 10 */
    if (nd_kccmd_decode(list, 10u, bm, NW)) return 0;
    list[9] = 0x00000003u;
    return 1;
}

int main(void) {
    int ok = 0;
    ok += check_starts();
    ok += check_param_refused();
    ok += check_count_past_end_refused();
    if (ok == 3) { printf("kccmd ok 3/3\n"); return 0; }
    printf("kccmd FAIL %d/3\n", ok);
    return 1;
}
