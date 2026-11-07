/* pm_gbi_walk.h: the walker helpers (pm_gbi.c) that the handler modules use (CORE-3). nd_rect.c holds the G_TEXRECT and G_FILLRECT handlers. */
#ifndef PM_GBI_WALK_H
#define PM_GBI_WALK_H

#include "ultra64.h"

typedef struct { int p0, p1; float u0, u1; } FoldSeg;

extern unsigned tex_gen;

/* the image roles of img_role (pm_gbi.c) */
enum { ROLE_MAIN, ROLE_DEPTHQ, ROLE_CPUFILL, ROLE_DEPTH, ROLE_CPUPAL, ROLE_SLOT };

extern u16 DepthCopyBuffer[16];
extern unsigned short* nuGfxZBuffer;
void pm_depth_query(const void* timg, int x, int id); /* F03, pm_depth.c */
void pm_cfb_capture_prev_cpu_for(const void* fill);
extern int cimg_stride, cimg_zero, zcol, zslot_on, sci_x0;

int img_role(int for_fill);
float depth_of_word(unsigned w16);
void off_screen_check(void);
float shift_mul(unsigned n);
void ensure_tile(int n);
void gp3_t0_surv(void);
void box_fold_sync(void);
void sync_render(void);
unsigned rgba_from_5551(unsigned c);
int fold_axis(int plo, float width, float a0, float a1, float mw, int flip, int negclamp, FoldSeg* out);
void dual_bind(void);
void t1_fold(float u0, float u1, float v0, float v1);
int rect_to_image(int xl, int yl, int xh, int yh, unsigned tile, u32 hs, u32 hd);
int box_pattern_rect(int xl, int yl, int xh, int yh, u32 hs, u32 hd);

void nd_texrect(u32 w0, u32 w1, u32 hs, u32 hd); /* nd_rect.c: G_TEXRECT (3 slots) */
void nd_fillrect(u32 w0, u32 w1);                /* nd_rect.c: G_FILLRECT */
extern unsigned nd_rect_tex_n, nd_rect_fill_n;    /* nd_rect.c: calls of each handler */

#endif
