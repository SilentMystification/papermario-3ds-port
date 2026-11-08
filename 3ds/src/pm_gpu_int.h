/* pm_gpu_int.h: state that pm_gpu.c and pm_gpu_ui.c share (C8: the bottom screen text moved out of pm_gpu.c, no logic change). Not a public interface. */
#pragma once
#include <3ds.h>
/* Bottom screen rows, 1-based as the cursor and ANSI count them (row 1 is the top). The HUD (pm_gpu_hud_flush) has fixed rows:
 * 1 fps, 2 memory, 3 VRAM, 4 to 6 the map block (the old and new map during the 2 s change, else the current map).
 * Then, when on: the Mario row (poshud), the stats row (stats). Then the separator, the log header (console and SD level),
 * the log window (LOG_TOP to 26, it scrolls only here), separator 27, footer 28 to 30 (map, system).
 * hud_x = the rows added by poshud and stats: the separator is row 7+hud_x, the header 8+hud_x, the log window 9+hud_x to 26. */
#define HUD_X (pos_on + stats_on)
#define LOG_TOP (9 + HUD_X)
#define LOG_ROWS (18 - HUD_X)
#define LAY_KEY (2 * pos_on + stats_on)
extern const char* const pm_lvl_name[5]; /* pm_options.c: Error, Warn, Info, Debug, Off */
extern volatile int pos_on, stats_on;
extern int frame_key, frame_lay;
extern volatile int hud_new, foot_new;
extern char foot_row[2][40];
extern PrintConsole bottom_con;
extern int menu_on, menu_dirty; /* pm_gpu_ui.c: the menu text is on the bottom screen; a draw since the last present (pm_gpu_menu_present) */
#include <citro3d.h>
#include "pm_gpu.h"
enum { GPAL = 21 }; /* GV1c: the most game matrices in the palette of one pm4 batch */
/* FB-1: the raw ring is a chunk list. Chunk 0 is the boot allocation; its tail is the static area (GRING_VTX and up). stamp = the frame that used the chunk;
 * a chunk with stamp < ring_done is finished on the GPU. */
enum { RING_CHUNK_MAX = 8 };
typedef struct { unsigned char* vtx; unsigned short* idx; unsigned stamp; int used; } RingChunk;
/* The open raw batch and the vertex ring: the state the batch close (flush_draw, nd_raw_native), the triangle path (gtrip, gappend), the rectangle
 * queue and the ring code share (pm_gpu.c). One struct, so the close reads it from one base address. The hot fields come first. */
typedef struct NdBatch {
    unsigned char* gring;      /* the ring chunk in use: GRING_VTX x 16 bytes, linear */
    unsigned short* gidx;      /* its index ring: GRING_IDX, linear */
    unsigned gring_n, gidx_n, gi_start, gni; /* ring fill, index fill, first index of the open batch, indices in the open batch */
    unsigned gring_fl, gidx_fl; /* ring and index fill already flushed to memory (gpu_flush_written) */
    int gi_kind;               /* FB-1: kind of the open batch (0 ring, 1 static area) */
    int g_pal, g_use5, g_ct, g_cullm; /* the batch is a palette batch; program 5; the N64 cull type and the PICA mode made from it */
    int draw_3d, rect_2d, rect_z; /* the open batch is 3D; a 2D rectangle batch; the rectangle tests depth (X01) */
    unsigned pal_n, pal_gen;   /* palette entries in use; palette generation (a new batch) */
    unsigned pal_g[GPAL];      /* game matrix of each palette entry */
    float g_uvso[4], g_vp[4], g_cmul[4], g_cadd[4], g_zmix[2]; /* draw parameters of the open batch */
    int rect_open, rect_ec, rect_zb, rect_tex; /* DR1: a rectangle batch is open, its combine, depth flag and tile-0 cache slot (C7) */
    int rect_y0, rect_y1;      /* R13c: the rows the rectangles of the batch draw (the grab reads them) */
    float rect_pal_z;          /* RECT-1a: the depth of the rectangle batch, in pcomp[RECT_GM] */
    PmFx g_fx;                 /* GV5: light, fog and texgen block of a pm5 batch */
    RingChunk rchunk[RING_CHUNK_MAX]; /* the ring chunks (FB-1) */
    int rchunk_n, ring_cur;    /* chunks allocated; the chunk in use */
    unsigned ring_fr, ring_done; /* frame stamp of the ring; the last frame the GPU finished (W4) */
    int buf_cur;               /* RAW1: the buffer info citro3d holds (0 unknown, 1 CPU pool, 2 raw ring) */
} NdBatch;
extern NdBatch nb;
enum { OFF_N = 3, OFF_S = 4, OFF_P = OFF_N + OFF_S, OFF_ALL = OFF_P + 2 }; /* FX2a: slots OFF_P and OFF_P + 1 are the 8 bit images (flame), they keep their pixels from frame to frame like the N64 memory */ /* V05b: slots 0..OFF_N-1 are full size, the next OFF_S are 64 x 64. A slot keeps its size for good: a render target is never deleted in a frame. */ /* R5: OFF_N 4 to 3 (one full image, 512 KiB, back to the texture cache). OFF_S stays 4. */
/* V05: one off-screen colour image (a render target that a later draw samples) */
typedef struct { C3D_Tex tex; C3D_RenderTarget* rt; const void* base; int w, h, siz; unsigned stride; unsigned last; C3D_Tex snap; int snap_ready; int hdr, snap_hdr; } PmOffImg;
/* The render state of pm_gpu.c that the batch close, the setters, the rectangle queue and the ring share: one struct, so a draw reads it from one base address. */
typedef struct PmGpuSt {
    int gpu_ok, gpu_vtx_on, gv4_on; /* citro3d is up; the raw Vtx ring exists; the pm4 palette program is usable */
    int combine;               /* the combine mode of the open batch (8: the plan) */
    int z_on, r_zcmp, r_zupd, r_blend, r_atest, r_decal, r_nocol, r_mix, r_zmark; /* RDP render mode: depth on, compare, update, blend, alpha cut (-1 none), decal, depth only, mix, zmark (B3b) */
    float r_zmark_zn;          /* B3b: the constant clip depth (z / w) the zmark pass writes */
    unsigned env_col;          /* combine mode 5: the ENV colour */
    int off_active, off_cur;   /* V05: drawing goes to an off-screen image; its slot */
    int grab_dirty;            /* B2: the main target changed since the last grab */
    float vp_sx, vp_sy, vp_tx, vp_ty; /* the N64 viewport scale and translation */
    PmPlan plan;               /* S25: combine mode 8 */
    unsigned pcomp_gen[257];   /* post_gen of each palette product pcomp[gm] */
    PmOffImg offs[OFF_ALL];    /* the off-screen images */
} PmGpuSt;
extern PmGpuSt gs;
enum { GRING_VTX = 16384, GRING_IDX = 65536, GSTAT_VTX = 16384 }; /* DLC2: the static vertex area follows the ring in the same buffer (indices GRING_VTX and up) */
enum { RECT_GM = 256 }; /* RECT-1a: palette slot of a fill rectangle batch (screen pixels): pcomp[RECT_GM] = projection with the depth folded in, see pcomp_calc */
/* C8: the functions and the rectangle diagnostics that pm_gpu.c and pm_gpu_rect.c (ring, rectangle queue) share */
void pm_gpu_flush_from(const char* fn, int line); /* flush of the open batch, with the caller for the flush tracer */
void fl_rect_note(void); void fl_rect_done(void); /* PG0 tracer counts of texture rectangles */
void split_frame(void); void pm_gx_room(void);
void ring_frame_begin(void); void gi_kind_set(int k); int ring_need(unsigned nv, unsigned ni);
void rect_raw_open(int ec, float fz, const float uvso[4]); void rect_log(unsigned f);
void nd_tridrop_log(unsigned f); /* nd_geom.c */
extern int pm_wide;
typedef struct { int filt, ws, wt, cur, sx0, sy0, sx1, sy1, bl, rep, zc, zu, at, dec, noc, zon, mem, mix, comb, rgbs, pgen, mixc, prim, cc, env; } RectSig;
extern RectSig rsig_open; extern unsigned rsig_chk, rsig_diff; /* FB-1 RSIG (switch rectsig) */
void rect_sig_fill(RectSig* s); void rect_sig_check(const char* where);
#define RQ_MAX 64
extern int rd_tex, rq_n, rq_cur, rq_slot[RQ_MAX], rq_b[RQ_MAX]; extern short rq_st[RQ_MAX][4]; /* RECT diag: the queued rectangles of the batch */
extern unsigned dr1_merged, dr1_breaks;
