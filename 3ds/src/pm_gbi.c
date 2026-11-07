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
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__) /* PG0: the flush counter by cause (switch gbiprof) */
int gp_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("gbiprof"); return v; }
unsigned gp_same[8], gp_set[8]; /* PG0: state op calls and calls that set the value it already had: 0 combine, 1 prim, 2 env, 3 tile 0, 4 tile size, 5 sync_combine calls, 6 sync_render calls */
__attribute__((cold, noinline)) void gp_note(int k, int same) { gp_set[k]++; if (same) gp_same[k]++; }
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
/* X01 debug switch xtrace=<first frame>:<count>: log fill rects, prim depth and render state changes in that frame window. */
extern unsigned pm_frame_count(void);
int xt_on(void) {
    static int a = -2, b;
    if (a == -2) {
        char v[24];
        a = -1;
        if (pm_debug_value("xtrace", v, sizeof v) && sscanf(v, "%d:%d", &a, &b) == 2) b += a;
        else a = -1;
    }
    return a >= 0 && (int)pm_frame_count() >= a && (int)pm_frame_count() < b;
}

/* V02 debug switch drawlog=<first frame>:<count> or drawlog=file (see dd_on): one line per mesh chunk (a G_VTX load at slot 0 starts one) with the
 * vertex source (the model data address), the two innermost G_DL targets, render mode, texture and screen bounds. */
int dd_on(void) {
    static int a = -2, b, file;
    if (a == -2) {
        char v[24];
        a = -1;
        if (pm_debug_value("drawlog", v, sizeof v) && sscanf(v, "%d:%d", &a, &b) == 2) b += a;
        else { a = -1; file = pm_debug_value("drawlog", v, sizeof v) && !strcmp(v, "file"); }
    }
    if (file) { /* drawlog=file: create sdmc:/3ds/PaperMario/drawlog.now (Azahar: %APPDATA%/Azahar/sdmc/3ds/PaperMario) and the next frame is logged. For a spot a person walks to. */
        static unsigned last;
        unsigned f = pm_frame_count();
        if (f != last) {
            last = f;
            if (f % 10 == 0) {
                FILE* t = fopen("sdmc:/3ds/PaperMario/drawlog.now", "rb");
                if (t) { fclose(t); remove("sdmc:/3ds/PaperMario/drawlog.now"); a = (int)f + 1; b = a + 1; }
            }
        }
    }
    return a >= 0 && (int)pm_frame_count() >= a && (int)pm_frame_count() < b;
}
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
struct {
    unsigned vc, gm, idx, top, src, dl0, dl1, oml, omh, zb, timg, fmt, siz, w, h, prim, ntri, tex;
    float x0, y0, x1, y1, w0, w1;
} dd;
unsigned dd_idx, dd_dl0, dd_dl1, dd_top;
/* ddonly=<lo>:<hi>: inside the drawlog window only chunks with lo <= index <= hi draw (bisect which draw makes a fault). */
static int dd_skip(void) {
    static int lo = -2, hi;
    if (lo == -2) {
        char v[24];
        if (!(pm_debug_value("ddonly", v, sizeof v) && sscanf(v, "%d:%d", &lo, &hi) == 2)) lo = -1, hi = -1;
    }
    return lo >= 0 && dd_on() && ((int)dd.idx < lo || (int)dd.idx > hi);
}
static void dd_line(void);
void dd_end(void) {
    if (dd.ntri && dd_on()) dd_line(); /* TEXGARBLE: one line per chunk, written after tst1 is declared */
    dd.ntri = 0;
}

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
/* A texture tile as the RDP holds it. Tile 0 is the render tile. A combiner that reads only TEXEL1
 * samples tile 1 (draw_box takes its window body from tile 1). One texture is bound at a time,
 * so the tile the next draw needs is bound just before it. */
/* S6-2a (entcheck): at the end record of an entity model draw (G_PM_ENT with G_PM_ENT_END, entity_model.c), the walker's tile 0 state (td[0], pending for the
 * next draw) against nd_dlc_tex_decode of the model list. eq: the same src, fmt, siz, w, h, stride, origin 0, and pal and pal_n for CI. Counts: ent (lists with a texture op),
 * nat (NDTX_OK, native-ready), fb per code (NDTX_MULTI and the fallback codes, index 2 to 8). Line every 300 frames. */
void pm_gbi_ent_texchk(const uint32_t* dl) {
    static unsigned n_ent, n_eq, n_diff, n_nat, n_fb[NDTX_N], n_dlog, n_tbad, next = 300u;
    NdTexDec d;
    int c = nd_dlc_tex_decode(dl, &d);
    unsigned f;
    extern void pm_log_prof_line(const char* fmt, ...);
    extern unsigned pm_frame_count(void);
    if (c != NDTX_NONE) {
        n_ent++;
        if (c == NDTX_OK) n_nat++; else n_fb[c]++;
        if (c == NDTX_WIN && n_tbad++ < 6) pm_log_prof_line("texwin f%u w %u h %u ldw %u ldh %u tsiz %u", pm_frame_count(), d.w, d.h, d.stride, d.pal_n, d.fmt);
        if (c == NDTX_TILE && n_tbad++ < 6) pm_log_prof_line("texbad f%u op %02x tile %u", pm_frame_count(), d.bad_op >> 8, d.bad_op & 7u);
        if (c == NDTX_OK || c == NDTX_MULTI) {
            int same = nd_ws.td[0].valid && (const void*)nd_ws.td[0].src == d.img && nd_ws.td[0].fmt == d.fmt && nd_ws.td[0].siz == d.siz && nd_ws.td[0].w == (int)d.w && nd_ws.td[0].h == (int)d.h &&
                       nd_ws.td[0].stride == (int)d.stride && nd_ws.td[0].x0 == 0 && nd_ws.td[0].y0 == 0 && (d.fmt != 2u || (nd_ws.td[0].pal == d.pal && nd_ws.td[0].pal_n == (int)d.pal_n));
            if (same) n_eq++;
            else {
                n_diff++;
                if (n_dlog++ < 8) pm_log_prof_line("texdiff f%u td v%d src %p fmt %u siz %u w %d h %d str %d x0 %d y0 %d pal %p n %d / dec src %p fmt %u siz %u w %u h %u str %u pal %p n %u",
                    pm_frame_count(), nd_ws.td[0].valid, (const void*)nd_ws.td[0].src, nd_ws.td[0].fmt, nd_ws.td[0].siz, nd_ws.td[0].w, nd_ws.td[0].h, nd_ws.td[0].stride, nd_ws.td[0].x0, nd_ws.td[0].y0, nd_ws.td[0].pal, nd_ws.td[0].pal_n,
                    d.img, d.fmt, d.siz, d.w, d.h, d.stride, d.pal, d.pal_n);
            }
        }
    }
    f = pm_frame_count();
    if (f >= next) {
        next = (f / 300u + 1u) * 300u;
        pm_log_prof_line("texchk f%u ent %u eq %u diff %u nat %u fb %u %u %u %u %u %u %u", f, n_ent, n_eq, n_diff, n_nat, n_fb[2], n_fb[3], n_fb[4], n_fb[5], n_fb[6], n_fb[7], n_fb[8]);
    }
}
/* S6-2c-2 (entcheck census): at the end record of an entity model draw, the list cut at its state ops (nd_ent_segments). Counts per draw: ok (the list
 * compiles to segments), refuse (NDR_OP and the other refusals), the first refused op code, segments and triangles. Line every 300 frames: entseg. */
void pm_gbi_ent_segchk(const uint32_t* dl) {
    static NdEntSeg seg[64];
    static unsigned n_draw, n_ok, n_ref, n_seg, n_tri, n_bad[6], next = 300u;
    static uint32_t last_bad;
    static unsigned n_opc[256];
    unsigned ns = 0, f;
    uint32_t bad = 0;
    extern void pm_log_prof_line(const char* fmt, ...);
    extern unsigned pm_frame_count(void);
    int rc = nd_ent_segments(dl, seg, 64u, &ns, &bad);
    n_draw++;
    if (rc == NDR_OK) {
        n_ok++; n_seg += ns;
        for (unsigned i = 0; i < ns; i++) n_tri += seg[i].ntri;
    } else {
        n_ref++;
        last_bad = bad;
        n_opc[bad & 0xFFu]++;
        if (n_bad[rc]++ < 1) pm_log_prof_line("entseg refuse f%u rc %d op %02x", pm_frame_count(), rc, bad);
    }
    f = pm_frame_count();
    if (f >= next) {
        next = (f / 300u + 1u) * 300u;
        pm_log_prof_line("entseg f%u draws %u ok %u refuse %u segs %u tri %u lastbad %02x", f, n_draw, n_ok, n_ref, n_seg, n_tri, last_bad);
        for (unsigned k = 0; k < 256u; k++) if (n_opc[k] && (k == 0x0Bu || k == 0x0Du || k == 0x0Fu || k == 0x11u || k == 0x09u || k == 0x08u || k == 0x07u || k == 0x02u || k == 0x03u || k == 0x04u || k >= 0xD0u)) pm_log_prof_line("entseg op %02x n %u", k, n_opc[k]);
    }
}
static void dd_line(void) {
        pm_log("dd f%u #%u src %08x dl %08x %08x geom %08x oml %08x omh %08x tex %u timg %08x fmt %u siz %u %dx%d tri %u scr %.0f %.0f %.0f %.0f w %.2f %.2f tw %08x %08x %08x %08x ts %u %u sz %d %d %d %d",
               pm_frame_count(), dd.idx, dd.src, dd.dl0, dd.dl1, dd.gm, dd.oml, dd.omh, dd.tex, dd.timg, dd.fmt, dd.siz, dd.w, dd.h, dd.ntri,
               dd.x0, dd.y0, dd.x1, dd.y1, dd.w0, dd.w1, nd_ws.tst1.tile_wrap[0][0], nd_ws.tst1.tile_wrap[0][1], nd_ws.tst1.tile_wrap[0][2], nd_ws.tst1.tile_wrap[0][3],
               nd_ws.tst1.tile_shift[0][0], nd_ws.tst1.tile_shift[0][1], nd_ws.tst1.tile_sz[0][0], nd_ws.tst1.tile_sz[0][1], nd_ws.tst1.tile_sz[0][2], nd_ws.tst1.tile_sz[0][3]);
    pm_log("dd1 f%u #%u tile1 fmt %u siz %u tw %08x %08x %08x %08x ts %u %u sz %d %d %d %d td1 valid %d src %08x %dx%d stride %d xy %d %d pal %d dual %d",
           pm_frame_count(), dd.idx, nd_ws.tst1.tile_fmt[1], nd_ws.tst1.tile_siz[1], nd_ws.tst1.tile_wrap[1][0], nd_ws.tst1.tile_wrap[1][1], nd_ws.tst1.tile_wrap[1][2], nd_ws.tst1.tile_wrap[1][3],
           nd_ws.tst1.tile_shift[1][0], nd_ws.tst1.tile_shift[1][1], nd_ws.tst1.tile_sz[1][0], nd_ws.tst1.tile_sz[1][1], nd_ws.tst1.tile_sz[1][2], nd_ws.tst1.tile_sz[1][3],
           nd_ws.td[1].valid, (unsigned)(uintptr_t)nd_ws.td[1].src, nd_ws.td[1].w, nd_ws.td[1].h, nd_ws.td[1].stride, nd_ws.td[1].x0, nd_ws.td[1].y0, nd_ws.td[1].pal_n, dual_on);
}
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
Vtx verts[64];
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
unsigned gv_fb[GVR_N], gv_load_n, gv_tri_n, gv_rej_n, gv_xf_n, gv_chk_out, gv_chk_cull, gv_chk_tri;
unsigned gp1_ld_in, gp1_ld_out, gp1_ld_cross, gp1_vx_in, gp1_vx_out, gp1_vx_cross, gp1_subset;
/* GP3 gate: tile 0 G_SETTILESIZE with td[0] set vs no surviving tri or tile-0 texrect before the next one. */
unsigned gp3_t0_ld, gp3_t0_dead, gp3_t0_pending;
void gp3_t0_set(void) {
    if (gp3_t0_pending) gp3_t0_dead++;
    gp3_t0_ld++;
    gp3_t0_pending = 1;
}
void gp3_t0_surv(void) { gp3_t0_pending = 0; }
float gv_rel_ref[2];
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
/* OPT-SC: the result of sync_combine_work by the PG0 signature of its inputs (the same hash as gbiprof in sync_combine_work). A hit repeats the outputs and the calls into pm_gpu: no decision lookup, no
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
/* PERF-GBI (switch gbiprof, default off): cycle laps inside draw_tri, a tick read costs 1.3 us in Azahar so the totals are inflated, the ranking holds. One load and one branch when off. */
unsigned gp_acc[8], gp_last, gp_ntri, gp_frames;
/* PG0 period: frames per gbiprof report. Switch gbiprofn=<frames> (default 300); a window of 40 frames gets a report at frame 40. */
unsigned gp_period(void) {
    static unsigned v;
    char b[16];
    if (!v) v = pm_debug_value("gbiprofn", b, sizeof b) ? (unsigned)atoi(b) : 300u;
    if (!v) v = 300u;
    return v;
}
unsigned gp_opt[256], gp_opn[256], gp_opt_last; int gp_prev_op = -1; /* time between two ops, charged to the first */

int rmlog_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("rmlog"); return v; }
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
/* DLC1: the static list compiler and the group runner (S3F-4: the former dlc include, inlined here, same unit) */
/* DLC1: compile and replay of the map shape display lists (docs/plans/DLC.md, kind G). Included by pm_gbi.c after draw_tri, so it reads the statics of the interpreter.
 * `dlc` is always on (dlc_enabled turns it off under the dd, xt and rmlog tools), `dlccheck` (hash, exit state and the reject proof of every skipped group, counters in the `dlc` line).
 * A shape list is cut into groups: a group starts at a G_VTX at slot 0 after triangles, so a group reads only the vertices that it loads itself. The compiler keeps the box of the
 * object coordinates of each group. At replay a group whose box lies outside one clip plane is skipped: the interpreter rejects every triangle of it as well. A group in view runs
 * the same code as the interpreter (G_VTX copy to the GV1 ring, draw_tri) without the CPU transform of the vertices. A group that is near the camera plane, lit, or does not
 * fit the ring runs through the interpreter itself (pm_gbi_run on the op range of the group). */
unsigned pm_dlc_gen = 1; /* bumped when the shape buffer is written (pm_tex_invalidate, decode_yay0) */

typedef struct { u32 op0, op1, am, om; s16 lo[3], hi[3]; u16 nvtx, ntri, goff, ng, sn, nbat; u8 tsv_exit; u16* sd; u16* bat_q; u16* ends; } DlcGrp; /* C3: sd = indices then nbat batch end counts; bat_q[gop index before each batch] */
typedef struct { u32 addr, gen, hash, nops, exit_am, exit_om; u16 ngrp; u8 refused; DlcGrp* grp; u32 (*gop)[2]; NdObj o; } DlcEnt;
#define DLC_TAB 2048
#define DLC_MAXOPS 4096
DlcEnt* dlc_tab[DLC_TAB];
unsigned dlc_nent;
int dlc_on; int dlc_depth;
DlcCnt dlc_c;
unsigned sp0_grp, sp0_skip; /* SPLIT0C: groups and skips per phase window (sp0c line), counted under phase only */
unsigned dlc_dirty; /* the last replay left vert_live cleared although the interpreter would have left slots live */
const Gfx dlc_end[1] = { { { (u32)G_ENDDL << 24, 0 } } };

/* hash of a list (words up to G_ENDDL) and of the Vtx bytes of its G_VTX ops */
static u32 dlc_hash(const Gfx* l) {
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
int dlc_enabled(void) {
    return !dd_on() && !xt_on() && !rmlog_on();
}
static int dlcchk_on(void) { return PMDL("dlccheck"); }
static int behroute_on(void) { return PMDL("behroute"); } /* FLK1 debug switch behroute: a visible group with a box corner at w < 0.05 runs through the interpreter, not the static GPU segment */
static __attribute__((noinline)) int dlc_behind(const DlcGrp* g, const float (*m)[4]) {
    for (int c = 0; c < 8; c++) {
        float x = (c & 1) ? g->hi[0] : g->lo[0], y = (c & 2) ? g->hi[1] : g->lo[1], z = (c & 4) ? g->hi[2] : g->lo[2];
        if (m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3] < 0.05f) return 1;
    }
    return 0;
}
static int stdump_on(void) { return PMDL("stdump"); } /* FLK1 state dump per visible static segment, frames 200 to 205 */
int pm_gpu_rgb_scale_get(void);
static int nobox_on(void) { return PMDL("nobox"); } /* FLK1 debug switch nobox: the group box skip of dlc_try is off */
static int skipdump_on(void) { return PMDL("skipdump"); } /* FLK1 debug switch skipdump: at frame 200 each group of a list logs its box, screen box and skip result */
/* FLK1: the screen box of a group (pixels, pm_gbi vp_ scale) and the first clip plane that rejects it. */
static __attribute__((cold, noinline)) void dlc_grp_dump(const DlcGrp* g, const float (*m)[4], unsigned k, unsigned gsk, unsigned gop) {
    static const struct { signed char a, b; } pl[5] = { { 0, 1 }, { 0, -1 }, { 1, 1 }, { 1, -1 }, { 2, -1 } };
    float smin[2] = { 1e9f, 1e9f }, smax[2] = { -1e9f, -1e9f };
    int behind = 0, rej = -1;
    for (int c = 0; c < 8; c++) {
        float x = (c & 1) ? g->hi[0] : g->lo[0], y = (c & 2) ? g->hi[1] : g->lo[1], z = (c & 4) ? g->hi[2] : g->lo[2];
        float p[4];
        for (int q = 0; q < 4; q++) p[q] = m[0][q] * x + m[1][q] * y + m[2][q] * z + m[3][q];
        if (p[3] <= 0) { behind = 1; continue; }
        float sx = p[0] / p[3] * vp_sx + vp_tx, sy = vp_ty - p[1] / p[3] * vp_sy;
        if (sx < smin[0]) smin[0] = sx; if (sx > smax[0]) smax[0] = sx;
        if (sy < smin[1]) smin[1] = sy; if (sy > smax[1]) smax[1] = sy;
    }
    for (int p = 0; p < 5 && rej < 0; p++) {
        int out = 1;
        for (int c = 0; c < 8 && out; c++) {
            float x = (c & 1) ? g->hi[0] : g->lo[0], y = (c & 2) ? g->hi[1] : g->lo[1], z = (c & 4) ? g->hi[2] : g->lo[2];
            float v = m[0][pl[p].a] * x + m[1][pl[p].a] * y + m[2][pl[p].a] * z + m[3][pl[p].a];
            float wv = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];
            float d = v + (float)pl[p].b * wv;
            if (pl[p].b > 0 ? !(d < 0) : !(d > 0)) out = 0;
        }
        if (out) rej = p;
    }
    static unsigned sd_n; if (++sd_n > 400) return;
    pm_log("skipg f%d k %u op0 %u ntri %u nvtx %u skip %u plane %d behind %d scr x %.0f %.0f y %.0f %.0f box %d %d %d %d %d %d", (int)pm_frame_count(), k, gop, g->ntri, g->nvtx, gsk, rej, behind, smin[0], smax[0], smin[1], smax[1], g->lo[0], g->lo[1], g->lo[2], g->hi[0], g->hi[1], g->hi[2]);
}
static int noboxint_on(void) { return PMDL("noboxint"); } /* FLK1 debug switch noboxint: an out group runs through the interpreter instead of the skip */
static void dlc_free(DlcEnt* e) {
    for (unsigned k = 0; k < e->ngrp; k++) { free(e->grp[k].ends); free(e->grp[k].bat_q); }
    free(e->grp); e->grp = NULL; e->gop = NULL; e->ngrp = 0;
    nd_dlc_free(&e->o); /* the vertex and index copies live in the static pools until the next pm_gpu_gstat_reset */
}

#define DLC_GPU_GEOM(g) ((g) & (G_ZBUFFER | G_LIGHTING))

static __attribute__((noinline)) u32 dlc_geom_at(const DlcEnt* e, const DlcGrp* g, u32 entry, unsigned sg) {
    u32 x = (entry & g->am) | g->om;
    for (unsigned i = 0; i < sg; i++) {
        const u32* op = e->gop[g->goff + i];
        x = (x & op[0]) | op[1];
    }
    return x;
}

/* C3: adjacent gop segments share the same draw geom for every list entry (PSO inputs unchanged). */
static __attribute__((noinline)) int dlc_gop_same_geom(const DlcEnt* e, const DlcGrp* g, unsigned sg) {
    u32 a0 = dlc_geom_at(e, g, 0, sg);
    u32 a1 = dlc_geom_at(e, g, 0xffffffffu, sg);
    u32 b0 = dlc_geom_at(e, g, 0, sg + 1u);
    u32 b1 = dlc_geom_at(e, g, 0xffffffffu, sg + 1u);
    return a0 == a1 && b0 == b1 && a0 == b0;
}

/* C3: record draw ranges at compile; merge segments that share PSO geom (ends + gop index before each range). */
static __attribute__((cold, noinline)) void dlc_batch_compile(DlcGrp* g, const DlcEnt* e) {
    unsigned sn = g->sn, ng = g->ng;
    g->nbat = 0;
    free(g->bat_q);
    g->bat_q = NULL;
    free(g->ends);
    g->ends = (u16*)malloc((ng + 1u) * sizeof(u16)); /* the triangle index before each geometry op (nd_dlc gat), then the group triangle count */
    if (!g->ends) return;
    unsigned short* ends = g->ends;
    for (unsigned i = 0; i < ng; i++) ends[i] = e->o.gat[g->goff + i];
    ends[ng] = (u16)sn;
    if (!sn) return;
    if (!ng) {
        g->nbat = 1;
        g->bat_q = (u16*)malloc(sizeof(u16));
        if (!g->bat_q) { g->nbat = 0; return; }
        g->bat_q[0] = 0;
        ends[0] = (u16)sn;
        return;
    }
    g->nbat = (u16)(ng + 1u);
    g->bat_q = (u16*)malloc((ng + 1u) * sizeof(u16));
    if (!g->bat_q) { g->nbat = 0; return; }
    unsigned nb = 0, sg = 0;
    while (sg <= ng) {
        unsigned sr = sg;
        while (sr < ng && dlc_gop_same_geom(e, g, sr)) sr++;
        g->bat_q[nb] = (u16)sg;
        ends[nb] = ends[sr];
        nb++;
        sg = sr + 1u;
    }
    ends[nb - 1u] = (u16)sn;
    g->nbat = (u16)nb;
}

static void dlc_set_geom(u32 g);
static unsigned dlc_gmi(void);
/* S3 M2: the static list compile is nd_dlc (moved from dlc_compile and dlc_static_build): groups, boxes, geometry ops, the Vtx copies in the static vertex area (flag 0)
 * and the triangle indices in the static index area. The DlcEnt keeps the NdObj and the per group view the replay loop reads. A group without pool room keeps sd NULL and
 * runs through the ring (dlc_run_group). */
unsigned short* pm_gpu_gidxs_alloc(unsigned n);
void pm_gpu_gidxs_commit(const unsigned short* p, unsigned n);
static const void* nd_env_vtx(uint32_t w1, unsigned n) {
    const Vtx* src = (const Vtx*)ptr_of(w1);
    if (!src || (u32)src - (u32)gMapShapeData >= 0x40000u || n == 0 || n > 64) return NULL;
    return src;
}
static uint8_t* nd_env_valloc(unsigned n, unsigned* first) {
    unsigned char* dst = NULL;
    int f = pm_gpu_gstat_alloc(n, &dst);
    if (f < 0) return NULL;
    *first = (unsigned)f;
    return dst;
}
static uint16_t* nd_env_ialloc(unsigned n) { return pm_gpu_gidxs_alloc(n); }
static u32 dlc_words[2 * (ND_DLC_MAXOPS + 1)];
static __attribute__((cold, noinline)) int dlc_build(DlcEnt* e, const Gfx* l) {
    unsigned i;
    for (i = 0; i < ND_DLC_MAXOPS; i++) { dlc_words[2 * i] = l[i].words.w0; dlc_words[2 * i + 1] = l[i].words.w1; if ((l[i].words.w0 >> 24) == G_ENDDL) break; }
    NdDlcEnv env = { nd_env_vtx, nd_env_valloc, nd_env_ialloc };
    int rc = nd_dlc_compile(&e->o, dlc_words, &env), pooled = 1;
    if (rc == NDR_FULL) { /* no room in the static areas: compile without the copies, every group runs through the ring */
        NdDlcEnv env2 = { nd_env_vtx, NULL, NULL };
        dlc_c.sfull++;
        rc = nd_dlc_compile(&e->o, dlc_words, &env2);
        pooled = 0;
        if (rc == NDR_OK) { free(e->o.vtx); free(e->o.idx); e->o.vtx = NULL; e->o.idx = NULL; }
    }
    if (rc) return rc;
    e->nops = e->o.nops; e->exit_am = e->o.exit_am; e->exit_om = e->o.exit_om; e->ngrp = e->o.ngrp; e->gop = e->o.gop;
    e->grp = e->ngrp ? (DlcGrp*)calloc(e->ngrp, sizeof(DlcGrp)) : NULL;
    if (e->ngrp && !e->grp) { nd_dlc_free(&e->o); e->ngrp = 0; e->gop = NULL; return NDR_LONG; }
    for (unsigned k = 0; k < e->ngrp; k++) {
        DlcGrp* g = &e->grp[k]; const NdGrp* q = &e->o.grp[k];
        g->op0 = q->op0; g->op1 = q->op1; g->am = q->am; g->om = q->om; memcpy(g->lo, q->lo, 6); memcpy(g->hi, q->hi, 6);
        g->nvtx = q->nvtx; g->ntri = q->ntri; g->goff = (u16)q->gbase; g->ng = q->ng; g->tsv_exit = q->tsv_exit;
        if (pooled) { g->sn = q->ntri; g->sd = e->o.idx + q->ibase; dlc_batch_compile(g, e); }
    }
    if (pooled) {
        if (e->o.nvtx) pm_gpu_gstat_commit(e->o.vfirst, e->o.nvtx);
        if (e->o.nidx) pm_gpu_gidxs_commit(e->o.idx, e->o.nidx);
    }
    return 0;
}

unsigned dbg_dsg, dbg_seg; /* S3N-3c debug: the static refusal codes */
/* DLC2: one range of static triangles with the state of the current epoch. Returns 0 when the raw path refuses (nothing was queued). */
static __attribute__((noinline)) int dlc_seg(const unsigned short* idx, unsigned nt, unsigned gmi) {
    unsigned tss = ((unsigned)nd_ws.tex_ss << 16) | ((unsigned)nd_ws.tex_ts & 0xffffu);
    if (ep_epoch != gv_epoch) gv_epoch_enter();
    dbg_seg = 0;
    if (ep_why >= 0) { dbg_seg = 1 + (unsigned)ep_why; return 0; }
    if (ep_cullboth) { n_out += (int)nt; return 1; }
    if (ep_pepoch != gv_epoch || ep_ptss != tss) {
        gv_params_calc(tss, &ep_p);
        pm_gpu_set_rgb_scale(ep_p.boost ? 2 : 1);
        ep_pepoch = gv_epoch; ep_ptss = tss;
    }
    int gr = pm_gpu_grange((const float(*)[4][4])gm_tab, gmi, ep_p.vp, ep_p.uvso, ep_p.cm, ep_p.ca, ep_p.zm, ep_ct, ep_p.fxon ? &ep_p.fx : NULL, idx, 3u * nt);
    if (!gr) { ep_open = 0; dbg_seg = 9; return 0; }
    if (gr == 2) dlc_c.batch++;
    ep_n += nt;
    ep_open = 1; ep_open_gen = pm_gpu_flush_gen(); ep_open_epoch = gv_epoch; ep_open_tss = tss;
    n_cmd += (int)nt; gp_ntri += nt; n_in += (int)nt; st_gpu_tri += nt; gv_tri_n += nt; PH_ADD(C_GVTRI, nt);
    dlc_c.stri += nt;
    return 1;
}

/* DLC2/C3: a group in view from the static area. Returns 0 when nothing was queued and the group must run through the ring. */
static __attribute__((noinline)) int dlc_static_group(const DlcEnt* e, const DlcGrp* g, unsigned gmi, u32 gg) {
    u32 bad = (gv_lit_ok() ? 0 : G_LIGHTING) | (fogmix_on && !gv_fog_gpu_ok() ? G_FOG : 0), x = nd_ws.geom;
    const u32 lm = G_LIGHTING | G_TEXTURE_GEN, x0 = x & lm; /* the RSP lights at the vertex load: the lighting state must not change inside the group, else the interpreter runs it */
    dbg_dsg = 0;
    if (x & bad) { dbg_dsg = 1; return 0; }
    for (unsigned q = 0; q < g->ng; q++) { x = (x & e->gop[g->goff + q][0]) | e->gop[g->goff + q][1]; if ((x & bad) || (x & lm) != x0) { dbg_dsg = 2; return 0; } }
    if (!g->nbat || !g->bat_q || !pm_gpu_gidx_room(3u * g->sn)) { dbg_dsg = 3; return 0; }
    const unsigned short* ends = g->ends;
    unsigned t = 0, gop = 0;
    int drawn = 0;
    u32 geom_at = gg;
    for (unsigned b = 0; b < g->nbat; b++) {
        while (gop < g->bat_q[b])
            geom_at = (geom_at & e->gop[g->goff + gop][0]) | e->gop[g->goff + gop++][1];
        unsigned end = ends[b];
        if (end > t) {
            dlc_set_geom(geom_at);
            if (!dlc_seg(g->sd + 3u * t, end - t, gmi)) { if (!drawn) { dbg_dsg = 4; return 0; } dlc_c.sfail++; return 1; }
            drawn = 1;
            t = end;
        }
    }
    if (drawn && g->nbat < g->ng + 1u) dlc_c.batch += g->ng + 1u - g->nbat;
    return drawn;
}

static __attribute__((noinline)) DlcEnt* dlc_find(const Gfx* l) {
    u32 a = (u32)l;
    unsigned k = ((a >> 3) * 2654435761u) >> 21 & (DLC_TAB - 1);
    for (unsigned n = 0; n < DLC_TAB; n++, k = (k + 1) & (DLC_TAB - 1)) {
        DlcEnt* e = dlc_tab[k];
        if (!e) {
            if (dlc_nent >= DLC_TAB * 3 / 4) { dlc_c.full++; return NULL; }
            e = (DlcEnt*)calloc(1, sizeof *e);
            if (!e) return NULL;
            e->addr = a; dlc_tab[k] = e; dlc_nent++;
            return e;
        }
        if (e->addr == a) return e;
    }
    return NULL;
}

static __attribute__((noinline)) void dlc_set_geom(u32 g) {
    if (g == nd_ws.geom) return;
    nd_ws.geom = g;
    pm_gpu_set_mode((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0);
    gv_epoch++;
}

/* The matrix id of `combined` (the same search as the G_VTX code), GV_NONE when the table is full. */
static __attribute__((noinline)) unsigned dlc_gmi(void) { return gv_matrix_id(0); }

/* Box test against the clip planes of clip_bits4. Returns 1 when every vertex of the box is outside one plane (by a margin for the float error of the interpreter). */
static __attribute__((noinline)) int dlc_box_out(const DlcGrp* g, const float (*m)[4]) {
    static const struct { signed char a, b; } pl[5] = { { 0, 1 }, { 0, -1 }, { 1, 1 }, { 1, -1 }, { 2, -1 } }; /* coordinate, sign of w: x + w < 0, x - w > 0, y + w < 0, y - w > 0, z - w > 0 */
    float lo[3] = { g->lo[0], g->lo[1], g->lo[2] }, hi[3] = { g->hi[0], g->hi[1], g->hi[2] };
    int out = 0;
    for (int p = 0; p < 5; p++) {
        float k = m[3][pl[p].a] + (float)pl[p].b * m[3][3], mn = k, mx = k, mag = k < 0 ? -k : k;
        for (int q = 0; q < 3; q++) {
            float c = m[q][pl[p].a] + (float)pl[p].b * m[q][3], x = c * lo[q], y = c * hi[q];
            if (x < y) { mn += x; mx += y; } else { mn += y; mx += x; }
            mag += (x < 0 ? -x : x) > (y < 0 ? -y : y) ? (x < 0 ? -x : x) : (y < 0 ? -y : y);
        }
        float eps = mag * 1e-5f + 1e-3f;
        if (pl[p].b > 0 ? mx < -eps : mn > eps) out = 1;
    }
    return out;
}


/* AND of clip_bits4 on the 8 corners of an object-space box (subset P for GP1 out loads). */
static __attribute__((noinline)) unsigned char gp1_box_subset(const s16 lo[3], const s16 hi[3], const float (*m)[4]) {
    unsigned bits = 0x3f;
    for (int i = 0; i < 8; i++) {
        float x = (float)((i & 1) ? hi[0] : lo[0]), y = (float)((i & 2) ? hi[1] : lo[1]), z = (float)((i & 4) ? hi[2] : lo[2]);
        float p[4];
        for (int c = 0; c < 4; c++) p[c] = m[0][c] * x + m[1][c] * y + m[2][c] * z + m[3][c];
        bits &= (unsigned)clip_bits4(p);
    }
    return (unsigned char)bits;
}

/* 1 when every corner is inside all clip planes of clip_bits4. */
static __attribute__((noinline)) int gp1_box_in(const s16 lo[3], const s16 hi[3], const float (*m)[4]) {
    static const struct { signed char a, b; } pl[5] = { { 0, 1 }, { 0, -1 }, { 1, 1 }, { 1, -1 }, { 2, -1 } };
    for (int p = 0; p < 5; p++) {
        float k = m[3][pl[p].a] + (float)pl[p].b * m[3][3], mn = k, mx = k, mag = k < 0 ? -k : k;
        for (int q = 0; q < 3; q++) {
            float c = m[q][pl[p].a] + (float)pl[p].b * m[q][3], x = c * (float)lo[q], y = c * (float)hi[q];
            if (x < y) { mn += x; mx += y; } else { mn += y; mx += x; }
            mag += (x < 0 ? -x : x) > (y < 0 ? -y : y) ? (x < 0 ? -x : x) : (y < 0 ? -y : y);
        }
        float eps = mag * 1e-5f + 1e-3f;
        if (pl[p].b > 0 ? mn < -eps : mx > eps) return 0;
    }
    return 1;
}

/* GP1: exact hybrid box per non-DLC G_VTX load. */
int gp1_box_class(const s16 lo[3], const s16 hi[3], const float (*m)[4], unsigned char* pbits) {
    DlcGrp g;
    g.lo[0] = lo[0]; g.lo[1] = lo[1]; g.lo[2] = lo[2];
    g.hi[0] = hi[0]; g.hi[1] = hi[1]; g.hi[2] = hi[2];
    int out = dlc_box_out(&g, m);
    if (gp1_box_in(lo, hi, m)) return GP1_IN;
    if (out) {
        *pbits = gp1_box_subset(lo, hi, m);
        return GP1_OUT;
    }
    return GP1_CROSS;
}

/* dlccheck: the interpreter rejects every triangle of a skipped group (all three clip bits share one plane). Counts the triangles that it would not reject. */
static __attribute__((cold, noinline)) void dlc_verify_skip(const Gfx* l, const DlcGrp* g, const float (*m)[4]) {
    unsigned char cb[64], live[64] = { 0 }, ever[64] = { 0 };
    int tsv = 1;
    for (u32 j = 0; j < g->op0; j++) { /* FLK1: the slots that an earlier group of this list loaded (the ring keeps them) */
        u32 w0 = l[j].words.w0;
        if ((w0 >> 24) == G_ENDDL) break;
        if ((w0 >> 24) == G_VTX) {
            unsigned n = (w0 >> 12) & 0xff, v0n = (w0 >> 1) & 0x7f, v0 = v0n >= n ? v0n - n : 0;
            for (unsigned k = 0; k < n && v0 + k < 64; k++) ever[v0 + k] = 1;
        }
    }
    for (u32 j = g->op0; j < g->op1; j++) {
        u32 w0 = l[j].words.w0, w1 = l[j].words.w1;
        unsigned op = w0 >> 24;
        if (op == G_VTX) {
            unsigned n = (w0 >> 12) & 0xff, v0n = (w0 >> 1) & 0x7f, v0 = v0n >= n ? v0n - n : 0;
            const Vtx* src = (const Vtx*)ptr_of(w1);
            if (v0 + n > 64) n = 64 - v0;
            if (v0 == 0 && tsv) memset(live, 0, sizeof live);
            tsv = 0;
            for (unsigned k = 0; k < n; k++) {
                float x = src[k].v.ob[0], y = src[k].v.ob[1], z = src[k].v.ob[2], p[4];
                for (int q = 0; q < 4; q++) p[q] = m[0][q] * x + m[1][q] * y + m[2][q] * z + m[3][q];
                cb[v0 + k] = (unsigned char)clip_bits4(p); live[v0 + k] = 1;
            }
        } else if (op == G_TRI1 || op == G_TRI2) {
            tsv = 1;
            for (int t = 0; t < (op == G_TRI2 ? 2 : 1); t++) {
                u32 w = t ? w1 : w0;
                int a = (int)((w >> 16) & 0xff) / 2, b = (int)((w >> 8) & 0xff) / 2, c = (int)(w & 0xff) / 2;
                if (a > 63 || b > 63 || c > 63) continue;
                if (!live[a] || !live[b] || !live[c]) { if (ever[a] || ever[b] || ever[c]) dlc_c.xgrp++; continue; }
                if (!(cb[a] & cb[b] & cb[c] & 0x2f)) dlc_c.missing++;
            }
        }
    }
}

/* One group in view: the interpreter code for G_VTX (GV1 ring copy, no CPU transform) and draw_tri. Returns 0 on success. */
static __attribute__((noinline)) void dlc_run_group(const Gfx* l, const DlcGrp* g, unsigned gmi) {
    unsigned tss = ((unsigned)nd_ws.tex_ss << 16) | ((unsigned)nd_ws.tex_ts & 0xffffu);
    for (u32 j = g->op0; j < g->op1; j++) {
        u32 w0 = l[j].words.w0, w1 = l[j].words.w1;
        switch (w0 >> 24) {
        case G_VTX: {
            unsigned n = (w0 >> 12) & 0xff, v0n = (w0 >> 1) & 0x7f, v0 = v0n >= n ? v0n - n : 0;
            const Vtx* src = (const Vtx*)ptr_of(w1);
            if (v0 + n > 64) n = 64 - v0;
            PH_ADD(C_VTXLOAD, n);
            if (v0 == 0 && tri_since_vtx) memset(vert_live, 0, sizeof vert_live);
            tri_since_vtx = 0;
            int r = pm_gpu_gvtx_load(src, n);
            if (r < 0) { dlc_c.ringfail++; gv_fb[GVR_RING]++; break; } /* cannot happen: the room test counted every vertex and every copy */
            gv_load_n += n; PH_ADD(C_GVLOAD, n);
            for (unsigned i = 0; i < n; i++) {
                unsigned s = v0 + i;
                vert_ring[s] = (unsigned short)(r + (int)i);
                vert_gmi[s] = (unsigned short)gmi;
                vert_tss[s] = tss;
                vert_live[s] = 1;
                vert_scr[s] = 0;
                vert_gvf[s] = 0;
                vert_cb[s] = 0;
            }
            break;
        }
        case G_TRI1:
            tri_prep_ok = 0;
            draw_tri(w0);
            break;
        case G_TRI2:
            tri_prep_ok = 0;
            draw_tri(w0);
            draw_tri(w1);
            break;
        case G_GEOMETRYMODE:
            set_geom(w0, w1);
            gv_epoch++;
            break;
        default:
            break;
        }
    }
}

/* the shape buffer was written: every entry is stale */
static __attribute__((cold, noinline)) void dlc_reset(void) {
    for (unsigned k = 0; k < DLC_TAB; k++) if (dlc_tab[k]) { dlc_free(dlc_tab[k]); free(dlc_tab[k]); dlc_tab[k] = NULL; }
    dlc_nent = 0;
    pm_gpu_gstat_reset();
}
unsigned dlc_seen_gen;
/* A G_DL to l. Returns 1 when the list is done (replayed), 0 when the interpreter must run it. */

/* S3N-3c debug: the walker DLC counters (calls, groups, skips, static, ring, lit, material, static refusals) for the ndmcheck nraw log */
void pm_gbi_dlcc(unsigned* o) {
    o[0] = dlc_c.hit + dlc_c.compile; o[1] = dlc_c.grp; o[2] = dlc_c.skip; o[3] = dlc_c.srep; o[4] = dlc_c.i_ring; o[5] = dlc_c.i_mat; o[6] = dlc_c.i_lit; o[7] = dlc_c.sfail + dlc_c.replay;
    o[8] = dbg_dsg; o[9] = dbg_seg;
}
/* S3N-3c: the static ranges of a shape list for nd_model_draw. The groups that dlc_try replays statically (box test, no lit group, room in the pools), one range per batch as
 * dlc_static_group cuts them. No replay and no counters. Returns the count; -1 when dlc_try would not replay the list statically (the fallback); -2 when out is full. */
int pm_gpu_grange_would(const float m[4][4], unsigned nidx, const unsigned short* idx);
int pm_gpu_grange_why(const float m[4][4], unsigned nidx, const unsigned short* idx); /* S4-1 (diagnostic) */
int pm_gpu_grange_native_would(const float m[4][4], unsigned nidx, const unsigned short* idx); /* S5-1 (b); WIDE1-1: idx = the indices of the range, tested for edge vertices in wide mode */
/* S4-1 (diagnostic): the refusal of pm_dlc_native_ranges by class (1 dlc off or nobox, 2 list range, 3 table, 4 refused build, 5 depth pass, 6 group without static data, 7 lit group, 8 room, 9 walker grange refuses, 10 matrix id) */
unsigned dnr_c[16];
static int dnr(unsigned k) { dnr_c[k]++; return -1; }
void pm_gbi_dnr_take(unsigned* o, unsigned n) { for (unsigned i = 0; i < n && i < 16u; i++) { o[i] = dnr_c[i]; dnr_c[i] = 0; } }
int pm_dlc_native_ranges(const void* lv, uint32_t entry, NdRng* out, unsigned max) {
    const Gfx* l = (const Gfx*)lv;
    if (!dlc_on || !dlc_enabled() || nobox_on() || noboxint_on()) return dnr(1);
    if (!is_host((u32)(uintptr_t)l)) return 0; /* rec_dl: a list outside host memory is not run (gbi_cold_bad_next): no draw */
    if ((u32)l - (u32)gMapShapeData >= 0x40000u || ((u32)l & 7)) return dnr(2);
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    DlcEnt* e = dlc_find(l);
    if (!e) return dnr(3);
    if (e->gen != pm_dlc_gen) {
        dlc_free(e);
        e->gen = pm_dlc_gen; e->refused = 0;
        int r = dlc_build(e, l);
        e->hash = dlc_hash(l);
        if (r) e->refused = (u8)r;
    }
    if (e->refused) return dnr(4);
    const float(*m)[4] = combined;
    if (img_role(0) == ROLE_DEPTH) return dnr(5); /* gv_state_why: the z mark pass (fallback) */
    const u32 lm = G_TEXTURE_GEN;
    const u32 bad = fogmix_on && !gv_fog_gpu_ok() ? G_FOG : 0;
    unsigned n = 0, gmi = GV_NONE;
    int gmi_ready = 0;
    for (unsigned k = 0; k < e->ngrp; k++) {
        const DlcGrp* g = &e->grp[k];
        if (dlc_box_out(g, m)) continue;
        const u32 gg = (entry & g->am) | g->om;
        if (!g->sd || !g->nbat || !g->bat_q) return dnr(6);
        if (gg & (G_LIGHTING | bad)) return dnr(7); /* lit groups run through the GPU lights path (S3N later row) */
        u32 x = gg;
        for (unsigned q = 0; q < g->ng; q++) {
            const u32* op = e->gop[g->goff + q];
            x = (x & op[0]) | op[1];
            if ((x & (G_LIGHTING | bad)) || (x & lm) != (gg & lm)) return dnr(7);
        }
        if (!pm_gpu_gidx_room(3u * g->sn) || !pm_gpu_gv_room(g->nvtx + 3u * g->ntri)) return dnr(8);
        if (!pm_gpu_grange_native_would(m, 3u * g->sn, g->sd)) { int w = pm_gpu_grange_why(m, 3u * g->sn, g->sd); return dnr(w ? 10 + w : 9); } /* S4-1: 11 gv or gpu off, 12 zmark, 13 decal and blend, 14 wide, 15 ring */ /* the walker's grange refuses: the group runs on the ring path (not a static draw) */
        if (!gmi_ready) { gmi = dlc_gmi(); gmi_ready = 1; }
        if (gmi == GV_NONE) return dnr(10);
        unsigned t = 0, go = 0;
        u32 geom_at = gg;
        for (unsigned b = 0; b < g->nbat; b++) {
            while (go < g->bat_q[b]) { const u32* op = e->gop[g->goff + go]; geom_at = (geom_at & op[0]) | op[1]; go++; }
            unsigned end = g->ends[b];
            if (end > t) {
                /* the walker's dlc_seg: G_CULL_BOTH drops the triangles (no draw), cull_off draws with no cull */
                const int off = cull_off();
                if (!off && (geom_at & G_CULL_BOTH) == G_CULL_BOTH) { t = end; continue; }
                if (n >= max) return -2;
                out[n].sd = g->sd + 3u * t; out[n].nidx = 3u * (end - t); out[n].geom = off ? (geom_at & ~G_CULL_BOTH) : geom_at; n++;
                t = end;
            }
        }
    }
    return (int)n;
}

int dlc_try(const Gfx* l) {
    if ((u32)l - (u32)gMapShapeData >= 0x40000u || ((u32)l & 7)) return 0;
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    DlcEnt* e = dlc_find(l);
    if (!e) return 0;
    int chk = dlcchk_on();
    if (e->gen != pm_dlc_gen) {
        dlc_free(e);
        e->gen = pm_dlc_gen; e->refused = 0;
        int r = dlc_build(e, l);
        e->hash = dlc_hash(l);
        dlc_c.compile++;
        if (r) { e->refused = (u8)r; dlc_c.refused[r < DLR_N ? r : DLR_LONG]++; }
    } else { dlc_c.hit++; }
    if (e->refused) return 0;
    if (chk && dlc_hash(l) != e->hash) { dlc_c.hashbad++; e->gen = 0; return 0; }
    const float(*m)[4] = combined;
    u32 entry = nd_ws.geom;
    unsigned gmi = GV_NONE;
    int gmi_ready = 0, skipped_last = 0, skipped_any = 0;
    dlc_c.lists++;
    dlc_c.opsrep += e->nops;
    if (e->ngrp) { __builtin_prefetch(e->grp); __builtin_prefetch(e->gop); }
    for (unsigned k = 0; k < e->ngrp; k++) {
        const DlcGrp* g = &e->grp[k];
        if (k + 1 < e->ngrp) { __builtin_prefetch(g + 1); __builtin_prefetch(g[1].sd); }
        dlc_c.grp++;
        if (pm_ph_on) sp0_grp++;
        if (skipdump_on() && pm_frame_count() >= 200 && pm_frame_count() < 206) dlc_grp_dump(g, m, k, dlc_box_out(g, m), (unsigned)g->op0);
        if (!nobox_on() && dlc_box_out(g, m)) {
            if (chk) dlc_verify_skip(l, g, m);
            if (noboxint_on()) {
                dlc_set_geom((entry & g->am) | g->om);
                dlc_depth++;
                pm_gbi_run((void*)(l + g->op0), (g->op1 - g->op0) * 8u);
                dlc_depth--;
                continue;
            }
            tri_since_vtx = g->tsv_exit;
            skipped_last = skipped_any = 1;
            dlc_c.skip++;
            if (pm_ph_on) sp0_skip++;
            continue;
        }
        skipped_last = 0;
        u32 gg = (entry & g->am) | g->om;
        int why = 0;
        int lit = 0; /* GV5: a lit group is replayed from the static area (the GPU lights it) or runs through the interpreter */
        if (!why) {
            u32 x = gg;
            lit = (x & G_LIGHTING) != 0;
            for (unsigned q = 0; q < g->ng; q++) { x = (x & e->gop[g->goff + q][0]) | e->gop[g->goff + q][1]; if (x & G_LIGHTING) lit = 1; }
            if (lit && !(g->sd && gv_lit_ok())) why = 2;
        }
        if (!why && !pm_gpu_gv_room(g->nvtx + 3u * g->ntri)) why = 3;
        if (!why) {
            if (lit) gmi = gv_matrix_id(1); /* the normal matrix is part of the entry */
            else if (!gmi_ready) { gmi = dlc_gmi(); gmi_ready = 1; }
            if (gmi == GV_NONE) why = 4;
        }
        dlc_set_geom(gg);
        if (!why && behroute_on() && dlc_behind(g, m)) why = 5;
        if (!why && g->sd && dlc_static_group(e, g, gmi, gg)) {
            dlc_c.replay++; dlc_c.srep++; dlc_c.tri += g->ntri;
            tri_since_vtx = g->tsv_exit;
            skipped_last = skipped_any = 1;
            continue;
        }
        if (!why && lit) why = 2; /* the static group was not taken */
        if (why) {
            if (why == 2) dlc_c.i_lit++; else if (why == 3) dlc_c.i_ring++; else if (why == 4) dlc_c.i_mat++;
            dlc_depth++;
            pm_gbi_run((void*)(l + g->op0), (g->op1 - g->op0) * 8u);
            dlc_depth--;
            continue;
        }
        dlc_c.replay++; dlc_c.tri += g->ntri;
        dlc_run_group(l, g, gmi);
    }
    dlc_set_geom((entry & e->exit_am) | e->exit_om);
    if (skipped_last) memset(vert_live, 0, sizeof vert_live);
    if (skipped_any) dlc_dirty = 1;
    if (chk) { /* the exit state against a plain walk of the list */
        u32 g2 = entry;
        int anyg = 0;
        for (unsigned i = 0; i < DLC_MAXOPS; i++) {
            u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
            if ((w0 >> 24) == G_ENDDL) break;
            if ((w0 >> 24) == G_GEOMETRYMODE) g2 = (g2 & (w0 & 0xffffffu)) | w1;
            if ((w0 >> 24) == G_VTX) anyg = 1;
        }
        if (g2 != nd_ws.geom) dlc_c.exitbad++;
        if (anyg && e->ngrp && tri_since_vtx != e->grp[e->ngrp - 1].tsv_exit) dlc_c.exitbad++;
    }
    return 1;
}

/* S6-2c-2, S6-2c-3: entity model lists from the static pools. The record G_PM_ENT with G_PM_ENT_DRAW (entity_model.c) sits right before the model G_DL. The list is compiled once per
 * key (list address, vertex segment base): nd_ent_segments cuts it at its state ops, nd_ent_strip leaves the vertex loads and triangles, nd_dlc_compile copies them into the static
 * vertex and index pools (the area of the map shape groups, reset with pm_dlc_gen). The draw runs the state ops of each segment in place on the walker (same words, same order), then
 * draws the segment triangles from the pool with dlc_seg: the walker state at the segment is the state of the draw, no second copy of it. A segment the static path refuses
 * (lit, fog without GPU fog, zmark, decal with blend, ring full) runs its old words (the vertex loads before it, then its own ops). Content (the list words and the vertex span of a
 * vertex array) is hashed once per frame and key; a change recompiles, 4 changes make the key dynamic (walker). entcheck: compile and compare, the walker draws the old words. */
typedef struct { u32 a0, op0, op1, tri0, ntri; } EntSeg;
typedef struct {
    u32 list, vb, nops, hash, fr, vlen, retry;
    const u8* vp;
    const u16* idx;
    const u8* vtx;
    unsigned vfirst, nvtx;
    EntSeg* seg;
    u16 nseg;
    u8 st, why, nrec;
    u32 ndraw, id, segid;
    u32 nint;
    u8 useseg; /* S SGEN-1: the list reads vertices through a segment address: its entries are keyed by the segment bases (ent_segkey) */
    Gfx* fw; /* the list with its nested G_DL lists inlined (S6-5), G_ENDDL last: the segment ops index it */
} EntKey;
enum { ENT_NEW, ENT_OK, ENT_REF, ENT_DYN };
#define ENT_TAB 1024
#define ENT_SEGMAX 192
EntKey* ent_tab[ENT_TAB];
unsigned ent_nent, ent_gen_seen;
NdEntSeg ent_sbuf[ENT_SEGMAX];
unsigned ent_opc[256];
EntCount ent_c;
GdlCount gdl_c; /* SGEN-1: the generic G_DL path */
unsigned ent_skipgen; /* the G_DL after a draw record that ent_draw refused: no second try */
static int entchk_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("entcheck"); return v; }
static const void* ent_env_vtx(uint32_t w1, unsigned n) {
    const Vtx* src = (const Vtx*)ptr_of(w1);
    if (!src || !is_host((u32)src) || n == 0 || n > 64) return NULL;
    return src;
}
/* S6-5: the words of the list and of every nested list (G_DL), then the vertex span */
static u32 ent_hash_walk(const Gfx* l, u32 h, int depth) {
    for (unsigned i = 0; i < ND_DLC_MAXOPS; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        h = (h ^ w0) * 16777619u; h = (h ^ w1) * 16777619u;
        if ((w0 >> 24) == G_ENDDL) return h;
        if ((w0 >> 24) == G_DL) {
            const Gfx* t = (const Gfx*)ptr_of(w1);
            if (t && is_host((u32)t) && depth < 5) h = ent_hash_walk(t, h, depth + 1);
            if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) return h;
        }
    }
    return h;
}
static u32 ent_hash(const Gfx* l, const u8* vp, u32 vlen) {
    u32 h = ent_hash_walk(l, 2166136261u, 0);
    for (u32 i = 0; i < vlen; i += 4u) { u32 v; memcpy(&v, vp + i, 4); h = (h ^ v) * 16777619u; }
    return h;
}
/* S6-5: the list with the nested lists inlined. 0: a nested target is not host memory, too deep or too long. */
Gfx ent_flat_buf[ND_DLC_MAXOPS + 1];
static int ent_flat(const Gfx* l, unsigned* n, int depth) {
    for (unsigned i = 0; i < ND_DLC_MAXOPS; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        unsigned op = w0 >> 24;
        if (op == G_ENDDL) return 1;
        if (op == G_DL) {
            const Gfx* t = (const Gfx*)ptr_of(w1);
            if (!t || !is_host((u32)t) || depth >= 5 || !ent_flat(t, n, depth + 1)) return 0;
            if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) return 1;
            continue;
        }
        if (*n >= ND_DLC_MAXOPS) return 0;
        ent_flat_buf[(*n)++] = l[i];
    }
    return 0;
}
/* the pool copies against the list: every triangle corner is the vertex the slot holds at the triangle (bytes 0 to 5 and 8 to 15; the flag word is the palette index). Per segment. */
static void ent_refcheck(const EntKey* k, const Gfx* l) {
    const u8* slot[64];
    u8 bad[ENT_SEGMAX];
    unsigned gt = 0, s = 0;
    memset(slot, 0, sizeof slot);
    memset(bad, 0, sizeof bad);
    for (u32 i = 0; i < k->nops; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        unsigned op = w0 >> 24;
        if (op == G_VTX) {
            unsigned n = (w0 >> 12) & 0xff, e = (w0 >> 1) & 0x7f, v0 = e >= n ? e - n : 0;
            const u8* src = (const u8*)ent_env_vtx(w1, n);
            if (v0 + n > 64) n = 64 - v0;
            for (unsigned t = 0; t < n; t++) slot[v0 + t] = src ? src + 16u * t : NULL;
        } else if (op == G_TRI1 || op == G_TRI2) {
            for (int t = 0; t < (op == G_TRI2 ? 2 : 1); t++, gt++) {
                u32 w = t ? w1 : w0;
                int ix[3] = { (int)((w >> 16) & 0xff) / 2, (int)((w >> 8) & 0xff) / 2, (int)(w & 0xff) / 2 };
                while (s + 1 < k->nseg && gt >= k->seg[s].tri0 + k->seg[s].ntri) s++;
                for (int c = 0; c < 3; c++) {
                    const u8* p = ix[c] < 64 ? slot[ix[c]] : NULL;
                    const u8* q = k->vtx + 16u * (k->idx[3u * gt + (unsigned)c] - k->vfirst);
                    if (!p || memcmp(p, q, 6) != 0 || memcmp(p + 8, q + 8, 8) != 0) bad[s] = 1;
                }
            }
        }
    }
    for (unsigned q = 0; q < k->nseg; q++) {
        if (!k->seg[q].ntri) continue;
        ent_c.cseg++;
        if (bad[q] || gt != k->seg[k->nseg - 1].tri0 + k->seg[k->nseg - 1].ntri) ent_c.cdiff++; else ent_c.ceq++;
    }
}
static void ent_compile(EntKey* k, const Gfx* l0) {
    unsigned ns = 0, nops = 0;
    u32 bad = 0, ntri = 0;
    const u8 *lo = NULL, *hi = NULL;
    NdObj o;
    free(k->seg); k->seg = NULL; k->nseg = 0;
    free(k->fw); k->fw = NULL;
    k->st = ENT_REF; k->why = 0; k->fr = pm_frame_count(); k->retry = k->fr + 300u;
    ent_c.comp++;
    if (!ent_flat(l0, &nops, 0)) { k->why = 5; return; }
    ent_flat_buf[nops].words.w0 = (u32)G_ENDDL << 24; ent_flat_buf[nops].words.w1 = 0;
    if (!k->vb) { for (unsigned i = 0; i < nops; i++) if ((ent_flat_buf[i].words.w0 >> 24) == G_VTX) { u32 a = ent_flat_buf[i].words.w1; if (!is_host(a) && (a >> 24) > 0 && (a >> 24) < 16u) { k->useseg = 1; break; } } if (k->useseg) { k->why = 9; return; } }
    k->fw = (Gfx*)malloc((nops + 1u) * sizeof(Gfx));
    if (!k->fw) { k->why = 5; return; }
    memcpy(k->fw, ent_flat_buf, (nops + 1u) * sizeof(Gfx));
    const Gfx* l = k->fw;
    const u32* w = (const u32*)l;
    int rc = nd_ent_segments(w, ent_sbuf, ENT_SEGMAX, &ns, &bad);
    if (rc != NDR_OK) { k->why = (u8)(rc + 1); if (rc == NDR_OP) ent_opc[bad & 0xffu]++; return; }
    if (ns == 0) { k->why = 8; return; }
    nd_ent_strip(w, dlc_words);
    for (unsigned s = 0; s < ns; s++) ntri += ent_sbuf[s].ntri;
    if (k->vb) for (unsigned i = 0; i < nops; i++) { /* the vertex span of the vertex array: hashed once per frame */
        if ((l[i].words.w0 >> 24) != G_VTX) continue;
        const u8* p = (const u8*)ptr_of(l[i].words.w1);
        unsigned n = (l[i].words.w0 >> 12) & 0xff;
        if (!p || !is_host((u32)p)) continue;
        if (!lo || p < lo) lo = p;
        if (!hi || p + 16u * n > hi) hi = p + 16u * n;
    }
    if (lo && (u32)(hi - lo) > 0x20000u) { k->why = 5; return; }
    NdDlcEnv env = { ent_env_vtx, nd_env_valloc, nd_env_ialloc };
    rc = nd_dlc_compile(&o, dlc_words, &env);
    if (rc != NDR_OK) { k->why = (u8)(rc + 1); return; }
    if (o.nidx != 3u * ntri) { nd_dlc_free(&o); k->why = 7; return; }
    k->seg = (EntSeg*)malloc(ns * sizeof(EntSeg));
    if (!k->seg) { nd_dlc_free(&o); k->why = 5; return; }
    for (unsigned s = 0; s < ns; s++) {
        k->seg[s].a0 = s ? ent_sbuf[s - 1].op1 : 0u; k->seg[s].op0 = ent_sbuf[s].op0; k->seg[s].op1 = ent_sbuf[s].op1;
        k->seg[s].tri0 = ent_sbuf[s].tri0; k->seg[s].ntri = ent_sbuf[s].ntri;
    }
    k->nseg = (u16)ns; k->nops = nops;
    k->idx = o.idx; k->vtx = o.vtx; k->vfirst = o.vfirst; k->nvtx = o.nvtx;
    if (o.nvtx) pm_gpu_gstat_commit(o.vfirst, o.nvtx);
    if (o.nidx) pm_gpu_gidxs_commit(o.idx, o.nidx);
    nd_dlc_free(&o); /* the groups and geometry ops; the pool copies stay */
    k->vp = lo; k->vlen = lo ? (u32)(hi - lo) & ~3u : 0u;
    k->hash = ent_hash(l0, k->vp, k->vlen);
    k->st = ENT_OK;
    if (entchk_on()) ent_refcheck(k, l);
}
static void ent_reset(void) {
    for (unsigned i = 0; i < ENT_TAB; i++) if (ent_tab[i]) { free(ent_tab[i]->seg); free(ent_tab[i]->fw); free(ent_tab[i]); ent_tab[i] = NULL; }
    ent_nent = 0; ent_gen_seen = pm_dlc_gen;
}
static EntKey* ent_lookup(const Gfx* l, u32 vb) {
    unsigned h = (((u32)l >> 3) * 2654435761u ^ vb * 40503u) >> 20 & (ENT_TAB - 1);
    for (unsigned n = 0; n < ENT_TAB; n++, h = (h + 1) & (ENT_TAB - 1)) {
        EntKey* k = ent_tab[h];
        if (!k) {
            if (ent_nent >= ENT_TAB * 3 / 4) return NULL;
            k = (EntKey*)calloc(1, sizeof *k);
            if (!k) return NULL;
            k->list = (u32)l; k->vb = vb; k->id = ent_nent;
            ent_tab[h] = k; ent_nent++;
            ent_compile(k, l);
            return k;
        }
        if (k->list == (u32)l && k->vb == vb) return k;
    }
    return NULL;
}
extern unsigned char pm_ent_decal_ok; /* pm_gpu.c: pm_gpu_grange takes a decal with blend (two native passes at the close) */
unsigned ent_sw[16], ent_gw[8]; /* ent_gw: pm_gpu_grange_why of a grange refusal (1 gv or gpu off, 2 zmark, 3 decal and blend, 4 wide, 5 ring) */ /* segments the static path refused, by reason: 1 lit, 2 fog, 3 index room, 4 matrix table, 5 + dlc_seg code (1 + gv state why, 9 grange) */
static __attribute__((noinline)) int ent_seg_native(const EntKey* k, const EntSeg* s) {
    extern unsigned char pm_ent_decal_ok;
    if (nd_ws.geom & G_LIGHTING) { ent_sw[1]++; return 0; }
    if (fogmix_on && (nd_ws.geom & G_FOG) && !gv_fog_gpu_ok()) { ent_sw[2]++; return 0; }
    if (!pm_gpu_gidx_room(3u * s->ntri)) { ent_sw[3]++; return 0; }
    pm_gpu_set_mode((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0);
    unsigned gmi = dlc_gmi();
    if (gmi == GV_NONE) { ent_sw[4]++; return 0; }
    pm_ent_decal_ok = 1;
    int r = dlc_seg(k->idx + 3u * s->tri0, s->ntri, gmi);
    pm_ent_decal_ok = 0;
    if (r) return 1;
    ent_sw[5 + (dbg_seg < 10 ? dbg_seg : 10)]++;
    if (dbg_seg == 9) { pm_ent_decal_ok = 1; ent_gw[pm_gpu_grange_why(gm_tab[gmi], 3u * s->ntri, k->idx + 3u * s->tri0) & 7]++; pm_ent_decal_ok = 0; }
    ((EntKey*)k)->nint++;
    return 0;
}
static void ent_log(void) {
    static unsigned next = 300u;
    unsigned f = pm_frame_count();
    extern void pm_log_prof_line(const char* fmt, ...);
    if (f < next) return;
    next = (f / 300u + 1u) * 300u;
    pm_log_prof_line("entnat f%u draws %u native %u fb %u segs native %u interp %u keys %u comp %u dyn %u", f, ent_c.draws, ent_c.nat, ent_c.fb, ent_c.segn, ent_c.segi, ent_nent, ent_c.comp, ent_c.dyn);
    pm_log_prof_line("entnat why 1 table %u 2 op %u 3 slot %u 4 vtx %u 5 long %u 6 pool %u 7 count %u 8 none %u", ent_c.why[1], ent_c.why[2], ent_c.why[3], ent_c.why[4], ent_c.why[5], ent_c.why[6], ent_c.why[7], ent_c.why[8]);
    for (unsigned q = 0; q < 256u; q++) if (ent_opc[q]) pm_log_prof_line("entnat refused op %02x keys %u", q, ent_opc[q]);
    for (unsigned q = 0, n = 0; q < ENT_TAB && n < 16u; q++) if (ent_tab[q] && (ent_tab[q]->nint || ent_tab[q]->st != ENT_OK)) { const EntKey* k = ent_tab[q]; n++; pm_log_prof_line("entnat key list %08x vb %08x st %u why %u nseg %u nops %u draws %u seg %u int %u", k->list, k->vb, k->st, k->why, k->nseg, k->nops, k->ndraw, k->segid, k->nint); }
    pm_log_prof_line("entnat grange why 0 %u 1 %u 2 %u 3 %u 4 %u 5 %u", ent_gw[0], ent_gw[1], ent_gw[2], ent_gw[3], ent_gw[4], ent_gw[5]);
    pm_log_prof_line("entnat segwhy lit %u fog %u room %u mtx %u | dlc_seg code (0 none, 1 + gv why, 9 grange) 0:%u 1:%u 2:%u 3:%u 4:%u 5:%u 6:%u 7:%u 8:%u 9:%u", ent_sw[1], ent_sw[2], ent_sw[3], ent_sw[4], ent_sw[5], ent_sw[6], ent_sw[7], ent_sw[8], ent_sw[9], ent_sw[10], ent_sw[11], ent_sw[12], ent_sw[13], ent_sw[14]);
    { extern void pm_gpu_nraw_why(unsigned* o); unsigned nw[10]; pm_gpu_nraw_why(nw); pm_log_prof_line("nrawwhy f%u cmdbuf %u off %u grab %u zmark %u nozt %u tex1 %u noise %u unit0 %u tmpl %u", f, nw[1], nw[2], nw[3], nw[4], nw[5], nw[6], nw[7], nw[8], nw[9]); }
    pm_log_prof_line("gdl f%u lists %u native %u dyn %u refuse %u skip %u draws native %u walker %u", f, gdl_c.lists, gdl_c.nat, gdl_c.dyn, gdl_c.ref, gdl_c.skip, ent_c.segn, ent_c.segi);
    if (entchk_on()) pm_log_prof_line("entsegcmp f%u segs %u eq %u diff %u", f, ent_c.cseg, ent_c.ceq, ent_c.cdiff);
}
/* 1: the list is drawn (the caller skips the G_DL after the record); 0: the walker runs the old words */
static int ent_run(const Gfx* l, EntKey* k, u32 segid);
static EntKey* ent_find(const Gfx* l, u32 vb) {
    unsigned h = (((u32)l >> 3) * 2654435761u ^ vb * 40503u) >> 20 & (ENT_TAB - 1);
    for (unsigned n = 0; n < ENT_TAB; n++, h = (h + 1) & (ENT_TAB - 1)) {
        EntKey* k = ent_tab[h];
        if (!k) return NULL;
        if (k->list == (u32)l && k->vb == vb) return k;
    }
    return NULL;
}
static u32 ent_segkey(void) { u32 h = 2166136261u; for (unsigned i = 1; i < 16; i++) h = (h ^ nd_ws.segbase[i]) * 16777619u; return h | 1u; }
/* SGEN-1: every static list the walker meets (G_DL arm, no record). Lists in the display contexts (the frame heap) are dynamic by address: walker. 1: drawn. */
int ent_gdl(const Gfx* l) {
    if (!dlc_on || ((u32)l & 7u) || (u32)l - (u32)D_80164000 < 2u * sizeof(DisplayContext)) { gdl_c.skip++; return 0; }
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    if (ent_gen_seen != pm_dlc_gen) ent_reset();
    gdl_c.lists++;
    EntKey* k = ent_find(l, 0);
    if (!k) {
        if (ent_nent >= ENT_TAB * 3 / 4) { gdl_c.ref++; return 0; }
        k = (EntKey*)calloc(1, sizeof *k);
        if (!k) return 0;
        unsigned h = (((u32)l >> 3) * 2654435761u) >> 20 & (ENT_TAB - 1);
        while (ent_tab[h]) h = (h + 1) & (ENT_TAB - 1);
        k->list = (u32)l; k->vb = 0; k->id = ent_nent; ent_tab[h] = k; ent_nent++;
        ent_compile(k, l);
    }
    if (k->useseg) { /* the vertices sit behind segment addresses: the entry of these segment bases */
        u32 vb = ent_segkey();
        k = ent_find(l, vb);
        if (!k) {
            if (ent_nent >= ENT_TAB * 3 / 4) { gdl_c.ref++; return 0; }
            k = ent_lookup(l, vb);
            if (!k) { gdl_c.ref++; return 0; }
        }
    }
    if (k->st == ENT_DYN) { gdl_c.dyn++; return 0; }
    if (k->st == ENT_REF && k->why != 6) { gdl_c.ref++; return 0; }
    int r = ent_run(l, k, 0);
    if (r) gdl_c.nat++; else if (k->st == ENT_DYN) gdl_c.dyn++; else gdl_c.ref++;
    return r;
}
int ent_draw(const Gfx* rec) {
    const Gfx* l = (const Gfx*)ptr_of(rec[0].words.w1);
    const Gfx* nx = rec + G_PM_ENT_WORDS;
    if (!l || !dlc_on || !is_host((u32)l) || ((u32)l & 7u) || (nx->words.w0 >> 24) != G_DL || nx->words.w1 != rec[0].words.w1) return 0;
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    if (ent_gen_seen != pm_dlc_gen) ent_reset();
    ent_c.draws++;
    EntKey* k = ent_lookup(l, rec[1].words.w1);
    return ent_run(l, k, rec[1].words.w0 | (rec[2].words.w0 << 8));
}
static int ent_run(const Gfx* l, EntKey* k, u32 segid) {
    const unsigned f = pm_frame_count();
    if (!k) { ent_c.why[1]++; ent_c.fb++; ent_log(); return 0; }
    if (k->st == ENT_OK && k->fr != f) { /* content, once per frame */
        k->fr = f;
        if (ent_hash(l, k->vp, k->vlen) != k->hash) {
            if (++k->nrec > 3) { k->st = ENT_DYN; ent_c.dyn++; }
            else { u8 nr = k->nrec; ent_compile(k, l); k->nrec = nr; }
        }
    } else if (k->st == ENT_REF && k->why == 6 && f >= k->retry) { ent_compile(k, l); }
    k->ndraw++; k->segid = segid;
    { static unsigned msk = 0xffffffffu; if (msk == 0xffffffffu) { char b[16]; msk = pm_debug_value("entskip", b, sizeof b) ? (unsigned)strtoul(b, NULL, 16) : 0u; } if (k->id < 32u && (msk >> k->id & 1u)) return 0; } /* debug switch entskip=<hex mask of key ids>: those keys run the walker (bisect aid) */
    if (k->st != ENT_OK) { ent_c.fb++; if (k->st == ENT_REF) ent_c.why[k->why < 12 ? k->why : 0]++; ent_log(); return 0; }
    if (entchk_on()) { ent_c.nat++; ent_log(); return 0; }
    for (unsigned s = 0; s < k->nseg; s++) {
        const EntSeg* e = &k->seg[s];
        const Gfx* fw = k->fw;
        if (e->op0 > e->a0) pm_gbi_run((void*)(fw + e->a0), (e->op0 - e->a0) * 8u);
        if (!e->ntri) continue; /* a state-only or vertex-only segment: the loads replay when a later segment needs them */
        if (e->ntri && ent_seg_native(k, e)) { ent_c.segn++; continue; }
        ent_c.segi++; /* the old words: the vertex loads before the segment, then its own ops */
        for (u32 i = 0; i < e->op0; i++) if ((fw[i].words.w0 >> 24) == G_VTX) pm_gbi_run((void*)(fw + i), 8u);
        if (e->op1 > e->op0) pm_gbi_run((void*)(fw + e->op0), (e->op1 - e->op0) * 8u);
    }
    { const EntSeg* e = &k->seg[k->nseg - 1]; if (k->nops > e->op1) pm_gbi_run((void*)(k->fw + e->op1), (k->nops - e->op1) * 8u); }
    tri_since_vtx = 1; memset(vert_live, 0, sizeof vert_live); dlc_dirty = 1; /* as the skipped groups of dlc_try */
    ent_c.nat++;
    ent_log();
    return 1;
}

__attribute__((cold, noinline)) void dlc_report(void) {
    pm_log_hw("dlc2 f%d tri %u opsrep %u | hashbad %u exitbad %u missing %u slotfix %u ringfail %u ents %u | static grp %u tri %u fail %u full %u batch %u kb %u skip %u grp %u xgrp %u", rm_frame, dlc_c.tri, dlc_c.opsrep, dlc_c.hashbad, dlc_c.exitbad, dlc_c.missing, dlc_c.slotfix, dlc_c.ringfail, dlc_nent, dlc_c.srep, dlc_c.stri, dlc_c.sfail, dlc_c.sfull, dlc_c.batch, pm_gpu_gstat_used() / 64u, dlc_c.skip, dlc_c.grp, dlc_c.xgrp);
    memset(&dlc_c, 0, sizeof dlc_c);
}
static __attribute__((cold, noinline)) void tri_dd_log(const ClipV* poly, int n) {
    int i;
    if (!dd.ntri) {
        dd.oml = nd_ws.omode_l; dd.omh = nd_ws.omode_h; dd.zb = (nd_ws.geom & G_ZBUFFER) != 0; dd.tex = (unsigned)nd_ws.tex_on; dd.timg = (u32)nd_ws.td[0].src; dd.fmt = nd_ws.td[0].fmt;
        dd.siz = nd_ws.td[0].siz; dd.w = nd_ws.td[0].w; dd.h = nd_ws.td[0].h; dd.prim = nd_ws.prim;
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
    if (lo != nd_ws.omode_l || lg != (nd_ws.geom & G_ZBUFFER) || la != nd_ws.tst1.comb_a || lb != nd_ws.tst1.comb_b || lc != nd_ws.tst1.comb_c || ld != nd_ws.tst1.comb_d ||
        lh != nd_ws.omode_h || lp != nd_ws.prim || lf != nd_ws.tst1.tile_fmt[0] * 4 + nd_ws.tst1.tile_siz[0]) {
        lp = nd_ws.prim; lf = nd_ws.tst1.tile_fmt[0] * 4 + nd_ws.tst1.tile_siz[0];
        lo = nd_ws.omode_l; lg = nd_ws.geom & G_ZBUFFER; la = nd_ws.tst1.comb_a; lb = nd_ws.tst1.comb_b; lc = nd_ws.tst1.comb_c; ld = nd_ws.tst1.comb_d; lh = nd_ws.omode_h;
    }
}

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
        if ((nd_ws.geom & G_CULL_BOTH) && !cull_off()) {
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
    if (!early && (nd_ws.geom & G_CULL_BOTH) && !cull_off()) {
        /* V02: the RSP culls by the sign of the screen space area (G_CULL_FRONT / G_CULL_BACK / both), same test as
         * libultraship. A one sided wall (the cliff skirt around the Mario house yard, seen from inside) draws only
         * if this test runs. nocull turns it off for A/B checks. */
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
    if ((rmlog_on() && (rm_frame % 30) == 1) || xt_on()) tri_rm_log();
    if (comb_mode5 && !plan_active) {
        float en[4];
        unpack8(nd_ws.envc, en);
        for (i = 0; i < n; i++) { poly[i].r = en[0] * 255.f; poly[i].g = en[1] * 255.f; poly[i].b = en[2] * 255.f; poly[i].a = en[3] * 255.f; }
    }
    if (dd_skip()) return;
    sync_render();
    if (dd_on()) tri_dd_log(poly, n);
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

/* G1: ops with an empty handler (sync and no-op). The loop skips them before the prologue. G_NOOP is not here: the HANG1a splash marks are G_NOOP words. */


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
void gbi_cold_dd_load(const Vtx* src) {
    dd_end(); dd.idx = dd_idx++; dd.top = dd_top; dd.src = (u32)src; dd.vc = __builtin_bswap32(*(const u32*)((const u8*)src + 12)); dd.gm = nd_ws.geom; dd.dl0 = dd_dl0; dd.dl1 = dd_dl1;
}
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
