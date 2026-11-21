#ifndef PM_TEX_KEY_H
#define PM_TEX_KEY_H
#include <stdint.h>
/* DEC1: t is a tiled RGBA5551 image (pm_tex.c tiled16 order), pw and ph powers of two 8..1024. An alpha-0 texel with an opaque 8-neighbour takes the floor mean RGB of those neighbours, alpha stays 0. Opaque texels never change, so the walk order does not matter. ws: pw + ph free words. Returns the changed count. */
static __attribute__((noinline)) unsigned tk_fill5551(uint16_t* t, int pw, int ph, uint32_t* ws) {
    static const uint32_t rc[9] = { 0u, 65536u, 32768u, 21846u, 16384u, 13108u, 10923u, 9363u, 8192u }; /* ceil(65536 / c): (s * rc[c]) >> 16 = s / c for s <= 31 c */
    uint32_t *ox = ws, *oy = ws + pw; unsigned n = 0;
    for (int x = 0; x < pw; x++) ox[x] = (uint32_t)(x >> 3) * 64u + (uint32_t)((x & 1) | (x & 2) << 1 | (x & 4) << 2);
    for (int y = 0; y < ph; y++) oy[y] = (uint32_t)(y >> 3) * (uint32_t)(pw >> 3) * 64u + (uint32_t)((y & 1) << 1 | (y & 2) << 2 | (y & 4) << 3);
    for (int y = 0; y < ph; y++) for (int x = 0; x < pw; x++) {
        uint16_t* p = t + oy[y] + ox[x]; if (*p & 1u) continue;
        unsigned r = 0, g = 0, b = 0, c = 0;
        for (int j = y > 0 ? y - 1 : 0; j <= y + 1 && j < ph; j++) for (int i = x > 0 ? x - 1 : 0; i <= x + 1 && i < pw; i++) {
#ifdef TK_SEED_FAULT
            if (i != x && j != y) continue; /* seeded fault: 4-neighbours only */
#endif
            const unsigned v = t[oy[j] + ox[i]]; if (v & 1u) { r += v >> 11; g += (v >> 6) & 31u; b += (v >> 1) & 31u; c++; } }
        if (!c) continue;
        const uint16_t nv = (uint16_t)(((r * rc[c]) >> 16) << 11 | ((g * rc[c]) >> 16) << 6 | ((b * rc[c]) >> 16) << 1);
        if (nv != *p) { *p = nv; n++; } }
    return n;
}
/* DEC1: 1 when a big-endian N64 palette of n entries has an alpha-0 entry (bit 0 of the low byte) */ static int tk_pal_a0(const unsigned char* p, int n) { if (!p) return 0; for (int i = 0; i < n && i < 256; i++) if (!(p[2 * i + 1] & 1u)) return 1; return 0; }
#endif
