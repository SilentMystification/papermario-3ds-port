#include <3ds.h>
void pm_gx_room(void); /* HANG1a */
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
#include "nd.h"
#include "nd_sprite.h"
#include "pm_shader4_shbin.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "pm_gpu_int.h"

static int drew;
/* 3D writes the depth buffer. A later texrect has to win even if the test stays on. */
static int light_on;
/* RDP render mode: depth compare, depth update, alpha blend, alpha cut-off (-1 none), decal. */
int pm_debug_value(const char* key, char* out, unsigned n);
static int dview = -1; /* V02 depth view mode, see dv_color */
static __attribute__((cold, noinline)) int dv_read(void) { char vv[8]; dview = pm_debug_value("depthview", vv, sizeof vv) ? atoi(vv) : 0; return dview; }
static inline int dv_mode(void) { /* RECT-1b: read the switch on first use in any path (the key fields below read it before put4 ever ran in a native-only frame: -1 keyed as depth view). PERF2: the read out of line */
    return __builtin_expect(dview >= 0, 1) ? dview : dv_read();
}
static unsigned dv_draw;
static unsigned rs_src[3]; /* S1-3: other mode words and depth image flag of the last sync_render, for the ndref oracle */
static unsigned plan_k[4], plan_kp[4]; /* S1-3: the combiner words of the plan: pm_comb_build(w0, w1, two, t1only) gives plan (pending key from pm_gpu_plan_key, set with the plan) */
static int stencil_on; /* B3b: case 6 model, see flush */
static int env_hi = 1; /* texenv stages configured so far (stage 0 is always reset) */
static unsigned cconst;
static int rgb_scale = 1;
static int batch_identity;
static unsigned prim = 0xffffffffu;
static unsigned dbg_omode_l; /* FX1-2b: the L othermode word of the current render state (orbdbg log only) */
void pm_gpu_dbg_mode(unsigned m) { dbg_omode_l = m; }
/* POND-LOG (debug switch waterlog): one line per submitted batch in frames 200 and 201 (the pond water passes). wl_primz: G_SETPRIMDEPTH (the former pm_gbi.c). The depth
 * branch of a draw: fn 0 test off, 1 GREATER, 2 GEQUAL, 3 ALWAYS; wr 0 colour only, 1 depth only, 2 colour and depth (as the walker and the native words set it). */
static unsigned wl_primz;
void pm_gpu_dbg_primz(unsigned z) { wl_primz = z; }
static int wl_on(void) { extern unsigned pm_frame_count(void); if (__builtin_expect(!PMDL("waterlog"), 1)) return 0; unsigned f = pm_frame_count(); return f >= 200u && f <= 201u; } /* PERF2: the cached switch first */
static void wl_depth(int z, int zmark, int zcmp, int zupd, int decal, int nocol, unsigned* fn, unsigned* wr) {
    *fn = 0; *wr = 0;
    if (!z) return;
    if (zmark) { *fn = 1; *wr = 1; return; }
    if (zcmp) { *fn = decal ? 2 : 1; *wr = nocol ? 1 : zupd ? 2 : 0; return; }
    if (zupd) { *fn = 3; *wr = nocol ? 1 : 2; return; }
}
static float game_mvp[4][4];
static C3D_Mtx shader_3d;
/* N64 scissor, y down. The 400-wide target is the 320 game plus 40px bars. */
static int sci_x0, sci_y0, sci_x1 = 320, sci_y1 = 240;
static C3D_Mtx projection;
/* PT07: the RDP NOISE input. A 64 x 64 L8 texture of random bytes (fixed seed) on unit 2. The vertex shader gives it the screen position
 * (one texel per pixel) plus a random offset per drawn frame, so every pixel reads a new random value every frame. */
static C3D_Tex noise_tex;
static int noise_ready; /* 1 ready, -1 failed */
static float noise_off[2];
static unsigned noise_lcg = 0x2545f491u;
static unsigned noise_next(void) { noise_lcg = noise_lcg * 1664525u + 1013904223u; return noise_lcg >> 8; }
unsigned pm_gpu_noise_state(void) { return noise_lcg; } /* DET: the detlog line reads it */
static int noise_init(void) {
    if (noise_ready) return noise_ready > 0;
    if (!C3D_TexInit(&noise_tex, 64, 64, GPU_L8)) { pm_log_e("PT07: no memory for the noise texture, NOISE stays 0"); noise_ready = -1; return 0; }
    {   /* random bytes: any order of independent random bytes is the same picture, so the tiled layout needs no swizzle */
        unsigned char* d = (unsigned char*)noise_tex.data;
        for (int i = 0; i < 64 * 64; i++) d[i] = (unsigned char)(noise_next() & 255u);
        pm_fl_dc(d, 64 * 64);
    }
    C3D_TexSetFilter(&noise_tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&noise_tex, GPU_REPEAT, GPU_REPEAT);
    noise_ready = 1;
    return 1;
}
static float t1_s[4] = { 1.f, 1.f, 0.f, 0.f };
static int aux_bound;
static C3D_RenderTarget* target;
/* STEREO: the right eye target. cur_target is the target of the pass in progress (the left one in a mono frame). pm_eye_e is the signed NDC shift of this eye
 * (0 in a mono frame: the original perspective, one pass). pm_eye_conv is the eye distance in world units that has zero parallax (set by the camera). */
static C3D_RenderTarget *targetR, *cur_target;
static int stereo_frame, r_wantclear, r_cleared;
static float eye_slider;
float pm_eye_e, pm_eye_conv = 300.f;
#define PM_EYE_E 0.015f /* NDC shift per eye at slider 1 for a point at infinity (3 px of 400). The calibration knob. */
/* H02 GPU limit stats, maxima since the last `mem` log line. Verts and command buffer overflow drop work without a message. */
static unsigned draws_frame, draws_max, frames_seen;
static float cmd_max;
static DVLB_s* dvlb;
/* PO3b (switch gpuvtx): raw Vtx path. A ring of the 16 byte Vtx records of this frame and a u16 index ring. A batch is the triangles that share one
 * matrix and one uv map. Program prog4 (matrix palette) or prog5 reads the Vtx as they are. The batch is drawn by flush() when gni > 0. */
static unsigned gstat_n; /* DLC2: static vertices in use */
/* FB-1 ring_wrap: the raw ring is a chunk list. Chunk 0 is the boot allocation; its tail is the static area (GRING_VTX and up). ring_cur is the chunk the
 * open batch and the fills use: gring and gidx point to it. stamp = the frame that used the chunk; a chunk with stamp < ring_done is finished on the GPU
 * (ring_done = the frame of the last begin: C3D_FrameBegin waits for the GX queue). A batch has one vertex base: gi_kind 0 is the ring chunk in use (walker, rect, spr),
 * gi_kind 1 the static area of chunk 0 (grange). Model draws use chunk 0 too. A batch never mixes the kinds off chunk 0. */
static const void* buf_bp; /* FB-1: the vertex base of the buffer info while buf_cur is 2 */
/* PRIMZFIX repro, switch zround (debug only): zmix.y rounded to 2^-16 steps, the float24 z step of the console (diag_primz.md). The shader multiplies this value. */
static float zmix_y_dbg(float y) { static int on = -1; if (on < 0) on = pm_debug_has("zround"); return on ? (float)(int)(y * 65536.f + 0.5f) * (1.f / 65536.f) : y; }
/* UC2 probe, log only: logs the C3D_StencilTest arguments and the 0x105 word the call stores when the word changes (switch stprobe, 200 lines max).
 * Word = im << 24 | ref << 16 | wm << 8 | (fn << 4) & 0x70 | en (encoding from the citro3d disassembly, docs/handoff/diag_uc2_0x110.md section 3). */
extern unsigned char __C3D_Context[]; /* citro3d context, read only: the 0x105 word is at +0xF4 (diag_uc2_ctxword.md section 2) */
static int stp_on = -1; static unsigned stp_n, stp_lw[8], stp_cnt[8], stp_last, stp_lc[8];
static unsigned stc_ctx(void) { return *(volatile unsigned*)(__C3D_Context + 0xF4); }
static int stp_sw(void) { if (stp_on < 0) stp_on = pm_debug_has("stprobe") ? 1 : 0; return stp_on; }
static __attribute__((cold, noinline)) void stc_set(int site, int en, GPU_TESTFUNC fn, int ref, int im, int wm) {
    C3D_StencilTest(en, fn, ref, im, wm);
    if (!stp_sw()) return;
    stp_cnt[site & 7]++;
    unsigned w = ((unsigned)im << 24) | ((unsigned)ref << 16) | ((unsigned)wm << 8) | (((unsigned)fn << 4) & 0x70u) | (en ? 1u : 0u);
    unsigned c = stc_ctx();
    stp_last = w;
    if ((stp_cnt[site & 7] > 1 && w == stp_lw[site & 7] && c == stp_lc[site & 7]) || stp_n >= 200) return;
    stp_lw[site & 7] = w; stp_lc[site & 7] = c; stp_n++;
    pm_log_d("UC2P site %d word105 %08x en %d fn %d ref %d im %02x wm %02x ctxF4 %08x frame %u calls %u", site, w, en ? 1 : 0, (int)fn, ref, im, wm, c, pm_frame_count(), stp_cnt[site & 7]);
}
/* GV1c: matrix palette. A raw batch (switch gv1) holds up to GPAL game matrices. A Vtx copy in the ring carries its palette index in the flag field (bytes 6 and 7).
 * The vertex shader pm4 reads pal[4 * index]. An entry is the post matrix (OrthoTilt, viewport) times the game matrix. */
NdBatch nb = { .pal_gen = 1, .rect_pal_z = 0.5f, .rchunk_n = 1 }; /* the open raw batch and the ring (pm_gpu_int.h) */
PmGpuSt gs = { .r_zcmp = 1, .r_zupd = 1, .r_blend = 1, .r_atest = -1, .env_col = 0xffffffffu, .grab_dirty = 1, .vp_sx = 160.f, .vp_sy = 120.f, .vp_tx = 160.f, .vp_ty = 120.f }; /* the render state (pm_gpu_int.h) */
static shaderProgram_s prog4;
static C3D_AttrInfo attr4;
/* GV5: program pm5 (lighting, texgen, fog). A batch with g_use5 holds up to GPAL5 matrices, 7 registers each (4 position rows, 3 normal rows). */
enum { GPAL5 = 8 };
static shaderProgram_s prog5;
static DVLB_s* dvlb5;
static int u5_uvx = -1, u5_pal = -1, u5_lamb = -1, u5_lcol = -1, u5_ldir = -1, u5_lk0 = -1, u5_lk1 = -1, u5_lflag = -1, u5_fogu = -1, u5_uvso = -1, u5_cmul = -1, u5_cadd = -1, u5_nzo = -1, u5_zmix = -1, gv5_on;
static const float (*pal_ntab)[3][4];
void pm_gpu_set_ntab(const float (*ntab)[3][4]) { pal_ntab = ntab; }
int pm_gpu_gv5_ok(void) { return gv5_on; }
/* RAW1: lean binding. bound = the program citro3d has (0 unknown, 1 cpu, 3 pm4, 4 pm5), buf_cur = the buffer info (0 unknown, 1 CPU pool, 2 raw ring). shadows of the pm4 uniforms
 * and of the palette entries skip a set that holds the value already. They restart at every frame begin. */
static int bound, raw_dvlb_ok; static DVLB_s* dvlb_all;
static void raw1_reset(void) { bound = 0; nb.buf_cur = 0; }
static int u4_pal = -1, u4_uvx = -1, u4_uvso = -1, u4_cmul = -1, u4_cadd = -1, u4_zmix = -1, u4_nzo = -1;
static void rd_sync(void); /* RECT-1b diag: parse the GPUCMD words since the last sync (before every submit) */
static unsigned post_gen = 1;
static unsigned* rgen;          /* per ring vertex: the palette generation in which its flag was set (0 = never) */
static unsigned char* rk;       /* per ring vertex: the flag value */
static C3D_Mtx pcomp[257];      /* palette entry per game matrix index: post x game */
static const float (*pal_tab)[4][4];
static unsigned gv_recopy_n, gv_palup_n, gv_palmax, gv_palflush;
static C3D_Mtx gv_post;
static int gv_wind = 1;   /* GV1a: sign of the 3 x 3 determinant (x y w) of the matrix after the game matrix: +1 keeps the orientation of N64 clip space */

int pm_wide_cfg; /* F07-5: the user setting. pm_wide is 0 in 4:3 game modes (logos, title, file select). */
int pm_wide; /* F07-1: 1 = the 320 px game picture is stretched over the 400 px screen (X scale 1.25). Read once in pm_gpu_init. */
static int wedge_x; /* WIDE1-2: 1 while pm_gpu_grange draws an edge-only orthographic range with x scale 1.25 in the post matrix */
static int ui_ortho; /* F07-3: the game matrix is orthographic (UI): drawn 1:1 and centered with wide, not stretched */
static C3D_Mtx off_proj;

static void ident(float m[4][4]) {
    memset(m, 0, sizeof(float) * 16);
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
}

/* F3DEX stores a row-vector multiply. The shader does dp4(row, v), which is
 * the transpose. Viewport then maps N64 clip/w into upright pixels, and
 * OrthoTilt turns those pixels into the rotated framebuffer. */
static void rebuild_3d(void) {
    C3D_Mtx view;
    Mtx_Zeros(&view);
    /* V05: an off-screen pass is not rotated and has no 40 pixel margin: GPU y is the game y (y runs down) */
    view.r[0].x = (pm_wide && !gs.off_active && (!ui_ortho || wedge_x)) ? gs.vp_sx * 1.25f : gs.vp_sx; /* WIDE1-2: wedge_x scales the x of an orthographic draw whose vertices all sit on the game edge */
    view.r[0].w = gs.off_active ? gs.vp_tx : (pm_wide && !ui_ortho) ? gs.vp_tx * 1.25f : 40.f + gs.vp_tx;
    view.r[1].y = gs.off_active ? -gs.vp_sy : gs.vp_sy;
    view.r[1].w = gs.off_active ? gs.vp_ty : 240.f - gs.vp_ty;
    /* Clip z/w is -1 at the near plane and +1 at the far plane. Citro's
     * ortho then wants near at -1 and far at 0, which is the value GEQUAL
     * keeps. The opposite sign stores the far wall as the nearer depth, so
     * the house covers the yard. */
    view.r[2].z = 0.5f;
    view.r[2].w = 0.5f;
    view.r[3].w = 1.f;
    {   /* GV1a: orientation of the map from N64 clip (x y w) to PICA clip. The game matrix is applied before, so it does not count. */
        C3D_Mtx* post = &gv_post;
        Mtx_Multiply(post, gs.off_active ? &off_proj : &projection, &view);
        const C3D_FVec *a = &post->r[0], *b = &post->r[1], *c = &post->r[3];
        float det = a->x * (b->y * c->w - b->w * c->y) - a->y * (b->x * c->w - b->w * c->x) + a->w * (b->x * c->y - b->y * c->x);
        gv_wind = det < 0.f ? -1 : 1;
        post_gen++;
        nd_mtx_pcomp(shader_3d.m, post->m, game_mvp); /* S1-4: nd_mtx.c. SEAM: the same product, in the same order, as pcomp_calc: post x game. The CPU path and the raw path then give bit-identical positions. */
    }
}

/* V05: an off-screen colour image. draw_box renders the contents of a rotating window (pause pages, file select panels)
 * into a game buffer with the color image set to it, then draws that buffer as strips through the rotation matrix.
 * Such a pass renders into a PICA render target that is a texture (VRAM), and pm_tex samples it for the strips. */
/* X02: w x h is the size of the slot (64 x 64 for a game image up to 64 wide, else PM_OFF_W x PM_OFF_H). siz is the game pixel size
 * (3 = 32 bit, 2 = 16 bit, 1 = 8 bit). The same memory can be a 32 bit image and an 8 bit image in one frame: two slots. */
void pm_vram_note(void); /* pm_mem.c: VRAM low-water mark */
void pm_vram_alloc_fail(const char* what, unsigned kb); /* pm_mem.c */
void pm_vram_reg(const void* p, unsigned bytes); /* pm_mem.c: per-bank accounting */
void pm_vram_unreg(const void* p, unsigned bytes);
int pm_vram_bank_of(const void* p);
static int off_used, off_sused, off_fail, off_w = PM_OFF_W, off_h = PM_OFF_H, off_i8; /* off_fail: a VRAM failure, cleared at pm_gpu_begin */
static unsigned off_mask_f; /* VRAM3 trace: bit i set when full image i is drawn in this frame */
static unsigned off_trace_n;
static int prev_want, prev_pre; /* PT12: see pm_gpu_grab_prev */
static int prev_done; /* GAMEOVER: the slot of the old frame is drawn in this drawn frame (key or t = 0.5 pass). Cleared with the slots (pm_gpu_begin_wait) */
extern unsigned pm_frame_count(void);
static unsigned r_mixcol; /* V06: see pm_gpu_set_mix */
static int r_replace; /* off-screen fill: the pixel is written, not blended */
static int r_memblend; /* FX2a: the blender mixes with the memory colour by the memory alpha (G_BL_A_MEM) */
/* B2: T2 grab state. grab_tex holds a copy of the main target strips (same tiled layout, 256 x 512). grab_bind: flush binds it
 * instead of the pm_tex texture. A grab copies the full width. grab_dirty: a draw of unknown rows went to the main target since the
 * last grab. [grab_dy0, grab_dy1) are the rows that texture rectangles drew since then (R13c: motion_blur_flame loads one strip of
 * rows per rectangle, the strips do not overlap, so one copy serves them). */
enum { GRAB_MAX = 8 };
static C3D_Tex grab_tex;
static int grab_bind, grab_dy0, grab_dy1, grab_n, grab_split;

static void ndunit_scan(void); /* switch ndunit */
static void sh_clear(void);
static int want_clear; /* see below */
/* GV0 and KICK1: the GPU reads the vertex pool, the raw Vtx ring and the index ring when it runs the command list. Everything written since the last
 * flush goes out of the data cache before every C3D_FrameSplit and at the end of the frame (GSPGPU_FlushDataCache is one call to the GSP, so each
 * range is flushed once). gring_fl and gidx_fl mark what is flushed. They restart at pm_gpu_begin. */
/* DC1: the sampled close (set in pm_gpu_flush_at). Only one close in 8 reads the tick, so the timers are sampled and scaled by 8. */
static int dc_on;
/* DS0: cold counters for pm_gpu_dcsum_frame. Ticks and counts in this file's flushes, sampled closes and cmd words; sums over 300 drawn frames. */
static unsigned dcs_gt, dcs_fl, dcs_cmd, dcs_smp, dcs_cl, dcs_us[DCU_N], dcs_n, dcs_c[DC_N];
/* PERF2: submit counters (totals, the prof cls line takes them per window): program switches, native shadow resets, C3Di_UpdateContext calls, command list splits */
static unsigned sc_prg, sc_shr, sc_upd, sc_spl;
static void gpu_flush_written(void) {
    unsigned g0 = dc_on ? pm_ticks() : 0;
    pm_tex_flush_pend_only();
    if (nb.gring && nb.gring_n > nb.gring_fl) { pm_fl_dc(nb.gring + nb.gring_fl * 16, (nb.gring_n - nb.gring_fl) * 16); PH_ADD(C_FLCALL, 1); PH_ADD(C_FLKB, (nb.gring_n - nb.gring_fl) * 16 >> 10); nb.gring_fl = nb.gring_n; dcs_fl++; }
    if (nb.gidx && nb.gidx_n > nb.gidx_fl) { pm_fl_dc(nb.gidx + nb.gidx_fl, (nb.gidx_n - nb.gidx_fl) * 2); PH_ADD(C_FLCALL, 1); PH_ADD(C_FLKB, (nb.gidx_n - nb.gidx_fl) * 2 >> 10); nb.gidx_fl = nb.gidx_n; dcs_fl++; }
    if (dc_on) dcs_gt += (pm_ticks() - g0) * 8u;
}
/* B2: queue order is clear of the main target, then every part. The first split of a frame queues the clear first. */
void split_frame(void) {
    gpu_flush_written(); /* GV0b: the GPU runs the first part now, its vertices must be in memory */
    if (!grab_split++ && want_clear) {
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0);
        want_clear = 0;
    }
    sh_clear();
    ndunit_scan();
    rd_sync(); C3D_FrameSplit(GX_CMDLIST_FLUSH); sc_spl++;
}

/* Boot arena for the 64 x 64 RGBA8 textures (4 small slots, 2 eight bit slots, and a snap of each): 12 blocks of 16 KB, 6 per bank.
 * Made once at boot and never freed, so no 16 KB hole can form in a bank before the cache halves come up. */
#define SMALL_BLK (64u * 64u * 4u)
#define SMALL_N 12
static u8* small_blk[SMALL_N];
static int small_n, small_next;
static void small_arena_init(void) {
    for (int k = 0; k < SMALL_N; k++) {
        u8* p = (u8*)vramMemAlignAt(SMALL_BLK, 0x80, (k & 1) ? VRAM_ALLOC_B : VRAM_ALLOC_A);
        if (!p) { pm_vram_alloc_fail("small arena", SMALL_BLK >> 10); pm_log_e("small arena: block %d of %d failed", k, SMALL_N); return; }
        pm_vram_reg(p, SMALL_BLK);
        small_blk[small_n++] = p;
    }
}
/* VRAM2: the full images (512 x 256) and the copy buffer (512 KB, grab_tex and the snap of a full image share it) are reserved at boot
 * (full_reserve) and never freed. A block goes to bank A first, then B. The texture cache takes what is left (pm_tex.c slab_init). */
#define BLK_FULL (PM_OFF_W * PM_OFF_H * 4u)
#define BLK_FULL16 (PM_OFF_W * PM_OFF_H * 2u) /* R4: the off-screen full images are RGBA5551 (2 bytes a texel) */
static u8* copy_blk;
static u8* full_blk[OFF_N];
static u8* bank_block(unsigned bytes) {
    u8* p = (u8*)vramMemAlignAt(bytes, 0x80, VRAM_ALLOC_A);
    if (!p) p = (u8*)vramMemAlignAt(bytes, 0x80, VRAM_ALLOC_B);
    return p;
}
static void full_reserve(void) {
    copy_blk = bank_block(BLK_FULL);
    if (copy_blk) { pm_vram_reg(copy_blk, BLK_FULL); pm_log_w("vram reserved: copy buffer in bank %c", pm_vram_bank_of(copy_blk) ? 'B' : 'A'); }
    else { pm_vram_alloc_fail("copy", BLK_FULL >> 10); pm_log_e("vram reserve: copy buffer failed, grabs read on the CPU"); }
    for (int i = 0; i < OFF_N; i++) { /* R4: a full image is RGBA5551, half the block of RGBA8 */
        full_blk[i] = bank_block(BLK_FULL16);
        if (full_blk[i]) { pm_vram_reg(full_blk[i], BLK_FULL16); pm_log_w("vram reserved: full image %d in bank %c", i, pm_vram_bank_of(full_blk[i]) ? 'B' : 'A'); }
        else { pm_vram_alloc_fail("full image", BLK_FULL16 >> 10); pm_log_e("vram reserve: full image %d failed, its off-screen passes draw on the screen", i); }
    }
    pm_vram_note();
}
/* A header whose data is set later. C3D_TexInit takes linear memory for its own data, which is freed at once. */
static int tex_hdr(C3D_Tex* t, int w, int h, GPU_TEXCOLOR fmt) {
    if (!C3D_TexInit(t, w, h, fmt)) return 0;
    linearFree(t->data);
    t->data = NULL;
    return 1;
}
/* the copy block from full_reserve, NULL when the boot reserve failed */
static u8* copy_get(void) { return copy_blk; }
/* TEXF1: copy_blk takes one copy per frame. A second copy in a frame waits for the GPU first (pm_gx_drain), so the part that sampled
 * the earlier copy is done before the buffer is written. pm_copy_drain counts the waits (texstat). */
void pm_gx_drain(void);
unsigned pm_copy_drain;
static int copy_used;
static void copy_guard(void) {
    if (copy_used) { pm_copy_drain++; pm_gx_drain(); }
    copy_used = 1;
}
/* the data of a full image: its reserved block (full_reserve). A failed boot reserve returns 0, the pass draws on the screen. */
static int full_tex_init(int i) {
    if (!full_blk[i]) return 0;
    if (!gs.offs[i].hdr) {
        if (!tex_hdr(&gs.offs[i].tex, PM_OFF_W, PM_OFF_H, GPU_RGBA5551)) return 0;
        gs.offs[i].hdr = 1;
    }
    gs.offs[i].tex.data = full_blk[i];
    return 1;
}
/* a 64 x 64 image takes an arena block (made at boot) */
static int small_tex_init(C3D_Tex* t) {
    if (small_next >= small_n || !C3D_TexInit(t, 64, 64, GPU_RGBA8)) return 0;
    linearFree(t->data);
    t->data = small_blk[small_next++];
    return 1;
}
/* FX2a: the RDP loads a tile into TMEM before it draws, so a pass that reads the image it writes (the flame blur) sees the pixels from before
 * the pass. The PICA samples the render target itself with no such copy. Copy the slot before the batch and sample the copy. */
static void off_snapshot(int i) {
    u8* dst;
    u32 bytes = (u32)gs.offs[i].w * (u32)gs.offs[i].h * (i < OFF_N ? 2u : 4u); /* R4: a full image is 16 bit */
    if (gs.offs[i].w == 64) {
        if (!gs.offs[i].snap_ready) {
            if (!small_tex_init(&gs.offs[i].snap)) { pm_vram_alloc_fail("snap", 16); pm_log_e("FX2a: no arena block for the copy of slot %d", i); return; }
            C3D_TexSetFilter(&gs.offs[i].snap, GPU_LINEAR, GPU_NEAREST);
            C3D_TexSetWrap(&gs.offs[i].snap, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
            gs.offs[i].snap_ready = 1;
        }
        dst = gs.offs[i].snap.data;
    } else {
        /* the copy buffer: this copy overwrites the frame start grab (PT12), so that request is void */
        prev_pre = 0;
        dst = copy_get();
        if (!dst) { gs.offs[i].snap_ready = 0; pm_log_e("FX2a: no VRAM for the copy of slot %d", i); return; }
        if (!gs.offs[i].snap_hdr) {
            if (!tex_hdr(&gs.offs[i].snap, PM_OFF_W, PM_OFF_H, GPU_RGBA5551)) { gs.offs[i].snap_ready = 0; return; } /* R4: the copy of a full image is 5551 too */
            C3D_TexSetFilter(&gs.offs[i].snap, GPU_LINEAR, GPU_NEAREST);
            C3D_TexSetWrap(&gs.offs[i].snap, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
            gs.offs[i].snap_hdr = 1;
        }
        gs.offs[i].snap.data = dst;
        gs.offs[i].snap_ready = 1;
    }
    split_frame();
    pm_gx_room();
    if (dst == copy_blk) copy_guard();
    GX_TextureCopy((u32*)gs.offs[i].tex.data, GX_BUFFER_DIM(2048 / 16, 0), (u32*)dst, GX_BUFFER_DIM(2048 / 16, 0), bytes, BIT(3));
}

/* PG0 (prof mode): flush count by caller. The macro below records the function and line of the call. */
static const char* fl_fn = "?";
static int fl_line;
typedef struct { const char* fn; int line; unsigned n, e; } FlCause; /* n = flush calls, e = calls that found an empty batch */
static FlCause fl_tab[48];
static int fl_nt;
static int fl_prev_rect; static unsigned fl_rects, fl_consec; /* PG0: texture rectangles, and those that follow another with no other flush call between */
#define fl_on() (pm_ph_cen) /* PROF2-2: the flush tracer is phase mode only */
extern unsigned pm_frame_count(void);
static void fl_count(int empty) {
    for (int i = 0; i < fl_nt; i++) if (fl_tab[i].fn == fl_fn && fl_tab[i].line == fl_line) { fl_tab[i].n++; fl_tab[i].e += empty; return; }
    if (fl_nt < 48) { fl_tab[fl_nt].fn = fl_fn; fl_tab[fl_nt].line = fl_line; fl_tab[fl_nt].n = 1; fl_tab[fl_nt].e = empty; fl_nt++; }
}
void pm_gpu_gbiprof_report(unsigned frames) { /* the six most frequent causes since the last report, per frame in 0.01 */
    unsigned tn = 0, te = 0;
    for (int i = 0; i < fl_nt; i++) { tn += fl_tab[i].n; te += fl_tab[i].e; }
    if (frames) pm_log_d("PG0 flush per frame %u.%02u (empty %u.%02u), rects %u consecutive %u, causes:", tn / frames, (tn * 100 / frames) % 100, te / frames, (te * 100 / frames) % 100, fl_rects, fl_consec);
    for (int r = 0; r < 6; r++) { /* the six most frequent causes: function, line, flushes per frame in 0.01, empty batches */
        int best = -1; unsigned bn = 0;
        for (int i = 0; i < fl_nt; i++) if (fl_tab[i].n > bn) { bn = fl_tab[i].n; best = i; }
        if (best < 0) break;
        if (frames) pm_log_d("PG0 %s:%d %u.%02u per frame, empty %u", fl_tab[best].fn, fl_tab[best].line, bn / frames, (bn * 100 / frames) % 100, fl_tab[best].e);
        fl_tab[best].n = 0;
    }
    fl_rects = fl_consec = 0;
    for (int i = 0; i < fl_nt; i++) fl_tab[i].n = fl_tab[i].e = 0;
}
/* V19: decal depth bias (0.00003 would be about 2 float24 steps at mid range) */
static float decal_bias(void) { return 0.001f; }
/* V16b: an N64 decal (Z_CMP, ZMODE_DEC) passes only where the stored depth is close to the depth of the decal. GPU_GEQUAL with a bias passes everywhere the decal is
 * nearer, so an overlay on a sprite also landed on the wall behind it (Thunder Rage on Shy Guys: a flickering rectangle). Two passes with the stencil: pass 1 marks the
 * pixels where the stored depth is not farther than decal - DECAL_EPS, pass 2 draws where the mark is set and the usual test (decal + bias is not behind) passes. */
#define DECAL_EPS 0.003f
typedef struct { int combine, tex, tex1, i8_0, i8_1, rgb_scale, r_mix, draw_3d, rect_2d, dview, off_active, off_i8, r_memblend, r_replace, r_blend; unsigned prim, cconst, mixcol; unsigned plan_gen; } TevKey;
unsigned pm_tex_census_id(int unit);
/* CEN1: draw census (debug tool, switch drawcensus=<frame>). Each flush batch adds one entry: template key (TevKey without plan_gen, z, raw, program, cull), texture ids of unit 0 and 1, triangle count. At the end of the frame the switch frame prints one line per distinct entry and a summary. Without the switch only the draw and triangle counts of the prof line are kept (no hash, no table). */
#define CEN_N 256
typedef struct { unsigned k, tex0, tex1, tris, n; } CenEnt;
static CenEnt cen_tab[CEN_N];
static unsigned cen_nt, cen_draws, cen_tris, cen_over;
static int cen_fr = -2; /* drawcensus frame: -1 off, -2 not read yet (cen_frame_end reads it) */
static void cen_put(unsigned h, unsigned id0, unsigned id1, unsigned tris) {
    unsigned i = (h ^ id0 * 0x9E3779B1u ^ id1 * 0x85EBCA6Bu ^ tris * 0xC2B2AE35u) & (CEN_N - 1);
    for (unsigned p = 0; p < CEN_N; p++, i = (i + 1) & (CEN_N - 1)) {
        CenEnt* e = &cen_tab[i];
        if (e->n == 0) { e->k = h; e->tex0 = id0; e->tex1 = id1; e->tris = tris; e->n = 1; cen_nt++; return; }
        if (e->k == h && e->tex0 == id0 && e->tex1 == id1 && e->tris == tris) { e->n++; return; }
    }
    cen_over++;
}
static void cen_add(const TevKey* tk, int z, int graw, int prog, unsigned cull, unsigned id0, unsigned id1, unsigned tris) {
    TevKey kc = *tk; /* N3: prim reaches the TEV constant only for combine 1, 5, 6, 7; other keys hash without it */
    if (kc.combine != 1 && kc.combine != 5 && kc.combine != 6 && kc.combine != 7) kc.prim = 0;
    const unsigned* w = (const unsigned*)&kc;
    unsigned h = 2166136261u;
    for (unsigned i = 0; i < (sizeof *tk - sizeof tk->plan_gen) / sizeof(unsigned); i++) h = (h ^ w[i]) * 16777619u;
    h = (h ^ (unsigned)(z | graw << 1 | prog << 2)) * 16777619u;
    h = (h ^ cull) * 16777619u;
    cen_put(h, id0, id1, tris);
}
/* CEN-1a: why each walker GPU draw (wd) is not native (the former pm_gbi.c os_report, lines wcen4). Only other, pm5 and the nd_raw_native refusals (nr4, nr6, nr8, nr9) are set now; the other names keep the log columns. */
enum { CW_OTHER, CW_RECT, CW_CLIP, CW_SEAM, CW_WIDE, CW_RING, CW_PM5, CW_NR1, CW_NR2, CW_NR3, CW_NR4, CW_NR5, CW_NR6, CW_NR7, CW_NR8, CW_NR9, CW_N };
#define CW_NRK(k) ((unsigned)(CW_NR1 - 1 + (k)))
static unsigned cen_why;
unsigned pm_gpu_cen_why(void) { return cen_why; }
const char* pm_gpu_cen_name(unsigned c) {
    static const char* const nm[CW_N] = { "other", "rect", "clip", "seam", "wide", "ring", "pm5", "nr1", "nr2", "nr3", "nr4", "nr5", "nr6", "nr7", "nr8", "nr9" };
    return c < CW_N ? nm[c] : "?";
}
/* NDREF1: reference draw stream, switch ndref=<frame> (the former pm_gbi.c pm_gbi_ndref_frame). The draws and producer tags of that frame go to a
 * buffer. cen_frame_end writes sdmc:/3ds/PaperMario/ndref.bin. Layout: 3ds/tests/ndref/FORMAT.md. */
int pm_gbi_ndref_frame(void);
void pm_tex_cur_slots(int* a, int* b);
int pm_tex_slot_words(int i, uint32_t* out);
int ndref_on;
static int ndref_target = -2, ndref_wr, ndref_over;
static unsigned char* ndref_buf; static unsigned ndref_len, ndref_draws;
#define NDREF_CAP (2u << 20)
typedef struct { int z, graw, prog; unsigned cull; int decal, blend, atest, sx0, sy0, sx1, sy1; int sa, sb; uint32_t tw0[4], tw1[4]; } NdrefDraw;
static void ndref_put(const void* p, unsigned n) {
    if (ndref_len + n > NDREF_CAP) { ndref_over = 1; return; }
    memcpy(ndref_buf + ndref_len, p, n); ndref_len += n;
}
static void ndref_patch(unsigned at) { /* record size of the record that starts at at (header is 8 bytes) */
    if (ndref_over) return;
    unsigned n = ndref_len - at - 8u; memcpy(ndref_buf + at + 4, &n, 4);
}
static unsigned ndcmd_prev;
static void ndref_begin(void) { /* per frame, from pm_gpu_begin */
    if (ndref_target == -2) ndref_target = pm_gbi_ndref_frame();
    ndref_on = ndref_target >= 0 && (int)pm_frame_count() == ndref_target;
    if (!ndref_on || ndref_wr) return;
    if (!ndref_buf) ndref_buf = (unsigned char*)malloc(NDREF_CAP);
    ndref_wr = ndref_buf != NULL; ndref_len = 0; ndref_draws = 0; ndref_over = 0;
    if (!ndref_wr) ndref_on = 0;
    else { u32 off; GPUCMD_GetBuffer(NULL, NULL, &off); ndcmd_prev = off; }
}
void ndref_tag(unsigned kind, unsigned id, unsigned extra) { /* the former pm_gbi.c producer and model marks, record kind 2 and 3 */
    unsigned hd[2] = { kind, 8u }, p[2] = { id, extra };
    ndref_put(hd, 8); ndref_put(p, 8);
}
static __attribute__((cold, noinline)) void ndref_draw(const TevKey* tk, int z, int graw, int prog, unsigned cull) {
    unsigned at = ndref_len, kind = 1u, n = 0;
    NdrefDraw d; memset(&d, 0, sizeof d);
    ndref_put(&kind, 4); ndref_put(&n, 4);
    d.z = z; d.graw = graw; d.prog = prog; d.cull = cull;
    d.decal = gs.r_decal; d.blend = gs.r_blend; d.atest = gs.r_atest;
    d.sx0 = sci_x0; d.sy0 = sci_y0; d.sx1 = sci_x1; d.sy1 = sci_y1;
    pm_tex_cur_slots(&d.sa, &d.sb);
    pm_tex_slot_words(d.sa, d.tw0); pm_tex_slot_words(d.sb, d.tw1);
    ndref_put(tk, sizeof *tk); ndref_put(&d, sizeof d);
    ndref_put(nb.g_uvso, sizeof nb.g_uvso); ndref_put(nb.g_cmul, sizeof nb.g_cmul); ndref_put(nb.g_cadd, sizeof nb.g_cadd); ndref_put(nb.g_zmix, sizeof nb.g_zmix);
    ndref_put(&nb.g_fx, sizeof nb.g_fx);
    unsigned np = graw ? nb.pal_n : 0u; /* palette: game matrix index and the composed matrix pcomp of each entry */
    ndref_put(&np, 4);
    for (unsigned k = 0; k < np; k++) { ndref_put(&nb.pal_g[k], 4); ndref_put(&pcomp[nb.pal_g[k]], sizeof(C3D_Mtx)); }
    unsigned nv = nb.gni; /* resolved vertices in draw order: 16-byte ring values */
    ndref_put(&nv, 4);
    for (unsigned i = 0; i < nb.gni; i++) ndref_put((nb.gi_kind == 1 ? nb.rchunk[0].vtx : nb.gring) + 16u * nb.gidx[nb.gi_start + i], 16);
    ndref_patch(at);
    if (np) { /* S1-4 oracle: kind 5, the inputs of the palette product pcomp = post x game of this draw: post (gv_post), then per entry the game matrix (u32 gm, 16 floats pal_tab[gm]) */
        unsigned hd[2] = { 5u, 0u }, at5 = ndref_len;
        ndref_put(hd, 8); ndref_put(&gv_post, sizeof gv_post); ndref_put(&np, 4);
        for (unsigned k = 0; k < np; k++) { ndref_put(&nb.pal_g[k], 4); ndref_put(pal_tab[nb.pal_g[k]], 64); }
        ndref_patch(at5);
    }
    ndref_draws++;
}
/* S1-3 oracle: kind 4, one record per GPU draw call (pass). Body: u32 pass (0 main, 1 decal pass A, 2 decal pass B, 3 zmark pass 2), z, zmark, zcmp, zupd, nocol, noise, raw,
 * plan key w0, w1, two, t1only (0 when combine is not 8), omode_h, omode_l, depth image flag of the last sync_render, then ctx[216..268) (13 words, the effect block C3Di_EffectBind writes) and ctx[292..428) (34 words: 6 texenv stages of 5 words,
 * texenv buffer update, buffer colour, third buffer word). The words are read from the citro3d context at the draw call, so they are exactly what the flush turns into register writes. */
static int ndref_nz, ndref_raw;
static __attribute__((cold, noinline)) void ndref_state(unsigned pass, int z) {
    unsigned hd[2] = { 4u, 0u }, b[15] = { pass, (unsigned)z, (unsigned)gs.r_zmark, (unsigned)gs.r_zcmp, (unsigned)gs.r_zupd, (unsigned)gs.r_nocol, (unsigned)ndref_nz, (unsigned)ndref_raw,
        gs.combine == 8 ? plan_k[0] : 0u, gs.combine == 8 ? plan_k[1] : 0u, gs.combine == 8 ? plan_k[2] : 0u, gs.combine == 8 ? plan_k[3] : 0u, rs_src[0], rs_src[1], rs_src[2] };
    unsigned at = ndref_len;
    ndref_put(hd, 8); ndref_put(b, sizeof b);
    ndref_put(__C3D_Context + 216, 13 * 4); ndref_put(__C3D_Context + 292, 34 * 4);
    ndref_patch(at);
}
/* S3 oracle: kind 6, the command words the citro3d command buffer got since the end of the draw before, up to the end of this draw call (uniform and state updates, the draw).
 * Body: u32 n, n words. If the buffer was split in between (offset smaller than before) n is 0. */
static __attribute__((cold, noinline)) void ndref_cmd(void) {
    u32 *buf, size, off;
    GPUCMD_GetBuffer(&buf, &size, &off);
    unsigned n = off >= ndcmd_prev ? off - ndcmd_prev : 0u, at = ndref_len, hd[2] = { 6u, 0u };
    ndref_put(hd, 8); ndref_put(&n, 4);
    if (n) ndref_put(buf + ndcmd_prev, n * 4u);
    ndref_patch(at);
    ndcmd_prev = off;
}
static __attribute__((cold, noinline)) void ndref_end(void) { /* end of frame: write the file, one log line when it is written */
    if (!ndref_wr) return;
    ndref_wr = 0; ndref_on = 0; ndref_target = -1;
    unsigned fr = pm_frame_count();
    uint32_t hdr[5] = { 0x3152444Eu /* NDR1 */, 4u, fr, ndref_draws, ndref_len };
    FILE* f = fopen("sdmc:/3ds/PaperMario/ndref.bin", "wb");
    if (f) { fwrite(hdr, 4, 5, f); fwrite(ndref_buf, 1, ndref_len, f); fclose(f); }
    pm_log_evt("ndref f%u done draws %u bytes %u%s%s", fr, ndref_draws, ndref_len, f ? "" : " open failed", ndref_over ? " over" : "");
    free(ndref_buf); ndref_buf = NULL;
}
/* S0-6: prof counters. pfd_add runs at each frame end (cen_frame_end), before pm_dc_c is cleared: draws, TEV rebuilds, texture binds and
 * triangles of the frame. gb: GPU busy ticks of the sampled frames, from the submit (pm_gpu_end) to the sync (the frame begin after it). */
static unsigned long long pfd_sum[4], gb_sum, gb_n, gb_sub;
static int gb_on;
unsigned pm_gpu_fr_last[4]; /* MAGI fspk: draws, TEV rebuilds, texture binds, triangles of the last finished frame */
extern unsigned pm_pf_late; /* PROF2-1, pm_phase.c */
static void pfd_add(void) {
    pm_gpu_fr_last[0] = cen_draws; pm_gpu_fr_last[1] = pm_dc_c[DC_TEVR]; pm_gpu_fr_last[2] = pm_dc_c[DC_TB0] + pm_dc_c[DC_TB1]; pm_gpu_fr_last[3] = cen_tris;
    pfd_sum[0] += cen_draws; pfd_sum[1] += pm_dc_c[DC_TEVR]; pfd_sum[2] += pm_dc_c[DC_TB0] + pm_dc_c[DC_TB1]; pfd_sum[3] += cen_tris;
}
/* out: draws, TEV rebuilds, texture binds, triangles (summed since the last read), GPU busy ticks and sample count. Reads reset the sums. */
void pm_gpu_pf_read(unsigned long long out[6]) {
    for (int k = 0; k < 4; k++) { out[k] = pfd_sum[k]; pfd_sum[k] = 0; }
    out[4] = gb_sum; out[5] = gb_n; gb_sum = gb_n = 0;
}
static void cen_frame_end(void) {
    ndref_end();
    pfd_add();
    if (cen_fr == -2) { char v[16]; cen_fr = pm_debug_value("drawcensus", v, sizeof v) ? atoi(v) : -1; }
    const int fr = cen_fr;
    if (fr >= 0 && (int)pm_frame_count() == fr) {
        for (unsigned i = 0; i < CEN_N; i++) if (cen_tab[i].n)
            pm_log_d("cen f%u k %08x tex %08x %08x tris %u n %u", fr, cen_tab[i].k, cen_tab[i].tex0, cen_tab[i].tex1, cen_tab[i].tris, cen_tab[i].n);
        pm_log_d("cen f%u summary batches %u tris %u distinct %u over %u", fr, cen_draws, cen_tris, cen_nt, cen_over);
    }
    if (cen_nt) memset(cen_tab, 0, sizeof cen_tab);
    cen_nt = cen_draws = cen_tris = cen_over = 0;
}
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
void nd_tridrop_log(unsigned f); /* DEL (nd_geom.c) */
static unsigned plan_gen; /* DR1: counts changes of `plan`, the texenv key compares this instead of the plan */
unsigned dr1_merged, dr1_breaks; /* the dr1 log line */
static unsigned kick_n; /* KICK1 splits */
void pm_tex_cur_slots(int* a, int* b);
int pm_tex_cur_vram_pending(void); int pm_tex_aux_vram_pending(void);
/* S3 M3: the native raw draw (nd_raw_native) writes the effect, TEV, texture unit and uniform registers itself. The GPU then holds state that citro3d did not write. nd_leave_native
 * marks the context parts a native draw touched as dirty, so the next citro3d draw re-emits what the walker staged (its shadows still match that staged copy), and clears the
 * program binding, which makes C3D_BindProgram mark every uniform dirty. Runs at a frame begin and a split (sh_clear) and before a draw that citro3d makes. */
static int ndn_mode; /* 1: the GPU registers hold native state */
static void nd_leave_native(void) {
    if (!ndn_mode) return;
    ndn_mode = 0;
    *(volatile unsigned*)(__C3D_Context + 32) |= 0x10u | 0x200u | 0x20000u | 0x1800000u | 0xFC000000u; /* effect, texenv buffer, texenv stages, texture units 0 and 1 (the native draw writes unit 1 for a two texture combine, SGEN-2; unit 2 keeps its registers) */
    raw1_reset(); /* program, buffer info, and the uniform shadows of pm4 and pm5: the native draw wrote its own palette entries and uniforms over those registers */
}
static void sh_clear(void) { nd_leave_native(); sh.has = 0; pm_tex_bind_reset(); }
static void sh_scissor(GPU_SCISSORMODE m, u32 l, u32 t, u32 r, u32 b) {
    if ((sh.has & SH_SC) && sh.sc[0] == (unsigned)m && sh.sc[1] == l && sh.sc[2] == t && sh.sc[3] == r && sh.sc[4] == b) return;
    sh.sc[0] = m; sh.sc[1] = l; sh.sc[2] = t; sh.sc[3] = r; sh.sc[4] = b; sh.has |= SH_SC; pm_dc_c[DC_SHM]++; C3D_SetScissor(m, l, t, r, b);
}
static void sh_user_clip(int on) {
    if ((sh.has & SH_UC) && sh.uc == on) return;
    sh.uc = on; sh.has |= SH_UC; pm_dc_c[DC_SHM]++;
    if (on) {
        GPUCMD_AddWrite(GPUREG_FRAGOP_CLIP, 1);
        GPUCMD_AddIncrementalWrites(GPUREG_FRAGOP_CLIP_DATA0, gp2_clip_dat, 4);
    } else
        GPUCMD_AddWrite(GPUREG_FRAGOP_CLIP, 0);
}

static void flush_tev_rebuild(C3D_TexEnv* env, int tex, int tex1, int i8_0, int i8_1, int noise, int* memb_out);
static void flush_impl(void);
#define flush() do { fl_fn = __func__; fl_line = __LINE__; flush_impl(); } while (0)
/* KICK1: citro3d holds the GX queue inside a frame and starts the PICA at C3D_FrameEnd, so the GPU idled for the whole CPU part of the frame. One
 * C3D_FrameSplit when about 55 percent of the draws of the last frame are queued starts the GPU on the first part while the CPU builds the rest.
 * Cost: one more cache flush and one more command list submit on the CPU. Only on the main target (an off-screen pass and a grab split on their own).
 * Switch nokick turns it off. */
static unsigned prev_draws, kick_at;
static int kick_done; /* PERF2: the one split of this frame is done (KICK2's second split at 85 percent is deleted: each split costs a cache flush and a GSP submit on the CPU) */
extern int pm_ph_on;
void pm_gbi_draw_note(int native); /* WCEN3 */
extern unsigned pm_ph_kick_n, pm_ph_kick_us, pm_ph_draws; /* PI4 */
static void kick_split(void);
static inline void kick_check(void) { /* PERF2 step 4: the test per draw inline, the split (once a frame) out of line */
    if (kick_done || gs.off_active || !kick_at || draws_frame < kick_at || nb.rect_open || nb.gni > 0) return;
    kick_split();
}
static __attribute__((noinline)) void kick_split(void) {
    NDT_B(tk); /* PERF2: the kick class (out of sub) */
    kick_done = 1;
    gpu_flush_written();
    if (want_clear) { C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0); want_clear = 0; } /* queue order: the clear comes before the first part */
    sh_clear();
    PH_PUSH(PH_GPU);
    { unsigned k0 = pm_ph_cen ? pm_ticks() : 0; /* PI4: the split is timed on its own (phase mode only) */
      ndunit_scan(); rd_sync(); C3D_FrameSplit(GX_CMDLIST_FLUSH); sc_spl++;
      if (pm_ph_cen) pm_ph_kick_us += (pm_ticks() - k0) / 268u; }
    PH_POP();
    kick_n++;
    pm_ph_kick_n++;
    NDT_E(tk, NDT_KICK);
}
static void flush_draw(void);
/* S3-2 measurement: ticks spent in flush_impl (phase on), read by the tex_op timer of the former pm_gbi.c to split the close from the op */
static unsigned long long fl_acc;
unsigned long long pm_gpu_fl_acc(void) { return fl_acc; }
static void flush_impl_body(void);
/* C8: flush for pm_gpu_rect.c (the flush tracer fl_fn and fl_line stays in this file) */
void pm_gpu_flush_from(const char* fn, int line) { fl_fn = fn; fl_line = line; flush_impl(); }
/* PG0: the texture rectangle counts of the flush tracer (pm_gpu_tex_rect) */
void fl_rect_note(void) { if (fl_on()) { fl_rects++; if (fl_prev_rect) fl_consec++; } }
void fl_rect_done(void) { fl_prev_rect = 1; }
/* MSGBOX debug switch flushlog=<frame>: one line per flush of a non-empty batch in that frame: caller, line, kind, combine class, texture slot, indices */
static int flushlog_fr = -2; /* PERF2: -2 unread, -1 off, else the frame; flush_impl calls the cold line only while it is not -1 */
static __attribute__((cold, noinline)) void flushlog_line(void) {
    int fr = flushlog_fr;
    if (fr == -2) { char v[16]; fr = flushlog_fr = pm_debug_value("flushlog", v, sizeof v) ? atoi(v) : -1; }
    if (fr < 0 || (int)pm_frame_count() != fr || nb.gni <= 0) return;
    pm_log_d("flushlog f%d %s:%d rect %d ec %d tex %d comb %d gni %d", fr, fl_fn, fl_line, nb.rect_open, nb.rect_ec, nb.rect_tex, gs.combine, nb.gni);
}
static void flush_impl(void) {
    if (__builtin_expect(flushlog_fr != -1, 0)) flushlog_line(); /* PERF2: an unconditional call of a cold function made GCC predict every flush path never executed (.text.unlikely) */
    if (!pm_ph_cen) { flush_impl_body(); return; } /* PROF2-2: fl_acc feeds the tex_op timer only (phase mode) */
    unsigned t0 = pm_ticks();
    flush_impl_body();
    fl_acc += (unsigned)(pm_ticks() - t0);
}
/* FB-1 RSIG (debug switch rectsig): the render state a raw rectangle batch was opened with (rect_sig_fill at the open), compared with the live state at
 * every merge and at the close. The close draws the batch with the live state, so a field that differs names a setter that does not flush an open batch. */
RectSig rsig_open;
unsigned rsig_chk, rsig_diff;
extern void pm_tex_unit0_param(uint32_t* ws, uint32_t* wt, unsigned* filter);
void rect_sig_fill(RectSig* s) {
    uint32_t ws = 0, wt = 0; unsigned f = 0;
    pm_tex_unit0_param(&ws, &wt, &f);
    s->filt = (int)f; s->ws = (int)ws; s->wt = (int)wt; s->cur = pm_tex_cur_idx();
    s->sx0 = sci_x0; s->sy0 = sci_y0; s->sx1 = sci_x1; s->sy1 = sci_y1;
    s->bl = gs.r_blend; s->rep = r_replace; s->zc = gs.r_zcmp; s->zu = gs.r_zupd; s->at = gs.r_atest; s->dec = gs.r_decal; s->noc = gs.r_nocol;
    s->zon = gs.z_on; s->mem = r_memblend; s->mix = gs.r_mix; s->comb = gs.combine; s->rgbs = rgb_scale; s->pgen = plan_gen;
    s->mixc = (int)r_mixcol; s->prim = (int)prim; s->cc = (int)cconst; s->env = (int)gs.env_col;
}
void rect_sig_check(const char* where) {
    if (!PMDL("rectsig")) return;
    RectSig n;
    rect_sig_fill(&n);
    rsig_chk++;
#define RSIG_CMP(f) if (rsig_open.f != n.f) { rsig_diff++; if (rsig_diff <= 60) pm_log_d("rsig %s f%u %s open %d live %d", where, pm_frame_count(), #f, rsig_open.f, n.f); }
    RSIG_CMP(filt) RSIG_CMP(ws) RSIG_CMP(wt) RSIG_CMP(cur) RSIG_CMP(sx0) RSIG_CMP(sy0) RSIG_CMP(sx1) RSIG_CMP(sy1)
    RSIG_CMP(bl) RSIG_CMP(rep) RSIG_CMP(zc) RSIG_CMP(zu) RSIG_CMP(at) RSIG_CMP(dec) RSIG_CMP(noc) RSIG_CMP(zon) RSIG_CMP(mem) RSIG_CMP(mix)
    RSIG_CMP(comb) RSIG_CMP(rgbs) RSIG_CMP(pgen) RSIG_CMP(mixc) RSIG_CMP(prim) RSIG_CMP(cc) RSIG_CMP(env)
#undef RSIG_CMP
}
static void flush_impl_body(void) {
    if (nb.rect_open) { /* DR1: the batch is rectangles: draw it with the state they were queued in */
        rect_sig_check("close");
        int sc = gs.combine, sr = nb.rect_2d, sz = nb.rect_z;
        gs.combine = nb.rect_ec; nb.rect_2d = 1; nb.rect_z = nb.rect_zb; nb.rect_open = 0; nb.rect_tex = -1;
        flush_draw();
        gs.combine = sc; nb.rect_2d = sr; nb.rect_z = sz;
        if (!gs.off_active) { /* RECT-1b: the grab rows of this batch (the native raw close returns before flush_draw's bookkeeping) */
            if (nb.rect_y1 <= nb.rect_y0) gs.grab_dirty = 1;
            else if (grab_dy1 <= grab_dy0) { grab_dy0 = nb.rect_y0; grab_dy1 = nb.rect_y1; }
            else { if (nb.rect_y0 < grab_dy0) grab_dy0 = nb.rect_y0; if (nb.rect_y1 > grab_dy1) grab_dy1 = nb.rect_y1; }
        }
        nb.rect_y0 = nb.rect_y1 = 0; /* the rows of the rectangles in this batch are used up */
        return;
    }
    flush_draw();
}
static __attribute__((noinline)) void flush_tev_rebuild(C3D_TexEnv* env, int tex, int tex1, int i8_0, int i8_1, int noise, int* memb_out) { /* DLF23: out of line, runs only on a TEV key change */
    int memb = 0;
    TevKey tk;
    const size_t tk_n = sizeof tk;
    memset(&tk, 0, tk_n);
    tk.combine = gs.combine; tk.tex = tex; tk.tex1 = tex1; tk.i8_0 = i8_0; tk.i8_1 = i8_1; tk.rgb_scale = rgb_scale; tk.r_mix = gs.r_mix; tk.draw_3d = nb.draw_3d; tk.rect_2d = nb.rect_2d;
    tk.dview = dv_mode(); tk.off_active = gs.off_active; tk.off_i8 = off_i8; tk.r_memblend = r_memblend; tk.r_replace = r_replace; tk.r_blend = gs.r_blend;
    tk.prim = prim; tk.cconst = cconst; tk.mixcol = r_mixcol;
    tk.plan_gen = gs.combine == 8 ? plan_gen : 0;
    /* S1-3: the stage words come from nd_tmpl.c nd_tev_build (host tested against the ndref oracle). Stages 0 to max(old, new) - 1 are written: the builder
     * resets every stage, so a stage above the new count returns to the C3D_TexEnvInit words. */
    NdTmplKey nk;
    memset(&nk, 0, sizeof nk);
    nk.combine = (uint8_t)gs.combine; nk.tex = (uint8_t)tex; nk.tex1 = (uint8_t)tex1; nk.i8_0 = (uint8_t)i8_0; nk.i8_1 = (uint8_t)i8_1; nk.rgb_scale = (uint8_t)rgb_scale;
    nk.r_mix = (uint8_t)gs.r_mix; nk.draw_3d = (uint8_t)nb.draw_3d; nk.rect_2d = (uint8_t)nb.rect_2d; nk.dview = dv_mode() != 0; nk.off_active = (uint8_t)gs.off_active;
    nk.off_i8 = (uint8_t)off_i8; nk.r_memblend = (uint8_t)r_memblend; nk.r_replace = (uint8_t)r_replace; nk.r_blend = (uint8_t)gs.r_blend; nk.noise = noise != 0;
    NdTevIn ti = { &nk, &gs.plan, prim, cconst, r_mixcol };
    NdTevOut to;
    nd_tev_build(&ti, &to);
    _Static_assert(sizeof(C3D_TexEnv) == 20, "C3D_TexEnv is the 5 register words");
    if (to.buf_set) {
        C3D_TexEnvBufUpdate(C3D_Both, 0);
        C3D_TexEnvBufColor(to.buf_color);
    }
    for (int i = 0, hi = env_hi > to.n ? env_hi : to.n; i < hi; i++) memcpy(C3D_GetTexEnv(i), to.st[i], 20);
    env_hi = to.n;
    memb = to.memb;
    if (memb_out) *memb_out = memb;
}

static unsigned flush_gen; /* GV1b: counts the raw batches closed */
/* DC1: the sampled close. Marks: 0 start, 1 after the uniforms, 2 after the texture binds, 3 after TEV, 4 after the fragment state,
 * 5 after the draw call, 6 and 7 around kick_check. Set by pm_gpu_flush_at for one close in 8 (phase or dcsum on). */
static unsigned dc_seq, dc_t[8];
static u32 dc_w0; /* DS0: GPUCMD offset (words) at the start of the sampled close; C3D_GetCmdBufUsage updates only at a split, so it is not used */
#define DC_T(k) do { if (dc_on) dc_t[k] = pm_ticks(); } while (0)
static void dc_end(void) {
    pm_dc_us[DCU_PAL] += (dc_t[1] - dc_t[0]) / 268u;
    pm_dc_us[DCU_BIND] += (dc_t[2] - dc_t[1]) / 268u;
    pm_dc_us[DCU_TEV] += (dc_t[3] - dc_t[2]) / 268u;
    pm_dc_us[DCU_ST] += (dc_t[4] - dc_t[3]) / 268u;
    pm_dc_us[DCU_DRAW] += (dc_t[5] - dc_t[4]) / 268u;
    pm_dc_us[DCU_KICK] += (dc_t[7] - dc_t[6]) / 268u;
    dcs_smp++;
    dc_on = 0;
}
static __attribute__((cold, noinline)) void dc_kick_smp(void);
static inline void dc_kick(void) {
    if (!dc_on) { kick_check(); return; }
    dc_kick_smp();
}
static __attribute__((cold, noinline)) void dc_kick_smp(void) { /* PERF1: the sampled close (one in 8 with phase or dcsum) out of the hot callers */
    DC_T(6);
    { u32 w1; GPUCMD_GetBuffer(NULL, NULL, &w1); if (w1 >= dc_w0) dcs_cmd += w1 - dc_w0; } /* DS0: words of the close, before its own kick */
    kick_check();
    DC_T(7);
    dc_end();
}
static void dc_bind(int unit, C3D_Tex* t) { pm_dc_c[DC_TB0 + unit]++; C3D_TexBind(unit, t); }
/* FLH: cold and per-change parts of flush_draw, moved out of line. Same operations, same order. */
static __attribute__((cold, noinline)) void flush_fl_dbg(int graw) {
    fl_count(!graw);
    if (fl_fn != (const char*)"pm_gpu_tex_rect" && strcmp(fl_fn, "pm_gpu_tex_rect") != 0) fl_prev_rect = 0;
}
/* PM5 uniforms go through citro3d (C3D_FVUnif). A setter call marks the vector dirty and C3Di_UpdateContext sends it again, even with the same value. u5set and u5mtx
 * write only a vector that differs from citro3d's copy, except in the first pm5 batch after a program change (u5_valid 0): the pm4 path writes the shared
 * uniform registers directly, so citro3d's copy is not what the GPU holds then. */
static int u5_valid;
static NdShadow ndn_sh; /* the GPU registers a native draw wrote (nd_emit_draw) */
static void u5vec(int id, float x, float y, float z, float w) {
    const C3D_FVec* c = &C3D_FVUnif[GPU_VERTEX_SHADER][id];
    if (u5_valid && c->x == x && c->y == y && c->z == z && c->w == w) return;
    C3D_FVUnifSet(GPU_VERTEX_SHADER, id, x, y, z, w);
}
static void u5set(int id, const float v[4]) { u5vec(id, v[0], v[1], v[2], v[3]); }
static void u5mtx(int id, const C3D_Mtx* m) {
    if (u5_valid && memcmp(&C3D_FVUnif[GPU_VERTEX_SHADER][id], m, sizeof *m) == 0) return;
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, id, m);
}
/* the program of the next native draw: a change clears the native shadow (pm4 and pm5 share the uniform registers, and citro3d marks every uniform dirty at a bind) */
static void prog_use(int p) {
    if (bound == p) return;
    pm_dc_c[DC_PRG]++; C3D_BindProgram(p == 4 ? &prog5 : &prog4); C3D_SetAttrInfo(&attr4); bound = p;
    nd_shadow_reset(&ndn_sh); ndn_mode = 1; u5_valid = 0; sc_prg++; sc_shr++;
}
/* the pm5 light table of a draw (u5set: only the vectors that differ from citro3d's copy are marked dirty) */
static __attribute__((noinline)) void flush_light_tab(const PmFx* fx) {
    u5set(u5_lamb, fx->amb);
    for (int i = 0; i < 7; i++) { u5set(u5_lcol + i, fx->col[i]); u5set(u5_ldir + i, fx->dir[i]); }
    u5set(u5_lk0, fx->lk0);
    u5set(u5_lk1, fx->lk1);
}
/* N4 2b: the palette product post x game, one function for the live path (pcomp) and the per-eye fill. Same order of products. */
static void pal_mul(C3D_Mtx* o, const float (*m)[4]) { nd_mtx_pcomp(o->m, gv_post.m, m); } /* S1-4 */
static __attribute__((noinline)) void pcomp_calc(unsigned gm) {
    if (gm == RECT_GM) { /* RECT-1a: a rectangle vertex is (x, y, 0, 1) in screen pixels; P (x, y, z, 1) = P (x, y, 0, 1) + z * column 2 of P */
        pcomp[gm] = gs.off_active ? off_proj : projection; /* OFF-1: the off-screen pass has its own ortho (pixels of the target) */
        for (int i = 0; i < 4; i++) pcomp[gm].r[i].w += pcomp[gm].r[i].z * nb.rect_pal_z;
    } else
        pal_mul(&pcomp[gm], pal_tab[gm]);
                    gs.pcomp_gen[gm] = post_gen;
                    pm_dc_c[DC_PMUL]++;
}
static __attribute__((noinline)) void scissor_off(void) {
        /* not rotated: x is the game x. GPU y is the game y: row 0 of the image is the last memory row, where v = 0 samples */
        int x1 = sci_x1 > off_w ? off_w : sci_x1, y1 = sci_y1 > off_h ? off_h : sci_y1;
        if (x1 > sci_x0 && y1 > sci_y0)
            sh_scissor(GPU_SCISSOR_NORMAL, (u32)sci_x0, (u32)sci_y0, (u32)x1, (u32)y1);
        else
            sh_scissor(GPU_SCISSOR_NORMAL, 0, 0, off_w, off_h);
}
/* RECT-1b diag (switch rectreg=<frame>): citro3d and the raw close write one GPUCMD buffer. Entry = [param0][header][param1..paramk] with
 * header bits 0-15 id, 16-19 mask, 20-27 k, 31 consecutive (word j goes to id + j), padded to an even word count (libctru GPUCMD_AddInternal). rd_sync
 * parses every word before each submit, so rd_reg is the GPU register shadow since boot (citro3d writes only changed registers). rd_tex marks a textured rectangle batch. */
static u32 rd_reg[0x400];
static u32 rd_uf[256][4];
static unsigned rd_uidx, rd_ucnt, rd_last, rd_fr = 0xffffffffu, rd_nl, rd_pairs;
int rd_tex;
static int rd_st = -1;
static void rd_wr(unsigned id, unsigned mk, u32 v) { /* mk: header bits 16-19, byte b of the register is written when bit b is set */
    if (id >= 0x400u) return;
    if (id == 0x2C0u) { rd_uidx = v & 0xFFu; rd_ucnt = 0; return; }
    if (id == 0x2C1u) { rd_uf[rd_uidx & 0xFFu][rd_ucnt & 3u] = v; if (++rd_ucnt % 4u == 0) rd_uidx++; return; }
    if (mk != 0xFu) { u32 m = 0; for (int b = 0; b < 4; b++) if (mk >> b & 1u) m |= 0xFFu << (8 * b); rd_reg[id] = (rd_reg[id] & ~m) | (v & m); return; }
    rd_reg[id] = v;
}
static void rd_upto(const u32* cb, unsigned to) {
    unsigned i = to < rd_last ? 0 : rd_last;
    while (i + 1 < to) {
        u32 h = cb[i + 1];
        unsigned k = (h >> 20) & 0xFFu, id = h & 0xFFFFu;
        if (i + 2 + k > to) break;
        int cons = (int)(h >> 31);
        const unsigned mk = (h >> 16) & 0xFu;
        for (unsigned j = 0; j <= k; j++) rd_wr(id + (cons ? j : 0), mk, j ? cb[i + 1 + j] : cb[i]);
        i += k + 2 + (k & 1u);
    }
    rd_last = to;
}
static void rd_sync(void) {
    if (rd_st < 0) { char v[16]; rd_st = pm_debug_value("rectreg", v, sizeof v) ? 1 : 0; if (rd_st) rd_fr = (unsigned)atoi(v);
        if (!rd_st && pm_debug_value("orbreg", v, sizeof v)) { rd_st = 2; rd_fr = (unsigned)atoi(v); } } /* FX1-2 plan 5: orbreg=<frame> keeps the shadow on */
    if (!rd_st) return;
    u32* cb; u32 sz, off; GPUCMD_GetBuffer(&cb, &sz, &off);
    rd_upto(cb, off);
    rd_last = 0; /* the submit empties the buffer: the next words start at 0 */
}
void pm_log_prof_line(const char* fmt, ...);
/* FX1-2 plan 5 orbreg=<frame>: at the glow draws (effect tags 3, 4) of that frame, the register shadow: unit 0 block, the 6 TEV stages, 0xE0, 0xFD, 0x100 to 0x107, 0x2C0 to 0x2C1 uniforms. */
static __attribute__((cold, noinline)) void orbreg_dump(void) {
    extern unsigned pm_gbi_dbg_orb_tag(void);
    extern void pm_log(const char* fmt, ...);
    static int nd_lines;
    if (rd_st != 2 || pm_frame_count() != rd_fr || nd_lines >= 4) return;
    unsigned ot_ = pm_gbi_dbg_orb_tag();
    if (ot_ != 3u && ot_ != 4u) return;
    nd_lines++;
    u32* cb; u32 sz, off; GPUCMD_GetBuffer(&cb, &sz, &off);
    rd_upto(cb, off);
    const unsigned f = pm_frame_count();
    unsigned tb[6] = { 0xC0u, 0xC8u, 0xD0u, 0xD8u, 0xF0u, 0xF8u };
    pm_log("orbreg f%u list %u unit0 80 %08x 81 %08x 82 %08x 83 %08x 84 %08x 85 %08x 8e %08x 8f %08x", f, ot_, rd_reg[0x80], rd_reg[0x81], rd_reg[0x82], rd_reg[0x83], rd_reg[0x84], rd_reg[0x85], rd_reg[0x8E], rd_reg[0x8F]);
    for (int st = 0; st < 6; st++) pm_log("orbreg f%u stage %d base %03x src %08x op %08x fn %08x col %08x sc %08x", f, st, tb[st], rd_reg[tb[st]], rd_reg[tb[st] + 1], rd_reg[tb[st] + 2], rd_reg[tb[st] + 3], rd_reg[tb[st] + 4]);
    pm_log("orbreg f%u e0 %08x fd %08x 100 %08x 101 %08x 102 %08x 103 %08x 104 %08x 105 %08x 106 %08x 107 %08x", f, rd_reg[0xE0], rd_reg[0xFD], rd_reg[0x100], rd_reg[0x101], rd_reg[0x102], rd_reg[0x103], rd_reg[0x104], rd_reg[0x105], rd_reg[0x106], rd_reg[0x107]);
}
int rq_n, rq_cur, rq_slot[RQ_MAX], rq_b[RQ_MAX];
short rq_st[RQ_MAX][4];
static unsigned rd_ncl;
const void* pm_tex_slot_img(int idx);
static __attribute__((cold, noinline)) void rd_close(unsigned addr, unsigned dim, unsigned type, int tex) {
    if (rd_st <= 0 || pm_frame_count() != rd_fr) { rq_n = 0; return; }
    const unsigned k = rd_ncl++;
    const unsigned char* tb = (const unsigned char*)pm_tex_slot_img(pm_tex_cur_idx());
    pm_log_prof_line("rdcl f%u #%u rects %d gni %u cur %d addr %08x dim %08x type %08x tex %d texel %02x%02x%02x%02x%02x%02x%02x%02x why %s:%d", pm_frame_count(), k, rq_n, nb.gni, pm_tex_cur_idx(),
                     addr, dim, type, tex, tb ? tb[0] : 0u, tb ? tb[1] : 0u, tb ? tb[2] : 0u, tb ? tb[3] : 0u, tb ? tb[4] : 0u, tb ? tb[5] : 0u, tb ? tb[6] : 0u, tb ? tb[7] : 0u, fl_fn, fl_line);
    pm_log_prof_line("rdidx #%u gi_start %u gni %u idx %u %u %u %u %u %u first ring %u", k, nb.gi_start, nb.gni, nb.gidx[nb.gi_start], nb.gidx[nb.gi_start+1], nb.gidx[nb.gi_start+2], nb.gidx[nb.gi_start+3], nb.gidx[nb.gi_start+4], nb.gidx[nb.gi_start+5], (unsigned)nb.gring_n);
    for (int i = 0; i < rq_n && i < RQ_MAX; i++) {
        const short* g = (const short*)(nb.gring + 16u * (unsigned)rq_b[i]);
        const short* h = (const short*)(nb.gring + 16u * (unsigned)(rq_b[i] + 2));
        pm_log_prof_line("rdrq #%u i %d slot %d b %d xy %d %d %d %d st %d %d %d %d queued %d %d %d %d", k, i, rq_slot[i], rq_b[i], g[0], g[1], h[0], h[1], g[4], g[5], h[4], h[5], rq_st[i][0], rq_st[i][1], rq_st[i][2], rq_st[i][3]);
    }
    rq_n = 0;
}
/* RECT-1b plan 3 (switch rectpost=<frame>): every textured rectangle (rect_2d) opens a window; the window logs its rect and the next 15 draws
 * (nat = nd_native_submit), one rpD line each with the GPU register shadow (rd_reg) at the draw. The last window of the frame is the
 * one that matters: the first line that differs between two runs names the field. */
void pm_tex_size(int* w, int* h);
static int rp_src_, rp_tmpl_ = -1, rp_u0_;
int nd_tmpl_dbg_key(int id, int* tex, int* comb, int* d3, int* r2);
static int rp_on_ = -1; static unsigned rp_fr, rp_win, rp_n, rp_lines;
static __attribute__((cold, noinline)) void rp_draw(const char* path, int tex) {
    if (rp_on_ < 0) { char v[16]; if (pm_frame_count() == 0) return; /* the switches are read after boot draws: start at frame 1 */ rp_on_ = pm_debug_value("rectpost", v, sizeof v) ? 1 : 0; if (rp_on_) rp_fr = (unsigned)atoi(v); if (rp_on_ && !rd_st) { rd_st = 1; rd_fr = rp_fr; } pm_log_d("rpI on %d fr %u frame %u path %s", rp_on_, rp_fr, pm_frame_count(), path); }
    if (!rp_on_ || pm_frame_count() != rp_fr || rp_lines >= 700u) return;
    if (nb.rect_2d) { rp_win++; rp_n = 0; }
    if (rp_n >= 16u) return;
    rp_n++; rp_lines++;
    u32* cb; u32 sz, off; GPUCMD_GetBuffer(&cb, &sz, &off);
    rd_upto(cb, off);
    const int uv = u4_uvso, up = u4_pal;
    pm_log_d("rpD w%u n%u %s f%u rect %d d3 %d comb %d tex %d cur %d rtex %d | 80 %08x 81 %08x 82 %08x 83 %08x 84 %08x 85 %08x 8e %08x c0 %08x c1 %08x c2 %08x",
        rp_win, rp_n, path, pm_frame_count(), nb.rect_2d, nb.draw_3d, gs.combine, tex, pm_tex_cur_idx(), nb.rect_tex,
        rd_reg[0x80], rd_reg[0x81], rd_reg[0x82], rd_reg[0x83], rd_reg[0x84], rd_reg[0x85], rd_reg[0x8E], rd_reg[0xC0], rd_reg[0xC1], rd_reg[0xC2]);
    pm_log_d("rpE %u %u %s c3 %08x c4 %08x c8 %08x d0 %08x 100 %08x 101 %08x 102 %08x 103 %08x 104 %08x 200 %08x 201 %08x 202 %08x",
        rp_win, rp_n, path, rd_reg[0xC3], rd_reg[0xC4], rd_reg[0xC8], rd_reg[0xD0], rd_reg[0x100], rd_reg[0x101], rd_reg[0x102], rd_reg[0x103], rd_reg[0x104], rd_reg[0x200], rd_reg[0x201], rd_reg[0x202]);
    pm_log_d("rpF %u %u %s 2bb %08x e0 %08x fd %08x uvso %08x %08x %08x %08x pal0 %08x %08x %08x %08x",
        rp_win, rp_n, path, rd_reg[0x2BB], rd_reg[0xE0], rd_reg[0xFD],
        uv >= 0 ? rd_uf[uv & 0xFF][0] : 0u, uv >= 0 ? rd_uf[uv & 0xFF][1] : 0u, uv >= 0 ? rd_uf[uv & 0xFF][2] : 0u, uv >= 0 ? rd_uf[uv & 0xFF][3] : 0u,
        up >= 0 ? rd_uf[up & 0xFF][0] : 0u, up >= 0 ? rd_uf[up & 0xFF][1] : 0u, up >= 0 ? rd_uf[up & 0xFF][2] : 0u, up >= 0 ? rd_uf[up & 0xFF][3] : 0u);
    { int kt_ = -1, kc_ = -1, kd_ = -1, kr_ = -1; int kok_ = nd_tmpl_dbg_key(rp_tmpl_, &kt_, &kc_, &kd_, &kr_);
      pm_log_d("rpK %u %u %s src %d tmpl %d key %d tex %d comb %d d3 %d r2 %d u0 %d cur %d tex_arg %d", rp_win, rp_n, path, rp_src_, rp_tmpl_, kok_, kt_, kc_, kd_, kr_, rp_u0_, pm_tex_cur_idx(), tex); }
    { const NdTmplWord* tw_ = NULL; unsigned tn_ = rp_tmpl_ >= 0 ? nd_tmpl_words(rp_tmpl_, &tw_) : 0;
      pm_log_d("rpT %u %u %s tmpl %d n %u c0 %08x c1 %08x c2 %08x c3 %08x fd %08x cstage %08x %08x", rp_win, rp_n, path, rp_tmpl_, tn_,
          0u, 0u, 0u, 0u, 0u, 0u, 0u);
      for (unsigned q_ = 0; tw_ && q_ < tn_ && q_ < 40u; q_++) if (tw_[q_].reg >= 0xC0u && tw_[q_].reg <= 0xFDu) pm_log_d("rpT %u %u i %u reg %02x val %08x", rp_win, rp_n, q_, (unsigned)tw_[q_].reg, (unsigned)tw_[q_].val); }
    { void pm_gbi_sticky(unsigned* tss, unsigned* geo, unsigned* cw0, unsigned* cw1); unsigned st_tss, st_geo, st_cw0, st_cw1; int tw_ = 1, th_ = 1;
      pm_gbi_sticky(&st_tss, &st_geo, &st_cw0, &st_cw1); pm_tex_size(&tw_, &th_);
      pm_log_d("rpG %u %u %s env %08x cconst %08x mix %08x sticky tss %08x geo %08x cw %08x %08x tex %dx%d plan_gen %u comb %d", rp_win, rp_n, path,
          gs.env_col, cconst, r_mixcol, st_tss, st_geo, st_cw0, st_cw1, tw_, th_, plan_gen, gs.combine); }
}

/* the scissor of the main target (flush_draw and the native draw): the game scissor in the rotated 240 x 400 buffer */
static void stage_scissor(void) {
        int left = 240 - sci_y1;
        int right = 240 - sci_y0;
        /* F07-3: a 2D or orthographic batch is 1:1 and centered (40 + x). A scissor edge at 0 or 320 reaches the screen edge. A 3D batch scales by 1.25. */
        int ui = pm_wide && (!nb.draw_3d || ui_ortho);
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
static inline int graw_is_pm4(void) { return nb.gni > 0 && nb.g_pal && !nb.g_use5 && gs.gv4_on; }
/* S3 M3: the native raw close. A raw batch with the palette program (pm4, no lights) on the main target, one texture unit at most, no stencil pass: the effect, TEV, texture unit
 * and uniform words come from nd_emit_draw (host tested against the real command stream) and go into the command buffer in place (PERF1-6). The state that is not in
 * those words (program, buffer info, scissor, user clip) is staged through citro3d as before, and C3Di_UpdateContext writes it first. Returns 0 when the batch needs the old close. */
extern void C3Di_UpdateContext(void);
int pm_tex_native_unit0(NdUnit* u);
int pm_tex_native_unit1(NdUnit* u);
static NdTevOut ndn_to; static int ndn_to_ok;
unsigned nd_native_draws, nd_native_refused;
static void ndplan_check(unsigned w0, unsigned w1, int two, int t1only); /* switch ndplan */
static int ndplan_on = -1; /* PERF2: -1 unread; the close calls the cold check only while it is not 0 */
static void ndm_cmp(const NdTmplKey* wk, const NdDraw* wd); /* S3N-3b: switch ndmcheck */
static int ndm_live; /* S3N-3b: a model record is open (set by pm_gpu_ndm_begin) */
static unsigned ndm_old, ndm_refused_total, ndm_nraw_log, ndm_w_n; static unsigned ndm_geo, ndm_dlc0[10]; void pm_gbi_dlcc(unsigned* o); /* S3N-3c: batches of open records that the native raw path refused; walker batches of the record; its geometry */
void pm_log_prof_line(const char* fmt, ...); /* the one line that prints in prof=4 mode (pm_log.c) */
static void ndunit_after(void);
/* PERF1, PROF2-5: the producer class timers (nd.h NDT_*). A 4-deep stack of u32 start ticks and of the ticks of the regions timed inside each open
 * region: a region's exclusive ticks are its total less its children. A deeper nest is counted (ndt_sp) but not timed. Sampled frames only (NDT_B and
 * NDT_BD read the tick only with pm_ph_on). ndt_mdl: the submit is a model draw. */
static uint64_t ndt_sum[NDT_N];
static uint32_t ndt_t0[4], ndt_in[4];
static unsigned ndt_sp;
static uint32_t ndt_top; /* PERF2: the ticks of the outer regions so far (the op class spans subtract them) */
static int cls_cur = -1; static uint32_t cls_t, cls_top0; /* PERF2: the open op class span: its class, start tick, ndt_top at its start */
static int ndt_mdl;
__attribute__((cold, noinline)) void pm_gpu_ndt_push(void) { /* sampled frames only: out of the hot callers */
    if (ndt_sp < 4u) { ndt_t0[ndt_sp] = (uint32_t)svcGetSystemTick(); ndt_in[ndt_sp] = 0; }
    ndt_sp++;
}
__attribute__((cold, noinline)) void pm_gpu_ndt_pop(int k) {
    if (!ndt_sp) return;
    unsigned i = --ndt_sp;
    if (i >= 4u) return;
    uint32_t tot = (uint32_t)svcGetSystemTick() - ndt_t0[i], in = ndt_in[i];
    ndt_sum[k] += tot > in ? tot - in : 0u;
    if (i) ndt_in[i - 1] += tot; else ndt_top += tot;
}
/* PERF2 step 6: the op class timer of the walk (sampled frames, walk_dbg_op with WD_PH). Outside the regions only (a region times its own ops); a span ends at
 * the next class change, its ticks less the outer regions inside it go to its class. One tick read per change of class. */
__attribute__((cold, noinline)) void pm_gpu_ndt_cls(int k) {
    if (ndt_sp || k == cls_cur) return;
    const uint32_t now = (uint32_t)svcGetSystemTick();
    if (cls_cur >= 0) { const uint32_t tot = now - cls_t, in = ndt_top - cls_top0; ndt_sum[cls_cur] += tot > in ? tot - in : 0u; }
    cls_cur = k; cls_t = now; cls_top0 = ndt_top;
}
void pm_gpu_ndt_take(uint64_t o[NDT_N]) { for (int k = 0; k < NDT_N; k++) { o[k] = ndt_sum[k]; ndt_sum[k] = 0; } }
/* S3N-2: the submit of one native draw, moved from nd_raw_native as it was (the state staging, the shadow, citro3d's context update, the emit and the
 * counters). The palette and the NdDraw are built by the caller before. Model draws (nd_model.c, S3N-4) use this too. */
static unsigned ndn_tgen;
static __attribute__((noinline)) void pm5_upload_draw(const NdDraw* d);
static __attribute__((cold, noinline)) void t1log_draw(const NdDraw* d);
static int t1l_on = -1; /* switch t1log: -1 until t1log_draw reads it */
static __attribute__((cold, noinline)) void submit_dbg(const NdDraw* d, int tex);
static __attribute__((cold, noinline)) void pm5tex1_log(const NdDraw* d);
/* FX1-3 orbdbg (debug tool): one line per draw inside an effect list (the tag from pm_gbi_dbg_orb_tag, nd_walk.c). path: walk (flush_draw) or native (nd_raw_native). */
static __attribute__((cold, noinline)) void orb_log(const char* path, int z, int memb, int i8_0, int i8_1, unsigned tris, int graw) {
    extern unsigned pm_gbi_dbg_orb_tag(void);
    unsigned ot_ = pm_gbi_dbg_orb_tag();
    if (!ot_ || !PMDL("orbdbg")) return;
    const PmTevCh* la = gs.combine == 8 && gs.plan.n > 0 ? &gs.plan.st[gs.plan.n - 1].g[1] : NULL; /* the alpha channel of the last stage */
    pm_log("orbdbg draw %s f%u list %u cmb %d stages %d uses t0 %d t1 %d noise %d i8_0 %d i8_1 %d blend %d memb %d atest %d z %d graw %d tris %u prim %08x cconst %08x afin src %u %u %u op %u %u %u fn %u sc %u",
           path, pm_frame_count(), ot_, gs.combine, gs.combine == 8 ? gs.plan.n : -1, gs.plan.uses_tex, gs.plan.uses_tex1, gs.plan.uses_noise, i8_0, i8_1, gs.r_blend, memb, gs.r_atest, z, graw, tris, prim, cconst,
           la ? la->src[0] : 99u, la ? la->src[1] : 99u, la ? la->src[2] : 99u, la ? la->op[0] : 99u, la ? la->op[1] : 99u, la ? la->op[2] : 99u,
           la ? la->func : 99u, la ? la->scale : 99u);
}
/* PERF1-6: C3Di_UpdateContext writes nothing when (objdump of libcitro3d.a, this devkitPro): no flag it tests is set in the context word at +32 (frame buffer,
 * viewport, scissor, program, attribute and buffer info, effect, texture units, texture status, proc tex, texenv buffer, gas, texenvs, light env), no light
 * env is bound (+268: it calls C3Di_LightEnvUpdate whenever one is), no fixed attribute is dirty (+0x208), the gas mode (+412 bits 0 to 2) is not 7 (it calls
 * C3Di_GasUpdate then), and no uniform is dirty (C3D_UpdateUniforms of both shaders sends only dirty ones; a program bind sets the program flag). Then the call
 * is skipped: the command stream is the same. Uniform arrays not word aligned: always update. */
#define C3D_UPD_FLAGS (0x4u | 0x8u | 0x10u | 0x20u | 0x40u | 0x80u | 0x100u | 0x200u | 0x400u | 0x4000u | 0x718000u | 0x20000u | 0x3800000u | 0xfc000000u)
static int c3d_upd_needed(void) {
    const unsigned char* c = __C3D_Context;
    const unsigned gm = *(const unsigned*)(c + 412) & 7u;
    /* PERF2: the fog LUT flag (0x20000) is cleared by C3Di_UpdateContext only when the fog mode (+412 bits 0 to 2) is not 0; with fog off it writes nothing
     * and the flag stays set (nd_leave_native sets it). It counted as dirty at every native draw (subcnt: upd equal to nat), so the call ran per draw. */
    if (*(const volatile unsigned*)(c + 32) & (gm ? C3D_UPD_FLAGS : C3D_UPD_FLAGS & ~0x20000u)) return 1;
    if (*(void* const*)(c + 268)) return 1;
    if (*(const uint16_t*)(c + 0x208)) return 1;
    if (gm == 7u) return 1;
    if (((uintptr_t)C3D_FVUnifDirty | (uintptr_t)C3D_IVUnifDirty) & 3u) return 1;
    const uint32_t* f = (const uint32_t*)(const void*)C3D_FVUnifDirty;
    const uint32_t* iv = (const uint32_t*)(const void*)C3D_IVUnifDirty;
    uint32_t a = iv[0] | iv[1] | C3D_BoolUnifsDirty[0] | C3D_BoolUnifsDirty[1];
    for (unsigned i = 0; i < 2u * C3D_FVUNIF_COUNT / 4u; i++) a |= f[i];
    return a != 0;
}
static __attribute__((cold, noinline)) void cmd_split_forced(void);
/* FB-3: a program 5 draw (model, sprite) writes its pm5 block here, once; the copy keeps the caller's NdDraw; uvx goes to the pm5 uniform (PERF2: cold, out of line) */
static __attribute__((cold, noinline)) const NdDraw* sub_pm5(const NdDraw* dp, NdDraw* d5) {
    *d5 = *dp; pm5_upload_draw(d5); d5->no_uni = 1; d5->npal = 0; if (d5->has_uvx) d5->loc_uvx = (uint16_t)u5_uvx;
    return d5;
}
static void nd_native_submit(const NdDraw* dp, int tex) {
    NDT_B(tsub); /* PERF1: the native submit class */
    { u32 *cb_, csz_, coff_; GPUCMD_GetBuffer(&cb_, &csz_, &coff_); if (coff_ + ND_EMIT_MAX + 1024u >= csz_) cmd_split_forced(); } /* PERF1-6: the draw words go straight into the command buffer: near its end the frame is split first (as nd_raw_native) */
    pm_gbi_draw_note(1); /* WCEN3: a native draw (per producer, the former pm_gbi.c) */
    /* FB-3: every program 5 draw (model, sprite, raw) writes its pm5 block here, once. The copy keeps the caller's NdDraw; uvx goes to the pm5 uniform. */
    NdDraw d5;
    if (__builtin_expect(dp->use5 && !dp->no_uni, 0)) dp = sub_pm5(dp, &d5); /* PERF2: out of line */
    const NdDraw* d = dp;
    if (ndn_tgen != nd_tmpl_gen()) { ndn_tgen = nd_tmpl_gen(); nd_shadow_reset(&ndn_sh); sc_shr++; } /* FB-2: a template id was taken over by another key (LRU): the id shadow is stale */
    prog_use(d->use5 ? 4 : 3);
    { /* FB-1: a model draw indexes the static area (chunk 0), a walker draw the ring chunk in use */
        const void* bb = (ndt_mdl || nb.gi_kind == 1) ? (const void*)nb.rchunk[0].vtx : (const void*)nb.gring;
        if (nb.buf_cur != 2 || buf_bp != bb) { C3D_BufInfo* gb = C3D_GetBufInfo(); BufInfo_Init(gb); BufInfo_Add(gb, bb, 16, 3, 0x210); nb.buf_cur = 2; buf_bp = bb; }
    }
    if (gs.off_active) scissor_off(); else stage_scissor(); /* OFF-1: an off-screen target has its own scissor (citro3d state, not in the template words) */
    sh_user_clip(1);
    if (stencil_on) { stc_set(6, false, GPU_ALWAYS, 0, 0xff, 0); stencil_on = 0; } /* the citro3d effect copy says off too, as flush_draw does: nd_leave_native sends that copy again */
    if (!ndn_mode) { nd_shadow_reset(&ndn_sh); ndn_mode = 1; sc_shr++; }
    if (*(volatile unsigned*)(__C3D_Context + 32) & 0x3800000u) { ndn_sh.cfg_valid = 0; ndn_sh.u_on[0] = ndn_sh.u_on[1] = 0; } /* a dirty texture bind: C3Di_UpdateContext rebuilds the unit enable word from the units citro3d holds (a null unit 0 after a slot drop writes it off) and sends their blocks: the native shadow of both is stale, the native words write them again */
    if (c3d_upd_needed()) { C3Di_UpdateContext(); sc_upd++; } /* what citro3d staged (program, buffers, scissor, an older TEV) goes out first, the native words then overwrite it */
    { u32 *cb, csz, coff; GPUCMD_GetBuffer(&cb, &csz, &coff); GPUCMD_SetBufferOffset(coff + nd_emit_draw(cb + coff, &ndn_sh, d)); } /* PERF1-6: emitted in place (no copy) */
    if (tex) ndunit_after();
    *(volatile unsigned*)(__C3D_Context + 32) |= 2u; /* C3DiF_DrawUsed, as the citro3d draw sets it: citro3d then sends the framebuffer flush at a target change, a split and the frame end */
    if (ndref_on) ndref_cmd();
    nd_native_draws++;
    dv_draw++;
    draws_frame++;
    PH_ADD(C_DRAW, 1);
    PH_ADD(C_VTXSLOT, nb.gni);
    nb.gi_start = nb.gidx_n;
    nb.gni = 0; nb.pal_n = 0;
    batch_identity = 0;
    drew = 1;
    if (!gs.off_active) { /* B2: the main target changed, the grab slot is old (as flush_draw) */
        if (nb.rect_y1 <= nb.rect_y0) gs.grab_dirty = 1;
        else if (grab_dy1 <= grab_dy0) { grab_dy0 = nb.rect_y0; grab_dy1 = nb.rect_y1; }
        else { if (nb.rect_y0 < grab_dy0) grab_dy0 = nb.rect_y0; if (nb.rect_y1 > grab_dy1) grab_dy1 = nb.rect_y1; }
    }
    if (__builtin_expect(t1l_on | rp_on_ | (d->use5 & d->has_uvx), 0)) submit_dbg(d, tex); /* PERF1: the debug logs are tested here (cached switches, the draw flags), not called per draw */
    dc_kick();
    NDT_E(tsub, NDT_SUB);
}
/* the debug logs of one native submit (t1log, pm5tex1, rectpost) */
static __attribute__((cold, noinline)) void submit_dbg(const NdDraw* d, int tex) {
    if (t1l_on) t1log_draw(d);
    pm5tex1_log(d);
    rp_tmpl_ = d->tmpl; rp_u0_ = d->u[0].dim != 0;
    if (rp_on_) rp_draw("nat", tex); /* -1 until the first read */
}

/* S3N-3c: the palette 0 product post x game (the native model draw, nd_model.c) and the submit of one native model draw (emit 1, S3N-4):
 * the attribute locations and the index address of its static range. */
void pm_gpu_pcomp_native(float o[16], const float m[4][4]) { nd_mtx_pcomp(o, gv_post.m, m); }
/* PM5-3: the pm5 block of a model draw from its NdDraw fields (the same calls as pm5_upload, values of the draw) */
static __attribute__((noinline)) void pm5_upload_draw(const NdDraw* d) {
    prog_use(4);
    u5set(u5_uvso, d->uvso); u5set(u5_cmul, d->cmul); u5set(u5_cadd, d->cadd);
    u5vec(u5_zmix, d->zmix[0], zmix_y_dbg(d->zmix[1]), 0.f, 0.f);
    u5set(u5_lflag, d->p5->fx.lflag); u5set(u5_fogu, d->p5->fx.fogu);
    if (d->p5->fx.lflag[0] != 0.f) flush_light_tab(&d->p5->fx); /* the model range carries its own light block */
    u5mtx(u5_pal, (const C3D_Mtx*)d->pal[0]);
    for (int r = 0; r < 3; r++) u5vec(u5_pal + 4 + r, d->p5->nrm[r][0], d->p5->nrm[r][1], d->p5->nrm[r][2], 0.f);
    u5_valid = 1;
    gv_palup_n++; pm_dc_c[DC_PALUP]++;
}
/* FB-3 t1log (debug switch, log only): one line per tex1 draw of a native record or raw batch, with the unit 1 words and the uvx values sent */
static __attribute__((cold, noinline)) void t1log_draw(const NdDraw* d) {
    static int t1l_n; extern unsigned pm_frame_count(void);
    if (t1l_on < 0) t1l_on = pm_debug_has("t1log");
    if (t1l_on && d->has_uvx && d->use5 && pm_frame_count() >= 100 && t1l_n++ < 400) pm_log("t1draw f%u use5 %d loc_uvx %u u1 dim %x addr %x param %x lod %x type %x uvx %.4f %.4f %.4f %.4f", pm_frame_count(), d->use5, d->loc_uvx, d->u[1].dim, d->u[1].addr, d->u[1].param, d->u[1].lod, d->u[1].type, d->uvx[0], d->uvx[1], d->uvx[2], d->uvx[3]);
}
/* FB-3 pm5tex1: one line per producer the first time it submits a program 5 draw with tex1 (the unit 1 uv map, uvx): the pm5 tex1 path is in use */
void pm_gbi_cur_prod_name(char* b, int n);
static __attribute__((cold, noinline)) void pm5tex1_log(const NdDraw* d) {
    static char seen[24][48]; static unsigned nseen;
    if (!d->use5 || !d->has_uvx || pm_log_level() < 2) return; /* pm_log is an info line */
    char prod[48]; pm_gbi_cur_prod_name(prod, sizeof prod);
    for (unsigned i = 0; i < nseen; i++) if (strcmp(seen[i], prod) == 0) return;
    if (nseen < 24) { strncpy(seen[nseen], prod, 47); seen[nseen][47] = 0; nseen++; }
    extern unsigned pm_frame_count(void);
    pm_log("pm5tex1 %s f%u", prod, pm_frame_count());
}
void pm_gpu_native_submit_draw(NdDraw* d, const uint16_t* sd, unsigned nidx, int tex) {
    rp_src_ = 2;
    d->loc_pal = (unsigned)u4_pal; d->loc_var = (unsigned)u4_uvso;
    if (d->has_nzo) d->loc_nzo = (unsigned)(d->use5 ? u5_nzo : u4_nzo); /* FB-3 part 2: the noise offset of the program of the draw */
    d->idx_word = 0x80000000u | ((uint32_t)osConvertVirtToPhys((void*)sd) - 0x18000000u);
    d->nidx = nidx;
    ndt_mdl = 1;
    nd_native_submit(d, tex);
    ndt_mdl = 0;
}


const float (*pm_gbi_combined_ptr(void))[4];
static unsigned nraw_why[10]; /* nd_raw_native refusals (the batch is dropped): 4 pm5 zmark, 6 plan tex1 without unit 1 words, 8 unit 0 (off-screen image or debug override), 9 template table full. Slots 1, 2, 3, 5 and 7 are no longer refusals. */
void pm_gpu_nraw_why(unsigned* o) { for (unsigned i = 0; i < 10; i++) o[i] = nraw_why[i]; }
void pm_gpu_fb2_stat(unsigned* o); /* FB-2 log: template high mark, evictions, forced splits */
/* OFF-2: the unit 0 words of the grab copy (grab_tex), as the walker bind (dc_bind) sends them. The copy is one fixed buffer: force keeps the words in
 * every draw of the grab, as the walker binds it at every draw. 1 always. */
static int tex_native_words(NdUnit* u, const C3D_Tex* tp) {
    u->force = 1;
    u->dim = tp->dim;
    u->param = tp->param;
    u->lod = tp->lodParam;
    u->addr = (uint32_t)osConvertVirtToPhys(tp->data) >> 3;
    u->type = (uint32_t)tp->fmt;
    return 1;
}
static int grab_native_unit0(NdUnit* u) { return tex_native_words(u, &grab_tex); }
/* PM5-2: the program 5 uniform block of the walker state (the same calls as flush_draw's pm5 block, no cache: the native batch writes them
 * in order before its draw). Palette entries: the pm5 register layout, 7 rows per entry (4 position, 3 normal). */
static __attribute__((noinline)) void pm5_upload(void) {
    prog_use(4);
    u5set(u5_uvso, nb.g_uvso); u5set(u5_cmul, nb.g_cmul); u5set(u5_cadd, nb.g_cadd);
    u5vec(u5_zmix, nb.g_zmix[0], zmix_y_dbg(nb.g_zmix[1]), 0.f, 0.f);
    u5set(u5_lflag, nb.g_fx.lflag); u5set(u5_fogu, nb.g_fx.fogu);
    if (nb.g_fx.lflag[0] != 0.f) flush_light_tab(&nb.g_fx);
    for (unsigned k = 0; k < nb.pal_n; k++) {
        unsigned gm = nb.pal_g[k];
        if (gs.pcomp_gen[gm] != post_gen) pcomp_calc(gm);
        u5mtx(u5_pal + 7 * (int)k, &pcomp[gm]);
        if (pal_ntab) for (int r = 0; r < 3; r++) u5vec(u5_pal + 7 * (int)k + 4 + r, pal_ntab[gm][r][0], pal_ntab[gm][r][1], pal_ntab[gm][r][2], 0.f);
        gv_palup_n++; pm_dc_c[DC_PALUP]++;
    }
    u5_valid = 1;
}
static unsigned cmd_split_n;
unsigned pm_gpu_cmd_split_n(void) { return cmd_split_n; }
void pm_gpu_fb2_stat(unsigned* o) { o[0] = nd_tmpl_hi(); o[1] = nd_tmpl_gen(); o[2] = cmd_split_n; }
/* FB-2: the forced split of nd_raw_native (kick_check's split without its draw threshold, run inside a raw close: the open batch's ring words stay in place).
 * The split adds a GX entry, so the queue room comes first (HANG1a: the GX queue of 32 entries overflowed at 30k forced splits, svcBreak in gxCmdQueueAdd). */
void pm_gx_room(void);
static __attribute__((cold, noinline)) void cmd_split_forced(void) {
    NDT_B(tk); /* PERF2: the kick class */
    pm_gx_room();
    split_frame(); /* the same split as the grab and the frame split (grab_split counts it: the first split of a frame queues the clear) */
    kick_n++; pm_ph_kick_n++; cmd_split_n++;
    NDT_E(tk, NDT_KICK);
}
/* FB-2: the unit 2 words of the noise texture (fixed for the run: force 0, the shadow decides when they go out) */
static void noise_native_words(NdUnit* u) {
    u->force = 0;
    u->dim = noise_tex.dim; u->param = noise_tex.param; u->lod = noise_tex.lodParam;
    u->addr = (uint32_t)osConvertVirtToPhys(noise_tex.data) >> 3; u->type = (uint32_t)noise_tex.fmt;
}
/* FB-3 part 2: the noise unit and offset of a native model draw (nd_model.c, a plan with noise): the same words as the raw path (d.u[2], d.nzo) */
int pm_gpu_noise_ok(void) { return noise_init(); }
void pm_gpu_noise_draw(NdUnit* u, float* nzo) { noise_native_words(u); nzo[0] = 120.f / 64.f; nzo[1] = 200.f / 64.f; nzo[2] = noise_off[0]; nzo[3] = noise_off[1]; }
/* the TEV inputs of a native raw batch (nk bytes combine to noise, the colours, the plan): the change test of the stage words */
typedef struct { uint8_t b[16]; uint32_t prim, cconst, mixcol, plan_gen; } NdTevK;
_Static_assert(offsetof(NdTmplKey, noise) + 1 - offsetof(NdTmplKey, combine) == 16, "NdTevK holds the TEV bytes of NdTmplKey");
static NdTevK ndn_tk;
/* ndref and the draw census record the TevKey layout (3ds/tests/ndref/FORMAT.md): built only when one of them is on */
static __attribute__((cold, noinline)) void raw_dbg_keys(int z, int tex, int tex1, int i8_0, int i8_1) {
    TevKey tk;
    memset(&tk, 0, sizeof tk);
    tk.combine = gs.combine; tk.tex = tex; tk.tex1 = tex1; tk.i8_0 = i8_0; tk.i8_1 = i8_1; tk.rgb_scale = rgb_scale; tk.r_mix = gs.r_mix; tk.draw_3d = nb.draw_3d; tk.rect_2d = nb.rect_2d;
    tk.dview = dv_mode(); tk.off_active = (uint8_t)gs.off_active; tk.off_i8 = off_i8; tk.r_memblend = r_memblend; tk.r_replace = r_replace; tk.r_blend = gs.r_blend;
    tk.prim = prim; tk.cconst = cconst; tk.mixcol = r_mixcol; tk.plan_gen = gs.combine == 8 ? plan_gen : 0;
    if (ndref_on) { ndref_nz = 0; ndref_raw = 1; ndref_draw(&tk, z, 1, 4, (unsigned)nb.g_cullm); ndref_state(0, z); }
    if (cen_fr >= 0) cen_add(&tk, z, 1, 4, (unsigned)nb.g_cullm, grab_bind && tex ? 0x20000000u : tex ? pm_tex_census_id(0) : 0u, 0u, nb.gni / 3u);
}
/* POND-M1 and POND-S debug lines of a native raw batch (switch waterlog, frames 200 and 201): out of line, the close only tests wl_on */
static __attribute__((cold, noinline)) void wl_log(const NdDraw* d, int z, int decal2, int id, int tex, int tex1, const NdUnit* u0, const NdUnit* u1) {
    unsigned fn, wr; wl_depth(z, 0, gs.r_zcmp, gs.r_zupd, gs.r_decal, gs.r_nocol, &fn, &wr);
    extern void pm_gbi_sticky(unsigned* tss, unsigned* geo, unsigned* cw0, unsigned* cw1); unsigned wtss_, wgeo_, wcw0_, wcw1_; pm_gbi_sticky(&wtss_, &wgeo_, &wcw0_, &wcw1_); /* POND-M1: walker combine words */
    pm_log("wdraw f%u nat om %08x zs %u fn %u wr %u dec2 %d zc %d zu %d dec %d z %d bl %d tmpl %d tc %08x t1 %d u0 %08x u1 %08x pz %04x prim %08x cc %08x cmb %u cw %08x %08x zm %.4f %.6f ca %.3f %.3f uvx %.4f %.4f %.4f %.4f ts0 %08x %08x tris %u",
           pm_frame_count(), dbg_omode_l, (dbg_omode_l >> 2) & 1u, fn, wr, decal2, gs.r_zcmp, gs.r_zupd, gs.r_decal, z, gs.r_blend, id, tex ? pm_tex_census_id(0) : 0u, tex1,
           (unsigned)u0->addr, (unsigned)u1->addr, wl_primz, prim, cconst, (unsigned)gs.combine, wcw0_, wcw1_,
           (double)d->zmix[0], (double)d->zmix[1], (double)nb.g_cmul[3], (double)nb.g_cadd[3], (double)t1_s[0], (double)t1_s[1], (double)t1_s[2], (double)t1_s[3],
           (unsigned)ndn_to.st[0][0], (unsigned)ndn_to.st[0][1], nb.gni / 3u); /* G_MDSFT_ZSRCSEL is 2 */
    if (gs.r_blend && !((dbg_omode_l >> 2) & 1u)) { /* POND-S: one line per surface batch (blend on, not G_ZS_PRIM): texel alpha share and vertex alpha range of the batch */
        extern int pm_tex_alpha0_permille(int unit);
        unsigned vmin = 255u, vmax = 0u;
        for (unsigned i = 0; nb.gring && nb.gidx && i < nb.gni; i++) { unsigned a = (nb.gi_kind == 1 ? nb.rchunk[0].vtx : nb.gring)[16u * nb.gidx[nb.gi_start + i] + 15u]; if (a < vmin) vmin = a; if (a > vmax) vmax = a; }
        pm_log("wsurf f%u a0 %d a1 %d vamin %u vamax %u tris %u tex0 %08x tex1 %08x", pm_frame_count(), pm_tex_alpha0_permille(0), pm_tex_alpha0_permille(1), vmin, vmax, nb.gni / 3u, (unsigned)u0->addr, (unsigned)u1->addr);
    }
}
/* S6-2c-3 decal pass A of a native raw batch: the stencil mark, no colour, no depth write (out of line: rare, keeps nd_raw_native small) */
static __attribute__((noinline)) void raw_decal_pass_a(const NdTmplKey* nk, const NdDraw* d, int tex) {
    NdTmplKey ka = *nk;
    ka.pass = 1;
    int ida = nd_tmpl_build(&ka);
    if (ida >= 0) { NdDraw da = *d; da.tmpl = ida; nd_native_submit(&da, tex); }
}
/* FB-1 ZM: pass 3 is the same triangles at the constant depth r_zmark_zn (z clip = zn * w: palette row 2 := zn * row 3, as nd_model.c). Stencil EQUAL, cleared as it passes */
static __attribute__((noinline)) void raw_zmark_pass(const NdTmplKey* nk, const NdDraw* d, const float (*palk)[16], unsigned npal, int tex) {
    NdTmplKey kb = *nk;
    kb.pass = 3; kb.zmark = 0;
    int idb = nd_tmpl_build(&kb);
    if (idb >= 0) {
        float palz[ND_NPAL][16];
        for (unsigned k = 0; k < npal; k++) {
            memcpy(palz[k], palk[k], 64);
            for (int i = 0; i < 4; i++) palz[k][8 + i] = gs.r_zmark_zn * palk[k][12 + i];
        }
        NdDraw db = *d;
        db.tmpl = idb; db.pal = (const float(*)[16])palz; db.npal = npal;
        nd_native_submit(&db, tex);
    }
}
/* PERF2 step 4: the rare parts of nd_raw_native, out of line (cold): the pm4 one-unit close stays in the hot span */
static __attribute__((cold, noinline)) int raw_unit0_rare(NdUnit* u0) { /* OFF-2 grab copy, OFF-3 the snapshot of the target of this pass */
    int t = grab_bind ? grab_native_unit0(u0) : pm_tex_native_unit0(u0);
    if (grab_bind && PMDL("grablog")) pm_log_d("grabw f%u nat dim %08x param %08x lod %08x addr %08x type %u", pm_frame_count(), u0->dim, u0->param, u0->lod, u0->addr, u0->type); /* FB-1 GRAB: the words of the grab slot, native */
    if (t == 2 && gs.off_active && pm_tex_off_idx() == gs.off_cur && gs.offs[gs.off_cur].snap_ready) t = tex_native_words(u0, &gs.offs[gs.off_cur].snap);
    return t;
}
static __attribute__((cold, noinline)) int raw_unit1(NdUnit* u1) { /* SGEN-2: unit 1 words; FB-2: the aux image is the target of this pass: its snapshot (OFF-3, as flush_draw) */
    int t1 = pm_tex_native_unit1(u1);
    if (t1 == 2 && gs.off_active && pm_tex_aux_off_idx() == gs.off_cur && gs.offs[gs.off_cur].snap_ready) t1 = tex_native_words(u1, &gs.offs[gs.off_cur].snap);
    return t1;
}
static __attribute__((cold, noinline)) void raw_draw_rare(NdDraw* d, const NdUnit* u1, int noise, int tex1) {
    d->u[1] = *u1;
    if (noise) { /* FB-2: unit 2 (the noise texture) and its offset uniform, as the walker's noise block of flush_draw */
        noise_native_words(&d->u[2]);
        d->nzo[0] = 120.f / 64.f; d->nzo[1] = 200.f / 64.f; d->nzo[2] = noise_off[0]; d->nzo[3] = noise_off[1];
        d->loc_nzo = (uint16_t)(nb.g_use5 ? u5_nzo : u4_nzo); d->has_nzo = (nb.g_use5 ? u5_nzo : u4_nzo) >= 0;
    }
    if (tex1) { memcpy(d->uvx, t1_s, 16); d->loc_uvx = (uint16_t)(nb.g_use5 ? u5_uvx : u4_uvx); d->has_uvx = 1; }
    if (nb.g_use5) { pm5_upload(); d->use5 = 1; d->no_uni = 1; d->npal = 0; } /* PM5-2; FB-3: the uvx uniform of pm5 is d->loc_uvx (set above) */
}
static int nd_raw_native(void) {
    nd_tmpl_pin(); /* FB-2: the template ids of this batch stay valid until its submit */
    if (!(nb.gni > 0 && nb.g_pal && gs.gv4_on && (!nb.g_use5 || gv5_on))) { cen_why = nb.g_use5 ? CW_PM5 : CW_OTHER; return 0; } /* CEN-1a; PM5-2: program 5 batches go native too */
    { u32 *cb, csz, coff; GPUCMD_GetBuffer(&cb, &csz, &coff); if (coff + ND_EMIT_MAX + 1024u >= csz) cmd_split_forced(); } /* FB-2: the draw words go into the command buffer with no bounds check: near its end the batch is split first (nraw1 is no longer a refusal) */
    /* CEN-1b: one refusal line per reason, same order and counters as the old combined line */
    if (gs.r_zmark && nb.g_use5) { nd_native_refused++; nraw_why[4]++; cen_why = CW_NRK(4); return 0; } /* FB-1 ZM: pm4 zmark is native (two pass); pm5 zmark stays on the CPU path (FB-3) */
    int z = gs.z_on && (!nb.rect_2d || nb.rect_z) && !gs.off_active; /* OFF-1: as flush_draw, no depth test on an off-screen target */
    NdUnit u0;
    memset(&u0, 0, sizeof u0);
    int t = 0;
    if (gs.combine >= 2) { /* OFF-2: a grab draw binds the copy. OFF-3: an off-screen image (2) that is the target of this pass is read from its snapshot (FX2a, flush_draw made it) */
        t = (grab_bind | gs.off_active) ? raw_unit0_rare(&u0) : pm_tex_native_unit0(&u0); /* PERF2: the grab and off-screen sources out of line */
        if (t < 0) { nd_native_refused++; nraw_why[8]++; cen_why = CW_NRK(8); return 0; }
    }
    NdUnit u1;
    memset(&u1, 0, sizeof u1);
    int t1 = 0;
    if (gs.combine == 8 && gs.plan.uses_tex1) { t1 = raw_unit1(&u1); if (t1 < 0) { nd_native_refused++; nraw_why[6]++; cen_why = CW_NRK(6); return 0; } } /* SGEN-2: unit 1 words and the uv map uniform (PERF2: out of line) */
    const int tex = t > 0, i8_0 = tex && pm_tex_i8(0), tex1 = t1 > 0, i8_1 = tex1 && pm_tex_i8(1);
    const int noise = gs.combine == 8 && gs.plan.uses_noise && noise_init(); /* FB-2: as flush_draw; noise_init 0 (no memory): the walker draws without it, so does the native draw */
    NdTmplKey nk;
    memset(&nk, 0, sizeof nk);
    nk.pass = 0; nk.raw = 1; nk.cull = (uint8_t)nb.g_cullm;
    nk.z = (uint8_t)z; nk.zmark = (uint8_t)(z && gs.r_zmark); /* FB-1 ZM: pass 1 tests the model and marks the stencil (nd_tmpl zmark branch) */ nk.zcmp = (uint8_t)gs.r_zcmp; nk.zupd = (uint8_t)gs.r_zupd; nk.nocol = (uint8_t)gs.r_nocol; nk.decal = (uint8_t)gs.r_decal;
    { float db = decal_bias(); memcpy(&nk.dbias, &db, 4); }
    nk.atest_on = gs.r_atest >= 0; nk.aref = (uint8_t)(gs.r_atest >= 0 ? gs.r_atest : 0);
    nk.combine = (uint8_t)gs.combine; nk.tex = (uint8_t)tex; nk.tex1 = (uint8_t)tex1; nk.i8_0 = (uint8_t)i8_0; nk.i8_1 = (uint8_t)i8_1; nk.rgb_scale = (uint8_t)rgb_scale; nk.r_mix = (uint8_t)gs.r_mix;
    nk.draw_3d = (uint8_t)nb.draw_3d; nk.rect_2d = (uint8_t)nb.rect_2d; nk.dview = dv_mode() != 0; nk.off_active = (uint8_t)gs.off_active; nk.off_i8 = (uint8_t)off_i8; nk.r_memblend = (uint8_t)r_memblend;
    nk.r_replace = (uint8_t)r_replace; nk.r_blend = (uint8_t)gs.r_blend; nk.noise = (uint8_t)noise;
    const int decal2 = z && gs.r_zcmp && gs.r_decal && gs.r_blend && !gs.r_nocol && nb.draw_3d && !gs.r_zmark; /* S6-2c-3: as flush_draw: pass A marks the stencil, pass B draws where it is set (nd_model_state, S5-1 b) */
    if (decal2) nk.pass = 2;
    if (gs.combine == 8) { nk.w0 = plan_k[0]; nk.w1 = plan_k[1]; nk.two = (uint8_t)plan_k[2]; nk.t1only = (uint8_t)plan_k[3]; }
    int id = nd_tmpl_build(&nk);
    if (id < 0) { nd_native_refused++; nraw_why[9]++; cen_why = CW_NRK(9); return 0; }
    if (gs.combine == 8 && __builtin_expect(ndplan_on != 0, 0)) ndplan_check(nk.w0, nk.w1, nk.two, nk.t1only);
    { extern unsigned pm_gbi_dbg_orb_tag(void); if (__builtin_expect(pm_gbi_dbg_orb_tag() != 0, 0)) orb_log("native", z, -1, i8_0, i8_1, nb.gni / 3u, 1); } /* FX1-3; PERF2: the cold call only for a tagged list (else GCC predicts the close never executed) */
    /* from here the batch is drawn natively */
    if (grab_bind) pm_tex_bind_reset(); /* as flush_draw: the next walker bind of unit 0 sends its own words */
    { /* the stage words with the colours, built when an input of nd_tev_build changes: the TEV bytes of nk, the colours and the plan */
        NdTevK tk;
        memcpy(tk.b, &nk.combine, sizeof tk.b);
        tk.prim = prim; tk.cconst = cconst; tk.mixcol = r_mixcol; tk.plan_gen = gs.combine == 8 ? plan_gen : 0;
        if (!ndn_to_ok || memcmp(&tk, &ndn_tk, sizeof tk) != 0) {
            NdTevIn ti = { &nk, &gs.plan, prim, cconst, r_mixcol };
            nd_tev_build(&ti, &ndn_to);
            ndn_tk = tk; ndn_to_ok = 1;
        }
    }
    if (ndref_on || cen_fr >= 0) raw_dbg_keys(z, tex, tex1, i8_0, i8_1);
    cen_draws++; cen_tris += nb.gni / 3u;
    dc_on = 0;
    float palk[ND_NPAL][16];
    unsigned npal = nb.g_use5 ? 0u : nb.pal_n < ND_NPAL ? nb.pal_n : ND_NPAL; /* pm5: pm5_upload sends the palette (A4: no copy here) */
    for (unsigned k = 0; k < npal; k++) {
        unsigned gm = nb.pal_g[k];
        if (gs.pcomp_gen[gm] != post_gen) pcomp_calc(gm);
        memcpy(palk[k], pcomp[gm].m, 64);
        gv_palup_n++; pm_dc_c[DC_PALUP]++;
    }
    NdDraw d;
    memset(&d, 0, sizeof d);
    d.tmpl = id;
    for (int i = 0; i < 6; i++) d.tev_col[i] = ndn_to.st[i][3];
    d.buf_col = ndn_to.buf_color; d.has_buf = (uint8_t)ndn_to.buf_set;
    d.u[0] = u0;
    d.pal = (const float(*)[16])palk; d.npal = npal; d.loc_pal = (unsigned)u4_pal; d.loc_var = (unsigned)u4_uvso;
    if (__builtin_expect(noise | tex1 | nb.g_use5 | (u1.dim != 0), 0)) raw_draw_rare(&d, &u1, noise, tex1); /* PERF2: unit 1, the noise unit and pm5 out of line */
    memcpy(d.uvso, nb.g_uvso, 16); memcpy(d.cmul, nb.g_cmul, 16); memcpy(d.cadd, nb.g_cadd, 16);
    d.zmix[0] = nb.g_zmix[0]; d.zmix[1] = zmix_y_dbg(nb.g_zmix[1]);
    d.idx_word = 0x80000000u | ((uint32_t)osConvertVirtToPhys(nb.gidx + nb.gi_start) - 0x18000000u);
    d.nidx = nb.gni;
    if (ndm_live) ndm_cmp(&nk, &d);
    if (wl_on()) wl_log(&d, z, decal2, id, tex, tex1, &u0, &u1);
    if (decal2) raw_decal_pass_a(&nk, &d, tex);
    nd_native_submit(&d, tex);
    if (z && gs.r_zmark) raw_zmark_pass(&nk, &d, (const float(*)[16])palk, npal, tex);
    if (rd_st == 2) orbreg_dump(); /* FX1-2 plan 5 orbreg: the register shadow at a glow draw */
    return 1;
}
/* PERF2 step 4: the rare parts of flush_draw out of line */
static __attribute__((cold, noinline)) void flush_h03_log(void) {
    extern unsigned pm_gbi_dbg_orb_tag(void); unsigned ot_ = pm_gbi_dbg_orb_tag();
    if (ot_ && PMDL("orbdbg")) pm_log("orbdbg skip f%u list %u H03 cmb %d", pm_frame_count(), ot_, gs.combine);
}
static __attribute__((cold, noinline)) void flush_off_snap(void) {
    const int snap0 = gs.combine >= 2 && !grab_bind && pm_tex_off_idx() == gs.off_cur;
    const int snap1 = gs.combine == 8 && gs.plan.uses_tex1 && pm_tex_aux_off_idx() == gs.off_cur;
    if (snap0 || snap1) off_snapshot(gs.off_cur);
}
static void flush_draw(void) {
    int graw = nb.gni > 0;
    if (graw) flush_gen++; /* PO3b: the open batch is a raw Vtx batch */
    if (fl_on() && gs.gpu_ok) flush_fl_dbg(graw);
    if (!graw || !gs.gpu_ok) {
        nb.gni = 0; nb.gi_start = nb.gidx_n;
        batch_identity = 0;
        return;
    }
    if (gs.r_nocol && !(gs.z_on && (!nb.rect_2d || nb.rect_z) && gs.r_zupd)) {
        nb.gni = 0; nb.gi_start = nb.gidx_n;
        batch_identity = 0;
        return;
    }
    /* H03: the texture of this batch failed to load (cache full). Skip the batch, a white quad is worse than a missing one. */
    if (gs.combine >= 2 && !grab_bind && pm_tex_failed() && (gs.combine != 8 || gs.plan.uses_tex || gs.plan.uses_tex1)) {
        flush_h03_log(); /* FX1-3: a skipped draw of an effect list is logged too (PERF2: out of line) */
        nb.gni = 0; nb.gi_start = nb.gidx_n;
        batch_identity = 0;
        return;
    }
    if (gs.off_active) flush_off_snap(); /* FX2a: this batch reads the slot it writes (PERF2: out of line) */
    cen_why = CW_OTHER; /* the batch is raw here (the !graw case returned above) */
    { NDT_B(tr); const int nr = nd_raw_native(); NDT_E(tr, NDT_RAW); if (nr) return; } /* PERF2: the raw class (its submit is sub) */
    if (ndm_live) ndm_old++, ndm_refused_total++; /* S3N-3c: a raw batch the native path refused is dropped (nd_raw_native counted the reason, census 0 at the DEL row) */
    nb.gni = 0; nb.gi_start = nb.gidx_n; batch_identity = 0;
    return;
}


/* PG3 O11: byte to float tables, filled with the same divide the code used, so every value is equal. The VFP divide costs about 19 cycles on the O3DS. */
float pm_f255[256], pm_f127[256]; /* pm_f127 is indexed by the byte of a signed char */
static void init_f_tables(void) {
    for (int i = 0; i < 256; i++) { pm_f255[i] = (float)i / 255.f; pm_f127[i] = (float)(signed char)i / 127.f; }
}

/* JT1: DEBUG TOOL, not a finished path. Switch jumptest (read once in pm_gpu_init). A jump is queued at the end of a frame when
 * one second (SYSCLOCK_ARM11 ticks, pm_loop_tick from the main loop) has passed since the last jump. Each jump is a list jump into a 16-byte sub-buffer in linear memory and back. Layout is INFERRED from the PICA documentation
 * (docs/handoff/inv_citro3d_decode.md section 3), not confirmed on hardware:
 *   frame list tail: SIZE0=2 ADDR0=pa(continuation)>>3 SIZE1=2 ADDR1=pa(sub)>>3 JUMP1=1 [pad: CLIP=0 if needed] FINALIZE 0x12345678
 *   sub (16 B):      FINALIZE=0x12345678, JUMP0=1 (channel 0 returns to the continuation)
 *   continuation:    the 16 bytes GPUCMD_Split appends at frame end (FINALIZE x2)
 * JT11: no register write and no readback. The result line only says the jump ran. */
static int jt_on;
static int jt_state; /* 0 idle, 1 list queued (result read at the next frame end), 2 stopped (alloc failed) */
static u32* jt_sub;
static u32 jt_cont_pa, jt_sub_pa, jt_cont_word, jt_size0;
static u64 jt_last; /* tick of the last queued jump, 0 before the first */
static unsigned jt_frame; /* frame of the last queued jump, for the result line */
extern u64 pm_loop_tick;
static void jt_queue(void) {
    u32* buf; u32 size, off, pad;
    if (!jt_sub) jt_sub = (u32*)linearAlloc(16 * 2);
    if (!jt_sub) { pm_log_e("jumptest: linearAlloc failed"); jt_state = 2; return; }
    jt_last = pm_loop_tick;
    jt_frame = pm_frame_count();
    jt_sub[0] = 0x12345678u;                        /* FINALIZE param, same value as the frame list tail */
    jt_sub[1] = 0xF0000u | GPUREG_FINALIZE;         /* header: reg, mask 0xF, single */
    jt_sub[2] = 1;                                  /* JUMP0 param: any value */
    jt_sub[3] = 0xF0000u | GPUREG_CMDBUF_JUMP0;     /* returns to the continuation: without it the list does not come back */
    pm_fl_dc(jt_sub, 16);
    GPUCMD_GetBuffer(&buf, &size, &off);
    pad = ((off + 12u) & 3u) ? 2u : 0u; /* 5 writes = 10 words, pad 0 or 2 words, FINALIZE = 2 words: the list ends on a 16-byte boundary */
    jt_cont_word = off + 12u + pad;
    jt_cont_pa = osConvertVirtToPhys(buf + jt_cont_word) >> 3;
    jt_sub_pa = osConvertVirtToPhys(jt_sub) >> 3;
    jt_size0 = drew ? 4 : 2;                        /* 32 bytes >> 3 with a draw (FLUSH, INVALIDATE, EARLYDEPTH_CLEAR, FINALIZE), else 16 bytes */
    GPUCMD_AddWrite(GPUREG_CMDBUF_SIZE0, jt_size0);
    GPUCMD_AddWrite(GPUREG_CMDBUF_ADDR0, jt_cont_pa);
    GPUCMD_AddWrite(GPUREG_CMDBUF_SIZE1, 2);
    GPUCMD_AddWrite(GPUREG_CMDBUF_ADDR1, jt_sub_pa);
    GPUCMD_AddWrite(GPUREG_CMDBUF_JUMP1, 1);
    if (pad) GPUCMD_AddWrite(GPUREG_FRAGOP_CLIP, 0);
    GPUCMD_AddWrite(GPUREG_FINALIZE, 0x12345678u); /* JT10: the finalization write GPUCMD_Split uses; the list ends on it */
    jt_state = 1;
}
/* JT1: the result. gspWaitForP3D returns when the GPU has finished the list of the frame (libctru has no timeout form), so a hang
 * shows as no line. */
static void jt_result(void) {
    gspWaitForP3D();
    pm_log_evt("jump f%u ran", jt_frame);
    pm_log_d("jump f%u layout: size0=%u addr0=%08x size1=2 addr1=%08x jump1=1 cont_word=%u", jt_frame, (unsigned)jt_size0,
             (unsigned)(jt_cont_pa << 3), (unsigned)(jt_sub_pa << 3), (unsigned)jt_cont_word);
    jt_state = 0;
}

/* CORE2-3a: walker side entry (nd_wq.c): the game thread calls it, the body runs on the walker side. */
static void pm_gpu_init_w(void);
typedef struct { int unused; } Wq_pm_gpu_init;
static void wq_pm_gpu_init(void* a) { Wq_pm_gpu_init* q = (Wq_pm_gpu_init*)a; (void)q; pm_gpu_init_w(); }
void pm_gpu_init(void) { Wq_pm_gpu_init q = { 0 }; pm_wq_call(wq_pm_gpu_init, &q, sizeof q); }
static void pm_gpu_init_w(void) {
    init_f_tables();
    gp2_clip_init();
    if (gs.gpu_ok) return;
    jt_on = pm_debug_has("jumptest");
    pm_wide = pm_wide_cfg = (pm_debug_has("wide") || pm_opt.wide) && !pm_debug_has("nowide"); /* nowide: 4:3 layout with wide set in options.ini */
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
    pm_vram_reg(target->frameBuf.colorBuf, 240 * 400 * 4);
    pm_vram_reg(target->frameBuf.depthBuf, 240 * 400 * 4);
    pm_log_d("vram boot: colour in bank %c, depth in bank %c", pm_vram_bank_of(target->frameBuf.colorBuf) ? 'B' : 'A', pm_vram_bank_of(target->frameBuf.depthBuf) ? 'B' : 'A');
    small_arena_init();
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT,
        GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    cur_target = target;
    targetR = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8); /* STEREO: the right eye. It draws only in a frame with the 3D slider up. */
    if (targetR) {
        pm_vram_reg(targetR->frameBuf.colorBuf, 240 * 400 * 4);
        pm_vram_reg(targetR->frameBuf.depthBuf, 240 * 400 * 4);
        C3D_RenderTargetSetOutput(targetR, GFX_TOP, GFX_RIGHT,
            GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
            GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
            GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
            GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    } else pm_log_w("stereo: right eye target failed, mono only");
    full_reserve(); /* VRAM2: the copy block and the full images, after all targets and before the texture cache takes the rest (pm_tex_init) */
    dvlb_all = DVLB_ParseFile((u32*)pm_shader4_shbin, pm_shader4_shbin_size); /* the pm4 program (raw Vtx, matrix palette); pm5 has its own DVLB below */
    raw_dvlb_ok = dvlb_all && dvlb_all->numDVLE >= 1;
    pm_log("raw1: pm4 DVLB %s (%d entries)", raw_dvlb_ok ? "on" : "FAILED", dvlb_all ? (int)dvlb_all->numDVLE : 0);
    if (!raw_dvlb_ok) { pm_log_e("shader library failed"); return; }
    dvlb = dvlb_all;
    /* y grows up, matching the citro3d immediate example. 320 game pixels sit in the middle. */
    Mtx_OrthoTilt(&projection, 0.f, 400.f, 0.f, 240.f, 0.f, 1.f, true);
    C3D_AttrInfo* ai = C3D_GetAttrInfo();
    AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 4);
    AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2);
    AttrInfo_AddLoader(ai, 2, GPU_FLOAT, 4);
    { /* GV1: the raw Vtx path: a ring of the Vtx records of the frame and a u16 index ring */
        nb.gring = (unsigned char*)linearAlloc((GRING_VTX + GSTAT_VTX) * 16);
        nb.gidx = (unsigned short*)linearAlloc(GRING_IDX * 2);
        nb.rchunk[0].vtx = nb.gring; nb.rchunk[0].idx = nb.gidx; nb.rchunk[0].used = 1;
        gs.gpu_vtx_on = nb.gring && nb.gidx;
        if (gs.gpu_vtx_on) { /* GV1c: the palette program */
            shaderProgramInit(&prog4);
            shaderProgramSetVsh(&prog4, &dvlb_all->DVLE[0]);
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
            gs.gv4_on = rgen && rk && prog4.vertexShader && u4_pal >= 0 && u4_uvso >= 0 && u4_cmul == u4_uvso + 1 && u4_cadd == u4_uvso + 2 && u4_zmix == u4_uvso + 3; /* nd_emit_draw writes uvso, cmul, cadd, zmix as one block */
            if (gs.gv4_on) { /* GV5 */
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
                u5_uvx = shaderInstanceGetUniformLocation(prog5.vertexShader, "uvx"); /* FB-3: the unit 1 uv map of a pm5 draw */
                gv5_on = prog5.vertexShader && u5_pal >= 0 && u5_lamb >= 0 && u5_lcol >= 0 && u5_ldir >= 0 && u5_lk0 >= 0 && u5_lk1 >= 0 && u5_lflag >= 0 && u5_fogu >= 0 && u5_uvso >= 0 && u5_cmul >= 0 && u5_cadd >= 0 && u5_nzo >= 0 && u5_zmix >= 0 && u5_uvx >= 0;
                pm_log("gv5: lighting, texgen and fog program %s (pal %d lcol %d ldir %d)", gv5_on ? "on" : "FAILED", u5_pal, u5_lcol, u5_ldir);
            }
            pm_log("gv1: palette path %s (pal %d uvso %d cmul %d cadd %d zmix %d nzo %d)", gs.gv4_on ? "on" : "FAILED", u4_pal, u4_uvso, u4_cmul, u4_cadd, u4_zmix, u4_nzo);
        }
        pm_log("gpuvtx: raw Vtx path %s (ring %u vtx, %u idx)", gs.gpu_vtx_on ? "on" : "FAILED", (unsigned)GRING_VTX, (unsigned)GRING_IDX);
    }
    ident(game_mvp);
    rebuild_3d();
    pm_tex_init();
    /* consoleGetDefault() stays the uninitialized template. The console printf
     * writes is the one passed to consoleInit. */
    pm_con_lock();
    consoleInit(GFX_BOTTOM, &bottom_con);
    setvbuf(stdout, NULL, _IONBF, 0);
    consoleSetWindow(&bottom_con, 1, LOG_TOP, 40, LOG_ROWS); /* BOTLAYOUT: the log window, rows 9 to 26 */
    pm_con_unlock();
    gs.gpu_ok = gs.gpu_vtx_on;
    pm_log_d("gpu %s m %d %d", gs.gpu_ok ? "ok" : "no vertex buffer",
           (int)(projection.r[0].y * 1000.f), (int)(projection.r[0].w * 1000.f));
}


/* MN1: the game is frozen while the menu is open, so no game frame ends and nothing presents the menu text. This frame has no draw:
 * the target still holds the last picture (no clear is queued for a frame without draws), the display transfer at C3D_FrameEnd puts it
 * on the top screen again, and the swap shows the bottom console text. Called by the frozen pass when a menu draw happened. */
/* CORE2-3a: walker side entry (nd_wq.c): the game thread posts it, the body runs on the walker side. */
static void pm_gpu_menu_present_w(void);
typedef struct { int unused; } Wq_pm_gpu_menu_present;
static void wq_pm_gpu_menu_present(void* a) { Wq_pm_gpu_menu_present* q = (Wq_pm_gpu_menu_present*)a; (void)q; pm_gpu_menu_present_w(); }
void pm_gpu_menu_present(void) { Wq_pm_gpu_menu_present q = { 0 }; pm_wq_post(wq_pm_gpu_menu_present, &q, sizeof q); }
static void pm_gpu_menu_present_w(void) {
    if (!menu_dirty || !gs.gpu_ok || !target) return;
    menu_dirty = 0;
    eye_slider = osGet3DSliderState();
    stereo_frame = targetR && eye_slider > 0.02f;
    gfxSet3D(stereo_frame);
    raw1_reset(); sh_clear(); C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    sh_clear(); C3D_FrameDrawOn(target);
    if (stereo_frame) C3D_FrameDrawOn(targetR);
    rd_sync(); C3D_FrameEnd(GX_CMDLIST_FLUSH);
    pm_gpu_menu_grey(); /* PAUSE GREY: the present drew the colour frame on the top screen; grey the next top buffer and swap it, so every present while paused shows grey */
}

/* GREY test aid: the two top buffers seen as the back buffer (gfxGetFramebuffer returns the back one). The displayed one is the other. */
static u8* top_seen[2];
static void top_note(u8* fb) {
    if (fb == top_seen[0] || fb == top_seen[1]) return;
    top_seen[top_seen[0] ? 1 : 0] = fb;
}
/* GREY: entries still queued in the GX queue (numEntries - curEntry, as pm_gx_room reads it). */
unsigned pm_gx_qpend(void) {
    gxCmdQueue_s* q = (gxCmdQueue_s*)&__C3D_Context;
    return (unsigned)(q->numEntries - q->curEntry);
}
/* CORE2-3a: walker side entry (nd_wq.c): the game thread posts it, the body runs on the walker side. */
static void pm_gpu_menu_dirty_set_w(void);
typedef struct { int unused; } Wq_pm_gpu_menu_dirty_set;
static void wq_pm_gpu_menu_dirty_set(void* a) { Wq_pm_gpu_menu_dirty_set* q = (Wq_pm_gpu_menu_dirty_set*)a; (void)q; pm_gpu_menu_dirty_set_w(); }
void pm_gpu_menu_dirty_set(void) { Wq_pm_gpu_menu_dirty_set q = { 0 }; pm_wq_post(wq_pm_gpu_menu_dirty_set, &q, sizeof q); }
static void pm_gpu_menu_dirty_set_w(void) { menu_dirty = 1; }

/* MN2: the top screen goes grey while the menu is open. One CPU pass over the top back buffer (BGR8, luma (77 r + 150 g + 29 b) >> 8 into all
 * three bytes), then one data cache flush, then the swap shows it. Called at the pause and after each menu present (the transfer is done first). */
void pm_gpu_shot_top(const char* name);
void pm_gpu_menu_grey(void) {
    u16 w, h;
    u8* fb;
    size_t n, i;
    if (!gs.gpu_ok) return;
    gspWaitForPPF(); /* the display transfer of the menu present is done */
    fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &w, &h);
    top_note(fb);
    n = (size_t)w * h * 3;
    for (i = 0; i + 2 < n; i += 3) {
        unsigned y = (77u * fb[i + 2] + 150u * fb[i + 1] + 29u * fb[i]) >> 8;
        fb[i] = (u8)y; fb[i + 1] = (u8)y; fb[i + 2] = (u8)y;
    }
    GSPGPU_FlushDataCache(fb, (u32)n);
    if (pm_debug_has("shotgrey")) { static char gn[24]; static unsigned gk; snprintf(gn, sizeof gn, "top_grey%04u", gk++); pm_gpu_shot_top(gn); }
    gfxScreenSwapBuffers(GFX_TOP, false);
    { u16 w2, h2; top_note(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &w2, &h2)); }
    pm_log_d("menu: grey top %u x %u", (unsigned)w, (unsigned)h);
}

/* GREY: shot of the DISPLAYED top buffer (the one gfxGetFramebuffer does not return), as top_after<k>. */
/* CORE2-3a: walker side entry (nd_wq.c): the game thread calls it, the body runs on the walker side. */
static void pm_gpu_shot_top_displayed_w(const char* name);
typedef struct { const char* name; } Wq_pm_gpu_shot_top_displayed;
static void wq_pm_gpu_shot_top_displayed(void* a) { Wq_pm_gpu_shot_top_displayed* q = (Wq_pm_gpu_shot_top_displayed*)a; (void)q; pm_gpu_shot_top_displayed_w(q->name); }
void pm_gpu_shot_top_displayed(const char* name) { Wq_pm_gpu_shot_top_displayed q = { name }; pm_wq_call(wq_pm_gpu_shot_top_displayed, &q, sizeof q); }
static void pm_gpu_shot_top_displayed_w(const char* name) {
    u16 tw, th;
    u8* back = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &tw, &th);
    u8* fb;
    char tpath[96];
    FILE* sf;
    unsigned x, y;
    top_note(back);
    fb = back == top_seen[0] ? top_seen[1] : top_seen[0];
    if (!fb) { pm_log_d("top_after: displayed buffer unknown"); return; }
    mkdir("sdmc:/3ds/PaperMario/shots", 0777);
    snprintf(tpath, sizeof tpath, "sdmc:/3ds/PaperMario/shots/%s.ppm", name);
    sf = fopen(tpath, "wb");
    if (sf) {
        fprintf(sf, "P6\n%u %u\n255\n", (unsigned)th, (unsigned)tw);
        for (y = 0; y < tw; y++)
            for (x = 0; x < th; x++) { const u8* px = fb + ((size_t)x * tw + (tw - 1 - y)) * 3; fputc(px[2], sf); fputc(px[1], sf); fputc(px[0], sf); }
        fclose(sf);
    }
    pm_log("shot: %s", name);
}

/* MN2 test aid: the top back buffer (what the grey pass wrote) as sdmc:/3ds/PaperMario/shots/<name>.ppm, 400 x 240, rows as the bottom shot. */
void pm_gpu_shot_top(const char* name) {
    u16 tw, th;
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &tw, &th);
    char tpath[96];
    FILE* sf;
    unsigned x, y;
    mkdir("sdmc:/3ds/PaperMario/shots", 0777);
    snprintf(tpath, sizeof tpath, "sdmc:/3ds/PaperMario/shots/%s.ppm", name);
    sf = fopen(tpath, "wb");
    if (sf && fb) {
        fprintf(sf, "P6\n%u %u\n255\n", (unsigned)th, (unsigned)tw);
        for (y = 0; y < tw; y++)
            for (x = 0; x < th; x++) { const u8* px = fb + ((size_t)x * tw + (tw - 1 - y)) * 3; fputc(px[2], sf); fputc(px[1], sf); fputc(px[0], sf); }
    }
    if (sf) fclose(sf);
}

/* MN0: the bottom screen as sdmc:/3ds/PaperMario/shots/<name>.ppm (240x320 RGB565 rotated to P6). Used by shotbottom (pm_main.c) and shotmenu (pm_options_ui.c). */
/* CORE2-3a: walker side entry (nd_wq.c): the game thread calls it, the body runs on the walker side. */
static void pm_gpu_shot_bottom_w(const char* name);
typedef struct { const char* name; } Wq_pm_gpu_shot_bottom;
static void wq_pm_gpu_shot_bottom(void* a) { Wq_pm_gpu_shot_bottom* q = (Wq_pm_gpu_shot_bottom*)a; (void)q; pm_gpu_shot_bottom_w(q->name); }
void pm_gpu_shot_bottom(const char* name) { Wq_pm_gpu_shot_bottom q = { name }; pm_wq_call(wq_pm_gpu_shot_bottom, &q, sizeof q); }
static void pm_gpu_shot_bottom_w(const char* name) {
    u16 bw, bh;
    u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &bw, &bh);
    char bpath[96];
    FILE* sf;
    mkdir("sdmc:/3ds/PaperMario/shots", 0777);
    snprintf(bpath, sizeof bpath, "sdmc:/3ds/PaperMario/shots/%s.ppm", name);
    sf = fopen(bpath, "wb");
    if (sf && fb) {
        fprintf(sf, "P6\n%u %u\n255\n", (unsigned)bh, (unsigned)bw);
        for (unsigned y = 0; y < bw; y++) /* screen row y, screen column x: the buffer is column major, bottom row first */
            for (unsigned x = 0; x < bh; x++) { const u8* px = fb + ((size_t)x * bw + (bw - 1 - y)) * 2; unsigned c = px[0] | (px[1] << 8); fputc(((c >> 11) & 31) * 255 / 31, sf); fputc(((c >> 5) & 63) * 255 / 63, sf); fputc((c & 31) * 255 / 31, sf); }
    }
    if (sf) fclose(sf);
}


/* shots switch: dump the last finished frame (rotated 240x400 buffer, tiled RGBA8) as a PPM. */
int pm_debug_value(const char* key, char* out, unsigned n);
static int want_clear; /* X01: the clear runs at frame end, and only for a frame the game drew */
/* SHOT-1: a burst is a frame window. Armed at game frame F for n: the window is F..F+n-1. The one file is written at the first drawn
 * frame >= F+n-1 (the settle frame), and the log line "shot" follows fclose. Drawn frames before it log "burst" lines (no file). Then the
 * burst is off. (A count of drawn frames ran past the window: battle 182 was an intermediate frame, the file came at 185.) */
static int shot_burst_on;
static unsigned shot_burst_w1;      /* first frame after the window */
static unsigned shot_burst_f = ~0u; /* the game frame of the last burst call (a game frame may begin the GPU more than once) */
/* SHOT-1: set by a window scene (shotmode=, shotwin=, pm_gpu_shot_window). Then only the window's frames are shot: the periodic one-per-30-frames dump (no window) stays off. A single burst (fxtest) is no window. */
static int shot_armed;
/* CORE2-3a: walker side entry (nd_wq.c): the game thread posts it, the body runs on the walker side. */
static void pm_gpu_shot_burst_w(int n);
typedef struct { int n; } Wq_pm_gpu_shot_burst;
static void wq_pm_gpu_shot_burst(void* a) { Wq_pm_gpu_shot_burst* q = (Wq_pm_gpu_shot_burst*)a; (void)q; pm_gpu_shot_burst_w(q->n); }
void pm_gpu_shot_burst(int n) { Wq_pm_gpu_shot_burst q = { n }; pm_wq_post(wq_pm_gpu_shot_burst, &q, sizeof q); }
static void pm_gpu_shot_burst_w(int n) { extern unsigned pm_frame_count(void); shot_burst_on = 1; shot_burst_w1 = pm_frame_count() + (unsigned)n; shot_burst_f = ~0u; }
/* encwarp: one shot per frame for the frames from..from+n-1 (overrides shotwin) */
static int shot_w0 = -2, shot_w1, shot_ws = 1; /* shot_ws: shotwin=<first>:<count>:<step>, one shot every step frames of the window (default 1) */
/* CORE2-3a: walker side entry (nd_wq.c): the game thread posts it, the body runs on the walker side. */
static void pm_gpu_shot_window_w(unsigned from, unsigned n);
typedef struct { unsigned from; unsigned n; } Wq_pm_gpu_shot_window;
static void wq_pm_gpu_shot_window(void* a) { Wq_pm_gpu_shot_window* q = (Wq_pm_gpu_shot_window*)a; (void)q; pm_gpu_shot_window_w(q->from, q->n); }
void pm_gpu_shot_window(unsigned from, unsigned n) { Wq_pm_gpu_shot_window q = { from, n }; pm_wq_post(wq_pm_gpu_shot_window, &q, sizeof q); }
static void pm_gpu_shot_window_w(unsigned from, unsigned n) { shot_w0 = (int)from; shot_w1 = (int)(from + n); shot_armed = 1; }
void pm_route_mapid(int* area, int* map);
extern unsigned pm_uptime_ms(void);
static void dump_shot(void) {
    static unsigned n, frames;
    char path[64];
    char msg[160] = ""; /* SHOT1: the shot log line, logged after fclose */
    FILE* f;
    unsigned char* row;
    const unsigned char* src;
    int x, y;
    if (!pm_debug_has("shots")) return;
    {   /* X01: shotwin=<first frame>:<count> takes one shot per game frame (frame = pm_frame_count) */
        extern unsigned pm_frame_count(void);
        if (shot_w0 == -2) {
            char v[24];
            shot_w0 = -1;
            if (pm_debug_value("shotmode", v, sizeof v)) shot_armed = 1; /* SHOT-1: a shotmode scene has no shot before its burst (the periodic dump is for a run without a window) */
            if (pm_debug_value("shotwin", v, sizeof v) && sscanf(v, "%d:%d:%d", &shot_w0, &shot_w1, &shot_ws) >= 2) { shot_w1 += shot_w0; shot_armed = 1; if (shot_ws < 1) shot_ws = 1; }
            else shot_w0 = -1;
        }
        if (shot_burst_on) { /* shotmode: one file, the settle frame (the last frame of the window). Every other drawn frame logs t, C_CMD and C_FLUSH of the frame before (pm_ph_cnt is per frame). */
            int ma, mm;
            unsigned fc = pm_frame_count();
            /* AZT10b-1: a game frame can begin the GPU more than once. A call in the same game frame does nothing. */
            if (fc == shot_burst_f) return;
            shot_burst_f = fc;
            pm_route_mapid(&ma, &mm);
            if (fc + 1 < shot_burst_w1) {
                pm_log_evt("burst frame %u map %d:%d t=%u ms cmd %u flush %u", fc, ma, mm, pm_uptime_ms(), pm_ph_prev(C_CMD), pm_ph_prev(C_FLUSH));
                return;
            }
            shot_burst_on = 0;
            snprintf(msg, sizeof msg, "shot %u frame %u map %d:%d t=%u ms cmd %u flush %u", n, fc, ma, mm, pm_uptime_ms(), pm_ph_prev(C_CMD), pm_ph_prev(C_FLUSH));
        } else if (shot_w0 >= 0) {
            unsigned fc = pm_frame_count();
            if ((int)fc < shot_w0 || (int)fc >= shot_w1 || ((int)fc - shot_w0) % shot_ws) return;
            snprintf(msg, sizeof msg, "shot %u frame %u", n, fc);
        } else if (shot_armed) {
            return; /* SHOT-1: a burst or window ran: no shot outside it */
        } else if ((++frames % 30) != 0 || n >= 200) return;
    }
    src = (const unsigned char*)((pm_debug_has("shoteye") && targetR ? targetR : target)->frameBuf.colorBuf); /* shoteye: the right eye */
    if (!src) { pm_log_w("shot: no colour buffer (target %p)", (void*)target); return; } /* ROUTE diag */
    mkdir("sdmc:/3ds/PaperMario/shots", 0777); /* the relative mkdir("shots") left no folder (sdmc: prefix as in pm_main.c) */
    { int ma, mm; pm_route_mapid(&ma, &mm); snprintf(path, sizeof path, "sdmc:/3ds/PaperMario/shots/%04u_%d_%d.ppm", n++, ma, mm); }
    f = fopen(path, "wb");
    if (!f) { pm_log_w("shot: fopen %s failed", path); return; } /* ROUTE diag */
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
    if (msg[0]) pm_log_evt("%s", msg); /* SHOT1: after fclose, so the until line (run_azahar.ps1) means the file is complete */
}

/* D03: the last finished frame as N64 RGBA5551 (native u16, alpha bit set), 320x240, for game code that
 * reads the frame buffer. Reads the PICA color buffer (tiled RGBA8, 240 wide x 400 tall, the screen rotated;
 * game x is the long axis shifted by 40, game y runs backwards on the short axis). Costs one pass over 76800
 * pixels of VRAM, so it runs only when the game asks (pause screen, previous-frame draws). */
/* PT56: with wide the 40 screen columns on each side of the game picture (game x -40 to -1 and 320 to 359), saved with every frame read. The pause screen draws them beside the saved frame. Layout: [side][row * 40 + column], row 0 is the top. */
unsigned short pm_wide_side[2][40 * 240];
unsigned short pm_wide_side_pause[2][40 * 240]; /* the copy the pause screen draws (src/background_gfx.c) */
void pm_gpu_read_frame(unsigned short* dst) {
    PM_WALK_OWN(); /* CORE2-3a */
    pm_gpu_begin_wait();
    if (nb.rect_open) flush(); /* DR1 */
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
    pm_gpu_begin_wait();
    if (nb.rect_open) flush(); /* DR1 */
    int i, w, h;
    if (!gs.gpu_ok || off_fail || stride < 1 || stride > PM_OFF_W) return 0;
    if (gs.off_active && gs.offs[gs.off_cur].base == base && gs.offs[gs.off_cur].siz == siz && (gs.off_cur >= OFF_P || gs.offs[gs.off_cur].stride == (unsigned)stride)) return 1;
    flush();
    for (i = 0; i < OFF_ALL; i++) if (gs.offs[i].base == base && gs.offs[i].siz == siz) break;
    if (i < OFF_P && gs.offs[i].stride != (unsigned)stride) {
        /* PT50: the same address with another row width is another image. The N64 draws two windows (item box: 280 x 32 and 136 x 40) one after the other into the same heap address. Here each window gets its own slot, so the second one does not draw over the first. The first slot stays valid for the draws that already read it. */
        gs.offs[i].base = NULL;
        pm_tex_set_offscreen(i, NULL, 0, &gs.offs[i].tex, siz, gs.offs[i].w, gs.offs[i].h);
        i = OFF_ALL;
    }
    w = stride <= 64 ? 64 : PM_OFF_W; /* X02: the darkness stencil is 64 x 64: 16 KB of VRAM, not 512 KB */
    h = stride <= 64 ? 64 : PM_OFF_H;
    if (i < OFF_ALL && i >= OFF_P && gs.offs[i].last != pm_frame_count()) { /* FX2a: a kept 8 bit image, back in this frame with last frame's pixels */
        gs.offs[i].last = pm_frame_count();
        pm_tex_set_offscreen(i, base, (unsigned)stride, &gs.offs[i].tex, siz, gs.offs[i].w, gs.offs[i].h);
    }
    if (i == OFF_ALL) {
        if (stride > 64 && siz != 2) { /* R4: a full slot is RGBA5551, it can hold only a 16 bit image. Other sizes draw on the screen. */
            static int n_r4;
            if (n_r4++ < 20) pm_log_e("R4: full off-screen image with siz %d in frame %u, draws on the screen (full slots are 16 bit)", siz, pm_frame_count());
            return 0;
        }
        if (siz == 1 && stride <= 64) {
            /* FX2a: the flame writes D_800B32E0 from its own last frame, so 8 bit images keep their slot and pixels. A slot not named in this frame is taken over. */
            int k;
            for (k = OFF_P; k < OFF_ALL && gs.offs[k].base; k++) {}
            if (k == OFF_ALL) for (k = OFF_P; k < OFF_ALL && gs.offs[k].last == pm_frame_count(); k++) {}
            if (k == OFF_ALL) { pm_log_w("FX2a: more than 2 8 bit images in one frame, the rest draw on the screen"); return 0; }
            i = k;
        } else if (w == 64 && off_sused >= OFF_S) {
            static int once;
            if (!once++) pm_log_w("V05: more than %d 64 x 64 off-screen images in one frame, the rest draw on the screen", OFF_S);
            return 0;
        } else if (w != 64 && off_used >= OFF_N) {
            /* TEX2 B: the file select flip draws more than 4 boxes into off-screen images in one frame. The 5th drew on the screen
             * (contents at the top left). Take the full size slots again in turn: the GPU runs the draws in list order, so the strips
             * of an earlier box have read the slot already. A later read of the earlier image does not find it. R5: OFF_N is 3, so
             * a fourth full image in one frame reuses a slot. This line is the hard record of that demand (first 20 cases). */
            static int n_over;
            if (n_over++ < 20) pm_log_e("V05 demand: full image %d in frame %u, OFF_N %d: slot %d is used again", off_used + 1, pm_frame_count(), OFF_N, (int)(off_used % OFF_N));
            i = off_used++ % OFF_N;
            gs.offs[i].base = NULL;
            pm_tex_set_offscreen(i, NULL, 0, &gs.offs[i].tex, siz, gs.offs[i].w, gs.offs[i].h);
        } else i = w == 64 ? OFF_N + off_sused++ : off_used++;
        if (!gs.offs[i].rt) {
            if (!(w == 64 ? small_tex_init(&gs.offs[i].tex) : full_tex_init(i))) {
                pm_vram_alloc_fail("offslot", (unsigned)(w * h * 4 / 1024));
                pm_log_e("V05: off-screen image %d: no VRAM, off-screen passes stay on the screen", i);
                off_fail = 1;
                return 0;
            }
            pm_vram_note();
            C3D_TexSetFilter(&gs.offs[i].tex, GPU_LINEAR, GPU_NEAREST);
            C3D_TexSetWrap(&gs.offs[i].tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
            gs.offs[i].rt = C3D_RenderTargetCreateFromTex(&gs.offs[i].tex, GPU_TEXFACE_2D, 0, -1);
            if (!gs.offs[i].rt) {
                pm_log_e("V05: off-screen image %d: render target failed", i);
                off_fail = 1;
                return 0;
            }
            gs.offs[i].w = w;
            gs.offs[i].h = h;
        }
        C3D_RenderTargetClear(gs.offs[i].rt, C3D_CLEAR_COLOR, 0, 0);
        gs.offs[i].base = base;
        gs.offs[i].siz = siz;
        gs.offs[i].stride = (unsigned)stride;
        gs.offs[i].last = pm_frame_count();
        /* F07-7: the wide grab slot starts at screen column 0, game x 0 is column 40: a read at cfb + x finds column 40 + x */
        pm_tex_set_offscreen(i, (pm_wide && siz == 2 && stride == 320 && pm_is_cfb(base)) ? (const u8*)base - 80 : base, (unsigned)stride, &gs.offs[i].tex, siz, gs.offs[i].w, gs.offs[i].h);
    }
    if (i < OFF_N) off_mask_f |= 1u << i; /* VRAM3 trace: which full images a frame uses */
    gs.off_cur = i;
    off_w = gs.offs[i].w;
    off_h = gs.offs[i].h;
    off_i8 = siz == 1;
    Mtx_Ortho(&off_proj, 0.f, (float)off_w, 0.f, (float)off_h, 0.f, 1.f, true);
    gs.off_active = 1;
    rebuild_3d();
    sh_clear(); C3D_FrameDrawOn(gs.offs[i].rt);
    return 1;
}

/* V05: back to the screen. */
int pm_gpu_off_count(void) { return off_used + off_sused; }
void pm_gpu_off_end(void) {
    if (nb.rect_open) flush(); /* DR1 */
    if (!gs.off_active) return;
    flush();
    gs.off_active = 0;
    off_i8 = 0;
    rebuild_3d();
    sh_clear(); C3D_FrameDrawOn(cur_target);
}

/* STEREO: one task of the frame runs once per eye (pm_nusys.c gbi_task). A mono frame (3D slider at 0) has one pass with pm_eye_e = 0. */
int pm_gpu_stereo(void) { pm_gpu_begin_wait(); return stereo_frame; }
int pm_gpu_stereo_flag(void) { return stereo_frame; } /* read only (the prof line): pm_gpu_stereo waits and changes draw state */
void pm_gpu_eye(int right) {
    PM_WALK_OWN(); /* CORE2-3a */
    if (!stereo_frame || !gs.gpu_ok) return;
    flush();
    if (gs.off_active) pm_gpu_off_end();
    if (right && !r_cleared) { r_cleared = 1; if (r_wantclear) C3D_RenderTargetClear(targetR, C3D_CLEAR_ALL, 0, 0); } /* queue order: the clear comes before every part of the frame */
    gs.grab_dirty = 1; grab_dy0 = grab_dy1 = 0; /* K2: the copy of the other eye is not this eye's frame, so the next grab copies again */
    cur_target = right ? targetR : target;
    pm_eye_e = (right ? PM_EYE_E : -PM_EYE_E) * eye_slider;
    sh_clear(); C3D_FrameDrawOn(cur_target);
    pm_gbi_eye_changed();
}

void pm_gpu_set_replace(int on) { if (on != r_replace) flush(); r_replace = on; } /* DR1: a queued rectangle batch keeps the blender it was queued with */
void pm_gpu_set_memblend(int on) { if (on == r_memblend) return; flush(); r_memblend = on; }

/* B2: T2 grab. The PICA cannot sample the target it draws to, and a CPU read mid-frame sees the last frame (the GPU has not run
 * yet). So: draw what is pending, split the command list, copy the strips of the main target (GX_TextureCopy, queued after the
 * split part, so it sees every draw so far), then draw the copy rotated upright into the slot (cfb, siz 2). Later loads of that
 * address find the slot (T1). citro3d stops the GX queue in a frame and runs it at C3D_FrameEnd, so the order is the order of
 * these calls. The end-of-frame clear of the main target is a queue entry too: it has to come before the first split. */
static int grab_impl(const void* cfb, int y0, int y1, int prev_frame);
/* grab_tex is the copy buffer seen as 256 x 512. A failure is for this call only: the next grab tries again. */
static int grab_hdr;
static int grab_tex_init(void) {
    u8* cb = copy_get();
    if (!cb) return 0;
    if (!grab_hdr) {
        if (!tex_hdr(&grab_tex, 256, 512, GPU_RGBA8)) return 0;
        C3D_TexSetFilter(&grab_tex, GPU_NEAREST, GPU_NEAREST);
        C3D_TexSetWrap(&grab_tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
        grab_hdr = 1;
    }
    grab_tex.data = cb;
    return 1;
}
/* HANG1a: the GX queue of citro3d holds 32 entries (base.c maxEntries) and runs only at frame begin or end, so a splash frame (one texture
 * copy per particle) overflows it (svcBreak in gxCmdQueueAdd). Past 24 entries, wait for the queue to empty, as C3Di_WaitAndClearQueue does.
 * gxQueue is the first member of the citro3d context, so its address is the queue. */
void pm_gx_room(void) {
    gxCmdQueue_s* q = (gxCmdQueue_s*)&__C3D_Context;
    if (q->numEntries < 24) return;
    gxCmdQueueRun(q);
    gxCmdQueueWait(q, -1);
    gxCmdQueueStop(q);
    gxCmdQueueClear(q);
}
/* TEXF1: 1 when the GX queue has no entries or the GPU has finished them. Does not wait (timeout 0). Used before a frame begins. */
int pm_gx_done(void) {
    gxCmdQueue_s* q = (gxCmdQueue_s*)&__C3D_Context;
    if (!q->numEntries) return 1;
    return gxCmdQueueWait(q, 0);
}
/* TEXF1: run the queue and wait for the GPU, as pm_gx_room does. Every entry added before the call is done after it. */
void pm_gx_drain(void) {
    gxCmdQueue_s* q = (gxCmdQueue_s*)&__C3D_Context;
    gxCmdQueueRun(q);
    gxCmdQueueWait(q, -1);
    gxCmdQueueStop(q);
    gxCmdQueueClear(q);
}
/* queue the copy of the colour buffer strips (all 400 screen columns with wide, else the 320 game columns) into grab_tex */
static void grab_queue_copy(void) {
    const int mo = pm_wide ? 0 : 40, x1 = pm_wide ? 400 : 320;
    int s0 = mo >> 3, s1 = (mo + x1 - 1) >> 3;
    pm_gx_room();
    copy_guard(); /* TEXF1: grab_tex is copy_blk */
    GX_TextureCopy((u32*)((u8*)cur_target->frameBuf.colorBuf + s0 * 7680), GX_BUFFER_DIM(7680 / 16, 0),
                   (u32*)((u8*)grab_tex.data + s0 * 8192), GX_BUFFER_DIM(7680 / 16, 512 / 16), (u32)(s1 - s0 + 1) * 7680, BIT(3));
}
int pm_gpu_grab(const void* cfb, int y0, int y1) { return grab_impl(cfb, y0, y1, 0); }
/* PT12: role T3. The old frame buffer is the finished frame that is still in the colour buffer at the start of the frame. The clear of the
 * main target is queued at the first split (KICK1 makes one at 55 percent of the draws, the overlay comes after it) and wipes that frame.
 * So a request sets prev_want, and the next frame queues the copy into grab_tex at pm_gpu_begin, before any other entry (prev_pre). The
 * first frame of a run of requests, or a frame with an earlier T2 grab (it overwrites grab_tex), returns 0: the caller reads with the CPU.
 * Returns 2 if the old frame is in the slot of prev_cfb. */
int pm_gpu_grab_prev(const void* prev_cfb) {
    PM_WALK_OWN(); /* CORE2-3a */
    prev_want = 2; /* BTRANS: the request holds for 2 drawn frames */
    /* GAMEOVER: each strip of a blur asks. The answer of the first ask holds for the drawn frame (prev_pre does not come back in a frame), so the
     * later asks do not flush: the strips stay one batch. */
    if (prev_done) return prev_done > 0 ? 2 : 0;
    if (!target || !gs.gpu_ok) return 0;
    const int r = grab_impl(prev_cfb, 0, 240, 1);
    prev_done = r == 2 ? 1 : -1;
    return r;
}
/* BTRANS fix 3: an encounter start (pre-battle init) asks for the copy of the frame before the star wipe, at the next two begins. */
/* CORE2-3a: walker side entry (nd_wq.c): the game thread posts it, the body runs on the walker side. */
static void pm_gpu_want_prev_w(void);
typedef struct { int unused; } Wq_pm_gpu_want_prev;
static void wq_pm_gpu_want_prev(void* a) { Wq_pm_gpu_want_prev* q = (Wq_pm_gpu_want_prev*)a; (void)q; pm_gpu_want_prev_w(); }
void pm_gpu_want_prev(void) { Wq_pm_gpu_want_prev q = { 0 }; pm_wq_post(wq_pm_gpu_want_prev, &q, sizeof q); }
static void pm_gpu_want_prev_w(void) { prev_want = 2; }
/* BTRANS caplog: the capture state read by cfb_capture_prev (pm_main.c, switch caplog). */
void pm_gpu_cap_state(int* o) {
    PM_WALK_OWN(); /* CORE2-3a */
    o[0] = prev_pre; o[1] = grab_n; o[2] = grab_split; o[3] = kick_done; o[4] = want_clear; o[5] = (int)draws_frame; o[6] = off_fail; o[7] = (gs.gpu_ok && target) ? 1 : 0;
}
/* K1: debug switch grablog. One line per grab call: id (grab_n of the frame), source colour buffer, rows, frame, reuse yes or no. */
static void grab_log(const void* cfb, int y0, int y1, int reuse) {
    static int on = -1;
    if (on < 0) on = pm_debug_has("grablog");
    if (on) pm_log_d("grab %d src %p rows %d-%d frame %u reuse %s", grab_n, cfb, y0, y1, pm_frame_count(), reuse ? "yes" : "no");
}
/* FB-1 GRAB: the copy quad as four ring vertices (rect_raw_quad form, identity positions as the off-screen rect path). uv = tc / 256, shorts 256 - row / 2
 * (mo + x is even at both edges): s runs along y (240 at the top, 0 at the bottom), t along x, as the CPU quad. */
static void grab_raw_quad(int x1, int mo) {
    short v[32];
    const short tva = (short)(256 - mo / 2), tvb = (short)(256 - (mo + x1) / 2);
    const int px_[4] = { 0, x1, x1, 0 }, py_[4] = { 0, 0, 240, 240 };
    const short ps[4] = { 240, 240, 0, 0 }, pt[4] = { tva, tvb, tvb, tva };
    for (int i = 0; i < 4; i++) {
        short* q = v + 8 * i;
        unsigned char* cb = (unsigned char*)q;
        q[0] = (short)px_[i]; q[1] = (short)py_[i]; q[2] = 0; q[3] = 0; q[4] = ps[i]; q[5] = pt[i];
        cb[12] = cb[13] = cb[14] = cb[15] = 0xff;
    }
    const int b = pm_gpu_gvtx_load(v, 4);
    unsigned short* ix = nb.gidx + nb.gidx_n;
    ix[0] = (unsigned short)b; ix[1] = (unsigned short)(b + 1); ix[2] = (unsigned short)(b + 2);
    ix[3] = (unsigned short)b; ix[4] = (unsigned short)(b + 2); ix[5] = (unsigned short)(b + 3);
    nb.gidx_n += 6; nb.gni += 6;
}
static int grab_impl(const void* cfb, int y0, int y1, int prev_frame) {
    pm_gpu_begin_wait();
    if (nb.rect_open) flush(); /* DR1 */
    int was_off = gs.off_active, prev = gs.off_cur;
    static int once;
    /* F07-7: with wide the slot holds all 400 screen columns (slot x = screen x). The game reads game x at slot x + 40 (pm_gpu_off_begin shifts the base). */
    const int x0 = 0, x1 = pm_wide ? 400 : 320, mo = pm_wide ? 0 : 40;
    if (!gs.gpu_ok || off_fail || y1 <= y0) return 0;
    if (!(gs.gpu_vtx_on && gs.gv4_on)) return 0;
    if (grab_n && !prev_frame) {
        /* no draw since the last grab, or only rectangles of other rows: same slot */
        if (!gs.grab_dirty && (grab_dy1 <= grab_dy0 || y1 <= grab_dy0 || y0 >= grab_dy1)) { grab_log(cfb, y0, y1, 1); return 1; }
        /* K1: a draw dirtied these rows, so the copy is old. Copy again past GRAB_MAX too (the cap used to return the old copy here). */
        if (grab_n >= GRAB_MAX && !once++) pm_log_w("B2: %d grabs in one frame, each copies the frame again", (int)GRAB_MAX);
    }
    flush();
    if (prev_frame && (!prev_pre || grab_n)) return 0;
    if (!ring_need(4, 6)) return 0; /* FB-1 GRAB: the copy quad is native; a full ring wraps first */
    if (!grab_tex_init()) return 0;
    if (!pm_gpu_off_begin(cfb, 320, 2)) return 0; /* no free slot: the load reads the frame buffer memory as before */
    pm_gpu_off_end(); /* the slot exists (cleared on first use); the copy goes before the quad */
    if (!prev_frame) {
        if (!grab_split++ && want_clear) { /* queue entry order: clear, part 1, copy, part 2 */
            C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0);
            want_clear = 0;
        }
        pm_tex_flush_pend_only();
        sh_clear(); ndunit_scan(); rd_sync(); C3D_FrameSplit(GX_CMDLIST_FLUSH); sc_spl++;
    }
    if (!prev_frame) grab_queue_copy(); /* PT12: the old frame was copied at pm_gpu_begin */
    pm_gpu_off_begin(cfb, 320, 2);
    {
        int sc[4] = { sci_x0, sci_y0, sci_x1, sci_y1 };
        int s_comb = gs.combine, s_2d = nb.rect_2d, s_bi = batch_identity, s_rep = r_replace, s_at = gs.r_atest, s_nc = gs.r_nocol, s_mix = gs.r_mix,
            s_rs = rgb_scale, s_3d = nb.draw_3d;
        sci_x0 = 0; sci_y0 = 0; sci_x1 = x1; sci_y1 = 240;
        gs.combine = 3; nb.rect_2d = 1; batch_identity = 1; r_replace = 1; gs.r_atest = -1; gs.r_nocol = 0; gs.r_mix = 0; rgb_scale = 1; nb.draw_3d = 0;
        grab_bind = 1;
        /* FB-1 GRAB: the copy quad is a native raw batch (rect path at fill depth 0.5, combine 3, unit 0 = grab_tex). */
        static const float uvso_g[4] = { 1.f / 256.f, 1.f / 256.f, 0.f, 0.f };
        rect_raw_open(3, 0.5f, uvso_g);
        grab_raw_quad(x1, mo);
        flush();
        grab_bind = 0;
        sci_x0 = sc[0]; sci_y0 = sc[1]; sci_x1 = sc[2]; sci_y1 = sc[3];
        gs.combine = s_comb; nb.rect_2d = s_2d; batch_identity = s_bi; r_replace = s_rep; gs.r_atest = s_at; gs.r_nocol = s_nc; gs.r_mix = s_mix;
        rgb_scale = s_rs; nb.draw_3d = s_3d;
    }
    if (was_off) pm_gpu_off_begin(gs.offs[prev].base, gs.offs[prev].w == 64 ? 64 : 320, gs.offs[prev].siz);
    else pm_gpu_off_end();
    grab_log(cfb, y0, y1, 0);
    if (!prev_frame) { /* PT12: the old frame in grab_tex does not stand for the current frame */
        gs.grab_dirty = 0;
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
    if (!on || !(off_used + off_sused || gs.offs[OFF_P].base || gs.offs[OFF_P + 1].base) || n >= 12 || (int)pm_frame_count() < from) return;
    /* X02: every slot of the frame, 64 x 64 slots too: off<frame>_<slot>.ppm */
  for (k = 0; k < OFF_ALL && n < 12; k++) {
    if (!gs.offs[k].base) continue;
    src = (const unsigned char*)gs.offs[k].tex.data;
    mkdir("shots", 0777);
    char path[48];
    snprintf(path, sizeof path, "shots/off%03u_%d.ppm", pm_frame_count(), k);
    n++;
    f = fopen(path, "wb");
    if (!f) continue;
    fprintf(f, "P6 %d %d 255\n", gs.offs[k].w, gs.offs[k].h);
    row = (unsigned char*)malloc(gs.offs[k].w * 3);
    for (int y = 0; y < gs.offs[k].h; y++) {
        for (int x = 0; x < gs.offs[k].w; x++) {
            unsigned mx = x & 7u, my = y & 7u;
            unsigned m = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) | ((mx & 4u) << 2) | ((my & 4u) << 3);
            const unsigned char* p = src + ((((unsigned)(y >> 3) * (gs.offs[k].w / 8) + (unsigned)(x >> 3)) * 64u + m) * 4u);
            unsigned a = p[0];
            row[x * 3] = (unsigned char)(p[3] * a / 255); row[x * 3 + 1] = (unsigned char)(p[2] * a / 255); row[x * 3 + 2] = (unsigned char)(p[1] * a / 255);
            if (a == 0) row[x * 3] = row[x * 3 + 1] = row[x * 3 + 2] = 40;
            if (gs.offs[k].siz == 1) row[x * 3] = row[x * 3 + 1] = row[x * 3 + 2] = (unsigned char)p[3]; /* FX2a: an 8 bit image shows its intensity */
        }
        fwrite(row, 1, gs.offs[k].w * 3, f);
    }
    free(row);
    fclose(f);
  }
}

/* LB1: pm_gpu_begin records the frame start. With fps60 the GPU part runs at once, as before. Otherwise the GPU part runs in
 * pm_gpu_begin_wait: at the first gbi task, at the first CPU read of GPU memory, at pm_gpu_drew and at pm_gpu_end. So the game
 * logic and the display list build run before the wait for the GPU tail of the last frame. */
static int begin_pend, begin_clear;
void pm_gpu_begin_wait(void) {
    PM_WALK_OWN(); /* CORE2-3a */
    if (!begin_pend) return;
    begin_pend = 0;
    if ((off_mask_f || off_sused) && off_trace_n++ < 400) pm_log_d("offuse f%u full %d mask %x small %d", pm_frame_count(), off_used, off_mask_f, off_sused); /* VRAM3 trace */
    off_mask_f = 0;
    off_dump();
    for (int k = 0; k < OFF_P; k++) gs.offs[k].base = NULL; /* FX2a: the 8 bit slots keep their base */
    off_used = off_sused = 0;
    prev_done = 0; /* GAMEOVER: the old frame slot went with the slots */
    gs.off_active = 0;
    off_i8 = 0;
    off_fail = 0; /* row 4: a VRAM failure lasts one frame */
    gs.grab_dirty = 1;
    copy_used = 0; /* TEXF1 */
    grab_dy0 = grab_dy1 = grab_n = grab_split = 0;
    pm_tex_clear_offscreen();
    dv_draw = 0;
    draws_frame = 0;
    drew = 0;
    nb.gni = 0; nb.pal_n = 0; post_gen++; ring_frame_begin(); /* PO3b, GV1c; FB-1: chunk 0 with empty fills */
    nb.gring_fl = nb.gidx_fl = 0; kick_done = 0; /* GV0, KICK1 */
    sci_x0 = 0;
    sci_y0 = 0;
    sci_x1 = 320;
    sci_y1 = 240;
    nb.draw_3d = 0;
    if (!gs.gpu_ok) return;
    pm_tex_upload_vram_before_frame();
    FS_MARK(FS_GWAIT);
    PH_PUSH(PH_FBEGIN); /* C3D_FRAME_SYNCDRAW: waits for the previous frame on the GPU */
    /* SLP2: for pm_wd_wake_frames frames after a wake, the frame begin does not wait for the GPU. A long sleep may leave the GSP event
     * unsignalled (not verified); the wait then may never return, and the game thread would not reach aptMainLoop. The flag is set in wake_hook (pm_log.c). */
    unsigned fb_flags = (pm_interp_on > 0 && !pm_wd_wake_frames) ? C3D_FRAME_SYNCDRAW : 0;
    if (pm_wd_wake_frames) pm_wd_wake_frames--;
    raw1_reset(); sh_clear(); C3D_FrameBegin(fb_flags);
    nb.ring_done = nb.ring_fr; /* FB-1: C3D_FrameBegin waits for the GX queue (C3Di_WaitAndClearQueue) with or without SYNCDRAW: every chunk used before this frame is finished */
    if (gb_on) { gb_sum += pm_ticks() - gb_sub; gb_n++; gb_on = 0; pm_pf_late++; } /* S0-6: the sync of the previous frame returned. PROF2-1: this read belongs to the sample (ssvc), it lands in the next frame */ /* PT88: at 30 steps the loop paces itself by time and the frame begin waits only for the GPU (the previous frame, so the buffers are free); with fps60 the begin waits for the vblank as before: two draws a step, one per vblank */
    FS_MARK(FS_REST);
    PH_POP();
    pm_ph_gpu_note();
    dump_shot();
    PH_PUSH(PH_TEX);
    pm_tex_begin_frame();
    PH_POP();
    PH_PUSH(PH_GPU);
    want_clear = begin_clear;
    noise_off[0] = (float)(noise_next() & 1023u) / 1024.f; noise_off[1] = (float)(noise_next() & 1023u) / 1024.f; /* PT07: one new offset per drawn frame */
    prev_pre = 0;
    if (prev_want && grab_tex_init()) { grab_queue_copy(); prev_pre = 1; } /* PT12: first entry of the queue, before the clear */
    if (prev_want) prev_want--;
    eye_slider = osGet3DSliderState(); /* STEREO: the slider decides per frame. At 0 the frame is mono: one pass, the original perspective, no shift. */
    if (PMDL("stereo")) eye_slider = 1.f;
    stereo_frame = targetR && eye_slider > 0.02f;
    gfxSet3D(stereo_frame);
    r_wantclear = begin_clear; r_cleared = 0;
    cur_target = target;
    pm_eye_e = stereo_frame ? -PM_EYE_E * eye_slider : 0.f;
    sh_clear(); C3D_FrameDrawOn(target);
    PH_POP();
}

/* CORE2-3a: walker side entry (nd_wq.c): the game thread posts it, the body runs on the walker side. */
static void pm_gpu_begin_w(int clear);
typedef struct { int clear; } Wq_pm_gpu_begin;
static void wq_pm_gpu_begin(void* a) { Wq_pm_gpu_begin* q = (Wq_pm_gpu_begin*)a; (void)q; pm_gpu_begin_w(q->clear); }
void pm_gpu_begin(int clear) { Wq_pm_gpu_begin q = { clear }; pm_wq_post(wq_pm_gpu_begin, &q, sizeof q); }
static void pm_gpu_begin_w(int clear) {
    ndref_begin();
    begin_clear = clear;
    begin_pend = 1;
    if (pm_interp_on > 0) pm_gpu_begin_wait();
}

/* CORE2-3a: walker side entry (nd_wq.c): the game thread posts it, the body runs on the walker side. */
static void pm_gpu_end_w(void);
static void cen_lines_frame(void);
typedef struct { int unused; } Wq_pm_gpu_end;
static void wq_pm_gpu_end(void* a) { Wq_pm_gpu_end* q = (Wq_pm_gpu_end*)a; (void)q; pm_gpu_end_w(); }
void pm_gpu_end(void) { Wq_pm_gpu_end q = { 0 }; pm_wq_post(wq_pm_gpu_end, &q, sizeof q); }
static void pm_gpu_end_w(void) {
    if (__builtin_expect(pm_cen_lines, 0)) cen_lines_frame(); /* PROF2-3: the census lines, once per frame at the frame end (rect_log every 100 frames there) */
    if (pm_frame_count() % 100u == 0) pm_tex_cell_log(pm_frame_count()); /* MSGBOX 3: glyph cells every 100 frames (debug level) */
    pm_gpu_begin_wait();
    if (!gs.gpu_ok) return;
    FS_MARK(FS_FLUSH);
    flush();
    cen_frame_end();
    if (gs.off_active) pm_gpu_off_end();
    /* X01: a frame the game did not draw (it skips gfx_draw_frame while the pause screen takes the picture) keeps the last
     * picture on screen, as the N64 does when no task swaps the buffer. A clear here would show black for that frame. The clear
     * is a GX memory fill that runs before the command list queued by C3D_FrameEnd, so it can come after the draw calls. */
    if (want_clear && drew) C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0);
    /* PO1: the GPU reads the vertex buffer when C3D_FrameEnd runs the command list. One flush of the
     * whole range written this frame replaces one GSPGPU call per draw. Each frame's writes
     * again, and FrameBegin(SYNCDRAW) has waited for the GPU, so no draw of this frame overlaps the
     * writes. */
    gpu_flush_written(); /* GV0: pool, raw Vtx ring and index ring, what a split has not flushed already */
    {
        float cmd = C3D_GetCmdBufUsage();
        static unsigned warned;
        frames_seen++;
        if (draws_frame > draws_max) draws_max = draws_frame;
        prev_draws = draws_frame; kick_at = draws_frame >= 48 ? draws_frame * 55 / 100 : 0; /* KICK1 */
        if (cmd > cmd_max) cmd_max = cmd;
        pm_ph_cmd_pm = (unsigned)(cmd * 1000.f);
        pm_ph_draws = draws_frame; /* PI4 */
        if (cmd > 0.9f && (warned < 5 || frames_seen % 600 == 0)) {
            warned++;
            pm_log_w("GPU LIMIT NEAR: command buffer %d%%, draws %u", (int)(cmd * 100.f), draws_frame);
        }
    }
    pm_gbi_end_frame();
    /* Without this flag citro3d flushes the entire linear heap. That heap is
     * the 20MB port pool, so every frame was writing it back through the cache. */
    if (jt_on) { /* JT1: debug tool, see jt_queue. One action per frame: read the last jump, or queue the next one once a second has passed */
        if (jt_state == 1) jt_result();
        else if (jt_state == 0 && (!jt_last || pm_loop_tick - jt_last >= SYSCLOCK_ARM11)) jt_queue();
    }
    FS_MARK(FS_SWAP);
    PH_PUSH(PH_FEND);
    ndunit_scan(); rd_sync(); C3D_FrameEnd(GX_CMDLIST_FLUSH);
    if (pm_ph_on) { gb_sub = pm_ticks(); gb_on = 1; } /* S0-6: submit tick of a sampled frame */
    PH_POP();
}

void pm_gpu_rect_sync(void) { if (nb.rect_open) flush(); }
/* CORE2-3a: walker side entry (nd_wq.c): the game thread calls it, the body runs on the walker side. */
static int pm_gpu_drew_w(void);
typedef struct { int ret; } Wq_pm_gpu_drew;
static void wq_pm_gpu_drew(void* a) { Wq_pm_gpu_drew* q = (Wq_pm_gpu_drew*)a; (void)q; q->ret = pm_gpu_drew_w(); }
int pm_gpu_drew(void) { Wq_pm_gpu_drew q = { 0 }; pm_wq_call(wq_pm_gpu_drew, &q, sizeof q); return q.ret; }
static int pm_gpu_drew_w(void) { pm_gpu_begin_wait(); if (nb.rect_open) flush(); return drew; }

void pm_gpu_log_stats(const char* tag) {
    pm_log_d("mem %s gpu max per frame: draws %u, command buffer %d%%", tag, draws_max, (int)(cmd_max * 100.f));
    draws_max = 0;
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
    if (sx == gs.vp_sx && sy == gs.vp_sy && tx == gs.vp_tx && ty == gs.vp_ty && ortho == ui_ortho &&
        memcmp(game_mvp, m, sizeof(game_mvp)) == 0) return;
    if (nb.draw_3d && nb.gni > 0) flush();
    ui_ortho = ortho;
    memcpy(game_mvp, m, sizeof(game_mvp));
    gs.vp_sx = sx;
    gs.vp_sy = sy;
    gs.vp_tx = tx;
    gs.vp_ty = ty;
    rebuild_3d();
}

/* PO3b: copy n raw Vtx records into the frame ring. Returns the ring index of the first one, or -1 (path off or ring full). */
const unsigned char* pm_gpu_gring(void) { return nb.gring; }
unsigned pm_gpu_flush_gen(void) { return flush_gen; }
static int wide_edge_k(const float m[4][4], const unsigned short* idx, unsigned n);
/* GV1b: add one triangle to the open raw batch. The caller knows that the batch has the same draw parameters (it saw no flush since its last gtrip). The
 * vertices must already carry a flag of the palette of this batch. */
int pm_gpu_gappend(unsigned i0, unsigned i1, unsigned i2, unsigned g0, unsigned g1, unsigned g2) {
    if (nb.gni == 0 || nb.gidx_n + 3 > GRING_IDX || !nb.g_pal) return 0;
    if (nb.ring_cur && nb.gi_kind != 0) return 0; /* FB-1: a static batch on chunk k takes no ring triangles */
    const unsigned r3[3] = { i0, i1, i2 }, g3[3] = { g0, g1, g2 };
    if (pm_wide && !gs.off_active && ui_ortho) { /* WIDE1-3: the same edge test as pm_gpu_gtrip, the slow path decides (once per triangle) */
        unsigned short t3[3] = { (unsigned short)i0, (unsigned short)i1, (unsigned short)i2 };
        if (g1 != g0 || g2 != g0 || i0 > 0xffffu || i1 > 0xffffu || i2 > 0xffffu || wide_edge_k(pal_tab[g0], t3, 3) != 0) return 0;
    }
    for (int i = 0; i < 3; i++) {
        unsigned r = r3[i];
        if (rgen[r] == nb.pal_gen) continue;
        unsigned k; for (k = 0; k < nb.pal_n; k++) if (nb.pal_g[k] == g3[i]) break;
        if (k == nb.pal_n) return 0; /* a matrix that is not in the palette yet: the slow path adds it */
        if (rgen[r] != 0 && rk[r] == k) { rgen[r] = nb.pal_gen; continue; }
        if (rgen[r] == 0 && r >= nb.gring_fl) {
            *(unsigned short*)(nb.gring + 16u * r + 6) = (unsigned short)k;
            rgen[r] = nb.pal_gen; rk[r] = (unsigned char)k;
            continue;
        }
        return 0; /* needs a copy: the slow path */
    }
    nb.gidx[nb.gidx_n++] = (unsigned short)i0;
    nb.gidx[nb.gidx_n++] = (unsigned short)i1;
    nb.gidx[nb.gidx_n++] = (unsigned short)i2;
    nb.gni += 3;
    return 1;
}
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
void pm_gpu_gv_reset(void) { post_gen++; }
void pm_gpu_gv_stats(unsigned o[4]) { o[0] = gv_palup_n; o[1] = gv_palmax; o[2] = gv_palflush; o[3] = gv_recopy_n; }
/* GV1c: G_MTX in gv1 mode. The game matrix is in the palette of the triangles, so a matrix change closes no batch. A change of the viewport or of the orthographic
 * flag still does. */
void pm_gpu_note_mvp(const float m[4][4], float sx, float sy, float tx, float ty) {
    int ortho = m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f;
    if (sx == gs.vp_sx && sy == gs.vp_sy && tx == gs.vp_tx && ty == gs.vp_ty && ortho == ui_ortho) return;
    pm_gpu_set_mvp(m, sx, sy, tx, ty);
}

/* the open raw batch has the draw parameters of this triangle */
static int gtrip_params_same(int use5, const PmFx* fx, const float vp[4], const float uvso[4], const float cmul[4], const float cadd[4], const float zmix[2], int ct) {
    if (!nb.g_pal || use5 != nb.g_use5) return 0;
    if (use5 && memcmp(&nb.g_fx, fx, sizeof nb.g_fx)) return 0;
    return !(memcmp(nb.g_uvso, uvso, sizeof(nb.g_uvso)) || memcmp(nb.g_vp, vp, sizeof(nb.g_vp)) || memcmp(nb.g_cmul, cmul, sizeof(nb.g_cmul)) || memcmp(nb.g_cadd, cadd, sizeof(nb.g_cadd)) ||
             memcmp(nb.g_zmix, zmix, sizeof(nb.g_zmix)) || nb.g_ct != ct);
}
#define PAL_NONE 0xffffffffu
/* the palette entry of each game matrix (PAL_NONE: not in the palette); returns the count of distinct matrices that are not in it */
static unsigned pal_lookup(const unsigned g[3], unsigned kf[3]) {
    unsigned miss = 0;
    for (int i = 0; i < 3; i++) {
        unsigned k; for (k = 0; k < nb.pal_n; k++) if (nb.pal_g[k] == g[i]) break;
        kf[i] = k < nb.pal_n ? k : PAL_NONE;
        if (k == nb.pal_n && !(i > 0 && g[i] == g[0]) && !(i > 1 && g[i] == g[1])) miss++;
    }
    return miss;
}

/* GV1c: one triangle of ring vertices ri[3], vertex i with game matrix tab[g[i]]. ro[3] gets the ring indices to use from now on (a vertex whose palette index
 * has to change is copied). Returns 0 when the raw path cannot take it now. */
/* WIDE1-1: with wide on, an orthographic draw is native unless a vertex sits on the left or right game edge (the vertex copy moves those to the screen edge, wide_edge_k). The post matrix is made for the matrix kind of the
 * last note_mvp (ui_ortho), so the draw matrix must be that kind. The vertices are tested like the CPU path: clip x at +-w * 0.999. At most WIDE_MAXV index entries are tested (a bigger draw keeps the CPU path).
 * 0: native, 1: edge vertex or a matrix of the other kind, 2: too many vertices or no index list. */
#define WIDE_MAXV 192u
static struct { unsigned ortho, nat, edge, big, all, copy; } wide_c;
static void wide_log(unsigned f) { /* called from ndn_log every 300 frames; the window counts reset at the print */
    pm_log_w("wide f%u ortho %u native %u edge %u big %u all %u copy %u", f, wide_c.ortho, wide_c.nat, wide_c.edge, wide_c.big, wide_c.all, wide_c.copy);
    memset(&wide_c, 0, sizeof wide_c);
}
/* MSGBOX 4: the draw under test (wide_edge_k) has vertices at or past both game edges (a full-width quad such as a fade). Only then does a vertex past an edge move. */
static int wide_span;
static void wide_clip(const float m[4][4], unsigned v, float* cx, float* cw) {
    const short* p = (const short*)(nb.gring + 16u * v);
    float x = (float)p[0], y = (float)p[1], z = (float)p[2];
    *cx = m[0][0] * x + m[1][0] * y + m[2][0] * z + m[3][0]; *cw = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];
}
/* TRI-2: the screen-edge rule of the vertex copy for object vertex v under the matrix m: cxo gets the clip x; the result is 1 when x sits on the game edge (w > 0, x at +-w * 0.999 to 1.001).
 * MSGBOX 4: a vertex past the edge (|x| > w * 1.001) is a UI part placed in the wide margin (a message bubble clamped to pm_wide_left(18)): it keeps its place (1:1) unless
 * the draw spans both edges (wide_span). Moving it stretched the bubble's end curve into a blank box. */
static int wide_on_edge(const float m[4][4], unsigned v, float* cxo) {
    float cx, cw;
    wide_clip(m, v, &cx, &cw);
    if (cxo) *cxo = cx;
    if (!(cw > 0.f) || (cx < cw * 0.999f && cx > -cw * 0.999f)) return 0;
    return wide_span || (cx <= cw * 1.001f && cx >= -cw * 1.001f);
}
/* TRI-2: a new ring slot with a copy of src whose clip x is 1.25 times the CPU clip x. For the orthographic kind only the object x moves: x' = x + 0.25 * cx / m00 keeps cw.
 * The short x is rounded (at most 0.5 raw unit). The caller sets the flag and rgen of the slot. src stays as it was (the CPU path reads it). Returns the slot or -1. */
static int wide_copy_vtx(const float m[4][4], unsigned src) {
    float cx = 0.f;
    wide_on_edge(m, src, &cx);
    float xn = (float)((const short*)(nb.gring + 16u * src))[0] + 0.25f * cx / m[0][0];
    if (nb.gring_n + 1 > GRING_VTX || !(xn > -32768.f && xn < 32767.f)) return -1;
    unsigned n = nb.gring_n++;
    memcpy(nb.gring + 16u * n, nb.gring + 16u * src, 16);
    *(short*)(nb.gring + 16u * n) = (short)(xn >= 0.f ? xn + 0.5f : xn - 0.5f);
    rgen[n] = 0; rk[n] = 0;
    return (int)n;
}
/* TRI-2: out[i] = idx[i] for a vertex off the edge, else a copy with x moved (one copy per vertex). 0: no room. */
static int wide_copy_list(const float m[4][4], const unsigned short* idx, unsigned n, unsigned short* out) {
    for (unsigned i = 0; i < n; i++) {
        int c = -1;
        if (!wide_on_edge(m, idx[i], NULL)) { out[i] = idx[i]; continue; }
        for (unsigned j = 0; j < i; j++) if (idx[j] == idx[i] && out[j] != idx[j]) { c = (int)out[j]; break; }
        if (c < 0 && (c = wide_copy_vtx(m, idx[i])) < 0) return 0;
        out[i] = (unsigned short)c;
    }
    return 1;
}
static int wide_edge_k(const float m[4][4], const unsigned short* idx, unsigned n) {
    int r = 0;
    const int wz = m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f;
    unsigned ne = 0;
    wide_c.ortho++;
    if (!idx || !nb.gring || !rgen) r = 2;
    else if (!wz || !ui_ortho) r = 1;
    else if (n > WIDE_MAXV) r = 2;
    else {
        int lo = 0, hi = 0; /* MSGBOX 4: wide_span for this draw */
        for (unsigned i = 0; i < n; i++) { float cx, cw; wide_clip(m, idx[i], &cx, &cw); if (cw > 0.f) { lo |= cx <= -cw * 0.999f; hi |= cx >= cw * 0.999f; } }
        wide_span = lo && hi;
        for (unsigned i = 0; i < n; i++) if (wide_on_edge(m, idx[i], NULL)) ne++;
    }
    if (r == 0 && ne) { /* TRI-2: edge vertices are copied with x moved (wide_copy_vtx) when m00 moves them and the ring has room */
        if (m[0][0] == 0.f || nb.gring_n + ne > GRING_VTX) r = 1;
        else r = ne == n ? 3 : 4;
    }
    if (r == 3) wide_c.all++;
    else if (r == 0) wide_c.nat++;
    else if (r == 1) wide_c.edge++;
    else if (r == 4) wide_c.copy++;
    else wide_c.big++;
    { static unsigned seen; if (r == 1 && seen < 8u) { seen++; pm_log("wideedge f%u n %u ui %d wz %d", pm_frame_count(), n, ui_ortho, wz); } } /* WIDE1-2: which draws hit the edge rule */
    return r;
}
/* 0: native (no vertex on the edge, or the edge vertices are copies: wide_copy_list, pm_gpu_gtrip). 1 and 2: the CPU path keeps the draw. 3 (every vertex on the edge) is the wedge of pm_gpu_grange. */
static int wide_edge(const float m[4][4], const unsigned short* idx, unsigned n) { int r = wide_edge_k(m, idx, n); return r == 3 || r == 4 ? 0 : r; }
/* PERF2 step 4: the cold parts of pm_gpu_gtrip. gtrip_wide: F07-7 the CPU path moves edge vertices of UI quads; the wide post matrix is made for a perspective game
 * matrix. -1 refused, else the mask of edge vertices (TRI-2: copied in the loop). gtrip_copy: the slot is in use by another palette index, or already flushed: a copy. */
static __attribute__((cold, noinline)) int gtrip_wide(const float (*tab)[4][4], const unsigned g[3], const unsigned ri[3]) {
    unsigned mvm = 0;
    if (ui_ortho) { /* WIDE1-3: one matrix for the three vertices, no vertex on the game edge (an edge triangle keeps the CPU path) */
        unsigned short t3[3];
        if (g[1] != g[0] || g[2] != g[0] || ri[0] > 0xffffu || ri[1] > 0xffffu || ri[2] > 0xffffu) return -1;
        t3[0] = (unsigned short)ri[0]; t3[1] = (unsigned short)ri[1]; t3[2] = (unsigned short)ri[2];
        const int we = wide_edge_k(tab[g[0]], t3, 3); /* TRI-2: 3 or 4 = edge vertices, copied in the loop below */
        if (we == 1 || we == 2) return -1;
        if (we) for (int i = 0; i < 3; i++) if (wide_on_edge(tab[g[0]], ri[i], NULL)) mvm |= 1u << i;
    } else for (int i = 0; i < 3; i++) { const float(*m)[4] = tab[g[i]]; if (m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f) return -1; }
    return (int)mvm;
}
static __attribute__((noinline)) void gtrip_open(const float vp[4], const float uvso[4], const float cmul[4], const float cadd[4], const float zmix[2], int ct, const PmFx* fx, int use5) {
    memcpy(nb.g_uvso, uvso, sizeof(nb.g_uvso));
    memcpy(nb.g_vp, vp, sizeof(nb.g_vp));
    memcpy(nb.g_cmul, cmul, sizeof(nb.g_cmul));
    memcpy(nb.g_cadd, cadd, sizeof(nb.g_cadd));
    memcpy(nb.g_zmix, zmix, sizeof(nb.g_zmix));
    nb.g_ct = ct; nb.g_pal = 1; nb.pal_n = 0; nb.pal_gen++;
    nb.g_use5 = use5; if (use5) nb.g_fx = *fx;
    if (vp[0] != gs.vp_sx || vp[1] != gs.vp_sy || vp[2] != gs.vp_tx || vp[3] != gs.vp_ty) { gs.vp_sx = vp[0]; gs.vp_sy = vp[1]; gs.vp_tx = vp[2]; gs.vp_ty = vp[3]; rebuild_3d(); }
    if (ct) {
        int ccw = gv_wind <= 0; /* golden yard and toad: the first sign was inverted. */
        if (ct == 2) ccw = !ccw;
        nb.g_cullm = ccw ? GPU_CULL_BACK_CCW : GPU_CULL_FRONT_CCW;
    } else nb.g_cullm = GPU_CULL_NONE;
}
static __attribute__((cold, noinline)) int gtrip_copy(unsigned r, unsigned k, unsigned* ro) {
    if (nb.gring_n + 1 > GRING_VTX) return 0;
    unsigned n = nb.gring_n++;
    memcpy(nb.gring + 16u * n, nb.gring + 16u * r, 16);
    *(unsigned short*)(nb.gring + 16u * n + 6) = (unsigned short)k;
    rgen[n] = nb.pal_gen; rk[n] = (unsigned char)k;
    *ro = n;
    gv_recopy_n++;
    return 1;
}
int pm_gpu_gtrip(const float (*tab)[4][4], const unsigned g[3], const float vp[4], const float uvso[4], const float cmul[4], const float cadd[4], const float zmix[2], int ct,
                 const PmFx* fx, const unsigned ri[3], unsigned ro[3], unsigned* mv) {
    if (!gs.gv4_on || !gs.gpu_ok) return 0;
    gi_kind_set(0);
    const int use5 = fx != NULL;
    if (use5 && !gv5_on) return 0;
    unsigned mvm = 0; /* TRI-2: bit i: ro[i] is a copy with x moved (the caller keeps vert_ring) */
    if (mv) *mv = 0;
    if (gs.r_zmark) return 0; /* B3b depth pass: the second pass needs the CPU vertices */
    if (gs.r_decal && gs.r_blend) return 0; /* V16b: a decal is drawn in two passes from the CPU vertices */
    if (pm_wide && !gs.off_active) { int w_ = gtrip_wide(tab, g, ri); if (w_ < 0) return 0; mvm = (unsigned)w_; } /* PERF2: the wide edge test out of line */
    if (nb.gidx_n + 3 > GRING_IDX) return 0;
    if (!nb.draw_3d) { flush(); nb.draw_3d = 1; }
    unsigned kf[3]; /* one palette search per vertex: the fit test and the index assignment below use it */
    if (nb.gni > 0) {
        const unsigned miss = pal_lookup(g, kf);
        const int fits = nb.pal_n + miss <= (use5 ? GPAL5 : GPAL);
        if (!fits || !gtrip_params_same(use5, fx, vp, uvso, cmul, cadd, zmix, ct)) {
            if (nb.pal_n && !fits) gv_palflush++; /* a palette overflow: the matrices do not fit */
            flush();
        }
    }
    if (nb.gni == 0) kf[0] = kf[1] = kf[2] = PAL_NONE; /* a new batch: an empty palette */
    if (nb.gni == 0) gtrip_open(vp, uvso, cmul, cadd, zmix, ct, fx, use5); /* PERF2: once per batch, out of line */
    pal_tab = tab;
    unsigned k3[3];
    for (int i = 0; i < 3; i++) {
        unsigned k = kf[i];
        if (k == PAL_NONE) {
            if (i > 0 && g[i] == g[0]) k = k3[0];
            else if (i > 1 && g[i] == g[1]) k = k3[1];
            else { k = nb.pal_n; nb.pal_g[nb.pal_n++] = g[i]; if (nb.pal_n > gv_palmax) gv_palmax = nb.pal_n; }
        }
        k3[i] = k;
    }
    for (int i = 0; i < 3; i++) {
        unsigned r = ri[i];
        ro[i] = r;
        if (mvm & (1u << i)) { /* TRI-2: an edge vertex: a copy with x moved (the CPU rule), with this palette index */
            const int n = wide_copy_vtx(tab[g[i]], r);
            if (n < 0) return 0;
            *(unsigned short*)(nb.gring + 16u * (unsigned)n + 6) = (unsigned short)k3[i];
            rgen[n] = nb.pal_gen; rk[n] = (unsigned char)k3[i];
            ro[i] = (unsigned)n;
            continue;
        }
        if (rgen[r] == nb.pal_gen) continue; /* in this palette already, with the right index */
        if (rgen[r] != 0 && rk[r] == k3[i]) { rgen[r] = nb.pal_gen; continue; } /* an older palette gave it the same index */
        if (rgen[r] == 0 && r >= nb.gring_fl) { /* first use, and the GPU has not seen the ring slot */
            *(unsigned short*)(nb.gring + 16u * r + 6) = (unsigned short)k3[i];
            rgen[r] = nb.pal_gen; rk[r] = (unsigned char)k3[i];
            continue;
        }
        if (!gtrip_copy(r, k3[i], &ro[i])) return 0; /* PERF2: the slot copy out of line */
    }
    nb.gidx[nb.gidx_n++] = (unsigned short)ro[0];
    nb.gidx[nb.gidx_n++] = (unsigned short)ro[1];
    nb.gidx[nb.gidx_n++] = (unsigned short)ro[2];
    if (mv) *mv = mvm;
    nb.gni += 3;
    return 1;
}

/* DLC2: static vertex area. A compiled shape group copies its Vtx here once (flag 0 = palette entry 0). The area is rewritten only after pm_gpu_gstat_reset (a new
 * map), which runs after C3D_FrameBegin(SYNCDRAW) has waited for the GPU. Returns the first vertex index (GRING_VTX and up) or -1. */
int pm_gpu_gstat_alloc(unsigned n, unsigned char** out) {
    if (!gs.gpu_vtx_on || !gs.gpu_ok || !gs.gv4_on || gstat_n + n > GSTAT_VTX) return -1;
    unsigned first = GRING_VTX + gstat_n;
    gstat_n += n;
    *out = nb.rchunk[0].vtx + 16u * first;
    return (int)first;
}
void pm_gpu_gstat_commit(unsigned first, unsigned n) { pm_fl_dc(nb.rchunk[0].vtx + 16u * first, n * 16u); }
/* DS1: svc flush first; the GSP call only when the svc fails. */
unsigned pm_fl_gsp_n;
void pm_fl_dc(const void* p, unsigned n) {
    if (R_FAILED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (Handle)(uintptr_t)p, n))) { pm_fl_gsp_n++; GSPGPU_FlushDataCache(p, n); }
}
/* S3-1: the static index area. A compiled object (nd_dlc) keeps its triangle indices here for the whole map: the native draws read them in place. Reset with the vertex area. */
enum { GSTAT_IDX = 131072 };
static unsigned short* gidxs; static unsigned gidxs_n;
unsigned short* pm_gpu_gidxs_alloc(unsigned n) {
    if (!gs.gpu_vtx_on || !gs.gpu_ok) return NULL;
    if (!gidxs) gidxs = (unsigned short*)linearAlloc(GSTAT_IDX * 2);
    if (!gidxs || gidxs_n + n > GSTAT_IDX) return NULL;
    unsigned short* p = gidxs + gidxs_n;
    gidxs_n += n;
    return p;
}
void pm_gpu_gidxs_commit(const unsigned short* p, unsigned n) { pm_fl_dc(p, n * 2u); }
unsigned pm_gpu_gidxs_used(void) { return gidxs_n; }
void pm_gpu_gstat_reset(void) { gidxs_n = 0; if (gstat_n) { flush(); gstat_n = 0; } }
unsigned pm_gpu_gstat_used(void) { return gstat_n; }

/* DLC2: nidx indices of static vertices (all flag 0) drawn with the game matrix tab[gm]. The batch palette must hold gm at entry 0, else a new batch starts.
 * Returns 0 when the raw path cannot take it (nothing changed, the caller draws the group through the ring). */
static int grange_batch_open(unsigned gm, const float vp[4], const float uvso[4], const float cmul[4], const float cadd[4], const float zmix[2], int ct, const PmFx* fx, int use5) {
    if (nb.gni == 0 || !nb.g_pal || nb.pal_n == 0 || nb.pal_g[0] != gm || nb.g_ct != ct || use5 != nb.g_use5) return 0;
    if (use5 && memcmp(&nb.g_fx, fx, sizeof nb.g_fx)) return 0;
    if (memcmp(nb.g_uvso, uvso, sizeof(nb.g_uvso)) || memcmp(nb.g_vp, vp, sizeof(nb.g_vp)) || memcmp(nb.g_cmul, cmul, sizeof(nb.g_cmul)) || memcmp(nb.g_cadd, cadd, sizeof(nb.g_cadd)) ||
        memcmp(nb.g_zmix, zmix, sizeof(nb.g_zmix)))
        return 0;
    return 1;
}

unsigned char pm_ent_decal_ok; /* S6-2c-3: set by the entity segment draw (the former pm_gbi.c ent_seg_native) around dlc_seg */
/* PERF2 step 4: the wide screen edge test of pm_gpu_grange (cold, out of line). 0 refused, 1 drawn alone with the edge scale (WIDE1-2), 2 go on with *idx
 * (TRI-2: a copy with the edge vertices moved, in a static buffer: the wedge recursion skips this test). */
static __attribute__((cold, noinline)) int grange_wide(const float (*tab)[4][4], unsigned gm, const float vp[4], const float uvso[4], const float cmul[4], const float cadd[4], const float zmix[2], int ct,
                  const PmFx* fx, const unsigned short** idx, unsigned nidx) {
    static unsigned short wide_idx[WIDE_MAXV]; /* TRI-2: the range with its edge vertices copied */
    const float(*m)[4] = tab[gm];
    if (ui_ortho || (m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f)) {
        int wr = wide_edge_k(m, *idx, nidx); /* WIDE1-1 */
        if (wr == 3) { /* WIDE1-2: every vertex on the game edge: draw it alone with x scale 1.25 in the post matrix, as the CPU path moves each vertex */
            int r2;
            if (!nb.draw_3d) { flush(); nb.draw_3d = 1; }
            flush();
            wedge_x = 1; rebuild_3d();
            r2 = pm_gpu_grange(tab, gm, vp, uvso, cmul, cadd, zmix, ct, fx, *idx, nidx);
            flush();
            wedge_x = 0; rebuild_3d();
            return r2 ? 1 : 0;
        }
        if (wr == 4) { if (!wide_copy_list(m, *idx, nidx, wide_idx)) return 0; *idx = wide_idx; } /* TRI-2 */
        else if (wr) return 0;
    }
    return 2;
}
int pm_gpu_grange(const float (*tab)[4][4], unsigned gm, const float vp[4], const float uvso[4], const float cmul[4], const float cadd[4], const float zmix[2], int ct,
                  const PmFx* fx, const unsigned short* idx, unsigned nidx) {
    if (!gs.gv4_on || !gs.gpu_ok) return 0;
    const int use5 = fx != NULL;
    if (use5 && !gv5_on) return 0;
    if (gs.r_zmark) return 0;
    if (gs.r_decal && gs.r_blend && !pm_ent_decal_ok) return 0; /* S6: the native raw close draws a decal with blend in two passes (nd_raw_native); the entity segment path allows it */
    if (pm_wide && !gs.off_active && !wedge_x) { int wr = grange_wide(tab, gm, vp, uvso, cmul, cadd, zmix, ct, fx, &idx, nidx); if (wr < 2) return wr; } /* PERF2: the wide edge path out of line */
    if (nb.gidx_n + nidx > GRING_IDX) return 0;
    if (!nb.draw_3d) { flush(); nb.draw_3d = 1; }
    gi_kind_set(1); /* FB-1: static indices name chunk 0 vertices */
    /* C3: extend an open static batch across consecutive compiled lists (same PSO inputs and matrix slot). */
    if (grange_batch_open(gm, vp, uvso, cmul, cadd, zmix, ct, fx, use5)) {
        pal_tab = tab;
        memcpy(nb.gidx + nb.gidx_n, idx, nidx * 2u);
        nb.gidx_n += nidx;
        nb.gni += nidx;
        return 2; /* extended an open static batch (C3 cross-list merge) */
    }
    if (nb.gni > 0 && (!nb.g_pal || nb.pal_n == 0 || nb.pal_g[0] != gm || use5 != nb.g_use5 || (use5 && memcmp(&nb.g_fx, fx, sizeof nb.g_fx)) || memcmp(nb.g_uvso, uvso, sizeof(nb.g_uvso)) || memcmp(nb.g_vp, vp, sizeof(nb.g_vp)) || memcmp(nb.g_cmul, cmul, sizeof(nb.g_cmul)) ||
                    memcmp(nb.g_cadd, cadd, sizeof(nb.g_cadd)) || memcmp(nb.g_zmix, zmix, sizeof(nb.g_zmix)) || nb.g_ct != ct)) flush();
    if (nb.gni == 0) {
        memcpy(nb.g_uvso, uvso, sizeof(nb.g_uvso));
        memcpy(nb.g_vp, vp, sizeof(nb.g_vp));
        memcpy(nb.g_cmul, cmul, sizeof(nb.g_cmul));
        memcpy(nb.g_cadd, cadd, sizeof(nb.g_cadd));
        memcpy(nb.g_zmix, zmix, sizeof(nb.g_zmix));
        nb.g_ct = ct; nb.g_pal = 1; nb.pal_n = 1; nb.pal_g[0] = gm; nb.pal_gen++;
        nb.g_use5 = use5; if (use5) nb.g_fx = *fx;
        if (gv_palmax < 1) gv_palmax = 1;
        if (vp[0] != gs.vp_sx || vp[1] != gs.vp_sy || vp[2] != gs.vp_tx || vp[3] != gs.vp_ty) { gs.vp_sx = vp[0]; gs.vp_sy = vp[1]; gs.vp_tx = vp[2]; gs.vp_ty = vp[3]; rebuild_3d(); }
        if (ct) {
            int ccw = gv_wind <= 0;
            if (ct == 2) ccw = !ccw;
            nb.g_cullm = ccw ? GPU_CULL_BACK_CCW : GPU_CULL_FRONT_CCW;
        } else nb.g_cullm = GPU_CULL_NONE;
    }
    pal_tab = tab;
    memcpy(nb.gidx + nb.gidx_n, idx, nidx * 2u);
    nb.gidx_n += nidx;
    nb.gni += nidx;
    return 1;
}
int pm_gpu_gidx_room(unsigned n) { return gs.gv4_on && gs.gpu_ok && nb.gidx_n + n <= GRING_IDX; }
/* S3N-3c: the refusals of pm_gpu_grange that come before any state change, for a static batch with matrix m (the same tests, no side effect). 1: grange takes the batch. */
int pm_gpu_grange_why(const float m[4][4], unsigned nidx, const unsigned short* idx) { /* S4-1: the class of pm_gpu_grange_would's refusal (0 none) */
    if (!gs.gv4_on || !gs.gpu_ok) return 1;
    if (gs.r_zmark) return 2;
    if (gs.r_decal && gs.r_blend && !pm_ent_decal_ok) return 3;
    if (pm_wide && !gs.off_active && (ui_ortho || (m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f)) && wide_edge(m, idx, nidx)) return 4;
    return nb.gidx_n + nidx <= GRING_IDX ? 0 : 5;
}
/* S5-1 (b): the native range check. The walker's decal with blend (decal2) is a native two-pass draw (nd_model_draw), so that refusal is not kept here. */
int pm_gpu_grange_native_would(const float m[4][4], unsigned nidx, const unsigned short* idx) {
    if (!gs.gv4_on || !gs.gpu_ok) return 0;
    if (pm_wide && !gs.off_active && (ui_ortho || (m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f)) && wide_edge(m, idx, nidx)) return 0;
    return nb.gidx_n + nidx <= GRING_IDX;
}
int pm_gpu_grange_would(const float m[4][4], unsigned nidx, const unsigned short* idx) {
    if (!gs.gv4_on || !gs.gpu_ok) return 0;
    if (gs.r_zmark) return 0;
    if (gs.r_decal && gs.r_blend) return 0;
    if (pm_wide && !gs.off_active && (ui_ortho || (m[0][3] == 0.f && m[1][3] == 0.f && m[2][3] == 0.f)) && wide_edge(m, idx, nidx)) return 0;
    return nb.gidx_n + nidx <= GRING_IDX;
}

/* DLC1: 1 when the ring has room for n more vertices (and the path is on) */
int pm_gpu_gv_room(unsigned n) { return gs.gpu_vtx_on && gs.gpu_ok && nb.gring_n + n <= GRING_VTX; }
int pm_gpu_gvtx_load(const void* vtx, unsigned n) {
    if (!gs.gpu_vtx_on || !gs.gpu_ok || nb.gring_n + n > GRING_VTX) return -1;
    memcpy(nb.gring + nb.gring_n * 16, vtx, n * 16);
    unsigned b = nb.gring_n;
    if (rgen) memset(rgen + b, 0, n * sizeof(unsigned));
    nb.gring_n += n;
    return (int)b;
}

/* R2B: 1 when pm_gpu_set_mode with these bits would return at once */
int pm_gpu_mode_is(int zbuffer, int lighting) { return zbuffer == gs.z_on && lighting == light_on; }
void pm_gpu_set_mode(int zbuffer, int lighting) {
    if (zbuffer == gs.z_on && lighting == light_on) return;
    flush();
    gs.z_on = zbuffer;
    light_on = lighting;
}

void pm_gpu_set_nocolor(int on) {
    if (on == gs.r_nocol) return;
    flush();
    gs.r_nocol = on;
}

/* B3b: on = the next triangles are a case 6 model (a depth test against the world, a constant depth write). zn = clip z / w of the constant. */
void pm_gpu_set_zmark(int on, float zn) {
    if (on == gs.r_zmark && (!on || zn == gs.r_zmark_zn)) return;
    flush();
    gs.r_zmark = on;
    gs.r_zmark_zn = zn;
}

void pm_gpu_set_render(int zcmp, int zupd, int blend, int atest, int decal) {
    if (zcmp == gs.r_zcmp && zupd == gs.r_zupd && blend == gs.r_blend && atest == gs.r_atest && decal == gs.r_decal) return;
    flush();
    gs.r_zmark = 0; /* B3b: draw_tri turns it on again for a case 6 model */
    gs.r_zcmp = zcmp;
    gs.r_zupd = zupd;
    gs.r_blend = blend;
    gs.r_atest = atest;
    gs.r_decal = decal;
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

/* S3N-3b: switch ndmcheck (debug). The record arm (the former pm_gbi.c rec_run) passes the record words, the mode and combine entries, the texture handle and the geometry list to
 * pm_gpu_ndm_begin. The native model draws (nd_model_draw, emit 0) are built there, before the walker draws the record. Each walker draw (nd_raw_native) is compared with
 * the native draw at its place: tmpl key, TEV colours, buffer colour, unit 0, uvso, cmul, cadd, zmix, palette 0, and the index words (the walker copies them into its batch).
 * A walker draw may span several native draws (consumed in order). nwalk: a walker draw with no native draw left. nraw: a native draw the walker did not draw. Both are diffs. */
static unsigned sbkw_rec, sbkw_dsw, sbkw_dsh, sbkw_ctw, sbkw_cth; static int sbkw_lines; /* SBKW-2 diagnostic: the native desc size of the last model_draw (ndm_cmp logs the dim diffs) */
static unsigned sbkw_bits(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static int ndm_on_v = -1, ndm_rec_ok, ndm_rec_cmp, ndm_rec_diff, ndm_rec_nost;
static int ndm_mode_ok;
static int ndm_nat_n, ndm_j;
static const uint32_t* ndm_rec;
static unsigned ndm_recs, ndm_eq, ndm_diff, ndm_fb, ndm_nodraw, ndm_nost, ndm_lines, ndm_f_last;
static unsigned ndm_why[16], ndm_badn; static uint32_t ndm_badop; /* S3N-3b: fallback draws by reason code (NdMState.why, nd_nat_why) */
int pm_gpu_ndm_on(void) { if (ndm_on_v < 0) ndm_on_v = pm_debug_has("ndmcheck"); return ndm_on_v; }
void pm_gbi_sticky(unsigned* tss, unsigned* geo, unsigned* cw0, unsigned* cw1); /* S3N-3d */
/* S3N-4: one model record as native draws, after rec_apply (the walker state of the record is set). The context comes from the record and the
 * walker state (the list and record values, the texture unit of the record's descriptor). emit 0: build only (ndmcheck compare). emit 1: the draws go
 * out (the switch). Returns the draw count, -1 fallback (nd_nat_why), -2 nost. */
static NdMode mdl_mode; static NdUnit mdl_un;
/* S3N-6: the cache key of a model record: the 14 record words and the walker state the native draw reads, before the record is applied (rec_run calls
 * this first). The walker-sticky values come from the same globals model_draw reads (the pre-apply values; the post-apply values are a function of them). */
extern unsigned pm_dlc_gen;
unsigned pm_gbi_gvkey(uint32_t* o, unsigned max);
/* S3L-3: hash of the R13 words (the Gfx pairs after the record, count in R13 w0): the cache key holds it, the decode of the words is not in the key */
static uint32_t aux_hash(const uint32_t* rec) {
    unsigned n = rec[26]; uint32_t h = 2166136261u ^ n;
    if (!n) return 0;
    for (unsigned k = 0; k < 2u * n && k < 16u; k++) h = (h ^ rec[NDMC_RW + k]) * 16777619u;
    return h;
}
/* MAPX-2: the other mode bits the record's render mode list writes (per render mode index, compiled once; the lists are static). The draw merges the
 * walker's other mode as (walker & ~written) | list (nd_model_state, B8), so a written bit of the walker never reaches it: the key leaves those bits out.
 * Before, the walker other mode in the key depended on whether the record before was a cache hit (a hit skips rec_apply, so the walker keeps the older
 * mode): every new or changed record in a door or a room change missed again the records after it, and the misses filled the cache to a clear. */
static uint32_t rm_wh[256], rm_wl[256]; static uint8_t rm_done[256];
static __attribute__((cold, noinline)) void rm_masks(unsigned i) {
    extern void* ModelRenderModes[]; /* src/model.c: Gfx* per render mode index */
    NdMode m;
    rm_done[i] = 1;
    if (nd_mode_compile(&m, (const unsigned*)ModelRenderModes[i])) { rm_wh[i] = m.wh; rm_wl[i] = m.wl; }
}
void pm_gpu_model_key(const uint32_t* rec, NdMKey* k) {
    memcpy(k->w, rec, NDMC_RW * sizeof(uint32_t));
    k->w[3] = 0; /* MAPX-2: R1 w1 (pmSlot, static matrix flag, pmMtxVer) is matrix bookkeeping: a hit runs the matrix of this frame (rec_matrix, the palette); a lit entry keeps its version (NdMEnt mver) */
    const unsigned rmi = (rec[0] >> 16) & 0xffu;
    if (!rm_done[rmi]) rm_masks(rmi);
    const uint32_t kh = ND_OMH_READ & ~rm_wh[rmi], kl = ~rm_wl[rmi]; /* H: only the cycle type is read (nd.h); L: nd_mix1 reads bits 18 to 31 too */
    /* MERLEE: a shroud record (R3 bit 32) changes its fog colour alpha (ShroudTintAmt) and may change its blend colour every frame of a fade. Only the mix colour
     * reads them, and the hit sets that colour from the record (pm_gpu_model_hit), so they are not in the key: a fade hits instead of rebuilding every model each frame. */
    if (rec[6] & 32u) { k->w[8] &= 0xffffff00u; k->w[14] = 0; }
    uint32_t* s = k->w + NDMC_RW;
    unsigned sw_tss, sw_geo, sw_cw0, sw_cw1; pm_gbi_sticky(&sw_tss, &sw_geo, &sw_cw0, &sw_cw1);
    float db = decal_bias(); uint32_t dbits; memcpy(&dbits, &db, 4);
    unsigned i = 0;
    uint32_t gk = 0xFFFFFFFFu; /* MAPX-2: the walker geometry reaches the draw only through the bits the list's groups keep (gg = (entry & am) | om) */
    if (rec[24] && pm_dlc_geom_keep((const void*)(uintptr_t)rec[24], &gk) != 0) gk = 0xFFFFFFFFu;
    s[i++] = nd_ws.omode_h & kh; s[i++] = nd_ws.omode_l & kl; s[i++] = nd_ws.om_kh & kh; s[i++] = nd_ws.om_kl & kl; s[i++] = nd_ws.geom & gk;
    s[i++] = sw_tss; s[i++] = sw_cw0; s[i++] = sw_cw1;
    s[i++] = nd_ws.prim; s[i++] = nd_ws.prim_lod; s[i++] = aux_hash(rec); /* S3N-7: env_col is not in the key (it changes per draw order; the cached draws do not read it: ndmcheck diff 0). S3L-3: slot 10 = hash of the R13 words (compared) */ s[i++] = cconst; s[i++] = gs.r_mix == 2 ? r_mixcol : 0u; s[i++] = dbits; /* MERLEE: the walker's mix reaches the native draw only as the F08 fog mix (model_draw) */
    s[i++] = (uint32_t)dview; s[i++] = gs.r_mix == 2 ? 2u : 0u; s[i++] = (uint32_t)r_memblend; s[i++] = (uint32_t)r_replace; s[i++] = (uint32_t)gv_wind;
    { float zb_ = gs.r_zmark ? gs.r_zmark_zn : 0.f; uint32_t zu; memcpy(&zu, &zb_, 4); s[i++] = gs.r_zmark ? (zu | 1u) : 0u; } /* S5-2: zmark on and its depth (bit 0 set when on) */
    s[i++] = (uint32_t)gs.r_decal; s[i++] = (uint32_t)gs.r_blend; s[i++] = (uint32_t)pm_wide; s[i++] = (uint32_t)gs.off_active; s[i++] = (uint32_t)ui_ortho;
    s[i++] = nd_handle_gen(); s[i++] = pm_dlc_gen;
    i += pm_gbi_gvkey(s + i, NDMC_SW - i);
    while (i < NDMC_SW) s[i++] = 0;
    k->rp = (uint32_t)(uintptr_t)rec; /* S3N-7: the record address (PERF1-4a: the lookup memo, nd_mc_find_rec) */
    k->h = 0; /* set by nd_mc_find_rec */
}
static int ndc_compare(const NdMKey* k, const NdTexDesc* ds, int id, int hs, unsigned hg); /* S3N-6 check: 1 when the entry is valid */
unsigned nd_r13_nohit; /* MDL-1: R13 texture records without a desc, stored no-hit (mdlw r13nohit) */
static int r13_last;
static NdUnit r13u1; static int r13u1_ok, r13i8_1; int pm_gbi_bind_tile1(void);
/* MDL-1b (oracle decision): pm_gbi_bind_tile1 runs once per R13 record after rec_aux, before the model draw; unit 1 from the live state. */
static int r13t1b_ok;
/* MDL-1b v2: called by rec_run after rec_apply with the result of pm_gbi_bind_tile1 (0 for every other record): unit 1 from the live state */
void pm_gpu_set_r13_t1(int bound) {
    r13t1b_ok = bound; r13u1_ok = 0; r13i8_1 = 0;
    if (!bound) return;
    memset(&r13u1, 0, sizeof r13u1);
    if (pm_tex_native_unit1(&r13u1) == 1) { r13u1_ok = 1; r13i8_1 = pm_tex_i8(1); }
    /* FB-2: 2 (an off-screen aux image) stays a fallback here: the model path has no snapshot */
}
int nd_mix1(unsigned ol, unsigned* col); /* nd_set.c: the V06 colour mix rule of sync_render */
extern unsigned fogc, blendc;
static int model_draw(const uint32_t* rec, const uint32_t* modew, const uint32_t* combw, const void* gfx, unsigned texidx, const void* geo, int emit, int* mode_ok, const NdMKey* mk) {
    *mode_ok = modew != NULL && nd_mode_compile(&mdl_mode, (const unsigned*)modew) != 0;
    if (!*mode_ok) { static unsigned mb_; if (mb_++ < 8u) { const uint32_t* mw_ = (const uint32_t*)modew; pm_log_prof_line("modebad modew %p bad %02x w0 %08x w1 %08x w2 %08x w3 %08x", modew, mdl_mode.bad, modew ? mw_[0] : 0u, modew ? mw_[1] : 0u, modew ? mw_[2] : 0u, modew ? mw_[3] : 0u); } }
    int tex = 0, i8 = 0, hs = -1, id = -1; unsigned hg = 0; /* S3N-6: the unit slot hint of the entry */
    const NdTexDesc* ds = nd_tex_handle_desc((int)texidx, gfx);
    if (gfx != NULL && ds == NULL) { nd_nat_why = nd_tex_handle_miss((int)texidx, gfx); return -1; } /* S3N-3d: a textured handle without a desc (B4 common case only): fallback */
    /* SBKW-2: the walker binds the tile 0 window (LoadTile w x h from the handle raster, ensure_tile), not the whole handle. The handle desc is
     * the full texture (mainW x mainH, 32 x 64 on the sbk_56 water): the native draw sampled 64 rows and took uv scale 1/64 (uvso1 2x, u0.dim 0x40).
     * The desc takes the window size of the live tile 0 when that window starts at the handle raster with the same form and stride. */
    NdTexDesc wdesc;
    if (ds && nd_ws.td[0].valid && nd_ws.td[0].src == ds->img && nd_ws.td[0].fmt == ds->fmt && nd_ws.td[0].siz == ds->siz && nd_ws.td[0].stride == ds->stride &&
        nd_ws.td[0].w > 0 && nd_ws.td[0].h > 0 && nd_ws.td[0].w <= ds->w && nd_ws.td[0].h <= ds->h && (nd_ws.td[0].w != ds->w || nd_ws.td[0].h != ds->h)) {
        wdesc = *ds; wdesc.w = nd_ws.td[0].w; wdesc.h = nd_ws.td[0].h;
        ds = &wdesc;
    }
    NdMode ax; int ax_ok = 0; /* S3L-3: R13 words of an untextured record (no bind): decoded as the list ops that run before the render mode list */
    if (rec[26] != 0 && gfx == NULL) ax_ok = nd_aux_state(rec + NDMC_RW, rec[26], &ax);
    /* MDL-1: an R13 texture block (no handle, no mode list) is the tile 0 descriptor of the walker's bind */
    /* MDL-1 (oracle decision): an R13 texture block is aux_ok; the walker ran it (rec_aux) so the bound tile state is current. Without a handle
     * desc, unit 0 is the bound slot (pm_tex_native_unit0), the raw path's rule. */
    uint32_t r13cw[4] = { 0, 0, 0, 0 }; int r13cb = 0; int r13 = 0;
    r13_last = 0; /* MDL-5a: r13u1_ok and r13i8_1 are set by pm_gpu_set_r13_t1 (rec_run, before this draw); clearing them here made every bound R13 tile 1 a why 6 sub b */
    if (rec[26] != 0 && !ax_ok && !PMDL("r13off") && nd_r13_texonly(rec + NDMC_RW, rec[26], r13cw, &r13cb)) {
        r13_last = 1;
        r13 = 1;
        if (ds == NULL && gfx == NULL) {
            NdUnit wu; memset(&wu, 0, sizeof wu);
            /* no bound slot: the draw would have no texture (the walker's combine reads one): the walker draws it (counted, why 2) */
            if (pm_tex_native_unit0(&wu) > 0) { mdl_un = wu; tex = 1; i8 = pm_tex_i8(0); }
            else { nd_nat_why = 2; return -1; }
        }
    }
    if (ds) { /* S3N-3c: the unit 0 words from the descriptor (nd_tex_get, nd_tex_unit), not from the walker-bound slot */
        id = nd_tex_get(ds);
        if (id < 0) { nd_nat_why = 13; return -1; } /* S3L-2: nd_tex_get failed (load or table full) */
        extern int pm_tex_tagi; pm_tex_tagi = 1; /* STALE2 texsrc path tag: model */
        if (!nd_tex_unit_hint(id, &hs, &hg, &mdl_un)) { nd_nat_why = 14; return -1; } /* S3L-2: the slot of the desc is not found and not reloaded */
        tex = 1;
        i8 = ds->fmt == 4 && ds->siz == 1; /* G_IM_FMT_I, G_IM_SIZ_8b: the unit 0 raster is I8 */
    }
    NdMCtx c;
    memset(&c, 0, sizeof c);
    c.mode = *mode_ok ? &mdl_mode : NULL;
    c.u0 = tex ? &mdl_un : NULL;
    c.i8_0 = i8;
    unsigned sw_tss, sw_geo, sw_cw0, sw_cw1; pm_gbi_sticky(&sw_tss, &sw_geo, &sw_cw0, &sw_cw1);
    if (ax_ok) { int tw = 1, th = 1; pm_tex_size(&tw, &th); c.tex_w = tw; c.tex_h = th; } /* S3L-3: no bind: the uv params of the walker use the size of its last bound texture (gv_params_calc); the draw reads no texel */
    c.has_comb = 1; c.aux_ok = ax_ok || r13;
    if (r13t1b_ok && !r13u1_ok) { nd_nat_why = 6; nd_nat_sub = 2; return -1; } /* tile 1 bound with no unit 1: named, why 6 */
    c.t1_valid = r13t1b_ok; c.u1 = r13u1_ok ? &r13u1 : NULL; c.i8_1 = r13i8_1; c.t1only = (int)plan_k[3]; /* MDL-1b: tile 1 from pm_gbi_bind_tile1 */
    memcpy(c.uvx, t1_s, 16); c.loc_uvx = (unsigned)u4_uvx;
    c.comb_w0 = combw ? combw[0] : (ax_ok && ax.has_cb ? ax.cw0 : sw_cw0); c.comb_w1 = combw ? combw[1] : (ax_ok && ax.has_cb ? ax.cw1 : sw_cw1); /* S3L-3: R4 wins over R13 (it runs after it) */
    if (r13 && r13cb) { c.comb_w0 = r13cw[0]; c.comb_w1 = r13cw[1]; } /* MDL-1: the block SETCOMBINE (the walker runs it in place) */
    c.geom_base = sw_geo; c.wind = gv_wind;
    c.tss = *mode_ok && mdl_mode.has_tx ? mdl_mode.tw1 : (ax_ok && ax.has_tx ? ax.tw1 : sw_tss);
    if (r13 && r13cw[3]) c.tss = r13cw[2]; /* MDL-1: the block's G_TEXTURE scale */ /* B9: the list TEXTURE wins; gv per range in nd_model_draw */
    if (ds) { int w = 8, h = 8; while (w < ds->w) w <<= 1; while (h < ds->h) h <<= 1; c.tex_w = w; c.tex_h = h; } /* the size of the record's texture, not the walker-bound slot */
    c.draw_3d = 1; /* S3N-3c: a native batch goes through pm_gpu_grange, which sets draw_3d */
    c.zmark = gs.r_zmark; c.zn = gs.r_zmark_zn; /* S5-2 (a) */
    c.dview = dv_mode(); c.r_memblend = r_memblend; c.r_replace = r_replace; /* MERLEE: r_mix and mixcol below, from the record's other mode */
    c.prim = nd_ws.prim; c.prim_lod = nd_ws.prim_lod; c.env = gs.env_col; c.cconst = cconst; /* B9: prim from the bridge state */
    { float db = decal_bias(); memcpy(&c.dbias, &db, 4); }
    c.base_oh = ax_ok ? (nd_ws.omode_h & ~ax.wh) | ax.om_h : nd_ws.omode_h; c.base_ol = ax_ok ? (nd_ws.omode_l & ~ax.wl) | ax.om_l : nd_ws.omode_l; /* B8; S3L-3: the R13 mode ops first */
    c.known_h = nd_ws.om_kh | (ax_ok ? ax.wh : 0u); c.known_l = nd_ws.om_kl | (ax_ok ? ax.wl : 0u);
    { /* MERLEE: the V06 colour mix (the room shroud, PM_RM_SHROUD) from this record's own other mode (the base, then its list; the shroud word is the last list op, nd_recs.c)
       * and the fog and blend colours rec_apply set. gs.r_mix is the state of the walker's last sync_render, not of this record. The F08 fog mix (2) stays the walker's. */
        unsigned mixc; const uint32_t ol_ = *mode_ok ? (c.base_ol & ~mdl_mode.wl) | mdl_mode.om_l : c.base_ol;
        if (nd_mix1(ol_, &mixc)) { c.r_mix = 1; c.mixcol = mixc; }
        else { c.r_mix = gs.r_mix == 2 ? 2 : 0; c.mixcol = r_mixcol; }
    }
    /* PM5-3: the normal rows of the walker's matrix (gm_nrm: model view rows 0 to 2, w 0) for a lit or fogged range */
    c.nrm_ok = gv5_on; for (int r_ = 0; r_ < 3; r_++) { c.nrm5[r_][0] = nd_mv[nd_mv_sp][r_][0]; c.nrm5[r_][1] = nd_mv[nd_mv_sp][r_][1]; c.nrm5[r_][2] = nd_mv[nd_mv_sp][r_][2]; c.nrm5[r_][3] = 0.f; }
    nd_mdl_cur_x = nd_tex_handle_extra((int)texidx, gfx); /* MDL-5a census: extraTiles of this draw's handle */
    int n = nd_model_draw(rec, &c, geo, emit);
    if (!emit && pm_gpu_ndm_on()) { /* SBKW-2 diagnostic: the native desc size of the record, read by ndm_cmp against the walker */
        sbkw_rec = (unsigned)(uintptr_t)rec; sbkw_dsw = ds ? ds->w : 0; sbkw_dsh = ds ? ds->h : 0; sbkw_ctw = (unsigned)c.tex_w; sbkw_cth = (unsigned)c.tex_h;
    }
    if (n < 0 && nd_nat_why == 6 && nd_nat_sub == 2 && pm_tex_aux_off_idx() >= 0) nd_nat_sub = 3; /* MDL-5 census: tile 1 is the aux off-screen image (sub c) */
    int valid = 0;
    if (n >= 0 && !emit) valid = ndc_compare(mk, ds, id, hs, hg); /* S3N-6 check: the cached draws of this key against these */
    /* MDL-1: a textured draw without a handle desc (the bound slot, R13 block) stores tex 1: its hit has no desc, so it falls back (ndn_texf) and never reuses the slot of another frame */
    if (n >= 0 && !valid) { int r13nd_ = (r13_last && ds == NULL && gfx == NULL && tex); if (r13nd_) nd_r13_nohit++; nd_mc_store(mk, ds != NULL || r13nd_, hs, hg); } /* MDL-1 cache (oracle review): only R13 texture records without a desc store tex 1 */ /* S3N-6: the draws and ranges of this key (a miss; a valid entry stays) */
    return n;
}
void pm_gpu_ndm_begin(const uint32_t* rec, const uint32_t* modew, const uint32_t* combw, const void* gfx, unsigned texidx, const void* geo, const NdMKey* mk) {
    ndm_rec = rec; ndm_live = 1; ndm_rec_ok = 1; ndm_rec_cmp = ndm_rec_diff = ndm_rec_nost = 0; ndm_nat_n = 0; ndm_j = 0; ndm_w_n = 0; ndm_geo = (unsigned)(uintptr_t)geo;
    pm_gbi_dlcc(ndm_dlc0);
    int mode_ok = 0;
    int n = model_draw(rec, modew, combw, gfx, texidx, geo, 0, &mode_ok, mk);
    ndm_mode_ok = mode_ok;
    if (!ndm_mode_ok && modew) { ndm_badop = mdl_mode.bad; ndm_badn++; } /* B9: the other op of a refused list, last one seen */
    if (n == -2) { ndm_rec_nost = 1; ndm_rec_ok = 0; return; }
    if (n < 0) { ndm_rec_ok = 0; if (nd_nat_why > 0 && nd_nat_why < 16) ndm_why[nd_nat_why]++; return; }
    ndm_nat_n = n;
}
/* S3N-4: the switch. One model record drawn natively (emit 1) after rec_apply; a fallback returns -1 and the record runs the walker geometry.
 * The counters print every 300 frames: native records, fallback records, native draws (the ranges; the wref class is not split in this mode). */
static unsigned ndn_rec, ndn_fb, ndn_draws, ndn_hit, ndn_miss, ndn_nokey, ndn_rng, ndn_texf, ndn_empty; /* PERF1: empty = hits with no draw */
void pm_gbi_dnr_take(unsigned* o, unsigned n);
static unsigned ndn_miss_all; /* MERLEE: model cache misses since boot (pm_gpu_mdl_stat) */
void pm_gpu_mdl_stat(unsigned* miss, unsigned* clears) { *miss = ndn_miss_all; *clears = nd_mc_clears(); } /* MERLEE: the fade lines of dro_02 npc_merlee.c */
static unsigned ndn_why[16]; /* S3L-1: fallback records of the window by code (0 = nost: no state bit known, the record words are not read) */
/* S3N-7: the counters of one 300-frame window (reset at the print). ndmt: ms per sampled frame of each region (ticks / 268112 per ms / frames). */
static unsigned ndn_clr0;
/* MDL-5e debug: the window so far at the end of a tour (a tour shorter than 300 frames has no ndm window line). */
/* CORE2-3a: walker side entry (nd_wq.c): the game thread posts it, the body runs on the walker side. */
static void pm_gpu_ndn_window_end_w(void);
typedef struct { int unused; } Wq_pm_gpu_ndn_window_end;
static void wq_pm_gpu_ndn_window_end(void* a) { Wq_pm_gpu_ndn_window_end* q = (Wq_pm_gpu_ndn_window_end*)a; (void)q; pm_gpu_ndn_window_end_w(); }
void pm_gpu_ndn_window_end(void) { Wq_pm_gpu_ndn_window_end q = { 0 }; pm_wq_post(wq_pm_gpu_ndn_window_end, &q, sizeof q); }
static void pm_gpu_ndn_window_end_w(void) {
    unsigned f = pm_frame_count();
    char wb[160]; unsigned wn = 0; wb[0] = 0;
    for (unsigned c = 0; c < 16u; c++) if (ndn_why[c]) { if (!wn) wn += (unsigned)snprintf(wb, sizeof wb, " why"); wn += (unsigned)snprintf(wb + wn, sizeof wb - wn, " %u:%u", c, ndn_why[c]); }
    if (pm_cen_lines) pm_log_w("ndm end f%u native %u fb %u wref %u hit %u miss %u%s", f, ndn_rec, ndn_fb, ndn_draws, ndn_hit, ndn_miss, wb);
    ndn_rec = ndn_fb = ndn_draws = ndn_hit = ndn_miss = ndn_nokey = ndn_rng = ndn_texf = 0;
    for (unsigned c = 0; c < 16u; c++) ndn_why[c] = 0;
}
static void ndn_log(unsigned f) { /* PROF2-3: census lines, from cen_lines_frame (frame end) */
    {
        if (f % 300u) return;
        unsigned clr = nd_mc_clears();
        wide_log(f);
        char wb[160]; unsigned wn = 0; wb[0] = 0; /* S3L-1: the codes of the window, nonzero only */
        for (unsigned c = 0; c < 16u; c++) if (ndn_why[c]) { if (!wn) wn += (unsigned)snprintf(wb, sizeof wb, " why"); wn += (unsigned)snprintf(wb + wn, sizeof wb - wn, " %u:%u", c, ndn_why[c]); }
        pm_log_w("ndm f%u native %u fb %u wref %u hit %u miss %u clears %u%s", f, ndn_rec, ndn_fb, ndn_draws, ndn_hit, ndn_miss, clr - ndn_clr0, wb);
        pm_log_w("ndmhit f%u nokey %u rng %u tex %u empty %u", f, ndn_nokey, ndn_rng, ndn_texf, ndn_empty);
        { static int szd; if (!szd) { szd = 1; pm_log_w("ndmsz entry %u draw %u", (unsigned)sizeof(NdMEnt), (unsigned)sizeof(NdNat)); } }
        { unsigned dg[15]; nd_mc_diag_take(dg);
          pm_log_w("ndmmiss f%u new %u clr %u rec %u st %u hd %u gv %u same %u rcap %u ncap %u ins %u ent %u pool %u ehi %u phi %u cap %u", f, dg[0], dg[1], dg[2], dg[3], dg[4], dg[5], dg[6], dg[7], dg[8], dg[9], dg[10], dg[11], dg[12], dg[13], dg[14]); }
        { unsigned dr[16]; pm_gbi_dnr_take(dr, 16u); /* S4-1: the refusal classes of the native ranges (code 12 = any of them) */
          pm_log_w("ndmr f%u c1 %u c2 %u c3 %u c4 %u c5 %u c6 %u c7 %u c8 %u c9 %u c10 %u", f, dr[1], dr[2], dr[3], dr[4], dr[5], dr[6], dr[7], dr[8], dr[9], dr[10]);
          pm_log_w("ndmr2 f%u c11 %u c12 %u c13 %u c14 %u c15 %u", f, dr[11], dr[12], dr[13], dr[14], dr[15]); }
        ndn_clr0 = clr;
        ndn_rec = ndn_fb = ndn_draws = ndn_hit = ndn_miss = ndn_nokey = ndn_rng = ndn_texf = ndn_empty = 0;
        memset(ndn_why, 0, sizeof ndn_why);
    }
}
/* PERF1-4b: a cached entry holds when the groups of its list in view now are the groups its draws were cut from (the box test of each group under the
 * combined matrix). The cut reads only those groups and the state in the key, so equal masks give the stored ranges; the re-walk of the ranges is gone. */
static int model_vis_same(const NdMKey* k, const NdMEnt* e) {
    uint64_t vm;
    return pm_dlc_vis_mask((const void*)(uintptr_t)k->w[24], &vm) == 0 && vm == e->vmask;
}
/* S3N-6 check (ndmcheck, emit 0): a cached entry of key k that the hit would take (same ranges) against the fresh draws of this record: every field the draw
 * sends. hit / miss counts the entries; diff counts the differing fields (the first 20 lines are printed). The unit words are the hinted ones, as the hit sends them. */
static unsigned ndc_hit, ndc_miss, ndc_diff, ndc_lines, ndc_copy;
static unsigned ndc_bits(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
/* S3N-7: the entry and the key of the current check (ndc_compare), and the count per walker word of the diffs (ndmcw line) */
static const NdMKey* ndc_k; static const NdMEnt* ndc_e; static unsigned ndc_wc[NDMC_RW + NDMC_SW];
static __attribute__((cold, noinline)) void ndc_cmp(const char* nm, unsigned a, unsigned b) {
    if (a == b) return;
    ndc_diff++;
    unsigned wl[8], nw = 0;
    if (ndc_e && ndc_k) for (unsigned j = NDMC_RW; j < NDMC_RW + NDMC_SW; j++) if (nd_mc_keyw(ndc_e)[j] != ndc_k->w[j]) { ndc_wc[j]++; if (nw < 8) wl[nw++] = j; }
    if (ndc_lines++ < 20) { char wb[64]; unsigned p = 0; for (unsigned i = 0; i < nw && p < 50; i++) p += (unsigned)snprintf(wb + p, sizeof wb - p, "%u ", wl[i]);
        pm_log_prof_line("ndmcache diff %s cache %x fresh %x walk %s", nm, a, b, wb); }
}
static __attribute__((noinline)) int ndc_compare(const NdMKey* k, const NdTexDesc* ds, int id, int hs, unsigned hg) {
    (void)hs; (void)hg;
    NdMEnt* e = nd_mc_find(k);
    if (!e || !model_vis_same(k, e)) { ndc_miss++; return 0; }
    ndc_hit++;
    ndc_k = k; ndc_e = e;
    NdUnit u; memset(&u, 0, sizeof u);
    if (e->tex) { if (!ds || id < 0 || !nd_tex_unit_hint(id, &e->hs, &e->hg, &u)) { ndc_diff++; return 1; } }
    unsigned fn = 0;
    while (nd_model_native(fn)) fn++;
    ndc_cmp("cache.n", e->n, fn);
    for (unsigned j = 0; j < e->n && j < fn; j++) {
        const NdNatC* c = nd_mc_nat(e, j);
        const NdNat* f = nd_model_native(j);
        ndc_cmp("tmpl", (unsigned)c->d.tmpl, (unsigned)f->d.tmpl);
        for (int i = 0; i < 6; i++) ndc_cmp("tev", c->d.tev_col[i], f->d.tev_col[i]);
        ndc_cmp("buf_col", c->d.buf_col, f->d.buf_col); ndc_cmp("has_buf", c->d.has_buf, f->d.has_buf);
        const NdUnit* cu = e->tex ? &u : &c->d.u[0];
        ndc_cmp("u0.dim", cu->dim, f->d.u[0].dim); ndc_cmp("u0.param", cu->param, f->d.u[0].param);
        ndc_cmp("u0.lod", cu->lod, f->d.u[0].lod); ndc_cmp("u0.type", cu->type, f->d.u[0].type);
        if (cu->addr != f->d.u[0].addr) ndc_copy++; /* an equal copy (same texels) at another address: counted apart */
        for (int i = 0; i < 4; i++) { if (cu->dim) ndc_cmp("uvso", ndc_bits(c->d.uvso[i]), ndc_bits(f->d.uvso[i])); /* S3L-3: uv params of an untextured draw are not read (no texel) and depend on the walker tile state: not compared */ ndc_cmp("cmul", ndc_bits(c->d.cmul[i]), ndc_bits(f->d.cmul[i])); ndc_cmp("cadd", ndc_bits(c->d.cadd[i]), ndc_bits(f->d.cadd[i])); }
        for (int i = 0; i < 2; i++) ndc_cmp("zmix", ndc_bits(c->d.zmix[i]), ndc_bits(f->d.zmix[i]));
        ndc_cmp("sd", (unsigned)(uintptr_t)c->sd, (unsigned)(uintptr_t)f->sd); ndc_cmp("nidx", c->nidx, f->nidx);
    }
    return 1;
}
/* S3N-7 diagnostic (sampled misses): the stored entry of the same model (R0 word 0 and R12 word 0 equal) and the first key word that differs. */
static unsigned mdg_cd, mdg_printed;
void pm_gpu_model_miss_diag(const NdMKey* k) {
    if (mdg_printed >= 16u) return;
    if (mdg_cd) { mdg_cd--; return; } /* PERF1: one miss in 400 (a countdown, no modulo per miss) */
    mdg_cd = 399u;
    mdg_printed++;
    unsigned n = nd_mc_size();
    for (unsigned i = 0; i < n; i++) {
        const NdMEnt* e = nd_mc_ent(i);
        const uint32_t* ew = nd_mc_keyw(e);
        if (!e->used || ew[0] != k->w[0] || ew[24] != k->w[24]) continue;
        unsigned j = 0;
        while (j < NDMC_RW + NDMC_SW && ew[j] == k->w[j]) j++;
        if (j == NDMC_RW + NDMC_SW) { pm_log_prof_line("ndmdiff same key not found rp %08x", k->rp); return; }
        pm_log_prof_line("ndmdiff w%u cache %08x now %08x rp %08x was %08x", j, ew[j], k->w[j], k->rp, e->rp);
        return;
    }
    pm_log_prof_line("ndmnew r0 %08x r0b %08x r12 %08x r2b %08x rp %08x", k->w[0], k->w[1], k->w[24], k->w[5], k->rp);
}
int pm_gpu_model_native(const uint32_t* rec, const uint32_t* modew, const uint32_t* combw, const void* gfx, unsigned texidx, const void* geo, const NdMKey* mk) {
    int mode_ok = 0;
    int n = model_draw(rec, modew, combw, gfx, texidx, geo, 1, &mode_ok, mk);
    ndn_miss_all++;
    if (n >= 0) { ndn_rec++; ndn_draws += (unsigned)n; }
    else { ndn_fb++; ndn_why[nd_nat_why & 15]++; const unsigned sub = nd_nat_why == 6 ? nd_nat_sub : 0u; if (sub) nd_mdlfb_note(mk->rp, sub); } /* S3N-7: a fallback is stored for this key (every early return of model_draw too): not drawn again while the texture epoch holds, so a texture that loads later gets the native path then */
    ndn_miss++;
    return n;
}
/* S3N-6: the hit. The cached draws of key k go out (after pm_gpu_flush) when the fresh ranges are the cached ones: each with the palette of the current view
 * and, for a textured record, the unit 0 words of the hinted slot (nd_tex_unit_hint). 1: drawn. 0: no entry, other ranges, or no texture: the caller
 * applies the record (its matrix ops are done). */
int pm_gpu_model_hit(const NdMKey* k, NdMEnt* e) {
    if (!e) { ndn_nokey++; return 0; }
    if (!model_vis_same(k, e)) { ndn_rng++; return 0; }
    if (e->mver && e->mver != ((((const uint32_t*)(uintptr_t)k->rp)[3] >> 16) | 0x10000u)) { ndn_rng++; return 0; } /* MAPX-2: a lit entry whose matrix moved: the miss path stores it again in place */
    if (!e->n) { ndn_empty++; ndn_rec++; ndn_hit++; return 1; } /* PERF1-4c: no draw (no group in view, or all culled): no texture lookup, template build, matrix product or flush */
    NdUnit u; memset(&u, 0, sizeof u);
    if (e->tex) {
        const NdTexDesc* ds = nd_tex_handle_desc((int)((k->w[0] >> 8) & 0x7fu), (const void*)(uintptr_t)k->w[1]);
        int id = ds ? nd_tex_get(ds) : -1;
        { extern int pm_tex_tagi; pm_tex_tagi = 2; } /* STALE2 texsrc path tag: hit */
        if (id < 0 || !nd_tex_unit_hint(id, &e->hs, &e->hg, &u)) { ndn_texf++; return 0; }
    }
    /* FB-2: the template ids of the cached draws are built again before any draw goes out (the LRU may have taken an id over since the store): a full table falls back */
    /* PERF1-4c: only when an eviction happened since the last build of these ids (nd_tmpl_gen); else the ids hold and only their LRU clock moves */
    if (e->tgen != nd_tmpl_gen()) {
        nd_tmpl_pin();
        for (unsigned j = 0; j < e->n; j++) { int id = nd_tmpl_build(&nd_mc_nat(e, j)->tk); if (id < 0) return 0; nd_mc_nat(e, j)->d.tmpl = id; }
        e->tgen = nd_tmpl_gen();
    } else for (unsigned j = 0; j < e->n; j++) nd_tmpl_touch(nd_mc_nat(e, j)->d.tmpl);
    static float pal0[16], zpal[16]; /* PERF2 step 3: palette 0 of this view and the zmark pass 2 row, shared by the draws (the cache entry keeps no palette) */
    float comb[4][4];
    nd_mtx_combine(comb, pm_eye_e, pm_eye_conv);
    pm_gpu_pcomp_native(pal0, comb);
    pm_gpu_flush();
    const uint32_t* rw = (const uint32_t*)(uintptr_t)k->rp; /* MERLEE: the shroud mix colour of this frame (the key leaves it out): blend colour RGB, fog colour alpha, as rec_apply sets them */
    const int shr = rw && (rw[6] & 32u);
    const uint32_t shc = shr ? nd_abgr((((rw[6] & 16u) ? rw[14] : blendc) & 0xffffff00u) | (rw[8] & 0xffu)) : 0u;
    for (unsigned j = 0; j < e->n; j++) {
        NdNatC* q = nd_mc_nat(e, j);
        if (shr && q->tk.r_mix == 1 && q->mix_st >= 0) q->d.tev_col[q->mix_st] = shc;
        if (q->zp) { memcpy(zpal, pal0, sizeof pal0); for (int i = 0; i < 4; i++) zpal[8 + i] = q->zn * pal0[12 + i]; q->d.pal = (const float(*)[16])zpal; } /* S5-2: zmark pass 2 */
        else q->d.pal = (const float(*)[16])pal0;
        if (e->tex) q->d.u[0] = u;
        pm_gpu_native_submit_draw(&q->d, q->sd, q->nidx, q->d.u[0].dim != 0);
    }
    ndn_rec++; ndn_draws += e->n; ndn_hit++;
    return 1;
}
/* S3N-3d: diff count per compared field (the first 20 lines are printed; the counts show every field) */
static char ndm_fn[48][24]; static unsigned ndm_fn_n[48], ndm_fn_used;
static void ndm_fn_count(const char* nm) {
    for (unsigned i = 0; i < ndm_fn_used; i++) if (strcmp(ndm_fn[i], nm) == 0) { ndm_fn_n[i]++; return; }
    if (ndm_fn_used < 48) { strncpy(ndm_fn[ndm_fn_used], nm, 23); ndm_fn[ndm_fn_used][23] = 0; ndm_fn_n[ndm_fn_used++] = 1; }
}
static __attribute__((cold, noinline)) void ndm_end_log(void) {
    unsigned f = pm_frame_count();
    if (f % 300u || f == ndm_f_last) return;
    ndm_f_last = f;
    pm_log_prof_line("ndmcheck f%u recs %u eq %u diff %u fb %u nodraw %u nost %u why %u %u %u %u %u %u %u %u", f, ndm_recs, ndm_eq, ndm_diff, ndm_fb, ndm_nodraw, ndm_nost, ndm_why[1], ndm_why[2], ndm_why[3], ndm_why[4], ndm_why[5], ndm_why[6], ndm_why[7], ndm_why[8]);
    pm_log_prof_line("ndmcheck why9 %u why10 %u why11 %u why12 %u badop %02x n %u", ndm_why[9], ndm_why[10], ndm_why[11], ndm_why[12], ndm_badop >> 24, ndm_badn);
    pm_log_prof_line("ndmcheck refused batches %u", ndm_refused_total);
    pm_log_prof_line("ndmcache f%u hit %u miss %u diff %u copy %u", f, ndc_hit, ndc_miss, ndc_diff, ndc_copy);
    { char wb[240]; unsigned p = 0; for (unsigned j = NDMC_RW; j < NDMC_RW + NDMC_SW && p < 200; j++) if (ndc_wc[j]) p += (unsigned)snprintf(wb + p, sizeof wb - p, "%u:%u ", j, ndc_wc[j]); pm_log_prof_line("ndmcw f%u %s", f, wb); }
    { char b[200]; unsigned p = 0; for (unsigned i = 0; i < ndm_fn_used && p < 150; i++) p += (unsigned)snprintf(b + p, sizeof b - p, "%s %u ", ndm_fn[i], ndm_fn_n[i]); pm_log_prof_line("ndmcheck fields %s", b); }
}
void pm_gpu_ndm_end(void) {
    flush(); /* the last batch of the record is drawn before its record is classified */
    if (ndm_rec_ok && ndm_j < ndm_nat_n) {
        unsigned dw[10]; pm_gbi_dlcc(dw);
        if (ndm_old) ndm_fn_count("nraw.old"); /* a refused batch is the walker's own old path: counted apart */
        else if (ndm_w_n == 0 && dw[8] == 4) ndm_fn_count("wref"); /* walker dlc_seg refused the static range (pm_gpu_grange returned 0): the ring path draws it, class wref, apart (S3N-4 decides) */
        else { ndm_rec_diff = 1; ndm_fn_count("nraw"); }
    }
    if (ndm_rec_ok && ndm_j < ndm_nat_n && !ndm_old && ndm_nraw_log++ < 6) { const NdNat* q = nd_model_native((unsigned)ndm_j); unsigned dl[10]; pm_gbi_dlcc(dl); pm_log_prof_line("ndmcheck nraw rec %08x nat %d used %d walk %u geo %08x sd %08x nidx %u tmpl %d dlc calls %u grp %u skip %u srep %u ring %u mat %u lit %u static %u dsg %u seg %u", (unsigned)(uintptr_t)ndm_rec, ndm_nat_n, ndm_j, ndm_w_n, (unsigned)ndm_geo, (unsigned)(uintptr_t)q->sd, q->nidx, q->d.tmpl, dl[0] - ndm_dlc0[0], dl[1] - ndm_dlc0[1], dl[2] - ndm_dlc0[2], dl[3] - ndm_dlc0[3], dl[4] - ndm_dlc0[4], dl[5] - ndm_dlc0[5], dl[6] - ndm_dlc0[6], dl[7] - ndm_dlc0[7], dl[8], dl[9]); }
    ndm_old = 0;
    ndm_live = 0;
    ndm_recs++;
    if (ndm_rec_nost) ndm_nost++;
    else if (!ndm_rec_ok || !ndm_mode_ok) ndm_fb++; /* B9: a record without an entry is decided in nd_model_state (why 1) */
    else if (ndm_rec_diff) ndm_diff++;
    else if (ndm_rec_cmp) ndm_eq++;
    else ndm_nodraw++;
    ndm_end_log();
}
static unsigned ndm_bits(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
#define NDM_CMPF(name, a, b) NDM_CMP(name, ndm_bits(a), ndm_bits(b))
#define NDM_CMP(name, a, b) do { if ((unsigned)(a) != (unsigned)(b)) { ndm_rec_diff = 1; ndm_fn_count(name); if (ndm_lines++ < 20) pm_log_prof_line("ndmcheck diff %s rec %08x nat %x walk %x", name, (unsigned)(uintptr_t)ndm_rec, (unsigned)(a), (unsigned)(b)); } } while (0)
static __attribute__((noinline)) void ndm_cmp(const NdTmplKey* wk, const NdDraw* wd) {
    if (!ndm_rec_ok) return;
    if (sbkw_lines < 12 && ndm_j < ndm_nat_n && nd_model_native((unsigned)ndm_j)->d.u[0].dim != wd->u[0].dim) { /* SBKW-2: the walker side at this draw: bound slot size and tile 0 window */
        const NdDraw* nd_ = &nd_model_native((unsigned)ndm_j)->d;
        int wtw = 1, wth = 1; pm_tex_size(&wtw, &wth); sbkw_lines++;
        pm_log_prof_line("sbkw dim rec %08x desc %ux%u ctex %ux%u tile0 win %dx%d td0 %dx%d walk slot %dx%d nat dim %08x walk dim %08x nat uvso1 %08x walk uvso1 %08x nat uvso3 %08x walk uvso3 %08x",
            sbkw_rec, sbkw_dsw, sbkw_dsh, sbkw_ctw, sbkw_cth,
            ((nd_ws.tst1.tile_sz[0][2] - nd_ws.tst1.tile_sz[0][0]) >> 2) + 1, ((nd_ws.tst1.tile_sz[0][3] - nd_ws.tst1.tile_sz[0][1]) >> 2) + 1, nd_ws.td[0].w, nd_ws.td[0].h, wtw, wth,
            nd_->u[0].dim, wd->u[0].dim, sbkw_bits(nd_->uvso[1]), sbkw_bits(wd->uvso[1]), sbkw_bits(nd_->uvso[3]), sbkw_bits(wd->uvso[3]));
    }
    ndm_w_n++;
    if (ndm_j >= ndm_nat_n) { ndm_rec_diff = 1; ndm_fn_count("nwalk"); return; }
    const NdNat* n = nd_model_native((unsigned)ndm_j);
    const NdTmplKey* k = &n->s.k;
    ndm_rec_cmp = 1;
    NDM_CMP("k.pass", k->pass, wk->pass); NDM_CMP("k.raw", k->raw, wk->raw); NDM_CMP("k.cull", k->cull, wk->cull);
    NDM_CMP("k.z", k->z, wk->z); NDM_CMP("k.zmark", k->zmark, wk->zmark); NDM_CMP("k.zcmp", k->zcmp, wk->zcmp); NDM_CMP("k.zupd", k->zupd, wk->zupd);
    NDM_CMP("k.nocol", k->nocol, wk->nocol); NDM_CMP("k.decal", k->decal, wk->decal); NDM_CMP("k.atest_on", k->atest_on, wk->atest_on); NDM_CMP("k.aref", k->aref, wk->aref);
    NDM_CMP("k.combine", k->combine, wk->combine); NDM_CMP("k.tex", k->tex, wk->tex); NDM_CMP("k.tex1", k->tex1, wk->tex1);
    NDM_CMP("k.i8_0", k->i8_0, wk->i8_0); NDM_CMP("k.i8_1", k->i8_1, wk->i8_1); NDM_CMP("k.rgb_scale", k->rgb_scale, wk->rgb_scale); NDM_CMP("k.r_mix", k->r_mix, wk->r_mix);
    NDM_CMP("k.draw_3d", k->draw_3d, wk->draw_3d); NDM_CMP("k.rect_2d", k->rect_2d, wk->rect_2d); NDM_CMP("k.dview", k->dview, wk->dview);
    NDM_CMP("k.off_active", k->off_active, wk->off_active); NDM_CMP("k.off_i8", k->off_i8, wk->off_i8);
    NDM_CMP("k.r_memblend", k->r_memblend, wk->r_memblend); NDM_CMP("k.r_replace", k->r_replace, wk->r_replace); NDM_CMP("k.r_blend", k->r_blend, wk->r_blend);
    NDM_CMP("k.noise", k->noise, wk->noise); NDM_CMP("k.two", k->two, wk->two); NDM_CMP("k.t1only", k->t1only, wk->t1only);
    NDM_CMP("k.w0", k->w0, wk->w0); NDM_CMP("k.w1", k->w1, wk->w1); NDM_CMP("k.dbias", k->dbias, wk->dbias);
    NDM_CMP("tmpl", n->d.tmpl, wd->tmpl);
    for (int i = 0; i < 6; i++) { char nm[16]; snprintf(nm, sizeof nm, "tev%d", i); NDM_CMP(nm, n->d.tev_col[i], wd->tev_col[i]); }
    NDM_CMP("buf_col", n->d.buf_col, wd->buf_col); NDM_CMP("has_buf", n->d.has_buf, wd->has_buf);
    NDM_CMP("u0.dim", n->d.u[0].dim, wd->u[0].dim); NDM_CMP("u0.param", n->d.u[0].param, wd->u[0].param); NDM_CMP("u0.lod", n->d.u[0].lod, wd->u[0].lod);
    NDM_CMP("u0.type", n->d.u[0].type, wd->u[0].type);
    if (n->d.u[0].addr != wd->u[0].addr) ndm_fn_count("u0.copy"); /* an equal copy (same key, format, size, dim, param, lod, type; same texels) at another address: counted apart */
    for (int i = 0; i < 4; i++) { char nm[16]; snprintf(nm, sizeof nm, "uvso%d", i); if (n->d.u[0].dim) NDM_CMPF(nm, n->d.uvso[i], wd->uvso[i]); } /* S3L-3: untextured draws: not compared (see ndc_cmp) */
    /* SBKW-3: unit 1 and its uv map (tile 1 of a two-tile surface); not compared before. */
    NDM_CMP("has_uvx", n->d.has_uvx, wd->has_uvx);
    if (wd->has_uvx) {
        NDM_CMP("u1.dim", n->d.u[1].dim, wd->u[1].dim); NDM_CMP("u1.param", n->d.u[1].param, wd->u[1].param); NDM_CMP("u1.lod", n->d.u[1].lod, wd->u[1].lod);
        NDM_CMP("u1.type", n->d.u[1].type, wd->u[1].type); NDM_CMP("loc_uvx", n->d.loc_uvx, wd->loc_uvx);
        if (n->d.u[1].addr != wd->u[1].addr) ndm_fn_count("u1.copy");
        for (int i = 0; i < 4; i++) { char nm[16]; snprintf(nm, sizeof nm, "uvx%d", i); NDM_CMPF(nm, n->d.uvx[i], wd->uvx[i]); }
    }
    /* SBKW-3: the lit and fogged block (pm5 program 5): fx5 (light, fog, texgen), normals, the pm5 flag. Not compared before. */
    NDM_CMP("use5", n->d.use5, wd->use5);
    { static const NdPm5 z5; const NdPm5* a5 = n->d.p5 ? n->d.p5 : &z5; const NdPm5* b5 = wd->p5 ? wd->p5 : &z5; /* PERF2: no block reads as zeros (the walker draw has none) */
      NDM_CMP("fx5", memcmp(&a5->fx, &b5->fx, sizeof a5->fx) != 0, 0);
      NDM_CMP("nrm5", memcmp(a5->nrm, b5->nrm, sizeof a5->nrm) != 0, 0);
      if (a5->fx.fogu[0] != b5->fx.fogu[0] || a5->fx.fogu[1] != b5->fx.fogu[1]) ndm_fn_count("fogu"); }
    for (int i = 0; i < 4; i++) { char nm[16]; snprintf(nm, sizeof nm, "cmul%d", i); NDM_CMPF(nm, n->d.cmul[i], wd->cmul[i]); }
    for (int i = 0; i < 4; i++) { char nm[16]; snprintf(nm, sizeof nm, "cadd%d", i); NDM_CMPF(nm, n->d.cadd[i], wd->cadd[i]); }
    for (int i = 0; i < 2; i++) { char nm[16]; snprintf(nm, sizeof nm, "zmix%d", i); NDM_CMPF(nm, n->d.zmix[i], wd->zmix[i]); }
    NDM_CMP("npal", 1u, wd->npal);
    if (wd->npal >= 1) for (int i = 0; i < 16; i++) NDM_CMPF("pal0", n->pal[i], wd->pal[0][i]);
    /* index words: the walker batch (gidx from gi_start, gni indices) is the native draws in order, each copied in place */
    const uint16_t* wi = nb.gidx + nb.gi_start;
    unsigned got = 0;
    while (got < wd->nidx && ndm_j < ndm_nat_n) {
        const NdNat* q = nd_model_native((unsigned)ndm_j);
        if (got + q->nidx > wd->nidx) break;
        if (q->d.tmpl != n->d.tmpl) { ndm_rec_diff = 1; ndm_fn_count("tmpl.span"); }
        if (memcmp(wi + got, q->sd, q->nidx * 2u) != 0) { ndm_rec_diff = 1; ndm_fn_count("idx.content"); }
        got += q->nidx; ndm_j++;
    }
    if (got != wd->nidx) { ndm_rec_diff = 1; ndm_fn_count("idx.span"); ndm_j = ndm_nat_n; }
}
/* debug switch ndplan: the layout of the plan that pm_comb_build gives for the key words of a native draw against the plan the old path draws with. A mismatch means the template of that key holds another TEV layout. */
extern int pm_comb_build(unsigned w0, unsigned w1, int two, int t1only, PmPlan* out);
static __attribute__((cold, noinline)) void ndplan_check(unsigned w0, unsigned w1, int two, int t1only) {
    static unsigned bad, runs;
    if (ndplan_on < 0) ndplan_on = pm_debug_has("ndplan");
    if (!ndplan_on) return;
    PmPlan pp;
    memset(&pp, 0, sizeof pp);
    runs++;
    int rc = pm_comb_build(w0, w1, two, t1only, &pp);
    if ((rc != 0 || !plan_same_layout(&gs.plan, &pp)) && bad++ < 40) pm_log("ndplan MISMATCH f%u w0 %08x w1 %08x two %d t1 %d rc %d run %u", pm_frame_count(), w0, w1, two, t1only, rc, runs);
    if (!(runs % 3000)) pm_log("ndplan runs %u mismatches %u", runs, bad);
}

static uint32_t ndu_reg[0x20], ndu_seen[0x20]; static unsigned ndu_pos; static int ndu_seen_now, ndu_on = -1;
/* decodes the commands written since the last scan into the register shadow; runs before every command list split too (the unread tail of the old buffer is gone after it) */
static __attribute__((cold, noinline)) void ndunit_scan(void) {
    if (ndu_on < 0) ndu_on = pm_debug_has("ndunit");
    if (!ndu_on) return;
    u32 *buf, size, off;
    GPUCMD_GetBuffer(&buf, &size, &off);
    if (off < ndu_pos) ndu_pos = 0; /* the buffer restarted */
    while (ndu_pos < off) {
        uint32_t v = buf[ndu_pos], h = buf[ndu_pos + 1];
        unsigned r = h & 0xFFFFu, n = ((h >> 20) & 0xFFu) + 1u, cons = h >> 31;
        for (unsigned i = 0; i < n; i++) {
            unsigned rr = cons ? r + i : r;
            uint32_t w = i ? buf[ndu_pos + 1 + i] : v;
            if (rr >= 0x80 && rr < 0xA0) { unsigned m = (h >> 16) & 0xFu, bm = (m & 1u ? 0xFFu : 0u) | (m & 2u ? 0xFF00u : 0u) | (m & 4u ? 0xFF0000u : 0u) | (m & 8u ? 0xFF000000u : 0u); ndu_reg[rr - 0x80] = (ndu_reg[rr - 0x80] & ~bm) | (w & bm); ndu_seen[rr - 0x80] = 1; if (rr >= 0x81 && rr <= 0x85) ndu_seen_now = 1; }
        }
        ndu_pos += n + 1u + ((n & 1u) ? 0u : 1u);
    }
}
/* after the native words of a textured draw: the unit enable word the command buffer ends with must have unit 0 on */
static __attribute__((cold, noinline)) void ndunit_after(void) {
    static unsigned bad, runs;
    if (ndu_on < 0) ndu_on = pm_debug_has("ndunit");
    if (!ndu_on) return;
    ndunit_scan();
    runs++;
    if (!(ndu_reg[0] & 1u) && bad++ < 30) pm_log("ndunit UNIT 0 OFF after a textured native draw f%u reg80 %08x run %u", pm_frame_count(), ndu_reg[0], runs);
    if (!(runs % 3000)) pm_log("ndunit after: runs %u unit off %u", runs, bad);
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

void pm_gpu_render_src(unsigned omode_h, unsigned omode_l, int depth_img) { rs_src[0] = omode_h; rs_src[1] = omode_l; rs_src[2] = (unsigned)depth_img; }
void pm_gpu_plan_key(unsigned w0, unsigned w1, int two, int t1only) { plan_kp[0] = w0; plan_kp[1] = w1; plan_kp[2] = (unsigned)two; plan_kp[3] = (unsigned)t1only; }
void pm_gpu_set_plan(const PmPlan* p) {
    if (gs.combine == 8 && memcmp(&gs.plan, p, sizeof gs.plan) == 0) return;
    CS_ADD(CS_PLAN, 1);
    if (gs.combine == 8 && plan_same_layout(&gs.plan, p)) {
        CS_ADD(CS_CON, 1);
        if (prim != cs_last_prim) CS_ADD(CS_PRIM, 1);
        if (gs.env_col != cs_last_env) CS_ADD(CS_ENV, 1);
    } else {
        CS_ADD(CS_LAY, 1);
        if (gs.combine == 8) plan_layout_field_diffs(&gs.plan, p);
    }
    flush();
    gs.plan = *p;
    memcpy(plan_k, plan_kp, sizeof plan_k);
    plan_gen++;
    gs.combine = 8;
    cs_last_prim = prim;
    cs_last_env = gs.env_col;
}

void pm_gpu_set_combine(int mode) {
    if (mode == gs.combine) return;
    CS_ADD(CS_MODE, 1);
    flush();
    gs.combine = mode;
}

/* The RDP can brighten past 1 in the second cycle. Texture * color * 2 gives that range. */
int pm_gpu_rgb_scale_get(void) { return rgb_scale; } /* FLK1 state dump */
void pm_gpu_set_rgb_scale(int scale) {
    if (scale == rgb_scale) return;
    flush();
    rgb_scale = scale;
}

/* SPR4-prim: the open batch reads PRIM and PRIM_LOD in combine 1, 5, 6, 7 and in combine 8 (the plan has no source mask, so any change closes it). The
 * batch draws with the prim of its flush, so a change while it is open must close it first. */
static unsigned prim_lod_g = 0xffffffffu;
void pm_gpu_set_prim(unsigned rgba, unsigned lod) {
    int reads = gs.combine == 1 || gs.combine == 5 || gs.combine == 6 || gs.combine == 7 || gs.combine == 8;
    if (reads && (rgba != prim || lod != prim_lod_g)) flush();
    prim = rgba;
    prim_lod_g = lod;
}

void pm_gpu_set_mix(int on, unsigned rgba) {
    if (!on) rgba = 0;
    if (on == gs.r_mix && rgba == r_mixcol) return;
    flush();
    gs.r_mix = on;
    r_mixcol = rgba;
}

/* SPR4-env: the open batch reads ENV at its flush (combine 5, 7 and the combine 8 plans), so a change while it is open closes it first.
 * The batch-open tests need no env compare: no change can happen while a batch is open. */
void pm_gpu_set_env(unsigned rgba) {
    if (rgba != gs.env_col && (nb.gni > 0 || nb.rect_open)) flush();
    gs.env_col = rgba;
}

/* SPR4-env: cconst is in the TEV key of combine 4 and 8 (the batch draws with the cconst of its flush). */
void pm_gpu_set_const(unsigned rgba) {
    if (rgba == cconst) return;
    if (gs.combine == 4 || gs.combine == 8 || (nb.rect_open && nb.rect_ec == 4)) flush(); /* MSGBOX 2: an open class 4 rectangle batch draws with the cconst of its flush */
    cconst = rgba;
}

void pm_gpu_flush(void) { flush(); }

/* SPR4-3: shaded sprites (nd_int.h is not included here; the former pm_gbi.c, nd_tex.c and nd_geom.c own these) */
extern int dual_on; void ensure_tile(int n); void dual_bind(void); int dual_boost_on(void);
static unsigned spr_nat_n, spr_nat_fb[16];
static unsigned spr5_sub[8]; /* MDL-3a: class 5 refusals per flag: 1 no pm4 palette path (g_pal, g_use5, gv4_on), 2 off-screen target, 3 grab, 4 zmark, 5 nozt */
static unsigned spc_texget, spc_miss, spc_rh0[2]; /* PERF2: full texture lookups of sprites and the dirty-slot rehash totals at the last sprcost line */
/* PERF2 step 5: the sprite texture memo. Key: raster, palette, width and height of the record; value: the block id (valid in its nd_tex_gen) and the slot hint
 * (hs, hg) of nd_tex_unit_hint, which checks the slot (clean, key, generation) at each draw. 32 entries, direct mapped. A miss is the old lookup (nd_tex_get). */
typedef struct { uint32_t raster, pal, wh; unsigned gen; int id, hs; unsigned hg; } SprTexMemo;
static SprTexMemo sptm[32];
static unsigned spck_on = 2, spck_recs, spck_eq, spck_diff, spck_fb; /* sprcheck switch (2 unread): the memo unit words against a fresh lookup */
static __attribute__((cold, noinline)) void spr_tex_check(const NdTexDesc* ds, const NdUnit* got) {
    static unsigned lastf;
    const unsigned f = pm_frame_count();
    if (f != lastf && f % 300u == 0) pm_log_evt("sprcheck f%u recs %u eq %u diff %u fb %u", f, spck_recs, spck_eq, spck_diff, spck_fb); /* the until-line of spot_mac01_sprcheck (the counts before this frame) */
    lastf = f;
    NdUnit u; int id = nd_tex_get(ds);
    spck_recs++;
    if (id < 0 || !nd_tex_unit(id, &u)) { spck_fb++; return; }
    if (u.dim == got->dim && u.param == got->param && u.lod == got->lod && u.addr == got->addr && u.type == got->type) spck_eq++;
    else { spck_diff++; if (spck_diff <= 8) pm_log_prof_line("sprcheck diff f%u dim %x/%x param %x/%x addr %x/%x type %x/%x", pm_frame_count(), u.dim, got->dim, u.param, got->param, u.addr, got->addr, u.type, got->type); }
}
static int spr_tex_unit(const NdTexDesc* ds, uint32_t raster, uint32_t pal, uint32_t wh, NdUnit* un) {
    const uint32_t h = (raster >> 4) ^ (raster >> 11) ^ (pal >> 5) ^ wh;
    SprTexMemo* m = &sptm[(h ^ (h >> 16)) & 31u];
    const unsigned g = nd_tex_gen();
    if (m->raster != raster || m->pal != pal || m->wh != wh || m->gen != g || m->id < 0) {
        m->id = nd_tex_get(ds); spc_texget++; spc_miss++;
        m->raster = raster; m->pal = pal; m->wh = wh; m->gen = nd_tex_gen(); m->hs = -1; m->hg = 0;
        if (m->id < 0) return -1;
    }
    if (!nd_tex_unit_hint(m->id, &m->hs, &m->hg, un)) return 0;
    if (__builtin_expect(spck_on != 0, 0)) { if (spck_on == 2) spck_on = pm_debug_has("sprcheck") ? 1u : 0u; if (spck_on) spr_tex_check(ds, un); }
    return 1;
}
static void spr_nat_log(unsigned f) { /* PROF2-3: census lines, from cen_lines_frame (frame end) */
    if (f % 300u) return;
    { void pm_tex_rehash_counts(unsigned* same, unsigned* diff); unsigned rs, rd; pm_tex_rehash_counts(&rs, &rd);
      pm_log_w("sprcost f%u spr %u texget %u rehash same %u diff %u (spr total, the rest per 300 frames)", f, spr_nat_n, spc_texget, rs - spc_rh0[0], rd - spc_rh0[1]);
      spc_rh0[0] = rs; spc_rh0[1] = rd; spc_texget = 0; }
    pm_log_w("sprnat f%u native %u fb %u why 2 %u why 3 %u why 4 %u why 5 %u other %u", f, spr_nat_n, spr_nat_fb[1] + spr_nat_fb[2] + spr_nat_fb[3] + spr_nat_fb[4] + spr_nat_fb[5] + spr_nat_fb[20] + spr_nat_fb[11] + spr_nat_fb[12],
        spr_nat_fb[2], spr_nat_fb[3], spr_nat_fb[4], spr_nat_fb[5], spr_nat_fb[1] + spr_nat_fb[20] + spr_nat_fb[11] + spr_nat_fb[12]);
    pm_log_w("spr5 f%u nopal %u off %u grab %u zmark %u nozt %u", f, spr5_sub[1], spr5_sub[2], spr5_sub[3], spr5_sub[4], spr5_sub[5]);
}
int pm_gpu_spr_native(const uint32_t* rec) {
    NdDraw n;
    NdMState st;
    float pal0[16];
    int why = 0;
    if (!nd_sprite_draw(rec, 1, &n, &st, pal0, &why)) { spr_nat_fb[why & 15]++; return 0; }
    spr_nat_n++;
    return 1;
}
/* PROF2-3: the census lines, once per frame at the frame end (pm_gpu_end_w), only with pm_cen_lines (phase mode or the census word, never prof mode).
 * They print with pm_log_w, not as prof lines. The counters stay on every path; the record paths no longer test the frame. */
void ent_log(void); /* nd_ent.c */
void nd_model_log(unsigned f); /* nd_model.c */
static __attribute__((cold, noinline)) void cen_lines_frame(void) {
    unsigned f = pm_frame_count();
    if (!f) return; /* the boot frame: no window yet (the record paths printed no window line at frame 0) */
    if (f % 100u == 0) rect_log(f); /* RECT-2b diag: rect and rectwhy lines every 100 frames */
    ndn_log(f);
    spr_nat_log(f);
    ent_log();
    nd_model_log(f);
}
void pm_gpu_cnt_take(unsigned o[8]) { /* PERF2 step 6: the counters of the prof cls line, since the last take */
    static unsigned last[8];
    const unsigned now[8] = { sc_prg, sc_shr, sc_upd, sc_spl, spr_nat_n - spc_miss, spc_miss, nd_ent_cfull_all(), nd_gdl_lists() };
    for (int i = 0; i < 8; i++) { o[i] = now[i] - last[i]; last[i] = now[i]; }
}
void pm_gbi_gv_at(unsigned tss, uint32_t geo, int tw, int th, NdGvP* o);
/* S4-2: the sprite record as a native draw (nd_sprite_draw, nd_sprite.c). The walker inputs of the draw (sticky state, gv, env and prim of the record),
 * the NdDraw without palette and index (set by pm_gpu_spr_submit), the key st and palette 0 (the walker combined matrix, post x combined). 1 native, 0 with why
 * (1 mode, 2 unit 0, 10 + nd_model_state code). */
int pm_gpu_spr_build(const uint32_t* rec, NdDraw* n, NdMState* st, float pal0[16], int* why) {
    NdSprRec r;
    nd_sprite_decode(rec, &r);
    /* SPR4: the record's render mode list and final list ran on the walker before this vertex word, so the draw state is nd_ws live and the mode adds nothing */
    NdMode mode;
    memset(&mode, 0, sizeof mode);
    mode.ok = 1; mode.gand = 0xFFFFFFFFu;
    { /* MDL-3a: class 5 by sub-reason, each flag counted on its own (a record can hold several) */
        const int nopal = !(nb.g_pal && !nb.g_use5 && gs.gv4_on);
        spr5_sub[1] += (unsigned)nopal; spr5_sub[2] += (unsigned)(gs.off_active != 0); spr5_sub[3] += (unsigned)(grab_bind != 0); spr5_sub[4] += (unsigned)(gs.r_zmark != 0);
        if (nopal || gs.off_active || grab_bind || gs.r_zmark) { *why = 5; return 0; } /* the walker raw path refusals (nd_raw_native) */
    }
    NdTexDesc ds;
    NdUnit un;
    memset(&ds, 0, sizeof ds); memset(&un, 0, sizeof un);
    const int shaded = (r.flags & 1u) != 0; /* SPR4-3: SPR_SHADING_FLAG_ENABLED. The walker bakes the shaded image at SetTileSize 0 (tex_shaded0, dual_on), before the vertex word: the live tile 0 is that texture */
    const int tex = r.raster != 0;
    int tw = 8, th = 8;
    if (shaded) {
        if (!dual_on || nd_ws.use_t1) { *why = 3; return 0; } /* no bake live, or the plan reads TEXEL1 (no unit 1 form yet) */
        ensure_tile(0);
        dual_bind();
        if (pm_tex_native_unit0(&un) != 1) { *why = 2; return 0; }
        while (tw < (int)r.w) tw <<= 1;
        while (th < (int)r.h) th <<= 1;
    } else if (tex) { /* the unit 0 words of the record's raster (CI4, clamp), as the walker binds it */
        ds.img = (const void*)(uintptr_t)r.raster; ds.pal = (const void*)(uintptr_t)r.palette;
        ds.fmt = 2; ds.siz = 0; ds.cms = 2; ds.cmt = 2; ds.masks = 8; ds.maskt = 8; /* G_IM_FMT_CI, G_IM_SIZ_4b, G_TX_CLAMP (gbi.h values; nd_model.c uses the numbers too) */
        ds.w = (uint16_t)r.w; ds.h = (uint16_t)r.h; ds.stride = (uint16_t)r.w; ds.pal_n = 16;
        const int ur = spr_tex_unit(&ds, r.raster, r.palette, r.w << 16 | r.h, &un); /* PERF2: through the sprite texture memo */
        { extern int pm_tex_tagi; pm_tex_tagi = 3; } /* STALE2 texsrc path tag: spr */
        if (ur != 1) { /* S4-2 diagnosis: the first 3 texture refusals (record size, result) */
            static unsigned tn; if (tn++ < 3u) pm_log_prof_line("sprtex %u w %u h %u raster %08x pal %08x unit %d", tn, r.w, r.h, (unsigned)r.raster, (unsigned)r.palette, ur);
            *why = 2; return 0; }
        while (tw < (int)r.w) tw <<= 1;
        while (th < (int)r.h) th <<= 1;
    }
    unsigned sw_tss, sw_geo, sw_cw0, sw_cw1;
    pm_gbi_sticky(&sw_tss, &sw_geo, &sw_cw0, &sw_cw1);
    const uint32_t geo = (sw_geo & mode.gand) | mode.gor;
    NdGvP gv;
    pm_gbi_gv_at(mode.has_tx ? mode.tw1 : sw_tss, geo, tw, th, &gv);
    NdMCtx c;
    memset(&c, 0, sizeof c);
    c.mode = &mode; c.u0 = (tex || shaded) ? &un : NULL; c.i8_0 = 0; c.gv = &gv;
    c.rgb_scale = shaded ? (dual_boost_on() ? 2 : 1) : rgb_scale; /* S4-1 fix 1: the walker's sticky rgb scale (1 or 2, set by pm_gpu_set_rgb_scale) at the batch; the key diff showed rgb_scale 0/1. Shaded: the walker sets 2 under the dual boost, else 1 */
    c.has_comb = 1; c.comb_w0 = sw_cw0; c.comb_w1 = sw_cw1;
    c.tex_w = tw; c.tex_h = th; c.tss = mode.has_tx ? mode.tw1 : sw_tss;
    c.geom_base = sw_geo; c.wind = gv_wind; c.geom_at_set = 0;
    c.draw_3d = 1; c.dview = dv_mode(); c.r_mix = gs.r_mix; c.r_memblend = r_memblend; c.r_replace = r_replace;
    c.env = nd_ws.envc; c.prim = nd_ws.prim; c.prim_lod = nd_ws.prim_lod; c.cconst = cconst; c.mixcol = r_mixcol;
    { float db = decal_bias(); memcpy(&c.dbias, &db, 4); }
    c.base_oh = nd_ws.omode_h; c.base_ol = nd_ws.omode_l; c.known_h = nd_ws.om_kh; c.known_l = nd_ws.om_kl;
    uint32_t zr[27];
    memset(zr, 0, sizeof zr); /* the record words the model state reads (R3 flags, R11, R13) are zero: a sprite has none */
    if (nd_model_state_spr(zr, &c, n, st) != 1) { *why = 10 + st->why; return 0; } /* PERF1-5: through the sprite state memo */
    n->nidx = 6; /* pm_gpu_spr_submit: two triangles of the quad */
    pm_gpu_pcomp_native(pal0, pm_gbi_combined_ptr());
    return 1;
}
/* S4-2: the native submit of a sprite (nd_sprite_draw with emit): flush the open walker batch, the four vertices of the record quad into the ring with palette
 * entry 0 (the sprite matrix, pal[0] of d), the two triangles 0 2 1, 0 3 2 into the index ring, then nd_native_submit. 0 when the ring has no room (nothing drawn). */
int pm_gpu_spr_submit(NdDraw* d, const void* quad, int tex) {
    rp_src_ = 3;
    static const unsigned char tri[6] = { 0, 2, 1, 0, 3, 2 };
    flush();
    nb.gi_kind = 0;
    if (!gs.gpu_vtx_on || !gs.gpu_ok || !ring_need(4, 6)) return 0; /* FB-1: a full ring wraps first */
    const int b = pm_gpu_gvtx_load(quad, 4);
    if (b < 0) return 0;
    for (unsigned i = 0; i < 4; i++) *(unsigned short*)(nb.gring + 16u * ((unsigned)b + i) + 6) = 0;
    unsigned short* ix = nb.gidx + nb.gidx_n;
    for (unsigned i = 0; i < 6; i++) ix[i] = (unsigned short)((unsigned)b + tri[i]);
    nb.gidx_n += 6;
    d->loc_pal = (unsigned)u4_pal; d->loc_var = (unsigned)u4_uvso;
    d->idx_word = 0x80000000u | ((uint32_t)osConvertVirtToPhys(ix) - 0x18000000u);
    d->nidx = 6;
    nd_native_submit(d, tex);
    return 1;
}
const char* pm_gpu_last_flush(int* line) { *line = fl_line; return fl_fn; } /* the hang watchdog prints it */
unsigned pm_ec_n, pm_ec_svc; /* EC2: empty closes, svc reads inside them */
unsigned pm_fl_n[6], pm_fl_e[6], pm_fl_us[6]; /* PI2: batch closes per class, empty closes, us per class (drawn frame, phase on) */
unsigned pm_dc_c[DC_N], pm_dc_us[DCU_N]; /* DC1 */
void pm_gpu_flush_at(int line) { /* PG0: a flush asked for by the display list interpreter, line of the call in the former pm_gbi.c */
    fl_fn = "gbi"; fl_line = line;
    /* EC2: no vertex and no raw batch queued: flush_draw returns before any state, texture, cache or GPU call. Only the
     * rectangle bookkeeping of flush_impl runs. No timer (svc read) here, only a plain count. */
    if (nb.gni <= 0) {
        unsigned s0 = pm_svc_n;
        pm_ec_n++;
        if (pm_ph_cen) { int c = pm_gbi_flush_class(); pm_fl_n[c]++; pm_fl_e[c]++; }
        flush_impl();
        pm_ec_svc += pm_svc_n - s0; /* 0 when the path makes no svc call */
        return;
    }
    pm_dc_c[DC_CL]++;
    int c = 0;
    if (pm_ph_cen) { c = pm_gbi_flush_class(); pm_fl_n[c]++; { extern void pm_gbi_census_close(void); pm_gbi_census_close(); } } /* T2b: non-empty close per producer */
    dc_on = (dc_seq++ & 7) == 0 && (pm_ph_cen || pm_dcs_on); /* DC1: one close in 8 is sampled, with phase mode or dcsum (PROF2-2: not in a prof sample) */
    if (!dc_on) { flush_impl(); return; } /* W2: the flush timer uses the DC1 sample, one close in 8, scaled by 8 */
    unsigned t0 = pm_ph_cen ? pm_ticks() : 0; /* the per-class flush time is phase only */
    flush_impl();
    dc_on = 0;
    if (pm_ph_cen) pm_fl_us[c] += (pm_ticks() - t0) * 8u / 268u;
}
/* DS0: cold. Called once per drawn frame from pm_gbi_end_frame (phase or dcsum on). It takes and clears pm_dc_c and pm_dc_us. Every 300 frames it prints one dcsum line. */
void pm_gpu_dcsum_frame(unsigned frame) {
    dcs_cl += pm_dc_c[DC_CL];
    for (int k = 0; k < DCU_N; k++) dcs_us[k] += pm_dc_us[k];
    for (int k = 0; k < DC_N; k++) dcs_c[k] += pm_dc_c[k];
    memset(pm_dc_c, 0, sizeof pm_dc_c);
    memset(pm_dc_us, 0, sizeof pm_dc_us);
    if (++dcs_n < 300) return;
    unsigned smp = dcs_smp ? dcs_smp : 1, cl = dcs_cl ? dcs_cl : 1, fl = dcs_fl ? dcs_fl : 1;
    pm_log_hw("dcsum f%u cl %u per frame | us per close pal %u bind %u tev %u st %u draw %u | gc %u us per flush, %u flushes per frame | cmd %u words per sampled close, %u sampled closes | svc flush fallbacks %u",
              frame, dcs_cl / 300u, dcs_us[DCU_PAL] / smp, dcs_us[DCU_BIND] / smp, dcs_us[DCU_TEV] / smp, dcs_us[DCU_ST] / smp, dcs_us[DCU_DRAW] / smp,
              (unsigned)(dcs_gt / 268u / fl), dcs_fl / 300u, dcs_cmd / smp, dcs_smp, pm_fl_gsp_n);
    /* DC6: groups per close (sums over all closes): shadow misses, texture binds unit 0 / 1 / staged, TEV rebuilds, palette uploads */
    pm_log_hw("dcgrp f%u hundredths per close: shadow miss %u, bind0 %u, bind1 %u, bindstaged %u, tev %u, palup %u, palsame %u, pmul %u, prog %u, bufinfo %u",
              frame, dcs_c[DC_SHM] * 100u / cl, dcs_c[DC_TB0] * 100u / cl, dcs_c[DC_TB1] * 100u / cl, dcs_c[DC_TBS] * 100u / cl,
              dcs_c[DC_TEVR] * 100u / cl, dcs_c[DC_PALUP] * 100u / cl, dcs_c[DC_PKEQ] * 100u / cl, dcs_c[DC_PMUL] * 100u / cl, dcs_c[DC_PRG] * 100u / cl, dcs_c[DC_BUF] * 100u / cl);
    dcs_gt = dcs_fl = dcs_cmd = dcs_smp = dcs_cl = dcs_n = 0;
    memset(dcs_us, 0, sizeof dcs_us);
    memset(dcs_c, 0, sizeof dcs_c);
}

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





/* F03: one pixel of the last finished depth buffer (D24S8, same tiling and rotation as pm_gpu_read_frame). */
/* CORE2-3a: walker side entry (nd_wq.c): the game thread calls it, the body runs on the walker side. */
static int pm_gpu_depth_at_w(int x, int y, unsigned* d24);
typedef struct { int x; int y; unsigned* d24; int ret; } Wq_pm_gpu_depth_at;
static void wq_pm_gpu_depth_at(void* a) { Wq_pm_gpu_depth_at* q = (Wq_pm_gpu_depth_at*)a; (void)q; q->ret = pm_gpu_depth_at_w(q->x, q->y, q->d24); }
int pm_gpu_depth_at(int x, int y, unsigned* d24) { Wq_pm_gpu_depth_at q = { x, y, d24 }; pm_wq_call(wq_pm_gpu_depth_at, &q, sizeof q); return q.ret; }
static int pm_gpu_depth_at_w(int x, int y, unsigned* d24) {
    pm_gpu_begin_wait();
    if (nb.rect_open) flush(); /* DR1 */
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

