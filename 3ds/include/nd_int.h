/* nd_int.h: the internal interface of the native walker modules (plan_s10_path D5, WK-1): the walk group (nd_walk.c), the rect
 * handlers (nd_rect.c) and the walker helpers of pm_gbi.c that they use. One header (D5). Sections per module, filled as the groups move. */
#ifndef ND_INT_H
#define ND_INT_H

/* pm_gbi_walk.h: the walker helpers (pm_gbi.c) that the handler modules use (CORE-3). nd_rect.c holds the G_TEXRECT and G_FILLRECT handlers. */

#include <stdint.h>
#include "ultra64.h"
#include "nd.h"

typedef struct { int p0, p1; float u0, u1; } FoldSeg;

extern unsigned tex_gen;

/* the image roles of img_role (pm_gbi.c) */
enum { ROLE_MAIN, ROLE_DEPTHQ, ROLE_CPUFILL, ROLE_DEPTH, ROLE_CPUPAL, ROLE_SLOT };

extern u16 DepthCopyBuffer[16];
extern unsigned short* nuGfxZBuffer;
void pm_depth_query(const void* timg, int x, int id); /* F03, pm_depth.c */
void pm_cfb_capture_prev_cpu_for(const void* fill);
extern int cimg_stride, cimg_zero, zcol, zslot_on, sci_x0;

int img_role(int for_fill);
float depth_of_word(unsigned w16);
void off_screen_check(void);
float shift_mul(unsigned n);
void ensure_tile(int n);
void gp3_t0_surv(void);
void box_fold_sync(void);
void sync_render(void);
unsigned rgba_from_5551(unsigned c);
int fold_axis(int plo, float width, float a0, float a1, float mw, int flip, int negclamp, FoldSeg* out);
void dual_bind(void);
void t1_fold(float u0, float u1, float v0, float v1);
int rect_to_image(int xl, int yl, int xh, int yh, unsigned tile, u32 hs, u32 hd);
int box_pattern_rect(int xl, int yl, int xh, int yh, u32 hs, u32 hd);

void nd_texrect(u32 w0, u32 w1, u32 hs, u32 hd); /* nd_rect.c: G_TEXRECT (3 slots) */
void nd_fillrect(u32 w0, u32 w1);                /* nd_rect.c: G_FILLRECT */
extern unsigned nd_rect_tex_n, nd_rect_fill_n;    /* nd_rect.c: calls of each handler */

typedef struct { unsigned draws, nat, fb, segn, segi, comp, dyn, why[12], ceq, cdiff, cseg; } EntCount;
typedef struct { unsigned lists, nat, dyn, ref, skip; } GdlCount;
typedef struct { const Gfx* p; unsigned s; } PkMark;

/* ---- W: enums and defines of the gather and walk paths (cut from pm_gbi.c) ---- */
enum { GVR_SCR, GVR_MIXED, GVR_TSS, GVR_LIT, GVR_FOG, GVR_ZMARK, GVR_TEX1, GVR_DBG, GVR_REFUSE, GVR_LITLOAD, GVR_RING, GVR_N };
enum { GP1_IN, GP1_OUT, GP1_CROSS };
#define GV_NONE 0xffffu
extern int pm_tex_tagi;
extern unsigned int __ctru_heap, __ctru_heap_size, __ctru_linear_heap, __ctru_linear_heap_size;
extern char __end__[];
int pm_gpu_spr_native(const uint32_t* rec);
/* ---- W: globals and prototypes of the walker helpers that stayed in pm_gbi.c (moved with the group) ---- */
const float (*pm_gbi_combined_ptr(void))[4];
int clip_bits4(const float* p);
int dd_on(void);
int dlc_enabled(void);
int dlc_try(const Gfx* l);
int ent_draw(const Gfx* rec);
int gp1_box_class(const s16 lo[3], const s16 hi[3], const float (*m)[4], unsigned char* pbits);
int gp_on(void);
int gv_lit_ok(void);
int is_host(u32 p);
int lt_mem_same(u32 w0, u32 w1);
int pm_gpu_spr_native(const uint32_t* rec);
int rm_apply(u32 w1);
int xt_on(void);
unsigned gv_matrix_id(int need_nm);
unsigned rec_run(const Gfx* q);
unsigned sc_sig_now(void);
void dd_end(void);
void draw_tri(unsigned w);
void gbi_cold_dd_load(const Vtx* src);
void gbi_cold_modifyvtx(u32 w0, u32 w1);
void gbi_movemem(u32 w0, u32 w1);
void gbi_moveword(u32 w0, u32 w1);
void gbi_popmtx(u32 w1);
void gbi_setcimg(u32 w0, u32 w1);
void gbi_setenv(u32 w1);
void gbi_vtx_cpu1(const Vtx* s, unsigned k);
void gv_streak_flush(void);
void gv_streak_op(unsigned op);
void load_mtx(u32 w0, u32 w1, u32 hdr);
void mdc_op(const Gfx* at, int sp, unsigned op, u32 w0, u32 w1);
void mr0_walk_end(void);
void note(unsigned op);
void os_dlc_hit(void);
void os_op_hook(const Gfx* at, int sp, unsigned op);
void pm_gbi_ent_segchk(const uint32_t* dl);
void pm_gbi_ent_texchk(const uint32_t* dl);
void set_fillcolor(u32 w1);
void set_fogcolor(u32 w1);
void set_geom(u32 w0, u32 w1);
void set_primdepth(u32 w1);
void sync_combine(void);
void tb_inline(const Gfx* g, unsigned n);
void tex_op(u32 w0, u32 w1);
void tex_streak_flush(void);
void tex_streak_op(unsigned op);
void* ptr_of(u32 w);
extern const u8* src;
extern float vert_p[][4];
extern int box_folded;
extern int h;
extern int splash_on;
extern unsigned conv_k4;
extern unsigned s;

/* ---- W: variables of pm_gbi.c that the walk group uses (global now) ---- */
extern NdLight lt[9];
extern Vtx verts[64];
extern const Gfx dlc_end[1];
extern float combined[4][4];
extern float vert_p[64][4];
extern int comb_set;
extern int dlc_on;
extern int lit_pending;
extern int mr0_dlc_hit;
extern int rec_nat_last;
extern int rm_frame;
extern int splash_on;
extern int sr_ok;
extern int tri_prep_ok;
extern int tri_since_vtx;
extern u8 vert_gvf[64];
extern u8 vert_litf[64];
extern u8 vert_live[64];
extern u8 vert_scr[64];
extern unsigned blendc;
extern unsigned char aud_prim_lod;
extern unsigned char vert_cb[64];
extern unsigned cmb_gen_seen;
extern unsigned comb_gen;
extern unsigned conv_k4;
extern unsigned dd_dl0;
extern unsigned dd_dl1;
extern unsigned dd_idx;
extern unsigned dd_top;
extern unsigned ent_skipgen;
extern unsigned fogc;
extern unsigned gp1_ld_cross;
extern unsigned gp1_ld_in;
extern unsigned gp1_ld_out;
extern unsigned gp1_vx_cross;
extern unsigned gp1_vx_in;
extern unsigned gp1_vx_out;
extern unsigned gp_acc[8];
extern unsigned gp_opn[256];
extern unsigned gp_opt[256];
extern unsigned gp_opt_last;
extern unsigned gv_epoch;
extern unsigned gv_fb[];
extern unsigned gv_load_n;
extern unsigned keyc;
extern unsigned keys;
extern unsigned n_ops;
extern unsigned pk_nat[];
extern unsigned pk_rec[];
extern unsigned pm_gbi_cur_op;
extern unsigned pmx_cur;
extern unsigned sc_sig;
extern unsigned short vert_gmi[64];
extern unsigned short vert_ring[64];
extern unsigned st_vx;
extern unsigned vert_tss[64];

/* ---- W: the walk group's own entry points and tables (defined in nd_walk.c), and the pm_gbi.c helpers it calls ---- */
void nd_walk_run(void* list, unsigned nbytes);
extern const u8 g1_skip[256];
int host_range(u32 p, u32* lo, u32* hi);
int g2_same(unsigned op, u32 w0, u32 w1);
void g2_note(unsigned op, u32 w0);
void gbi_cold_bad_dl(u32 pc, u32 list, unsigned nbytes, int sp);
void gbi_cold_bad_next(u32 w1, u32 w0, u32 list);
void gbi_cold_bad_vtx(u32 w1, u32 w0, u32 list);
void gbi_cold_bad_mtx(u32 w0, u32 w1, Gfx* list, Gfx* pc);
void set_scissor(u32 w0, u32 w1);
void set_blendcolor(u32 w1);
void set_fogcolor(u32 w1);
void set_primdepth(u32 w1);
void set_fillcolor(u32 w1);
int ent_gdl(const Gfx* l);
int rmlog_on(void);
extern u8 gMapShapeData[];
extern int sc_ok, gp_prev_op, box_folded;


/* ---- T: texture group (plan_s10_path TX). TX-1 cut the setters to nd_tex.c; globals not yet moved stay in pm_gbi.c (extern here). ---- */
struct TszLast { const u8* src; unsigned fmt, siz; int w, h, stride; const void* pal; int pal_n; unsigned ep; int ox, oy; int ok; };
typedef struct { int valid; const u8* src; int stride, w, h, x0, y0; } M1Rec;
extern M1Rec m1;                 /* pm_gbi.c: the shaded sprite tile 1 raster */
extern struct TszLast tsz_last;  /* nd_tex.c */
extern unsigned st0_ep; extern int st0_ok; /* nd_tex.c */
extern unsigned blk_n;           /* nd_tex.c */
extern const void* tlut_pal[16]; /* nd_tex.c */
extern int ld_valid, ld_x, ld_y, ld_h, ld_w; /* pm_gbi.c: last LoadTile window */
extern int dual_on;              /* pm_gbi.c */
extern void* const zb_fake;      /* pm_gbi.c */
extern unsigned sp0_cur;         /* pm_gbi.c: split timer kind */
extern unsigned long long sp0_F; /* pm_gbi.c */
int pm_is_cfb(const void* p);
int pm_is_cur_cfb(const void* p);
void pm_cfb_capture_prev(void);
void gp_note(int k, int same);
void gp3_t0_set(void);
void suntile_t0(const u8* src, int stride, int w, int h, int bpp, int uls, int ult, int lrs, int lrt);
void tlut_v20(unsigned slot);
void tex_tile1(unsigned tile, int uls, int ult, int lrs, int lrt);
int tex_shaded0(int uls, int ult);
void tex_win_s25(int* p_uls, int* p_ult, int* p_lrs, int* p_lrt);
void op_settimg(u32 w0, u32 w1);
void op_settile(u32 w0, u32 w1);
void op_loadblock(u32 w0, u32 w1);
void op_loadtlut(u32 w0, u32 w1);
void op_settilesize(u32 w0, u32 w1);
void op_texture(u32 w0, u32 w1);

/* ---- M: the matrix group (nd_mtx.c, GM-1). Globals defined in nd_mtx.c; pm_gbi.c and the walker read them. ---- */
extern float vp_sx, vp_sy, vp_sz, vp_tx, vp_ty, vp_tz;
extern unsigned gm_last, gm_steps, gm_n;
extern float (*gm_tab)[4][4];
extern float (*gm_nrm)[3][4];
extern unsigned short gm_idx[512];
extern float mtx_cache[256][16];
extern u32 mtx_tag[256];
void upload_mvp(void);
void mtx_elem(Mtx* dst, int r, int c, float v);
void rec_lights(const Lightsn* L, unsigned k);
void lit_freeze(void);
extern s8 lk[2][3];
extern unsigned lt_n;

/* TX-2 (nd_tex.c). owner: nd_tex.c unless noted. */
extern unsigned long long tx_t[256], tx_f[256]; extern unsigned tx_n[256]; /* owner: pm_gbi.c (sp0 report) */
extern const void* ens_img[2]; extern const void* ens_src; extern const void* ens_pal;
extern unsigned ens_fmt, ens_siz; extern unsigned ens_wrap[4]; extern int ens_sz[4];
void unpack8(unsigned c, float o[4]);
void pal_rgba(const void* base, unsigned i, float o[4]);
int tile_texel(int n, int s, int t, unsigned char o[4]);
int tile_bound(int n);
const u8* fb_prev_base(int n);
void ens_clear(void);
extern unsigned short** nuGfxCfb; extern unsigned nuGfxCfbNum;
/* owner: pm_gbi.c (LoadTile window; G_LOADTILE writes it in tex_op_impl (nd_tex.c) and the snapshot ts_load (pm_gbi.c) until TX-3 moves ts_*) */
extern unsigned ldrec_n;
void grab_cur(const void* src, int w, int h);       /* pm_gbi.c, target group */
int load_shaded_sprite(int dx4, int dy4);           /* pm_gbi.c, TX-3 */
int glyph_atlas_bound(void);                        /* pm_gbi.c, TX-3 */
void op_setcombine(u32 w0, u32 w1);                 /* pm_gbi.c, combine group */
void op_setothermode(unsigned op, u32 w0, u32 w1);  /* pm_gbi.c, state group */
void op_setprimcolor(u32 w0, u32 w1);               /* pm_gbi.c, state group */
unsigned long long pm_gpu_fl_acc(void);             /* pm_gpu.c */
/* TX-3: the texture state snapshot (texdiff check, pm_gbi.c and nd_tex.c). Fields of the other groups as they are. */
typedef struct {
    __typeof__(nd_ws.tst1) tst; const void *timg, *tlut; int tlut_n; const void* tlut_pal[16]; __typeof__(nd_ws.td) td; int tile_x0, tile_y0; float tile_fx, tile_fy;
    int ld_valid, ld_x, ld_y, ld_h, ld_w; unsigned ldrec_n, blk_n; __typeof__(m1) m1; int dual_on, cur_tile, use_t1, tex_on; unsigned tex_ss, tex_ts; int comb_set;
    unsigned comb_w0, comb_w1; int box_folded, box_prod, box_pat, comb_alpha_shade; unsigned cmb_gen_seen; struct TszLast tsz; unsigned st0_ep; int st0_ok;
    unsigned omode_h, omode_l; int leg_mode, plan_active, comb_mode5; PmPlan plan; int sc_ok; unsigned sc_sig;
    unsigned fogc, blendc; int fog_fm, fog_fo; /* S3F-2: the colour and fog state the record writes */
    unsigned prim, envc;       /* S3F-2b: prim and env colour, the record writes both */
} TexSnap;
#define TS_FIELDS(X) X(tst) X(timg) X(tlut) X(tlut_n) X(tlut_pal) X(td) X(tile_x0) X(tile_y0) X(tile_fx) X(tile_fy) X(ld_valid) X(ld_x) X(ld_y) X(ld_h) X(ld_w) X(ldrec_n) X(blk_n) X(m1) \
    X(dual_on) X(cur_tile) X(use_t1) X(tex_on) X(tex_ss) X(tex_ts) X(comb_set) X(comb_w0) X(comb_w1) X(box_folded) X(box_prod) X(box_pat) X(comb_alpha_shade) X(cmb_gen_seen) X(tsz) \
    X(st0_ep) X(st0_ok) X(omode_h) X(omode_l) X(leg_mode) X(plan_active) X(comb_mode5) X(plan) X(sc_ok) X(sc_sig)     X(fogc) X(blendc) X(fog_fm) X(fog_fo) X(prim) X(envc)

/* TX-3 externs. owner: pm_gbi.c (combine and state groups, read by the snapshot in nd_tex.c until those groups move) */
extern int comb_mode5, plan_active, fog_fm, fog_fo, box_prod, box_pat, comb_alpha_shade, leg_mode;
extern PmPlan cur_plan; extern float dual_gain[3];
void cc_cycle1(const float* t0, const float* t1, float out[4]);
void ts_save(TexSnap* o);
void ts_load(const TexSnap* o);
void tb_ops(const Gfx* g);
#endif
