/* pm_tex_key_test.c: host test of the RGBA5551 key colour fill (pm_tex_key.h, used by pm_tex.c, DEC1).
 * Offsets come from a copy of tiled16 (pm_tex.c) divided by 2 (u16 index). */
#include <stdio.h>
#include <stdint.h>
#include "pm_tex_key.h"

static unsigned tiled16_idx(int x, int y, int pw) {
    unsigned mx = (unsigned)x & 7u;
    unsigned my = (unsigned)y & 7u;
    unsigned morton = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) |
                      ((mx & 4u) << 2) | ((my & 4u) << 3);
    unsigned tile = ((unsigned)y >> 3) * ((unsigned)pw >> 3) + ((unsigned)x >> 3);
    return tile * 64u + morton;
}

static int fails;

static void check(int ok, const char* what) {
    if (!ok) { printf("tkd FAIL %s\n", what); fails++; }
}

int main(void) {
    static uint16_t t[256];
    static uint32_t ws[32];
    unsigned n;

    /* (1) 8x8 zeros, (3,3) opaque red: 8 neighbours take 0xF800, the rest stay 0. */
    for (int i = 0; i < 64; i++) t[i] = 0;
    t[tiled16_idx(3, 3, 8)] = 0xF801;
    n = tk_fill5551(t, 8, 8, ws);
    check(n == 8, "case1 count");
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
        uint16_t v = t[tiled16_idx(x, y, 8)];
        int nb = (x >= 2 && x <= 4 && y >= 2 && y <= 4) && !(x == 3 && y == 3);
        if (x == 3 && y == 3) check(v == 0xF801, "case1 centre");
        else if (nb) check(v == 0xF800, "case1 neighbour");
        else check(v == 0, "case1 far");
    }

    /* (2) 16x16, (7,7) opaque green: diagonal neighbour (8,8) takes 0x07C0. */
    for (int i = 0; i < 256; i++) t[i] = 0;
    t[tiled16_idx(7, 7, 16)] = 0x07C1;
    n = tk_fill5551(t, 16, 16, ws);
    check(t[tiled16_idx(8, 8, 16)] == 0x07C0, "case2 diagonal");
    check(n == 8, "case2 count");

    /* (3) 8x8, (1,0)=0x5001 and (0,1)=0x0501: (0,0) takes mean (5,10,0) = 0x2A80. */
    for (int i = 0; i < 64; i++) t[i] = 0;
    t[tiled16_idx(1, 0, 8)] = 0x5001;
    t[tiled16_idx(0, 1, 8)] = 0x0501;
    tk_fill5551(t, 8, 8, ws);
    check(t[tiled16_idx(0, 0, 8)] == 0x2A80, "case3 mean");

    /* (4) tk_pal_a0: an alpha-0 entry is bit 0 of the low byte of a big-endian entry. */
    {
        const unsigned char p1[4] = { 0x00, 0x01, 0x00, 0x00 };
        const unsigned char p2[2] = { 0x00, 0x01 };
        check(tk_pal_a0(p1, 2) == 1, "case4 a0 present");
        check(tk_pal_a0(p2, 1) == 0, "case4 all opaque");
        check(tk_pal_a0(NULL, 2) == 0, "case4 null");
    }

    if (fails) return 1;
    printf("tkd test PASS\n");
    return 0;
}
