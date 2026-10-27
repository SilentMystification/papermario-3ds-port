#include <3ds.h>
#include <3ds/gpu/gpu.h>
#include <3ds/gpu/registers.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include "pm_shader5_shbin.h"
#include "pm_shaderall_shbin.h"
#include "pm_gpu.h"
#include "pm_gbi.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "pm_pdl.h"

typedef struct {
    float x, y, z, w;
    float u, v;
    float r, g, b, a;
} Vert;

enum { VERT_SLOTS = 32768 };
/* CONFIG2 stores bytes-per-vertex in 8 bits, and that value has to equal the
 * attribute size or the PICA freezes. 4+2+4 floats, no padding. */
_Static_assert(sizeof(Vert) == (4 + 2 + 4) * sizeof(float), "vertex stride");
static Vert* vert_base;
static Vert* verts;
static int nverts;
static int rect_2d;
static int drew;
static int gpu_ok;
/* 3D writes the depth buffer. A later texrect has to win even if the test stays on. */
static PrintConsole bottom_con;
static int z_on;
static int light_on;
/* RDP render mode: depth compare, depth update, alpha blend, alpha cut-off (-1 none), decal. */
static int r_zcmp = 1, r_zupd = 1, r_blend = 1, r_atest = -1, r_decal;
static int combine;
int pm_debug_value(const char* key, char* out, unsigned n);
static int dview = -1; /* V02 depth view mode, see dv_color */
static unsigned dv_draw;
static int rect_z; /* X01: the 2D rectangle in flight tests depth (fill rect with G_ZS_PRIM) */
static int r_zmark, stencil_on; /* B3b: case 6 model, see flush */
static float r_zmark_zn; /* B3b: the constant clip depth (z / w) the model writes */
static int r_nocol; /* S25: the blender gives back the frame buffer colour: depth only */
static PmPlan plan; /* S25: combine mode 8 */
static int env_hi = 1; /* texenv stages configured so far (stage 0 is always reset) */
static unsigned env_col = 0xffffffffu; /* combine mode 5: the ENV colour */
static unsigned cconst;
static int rgb_scale = 1;
static int batch_identity;
static int draw_3d;
static unsigned prim = 0xffffffffu;
static float game_mvp[4][4];
static float vp_sx = 160.f, vp_sy = 120.f, vp_tx = 160.f, vp_ty = 120.f;
static C3D_Mtx shader_3d;
/* N64 scissor, y down. The 400-wide target is the 320 game plus 40px bars. */
static int sci_x0, sci_y0, sci_x1 = 320, sci_y1 = 240;
static C3D_Mtx projection;
static int u_proj = -1, u_uvx = -1, u_nzo = -1;
/* PT07: the RDP NOISE input. A 64 x 64 L8 texture of random bytes (fixed seed) on unit 2. The vertex shader gives it the screen position
 * (one texel per pixel) plus a random offset per drawn frame, so every pixel reads a new random value every frame. */
static C3D_Tex noise_tex;
static int noise_ready; /* 1 ready, -1 failed */
static float noise_off[2];
static unsigned noise_lcg = 0x2545f491u;
static unsigned noise_next(void) { noise_lcg = noise_lcg * 1664525u + 1013904223u; return noise_lcg >> 8; }
static int noise_init(void) {
    if (noise_ready) return noise_ready > 0;
    if (!C3D_TexInit(&noise_tex, 64, 64, GPU_L8)) { pm_log_e("PT07: no memory for the noise texture, NOISE stays 0"); noise_ready = -1; return 0; }
    {   /* random bytes: any order of independent random bytes is the same picture, so the tiled layout needs no swizzle */
        unsigned char* d = (unsigned char*)noise_tex.data;
        for (int i = 0; i < 64 * 64; i++) d[i] = (unsigned char)(noise_next() & 255u);
        GSPGPU_FlushDataCache(d, 64 * 64);
    }
    C3D_TexSetFilter(&noise_tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&noise_tex, GPU_REPEAT, GPU_REPEAT);
    noise_ready = 1;
    return 1;
}
static float t1_s[4] = { 1.f, 1.f, 0.f, 0.f };
static int aux_bound;
static C3D_RenderTarget* target;
/* H02 GPU limit stats, maxima since the last `mem` log line. Verts and command buffer overflow drop work without a message. */
static unsigned draws_frame, draws_max, verts_max, vert_drops, frames_seen;
static float cmd_max;
static shaderProgram_s prog;
static DVLB_s* dvlb;
/* PO3b (switch gpuvtx): raw Vtx path. A ring of the 16 byte Vtx records of this frame and a u16 index ring. A batch is the triangles that share one
 * matrix and one uv map. Program prog4 (matrix palette) or prog5 reads the Vtx as they are. The batch is drawn by flush() when gni > 0. */
enum { GRING_VTX = 16384, GRING_IDX = 65536, GSTAT_VTX = 16384 }; /* DLC2: the static vertex area follows the ring in the same buffer (indices GRING_VTX and up) */
static C3D_AttrInfo attr_cpu;
static unsigned char* gring; /* GRING_VTX x 16 bytes, linear */
static unsigned short* gidx; /* GRING_IDX, linear */
static unsigned gstat_n; /* DLC2: static vertices in use */
static unsigned gring_n, gidx_n, gi_start, gni; /* ring fill, index fill, first index of the open batch, indices in the open batch */
static float g_uvso[4], g_vp[4], g_cmul[4], g_cadd[4], g_zmix[2];
static int gpu_vtx_on;
static int g_ct, g_cullm; /* GV1a: the N64 cull type of the open raw batch (0 none, 1 G_CULL_FRONT, 2 G_CULL_BACK) and the PICA mode made from it */
/* GV1c: matrix palette. A raw batch (switch gv1) holds up to GPAL game matrices. A Vtx copy in the ring carries its palette index in the flag field (bytes 6 and 7).
 * The vertex shader pm4 reads pal[4 * index]. An entry is the post matrix (OrthoTilt, viewport) times the game matrix. */
enum { GPAL = 21 };
static shaderProgram_s prog4;
static C3D_AttrInfo attr4;
/* GV5: program pm5 (lighting, texgen, fog). A batch with g_use5 holds up to GPAL5 matrices, 7 registers each (4 position rows, 3 normal rows). */
enum { GPAL5 = 8 };
static shaderProgram_s prog5;
static DVLB_s* dvlb5;
static int u5_pal = -1, u5_lamb = -1, u5_lcol = -1, u5_ldir = -1, u5_lk0 = -1, u5_lk1 = -1, u5_lflag = -1, u5_fogu = -1, u5_uvso = -1, u5_cmul = -1, u5_cadd = -1, u5_nzo = -1, u5_zmix = -1, gv5_on, g_use5;
static PmFx g_fx, sh5_fx;
static float sh5_uvso[4], sh5_cmul[4], sh5_cadd[4], sh5_zmix[2];
static int sh5_ok;
static unsigned sh5_pal_gm[GPAL5], sh5_pal_gen[GPAL5]; static unsigned char sh5_pal_ok[GPAL5];
static const float (*pal_ntab)[3][4];
void pm_gpu_set_ntab(const float (*ntab)[3][4]) { pal_ntab = ntab; }
int pm_gpu_gv5_ok(void) { return gv5_on; }
/* RAW1: lean binding. bound = the program citro3d has (0 unknown, 1 cpu, 3 pm4, 4 pm5), buf_cur = the buffer info (0 unknown, 1 CPU pool, 2 raw ring). shadows of the pm4 uniforms
 * and of the palette entries skip a set that holds the value already. They restart at every frame begin. */
static int bound, buf_cur, raw_dvlb_ok; static DVLB_s* dvlb_all;
static float sh_uvso[4], sh_cmul[4], sh_cadd[4], sh_zmix[2]; static int sh_ok;
static unsigned sh_pal_gm[32], sh_pal_gen[32]; static unsigned char sh_pal_ok[32];
static void raw1_reset(void) { bound = 0; buf_cur = 0; sh_ok = 0; memset(sh_pal_ok, 0, sizeof sh_pal_ok); sh5_ok = 0; memset(sh5_pal_ok, 0, sizeof sh5_pal_ok); }
static int u4_pal = -1, u4_uvx = -1, u4_uvso = -1, u4_cmul = -1, u4_cadd = -1, u4_zmix = -1, u4_nzo = -1, g_pal, gv4_on;
static unsigned pal_n, pal_g[GPAL], pal_gen = 1, post_gen = 1;
static unsigned* rgen;          /* per ring vertex: the palette generation in which its flag was set (0 = never) */
static unsigned char* rk;       /* per ring vertex: the flag value */
static C3D_Mtx pcomp[256];      /* palette entry per game matrix index: post x game */
static unsigned pcomp_gen[256];
static const float (*pal_tab)[4][4];
static unsigned gv_recopy_n, gv_palup_n, gv_palmax, gv_palflush;
static C3D_Mtx gv_post;
static int gv_wind = 1;   /* GV1a: sign of the 3 x 3 determinant (x y w) of the matrix after the game matrix: +1 keeps the orientation of N64 clip space */

int pm_wide_cfg; /* F07-5: the user setting. pm_wide is 0 in 4:3 game modes (logos, title, file select). */
int pm_wide; /* F07-1: 1 = the 320 px game picture is stretched over the 400 px screen (X scale 1.25). Read once in pm_gpu_init. */
static int ui_ortho; /* F07-3: the game matrix is orthographic (UI): drawn 1:1 and centered with wide, not stretched */
static int off_active; /* V05: drawing goes to an off-screen image (see pm_gpu_off_begin) */
static C3D_Mtx off_proj;

static void ident(float m[4][4]) {
    memset(m, 0, sizeof(float) * 16);
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
}

/* F3DEX stores a row-vector multiply. The shader does dp4(row, v), which is
 * the transpose. Viewport then maps N64 clip/w into upright pixels, and
 * OrthoTilt turns those pixels into the rotated framebuffer. */
static void rebuild_3d(void) {
    C3D_Mtx game, view, tmp;
    int r;
    for (r = 0; r < 4; r++) {
        game.r[r].x = game_mvp[0][r];
        game.r[r].y = game_mvp[1][r];
        game.r[r].z = game_mvp[2][r];
        game.r[r].w = game_mvp[3][r];
    }
    Mtx_Zeros(&view);
    /* V05: an off-screen pass is not rotated and has no 40 pixel margin: GPU y is the game y (y runs down) */
    view.r[0].x = (pm_wide && !off_active && !ui_ortho) ? vp_sx * 1.25f : vp_sx;
    view.r[0].w = off_active ? vp_tx : (pm_wide && !ui_ortho) ? vp_tx * 1.25f : 40.f + vp_tx;
    view.r[1].y = off_active ? -vp_sy : vp_sy;
    view.r[1].w = off_active ? vp_ty : 240.f - vp_ty;
    /* Clip z/w is -1 at the near plane and +1 at the far plane. Citro's
     * ortho then wants near at -1 and far at 0, which is the value GEQUAL
     * keeps. The opposite sign stores the far wall as the nearer depth, so
     * the house covers the yard. */
    view.r[2].z = 0.5f;
    view.r[2].w = 0.5f;
    view.r[3].w = 1.f;
    Mtx_Multiply(&tmp, &view, &game);
    Mtx_Multiply(&shader_3d, off_active ? &off_proj : &projection, &tmp);
    {   /* GV1a: orientation of the map from N64 clip (x y w) to PICA clip. The game matrix is applied before, so it does not count. */
        C3D_Mtx* post = &gv_post;
        Mtx_Multiply(post, off_active ? &off_proj : &projection, &view);
        const C3D_FVec *a = &post->r[0], *b = &post->r[1], *c = &post->r[3];
        float det = a->x * (b->y * c->w - b->w * c->y) - a->y * (b->x * c->w - b->w * c->x) + a->w * (b->x * c->y - b->y * c->x);
        gv_wind = det < 0.f ? -1 : 1;
        post_gen++;
    }
}

/* The PICA constant colour register is 0xAABBGGRR. The port keeps colours as 0xRRGGBBAA. */
static unsigned abgr(unsigned c) {
    return ((c & 255u) << 24) | (((c >> 8) & 255u) << 16) | (((c >> 16) & 255u) << 8) | (c >> 24);
}

static GPU_TEVSRC tev_src(int s) {
    switch (s) {
    case PM_S_TEX0: return GPU_TEXTURE0;
    case PM_S_TEX1: return GPU_TEXTURE1;
    case PM_S_TEX2: return GPU_TEXTURE2;
    case PM_S_VTX: return GPU_PRIMARY_COLOR;
    case PM_S_CONST: return GPU_CONSTANT;
    case PM_S_PREV: return GPU_PREVIOUS;
    default: return GPU_PREVIOUS_BUFFER;
    }
}
static GPU_TEVOP_RGB tev_op_rgb(int o) {
    return o == PM_O_C ? GPU_TEVOP_RGB_SRC_COLOR : o == PM_O_1MC ? GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR :
           o == PM_O_A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA;
}
static GPU_TEVOP_A tev_op_a(int o) { return (o == PM_O_1MA || o == PM_O_1MC) ? GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA : GPU_TEVOP_A_SRC_ALPHA; }
/* FX2a: an 8 bit slot is one channel, I, in R. A blended write leaves the alpha channel as the blend weight, so a read takes I from R, never from A. */
static int tev_i8(int src, int i8_0, int i8_1) { return (src == PM_S_TEX0 && i8_0) || (src == PM_S_TEX1 && i8_1); }
static GPU_TEVOP_RGB tev_op_rgb_i(int o, int i8) {
    if (i8 && (o == PM_O_A || o == PM_O_1MA)) return o == PM_O_A ? GPU_TEVOP_RGB_SRC_R : GPU_TEVOP_RGB_ONE_MINUS_SRC_R;
    return tev_op_rgb(o);
}
static GPU_TEVOP_A tev_op_a_i(int o, int i8) {
    if (i8) return (o == PM_O_1MA || o == PM_O_1MC) ? GPU_TEVOP_A_ONE_MINUS_SRC_R : GPU_TEVOP_A_SRC_R;
    return tev_op_a(o);
}
static GPU_COMBINEFUNC tev_func(int f) {
    switch (f) {
    case PM_F_REPLACE: return GPU_REPLACE;
    case PM_F_MODULATE: return GPU_MODULATE;
    case PM_F_ADD: return GPU_ADD;
    case PM_F_SUBTRACT: return GPU_SUBTRACT;
    case PM_F_INTERP: return GPU_INTERPOLATE;
    case PM_F_ADDSIGNED: return GPU_ADD_SIGNED;
    default: return GPU_MULTIPLY_ADD;
    }
}

/* V05: an off-screen colour image. draw_box renders the contents of a rotating window (pause pages, file select panels)
 * into a game buffer with the color image set to it, then draws that buffer as strips through the rotation matrix.
 * Such a pass renders into a PICA render target that is a texture (VRAM), and pm_tex samples it for the strips. */
enum { OFF_N = 4, OFF_P = 2 * OFF_N, OFF_ALL = OFF_P + 2 }; /* FX2a: slots OFF_P and OFF_P + 1 are the 8 bit images (flame), they keep their pixels from frame to frame like the N64 memory */ /* V05b: slots 0..3 are full size, 4..7 are 64 x 64. A slot keeps its size for good: a render target is never deleted in a frame. */
/* X02: w x h is the size of the slot (64 x 64 for a game image up to 64 wide, else PM_OFF_W x PM_OFF_H). siz is the game pixel size
 * (3 = 32 bit, 2 = 16 bit, 1 = 8 bit). The same memory can be a 32 bit image and an 8 bit image in one frame: two slots. */
static struct { C3D_Tex tex; C3D_RenderTarget* rt; const void* base; int w, h, siz; unsigned stride; unsigned last; C3D_Tex snap; int snap_ready; } offs[OFF_ALL];
static int off_used, off_sused, off_cur, off_fail, off_w = PM_OFF_W, off_h = PM_OFF_H, off_i8;
extern unsigned pm_frame_count(void);
static int r_mix; static unsigned r_mixcol; /* V06: see pm_gpu_set_mix */
static int r_replace; /* off-screen fill: the pixel is written, not blended */
static int r_memblend; /* FX2a: the blender mixes with the memory colour by the memory alpha (G_BL_A_MEM) */
/* B2: T2 grab state. grab_tex holds a copy of the main target strips (same tiled layout, 256 x 512). grab_bind: flush binds it
 * instead of the pm_tex texture. A grab copies the full width. grab_dirty: a draw of unknown rows went to the main target since the
 * last grab. [grab_dy0, grab_dy1) are the rows that texture rectangles drew since then (R13c: motion_blur_flame loads one strip of
 * rows per rectangle, the strips do not overlap, so one copy serves them). */
enum { GRAB_MAX = 8 };
static C3D_Tex grab_tex;
static int grab_ready, grab_bind, grab_dirty = 1, grab_dy0, grab_dy1, grab_n, grab_split, rect_y0, rect_y1;

static void sh_clear(void);
static int want_clear; /* see below */
/* GV0 and KICK1: the GPU reads the vertex pool, the raw Vtx ring and the index ring when it runs the command list. Everything written since the last
 * flush goes out of the data cache before every C3D_FrameSplit and at the end of the frame (GSPGPU_FlushDataCache is one call to the GSP, so each
 * range is flushed once). v_fl, gring_fl and gidx_fl mark what is flushed. They restart at pm_gpu_begin. */
static Vert* v_fl;
static unsigned gring_fl, gidx_fl;
static void gpu_flush_written(void) {
    pm_tex_flush_pend_only();
    if (verts && v_fl && verts > v_fl) {
        PH_PUSH(PH_GCACHE);
        GSPGPU_FlushDataCache(v_fl, (u32)((uintptr_t)verts - (uintptr_t)v_fl)); PH_ADD(C_FLCALL, 1); PH_ADD(C_FLKB, (unsigned)((uintptr_t)verts - (uintptr_t)v_fl) >> 10);
        PH_POP();
        v_fl = verts;
    }
    if (gring && gring_n > gring_fl) { GSPGPU_FlushDataCache(gring + gring_fl * 16, (gring_n - gring_fl) * 16); PH_ADD(C_FLCALL, 1); PH_ADD(C_FLKB, (gring_n - gring_fl) * 16 >> 10); gring_fl = gring_n; }
    if (gidx && gidx_n > gidx_fl) { GSPGPU_FlushDataCache(gidx + gidx_fl, (gidx_n - gidx_fl) * 2); PH_ADD(C_FLCALL, 1); PH_ADD(C_FLKB, (gidx_n - gidx_fl) * 2 >> 10); gidx_fl = gidx_n; }
}
/* B2: queue order is clear of the main target, then every part. The first split of a frame queues the clear first. */
static void split_frame(void) {
    gpu_flush_written(); /* GV0b: the GPU runs the first part now, its vertices must be in memory */
    if (!grab_split++ && want_clear) {
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0);
        want_clear = 0;
    }
    sh_clear();
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
}

/* FX2a: the RDP loads a tile into TMEM before it draws, so a pass that reads the image it writes (the flame blur) sees the pixels from before
 * the pass. The PICA samples the render target itself with no such copy. Copy the slot before the batch and sample the copy. */
static void off_snapshot(int i) {
    if (!offs[i].snap_ready) {
        if (!C3D_TexInitVRAM(&offs[i].snap, offs[i].w, offs[i].h, GPU_RGBA8)) { pm_log_e("FX2a: no VRAM for the copy of slot %d", i); return; }
        C3D_TexSetFilter(&offs[i].snap, GPU_LINEAR, GPU_NEAREST);
        C3D_TexSetWrap(&offs[i].snap, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
        offs[i].snap_ready = 1;
    }
    split_frame();
    u32 bytes = (u32)offs[i].w * (u32)offs[i].h * 4u;
    GX_TextureCopy((u32*)offs[i].tex.data, GX_BUFFER_DIM(2048 / 16, 0), (u32*)offs[i].snap.data, GX_BUFFER_DIM(2048 / 16, 0), bytes, BIT(3));
}

/* PG0 (switch gbiprof): flush count by caller. The macro below records the function and line of the call. */
static const char* fl_fn = "?";
static int fl_line;
typedef struct { const char* fn; int line; unsigned n, e; } FlCause; /* n = flush calls, e = calls that found an empty batch */
static FlCause fl_tab[48];
static int fl_nt;
static int fl_prev_rect; static unsigned fl_rects, fl_consec; /* PG0: texture rectangles, and those that follow another with no other flush call between */
static int fl_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("gbiprof"); return v; }
extern unsigned pm_frame_count(void);
/* Debug tool, switch flushlog=<frame>: one log line per flush call of that frame (function, line, raw batch triangles, CPU vertices): finds what ends a batch. */
static void fl_trace(int empty) {
    static int fr = -2;
    char v[16];
    if (fr == -2) fr = pm_debug_value("flushlog", v, sizeof v) ? atoi(v) : -1;
    if (fr >= 0 && (int)pm_frame_count() == fr && !empty) pm_log("flushlog f%d %s line %d gni %u nverts %d", fr, fl_fn, fl_line, gni, nverts);
}
static void fl_count(int empty) {
    fl_trace(empty);
    for (int i = 0; i < fl_nt; i++) if (fl_tab[i].fn == fl_fn && fl_tab[i].line == fl_line) { fl_tab[i].n++; fl_tab[i].e += empty; return; }
    if (fl_nt < 48) { fl_tab[fl_nt].fn = fl_fn; fl_tab[fl_nt].line = fl_line; fl_tab[fl_nt].n = 1; fl_tab[fl_nt].e = empty; fl_nt++; }
}
void pm_gpu_gbiprof_report(unsigned frames) { /* the six most frequent causes since the last report, per frame in 0.01 */
    unsigned tn = 0, te = 0;
    for (int i = 0; i < fl_nt; i++) { tn += fl_tab[i].n; te += fl_tab[i].e; }
    pm_log("gbiprof flush total calls per frame x100 %u, empty batch x100 %u, drawn x100 %u", tn * 100 / (frames ? frames : 1), te * 100 / (frames ? frames : 1), (tn - te) * 100 / (frames ? frames : 1));
    for (int r = 0; r < 24; r++) {
        int best = -1; unsigned bn = 0;
        for (int i = 0; i < fl_nt; i++) if (fl_tab[i].n > bn) { bn = fl_tab[i].n; best = i; }
        if (best < 0) break;
        pm_log("gbiprof flush %s line %d calls per frame x100 %u empty x100 %u", fl_tab[best].fn, fl_tab[best].line, bn * 100 / (frames ? frames : 1), fl_tab[best].e * 100 / (frames ? frames : 1));
        fl_tab[best].n = 0;
    }
    pm_log("gbiprof texrect per frame x100: rects %u consecutive (no other flush between) %u", fl_rects * 100 / (frames ? frames : 1), fl_consec * 100 / (frames ? frames : 1));
    fl_rects = fl_consec = 0;
    for (int i = 0; i < fl_nt; i++) fl_tab[i].n = fl_tab[i].e = 0;
}
/* V19: decal depth bias, switch decalbias=<float> (default 0.001 as before; 0.00003 is about 2 float24 steps at mid range, to test on the device) */
static float decal_bias(void) {
    static float dbias = -1.f;
    if (dbias < 0.f) { char dv[24]; dbias = pm_debug_value("decalbias", dv, sizeof dv) ? (float)atof(dv) : 0.001f; }
    return dbias;
}
/* V16b: an N64 decal (Z_CMP, ZMODE_DEC) passes only where the stored depth is close to the depth of the decal. GPU_GEQUAL with a bias passes everywhere the decal is
 * nearer, so an overlay on a sprite also landed on the wall behind it (Thunder Rage on Shy Guys: a flickering rectangle). Two passes with the stencil: pass 1 marks the
 * pixels where the stored depth is not farther than decal - DECAL_EPS, pass 2 draws where the mark is set and the usual test (decal + bias is not behind) passes. */
#define DECAL_EPS 0.003f
typedef struct { int combine, tex, tex1, i8_0, i8_1, rgb_scale, r_mix, draw_3d, rect_2d, dview, off_active, off_i8, r_memblend, r_replace, r_blend; unsigned prim, cconst, mixcol; unsigned plan_gen; } TevKey;
static TevKey tev_key;
static int tev_valid, tev_memb;
/* C6: pre-built texenv and fixed GPU state per PSO id (compiled draws). */
typedef struct {
    u8 valid, env_hi, memb, z_on;
    TevKey tk;
    C3D_TexEnv env[6];
    struct { int dt[3]; float dm[2]; int dm_on, at[2]; unsigned sc[5]; int bl[6]; int cull, ed[3]; unsigned has; } shsnap;
    int stencil_on;
} PsoGpuPkt;
static PsoGpuPkt pso_pkt[PM_PSO_N];
static unsigned pso_pkt_napply;
/* PG3 O4: citro3d marks a state group dirty at every setter call and sends it again at the next draw. These wrappers call the setter only when an
 * argument differs from the last call. sh.has is cleared where citro3d may change the state itself (frame begin, target change, frame split). */
static struct { int dt[3]; float dm[2]; int dm_on, at[2]; unsigned sc[5]; int bl[6]; int cull, ed[3], uc; unsigned has; } sh;
#define SH_DT 1u
#define SH_DM 2u
#define SH_AT 4u
#define SH_SC 8u
#define SH_BL 16u
#define SH_CU 32u
#define SH_ED 64u
#define SH_UC 128u
/* GP2: PICA user clip w - 0.05 >= 0 on raw-batch clip positions (gv_post row 3 is 0,0,0,1). */
static u32 gp2_clip_dat[4];
static void gp2_clip_init(void) {
    gp2_clip_dat[0] = f32tof24(0.f);
    gp2_clip_dat[1] = f32tof24(0.f);
    gp2_clip_dat[2] = f32tof24(0.f);
    gp2_clip_dat[3] = f32tof24(1.f);
}
/* DR1: O2 merges consecutive texture rectangles and fills with equal state into one draw.
 * rect_open: the open batch holds rectangles. Their draw state (effective combine mode, rect_z) is kept in rect_ec and rect_zb and
 * flush_impl puts it in place for the draw, then restores the caller's state. pm_gpu_drew and the grab and off-screen entry points flush it first. */
static int rect_open, rect_ec, rect_zb, rect_tex; /* C7: rect_tex is the tile-0 cache slot for an open DR1 batch */
static unsigned plan_gen; /* DR1: counts changes of `plan`, the texenv key compares this instead of the plan */
static unsigned dr1_merged, dr1_breaks, dr1_skips, kick_n; /* the dr1 log line (kick_n: KICK1 splits) */
static int pool_buf_ok; /* DR1: the citro3d buffer info holds the vertex pool (set once per frame, cleared by a raw batch and by sh_clear) */
static int proj_ok; static C3D_Mtx proj_last; /* DR1: the projection uniform of the last draw */
static void sh_clear(void) { sh.has = 0; tev_valid = 0; pool_buf_ok = 0; proj_ok = 0; pm_tex_bind_reset(); }
static void sh_depth_test(bool en, GPU_TESTFUNC f, GPU_WRITEMASK m) {
    if ((sh.has & SH_DT) && sh.dt[0] == (int)en && sh.dt[1] == (int)f && sh.dt[2] == (int)m) return;
    sh.dt[0] = en; sh.dt[1] = (int)f; sh.dt[2] = (int)m; sh.has |= SH_DT; C3D_DepthTest(en, f, m);
}
static void sh_depth_map(bool en, float sc, float off) {
    if ((sh.has & SH_DM) && sh.dm_on == (int)en && sh.dm[0] == sc && sh.dm[1] == off) return;
    sh.dm_on = en; sh.dm[0] = sc; sh.dm[1] = off; sh.has |= SH_DM; C3D_DepthMap(en, sc, off);
}
static void sh_alpha_test(bool en, GPU_TESTFUNC f, int ref) {
    if ((sh.has & SH_AT) && sh.at[0] == (int)en && sh.at[1] == ref) return;
    sh.at[0] = en; sh.at[1] = ref; sh.has |= SH_AT; C3D_AlphaTest(en, f, ref);
}
static void sh_scissor(GPU_SCISSORMODE m, u32 l, u32 t, u32 r, u32 b) {
    if ((sh.has & SH_SC) && sh.sc[0] == (unsigned)m && sh.sc[1] == l && sh.sc[2] == t && sh.sc[3] == r && sh.sc[4] == b) return;
    sh.sc[0] = m; sh.sc[1] = l; sh.sc[2] = t; sh.sc[3] = r; sh.sc[4] = b; sh.has |= SH_SC; C3D_SetScissor(m, l, t, r, b);
}
static void sh_blend(GPU_BLENDEQUATION c, GPU_BLENDEQUATION a, GPU_BLENDFACTOR sc, GPU_BLENDFACTOR dc, GPU_BLENDFACTOR sa, GPU_BLENDFACTOR da) {
    if ((sh.has & SH_BL) && sh.bl[0] == (int)c && sh.bl[1] == (int)a && sh.bl[2] == (int)sc && sh.bl[3] == (int)dc && sh.bl[4] == (int)sa && sh.bl[5] == (int)da) return;
    sh.bl[0] = c; sh.bl[1] = a; sh.bl[2] = sc; sh.bl[3] = dc; sh.bl[4] = sa; sh.bl[5] = da; sh.has |= SH_BL; C3D_AlphaBlend(c, a, sc, dc, sa, da);
}
static void sh_cull(GPU_CULLMODE m) { if ((sh.has & SH_CU) && sh.cull == (int)m) return; sh.cull = (int)m; sh.has |= SH_CU; C3D_CullFace(m); }
static void sh_early(bool en, GPU_EARLYDEPTHFUNC f, u32 v) {
    if ((sh.has & SH_ED) && sh.ed[0] == (int)en && sh.ed[1] == (int)f && sh.ed[2] == (int)v) return;
    sh.ed[0] = en; sh.ed[1] = (int)f; sh.ed[2] = (int)v; sh.has |= SH_ED; C3D_EarlyDepthTest(en, f, v);
}
static void sh_user_clip(int on) {
    if ((sh.has & SH_UC) && sh.uc == on) return;
    sh.uc = on; sh.has |= SH_UC;
    if (on) {
        GPUCMD_AddWrite(GPUREG_FRAGOP_CLIP, 1);
        GPUCMD_AddIncrementalWrites(GPUREG_FRAGOP_CLIP_DATA0, gp2_clip_dat, 4);
    } else
        GPUCMD_AddWrite(GPUREG_FRAGOP_CLIP, 0);
}

static void flush_tev_rebuild(C3D_TexEnv* env, int tex, int tex1, int i8_0, int i8_1, int noise, int* memb_out);
static void pso_gst_record(int z, int graw, PsoGpuPkt* p);
static int pso_pkt_try(unsigned id, int z, int graw, int* memb_out);
static void flush_impl(void);
#define flush() do { fl_fn = __func__; fl_line = __LINE__; flush_impl(); } while (0)
/* switch rectdbg: compare the draw state when a queued rectangle batch is drawn with the state when it was queued (finds a setter that forgot to flush) */
#define SN_N 36
static int dbg_snap[SN_N], dbg_have;
static void dbg_take(int* o) {
    int k = 0;
    o[k++] = r_zcmp; o[k++] = r_zupd; o[k++] = r_blend; o[k++] = r_atest; o[k++] = r_decal; o[k++] = r_zmark; o[k++] = r_nocol; o[k++] = r_mix; o[k++] = (int)r_mixcol;
    o[k++] = r_replace; o[k++] = r_memblend; o[k++] = z_on; o[k++] = light_on; o[k++] = rgb_scale; o[k++] = (int)prim; o[k++] = (int)cconst; o[k++] = sci_x0; o[k++] = sci_y0;
    o[k++] = sci_x1; o[k++] = sci_y1; o[k++] = off_active; o[k++] = off_cur; o[k++] = off_i8; o[k++] = grab_bind; o[k++] = draw_3d; o[k++] = (int)plan_gen; o[k++] = pm_tex_off_idx();
    o[k++] = (int)(t1_s[0] * 1000.f); o[k++] = pm_wide; o[k++] = ui_ortho; o[k++] = stencil_on; o[k++] = pm_tex_i8(0); o[k++] = pm_tex_failed(); o[k++] = (int)pm_tex_epoch();
    while (k < SN_N) o[k++] = 0;
}
static void dbg_cmp(void) {
    int now[SN_N];
    static int n;
    dbg_take(now);
    for (int i = 0; i < SN_N - 1; i++) if (i != 33 && now[i] != dbg_snap[i] && n++ < 40) pm_log_w("rectdbg: state %d was %d now %d at the draw of a queued rectangle batch (frame %u) flush from %s line %d", i, dbg_snap[i], now[i], pm_frame_count(), fl_fn, fl_line);
}
/* KICK1: citro3d holds the GX queue inside a frame and starts the PICA at C3D_FrameEnd, so the GPU idled for the whole CPU part of the frame. One
 * C3D_FrameSplit when about 55 percent of the draws of the last frame are queued starts the GPU on the first part while the CPU builds the rest.
 * Cost: one more cache flush and one more command list submit on the CPU. Only on the main target (an off-screen pass and a grab split on their own).
 * Switch nokick turns it off. */
static unsigned prev_draws, kick_at;
static int kick_done;
static void kick_check(void) {
    if (kick_done || off_active || !kick_at || draws_frame < kick_at || rect_open || nverts > 0 || gni > 0) return;
    kick_done = 1;
    gpu_flush_written();
    if (want_clear) { C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0); want_clear = 0; } /* queue order: the clear comes before the first part */
    sh_clear();
    PH_PUSH(PH_GPU);
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    PH_POP();
    kick_n++;
}
static void flush_draw(void);
static void flush_impl(void) {
    if (rect_open && dbg_have) { dbg_cmp(); dbg_have = 0; }
    if (rect_open) { /* DR1: the batch is rectangles: draw it with the state they were queued in */
        int sc = combine, sr = rect_2d, sz = rect_z;
        combine = rect_ec; rect_2d = 1; rect_z = rect_zb; rect_open = 0; rect_tex = -1;
        flush_draw();
        combine = sc; rect_2d = sr; rect_z = sz;
        rect_y0 = rect_y1 = 0; /* the rows of the rectangles in this batch are used up */
        return;
    }
    flush_draw();
}
static void flush_tev_rebuild(C3D_TexEnv* env, int tex, int tex1, int i8_0, int i8_1, int noise, int* memb_out) {
    int memb = 0;
    TevKey tk;
    const size_t tk_n = sizeof tk;
    memset(&tk, 0, tk_n);
    tk.combine = combine; tk.tex = tex; tk.tex1 = tex1; tk.i8_0 = i8_0; tk.i8_1 = i8_1; tk.rgb_scale = rgb_scale; tk.r_mix = r_mix; tk.draw_3d = draw_3d; tk.rect_2d = rect_2d;
    tk.dview = dview; tk.off_active = off_active; tk.off_i8 = off_i8; tk.r_memblend = r_memblend; tk.r_replace = r_replace; tk.r_blend = r_blend;
    tk.prim = prim; tk.cconst = cconst; tk.mixcol = r_mixcol;
    tk.plan_gen = combine == 8 ? plan_gen : 0;
        for (int i = 1; i < env_hi; i++) C3D_TexEnvInit(C3D_GetTexEnv(i));
        env_hi = 1;
        C3D_TexEnvInit(env);
        /* Unit 1 stays bound to the last TEXEL1 texture: C3D_TexBind(1, NULL) crashes. No stage reads it. */
        if (combine == 8 && (!plan.uses_tex || tex) && (!plan.uses_tex1 || tex1) && (!plan.uses_noise || noise)) {
            /* S25: general combiner. A stage reads the texture, the vertex colour, its constant, the previous
             * stage or the buffer colour. The buffer is never updated by a stage, so it holds one constant. */
            C3D_TexEnvBufUpdate(C3D_Both, 0);
            C3D_TexEnvBufColor(abgr(plan.buf));
            for (int i = 0; i < plan.n && i < 6; i++) {
                C3D_TexEnv* e = C3D_GetTexEnv(i);
                const PmTevStage* st = &plan.st[i];
                C3D_TexEnvInit(e);
                C3D_TexEnvSrc(e, C3D_RGB, tev_src(st->g[0].src[0]), tev_src(st->g[0].src[1]), tev_src(st->g[0].src[2]));
                C3D_TexEnvOpRgb(e, tev_op_rgb_i(st->g[0].op[0], tev_i8(st->g[0].src[0], i8_0, i8_1)), tev_op_rgb_i(st->g[0].op[1], tev_i8(st->g[0].src[1], i8_0, i8_1)),
                                tev_op_rgb_i(st->g[0].op[2], tev_i8(st->g[0].src[2], i8_0, i8_1)));
                C3D_TexEnvFunc(e, C3D_RGB, tev_func(st->g[0].func));
                C3D_TexEnvSrc(e, C3D_Alpha, tev_src(st->g[1].src[0]), tev_src(st->g[1].src[1]), tev_src(st->g[1].src[2]));
                C3D_TexEnvOpAlpha(e, tev_op_a_i(st->g[1].op[0], tev_i8(st->g[1].src[0], i8_0, i8_1)), tev_op_a_i(st->g[1].op[1], tev_i8(st->g[1].src[1], i8_0, i8_1)),
                                  tev_op_a_i(st->g[1].op[2], tev_i8(st->g[1].src[2], i8_0, i8_1)));
                C3D_TexEnvFunc(e, C3D_Alpha, tev_func(st->g[1].func));
                C3D_TexEnvColor(e, abgr(st->constant));
                if (st->g[0].scale > 1) C3D_TexEnvScale(e, C3D_RGB, st->g[0].scale == 4 ? GPU_TEVSCALE_4 : GPU_TEVSCALE_2);
                if (st->g[1].scale > 1) C3D_TexEnvScale(e, C3D_Alpha, st->g[1].scale == 4 ? GPU_TEVSCALE_4 : GPU_TEVSCALE_2);
            }
            env_hi = plan.n;
        } else if (combine == 8) {
            C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
        } else if (tex && combine == 6) {
            /* (TEXEL - PRIM) * PRIM_ALPHA + PRIM: the texture fades to the primitive colour (pause screen
             * background, darkened by gray 40 at alpha 128). Alpha = texel alpha. */
            C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_TEXTURE0, GPU_CONSTANT);
            C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0);
            if (i8_0)
                C3D_TexEnvOpAlpha(env, GPU_TEVOP_A_SRC_R, GPU_TEVOP_A_SRC_R, GPU_TEVOP_A_SRC_R);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
            C3D_TexEnvColor(env, abgr(prim));
        } else if (tex && (combine == 5 || combine == 7)) {
            /* (PRIM - ENV) * TEXEL + ENV, alpha = texel alpha. ENV rides in the vertex colour. */
            C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_PRIMARY_COLOR, GPU_TEXTURE0);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
            if (combine == 7) {
                C3D_TexEnvSrc(env, C3D_Alpha, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
                C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
                C3D_TexEnvColor(env, abgr(prim | 0xffu));
            } else {
                C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0);
                if (i8_0)
                    C3D_TexEnvOpAlpha(env, GPU_TEVOP_A_SRC_R, GPU_TEVOP_A_SRC_R, GPU_TEVOP_A_SRC_R);
                C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
                C3D_TexEnvColor(env, abgr(prim));
            }
        } else if (tex && combine == 4) {
            /* Flat colour, alpha = texture alpha times the constant's alpha. */
            C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT, GPU_CONSTANT);
            if (i8_0)
                C3D_TexEnvOpAlpha(env, GPU_TEVOP_A_SRC_R, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
            C3D_TexEnvColor(env, abgr(cconst));
        } else if (tex) {
            C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_MODULATE);
            if (combine == 3) {
                C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
            } else {
                C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                if (i8_0)
                    C3D_TexEnvOpAlpha(env, GPU_TEVOP_A_SRC_R, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
                C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
            }
            if (rgb_scale == 2) C3D_TexEnvScale(env, C3D_RGB, GPU_TEVSCALE_2);
        } else if (combine == 1) {
            C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            C3D_TexEnvColor(env, abgr(prim));
        } else {
            C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
        }
        if (r_mix && draw_3d && !rect_2d && env_hi < 6 && !dview) {
            /* V06: colour = mix * alpha + previous * (1 - alpha), alpha = the constant's alpha. The alpha channel passes. */
            C3D_TexEnv* e = C3D_GetTexEnv(env_hi);
            C3D_TexEnvInit(e);
            C3D_TexEnvSrc(e, C3D_RGB, GPU_CONSTANT, GPU_PREVIOUS, r_mix == 2 ? GPU_PRIMARY_COLOR : GPU_CONSTANT); /* F08: r_mix 2 = weight is the vertex alpha */
            C3D_TexEnvOpRgb(e, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA);
            C3D_TexEnvFunc(e, C3D_RGB, GPU_INTERPOLATE);
            C3D_TexEnvSrc(e, C3D_Alpha, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
            C3D_TexEnvFunc(e, C3D_Alpha, GPU_REPLACE);
            C3D_TexEnvColor(e, abgr(r_mixcol));
            env_hi++;
        }
        /* FX2a: an 8 bit image has memory alpha 0xE0 (7/8). The RDP blender then gives (P*a + M*0.88) / (a + 0.88): the weight of P is
         * a / (a + 0.88), 0.53 at a = 1. The PICA blends by src alpha, so the fragment alpha becomes 0.532 * (1 - (1 - a)^2), a fit of that weight
         * (error under 0.04). Pass A of the flame (colour 0, alpha 1) so keeps 0.47 of the old pixel and pass B keeps the old pixel where the blob alpha is 0. */
        memb = off_active && off_i8 && r_memblend && !r_replace;
        if (memb && env_hi <= 4) {
            C3D_TexEnv* e = C3D_GetTexEnv(env_hi++);
            C3D_TexEnvInit(e);
            C3D_TexEnvSrc(e, C3D_RGB, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
            C3D_TexEnvFunc(e, C3D_RGB, GPU_REPLACE);
            C3D_TexEnvSrc(e, C3D_Alpha, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
            C3D_TexEnvOpAlpha(e, GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA, GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA, GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA);
            C3D_TexEnvFunc(e, C3D_Alpha, GPU_MODULATE);
            e = C3D_GetTexEnv(env_hi++);
            C3D_TexEnvInit(e);
            C3D_TexEnvSrc(e, C3D_RGB, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
            C3D_TexEnvFunc(e, C3D_RGB, GPU_REPLACE);
            C3D_TexEnvSrc(e, C3D_Alpha, GPU_PREVIOUS, GPU_CONSTANT, GPU_CONSTANT);
            C3D_TexEnvOpAlpha(e, GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
            C3D_TexEnvFunc(e, C3D_Alpha, GPU_MODULATE);
            C3D_TexEnvColor(e, 0x88000000u);
        } else memb = 0;
        if (off_active && off_i8 && !(r_blend && !r_replace) && !memb && env_hi < 6) { /* FX2a: a blended 8 bit write keeps the blend weight in A */
            /* X02: an 8 bit intensity image reads as I in all four channels. The pixel written is (R, R, R, R). */
            C3D_TexEnv* e = C3D_GetTexEnv(env_hi);
            C3D_TexEnvInit(e);
            C3D_TexEnvSrc(e, C3D_Both, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
            C3D_TexEnvOpAlpha(e, GPU_TEVOP_A_SRC_R, GPU_TEVOP_A_SRC_R, GPU_TEVOP_A_SRC_R);
            C3D_TexEnvFunc(e, C3D_Both, GPU_REPLACE);
            env_hi++;
        }
        /* Fill and texrects ignore the Z buffer. Startup turns G_ZBUFFER on
         * before the logo, and every 2D quad shares z, so a depth test keeps
         * only the first rectangle. */
        if (dview && draw_3d && !rect_2d) { /* V02 depth view: vertex colour only, alpha from the texture */
            for (int i = 0; i < 6; i++) C3D_TexEnvInit(C3D_GetTexEnv(i));
            env_hi = 6;
            env = C3D_GetTexEnv(0);
            C3D_TexEnvSrc(env, C3D_RGB, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
            if (tex) {
                C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
            } else {
                C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
            }
        }
    memcpy(&tev_key, &tk, tk_n); tev_valid = 1; tev_memb = memb;
    if (memb_out) *memb_out = memb;
}

static void pso_gst_record(int z, int graw, PsoGpuPkt* p) {
    sh.has = 0;
    if (z && r_zmark) {
        sh_depth_test(true, GPU_GREATER, GPU_WRITE_DEPTH);
        C3D_StencilTest(true, GPU_ALWAYS, 1, 0xff, 0xff);
        C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
        stencil_on = 1;
    } else if (z && r_zcmp)
        sh_depth_test(true, r_decal ? GPU_GEQUAL : GPU_GREATER, r_nocol ? GPU_WRITE_DEPTH : r_zupd ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
    else if (z && r_zupd)
        sh_depth_test(true, GPU_ALWAYS, r_nocol ? GPU_WRITE_DEPTH : GPU_WRITE_ALL);
    else
        sh_depth_test(false, GPU_GREATER, GPU_WRITE_COLOR);
    if (stencil_on && !(z && r_zmark)) { C3D_StencilTest(false, GPU_ALWAYS, 0, 0xff, 0); stencil_on = 0; }
    sh_depth_map(true, -1.0f, r_decal ? decal_bias() : 0.0f);
    sh_early(false, GPU_EARLYDEPTH_GREATER, 0);
    if (r_atest >= 0) sh_alpha_test(true, GPU_GREATER, r_atest);
    else sh_alpha_test(false, GPU_ALWAYS, 0);
    sh_cull(graw ? (GPU_CULLMODE)g_cullm : GPU_CULL_NONE);
    if (off_active && (r_replace || (off_i8 && !r_blend && !tev_memb)))
        sh_blend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    else if (off_active)
        sh_blend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA);
    else if (r_blend)
        sh_blend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    else
        sh_blend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    memcpy(&p->shsnap, &sh, sizeof sh);
    p->stencil_on = stencil_on;
    p->z_on = (u8)z;
}

static void pso_sh_restore(const PsoGpuPkt* p) {
    sh.has = 0;
    C3D_DepthTest((bool)p->shsnap.dt[0], (GPU_TESTFUNC)p->shsnap.dt[1], (GPU_WRITEMASK)p->shsnap.dt[2]);
    C3D_DepthMap((bool)p->shsnap.dm_on, p->shsnap.dm[0], p->shsnap.dm[1]);
    C3D_AlphaTest((bool)p->shsnap.at[0], GPU_GREATER, p->shsnap.at[1]);
    C3D_EarlyDepthTest((bool)p->shsnap.ed[0], (GPU_EARLYDEPTHFUNC)p->shsnap.ed[1], (u32)p->shsnap.ed[2]);
    C3D_CullFace((GPU_CULLMODE)p->shsnap.cull);
    C3D_AlphaBlend((GPU_BLENDEQUATION)p->shsnap.bl[0], (GPU_BLENDEQUATION)p->shsnap.bl[1], (GPU_BLENDFACTOR)p->shsnap.bl[2], (GPU_BLENDFACTOR)p->shsnap.bl[3], (GPU_BLENDFACTOR)p->shsnap.bl[4], (GPU_BLENDFACTOR)p->shsnap.bl[5]);
    memcpy(&sh, &p->shsnap, sizeof sh);
    if (p->stencil_on) {
        C3D_StencilTest(true, GPU_ALWAYS, 1, 0xff, 0xff);
        C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
        stencil_on = 1;
    }
}

static int pso_pkt_try(unsigned id, int z, int graw, int* memb_out) {
    if (!id || id >= PM_PSO_N || !pso_pkt[id].valid || pso_pkt[id].z_on != (u8)z) return 0;
    const PsoGpuPkt* p = &pso_pkt[id];
    env_hi = (int)p->env_hi;
    for (int i = 0; i < p->env_hi && i < 6; i++) *C3D_GetTexEnv(i) = p->env[i];
    tev_key = p->tk;
    tev_valid = 1;
    tev_memb = p->memb;
    if (memb_out) *memb_out = p->memb;
    pso_sh_restore(p);
    pso_pkt_napply++;
    return 1;
}

void pm_pso_bake(unsigned id) {
    if (!id || id >= PM_PSO_N) return;
    PsoGpuPkt* p = &pso_pkt[id];
    memset(p, 0, sizeof *p);
    int tex = (combine >= 2) && !grab_bind && pm_tex_bind();
    int tex1 = combine == 8 && plan.uses_tex1 && pm_tex_bind_aux();
    int i8_0 = tex && pm_tex_i8(0), i8_1 = tex1 && pm_tex_i8(1);
    int noise = combine == 8 && plan.uses_noise && noise_init();
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    flush_tev_rebuild(env, tex, tex1, i8_0, i8_1, noise, NULL);
    p->env_hi = (u8)env_hi;
    for (int i = 0; i < env_hi && i < 6; i++) p->env[i] = *C3D_GetTexEnv(i);
    p->tk = tev_key;
    p->memb = (u8)tev_memb;
    int z = z_on && !off_active;
    pso_gst_record(z, 1, p);
    p->valid = 1;
}

static unsigned flush_gen; /* GV1b: counts the raw batches closed */
static void flush_draw(void) {
    int graw = gni > 0;
    if (graw) flush_gen++; /* PO3b: the open batch is a raw Vtx batch */
    if (fl_on() && gpu_ok) { fl_count(nverts <= 0 && !graw); if (fl_fn != (const char*)"pm_gpu_tex_rect" && strcmp(fl_fn, "pm_gpu_tex_rect") != 0) fl_prev_rect = 0; }
    if ((nverts <= 0 && !graw) || !gpu_ok) {
        nverts = 0;
        gni = 0; gi_start = gidx_n;
        batch_identity = 0;
        return;
    }
    if (r_nocol && !(z_on && (!rect_2d || rect_z) && r_zupd)) {
        nverts = 0;
        gni = 0; gi_start = gidx_n;
        batch_identity = 0;
        return;
    }
    /* H03: the texture of this batch failed to load (cache full). Skip the batch, a white quad is worse than a missing one. */
    if (combine >= 2 && !grab_bind && pm_tex_failed() && (combine != 8 || plan.uses_tex || plan.uses_tex1)) {
        nverts = 0;
        gni = 0; gi_start = gidx_n;
        batch_identity = 0;
        return;
    }
    int snap0 = 0, snap1 = 0;
    if (off_active) { /* FX2a: this batch reads the slot it writes */
        snap0 = combine >= 2 && !grab_bind && pm_tex_off_idx() == off_cur;
        snap1 = combine == 8 && plan.uses_tex1 && pm_tex_aux_off_idx() == off_cur;
        if (snap0 || snap1) off_snapshot(off_cur);
        if (!offs[off_cur].snap_ready) snap0 = snap1 = 0;
    }
    PH_ADD(C_FLUSH, 1);
    PH_PUSH(PH_GPU);
    PHF_PUSH(PH_GBIND);
    const int gpal = graw && g_pal; /* GV1c: the palette program */
    if (graw) { /* PO3b */
        proj_ok = 0; pool_buf_ok = 0; /* DR1: the program and the buffer change */
        if (gpal && g_use5) { /* GV5: lighting, texgen, fog */
            if (bound != 4) { C3D_BindProgram(&prog5); C3D_SetAttrInfo(&attr4); bound = 4; PH_ADD(C_RAWCALL, 2); }
            if (!sh5_ok || memcmp(sh5_uvso, g_uvso, sizeof sh5_uvso)) { C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_uvso, g_uvso[0], g_uvso[1], g_uvso[2], g_uvso[3]); memcpy(sh5_uvso, g_uvso, sizeof sh5_uvso); PH_ADD(C_RAWCALL, 1); }
            if (!sh5_ok || memcmp(sh5_cmul, g_cmul, sizeof sh5_cmul)) { C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_cmul, g_cmul[0], g_cmul[1], g_cmul[2], g_cmul[3]); memcpy(sh5_cmul, g_cmul, sizeof sh5_cmul); PH_ADD(C_RAWCALL, 1); }
            if (!sh5_ok || memcmp(sh5_cadd, g_cadd, sizeof sh5_cadd)) { C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_cadd, g_cadd[0], g_cadd[1], g_cadd[2], g_cadd[3]); memcpy(sh5_cadd, g_cadd, sizeof sh5_cadd); PH_ADD(C_RAWCALL, 1); }
            if (!sh5_ok || memcmp(sh5_zmix, g_zmix, sizeof sh5_zmix)) { C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_zmix, g_zmix[0], g_zmix[1], 0.f, 0.f); memcpy(sh5_zmix, g_zmix, sizeof sh5_zmix); PH_ADD(C_RAWCALL, 1); }
            if (!sh5_ok || memcmp(&sh5_fx, &g_fx, sizeof g_fx)) { /* the light block, the look-at vectors, the fog factor and the flags: one set per change */
                C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_lflag, g_fx.lflag[0], g_fx.lflag[1], g_fx.lflag[2], g_fx.lflag[3]);
                C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_fogu, g_fx.fogu[0], g_fx.fogu[1], g_fx.fogu[2], g_fx.fogu[3]);
                if (g_fx.lflag[0] != 0.f) { /* lighting: the light table and, for texgen, the look-at vectors */
                    C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_lamb, g_fx.amb[0], g_fx.amb[1], g_fx.amb[2], g_fx.amb[3]);
                    for (int i = 0; i < 7; i++) {
                        C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_lcol + i, g_fx.col[i][0], g_fx.col[i][1], g_fx.col[i][2], g_fx.col[i][3]);
                        C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_ldir + i, g_fx.dir[i][0], g_fx.dir[i][1], g_fx.dir[i][2], g_fx.dir[i][3]);
                    }
                    C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_lk0, g_fx.lk0[0], g_fx.lk0[1], g_fx.lk0[2], g_fx.lk0[3]);
                    C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_lk1, g_fx.lk1[0], g_fx.lk1[1], g_fx.lk1[2], g_fx.lk1[3]);
                }
                sh5_fx = g_fx;
                PH_ADD(C_RAWCALL, 2 + (g_fx.lflag[0] != 0.f ? 17 : 0));
            }
            sh5_ok = 1;
            sh_ok = 0; memset(sh_pal_ok, 0, sizeof sh_pal_ok); /* the uniform registers are one file for all programs: pm5 wrote over what pm4 and pm had */
            for (unsigned k = 0; k < pal_n; k++) {
                unsigned gm = pal_g[k];
                if (pcomp_gen[gm] != post_gen) {
                    C3D_Mtx game;
                    const float(*m)[4] = pal_tab[gm];
                    for (int r = 0; r < 4; r++) { game.r[r].x = m[0][r]; game.r[r].y = m[1][r]; game.r[r].z = m[2][r]; game.r[r].w = m[3][r]; }
                    Mtx_Multiply(&pcomp[gm], &gv_post, &game);
                    pcomp_gen[gm] = post_gen;
                    RS_ADD(RS_PCOMP, 1);
                }
                if (sh5_pal_ok[k] && sh5_pal_gm[k] == gm && sh5_pal_gen[k] == post_gen) continue;
                C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, u5_pal + 7 * (int)k, &pcomp[gm]);
                if (pal_ntab) for (int r = 0; r < 3; r++) C3D_FVUnifSet(GPU_VERTEX_SHADER, u5_pal + 7 * (int)k + 4 + r, pal_ntab[gm][r][0], pal_ntab[gm][r][1], pal_ntab[gm][r][2], 0.f);
                sh5_pal_ok[k] = 1; sh5_pal_gm[k] = gm; sh5_pal_gen[k] = post_gen;
                PH_ADD(C_RAWCALL, 4);
                gv_palup_n++;
            }
        } else if (gpal) {
            sh5_ok = 0; memset(sh5_pal_ok, 0, sizeof sh5_pal_ok); /* pm4 writes over the registers of pm5 */
            if (bound != 3) { C3D_BindProgram(&prog4); C3D_SetAttrInfo(&attr4); bound = 3; PH_ADD(C_RAWCALL, 2); }
            if (!sh_ok || memcmp(sh_uvso, g_uvso, sizeof sh_uvso)) { C3D_FVUnifSet(GPU_VERTEX_SHADER, u4_uvso, g_uvso[0], g_uvso[1], g_uvso[2], g_uvso[3]); memcpy(sh_uvso, g_uvso, sizeof sh_uvso); PH_ADD(C_RAWCALL, 1); }
            if (!sh_ok || memcmp(sh_cmul, g_cmul, sizeof sh_cmul)) { C3D_FVUnifSet(GPU_VERTEX_SHADER, u4_cmul, g_cmul[0], g_cmul[1], g_cmul[2], g_cmul[3]); memcpy(sh_cmul, g_cmul, sizeof sh_cmul); PH_ADD(C_RAWCALL, 1); }
            if (!sh_ok || memcmp(sh_cadd, g_cadd, sizeof sh_cadd)) { C3D_FVUnifSet(GPU_VERTEX_SHADER, u4_cadd, g_cadd[0], g_cadd[1], g_cadd[2], g_cadd[3]); memcpy(sh_cadd, g_cadd, sizeof sh_cadd); PH_ADD(C_RAWCALL, 1); }
            if (!sh_ok || memcmp(sh_zmix, g_zmix, sizeof sh_zmix)) { C3D_FVUnifSet(GPU_VERTEX_SHADER, u4_zmix, g_zmix[0], g_zmix[1], 0.f, 0.f); memcpy(sh_zmix, g_zmix, sizeof sh_zmix); PH_ADD(C_RAWCALL, 1); }
            sh_ok = 1;
            for (unsigned k = 0; k < pal_n; k++) {
                unsigned gm = pal_g[k];
                if (pcomp_gen[gm] != post_gen) {
                    C3D_Mtx game;
                    const float(*m)[4] = pal_tab[gm];
                    for (int r = 0; r < 4; r++) { game.r[r].x = m[0][r]; game.r[r].y = m[1][r]; game.r[r].z = m[2][r]; game.r[r].w = m[3][r]; }
                    Mtx_Multiply(&pcomp[gm], &gv_post, &game);
                    pcomp_gen[gm] = post_gen;
                    RS_ADD(RS_PCOMP, 1);
                }
                if (sh_pal_ok[k] && sh_pal_gm[k] == gm && sh_pal_gen[k] == post_gen) continue; /* the register block holds this matrix already */
                C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, u4_pal + 4 * (int)k, &pcomp[gm]);
                sh_pal_ok[k] = 1; sh_pal_gm[k] = gm; sh_pal_gen[k] = post_gen;
                PH_ADD(C_RAWCALL, 1);
                gv_palup_n++;
            }
        }
    }
    /* 2D batches are already pixels. 3D batches are object space. */
    if (u_proj >= 0 && !gpal) {
        const C3D_Mtx* pm_ = draw_3d ? &shader_3d : off_active ? &off_proj : &projection; /* V05: shader_3d follows off_active, see rebuild_3d */
        if (!graw && proj_ok && memcmp(&proj_last, pm_, sizeof proj_last) == 0) dr1_skips++; /* DR1: same value as the last draw */
        else {
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, u_proj, pm_); sh_pal_ok[0] = sh_pal_ok[1] = 0; sh5_pal_ok[0] = 0; /* RAW1: registers 0 to 3 are also pal 0 */
            proj_last = *pm_; proj_ok = !graw;
        }
    }
    /* Rectangles skip the depth test, so a story page already covers the mesh.
     * Wiping the buffer here let the next 3D batch ignore the house. */
    (void)batch_identity;
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    int tex = (combine >= 2) && (grab_bind ? (C3D_TexBind(0, &grab_tex), pm_tex_bind_reset(), 1) : pm_tex_bind());
    int tex1 = combine == 8 && plan.uses_tex1 && pm_tex_bind_aux();
    int i8_0 = tex && pm_tex_i8(0), i8_1 = tex1 && pm_tex_i8(1);
    if (tex && snap0) { C3D_TexBind(0, &offs[off_cur].snap); pm_tex_bind_reset(); } /* DR1: unit 0 no longer holds the texture of the shadow */
    if (tex1 && snap1) C3D_TexBind(1, &offs[off_cur].snap);
    if (!gpal) { sh_pal_ok[0] = sh_pal_ok[1] = 0; sh5_pal_ok[0] = 0; } /* RAW1: the cpu program writes registers 0 to 5 (projection, uvx, nzo): pal 0 and 1 */
    if (tex1 && !(gpal && g_use5) && (gpal ? u4_uvx : u_uvx) >= 0) C3D_FVUnifSet(GPU_VERTEX_SHADER, gpal ? u4_uvx : u_uvx, t1_s[0], t1_s[1], t1_s[2], t1_s[3]);
    int noise = combine == 8 && plan.uses_noise && noise_init();
    if (noise) { /* PT07: unit 2 stays bound after the first use, as unit 1 does. The target is 240 x 400 screen pixels: ndc * 120 and ndc * 200 */
        int u = gpal ? (g_use5 ? u5_nzo : u4_nzo) : u_nzo;
        C3D_TexBind(2, &noise_tex);
        if (u >= 0) C3D_FVUnifSet(GPU_VERTEX_SHADER, u, 120.f / 64.f, 200.f / 64.f, noise_off[0], noise_off[1]);
    }
    PHF_POP();
    int z = z_on && (!rect_2d || rect_z) && !off_active;
    { static int nozt = -1; if (nozt < 0) nozt = pm_debug_has("nozt"); if (nozt) z = 0; }
    unsigned pso_id = graw ? pm_pdl_flush_pso() : 0;
    int pso_fast = 0;
    PHF_PUSH(PH_GTEV);
    int memb = 0;
    if (pso_id && pso_pkt_try(pso_id, z, graw, &memb)) pso_fast = 1;
    else {
        TevKey tk;
        const size_t tk_n = sizeof tk;
        memset(&tk, 0, tk_n);
        tk.combine = combine; tk.tex = tex; tk.tex1 = tex1; tk.i8_0 = i8_0; tk.i8_1 = i8_1; tk.rgb_scale = rgb_scale; tk.r_mix = r_mix; tk.draw_3d = draw_3d; tk.rect_2d = rect_2d;
        tk.dview = dview; tk.off_active = off_active; tk.off_i8 = off_i8; tk.r_memblend = r_memblend; tk.r_replace = r_replace; tk.r_blend = r_blend;
        tk.prim = prim; tk.cconst = cconst; tk.mixcol = r_mixcol;
        tk.plan_gen = combine == 8 ? plan_gen : 0;
        int tev_same = tev_valid && memcmp(&tk, &tev_key, tk_n) == 0;
        if (tev_same) memb = tev_memb;
        else flush_tev_rebuild(env, tex, tex1, i8_0, i8_1, noise, &memb);
    }
    PHF_POP();
    PHF_PUSH(PH_GST);
    dv_draw++;
    if (!pso_fast) {
    if (z && r_zmark) {
        /* B3b: pass 1 tests the model with its own depth and marks the stencil where it is nearer than the world. Pass 2 (below) writes the constant there. */
        sh_depth_test(true, GPU_GREATER, GPU_WRITE_DEPTH);
        C3D_StencilTest(true, GPU_ALWAYS, 1, 0xff, 0xff);
        C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
        stencil_on = 1;
    } else if (z && r_zcmp)
        sh_depth_test(true, r_decal ? GPU_GEQUAL : GPU_GREATER, r_nocol ? GPU_WRITE_DEPTH : r_zupd ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
    else if (z && r_zupd)
        sh_depth_test(true, GPU_ALWAYS, r_nocol ? GPU_WRITE_DEPTH : GPU_WRITE_ALL);
    else
        sh_depth_test(false, GPU_GREATER, GPU_WRITE_COLOR);
    /* A decal lies in the plane of its base surface. Move it a little toward the eye. */
    if (stencil_on && !(z && r_zmark)) { C3D_StencilTest(false, GPU_ALWAYS, 0, 0xff, 0); stencil_on = 0; }
    sh_depth_map(true, -1.0f, r_decal ? decal_bias() : 0.0f);
    sh_early(false, GPU_EARLYDEPTH_GREATER, 0);
    if (r_atest >= 0) sh_alpha_test(true, GPU_GREATER, r_atest);
    else sh_alpha_test(false, GPU_ALWAYS, 0);
    }
    /* Target is 240x400, rotated so buffer Y is landscape X. Keep the 320-wide
     * game picture and whatever scissor the display list set inside it. */
    if (off_active) {
        /* not rotated: x is the game x. GPU y is the game y: row 0 of the image is the last memory row, where v = 0 samples */
        int x1 = sci_x1 > off_w ? off_w : sci_x1, y1 = sci_y1 > off_h ? off_h : sci_y1;
        if (x1 > sci_x0 && y1 > sci_y0)
            sh_scissor(GPU_SCISSOR_NORMAL, (u32)sci_x0, (u32)sci_y0, (u32)x1, (u32)y1);
        else
            sh_scissor(GPU_SCISSOR_NORMAL, 0, 0, off_w, off_h);
    } else {
        int left = 240 - sci_y1;
        int right = 240 - sci_y0;
        /* F07-3: a 2D or orthographic batch is 1:1 and centered (40 + x). A scissor edge at 0 or 320 reaches the screen edge. A 3D batch scales by 1.25. */
        int ui = pm_wide && (!draw_3d || ui_ortho);
        int top = !pm_wide ? 40 + sci_x0 : ui ? (sci_x0 <= 0 ? 0 : 40 + sci_x0) : sci_x0 * 5 / 4;
        int bottom = !pm_wide ? 40 + sci_x1 : ui ? (sci_x1 >= 320 ? 400 : 40 + sci_x1) : sci_x1 * 5 / 4;
        if (left < 0) left = 0;
        if (right > 240) right = 240;
        if (top < (pm_wide ? 0 : 40)) top = pm_wide ? 0 : 40;
        if (bottom > (pm_wide ? 400 : 360)) bottom = pm_wide ? 400 : 360;
        if (right > left && bottom > top)
            sh_scissor(GPU_SCISSOR_NORMAL, (u32)left, (u32)(400 - bottom), (u32)right, (u32)(400 - top)); /* V23: the GPU counts the 400 axis from the other end */
        else
            sh_scissor(GPU_SCISSOR_NORMAL, 0, pm_wide ? 0 : 40, 240, pm_wide ? 400 : 360);
    }
    if (!pso_fast) {
    sh_cull(graw ? (GPU_CULLMODE)g_cullm : GPU_CULL_NONE);
    if (off_active && (r_replace || (off_i8 && !r_blend && !memb)))
        sh_blend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    else if (off_active) /* alpha accumulates as source over destination: a covered pixel becomes opaque */
        sh_blend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
            GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA);
    else if (r_blend) /* V25: a 2D rectangle follows its own blender mode (the story page strips use alpha = PRIM with an opaque blender) */
        sh_blend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
            GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    else
        sh_blend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    }
    PHF_POP();
    sh_user_clip(graw); /* GP2: near clip on raw batches only */
    /* The GPU runs at FrameEnd. Each draw must keep its own vertices until then. */
    if (graw) { /* PO3b: one DrawElements for the open batch, then back to the CPU vertex program */
        PH_PUSH(PH_GDRAW);
        if (buf_cur != 2) {
            C3D_BufInfo* gb = C3D_GetBufInfo();
            BufInfo_Init(gb);
            BufInfo_Add(gb, gring, 16, 3, 0x210);
            buf_cur = 2; PH_ADD(C_RAWCALL, 2);
        }
        C3D_DrawElements(GPU_TRIANGLES, (int)gni, C3D_UNSIGNED_SHORT, gidx + gi_start);
        PH_ADD(C_RAWD, 1); PH_ADD(C_RAWCALL, 1);
        PH_POP();
        draws_frame++;
        PH_ADD(C_DRAW, 1);
        PH_ADD(C_VTXSLOT, gni);
        PH_POP();
        gi_start = gidx_n;
        gni = 0; pal_n = 0;
        batch_identity = 0;
        drew = 1;
        if (!off_active) { /* B2: the main target changed, the grab slot is old */
            if (rect_y1 <= rect_y0) grab_dirty = 1;
            else if (grab_dy1 <= grab_dy0) { grab_dy0 = rect_y0; grab_dy1 = rect_y1; }
            else { if (rect_y0 < grab_dy0) grab_dy0 = rect_y0; if (rect_y1 > grab_dy1) grab_dy1 = rect_y1; }
        }
        kick_check();
        return;
    }
    if (bound != 1) { C3D_BindProgram(&prog); C3D_SetAttrInfo(&attr_cpu); bound = 1; pool_buf_ok = 0; }
    Vert* batch = verts;
    int count = nverts;
    PH_PUSH(PH_GDRAW);
    C3D_BufInfo* buf = C3D_GetBufInfo();
    const int first = (int)(batch - vert_base); /* DR1: one buffer info for the whole pool, the draw starts at the batch */
    if (!pool_buf_ok) {
        BufInfo_Init(buf); buf_cur = 1;
        BufInfo_Add(buf, vert_base, sizeof(Vert), 3, 0x210);
        pool_buf_ok = 1;
    }
    int decal2 = z && r_zcmp && r_decal && r_blend && !r_zmark && !r_nocol && draw_3d;
    if (decal2) { /* V16b pass 1: no colour, no depth write, mark the stencil where the stored depth is not behind the decal */
        sh_depth_map(true, -1.0f, -DECAL_EPS);
        sh_depth_test(true, GPU_LEQUAL, (GPU_WRITEMASK)0);
        C3D_StencilTest(true, GPU_ALWAYS, 1, 0xff, 0xff);
        C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
        C3D_DrawArrays(GPU_TRIANGLES, first, count);
        sh_depth_map(true, -1.0f, decal_bias());
        sh_depth_test(true, GPU_GEQUAL, r_zupd ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
        C3D_StencilTest(true, GPU_EQUAL, 1, 0xff, 0xff);
        C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_ZERO, GPU_STENCIL_ZERO);
        draws_frame++;
    }
    C3D_DrawArrays(GPU_TRIANGLES, first, count);
    if (decal2) { C3D_StencilTest(false, GPU_ALWAYS, 0, 0xff, 0); }
    PH_POP();
    verts = batch + count;
    /* 16-byte aligned start for the next batch (a vertex is 40 bytes, so every other batch start moves one slot). */
    if ((unsigned)(uintptr_t)verts & 15u) verts++;
    if (z && r_zmark && verts + count < vert_base + VERT_SLOTS) {
        /* B3b pass 2: the same triangles with the constant depth, where pass 1 marked the stencil. The stencil is cleared as it passes. */
        Vert* b2 = verts;
        for (int i = 0; i < count; i++) { b2[i] = batch[i]; b2[i].z = r_zmark_zn * b2[i].w; }
        sh_depth_test(true, GPU_ALWAYS, GPU_WRITE_DEPTH);
        C3D_StencilTest(true, GPU_EQUAL, 1, 0xff, 0xff);
        C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_ZERO);
        C3D_DrawArrays(GPU_TRIANGLES, (int)(b2 - vert_base), count);
        verts = b2 + count;
        if ((unsigned)(uintptr_t)verts & 15u) verts++;
        draws_frame++;
    }
    draws_frame++;
    PH_ADD(C_DRAW, 1);
    PH_ADD(C_VTXSLOT, count);
    PH_POP();
    nverts = 0;
    batch_identity = 0;
    drew = 1;
    if (!off_active) { /* B2: the main target changed, the grab slot is old */
        if (rect_y1 <= rect_y0) grab_dirty = 1;
        else if (grab_dy1 <= grab_dy0) { grab_dy0 = rect_y0; grab_dy1 = rect_y1; }
        else { if (rect_y0 < grab_dy0) grab_dy0 = rect_y0; if (rect_y1 > grab_dy1) grab_dy1 = rect_y1; }
    }
    kick_check();
}

/* V02 debug switch depthview=<1|2|3>: 3D fragments get a depth or draw index colour instead of the combiner colour
 * (texture alpha kept, so cut-outs show). 1: depth, red near to blue far. 2: one colour per draw (flush) index.
 * 3: depth bands (repeating, 64 per range) to see depth order faults of near-equal surfaces. */
static unsigned dv_color(float z, float w, unsigned c) {
    float r, g, b, d;
    if (dview == 2) {
        unsigned h = dv_draw * 2654435761u;
        r = (float)((h >> 8) & 255) / 255.f; g = (float)((h >> 16) & 255) / 255.f; b = (float)((h >> 24) & 255) / 255.f;
    } else {
        d = (w > 0.f ? z / w : 0.f) * 0.5f + 0.5f;
        if (d < 0.f) d = 0.f;
        if (d > 1.f) d = 1.f;
        if (dview == 3) {
            float f = d * 64.f;
            f -= (float)(int)f;
            r = f; g = 1.f - f; b = d;
        } else {
            r = d; g = 1.f - (d < 0.5f ? 1.f - 2.f * d : 2.f * d - 1.f); b = 1.f - d;
        }
    }
    return ((unsigned)(r * 255.f) << 24) | ((unsigned)(g * 255.f) << 16) | ((unsigned)(b * 255.f) << 8) | (c & 255u);
}

static void put4(float x, float y, float z, float w, float u, float v, unsigned c) {
    if (dview < 0) {
        char vv[8];
        dview = pm_debug_value("depthview", vv, sizeof vv) ? atoi(vv) : 0;
    }
    if (dview && draw_3d && !rect_2d) c = dv_color(z, w, c);
    if (nverts >= 510) {
        int id = batch_identity;
        flush();
        batch_identity = id;
    }
    if (!verts || verts + nverts >= vert_base + VERT_SLOTS) {
        vert_drops++;
        return;
    }
    Vert* p = &verts[nverts++];
    p->x = x;
    p->y = y;
    p->z = z;
    p->w = w;
    p->u = u;
    p->v = v;
    p->r = pm_f255[(c >> 24) & 255];
    p->g = pm_f255[(c >> 16) & 255];
    p->b = pm_f255[(c >> 8) & 255];
    p->a = pm_f255[c & 255];
}

static void put(float x, float y, float z, float u, float v, unsigned c) {
    put4(x, y, z, 1.f, u, v, c);
}

/* PG3 O11: byte to float tables, filled with the same divide the code used, so every value is equal. The VFP divide costs about 19 cycles on the O3DS. */
float pm_f255[256], pm_f127[256]; /* pm_f127 is indexed by the byte of a signed char */
static void init_f_tables(void) {
    for (int i = 0; i < 256; i++) { pm_f255[i] = (float)i / 255.f; pm_f127[i] = (float)(signed char)i / 127.f; }
}

void pm_gpu_init(void) {
    init_f_tables();
    gp2_clip_init();
    if (gpu_ok) return;
    pm_wide = pm_wide_cfg = pm_debug_has("wide") || pm_opt.wide;
    pm_log("gpu: wide=%d", pm_wide);
    gfxInitDefault();
    if (!C3D_Init(0x180000)) { /* MEM1: 1.5 MB command buffer. The worst fill seen was 9 percent of 4 MB (369 KB), the new size keeps 4 times that. 2.5 MB of linear memory come back */
        pm_con_lock();
        consoleInit(GFX_BOTTOM, &bottom_con);
        printf("C3D_Init failed. No graphics.");
        pm_con_unlock();
        pm_log_e("C3D_Init failed (command buffer 1.5MB), no graphics");
        return;
    }
    /* Width 240, height 400: that is the top screen's rotated framebuffer.
     * A 320-tall target copied into it skews every scanline. */
    target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (!target) {
        pm_log_e("render target failed\n");
        return;
    }
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT,
        GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    dvlb_all = DVLB_ParseFile((u32*)pm_shaderall_shbin, pm_shaderall_shbin_size); /* RAW1: one DVLB for the programs pm (CPU vertices) and pm4 (raw Vtx, matrix palette): a program switch sends no shader code */
    raw_dvlb_ok = dvlb_all && dvlb_all->numDVLE >= 2;
    pm_log("raw1: one DVLB %s (%d entries)", raw_dvlb_ok ? "on" : "FAILED", dvlb_all ? (int)dvlb_all->numDVLE : 0);
    if (!raw_dvlb_ok) { pm_log_e("shader library failed"); return; }
    dvlb = dvlb_all;
    shaderProgramInit(&prog);
    shaderProgramSetVsh(&prog, &dvlb->DVLE[0]);
    C3D_BindProgram(&prog);
    u_proj = shaderInstanceGetUniformLocation(prog.vertexShader, "projection");
    u_uvx = shaderInstanceGetUniformLocation(prog.vertexShader, "uvx");
    u_nzo = shaderInstanceGetUniformLocation(prog.vertexShader, "nzo");
    /* y grows up, matching the citro3d immediate example. 320 game pixels sit in the middle. */
    Mtx_OrthoTilt(&projection, 0.f, 400.f, 0.f, 240.f, 0.f, 1.f, true);
    C3D_AttrInfo* ai = C3D_GetAttrInfo();
    AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 4);
    AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2);
    AttrInfo_AddLoader(ai, 2, GPU_FLOAT, 4);
    attr_cpu = *ai;
    { /* GV1: the raw Vtx path: a ring of the Vtx records of the frame and a u16 index ring */
        gring = (unsigned char*)linearAlloc((GRING_VTX + GSTAT_VTX) * 16);
        gidx = (unsigned short*)linearAlloc(GRING_IDX * 2);
        gpu_vtx_on = gring && gidx;
        if (gpu_vtx_on) { /* GV1c: the palette program */
            shaderProgramInit(&prog4);
            shaderProgramSetVsh(&prog4, &dvlb_all->DVLE[1]);
            u4_pal = shaderInstanceGetUniformLocation(prog4.vertexShader, "pal");
            u4_uvx = shaderInstanceGetUniformLocation(prog4.vertexShader, "uvx");
            u4_uvso = shaderInstanceGetUniformLocation(prog4.vertexShader, "uvso");
            u4_cmul = shaderInstanceGetUniformLocation(prog4.vertexShader, "cmul");
            u4_cadd = shaderInstanceGetUniformLocation(prog4.vertexShader, "cadd");
            u4_zmix = shaderInstanceGetUniformLocation(prog4.vertexShader, "zmix");
            u4_nzo = shaderInstanceGetUniformLocation(prog4.vertexShader, "nzo");
            AttrInfo_Init(&attr4);
            AttrInfo_AddLoader(&attr4, 0, GPU_SHORT, 4);
            AttrInfo_AddLoader(&attr4, 1, GPU_SHORT, 2);
            AttrInfo_AddLoader(&attr4, 2, GPU_UNSIGNED_BYTE, 4);
            rgen = (unsigned*)calloc(GRING_VTX, sizeof(unsigned));
            rk = (unsigned char*)calloc(GRING_VTX, 1);
            gv4_on = rgen && rk && prog4.vertexShader && u4_pal >= 0 && u4_uvso >= 0 && u4_cmul >= 0 && u4_cadd >= 0 && u4_zmix >= 0;
            if (gv4_on) { /* GV5 */
                dvlb5 = DVLB_ParseFile((u32*)pm_shader5_shbin, pm_shader5_shbin_size);
                shaderProgramInit(&prog5);
                if (dvlb5) shaderProgramSetVsh(&prog5, &dvlb5->DVLE[0]);
                u5_pal = shaderInstanceGetUniformLocation(prog5.vertexShader, "pal5");
                u5_lamb = shaderInstanceGetUniformLocation(prog5.vertexShader, "lamb");
                u5_lcol = shaderInstanceGetUniformLocation(prog5.vertexShader, "lcol");
                u5_ldir = shaderInstanceGetUniformLocation(prog5.vertexShader, "ldir");
                u5_lk0 = shaderInstanceGetUniformLocation(prog5.vertexShader, "lk0");
                u5_lk1 = shaderInstanceGetUniformLocation(prog5.vertexShader, "lk1");
                u5_lflag = shaderInstanceGetUniformLocation(prog5.vertexShader, "lflag");
                u5_fogu = shaderInstanceGetUniformLocation(prog5.vertexShader, "fogu");
                u5_uvso = shaderInstanceGetUniformLocation(prog5.vertexShader, "uvso");
                u5_cmul = shaderInstanceGetUniformLocation(prog5.vertexShader, "cmul");
                u5_cadd = shaderInstanceGetUniformLocation(prog5.vertexShader, "cadd");
                u5_nzo = shaderInstanceGetUniformLocation(prog5.vertexShader, "nzo");
                u5_zmix = shaderInstanceGetUniformLocation(prog5.vertexShader, "zmix");
                gv5_on = prog5.vertexShader && u5_pal >= 0 && u5_lamb >= 0 && u5_lcol >= 0 && u5_ldir >= 0 && u5_lk0 >= 0 && u5_lk1 >= 0 && u5_lflag >= 0 && u5_fogu >= 0 && u5_uvso >= 0 && u5_cmul >= 0 && u5_cadd >= 0 && u5_nzo >= 0 && u5_zmix >= 0;
                pm_log("gv5: lighting, texgen and fog program %s (pal %d lcol %d ldir %d)", gv5_on ? "on" : "FAILED", u5_pal, u5_lcol, u5_ldir);
            }
            pm_log("gv1: palette path %s (pal %d uvso %d cmul %d cadd %d zmix %d nzo %d)", gv4_on ? "on" : "FAILED", u4_pal, u4_uvso, u4_cmul, u4_cadd, u4_zmix, u4_nzo);
        }
        pm_log("gpuvtx: raw Vtx path %s (ring %u vtx, %u idx)", gpu_vtx_on ? "on" : "FAILED", (unsigned)GRING_VTX, (unsigned)GRING_IDX);
    }
    vert_base = (Vert*)linearAlloc(sizeof(Vert) * VERT_SLOTS);
    verts = vert_base;
    ident(game_mvp);
    rebuild_3d();
    pm_tex_init();
    /* consoleGetDefault() stays the uninitialized template. The console printf
     * writes is the one passed to consoleInit. */
    pm_con_lock();
    consoleInit(GFX_BOTTOM, &bottom_con);
    setvbuf(stdout, NULL, _IONBF, 0);
    consoleSetWindow(&bottom_con, 1, 3, 40, 28);
    pm_con_unlock();
    gpu_ok = vert_base != NULL;
    if (!vert_base) pm_log_e("linearAlloc for %u vertex slots failed, nothing will draw", (unsigned)VERT_SLOTS);
    pm_log_d("gpu %s proj %d m %d %d", gpu_ok ? "ok" : "no vertex buffer", u_proj,
           (int)(projection.r[0].y * 1000.f), (int)(projection.r[0].w * 1000.f));
}

/* F11-6, PT64, PT65: the port menu text on the bottom console. The menu owns rows 3 to 14 (MENU_ROWS lines), the log keeps rows 16 to 30 and keeps
 * scrolling there while the menu is open. Nothing is cleared: a line is rewritten, padded with spaces, only when its text changed, so the
 * screen never shows an empty frame between a clear and a redraw (PT64). On close the menu rows are blanked and the log gets its old window back. */
#define MENU_ROWS 30 /* PT69: the menu fills the whole bottom screen (rows 0 to 29); the log and the HUD wait while it is open */
#define MENU_W 39
static char menu_prev[MENU_ROWS][MENU_W + 1];
static int menu_on;
static char menu_l0[MENU_W + 1];
static void menu_put_line(PrintConsole* c, int i, const char* s) {
    char b[MENU_W + 1];
    snprintf(b, sizeof b, "%-*.*s", MENU_W - 1, MENU_W - 1, s); /* one column spare: the last column never wraps the 1 row window */
    if (menu_on && !strcmp(b, menu_prev[i])) return;
    memcpy(menu_prev[i], b, sizeof b);
    consoleSetWindow(c, 0, 0, 40, 30); /* PT79: the whole screen as the window and an absolute cursor move: a 1 row window on row 29 scrolled the text up by a row */
    printf("\x1b[%d;1H", i + 1);
    fputs(b, stdout);
}
void pm_gpu_menu_draw(const char* text) {
    PrintConsole* c = &bottom_con;
    int i = 0, was_on = menu_on, cx, cy, wx, wy, ww, wh;
    if (!c->consoleInitialised) return;
    pm_con_lock();
    cx = c->cursorX; cy = c->cursorY; wx = c->windowX; wy = c->windowY; ww = c->windowWidth; wh = c->windowHeight;
    consoleSelect(c);
    if (!text || !text[0]) {
        if (menu_on) {
            for (i = 0; i < MENU_ROWS; i++) { menu_prev[i][0] = 1; menu_put_line(c, i, ""); }
            menu_on = 0;
            consoleSetWindow(c, 1, 3, 40, 28); /* the log window again, new lines come at the bottom */
            c->cursorX = 0; c->cursorY = 27;
            fflush(stdout);
        }
        pm_con_unlock();
        return;
    }
    if (!menu_on) { consoleSetWindow(c, 0, 0, 40, 30); consoleClear(); for (int k = 0; k < MENU_ROWS; k++) menu_prev[k][0] = 1; } /* once at open: no old log text in the last columns */
    for (; i < MENU_ROWS; i++) { /* one text line per row, an empty line stays an empty row */
        const char* e = strchr(text, '\n');
        char ln[MENU_W + 1];
        size_t n = e ? (size_t)(e - text) : strlen(text);
        size_t k = n > MENU_W ? MENU_W : n;
        memcpy(ln, text, k);
        ln[k] = 0;
        if (i == 0) memcpy(menu_l0, ln, k + 1);
        menu_put_line(c, i, ln);
        text += e ? n + 1 : n;
    }
    for (; i < MENU_ROWS; i++) menu_put_line(c, i, "");
    menu_on = 1;
    if (!was_on) { menu_prev[0][0] = 1; menu_put_line(c, 0, menu_l0); } /* PT75: row 0 (the tab line) was lost on the first draw: write it again */
    fflush(stdout);
    (void)was_on;
    consoleSetWindow(c, wx, wy, ww, wh); /* the log window and its cursor stay as they were (the log is not printed while the menu is open) */
    c->cursorX = cx; c->cursorY = cy;
    pm_con_unlock();
}

static void hud_put_line(PrintConsole* c, int row, const char* s) {
    char b[40];
    snprintf(b, sizeof b, "%-39.39s", s ? s : "");
    c->cursorX = 1;
    c->cursorY = row;
    fputs(b, stdout);
}

void pm_gpu_hud(const char* line0, const char* line1) {
    PrintConsole* c = &bottom_con;
    int cx, cy, wx, wy, ww, wh;
    if (!c->consoleInitialised || menu_on) return; /* PT69: the menu owns all rows */
    /* The log writer thread prints on this console. Skip the HUD update when it is busy: the game thread never waits for it. */
    if (pm_con_trylock() != 0) return;
    cx = c->cursorX;
    cy = c->cursorY;
    wx = c->windowX;
    wy = c->windowY;
    ww = c->windowWidth;
    wh = c->windowHeight;
    consoleSelect(c);
    consoleSetWindow(c, 1, 1, 40, 2);
    hud_put_line(c, 1, line0);
    hud_put_line(c, 2, line1);
    consoleSetWindow(c, wx, wy, ww, wh);
    c->cursorX = cx;
    c->cursorY = cy;
    pm_con_unlock();
}

/* shots switch: dump the last finished frame (rotated 240x400 buffer, tiled RGBA8) as a PPM. */
int pm_debug_value(const char* key, char* out, unsigned n);
static int want_clear; /* X01: the clear runs at frame end, and only for a frame the game drew */
static int shot_burst;
void pm_gpu_shot_burst(int n) { shot_burst = n; }
static void dump_shot(void) {
    static unsigned n, frames;
    char path[64];
    FILE* f;
    unsigned char* row;
    const unsigned char* src;
    int x, y;
    static int skip = -1;
    if (!pm_debug_has("shots")) return;
    if (skip < 0) { /* shotskip=<n>: no shot in the first n frames (a later scene, such as the second intro after the demo) */
        char v[16];
        skip = pm_debug_value("shotskip", v, sizeof v) ? atoi(v) : 0;
    }
    {   /* X01: shotwin=<first frame>:<count> takes one shot per game frame (frame = pm_frame_count) */
        static int w0 = -2, w1;
        extern unsigned pm_frame_count(void);
        if (w0 == -2) {
            char v[24];
            w0 = -1;
            if (pm_debug_value("shotwin", v, sizeof v) && sscanf(v, "%d:%d", &w0, &w1) == 2) w1 += w0;
            else w0 = -1;
        }
        if (shot_burst > 0) {
            shot_burst--;
            pm_log_d("shot %u frame %u", n, pm_frame_count());
        } else if (w0 >= 0) {
            unsigned fc = pm_frame_count();
            if ((int)fc < w0 || (int)fc >= w1) return;
            pm_log_d("shot %u frame %u", n, fc);
        } else if (frames++ < (unsigned)skip || (frames % 30) != 0 || n >= 200) return;
    }
    src = (const unsigned char*)target->frameBuf.colorBuf;
    if (!src) return;
    mkdir("shots", 0777);
    snprintf(path, sizeof path, "shots/%04u.ppm", n++);
    f = fopen(path, "wb");
    if (!f) return;
    fputs("P6 240 400 255\n", f);
    row = (unsigned char*)malloc(240 * 3);
    for (y = 0; y < 400; y++) {
        for (x = 0; x < 240; x++) {
            unsigned mx = x & 7u, my = y & 7u;
            unsigned m = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) |
                         ((mx & 4u) << 2) | ((my & 4u) << 3);
            const unsigned char* p = src + ((((unsigned)(y >> 3) * 30u + (unsigned)(x >> 3)) * 64u + m) * 4u);
            row[x * 3] = p[3];
            row[x * 3 + 1] = p[2];
            row[x * 3 + 2] = p[1];
        }
        fwrite(row, 1, 240 * 3, f);
    }
    free(row);
    fclose(f);
}

/* D03: the last finished frame as N64 RGBA5551 (native u16, alpha bit set), 320x240, for game code that
 * reads the frame buffer. Reads the PICA color buffer (tiled RGBA8, 240 wide x 400 tall, the screen rotated;
 * game x is the long axis shifted by 40, game y runs backwards on the short axis). Costs one pass over 76800
 * pixels of VRAM, so it runs only when the game asks (pause screen, previous-frame draws). */
/* PT56: with wide the 40 screen columns on each side of the game picture (game x -40 to -1 and 320 to 359), saved with every frame read. The pause screen draws them beside the saved frame. Layout: [side][row * 40 + column], row 0 is the top. */
unsigned short pm_wide_side[2][40 * 240];
unsigned short pm_wide_side_pause[2][40 * 240]; /* the copy the pause screen draws (src/background_gfx.c) */
void pm_gpu_read_frame(unsigned short* dst) {
    if (rect_open) flush(); /* DR1 */
    const unsigned char* src;
    int x, y;
    if (!target || !dst) return;
    src = (const unsigned char*)target->frameBuf.colorBuf;
    if (!src) return;
    PH_PUSH(PH_FBREAD);
    for (y = 0; y < 240; y++) {
        unsigned fx = (unsigned)(239 - y);
        for (x = 0; x < 320; x++) {
            unsigned fy = (unsigned)(40 + x);
            unsigned mx = fx & 7u, my = fy & 7u;
            unsigned m = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) |
                         ((mx & 4u) << 2) | ((my & 4u) << 3);
            const unsigned char* p = src + ((((fy >> 3) * 30u + (fx >> 3)) * 64u + m) * 4u);
            dst[y * 320 + x] = (unsigned short)(((p[3] >> 3) << 11) | ((p[2] >> 3) << 6) | ((p[1] >> 3) << 1) | 1u);
        }
        if (pm_wide) {
            for (x = 0; x < 80; x++) {
                unsigned fy = (unsigned)(x < 40 ? x : 360 + (x - 40));
                unsigned mx = fx & 7u, my = fy & 7u;
                unsigned m = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) |
                             ((mx & 4u) << 2) | ((my & 4u) << 3);
                const unsigned char* p = src + ((((fy >> 3) * 30u + (fx >> 3)) * 64u + m) * 4u);
                pm_wide_side[x >= 40][y * 40 + (x % 40)] = (unsigned short)(((p[3] >> 3) << 11) | ((p[2] >> 3) << 6) | ((p[1] >> 3) << 1) | 1u);
            }
        }
    }
    PH_POP();
}

/* V05: start (or go back to) the off-screen pass for the colour image at base, width stride pixels. Returns 1 if
 * drawing goes there now. Each base gets one image for the frame (cleared on first use). */
extern int pm_is_cfb(const void* p);
int pm_gpu_off_begin(const void* base, int stride, int siz) {
    if (rect_open) flush(); /* DR1 */
    int i, w, h;
    if (!gpu_ok || off_fail || stride < 1 || stride > PM_OFF_W) return 0;
    if (off_active && offs[off_cur].base == base && offs[off_cur].siz == siz && (off_cur >= OFF_P || offs[off_cur].stride == (unsigned)stride)) return 1;
    flush();
    for (i = 0; i < OFF_ALL; i++) if (offs[i].base == base && offs[i].siz == siz) break;
    if (i < OFF_P && offs[i].stride != (unsigned)stride) {
        /* PT50: the same address with another row width is another image. The N64 draws two windows (item box: 280 x 32 and 136 x 40) one after the other into the same heap address. Here each window gets its own slot, so the second one does not draw over the first. The first slot stays valid for the draws that already read it. */
        offs[i].base = NULL;
        pm_tex_set_offscreen(i, NULL, 0, &offs[i].tex, siz, offs[i].w, offs[i].h);
        i = OFF_ALL;
    }
    w = stride <= 64 ? 64 : PM_OFF_W; /* X02: the darkness stencil is 64 x 64: 16 KB of VRAM, not 512 KB */
    h = stride <= 64 ? 64 : PM_OFF_H;
    if (i < OFF_ALL && i >= OFF_P && offs[i].last != pm_frame_count()) { /* FX2a: a kept 8 bit image, back in this frame with last frame's pixels */
        offs[i].last = pm_frame_count();
        pm_tex_set_offscreen(i, base, (unsigned)stride, &offs[i].tex, siz, offs[i].w, offs[i].h);
    }
    if (i == OFF_ALL) {
        if (siz == 1 && stride <= 64) {
            /* FX2a: the flame writes D_800B32E0 from its own last frame, so 8 bit images keep their slot and pixels. A slot not named in this frame is taken over. */
            int k;
            for (k = OFF_P; k < OFF_ALL && offs[k].base; k++) {}
            if (k == OFF_ALL) for (k = OFF_P; k < OFF_ALL && offs[k].last == pm_frame_count(); k++) {}
            if (k == OFF_ALL) { pm_log_w("FX2a: more than 2 8 bit images in one frame, the rest draw on the screen"); return 0; }
            i = k;
        } else if (w == 64 ? off_sused >= OFF_N : off_used >= OFF_N) {
            static int once;
            if (!once++) pm_log_w("V05: more than %d off-screen images in one frame, the rest draw on the screen", OFF_N);
            return 0;
        } else i = w == 64 ? OFF_N + off_sused++ : off_used++;
        if (!offs[i].rt) {
            if (!C3D_TexInitVRAM(&offs[i].tex, w, h, GPU_RGBA8)) {
                pm_log_e("V05: off-screen image %d: no VRAM, off-screen passes stay on the screen", i);
                off_fail = 1;
                return 0;
            }
            C3D_TexSetFilter(&offs[i].tex, GPU_LINEAR, GPU_NEAREST);
            C3D_TexSetWrap(&offs[i].tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
            offs[i].rt = C3D_RenderTargetCreateFromTex(&offs[i].tex, GPU_TEXFACE_2D, 0, -1);
            if (!offs[i].rt) {
                pm_log_e("V05: off-screen image %d: render target failed", i);
                C3D_TexDelete(&offs[i].tex);
                off_fail = 1;
                return 0;
            }
            offs[i].w = w;
            offs[i].h = h;
        }
        C3D_RenderTargetClear(offs[i].rt, C3D_CLEAR_COLOR, 0, 0);
        offs[i].base = base;
        offs[i].siz = siz;
        offs[i].stride = (unsigned)stride;
        offs[i].last = pm_frame_count();
        /* F07-7: the wide grab slot starts at screen column 0, game x 0 is column 40: a read at cfb + x finds column 40 + x */
        pm_tex_set_offscreen(i, (pm_wide && siz == 2 && stride == 320 && pm_is_cfb(base)) ? (const u8*)base - 80 : base, (unsigned)stride, &offs[i].tex, siz, offs[i].w, offs[i].h);
        if (w == 64) { static int n; if (n++ < 6) pm_log_d("X02: darkness draw to 64x64 slot %d size %d bits %d addr %p", i, stride, siz == 3 ? 32 : siz == 2 ? 16 : 8, base); }
    }
    off_cur = i;
    off_w = offs[i].w;
    off_h = offs[i].h;
    off_i8 = siz == 1;
    Mtx_Ortho(&off_proj, 0.f, (float)off_w, 0.f, (float)off_h, 0.f, 1.f, true);
    off_active = 1;
    rebuild_3d();
    sh_clear(); C3D_FrameDrawOn(offs[i].rt);
    return 1;
}

/* V05: back to the screen. */
int pm_gpu_off_count(void) { return off_used + off_sused; }
void pm_gpu_off_end(void) {
    if (rect_open) flush(); /* DR1 */
    if (!off_active) return;
    flush();
    off_active = 0;
    off_i8 = 0;
    rebuild_3d();
    sh_clear(); C3D_FrameDrawOn(target);
}

void pm_gpu_set_replace(int on) { if (on != r_replace) flush(); r_replace = on; } /* DR1: a queued rectangle batch keeps the blender it was queued with */
void pm_gpu_set_memblend(int on) { if (on == r_memblend) return; flush(); r_memblend = on; }

/* B2: T2 grab. The PICA cannot sample the target it draws to, and a CPU read mid-frame sees the last frame (the GPU has not run
 * yet). So: draw what is pending, split the command list, copy the strips of the main target (GX_TextureCopy, queued after the
 * split part, so it sees every draw so far), then draw the copy rotated upright into the slot (cfb, siz 2). Later loads of that
 * address find the slot (T1). citro3d stops the GX queue in a frame and runs it at C3D_FrameEnd, so the order is the order of
 * these calls. The end-of-frame clear of the main target is a queue entry too: it has to come before the first split. */
static int grab_impl(const void* cfb, int y0, int y1, int prev_frame);
static int prev_want, prev_pre; /* PT12: see pm_gpu_grab_prev */
static int grab_tex_init(void) {
    if (grab_ready) return grab_ready > 0;
    if (!C3D_TexInitVRAM(&grab_tex, 256, 512, GPU_RGBA8)) {
        pm_log_e("B2: no VRAM for the 256 x 512 grab copy, grabs stay off");
        grab_ready = -1;
        return 0;
    }
    C3D_TexSetFilter(&grab_tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&grab_tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    grab_ready = 1;
    return 1;
}
/* queue the copy of the colour buffer strips (all 400 screen columns with wide, else the 320 game columns) into grab_tex */
static void grab_queue_copy(void) {
    const int mo = pm_wide ? 0 : 40, x1 = pm_wide ? 400 : 320;
    int s0 = mo >> 3, s1 = (mo + x1 - 1) >> 3;
    GX_TextureCopy((u32*)((u8*)target->frameBuf.colorBuf + s0 * 7680), GX_BUFFER_DIM(7680 / 16, 0),
                   (u32*)((u8*)grab_tex.data + s0 * 8192), GX_BUFFER_DIM(7680 / 16, 512 / 16), (u32)(s1 - s0 + 1) * 7680, BIT(3));
}
int pm_gpu_grab(const void* cfb, int y0, int y1) { return grab_impl(cfb, y0, y1, 0); }
/* PT12: role T3. The old frame buffer is the finished frame that is still in the colour buffer at the start of the frame. The clear of the
 * main target is queued at the first split (KICK1 makes one at 55 percent of the draws, the overlay comes after it) and wipes that frame.
 * So a request sets prev_want, and the next frame queues the copy into grab_tex at pm_gpu_begin, before any other entry (prev_pre). The
 * first frame of a run of requests, or a frame with an earlier T2 grab (it overwrites grab_tex), returns 0: the caller reads with the CPU.
 * Returns 2 if the old frame is in the slot of prev_cfb. */
int pm_gpu_grab_prev(const void* prev_cfb) {
    prev_want = 1;
    if (!target || !gpu_ok) return 0;
    return grab_impl(prev_cfb, 0, 240, 1);
}
static int grab_impl(const void* cfb, int y0, int y1, int prev_frame) {
    if (rect_open) flush(); /* DR1 */
    int was_off = off_active, prev = off_cur;
    static int once;
    /* F07-7: with wide the slot holds all 400 screen columns (slot x = screen x). The game reads game x at slot x + 40 (pm_gpu_off_begin shifts the base). */
    const int x0 = 0, x1 = pm_wide ? 400 : 320, mo = pm_wide ? 0 : 40;
    if (!gpu_ok || off_fail || grab_ready < 0 || y1 <= y0) return 0;
    if (grab_n && !prev_frame) {
        /* no draw since the last grab, or only rectangles of other rows: same slot */
        if (!grab_dirty && (grab_dy1 <= grab_dy0 || y1 <= grab_dy0 || y0 >= grab_dy1)) return 1;
        if (grab_n >= GRAB_MAX) {
            if (!once++) pm_log_w("B2: more than %d grabs in one frame, the rest reuse the last one", (int)GRAB_MAX);
            return 1;
        }
    }
    flush();
    if (prev_frame && (!prev_pre || grab_n)) return 0;
    if (!grab_tex_init()) return 0;
    if (!pm_gpu_off_begin(cfb, 320, 2)) return 0; /* no free slot: the load reads the frame buffer memory as before */
    pm_gpu_off_end(); /* the slot exists (cleared on first use); the copy goes before the quad */
    if (!prev_frame) {
        if (!grab_split++ && want_clear) { /* queue entry order: clear, part 1, copy, part 2 */
            C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0);
            want_clear = 0;
        }
        pm_tex_flush_pend_only();
        sh_clear(); C3D_FrameSplit(GX_CMDLIST_FLUSH);
    }
    if (!prev_frame) grab_queue_copy(); /* PT12: the old frame was copied at pm_gpu_begin */
    pm_gpu_off_begin(cfb, 320, 2);
    {
        int sc[4] = { sci_x0, sci_y0, sci_x1, sci_y1 };
        int s_comb = combine, s_2d = rect_2d, s_bi = batch_identity, s_rep = r_replace, s_at = r_atest, s_nc = r_nocol, s_mix = r_mix,
            s_rs = rgb_scale, s_3d = draw_3d;
        float xa = (float)x0, xb = (float)x1, va = 1.f - ((float)mo + xa) / 512.f, vb = 1.f - ((float)mo + xb) / 512.f, ua = 240.f / 256.f;
        sci_x0 = 0; sci_y0 = 0; sci_x1 = x1; sci_y1 = 240;
        combine = 3; rect_2d = 1; batch_identity = 1; r_replace = 1; r_atest = -1; r_nocol = 0; r_mix = 0; rgb_scale = 1; draw_3d = 0;
        grab_bind = 1;
        /* game (x, y) reads texel column 239 - y, memory row 40 + x of the copy. v = 0 samples the last memory row (found with
         * the grabtest dump: with v = row / 512 the slot came out mirrored in x), so v = 1 - row / 512. */
        pm_gpu_tri(xa, 0.f, 0.5f, ua, va, 0xffffffffu, xb, 0.f, 0.5f, ua, vb, 0xffffffffu, xb, 240.f, 0.5f, 0.f, vb, 0xffffffffu);
        pm_gpu_tri(xa, 0.f, 0.5f, ua, va, 0xffffffffu, xb, 240.f, 0.5f, 0.f, vb, 0xffffffffu, xa, 240.f, 0.5f, 0.f, va, 0xffffffffu);
        flush();
        grab_bind = 0;
        sci_x0 = sc[0]; sci_y0 = sc[1]; sci_x1 = sc[2]; sci_y1 = sc[3];
        combine = s_comb; rect_2d = s_2d; batch_identity = s_bi; r_replace = s_rep; r_atest = s_at; r_nocol = s_nc; r_mix = s_mix;
        rgb_scale = s_rs; draw_3d = s_3d;
    }
    if (was_off) pm_gpu_off_begin(offs[prev].base, offs[prev].w == 64 ? 64 : 320, offs[prev].siz);
    else pm_gpu_off_end();
    if (!prev_frame) { /* PT12: the old frame in grab_tex does not stand for the current frame */
        grab_dirty = 0;
        grab_dy0 = grab_dy1 = 0;
        grab_n++;
    }
    return 2;
}

/* V05 debug switch offdump: the first off-screen image of the last frame, as shots/off<frame>.ppm (memory row 0 on top) */
static void off_dump(void) {
    static int on = -1, n, from;
    int k;
    char v[16];
    FILE* f;
    unsigned char* row;
    const unsigned char* src;
    if (on < 0) { from = pm_debug_value("offdump", v, sizeof v) ? atoi(v) : 0; on = from || pm_debug_has("offdump"); } /* offdump=<first frame> */
    if (!on || !(off_used + off_sused || offs[OFF_P].base || offs[OFF_P + 1].base) || n >= 12 || (int)pm_frame_count() < from) return;
    /* X02: every slot of the frame, 64 x 64 slots too: off<frame>_<slot>.ppm */
  for (k = 0; k < OFF_ALL && n < 12; k++) {
    if (!offs[k].base) continue;
    src = (const unsigned char*)offs[k].tex.data;
    mkdir("shots", 0777);
    char path[48];
    snprintf(path, sizeof path, "shots/off%03u_%d.ppm", pm_frame_count(), k);
    n++;
    f = fopen(path, "wb");
    if (!f) continue;
    fprintf(f, "P6 %d %d 255\n", offs[k].w, offs[k].h);
    row = (unsigned char*)malloc(offs[k].w * 3);
    for (int y = 0; y < offs[k].h; y++) {
        for (int x = 0; x < offs[k].w; x++) {
            unsigned mx = x & 7u, my = y & 7u;
            unsigned m = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) | ((mx & 4u) << 2) | ((my & 4u) << 3);
            const unsigned char* p = src + ((((unsigned)(y >> 3) * (offs[k].w / 8) + (unsigned)(x >> 3)) * 64u + m) * 4u);
            unsigned a = p[0];
            row[x * 3] = (unsigned char)(p[3] * a / 255); row[x * 3 + 1] = (unsigned char)(p[2] * a / 255); row[x * 3 + 2] = (unsigned char)(p[1] * a / 255);
            if (a == 0) row[x * 3] = row[x * 3 + 1] = row[x * 3 + 2] = 40;
            if (offs[k].siz == 1) row[x * 3] = row[x * 3 + 1] = row[x * 3 + 2] = (unsigned char)p[3]; /* FX2a: an 8 bit image shows its intensity */
        }
        fwrite(row, 1, offs[k].w * 3, f);
    }
    free(row);
    fclose(f);
  }
}

void pm_gpu_begin(int clear) {
    off_dump();
    for (int k = 0; k < OFF_P; k++) offs[k].base = NULL; /* FX2a: the 8 bit slots keep their base */
    off_used = off_sused = 0;
    off_active = 0;
    off_i8 = 0;
    grab_dirty = 1;
    grab_dy0 = grab_dy1 = grab_n = grab_split = 0;
    pm_tex_clear_offscreen();
    dv_draw = 0;
    draws_frame = 0;
    drew = 0;
    nverts = 0;
    gni = 0; gidx_n = 0; gi_start = 0; gring_n = 0; pal_n = 0; post_gen++; /* PO3b, GV1c */
    verts = vert_base;
    v_fl = vert_base; gring_fl = gidx_fl = 0; kick_done = 0; /* GV0, KICK1 */
    sci_x0 = 0;
    sci_y0 = 0;
    sci_x1 = 320;
    sci_y1 = 240;
    draw_3d = 0;
    if (!gpu_ok) return;
    pm_tex_upload_vram_before_frame();
    PH_PUSH(PH_FBEGIN); /* C3D_FRAME_SYNCDRAW: waits for the previous frame on the GPU */
    raw1_reset(); sh_clear(); C3D_FrameBegin(pm_interp_on > 0 ? C3D_FRAME_SYNCDRAW : 0); /* PT88: at 30 steps the loop paces itself by time and the frame begin waits only for the GPU (the previous frame, so the buffers are free); with fps60 the begin waits for the vblank as before: two draws a step, one per vblank */
    PH_POP();
    pm_ph_gpu_note();
    dump_shot();
    PH_PUSH(PH_TEX);
    pm_tex_begin_frame();
    PH_POP();
    PH_PUSH(PH_GPU);
    want_clear = clear;
    noise_off[0] = (float)(noise_next() & 1023u) / 1024.f; noise_off[1] = (float)(noise_next() & 1023u) / 1024.f; /* PT07: one new offset per drawn frame */
    prev_pre = 0;
    if (prev_want && !pm_debug_has("prevcpu") && grab_tex_init()) { grab_queue_copy(); prev_pre = 1; } /* PT12: first entry of the queue, before the clear */
    prev_want = 0;
    sh_clear(); C3D_FrameDrawOn(target);
    PH_POP();
}

void pm_gpu_end(void) {
    if (!gpu_ok) return;
    flush();
    if (off_active) pm_gpu_off_end();
    if (drew && pm_debug_has("grabtest")) { /* B2 test switch: grab the whole finished frame, `offdump` then shows it next to the shot */
        extern unsigned short* nuGfxCfb_ptr;
        pm_gpu_grab(nuGfxCfb_ptr, 0, 240);
    }
    /* X01: a frame the game did not draw (it skips gfx_draw_frame while the pause screen takes the picture) keeps the last
     * picture on screen, as the N64 does when no task swaps the buffer. A clear here would show black for that frame. The clear
     * is a GX memory fill that runs before the command list queued by C3D_FrameEnd, so it can come after the draw calls. */
    if (want_clear && drew) C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0);
    /* PO1: the GPU reads the vertex buffer when C3D_FrameEnd runs the command list. One flush of the
     * whole range written this frame replaces one GSPGPU call per draw. Each frame starts at vert_base
     * again, and FrameBegin(SYNCDRAW) has waited for the GPU, so no draw of this frame overlaps the
     * writes. */
    gpu_flush_written(); /* GV0: pool, raw Vtx ring and index ring, what a split has not flushed already */
    {
        unsigned used = verts ? (unsigned)(verts - vert_base) : 0;
        float cmd = C3D_GetCmdBufUsage();
        static unsigned warned;
        frames_seen++;
        if (frames_seen % 300 == 0) pm_log_hw("dr1 f%u merged %u splits %u uniform skips %u rect breaks %u", frames_seen, dr1_merged, kick_n, dr1_skips, dr1_breaks);
        if (draws_frame > draws_max) draws_max = draws_frame;
        prev_draws = draws_frame; kick_at = draws_frame >= 48 ? draws_frame * 55 / 100 : 0; /* KICK1 */
        if (used > verts_max) verts_max = used;
        if (cmd > cmd_max) cmd_max = cmd;
        pm_ph_cmd_pm = (unsigned)(cmd * 1000.f);
        if ((used * 10u > VERT_SLOTS * 9u || cmd > 0.9f || vert_drops) && (warned < 5 || frames_seen % 600 == 0)) {
            warned++;
            pm_log_w("GPU LIMIT NEAR: vertex slots %u of %u (dropped %u), command buffer %d%%, draws %u", used,
                   (unsigned)VERT_SLOTS, vert_drops, (int)(cmd * 100.f), draws_frame);
        }
    }
    pm_gbi_end_frame();
    /* Without this flag citro3d flushes the entire linear heap. That heap is
     * the 20MB port pool, so every frame was writing it back through the cache. */
    PH_PUSH(PH_FEND);
    C3D_FrameEnd(GX_CMDLIST_FLUSH);
    PH_POP();
}

void pm_gpu_rect_sync(void) { if (rect_open) flush(); }
int pm_gpu_drew(void) { if (rect_open) flush(); return drew; }

void pm_gpu_log_stats(const char* tag) {
    pm_log_d("mem %s gpu max per frame: draws %u, vertex slots %u of %u, command buffer %d%%, vertex drops (total) %u", tag, draws_max,
           verts_max, (unsigned)VERT_SLOTS, (int)(cmd_max * 100.f), vert_drops);
    draws_max = verts_max = 0;
    cmd_max = 0.f;
}

/* Fatal boot problem (no ROM, wrong ROM): show it on the bottom screen and wait for START. Used before the GPU is up. */
void pm_fatal_screen(const char* msg) {
    pm_log_e("FATAL: %s", msg);
    pm_con_lock();
    if (!bottom_con.consoleInitialised) {
        gfxInitDefault();
        consoleInit(GFX_BOTTOM, &bottom_con);
    } else {
        consoleSelect(&bottom_con);
        consoleSetWindow(&bottom_con, 0, 0, 40, 30);
    }
    consoleClear();
    printf("Paper Mario 3DS cannot start.%c%c%s%c%cPress START to exit.%c", 10, 10, msg, 10, 10, 10);
    fflush(stdout);
    pm_con_unlock();
    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
    }
}

void pm_gpu_set_mvp(const float m[4][4], float sx, float sy, float tx, float ty) {
    static const float id4[4][4] = { {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1} };
    /* F07-3: orthographic = no perspective divide. The clipped-polygon path passes identity and keeps the flag of the last real matrix. */
    int ortho = memcmp(m, id4, sizeof(id4)) == 0 ? ui_ortho : (m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f);
    if (sx == vp_sx && sy == vp_sy && tx == vp_tx && ty == vp_ty && ortho == ui_ortho &&
        memcmp(game_mvp, m, sizeof(game_mvp)) == 0) return;
    if (draw_3d && (nverts > 0 || gni > 0)) flush();
    ui_ortho = ortho;
    memcpy(game_mvp, m, sizeof(game_mvp));
    vp_sx = sx;
    vp_sy = sy;
    vp_tx = tx;
    vp_ty = ty;
    rebuild_3d();
}

/* PO3b: copy n raw Vtx records into the frame ring. Returns the ring index of the first one, or -1 (path off or ring full). */
const unsigned char* pm_gpu_gring(void) { return gring; }
unsigned pm_gpu_flush_gen(void) { return flush_gen; }
/* GV1b: add one triangle to the open raw batch. The caller knows that the batch has the same draw parameters (it saw no flush since its last gtrip). The
 * vertices must already carry a flag of the palette of this batch. */
int pm_gpu_gappend(unsigned i0, unsigned i1, unsigned i2, unsigned g0, unsigned g1, unsigned g2) {
    if (gni == 0 || gidx_n + 3 > GRING_IDX || !g_pal) return 0;
    const unsigned r3[3] = { i0, i1, i2 }, g3[3] = { g0, g1, g2 };
    for (int i = 0; i < 3; i++) {
        unsigned r = r3[i];
        if (rgen[r] == pal_gen) continue;
        unsigned k; for (k = 0; k < pal_n; k++) if (pal_g[k] == g3[i]) break;
        if (k == pal_n) return 0; /* a matrix that is not in the palette yet: the slow path adds it */
        if (rgen[r] != 0 && rk[r] == k) { rgen[r] = pal_gen; continue; }
        if (rgen[r] == 0 && r >= gring_fl) {
            *(unsigned short*)(gring + 16u * r + 6) = (unsigned short)k;
            rgen[r] = pal_gen; rk[r] = (unsigned char)k;
            continue;
        }
        return 0; /* needs a copy: the slow path */
    }
    gidx[gidx_n++] = (unsigned short)i0;
    gidx[gidx_n++] = (unsigned short)i1;
    gidx[gidx_n++] = (unsigned short)i2;
    gni += 3;
    return 1;
}
int pm_gpu_gv_wind(void) { return gv_wind; }
/* GV5: the PICA clip z is a * z + b * w of the N64 clip position and w stays w, when the matrix after the game matrix is a plain depth map. Returns 0 when it is not. */
int pm_gpu_gv_zcoef(float* a, float* b) {
    const C3D_FVec *z = &gv_post.r[2], *w = &gv_post.r[3];
    const float e = 1e-5f;
    if (!(gv_post.r[2].z > e || gv_post.r[2].z < -e)) return 0;
    if (z->x > e || z->x < -e || z->y > e || z->y < -e) return 0;
    if (w->x > e || w->x < -e || w->y > e || w->y < -e || w->z > e || w->z < -e || w->w < 1.f - e || w->w > 1.f + e) return 0;
    if (a) *a = z->z;
    if (b) *b = z->w;
    return 1;
}
void pm_gpu_gv_post(const float p[4], float o[4]) { /* gv1check: the clip position after the viewport and tilt (the game matrix is already in p) */
    for (int r = 0; r < 4; r++) o[r] = gv_post.r[r].x * p[0] + gv_post.r[r].y * p[1] + gv_post.r[r].z * p[2] + gv_post.r[r].w * p[3];
}
void pm_gpu_gv_reset(void) { post_gen++; }
void pm_gpu_gv_stats(unsigned o[4]) { o[0] = gv_palup_n; o[1] = gv_palmax; o[2] = gv_palflush; o[3] = gv_recopy_n; }
/* GV1c: G_MTX in gv1 mode. The game matrix is in the palette of the triangles, so a matrix change closes no batch. A change of the viewport or of the orthographic
 * flag still does. */
void pm_gpu_note_mvp(const float m[4][4], float sx, float sy, float tx, float ty) {
    int ortho = m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f;
    if (sx == vp_sx && sy == vp_sy && tx == vp_tx && ty == vp_ty && ortho == ui_ortho) return;
    pm_gpu_set_mvp(m, sx, sy, tx, ty);
}

static int gtrip_batch_open(int use5, const PmFx* fx, const float vp[4], const float uvso[4], const float cmul[4], const float cadd[4], const float zmix[2], int ct,
                            const unsigned g[3]) {
    if (!g_pal || use5 != g_use5) return 0;
    if (use5 && memcmp(&g_fx, fx, sizeof g_fx)) return 0;
    if (memcmp(g_uvso, uvso, sizeof(g_uvso)) || memcmp(g_vp, vp, sizeof(g_vp)) || memcmp(g_cmul, cmul, sizeof(g_cmul)) || memcmp(g_cadd, cadd, sizeof(g_cadd)) ||
        memcmp(g_zmix, zmix, sizeof(g_zmix)) || g_ct != ct)
        return 0;
    unsigned miss = 0;
    for (int i = 0; i < 3; i++) {
        unsigned k; for (k = 0; k < pal_n; k++) if (pal_g[k] == g[i]) break;
        if (k == pal_n && !(i > 0 && g[i] == g[0]) && !(i > 1 && g[i] == g[1])) miss++;
    }
    return pal_n + miss <= (use5 ? GPAL5 : GPAL);
}

/* GV1c: one triangle of ring vertices ri[3], vertex i with game matrix tab[g[i]]. ro[3] gets the ring indices to use from now on (a vertex whose palette index
 * has to change is copied). Returns 0 when the raw path cannot take it now. */
int pm_gpu_gtrip(const float (*tab)[4][4], const unsigned g[3], const float vp[4], const float uvso[4], const float cmul[4], const float cadd[4], const float zmix[2], int ct,
                 const PmFx* fx, const unsigned ri[3], unsigned ro[3]) {
    if (!gv4_on || !gpu_ok) return 0;
    const int use5 = fx != NULL;
    if (use5 && (!gv5_on || (combine == 8 && plan.uses_tex1))) return 0; /* pm5 has no second texture coordinate */
    if (r_zmark) return 0; /* B3b depth pass: the second pass needs the CPU vertices */
    if (r_decal && r_blend) return 0; /* V16b: a decal is drawn in two passes from the CPU vertices */
    if (pm_wide && !off_active) { /* F07-7: the CPU path moves edge vertices of UI quads. The wide post matrix is made for a perspective game matrix. */
        if (ui_ortho) return 0;
        for (int i = 0; i < 3; i++) { const float(*m)[4] = tab[g[i]]; if (m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f) return 0; }
    }
    if (gidx_n + 3 > GRING_IDX) return 0;
    if (!draw_3d) { flush(); draw_3d = 1; }
    if (nverts > 0) flush(); /* the CPU batch in flight comes first, draw order */
    if (gni > 0 && !gtrip_batch_open(use5, fx, vp, uvso, cmul, cadd, zmix, ct, g)) {
        if (pal_n) { /* count a palette overflow only when the matrices do not fit */
            unsigned miss = 0;
            for (int i = 0; i < 3; i++) {
                unsigned k; for (k = 0; k < pal_n; k++) if (pal_g[k] == g[i]) break;
                if (k == pal_n && !(i > 0 && g[i] == g[0]) && !(i > 1 && g[i] == g[1])) miss++;
            }
            if (pal_n + miss > (use5 ? GPAL5 : GPAL)) gv_palflush++;
        }
        flush();
    }
    if (gni == 0) {
        memcpy(g_uvso, uvso, sizeof(g_uvso));
        memcpy(g_vp, vp, sizeof(g_vp));
        memcpy(g_cmul, cmul, sizeof(g_cmul));
        memcpy(g_cadd, cadd, sizeof(g_cadd));
        memcpy(g_zmix, zmix, sizeof(g_zmix));
        g_ct = ct; g_pal = 1; pal_n = 0; pal_gen++;
        g_use5 = use5; if (use5) g_fx = *fx;
        if (vp[0] != vp_sx || vp[1] != vp_sy || vp[2] != vp_tx || vp[3] != vp_ty) { vp_sx = vp[0]; vp_sy = vp[1]; vp_tx = vp[2]; vp_ty = vp[3]; rebuild_3d(); }
        if (ct) {
            int ccw = gv_wind <= 0; /* golden yard and toad: the first sign was inverted. */
            if (ct == 2) ccw = !ccw;
            g_cullm = ccw ? GPU_CULL_BACK_CCW : GPU_CULL_FRONT_CCW;
        } else g_cullm = GPU_CULL_NONE;
    }
    pal_tab = tab;
    unsigned k3[3];
    for (int i = 0; i < 3; i++) {
        unsigned k; for (k = 0; k < pal_n; k++) if (pal_g[k] == g[i]) break;
        if (k == pal_n) { pal_g[pal_n++] = g[i]; if (pal_n > gv_palmax) gv_palmax = pal_n; }
        k3[i] = k;
    }
    for (int i = 0; i < 3; i++) {
        unsigned r = ri[i];
        ro[i] = r;
        if (rgen[r] == pal_gen) continue; /* in this palette already, with the right index */
        if (rgen[r] != 0 && rk[r] == k3[i]) { rgen[r] = pal_gen; continue; } /* an older palette gave it the same index */
        if (rgen[r] == 0 && r >= gring_fl) { /* first use, and the GPU has not seen the ring slot */
            *(unsigned short*)(gring + 16u * r + 6) = (unsigned short)k3[i];
            rgen[r] = pal_gen; rk[r] = (unsigned char)k3[i];
            continue;
        }
        if (gring_n + 1 > GRING_VTX) return 0;
        {   /* the slot is in use by another palette index, or already flushed: use a copy */
            unsigned n = gring_n++;
            memcpy(gring + 16u * n, gring + 16u * r, 16);
            *(unsigned short*)(gring + 16u * n + 6) = (unsigned short)k3[i];
            rgen[n] = pal_gen; rk[n] = (unsigned char)k3[i];
            ro[i] = n;
            gv_recopy_n++;
        }
    }
    gidx[gidx_n++] = (unsigned short)ro[0];
    gidx[gidx_n++] = (unsigned short)ro[1];
    gidx[gidx_n++] = (unsigned short)ro[2];
    gni += 3;
    return 1;
}

/* DLC2: static vertex area. A compiled shape group copies its Vtx here once (flag 0 = palette entry 0). The area is rewritten only after pm_gpu_gstat_reset (a new
 * map), which runs after C3D_FrameBegin(SYNCDRAW) has waited for the GPU. Returns the first vertex index (GRING_VTX and up) or -1. */
int pm_gpu_gstat_alloc(unsigned n, unsigned char** out) {
    if (!gpu_vtx_on || !gpu_ok || !gv4_on || gstat_n + n > GSTAT_VTX) return -1;
    unsigned first = GRING_VTX + gstat_n;
    gstat_n += n;
    *out = gring + 16u * first;
    return (int)first;
}
void pm_gpu_gstat_commit(unsigned first, unsigned n) { GSPGPU_FlushDataCache(gring + 16u * first, n * 16u); }
void pm_gpu_gstat_reset(void) { if (gstat_n) { flush(); gstat_n = 0; } }
unsigned pm_gpu_gstat_used(void) { return gstat_n; }

/* DLC2: nidx indices of static vertices (all flag 0) drawn with the game matrix tab[gm]. The batch palette must hold gm at entry 0, else a new batch starts.
 * Returns 0 when the raw path cannot take it (nothing changed, the caller draws the group through the ring). */
static int grange_batch_open(unsigned gm, const float vp[4], const float uvso[4], const float cmul[4], const float cadd[4], const float zmix[2], int ct, const PmFx* fx, int use5) {
    if (gni == 0 || !g_pal || pal_n == 0 || pal_g[0] != gm || g_ct != ct || use5 != g_use5) return 0;
    if (use5 && memcmp(&g_fx, fx, sizeof g_fx)) return 0;
    if (memcmp(g_uvso, uvso, sizeof(g_uvso)) || memcmp(g_vp, vp, sizeof(g_vp)) || memcmp(g_cmul, cmul, sizeof(g_cmul)) || memcmp(g_cadd, cadd, sizeof(g_cadd)) ||
        memcmp(g_zmix, zmix, sizeof(g_zmix)))
        return 0;
    return 1;
}

int pm_gpu_grange(const float (*tab)[4][4], unsigned gm, const float vp[4], const float uvso[4], const float cmul[4], const float cadd[4], const float zmix[2], int ct,
                  const PmFx* fx, const unsigned short* idx, unsigned nidx) {
    if (!gv4_on || !gpu_ok) return 0;
    const int use5 = fx != NULL;
    if (use5 && (!gv5_on || (combine == 8 && plan.uses_tex1))) return 0; /* pm5 has no second texture coordinate */
    if (r_zmark) return 0;
    if (r_decal && r_blend) return 0;
    if (pm_wide && !off_active) {
        const float(*m)[4] = tab[gm];
        if (ui_ortho || (m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f)) return 0;
    }
    if (gidx_n + nidx > GRING_IDX) return 0;
    if (!draw_3d) { flush(); draw_3d = 1; }
    if (nverts > 0) flush();
    /* C3: extend an open static batch across consecutive compiled lists (same PSO inputs and matrix slot). */
    if (grange_batch_open(gm, vp, uvso, cmul, cadd, zmix, ct, fx, use5)) {
        pal_tab = tab;
        memcpy(gidx + gidx_n, idx, nidx * 2u);
        gidx_n += nidx;
        gni += nidx;
        return 2; /* extended an open static batch (C3 cross-list merge) */
    }
    if (gni > 0 && (!g_pal || pal_n == 0 || pal_g[0] != gm || use5 != g_use5 || (use5 && memcmp(&g_fx, fx, sizeof g_fx)) || memcmp(g_uvso, uvso, sizeof(g_uvso)) || memcmp(g_vp, vp, sizeof(g_vp)) || memcmp(g_cmul, cmul, sizeof(g_cmul)) ||
                    memcmp(g_cadd, cadd, sizeof(g_cadd)) || memcmp(g_zmix, zmix, sizeof(g_zmix)) || g_ct != ct)) flush();
    if (gni == 0) {
        memcpy(g_uvso, uvso, sizeof(g_uvso));
        memcpy(g_vp, vp, sizeof(g_vp));
        memcpy(g_cmul, cmul, sizeof(g_cmul));
        memcpy(g_cadd, cadd, sizeof(g_cadd));
        memcpy(g_zmix, zmix, sizeof(g_zmix));
        g_ct = ct; g_pal = 1; pal_n = 1; pal_g[0] = gm; pal_gen++;
        g_use5 = use5; if (use5) g_fx = *fx;
        if (gv_palmax < 1) gv_palmax = 1;
        if (vp[0] != vp_sx || vp[1] != vp_sy || vp[2] != vp_tx || vp[3] != vp_ty) { vp_sx = vp[0]; vp_sy = vp[1]; vp_tx = vp[2]; vp_ty = vp[3]; rebuild_3d(); }
        if (ct) {
            int ccw = gv_wind <= 0;
            if (ct == 2) ccw = !ccw;
            g_cullm = ccw ? GPU_CULL_BACK_CCW : GPU_CULL_FRONT_CCW;
        } else g_cullm = GPU_CULL_NONE;
    }
    pal_tab = tab;
    memcpy(gidx + gidx_n, idx, nidx * 2u);
    gidx_n += nidx;
    gni += nidx;
    return 1;
}
int pm_gpu_gidx_room(unsigned n) { return gv4_on && gpu_ok && gidx_n + n <= GRING_IDX; }

/* DLC1: 1 when the ring has room for n more vertices (and the path is on) */
int pm_gpu_gv_room(unsigned n) { return gpu_vtx_on && gpu_ok && gring_n + n <= GRING_VTX; }
int pm_gpu_gvtx_load(const void* vtx, unsigned n) {
    if (!gpu_vtx_on || !gpu_ok || gring_n + n > GRING_VTX) return -1;
    memcpy(gring + gring_n * 16, vtx, n * 16);
    unsigned b = gring_n;
    if (rgen) memset(rgen + b, 0, n * sizeof(unsigned));
    gring_n += n;
    return (int)b;
}

void pm_gpu_set_mode(int zbuffer, int lighting) {
    if (zbuffer == z_on && lighting == light_on) return;
    flush();
    z_on = zbuffer;
    light_on = lighting;
}

void pm_gpu_set_nocolor(int on) {
    if (on == r_nocol) return;
    flush();
    r_nocol = on;
}

/* B3b: on = the next triangles are a case 6 model (a depth test against the world, a constant depth write). zn = clip z / w of the constant. */
void pm_gpu_set_zmark(int on, float zn) {
    if (on == r_zmark && (!on || zn == r_zmark_zn)) return;
    flush();
    r_zmark = on;
    r_zmark_zn = zn;
}

void pm_gpu_set_render(int zcmp, int zupd, int blend, int atest, int decal) {
    if (zcmp == r_zcmp && zupd == r_zupd && blend == r_blend && atest == r_atest && decal == r_decal) return;
    flush();
    r_zmark = 0; /* B3b: draw_tri turns it on again for a case 6 model */
    r_zcmp = zcmp;
    r_zupd = zupd;
    r_blend = blend;
    r_atest = atest;
    r_decal = decal;
}

void pm_gpu_set_tex1(float sx, float sy, float ox, float oy) {
    if (sx == t1_s[0] && sy == t1_s[1] && ox == t1_s[2] && oy == t1_s[3]) return;
    flush();
    t1_s[0] = sx; t1_s[1] = sy; t1_s[2] = ox; t1_s[3] = oy;
}

static unsigned cs_last_prim = 0xffffffffu, cs_last_env = 0xffffffffu;

static int plan_same_layout(const PmPlan* a, const PmPlan* b) {
    PmPlan x = *a, y = *b;
    x.buf = x.vtx = 0;
    y.buf = y.vtx = 0;
    for (int i = 0; i < 6; i++) x.st[i].constant = y.st[i].constant = 0;
    return memcmp(&x, &y, sizeof x) == 0;
}

static void plan_layout_field_diffs(const PmPlan* a, const PmPlan* b) {
    if (a->n != b->n) CS_ADD(CS_PF_N, 1);
    for (int i = 0; i < 6; i++) {
        if (memcmp(a->st[i].g, b->st[i].g, sizeof a->st[i].g)) {
            CS_ADD(CS_PF_G, 1);
            break;
        }
    }
    for (int i = 0; i < 6; i++) {
        if (a->st[i].cname != b->st[i].cname) {
            CS_ADD(CS_PF_CNAME, 1);
            break;
        }
    }
    if (a->bufname != b->bufname) CS_ADD(CS_PF_BUFNAME, 1);
    if (a->vtxname != b->vtxname) CS_ADD(CS_PF_VTXNAME, 1);
    if (a->uses_tex != b->uses_tex) CS_ADD(CS_PF_UTEX, 1);
    if (a->uses_sha != b->uses_sha) CS_ADD(CS_PF_USHA, 1);
    if (a->uses_tex1 != b->uses_tex1) CS_ADD(CS_PF_UT1, 1);
    if (a->uses_noise != b->uses_noise) CS_ADD(CS_PF_UNOISE, 1);
    if (memcmp(a->ks, b->ks, sizeof a->ks)) CS_ADD(CS_PF_KS, 1);
    if (memcmp(a->fold, b->fold, sizeof a->fold)) CS_ADD(CS_PF_FOLD, 1);
}

void pm_gpu_set_plan(const PmPlan* p) {
    if (combine == 8 && memcmp(&plan, p, sizeof plan) == 0) return;
    RS_ADD(RS_COMB, 1);
    CS_ADD(CS_PLAN, 1);
    if (combine == 8 && plan_same_layout(&plan, p)) {
        CS_ADD(CS_CON, 1);
        if (prim != cs_last_prim) CS_ADD(CS_PRIM, 1);
        if (env_col != cs_last_env) CS_ADD(CS_ENV, 1);
    } else {
        CS_ADD(CS_LAY, 1);
        if (combine == 8) plan_layout_field_diffs(&plan, p);
    }
    flush();
    plan = *p;
    plan_gen++;
    combine = 8;
    cs_last_prim = prim;
    cs_last_env = env_col;
}

void pm_gpu_set_combine(int mode) {
    if (mode == combine) return;
    RS_ADD(RS_COMB, 1);
    CS_ADD(CS_MODE, 1);
    flush();
    combine = mode;
}

/* The RDP can brighten past 1 in the second cycle. Texture * color * 2 gives that range. */
void pm_gpu_set_rgb_scale(int scale) {
    if (scale == rgb_scale) return;
    flush();
    rgb_scale = scale;
}

void pm_gpu_set_prim(unsigned rgba) {
    if ((combine == 5 || combine == 6 || combine == 7) && rgba != prim) flush();
    prim = rgba;
}

void pm_gpu_set_mix(int on, unsigned rgba) {
    if (!on) rgba = 0;
    if (on == r_mix && rgba == r_mixcol) return;
    flush();
    r_mix = on;
    r_mixcol = rgba;
}

void pm_gpu_set_env(unsigned rgba) { env_col = rgba; }

void pm_gpu_set_const(unsigned rgba) {
    if (rgba == cconst) return;
    if (combine == 4) flush();
    cconst = rgba;
}

void pm_gpu_flush(void) { flush(); }
void pm_gpu_flush_at(int line) { fl_fn = "gbi"; fl_line = line; flush_impl(); } /* PG0: a flush asked for by the display list interpreter, line of the call in pm_gbi.c */

void pm_gpu_set_scissor(int x0, int y0, int x1, int y1) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > 320) x1 = 320;
    if (y1 > 240) y1 = 240;
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    /* V05: the batch in flight was drawn under the old scissor on the N64. Draw it before the scissor changes: the last
     * background piece of a window (its bottom right quarter) was cut by the next pass's scissor, a smaller one. */
    if (x0 != sci_x0 || y0 != sci_y0 || x1 != sci_x1 || y1 != sci_y1) flush();
    sci_x0 = x0;
    sci_y0 = y0;
    sci_x1 = x1;
    sci_y1 = y1;
}

static void tri_raw(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                    float x1, float y1, float z1, float u1, float v1, unsigned c1,
                    float x2, float y2, float z2, float u2, float v2, unsigned c2) {
    if (draw_3d) {
        flush();
        draw_3d = 0;
    }
    put(x0, y0, z0, u0, v0, c0);
    put(x1, y1, z1, u1, v1, c1);
    put(x2, y2, z2, u2, v2, c2);
}

void pm_gpu_tri(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                float x1, float y1, float z1, float u1, float v1, unsigned c1,
                float x2, float y2, float z2, float u2, float v2, unsigned c2) {
    if (rect_open) flush(); /* DR1: a plain 2D triangle does not join a rectangle batch */
    tri_raw(x0, y0, z0, u0, v0, c0, x1, y1, z1, u1, v1, c1, x2, y2, z2, u2, v2, c2);
}

void pm_gpu_tri3d(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                  float x1, float y1, float z1, float u1, float v1, unsigned c1,
                  float x2, float y2, float z2, float u2, float v2, unsigned c2) {
    if (gni > 0) flush(); /* GV1: a raw batch is open, it was queued first */
    if (!draw_3d) {
        flush();
        draw_3d = 1;
    }
    put(x0, y0, z0, u0, v0, c0);
    put(x1, y1, z1, u1, v1, c1);
    put(x2, y2, z2, u2, v2, c2);
}

void pm_gpu_vert_clip(float x, float y, float z, float w, float u, float v, unsigned c) {
    if (gni > 0) flush(); /* GV1: a raw batch is open, it was queued first */
    if (!draw_3d) {
        flush();
        draw_3d = 1;
    }
    /* F07-7: a UI (orthographic) vertex on the left or right screen edge goes to the 400 px screen edge: bars, fades and overlay quads fill the gutters. Same rule as the fill rectangle. */
    if (pm_wide && !off_active && ui_ortho && w > 0.f && (x >= w * 0.999f || x <= -w * 0.999f)) x *= 1.25f;
    put4(x, y, z, w, u, v, c);
}

/* Game pixels are y-down. The ortho matrix is y-up, and 320px is centered in 400. */
static float px(int x) { return off_active ? (float)x : 40.f + (float)x; } /* F07-3: 2D is 1:1 and centered, also with wide */
static float py(int y) { return off_active ? (float)y : 240.f - (float)y; }

static float fill_z = 0.5f;
/* DR1/O2: a rectangle joins the open rectangle batch when its draw state is the same (effective combine mode, rect_z). Else the batch is drawn first. */
static void rect_begin(int ec) {
    int rtx = pm_tex_cur_idx();
    if (rect_open && rect_ec == ec && rect_zb == rect_z && rect_tex == rtx && nverts + 6 < 500 && !(off_active && pm_tex_off_idx() >= 0)) /* a batch that samples the image it writes needs one snapshot per rectangle (FX2a) */ { dr1_merged++; return; }
    if (rect_open) dr1_breaks++;
    flush();
    draw_3d = 0; /* the 2D rectangle follows; tri_raw must not flush the empty batch with the rectangle state in place */
    rect_ec = ec; rect_zb = rect_z; rect_tex = rtx; rect_open = 1;
}
static void rect_end(void) { if (PMDL("rectdbg")) { dbg_take(dbg_snap); dbg_have = 1; } }
void pm_gpu_fill_rect(int x0, int y0, int x1, int y1, unsigned rgba) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    rect_begin(0);
    int saved = combine;
    combine = 0;
    rect_2d = 1;
    batch_identity = 1;
    float xa = (pm_wide && !off_active && x0 <= 0) ? 0.f : px(x0); /* F07-3: a fill from the game edge reaches the screen edge */
    float xb = (pm_wide && !off_active && x1 >= 319) ? 400.f : px(x1 + 1);
    float ya = py(y0);
    float yb = py(y1 + 1);
    tri_raw(xa, ya, fill_z, 0, 0, rgba, xb, ya, fill_z, 0, 0, rgba, xb, yb, fill_z, 0, 0, rgba);
    tri_raw(xa, ya, fill_z, 0, 0, rgba, xb, yb, fill_z, 0, 0, rgba, xa, yb, fill_z, 0, 0, rgba);
    rect_end();
    rect_2d = 0;
    combine = saved;
}

void pm_gpu_fill_rect_z(int x0, int y0, int x1, int y1, unsigned rgba, float d) {
    fill_z = d; /* the batch in flight keeps its own depth state: rect_begin draws it first when rect_z differs */
    rect_z = 1;
    pm_gpu_fill_rect(x0, y0, x1, y1, rgba);
    rect_z = 0;
    fill_z = 0.5f;
}

/* B3: a fill into the z-buffer image: depth d in the rectangle, colour untouched, no depth test (r_nocol, r_zupd, no r_zcmp). */
void pm_gpu_fill_depth(int x0, int y0, int x1, int y1, float d) {
    int s_nc = r_nocol, s_zc = r_zcmp, s_zu = r_zupd, s_bl = r_blend, s_at = r_atest, s_de = r_decal, s_z = z_on, s_mix = r_mix;
    flush();
    int s_zm = r_zmark; r_zmark = 0;
    r_nocol = 1; r_zcmp = 0; r_zupd = 1; r_blend = 0; r_atest = -1; r_decal = 0; z_on = 1; r_mix = 0;
    pm_gpu_fill_rect_z(x0, y0, x1, y1, 0, d);
    flush(); /* DR1: the rectangle is drawn with the temporary render state */
    r_zmark = s_zm;
    r_nocol = s_nc; r_zcmp = s_zc; r_zupd = s_zu; r_blend = s_bl; r_atest = s_at; r_decal = s_de; z_on = s_z; r_mix = s_mix;
}

void pm_gpu_tex_rect(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; float s = s0; s0 = s1; s1 = s; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; float s = t0; t0 = t1; t1 = s; }
    if (fl_on()) { fl_rects++; if (fl_prev_rect) fl_consec++; }
    int saved = combine;
    rect_begin((saved == 5 || saved == 6 || saved == 7 || saved == 8) ? saved : 2);
    combine = (saved == 5 || saved == 6 || saved == 7 || saved == 8) ? saved : 2;
    rect_2d = 1;
    batch_identity = 1;
    float xa = px(x0);
    float xb = px(x1 + 1);
    if (pm_wide && !off_active && xb > xa) { /* F07-7: a rectangle on the game edge reaches the screen edge, the texture continues at the same density */
        float ds = (s1 - s0) / (xb - xa);
        if (x0 == 0) { s0 -= ds * xa; xa = 0.f; }
        if (x1 == 319) { s1 += ds * (400.f - xb); xb = 400.f; }
    }
    float ya = py(y0);
    float yb = py(y1 + 1);
    unsigned c = (saved == 5 || saved == 7) ? env_col : (saved == 8 && plan.vtxname != PM_NM_SHADE) ? plan.vtx : 0xffffffffu;
    tri_raw(xa, ya, fill_z, s0, t0, c, xb, ya, fill_z, s1, t0, c, xb, yb, fill_z, s1, t1, c);
    tri_raw(xa, ya, fill_z, s0, t0, c, xb, yb, fill_z, s1, t1, c, xa, yb, fill_z, s0, t1, c);
    { /* R13c: the rows this rectangle draws, see grab_dy0. Merged rectangles keep the union. */
        int ry0 = y0 < 0 ? 0 : y0, ry1 = y1 + 1;
        if (rect_y1 > rect_y0) { if (ry0 < rect_y0) rect_y0 = ry0; if (ry1 > rect_y1) rect_y1 = ry1; }
        else { rect_y0 = ry0; rect_y1 = ry1; }
    }
    rect_end();
    rect_2d = 0;
    combine = saved;
    fl_prev_rect = 1;
}

void pm_gpu_tex_rect_z(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1, float d) {
    fill_z = d;
    rect_z = 1;
    pm_gpu_tex_rect(x0, y0, x1, y1, s0, t0, s1, t1);
    rect_z = 0;
    fill_z = 0.5f;
}

/* F03: one pixel of the last finished depth buffer (D24S8, same tiling and rotation as pm_gpu_read_frame). */
int pm_gpu_depth_at(int x, int y, unsigned* d24) {
    if (rect_open) flush(); /* DR1 */
    const unsigned char* src;
    unsigned fx, fy, mx, my, m;
    if (!target || x < 0 || x > 319 || y < 0 || y > 239) return 0;
    src = (const unsigned char*)target->frameBuf.depthBuf;
    if (!src) return 0;
    fx = (unsigned)(239 - y);
    fy = (unsigned)(40 + x);
    mx = fx & 7u;
    my = fy & 7u;
    m = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) | ((mx & 4u) << 2) | ((my & 4u) << 3);
    *d24 = *(const unsigned*)(src + ((((fy >> 3) * 30u + (fx >> 3)) * 64u + m) * 4u)) & 0xFFFFFFu;
    return 1;
}
