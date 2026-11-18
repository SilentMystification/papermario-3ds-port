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
#include "nd_fs.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
#include "nd_grabonce.h"

/* GLOB: definitions moved here from the former pm_gbi.c (layout only). */
void pm_zbuf_need(void);
int pm_fb_native(const void* p);
int pm_is_cur_cfb(const void* p);

/* V05: the colour image is a game buffer (not a frame buffer, not the saved frame): the draw goes to an off-screen render
 * texture, which a later draw reads as an image (draw_box: the contents of a rotating window). */
/* B1: the one decision for a colour image (docs/plans/buffer_bridge.md table 1.2). for_fill = 1 at G_FILLRECT.
 * ROLE_CPUPAL and ROLE_DEPTH are not returned yet (rect_to_image keeps its own test, B3 adds the depth role).
 * B3, R4: the z-buffer image at siz 2 with a FILL cycle or Z_CMP or Z_UPD is DEPTH (a depth write on the main target). */
#define nd_cfb ((const unsigned short*)pm_walk_cfb) /* CORE2C-2: the colour buffer of the walked frame */
int zcol, zslot_on; /* B5: zcol = this texrect copies colour into the z-buffer image (underwater, case 7): a slot, not a depth write */
void* const zb_fake = (void*)0x1000; /* the key of that slot when nuGfxZBuffer is NULL (address 0 cannot name a texture) */
/* NDK_ZERO (nd_tgt.kind): B3: the last G_SETCIMG named address 0. nuGfxZBuffer is NULL until the pause or the darkness overlay allocates it (D03), so the game's z-buffer image is address 0 */
/* The role is nd_tgt_role (nd_tgt.h), a pure function of the target record set in set_cimg. */
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
int grab_hold;                  /* SBLK2 B1: set by the G_PM_ND_GRABHOLD bracket (motion_blur_flame.c, nd_native_rec) */
unsigned hold_grab_f = ~0u;     /* SBLK2 B1: the frame whose one copy the hold reuses */
extern unsigned pm_gpu_seq; /* GRABSEQ: keys both grabs on the drawn frame, not on pm_frame_count() */
/* LS-4: the G_PM_ND payload kinds (gbi_custom.h), called by the walker (nd_walk.c, HC4d2: the NDO_MISC_ND fields). */
void nd_native_rec(unsigned kind, unsigned on) {
    if (kind == G_PM_ND_SPLASH) splash_on = (int)on;
    if (kind == G_PM_ND_GRABHOLD) grab_hold = (int)on;
    if (kind == G_PM_ND_SPRSHADE) { extern int nd_spr_walk; extern unsigned shc[16]; nd_spr_walk = (int)on; if (on && PMDL("shadecen")) shc[14]++; } /* SHADEIMG: the imgfx shaded sprite, the walker's KZN04 multiply on inside */
    if (kind == G_PM_ND_GUTTER) { extern void pm_gpu_pause_gutters(unsigned fade); pm_gpu_pause_gutters(on); } /* PAUSEOPEN: the pause gutters; PAUSEFADE: on is the fade (0..128), the gutter mix colour */
}
void grab_cur(const void* src, int w, int h) {
    int x, y, r;
    if (!src || w < 1 || !pm_is_cur_cfb(src)) return;
    if (grab_hold) { /* SBLK2 B1: one full-screen copy per frame for the whole flame draw; the strips read it (the splash rule) */
        if (!nd_grab_once(&hold_grab_f, pm_gpu_seq)) return;
        r = pm_gpu_grab(nd_cfb, 0, 240);
        if (r == 2) { n_grabs++; n_splits++; }
        return;
    }
    if (splash_on) { /* HANG1a: a water splash window: one full-height copy per frame, the other windows of the frame read it */
        if (!nd_grab_once(&splash_grab_f, pm_gpu_seq)) return;
        r = pm_gpu_grab(nd_cfb, 0, 240);
        if (r == 2) { n_grabs++; n_splits++; }
        return;
    }
    x = (int)(((const u8*)src - (const u8*)nd_cfb) >> 1) % 320;
    y = (int)(((const u8*)src - (const u8*)nd_cfb) >> 1) / 320; /* rows of the window: a later load of other rows reuses the copy (R13c) */
    r = pm_gpu_grab(nd_cfb, y, y + (h < 1 ? 1 : h));
    if (r == 2) { n_grabs++; n_splits++; }
}
void off_screen_check(void) {
    int r;
    r = nd_img_role(0);
    /* X02: an image that is not 16 bit (the darkness stencil: 32 bit, then 8 bit, in the z-buffer memory) always renders off-screen */
    if ((r == ROLE_SLOT || r == ROLE_DEPTHQ)) {
        /* FSELLAG2: a window image (draw_box: scissor 0, 0, width, height, then the fill of that rectangle) needs a clear only in the row blocks of its height.
         * Only a stride from 65 to 319 with the scissor equal to the image width qualifies: the 320 wide users (zbuffer copies, frozen frame, water) and the 64 wide stencils clear the whole slot. */
        int band = (nd_tgt.stride > 64 && nd_tgt.stride < 320 && nd_tgt.siz == 2 && sci_x0 == 0 && sci_y0 == 0 && sci_x1 == nd_tgt.stride && sci_y1 > 0) ? sci_y1 : 0;
        pm_gpu_off_begin(nd_tgt.img ? nd_tgt.img : zb_fake, nd_tgt.stride, nd_tgt.siz, band);
    }
}
/* CORE-2b batch D: colour image setter (decoded fields; cimg_zero is set from the address word). */
static __attribute__((noinline)) void set_cimg(u32 addr, int ns, int stride) {
    void* nc = ptr_of(addr);
    if (nc != nd_tgt.img || ns != nd_tgt.siz) pm_gpu_off_end(); /* V05 */
    nd_tgt.img = nc;
    zcol = 0;
    nd_tgt.siz = (uint8_t)ns;
    nd_tgt.stride = (uint16_t)stride;
    /* NTGT: the address class, decided once here (each test as img_role had it) */
    nd_tgt.kind = (uint8_t)((nc == NULL ? NDK_NULL : 0u) | (addr == 0 ? NDK_ZERO : 0u) | (nc == (void*)DepthCopyBuffer ? NDK_DEPTHQ : 0u) |
        (pm_is_cfb(nc) ? NDK_CFB : 0u) |
        ((nuGfxZBuffer && (const unsigned short*)nc >= nuGfxZBuffer && (const unsigned short*)nc < nuGfxZBuffer + 320 * 240) ? NDK_INZ : 0u) |
        (pm_fb_native(nc) ? NDK_NATIVE : 0u));
}
void gbi_setcimg(u32 a, u32 b) { set_cimg(b, (int)((a >> 12) & 3), (int)(a & 0xfff) + 1); }
/* NTGT: the NDC_NAPI runner of nd_target_begin and nd_target_end (nd_geo.c): one G_SETCIMG unit as the walker runs a control op (op_ctl, nd_walk.c):
 * no skip test, the prologue (pm_gbi_cur_op, gv_epoch), then the setter. rec: w0 = siz << 12 | width - 1, w1 = the address. */
void nd_tgt_run(const uint32_t* rec) {
    pm_gbi_cur_op = ND_FS_MISC;
    gv_epoch++; /* GV1b */
    gbi_setcimg(rec[0], rec[1]);
}

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
