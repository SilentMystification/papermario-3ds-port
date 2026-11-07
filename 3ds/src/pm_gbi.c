#include <stdio.h>
int pm_gpu_spr_native(const uint32_t* rec); /* SPR4: native sprite draw at the final vertex word (nd_walk.c G_VTX arm, pm_gpu.c) */
int pm_gpu_spr_checking(void); /* SPR4: sprcheck switch on */
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "pm_gbi.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
/* O6: a switch word read once. Not for the words the options panel changes at run time (nofog, nolight). */
#define PMD(word) ({ static int v_ = -1; if (v_ < 0) v_ = pm_debug_has(word); v_; })
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__) /* PG0: the flush counter by cause (prof mode) */
#define GP_SET(k, same) do { if (gp_on()) gp_note(k, (same)); } while (0)

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
unsigned ph_seq;
int ph_pick(void) { return (++ph_seq & 7u) == 0; }
static int gv_run, gv_streak;
unsigned gv_streak_t0;
void gv_streak_flush(void) {
    gv_run = 0;
    if (!gv_streak) return;
    pm_gbi_gv_ticks += (pm_ticks() - gv_streak_t0) * 8u;
    gv_streak = 0;
}
void gv_streak_op(unsigned op) {
    if (op == G_TRI1 || op == G_TRI2) {
        if (pm_ph_on && !gv_run) { gv_run = 1; if (ph_pick()) { gv_streak_t0 = pm_ticks(); gv_streak = 1; } }
    } else
        gv_streak_flush();
}
int tex_run, tex_streak;
unsigned tex_streak_t0;
void tex_streak_flush(void) {
    tex_run = 0;
    if (!tex_streak) return;
    pm_gbi_tex_ticks += (pm_ticks() - tex_streak_t0) * 8u;
    tex_streak = 0;
}
void tex_streak_op(unsigned op) {
    if (op == G_SETTIMG || op == G_SETTILE || op == G_SETTILESIZE || op == G_LOADTILE || op == G_LOADBLOCK || op == G_LOADTLUT || op == G_TEXTURE ) {
        if (pm_ph_on && !tex_run) { tex_run = 1; if (ph_pick()) { tex_streak_t0 = pm_ticks(); tex_streak = 1; } }
    } else
        tex_streak_flush();
}

extern int pm_tex_tagi; /* STALE2 texsrc: bind path tag */
extern u16 DepthCopyBuffer[16];
extern unsigned short* nuGfxZBuffer;
void pm_depth_query(const void* timg, int x, int id); /* F03, pm_depth.c */

extern unsigned int __ctru_heap;
extern unsigned int __ctru_heap_size;
extern unsigned int __ctru_linear_heap;
extern unsigned int __ctru_linear_heap_size;

void pm_tex_set_filter(int linear);
int rm_frame;

/* SUNFIX debug switch suntile=<first frame>:<count>: tile 1 state at each dual_bind, the vertex S T of each triangle, and the sun
 * overlay spawn and draws (pm_sun_spawn, pm_sun_draw). At most 4000 lines per run. */
int suntile_on(void) {
    static int a = -2, b;
    if (a == -2) {
        char v[24];
        a = -1;
        if (pm_debug_value("suntile", v, sizeof v) && sscanf(v, "%d:%d", &a, &b) == 2) b += a;
        else a = -1;
    }
    return a >= 0 && (int)pm_frame_count() >= a && (int)pm_frame_count() < b;
}
int suntile_lines;

unsigned conv_k4; /* G_SETCONVERT K4 (8 bit part), the combiner input K4 */
unsigned keyc, keys; /* B6: G_SETKEYR/G_SETKEYGB chroma key CENTER, 0xRRGGBB00. Only the combiner input CENTER reads it. */
/* TBREP: tex_gen counts the writes of texture memory and texture state (the bind skip of tb_run is gone, S3F-4; the counter stays). */
unsigned tex_gen;
/* TST1: the tile, combine and texture-image state in one block, 32-byte aligned, in .data.pmhot. Types and meanings are unchanged. */
int ld_valid, ld_x, ld_y, ld_h, ld_w; /* last LoadTile window, in 8-bit texels of the source image */
/* V09: the source of each LoadTile by TMEM word. A tile 1 window that is set again later (chapter_change strips) still reads
 * the image that was loaded to its TMEM address, not the image of the load that came last. */
unsigned ldrec_n;
unsigned rec_n; /* M4-2: records run in the current opsplit window */
/* V13: a block load keeps its image and TMEM range. A half-height aux tile (tile 1 at TMEM + half) reads the second half. */
/* RDP state the shaded-sprite path needs: color image, TLUT per palette slot, per-tile TMEM
 * address and palette, tile sizes, scissor, env color and the second combiner cycle. */
extern int pm_is_cfb(const void* p);
extern void pm_cfb_capture_prev(void);
extern void pm_cfb_capture_prev_cpu_for(const void* fill);
extern void pm_cfb_capture_prev_cpu(void);
extern void pm_zbuf_need(void);
extern unsigned pm_frame_count(void);
int cimg_stride = 320;
extern int pm_fb_native(const void* p);
/* The sprite raster captured when tile 1 got its size. A shaded sprite sets two tiles on the
 * same 4-bit image (tile 0 reads the shading palette, tile 1 the sprite palette). */
M1Rec m1;
int dual_on;
/* RDP shift field: 1..10 divide by 2^n, 11..15 multiply by 2^(16-n). */
float shift_mul(unsigned n) { return nd_shift_mul(n); } /* S3N-1: nd_model.c */
int comb_mode5; /* (PRIM - ENV) * TEXEL + ENV */
/* draw_box (H04): cycle 1 = TEXEL0 * TEXEL1 (tile 0 window background, tile 1 corner image),
 * alpha = TEXEL1 alpha, cycle 2 = (PRIM - ENV) * COMBINED + ENV. box_prod marks that form.
 * box_folded is 1 while it is drawn as (PRIM - ENV) * TEXEL1 + ENV, which is exact when tile 0
 * is all white (the flat background of the window styles, `bg_flat`). */
int box_prod; PM_HOTD int box_folded;
/* R2B: comb_gen counts the writes of the combine state that the SETCOMBINE handler does not make (box_fold_sync toggles);
 * cmb_gen_seen is comb_gen at the last SETCOMBINE. */
unsigned comb_gen, cmb_gen_seen;
int box_pat; /* D03: (PRIM - ENV) * TEXEL0 + ENV with alpha from TEXEL1 only: window with a tile 0 pattern */
unsigned box_save[4];
float dual_gain[3];
/* A display list may only draw vertices its own G_VTX loaded. Slots left by
 * the previous model otherwise connect the two meshes into long triangles. */
u8 vert_live[64];
/* FX3f: a triangle was issued since the last G_VTX. A load at slot 0 only starts a new mesh then. aura loads slots 2,6 and then
 * 0,4 and draws 4,5,6,7, so the slot 0 load must keep the slots loaded just before it. */
int tri_since_vtx;
/* B5: G_MODIFYVTX G_MWO_POINT_XYSCREEN sets the screen position of a loaded vertex (underwater warp). x, y in 10.2 fixed point. */
u8 vert_scr[64];
s16 vert_sx[64], vert_sy[64];
/* The RSP transforms a vertex when it is loaded. A later matrix does not move it. */
unsigned char vert_cb[64]; /* DIP1: clip bits of vert_p, made once at G_VTX */
float vert_p[64][4]; /* PO2: clip space x y z w of a vertex, transformed once when it is loaded (G_VTX) */
/* PO3b (switch gpuvtx, default off): plain triangles are drawn from the raw Vtx ring by the GPU transform. The CPU path keeps its checks (reject, cull, near clip) from the
 * per vertex transform of PO2, so the decisions are the same. Per slot: ring index, matrix table index, texture scale (s and t) at the load. */
/* GV1 (switch gv1, default off, nogv1 turns it off): the full GPU geometry path. G_VTX of an unlit load copies the 16 byte Vtx to the ring and makes only the reject bits.
 * A triangle of ring vertices goes to the GPU as three indices. The GPU transforms, clips and culls. The CPU path stays for the rare cases and reads the ring copy
 * (gv_xf makes vert_p, vert_st and vert_c on demand). It replaces gpuvtx (PO3b) when both are on. */
u8 vert_litf[64]; /* GV5: bit 0 = loaded lit, bit 1 = loaded lit with texgen, bit 2 = the lights changed since (the colour was made then): the CPU path only */
u8 vert_gvf[64]; /* GV1: bit 0 = vert_p valid, bit 1 = vert_st and vert_c valid. 3 for every vertex the old code made. */
unsigned seam_obj, seam_clip; /* SEAM: CPU path triangles by position route, `seamstat` logs them */
unsigned gv_fb[GVR_N], gv_load_n, gv_tri_n, gv_rej_n, gv_xf_n;
unsigned gp1_ld_in, gp1_ld_out, gp1_ld_cross, gp1_vx_in, gp1_vx_out, gp1_vx_cross, gp1_subset;
/* GP3 gate: tile 0 G_SETTILESIZE with td[0] set vs no surviving tri or tile-0 texrect before the next one. */
unsigned gp3_t0_ld, gp3_t0_dead, gp3_t0_pending;
void gp3_t0_set(void) {
    if (gp3_t0_pending) gp3_t0_dead++;
    gp3_t0_ld++;
    gp3_t0_pending = 1;
}
void gp3_t0_surv(void) { gp3_t0_pending = 0; }
unsigned short vert_ring[64], vert_gmi[64];
unsigned vert_tss[64];
unsigned st_gpu_tri; /* triangles drawn by the raw path */
unsigned st_vx, st_cor; /* PO2: vertices transformed at G_VTX, triangle corners read (the old code transformed one matrix product per corner) */
u8 seen[256];
/* i = every corner in front of the eye, drawn where it projected.
 * c = a corner behind the eye was cut. d = nothing left in front.
 * x is the first vertex's N64 ndc * 100. */
int n_cmd, n_in, n_cut, n_drop, n_out, n_keep, sample_x, have_sample;
char stats_line[96];
int leg_mode;               /* S25: the combine mode sync_combine() last gave pm_gpu */
int comb_set;
/* 1 when the alpha combiner reads shade alpha. DECAL and MODULATE modes do not, and imgfx
 * meshes leave cn[3] at 0, so passing it through made them vanish. */
int comb_alpha_shade = 1;
extern char __end__[]; /* linker: end of .bss */
int is_host(u32 p) {
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

void* ptr_of(u32 w) {
    if (!w) return NULL;
    if (is_host(w)) return (void*)w;
    unsigned seg = (w >> 24) & 0xff;
    if (seg < 16 && nd_ws.segbase[seg]) return (void*)(nd_ws.segbase[seg] + (w & 0x00ffffffu));
    /* SW1: an N64 K0 or K1 address (OS_PHYSICAL_TO_K0 of a static buffer: the camera projection matrix of the credits parade) names host memory with the top bits set */
    if ((w & 0xc0000000u) == 0x80000000u && is_host(w & 0x1fffffffu)) return (void*)(w & 0x1fffffffu);
    return (void*)w;
}
void* pm_gbi_ptr_of(u32 w) { return ptr_of(w); } /* MDL-1: the walker's segment resolver, for the R13 texture block (nd_model.c nd_r13_desc) */





void set_leg(int m) { leg_mode = m; }
int plan_active; /* S25: the general combiner (mode 8) draws, not the legacy mode */
PmPlan cur_plan;
void sync_combine(void);
/* S3N-3d: set by nd_comb_decide. The combine work then makes no pm_gpu call, no stats and no log. */
int comb_dry;
int sl_const, sl_env; /* OPT-SC: the gpu calls of the last sync_legacy, kept in the memo */
unsigned sl_cc;
void sync_legacy(void) {
    unsigned cc;
    sl_const = sl_env = 0;
    if (const_rgb_tex_alpha(&cc)) {
        if (!comb_dry) pm_gpu_set_const(cc);
        sl_const = 1; sl_cc = cc;
        set_leg(4);
        return;
    }
    /* Decal modes put TEXEL0 in the addend, which is mux d, not a/b/c.
     * Missing it draws the sprite quad's vertex colors: a grey gradient. */
    /* H06b1: with G_TEXTURE off the RSP sends a triangle with no S and T, the RDP reads texel (0, 0) of tile 0 (draw_tri sets that), so TEXEL0 is read whatever the flag says. */
    int wants_tex = (nd_ws.tst1.comb_a == G_CCMUX_TEXEL0 || nd_ws.tst1.comb_b == G_CCMUX_TEXEL0 ||
        nd_ws.tst1.comb_c == G_CCMUX_TEXEL0 || nd_ws.tst1.comb_d == G_CCMUX_TEXEL0 || nd_ws.tst1.comb_c == G_CCMUX_TEXEL0_ALPHA);
    int wants_prim = nd_ws.tst1.comb_a == G_CCMUX_PRIMITIVE || nd_ws.tst1.comb_b == G_CCMUX_PRIMITIVE || nd_ws.tst1.comb_c == G_CCMUX_PRIMITIVE;
    int mode = 0;
    /* Mode 3 = texture colour, but alpha only from the vertex: the alpha combiner does not read TEXEL0. */
    int alpha_tex = nd_ws.tst1.comb_aa == 1 || nd_ws.tst1.comb_ab == 1 || nd_ws.tst1.comb_ac == 1 || nd_ws.tst1.comb_ad == 1 ||
                    nd_ws.tst1.comb_aa == 2 || nd_ws.tst1.comb_ab == 2 || nd_ws.tst1.comb_ac == 2 || nd_ws.tst1.comb_ad == 2;
    comb_mode5 = 0;
    box_pat = box_prod && !box_folded; /* V10: the 2-cycle window with a pattern in tile 0 (not white): product on the CPU */
    if (wants_tex && nd_ws.tst1.comb_a == G_CCMUX_PRIMITIVE && nd_ws.tst1.comb_b == G_CCMUX_ENVIRONMENT && nd_ws.tst1.comb_c == G_CCMUX_TEXEL0 &&
        nd_ws.tst1.comb_d == G_CCMUX_ENVIRONMENT && alpha_tex && nd_ws.tst1.comb2_aa != 0) { /* FX2f: a cycle 2 alpha that reads COMBINED (huff_puff_breath, times PRIM) is no window pass */
        comb_mode5 = 1;
        if (!comb_dry) pm_gpu_set_env(nd_ws.envc);
        sl_env = 1;
        /* D03: draw_box windows with the bg_tile pattern (tile 0, I4 0/F checker) take their alpha from
         * TEXEL1 (the corner image). Tile 0 alpha is the checker itself, so it must not become the alpha:
         * mode 7 draws the body opaque (the 8x8 rounded corner shape is lost). */
        {
            int a_t0 = nd_ws.tst1.comb_aa == 1 || nd_ws.tst1.comb_ab == 1 || nd_ws.tst1.comb_ac == 1 || nd_ws.tst1.comb_ad == 1;
            box_pat = !a_t0;
            set_leg(a_t0 ? 5 : 7);
        }
        return;
    }
    /* D03: (TEXEL0 - PRIM) * PRIM_ALPHA + PRIM, the pause screen background fade. */
    /* Texture rectangles read the texture whatever gSPTexture says, so tex_on is not tested. */
    if (nd_ws.tst1.comb_a == G_CCMUX_PRIMITIVE && nd_ws.tst1.comb_b == G_CCMUX_TEXEL0 && nd_ws.tst1.comb_c == G_CCMUX_PRIMITIVE_ALPHA &&
        nd_ws.tst1.comb_d == G_CCMUX_TEXEL0) {
        set_leg(6);
        return;
    }
    if (wants_tex) mode = alpha_tex ? 2 : 3;
    else if (wants_prim && !comb_set) mode = 1;
    else if (wants_prim && nd_ws.tst1.comb_a != G_CCMUX_SHADE) mode = 1;
    set_leg(mode);
}

/* sync_combine() is below, after the audit code it shares. */




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

/* L01: RSP light table. G_MW_NUMLIGHT sets the count, G_MOVEMEM index G_MV_LIGHT fills slots (slot 0 is light 1, slot nlights is the ambient light). */
NdLight lt[9];
unsigned lt_n;
void nd_light_count(u32 w1) { lit_freeze(); lt_n = w1 / 24; if (lt_n > 7) lt_n = 7; } /* ST-1: G_MW_NUMLIGHT; group M owns lt_n */
s8 lk[2][3]; /* T01: look-at X and Y vectors (G_MOVEMEM slots -2 and -1) */
float vert_st[64][2]; /* S and T in 10.5 units after the RSP scale or texgen, set when the vertex is loaded */
/* SUN3 debug (suntile switch): the tile 0 source of a draw, its state, and the first 16 bytes of rows 0 to 11 of the source. */
__attribute__((cold, noinline)) void suntile_t0(const u8* src, int stride, int w, int h, int bpp, int uls, int ult, int lrs, int lrt) {
    if (!suntile_on() || suntile_lines >= 400) return;
    suntile_lines++;
    pm_log("suntile t0 f%u src %p str %d w %d h %d bpp %d fmt %u siz %u tmem %u win %d %d %d %d",
           pm_frame_count(), (const void*)src, stride, w, h, bpp, nd_ws.tst1.tile_fmt[0], nd_ws.tst1.tile_siz[0], nd_ws.tst1.tile_tmem[0], uls, ult, lrs, lrt);
    pm_log("suntile t0 timg %p tstr %d ld %d %d w %d h %d v %d", nd_ws.timg, nd_ws.tst1.timg_stride, ld_x, ld_y, ld_w, ld_h, ld_valid);
    if (!src) return;
    for (int r = 0; r < 12; r++) {
        unsigned q[4];
        if (suntile_lines >= 400) break;
        suntile_lines++;
        memcpy(q, src + (size_t)r * (size_t)stride * (bpp > 0 ? (size_t)bpp : 1u), 16); /* stride counts texels */
        pm_log("suntile t0 row %d %08x %08x %08x %08x", r, q[0], q[1], q[2], q[3]);
    }
}
__attribute__((cold, noinline)) void suntile_tri(const char* path, const int* idx) {
    if (!suntile_on() || suntile_lines >= 400) return;
    suntile_lines++;
    pm_log("suntile tri %s f%u verts %d %d %d s %.3f %.3f %.3f t %.3f %.3f %.3f tss %08x",
           path, pm_frame_count(), idx[0], idx[1], idx[2],
           (double)vert_st[idx[0]][0], (double)vert_st[idx[1]][0], (double)vert_st[idx[2]][0],
           (double)vert_st[idx[0]][1], (double)vert_st[idx[1]][1], (double)vert_st[idx[2]][1], vert_tss[idx[0]]);
    if (path[0] == 'd') /* SUN3: the tile 1 state the draw uses */
        pm_log("suntile t1 draw fmt %u siz %u tmem %u sz %d %d %d %d td1 v %d src %p w %d h %d str %d x0 %d y0 %d tile %u",
               nd_ws.tst1.tile_fmt[1], nd_ws.tst1.tile_siz[1], nd_ws.tst1.tile_tmem[1], nd_ws.tst1.tile_sz[1][0], nd_ws.tst1.tile_sz[1][1], nd_ws.tst1.tile_sz[1][2], nd_ws.tst1.tile_sz[1][3],
               nd_ws.td[1].valid, (const void*)nd_ws.td[1].src, nd_ws.td[1].w, nd_ws.td[1].h, nd_ws.td[1].stride, nd_ws.td[1].x0, nd_ws.td[1].y0, nd_ws.cur_tile);
}
int tg_ok;
float tg_s, tg_t; /* T01: texgen result of the last vert_color call */
unsigned vert_c[64]; /* lit or plain vertex colour, set when the vertex is loaded, as the RSP does */

/* L01: RSP lighting. The normal goes through the model view matrix, then colour = ambient + sum(light colour * max(0, normal . direction)). Alpha stays the vertex alpha.
 * Debug switch nolight keeps the old fixed light for an A/B. */
int pm_nolight = -1; /* F11-10: -1 = read the switch at first use, the Options row sets 0 or 1 */
unsigned lit_color(const Vtx* v, const float m[4][4], int texgen) {
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
            tg_s = (dx + 1.f) / 4.f * (float)nd_ws.tex_ss;
            tg_t = (dy + 1.f) / 4.f * (float)nd_ws.tex_ts;
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
    if (nd_ws.geom & G_LIGHTING) return lit_color(v, m, (nd_ws.geom & G_TEXTURE_GEN) != 0);
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
    unsigned st = nd_ws.use_t1 ? 1 : 0;
    float ms = shift_mul(nd_ws.tst1.tile_shift[st][0]);
    float mt = shift_mul(nd_ws.tst1.tile_shift[st][1]);
    *u = (tc[0] / 32.f * ms - (float)nd_ws.tile_x0 - nd_ws.tile_fx) / (float)tw;
    *vcoord = (tc[1] / 32.f * mt - (float)nd_ws.tile_y0 - nd_ws.tile_fy) / (float)th;
}


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

const float ident_m[4][4] = {
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
int fold_axis(int plo, float width, float a0, float a1, float mw, int flip, int negclamp, FoldSeg* out) {
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

/* ---- S25: combiner audit. Every distinct (combiner words, cycle type, render mode, texture format)
 * is checked once against the combine mode pm_gpu got. The check is numeric: the RDP equation
 * (A-B)*C+D per cycle, on random inputs, against what that mode draws. A difference is logged once
 * ("comb new") and then with a draw count at 1, 10, 100, ... draws ("comb count"). COMBINED is 0 in
 * cycle 1. LOD fraction, key and convert read 0. NOISE reads 0.5 (PT07). */
typedef struct { float t0[4], t1[4], sh[4], pr[4], en[4], ke[4], ks[4]; float k4; } CcIn;





/* What the legacy combine mode draws. */
void cc_legacy(int mode, const CcIn* in, int t1src, float out[4]) {
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

const char* const cc_nm_a[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "1", "NOISE" };
const char* const cc_nm_b[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "KEYC", "K4" };
const char* const cc_nm_c[16] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "KEYS", "CMBA", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "LODF", "PLODF", "K5" };
const char* const cc_nm_d[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "1", "0" };
const char* const cc_nm_aa[8] = { "CMBA", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "1", "0" };
const char* const cc_nm_ac[8] = { "LODF", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "PLODF", "0" };



unsigned st_lit, st_fogg, st_fogb, st_blc, st_tgen, st_tri; /* S25: draws that use lights, fog, blend colour, texgen */
unsigned mm_cnt[256];
unsigned lcg_s = 12345u;
float lcg_f(void) { lcg_s = lcg_s * 1664525u + 1013904223u; return (float)((lcg_s >> 8) & 0xffff) / 65535.f; }

/* Decision per combiner: the legacy combine mode, or the general plan (mode 8). Cached by the combiner words and
 * the flags the legacy mode depends on. The plan is chosen when the legacy mode is off by more than 0.012
 * (of 1.0, max over colour and alpha, 24 random inputs) and the plan is closer. */
typedef struct { unsigned key; int use, why; float el, ep; PmPlan plan; } Dec;
unsigned plan_draws, plan_fails;


int ls_use = -9, ls_why, ls_cyc, ls_dual, ls_frame;
unsigned sc_sig; PM_HOTD int sc_ok; /* DIP2: the inputs of the last sync_combine and whether its result is still what pm_gpu holds */
/* OPT-SC: the result of sync_combine_work by the PG0 signature of its inputs (the same hash as the PG0 count in sync_combine_work). A hit repeats the outputs and the calls into pm_gpu: no decision lookup, no
 * constant colour test, no plan colours. 16 entries, the oldest is replaced. */
typedef struct { unsigned sg; int ok, leg, mode5, pat, use, why, cyc, dual, plan_on, cc_on, env_on; unsigned cc; PmPlan plan; } ScMemo;
/* S3N-3d: the combiner fields of tst1 that the combine work reads (nd_comb_decide inputs). */
unsigned* const tst_f[16] = { &nd_ws.tst1.comb_a, &nd_ws.tst1.comb_b, &nd_ws.tst1.comb_c, &nd_ws.tst1.comb_d, &nd_ws.tst1.comb_aa, &nd_ws.tst1.comb_ab, &nd_ws.tst1.comb_ac, &nd_ws.tst1.comb_ad,
    &nd_ws.tst1.comb2_a, &nd_ws.tst1.comb2_b, &nd_ws.tst1.comb2_c, &nd_ws.tst1.comb2_d, &nd_ws.tst1.comb2_aa, &nd_ws.tst1.comb2_ab, &nd_ws.tst1.comb2_ac, &nd_ws.tst1.comb2_ad };
/* S3N-3d: debug switch combchk. Each combine work of sync_combine runs nd_comb_decide on its inputs and compares the outputs with the walker state. */
int combchk_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("combchk"); return v; }
void pm_log_prof_line(const char* fmt, ...); /* pm_log.c: the one line of prof=4 mode */
unsigned cc_runs, cc_diff; static int cc_last_b = -1; /* the 300 frame bucket of the last log line */
__attribute__((cold, noinline)) void combchk_run(const NdCombIn* in, int worked) { /* worked: the work ran (sl_* are set); a memo hit replays the outputs only */
    NdCombOut o;
    nd_comb_decide(in, &o);
    cc_runs++;
    int bad = o.leg != leg_mode || o.mode5 != comb_mode5 || o.pat != box_pat || o.use != ls_use || o.why != ls_why || o.cyc != ls_cyc ||
              o.dual != ls_dual || o.plan_on != plan_active || (worked && (o.cc_on != sl_const || o.env_on != sl_env || (sl_const && o.cc != sl_cc))) ||
              (plan_active && memcmp(&o.plan, &cur_plan, sizeof cur_plan) != 0);
    if (bad) cc_diff++;
    unsigned f = pm_frame_count(), b = f / 300u;
    if ((int)b != cc_last_b) { cc_last_b = (int)b; pm_log_prof_line("combchk f%u runs %u diff %u", f, cc_runs, cc_diff); }
}

/* S3N-3d: the walker's sticky state at a draw (ndmcheck compare only: the native draw takes these as inputs): the TEXTURE scale word, the geometry mode and the combine words. */
/* MDL-1b: the tile-1 part of the walker's sync for a record, after rec_aux: dual_bind when the plan reads tile 1. 1 when tile 1 ended bound. */
int pm_gbi_bind_tile1(void) { /* MDL-1b v2: the walker's pre-draw tile step (gv_epoch_enter); rec_aux already ran the record's combine and tile words */
    if (!plan_active || !cur_plan.uses_tex1) return 0;
    ensure_tile(nd_ws.use_t1 ? 1 : 0);
    dual_bind();
    return nd_ws.td[1].valid && pm_tex_aux_off_idx() < 0;
}
void pm_gpu_set_r13_t1(int bound);
int pm_gbi_bind_tile1(void);
/* S3N-6: the walker matrix of the draws (the box test of the static ranges reads it) and the walker state of the gv params, the tile, vp and plan
 * state, pre-apply values for the native model cache key (pm_gpu.c pm_gpu_model_key). Returns the count written (at most max). */
const float (*pm_gbi_combined_ptr(void))[4] { return combined; }


/* UI1 debug: the PM_CC_07 combiner (flat PRIM colour times TEXEL0 alpha, the drop shadow). Logs the mode the
 * port chose and the PRIM alpha, so the shadow alpha is known per draw. Remove after UI1. */
void cc07_dbg(void) {
    static int cc07_n;
    if (comb_dry) return;
    if (cc07_n >= 40) return;
    if (!(nd_ws.tst1.comb_a == 0 && nd_ws.tst1.comb_b == 0 && nd_ws.tst1.comb_c == 0 && nd_ws.tst1.comb_d == G_CCMUX_PRIMITIVE &&
          nd_ws.tst1.comb_aa == G_ACMUX_TEXEL0 && nd_ws.tst1.comb_ab == 0 && nd_ws.tst1.comb_ac == G_ACMUX_PRIMITIVE && nd_ws.tst1.comb_ad == 0)) return;
    cc07_n++;
    pm_log("cc07 f%u n %d cyc %d leg %d plan %d why %d prim_a %u tex_on %d valid0 %d", pm_frame_count(), cc07_n,
           (int)((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3), leg_mode, plan_active, ls_why, nd_ws.prim & 0xffu, nd_ws.tex_on, nd_ws.td[0].valid);
}


/* S25: a plan that reads TEXEL1. Tile 1 is loaded and kept at unit 1, tile 0 stays the current texture, and the
 * unit 1 coordinates are the first ones moved by the difference of the two tile windows. */
unsigned aux_key;


/* Clip positions, not object positions. The identity matrix leaves x,y,z,w
 * alone and the viewport places them. */
const float st_off[2] = { 0.f, 0.f }; /* H06b1 */

/* FLK1 debug switch tritrace=<first frame>:<count>: one line per triangle of draw_tri_cpu in the window. Corner w, screen X and Y (x/w),
 * clip bits, the cull area sign and the decision (what). At most 4000 lines per run. */
static int tritrace_on(void) {
    static int a = -2, b;
    if (a == -2) {
        char v[24];
        a = -1;
        if (pm_debug_value("tritrace", v, sizeof v) && sscanf(v, "%d:%d", &a, &b) == 2) b += a;
    }
    return a >= 0 && (int)pm_frame_count() >= a && (int)pm_frame_count() < b;
}
unsigned tritrace_seq, tritrace_lines;
static void tritrace_tri(const char* what, unsigned seq, const float* p1, const float* p2, const float* p3, int cb1, int cb2, int cb3) {
    if (tritrace_lines >= 4000) return;
    tritrace_lines++;
    pm_log("tri f%u s%u %s w %.4f %.4f %.4f X %.3f %.3f %.3f Y %.3f %.3f %.3f cb %x %x %x cross %.6g", pm_frame_count(), seq, what,
           p1[3], p2[3], p3[3], p1[0] / p1[3], p2[0] / p2[3], p3[0] / p3[3], p1[1] / p1[3], p2[1] / p2[3], p3[1] / p3[3], cb1, cb2, cb3,
           cull_cross(p1, p2, p3));
}

/* PO3b, PO3c: one triangle of the raw Vtx path. The caller checked that the three vertices have a ring slot, one matrix and one texture scale, and that
 * the triangle is plain (unlit, no texgen, no screen vertex, no fog). The colour and depth overrides of the CPU path become uniforms: ENV or the plan
 * constant replace the colour, alpha 255, the boost of the second cycle, and G_ZS_PRIM replaces the depth. Returns 0 when the path refuses. */
typedef NdGvP GvP; /* S3N-1: the type and the function are in nd_model.c (nd_gv_params) */
/* The draw parameters of a raw triangle from the state and the texture scale `tss` of its vertices. */
void pm_gbi_gv_at(unsigned tss, uint32_t geo, int tw, int th, GvP* o);





/* GV1: the id of the matrix of the next vertex load or static group in gm_tab: the combined matrix, and for a lit load also the model view matrix of the normal. -1 when the table is full. */





extern u8 gMapShapeData[], TextureHeap[], pm_world_entity_heap[], heap_generalHead[], heap_spriteHead[];
/* 1: the list is drawn (the caller skips the G_DL after the record); 0: the walker runs the old words */




__attribute__((noinline)) void draw_tri_cpu(const int* idx) {
    ClipV poly[12];
    int n, i;
    const unsigned seq = ++tritrace_seq;
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
            if (tritrace_on()) tritrace_tri("early-clip", seq, a, b, c, vert_cb[idx[0]], vert_cb[idx[1]], vert_cb[idx[2]]);
            n_out++;
            GP_LAP(GPL_EARLY);
            return;
        }
        if (nd_ws.geom & G_CULL_BOTH) {
            float cross = cull_cross(a, b, c);
            unsigned ct = nd_ws.geom & G_CULL_BOTH;
            if (ct == G_CULL_BOTH || (ct == G_CULL_FRONT && cross <= 0) || (ct == G_CULL_BACK && cross >= 0)) {
                if (tritrace_on()) tritrace_tri("early-cull", seq, a, b, c, vert_cb[idx[0]], vert_cb[idx[1]], vert_cb[idx[2]]);
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
    ensure_tile(nd_ws.use_t1 ? 1 : 0);
    GP_LAP(GPL_ENSURE);
    dual_bind();
    GP_LAP(GPL_DUAL);
    GP_LAP(GPL_EARLY);
    for (i = 0; i < 3; i++) {
        if (!early) st_cor++;
        if (vert_gvf[idx[i]] != 3) gv_xf(idx[i]);
        xform_vtx(vert_p[idx[i]], nd_ws.tex_on ? vert_st[idx[i]] : st_off, vert_c[idx[i]], &poly[i]);
        if (!nd_ws.tex_on) { /* H06b1: texel (0, 0), the centre of the first texel */
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
        if (tritrace_on()) tritrace_tri("clip", seq, (float*)&poly[0], (float*)&poly[1], (float*)&poly[2], clip_bits(&poly[0]), clip_bits(&poly[1]), clip_bits(&poly[2]));
        n_out++;
        return;
    }
    if (!early && (nd_ws.geom & G_CULL_BOTH)) {
        /* V02: the RSP culls by the sign of the screen space area (G_CULL_FRONT / G_CULL_BACK / both), same test as
         * libultraship. A one sided wall (the cliff skirt around the Mario house yard, seen from inside) draws only
         * if this test runs. */
        float cross = cull_cross((const float*)&poly[0], (const float*)&poly[1], (const float*)&poly[2]);
        unsigned ct = nd_ws.geom & G_CULL_BOTH;
        if (ct == G_CULL_BOTH || (ct == G_CULL_FRONT && cross <= 0) || (ct == G_CULL_BACK && cross >= 0)) {
            if (tritrace_on()) tritrace_tri("cull", seq, (float*)&poly[0], (float*)&poly[1], (float*)&poly[2], clip_bits(&poly[0]), clip_bits(&poly[1]), clip_bits(&poly[2]));
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
    if (tritrace_on()) tritrace_tri(early ? "early-keep" : "keep", seq, (float*)&poly[0], (float*)&poly[1], (float*)&poly[2], clip_bits(&poly[0]), clip_bits(&poly[1]), clip_bits(&poly[2]));
    gp3_t0_surv();
    n_in++;
    n = 3;
    if (comb_mode5 && !plan_active) {
        float en[4];
        unpack8(nd_ws.envc, en);
        for (i = 0; i < n; i++) { poly[i].r = en[0] * 255.f; poly[i].g = en[1] * 255.f; poly[i].b = en[2] * 255.f; poly[i].a = en[3] * 255.f; }
    }
    sync_render();
    if ((nd_ws.omode_l & (1u << G_MDSFT_ZSRCSEL))) {
        /* X01: depth source G_ZS_PRIM, every pixel gets the primitive depth (the battle start swirl and other stencil passes) */
        float zn = prim_ndc_z();
        for (i = 0; i < n; i++) poly[i].z = zn * poly[i].w;
    } else if (img_role(0) == ROLE_DEPTH) {
        /* B3: case 6, the combiner is PRIM. B3b: the vertices keep their own depth, the GPU tests it against the world and writes zn there */
        pm_gpu_set_zmark(1, 2.f * depth_of_word(word_5551(nd_ws.prim)) - 1.f);
    }
    if (plan_active && cur_plan.vtxname != PM_NM_SHADE && cur_plan.vtxname != PM_NM_NONE) {
        /* S25: the vertex colour carries a constant (PRIM, ENV or PRIM_LOD_FRAC) because the shade is not read. */
        float vc[4];
        unpack8(cur_plan.vtx, vc);
        for (i = 0; i < n; i++) { poly[i].r = vc[0] * 255.f; poly[i].g = vc[1] * 255.f; poly[i].b = vc[2] * 255.f; poly[i].a = vc[3] * 255.f; }
    } else if (!comb_alpha_shade && !(plan_active && cur_plan.uses_sha)) {
        for (i = 0; i < n; i++) poly[i].a = 255;
    }
    if (fogmix_on && (nd_ws.geom & G_FOG)) { /* F08: RSP fog factor, clamp(z / w * fm + fo) replaces the vertex alpha */
        for (i = 0; i < n; i++) {
            float f = (poly[i].w > 0.f ? poly[i].z / poly[i].w : poly[i].z * 32767.f) * fog_fm + fog_fo;
            poly[i].a = f < 0.f ? 0.f : f > 255.f ? 255.f : f;
        }
    }
    {
        int boost = dual_on && nd_ws.tst1.comb2_a == G_CCMUX_SHADE && nd_ws.tst1.comb2_b == G_CCMUX_ENVIRONMENT &&
                    nd_ws.tst1.comb2_c == G_CCMUX_COMBINED && nd_ws.tst1.comb2_d == G_CCMUX_COMBINED;
        pm_gpu_set_rgb_scale(boost ? 2 : 1);
        if (!boost) goto no_boost;
    }
    {
        /* Cycle 2 is (SHADE - ENV) * COMBINED + COMBINED = COMBINED * (SHADE - ENV + 1), up to 2.
         * The texture stage multiplies by 2, so the vertex carries half of it. */
        float en[4];
        unpack8(nd_ws.envc, en);
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
    {   /* SEAM: a triangle the raw path refused still takes its position from the same GPU matrix as the raw path (post x game, one dp4 on the object vertex). Positions are then
         * bit-identical along a shared edge. Clip coordinates (CPU product, then the post matrix) round differently and left one pixel gaps. Not for a screen position (G_MODIFYVTX),
         * a primitive depth, vertices of different matrices, vertices not in the ring, or an orthographic matrix (UI). */
        const unsigned g = vert_gmi[idx[0]];
        int obj = !(nd_ws.omode_l & (1u << G_MDSFT_ZSRCSEL)) && g != GV_NONE && vert_gmi[idx[1]] == g && vert_gmi[idx[2]] == g;
        for (i = 0; obj && i < 3; i++) obj = vert_ring[idx[i]] != GV_NONE && !vert_scr[idx[i]];
        if (obj) { const float(*m)[4] = gm_tab[g]; obj = m[0][3] != 0.f || m[1][3] != 0.f || m[2][3] != 0.f; }
        if (obj) {
            const Vtx* r0 = (const Vtx*)(pm_gpu_gring() + 16u * vert_ring[idx[0]]);
            const Vtx* r1 = (const Vtx*)(pm_gpu_gring() + 16u * vert_ring[idx[1]]);
            const Vtx* r2 = (const Vtx*)(pm_gpu_gring() + 16u * vert_ring[idx[2]]);
            pm_gpu_set_mvp(gm_tab[g], vp_sx, vp_sy, vp_tx, vp_ty);
            pm_gpu_tri3d((float)r0->v.ob[0], (float)r0->v.ob[1], (float)r0->v.ob[2], poly[0].u, poly[0].v, pack_color(&poly[0]),
                         (float)r1->v.ob[0], (float)r1->v.ob[1], (float)r1->v.ob[2], poly[1].u, poly[1].v, pack_color(&poly[1]),
                         (float)r2->v.ob[0], (float)r2->v.ob[1], (float)r2->v.ob[2], poly[2].u, poly[2].v, pack_color(&poly[2]));
            seam_obj++;
            return;
        }
    }
    seam_clip++;
    pm_gpu_set_mvp(ident_m, vp_sx, vp_sy, vp_tx, vp_ty);
    pm_gpu_vert_clip(poly[0].x, poly[0].y, poly[0].z, poly[0].w, poly[0].u, poly[0].v, pack_color(&poly[0]));
    pm_gpu_vert_clip(poly[1].x, poly[1].y, poly[1].z, poly[1].w, poly[1].u, poly[1].v, pack_color(&poly[1]));
    pm_gpu_vert_clip(poly[2].x, poly[2].y, poly[2].z, poly[2].w, poly[2].u, poly[2].v, pack_color(&poly[2]));
}




/* S25: an opcode the interpreter does not run. Counted, logged at 1, 100, 10000 ... uses. */
unsigned op_cnt[256];
void pm_gbi_init(void) {
    nd_mtx_reset();
    memset(vert_live, 0, sizeof vert_live);
    nd_ws.geom = G_SHADE | G_SHADING_SMOOTH;
    upload_mvp();
}


/* ---- RDP color combiner, one cycle, for work the display list does off screen ---- */




/* A texture rectangle into an off-screen color image, sampled from a TLUT held in TMEM. The
 * shaded-sprite code draws its 16-entry shadow/highlight palette this way. Returns 1 if done. */
__attribute__((noinline)) int rect_to_image(int xl, int yl, int xh, int yh, unsigned tile, u32 hs, u32 hd) {
    tex_gen++; /* TBREP: writes an image */
    if (!nd_ws.cimg || pm_is_cfb(nd_ws.cimg) || tile >= 8 || nd_ws.tst1.tile_tmem[tile] < 256) return 0;
    int x0 = xl >> 2, y0 = yl >> 2, x1 = xh >> 2, y1 = yh >> 2;
    if (x0 < sci_x0) x0 = sci_x0;
    if (y0 < sci_y0) y0 = sci_y0;
    if (x1 > sci_x1) x1 = sci_x1;
    if (y1 > sci_y1) y1 = sci_y1;
    float s0 = (float)(s16)(hs >> 16) / 32.f;
    float t0 = (float)(s16)(hs & 0xffff) / 32.f;
    float dsdx = (float)(s16)(hd >> 16) / 1024.f;
    float dtdy = (float)(s16)(hd & 0xffff) / 1024.f;
    float sl = (float)nd_ws.tst1.tile_sz[tile][0] / 4.f, tl = (float)nd_ws.tst1.tile_sz[tile][1] / 4.f;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            float s = s0 + dsdx * (float)(x - (xl >> 2)) - sl;
            float t = t0 + dtdy * (float)(y - (yl >> 2)) - tl;
            int ts = (int)s;
            (void)t;
            if (ts < 0) ts = 0;
            /* A TLUT in TMEM holds each entry four times in a row. One row only. */
            unsigned word = nd_ws.tst1.tile_tmem[tile] + (unsigned)(ts >> 2);
            unsigned entry = word - 256u;
            if (entry > 255u) continue;
            float tx[4], none[4] = { 0, 0, 0, 0 }, o[4];
            pal_rgba(tlut_pal[entry >> 4], entry & 15u, tx);
            cc_cycle1(tx, none, o);
            unsigned r = (unsigned)(o[0] * 31.f + .5f), g = (unsigned)(o[1] * 31.f + .5f),
                     b = (unsigned)(o[2] * 31.f + .5f), a = o[3] >= .5f;
            unsigned c = (r << 11) | (g << 6) | (b << 1) | a;
            u8* d = (u8*)nd_ws.cimg + ((size_t)y * (size_t)cimg_stride + (size_t)x) * 2u;
            d[0] = (u8)(c >> 8);
            d[1] = (u8)c;
        }
    }
    pm_tex_invalidate(nd_ws.cimg, 64);
    return 1;
}

/* Tile 0 reads the shading palette, tile 1 the sprite palette, both through the same 4-bit
 * indices, and the first combiner cycle multiplies them. Build the 16 x 16 result the
 * combiner would give, and let pm_tex apply it per texel. Returns 1 if the texture is bound. */

/* Bind the texture of tile n (0 or 1) for the next draw. */
/* GA1: a glyph (4 bit CI, message font) is one cell of the sheet atlas. When the atlas of its sheet and palette is the bound
 * texture, the glyph needs no flush and no wrap call: tile_bound compares the raster, which differs per glyph. */
extern unsigned short** nuGfxCfb;
extern unsigned nuGfxCfbNum;
extern int pm_wide;
/* FB1: the frame buffer base of the previous colour buffer when tile n is a clamp window of it (no mask), else NULL. The strip rows
 * start at base + (y0 * 320 + x0) * 2 (stride 320, 16 bit). */

/* D03: one texel of tile n as RGBA 0..255. Tile 0 wraps, tile 1 clamps (draw_box: pattern and corner image).
 * Returns 0 for a format not handled. */

/* D03: a draw_box window with the bg_tile pattern. The combiner is (PRIM - ENV) * (TEXEL0 * TEXEL1) + ENV, alpha
 * TEXEL1: tile 0 is the wrapped pattern, tile 1 the corner image, clamped, so its edge texels make the border.
 * The PICA path binds one texture, so the product TEXEL0 * TEXEL1 (colour) and TEXEL1 alpha are composed on the
 * CPU into an RGBA texture (cached by content, 128 x 128 pixel chunks), then drawn with (PRIM - ENV) * texel + ENV.
 * Returns 1 if drawn. */
unsigned char box_buf[128 * 128 * 4];
__attribute__((noinline)) int box_pattern_rect(int xl, int yl, int xh, int yh, u32 hs, u32 hd) {
    tex_gen++; /* TBREP */
    int X0 = xl >> 2, Y0 = yl >> 2, X1 = xh >> 2, Y1 = yh >> 2; /* exclusive lower right, 1-cycle mode */
    float s0 = (float)(s16)(hs >> 16) / 32.f, t0 = (float)(s16)(hs & 0xffff) / 32.f;
    float dsdx = (float)(s16)(hd >> 16) / 1024.f, dtdy = (float)(s16)(hd & 0xffff) / 1024.f;
    unsigned char tst[4];
    if (!box_pat || !nd_ws.td[0].valid || !nd_ws.td[1].valid || !nd_ws.td[0].src || !nd_ws.td[1].src) return 0;
    if (!tile_texel(0, 0, 0, tst) || !tile_texel(1, 0, 0, tst)) return 0;
    int cx0 = X0 < 0 ? 0 : X0, cy0 = Y0 < 0 ? 0 : Y0, cx1 = X1 > 320 ? 320 : X1, cy1 = Y1 > 240 ? 240 : Y1;
    pm_gpu_flush();
    pm_gpu_set_combine(5);
    sc_ok = 0; nd_sr_forget(); /* DIP2: pm_gpu holds a combine mode sync_combine did not set */
    for (int ys = cy0; ys < cy1; ys += 128) {
        for (int xs = cx0; xs < cx1; xs += 128) {
            int cw = cx1 - xs < 128 ? cx1 - xs : 128, ch = cy1 - ys < 128 ? cy1 - ys : 128;
            int sa = (int)__builtin_floorf(s0 + dsdx * (float)(xs - X0));
            int ta = (int)__builtin_floorf(t0 + dtdy * (float)(ys - Y0));
            unsigned key = 2166136261u;
            unsigned parts[] = { (unsigned)nd_ws.td[0].src, (unsigned)nd_ws.td[1].src, (unsigned)nd_ws.td[0].x0, (unsigned)nd_ws.td[0].y0, (unsigned)nd_ws.td[1].x0,
                                 (unsigned)nd_ws.td[1].y0, (unsigned)(nd_ws.td[0].w | nd_ws.td[0].h << 8 | nd_ws.td[1].w << 16 | nd_ws.td[1].h << 24),
                                 (unsigned)(nd_ws.td[0].fmt | nd_ws.td[0].siz << 3 | nd_ws.td[1].fmt << 6 | nd_ws.td[1].siz << 9), (unsigned)sa, (unsigned)ta,
                                 hd, (unsigned)(cw | ch << 8), (unsigned)nd_ws.td[0].stride | (unsigned)nd_ws.td[1].stride << 16 };
            for (unsigned k = 0; k < sizeof parts / sizeof parts[0]; k++) key = (key ^ parts[k]) * 16777619u;
            if (!pm_tex_load_rgba8(box_buf, key, cw, ch, NULL)) {
                for (int y = 0; y < ch; y++) {
                    for (int x = 0; x < cw; x++) {
                        int S = (int)__builtin_floorf(s0 + dsdx * (float)(xs + x - X0));
                        int T = (int)__builtin_floorf(t0 + dtdy * (float)(ys + y - Y0));
                        unsigned char a[4], b[4];
                        tile_texel(0, S - nd_ws.td[0].x0, T - nd_ws.td[0].y0, a);
                        tile_texel(1, S - nd_ws.td[1].x0, T - nd_ws.td[1].y0, b);
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
    nd_ws.cur_tile = -1; /* the bound texture is the composite */
    ens_clear();
    sync_combine();
    return 1;
}

/* G1: ops with an empty handler (sync and no-op). The loop skips them before the prologue. */


/* G2: a state op that leaves the live state as it is. The loop skips it before the prologue. Ops that call sync_combine
 * also need sc_ok: a set sc_ok means that call would do nothing. */
/* R5B: a G_MOVEMEM light that writes the values already in lt[slot] changes nothing. The same op also runs lit_freeze, which
 * acts when a lit vertex is pending, so the skip needs lit_pending == 0 (then lit_freeze returns at once). */
int lt_mem_same(u32 w0, u32 w1) {
    int slot = (int)(((w0 >> 8) & 0xff) * 8 / 24) - 2;
    const u8* l = (const u8*)ptr_of(w1);
    return l && slot >= 0 && slot < 9 && !lit_pending && lt[slot].col[0] == l[0] && lt[slot].col[1] == l[1] && lt[slot].col[2] == l[2] &&
           lt[slot].dir[0] == (s8)l[8] && lt[slot].dir[1] == (s8)l[9] && lt[slot].dir[2] == (s8)l[10];
}

/* CORE-2a-3: a skipped op (g2_same true) still gives the state the native keys read what its write would have: the B8 known bits of a
 * SETOTHERMODE mask and the prim LOD word (w0 low 16 bits, which g2_same does not compare). */

/* H1: rare paths of nd_walk_run. Cold and noinline, so the hot loop does not carry their code. */
void gbi_cold_modifyvtx(u32 w0, u32 w1) { /* B5: F3DEX2 w0 = op, where, vertex * 2. Only G_MWO_POINT_XYSCREEN is used by the game (underwater.c) */
    unsigned where = (w0 >> 16) & 0xff, vi = (w0 & 0xffffu) / 2;
    if (where == G_MWO_POINT_XYSCREEN && vi < 64) {
        vert_sx[vi] = (s16)(w1 >> 16);
        vert_sy[vi] = (s16)(w1 & 0xffffu);
        vert_scr[vi] = 1;
    }
}
/* TEXH: cold and per-tile parts of tex_op, moved out of line. The same operations run in the same order. */



/* M4: the texture bind list of a model (make_texture_gfx: SETTIMG SETTILE LOADTLUT, SETCOMBINE, TEXTURE, SETTIMG SETTILE LOADBLOCK SETTILE SETTILESIZE). tb_ops_generic runs it
 * word by word through tex_op, as before. tb_ops runs the same op functions in list order without the dispatch (tex_op wrapper, tex_op_impl switch, one tex_gen step per op) and
 * leaves out the combine sync after a SETCOMBINE that the TEXTURE word after it repeats: that sync would be overwritten by the next one with no draw between. */
/* M4 state-diff check, switch texdiff: every bind list runs twice from the same start state, once word by word and once fused. The texture, tile, combine and mode state of both
 * ends is compared field by field. A difference is one log line (list, field names). The game goes on with the word by word state. */

/* W1/P1: the outer call sets the producer cursor state. Nested calls (dlc replay) run inside the outer op. */
void pm_gbi_run(void* list, unsigned nbytes) {
    if (!pm_ph_on) { nd_walk_run(list, nbytes); return; }
    if (os_nest == 0) { pre_w0 = (const Gfx*)list; pmx_main = gDisplayContext && list == (void*)gDisplayContext->mainGfx;
        pmk_n = 0; /* WCEN3: the walk copy holds the marks of this list only */
        if (pmx_main) for (int i = 0; i < pmk_all_n && pmk_n < PK_MARKS; i++) if (pmk_all[i].p >= (const Gfx*)list && pmk_all[i].p < (const Gfx*)list + MAIN_GFX_N) { pmk[pmk_n].p = pmk_all[i].p; pmk[pmk_n].s = pmk_all[i].s; pmk_n++; }
        pre_mkn = pmk_n; pre_mk0 = pmk_n ? pmk[0].p : NULL; pmx_cur = pmx_main ? PK_PRE : pk_slot((unsigned)(uintptr_t)list); /* WCEN2: a non-main list has its own key (its address) */ pmx_next = 0; mdcm_next = 0; mdcm_cur = -1; }
    os_nest++;
    nd_walk_run(list, nbytes);
    if (os_open && os_nest == 1) os_close();
    os_nest--;
}



void gbi_vtx_cpu1(const Vtx* s, unsigned k) {
vert_gvf[k] = 3;
vert_st[k][0] = (float)s->v.tc[0] * (float)nd_ws.tex_ss / 65536.f; /* T02: the RSP scales S and T */
vert_st[k][1] = (float)s->v.tc[1] * (float)nd_ws.tex_ts / 65536.f;
vert_c[k] = vert_color(s, nd_mv[nd_mv_sp]);
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




/* M4-2 / S3F-2 / S3F-4: producer record of a model (model.c appendGfx_model, TARGET_3DS), G_PM_REC_WORDS (11) words, R0 to R10. R0: w0 = G_PM_REC << 24 |
 * render mode index << 16 | (texture list index & 0x7f) << 8 (count byte 0); w1 = the bind list. R1: w0 G_MTX flags, w1 header w1. R2: w0 numLights,
 * w1 finalMtx. R3: w0 flags (bit 1 fog, 2 prim, 4 env, 8 blend, 16 shroud render mode, 32 shroud, 64 depth fog, 256 IGNORE_MATRIX), w1 lights group.
 * R4: w0 fog colour word, w1 combine entry (0 when none). R5: w1 fog position word. R6: w0 prim, w1 env. R7: w0 blend, w1 shroud c2.
 * R8: billboard matrix op (0 when none). R9, R10: panner tile size ops (0 when none).
 * rec_apply sets the walker state from them in the old word order: lights, bind list (tb_ops), combine (rec_op), render mode list
 * (the list runs as G_DL), fog (rec_fog), matrix (load_mtx), panner tiles, billboard. rec_op runs one op through the skip test and handler of the walker. */
extern Gfx* ModelRenderModes[];
/* S3F-2: one record op through the skip test and the handler of the walker (the ops the record writes). */
static __attribute__((noinline)) void rec_op(u32 w0, u32 w1) {
    unsigned op = w0 >> 24;
    if (g1_skip[op]) return;
    if (g2_same(op, w0, w1)) { g2_note(op, w0); return; }
    gv_epoch++; /* GV1b, as the walker */
    switch (op) {
    case G_MOVEWORD: gbi_moveword(w0, w1); break;
    case G_SETFOGCOLOR: set_fogcolor(w1); break;
    case G_SETBLENDCOLOR: set_blendcolor(w1); break;
    case G_SETENVCOLOR: gbi_setenv(w1); break;
    case G_SETPRIMCOLOR: case G_SETCOMBINE: case G_SETOTHERMODE_L: case G_SETOTHERMODE_H: case G_SETTILESIZE: tex_op(w0, w1); break;
    default: pm_log("rec: op %u is not a record op f%u", op, pm_frame_count()); break;
    }
}
/* S3F-2: the fog of a record: the colour word, then the position word (the MOVEWORD header is built with the gbi macro, no division here). */
static __attribute__((noinline)) void rec_fogpos(u32 pos) {
    Gfx t;
    gMoveWd(&t, G_MW_FOG, G_MWO_FOG, pos);
    rec_op(t.words.w0, t.words.w1);
}
static __attribute__((noinline)) void rec_fog(u32 col, u32 pos) {
    rec_op((u32)G_SETFOGCOLOR << 24, col);
    rec_fogpos(pos);
}
/* S3G-1: a model list of a record (custom gfx pre or post, geometry): run as G_DL does (dlc_try first). The list runs nested
 * (pm_gbi_run) and returns at its G_ENDDL, as the walker's G_DL push did. */
static __attribute__((noinline)) void rec_dl(const Gfx* l) {
    if (!l) return;
    if (!is_host((u32)(uintptr_t)l)) { gbi_cold_bad_next((u32)(uintptr_t)l, (u32)G_DL << 24, 0); return; }
    if (dlc_on && dlc_try(l)) { /* DLC1: a compiled list, as G_DL */
        if (pm_ph_on) { os_dlc_hit(); mr0_dlc_hit = 1; }
        return;
    }
    pm_gbi_run((void*)l, 0);
}
/* S3G-2: the words after a record: n words run in place (no ENDDL needed, pm_gbi_run stops at its end pointer). */
static __attribute__((noinline)) void rec_aux(const Gfx* l, unsigned n) {
    pm_gbi_run((void*)l, n * 8u);
}
/* S3N-6: the matrix ops of a record (R1, R2 load, R8 billboard), the first part of rec_apply. A cache hit runs them before it decides; a
 * miss after that skips them (mdone) and runs the rest of rec_apply in the old order. */
static __attribute__((noinline)) void rec_matrix(const Gfx* q) {
    u32 f = q[3].words.w0;
    if (!(f & 256u)) load_mtx(q[1].words.w0, q[2].words.w1, q[1].words.w1);
    if (q[8].words.w0) load_mtx(q[8].words.w0, q[8].words.w1, q[1].words.w1);
}
static __attribute__((noinline)) void rec_apply(const Gfx* q, int mdone) {
    u32 f = q[3].words.w0; /* S3F-2b: the flags of R3 (see model.c appendGfx_model) */
    rec_n++;
    gv_epoch++; /* GV1b: the texture and render mode words it replaces counted here */
    if (f & 4u) rec_op((u32)G_SETPRIMCOLOR << 24, q[6].words.w0); /* depth and remap: the old words before the lights */
    if (f & 8u) rec_op((u32)G_SETENVCOLOR << 24, q[6].words.w1);
    if (f & 64u) rec_fog(q[4].words.w0, q[5].words.w1);
    if (q[3].words.w1) rec_lights((const Lightsn*)(uintptr_t)q[3].words.w1, q[2].words.w0); /* S3F-1b: lights, as the old word order */
    u32 w0 = q->words.w0;
    const Gfx* tg = (const Gfx*)(uintptr_t)q->words.w1;
    if (q[13].words.w0) rec_aux(q + G_PM_REC_WORDS, q[13].words.w0); /* S3G-2: the words after the record (special tile list, untextured ops) run in place of the bind */
    else if (tg) tb_ops(tg);
    if (q[4].words.w1) { /* S3F-2: the combine word, after the bind as in the old order */
        const Gfx* e = (const Gfx*)(uintptr_t)q[4].words.w1;
        rec_op(e->words.w0, e->words.w1);
    }
    rec_dl(ModelRenderModes[(w0 >> 16) & 0xffu]); /* ST-4: the list runs as G_DL (the walker handlers) */
    if (f & 32u) { /* S3F-2b: shroud: render mode word, fog colour, blend colour, fog position (the old order) */
        Gfx t;
        gDPSetRenderMode(&t, PM_RM_SHROUD, q[7].words.w1);
        rec_op(t.words.w0, t.words.w1);
        rec_op((u32)G_SETFOGCOLOR << 24, q[4].words.w0);
        if (f & 16u) rec_op((u32)G_SETBLENDCOLOR << 24, q[7].words.w0);
        rec_fogpos(q[5].words.w1);
    } else if (f & 2u) {
        rec_fog(q[4].words.w0, q[5].words.w1); /* S3F-2: fog after the render mode, as the old word order */
    }
    if (!(f & 256u) && !mdone) load_mtx(q[1].words.w0, q[2].words.w1, q[1].words.w1); /* S3F-1a: the matrix load of the model, the same function as G_MTX (256: IGNORE_MATRIX) */
    rec_dl((const Gfx*)(uintptr_t)q[11].words.w0); /* S3G-1: custom gfx pre, after the matrix as the old order */
    if (q[9].words.w0) rec_op(q[9].words.w0, q[9].words.w1); /* S3F-2c: panner tile size, main then aux, as the old order */
    if (q[10].words.w0) rec_op(q[10].words.w0, q[10].words.w1);
    if (q[8].words.w0 && !mdone) load_mtx(q[8].words.w0, q[8].words.w1, q[1].words.w1); /* S3F-2c: billboard matrix op (MUL, no push) */
}
/* S3F-4: the record is the whole state of the model (the walker model path is gone, no old words follow it). */
extern int pm_gpu_ndm_on(void); /* S3N-3b: switch ndmcheck */
void pm_gpu_spr_rec(const uint32_t* rec); /* S4-1: a sprite record (sprcheck; pm_gpu.c) */
extern void pm_gpu_ndm_begin(const uint32_t* rec, const uint32_t* modew, const uint32_t* combw, const void* gfx, unsigned texidx, const void* geo, const NdMKey* mk);
extern void pm_gpu_model_key(const uint32_t* rec, NdMKey* k); /* S3N-6 */
extern void pm_gpu_ndm_end(void);
extern int pm_gpu_model_native(const uint32_t* rec, const uint32_t* modew, const uint32_t* combw, const void* gfx, unsigned texidx, const void* geo, const NdMKey* mk); /* S3N-4: the switch */
extern int pm_gpu_model_hit(const NdMKey* k, NdMEnt* e);
extern void pm_gpu_model_fb_cached(unsigned why); /* S3N-7, S3L-1: the code of the stored fallback */ /* S3N-6: the cached draws of the key, 1 when drawn (e: the entry found by the lookup) */
int rec_nat_last; /* WCEN2: 1 when the last rec_run drew natively */
unsigned rec_run(const Gfx* q) {
    NdMKey mk;
    NdtT tk = pm_gpu_ndt_begin(); /* S3N-7: key build */
    pm_gpu_model_key((const uint32_t*)q, &mk); /* S3N-6: the cache key, from the walker state before the record is applied */
    pm_gpu_ndt_end(tk, NDT_KEYB);
    NdtT tf0 = pm_gpu_ndt_begin(); /* S3N-7: cache lookup */
    NdMEnt* mce = nd_mc_find(&mk);
    int cached = mce != NULL;
    pm_gpu_ndt_end(tf0, NDT_FIND);
    if (!cached) { nd_mc_miss(&mk); extern void pm_gpu_model_miss_diag(const NdMKey* k); pm_gpu_model_miss_diag(&mk); } /* S3N-7: the miss class (diagnostic) */
    int ndm = pm_gpu_ndm_on();
    int nat = 0, mdone = 0, hit = 0;
    if (cached) { NdtT tm = pm_gpu_ndt_begin(); rec_matrix(q); pm_gpu_ndt_end(tm, NDT_MTX); mdone = 1; } /* S3N-6: a cached key: its matrix first, the hit test reads the matrix */
    if (!ndm && mdone && q[12].words.w0 && pm_gpu_model_hit(&mk, mce)) {
        /* S3N-6 hit: the record state is the cached one (rec_apply is not run; its matrix ops ran above). The sticky shadows are cleared as after a native draw.
         * POND (A1): a record with no geometry (q[12] 0, the water copy of model.c mdl_draw_hidden_panel_surface) never takes the hit path: its state is its whole effect, so rec_apply must run. */
        rec_n++;
        gv_epoch++;
        nat = 1; hit = 1;
        sc_ok = 0; nd_sr_forget();
    } else {
    NdtT tmi = pm_gpu_ndt_begin(); /* S3N-7: apply: the non-hit path, rec_apply and the fallback or the model call (exclusive of the submit and the matrix) */
    rec_apply(q, mdone);
    { /* MDL-1b v2: once per record after rec_aux, before the ndm and model branches (R13 texture-only records) */
        const uint32_t* qw = (const uint32_t*)q; uint32_t cw_[4]; int cb_ = 0; int t1 = 0;
        if (qw[26] && !pm_debug_has("r13off") && nd_r13_texonly(qw + NDMC_RW, qw[26], cw_, &cb_)) t1 = pm_gbi_bind_tile1();
        pm_gpu_set_r13_t1(t1);
    }
    if (ndm) pm_gpu_ndm_begin((const uint32_t*)q, (const uint32_t*)ModelRenderModes[(q->words.w0 >> 16) & 0xffu],
        q[4].words.w1 ? (const uint32_t*)(uintptr_t)q[4].words.w1 : NULL, (const void*)(uintptr_t)q->words.w1, (q->words.w0 >> 8) & 0x7fu, (const void*)(uintptr_t)q[12].words.w0, &mk);
    else if (mce && mce->neg && mce->ep == pm_tex_epoch()) { pm_gpu_model_fb_cached(mce->why); } /* S3N-7: a stored fallback: walker geometry, no draw attempt */
    else if (pm_gpu_model_native((const uint32_t*)q, (const uint32_t*)ModelRenderModes[(q->words.w0 >> 16) & 0xffu],
        q[4].words.w1 ? (const uint32_t*)(uintptr_t)q[4].words.w1 : NULL, (const void*)(uintptr_t)q->words.w1, (q->words.w0 >> 8) & 0x7fu, (const void*)(uintptr_t)q[12].words.w0, &mk) >= 0) {
        /* S3N-4: the record drew natively (nd_model_draw): no walker geometry. The native draws set the TEV and the texture unit, so the walker
         * sends its combine and render mode again at its next op (the shadows of those ops are cleared). */
        nat = 1;
        sc_ok = 0; nd_sr_forget();
    }
    pm_gpu_ndt_end(tmi, NDT_APPLY);
    }
    if (q[12].words.w0 == 0 && PMDL("waterlog") && pm_frame_count() >= 200u && pm_frame_count() <= 201u) { /* POND-M1: copy record with no geometry; hit taken or not, the combine after the decision */
        unsigned tss_, geo_, cw0_, cw1_; pm_gbi_sticky(&tss_, &geo_, &cw0_, &cw1_);
        pm_log("wrec f%u copy hit %d nat %d cw %08x %08x", pm_frame_count(), hit, nat, cw0_, cw1_);
    }
    if (!nat) { NdtT tf = pm_gpu_ndt_begin(); rec_dl((const Gfx*)(uintptr_t)q[12].words.w0); pm_gpu_ndt_end(tf, NDT_FB); }  /* S3G-1: geometry (0 when none); S3N-7: the fallback walk */
    rec_dl((const Gfx*)(uintptr_t)q[11].words.w1);  /* S3G-1: custom gfx post */
    if (q[12].words.w1) gbi_popmtx(64u);            /* S3G-1: gSPPopMatrix(G_MTX_MODELVIEW), one matrix */
    if (ndm) pm_gpu_ndm_end();
    rec_nat_last = nat;
    return G_PM_REC_WORDS - 1 + q[13].words.w0; /* R1 to R12: matrix, lights, fog, combine, prim, env, blend, shroud, billboard, panner, custom gfx, geometry, pop; then the R13 words */
}



void pm_gbi_fallback(void) {
    static int ready;
    static Gfx dl[8];
    static Vtx fv[3];
    static Mtx fm;
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
