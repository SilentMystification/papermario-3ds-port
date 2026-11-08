/* nd_target.c: the colour image and its role (plan_s10_path group R-target, WK-3): img_role, grab_cur, off_screen_check, the
 * depth word helpers and the colour image setter (G_SETCIMG). Cut from the former pm_gbi.c without logic change (D12). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"

/* GLOB: definitions moved here from the former pm_gbi.c (layout only). */
int cimg_stride = 320;
void pm_zbuf_need(void);
int pm_fb_native(const void* p);
int pm_is_cur_cfb(const void* p);

/* V05: the colour image is a game buffer (not a frame buffer, not the saved frame): the draw goes to an off-screen render
 * texture, which a later draw reads as an image (draw_box: the contents of a rotating window). */
/* B1: the one decision for a colour image (docs/plans/buffer_bridge.md table 1.2). for_fill = 1 at G_FILLRECT.
 * ROLE_CPUPAL and ROLE_DEPTH are not returned yet (rect_to_image keeps its own test, B3 adds the depth role).
 * B3, R4: the z-buffer image at siz 2 with a FILL cycle or Z_CMP or Z_UPD is DEPTH (a depth write on the main target). */
extern unsigned short* nuGfxCfb_ptr;
int zcol, zslot_on; /* B5: zcol = this texrect copies colour into the z-buffer image (underwater, case 7): a slot, not a depth write */
void* const zb_fake = (void*)0x1000; /* the key of that slot when nuGfxZBuffer is NULL (address 0 cannot name a texture) */
int cimg_zero; /* B3: the last G_SETCIMG named address 0. nuGfxZBuffer is NULL until the pause or the darkness overlay allocates it (D03), so the game's z-buffer image is address 0 */
int img_role(int for_fill) {
    if (!nd_ws.cimg && !cimg_zero) return ROLE_MAIN;
    if (nd_ws.cimg == (void*)DepthCopyBuffer) return ROLE_DEPTHQ;
    if (for_fill && pm_is_cfb(nd_ws.cimg) && (nd_ws.omode_l & ~7u) == ((G_RM_VISCVG | G_RM_VISCVG2) & ~7u)) {
        pm_zbuf_need(); /* D03: the single call site in this file; pm_main.c wraps the darkness stencil */
        return ROLE_CPUFILL;
    }
    if (nd_ws.cimg_siz == 2 && (cimg_zero || (nuGfxZBuffer && (const unsigned short*)nd_ws.cimg >= nuGfxZBuffer && (const unsigned short*)nd_ws.cimg < nuGfxZBuffer + 320 * 240)) &&
        (((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) == 3 || (nd_ws.omode_l & 0x30u)) && !zcol) return ROLE_DEPTH;
    if (!nd_ws.cimg && !zcol) return ROLE_MAIN;
    if (pm_fb_native(nd_ws.cimg) && nd_ws.cimg_siz == 2) return ROLE_MAIN;
    return ROLE_SLOT;
}
/* B3: the N64 writes a 16 bit colour word into the z-buffer memory, and the RDP reads it back as 14 bit z plus 2 bit dz. The port
 * keeps z as prim_z (15 bit, 0 near): the word shifted right by 1 (the decal uses prim depth 32734 = the word 0xFFBC of prim 248 240 240). */
float depth_of_word(unsigned w16) { return (float)((w16 & 0xffffu) >> 1) / 32767.f; }
unsigned word_5551(unsigned rgba) {
    return (((rgba >> 27) & 31u) << 11) | (((rgba >> 19) & 31u) << 6) | (((rgba >> 11) & 31u) << 1) | ((rgba >> 7) & 1u);
}
unsigned n_splits, n_grabs; /* B2: per frame, see grab_cur */
extern int pm_is_cur_cfb(const void* p);
/* B2: role T2. A 16 bit texture window inside the frame buffer being drawn (water ripples, crystal ball, bulb glow). Copy it into
 * the grab slot before the load, so that the load finds the slot like any off-screen image (T1). A window inside an older frame
 * buffer is T3: G_SETTIMG fetched it with pm_cfb_capture_prev. */
int splash_on; /* HANG1a: set by the G_PM_ND_SPLASH record (water_splash.c, nd_native_rec) */
unsigned splash_grab_f = ~0u;
/* LS-4: the G_PM_ND payload kinds (gbi_custom.h), called by the walker (nd_walk.c). w0 = G_PM_ND << 24 | kind << 16 | arg. */
void nd_native_rec(u32 w0) {
    unsigned kind = (w0 >> 16) & 0xffu;
    if (kind == G_PM_ND_SPLASH) splash_on = (int)(w0 & 1u);
}
void grab_cur(const void* src, int w, int h) {
    int x, y, r;
    if (!src || w < 1 || !pm_is_cur_cfb(src)) return;
    if (splash_on) { /* HANG1a: a water splash window: one full-height copy per frame, the other windows of the frame read it */
        if (splash_grab_f == pm_frame_count()) return;
        splash_grab_f = pm_frame_count();
        r = pm_gpu_grab(nuGfxCfb_ptr, 0, 240);
        if (r == 2) { n_grabs++; n_splits++; }
        return;
    }
    x = (int)(((const u8*)src - (const u8*)nuGfxCfb_ptr) >> 1) % 320;
    y = (int)(((const u8*)src - (const u8*)nuGfxCfb_ptr) >> 1) / 320; /* rows of the window: a later load of other rows reuses the copy (R13c) */
    r = pm_gpu_grab(nuGfxCfb_ptr, y, y + (h < 1 ? 1 : h));
    if (r == 2) { n_grabs++; n_splits++; }
}
void off_screen_check(void) {
    int r;
    r = img_role(0);
    /* X02: an image that is not 16 bit (the darkness stencil: 32 bit, then 8 bit, in the z-buffer memory) always renders off-screen */
    if ((r == ROLE_SLOT || r == ROLE_DEPTHQ)) pm_gpu_off_begin(nd_ws.cimg ? nd_ws.cimg : zb_fake, cimg_stride, nd_ws.cimg_siz);
}
/* CORE-2b batch D: colour image setter (decoded fields; cimg_zero is set from the address word). */
static __attribute__((noinline)) void set_cimg(u32 addr, int ns, int stride) {
    void* nc = ptr_of(addr);
    if (nc != nd_ws.cimg || ns != nd_ws.cimg_siz) pm_gpu_off_end(); /* V05 */
    nd_ws.cimg = nc;
    cimg_zero = (addr == 0); zcol = 0;
    nd_ws.cimg_siz = ns;
    cimg_stride = stride;
}
void gbi_setcimg(u32 w0, u32 w1) { set_cimg(w1, (int)((w0 >> 19) & 3), (int)(w0 & 0xfff) + 1); }

/* ---- MV-8: target helper, cut from the former pm_gbi.c (no change) ---- */
unsigned rgba_from_5551(unsigned c) {
    unsigned r = (c >> 11) & 31;
    unsigned g = (c >> 6) & 31;
    unsigned b = (c >> 1) & 31;
    unsigned a = (c & 1) ? 255 : 0;
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    return (r << 24) | (g << 16) | (b << 8) | a;
}
