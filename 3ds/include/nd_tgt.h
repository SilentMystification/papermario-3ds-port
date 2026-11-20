/* nd_tgt.h: the native colour target (NTGT). The target record (image, stride, size) and its address class are set once, at the target change
 * (set_cimg, nd_target.c). The role of the target is a pure function of that record and the draw's mode words (nd_tgt_role).
 * The address tests are stable between the change and the draw: nuGfxZBuffer is allocated once and never freed, the frame buffers are static. */
#ifndef ND_TGT_H
#define ND_TGT_H
#include <stdint.h>
#include "ultra64.h"
#include "nd.h"

/* the image roles of the colour target */
enum { ROLE_MAIN, ROLE_DEPTHQ, ROLE_CPUFILL, ROLE_DEPTH, ROLE_CPUPAL, ROLE_SLOT };

/* NdTgt.kind: one bit per address test */
#define NDK_NULL   1u  /* the image pointer is NULL */
#define NDK_ZERO   2u  /* the G_SETCIMG address word was 0 (the game's z-buffer image before nuGfxZBuffer exists, D03) */
#define NDK_DEPTHQ 4u  /* the image is DepthCopyBuffer */
#define NDK_CFB    8u  /* pm_is_cfb(img) */
#define NDK_INZ    16u /* inside nuGfxZBuffer + 320 * 240 */
#define NDK_NATIVE 32u /* pm_fb_native(img) */

typedef struct NdTgt { void* img; uint16_t stride; uint8_t siz, kind; } NdTgt;
extern NdTgt nd_tgt;
extern int zcol;
void pm_zbuf_need(void);

/* B1: the one decision for a colour image (docs/plans/buffer_bridge.md table 1.2). for_fill = 1 at G_FILLRECT. Pure: no global, no call.
 * oml, omh: the draw's other mode low and high words. zcol: this texrect copies colour into the z-buffer image (a slot, not a depth write). */
static inline int nd_tgt_role(unsigned kind, unsigned siz, int for_fill, uint32_t oml, uint32_t omh, int zc) {
    if ((kind & NDK_NULL) && !(kind & NDK_ZERO)) return ROLE_MAIN;
    if (kind & NDK_DEPTHQ) return ROLE_DEPTHQ;
    if (for_fill && (kind & NDK_CFB) && (oml & ~7u) == ((G_RM_VISCVG | G_RM_VISCVG2) & ~7u)) return ROLE_CPUFILL;
    if (siz == 2 && (kind & (NDK_ZERO | NDK_INZ)) && (((omh >> G_MDSFT_CYCLETYPE) & 3) == 3 || (oml & 0x30u)) && !zc) return ROLE_DEPTH;
    if ((kind & NDK_NULL) && !zc) return ROLE_MAIN;
    if ((kind & NDK_NATIVE) && siz == 2) return ROLE_MAIN;
    return ROLE_SLOT;
}
/* NC0R: the role from the draw's own other mode words (oml, omh); D03: the darkness stencil z-buffer is allocated at ROLE_CPUFILL (pm_main.c wraps it) */
static inline int nd_img_role_m(int ff, uint32_t oml, uint32_t omh) {
    int r = nd_tgt_role(nd_tgt.kind, nd_tgt.siz, ff, oml, omh, zcol);
    if (r == ROLE_CPUFILL) pm_zbuf_need();
    return r;
}
int nd_role_c(int ff, uint32_t oml, uint32_t omh, const char* f, int l); /* NC0R census (step 1) */
#endif
