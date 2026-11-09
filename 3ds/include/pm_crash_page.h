#ifndef PM_CRASH_PAGE_HDR
#define PM_CRASH_PAGE_HDR

#include <stdint.h>

/* Crash page size: the top screen's 320 x 240 region used by the page (the decomp's N64 crash screen size). */
#define PM_CRASH_PAGE_W 320
#define PM_CRASH_PAGE_H 240
#define PM_CRASH_PAGE_LINES 16

/* Calls lit(ctx, x, y) for every lit pixel of the page (x < PM_CRASH_PAGE_W, y < PM_CRASH_PAGE_H). The caller clears the
 * target first (the unlit pixels are not written). Lines are 13 px apart from y=12, glyph rows 7 px, no buffer is needed. */
typedef void (*PmCrashPx)(void* ctx, int x, int y);
void pm_crash_page_render(PmCrashPx lit, void* ctx, const char* const* lines, int n);

#endif
