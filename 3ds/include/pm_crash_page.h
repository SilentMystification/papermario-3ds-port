#ifndef PM_CRASH_PAGE_HDR
#define PM_CRASH_PAGE_HDR

#include <stdint.h>

/* Crash page size: the top screen's 320 x 240 region used by the page (the decomp's N64 crash screen size). */
#define PM_CRASH_PAGE_W 320
#define PM_CRASH_PAGE_H 240
#define PM_CRASH_PAGE_LINES 16

/* Draws the text lines (one per row, 13 px apart from y=12) into fb, a PM_CRASH_PAGE_W x PM_CRASH_PAGE_H RGBA16 buffer. */
void pm_crash_page_render(uint16_t* fb, const char* const* lines, int n);

#endif
