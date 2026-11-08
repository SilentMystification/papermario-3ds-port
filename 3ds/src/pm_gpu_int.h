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
#include "pm_gpu.h"
enum { GPAL = 21 }; /* GV1c: the most game matrices in the palette of one pm4 batch */
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
} NdBatch;
extern NdBatch nb;
