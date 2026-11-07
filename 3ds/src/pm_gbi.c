#include <stdio.h>
int pm_gpu_spr_native(const uint32_t* rec); /* SPR4: native sprite draw at the final vertex word (nd_walk.c G_VTX arm, pm_gpu.c) */
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

extern int pm_tex_tagi; /* STALE2 texsrc: bind path tag */
extern u16 DepthCopyBuffer[16];
extern unsigned short* nuGfxZBuffer;
void pm_depth_query(const void* timg, int x, int id); /* F03, pm_depth.c */

extern unsigned int __ctru_heap;
extern unsigned int __ctru_heap_size;
extern unsigned int __ctru_linear_heap;
extern unsigned int __ctru_linear_heap_size;

int rm_frame;


unsigned conv_k4; /* G_SETCONVERT K4 (8 bit part), the combiner input K4 */
unsigned keyc, keys; /* B6: G_SETKEYR/G_SETKEYGB chroma key CENTER, 0xRRGGBB00. Only the combiner input CENTER reads it. */
/* TBREP: tex_gen counts the writes of texture memory and texture state (the bind skip of tb_run is gone, S3F-4; the counter stays). */
unsigned tex_gen;
/* TST1: the tile, combine and texture-image state in one block, 32-byte aligned, in .data.pmhot. Types and meanings are unchanged. */
int ld_valid, ld_x, ld_y, ld_h, ld_w; /* last LoadTile window, in 8-bit texels of the source image */
/* V09: the source of each LoadTile by TMEM word. A tile 1 window that is set again later (chapter_change strips) still reads
 * the image that was loaded to its TMEM address, not the image of the load that came last. */
unsigned ldrec_n;
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
 * (gv_xf makes vert_p on demand). It replaces gpuvtx (PO3b) when both are on. */
u8 vert_litf[64]; /* GV5: bit 0 = loaded lit, bit 1 = loaded lit with texgen, bit 2 = the lights changed since (the colour was made then): the CPU path only */
u8 vert_gvf[64]; /* GV1: bit 0 = vert_p valid, bit 1 = the ring copy's S, T and colour read (gv_xf, GM-3). 3 for every vertex made. */
unsigned seam_obj, seam_clip; /* SEAM: CPU path triangles by position route, `seamstat` logs them */
unsigned gv_fb[GVR_N], gv_load_n, gv_tri_n, gv_rej_n, gv_xf_n;
unsigned gp1_ld_in, gp1_ld_out, gp1_ld_cross, gp1_vx_in, gp1_vx_out, gp1_vx_cross, gp1_subset;
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
int comb_set;
/* 1 when the alpha combiner reads shade alpha. DECAL and MODULATE modes do not, and imgfx
 * meshes leave cn[3] at 0, so passing it through made them vanish. */
int comb_alpha_shade = 1;





int plan_active; /* S25: the general combiner (mode 8) draws, not the legacy mode */
PmPlan cur_plan;
void sync_combine(void);
/* S3N-3d: set by nd_comb_decide. The combine work then makes no pm_gpu call, no stats and no log. */
int comb_dry;

/* sync_combine() is below, after the audit code it shares. */




/* L01: RSP light table. G_MW_NUMLIGHT sets the count, G_MOVEMEM index G_MV_LIGHT fills slots (slot 0 is light 1, slot nlights is the ambient light). */
NdLight lt[9];
unsigned lt_n;
s8 lk[2][3]; /* T01: look-at X and Y vectors (G_MOVEMEM slots -2 and -1) */

/* L01: RSP lighting. The normal goes through the model view matrix, then colour = ambient + sum(light colour * max(0, normal . direction)). Alpha stays the vertex alpha.
 * Debug switch nolight keeps the old fixed light for an A/B. */
int pm_nolight = -1; /* F11-10: -1 = read the switch at first use, the Options row sets 0 or 1 */




const float ident_m[4][4] = {
    { 1.f, 0.f, 0.f, 0.f },
    { 0.f, 1.f, 0.f, 0.f },
    { 0.f, 0.f, 1.f, 0.f },
    { 0.f, 0.f, 0.f, 1.f },
};


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






const char* const cc_nm_a[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "1", "NOISE" };
const char* const cc_nm_b[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "KEYC", "K4" };
const char* const cc_nm_c[16] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "KEYS", "CMBA", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "LODF", "PLODF", "K5" };
const char* const cc_nm_d[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "1", "0" };
const char* const cc_nm_aa[8] = { "CMBA", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "1", "0" };
const char* const cc_nm_ac[8] = { "LODF", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "PLODF", "0" };



unsigned st_lit, st_fogg, st_fogb, st_blc, st_tgen, st_tri; /* S25: draws that use lights, fog, blend colour, texgen */
unsigned mm_cnt[256];

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
/* S3N-3d: the walker's sticky state at a draw (ndmcheck compare only: the native draw takes these as inputs): the TEXTURE scale word, the geometry mode and the combine words. */
/* MDL-1b: the tile-1 part of the walker's sync for a record, after rec_aux: dual_bind when the plan reads tile 1. 1 when tile 1 ended bound. */
int pm_gbi_bind_tile1(void) { /* MDL-1b v2: the walker's pre-draw tile step (gv_epoch_enter); rec_aux already ran the record's combine and tile words */
    if (!plan_active || !cur_plan.uses_tex1) return 0;
    ensure_tile(nd_ws.use_t1 ? 1 : 0);
    dual_bind();
    return nd_ws.td[1].valid && pm_tex_aux_off_idx() < 0;
}


/* S25: a plan that reads TEXEL1. Tile 1 is loaded and kept at unit 1, tile 0 stays the current texture, and the
 * unit 1 coordinates are the first ones moved by the difference of the two tile windows. */
unsigned aux_key;


/* Clip positions, not object positions. The identity matrix leaves x,y,z,w
 * alone and the viewport places them. */
const float st_off[2] = { 0.f, 0.f }; /* H06b1 */

/* PO3b, PO3c: one triangle of the raw Vtx path. The caller checked that the three vertices have a ring slot, one matrix and one texture scale, and that
 * the triangle is plain (unlit, no texgen, no screen vertex, no fog). The colour and depth overrides of the CPU path become uniforms: ENV or the plan
 * constant replace the colour, alpha 255, the boost of the second cycle, and G_ZS_PRIM replaces the depth. Returns 0 when the path refuses. */
typedef NdGvP GvP; /* S3N-1: the type and the function are in nd_model.c (nd_gv_params) */
/* The draw parameters of a raw triangle from the state and the texture scale `tss` of its vertices. */
void pm_gbi_gv_at(unsigned tss, uint32_t geo, int tw, int th, GvP* o);





/* GV1: the id of the matrix of the next vertex load or static group in gm_tab: the combined matrix, and for a lit load also the model view matrix of the normal. -1 when the table is full. */





extern u8 gMapShapeData[], TextureHeap[], pm_world_entity_heap[], heap_generalHead[], heap_spriteHead[];
/* 1: the list is drawn (the caller skips the G_DL after the record); 0: the walker runs the old words */








/* S25: an opcode the interpreter does not run. Counted, logged at 1, 100, 10000 ... uses. */
unsigned op_cnt[256];


/* ---- RDP color combiner, one cycle, for work the display list does off screen ---- */






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


/* G1: ops with an empty handler (sync and no-op). The loop skips them before the prologue. */


/* G2: a state op that leaves the live state as it is. The loop skips it before the prologue. Ops that call sync_combine
 * also need sc_ok: a set sc_ok means that call would do nothing. */

/* CORE-2a-3: a skipped op (g2_same true) still gives the state the native keys read what its write would have: the B8 known bits of a
 * SETOTHERMODE mask and the prim LOD word (w0 low 16 bits, which g2_same does not compare). */

/* H1: rare paths of nd_walk_run. Cold and noinline, so the hot loop does not carry their code. */
/* TEXH: cold and per-tile parts of tex_op, moved out of line. The same operations run in the same order. */



/* M4: the texture bind list of a model (make_texture_gfx: SETTIMG SETTILE LOADTLUT, SETCOMBINE, TEXTURE, SETTIMG SETTILE LOADBLOCK SETTILE SETTILESIZE). tb_ops_generic runs it
 * word by word through tex_op, as before. tb_ops runs the same op functions in list order without the dispatch (tex_op wrapper, tex_op_impl switch, one tex_gen step per op) and
 * leaves out the combine sync after a SETCOMBINE that the TEXTURE word after it repeats: that sync would be overwritten by the next one with no draw between. */
/* M4 state-diff check, switch texdiff: every bind list runs twice from the same start state, once word by word and once fused. The texture, tile, combine and mode state of both
 * ends is compared field by field. A difference is one log line (list, field names). The game goes on with the word by word state. */
