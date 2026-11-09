/* CRASHALL part 2: the ARM crash page. The glyph table, the glyph bit layout and the rect shading are the decomp's
 * src/crash_screen.c (crash_screen_draw_glyph / crash_screen_draw_rect, the width SCREEN_WIDTH branch), copied here
 * because src/crash_screen.c needs the N64 OSThread and libultra printf, which the 3DS port does not build.
 * Pure code: no libctru, no locks. The handler (pm_main.c) draws into a static RGBA16 buffer and copies it to the screen. */
#include <stdint.h>
#include <string.h>
#include "pm_crash_page.h"

#define PAGE_W PM_CRASH_PAGE_W
#define PAGE_H PM_CRASH_PAGE_H

static const uint8_t charToGlyph[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 41, -1, -1, -1, 43, -1, -1, 37, 38, -1, 42,
    -1, 39, 44, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  36, -1, -1, -1, -1, 40, -1, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
    23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
};

/* gCrashScreenFont (crash_screen.c, the non-US branch): 5 glyphs of 6 bits per row, 7 rows per group, MSB first. */
static const uint32_t font[] = {
    0x70871C30, 0x8988A250, 0x88808290, 0x88831C90, 0x888402F8, 0x88882210, 0x71CF9C10, 0xF9CF9C70, 0x8228A288,
    0xF200A288, 0x0BC11C78, 0x0A222208, 0x8A222288, 0x71C21C70, 0x23C738F8, 0x5228A480, 0x8A282280, 0x8BC822F0,
    0xFA282280, 0x8A28A480, 0x8BC738F8, 0xF9C89C08, 0x82288808, 0x82088808, 0xF2EF8808, 0x82288888, 0x82288888,
    0x81C89C70, 0x8A08A270, 0x920DA288, 0xA20AB288, 0xC20AAA88, 0xA208A688, 0x9208A288, 0x8BE8A270, 0xF1CF1CF8,
    0x8A28A220, 0x8A28A020, 0xF22F1C20, 0x82AA0220, 0x82492220, 0x81A89C20, 0x8A28A288, 0x8A28A288, 0x8A289488,
    0x8A2A8850, 0x894A9420, 0x894AA220, 0x70852220, 0xF8011000, 0x08020800, 0x10840400, 0x20040470, 0x40840400,
    0x80020800, 0xF8011000, 0x70800000, 0x88822200, 0x08820400, 0x108F8800, 0x20821000, 0x00022200, 0x20800020,
    0x00000000,
};

static void page_rect(uint16_t* fb, int x, int y, int w, int h) {
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++) {
            uint16_t* p = &fb[(y + i) * PAGE_W + x + j];
            *p = ((*p & 0xE738) >> 2) | 1;
        }
}

/* crash_screen_draw_glyph, width == SCREEN_WIDTH branch. Clipped: a glyph outside the page is not drawn. */
static void page_glyph(uint16_t* fb, int x, int y, int glyph) {
    int shift = (glyph % 5) * 6;
    const uint32_t* data = &font[glyph / 5 * 7];
    if (x < 0 || y < 0 || x + 6 > PAGE_W || y + 7 > PAGE_H) return;
    uint16_t* ptr = fb + PAGE_W * y + x;
    for (int i = 0; i < 7; i++) {
        uint32_t bit = 0x80000000U >> shift;
        uint32_t rowMask = *data++;
        for (int j = 0; j < 6; j++) {
            *ptr++ = (bit & rowMask) ? 0xFFFF : 1;
            bit >>= 1;
        }
        ptr += PAGE_W - 6;
    }
}

/* crash_screen_printf without the va_list: one string per line, x += 6 per character, glyph 0xFF is skipped. */
static void page_text(uint16_t* fb, int x, int y, const char* s) {
    for (; *s && x + 6 <= PAGE_W; s++) {
        int glyph = charToGlyph[(uint8_t)*s & 0x7F];
        if (glyph != 0xFF) page_glyph(fb, x, y, glyph);
        x += 6;
    }
}

void pm_crash_page_render(uint16_t* fb, const char* const* lines, int n) {
    for (int i = 0; i < PAGE_W * PAGE_H; i++) fb[i] = 1; /* ground: dark, like the decomp's crash_screen_draw_rect */
    page_rect(fb, 4, 6, PAGE_W - 8, 12);
    for (int i = 0; i < n && i < PM_CRASH_PAGE_LINES; i++) page_text(fb, 8, 12 + i * 13, lines[i]);
}
