#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "pm_gbi.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
/* O6: a switch word read once. Not for the words the options panel changes at run time (nofog, nolight). */
#define PMD(word) ({ static int v_ = -1; if (v_ < 0) v_ = pm_debug_has(word); v_; })
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__) /* PG0: the flush counter by cause (switch gbiprof) */
struct TszLast { const u8* src; unsigned fmt, siz; int w, h, stride; const void* pal; int pal_n; unsigned ep; int ox, oy; int ok; };
static struct TszLast tsz_last; /* PG1 */
static unsigned st0_ep; static int st0_ok;
static int gp_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("gbiprof"); return v; }
static unsigned gp_same[8], gp_set[8]; /* PG0: state op calls and calls that set the value it already had: 0 combine, 1 prim, 2 env, 3 tile 0, 4 tile size, 5 sync_combine calls, 6 sync_render calls */
#define GP_SET(k, same) do { if (gp_on()) { gp_set[k]++; if (same) gp_same[k]++; } } while (0)

unsigned pm_gbi_sync_ticks, pm_gbi_gv_ticks, pm_gbi_tex_ticks;
unsigned pm_gbi_cur_op; /* PI2: op of the display list step now running (one store per op) */
/* PI2: batch class of the op that closes a batch. 0 TEX, 1 CC, 2 GEO, 3 RECT, 4 TRI, 5 other. */
int pm_gbi_flush_class(void) {
    switch (pm_gbi_cur_op) {
    case G_SETTIMG: case G_SETTILE: case G_SETTILESIZE: case G_LOADTILE: case G_LOADBLOCK: case G_LOADTLUT: case G_TEXTURE: return 0;
    case G_SETCOMBINE: case G_SETPRIMCOLOR: case G_SETENVCOLOR: case G_SETOTHERMODE_L: case G_SETOTHERMODE_H: case G_SETFOGCOLOR: return 1;
    case G_MTX: case G_POPMTX: case G_GEOMETRYMODE: case G_MOVEWORD: case G_MOVEMEM: return 2;
    case G_TEXRECT: case G_TEXRECTFLIP: case G_FILLRECT: return 3;
    case G_TRI1: case G_TRI2: return 4;
    default: return 5;
    }
}
/* W2: phase timers read the tick for one run in 8 and scale the sample by 8. A run is a streak of ops of one class. */
static unsigned ph_seq;
static int ph_pick(void) { return (++ph_seq & 7u) == 0; }
static int gv_run, gv_streak;
static unsigned gv_streak_t0;
static void gv_streak_flush(void) {
    gv_run = 0;
    if (!gv_streak) return;
    pm_gbi_gv_ticks += (pm_ticks() - gv_streak_t0) * 8u;
    gv_streak = 0;
}
static __attribute__((cold, noinline)) void gv_streak_op(unsigned op) {
    if (op == G_TRI1 || op == G_TRI2) {
        if (pm_ph_on && !gv_run) { gv_run = 1; if (ph_pick()) { gv_streak_t0 = pm_ticks(); gv_streak = 1; } }
    } else
        gv_streak_flush();
}
static int tex_run, tex_streak;
static unsigned tex_streak_t0;
static void tex_streak_flush(void) {
    tex_run = 0;
    if (!tex_streak) return;
    pm_gbi_tex_ticks += (pm_ticks() - tex_streak_t0) * 8u;
    tex_streak = 0;
}
static __attribute__((cold, noinline)) void tex_streak_op(unsigned op) {
    if (op == G_SETTIMG || op == G_SETTILE || op == G_SETTILESIZE || op == G_LOADTILE || op == G_LOADBLOCK || op == G_LOADTLUT || op == G_TEXTURE || op == G_PM_TEX || op == G_PM_TEXI) {
        if (pm_ph_on && !tex_run) { tex_run = 1; if (ph_pick()) { tex_streak_t0 = pm_ticks(); tex_streak = 1; } }
    } else
        tex_streak_flush();
}

extern u16 DepthCopyBuffer[16];
extern unsigned short* nuGfxZBuffer;
void pm_depth_query(const void* timg, int x, int id); /* F03, pm_depth.c */

extern unsigned int __ctru_heap;
extern unsigned int __ctru_heap_size;
extern unsigned int __ctru_linear_heap;
extern unsigned int __ctru_linear_heap_size;

static float proj[4][4];
static float mv[10][4][4];
static float combined[4][4];
static int mv_sp;
/* Viewport scale/translate after the 2-bit fraction. Full 320x240 until a display list sets one. */
static float vp_sx = 160.f, vp_sy = 120.f, vp_sz = 127.75f;
static float vp_tx = 160.f, vp_ty = 120.f, vp_tz = 127.75f;
static unsigned geom;
static unsigned omode_h;
void pm_tex_set_filter(int linear);
static unsigned omode_l = 0x00000030u; /* Z_CMP | Z_UPD until a list sets a mode */
static int rm_frame;
/* X01 debug switch xtrace=<first frame>:<count>: log fill rects, prim depth and render state changes in that frame window. */
extern unsigned pm_frame_count(void);
static int xt_on(void) {
    static int a = -2, b;
    if (a == -2) {
        char v[24];
        a = -1;
        if (pm_debug_value("xtrace", v, sizeof v) && sscanf(v, "%d:%d", &a, &b) == 2) b += a;
        else a = -1;
    }
    return a >= 0 && (int)pm_frame_count() >= a && (int)pm_frame_count() < b;
}

/* V02 debug switch drawlog=<first frame>:<count>: one line per mesh chunk (a G_VTX load at slot 0 starts one) with the
 * vertex source (the model data address), the two innermost G_DL targets, render mode, texture and screen bounds. */
static int dd_on(void) {
    static int a = -2, b;
    if (a == -2) {
        char v[24];
        a = -1;
        if (pm_debug_value("drawlog", v, sizeof v) && sscanf(v, "%d:%d", &a, &b) == 2) b += a;
        else a = -1;
    }
    return a >= 0 && (int)pm_frame_count() >= a && (int)pm_frame_count() < b;
}
/* SUNFIX debug switch suntile=<first frame>:<count>: tile 1 state at each dual_bind, the vertex S T of each triangle, and the sun
 * overlay spawn and draws (pm_sun_spawn, pm_sun_draw). At most 400 lines per run. */
static int suntile_on(void) {
    static int a = -2, b;
    if (a == -2) {
        char v[24];
        a = -1;
        if (pm_debug_value("suntile", v, sizeof v) && sscanf(v, "%d:%d", &a, &b) == 2) b += a;
        else a = -1;
    }
    return a >= 0 && (int)pm_frame_count() >= a && (int)pm_frame_count() < b;
}
static int suntile_lines;
void pm_sun_spawn(int fromRight, float x, float y, float z, int duration) {
    if (!suntile_on()) return;
    pm_log("sun spawn f%u fromRight %d pos %.1f %.1f %.1f duration %d", pm_frame_count(), fromRight, x, y, z, duration);
}
void pm_sun_draw(int alpha, int fromRight, const float* scroll, int n) {
    if (!suntile_on()) return;
    pm_log("sun draw f%u alpha %d fromRight %d scroll %.2f %.2f %.2f %.2f %.2f (n %d)", pm_frame_count(), alpha, fromRight,
           scroll[0], scroll[1], scroll[2], scroll[3], scroll[4], n);
}
/* SUN3 debug: the first words of the sun base list (segment 9 + 0x898) and of ray list 0 (+0x7C0), as they are in RAM. */
void pm_sun_base(const void* graphics) {
    const unsigned* p;
    if (!suntile_on() || !graphics) return;
    p = (const unsigned*)((const u8*)graphics + 0x898);
    pm_log("sun base gfx %p +898 %08x %08x %08x %08x", graphics, p[0], p[1], p[2], p[3]);
    p = (const unsigned*)((const u8*)graphics + 0x7C0);
    pm_log("sun base gfx +7C0 %08x %08x %08x %08x", p[0], p[1], p[2], p[3]);
}
static struct {
    unsigned vc, gm, idx, top, src, dl0, dl1, oml, omh, zb, timg, fmt, siz, w, h, prim, ntri, tex;
    float x0, y0, x1, y1, w0, w1;
} dd;
static unsigned dd_idx, dd_dl0, dd_dl1, dd_top;
/* ddonly=<lo>:<hi>: inside the drawlog window only chunks with lo <= index <= hi draw (bisect which draw makes a fault). */
static int dd_skip(void) {
    static int lo = -2, hi;
    if (lo == -2) {
        char v[24];
        if (!(pm_debug_value("ddonly", v, sizeof v) && sscanf(v, "%d:%d", &lo, &hi) == 2)) lo = -1, hi = -1;
    }
    return lo >= 0 && dd_on() && ((int)dd.idx < lo || (int)dd.idx > hi);
}
static void dd_end(void) {
    dd.ntri = 0;
}

static unsigned prim = 0xffffffffu;
static unsigned conv_k4; /* G_SETCONVERT K4 (8 bit part), the combiner input K4 */
static unsigned keyc, keys; /* B6: G_SETKEYR/G_SETKEYGB chroma key CENTER, 0xRRGGBB00. Only the combiner input CENTER reads it. */
static unsigned fill = 0xffffffffu;
static unsigned prim_z; /* X01: G_SETPRIMDEPTH, 15 bit, 0 near, 0x7fff far */
/* PRIMZFIX: the primitive depth is one unit (512 units of the 24-bit depth) nearer than G_SETPRIMDEPTH. The console rounds a 0x7FF0 blocker
 * behind the far background (water in the Toad Town pond). One helper for both NDC z uses; clamped at 0. */
static float prim_ndc_z(void) { return 2.f * (float)(prim_z ? prim_z - 1u : 0u) / 32767.f - 1.f; }
static int tex_on;
static const void* timg;
static unsigned timg_fmt, timg_siz;
static int timg_stride;
static int ld_valid, ld_x, ld_y, ld_h, ld_w; /* last LoadTile window, in 8-bit texels of the source image */
/* V09: the source of each LoadTile by TMEM word. A tile 1 window that is set again later (chapter_change strips) still reads
 * the image that was loaded to its TMEM address, not the image of the load that came last. */
static struct { unsigned tmem; const void* img; int stride, x, y; } ldrec[4];
static unsigned ldrec_n;
/* V13: a block load keeps its image and TMEM range. A half-height aux tile (tile 1 at TMEM + half) reads the second half. */
static struct { unsigned tmem, bytes; const void* img; } blk[4];
static unsigned blk_n;
static int tile_x0, tile_y0;
static float tile_fx, tile_fy; /* PT38: the quarter texel part of SetTileSize uls and ult (a pan moves in quarter texels) */
static const void* tlut;
static int tlut_n;
/* RDP state the shaded-sprite path needs: color image, TLUT per palette slot, per-tile TMEM
 * address and palette, tile sizes, scissor, env color and the second combiner cycle. */
extern int pm_is_cfb(const void* p);
extern void pm_cfb_capture_prev(void);
extern void pm_cfb_capture_prev_cpu_for(const void* fill);
extern void pm_cfb_capture_prev_cpu(void);
extern void pm_zbuf_need(void);
extern unsigned pm_frame_count(void);
static void* cimg;
static int cimg_stride = 320;
static int cimg_siz = 2; /* G_IM_SIZ of the colour image: 1 = 8 bit, 2 = 16 bit, 3 = 32 bit */
extern int pm_fb_native(const void* p);
/* V05: the colour image is a game buffer (not a frame buffer, not the saved frame): the draw goes to an off-screen render
 * texture, which a later draw reads as an image (draw_box: the contents of a rotating window). */
/* B1: the one decision for a colour image (docs/plans/buffer_bridge.md table 1.2). for_fill = 1 at G_FILLRECT.
 * ROLE_CPUPAL and ROLE_DEPTH are not returned yet (rect_to_image keeps its own test, B3 adds the depth role).
 * B3, R4: the z-buffer image at siz 2 with a FILL cycle or Z_CMP or Z_UPD is DEPTH (a depth write on the main target). */
enum { ROLE_MAIN, ROLE_DEPTHQ, ROLE_CPUFILL, ROLE_DEPTH, ROLE_CPUPAL, ROLE_SLOT };
extern unsigned short* nuGfxCfb_ptr;
static int zcol, zslot_on; /* B5: zcol = this texrect copies colour into the z-buffer image (underwater, case 7): a slot, not a depth write */
static void* const zb_fake = (void*)0x1000; /* the key of that slot when nuGfxZBuffer is NULL (address 0 cannot name a texture) */
static int cimg_zero; /* B3: the last G_SETCIMG named address 0. nuGfxZBuffer is NULL until the pause or the darkness overlay allocates it (D03), so the game's z-buffer image is address 0 */
static int img_role(int for_fill) {
    if (!cimg && !cimg_zero) return ROLE_MAIN;
    if (cimg == (void*)DepthCopyBuffer) return ROLE_DEPTHQ;
    if (for_fill && pm_is_cfb(cimg) && (omode_l & ~7u) == ((G_RM_VISCVG | G_RM_VISCVG2) & ~7u)) {
        pm_zbuf_need(); /* D03: the single call site in this file; pm_main.c wraps the darkness stencil */
        return ROLE_CPUFILL;
    }
    if (cimg_siz == 2 && (cimg_zero || (nuGfxZBuffer && (const unsigned short*)cimg >= nuGfxZBuffer && (const unsigned short*)cimg < nuGfxZBuffer + 320 * 240)) &&
        (((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 3 || (omode_l & 0x30u)) && !zcol) return ROLE_DEPTH;
    if (!cimg && !zcol) return ROLE_MAIN;
    if (pm_fb_native(cimg) && cimg_siz == 2) return ROLE_MAIN;
    return ROLE_SLOT;
}
/* B3: the N64 writes a 16 bit colour word into the z-buffer memory, and the RDP reads it back as 14 bit z plus 2 bit dz. The port
 * keeps z as prim_z (15 bit, 0 near): the word shifted right by 1 (the decal uses prim depth 32734 = the word 0xFFBC of prim 248 240 240). */
static float depth_of_word(unsigned w16) { return (float)((w16 & 0xffffu) >> 1) / 32767.f; }
static unsigned word_5551(unsigned rgba) {
    return (((rgba >> 27) & 31u) << 11) | (((rgba >> 19) & 31u) << 6) | (((rgba >> 11) & 31u) << 1) | ((rgba >> 7) & 1u);
}
static unsigned n_splits, n_grabs; /* B2: per frame, see grab_cur */
extern int pm_is_cur_cfb(const void* p);
/* B2: role T2. A 16 bit texture window inside the frame buffer being drawn (water ripples, crystal ball, bulb glow). Copy it into
 * the grab slot before the load, so that the load finds the slot like any off-screen image (T1). A window inside an older frame
 * buffer is T3: G_SETTIMG fetched it with pm_cfb_capture_prev. */
static void grab_cur(const void* src, int w, int h) {
    static int on = -1;
    int x, y, r;
    if (!src || w < 1 || !pm_is_cur_cfb(src)) return;
    x = (int)(((const u8*)src - (const u8*)nuGfxCfb_ptr) >> 1) % 320;
    y = (int)(((const u8*)src - (const u8*)nuGfxCfb_ptr) >> 1) / 320; /* rows of the window: a later load of other rows reuses the copy (R13c) */
    r = pm_gpu_grab(nuGfxCfb_ptr, y, y + (h < 1 ? 1 : h));
    if (r == 2) { n_grabs++; n_splits++; }
    if (on < 0) on = pm_debug_has("imglog");
}
static __attribute__((cold, noinline)) void imglog(const char* op, const void* a, int siz, int stride, int role) {
    static int on = -1;
    const char* kind;
    if (on < 0) on = pm_debug_has("imglog");
    if (!on) return;
    if (a == (const void*)DepthCopyBuffer) kind = "depthcopy";
    else if (!a) kind = "zero";
    else if (nuGfxZBuffer && (const unsigned short*)a >= nuGfxZBuffer && (const unsigned short*)a < nuGfxZBuffer + 320 * 240) kind = "zbuf";
    else if (pm_is_cfb(a)) kind = ((const unsigned short*)a >= nuGfxCfb_ptr && (const unsigned short*)a < nuGfxCfb_ptr + 320 * 240) ? "cfb_cur" : "cfb_prev";
    else { kind = "other"; if (role < 0) return; } /* settimg: log only frame buffer, z-buffer and depth copy reads */
}
static void off_screen_check(void) {
    static int nooff = -1;
    int r;
    if (nooff < 0) nooff = pm_debug_has("nooff");
    r = img_role(0);
    /* X02: an image that is not 16 bit (the darkness stencil: 32 bit, then 8 bit, in the z-buffer memory) always renders off-screen */
    if (!nooff && (r == ROLE_SLOT || r == ROLE_DEPTHQ)) pm_gpu_off_begin(cimg ? cimg : zb_fake, cimg_stride, cimg_siz);
}
static const void* tlut_pal[16];
static unsigned tile_tmem[8], tile_pal[8];
static int tile_sz[8][4];
static int sci_x0, sci_y0, sci_x1 = 320, sci_y1 = 240;
static unsigned envc = 0xffffffffu;
static unsigned comb2_a, comb2_b, comb2_c, comb2_d, comb2_aa, comb2_ab, comb2_ac, comb2_ad;
static unsigned comb_aa, comb_ab, comb_ac, comb_ad;
/* The sprite raster captured when tile 1 got its size. A shaded sprite sets two tiles on the
 * same 4-bit image (tile 0 reads the shading palette, tile 1 the sprite palette). */
static struct { int valid; const u8* src; int stride, w, h, x0, y0; } m1;
static int dual_on;
/* A texture tile as the RDP holds it. Tile 0 is the render tile. A combiner that reads only TEXEL1
 * samples tile 1 (draw_box takes its window body from tile 1). One texture is bound at a time,
 * so the tile the next draw needs is bound just before it. */
static struct {
    int valid;
    const u8* src;
    unsigned fmt, siz;
    int w, h, stride, x0, y0, pal_n;
    const void* pal;
} td[2];
static unsigned tile_wrap[8][4];
static unsigned tile_shift[8][2]; /* V12: G_SETTILE shifts (s, t) of each tile */
/* RDP shift field: 1..10 divide by 2^n, 11..15 multiply by 2^(16-n). */
static float shift_mul(unsigned n) {
    return n == 0 ? 1.f : n <= 10 ? 1.f / (float)(1 << n) : (float)(1 << (16 - n));
}
static int cur_tile;
static int use_t1;     /* the combiner reads TEXEL1 only: draw with tile 1 (TEXEL1 is renamed TEXEL0) */
static int comb_mode5; /* (PRIM - ENV) * TEXEL + ENV */
/* draw_box (H04): cycle 1 = TEXEL0 * TEXEL1 (tile 0 window background, tile 1 corner image),
 * alpha = TEXEL1 alpha, cycle 2 = (PRIM - ENV) * COMBINED + ENV. box_prod marks that form.
 * box_folded is 1 while it is drawn as (PRIM - ENV) * TEXEL1 + ENV, which is exact when tile 0
 * is all white (the flat background of the window styles, `bg_flat`). */
static int box_prod, box_folded;
/* R2B: comb_gen counts the writes of the combine state that the SETCOMBINE handler does not make (box_fold_sync toggles);
 * cmb_gen_seen is comb_gen at the last SETCOMBINE. */
static unsigned comb_gen, cmb_gen_seen;
static int box_pat; /* D03: (PRIM - ENV) * TEXEL0 + ENV with alpha from TEXEL1 only: window with a tile 0 pattern */
static unsigned box_save[4];
static float dual_gain[3];
static void unpack8(unsigned c, float o[4]);
static void ensure_tile(int n);
static void ens_clear(void);
static unsigned tile_fmt[8], tile_siz[8];
static u32 segbase[16];
static Vtx verts[64];
/* A display list may only draw vertices its own G_VTX loaded. Slots left by
 * the previous model otherwise connect the two meshes into long triangles. */
static u8 vert_live[64];
/* FX3f: a triangle was issued since the last G_VTX. A load at slot 0 only starts a new mesh then. aura loads slots 2,6 and then
 * 0,4 and draws 4,5,6,7, so the slot 0 load must keep the slots loaded just before it. */
static int tri_since_vtx;
/* B5: G_MODIFYVTX G_MWO_POINT_XYSCREEN sets the screen position of a loaded vertex (underwater warp). x, y in 10.2 fixed point. */
static u8 vert_scr[64];
static s16 vert_sx[64], vert_sy[64];
/* The RSP transforms a vertex when it is loaded. A later matrix does not move it. */
static unsigned char vert_cb[64]; /* DIP1: clip bits of vert_p, made once at G_VTX */
static float vert_p[64][4]; /* PO2: clip space x y z w of a vertex, transformed once when it is loaded (G_VTX) */
/* PO3a (switch po3a): counters for the go or no-go of PO3b. Matrix id = FNV-1a of the 16 bits of `combined` at each G_VTX. One log line per drawn frame. No rendering change. */
static int po3a_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("po3a"); return v; }
static unsigned vert_mid[64], mid_list[256], mid_n;
static unsigned p_cmd, p_mixed, p_vload, p_vcmd, p_lit, p_tgen, p_scr, p_drawn, p_primz, p_env, p_plan, p_a255, p_fog, p_boost, p_plain, p_plain2;
static int p_cur_mixed;
/* PO3b (switch gpuvtx, default off): plain triangles are drawn from the raw Vtx ring by the GPU transform. The CPU path keeps its checks (reject, cull, near clip) from the
 * per vertex transform of PO2, so the decisions are the same. Per slot: ring index, matrix table index, texture scale (s and t) at the load. */
#define GV_NONE 0xffffu
/* GV1 (switch gv1, default off, nogv1 turns it off): the full GPU geometry path. G_VTX of an unlit load copies the 16 byte Vtx to the ring and makes only the reject bits.
 * A triangle of ring vertices goes to the GPU as three indices. The GPU transforms, clips and culls. The CPU path stays for the rare cases and reads the ring copy
 * (gv_xf makes vert_p, vert_st and vert_c on demand). It replaces gpuvtx (PO3b) when both are on. */
static u8 vert_litf[64]; /* GV5: bit 0 = loaded lit, bit 1 = loaded lit with texgen, bit 2 = the lights changed since (the colour was made then): the CPU path only */
static u8 vert_gvf[64]; /* GV1: bit 0 = vert_p valid, bit 1 = vert_st and vert_c valid. 3 for every vertex the old code made. */
enum { GVR_SCR, GVR_MIXED, GVR_TSS, GVR_LIT, GVR_FOG, GVR_ZMARK, GVR_TEX1, GVR_DBG, GVR_REFUSE, GVR_LITLOAD, GVR_RING, GVR_N };
static unsigned gv_fb[GVR_N], gv_load_n, gv_tri_n, gv_rej_n, gv_xf_n, gv_chk_out, gv_chk_cull, gv_chk_tri;
static unsigned gp1_ld_in, gp1_ld_out, gp1_ld_cross, gp1_vx_in, gp1_vx_out, gp1_vx_cross, gp1_subset;
/* GP3 gate: tile 0 G_SETTILESIZE with td[0] set vs no surviving tri or tile-0 texrect before the next one. */
static unsigned gp3_t0_ld, gp3_t0_dead, gp3_t0_pending;
static void gp3_t0_set(void) {
    if (gp3_t0_pending) gp3_t0_dead++;
    gp3_t0_ld++;
    gp3_t0_pending = 1;
}
static void gp3_t0_surv(void) { gp3_t0_pending = 0; }
static float gv_rel_ref[2];
static unsigned gm_last = 0xffffu; /* GV1: the gm_tab entry of the last G_VTX */
static unsigned short vert_ring[64], vert_gmi[64];
static unsigned vert_tss[64];
static unsigned gm_steps; /* W1: entries compared by gv_matrix_id, summed over the report window */
static float (*gm_tab)[4][4]; /* matrices of the frame, 256 */
static float (*gm_nrm)[3][4]; /* GV5: the rows 0 to 2 of the model view matrix of each gm_tab entry (the normal matrix), made when the entry is made */
static unsigned gm_n;
static unsigned st_gpu_tri; /* triangles drawn by the raw path */
static unsigned st_vx, st_cor; /* PO2: vertices transformed at G_VTX, triangle corners read (the old code transformed one matrix product per corner) */
static u8 seen[256];
/* i = every corner in front of the eye, drawn where it projected.
 * c = a corner behind the eye was cut. d = nothing left in front.
 * x is the first vertex's N64 ndc * 100. */
static int n_cmd, n_in, n_cut, n_drop, n_out, n_keep, sample_x, have_sample;
static char stats_line[96];
static unsigned comb_a, comb_b, comb_c, comb_d;
static unsigned comb_w0, comb_w1; /* S25: raw G_SETCOMBINE words, for the combiner audit */
static int leg_mode;               /* S25: the combine mode sync_combine() last gave pm_gpu */
static int comb_set;
/* 1 when the alpha combiner reads shade alpha. DECAL and MODULATE modes do not, and imgfx
 * meshes leave cn[3] at 0, so passing it through made them vanish. */
static int comb_alpha_shade = 1;
extern char __end__[]; /* linker: end of .bss */
static int is_host(u32 p) {
    unsigned int heap = __ctru_heap;
    unsigned int hend = __ctru_heap + __ctru_heap_size;
    unsigned int lin = __ctru_linear_heap;
    unsigned int lend = lin + __ctru_linear_heap_size;
    if (p >= heap && p < hend) return 1;
    if (p >= lin && p < lend) return 1;
    if (p >= 0x00100000u && p < (u32)(uintptr_t)__end__) return 1;
    return 0;
}

/* PG3 A15: the host range that holds p (same three ranges as is_host). 0 when p is not host memory. */
static int host_range(u32 p, u32* lo, u32* hi) {
    u32 heap = __ctru_heap, hend = __ctru_heap + __ctru_heap_size, lin = __ctru_linear_heap, lend = lin + __ctru_linear_heap_size;
    if (p >= heap && p < hend) { *lo = heap; *hi = hend; return 1; }
    if (p >= lin && p < lend) { *lo = lin; *hi = lend; return 1; }
    if (p >= 0x00100000u && p < (u32)(uintptr_t)__end__) { *lo = 0x00100000u; *hi = (u32)(uintptr_t)__end__; return 1; }
    return 0;
}

static void* ptr_of(u32 w) {
    if (!w) return NULL;
    if (is_host(w)) return (void*)w;
    unsigned seg = (w >> 24) & 0xff;
    if (seg < 16 && segbase[seg]) return (void*)(segbase[seg] + (w & 0x00ffffffu));
    /* SW1: an N64 K0 or K1 address (OS_PHYSICAL_TO_K0 of a static buffer: the camera projection matrix of the credits parade) names host memory with the top bits set */
    if ((w & 0xc0000000u) == 0x80000000u && is_host(w & 0x1fffffffu)) return (void*)(w & 0x1fffffffu);
    return (void*)w;
}

static void ident(float m[4][4]) {
    memset(m, 0, sizeof(float) * 16);
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
}

static void mul(float c[4][4], const float a[4][4], const float b[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            c[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
        }
    }
}

static float elem(const Mtx* src, int row, int col) {
    const u32* m = (const u32*)src->m;
    int idx = row * 2 + (col >> 1);
    u32 sh = (col & 1) ? 0 : 16;
    s16 ip = (s16)((m[idx] >> sh) & 0xffff);
    s16 fp = (s16)((m[idx + 8] >> sh) & 0xffff);
    s32 fixed = ((s32)ip << 16) | (u16)fp;
    return (float)fixed / 65536.f;
}

static void unpack(const Mtx* src, float d[4][4]) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) d[r][c] = elem(src, r, c);
}

static void upload_mvp(void) {
    /* Row-vector matrices, same as transform_point: modelview first, then projection. */
    mul(combined, mv[mv_sp], proj);
    pm_gpu_note_mvp(combined, vp_sx, vp_sy, vp_tx, vp_ty); /* GV1c: the matrix is in the palette of the triangles */
}

/* Combiner input as a constant, or -1 when it needs a texel or the shade, -2 for COMBINED.
 * slot 0..3 is A, B, C, D of the colour equation. */
static float cc_const_in(int slot, unsigned sel, int ch, const float* pr, const float* en) {
    if (sel == 0) return -2.f;
    if (sel == 3) return pr[ch];
    if (sel == 5) return en[ch];
    if (slot == 0 && sel == 6) return 1.f;
    if (slot == 3 && sel == 6) return 1.f;
    if (slot == 3 && sel == 7) return 0.f;
    if (slot == 1 && sel == 7) return (float)conv_k4 / 255.f;
    if (slot == 1 && sel > 7) return 0.f;
    if (slot == 0 && sel >= 8) return 0.f;
    if (slot == 2 && sel == 10) return pr[3];
    if (slot == 2 && sel == 12) return en[3];
    if (slot == 2 && sel >= 16) return 0.f;
    return -1.f;
}

/* Alpha of one cycle as K * TEXEL0 alpha. Returns K, or -1 if the form is not that.
 * (T1 - T0) * E + T0 reads two textures. Tile 1 is not loaded, so TEXEL0 stands for both. */
static float cc_alpha_k(unsigned aa, unsigned ab, unsigned ac, unsigned ad, const float* pr, const float* en) {
    if (ab == 7 && ad == 7 && aa == 1 && (ac == 3 || ac == 5)) return ac == 3 ? pr[3] : en[3];
    if (ab == 7 && ad == 7 && ac == 1 && (aa == 3 || aa == 5)) return aa == 3 ? pr[3] : en[3];
    if (aa == 2 && ab == 1 && ad == 1 && (ac == 3 || ac == 5)) return 1.f;
    if (aa == 3 && ab == 0 && ac == 1 && ad == 0) return pr[3];
    return -1.f;
}

/* A combiner whose colour needs no texel and no shade, and whose alpha is a constant times
 * TEXEL0 alpha: a flat colour through a texture's alpha mask (a drop shadow is black this way).
 * Returns 1 and the RGBA constant, the alpha byte being the factor on texture alpha. */
static int const_rgb_tex_alpha(unsigned* out) {
    float pr[4], en[4], rgb[3], k, k2;
    int ch;
    int two = ((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 1;
    if (((omode_h >> G_MDSFT_CYCLETYPE) & 3) > 1 || !comb_set || !tex_on) return 0;
    unpack8(prim, pr);
    unpack8(envc, en);
    for (ch = 0; ch < 3; ch++) {
        float a, b, c, d;
        if (two) {
            a = cc_const_in(0, comb2_a, ch, pr, en); b = cc_const_in(1, comb2_b, ch, pr, en);
            c = cc_const_in(2, comb2_c, ch, pr, en); d = cc_const_in(3, comb2_d, ch, pr, en);
        } else {
            a = cc_const_in(0, comb_a, ch, pr, en); b = cc_const_in(1, comb_b, ch, pr, en);
            c = cc_const_in(2, comb_c, ch, pr, en); d = cc_const_in(3, comb_d, ch, pr, en);
            if (a == -2.f) a = 0.f; /* COMBINED is 0 in the only cycle */
            if (b == -2.f) b = 0.f;
            if (c == -2.f) c = 0.f;
            if (d == -2.f) d = 0.f;
        }
        if (a < 0.f || b < 0.f || c < 0.f || d < 0.f) return 0;
        rgb[ch] = (a - b) * c + d;
        rgb[ch] = rgb[ch] < 0.f ? 0.f : rgb[ch] > 1.f ? 1.f : rgb[ch];
    }
    if (comb_aa == 2 && td[1].valid) return 0; /* VIS3: (T1 - T0) * E + T0 reads tile 1 when it is loaded */
    k = cc_alpha_k(comb_aa, comb_ab, comb_ac, comb_ad, pr, en);
    if (k < 0.f) return 0;
    if (two) {
        /* Second cycle alpha: COMBINED * constant, or COMBINED passed through. */
        if (comb2_aa == 0 && comb2_ab == 7 && comb2_ad == 7 && (comb2_ac == 3 || comb2_ac == 5)) k2 = comb2_ac == 3 ? pr[3] : en[3];
        else if (comb2_aa == 7 && comb2_ab == 7 && comb2_ac == 7 && comb2_ad == 0) k2 = 1.f;
        else return 0;
        k *= k2;
    }
    *out = ((unsigned)(rgb[0] * 255.f + .5f) << 24) | ((unsigned)(rgb[1] * 255.f + .5f) << 16) |
           ((unsigned)(rgb[2] * 255.f + .5f) << 8) | (unsigned)(k * 255.f + .5f);
    return 1;
}

static void set_leg(int m) { leg_mode = m; }
static int plan_active; /* S25: the general combiner (mode 8) draws, not the legacy mode */
static PmPlan cur_plan;
static void sync_combine(void);
static void sync_combine_work(void);
static void sync_combine_work_timed(void) {
    if (!pm_ph_on || !ph_pick()) { sync_combine_work(); return; }
    unsigned t0 = pm_ticks();
    sync_combine_work();
    pm_gbi_sync_ticks += (pm_ticks() - t0) * 8u; /* W2: one call in 8 timed */
}
static int sl_const, sl_env; /* OPT-SC: the gpu calls of the last sync_legacy, kept in the memo */
static unsigned sl_cc;
static void sync_legacy(void) {
    unsigned cc;
    sl_const = sl_env = 0;
    if (const_rgb_tex_alpha(&cc)) {
        pm_gpu_set_const(cc);
        sl_const = 1; sl_cc = cc;
        set_leg(4);
        return;
    }
    /* Decal modes put TEXEL0 in the addend, which is mux d, not a/b/c.
     * Missing it draws the sprite quad's vertex colors: a grey gradient. */
    /* H06b1: with G_TEXTURE off the RSP sends a triangle with no S and T, the RDP reads texel (0, 0) of tile 0 (draw_tri sets that), so TEXEL0 is read whatever the flag says. */
    int wants_tex = (comb_a == G_CCMUX_TEXEL0 || comb_b == G_CCMUX_TEXEL0 ||
        comb_c == G_CCMUX_TEXEL0 || comb_d == G_CCMUX_TEXEL0 || comb_c == G_CCMUX_TEXEL0_ALPHA);
    int wants_prim = comb_a == G_CCMUX_PRIMITIVE || comb_b == G_CCMUX_PRIMITIVE || comb_c == G_CCMUX_PRIMITIVE;
    int mode = 0;
    /* Mode 3 = texture colour, but alpha only from the vertex: the alpha combiner does not read TEXEL0. */
    int alpha_tex = comb_aa == 1 || comb_ab == 1 || comb_ac == 1 || comb_ad == 1 ||
                    comb_aa == 2 || comb_ab == 2 || comb_ac == 2 || comb_ad == 2;
    comb_mode5 = 0;
    box_pat = box_prod && !box_folded; /* V10: the 2-cycle window with a pattern in tile 0 (not white): product on the CPU */
    if (wants_tex && comb_a == G_CCMUX_PRIMITIVE && comb_b == G_CCMUX_ENVIRONMENT && comb_c == G_CCMUX_TEXEL0 &&
        comb_d == G_CCMUX_ENVIRONMENT && alpha_tex && comb2_aa != 0) { /* FX2f: a cycle 2 alpha that reads COMBINED (huff_puff_breath, times PRIM) is no window pass */
        comb_mode5 = 1;
        pm_gpu_set_env(envc);
        sl_env = 1;
        /* D03: draw_box windows with the bg_tile pattern (tile 0, I4 0/F checker) take their alpha from
         * TEXEL1 (the corner image). Tile 0 alpha is the checker itself, so it must not become the alpha:
         * mode 7 draws the body opaque (the 8x8 rounded corner shape is lost). */
        {
            int a_t0 = comb_aa == 1 || comb_ab == 1 || comb_ac == 1 || comb_ad == 1;
            box_pat = !a_t0;
            set_leg(a_t0 ? 5 : 7);
        }
        return;
    }
    /* D03: (TEXEL0 - PRIM) * PRIM_ALPHA + PRIM, the pause screen background fade. */
    /* Texture rectangles read the texture whatever gSPTexture says, so tex_on is not tested. */
    if (comb_a == G_CCMUX_PRIMITIVE && comb_b == G_CCMUX_TEXEL0 && comb_c == G_CCMUX_PRIMITIVE_ALPHA &&
        comb_d == G_CCMUX_TEXEL0) {
        set_leg(6);
        return;
    }
    if (wants_tex) mode = alpha_tex ? 2 : 3;
    else if (wants_prim && !comb_set) mode = 1;
    else if (wants_prim && comb_a != G_CCMUX_SHADE) mode = 1;
    set_leg(mode);
}

/* sync_combine() is below, after the audit code it shares. */

/* All texels of tile 0 are 1.0 (I4 0xF or I8 0xFF)? Then TEXEL0 * TEXEL1 is TEXEL1. */
static int tile0_white(void) {
    const u8* src = td[0].src;
    if (!td[0].valid || !src || td[0].fmt != G_IM_FMT_I || td[0].siz > 1) return 0;
    for (int y = 0; y < td[0].h; y++) {
        for (int x = 0; x < td[0].w; x++) {
            if (td[0].siz == 0) {
                unsigned b = src[(y * td[0].stride + x) >> 1];
                if (((x & 1) ? (b & 15u) : (b >> 4)) != 15u) return 0;
            } else if (src[y * td[0].stride + x] != 255) {
                return 0;
            }
        }
    }
    return 1;
}

/* Called before each textured draw. Tiles 0 and 1 are known by then, the combine mode came
 * before them. */
static void box_fold_sync(void) {
    int want = box_prod && td[1].valid && tile0_white();
    if (want == box_folded) return;
    box_folded = want;
    comb_gen++;
    if (want) {
        box_save[0] = comb_a; box_save[1] = comb_b; box_save[2] = comb_c; box_save[3] = comb_d;
        comb_a = G_CCMUX_PRIMITIVE; comb_b = G_CCMUX_ENVIRONMENT; comb_c = G_CCMUX_TEXEL0; comb_d = G_CCMUX_ENVIRONMENT;
        comb_aa = 7; comb_ab = 7; comb_ac = 7; comb_ad = 1; /* TEXEL1 alpha, renamed TEXEL0 */
        use_t1 = 1;
    } else {
        comb_a = box_save[0]; comb_b = box_save[1]; comb_c = box_save[2]; comb_d = box_save[3];
        comb_aa = 7; comb_ab = 7; comb_ac = 7; comb_ad = 2;
        use_t1 = 0;
    }
    sync_combine();
}

/* Render mode bits of G_SETOTHERMODE_L, same reading as libultraship DeriveTriState: depth
 * compare and update come from Z_CMP and Z_UPD, a decal passes equal depth, blending is on when
 * the blender mixes with the frame buffer by (1 - alpha), texture edge and the alpha threshold
 * cut away near-transparent texels. */
static unsigned fogc, blendc; /* V06: G_SETFOGCOLOR and G_SETBLENDCOLOR, RGBA */
static int fog_fm, fog_fo, fogmix_on; /* F08: G_MW_FOG factor and offset, blender FOG/SHADE/IN/1MA active */
static unsigned sr_sig;
static int sr_ok;
static void sync_render(void) {
    unsigned sg = omode_l ^ (omode_h * 2654435761u) ^ geom ^ (unsigned)img_role(0) ^ (unsigned)PMD("rmoff");
    if (sr_ok && sg == sr_sig) {
        GP_SET(6, 1);
        return;
    }
    sr_sig = sg;
    sr_ok = 1;
    GP_SET(6, 0);
    unsigned m = omode_l;
    /* V06: the first blender cycle mixes the colour with the blend colour (m1a 2) or the fog colour (m1a 3) by the fog
     * colour alpha: (P * A + M * (1 - A)), P = BL or FOG, A = A_FOG, M = IN, B = 1MA. The room shroud (PM_RM_SHROUD) fades the
     * world to black this way. Other blenders are not touched. */
    {
        unsigned p = (m >> 30) & 3u, a = (m >> 26) & 3u, mm = (m >> 22) & 3u, b = (m >> 18) & 3u;
        if ((p == 2u || p == 3u) && a == 1u && mm == 0u && b == 0u)
            pm_gpu_set_mix(1, ((p == 2u ? blendc : fogc) & 0xffffff00u) | (fogc & 0xffu));
        else if (!PMDL("nofog") && p == 3u && a == 2u && mm == 0u && b == 0u) {
            /* F08: G_RM_FOG_SHADE_A, the weight is the shade alpha (the vertex fog factor, see draw_tri) */
            pm_gpu_set_mix(2, (fogc & 0xffffff00u) | 0xffu);
            fogmix_on = 1;
            goto mixed;
        } else
            pm_gpu_set_mix(0, 0);
        fogmix_on = 0;
    }
mixed:
    if (img_role(0) == ROLE_DEPTH) { /* B3: a model drawn into the z-buffer image is a depth write: no colour, always pass, constant depth (draw_tri) */
        pm_gpu_set_nocolor(1);
        pm_gpu_set_render(0, 1, 0, -1, 0);
        return;
    }
    if (PMD("rmoff")) { /* A/B check: the old always-blend, always-write state */
        pm_gpu_set_nocolor(0);
        pm_gpu_set_render(1, 1, 1, -1, 0);
        return;
    }
    int blend = ((m & (3u << 20)) == (1u << 20) && (m & (3u << 16)) == 0) ||
                ((m & (3u << 22)) == (1u << 22) && (m & (3u << 18)) == 0);
    int atest = -1;
    if (m & 0x1000u) { /* CVG_X_ALPHA, texture edge */
        if (blend) atest = 0x08;
        else atest = 0x30;
    } else if ((m & 3u) == 1u) { /* G_AC_THRESHOLD */
        atest = 0x08;
    }
    /* S25: a blender whose both colour inputs are the frame buffer colour (G_BL_CLR_MEM, cycle 1 bits 30 and 22,
     * cycle 2 bits 28 and 20) writes the old colour back: the draw changes depth only (a stencil pass, the
     * intro's white square around the Star Rod sparkle drew it as a coloured quad). */
    {
        int two = ((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 1;
        int nocol = two ? (((m >> 28) & 3u) == 1u && ((m >> 20) & 3u) == 1u) : (((m >> 30) & 3u) == 1u && ((m >> 22) & 3u) == 1u);
        pm_gpu_set_nocolor(nocol);
    }
    /* FX2a: two cycle only. The one cycle darkness stencil (CLD_SURF into an 8 bit slot) reads memory the port does not hold (the z buffer fill), it stays a write. */
    pm_gpu_set_memblend(((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 1 && ((m >> 20) & 3u) == 1u && ((m >> 16) & 3u) == 1u);
    pm_gpu_set_render((m & 0x10u) != 0, (m & 0x20u) != 0, blend, atest, (m & 0xc00u) == 0xc00u);
}

static unsigned rgba_from_5551(unsigned c) {
    unsigned r = (c >> 11) & 31;
    unsigned g = (c >> 6) & 31;
    unsigned b = (c >> 1) & 31;
    unsigned a = (c & 1) ? 255 : 0;
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    return (r << 24) | (g << 16) | (b << 8) | a;
}

/* L01: RSP light table. G_MW_NUMLIGHT sets the count, G_MOVEMEM index G_MV_LIGHT fills slots (slot 0 is light 1, slot nlights is the ambient light). */
static struct { u8 col[3]; s8 dir[3]; } lt[9];
static unsigned lt_n;
static s8 lk[2][3]; /* T01: look-at X and Y vectors (G_MOVEMEM slots -2 and -1) */
static unsigned tex_ss = 0xFFFF, tex_ts = 0xFFFF; /* T02: G_TEXTURE s and t scale, 0x10000 is 1.0 */
static float vert_st[64][2]; /* S and T in 10.5 units after the RSP scale or texgen, set when the vertex is loaded */
/* SUN3 debug (suntile switch): the tile 0 source of a draw, its state, and the first 16 bytes of rows 0 to 11 of the source. */
static __attribute__((cold, noinline)) void suntile_t0(const u8* src, int stride, int w, int h, int bpp, int uls, int ult, int lrs, int lrt) {
    if (!suntile_on() || suntile_lines >= 400) return;
    suntile_lines++;
    pm_log("suntile t0 f%u src %p str %d w %d h %d bpp %d fmt %u siz %u tmem %u win %d %d %d %d",
           pm_frame_count(), (const void*)src, stride, w, h, bpp, tile_fmt[0], tile_siz[0], tile_tmem[0], uls, ult, lrs, lrt);
    pm_log("suntile t0 timg %p tstr %d ld %d %d w %d h %d v %d", timg, timg_stride, ld_x, ld_y, ld_w, ld_h, ld_valid);
    if (!src) return;
    for (int r = 0; r < 12; r++) {
        unsigned q[4];
        if (suntile_lines >= 400) break;
        suntile_lines++;
        memcpy(q, src + (size_t)r * (size_t)stride * (bpp > 0 ? (size_t)bpp : 1u), 16); /* stride counts texels */
        pm_log("suntile t0 row %d %08x %08x %08x %08x", r, q[0], q[1], q[2], q[3]);
    }
}
static __attribute__((cold, noinline)) void suntile_tri(const char* path, const int* idx) {
    if (!suntile_on() || suntile_lines >= 400) return;
    suntile_lines++;
    pm_log("suntile tri %s f%u verts %d %d %d s %.3f %.3f %.3f t %.3f %.3f %.3f tss %08x",
           path, pm_frame_count(), idx[0], idx[1], idx[2],
           (double)vert_st[idx[0]][0], (double)vert_st[idx[1]][0], (double)vert_st[idx[2]][0],
           (double)vert_st[idx[0]][1], (double)vert_st[idx[1]][1], (double)vert_st[idx[2]][1], vert_tss[idx[0]]);
    if (path[0] == 'd') /* SUN3: the tile 1 state the draw uses */
        pm_log("suntile t1 draw fmt %u siz %u tmem %u sz %d %d %d %d td1 v %d src %p w %d h %d str %d x0 %d y0 %d tile %u",
               tile_fmt[1], tile_siz[1], tile_tmem[1], tile_sz[1][0], tile_sz[1][1], tile_sz[1][2], tile_sz[1][3],
               td[1].valid, (const void*)td[1].src, td[1].w, td[1].h, td[1].stride, td[1].x0, td[1].y0, cur_tile);
}
static int tg_ok;
static float tg_s, tg_t; /* T01: texgen result of the last vert_color call */
static unsigned vert_c[64]; /* lit or plain vertex colour, set when the vertex is loaded, as the RSP does */

/* L01: RSP lighting. The normal goes through the model view matrix, then colour = ambient + sum(light colour * max(0, normal . direction)). Alpha stays the vertex alpha.
 * Debug switch nolight keeps the old fixed light for an A/B. */
int pm_nolight = -1; /* F11-10: -1 = read the switch at first use, the Options row sets 0 or 1 */
static unsigned lit_color(const Vtx* v, const float m[4][4], int texgen) {
    tg_ok = 0;
    {
        float nx = pm_f127[(unsigned char)v->v.cn[0]];
        float ny = pm_f127[(unsigned char)v->v.cn[1]];
        float nz = pm_f127[(unsigned char)v->v.cn[2]];
        if (pm_nolight < 0) pm_nolight = pm_debug_has("nolight");
        if (pm_nolight) {
            float d = nx * 0.25f + ny * 0.85f + nz * 0.45f;
            if (d < 0.f) d = 0.f;
            if (d > 1.f) d = 1.f;
            unsigned c = (unsigned)((0.35f + 0.65f * d) * 255.f);
            return (c << 24) | (c << 16) | (c << 8) | 255u;
        }
        float tx = nx * m[0][0] + ny * m[1][0] + nz * m[2][0];
        float ty = nx * m[0][1] + ny * m[1][1] + nz * m[2][1];
        float tz = nx * m[0][2] + ny * m[1][2] + nz * m[2][2];
        float l2 = tx * tx + ty * ty + tz * tz;
        if (l2 > 0.f) { float k = 1.f / sqrtf(l2); tx *= k; ty *= k; tz *= k; }
        if (texgen) { /* T01: F3DEX2 texgen is in the lighting path. Non-linear: (dot + 1) / 4 * scale. G_TEXTURE_GEN_LINEAR has no user in src */
            float dx = (tx * lk[0][0] + ty * lk[0][1] + tz * lk[0][2]) / 127.f;
            float dy = (tx * lk[1][0] + ty * lk[1][1] + tz * lk[1][2]) / 127.f;
            tg_s = (dx + 1.f) / 4.f * (float)tex_ss;
            tg_t = (dy + 1.f) / 4.f * (float)tex_ts;
            tg_ok = 1;
        }
        float r = lt[lt_n].col[0], g = lt[lt_n].col[1], b = lt[lt_n].col[2];
        for (unsigned i = 0; i < lt_n; i++) {
            float d = (tx * lt[i].dir[0] + ty * lt[i].dir[1] + tz * lt[i].dir[2]) / 127.f;
            if (d <= 0.f) continue;
            r += lt[i].col[0] * d; g += lt[i].col[1] * d; b += lt[i].col[2] * d;
        }
        if (r > 255.f) r = 255.f;
        if (g > 255.f) g = 255.f;
        if (b > 255.f) b = 255.f;
        return ((unsigned)r << 24) | ((unsigned)g << 16) | ((unsigned)b << 8) | (unsigned)v->v.cn[3];
    }
}
static unsigned vert_color(const Vtx* v, const float m[4][4]) {
    tg_ok = 0;
    if (geom & G_LIGHTING) return lit_color(v, m, (geom & G_TEXTURE_GEN) != 0);
    return ((unsigned)v->v.cn[0] << 24) | ((unsigned)v->v.cn[1] << 16) |
           ((unsigned)v->v.cn[2] << 8) | (unsigned)v->v.cn[3];
}

static void vert_uv(const float* tc, float* u, float* vcoord) {
    int tw = 1, th = 1;
    pm_tex_size(&tw, &th);
    if (tw < 1) tw = 1;
    if (th < 1) th = 1;
    /* SetTileSize's origin is the start of the uploaded window. Absolute
     * vertex UVs have to be moved into that window, same as a texrect. */
    /* V12: the tile shift scales the coordinate before the tile origin is subtracted. */
    unsigned st = use_t1 ? 1 : 0;
    float ms = shift_mul(tile_shift[st][0]);
    float mt = shift_mul(tile_shift[st][1]);
    *u = (tc[0] / 32.f * ms - (float)tile_x0 - tile_fx) / (float)tw;
    *vcoord = (tc[1] / 32.f * mt - (float)tile_y0 - tile_fy) / (float)th;
}

typedef struct {
    float x, y, z, w, u, v, r, g, b, a;
} ClipV;

static unsigned pack_color(const ClipV* v) {
    int r = (int)v->r, g = (int)v->g, b = (int)v->b, a = (int)v->a;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (a < 0) a = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (a > 255) a = 255;
    return ((unsigned)r << 24) | ((unsigned)g << 16) | ((unsigned)b << 8) | (unsigned)a;
}

static const float ident_m[4][4] = {
    { 1.f, 0.f, 0.f, 0.f },
    { 0.f, 1.f, 0.f, 0.f },
    { 0.f, 0.f, 1.f, 0.f },
    { 0.f, 0.f, 0.f, 1.f },
};

/* The RSP multiplies when the vertex is loaded. Later matrices do not move it. */
static void xform_vtx(const float* p, const float* st, unsigned c, ClipV* o) {
    o->x = p[0];
    o->y = p[1];
    o->z = p[2];
    o->w = p[3];
    vert_uv(st, &o->u, &o->v);
    o->r = (float)((c >> 24) & 255u);
    o->g = (float)((c >> 16) & 255u);
    o->b = (float)((c >> 8) & 255u);
    o->a = (float)(c & 255u);
}

/* Cut only corners that passed behind the eye. A corner that is merely off the
 * side of the screen keeps its projected position, so a pan cannot swap the
 * triangle for a different polygon. */
/* DARK: a texrect that reads an off-screen image (64 texels) through a mirror + clamp + mask tile (the darkness stencil, tile 128 over a 64 image).
 * The RDP mirrors the window (period 2 * mask) and clamps outside +-2 * mask to texel 0. The PICA bound render texture has one wrap mode,
 * so the rect is cut at every mirror line and each part gets its own (flipped or constant) uv. a0 to a1 is the texel range over width pixels. */
typedef struct { int p0, p1; float u0, u1; } FoldSeg;
static int fold_axis(int plo, float width, float a0, float a1, float mw, int flip, int negclamp, FoldSeg* out) {
    float b[8];
    int nb = 0, n = 0, i;
    if (!(a1 > a0) || width < 1.f) return 0;
    b[nb++] = a0;
    for (i = -2; i <= 2; i++) if (i * mw > a0 && i * mw < a1) b[nb++] = i * mw;
    b[nb++] = a1;
    for (i = 0; i + 1 < nb; i++) {
        float x = b[i], y = b[i + 1], mid = (x + y) * 0.5f;
        FoldSeg* g = &out[n];
        g->p0 = plo + (int)((x - a0) / (a1 - a0) * width + 0.5f);
        g->p1 = plo + (int)((y - a0) / (a1 - a0) * width + 0.5f);
        if (g->p1 <= g->p0) continue;
        if (mid < -2.f * mw || mid >= 2.f * mw || (negclamp && mid < 0.f)) g->u0 = g->u1 = 0.f;
        else {
            int k = (int)floorf(mid / mw);
            if (k & 1) { g->u0 = ((k + 1) * mw - x) / mw; g->u1 = ((k + 1) * mw - y) / mw; }
            else { g->u0 = (x - k * mw) / mw; g->u1 = (y - k * mw) / mw; }
        }
        if (flip && !(mid < -2.f * mw || mid >= 2.f * mw || (negclamp && mid < 0.f))) { g->u0 = 1.f - g->u0; g->u1 = 1.f - g->u1; }
        n++;
    }
    return n;
}
/* Same bits as libultraship, except the near plane, which they leave commented out.
 * A triangle is dropped only when every corner is off the same side. */
static int clip_bits(const ClipV* p) {
    int b = 0;
    float w = p->w;
    if (p->x < -w) b |= 1;
    if (p->x > w) b |= 2;
    if (p->y < -w) b |= 4;
    if (p->y > w) b |= 8;
    if (p->z > w) b |= 32;
    return b;
}

/* ---- S25: combiner audit. Every distinct (combiner words, cycle type, render mode, texture format)
 * is checked once against the combine mode pm_gpu got. The check is numeric: the RDP equation
 * (A-B)*C+D per cycle, on random inputs, against what that mode draws. A difference is logged once
 * ("comb new") and then with a draw count at 1, 10, 100, ... draws ("comb count"). COMBINED is 0 in
 * cycle 1. LOD fraction, key and convert read 0. NOISE reads 0.5 (PT07). */
typedef struct { float t0[4], t1[4], sh[4], pr[4], en[4], ke[4], ks[4]; float k4; } CcIn;

static float cc_v(int slot, unsigned sel, int ch, const CcIn* in, const float* cb, int alpha, float prlod) {
    int rgb_c = !alpha && slot == 2;
    if (alpha) {
        switch (sel) {
        case 0: return slot == 2 ? 0.f : cb[3];
        case 1: return in->t0[3];
        case 2: return in->t1[3];
        case 3: return in->pr[3];
        case 4: return in->sh[3];
        case 5: return in->en[3];
        case 6: return slot == 2 ? prlod : 1.f;
        default: return 0.f;
        }
    }
    switch (sel) {
    case 0: return cb[ch];
    case 1: return in->t0[ch];
    case 2: return in->t1[ch];
    case 3: return in->pr[ch];
    case 4: return in->sh[ch];
    case 5: return in->en[ch];
    case 6: return slot == 1 ? in->ke[ch] : slot == 2 ? in->ks[ch] : slot == 0 || slot == 3 ? 1.f : 0.f;
    case 7: return slot == 1 ? in->k4 : slot == 0 ? 0.5f /* PT07: NOISE, the audit uses the mean */ : rgb_c ? cb[3] : 0.f;
    case 8: return rgb_c ? in->t0[3] : 0.f;
    case 9: return rgb_c ? in->t1[3] : 0.f;
    case 10: return rgb_c ? in->pr[3] : 0.f;
    case 11: return rgb_c ? in->sh[3] : 0.f;
    case 12: return rgb_c ? in->en[3] : 0.f;
    case 14: return rgb_c ? prlod : 0.f;
    default: return 0.f;
    }
}

/* s[cycle][0..7] = A B C D (colour), Aa Ab Ac Ad (alpha) */
static void cc_decode(unsigned w0, unsigned w1, unsigned s[2][8]) {
    s[0][0] = (w0 >> 20) & 0xf; s[0][1] = (w1 >> 28) & 0xf; s[0][2] = (w0 >> 15) & 0x1f; s[0][3] = (w1 >> 15) & 7;
    s[0][4] = (w0 >> 12) & 7;   s[0][5] = (w1 >> 12) & 7;   s[0][6] = (w0 >> 9) & 7;      s[0][7] = (w1 >> 9) & 7;
    s[1][0] = (w0 >> 5) & 0xf;  s[1][1] = (w1 >> 24) & 0xf; s[1][2] = w0 & 0x1f;          s[1][3] = (w1 >> 6) & 7;
    s[1][4] = (w1 >> 21) & 7;   s[1][5] = (w1 >> 3) & 7;    s[1][6] = (w1 >> 18) & 7;     s[1][7] = w1 & 7;
    pm_comb_swap_c2(s[1]);
}

static float cc_clamp(float v) { return v < 0.f ? 0.f : v > 1.f ? 1.f : v; }
/* (A-B)*C+D. RDP: the result is a 9 bit value, 129..255 below zero wraps to 255 and 1..128 below zero gives 0. The plan models this for
 * 0 - B*C only (pm_comb.c lower), so the reference applies it only there. Other negative results clamp to 0 as before. */
static float cc_rdp(float a, float b, float c, float d) {
    if (a == 0.f && d == 0.f && b * c > 0.f) return floorf(b * c * 255.f + .5f) >= 129.f ? 1.f : 0.f;
    return cc_clamp((a - b) * c + d);
}

/* The RDP result, one or two cycles. */
static void cc_eval(const unsigned s[2][8], int two, const CcIn* in, float prlod, float out[4]) {
    float cb[4] = { 0, 0, 0, 0 };
    for (int cyc = 0; cyc < (two ? 2 : 1); cyc++) {
        float r[4];
        for (int ch = 0; ch < 3; ch++) {
            float a = cc_v(0, s[cyc][0], ch, in, cb, 0, prlod), b = cc_v(1, s[cyc][1], ch, in, cb, 0, prlod);
            float c = cc_v(2, s[cyc][2], ch, in, cb, 0, prlod), d = cc_v(3, s[cyc][3], ch, in, cb, 0, prlod);
            r[ch] = cc_rdp(a, b, c, d);
        }
        {
            float a = cc_v(0, s[cyc][4], 3, in, cb, 1, prlod), b = cc_v(1, s[cyc][5], 3, in, cb, 1, prlod);
            float c = cc_v(2, s[cyc][6], 3, in, cb, 1, prlod), d = cc_v(3, s[cyc][7], 3, in, cb, 1, prlod);
            r[3] = cc_rdp(a, b, c, d);
        }
        memcpy(cb, r, sizeof cb);
    }
    memcpy(out, cb, sizeof cb);
}

/* What the legacy combine mode draws. */
static void cc_legacy(int mode, const CcIn* in, int t1src, float out[4]) {
    const float* t = t1src ? in->t1 : in->t0;
    float sa = comb_alpha_shade ? in->sh[3] : 1.f;
    unsigned cc = 0;
    switch (mode) {
    case 1: memcpy(out, in->pr, 4 * sizeof(float)); return;
    case 2: for (int i = 0; i < 3; i++) out[i] = t[i] * in->sh[i]; out[3] = t[3] * sa; return;
    case 3: for (int i = 0; i < 3; i++) out[i] = t[i] * in->sh[i]; out[3] = sa; return;
    case 4:
        if (const_rgb_tex_alpha(&cc)) {
            float c[4];
            unpack8(cc, c);
            out[0] = c[0]; out[1] = c[1]; out[2] = c[2]; out[3] = t[3] * c[3];
            return;
        }
        break;
    case 5: case 7:
        for (int i = 0; i < 3; i++) out[i] = (in->pr[i] - in->en[i]) * t[i] + in->en[i];
        out[3] = mode == 7 ? 1.f : t[3];
        return;
    case 6:
        for (int i = 0; i < 3; i++) out[i] = cc_clamp((t[i] - in->pr[i]) * in->pr[3] + in->pr[i]);
        out[3] = t[3];
        return;
    default: break;
    }
    memcpy(out, in->sh, 4 * sizeof(float));
}

static const char* const cc_nm_a[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "1", "NOISE" };
static const char* const cc_nm_b[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "KEYC", "K4" };
static const char* const cc_nm_c[16] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "KEYS", "CMBA", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "LODF", "PLODF", "K5" };
static const char* const cc_nm_d[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "1", "0" };
static const char* const cc_nm_aa[8] = { "CMBA", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "1", "0" };
static const char* const cc_nm_ac[8] = { "LODF", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "PLODF", "0" };

static void cc_text(const unsigned s[2][8], int cyc, char* o, int n) {
    snprintf(o, n, "(%s-%s)*%s+%s / (%s-%s)*%s+%s",
             s[cyc][0] < 8 ? cc_nm_a[s[cyc][0]] : "0", s[cyc][1] < 8 ? cc_nm_b[s[cyc][1]] : "0",
             s[cyc][2] < 16 ? cc_nm_c[s[cyc][2]] : "0", cc_nm_d[s[cyc][3]],
             cc_nm_aa[s[cyc][4]], cc_nm_aa[s[cyc][5]], cc_nm_ac[s[cyc][6]], cc_nm_aa[s[cyc][7]]);
}

/* 1 if the combiner reads texel tile t (0 or 1). */
static int cc_uses(const unsigned s[2][8], int two, int t) {
    int u = 0;
    for (int cyc = 0; cyc < (two ? 2 : 1); cyc++) {
        for (int i = 0; i < 4; i++) {
            unsigned v = s[cyc][i];
            if (v == (unsigned)t + 1 || (i == 2 && v == (unsigned)t + 8)) u = 1;
        }
        for (int i = 4; i < 8; i++)
            if (s[cyc][i] == (unsigned)t + 1) u = 1;
    }
    return u;
}

static unsigned st_lit, st_fogg, st_fogb, st_blc, st_tgen, st_tri; /* S25: draws that use lights, fog, blend colour, texgen */
static unsigned mm_cnt[256], mw_cnt[256];
static unsigned lcg_s = 12345u;
static float lcg_f(void) { lcg_s = lcg_s * 1664525u + 1013904223u; return (float)((lcg_s >> 8) & 0xffff) / 65535.f; }
static unsigned char aud_prim_lod;

/* Decision per combiner: the legacy combine mode, or the general plan (mode 8). Cached by the combiner words and
 * the flags the legacy mode depends on. The plan is chosen when the legacy mode is off by more than 0.012
 * (of 1.0, max over colour and alpha, 24 random inputs) and the plan is closer. */
typedef struct { unsigned key; int use, why; float el, ep; PmPlan plan; } Dec;
static Dec dec[128];
static int dec_n, dec_next; /* a full table replaces the oldest entry (a decision is recomputed in microseconds) */
static unsigned plan_draws, plan_fails;

static Dec* decide(int two) {
    unsigned key = 2166136261u, i;
    unsigned parts[] = { comb_w0, comb_w1, (unsigned)two, (unsigned)leg_mode, (unsigned)(tex_on | use_t1 << 1 | td[1].valid << 2), (unsigned)comb_alpha_shade };
    for (i = 0; i < sizeof parts / sizeof parts[0]; i++) key = (key ^ parts[i]) * 16777619u;
    {
        static unsigned last_i;
        if (last_i < (unsigned)dec_n && dec[last_i].key == key) return &dec[last_i];
        for (i = 0; i < (unsigned)dec_n; i++)
            if (dec[i].key == key) { last_i = i; return &dec[i]; }
    }
    {
        Dec* d;
        if (dec_n < 128) d = &dec[dec_n++];
        else { d = &dec[dec_next]; dec_next = (dec_next + 1) & 127; }
        unsigned s[2][8];
        float plod_f = (float)aud_prim_lod / 255.f;
        memset(d, 0, sizeof *d);
        d->key = key;
        cc_decode(comb_w0, comb_w1, s);
        if (leg_mode == 4) { d->why = -2; return d; } /* mode 4 is exact by construction (const_rgb_tex_alpha) */
        d->why = pm_comb_build(comb_w0, comb_w1, two, use_t1, &d->plan);
        if (!d->why && d->plan.uses_tex1 && !td[1].valid) d->why = 6; /* TEXEL1 read, tile 1 not loaded */
        {
            float el = 0.f, ep = 0.f;
            lcg_s = 777u;
            for (int k = 0; k < 24; k++) {
                CcIn in;
                float f[4], g[4], h[4];
                unsigned pc = 0, ec = 0, pcs[2];
                for (int c = 0; c < 4; c++) {
                    in.t0[c] = lcg_f(); in.t1[c] = lcg_f(); in.sh[c] = lcg_f();
                    pcs[0] = (unsigned)(lcg_f() * 255.f + .5f); pcs[1] = (unsigned)(lcg_f() * 255.f + .5f);
                    pc = pc << 8 | pcs[0]; ec = ec << 8 | pcs[1];
                }
                unpack8(pc, in.pr); unpack8(ec, in.en); unpack8(keyc, in.ke); unpack8(keys, in.ks); in.k4 = (float)conv_k4 / 255.f;
                cc_eval(s, two, &in, plod_f, f);
                cc_legacy(leg_mode, &in, use_t1, g);
                for (int c = 0; c < 4; c++) { float e = f[c] > g[c] ? f[c] - g[c] : g[c] - f[c]; if (e > el) el = e; }
                if (!d->why) {
                    PmPlan t = d->plan;
                    pm_comb_colours(&t, pc, ec, aud_prim_lod, keyc, keys);
                    pm_comb_eval(&t, use_t1 ? in.t1 : in.t0, in.t1, in.sh, h);
                    for (int c = 0; c < 4; c++) { float e = f[c] > h[c] ? f[c] - h[c] : h[c] - f[c]; if (e > ep) ep = e; }
                }
            }
            d->el = el;
            d->ep = d->why ? 9.f : ep;
            d->use = !d->why && el > 0.012f && ep < el;
        }
        return d;
    }
}

static int ls_use = -9, ls_why, ls_cyc, ls_dual, ls_frame;
static unsigned sc_sig; static int sc_ok; /* DIP2: the inputs of the last sync_combine and whether its result is still what pm_gpu holds */
/* OPT-SC: the result of sync_combine_work by the PG0 signature of its inputs (the same hash as gbiprof in sync_combine_work). A hit repeats the outputs and the calls into pm_gpu: no decision lookup, no
 * constant colour test, no plan colours. 16 entries, the oldest is replaced. */
typedef struct { unsigned sg; int ok, leg, mode5, pat, use, why, cyc, dual, plan_on, cc_on, env_on; unsigned cc; PmPlan plan; } ScMemo;
static ScMemo scm[16];
static unsigned scm_next;
static unsigned scm_sig(void) {
    unsigned parts[] = { comb_w0, comb_w1, omode_h, omode_l, prim, envc, (unsigned)aud_prim_lod, tile_fmt[0] * 4 + tile_siz[0],
        (unsigned)(tex_on | use_t1 << 1 | dual_on << 2 | box_prod << 3 | box_pat << 4 | comb_set << 5), geom };
    unsigned sg = 2166136261u;
    for (unsigned k = 0; k < sizeof parts / sizeof parts[0]; k++) sg = (sg ^ parts[k]) * 16777619u;
    return sg;
}
static ScMemo* scm_find(unsigned sg) {
    for (unsigned i = 0; i < 16; i++) if (scm[i].ok && scm[i].sg == sg) return &scm[i];
    return NULL;
}
static void scm_store(unsigned sg) {
    ScMemo* m = &scm[scm_next++ & 15];
    m->sg = sg; m->ok = 1; m->leg = leg_mode; m->mode5 = comb_mode5; m->pat = box_pat; m->use = ls_use; m->why = ls_why; m->cyc = ls_cyc; m->dual = ls_dual;
    m->plan_on = plan_active; m->cc_on = sl_const; m->cc = sl_cc; m->env_on = sl_env;
    if (plan_active) m->plan = cur_plan;
}
static void scm_replay(const ScMemo* m) {
    if (m->cc_on) pm_gpu_set_const(m->cc);
    if (m->env_on) pm_gpu_set_env(envc);
    leg_mode = m->leg; comb_mode5 = m->mode5; box_pat = m->pat;
    ls_use = m->use; ls_why = m->why; ls_cyc = m->cyc; ls_dual = m->dual; ls_frame = rm_frame;
    plan_active = m->plan_on;
    if (m->plan_on) { cur_plan = m->plan; pm_gpu_set_plan(&cur_plan); }
    else pm_gpu_set_combine(leg_mode);
}
/* R2B: the sync_combine input hash (the fields the skip test reads); sync_combine and the g2_same skip use the same value */
static unsigned sc_sig_now(void) {
    unsigned parts[] = { comb_w0, comb_w1, omode_h, prim, envc, (unsigned)aud_prim_lod, keyc, keys, conv_k4, (unsigned)leg_mode,
                         (unsigned)(tex_on | use_t1 << 1 | td[1].valid << 2 | dual_on << 3 | box_prod << 4 | box_pat << 5 | box_folded << 6 | comb_set << 7 | comb_alpha_shade << 8) };
    unsigned sg = 2166136261u;
    for (unsigned k = 0; k < sizeof parts / sizeof parts[0]; k++) sg = (sg ^ parts[k]) * 16777619u;
    return sg;
}
static void sync_combine(void) {
    {   /* DIP2: the same inputs as the call before give the same combine mode or plan. Skip the work. The inputs are everything sync_legacy, const_rgb_tex_alpha, decide and
         * pm_comb_colours read. pm_gpu_set_combine at the box pattern draw (below) and the start of a display list clear sc_ok. Switch nodip2 turns it off. */
        unsigned sg = sc_sig_now();
        if (sc_ok && sg == sc_sig) { GP_SET(5, 1); return; }
        sc_sig = sg;
        sc_ok = 0; sr_ok = 0; /* set again below when the work is done */
    }
    int leg0 = leg_mode, pat0 = box_pat;
    { unsigned sig = scm_sig(); ScMemo* m = scm_find(sig); if (m) scm_replay(m); else { sync_combine_work_timed(); scm_store(sig); } }
    sc_ok = 1;
    if (leg_mode != leg0 || box_pat != pat0) {   /* the work may change leg_mode and box_pat (inputs): the signature of the next call must start from the new value (PG4: no other input changes, so the hash is made again only then) */
        unsigned parts[] = { comb_w0, comb_w1, omode_h, prim, envc, (unsigned)aud_prim_lod, keyc, keys, conv_k4, (unsigned)leg_mode,
                             (unsigned)(tex_on | use_t1 << 1 | td[1].valid << 2 | dual_on << 3 | box_prod << 4 | box_pat << 5 | box_folded << 6 | comb_set << 7 | comb_alpha_shade << 8) };
        unsigned sg = 2166136261u;
        for (unsigned k = 0; k < sizeof parts / sizeof parts[0]; k++) sg = (sg ^ parts[k]) * 16777619u;
        sc_sig = sg;
    }
}
static void sync_combine_work(void) {
    if (gp_on()) { /* PG0: an estimate of how many calls repeat the inputs of the call before (hash of the combiner, render and tile state the function reads) */
        static unsigned last_sig;
        unsigned sg = scm_sig();
        GP_SET(5, sg == last_sig);
        last_sig = sg;
    }
    unsigned cyc = (omode_h >> G_MDSFT_CYCLETYPE) & 3;
    sync_legacy();
    plan_active = 0;
    ls_use = -1; ls_cyc = (int)cyc; ls_dual = dual_on | box_prod << 1 | box_pat << 2; ls_frame = rm_frame;
    if (cyc <= 1 && comb_set && !dual_on && (!box_prod || !box_folded)) { /* VIS1: a draw_box window with a pattern in tile 0 reads TEXEL0 * TEXEL1 on the GPU plan too. Triangles (rotated and scaled windows) have no CPU composite like box_pattern_rect, so they lost the corner image: no border, no round corners */
        Dec* d = decide((int)cyc);
        ls_use = d->use; ls_why = d->why;
        if (d->use) {
            cur_plan = d->plan;
            pm_comb_colours(&cur_plan, prim, envc, aud_prim_lod, keyc, keys);
            pm_gpu_set_plan(&cur_plan);
            plan_active = 1;
            return;
        }
    }
    pm_gpu_set_combine(leg_mode);
}

/* S25: a plan that reads TEXEL1. Tile 1 is loaded and kept at unit 1, tile 0 stays the current texture, and the
 * unit 1 coordinates are the first ones moved by the difference of the two tile windows. */
static unsigned aux_key;
static float t1_p[4]; /* the unit 1 uv map set by dual_bind: scale s t, offset s t */
static int t1_ox, t1_oy, t1_w, t1_h; /* window origin in the slot, texture size of unit 1 */
static void dual_bind(void) {
    unsigned key = 2166136261u, i;
    int tw0, th0, tw1, th1;
    if (!plan_active || !cur_plan.uses_tex1 || !td[0].valid || !td[1].valid) return;
    {
        unsigned parts[] = { (unsigned)td[1].src, td[1].fmt, td[1].siz, (unsigned)td[1].w, (unsigned)td[1].h, (unsigned)td[1].stride,
                             (unsigned)td[1].pal, tile_wrap[1][0], tile_wrap[1][1], tile_wrap[1][2], tile_wrap[1][3] };
        for (i = 0; i < sizeof parts / sizeof parts[0]; i++) key = (key ^ parts[i]) * 16777619u;
    }
    if (key != aux_key || !pm_tex_aux_valid()) {
        pm_gpu_flush();
        pm_tex_set_wrap(tile_wrap[1][0], tile_wrap[1][1], tile_wrap[1][2], tile_wrap[1][3]);
        if (td[1].fmt == 0 && td[1].siz == 2) grab_cur(td[1].src, td[1].w, td[1].h);
        pm_tex_load(td[1].src, td[1].fmt, td[1].siz, td[1].w, td[1].h, td[1].pal, td[1].pal_n, td[1].stride);
        pm_tex_set_aux(tile_wrap[1][0], tile_wrap[1][1], tile_wrap[1][2], tile_wrap[1][3]);
        aux_key = key;
        cur_tile = -1;
        ens_clear();
    }
    ensure_tile(0);
    pm_tex_size(&tw0, &th0);
    pm_tex_aux_size(&tw1, &th1);
    {   /* V12c: the two tiles may have different shifts (bulb glow: tile 0 shift 1, tile 1 shift 0) and TEXEL1 may be a slot with a window origin */
        int ox1, oy1;
        float rs = shift_mul(tile_shift[1][0]) / shift_mul(tile_shift[0][0]), rt = shift_mul(tile_shift[1][1]) / shift_mul(tile_shift[0][1]);
        pm_tex_aux_origin(&ox1, &oy1);
        t1_p[0] = (float)tw0 / (float)tw1 * rs;
        t1_p[1] = (float)th0 / (float)th1 * rt;
        t1_p[2] = ((float)tile_x0 * rs - (float)(td[1].x0 - ox1)) / (float)tw1;
        t1_p[3] = ((float)tile_y0 * rt - (float)(td[1].y0 - oy1)) / (float)th1;
        t1_ox = ox1; t1_oy = oy1; t1_w = tw1; t1_h = th1;
        pm_gpu_set_tex1(t1_p[0], t1_p[1], t1_p[2], t1_p[3]);
        if (suntile_on() && suntile_lines < 400) {
            suntile_lines++;
            pm_log("suntile t1 f%u uls %d ult %d lrs %d lrt %d | cms %u cmt %u masks %u maskt %u shifts %u %u | "
                   "t1 tex %dx%d fmt %u siz %u td.x0 %d td.y0 %d | tile_x0 %d tile_y0 %d fx %.2f fy %.2f | origin %d %d | "
                   "t0 tex %dx%d tile0 uls %d ult %d lrs %d lrt %d cms %u cmt %u masks %u maskt %u | "
                   "p %.5f %.5f %.5f %.5f | aux_off %d",
                   pm_frame_count(), tile_sz[1][0], tile_sz[1][1], tile_sz[1][2], tile_sz[1][3],
                   tile_wrap[1][0], tile_wrap[1][1], tile_wrap[1][2], tile_wrap[1][3], tile_shift[1][0], tile_shift[1][1],
                   tw1, th1, td[1].fmt, td[1].siz, td[1].x0, td[1].y0, tile_x0, tile_y0, (double)tile_fx, (double)tile_fy, ox1, oy1,
                   tw0, th0, tile_sz[0][0], tile_sz[0][1], tile_sz[0][2], tile_sz[0][3],
                   tile_wrap[0][0], tile_wrap[0][1], tile_wrap[0][2], tile_wrap[0][3],
                   (double)t1_p[0], (double)t1_p[1], (double)t1_p[2], (double)t1_p[3], pm_tex_aux_off_idx());
        }
    }
}

/* R13b: a tile 1 window that wraps (G_TX_WRAP, mask bits) is read at (s - sl) & (size - 1). The draw may start outside the window
 * (water_splash: window origin 65, droplet s from 0, so the RDP reads the window at 31, 0, 1 ...). dual_bind maps s linearly, which
 * reads the slot far from the window and finds nothing grabbed there. Fold the start of this draw into the window. The draw is
 * kept inside the window when it fits. The wrap inside one draw is not reproduced, the part past the window edge reads the
 * neighbour pixels of the slot. Tile 0 uv range of the draw: u0..u1 and v0..v1. */
static void t1_fold(float u0, float u1, float v0, float v1) {
    static int foldall = -1; /* V20: switch t1foldall restores the old behaviour (fold every TEXEL1) */
    if (foldall < 0) foldall = pm_debug_has("t1foldall");
    if (!foldall && pm_tex_aux_off_idx() < 0) return; /* an ordinary texture: the GPU repeat and mirror give the RDP wrap, the fold moved the start by the mask size (wrong for a mirrored axis, period twice the mask) */
    float o[2] = { t1_p[2], t1_p[3] };
    const float lo[2] = { u0, v0 }, ex[2] = { (u1 - u0) * t1_p[0] * (float)t1_w, (v1 - v0) * t1_p[1] * (float)t1_h };
    const float sc[2] = { t1_p[0], t1_p[1] };
    const int org[2] = { t1_ox, t1_oy }, dim[2] = { t1_w, t1_h };
    int changed = 0;
    for (int a = 0; a < 2; a++) {
        unsigned mask = tile_wrap[1][2 + a];
        float win, ws, wsz, wrapped;
        if (!mask || mask > 9) continue;
        if (tile_wrap[1][a] & 2u) continue; /* V21: a CLAMP axis has no wrap or mirror: the RDP reads (s - sl) clamped to the window, the linear map of dual_bind is right. Folding moved the star icons on the Bowser cards by 9 texels. */
        wsz = (float)(1u << mask);
        win = (lo[a] * sc[a] + o[a]) * (float)dim[a] - (float)org[a]; /* window position of the draw start, texels */
        if (win >= 0.f && win < wsz) continue;
        ws = win - wsz * floorf(win / wsz);
        { float lim = ex[a] >= wsz && (float)dim[a] > wsz ? (float)dim[a] : wsz; /* FX2a: a draw over the whole window (flame noise scroll) reads the wrapped pad of the texture */
          wrapped = ws + ex[a] > lim ? (lim - ex[a] > 0.f ? lim - ex[a] : 0.f) : ws; }
        o[a] += (wrapped - win) / (float)dim[a];
        changed = 1;
    }
    if (changed) pm_gpu_set_tex1(t1_p[0], t1_p[1], o[0], o[1]);
}

/* Clip positions, not object positions. The identity matrix leaves x,y,z,w
 * alone and the viewport places them. */
static const float st_off[2] = { 0.f, 0.f }; /* H06b1 */
/* PERF-GBI (switch gbiprof, default off): cycle laps inside draw_tri, a tick read costs 1.3 us in Azahar so the totals are inflated, the ranking holds. One load and one branch when off. */
static unsigned gp_acc[8], gp_last, gp_ntri, gp_frames;
static unsigned gp_opt[256], gp_opn[256], gp_opt_last; static int gp_prev_op = -1; /* time between two ops, charged to the first */
enum { GPL_A, GPL_ENSURE, GPL_DUAL, GPL_EARLY, GPL_SYNC, GPL_EMIT, GPL_OLD, GPL_N };
#define GP_START() do { if (gp_on()) gp_last = pm_ticks(); } while (0)
#define GP_LAP(id) do { if (gp_on()) { unsigned n_ = pm_ticks(); gp_acc[id] += n_ - gp_last; gp_last = n_; } } while (0)

static int clip_bits4(const float* p) { /* the same bits as clip_bits, from a clip position x y z w */
    int b = 0;
    float w = p[3];
    if (p[0] < -w) b |= 1;
    if (p[0] > w) b |= 2;
    if (p[1] < -w) b |= 4;
    if (p[1] > w) b |= 8;
    if (p[2] > w) b |= 32;
    return b;
}
static int cull_off(void) { static int v = -1; if (v < 0) v = pm_debug_has("nocull"); return v; }
static int rmlog_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("rmlog"); return v; }

/* PO3b, PO3c: one triangle of the raw Vtx path. The caller checked that the three vertices have a ring slot, one matrix and one texture scale, and that
 * the triangle is plain (unlit, no texgen, no screen vertex, no fog). The colour and depth overrides of the CPU path become uniforms: ENV or the plan
 * constant replace the colour, alpha 255, the boost of the second cycle, and G_ZS_PRIM replaces the depth. Returns 0 when the path refuses. */
typedef struct { float uvso[4], vp[4], cm[4], ca[4], zm[2]; int boost; PmFx fx; int fxon; } GvP;
/* The draw parameters of a raw triangle from the state and the texture scale `tss` of its vertices. */
static void gv_params_calc(unsigned tss, GvP* o) {
    int env = comb_mode5 && !plan_active;
    int plan = plan_active && cur_plan.vtxname != PM_NM_SHADE && cur_plan.vtxname != PM_NM_NONE;
    int a255 = !plan_active && !plan && !comb_alpha_shade;
    int boost = dual_on && comb2_a == G_CCMUX_SHADE && comb2_b == G_CCMUX_ENVIRONMENT && comb2_c == G_CCMUX_COMBINED && comb2_d == G_CCMUX_COMBINED;
    int primz = (omode_l & (1u << G_MDSFT_ZSRCSEL));
    int tw = 1, th = 1;
    pm_tex_size(&tw, &th);
    if (tw < 1) tw = 1;
    if (th < 1) th = 1;
    unsigned st = use_t1 ? 1 : 0;
    float ms = shift_mul(tile_shift[st][0]), mt = shift_mul(tile_shift[st][1]);
    float en[4];
    o->vp[0] = vp_sx; o->vp[1] = vp_sy; o->vp[2] = vp_tx; o->vp[3] = vp_ty;
    for (int q = 0; q < 4; q++) { o->cm[q] = 1.f; o->ca[q] = 0.f; }
    o->zm[0] = 1.f; o->zm[1] = 0.f;
    unpack8(envc, en);
    if (tex_on) {
        float itw = 1.f / (float)tw, ith = 1.f / (float)th; /* H5: one divide per axis, not one per term */
        o->uvso[0] = (float)(tss >> 16) / 65536.f / 32.f * ms * itw;
        o->uvso[1] = (float)(tss & 0xffffu) / 65536.f / 32.f * mt * ith;
        o->uvso[2] = -((float)tile_x0 + tile_fx) * itw;
        o->uvso[3] = -((float)tile_y0 + tile_fy) * ith;
    } else { /* H06b1: the vertex texcoords are 0, texel (0, 0) centre */
        o->uvso[0] = o->uvso[1] = 0.f;
        o->uvso[2] = -((float)tile_x0 + tile_fx) / (float)tw + 0.5f / (float)tw;
        o->uvso[3] = -((float)tile_y0 + tile_fy) / (float)th + 0.5f / (float)th;
    }
    if (env) { for (int q = 0; q < 4; q++) { o->cm[q] = 0.f; o->ca[q] = en[q]; } } /* the CPU order: ENV or the plan constant, then alpha 255, then boost */
    else if (plan) { float vc[4]; unpack8(cur_plan.vtx, vc); for (int q = 0; q < 4; q++) { o->cm[q] = 0.f; o->ca[q] = vc[q]; } }
    if (a255) { o->cm[3] = 0.f; o->ca[3] = 1.f; }
    if (boost) for (int q = 0; q < 3; q++) { o->ca[q] = o->ca[q] * 0.5f + (1.f - en[q]) * 0.5f; o->cm[q] *= 0.5f; }
    if (primz) { o->zm[0] = 0.f; o->zm[1] = prim_ndc_z(); }
    o->boost = boost;
    o->fxon = 0; /* o->fx is read only when fxon: it is cleared where it is filled */
    if (geom & G_LIGHTING) { /* GV5: the RSP light table (ambient is slot lt_n), the look-at vectors for texgen */
        memset(&o->fx, 0, sizeof o->fx);
        o->fx.lflag[0] = 1.f;
        if (geom & G_TEXTURE_GEN) o->fx.lflag[1] = 1.f;
        for (int q = 0; q < 3; q++) o->fx.amb[q] = (float)lt[lt_n].col[q];
        for (unsigned i = 0; i < lt_n && i < 7; i++)
            for (int q = 0; q < 3; q++) { o->fx.col[i][q] = (float)lt[i].col[q]; o->fx.dir[i][q] = (float)lt[i].dir[q] / 127.f; }
        for (int q = 0; q < 3; q++) { o->fx.lk0[q] = (float)lk[0][q] / 127.f; o->fx.lk1[q] = (float)lk[1][q] / 127.f; }
        o->fxon = 1;
    }
    if (fogmix_on && (geom & G_FOG)) { /* GV5: the RSP fog factor clamp(z / w * fm + fo) is the alpha. z / w of the N64 clip position from the PICA clip z and w. */
        float za = 1.f, zb = 0.f;
        if (pm_gpu_gv_zcoef(&za, &zb)) {
            if (!o->fxon) memset(&o->fx, 0, sizeof o->fx);
            o->fx.lflag[2] = 1.f;
            o->fx.fogu[0] = (float)fog_fm / za;
            o->fx.fogu[1] = (float)fog_fo - (float)fog_fm * zb / za;
            o->cm[3] = 1.f; o->ca[3] = 0.f; /* the fog factor replaces the alpha, after the alpha 255 rule */
            o->fxon = 1;
        }
    }
}

static unsigned tri_flags(void) {
    unsigned f = 0, bl = omode_l;
    int two = ((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 1;
    unsigned a1 = (bl >> 30) & 3, a2 = (bl >> 22) & 3, b1 = (bl >> 28) & 3, b2 = (bl >> 20) & 3;
    if (geom & G_LIGHTING) f |= 1;
    if (geom & G_FOG) f |= 2;
    if (geom & G_TEXTURE_GEN) f |= 4;
    if (two ? (b1 == 3 || b2 == 3 || a1 == 3 || a2 == 3) : (a1 == 3 || a2 == 3)) f |= 8;
    if (two ? (b1 == 2 || b2 == 2 || a1 == 2 || a2 == 2) : (a1 == 2 || a2 == 2)) f |= 16;
    return f;
}
static void tri_stats_add(unsigned f, unsigned n) {
    st_tri += n;
    if (f & 1) st_lit += n;
    if (f & 2) st_fogg += n;
    if (f & 4) st_tgen += n;
    if (f & 8) st_fogb += n;
    if (f & 16) st_blc += n;
}
static void tri_stats(void) { tri_stats_add(tri_flags(), 1); }

/* GV1: make what the CPU path reads from the ring copy of a vertex: the clip position with its exact reject bits, S and T, the colour. */
static void gv_xf(int sl) {
    if (vert_gvf[sl] == 3) return;
    const Vtx* r = (const Vtx*)(pm_gpu_gring() + 16u * vert_ring[sl]);
    if (!(vert_gvf[sl] & 1)) {
        const float(*m)[4] = gm_tab[vert_gmi[sl]];
        float x = (float)r->v.ob[0], y = (float)r->v.ob[1], z = (float)r->v.ob[2];
        float* p = vert_p[sl];
        p[0] = m[0][0] * x + m[1][0] * y + m[2][0] * z + m[3][0];
        p[1] = m[0][1] * x + m[1][1] * y + m[2][1] * z + m[3][1];
        p[2] = m[0][2] * x + m[1][2] * y + m[2][2] * z + m[3][2];
        p[3] = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];
        vert_cb[sl] = (unsigned char)clip_bits4(p);
        gv_xf_n++;
    }
    {
        unsigned tss = vert_tss[sl];
        vert_st[sl][0] = (float)r->v.tc[0] * (float)(tss >> 16) / 65536.f;
        vert_st[sl][1] = (float)r->v.tc[1] * (float)(tss & 0xffffu) / 65536.f;
        if (vert_litf[sl] & 3) { /* GV5: a vertex that was loaded lit goes to the GPU raw, the CPU path makes its colour from the stored normal matrix (the lights are the ones of the load: lit_freeze) */
            vert_c[sl] = lit_color(r, gm_nrm[vert_gmi[sl]], (vert_litf[sl] & 2) != 0);
            if (tg_ok) { vert_st[sl][0] = tg_s; vert_st[sl][1] = tg_t; }
        } else
        vert_c[sl] = ((unsigned)r->v.cn[0] << 24) | ((unsigned)r->v.cn[1] << 16) | ((unsigned)r->v.cn[2] << 8) | (unsigned)r->v.cn[3];
    }
    vert_gvf[sl] = 3;
}

/* GV5: the lights are about to change (G_MOVEMEM light, G_MW_NUMLIGHT). A lit vertex that is still in the ring has not seen its colour yet: make it now with the old lights and keep it on the CPU path. */
static int lit_pending; /* a lit vertex was loaded to the ring since the last lit_freeze */
static void lit_freeze(void) {
    if (!lit_pending) return; /* no vertex was loaded lit since the last freeze */
    lit_pending = 0;
    for (int s = 0; s < 64; s++) {
        if (!vert_live[s] || !(vert_litf[s] & 3) || (vert_litf[s] & 4) || vert_ring[s] == GV_NONE) continue;
        if (vert_gvf[s] != 3) gv_xf(s);
        vert_litf[s] |= 4;
    }
}

/* H3: open-addressing index of gm_tab by a hash of the combined matrix. Slot holds entry + 1, 0 is empty. 512 slots for at most 256 entries. */
static unsigned short gm_idx[512];
static unsigned gm_hash(const float (*m)[4]) {
    const unsigned* w = (const unsigned*)m;
    unsigned h = 2166136261u;
    for (int i = 0; i < 16; i++) h = (h ^ w[i]) * 16777619u;
    return (h ^ (h >> 16)) & 511u;
}

/* GV1: the id of the matrix of the next vertex load or static group in gm_tab: the combined matrix, and for a lit load also the model view matrix of the normal. -1 when the table is full. */
static unsigned gv_matrix_id(int need_nm) {
    if (!gm_tab) {
        gm_tab = (float(*)[4][4])malloc(256 * sizeof(float[4][4]));
        gm_nrm = (float(*)[3][4])malloc(256 * sizeof(float[3][4]));
        if (!gm_tab || !gm_nrm) { free(gm_tab); free(gm_nrm); gm_tab = NULL; gm_nrm = NULL; return GV_NONE; }
        pm_gpu_set_ntab((const float(*)[3][4])gm_nrm);
    }
    if (gm_last != GV_NONE && gm_last < gm_n && memcmp(gm_tab[gm_last], combined, sizeof(float[4][4])) == 0 && !need_nm) return gm_last;
    float nm[3][4];
    for (int i = 0; i < 3; i++) { nm[i][0] = mv[mv_sp][i][0]; nm[i][1] = mv[mv_sp][i][1]; nm[i][2] = mv[mv_sp][i][2]; nm[i][3] = 0.f; }
    if (gm_last != GV_NONE && gm_last < gm_n && memcmp(gm_tab[gm_last], combined, sizeof(float[4][4])) == 0 && memcmp(gm_nrm[gm_last], nm, sizeof nm) == 0) return gm_last;
    unsigned p, best = GV_NONE, gmi;
    /* H3: the newest entry with the same combined matrix (and normal when needed), found by probing its hash chain */
    for (p = gm_hash(combined); gm_idx[p]; p = (p + 1) & 511u) {
        unsigned e = gm_idx[p] - 1u;
        gm_steps++;
        if ((best == GV_NONE || e > best) && memcmp(gm_tab[e], combined, sizeof(float[4][4])) == 0 && (!need_nm || memcmp(gm_nrm[e], nm, sizeof nm) == 0)) best = e;
    }
    if (best != GV_NONE) gmi = best;
    else if (gm_n < 256) { memcpy(gm_tab[gm_n], combined, sizeof(float[4][4])); memcpy(gm_nrm[gm_n], nm, sizeof nm); gm_idx[p] = (unsigned short)(gm_n + 1); gmi = gm_n++; }
    else gmi = GV_NONE;
    gm_last = gmi;
    return gmi;
}

/* GV5: the fog factor can be made on the GPU */
/* GV5: a lit load can go to the ring: the program is there and the Options row for the fixed light is off */
static int gv_lit_ok(void) { if (pm_nolight < 0) pm_nolight = pm_debug_has("nolight"); return pm_gpu_gv5_ok() && !pm_nolight; }
static int gv_fog_gpu_ok(void) { return pm_gpu_gv5_ok() && pm_gpu_gv_zcoef(NULL, NULL); }
static int gv1chk_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("gv1check"); return v; }

/* GV1b: state epoch. gv_epoch goes up at every display list op that is not a vertex load, a triangle, a matrix op or a list jump. The decision that depends on state
 * only (fallback reason, cull mode, the combiner, texture and render sync, the draw parameters) is made once per epoch and per texture scale, not per triangle. */
static unsigned gv_epoch = 1, ep_epoch, ep_osc, ep_fl, ep_n, ep_pepoch, ep_ptss, ep_open_gen, ep_open_epoch, ep_open_tss, ep_open_gmi;
static int ep_why, ep_ct, ep_cullboth, ep_open;
static GvP ep_p;
static unsigned gv_chk_stale;
static void ep_flush_stats(void) { if (ep_n) { tri_stats_add(ep_fl, ep_n); ep_n = 0; } }
/* what the state alone says: a reason to leave the GPU path (or -1), the cull type, CULL_BOTH */
static int gv_state_why(int* ct, int* both) {
    unsigned cg = geom & G_CULL_BOTH;
    *ct = cull_off() ? 0 : cg == G_CULL_FRONT ? 1 : cg == G_CULL_BACK ? 2 : 0;
    *both = cg == G_CULL_BOTH && !cull_off();
    if ((geom & G_LIGHTING) && !gv_lit_ok()) return GVR_LIT;
    if (fogmix_on && (geom & G_FOG) && !gv_fog_gpu_ok()) return GVR_FOG;
    if (img_role(0) == ROLE_DEPTH) return GVR_ZMARK;
    if (dd_on() || xt_on() || rmlog_on() || po3a_on()) return GVR_DBG;
    return -1;
}

/* the first triangle (or DLC2 range) after a state change */
static void gv_epoch_enter(void) {
    ep_flush_stats();
    ep_why = gv_state_why(&ep_ct, &ep_cullboth);
    if (ep_why < 0 && !ep_cullboth) {
        ep_fl = tri_flags();
        ensure_tile(use_t1 ? 1 : 0);
        dual_bind();
        sync_render();
        /* TEXEL1: the GPU reads tile 1 through the second texture unit and the uv map of dual_bind (uvx). A TEXEL1 that is an off-screen slot needs the fold of the window (t1_fold) per draw: the CPU path. */
        if (plan_active && cur_plan.uses_tex1 && (!td[0].valid || !td[1].valid || pm_tex_aux_off_idx() >= 0)) ep_why = GVR_TEX1;
    }
    ep_epoch = gv_epoch;
    ep_pepoch = 0;
}

/* GV1: one triangle of ring vertices. Returns 1 when it is done (rejected or sent to the GPU). Returns 0 when the CPU path must take it (reason in gv_fb). */
static int gv_tri(const int* idx) {
    int a = idx[0], b = idx[1], c = idx[2];
    int why = -1;
    if ((vert_cb[a] & vert_cb[b] & vert_cb[c]) & 0x2f) { n_out++; gv_rej_n++; PH_ADD(C_GVREJ, 1); GP_LAP(GPL_EARLY); return 1; }
    if ((vert_cb[a] | vert_cb[b] | vert_cb[c]) & 0x2f) {
        if (!(vert_gvf[a] & 1) && vert_cb[a]) gv_xf(a);
        if (!(vert_gvf[b] & 1) && vert_cb[b]) gv_xf(b);
        if (!(vert_gvf[c] & 1) && vert_cb[c]) gv_xf(c);
        if ((vert_cb[a] & vert_cb[b] & vert_cb[c]) & 0x2f) { gp1_subset++; n_out++; gv_rej_n++; PH_ADD(C_GVREJ, 1); GP_LAP(GPL_EARLY); return 1; }
    }
    if (vert_scr[a] | vert_scr[b] | vert_scr[c]) why = GVR_SCR;
    else if (vert_tss[a] != vert_tss[b] || vert_tss[a] != vert_tss[c]) why = GVR_TSS;
    else if ((vert_litf[a] | vert_litf[b] | vert_litf[c]) || (geom & G_LIGHTING)) { /* GV5: the vertices were loaded with the lighting state of now, and the lights did not change since (the RSP lights at the load) */
        unsigned want = (geom & G_LIGHTING) ? 1u | ((geom & G_TEXTURE_GEN) ? 2u : 0u) : 0u;
        if (((vert_litf[a] | vert_litf[b] | vert_litf[c]) & 4) || vert_litf[a] != want || vert_litf[b] != want || vert_litf[c] != want) why = GVR_LIT;
    }
    if (why < 0) {
        if (ep_epoch != gv_epoch) gv_epoch_enter(); /* the first triangle after a state change */
        else if (gv1chk_on()) { /* the stored decision against a fresh one */
            int ct, both, w = gv_state_why(&ct, &both);
            if ((w != ep_why && ep_why != GVR_TEX1) || ct != ep_ct || both != ep_cullboth) gv_chk_stale++;
        }
        why = ep_why;
    }
    suntile_tri(why < 0 ? "gpu" : "cpu-pending", idx);
    if (why < 0) {
        unsigned tss = vert_tss[a];
        if (ep_cullboth) { n_out++; GP_LAP(GPL_EARLY); return 1; }
        if (gv1chk_on()) { /* Azahar check: the old rule and the old cull for this triangle, against the GPU rule */
            gv_xf(a); gv_xf(b); gv_xf(c);
            const float *pa = vert_p[a], *pb = vert_p[b], *pc = vert_p[c];
            float ex1 = pa[0] * pb[3] - pb[0] * pa[3], ey1 = pa[1] * pb[3] - pb[1] * pa[3];
            float ex2 = pc[0] * pb[3] - pb[0] * pc[3], ey2 = pc[1] * pb[3] - pb[1] * pc[3];
            float cross = (ex1 * ey2 - ey1 * ex2) * (pa[3] * pc[3]);
            if ((pa[3] < 0) ^ (pb[3] < 0) ^ (pc[3] < 0)) cross = -cross;
            int old_cull = ep_ct == 1 ? cross <= 0 : ep_ct == 2 ? cross >= 0 : 0;
            int old_rej = (vert_cb[a] & vert_cb[b] & vert_cb[c]) & 0x2f;
            gv_chk_tri++;
            if (!old_rej && !old_cull) gv_chk_out++;
            if (pa[3] > 0.05f && pb[3] > 0.05f && pc[3] > 0.05f) { /* window orientation from the matrix after the game matrix */
                float qa[4], qb[4], qc[4];
                pm_gpu_gv_post(pa, qa); pm_gpu_gv_post(pb, qb); pm_gpu_gv_post(pc, qc);
                float ox = (qb[0] / qb[3] - qa[0] / qa[3]) * (qc[1] / qc[3] - qa[1] / qa[3]) - (qb[1] / qb[3] - qa[1] / qa[3]) * (qc[0] / qc[3] - qa[0] / qa[3]);
                if (cross != 0.f && ox != 0.f) {
                    int k = pm_gpu_gv_wind() > 0, rel = (cross > 0.f) == (ox > 0.f);
                    if (gv_rel_ref[k] == 0.f) gv_rel_ref[k] = rel ? 1.f : -1.f;
                    else if ((gv_rel_ref[k] > 0.f) != (rel != 0)) gv_chk_cull++;
                }
            }
        }
        if (ep_pepoch != gv_epoch || ep_ptss != tss) {
            gv_params_calc(tss, &ep_p);
            pm_gpu_set_rgb_scale(ep_p.boost ? 2 : 1);
            ep_pepoch = gv_epoch; ep_ptss = tss;
        } else if (gv1chk_on()) {
            GvP f;
            gv_params_calc(tss, &f);
            if (memcmp(&f, &ep_p, sizeof f) != 0) gv_chk_stale++;
        }
        ep_n++;
        if (ep_open && ep_open_gen == pm_gpu_flush_gen() && ep_open_epoch == gv_epoch && ep_open_tss == tss && pm_gpu_gappend((unsigned)vert_ring[a], (unsigned)vert_ring[b], (unsigned)vert_ring[c], vert_gmi[a], vert_gmi[b], vert_gmi[c])) {
            gp3_t0_surv(); n_in++; st_gpu_tri++; gv_tri_n++; PH_ADD(C_GVTRI, 1); GP_LAP(GPL_EMIT); return 1;
        }
        unsigned gg[3] = { vert_gmi[a], vert_gmi[b], vert_gmi[c] }, ri[3] = { vert_ring[a], vert_ring[b], vert_ring[c] }, ro[3];
        if (pm_gpu_gtrip((const float(*)[4][4])gm_tab, gg, ep_p.vp, ep_p.uvso, ep_p.cm, ep_p.ca, ep_p.zm, ep_ct, ep_p.fxon ? &ep_p.fx : NULL, ri, ro)) {
            vert_ring[a] = (unsigned short)ro[0]; vert_ring[b] = (unsigned short)ro[1]; vert_ring[c] = (unsigned short)ro[2];
            ep_open = 1; ep_open_gen = pm_gpu_flush_gen(); ep_open_epoch = gv_epoch; ep_open_tss = tss;
            gp3_t0_surv(); n_in++; st_gpu_tri++; gv_tri_n++; PH_ADD(C_GVTRI, 1); GP_LAP(GPL_EMIT); return 1;
        }
        ep_n--; ep_open = 0;
        why = GVR_REFUSE;
    }
    gv_fb[why]++;
    PH_ADD(C_GVFB, 1);
    return 0;
}

/* DLC0 (switch dlsrc): where the work of a frame comes from. Class of an address: 0 main list, 1 map shape buffer, 2 TextureHeap, 3 model lists in .data (render modes, combine modes), 4 entity heap, 5 general heap, 6 sprite heap, 7 other.
 * Classes 1 to 3 are static. Counters per class: ops (geometry, flow, state), triangles (by list class and by vertex class), vertex loads, distinct G_DL targets and the share of static targets whose list and vertices did not change in 30 frames.
 * One line `dlsrc` every 300 drawn frames. No cost without the switch: one load and one branch per site. */
extern u8 gMapShapeData[], TextureHeap[], pm_world_entity_heap[], heap_generalHead[], heap_spriteHead[];
extern Gfx SolidCombineModes[][5], AlphaTestCombineModes[][5], Gfx_RM1_SURFACE_OPA[], Gfx_RM3_CLOUD_NO_ZB[], Gfx_RM3_SURFACE_XLU_NO_ZB[], Gfx_RM3_CLOUD[];
#define DLS_N 8
static int dls_on;
static unsigned dls_pc; /* class of the list that runs now */
static unsigned dls_ops[DLS_N][3], dls_tri[DLS_N], dls_triv[DLS_N], dls_vtx[DLS_N], dls_dl, dls_stab_ok, dls_stab_chg, dls_frames, dls_fno;
static unsigned char dls_vcls[64];
static u32 dls_main_lo, dls_main_hi, dls_rd_lo, dls_rd_hi;
static u32 dls_set_a[512], dls_set_s[512], dls_stamp = 1;
static struct { u32 a, h, f; } dls_tab[1024];
static __attribute__((cold, noinline)) unsigned dls_class(u32 p) {
    if (p >= dls_main_lo && p < dls_main_hi) return 0;
    if (p - (u32)gMapShapeData < 0x40000u) return 1;
    if (p - (u32)TextureHeap < 0x68000u) return 2;
    if (p >= dls_rd_lo && p < dls_rd_hi) return 3;
    if (p - (u32)pm_world_entity_heap < 0x18000u) return 4;
    if (p - (u32)heap_generalHead < 0x180000u) return 5;
    if (p - (u32)heap_spriteHead < 0x100000u) return 6;
    return 7;
}
static __attribute__((cold, noinline)) unsigned dls_kind(unsigned op) {
    switch (op) {
    case G_VTX: case G_TRI1: case G_TRI2: case G_MTX: case G_POPMTX: case G_GEOMETRYMODE: case G_MODIFYVTX: return 0;
    case G_DL: case G_ENDDL: return 1;
    default: return 2;
    }
}
/* hash of a list (words up to G_ENDDL) and of the Vtx bytes of its G_VTX ops */
static u32 dls_hash(const Gfx* l) {
    u32 h = 2166136261u;
    for (unsigned i = 0; i < 4096; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        h = (h ^ w0) * 16777619u; h = (h ^ w1) * 16777619u;
        unsigned op = w0 >> 24;
        if (op == G_VTX) {
            unsigned n = (w0 >> 12) & 0xff;
            const unsigned char* v = (const unsigned char*)ptr_of(w1);
            if (v && is_host((u32)v) && n <= 64) for (unsigned k = 0; k < n * 16; k++) h = (h ^ v[k]) * 16777619u;
        }
        if (op == G_ENDDL) break;
    }
    return h;
}
static __attribute__((cold, noinline)) void dls_target(const Gfx* t) { /* a G_DL target: distinct count and the stable test of a static list */
    u32 a = (u32)t;
    unsigned k = (a >> 3) * 2654435761u >> 23 & 511;
    for (unsigned n = 0; n < 512; n++, k = (k + 1) & 511) {
        if (dls_set_s[k] == dls_stamp && dls_set_a[k] == a) return;
        if (dls_set_s[k] != dls_stamp) break;
    }
    dls_set_a[k] = a; dls_set_s[k] = dls_stamp; dls_dl++;
    unsigned c = dls_class(a);
    if (c < 1 || c > 3 || !is_host(a)) return;
    u32 h = dls_hash(t);
    unsigned j = (a >> 3) * 2654435761u >> 22 & 1023;
    for (unsigned n = 0; n < 1024; n++, j = (j + 1) & 1023) if (!dls_tab[j].a || dls_tab[j].a == a) break;
    if (dls_tab[j].a != a) { dls_tab[j].a = a; dls_tab[j].h = h; dls_tab[j].f = dls_fno; return; }
    if (dls_fno - dls_tab[j].f >= 30) { if (dls_tab[j].h == h) dls_stab_ok++; else dls_stab_chg++; dls_tab[j].h = h; dls_tab[j].f = dls_frames; }
}
static __attribute__((cold, noinline)) void dls_init_ranges(void) {
    u32 lo = (u32)SolidCombineModes, hi = (u32)Gfx_RM3_CLOUD_NO_ZB;
    u32 c[] = { (u32)AlphaTestCombineModes, (u32)Gfx_RM1_SURFACE_OPA, (u32)Gfx_RM3_SURFACE_XLU_NO_ZB, (u32)Gfx_RM3_CLOUD };
    for (unsigned i = 0; i < 4; i++) { if (c[i] < lo) lo = c[i]; if (c[i] > hi) hi = c[i]; }
    dls_rd_lo = lo; dls_rd_hi = hi + 0x200;
}
static void dls_report(void) {
    int m = 0, a = 0, mp = 0;
    pm_ph_scene_get(&m, &a, &mp);
    char b[250];
    int n = snprintf(b, sizeof b, "dlsrc %d %d:%d n %u ops", m, a, mp, dls_frames);
#define DLS_ROW(arr) for (unsigned i = 0; i < DLS_N; i++) n += snprintf(b + n, sizeof b - n, " %u", arr[i]);
    for (unsigned k = 0; k < 3; k++) { n += snprintf(b + n, sizeof b - n, " |"); for (unsigned i = 0; i < DLS_N; i++) n += snprintf(b + n, sizeof b - n, " %u", dls_ops[i][k]); }
    pm_log_hw("%s", b);
    n = snprintf(b, sizeof b, "dlsrc2 %d %d:%d n %u tri", m, a, mp, dls_frames);
    DLS_ROW(dls_tri);
    n += snprintf(b + n, sizeof b - n, " | trivs"); DLS_ROW(dls_triv);
    n += snprintf(b + n, sizeof b - n, " | vtx"); DLS_ROW(dls_vtx);
    snprintf(b + n, sizeof b - n, " | dl %u stable %u/%u", dls_dl, dls_stab_ok, dls_stab_ok + dls_stab_chg);
    pm_log_hw("%s", b);
    memset(dls_ops, 0, sizeof dls_ops); memset(dls_tri, 0, sizeof dls_tri); memset(dls_triv, 0, sizeof dls_triv); memset(dls_vtx, 0, sizeof dls_vtx);
    dls_dl = dls_stab_ok = dls_stab_chg = 0; dls_frames = 0;
}

/* PF1: software prefetch (PLD) of the display list, list targets and vertex data. The ARM11 of the O3DS has no hardware prefetcher and a small L1. Azahar has no cache model: no change there. */
static int tri_prep_ok; /* PG4: off_screen_check and box_fold_sync ran for this op */
static void draw_tri(unsigned w);
static void tb_probe(unsigned slot, const Gfx* l, unsigned n) {
    if (!pm_ph_on || !l) return;
    unsigned c = 0;
    for (unsigned i = 0; i < n && i < 4096; i++) if ((l[i].words.w0 >> 24) == G_PM_TEX) c++;
    if (c) RS_ADD(slot, c);
}
#include "pm_dlc.inc.c"
static __attribute__((cold, noinline)) void tri_po3a_1(int i0, int i1, int i2) {
    p_cmd++;
    p_cur_mixed = vert_mid[i0] != vert_mid[i1] || vert_mid[i0] != vert_mid[i2];
    p_mixed += (unsigned)p_cur_mixed;
    if (geom & G_LIGHTING) p_lit++;
    if (geom & G_TEXTURE_GEN) p_tgen++;
    if (vert_scr[i0] | vert_scr[i1] | vert_scr[i2]) p_scr++;
}

static __attribute__((cold, noinline)) void tri_po3a_2(const int* idx) {
    int env = comb_mode5 && !plan_active;
    int plan = plan_active && cur_plan.vtxname != PM_NM_SHADE && cur_plan.vtxname != PM_NM_NONE;
    int a255 = !plan && !comb_alpha_shade && !(plan_active && cur_plan.uses_sha);
    int fog = fogmix_on && (geom & G_FOG);
    int boost = dual_on && comb2_a == G_CCMUX_SHADE && comb2_b == G_CCMUX_ENVIRONMENT && comb2_c == G_CCMUX_COMBINED && comb2_d == G_CCMUX_COMBINED;
    int primz = (omode_l & (1u << G_MDSFT_ZSRCSEL));
    int scr = vert_scr[idx[0]] | vert_scr[idx[1]] | vert_scr[idx[2]];
    int cm = p_cur_mixed;
    int base = !cm && !(geom & G_LIGHTING) && !(geom & G_TEXTURE_GEN) && !scr && !primz && !env && !plan && !fog && !boost;
    if (po3a_on()) {
        p_drawn++; p_env += env; p_plan += plan; p_a255 += a255; p_fog += fog; p_boost += boost; p_primz += primz;
        p_plain2 += base;          /* plain except alpha 255 (cheap override) */
        p_plain += base && !a255;  /* plain: one matrix, unlit, no override at all */
    }
}

static __attribute__((cold, noinline)) void tri_dd_log(const ClipV* poly, int n) {
    int i;
    if (!dd.ntri) {
        dd.oml = omode_l; dd.omh = omode_h; dd.zb = (geom & G_ZBUFFER) != 0; dd.tex = (unsigned)tex_on; dd.timg = (u32)td[0].src; dd.fmt = td[0].fmt;
        dd.siz = td[0].siz; dd.w = td[0].w; dd.h = td[0].h; dd.prim = prim;
        dd.x0 = dd.y0 = dd.w0 = 1e9f; dd.x1 = dd.y1 = dd.w1 = -1e9f;
    }
    dd.ntri++;
    for (i = 0; i < n; i++) {
        float sx = poly[i].x / poly[i].w * vp_sx + vp_tx, sy = vp_ty - poly[i].y / poly[i].w * vp_sy;
        if (sx < dd.x0) dd.x0 = sx;
        if (sx > dd.x1) dd.x1 = sx;
        if (sy < dd.y0) dd.y0 = sy;
        if (sy > dd.y1) dd.y1 = sy;
        if (poly[i].w < dd.w0) dd.w0 = poly[i].w;
        if (poly[i].w > dd.w1) dd.w1 = poly[i].w;
    }
}

static __attribute__((cold, noinline)) void tri_rm_log(void) {
    static unsigned lo, lg, la, lb, lc, ld, lh, lp, lf;
    if (lo != omode_l || lg != (geom & G_ZBUFFER) || la != comb_a || lb != comb_b || lc != comb_c || ld != comb_d ||
        lh != omode_h || lp != prim || lf != tile_fmt[0] * 4 + tile_siz[0]) {
        lp = prim; lf = tile_fmt[0] * 4 + tile_siz[0];
        lo = omode_l; lg = geom & G_ZBUFFER; la = comb_a; lb = comb_b; lc = comb_c; ld = comb_d; lh = omode_h;
    }
}

static __attribute__((noinline)) void draw_tri_cpu(const int* idx) {
    ClipV poly[12];
    int n, i;
    int early = 0; /* PO3b2, DIP1: reject and cull from the clip positions of PO2 and the clip bits made at G_VTX, before the statistics, ensure_tile, dual_bind and any ClipV. Same tests and counters as before except the debug statistics (tris lit fog), which now count the triangles that pass. */
    if (!(plan_active && cur_plan.uses_tex1) && !(vert_scr[idx[0]] | vert_scr[idx[1]] | vert_scr[idx[2]])) {
        for (i = 0; i < 3; i++)
            if ((vert_gvf[idx[i]] & 1) == 0) gv_xf(idx[i]);
        const float *a = vert_p[idx[0]], *b = vert_p[idx[1]], *c = vert_p[idx[2]];
        early = 1;
        st_cor += 3;
        if (!have_sample && a[3] != 0.f) {
            sample_x = (int)(a[0] / a[3] * 100.f);
            have_sample = 1;
        }
        if (vert_cb[idx[0]] & vert_cb[idx[1]] & vert_cb[idx[2]] & 0x2f) {
            n_out++;
            GP_LAP(GPL_EARLY);
            return;
        }
        if ((geom & G_CULL_BOTH) && !cull_off()) {
            float ex1 = a[0] * b[3] - b[0] * a[3], ey1 = a[1] * b[3] - b[1] * a[3];
            float ex2 = c[0] * b[3] - b[0] * c[3], ey2 = c[1] * b[3] - b[1] * c[3];
            float cross = (ex1 * ey2 - ey1 * ex2) * (a[3] * c[3]);
            unsigned ct = geom & G_CULL_BOTH;
            if ((a[3] < 0) ^ (b[3] < 0) ^ (c[3] < 0)) cross = -cross;
            if (ct == G_CULL_BOTH || (ct == G_CULL_FRONT && cross <= 0) || (ct == G_CULL_BACK && cross >= 0)) {
                n_out++;
                GP_LAP(GPL_EARLY);
                return;
            }
        }
        {
            const float* q[3] = { a, b, c };
            for (int k = 0; k < 3; k++) {
                float pw = q[k][3];
                if (pw > 0.05f && q[k][0] >= -pw && q[k][0] <= pw && q[k][1] >= -pw && q[k][1] <= pw) n_keep++;
            }
        }
    }
    GP_LAP(GPL_EARLY);
    GP_LAP(GPL_A);
    tri_stats();
    GP_LAP(GPL_A);
    ensure_tile(use_t1 ? 1 : 0);
    GP_LAP(GPL_ENSURE);
    dual_bind();
    GP_LAP(GPL_DUAL);
    GP_LAP(GPL_EARLY);
    for (i = 0; i < 3; i++) {
        if (!early) st_cor++;
        if (vert_gvf[idx[i]] != 3) gv_xf(idx[i]);
        xform_vtx(vert_p[idx[i]], tex_on ? vert_st[idx[i]] : st_off, vert_c[idx[i]], &poly[i]);
        if (!tex_on) { /* H06b1: texel (0, 0), the centre of the first texel */
            int tw, th;
            pm_tex_size(&tw, &th);
            if (tw > 0 && th > 0) { poly[i].u += 0.5f / (float)tw; poly[i].v += 0.5f / (float)th; }
        }
        if (vert_scr[idx[i]]) { /* B5: screen position from G_MODIFYVTX, the inverse of the viewport map (x, y only, z and w kept in ratio) */
            ClipV* p = &poly[i];
            p->z = p->w > 0.f ? p->z / p->w : 0.f;
            p->w = 1.f;
            p->x = ((float)vert_sx[idx[i]] / 4.f - vp_tx) / vp_sx;
            p->y = (vp_ty - (float)vert_sy[idx[i]] / 4.f) / vp_sy;
        }
    }
    if (plan_active && cur_plan.uses_tex1) {
        float u0 = poly[0].u, u1 = poly[1].u, u2 = poly[2].u, v0 = poly[0].v, v1 = poly[1].v, v2 = poly[2].v;
        t1_fold(fminf(u0, fminf(u1, u2)), fmaxf(u0, fmaxf(u1, u2)), fminf(v0, fminf(v1, v2)), fmaxf(v0, fmaxf(v1, v2)));
    }
    if (!early && !have_sample && poly[0].w != 0.f) {
        sample_x = (int)(poly[0].x / poly[0].w * 100.f);
        have_sample = 1;
    }
    if (!early && (clip_bits(&poly[0]) & clip_bits(&poly[1]) & clip_bits(&poly[2]))) {
        n_out++;
        return;
    }
    if (!early && (geom & G_CULL_BOTH) && !cull_off()) {
        /* V02: the RSP culls by the sign of the screen space area (G_CULL_FRONT / G_CULL_BACK / both), same test as
         * libultraship. A one sided wall (the cliff skirt around the Mario house yard, seen from inside) draws only
         * if this test runs. nocull turns it off for A/B checks. */
        const ClipV *p1 = &poly[0], *p2 = &poly[1], *p3 = &poly[2];
        float ex1 = p1->x * p2->w - p2->x * p1->w, ey1 = p1->y * p2->w - p2->y * p1->w;
        float ex2 = p3->x * p2->w - p2->x * p3->w, ey2 = p3->y * p2->w - p2->y * p3->w;
        float cross = (ex1 * ey2 - ey1 * ex2) * (p1->w * p3->w);
        unsigned ct = geom & G_CULL_BOTH;
        if ((p1->w < 0) ^ (p2->w < 0) ^ (p3->w < 0)) cross = -cross;
        if (ct == G_CULL_BOTH || (ct == G_CULL_FRONT && cross <= 0) || (ct == G_CULL_BACK && cross >= 0)) {
            n_out++;
            return;
        }
    }
    if (!early) {
        int k;
        for (k = 0; k < 3; k++) {
            float pw = poly[k].w;
            if (pw > 0.05f && poly[k].x >= -pw && poly[k].x <= pw && poly[k].y >= -pw && poly[k].y <= pw)
                n_keep++;
        }
    }
    gp3_t0_surv();
    n_in++;
    n = 3;
    if ((rmlog_on() && (rm_frame % 30) == 1) || xt_on()) tri_rm_log();
    if (comb_mode5 && !plan_active) {
        float en[4];
        unpack8(envc, en);
        for (i = 0; i < n; i++) { poly[i].r = en[0] * 255.f; poly[i].g = en[1] * 255.f; poly[i].b = en[2] * 255.f; poly[i].a = en[3] * 255.f; }
    }
    if (po3a_on()) tri_po3a_2(idx);
    if (dd_skip()) return;
    sync_render();
    if (dd_on()) tri_dd_log(poly, n);
    if ((omode_l & (1u << G_MDSFT_ZSRCSEL))) {
        /* X01: depth source G_ZS_PRIM, every pixel gets the primitive depth (the battle start swirl and other stencil passes) */
        float zn = prim_ndc_z();
        for (i = 0; i < n; i++) poly[i].z = zn * poly[i].w;
    } else if (img_role(0) == ROLE_DEPTH) {
        /* B3: case 6, the combiner is PRIM (log: imglog). B3b: the vertices keep their own depth, the GPU tests it against the world and writes zn there */
        pm_gpu_set_zmark(1, 2.f * depth_of_word(word_5551(prim)) - 1.f);
    }
    if (plan_active && cur_plan.vtxname != PM_NM_SHADE && cur_plan.vtxname != PM_NM_NONE) {
        /* S25: the vertex colour carries a constant (PRIM, ENV or PRIM_LOD_FRAC) because the shade is not read. */
        float vc[4];
        unpack8(cur_plan.vtx, vc);
        for (i = 0; i < n; i++) { poly[i].r = vc[0] * 255.f; poly[i].g = vc[1] * 255.f; poly[i].b = vc[2] * 255.f; poly[i].a = vc[3] * 255.f; }
    } else if (!comb_alpha_shade && !(plan_active && cur_plan.uses_sha)) {
        for (i = 0; i < n; i++) poly[i].a = 255;
    }
    if (fogmix_on && (geom & G_FOG)) { /* F08: RSP fog factor, clamp(z / w * fm + fo) replaces the vertex alpha */
        for (i = 0; i < n; i++) {
            float f = (poly[i].w > 0.f ? poly[i].z / poly[i].w : poly[i].z * 32767.f) * fog_fm + fog_fo;
            poly[i].a = f < 0.f ? 0.f : f > 255.f ? 255.f : f;
        }
    }
    {
        int boost = dual_on && comb2_a == G_CCMUX_SHADE && comb2_b == G_CCMUX_ENVIRONMENT &&
                    comb2_c == G_CCMUX_COMBINED && comb2_d == G_CCMUX_COMBINED;
        pm_gpu_set_rgb_scale(boost ? 2 : 1);
        if (!boost) goto no_boost;
    }
    {
        /* Cycle 2 is (SHADE - ENV) * COMBINED + COMBINED = COMBINED * (SHADE - ENV + 1), up to 2.
         * The texture stage multiplies by 2, so the vertex carries half of it. */
        float en[4];
        unpack8(envc, en);
        for (i = 0; i < n; i++) {
            float* ch[3] = { &poly[i].r, &poly[i].g, &poly[i].b };
            for (int k = 0; k < 3; k++) {
                float f = (*ch[k] / 255.f - en[k] + 1.f) / 2.f;
                *ch[k] = (f < 0.f ? 0.f : f > 1.f ? 1.f : f) * 255.f;
            }
        }
    }
no_boost:
    GP_LAP(GPL_OLD);
    pm_gpu_set_mvp(ident_m, vp_sx, vp_sy, vp_tx, vp_ty);
    RS_ADD(RS_CLIP, 1);
    pm_gpu_vert_clip(poly[0].x, poly[0].y, poly[0].z, poly[0].w, poly[0].u, poly[0].v, pack_color(&poly[0]));
    RS_ADD(RS_CLIP, 1);
    pm_gpu_vert_clip(poly[1].x, poly[1].y, poly[1].z, poly[1].w, poly[1].u, poly[1].v, pack_color(&poly[1]));
    RS_ADD(RS_CLIP, 1);
    pm_gpu_vert_clip(poly[2].x, poly[2].y, poly[2].z, poly[2].w, poly[2].u, poly[2].v, pack_color(&poly[2]));
}

static void draw_tri(unsigned w) {
    int i0 = (int)((w >> 16) & 0xff) / 2;
    int i1 = (int)((w >> 8) & 0xff) / 2;
    int i2 = (int)(w & 0xff) / 2;
    int idx[3];
    GP_START();
    gp_ntri++;
    n_cmd++;
    if (dls_on) { dls_tri[dls_pc]++; dls_triv[(i0 >= 0 && i0 < 64) ? dls_vcls[i0] : 7]++; }
    tri_since_vtx = 1;
    if (i0 < 0 || i1 < 0 || i2 < 0 || i0 > 63 || i1 > 63 || i2 > 63) {
        n_drop++;
        return;
    }
    if (!vert_live[i0] || !vert_live[i1] || !vert_live[i2]) {
        n_drop++;
        if (dlc_dirty) dlc_c.slotfix++; /* DLC1: a vertex that the interpreter would have kept live after a replayed list */
        return;
    }
    idx[0] = i0;
    idx[1] = i1;
    idx[2] = i2;
    suntile_tri("draw", idx);
    if (po3a_on()) tri_po3a_1(i0, i1, i2);
    if (!tri_prep_ok) { if (ep_osc != gv_epoch) { off_screen_check(); box_fold_sync(); ep_osc = gv_epoch; } tri_prep_ok = 1; } /* PG4: the second triangle of a G_TRI2 sees the same state */
    { /* GV1: ring vertices go to the GPU with no CPU transform. The rest reads the ring copy. */
        if (vert_ring[i0] != GV_NONE && vert_ring[i1] != GV_NONE && vert_ring[i2] != GV_NONE && gv_tri(idx)) return;
    }
    draw_tri_cpu(idx);
}

static void load_mtx(u32 w0, u32 w1) {
    unsigned flags = (w0 & 0xff) ^ G_MTX_PUSH;
    Mtx* src = (Mtx*)ptr_of(w1);
    if (!src) return;
    float in[4][4];
    const float (*lm)[4] = pm_interp_repl ? pm_interp_find(src) : 0; /* F06-2 */
    if (lm) memcpy(in, lm, sizeof(in));
    else unpack(src, in);
    int is_proj = (flags & G_MTX_PROJECTION) != 0;
    if (!is_proj && (flags & G_MTX_PUSH) && mv_sp + 1 < 10) {
        memcpy(mv[mv_sp + 1], mv[mv_sp], sizeof(mv[0]));
        mv_sp++;
    }
    float (*dst)[4] = is_proj ? proj : mv[mv_sp];
    if (flags & G_MTX_LOAD) {
        memcpy(dst, in, sizeof(in));
    } else {
        /* Row vector v * incoming * current. The RSP and PaperBoat's
         * interpreter both premultiply; the other order leaves a child
         * under the wrong parent. */
        float cur[4][4];
        memcpy(cur, dst, sizeof(cur));
        mul(dst, in, cur);
    }
    upload_mvp();
}

/* S25: an opcode the interpreter does not run. Counted, logged at 1, 100, 10000 ... uses. */
static unsigned op_cnt[256];
static __attribute__((cold, noinline)) void note(unsigned op) {
    unsigned n = ++op_cnt[op];
    seen[op] = 1;
}

static unsigned n_ops; /* PT01: display list ops run this frame */
unsigned pm_gbi_ops_now(void) { return n_ops; } /* PI1: ops so far this drawn frame */

/* W1: sampled time per op class. Phase on only. op_n is exact (one increment per op). One tick pair per 64 ops, outer list only.
 * P1: producer census. Marks come from pm_interp_open (gMainGfxPos at the open). The cursor moves forward at sp == 0 in the
 * main list. Each op counts to the producer under the cursor. Nested lists count to the calling producer. */
enum { OS_VTX, OS_MTX, OS_TRI, OS_TEX, OS_CC, OS_GEO, OS_DL, OS_DLC, OS_SYNC, OS_OTH, OS_N };
static const char* const os_nm[OS_N] = { "vtx", "mtx", "tri", "tex", "cc", "geo", "dl", "dlc", "sync", "oth" };
#define OS_DLC_OP 0xFF /* pseudo op: a G_DL that dlc_try answered */
static unsigned os_op_n[256], os_nn[256], os_tk[256], os_sn[256], os_fl[256];
static unsigned os_fr, os_off, os_nest, os_open, os_op, os_pk, os_t0, os_fl0;
#define PK_MARKS 256
#define PK_SLOTS 256
#define PK_PRE 253 /* main list ops before the first mark */
#define PK_BG 254 /* ops of a list that is not the main list */
#define PK_OTHER 255 /* a producer that found no free slot */
static struct { const Gfx* p; unsigned s; } pmk[PK_MARKS];
static int pmk_n, pmx_main, pmx_next;
static unsigned pmx_cur, pk_key[PK_SLOTS], pk_set[PK_SLOTS], pk_ops[PK_SLOTS], pk_tk[PK_SLOTS], pk_sn[PK_SLOTS];

static unsigned pk_slot(unsigned a) {
    unsigned h = ((a * 2654435761u) >> 16) % PK_PRE;
    for (int i = 0; i < PK_PRE; i++, h = (h + 1) % PK_PRE) {
        if (!pk_set[h]) { pk_set[h] = 1; pk_key[h] = a; return h; }
        if (pk_key[h] == a) return h;
    }
    return PK_OTHER;
}
void pm_gbi_prod_mark(unsigned a) { /* called by pm_interp_open under phase */
    const Gfx* p = gMainGfxPos;
    if (pmk_n && p < pmk[pmk_n - 1].p) pmk_n = 0; /* the main list starts again: marks of the last one are stale */
    if (pmk_n < PK_MARKS) { pmk[pmk_n].p = p; pmk[pmk_n].s = pk_slot(a); pmk_n++; }
}

static unsigned os_flsum(void) { unsigned s = 0; for (int i = 0; i < 6; i++) s += pm_fl_us[i]; return s; }
/* W1 calibration: one tick read costs about os_cal ticks, and each sample window holds one read. Measured at window start. */
static unsigned os_cal;
static void os_calibrate(void) {
    unsigned long long sum = 0;
    for (int i = 0; i < 16; i++) { unsigned a = pm_ticks(); unsigned b = pm_ticks(); sum += b - a; }
    os_cal = (unsigned)(sum / 16);
}
static unsigned os_win(unsigned t) { return t - os_t0 > os_cal ? t - os_t0 - os_cal : 0; }
static void os_close(void) {
    unsigned t = pm_ticks();
    unsigned w = os_win(t);
    os_tk[os_op] += w; os_sn[os_op]++; os_fl[os_op] += os_flsum() - os_fl0;
    pk_tk[os_pk] += w; pk_sn[os_pk]++;
    os_open = 0;
}
static __attribute__((cold, noinline)) void os_dlc_hit(void) { /* a G_DL answered by dlc_try: count it as the dlc pseudo op */
    unsigned* n = os_nest > 1 ? os_nn : os_op_n;
    n[G_DL]--; n[OS_DLC_OP]++;
    if (os_open && os_op == G_DL) os_op = OS_DLC_OP;
}
static int os_class_of(unsigned op) {
    switch (op) {
    case OS_DLC_OP: return OS_DLC;
    case G_VTX: case G_MODIFYVTX: return OS_VTX;
    case G_MTX: case G_POPMTX: return OS_MTX;
    case G_TRI1: case G_TRI2: return OS_TRI;
    case G_SETTIMG: case G_SETTILE: case G_SETTILESIZE: case G_LOADTILE: case G_LOADBLOCK: case G_LOADTLUT: case G_TEXTURE: case G_PM_TEX: case G_PM_TEXI: return OS_TEX;
    case G_SETCOMBINE: case G_SETPRIMCOLOR: case G_SETENVCOLOR: case G_SETOTHERMODE_H: case G_SETOTHERMODE_L: case G_SETFOGCOLOR: return OS_CC;
    case G_GEOMETRYMODE: case G_MOVEWORD: case G_MOVEMEM: case G_SETSCISSOR: return OS_GEO;
    case G_DL: case G_ENDDL: return OS_DL;
    case G_RDPPIPESYNC: case G_RDPTILESYNC: case G_RDPLOADSYNC: case G_RDPFULLSYNC: case G_NOOP: return OS_SYNC;
    default: return OS_OTH;
    }
}
/* P1 name: a label string when the key reads as printable text, else the hex key (a function address, resolved with the .map file). */
static void pk_name(unsigned s, char* b, int n) {
    if (s == PK_PRE) { snprintf(b, n, "pre"); return; }
    if (s == PK_BG) { snprintf(b, n, "bg"); return; }
    if (s == PK_OTHER) { snprintf(b, n, "other"); return; }
    unsigned a = pk_key[s];
    const char* p = (const char*)(uintptr_t)a;
    int ok = a && is_host(a) && a >= 0x00100000u;
    for (int i = 0; ok && i < 40; i++) { if (!p[i]) break; if (p[i] < 32 || p[i] > 126) ok = 0; else if (i == 39) ok = 0; }
    if (ok && p[0]) snprintf(b, n, "%.40s", p); else snprintf(b, n, "%08x", a);
}
static __attribute__((cold, noinline)) void os_op_hook(const Gfx* at, int sp, unsigned op) { /* one call per op, phase on */
    if (os_nest == 1) {
        if (os_open) os_close();
        if (pmx_main && sp == 0) while (pmx_next < pmk_n && at >= pmk[pmx_next].p) pmx_cur = pmk[pmx_next++].s;
    }
    if (os_nest > 1) os_nn[op]++; /* inside a dlc replay: its time is in the outer G_DL sample, so it is not a class term */
    else os_op_n[op]++;
    pk_ops[pmx_cur]++;
    if (os_nest == 1 && ((n_ops + os_off) & 63u) == 0) { os_open = 1; os_op = op; os_pk = pmx_cur; os_fl0 = os_flsum(); os_t0 = pm_ticks(); }
}
/* R0: counts the state ops that R1 to R3 would skip. Cold, and called only when the phase is on. It counts and never skips. */
static struct { unsigned bind, bind_w1, cc, cc_same, tex_same, geo_same, mtx, mtx_same, lt, lt_same, lt_skip; } mops_c;
static u32 mops_last_bind;
static __attribute__((cold, noinline)) void mops_count(unsigned op, u32 w0, u32 w1) {
    switch (op) {
    case G_PM_TEX:
        mops_c.bind++;
        if (w1 == mops_last_bind) mops_c.bind_w1++;
        mops_last_bind = w1;
        break;
    case G_SETCOMBINE:
        mops_c.cc++;
        if (comb_w0 == w0 && comb_w1 == w1 && comb_set && sc_ok) mops_c.cc_same++;
        break;
    case G_TEXTURE:
        if (((w0 & 0xff) != 0) == (tex_on != 0) && (w1 >> 16) == tex_ss && (w1 & 0xffffu) == tex_ts && sc_ok) mops_c.tex_same++;
        break;
    case G_GEOMETRYMODE:
        if (((geom & (w0 & 0xffffffu)) | w1) == geom) mops_c.geo_same++;
        break;
    case G_MTX: {
        mops_c.mtx++;
        if (!is_host((u32)(uintptr_t)ptr_of(w1))) break;
        unsigned flags = (w0 & 0xff) ^ G_MTX_PUSH;
        Mtx* src = (Mtx*)ptr_of(w1);
        float in[4][4], nt[4][4];
        const float (*lm)[4] = pm_interp_repl ? pm_interp_find(src) : 0;
        if (lm) memcpy(in, lm, sizeof(in));
        else unpack(src, in);
        int is_proj = (flags & G_MTX_PROJECTION) != 0;
        const float (*cur)[4] = is_proj ? proj : mv[mv_sp];
        if (flags & G_MTX_LOAD) memcpy(nt, in, sizeof(nt));
        else mul(nt, in, cur);
        if (memcmp(nt, cur, sizeof(nt)) == 0) mops_c.mtx_same++;
        break;
    }
    case G_POPMTX: {
        mops_c.mtx++;
        unsigned n = w1 / 64u;
        if (n == 0) n = 1;
        if (n > (unsigned)mv_sp) n = (unsigned)mv_sp;
        if (n == 0 || memcmp(mv[mv_sp - n], mv[mv_sp], sizeof(mv[0])) == 0) mops_c.mtx_same++;
        break;
    }
    case G_MOVEMEM:
        if ((w0 & 0xff) == G_MV_LIGHT) {
            mops_c.lt++;
            int slot = (int)(((w0 >> 8) & 0xff) * 8 / 24) - 2;
            const u8* l = (const u8*)ptr_of(w1);
            if (l && slot >= 0 && slot < 9 && lt[slot].col[0] == l[0] && lt[slot].col[1] == l[1] && lt[slot].col[2] == l[2] &&
                lt[slot].dir[0] == (s8)l[8] && lt[slot].dir[1] == (s8)l[9] && lt[slot].dir[2] == (s8)l[10]) {
                mops_c.lt_same++;
                if (!lit_pending) mops_c.lt_skip++; /* R5B: what g2_same skips */
            }
        }
        break;
    default: break;
    }
}
static __attribute__((cold, noinline)) void mops_report(void) {
    pm_log_hw("mops f%d bind %u bind_w1 %u cc %u cc_same %u tex_same %u geo_same %u mtx %u mtx_same %u lt %u lt_same %u lt_skip %u", rm_frame,
        mops_c.bind / 300u, mops_c.bind_w1 / 300u, mops_c.cc / 300u, mops_c.cc_same / 300u, mops_c.tex_same / 300u, mops_c.geo_same / 300u,
        mops_c.mtx / 300u, mops_c.mtx_same / 300u, mops_c.lt / 300u, mops_c.lt_same / 300u, mops_c.lt_skip / 300u);
    memset(&mops_c, 0, sizeof(mops_c));
}

/* MDL1: model census, phase on only, counts only. appendGfx_model marks the model ID at its entry (gMainGfxPos). The cursor
 * moves as the producer census does (main list, sp == 0). Draw = a run of tri ops (any other op ends the run). Static = the hash
 * of the G_MTX words of the model is equal to the hash of the last frame. Table 128 slots, mark list 1024 entries. */
#define MDC_MARKS 1024
#define MDC_SLOTS 128
typedef struct {
    u16 id;
    u8 used, pan, fog, has_prev;
    unsigned ops, draws, tris, binds, closes, tex_sum, frames, stat; /* window sums */
    unsigned seen, ntx, tx[8], prev_h, cur_h; /* frame state */
} mdc_t;
static mdc_t mdc[MDC_SLOTS];
static struct { const Gfx* p; int s; u8 plain; } mdcm[MDC_MARKS];
static int mdcm_n, mdcm_next, mdcm_cur = -1, mdcm_over, mdc_prev_tri, mdcm_last = -1;
static unsigned mdc_tot_ops, mdc_tot_draws, mdc_tot_tris, mdc_tot_binds, mdc_none;

static __attribute__((cold, noinline)) int mdc_slot(unsigned id) {
    unsigned h = (id * 2654435761u) >> 25;
    for (int i = 0; i < MDC_SLOTS; i++, h = (h + 1) & (MDC_SLOTS - 1)) {
        if (!mdc[h].used) { memset(&mdc[h], 0, sizeof mdc[h]); mdc[h].used = 1; mdc[h].id = (u16)id; mdc[h].cur_h = 2166136261u; return (int)h; }
        if (mdc[h].id == id) return (int)h;
    }
    return -1;
}
/* T2b: one pseudo ID per task function pointer (first-sight order, 32 entries, 0xF100 + index). A full table gives 0xF0FF. */
#define MDC_FNS 32
static const void* mdc_fn[MDC_FNS];
static int mdc_fn_n;
static __attribute__((cold, noinline)) unsigned mdc_fn_id(const void* fn) {
    for (int i = 0; i < mdc_fn_n; i++) if (mdc_fn[i] == fn) return 0xF100u + (unsigned)i;
    if (mdc_fn_n >= MDC_FNS) return 0xF0FFu;
    mdc_fn[mdc_fn_n] = fn;
    return 0xF100u + (unsigned)mdc_fn_n++;
}
unsigned pm_gbi_task_id(const void* fn) { return mdc_fn_id(fn); }
static const void* mdc_fn_of(unsigned id) { return id >= 0xF100u && id - 0xF100u < (unsigned)mdc_fn_n ? mdc_fn[id - 0xF100u] : NULL; }
/* T2b: a non-empty close (pm_gpu_flush_at) counts on the model or task the op stream is in. */
void pm_gbi_census_close(void) { int cur = pmx_main ? mdcm_cur : -1; if (cur >= 0) mdc[cur].closes++; }

void pm_gbi_mdl_mark(unsigned id, int pan) { /* called by appendGfx_model under phase */
    const Gfx* p = gMainGfxPos;
    if (mdcm_n && p < mdcm[mdcm_n - 1].p) mdcm_n = 0; /* the main list starts again */
    int s = mdc_slot(id);
    if (s >= 0) { mdc[s].seen = 1; mdc[s].pan = (u8)(pan != 0); }
    else mdcm_over++;
    mdcm_last = -1;
    if (mdcm_n < MDC_MARKS) { mdcm[mdcm_n].p = p; mdcm[mdcm_n].s = s; mdcm[mdcm_n].plain = 0; mdcm_last = mdcm_n; mdcm_n++; }
    else mdcm_over++;
}

/* MR0: plain static model census, phase on only, cold. A plain model has no fog change, no tint, no custom gfx, no panner and
 * no billboard (set by appendGfx_model through pm_gbi_mr0_plain). The walk cuts time into periods. A period has a kind: 0 ops,
 * 1 bind (G_PM_TEX), 2 dlc (a G_DL that dlc_try answers). A tick ends a period when the kind changes or the model changes.
 * Flush time (os_flsum) is cut out of each kind. Ticks are cheap: one per bind or dlc boundary and one per model change. */
static unsigned mr0_t, mr0_fl0, mr0_plain_n;
static int mr0_kind, mr0_plain, mr0_dlc_hit;
static unsigned long long mr0_T[3], mr0_F[3]; /* T in system ticks, F in us */
void pm_gbi_mr0_plain(int plain) { /* appendGfx_model, under phase, right after its mark */
    if (mdcm_last >= 0 && plain) { mdcm[mdcm_last].plain = 1; mr0_plain_n++; }
}
static __attribute__((cold, noinline)) void mr0_tick(int kind_new, int plain_new) {
    unsigned now = pm_ticks(), fl = os_flsum();
    if (mr0_plain) {
        int k = mr0_kind;
        if (k == 2 && !mr0_dlc_hit) k = 0; /* a G_DL that nested a list is ops, not dlc */
        mr0_T[k] += now - mr0_t;
        mr0_F[k] += fl >= mr0_fl0 ? fl - mr0_fl0 : fl; /* pm_fl_us is cleared by the phase report: a reset counts from 0 */
    }
    mr0_t = now; mr0_fl0 = fl; mr0_kind = kind_new; mr0_plain = plain_new; mr0_dlc_hit = 0;
}
static __attribute__((cold, noinline)) void mr0_op(unsigned op, int adv, int plain) {
    int kn = op == G_PM_TEX ? 1 : op == G_DL ? 2 : 0;
    if (!adv && kn == 0 && mr0_kind == 0) return;
    mr0_tick(kn, plain);
}
/* MS0: removable closes, phase only, an upper bound. A run is consecutive plain opaque model marks (non-model ops between them
 * are not checked). A stable sort by texture inside a run leaves one close per distinct texture: removable = changes + 1 - distinct. */
static int ms0_in, ms0_last = -1, ms0_nd;
static unsigned ms0_d[64];
static unsigned long long ms0_rem, ms0_chg, ms0_ctot, ms0_opm, ms0_runs;
static __attribute__((cold, noinline)) void ms0_end(void) {
    if (!ms0_in) return;
    int r = (int)ms0_nd > 0 ? (int)ms0_chg + 1 - (int)ms0_nd : 0;
    if (r > 0) ms0_rem += (unsigned)r;
    ms0_ctot += ms0_chg;
    ms0_in = 0; ms0_nd = 0; ms0_chg = 0; ms0_last = -1;
}
void pm_gbi_ms0_mark(int opaque, unsigned tex) { /* appendGfx_model, under phase: opaque = plain and RENDER_MODE_SURFACE_OPA */
    if (!opaque) { ms0_end(); return; }
    ms0_opm++;
    if (!ms0_in) { ms0_in = 1; ms0_runs++; }
    unsigned j = 0;
    while ((int)j < ms0_nd && ms0_d[j] != tex) j++;
    if ((int)j == ms0_nd && ms0_nd < 64) ms0_d[ms0_nd++] = tex;
    if (ms0_last >= 0 && ms0_last != (int)tex) ms0_chg++;
    ms0_last = (int)tex;
}
/* the main walk ends: the time after it (logic, other producers) is not a model period */
static __attribute__((cold, noinline)) void mr0_walk_end(void) {
    if (mr0_plain || mr0_kind) mr0_tick(0, 0);
}
static __attribute__((cold, noinline)) void mdc_op(const Gfx* at, int sp, unsigned op, u32 w0, u32 w1) {
    int adv = 0;
    if (os_nest == 1 && pmx_main && sp == 0) {
        while (mdcm_next < mdcm_n && at >= mdcm[mdcm_next].p) { mdcm_cur = mdcm[mdcm_next++].s; mdc_prev_tri = 0; adv = 1; }
    }
    int cur = pmx_main ? mdcm_cur : -1;
    mr0_op(op, adv, cur >= 0 && mdcm[mdcm_next - 1].plain);
    unsigned is_tri = op == G_TRI1 || op == G_TRI2, was_tri = (unsigned)mdc_prev_tri, ntri = op == G_TRI2 ? 2u : 1u;
    mdc_prev_tri = (int)is_tri;
    mdc_tot_ops++;
    if (is_tri) { mdc_tot_tris += ntri; if (!was_tri) mdc_tot_draws++; }
    if (op == G_PM_TEX) mdc_tot_binds++;
    if (cur < 0) { mdc_none++; return; }
    mdc_t* m = &mdc[cur];
    m->ops++;
    if (is_tri) { m->tris += ntri; if (!was_tri) m->draws++; }
    switch (op) {
    case G_TRI1: case G_TRI2: break;
    case G_PM_TEX: {
        m->binds++;
        unsigned j = 0;
        while (j < m->ntx && m->tx[j] != w1) j++;
        if (j == m->ntx && m->ntx < 8) m->tx[m->ntx++] = w1;
        break;
    }
    case G_SETFOGCOLOR: m->fog = 1; break;
    case G_MTX: {
        if (!is_host((u32)(uintptr_t)ptr_of(w1))) break;
        u32 v[16];
        memcpy(v, ptr_of(w1), sizeof(v));
        unsigned h = m->cur_h ^ w0;
        for (int i = 0; i < 16; i++) h = (h ^ v[i]) * 16777619u;
        m->cur_h = h;
        break;
    }
    default: break;
    }
}
static __attribute__((cold, noinline)) void mdc_frame_end(void) {
    for (int s = 0; s < MDC_SLOTS; s++) {
        mdc_t* m = &mdc[s];
        if (!m->used) continue;
        if (m->seen) {
            m->frames++;
            m->tex_sum += m->ntx;
            if (m->has_prev && m->cur_h == m->prev_h) m->stat++;
            m->prev_h = m->cur_h;
            m->has_prev = 1;
        } else {
            m->has_prev = 0;
        }
        m->seen = 0; m->ntx = 0; m->cur_h = 2166136261u;
    }
    mdcm_n = 0; mdcm_next = 0; mdcm_cur = -1;
    ms0_end();
}
static __attribute__((cold, noinline)) void mdc_report(void) {
    unsigned long long tot_o = 0, tot_d = 0, tot_t = 0, tot_b = 0;
    unsigned mdl_n = 0, mdl_st = 0;
    static unsigned char picked[MDC_SLOTS];
    memset(picked, 0, sizeof picked);
    for (int s = 0; s < MDC_SLOTS; s++) {
        mdc_t* m = &mdc[s];
        if (!m->used || !m->frames) continue;
        mdl_n++;
        if (m->frames > 1 && (unsigned long long)m->stat * 10 >= (unsigned long long)(m->frames - 1) * 9) mdl_st++;
        tot_o += m->ops; tot_d += m->draws; tot_t += m->tris; tot_b += m->binds;
    }
    pm_log_hw("mdlsum f%d tot ops %u draws %u tris %u binds %u | models %u static %u | unattr ops %u over %u", rm_frame,
        (unsigned)(mdc_tot_ops / 300), (unsigned)(mdc_tot_draws / 300), (unsigned)(mdc_tot_tris / 300), (unsigned)(mdc_tot_binds / 300),
        mdl_n, mdl_st, (unsigned)(mdc_none / 300), mdcm_over);
    for (int r = 0; r < 12; r++) {
        int best = -1; unsigned bv = 0;
        for (int s = 0; s < MDC_SLOTS; s++) if (!picked[s] && mdc[s].used && mdc[s].ops > bv) { bv = mdc[s].ops; best = s; }
        if (best < 0) break;
        picked[best] = 1;
        mdc_t* m = &mdc[best];
        unsigned tx = m->frames ? m->tex_sum / m->frames : 0;
        unsigned key = m->id >= 0xF100u ? (unsigned)(uintptr_t)mdc_fn_of(m->id) : (unsigned)m->id;
        pm_log_hw("mdlsum f%d r%d %s %08x draws %u ops %u tris %u binds %u closes %u tex %u static %u/%u pan %u fog %u", rm_frame, r + 1,
            m->id >= 0xF100u ? "fn" : m->id >= 0xF000 ? "task" : "id", key,
            (unsigned)(m->draws / 300), (unsigned)(m->ops / 300), (unsigned)(m->tris / 300), (unsigned)(m->binds / 300), (unsigned)(m->closes / 300), tx,
            m->frames > 1 ? m->stat : 0u, m->frames > 1 ? m->frames - 1 : 0u, (unsigned)m->pan, (unsigned)m->fog);
    }
    for (int s = 0; s < MDC_SLOTS; s++) {
        mdc[s].ops = mdc[s].draws = mdc[s].tris = mdc[s].binds = mdc[s].closes = mdc[s].tex_sum = mdc[s].frames = mdc[s].stat = 0;
        mdc[s].fog = 0;
    }
    { /* MR0: plain model census, us per model. Ticks run at 268 MHz (TICKS_MS 268112). F is pm_fl_us: already x8 for the 1-in-8 sample. */
        unsigned long long n = mr0_plain_n ? mr0_plain_n : 1;
        unsigned long long ops = mr0_T[0] / 268u, bind = mr0_T[1] / 268u, dlc = mr0_T[2] / 268u;
        unsigned long long fo = mr0_F[0] + mr0_F[1] + mr0_F[2];
        unsigned long long o = ops > mr0_F[0] ? ops - mr0_F[0] : 0, b = bind > mr0_F[1] ? bind - mr0_F[1] : 0;
        unsigned long long d = dlc > mr0_F[2] ? dlc - mr0_F[2] : 0;
        pm_log_hw("mr0 f%d plain %u per frame | per model us: ops %u bind %u close %u outside dlc %u dlc %u | console x2.86 outside dlc %u",
            rm_frame, (unsigned)(mr0_plain_n / 300u), (unsigned)(o / n), (unsigned)(b / n), (unsigned)(fo / n),
            (unsigned)((o + b + fo) / n), (unsigned)(d / n), (unsigned)((o + b + fo) * 286u / 100u / n));
    }
    memset(mr0_T, 0, sizeof mr0_T); memset(mr0_F, 0, sizeof mr0_F); mr0_plain_n = 0;
    pm_log_hw("ms0 f%d opaque plain models %u per frame, runs %u per frame, removable closes %u per frame (texture changes %u per frame)",
        rm_frame, (unsigned)(ms0_opm / 300u), (unsigned)(ms0_runs / 300u), (unsigned)(ms0_rem / 300u), (unsigned)(ms0_ctot / 300u));
    ms0_rem = ms0_chg = ms0_ctot = ms0_opm = ms0_runs = 0;
    mdc_tot_ops = mdc_tot_draws = mdc_tot_tris = mdc_tot_binds = mdc_none = 0;
    for (int s = 0; s < MDC_SLOTS; s++) mdc[s].used = 0; /* each window allocates its own slots; the table filled over several maps before (unattr ops) */
    mdcm_over = 0;
}

static void os_report(void) {
    static unsigned long long est[256], estc[256], pke[PK_SLOTS];
    unsigned long long cls_tk[OS_N] = { 0 }, cls_n[OS_N] = { 0 }, tot_n = 0, tot_fl = 0, tot_sn = 0, psum = 0;
    for (int op = 0; op < 256; op++) {
        est[op] = os_sn[op] ? (unsigned long long)os_tk[op] * os_op_n[op] / os_sn[op] : 0;
        tot_n += os_op_n[op] + os_nn[op]; tot_fl += os_fl[op]; tot_sn += os_sn[op];
        cls_tk[os_class_of(op)] += est[op]; cls_n[os_class_of(op)] += os_op_n[op];
    }
    char c[480]; int k = 0;
    for (int i = 0; i < OS_N; i++) k += snprintf(c + k, sizeof c - k, " %s %u/%u", os_nm[i], (unsigned)(cls_tk[i] / 268 / 300), (unsigned)(cls_n[i] / 300));
    char t[160]; int m = snprintf(t, sizeof t, "top");
    memcpy(estc, est, sizeof est);
    for (int r = 0; r < 6; r++) {
        int best = -1; unsigned long long bv = 0;
        for (int op = 0; op < 256; op++) if (estc[op] > bv) { bv = estc[op]; best = op; }
        if (best < 0) break;
        m += snprintf(t + m, sizeof t - m, " %02x:%u", best, (unsigned)(bv / 268 / 300));
        estc[best] = 0;
    }
    for (int s = 0; s < PK_SLOTS; s++) {
        pke[s] = pk_sn[s] ? (unsigned long long)pk_tk[s] * pk_ops[s] / pk_sn[s] : 0;
        psum += pk_ops[s];
    }
    static unsigned svc_last; /* W2: all pm_ticks reads per frame over the 300 frames (tk is the op sampler only) */
    unsigned svc_now = pm_svc_n;
    pm_log_hw("opsplit f%d ops %u |%s | %s | fl %u | tk %u | pt %u | gms %u gmn %u", rm_frame, (unsigned)(tot_n / 300), c, t, (unsigned)(tot_fl / 300), (unsigned)(2 * tot_sn / 300), (svc_now - svc_last) / 300u, gm_steps / 300, gm_n);
    svc_last = svc_now;
    pm_log_hw("prod f%d sum %u ops %u", rm_frame, (unsigned)(psum / 300), (unsigned)(tot_n / 300));
    for (int r = 0; r < 8; r++) {
        int best = -1; unsigned long long bv = 0;
        for (int s = 0; s < PK_SLOTS; s++) if (pk_ops[s] > bv) { bv = pk_ops[s]; best = s; }
        if (best < 0) break;
        char nm[48];
        pk_name((unsigned)best, nm, sizeof nm);
        pm_log_hw("prod f%d %s ops %u us %u", rm_frame, nm, (unsigned)(bv / 300), (unsigned)(pke[best] / 268 / 300));
        pk_ops[best] = 0;
    }
    memset(os_op_n, 0, sizeof os_op_n); memset(os_nn, 0, sizeof os_nn); memset(os_tk, 0, sizeof os_tk); memset(os_sn, 0, sizeof os_sn); memset(os_fl, 0, sizeof os_fl);
    memset(pk_ops, 0, sizeof pk_ops); memset(pk_tk, 0, sizeof pk_tk); memset(pk_sn, 0, sizeof pk_sn);
    gm_steps = 0;
}

void pm_gbi_end_frame(void) {
    /* One second of presented frames, summed. Identical seconds are not written. */
    static int frames, acc_t, acc_i, acc_c, acc_d, acc_o, acc_k, acc_x;
    static char prev[112];
    snprintf(stats_line, sizeof stats_line, "t%d i%d c%d d%d o%d k%d x%d sp%u sl%d gr%u",
             n_cmd, n_in, n_cut, n_drop, n_out, n_keep, have_sample ? sample_x : 0, n_splits, pm_gpu_off_count(), n_grabs);
    PH_ADD(C_CMD, n_ops);
    { static unsigned dlc_fr; if (dlc_on && ++dlc_fr >= 300) { dlc_fr = 0; dlc_report(); } }
    if (dls_on && n_ops) { dls_stamp++; dls_fno++; if (++dls_frames >= 300) dls_report(); }
    if (pm_ph_on || pm_dcs_on) pm_gpu_dcsum_frame(rm_frame); /* DS0: dcsum runs without phase; it must clear pm_dc_c and pm_dc_us every frame */
    if (pm_ph_on) { if (os_fr == 0) os_calibrate(); mdc_frame_end(); if (++os_fr >= 300) { os_report(); mops_report(); mdc_report(); os_fr = 0; } pmk_n = 0; }
    n_ops = 0;
    ep_flush_stats(); gv_epoch++; ep_open = 0;
    if (gp3_t0_pending) { gp3_t0_dead++; gp3_t0_pending = 0; }
    PH_ADD(C_TRI, n_cmd);
    PH_ADD(C_TRIOUT, n_in + n_cut);
    acc_t += n_cmd;
    acc_i += n_in;
    acc_c += n_cut;
    acc_d += n_drop;
    acc_o += n_out;
    acc_k += n_keep;
    if (have_sample) acc_x = sample_x;
    frames++;
    if (frames >= 30) {
        char line[112];
        snprintf(line, sizeof line, "t%d i%d c%d d%d o%d k%d x%d sp%u sl%d gr%u", acc_t, acc_i, acc_c, acc_d, acc_o, acc_k, acc_x, n_splits, pm_gpu_off_count(), n_grabs);
        if (strcmp(line, prev) != 0) {
            if (pm_debug_has("stats")) pm_log_d("stats %s", line); /* L01: per second line, only with the stats switch */
            snprintf(prev, sizeof prev, "%s", line);
        }
        frames = acc_t = acc_i = acc_c = acc_d = acc_o = acc_k = 0;
    }
    if (po3a_on()) { /* one line per 10 frames (a line per frame overflows the log queue in a fast run). Sums over the window, ids: sum and max of the per frame distinct matrix ids. */
        static unsigned w_n, w_ids, w_idmax, w_cmd, w_drawn, w_mixed, w_vcmd, w_vload, w_lit, w_tgen, w_scr, w_primz, w_env, w_plan, w_a255, w_fog, w_boost, w_plain, w_plain2;
        extern unsigned pm_frame_count(void);
        if (p_cmd) {
            w_n++; w_ids += mid_n; if (mid_n > w_idmax) w_idmax = mid_n;
            w_cmd += p_cmd; w_drawn += p_drawn; w_mixed += p_mixed; w_vcmd += p_vcmd; w_vload += p_vload; w_lit += p_lit; w_tgen += p_tgen; w_scr += p_scr;
            w_primz += p_primz; w_env += p_env; w_plan += p_plan; w_a255 += p_a255; w_fog += p_fog; w_boost += p_boost; w_plain += p_plain; w_plain2 += p_plain2;
        }
        if (pm_frame_count() % 10 == 9 && w_n) {
            w_n = w_ids = w_idmax = w_cmd = w_drawn = w_mixed = w_vcmd = w_vload = w_lit = w_tgen = w_scr = w_primz = w_env = w_plan = w_a255 = w_fog = w_boost = w_plain = w_plain2 = 0;
        }
    }
    if (gp_on() && ++gp_frames == 300) { /* PERF-GBI: per frame average in 0.1 ms units (ticks / 26811), tick reads inflate the sections */
        static const char* nm[8] = { "validity,off_screen,box_fold,comb_audit", "ensure_tile", "dual_bind", "reject,cull", "sync_render", "gpu_emit", "old path", "G_VTX" };
        char b[200]; int n = 0;
        for (int k = 0; k < 8; k++) n += snprintf(b + n, sizeof b - n, "%s %u ", k == 3 ? "early" : k == 0 ? "A" : k == 1 ? "ens" : k == 2 ? "dual" : k == 4 ? "sync" : k == 5 ? "emit" : k == 6 ? "old" : "gvtx", gp_acc[k] / 26811 / 300);
        for (int r = 0; r < 20; r++) { /* the twenty most expensive op codes: per frame time in 0.1 ms and count */
            int best = -1; unsigned bt = 0;
            for (int k = 0; k < 256; k++) if (gp_opt[k] > bt) { bt = gp_opt[k]; best = k; }
            if (best < 0) break;
            gp_opt[best] = 0;
        }
        memset(gp_opt, 0, sizeof gp_opt); memset(gp_opn, 0, sizeof gp_opn);
        memset(gp_same, 0, sizeof gp_same); memset(gp_set, 0, sizeof gp_set);
        pm_gpu_gbiprof_report(300);
        { extern void pm_debug_report(unsigned); pm_debug_report(300); }
        { extern void pm_tex_prof_report(unsigned); pm_tex_prof_report(300); }
        (void)nm;
        memset(gp_acc, 0, sizeof gp_acc); gp_ntri = gp_frames = 0;
    }
    mv_sp = 0; /* PT70: the RSP matrix stack starts empty in every task. A push without pop in the message draw leaked one level per frame, then the 10 level cap dropped the pad push of the blue pipe and its pop left the wrong matrix (the sunk pipe showed up while a text box waited) */
    if (gm_n) memset(gm_idx, 0, sizeof gm_idx);
    gm_n = 0; gm_last = GV_NONE; pm_gpu_gv_reset();
    memset(vert_ring, 0xff, sizeof vert_ring);
    memset(vert_gvf, 3, sizeof vert_gvf);
    p_cmd = p_mixed = p_vload = p_vcmd = p_lit = p_tgen = p_scr = p_drawn = p_primz = p_env = p_plan = p_a255 = p_fog = p_boost = p_plain = p_plain2 = 0;
    mid_n = 0;
    n_cmd = n_in = n_cut = n_drop = n_out = n_keep = 0;
    n_splits = n_grabs = 0;
    have_sample = 0;
    rm_frame++;
    os_off = ((unsigned)rm_frame * 37u) & 63u; /* W1: the sample phase changes each frame */
}

const char* pm_gbi_stats(void) { return stats_line; }

void pm_gbi_init(void) {
    ident(proj);
    ident(mv[0]);
    mv_sp = 0;
    memset(vert_live, 0, sizeof vert_live);
    geom = G_SHADE | G_SHADING_SMOOTH;
    upload_mvp();
}


/* ---- RDP color combiner, one cycle, for work the display list does off screen ---- */
static void unpack8(unsigned c, float o[4]) {
    o[0] = pm_f255[(c >> 24) & 255u];
    o[1] = pm_f255[(c >> 16) & 255u];
    o[2] = pm_f255[(c >> 8) & 255u];
    o[3] = pm_f255[c & 255u];
}

static float cc_rgb_in(unsigned sel, int ch, const float* t0, const float* t1, const float* pr,
                       const float* en, int a_ok) {
    switch (sel) {
    case 1: return t0[ch];
    case 2: return t1[ch];
    case 3: return pr[ch];
    case 4: return 1.f; /* SHADE: none off screen */
    case 5: return en[ch];
    case 6: return a_ok ? 1.f : 0.f;
    default: return 0.f;
    }
}

/* (A - B) * C + D, cycle 1 only. COMBINED reads as 0 there. */
static void cc_cycle1(const float* t0, const float* t1, float out[4]) {
    float pr[4], en[4];
    unpack8(prim, pr);
    unpack8(envc, en);
    for (int ch = 0; ch < 3; ch++) {
        float a = cc_rgb_in(comb_a, ch, t0, t1, pr, en, 1);
        float b = cc_rgb_in(comb_b, ch, t0, t1, pr, en, 0);
        float c;
        switch (comb_c) {
        case 8: c = t0[3]; break;
        case 9: c = t1[3]; break;
        case 10: c = pr[3]; break;
        case 11: c = 1.f; break;
        case 12: c = en[3]; break;
        default: c = cc_rgb_in(comb_c, ch, t0, t1, pr, en, 0); break;
        }
        float d = cc_rgb_in(comb_d, ch, t0, t1, pr, en, 1);
        float v = (a - b) * c + d;
        out[ch] = v < 0.f ? 0.f : v > 1.f ? 1.f : v;
    }
    {
        unsigned sel[4] = { comb_aa, comb_ab, comb_ac, comb_ad };
        float v[4];
        for (int i = 0; i < 4; i++) {
            switch (sel[i]) {
            case 1: v[i] = t0[3]; break;
            case 2: v[i] = t1[3]; break;
            case 3: v[i] = pr[3]; break;
            case 4: v[i] = 1.f; break;
            case 5: v[i] = en[3]; break;
            case 6: v[i] = (i == 2) ? 0.f : 1.f; break;
            default: v[i] = 0.f; break;
            }
        }
        float r = (v[0] - v[1]) * v[2] + v[3];
        out[3] = r < 0.f ? 0.f : r > 1.f ? 1.f : r;
    }
}

static void pal_rgba(const void* base, unsigned i, float o[4]) {
    const u8* p = (const u8*)base;
    unsigned c = 0;
    if (p) {
        p += (i & 15u) * 2u;
        c = ((unsigned)p[0] << 8) | p[1];
    }
    float r = (float)((c >> 11) & 31u) / 31.f, g = (float)((c >> 6) & 31u) / 31.f,
          b = (float)((c >> 1) & 31u) / 31.f;
    o[0] = r; o[1] = g; o[2] = b; o[3] = (c & 1u) ? 1.f : 0.f;
}

/* A texture rectangle into an off-screen color image, sampled from a TLUT held in TMEM. The
 * shaded-sprite code draws its 16-entry shadow/highlight palette this way. Returns 1 if done. */
static __attribute__((noinline)) int rect_to_image(int xl, int yl, int xh, int yh, unsigned tile, u32 hs, u32 hd) {
    if (!cimg || pm_is_cfb(cimg) || tile >= 8 || tile_tmem[tile] < 256) return 0;
    int x0 = xl >> 2, y0 = yl >> 2, x1 = xh >> 2, y1 = yh >> 2;
    if (x0 < sci_x0) x0 = sci_x0;
    if (y0 < sci_y0) y0 = sci_y0;
    if (x1 > sci_x1) x1 = sci_x1;
    if (y1 > sci_y1) y1 = sci_y1;
    float s0 = (float)(s16)(hs >> 16) / 32.f;
    float t0 = (float)(s16)(hs & 0xffff) / 32.f;
    float dsdx = (float)(s16)(hd >> 16) / 1024.f;
    float dtdy = (float)(s16)(hd & 0xffff) / 1024.f;
    float sl = (float)tile_sz[tile][0] / 4.f, tl = (float)tile_sz[tile][1] / 4.f;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            float s = s0 + dsdx * (float)(x - (xl >> 2)) - sl;
            float t = t0 + dtdy * (float)(y - (yl >> 2)) - tl;
            int ts = (int)s;
            (void)t;
            if (ts < 0) ts = 0;
            /* A TLUT in TMEM holds each entry four times in a row. One row only. */
            unsigned word = tile_tmem[tile] + (unsigned)(ts >> 2);
            unsigned entry = word - 256u;
            if (entry > 255u) continue;
            float tx[4], none[4] = { 0, 0, 0, 0 }, o[4];
            pal_rgba(tlut_pal[entry >> 4], entry & 15u, tx);
            cc_cycle1(tx, none, o);
            unsigned r = (unsigned)(o[0] * 31.f + .5f), g = (unsigned)(o[1] * 31.f + .5f),
                     b = (unsigned)(o[2] * 31.f + .5f), a = o[3] >= .5f;
            unsigned c = (r << 11) | (g << 6) | (b << 1) | a;
            u8* d = (u8*)cimg + ((size_t)y * (size_t)cimg_stride + (size_t)x) * 2u;
            d[0] = (u8)(c >> 8);
            d[1] = (u8)c;
        }
    }
    pm_tex_invalidate(cimg, 64);
    return 1;
}

/* Tile 0 reads the shading palette, tile 1 the sprite palette, both through the same 4-bit
 * indices, and the first combiner cycle multiplies them. Build the 16 x 16 result the
 * combiner would give, and let pm_tex apply it per texel. Returns 1 if the texture is bound. */
static int load_shaded_sprite(int dx4, int dy4) {
    const void* sp = tlut_pal[tile_pal[1] & 15];
    const void* sh = tlut_pal[tile_pal[0] & 15];
    unsigned lut[256];
    float env[4];
    if (!sp || !sh) return 0;
    unpack8(envc, env);
    /* Second cycle (SHADE - ENV) * COMBINED + COMBINED is linear in the vertex shade. It
     * can brighten past 1, so fold the largest factor into the texture and the rest into the
     * vertex color (see draw_tri). */
    (void)env;
    dual_gain[0] = dual_gain[1] = dual_gain[2] = 1.f;
    for (unsigned i = 0; i < 16; i++) {
        float t1[4];
        pal_rgba(sp, i, t1);
        for (unsigned j = 0; j < 16; j++) {
            float t0[4], o[4];
            pal_rgba(sh, j, t0);
            cc_cycle1(t0, t1, o);
            unsigned c[4];
            for (int k = 0; k < 4; k++) {
                float v = o[k] * (k < 3 ? dual_gain[k] : 1.f);
                if (v > 1.f) v = 1.f;
                c[k] = (unsigned)(v * 255.f + .5f);
            }
            lut[i * 16 + j] = (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3];
        }
    }
    return pm_tex_load_shaded(m1.src, m1.w, m1.h, m1.stride, sp, lut, dx4, dy4);
}

/* Bind the texture of tile n (0 or 1) for the next draw. */
static const void* ens_img[2];
static const void* ens_src;
static const void* ens_pal;
static unsigned ens_fmt, ens_siz;
static unsigned ens_wrap[4];
static int ens_sz[4];
static void ens_clear(void) { ens_src = NULL; ens_pal = NULL; }
static int tile_bound(int n) {
    if (!td[n].valid || pm_tex_cur_idx() < 0 || !ens_src) return 0;
    if (ens_src != td[n].src || ens_pal != td[n].pal || ens_fmt != td[n].fmt || ens_siz != td[n].siz) return 0;
    if (memcmp(tile_wrap[n], ens_wrap, sizeof ens_wrap) || memcmp(tile_sz[n], ens_sz, sizeof ens_sz)) return 0;
    return 1;
}
/* GA1: a glyph (4 bit CI, message font) is one cell of the sheet atlas. When the atlas of its sheet and palette is the bound
 * texture, the glyph needs no flush and no wrap call: tile_bound compares the raster, which differs per glyph. */
static int glyph_atlas_bound(void) {
    int gx, gy;
    if (!td[0].valid || !td[0].src || td[0].fmt != 2 || td[0].siz != 0 || !ens_src) { RS_ADD(RS_GA_TD, 1); return 0; }
    if (ens_pal != td[0].pal || ens_fmt != 2 || ens_siz != 0) { RS_ADD(RS_GA_ENS, 1); return 0; }
    if (memcmp(tile_wrap[0], ens_wrap, sizeof ens_wrap) || memcmp(tile_sz[0], ens_sz, sizeof ens_sz)) { RS_ADD(RS_GA_WRAP, 1); return 0; }
    if (!pm_tex_glyph_same(td[0].src, td[0].fmt, td[0].siz, td[0].w, td[0].h, td[0].pal, td[0].pal_n, &gx, &gy)) { RS_ADD(RS_GA_GLY, 1); return 0; }
    return 1;
}
static void ensure_tile(int n) {
    if (!td[n].valid) return;
    tile_fx = (float)(tile_sz[n][0] & 3) * 0.25f; /* PT38 */
    tile_fy = (float)(tile_sz[n][1] & 3) * 0.25f;
    if (n == 0 && !tile_bound(0) && glyph_atlas_bound()) {
        RS_ADD(RS_GAB, 1);
        cur_tile = 0;
        return;
    }
    if (tile_bound(n)) {
        cur_tile = n;
        return;
    }
    RS_ADD(RS_TILE, 1);
    RS_ADD(n ? RS_T1 : RS_T0, 1);
    pm_gpu_flush();
    pm_tex_set_wrap(tile_wrap[n][0], tile_wrap[n][1], tile_wrap[n][2], tile_wrap[n][3]);
    if (td[n].fmt == 0 && td[n].siz == 2) grab_cur(td[n].src, td[n].w, td[n].h);
    pm_tex_load(td[n].src, td[n].fmt, td[n].siz, td[n].w, td[n].h, td[n].pal, td[n].pal_n, td[n].stride);
    {   /* V05: a window of an off-screen texture starts at (ox, oy) in it */
        int ox, oy, gox, goy;
        pm_tex_origin(&ox, &oy);
        pm_tex_glyph_ofs(&gox, &goy);
        tile_x0 = td[n].x0 - ox - (n == 0 ? gox : 0);
        tile_y0 = td[n].y0 - oy - (n == 0 ? goy : 0);
        if (n == 0) {
            tsz_last.ox = ox - gox;
            tsz_last.oy = oy - goy;
            tsz_last.ep = pm_tex_epoch();
            tsz_last.ok = pm_tex_off_idx() < 0;
        }
    }
    ens_src = td[n].src;
    ens_pal = td[n].pal;
    ens_fmt = td[n].fmt;
    ens_siz = td[n].siz;
    memcpy(ens_wrap, tile_wrap[n], sizeof ens_wrap);
    memcpy(ens_sz, tile_sz[n], sizeof ens_sz);
    ens_img[n] = td[n].src;
    cur_tile = n;
}

/* D03: one texel of tile n as RGBA 0..255. Tile 0 wraps, tile 1 clamps (draw_box: pattern and corner image).
 * Returns 0 for a format not handled. */
static int tile_texel(int n, int s, int t, unsigned char o[4]) {
    const u8* p = td[n].src;
    int w = td[n].w, h = td[n].h;
    if (n == 0) {
        s %= w; if (s < 0) s += w;
        t %= h; if (t < 0) t += h;
    } else {
        if (s < 0) s = 0; else if (s >= w) s = w - 1;
        if (t < 0) t = 0; else if (t >= h) t = h - 1;
    }
    size_t i = (size_t)t * (size_t)td[n].stride + (size_t)s;
    unsigned c;
    switch (td[n].fmt * 4 + td[n].siz) {
    case 4 * 4 + 0: { u8 b = p[i >> 1]; unsigned v = ((i & 1) ? (b & 15u) : (b >> 4)) * 17u; o[0] = o[1] = o[2] = o[3] = (u8)v; return 1; }
    case 4 * 4 + 1: o[0] = o[1] = o[2] = o[3] = p[i]; return 1;
    case 3 * 4 + 0: { u8 b = p[i >> 1]; unsigned v = (i & 1) ? (b & 15u) : (b >> 4); o[0] = o[1] = o[2] = (u8)((v >> 1) * 36u); o[3] = (v & 1u) ? 255 : 0; return 1; }
    case 3 * 4 + 1: o[0] = o[1] = o[2] = (u8)((p[i] >> 4) * 17u); o[3] = (u8)((p[i] & 15u) * 17u); return 1;
    case 3 * 4 + 2: o[0] = o[1] = o[2] = p[i * 2]; o[3] = p[i * 2 + 1]; return 1;
    case 0 * 4 + 2: c = ((unsigned)p[i * 2] << 8) | p[i * 2 + 1]; break;
    case 2 * 4 + 0: case 2 * 4 + 1: {
        unsigned idx;
        if (!td[n].pal) return 0;
        if (td[n].siz == 0) { u8 b = p[i >> 1]; idx = (i & 1) ? (b & 15u) : (b >> 4); } else idx = p[i];
        c = ((unsigned)((const u8*)td[n].pal)[idx * 2] << 8) | ((const u8*)td[n].pal)[idx * 2 + 1];
        break;
    }
    default: return 0;
    }
    o[0] = (u8)(((c >> 11) & 31u) * 255u / 31u);
    o[1] = (u8)(((c >> 6) & 31u) * 255u / 31u);
    o[2] = (u8)(((c >> 1) & 31u) * 255u / 31u);
    o[3] = (c & 1u) ? 255 : 0;
    return 1;
}

/* D03: a draw_box window with the bg_tile pattern. The combiner is (PRIM - ENV) * (TEXEL0 * TEXEL1) + ENV, alpha
 * TEXEL1: tile 0 is the wrapped pattern, tile 1 the corner image, clamped, so its edge texels make the border.
 * The PICA path binds one texture, so the product TEXEL0 * TEXEL1 (colour) and TEXEL1 alpha are composed on the
 * CPU into an RGBA texture (cached by content, 128 x 128 pixel chunks), then drawn with (PRIM - ENV) * texel + ENV.
 * Returns 1 if drawn. */
static unsigned char box_buf[128 * 128 * 4];
static __attribute__((noinline)) int box_pattern_rect(int xl, int yl, int xh, int yh, u32 hs, u32 hd) {
    int X0 = xl >> 2, Y0 = yl >> 2, X1 = xh >> 2, Y1 = yh >> 2; /* exclusive lower right, 1-cycle mode */
    float s0 = (float)(s16)(hs >> 16) / 32.f, t0 = (float)(s16)(hs & 0xffff) / 32.f;
    float dsdx = (float)(s16)(hd >> 16) / 1024.f, dtdy = (float)(s16)(hd & 0xffff) / 1024.f;
    unsigned char tst[4];
    if (!box_pat || !td[0].valid || !td[1].valid || !td[0].src || !td[1].src) return 0;
    if (!tile_texel(0, 0, 0, tst) || !tile_texel(1, 0, 0, tst)) return 0;
    int cx0 = X0 < 0 ? 0 : X0, cy0 = Y0 < 0 ? 0 : Y0, cx1 = X1 > 320 ? 320 : X1, cy1 = Y1 > 240 ? 240 : Y1;
    pm_gpu_flush();
    pm_gpu_set_combine(5);
    sc_ok = 0; sr_ok = 0; /* DIP2: pm_gpu holds a combine mode sync_combine did not set */
    for (int ys = cy0; ys < cy1; ys += 128) {
        for (int xs = cx0; xs < cx1; xs += 128) {
            int cw = cx1 - xs < 128 ? cx1 - xs : 128, ch = cy1 - ys < 128 ? cy1 - ys : 128;
            int sa = (int)__builtin_floorf(s0 + dsdx * (float)(xs - X0));
            int ta = (int)__builtin_floorf(t0 + dtdy * (float)(ys - Y0));
            unsigned key = 2166136261u;
            unsigned parts[] = { (unsigned)td[0].src, (unsigned)td[1].src, (unsigned)td[0].x0, (unsigned)td[0].y0, (unsigned)td[1].x0,
                                 (unsigned)td[1].y0, (unsigned)(td[0].w | td[0].h << 8 | td[1].w << 16 | td[1].h << 24),
                                 (unsigned)(td[0].fmt | td[0].siz << 3 | td[1].fmt << 6 | td[1].siz << 9), (unsigned)sa, (unsigned)ta,
                                 hd, (unsigned)(cw | ch << 8), (unsigned)td[0].stride | (unsigned)td[1].stride << 16 };
            for (unsigned k = 0; k < sizeof parts / sizeof parts[0]; k++) key = (key ^ parts[k]) * 16777619u;
            if (!pm_tex_load_rgba8(box_buf, key, cw, ch, NULL)) {
                for (int y = 0; y < ch; y++) {
                    for (int x = 0; x < cw; x++) {
                        int S = (int)__builtin_floorf(s0 + dsdx * (float)(xs + x - X0));
                        int T = (int)__builtin_floorf(t0 + dtdy * (float)(ys + y - Y0));
                        unsigned char a[4], b[4];
                        tile_texel(0, S - td[0].x0, T - td[0].y0, a);
                        tile_texel(1, S - td[1].x0, T - td[1].y0, b);
                        unsigned char* d = &box_buf[(y * cw + x) * 4];
                        d[0] = (unsigned char)(a[0] * b[0] / 255u);
                        d[1] = (unsigned char)(a[1] * b[1] / 255u);
                        d[2] = (unsigned char)(a[2] * b[2] / 255u);
                        d[3] = b[3];
                    }
                }
                if (!pm_tex_load_rgba8(box_buf, key, cw, ch, box_buf)) { sync_combine(); return 0; }
            }
            int tw = 1, th = 1;
            pm_tex_size(&tw, &th);
            pm_gpu_tex_rect(xs, ys, xs + cw - 1, ys + ch - 1, 0.f, 0.f, (float)cw / (float)tw, (float)ch / (float)th);
        }
    }
    cur_tile = -1; /* the bound texture is the composite */
    ens_clear();
    sync_combine();
    return 1;
}

/* G1: ops with an empty handler (sync and no-op). The loop skips them before the prologue. */
static const u8 g1_skip[256] = { [G_NOOP] = 1, [G_SPNOOP] = 1, [G_RDPPIPESYNC] = 1, [G_RDPTILESYNC] = 1, [G_RDPLOADSYNC] = 1, [G_RDPFULLSYNC] = 1 };

/* C1: texture bind records. tb_list[idx] holds the handle's Gfx list only when pm_texbind_set has checked it. A list
 * holds only the ops make_texture_gfx emits (and g1_skip ops). A refused list stores NULL. */
static const Gfx* tb_list[128];
void pm_texbind_set(int idx, const Gfx* l) {
    tb_list[idx & 0x7f] = NULL;
    if (!l) return;
    for (int n = 0; n < 64; n++) {
        unsigned op = l[n].words.w0 >> 24;
        if (op == G_ENDDL) { tb_list[idx & 0x7f] = l; return; }
        if (g1_skip[op]) continue;
        if (op != G_SETTIMG && op != G_SETTILE && op != G_LOADBLOCK && op != G_LOADTILE && op != G_LOADTLUT && op != G_SETTILESIZE &&
            op != G_TEXTURE && op != G_SETCOMBINE && op != G_SETOTHERMODE_H) return;
    }
}
static __attribute__((noinline)) void tex_op(u32 w0, u32 w1);
/* C1: runs a checked texture list. The G_PM_TEX op pays the prologue once. The handlers run per word, in list order. */
static __attribute__((noinline)) void tb_run(const Gfx* g) {
    for (;; g++) {
        unsigned op = g->words.w0 >> 24;
        if (op == G_ENDDL) return;
        if (g1_skip[op]) continue;
        pm_gbi_cur_op = op;
        tex_op(g->words.w0, g->words.w1);
    }
}

/* GLYPH: runs n texture-load words after a G_PM_TEXI header, with the tex_op handlers. Syncs are skipped as in G1. */
static __attribute__((noinline)) void tb_inline(const Gfx* g, unsigned n) {
    for (; n; n--, g++) {
        unsigned op = g->words.w0 >> 24;
        if (g1_skip[op]) continue;
        pm_gbi_cur_op = op;
        tex_op(g->words.w0, g->words.w1);
    }
}

/* G2: a state op that leaves the live state as it is. The loop skips it before the prologue. Ops that call sync_combine
 * also need sc_ok: a set sc_ok means that call would do nothing. */
/* R5B: a G_MOVEMEM light that writes the values already in lt[slot] changes nothing. The same op also runs lit_freeze, which
 * acts when a lit vertex is pending, so the skip needs lit_pending == 0 (then lit_freeze returns at once). */
static int lt_mem_same(u32 w0, u32 w1) {
    int slot = (int)(((w0 >> 8) & 0xff) * 8 / 24) - 2;
    const u8* l = (const u8*)ptr_of(w1);
    return l && slot >= 0 && slot < 9 && !lit_pending && lt[slot].col[0] == l[0] && lt[slot].col[1] == l[1] && lt[slot].col[2] == l[2] &&
           lt[slot].dir[0] == (s8)l[8] && lt[slot].dir[1] == (s8)l[9] && lt[slot].dir[2] == (s8)l[10];
}

static int g2_same(unsigned op, u32 w0, u32 w1) {
    switch (op) {
    /* R2B: each case skips only when the handler's writes equal the current state and its calls do nothing (sync_combine: sc_ok and
     * the same input hash; pm_gpu_set_mode: the mode already set). */
    case G_SETCOMBINE:
        return comb_set && comb_w0 == w0 && comb_w1 == w1 && box_folded == 0 && cmb_gen_seen == comb_gen && sc_ok &&
               sc_sig_now() == sc_sig;
    case G_TEXTURE:
        return tex_on == ((w0 & 0xff) != 0) && tex_ss == (w1 >> 16) && tex_ts == (w1 & 0xffffu) && sc_ok && sc_sig_now() == sc_sig;
    case G_GEOMETRYMODE: {
        unsigned g = (geom & (w0 & 0xffffffu)) | w1;
        return g == geom && pm_gpu_mode_is((geom & G_ZBUFFER) != 0, (geom & G_LIGHTING) != 0);
    }
    case G_MOVEMEM: return (w0 & 0xff) == G_MV_LIGHT && lt_mem_same(w0, w1);
    case G_MOVEWORD: { /* R5B: the handler of G_MW_LIGHTCOL writes lt[k].col only (no other effect) */
        unsigned index = (w0 >> 16) & 0xff, offset = w0 & 0xffff, k = offset / 24;
        return index == G_MW_LIGHTCOL && k < 9 && (offset % 24) == 0 && lt[k].col[0] == (u8)(w1 >> 24) && lt[k].col[1] == (u8)(w1 >> 16) &&
               lt[k].col[2] == (u8)(w1 >> 8);
    }
    case G_SETPRIMCOLOR: return prim == w1 && aud_prim_lod == (u8)(w0 & 0xff) && sc_ok;
    case G_SETENVCOLOR: return envc == w1 && sc_ok;
    case G_SETFOGCOLOR: return fogc == w1;
    case G_SETBLENDCOLOR: return blendc == w1;
    case G_SETOTHERMODE_L:
    case G_SETOTHERMODE_H: {
        unsigned len = (w0 & 0xff) + 1, field = (w0 >> 8) & 0xff;
        unsigned sft = 32 - field - len;
        unsigned mask = (len >= 32) ? 0xffffffffu : (((1u << len) - 1u) << sft);
        unsigned old = op == G_SETOTHERMODE_H ? omode_h : omode_l;
        return (w1 & mask) == (old & mask) && (op == G_SETOTHERMODE_L || sc_ok);
    }
    default: return 0;
    }
}

/* C2: G_PM_RM (gSPPmRenderMode) w1 = a ModelRenderModes list (static, host memory). The list must hold only
 * SETOTHERMODE_L/H, GEOMETRYMODE and ENDDL. Its effects run here in list order, with the same state writes and the same
 * side-effect calls as the handlers for those ops (filter flush, sync_combine on the cycle type, pm_gpu_set_mode).
 * Returns 0 without any effect when the list has another op; the caller then runs it as a G_DL. */
static int rm_apply(u32 w1) {
    const Gfx* g = (const Gfx*)w1;
    int n = 0;
    for (;;) {
        unsigned op = g[n].words.w0 >> 24;
        if (op == G_ENDDL) break;
        if ((op != G_SETOTHERMODE_L && op != G_SETOTHERMODE_H && op != G_GEOMETRYMODE && op != G_SETCOMBINE && op != G_TEXTURE) || ++n >= 8) return 0;
    }
    for (int k = 0; k < n; k++) {
        u32 a0 = g[k].words.w0, a1 = g[k].words.w1;
        unsigned op = a0 >> 24;
        if (op == G_GEOMETRYMODE) {
            geom = (geom & (a0 & 0xffffffu)) | a1;
            pm_gpu_set_mode((geom & G_ZBUFFER) != 0, (geom & G_LIGHTING) != 0);
            continue;
        }
        if (op == G_SETCOMBINE || op == G_TEXTURE) { tex_op(a0, a1); continue; } /* DLF5: the dispatch handler, in list order */
        unsigned len = (a0 & 0xff) + 1, field = (a0 >> 8) & 0xff, sft = 32 - field - len;
        unsigned mask = (len >= 32) ? 0xffffffffu : (((1u << len) - 1u) << sft);
        if (op == G_SETOTHERMODE_H) {
            unsigned old_tf = omode_h & (3u << 12);
            omode_h = (omode_h & ~mask) | (a1 & mask);
            if ((omode_h & (3u << 12)) != old_tf) {
                pm_gpu_flush();
                pm_tex_set_filter((omode_h & (3u << 12)) != 0);
            }
            if (sft <= G_MDSFT_CYCLETYPE && sft + len > G_MDSFT_CYCLETYPE) sync_combine();
        } else {
            omode_l = (omode_l & ~mask) | (a1 & mask);
        }
    }
    return 1;
}

/* H1: rare paths of gbi_run_body. Cold and noinline, so the hot loop does not carry their code. */
static __attribute__((cold, noinline)) void gbi_cold_bad_dl(u32 pc, u32 list, unsigned nbytes, int sp) {
    static int bad; if (bad++ < 6) pm_log_e("bad dl pc %08x list %08x n %u sp %d", pc, list, nbytes, sp);
}
static __attribute__((cold, noinline)) void gbi_cold_bad_next(u32 w1, u32 w0, u32 list) {
    static int bad; if (bad++ < 8) pm_log_e("bad dl target %08x w0 %08x list %08x", w1, w0, list);
}
static __attribute__((cold, noinline)) void gbi_cold_bad_vtx(u32 w1, u32 w0, u32 list) {
    static int bad; if (bad++ < 8) pm_log_e("bad vtx %08x w0 %08x list %08x", w1, w0, list);
}
static __attribute__((cold, noinline)) void gbi_cold_bad_mtx(u32 w0, u32 w1, Gfx* list, Gfx* pc) {
    static unsigned bad; if (bad++ < 6) pm_log_w("gbi: G_MTX w0 %08x w1 %08x outside host memory, list %p at op %d, frame %d, previous ops %08x %08x %08x %08x", w0, w1, (void*)list, (int)(pc - list), rm_frame,
        pc > list ? pc[-1].words.w0 : 0, pc > list ? pc[-1].words.w1 : 0, pc > list + 1 ? pc[-2].words.w0 : 0, pc > list + 1 ? pc[-2].words.w1 : 0);
}
static __attribute__((cold, noinline)) unsigned gbi_cold_po3a_hash(void) {
    unsigned h = 2166136261u;
    for (int q = 0; q < 16; q++) { unsigned u; memcpy(&u, &((const float*)combined)[q], 4); h = (h ^ u) * 16777619u; }
    unsigned k;
    for (k = 0; k < mid_n; k++) if (mid_list[k] == h) break;
    if (k == mid_n && mid_n < 256) mid_list[mid_n++] = h;
    return h;
}
static __attribute__((cold, noinline)) void gbi_cold_dd_load(const Vtx* src) {
    dd_end(); dd.idx = dd_idx++; dd.top = dd_top; dd.src = (u32)src; dd.vc = __builtin_bswap32(*(const u32*)((const u8*)src + 12)); dd.gm = geom; dd.dl0 = dd_dl0; dd.dl1 = dd_dl1;
}
static __attribute__((cold, noinline)) void gbi_cold_modifyvtx(u32 w0, u32 w1) { /* B5: F3DEX2 w0 = op, where, vertex * 2. Only G_MWO_POINT_XYSCREEN is used by the game (underwater.c) */
    unsigned where = (w0 >> 16) & 0xff, vi = (w0 & 0xffffu) / 2;
    if (where == G_MWO_POINT_XYSCREEN && vi < 64) {
        vert_sx[vi] = (s16)(w1 >> 16);
        vert_sy[vi] = (s16)(w1 & 0xffffu);
        vert_scr[vi] = 1;
    }
}
/* C1-0: texture and combine state ops, out of the dispatch loop. Uses only w0, w1 and file state. */
static __attribute__((noinline)) void tex_op(u32 w0, u32 w1) {
    unsigned op = w0 >> 24;
    switch (op) {
        case G_SETTIMG:
            timg = ptr_of(w1);
            if (!w1 && zslot_on && ((w0 >> 19) & 3) == 2) timg = zb_fake; /* B5: the underwater mesh reads the z-buffer slot */
            imglog("settimg", timg, (int)((w0 >> 19) & 3), (int)(w0 & 0xfff) + 1, -1);
            /* D03: a texture read from a frame buffer is the last frame. Fetch it now. */
            if (timg && pm_is_cfb(timg) && !pm_is_cur_cfb(timg) && !pm_interp_repl) pm_cfb_capture_prev(); /* F06-5: key pass only */ /* B2: the frame buffer being drawn is a grab (T2, grab_cur) */
            timg_fmt = (w0 >> 21) & 7;
            timg_siz = (w0 >> 19) & 3;
            timg_stride = (int)(w0 & 0xfff) + 1;
            ld_valid = 0;
            break;
        case G_SETTILE: {
            unsigned tile = (w1 >> 24) & 7;
            int same_tile = tile == 0 && st0_ok && pm_tex_wrap_gen() == st0_ep && tile_fmt[0] == ((w0 >> 21) & 7) && tile_siz[0] == ((w0 >> 19) & 3) && tile_tmem[0] == (w0 & 0x1ff) && tile_pal[0] == ((w1 >> 20) & 15) &&
                          tile_wrap[0][0] == ((w1 >> 8) & 3) && tile_wrap[0][1] == ((w1 >> 18) & 3) && tile_wrap[0][2] == ((w1 >> 4) & 15) && tile_wrap[0][3] == ((w1 >> 14) & 15) &&
                          tile_shift[0][0] == (w1 & 15) && tile_shift[0][1] == ((w1 >> 10) & 15);
            if (tile == 0) GP_SET(3, same_tile);
            tile_fmt[tile] = (w0 >> 21) & 7;
            tile_siz[tile] = (w0 >> 19) & 3;
            tile_tmem[tile] = w0 & 0x1ff;
            tile_pal[tile] = (w1 >> 20) & 15;
            tile_wrap[tile][0] = (w1 >> 8) & 3;
            tile_wrap[tile][1] = (w1 >> 18) & 3;
            tile_wrap[tile][2] = (w1 >> 4) & 15;
            tile_wrap[tile][3] = (w1 >> 14) & 15;
            tile_shift[tile][0] = w1 & 15;
            tile_shift[tile][1] = (w1 >> 10) & 15;
            /* The render tile's wrap belongs to the triangles already queued.
             * Draw them before this tile replaces it. */
            if (tile == 0 && !same_tile) { /* PG1: an identical G_SETTILE changes nothing: no flush, no wrap call, no new texture epoch */
                pm_gpu_flush();
                pm_tex_set_wrap((w1 >> 8) & 3, (w1 >> 18) & 3, (w1 >> 4) & 15, (w1 >> 14) & 15);
                st0_ep = pm_tex_wrap_gen(); st0_ok = 1; /* the wrap state pm_tex has now is the one of tile 0: an identical G_SETTILE may skip while nothing else touched pm_tex */
            }
            break;
        }
        case G_LOADBLOCK: { /* V09: a block load replaces what a LoadTile put at that TMEM address */
            unsigned tm = tile_tmem[(w1 >> 24) & 7];
            {
                /* V13: the block also covers the TMEM above it (a half-height aux tile reads there) */
                unsigned words = ((((w1 >> 12) & 0xfff) + 1u) << tile_siz[(w1 >> 24) & 7]) >> 4;
                for (unsigned k = 0; k < 4; k++) if (ldrec[k].tmem >= tm && (ldrec[k].tmem == tm || ldrec[k].tmem < tm + words)) ldrec[k].img = NULL;
            }
            {
                unsigned k;
                for (k = 0; k < 4 && !(blk[k].img && blk[k].tmem == tm); k++) {}
                if (k == 4) k = blk_n++ & 3;
                blk[k].tmem = tm; blk[k].img = timg;
                blk[k].bytes = ((((w1 >> 12) & 0xfff) + 1u) << tile_siz[(w1 >> 24) & 7]) >> 1;
            }
            break;
        }
        case G_LOADTILE:
            ld_valid = 1;
            ld_x = (int)((w0 >> 12) & 0xfff) >> 2;
            {
                unsigned tm = tile_tmem[(w1 >> 24) & 7], k;
                for (k = 0; k < 4 && !(ldrec[k].img && ldrec[k].tmem == tm); k++) {}
                if (k == 4) k = ldrec_n++ & 3;
                ldrec[k].tmem = tm; ldrec[k].img = timg; ldrec[k].stride = timg_stride;
                ldrec[k].x = (int)((w0 >> 12) & 0xfff) >> 2; ldrec[k].y = (int)(w0 & 0xfff) >> 2;
            }
            ld_y = (int)(w0 & 0xfff) >> 2;
            ld_h = (((int)(w1 & 0xfff) - (int)(w0 & 0xfff)) >> 2) + 1;
            ld_w = ((((int)(w1 >> 12) & 0xfff) - ((int)(w0 >> 12) & 0xfff)) >> 2) + 1;
            break;
        case G_LOADTLUT:
            tlut = timg;
            tlut_n = (int)((w1 >> 14) & 0x3ff) + 1;
            {
                /* The palette slot is the 16-entry block at the load tile's TMEM address. */
                unsigned slot = (tile_tmem[(w1 >> 24) & 7] - 256u) >> 4;
                if (slot < 16) {
                    tlut_pal[slot] = timg;
                    /* V20: gDPLoadTextureBlock_4b sets the tile size (which binds the texture) BEFORE the palette load. A bound CI tile that
                     * uses this palette bank must take the new palette, else it keeps the one of the previous model (the brick block was
                     * drawn with a dark grey palette). */
                    for (int n = 0; n < 2; n++)
                        if (td[n].valid && td[n].fmt == 2 && (tile_pal[n] & 15) == (int)slot && td[n].pal != timg) {
                            pm_gpu_flush();
                            td[n].pal = timg;
                            td[n].pal_n = tlut_n;
                            if (n == cur_tile) cur_tile = -1;
                        }
                }
            }
            break;
        case G_SETTILESIZE: {
            unsigned tile = (w1 >> 24) & 7;
            int uls = (int)((w0 >> 12) & 0xfff);
            int ult = (int)(w0 & 0xfff);
            int lrs = (int)((w1 >> 12) & 0xfff);
            int lrt = (int)(w1 & 0xfff);
            if (((lrs - uls) >> 2) + 1 <= 0 || ((lrt - ult) >> 2) + 1 <= 0) {
                /* S25: a window with a negative start (a scrolled texture) is 12 bit two's complement. Texel 0 is
                 * still the first texel of the loaded image, so keep the size and start at 0. */
                int su = uls & 0x800 ? uls - 0x1000 : uls, st = ult & 0x800 ? ult - 0x1000 : ult;
                int sr = lrs & 0x800 ? lrs - 0x1000 : lrs, sl = lrt & 0x800 ? lrt - 0x1000 : lrt;
                if (sr >= su && sl >= st) {
                    lrs = (su < 0 ? 0 : su) + (sr - su);
                    lrt = (st < 0 ? 0 : st) + (sl - st);
                    uls = su < 0 ? 0 : su;
                    ult = st < 0 ? 0 : st;
                }
            }
            GP_SET(4, tile_sz[tile][0] == uls && tile_sz[tile][1] == ult && tile_sz[tile][2] == lrs && tile_sz[tile][3] == lrt);
            tile_sz[tile][0] = uls;
            tile_sz[tile][1] = ult;
            tile_sz[tile][2] = lrs;
            tile_sz[tile][3] = lrt;
            {   /* imglog: the SETTILESIZE window and the SETTIMG image of the same tile (role 9 = tile line) */
                static int on = -1;
                if (on < 0) on = pm_debug_has("imglog");
            }
            if (tile == 1 && tile_fmt[1] == 2 && tile_siz[1] == 0 && ld_valid && timg) {
                /* Second tile on the same 4-bit image: remember the raster for the shaded sprite. */
                m1.valid = 1;
                m1.src = (const u8*)timg + (size_t)ld_y * (size_t)timg_stride + (size_t)ld_x;
                m1.stride = timg_stride * 2;
                m1.w = ((lrs - uls) >> 2) + 1;
                m1.h = ((lrt - ult) >> 2) + 1;
                m1.x0 = uls >> 2;
                m1.y0 = ult >> 2;
            }
            if (tile == 1) {
                int w = ((lrs - uls) >> 2) + 1, h = ((lrt - ult) >> 2) + 1;
                if (tile_wrap[1][2] && !(tile_wrap[1][0] & 2) && ld_valid) { /* VIS3: the same as PT09 for tile 1. The RDP reads (s - sl) through the mask, a mirrored or wrapped read of a window to the right of the image start (walking dust frame i + 1) needs the sheet from its start. */
                    int per = 1 << tile_wrap[1][2];
                    int ldw = tile_siz[1] == 0 ? ld_w * 2 : ld_w;
                    if (per > w && per <= 256 && ldw > w) w = ldw < per ? ldw : per;
                }
                int stride = timg_stride > w ? timg_stride : w;
                int bpp = tile_siz[1] >= 3 ? 4 : tile_siz[1] == 2 ? 2 : tile_siz[1] == 0 ? 0 : 1;
                const u8* src = (const u8*)timg;
                int l1x = ld_x, l1y = ld_y, l1stride = timg_stride, l1v = ld_valid;
                for (unsigned k = 0; k < 4; k++) {
                    if (ldrec[k].img && ldrec[k].tmem == tile_tmem[1]) {
                        src = (const u8*)ldrec[k].img; l1x = ldrec[k].x; l1y = ldrec[k].y; l1stride = ldrec[k].stride; l1v = 1;
                        stride = l1stride > w ? l1stride : w;
                    }
                }
                int t1x = uls >> 2; /* texel column of the window origin; the V21 branch lowers it to the loaded origin */
                int boff = 0; /* V13: tile 1 inside an earlier block load: the image start moves by the TMEM difference */
                for (unsigned k = 0; k < 4; k++)
                    if (blk[k].img && blk[k].tmem < tile_tmem[1] && (tile_tmem[1] - blk[k].tmem) * 8u < blk[k].bytes && blk[k].tmem == tile_tmem[0] && src == (const u8*)blk[k].img)
                        boff = (int)((tile_tmem[1] - blk[k].tmem) * 8u);
                td[1].valid = 0;
                if (cur_tile == 1) cur_tile = -1; /* a new tile 1 image: bind it again */
                /* The image offset is the LoadTile origin. SetTileSize uls and ult only place
                 * it: gDPLoadMultiTile (draw_box corners) loads at 0,0 and moves tiles 1 to 3
                 * to the far side of the window by SetTileSize. */
                if (src && bpp && l1v) src += (size_t)(l1y * stride + l1x) * (size_t)bpp;
                else if (src && bpp) src += (size_t)boff; /* V13: a block load puts the image at TMEM start, SetTileSize only moves the coordinates */
                else if (src && tile_siz[1] == 0 && !l1v) src += (size_t)boff; /* PT82: the same for a 4 bit image (gDPScrollTextureBlockHalfHeight_4b: tile 1 is the lower half of one block load, mim_darkgreen2tif) */
                else if (src && l1v) {
                    /* V21: SetTileSize uls and ult are the window origin in the image (fire_breath scrolls tile 1 by imgFrame*32+32).
                     * This branch is 4 bit only (bpp 0). uls >> 2 counts texels, two per byte, so the byte offset is half of it.
                     * A byte pointer cannot start on an odd texel: the loaded origin is the even texel at or before the window
                     * origin. td[1].x0 uses the same origin, so the UV map and the loaded data agree. */
                    t1x = (uls >> 2) & ~1;
                    src += (size_t)(ult >> 2) * (size_t)l1stride + (size_t)(t1x >> 1);
                    stride = l1stride * 2;
                }
                if (src && w > 0 && h > 0) {
                    td[1].valid = 1;
                    td[1].src = src;
                    td[1].fmt = tile_fmt[1];
                    td[1].siz = tile_siz[1];
                    td[1].w = w;
                    td[1].h = h;
                    td[1].stride = stride;
                    td[1].x0 = t1x;
                    td[1].y0 = ult >> 2;
                    td[1].pal = tlut_pal[tile_pal[1] & 15] ? tlut_pal[tile_pal[1] & 15] : tlut;
                    td[1].pal_n = tlut_n;
                }
            }
            if (tile == 1) sync_combine(); /* S25: a plan that reads TEXEL1 needs tile 1 */
            if (tile != 0) break;
            td[0].valid = 0;
            cur_tile = -1; /* GP3: defer pm_tex_load to ensure_tile(0) at first draw */
            if (dual_on) { dual_on = 0; sync_combine(); } /* S25: the plan path is off while the shaded sprite is on */
            if (m1.valid && tile_fmt[0] == 2 && tile_siz[0] == 0 && tile_pal[0] != tile_pal[1] &&
                comb_a == G_CCMUX_TEXEL0 && comb_c == G_CCMUX_TEXEL1 &&
                m1.w > 0 && m1.h > 0) {
                /* Tile 0 starts 256 texels in (the vertices carry that) plus the light offset. */
                int dx4 = uls - (m1.x0 << 2);
                int dy4 = ult - (m1.y0 << 2);
                m1.valid = 0;
                pm_gpu_flush();
                tile_x0 = m1.x0;
                tile_y0 = m1.y0;
                tile_fx = tile_fy = 0.f;
                if (load_shaded_sprite(dx4, dy4)) {
                    dual_on = 1;
                    sync_combine();
                    break;
                }
            }
            m1.valid = 0;
            int x0 = uls >> 2;
            int y0 = ult >> 2;
            int w = ((lrs - uls) >> 2) + 1;
            int h = ((lrt - ult) >> 2) + 1;
            /* FX3f, PT37: a T wrap tile too (confetti: 16 rows of a 256 row sheet moved by ult). the RDP reads TMEM row (t - SetTileSize ult) wrapped by the T mask. A window shorter than the mask period (shape_spell:
             * 65 rows of a 256 row image, ult 256) reads rows outside the window, so the whole period must be bound. */
            if (tile_wrap[0][3] && !(tile_wrap[0][1] & 2) && ld_valid) {
                int per = 1 << tile_wrap[0][3];
                if (per > h && per <= 256 && ld_h >= per) h = per;
            }
            if (tile_wrap[0][2] && !(tile_wrap[0][0] & 2) && ld_valid) { /* PT09: a sprite sheet read through a window narrower than the mask period (emote icons): bind the loaded width up to the period */
                int per = 1 << tile_wrap[0][2];
                int ldw = tile_siz[0] == 0 ? ld_w * 2 : ld_w; /* VIS3: a 4 bit image loads through an 8 bit tile, ld_w counts bytes (walking dust: 88 for 176 texels) */
                if (per > w && per <= 256 && ldw > w) w = ldw < per ? ldw : per;
            }
            int stride = timg_stride > w ? timg_stride : w;
            int bpp = tile_siz[0] >= 3 ? 4 : tile_siz[0] == 2 ? 2 : tile_siz[0] == 0 ? 0 : 1;
            const u8* src = (const u8*)timg;
            int r_x = ld_x, r_y = ld_y, r_str = timg_stride, r_v = ld_valid;
            /* V12c: the render tile shows what was loaded at its own TMEM address. A later load of another image at another
             * address (bulb glow: the frame buffer window into the upper half of TMEM, then SetTileSize of tile 0 for the
             * next strip) must not replace the image of tile 0. Tile 1 does the same lookup below. */
            for (unsigned k = 0; k < 4; k++) {
                if (ldrec[k].img && ldrec[k].tmem == tile_tmem[0] && ldrec[k].img != timg) {
                    src = (const u8*)ldrec[k].img; r_x = ldrec[k].x; r_y = ldrec[k].y; r_str = ldrec[k].stride; r_v = 1;
                    stride = r_str > w ? r_str : w;
                }
            }
            /* V12c: a tile window wider than the loaded image only works through the tile mask (it repeats or mirrors the image).
             * The source rows are as long as the loaded image, not as long as the window (bulb glow: 32 texel image, 65 texel window). */
            if (tile_wrap[0][2] && (1u << tile_wrap[0][2]) < (unsigned)w && r_str > 0 && r_str < w) stride = r_str;
            /* V05: the source window starts at the LoadTile origin. SetTileSize only says which texel coordinate that is
             * (draw_box loads a strip at row idx * 6 and sets the tile origin to (160 - w / 2, 0)). Both are the same
             * for gDPLoadTextureTile, so this changes no other draw. */
            if (src && bpp && r_v) src += (size_t)(r_y * stride + r_x) * (size_t)bpp;
            /* V13: no source offset for a block load. SetTileSize only moves the coordinates (the pan of a lava model). */
            else if (src && tile_siz[0] == 0 && r_v) {
                /* 4-bit strips (imgfx meshes) load a window of an 8-bit-wide image. The
                 * tile size carries only the scroll, so the window comes from LoadTile. */
                src += (size_t)r_y * (size_t)r_str + (size_t)r_x;
                stride = r_str * 2;
            }
            suntile_t0(src, stride, w, h, bpp, uls, ult, lrs, lrt);
            tile_x0 = x0;
            tile_y0 = y0;
            tile_fx = (float)(uls & 3) * 0.25f; /* PT38 */
            tile_fy = (float)(ult & 3) * 0.25f;
            /* PG1: the same texture again (same source, format, size, palette, stride) and nothing touched the texture state since the last load (pm_tex_epoch):
             * the load would find the same cache slot. Skip the flush and the load, the batch in flight samples the same image. A frame buffer image is
             * excluded (its content changes under the same pointer, grab_cur). */
            {
                struct TszLast* lp = &tsz_last;
                const void* palc = tlut_pal[tile_pal[0] & 15] ? tlut_pal[tile_pal[0] & 15] : tlut;
                int ggx = 0, ggy = 0;
                if (src && w > 0 && h > 0 && tile_fmt[0] == 2 && tile_siz[0] == 0 &&
                    pm_tex_glyph_same(src, tile_fmt[0], tile_siz[0], w, h, palc, tlut_n, &ggx, &ggy)) {
                    tile_x0 = x0 - ggx; /* same sign as ensure_tile's new-bind path (x0 - gox) */
                    tile_y0 = y0 - ggy;
                    tile_fx = (float)(uls & 3) * 0.25f;
                    tile_fy = (float)(ult & 3) * 0.25f;
                    td[0].valid = 1; td[0].src = src; td[0].fmt = tile_fmt[0]; td[0].siz = tile_siz[0]; td[0].w = w; td[0].h = h; td[0].stride = stride;
                    td[0].x0 = x0; td[0].y0 = y0; td[0].pal = palc; td[0].pal_n = tlut_n;
                    gp3_t0_set(); GP_SET(7, 1);
                    break;
                }
                if (lp->ok && src == lp->src && tile_fmt[0] == lp->fmt && tile_siz[0] == lp->siz && w == lp->w && h == lp->h && stride == lp->stride &&
                    palc == lp->pal && tlut_n == lp->pal_n && pm_tex_epoch() == lp->ep && !pm_is_cfb(src) && src && w > 0 && h > 1) {
                    tile_x0 = x0 - lp->ox;
                    tile_y0 = y0 - lp->oy;
                    td[0].valid = 1; td[0].src = src; td[0].fmt = tile_fmt[0]; td[0].siz = tile_siz[0]; td[0].w = w; td[0].h = h; td[0].stride = stride;
                    td[0].x0 = x0; td[0].y0 = y0; td[0].pal = palc; td[0].pal_n = tlut_n;
                    gp3_t0_set(); GP_SET(7, 1);
                    break;
                }
                GP_SET(7, 0);
                lp->ok = 0;
                lp->src = src; lp->fmt = tile_fmt[0]; lp->siz = tile_siz[0]; lp->w = w; lp->h = h; lp->stride = stride; lp->pal = palc; lp->pal_n = tlut_n;
            }
            /* GP3: pm_tex_load runs in ensure_tile(0) at the first surviving draw. */
            /* F03: a 1 row tile of the N64 z-buffer is a depth query source (pm_depth_query), not a texture (the pause background loads tall z-buffer tiles, those are textures) */
            if (src && w > 0 && h > 0 && (h > 1 || (unsigned)timg - (unsigned)nuGfxZBuffer >= 320u * 240u * 2u)) {
                const void* pal0 = tlut_pal[tile_pal[0] & 15] ? tlut_pal[tile_pal[0] & 15] : tlut;
                td[0].valid = 1;
                td[0].src = src;
                td[0].fmt = tile_fmt[0];
                td[0].siz = tile_siz[0];
                td[0].w = w;
                td[0].h = h;
                td[0].stride = stride;
                td[0].x0 = x0;
                td[0].y0 = y0;
                td[0].pal = pal0;
                td[0].pal_n = tlut_n;
                gp3_t0_set();
            }
            break;
        }
        case G_TEXTURE:
            tex_on = (w0 & 0xff) != 0;
            tex_ss = w1 >> 16;
            tex_ts = w1 & 0xffffu;
            sync_combine();
            break;
        case G_SETOTHERMODE_H:
        case G_SETOTHERMODE_L: {
            unsigned len = (w0 & 0xff) + 1;
            unsigned field = (w0 >> 8) & 0xff;
            unsigned sft = 32 - field - len;
            unsigned mask = (len >= 32) ? 0xffffffffu : (((1u << len) - 1u) << sft);
            if (op == G_SETOTHERMODE_H) {
                unsigned old_tf = omode_h & (3u << 12);
                omode_h = (omode_h & ~mask) | (w1 & mask);
                if ((omode_h & (3u << 12)) != old_tf) { /* V15: G_MDSFT_TEXTFILT, POINT is 0, BILERP 2, AVERAGE 3 */
                    pm_gpu_flush();
                    pm_tex_set_filter((omode_h & (3u << 12)) != 0);
                }
            }
            else omode_l = (omode_l & ~mask) | (w1 & mask);
            /* S25: the cycle type decides 1 or 2 cycles of the combiner. */
            if (op == G_SETOTHERMODE_H && sft <= G_MDSFT_CYCLETYPE && sft + len > G_MDSFT_CYCLETYPE) sync_combine();
            break;
        }
        case G_SETPRIMCOLOR:
            GP_SET(1, prim == w1 && aud_prim_lod == (u8)(w0 & 0xff));
            aud_prim_lod = (u8)(w0 & 0xff);
            prim = w1;
            pm_gpu_set_prim(prim);
            sync_combine();
            break;
        case G_SETCOMBINE:
            GP_SET(0, comb_w0 == w0 && comb_w1 == w1 && comb_set);
            comb_w0 = w0;
            comb_w1 = w1;
            comb_a = (w0 >> 20) & 0xf;
            comb_c = (w0 >> 15) & 0x1f;
            comb_b = (w1 >> 28) & 0xf;
            comb_d = (w1 >> 15) & 7;
            comb_set = 1;
            comb_aa = (w0 >> 12) & 7;
            comb_ac = (w0 >> 9) & 7;
            comb_ab = (w1 >> 12) & 7;
            comb_ad = (w1 >> 9) & 7;
            comb2_a = (w0 >> 5) & 0xf;
            comb2_c = w0 & 0x1f;
            comb2_b = (w1 >> 24) & 0xf;
            comb2_d = (w1 >> 6) & 7;
            comb2_aa = (w1 >> 21) & 7;
            comb2_ac = (w1 >> 18) & 7;
            comb2_ab = (w1 >> 3) & 7;
            comb2_ad = w1 & 7;
            /* TEXEL1 with no TEXEL0 anywhere: the texture is tile 1. Rename it so one binding serves. */
            {
                int t0 = comb_a == 1 || comb_b == 1 || comb_d == 1 || comb_c == 1 || comb_c == 8 ||
                         comb_aa == 1 || comb_ab == 1 || comb_ac == 1 || comb_ad == 1;
                int t1 = comb_a == 2 || comb_b == 2 || comb_d == 2 || comb_c == 2 || comb_c == 9 ||
                         comb_aa == 2 || comb_ab == 2 || comb_ac == 2 || comb_ad == 2;
                int c2 = comb2_a == 1 || comb2_b == 1 || comb2_d == 1 || comb2_c == 1 || comb2_c == 8 ||
                         comb2_aa == 1 || comb2_ab == 1 || comb2_ac == 1 || comb2_ad == 1 ||
                         comb2_a == 2 || comb2_b == 2 || comb2_d == 2 || comb2_c == 2 || comb2_c == 9 ||
                         comb2_aa == 2 || comb2_ab == 2 || comb2_ac == 2 || comb2_ad == 2;
                use_t1 = 0;
                if (t1 && !t0 && !c2) {
                    use_t1 = 1;
                    if (comb_a == 2) comb_a = 1;
                    if (comb_b == 2) comb_b = 1;
                    if (comb_d == 2) comb_d = 1;
                    if (comb_c == 2) comb_c = 1;
                    if (comb_c == 9) comb_c = 8;
                    if (comb_aa == 2) comb_aa = 1;
                    if (comb_ab == 2) comb_ab = 1;
                    if (comb_ac == 2) comb_ac = 1;
                    if (comb_ad == 2) comb_ad = 1;
                }
            }
            box_folded = 0;
            box_prod = comb_a == 1 && comb_b >= 7 && comb_c == 2 && comb_d == 7 &&
                       comb_aa == 7 && comb_ab == 7 && comb_ac == 7 && comb_ad == 2 &&
                       comb2_a == 3 && comb2_b == 5 && comb2_c == 0 && comb2_d == 5 &&
                       comb2_aa == 7 && comb2_ab == 7 && comb2_ac == 7 && comb2_ad == 0;
            comb_alpha_shade = ((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 ||
                               ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4;
            cmb_gen_seen = comb_gen; /* R2B */
            sync_combine();
            break;
    }
}

static void gbi_run_body(void* list, unsigned nbytes);
/* W1/P1: the outer call sets the producer cursor state. Nested calls (dlc replay) run inside the outer op. */
void pm_gbi_run(void* list, unsigned nbytes) {
    if (!pm_ph_on) { gbi_run_body(list, nbytes); return; }
    if (os_nest == 0) { pmx_main = gDisplayContext && list == (void*)gDisplayContext->mainGfx; pmx_cur = pmx_main ? PK_PRE : PK_BG; pmx_next = 0; mdcm_next = 0; mdcm_cur = -1; }
    os_nest++;
    gbi_run_body(list, nbytes);
    if (os_open && os_nest == 1) os_close();
    os_nest--;
}

static __attribute__((noinline)) void gbi_texrect(u32 w0, u32 w1, u32 hs, u32 hd) {
    int xh = (int)((w0 >> 12) & 0xfff);
    int yh = (int)(w0 & 0xfff);
    int xl = (int)((w1 >> 12) & 0xfff);
    int yl = (int)(w1 & 0xfff);
    if (xh & 0x800) xh |= ~0xfff;
    if (yh & 0x800) yh |= ~0xfff;
    if (xl & 0x800) xl |= ~0xfff;
    if (yl & 0x800) yl |= ~0xfff;
    /* F03: the decomp copies one z-buffer pixel into DepthCopyBuffer[id] (scissor x0 = id). Read the PICA depth. */
    if (cimg == (void*)DepthCopyBuffer) { pm_depth_query(timg, (int)((s16)(hs >> 16) >> 5), sci_x0); return; }
    if (rect_to_image(xl, yl, xh, yh, (w1 >> 24) & 7, hs, hd)) return;
    /* B5: a texrect into the z-buffer image without G_ZS_PRIM copies colour (underwater strips of the frame): a slot keyed by the z-buffer address */
    zcol = cimg_siz == 2 && (cimg_zero || (nuGfxZBuffer && (const unsigned short*)cimg >= nuGfxZBuffer && (const unsigned short*)cimg < nuGfxZBuffer + 320 * 240)) && !(omode_l & (1u << G_MDSFT_ZSRCSEL));
    if (zcol) { zslot_on = 1; sync_render(); }
    else if (img_role(0) == ROLE_SLOT) sync_render(); /* FX2a: a texrect into an off-screen slot takes its own blender (flame passes: CLD_SURF2 is an alpha blend, OPA_SURF2 a write). The render state came from the last triangle before. */
    off_screen_check();
    box_fold_sync();
    sync_render(); /* V25: the rectangle carries the blender, depth and alpha test of its own mode */
    if (box_pattern_rect(xl, yl, xh, yh, hs, hd)) return;
    /* V23: the texrect names its tile. The pause map draws its path dots with tile 1 (the small marker). Tile 0 holds the big location circle. */
    int rtile = (use_t1 || (((w1 >> 24) & 7) == 1 && td[1].valid)) ? 1 : 0;
    if (!rtile) gp3_t0_surv();
    ensure_tile(rtile);
    dual_bind();
    int tw = 1, th = 1;
    pm_tex_size(&tw, &th);
    /* V12b: the tile shift scales a texrect coordinate too (bulb glow uses shift 1 1). It applies after the interpolation, so it scales the step as well. */
    float ms = shift_mul(tile_shift[rtile][0]);
    float mt = shift_mul(tile_shift[rtile][1]);
    float s0 = ((float)(s16)(hs >> 16) / 32.f * ms - (float)tile_x0 - tile_fx) / (float)tw;
    float t0 = ((float)(s16)(hs & 0xffff) / 32.f * mt - (float)tile_y0 - tile_fy) / (float)th;
    float dsdx = (float)(s16)(hd >> 16) / 1024.f * ms;
    float dtdy = (float)(s16)(hd & 0xffff) / 1024.f * mt;
    float width = (float)(xh - xl) / 4.f;
    float height = (float)(yh - yl) / 4.f;
    if (((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 2) {
        /* COPY mode: the RDP writes 4 pixels per cycle, so dsdx counts 4 texels per pixel (4.0 = 1:1).
         * The lower right corner is inclusive, one more pixel (as in libultraship). */
        dsdx /= 4.f;
        width += 1.f;
        height += 1.f; /* PT06: the lower edge is inclusive too. Without this a 6 row background strip showed 5 rows and every 6th screen row repeated the row above */
    }
    float s1 = s0 + dsdx * width / (float)tw;
    float t1 = t0 + dtdy * height / (float)th;
    t1_fold(s0 < s1 ? s0 : s1, s0 < s1 ? s1 : s0, t0 < t1 ? t0 : t1, t0 < t1 ? t1 : t0);
    {
        /* pm_gpu_tex_rect takes an inclusive lower right pixel. COPY mode rectangles are inclusive (width += 1
         * above). In 1-cycle and 2-cycle mode the lower right edge is exclusive: without this the quad was one
         * pixel too wide and stretched its texture. A previous-frame blur drawn every frame zoomed in. */
        int ex = xh / 4, ey = yh / 4;
        if (((omode_h >> G_MDSFT_CYCLETYPE) & 3) != 2) { ex -= 1; ey -= 1; }
        if (ex >= xl / 4 && ey >= yl / 4) {
            /* B3: the decal after a case 6 model (mac_01 crystal ball) is a G_ZS_PRIM texrect: it tests the primitive depth like the X01 fill */
            if ((omode_l & (1u << G_MDSFT_ZSRCSEL)) && (omode_l & 0x10u) && (geom & G_ZBUFFER)) {
                sync_render();
                pm_gpu_tex_rect_z(xl / 4, yl / 4, ex, ey, s0, t0, s1, t1, (float)prim_z / 32767.f);
            } else
            if (pm_tex_off_idx() >= 0 && (tile_wrap[rtile][0] & 3) == 3 && (tile_wrap[rtile][1] & 3) == 3 && tile_wrap[rtile][2] && tile_wrap[rtile][3] &&
                (1 << tile_wrap[rtile][2]) == tw && (1 << tile_wrap[rtile][3]) == th) {
                FoldSeg fs[8], ft[8];
                int ns = fold_axis(xl / 4, (float)(ex + 1 - xl / 4), s0 * tw, s1 * tw, (float)tw, 0, 1, fs);
                int nt = fold_axis(yl / 4, (float)(ey + 1 - yl / 4), t0 * th, t1 * th, (float)th, 0, 1, ft);
                if (ns && nt) {
                    for (int a = 0; a < ns; a++)
                        for (int c = 0; c < nt; c++)
                            pm_gpu_tex_rect(fs[a].p0, ft[c].p0, fs[a].p1 - 1, ft[c].p1 - 1, fs[a].u0, ft[c].u0, fs[a].u1, ft[c].u1);
                    goto rect_done;
                }
            }
            pm_gpu_tex_rect(xl / 4, yl / 4, ex, ey, s0, t0, s1, t1);
        }
    }
    rect_done:
    zcol = 0;
    return;
}

static __attribute__((noinline)) void gbi_fillrect(u32 w0, u32 w1) {
int lrx = (int)((w0 >> 14) & 0x3ff);
int lry = (int)((w0 >> 2) & 0x3ff);
int ulx = (int)((w1 >> 14) & 0x3ff);
int uly = (int)((w1 >> 2) & 0x3ff);
/* D03: the pause screen extracts per-pixel edge coverage of the scene into a frame buffer with
 * the VISCVG render mode (the RDP writes it as the pixel alpha bits). The PICA has no edge
 * coverage, so every pixel is a full pixel: write the full-coverage value (gray, alpha 1) to
 * that memory and draw nothing on the screen. The game filter then leaves each pixel as is. */
if (img_role(1) == ROLE_CPUFILL) {
    unsigned short* d = (unsigned short*)cimg;
    if (!pm_interp_repl) pm_cfb_capture_prev_cpu_for(cimg); /* PT78: the fill target decides the buffer. F06-5: key pass only. the filter reads the last frame from the other frame buffer */
    for (int y = uly; y <= lry && y < 240; y++)
        for (int x = ulx; x <= lrx && x < cimg_stride; x++) d[y * cimg_stride + x] = 0x8421;
    return;
}
if (img_role(1) == ROLE_DEPTH) { /* B3: case 5, the z-buffer clear is a depth write */
    pm_gpu_fill_depth(ulx, uly, lrx, lry, depth_of_word(fill));
    return;
}
/* FILL cycle uses the fill register. 1-cycle fills (screen fades) use primitive color. */
unsigned color = (((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 3)
    ? rgba_from_5551(fill & 0xffff) : prim;
off_screen_check();
if (((omode_h >> G_MDSFT_CYCLETYPE) & 3) != 3) sync_render(); /* PT71: a 1-cycle fill carries the blender of its mode (the choice box fill fades with its prim alpha), not the state of the last draw */
pm_gpu_set_replace(((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 3);
/* X01: a fill rect with depth compare and G_ZS_PRIM tests the primitive depth against the depth buffer */
if ((omode_l & (1u << G_MDSFT_ZSRCSEL)) && (omode_l & 0x10u) && (geom & G_ZBUFFER)) {
    sync_render();
    pm_gpu_fill_rect_z(ulx, uly, lrx, lry, color, (float)prim_z / 32767.f);
} else
pm_gpu_fill_rect(ulx, uly, lrx, lry, color);
pm_gpu_set_replace(0);
return;
}

static __attribute__((noinline)) void gbi_setcimg(u32 w0, u32 w1) {
void* nc = ptr_of(w1);
int ns = (int)((w0 >> 19) & 3);
if (nc != cimg || ns != cimg_siz) pm_gpu_off_end(); /* V05 */
cimg = nc;
cimg_zero = (w1 == 0); zcol = 0;
cimg_siz = ns;
cimg_stride = (int)(w0 & 0xfff) + 1;
imglog("setcimg", cimg, cimg_siz, cimg_stride, img_role(0));
return;
}

static __attribute__((noinline)) void gbi_viewport(u32 w1) {
    Vp* vp = (Vp*)ptr_of(w1);
    if (vp) {
        vp_sx = (float)vp->vp.vscale[0] / 4.f;
        vp_sy = (float)vp->vp.vscale[1] / 4.f;
        vp_sz = (float)vp->vp.vscale[2] / 4.f;
        vp_tx = (float)vp->vp.vtrans[0] / 4.f;
        vp_ty = (float)vp->vp.vtrans[1] / 4.f;
        vp_tz = (float)vp->vp.vtrans[2] / 4.f;
        upload_mvp();
    }
}

static __attribute__((noinline)) void gbi_vtx_cpu1(const Vtx* s, unsigned k) {
vert_gvf[k] = 3;
vert_st[k][0] = (float)s->v.tc[0] * (float)tex_ss / 65536.f; /* T02: the RSP scales S and T */
vert_st[k][1] = (float)s->v.tc[1] * (float)tex_ts / 65536.f;
vert_c[k] = vert_color(s, mv[mv_sp]);
if (tg_ok) { vert_st[k][0] = tg_s; vert_st[k][1] = tg_t; }
{   /* PO2: the same products as the old per corner code, once per vertex */
    const float(*m)[4] = combined;
    float x = (float)s->v.ob[0], y = (float)s->v.ob[1], z = (float)s->v.ob[2];
    vert_p[k][0] = m[0][0] * x + m[1][0] * y + m[2][0] * z + m[3][0];
    vert_p[k][1] = m[0][1] * x + m[1][1] * y + m[2][1] * z + m[3][1];
    vert_p[k][2] = m[0][2] * x + m[1][2] * y + m[2][2] * z + m[3][2];
    vert_p[k][3] = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];
    vert_cb[k] = (unsigned char)clip_bits4(vert_p[k]);
    st_vx++;
}
}

static void gbi_run_body(void* list, unsigned nbytes) {
    gv_epoch++; /* GV1b */
    tb_probe(RS_TBG, (const Gfx*)list, nbytes / 8u);
    dls_on = PMDL("dlsrc");
    dlc_on = dlc_enabled();
    if (dls_on && !dlc_depth) {
        if (!dls_rd_lo) dls_init_ranges();
        dls_main_lo = (u32)list; dls_main_hi = (u32)list + nbytes;
        dls_pc = dls_class((u32)list);
    }
    Gfx* stack[12];
    int sp = 0;
    Gfx* pc = (Gfx*)list;
    Gfx* end = (nbytes >= 8) ? pc + nbytes / 8 : NULL;
    unsigned guard = 0;
    u32 pc_lo = 0, pc_hi = 0;
    gp_prev_op = -1; /* PG4: the time between two lists is not an op cost */
    sc_ok = 0; sr_ok = 0; /* DIP2: a new display list starts with a full sync */
    if (!pc) { gv_streak_flush(); tex_streak_flush(); return; }
    dd_end();
    if (dd_on()) { dd_idx = 0; dd_dl0 = dd_dl1 = 0; }
    while (pc && guard++ < 200000) {
        if (end && sp == 0 && pc >= end) break;
        if ((u32)pc < pc_lo || (u32)pc >= pc_hi) { /* PG3 A15: the range of pc is looked up when pc leaves the last range (list entry, G_DL, return), not at every op */
            RS_ADD(RS_RANGE, 1);
            if (!host_range((u32)pc, &pc_lo, &pc_hi)) { pc_lo = pc_hi = 0;
            gbi_cold_bad_dl((u32)pc, (u32)list, nbytes, sp);
            gv_streak_flush();
            tex_streak_flush();
            return;
            }
        }
        u32 w0 = pc->words.w0;
        u32 w1 = pc->words.w1;
        unsigned op = w0 >> 24;
        if (g1_skip[op]) { pc++; RS_ADD(RS_SKN, 1); continue; }
        if (g2_same(op, w0, w1)) { pc++; RS_ADD(RS_SKS, 1); continue; }
        pm_gbi_cur_op = op;
        int fp = 0;
        if (pm_ph_on) { gv_streak_op(op); tex_streak_op(op); }
        pc++;
        if (!((u32)pc & 31u)) { __builtin_prefetch(pc + 4); __builtin_prefetch(pc + 8); }
        n_ops++;
        if (dls_on) dls_ops[dls_pc][dls_kind(op)]++;
        if (gp_on()) {
            unsigned t = pm_ticks();
            if (gp_prev_op >= 0) { gp_opt[gp_prev_op] += t - gp_opt_last; gp_opn[gp_prev_op]++; }
            gp_opt_last = t;
            gp_prev_op = (int)op;
        }
        if (pm_ph_fine) {
            /* only these three: the thousands of state ops stay in the gbi phase (a tick read costs 1.3 us in Azahar) */
            fp = op == G_VTX ? PH_GVTX : (op == G_TRI1 || op == G_TRI2) ? PH_GTRI : op == G_MTX ? PH_GMTX : 0;
            if (fp) pm_ph_push(fp);
        }
        if (op != G_VTX && op != G_TRI1 && op != G_TRI2 && op != G_MTX && op != G_POPMTX && op != G_DL && op != G_ENDDL && op != G_MODIFYVTX) {
            gv_epoch++; /* GV1b */
        }
        if (pm_ph_on) os_op_hook(pc - 1, sp, op); /* W1: after the prologue, so a sample is the handler only */
        if (pm_ph_on) mops_count(op, w0, w1); /* R0: counts only */
        if (pm_ph_on) mdc_op(pc - 1, sp, op, w0, w1); /* MDL1: model census, counts only */
        switch (op) {
        case G_ENDDL:
            if (sp == 0) {
                if (gp_on() && gp_prev_op >= 0) { gp_opt[gp_prev_op] += pm_ticks() - gp_opt_last; gp_opn[gp_prev_op]++; gp_prev_op = -1; }
                dd_end();
                if (pm_ph_on) mr0_walk_end(); /* MR0 */
                if (fp) pm_ph_pop();
                gv_streak_flush();
                tex_streak_flush();
                return;
            }
            RS_ADD(RS_RET, 1);
            pc = stack[--sp];
            if (dls_on) dls_pc = dls_class((u32)pc);
            break;
        case G_PM_TEXI: /* GLYPH: the texture-load words after this header run in tb_inline. */
            tb_inline(pc, w0 & 0xffffu);
            pc += w0 & 0xffffu;
            break;
        case G_PM_TEX: /* C1: a texture bind list, run by tb_run when its record matches. */
            if (w1 && tb_list[w0 & 0x7f] == (const Gfx*)w1) { RS_ADD(RS_TBN, 1); tb_run((const Gfx*)w1); break; }
            RS_ADD(RS_TBFB, 1);
            w0 = (u32)G_DL << 24; /* G_DL_PUSH */
            goto tex_as_dl;
        case G_PM_RM: /* C2: a render mode list, applied in place (rm_apply). */
            RS_ADD(RS_RMN, 1);
            if (rm_apply(w1)) break;
            RS_ADD(RS_RMSK, 1); /* fallback: the list has another op, run it as G_DL */
            w0 = (u32)G_DL << 24; /* G_DL_PUSH */
            /* fall through */
        tex_as_dl:
        case G_DL: {
            Gfx* next = (Gfx*)ptr_of(w1);
            if (next && !is_host((u32)next)) {
                gbi_cold_bad_next(w1, w0, (u32)list);
                break;
            }
            if (next) { __builtin_prefetch(next); __builtin_prefetch(next + 4); } /* H4: the G_DL target */
            if (dd_on()) { dd_dl1 = dd_dl0; dd_dl0 = (u32)next; if (sp == 0) dd_top = (u32)(pc - 1); }
            if (dls_on) dls_target(next);
            if (pm_ph_on) { unsigned dc = dls_class((u32)next); RS_ADD(RS_GDL, 1); if (dc == 1) RS_ADD(RS_GDL1, 1); else if (dc == 7) RS_ADD(RS_GDL7, 1); }
            __builtin_prefetch(next); __builtin_prefetch(next + 4);
            if (dlc_on && dlc_try(next)) { /* DLC1: a compiled map shape list. A G_DL_NOPUSH ends the list that holds it, as G_ENDDL. */
                if (pm_ph_on) { os_dlc_hit(); mr0_dlc_hit = 1; }
                if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) pc = (Gfx*)dlc_end;
                break;
            }
            if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) { RS_ADD(RS_NOPUSH, 1); pc = next; }
            else if (sp < 12) {
                RS_ADD(RS_PUSH, 1); RS_MAX(RS_SPMAX, sp + 1);
                stack[sp++] = pc;
                pc = next;
            }
            if (dls_on) dls_pc = dls_class((u32)pc);
            break;
        }
        case G_VTX: {
            unsigned gv_t0 = gp_on() ? pm_ticks() : 0;
            unsigned n = (w0 >> 12) & 0xff;
            unsigned v0n = (w0 >> 1) & 0x7f;
            unsigned v0 = (v0n >= n) ? v0n - n : 0;
            Vtx* src = (Vtx*)ptr_of(w1);
            if (src) __builtin_prefetch(src + n); /* H4: the next vertex block */
            if (src && !is_host((u32)src)) {
                gbi_cold_bad_vtx(w1, w0, (u32)list);
                src = NULL;
            }
            if (src && n <= 64) {
                for (unsigned pk = 0; pk < n * 16u; pk += 32u) __builtin_prefetch((const char*)src + pk);
                if (v0 >= 64) n = 0; /* S30: a vertex load that starts past the buffer (garbage list) copied 4 GB */
                else if (v0 + n > 64) n = 64 - v0;
                PH_ADD(C_VTXLOAD, n);
                if (dls_on) { unsigned dc = dls_class((u32)src); dls_vtx[dc] += n; memset(&dls_vcls[v0], (int)dc, n); }
                if (dd_on()) memcpy(&verts[v0], src, n * sizeof(Vtx)); /* PG4: verts is read only by the drawlog statistics */
                /* A load at slot 0 starts a new mesh. Later loads in the same
                 * list append above it and keep the earlier slots. */
                if (v0 == 0 && tri_since_vtx) memset(vert_live, 0, sizeof vert_live);
                tri_since_vtx = 0;
                if (v0 == 0 && dd_on()) gbi_cold_dd_load(src);
                unsigned po3a_mid = 0;
                if (po3a_on()) {
                    po3a_mid = gbi_cold_po3a_hash();
                    p_vload += n; p_vcmd++;
                }
                int gring0 = -1;
                unsigned gmi = GV_NONE;
                int gv = 0;
                const int lit_ld = (geom & G_LIGHTING) != 0;
                if (n) { /* GV1: a load goes to the ring as it is (GV5: also a lit load, the GPU lights it). A full ring takes the old code. */
                    if (lit_ld && !gv_lit_ok()) gv_fb[GVR_LITLOAD]++;
                    else if (!dd_on() && !xt_on() && !rmlog_on() && !po3a_on()) {
                        gmi = gv_matrix_id(lit_ld);
                        if (gmi != GV_NONE) gring0 = pm_gpu_gvtx_load(src, n);
                        if (gring0 >= 0) gv = 1; else { gmi = GV_NONE; gv_fb[GVR_RING]++; }
                    }
                }
                if (gv) { gv_load_n += n; PH_ADD(C_GVLOAD, n); }
                int gp1_cls = GP1_CROSS;
                unsigned char gp1_p = 0;
                s16 gp1_lo[3] = { 32767, 32767, 32767 }, gp1_hi[3] = { -32768, -32768, -32768 };
                if (gv) {
                    for (unsigned j = 0; j < n; j++)
                        for (int q = 0; q < 3; q++) {
                            s16 o = src[j].v.ob[q];
                            if (o < gp1_lo[q]) gp1_lo[q] = o;
                            if (o > gp1_hi[q]) gp1_hi[q] = o;
                        }
                    gp1_cls = gp1_box_class(gp1_lo, gp1_hi, combined, &gp1_p);
                    if (gp1_cls == GP1_IN) { gp1_ld_in++; gp1_vx_in += n; }
                    else if (gp1_cls == GP1_OUT) { gp1_ld_out++; gp1_vx_out += n; }
                    else { gp1_ld_cross++; gp1_vx_cross += n; }
                }
                for (unsigned i = 0; i < n; i++) {
                    vert_ring[v0 + i] = gring0 >= 0 ? (unsigned short)(gring0 + (int)i) : GV_NONE;
                    vert_gmi[v0 + i] = (unsigned short)gmi;
                    vert_tss[v0 + i] = ((unsigned)tex_ss << 16) | ((unsigned)tex_ts & 0xffffu);
                    vert_mid[v0 + i] = po3a_mid;
                    vert_live[v0 + i] = 1;
                    vert_scr[v0 + i] = 0;
                    vert_litf[v0 + i] = (u8)(gv && lit_ld ? 1 | ((geom & G_TEXTURE_GEN) ? 2 : 0) : 0);
                    if (gv && lit_ld) lit_pending = 1;
                    if (gv && gp1_cls == GP1_IN) {
                        vert_cb[v0 + i] = 0;
                        vert_gvf[v0 + i] = 0;
                        continue;
                    }
                    if (gv && gp1_cls == GP1_OUT) {
                        vert_cb[v0 + i] = gp1_p;
                        vert_gvf[v0 + i] = 0;
                        continue;
                    }
                    if (gv) { /* GP1 cross: per-vertex product and clip bits */
                        const float(*m)[4] = combined;
                        float x = (float)src[i].v.ob[0], y = (float)src[i].v.ob[1], z = (float)src[i].v.ob[2];
                        float* p = vert_p[v0 + i];
                        p[0] = m[0][0] * x + m[1][0] * y + m[2][0] * z + m[3][0];
                        p[1] = m[0][1] * x + m[1][1] * y + m[2][1] * z + m[3][1];
                        p[2] = m[0][2] * x + m[1][2] * y + m[2][2] * z + m[3][2];
                        p[3] = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];
                        vert_cb[v0 + i] = (unsigned char)clip_bits4(p);
                        vert_gvf[v0 + i] = 1;
                        st_vx++;
                        RS_ADD(RS_XDUP, 1);
                        continue;
                    }
                    gbi_vtx_cpu1(&src[i], v0 + i);
                }
            }
            if (gp_on()) gp_acc[7] += pm_ticks() - gv_t0;
            break;
        }
        case G_MODIFYVTX:
            gbi_cold_modifyvtx(w0, w1);
            break;
        case G_TRI1:
            tri_prep_ok = 0;
            draw_tri(w0);
            break;
        case G_TRI2:
            tri_prep_ok = 0;
            draw_tri(w0);
            draw_tri(w1);
            break;
        case G_MTX:
            if (!is_host((u32)(uintptr_t)ptr_of(w1))) gbi_cold_bad_mtx(w0, w1, (Gfx*)list, pc);
            load_mtx(w0, w1);
            break;
        case G_SETSCISSOR: {
            int ulx = (int)((w0 >> 12) & 0xfff);
            int uly = (int)(w0 & 0xfff);
            int lrx = (int)((w1 >> 12) & 0xfff);
            int lry = (int)(w1 & 0xfff);
            sci_x0 = ulx >> 2;
            sci_y0 = uly >> 2;
            sci_x1 = lrx >> 2;
            sci_y1 = lry >> 2;
            pm_gpu_set_scissor(ulx >> 2, uly >> 2, lrx >> 2, lry >> 2);
            break;
        }
        case G_MOVEMEM: {
            if ((w0 & 0xff) == G_MV_LIGHT) lit_freeze(); /* GV5 */
            if ((w0 & 0xff) == G_MV_LIGHT) { /* T01: look-at vectors */
                int ls = (int)(((w0 >> 8) & 0xff) * 8 / 24) - 2;
                const u8* l = (const u8*)ptr_of(w1);
                if (l && (ls == -2 || ls == -1)) { lk[ls + 2][0] = (s8)l[8]; lk[ls + 2][1] = (s8)l[9]; lk[ls + 2][2] = (s8)l[10]; }
            }
            if ((w0 & 0xff) == G_MV_LIGHT) { /* L01: offset 0 and 24 are the look-at vectors, light n is at n * 24 + 24 */
                int slot = (int)(((w0 >> 8) & 0xff) * 8 / 24) - 2;
                const u8* l = (const u8*)ptr_of(w1);
                if (l && slot >= 0 && slot < 9) { lt[slot].col[0] = l[0]; lt[slot].col[1] = l[1]; lt[slot].col[2] = l[2]; lt[slot].dir[0] = (s8)l[8]; lt[slot].dir[1] = (s8)l[9]; lt[slot].dir[2] = (s8)l[10]; }
            }
            if ((w0 & 0xff) == G_MV_VIEWPORT) gbi_viewport(w1);
            break;
        }
        case G_POPMTX: {
            unsigned count = w1 / 64;
            if (count == 0) count = 1;
            while (count-- && mv_sp > 0) mv_sp--;
            upload_mvp();
            break;
        }
        case G_GEOMETRYMODE:
            geom = (geom & (w0 & 0xffffffu)) | w1;
            pm_gpu_set_mode((geom & G_ZBUFFER) != 0, (geom & G_LIGHTING) != 0);
            break;
        case G_TEXTURE:
        case G_SETOTHERMODE_H:
        case G_SETOTHERMODE_L:
        case G_SETCOMBINE:
        case G_SETPRIMCOLOR: /* HT1: the prim body is in tex_op, so a G_PM_TEXI run can hold it */
            tex_op(w0, w1);
            break;
        case G_SETCONVERT:
            conv_k4 = (w1 >> 9) & 0xffu;
            pm_comb_set_k4(conv_k4);
            sync_combine();
            break;
        case G_SETKEYR:
            keyc = (keyc & 0x00ffffffu) | ((w1 >> 8 & 0xffu) << 24);
            keys = (keys & 0x00ffffffu) | ((w1 & 0xffu) << 24);
            sync_combine();
            break;
        case G_SETKEYGB:
            keyc = (keyc & 0xff000000u) | ((w1 >> 24 & 0xffu) << 16) | ((w1 >> 8 & 0xffu) << 8);
            keys = (keys & 0xff000000u) | ((w1 >> 16 & 0xffu) << 16) | ((w1 & 0xffu) << 8);
            sync_combine();
            break;
        case G_SETFOGCOLOR:
            fogc = w1;
            break;
        case G_SETBLENDCOLOR:
            blendc = w1;
            break;
        case G_SETENVCOLOR:
            GP_SET(2, envc == w1);
            envc = w1;
            pm_gpu_set_env(envc);
            sync_combine();
            break;
        case G_SETCIMG:
            gbi_setcimg(w0, w1);
            break;
        case G_SETPRIMDEPTH:
            prim_z = (w1 >> 16) & 0x7fffu;
            break;
        case G_SETFILLCOLOR:
            fill = w1;
            break;
        case G_FILLRECT: {
            gbi_fillrect(w0, w1);
            break;
        }
        case G_TEXRECT:
        case G_TEXRECTFLIP: {
            if (!pc || (end && sp == 0 && pc + 1 >= end)) break;
            u32 hs = pc->words.w1;
            pc++;
            u32 hd = pc->words.w1;
            pc++;
            gbi_texrect(w0, w1, hs, hd);
            break;
        }
        case G_SETTIMG:
        case G_SETTILE:
        case G_LOADBLOCK:
        case G_LOADTILE:
        case G_LOADTLUT:
        case G_SETTILESIZE:
            tex_op(w0, w1);
            break;
        case G_MOVEWORD: {
            unsigned index = (w0 >> 16) & 0xff;
            unsigned offset = w0 & 0xffff;
            if (index == G_MW_NUMLIGHT) { lit_freeze(); lt_n = w1 / 24; if (lt_n > 7) lt_n = 7; break; } /* L01 */
            if (index == G_MW_LIGHTCOL && offset / 24 < 9 && (offset % 24) == 0) { unsigned k = offset / 24; lt[k].col[0] = w1 >> 24; lt[k].col[1] = w1 >> 16; lt[k].col[2] = w1 >> 8; break; } /* L01 */
            if (index == G_MW_FOG) { fog_fm = (s16)(w1 >> 16); fog_fo = (s16)w1; break; } /* F08 */
            if (index != G_MW_SEGMENT) {
                unsigned n = ++mw_cnt[index];
            }
            if (index == G_MW_SEGMENT && (offset / 4) < 16) {
                unsigned seg = offset / 4;
                segbase[seg] = (u32)ptr_of(w1);
                if (!segbase[seg]) segbase[seg] = w1;
            }
            break;
        }
        case G_NOOP:
        case G_SPNOOP:
        case G_RDPFULLSYNC:
        case G_RDPTILESYNC:
        case G_RDPPIPESYNC:
        case G_RDPLOADSYNC:
        case G_SETZIMG:
        case G_RDPHALF_1:
        case G_RDPHALF_2:
            break;
        default:
            note(op);
            break;
        }
        if (fp) pm_ph_pop();
    }
    gv_streak_flush();
    tex_streak_flush();
}

static void mtx_elem(Mtx* dst, int r, int c, float v) {
    s32 fixed = (s32)(v * 65536.f);
    u32* m = (u32*)dst->m;
    int idx = r * 2 + (c >> 1);
    u32 sh = (c & 1) ? 0u : 16u;
    u32 mask = 0xffffu << sh;
    m[idx] = (m[idx] & ~mask) | ((((u32)(fixed >> 16)) & 0xffffu) << sh);
    m[idx + 8] = (m[idx + 8] & ~mask) | (((u32)fixed & 0xffffu) << sh);
}

void pm_gbi_fallback(void) {
    static int ready;
    static Gfx dl[8];
    static Vtx fv[3];
    static Mtx fm;
    if (pm_debug_has("nofallback")) return;
    if (!ready) {
        memset(&fm, 0, sizeof(fm));
        mtx_elem(&fm, 0, 0, 1.f / 160.f);
        mtx_elem(&fm, 1, 1, 1.f / 120.f);
        mtx_elem(&fm, 2, 2, 1.f);
        mtx_elem(&fm, 3, 3, 1.f);
        short xy[3][2] = { { -50, -40 }, { 50, -40 }, { 0, 50 } };
        u8 rgb[3][3] = { { 255, 80, 80 }, { 80, 255, 80 }, { 80, 80, 255 } };
        for (int i = 0; i < 3; i++) {
            fv[i].v.ob[0] = xy[i][0];
            fv[i].v.ob[1] = xy[i][1];
            fv[i].v.ob[2] = 0;
            fv[i].v.cn[0] = rgb[i][0];
            fv[i].v.cn[1] = rgb[i][1];
            fv[i].v.cn[2] = rgb[i][2];
            fv[i].v.cn[3] = 255;
        }
        Gfx* g = dl;
        gDPSetCycleType(g++, G_CYC_FILL);
        gDPSetFillColor(g++, 0x22292229);
        gDPFillRectangle(g++, 0, 0, 319, 239);
        gDPSetCycleType(g++, G_CYC_1CYCLE);
        gSPMatrix(g++, &fm, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPVertex(g++, fv, 3, 0);
        gSP1Triangle(g++, 0, 1, 2, 0);
        gSPEndDisplayList(g++);
        ready = 1;
    }
    pm_gbi_run(dl, 0);
}
