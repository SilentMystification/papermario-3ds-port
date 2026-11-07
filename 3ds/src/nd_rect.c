/* nd_rect.c: the handlers of G_TEXRECT and G_FILLRECT (CORE-3, plan_s10_path S10 handler modules). The walker (pm_gbi.c) dispatches the words
 * here. The rectangle state is nd_ws; the walker helpers they use are in pm_gbi.c and declared in pm_gbi_walk.h. Output: pm_gpu_tex_rect and pm_gpu_fill_rect. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "pm_gbi.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "pm_gbi_walk.h"

/* CORE-3 census: calls of each handler (the census counts ops by handler module: nd_rect here, pm_gbi.c for the rest) */
unsigned nd_rect_tex_n, nd_rect_fill_n;

#define PMD(word) ({ static int v_ = -1; if (v_ < 0) v_ = pm_debug_has(word); v_; })

/* HUDB1 debug switch texrlog: one line per texrect that returns before pm_gpu_tex_rect. why 1 depth copy, 2 rect_to_image, 3 box_pattern_rect,
 * 4 empty rectangle (lower right left of upper left after the 1-cycle step). At most 400 lines, repeats of the last line are not logged. */
static void texr_early(int why, int xl, int yl, int xh, int yh) {
    static int nlog = 0;
    static int lwhy = -1, lxl, lyl, lxh, lyh;
    if (nlog < 400 && PMD("texrlog") && !(why == lwhy && xl == lxl && yl == lyl && xh == lxh && yh == lyh)) {
        nlog++;
        lwhy = why; lxl = xl; lyl = yl; lxh = xh; lyh = yh;
        pm_log_d("texrlog f%u why %d rect %d,%d-%d,%d cyc %u", pm_frame_count(), why, xl / 4, yl / 4, xh / 4, yh / 4, (unsigned)((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3));
    }
}

__attribute__((noinline)) void nd_texrect(u32 w0, u32 w1, u32 hs, u32 hd) {
    nd_rect_tex_n++;
    tex_gen++; /* TBREP */
    int xh = (int)((w0 >> 12) & 0xfff);
    int yh = (int)(w0 & 0xfff);
    int xl = (int)((w1 >> 12) & 0xfff);
    int yl = (int)(w1 & 0xfff);
    if (xh & 0x800) xh |= ~0xfff;
    if (yh & 0x800) yh |= ~0xfff;
    if (xl & 0x800) xl |= ~0xfff;
    if (yl & 0x800) yl |= ~0xfff;
    /* F03: the decomp copies one z-buffer pixel into DepthCopyBuffer[id] (scissor x0 = id). Read the PICA depth. */
    if (nd_ws.cimg == (void*)DepthCopyBuffer) { pm_depth_query(nd_ws.timg, (int)((s16)(hs >> 16) >> 5), sci_x0); texr_early(1, xl, yl, xh, yh); return; }
    if (rect_to_image(xl, yl, xh, yh, (w1 >> 24) & 7, hs, hd)) { texr_early(2, xl, yl, xh, yh); return; }
    { /* FB3 debug switch texlog: one line per texrect: size, combine, render and tile formats, z source, alpha compare (ball debug) */
        static int tlon = -1;
        if (tlon < 0) tlon = pm_debug_has("texlog");
        if (tlon) pm_log_d("texlog f%u rect %d,%d-%d,%d w%d h%d cyc %u rm %08x zsrc %d ac %u tile0 %d/%d tile1 %d/%d valid1 %d",
                           pm_frame_count(), xl / 4, yl / 4, xh / 4, yh / 4, (xh - xl) / 4 + 1, (yh - yl) / 4 + 1, (nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3,
                           (unsigned)nd_ws.omode_l, (nd_ws.omode_l >> G_MDSFT_ZSRCSEL) & 1, (nd_ws.omode_l >> 0) & 3, nd_ws.tst1.tile_fmt[0], nd_ws.tst1.tile_siz[0], nd_ws.tst1.tile_fmt[1], nd_ws.tst1.tile_siz[1], nd_ws.td[1].valid);
    }
    /* B5: a texrect into the z-buffer image without G_ZS_PRIM copies colour (underwater strips of the frame): a slot keyed by the z-buffer address */
    zcol = nd_ws.cimg_siz == 2 && (cimg_zero || (nuGfxZBuffer && (const unsigned short*)nd_ws.cimg >= nuGfxZBuffer && (const unsigned short*)nd_ws.cimg < nuGfxZBuffer + 320 * 240)) && !(nd_ws.omode_l & (1u << G_MDSFT_ZSRCSEL));
    if (zcol) { zslot_on = 1; sync_render(); }
    else if (img_role(0) == ROLE_SLOT) sync_render(); /* FX2a: a texrect into an off-screen slot takes its own blender (flame passes: CLD_SURF2 is an alpha blend, OPA_SURF2 a write). The render state came from the last triangle before. */
    off_screen_check();
    box_fold_sync();
    sync_render(); /* V25: the rectangle carries the blender, depth and alpha test of its own mode */
    if (box_pattern_rect(xl, yl, xh, yh, hs, hd)) { texr_early(3, xl, yl, xh, yh); return; }
    /* V23: the texrect names its tile. The pause map draws its path dots with tile 1 (the small marker). Tile 0 holds the big location circle. */
    int rtile = (nd_ws.use_t1 || (((w1 >> 24) & 7) == 1 && nd_ws.td[1].valid)) ? 1 : 0;
    if (!rtile) gp3_t0_surv();
    ensure_tile(rtile);
    dual_bind();
    int tw = 1, th = 1;
    pm_tex_size(&tw, &th);
    /* V12b: the tile shift scales a texrect coordinate too (bulb glow uses shift 1 1). It applies after the interpolation, so it scales the step as well. */
    float ms = shift_mul(nd_ws.tst1.tile_shift[rtile][0]);
    float mt = shift_mul(nd_ws.tst1.tile_shift[rtile][1]);
    float s0 = ((float)(s16)(hs >> 16) / 32.f * ms - (float)nd_ws.tile_x0 - nd_ws.tile_fx) / (float)tw;
    float t0 = ((float)(s16)(hs & 0xffff) / 32.f * mt - (float)nd_ws.tile_y0 - nd_ws.tile_fy) / (float)th;
    float dsdx = (float)(s16)(hd >> 16) / 1024.f * ms;
    float dtdy = (float)(s16)(hd & 0xffff) / 1024.f * mt;
    float width = (float)(xh - xl) / 4.f;
    float height = (float)(yh - yl) / 4.f;
    /* BG1/BG2 oracle plan 2, fix 1: the pixel span is the RDP one (first pixel whose left edge is at or after xl, last whose left edge is at or before xh; arithmetic shifts). s0 and t0 move from the fractional edge to that pixel edge, so the texel of every pixel is the one at its centre. Multiples of 4 give the old values. COPY only (the sky). */
    int rx0 = xl / 4, ry0 = yl / 4, rx1 = xh / 4, ry1 = yh / 4;
    if (((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) == 2) {
        /* COPY mode: the RDP writes 4 pixels per cycle, so dsdx counts 4 texels per pixel (4.0 = 1:1).
         * The lower right corner is inclusive, one more pixel (as in libultraship). */
        dsdx /= 4.f;
        rx0 = (xl + 3) >> 2; ry0 = (yl + 3) >> 2; rx1 = xh >> 2; ry1 = yh >> 2;
        s0 += dsdx * (float)(rx0 * 4 - xl) * 0.25f / (float)tw;
        t0 += dtdy * (float)(ry0 * 4 - yl) * 0.25f / (float)th;
        width = (float)(rx1 - rx0 + 1);
        height = (float)(ry1 - ry0 + 1); /* PT06: the lower edge is inclusive too. Without this a 6 row background strip showed 5 rows and every 6th screen row repeated the row above */
    }
    float s1 = s0 + dsdx * width / (float)tw;
    float t1 = t0 + dtdy * height / (float)th;
    if (((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) == 2) { /* BG1 cause 3: a NOMASK tile (mask 0) clamps at its lower edge, as the RDP does. The end is the tile lrs (10.2, inclusive) plus one texel, minus the uls part. Samples past it read the clamped edge texel (the GPU texture pad holds that texel, pm_tex.c). */
        float s1pre = s1, t1pre = t1;
        if (!nd_ws.tst1.tile_wrap[rtile][2] && s1 > s0) { float smax = ((float)(nd_ws.tst1.tile_sz[rtile][2] >> 2) + 1.f - (float)nd_ws.tile_x0 - nd_ws.tile_fx) / (float)tw; if (s1 > smax) s1 = smax; }
        if (!nd_ws.tst1.tile_wrap[rtile][3] && t1 > t0) { float tmax = ((float)(nd_ws.tst1.tile_sz[rtile][3] >> 2) + 1.f - (float)nd_ws.tile_y0 - nd_ws.tile_fy) / (float)th; if (t1 > tmax) t1 = tmax; }
        if (PMDL("bgclamp") && pm_frame_count() >= 100u && pm_frame_count() <= 160u) {
            static int nbc = 0;
            if (nbc < 400) { nbc++; pm_log_d("bgclamp f%u rect %d,%d-%d,%d s %.2f>%.2f t %.2f>%.2f tex %dx%d mask %u %u lrs %d lrt %d ux %d fx %.2f", pm_frame_count(), xl / 4, yl / 4, xh / 4, yh / 4, s1pre * tw, s1 * tw, t1pre * th, t1 * th, tw, th, (unsigned)nd_ws.tst1.tile_wrap[rtile][2], (unsigned)nd_ws.tst1.tile_wrap[rtile][3], nd_ws.tst1.tile_sz[rtile][2], nd_ws.tst1.tile_sz[rtile][3], nd_ws.tile_x0, nd_ws.tile_fx); }
        }
    }
    t1_fold(s0 < s1 ? s0 : s1, s0 < s1 ? s1 : s0, t0 < t1 ? t0 : t1, t0 < t1 ? t1 : t0);
    {
        /* pm_gpu_tex_rect takes an inclusive lower right pixel. COPY mode rectangles are inclusive (width += 1
         * above). In 1-cycle and 2-cycle mode the lower right edge is exclusive: without this the quad was one
         * pixel too wide and stretched its texture. A previous-frame blur drawn every frame zoomed in. */
        int ex = rx1, ey = ry1;
        if (((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) != 2) { ex -= 1; ey -= 1; }
        if (ex >= rx0 && ey >= ry0) {
            /* B3: the decal after a case 6 model (mac_01 crystal ball) is a G_ZS_PRIM texrect: it tests the primitive depth like the X01 fill */
            if ((nd_ws.omode_l & (1u << G_MDSFT_ZSRCSEL)) && (nd_ws.omode_l & 0x10u) && (nd_ws.geom & G_ZBUFFER)) {
                sync_render();
                pm_gpu_tex_rect_z(rx0, ry0, ex, ey, s0, t0, s1, t1, (float)nd_ws.prim_z / 32767.f);
            } else
            if (pm_tex_off_idx() >= 0 && (nd_ws.tst1.tile_wrap[rtile][0] & 3) == 3 && (nd_ws.tst1.tile_wrap[rtile][1] & 3) == 3 && nd_ws.tst1.tile_wrap[rtile][2] && nd_ws.tst1.tile_wrap[rtile][3] &&
                (1 << nd_ws.tst1.tile_wrap[rtile][2]) == tw && (1 << nd_ws.tst1.tile_wrap[rtile][3]) == th) {
                FoldSeg fs[8], ft[8];
                int ns = fold_axis(rx0, (float)(ex + 1 - rx0), s0 * tw, s1 * tw, (float)tw, 0, 1, fs);
                int nt = fold_axis(ry0, (float)(ey + 1 - ry0), t0 * th, t1 * th, (float)th, 0, 1, ft);
                if (ns && nt) {
                    for (int a = 0; a < ns; a++)
                        for (int c = 0; c < nt; c++)
                            pm_gpu_tex_rect(fs[a].p0, ft[c].p0, fs[a].p1 - 1, ft[c].p1 - 1, fs[a].u0, ft[c].u0, fs[a].u1, ft[c].u1);
                    goto rect_done;
                }
            }
            pm_gpu_tex_rect(rx0, ry0, ex, ey, s0, t0, s1, t1);
        } else texr_early(4, xl, yl, xh, yh);
    }
    rect_done:
    zcol = 0;
    return;
}

__attribute__((noinline)) void nd_fillrect(u32 w0, u32 w1) {
    nd_rect_fill_n++;
    tex_gen++; /* TBREP */
#define FILL_SX10(v) ((int)(((v) & 0x3ffu) << 22) >> 22) /* signed 10-bit field: a left edge at x < 0 stays negative (HUDB1: unsigned wrap made ul > lr and swapped a full-width band) */
int lrx = FILL_SX10(w0 >> 14);
int lry = FILL_SX10(w0 >> 2);
int ulx = FILL_SX10(w1 >> 14);
int uly = FILL_SX10(w1 >> 2);
if (PMD("fillog")) { /* HUDB1 debug switch fillog: one line per fill with a wrapped field (raw 512 or more) */
    static int nlog = 0;
    if (nlog < 400 && (((w0 >> 14) & 0x3ff) >= 512 || ((w0 >> 2) & 0x3ff) >= 512 || ((w1 >> 14) & 0x3ff) >= 512 || ((w1 >> 2) & 0x3ff) >= 512)) {
        nlog++;
        pm_log_d("fillog f%u raw ul %u,%u lr %u,%u signed ul %d,%d lr %d,%d", pm_frame_count(), (unsigned)((w1 >> 14) & 0x3ff), (unsigned)((w1 >> 2) & 0x3ff),
                 (unsigned)((w0 >> 14) & 0x3ff), (unsigned)((w0 >> 2) & 0x3ff), ulx, uly, lrx, lry);
    }
}
if (PMD("fillcyc")) { /* HUDB1 debug switch fillcyc: one line per fill with the cycle type in force (0 1cycle, 1 2cycle, 2 copy, 3 fill) */
    static int ncyc = 0;
    if (ncyc < 1500) {
        ncyc++;
        pm_log_d("fillcyc f%u cyc %u ul %d,%d lr %d,%d prim %08x fill %08x", pm_frame_count(), (unsigned)((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3),
                 ulx, uly, lrx, lry, (unsigned)nd_ws.prim, (unsigned)nd_ws.fill);
    }
}
/* D03: the pause screen extracts per-pixel edge coverage of the scene into a frame buffer with
 * the VISCVG render mode (the RDP writes it as the pixel alpha bits). The PICA has no edge
 * coverage, so every pixel is a full pixel: write the full-coverage value (gray, alpha 1) to
 * that memory and draw nothing on the screen. The game filter then leaves each pixel as is. */
if (img_role(1) == ROLE_CPUFILL) {
    unsigned short* d = (unsigned short*)nd_ws.cimg;
    if (!pm_interp_repl) pm_cfb_capture_prev_cpu_for(nd_ws.cimg); /* PT78: the fill target decides the buffer. F06-5: key pass only. the filter reads the last frame from the other frame buffer */
    for (int y = uly < 0 ? 0 : uly; y <= lry && y < 240; y++)
        for (int x = ulx < 0 ? 0 : ulx; x <= lrx && x < cimg_stride; x++) d[y * cimg_stride + x] = 0x8421;
    return;
}
if (img_role(1) == ROLE_DEPTH) { /* B3: case 5, the z-buffer clear is a depth write */
    pm_gpu_fill_depth(ulx, uly, lrx, lry, depth_of_word(nd_ws.fill));
    return;
}
/* HUDB1: the lower right corner of a fill is inclusive only in G_CYC_FILL and G_CYC_COPY. 1-cycle and 2-cycle fills cover [ul, lr). */
if (((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) < 2) {
    lrx--;
    lry--;
    if (lrx < ulx || lry < uly) return; /* empty rectangle */
}
/* FILL cycle uses the fill register. 1-cycle fills (screen fades) use primitive color. */
unsigned color =(((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) == 3)
    ? rgba_from_5551(nd_ws.fill & 0xffff) : nd_ws.prim;
off_screen_check();
if (((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) != 3) sync_render(); /* PT71: a 1-cycle fill carries the blender of its mode (the choice box fill fades with its prim alpha), not the state of the last draw */
pm_gpu_set_replace(((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) == 3);
/* X01: a fill rect with depth compare and G_ZS_PRIM tests the primitive depth against the depth buffer */
if ((nd_ws.omode_l & (1u << G_MDSFT_ZSRCSEL)) && (nd_ws.omode_l & 0x10u) && (nd_ws.geom & G_ZBUFFER)) {
    sync_render();
    pm_gpu_fill_rect_z(ulx, uly, lrx, lry, color, (float)nd_ws.prim_z / 32767.f);
} else
pm_gpu_fill_rect(ulx, uly, lrx, lry, color);
pm_gpu_set_replace(0);
return;
}

