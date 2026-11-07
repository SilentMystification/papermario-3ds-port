/* nd_int.h: the internal interface of the native walker modules (plan_s10_path D5, WK-1): the walk group (nd_walk.c), the rect
 * handlers (nd_rect.c) and the walker helpers of the former pm_gbi.c that they use. One header (D5). Sections per module, filled as the groups move. */
#ifndef ND_INT_H
#define ND_INT_H

/* pm_gbi_walk.h: the walker helpers (the former pm_gbi.c) that the handler modules use (CORE-3). nd_rect.c holds the G_TEXRECT and G_FILLRECT handlers. */

#include <stdint.h>
#include "ultra64.h"
#include "nd.h"

typedef struct { int p0, p1; float u0, u1; } FoldSeg;

extern unsigned tex_gen;

/* the image roles of img_role (the former pm_gbi.c) */
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
void t1_fold_tri(const unsigned ri[3], unsigned tss);
extern unsigned t1fold_n; /* t1_fold calls that changed the map, per rect_log line */
extern int ep_t1f;
int rect_to_image(int xl, int yl, int xh, int yh, unsigned tile, u32 hs, u32 hd);
int box_pattern_rect(int xl, int yl, int xh, int yh, u32 hs, u32 hd);

void nd_texrect(u32 w0, u32 w1, u32 hs, u32 hd); /* nd_rect.c: G_TEXRECT (3 slots) */
void nd_fillrect(u32 w0, u32 w1);                /* nd_rect.c: G_FILLRECT */
extern unsigned nd_rect_tex_n, nd_rect_fill_n;    /* nd_rect.c: calls of each handler */

typedef struct { unsigned draws, nat, fb, segn, segi, comp, dyn, why[12], ceq, cdiff, cseg; } EntCount;
typedef struct { unsigned lists, nat, dyn, ref, skip; } GdlCount;
typedef struct { const Gfx* p; unsigned s; } PkMark;

/* ---- W: enums and defines of the gather and walk paths (cut from the former pm_gbi.c) ---- */
enum { GVR_SCR, GVR_MIXED, GVR_TSS, GVR_LIT, GVR_FOG, GVR_ZMARK, GVR_TEX1, GVR_REFUSE, GVR_LITLOAD, GVR_RING, GVR_N };
enum { GP1_IN, GP1_OUT, GP1_CROSS };
#define GV_NONE 0xffffu
extern int pm_tex_tagi;
extern unsigned int __ctru_heap, __ctru_heap_size, __ctru_linear_heap, __ctru_linear_heap_size;
extern char __end__[];
int pm_gpu_spr_native(const uint32_t* rec);
/* ---- W: globals and prototypes of the walker helpers that stayed in the former pm_gbi.c (moved with the group) ---- */
const float (*pm_gbi_combined_ptr(void))[4];
int clip_bits4(const float* p);
int dlc_try(const Gfx* l);
int ent_draw(const Gfx* rec);
int gp1_box_class(const s16 lo[3], const s16 hi[3], const float (*m)[4], unsigned char* pbits);
#define gp_on() (pm_ph_on) /* PG0 and PERF-GBI counters: fed in prof mode only */
int gv_lit_ok(void);
int is_host(u32 p);
int lt_mem_same(u32 w0, u32 w1);
int pm_gpu_spr_native(const uint32_t* rec);
unsigned gv_matrix_id(int need_nm);
unsigned rec_run(const Gfx* q);
int pm_gbi_bind_tile1(void); /* MDL-1b: the walker's tile 1 step, the former pm_gbi.c */
unsigned sc_sig_now(void);
void draw_tri(unsigned w);
void gbi_cold_modifyvtx(u32 w0, u32 w1);
void gbi_movemem(u32 w0, u32 w1);
void gbi_moveword(u32 w0, u32 w1);
void gbi_popmtx(u32 w1);
void gbi_setcimg(u32 w0, u32 w1);
void gbi_setenv(u32 w1);
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

/* ---- W: variables of the former pm_gbi.c that the walk group uses (global now) ---- */
extern NdLight lt[9];
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
extern s16 vert_sx[64], vert_sy[64];
extern unsigned blendc;
extern unsigned char vert_cb[64];
extern unsigned cmb_gen_seen;
extern unsigned comb_gen;
extern unsigned conv_k4;
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

/* ---- W: the walk group's own entry points and tables (defined in nd_walk.c), and the the former pm_gbi.c helpers it calls ---- */
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
void op_setprimcolor(u32 w0, u32 w1);        /* ST-1: nd_state.c */
void op_setothermode(unsigned op, u32 w0, u32 w1); /* ST-1: nd_state.c */
void nd_sr_forget(void);                     /* ST-1: nd_state.c, sync_render state forgotten */
void gp_note(int k, int same);               /* PG0 counter, the former pm_gbi.c */
void lit_freeze(void);                       /* GV5, the former pm_gbi.c */
extern int sci_x0, sci_y0, sci_x1, sci_y1;   /* ST-1: nd_state.c, scissor of the walker */
void nd_light_count(u32 w1);                 /* ST-1: the former pm_gbi.c, G_MW_NUMLIGHT (lit_freeze, clamp 7) */
extern unsigned lt_n;                        /* the former pm_gbi.c, lights count (GM-1) */
extern unsigned fogc, blendc;                /* ST-1: nd_state.c */
extern int fog_fm, fog_fo, fogmix_on;        /* ST-1: nd_state.c, F08 fog factor */
int ent_gdl(const Gfx* l);
extern u8 gMapShapeData[];
extern int sc_ok, gp_prev_op, box_folded;


/* ---- T: texture group (plan_s10_path TX). TX-1 cut the setters to nd_tex.c; globals not yet moved stay in the former pm_gbi.c (extern here). ---- */
struct TszLast { const u8* src; unsigned fmt, siz; int w, h, stride; const void* pal; int pal_n; unsigned ep; int ox, oy; int ok; };
typedef struct { int valid; const u8* src; int stride, w, h, x0, y0; } M1Rec;
extern M1Rec m1;                 /* the former pm_gbi.c: the shaded sprite tile 1 raster */
extern struct TszLast tsz_last;  /* nd_tex_set.c */
extern unsigned st0_ep; extern int st0_ok; /* nd_tex_set.c */
extern unsigned blk_n;           /* nd_tex_set.c */
extern const void* tlut_pal[16]; /* nd_tex_set.c */
extern int ld_valid, ld_x, ld_y, ld_h, ld_w; /* the former pm_gbi.c: last LoadTile window */
extern int dual_on;              /* the former pm_gbi.c */
extern void* const zb_fake;      /* the former pm_gbi.c */
extern unsigned sp0_cur;         /* the former pm_gbi.c: split timer kind */
extern unsigned long long sp0_F; /* the former pm_gbi.c */
int pm_is_cfb(const void* p);
int pm_is_cur_cfb(const void* p);
void pm_cfb_capture_prev(void);
void gp_note(int k, int same);
void gp3_t0_set(void);
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

/* ---- M: the matrix group (nd_mtx.c, GM-1). Globals defined in nd_mtx.c; the former pm_gbi.c and the walker read them. ---- */
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
extern unsigned long long tx_t[256], tx_f[256]; extern unsigned tx_n[256]; /* owner: the former pm_gbi.c (sp0 report) */
extern const void* ens_img[2]; extern const void* ens_src; extern const void* ens_pal;
extern unsigned ens_fmt, ens_siz; extern unsigned ens_wrap[4]; extern int ens_sz[4];
void unpack8(unsigned c, float o[4]);
void pal_rgba(const void* base, unsigned i, float o[4]);
int tile_texel(int n, int s, int t, unsigned char o[4]);
int tile_bound(int n);
const u8* fb_prev_base(int n);
void ens_clear(void);
extern unsigned short** nuGfxCfb; extern unsigned nuGfxCfbNum;
/* owner: the former pm_gbi.c (LoadTile window; G_LOADTILE writes it in tex_op_impl (nd_tex.c) and the snapshot ts_load (the former pm_gbi.c) until TX-3 moves ts_*) */
extern unsigned ldrec_n;
void grab_cur(const void* src, int w, int h);       /* the former pm_gbi.c, target group */
void nd_native_rec(u32 w0);                         /* nd_target.c, LS-4: G_PM_ND records */
int load_shaded_sprite(int dx4, int dy4);           /* the former pm_gbi.c, TX-3 */
int glyph_atlas_bound(void);                        /* the former pm_gbi.c, TX-3 */
void op_setcombine(u32 w0, u32 w1);                 /* the former pm_gbi.c, combine group */
void op_setothermode(unsigned op, u32 w0, u32 w1);  /* the former pm_gbi.c, state group */
void op_setprimcolor(u32 w0, u32 w1);               /* the former pm_gbi.c, state group */
unsigned long long pm_gpu_fl_acc(void);             /* pm_gpu.c */
/* TX-3: the texture state snapshot (texdiff check, the former pm_gbi.c and nd_tex.c). Fields of the other groups as they are. */
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

/* TX-3 externs. owner: the former pm_gbi.c (combine and state groups, read by the snapshot in nd_tex.c until those groups move) */
extern int comb_mode5, plan_active, fog_fm, fog_fo, box_prod, box_pat, comb_alpha_shade, leg_mode;
extern PmPlan cur_plan; extern float dual_gain[3];
void cc_cycle1(const float* t0, const float* t1, float out[4]);
void ts_save(TexSnap* o);
void ts_load(const TexSnap* o);
void tb_ops(const Gfx* g);
/* ---- C (ST-2): the combine group in nd_comb.c ---- */
int const_rgb_tex_alpha(unsigned* out);
float cc_clamp(float v);
void op_setcombine(u32 w0, u32 w1);
void pm_gbi_sticky(unsigned* tss, unsigned* geo, unsigned* cw0, unsigned* cw1);


/* ---- K: the census group (nd_cen.c, WK-2). Cut from the former pm_gbi.c; the walk (pm_gbi_run) and the frame hooks read these. ---- */
#define PK_MARKS 1024 /* WCEN3: the all-marks table holds two frames of both display contexts */
#define PK_SLOTS 256
#define PK_PRE 253 /* main list ops before the first mark */
#define PK_BG 254 /* ops of a list that is not the main list */
#define PK_OTHER 255 /* a producer that found no free slot */
extern PkMark pmk[PK_MARKS], pmk_all[PK_MARKS];
extern int pmk_n, pmx_main, pmx_next, pmk_all_n;
extern unsigned pmx_cur;
extern unsigned os_nest, os_open, os_op;
extern int mdcm_next, mdcm_cur;
extern const Gfx* pre_w0;
extern const Gfx* pre_mk0;
extern int pre_mkn;
void os_close(void);
unsigned pk_slot(unsigned a);

#define MAIN_GFX_N ((int)(sizeof gDisplayContext->mainGfx / sizeof(Gfx)))
/* K: census readers of the former pm_gbi.c globals (defined in the former pm_gbi.c, read by nd_cen.c). */
extern unsigned op_cnt[256], n_splits, n_grabs, rec_n, sp0_grp, sp0_skip, gp_ntri, gp_frames, gp3_t0_pending, gp3_t0_dead;
extern int ep_open;
extern unsigned gp_same[8], gp_set[8];
extern u8 seen[256];
extern int n_cmd, n_in, n_cut, n_drop, n_out, n_keep, sample_x, have_sample, dlc_on;
extern char stats_line[96];
void ep_flush_stats(void);
void dlc_report(void);
/* WK-3 (nd_target.c): the colour image role and the z helpers, read by the former pm_gbi.c and the rect and state modules. */
float prim_ndc_z(void);
unsigned word_5551(unsigned rgba);
/* G: the geometry group (nd_geom.c, GM-2): G_VTX (nd_geom_vtx) and G_TRI1/2 (draw_tri), the triangle clip and cull helpers and the epoch state. */
void nd_geom_vtx(u32 w0, u32 w1, const Gfx* list);
void draw_tri(unsigned w);
void gv_params_calc(unsigned tss, NdGvP* o);
void pm_gbi_gv_at(unsigned tss, uint32_t geo, int tw, int th, NdGvP* o);
void lit_freeze(void);
int gv_lit_ok(void);
int clip_bits4(const float* p);
unsigned tri_flags(void);
void tri_stats_add(unsigned f, unsigned n);
void tri_stats(void);
void gv_xf(int sl);
int gv_fog_gpu_ok(void);
int gv_state_why(int* ct, int* both);
void gv_epoch_enter(void);
int gv_tri(const int* idx);
float cull_cross(const float* p1, const float* p2, const float* p3);
void ep_flush_stats(void);
extern int tri_prep_ok, lit_pending, ep_why, ep_ct, ep_cullboth;
extern unsigned gv_epoch, ep_osc, ep_fl, ep_n;
extern NdGvP ep_p;
/* G (GM-2): the gather and triangle counters, the PERF-GBI laps and the state counters that nd_geom.c reads (defined in the former pm_gbi.c). */
enum { GPL_A, GPL_ENSURE, GPL_DUAL, GPL_EARLY, GPL_SYNC, GPL_EMIT, GPL_OLD, GPL_N };
#define GP_START() do { if (gp_on()) gp_last = pm_ticks(); } while (0)
#define GP_LAP(id) do { if (gp_on()) { unsigned n_ = pm_ticks(); gp_acc[id] += n_ - gp_last; gp_last = n_; } } while (0)
extern unsigned gp_acc[8], gp_last;
extern unsigned gv_fb[GVR_N], gv_load_n, gv_tri_n;
extern unsigned tri_drop[2];
extern unsigned gp1_ld_in, gp1_ld_out, gp1_ld_cross, gp1_vx_in, gp1_vx_out, gp1_vx_cross;
extern unsigned st_lit, st_fogg, st_fogb, st_blc, st_tgen, st_tri, st_gpu_tri, dlc_dirty;
extern u8 vert_gvf[64];
extern int pm_nolight;
enum { DLR_OK, DLR_OP, DLR_SLOT, DLR_VTX, DLR_LONG, DLR_N };
typedef struct {
    unsigned frames, hit, compile, refused[DLR_N], full, lists, grp, skip, replay, i_near, i_lit, i_ring, i_mat, tri, opsrep, hashbad, exitbad, missing, slotfix, ringfail, srep, sfail, sfull, sfb, stri, batch, xgrp;
} DlcCnt; /* LS-1 counters of the dlc path (defined in the former pm_gbi.c) */
extern DlcCnt dlc_c;
extern unsigned ep_epoch, ep_pepoch, ep_ptss, ep_open_gen, ep_open_epoch, ep_open_tss, ep_open_gmi;
/* LS-1 (nd_shape.c): the dlc and entity types and macros (shared by the walk and the handler modules). */
typedef struct { u32 op0, op1, am, om; s16 lo[3], hi[3]; u16 nvtx, ntri, goff, ng, sn, nbat; u8 tsv_exit; u16* sd; u16* bat_q; u16* ends; } DlcGrp; /* C3: sd = indices then nbat batch end counts; bat_q[gop index before each batch] */
typedef struct { u32 addr, gen, hash, nops, exit_am, exit_om; u16 ngrp; u8 refused; DlcGrp* grp; u32 (*gop)[2]; NdObj o; } DlcEnt;
#define DLC_TAB 2048
#define DLC_MAXOPS 4096
#define DLC_GPU_GEOM(g) ((g) & (G_ZBUFFER | G_LIGHTING))
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
extern unsigned pm_dlc_gen, dlc_seen_gen, dbg_seg, dbg_dsg, ent_opc[256], ent_nent, ent_gen_seen, dlc_nent;
extern unsigned ent_sw[16], ent_gw[8];
extern DlcEnt* dlc_tab[DLC_TAB];
extern EntKey* ent_tab[ENT_TAB];
extern GdlCount gdl_c;
extern EntCount ent_c;
extern const Gfx dlc_end[1];
void dlc_reset(void);
unsigned dlc_gmi(void);
int dlc_seg(const unsigned short* idx, unsigned nt, unsigned gmi);
void ent_reset(void);
void ent_compile(EntKey* k, const Gfx* l0);
EntKey* ent_lookup(const Gfx* l, u32 vb);
EntKey* ent_find(const Gfx* l, u32 vb);
int ent_seg_native(const EntKey* k, const EntSeg* s);
void ent_log(void);
int ent_run(const Gfx* l, EntKey* k, u32 segid);
u32 ent_segkey(void);
int entchk_on(void);
int pm_gpu_grange_why(const float m[4][4], unsigned nidx, const unsigned short* idx);
/* LS-2 (nd_ent.c): the entity record case of G_PM_ENT (the walk calls it). */
int nd_ent_case(const Gfx* rec, u32 w0, u32 w1);
void pm_gbi_ent_texchk(const uint32_t* dl);
void pm_gbi_ent_segchk(const uint32_t* dl);
extern Gfx ent_flat_buf[];
extern NdEntSeg ent_sbuf[];
extern u32 dlc_words[];
uint8_t* nd_env_valloc(unsigned n, unsigned* first);
uint16_t* nd_env_ialloc(unsigned n);
void pm_gpu_gidxs_commit(const unsigned short* p, unsigned n);

/* GLOB: prototypes and externs moved from the former pm_gbi.c. */
int pm_gpu_spr_native(const uint32_t* rec); /* SPR4: native sprite draw at the final vertex word (nd_walk.c G_VTX arm, pm_gpu.c) */
extern int pm_tex_tagi; /* STALE2 texsrc: bind path tag */
extern u16 DepthCopyBuffer[16];
extern unsigned short* nuGfxZBuffer;
void pm_depth_query(const void* timg, int x, int id); /* F03, pm_depth.c */
extern unsigned int __ctru_heap;
extern unsigned int __ctru_heap_size;
extern unsigned int __ctru_linear_heap;
extern unsigned int __ctru_linear_heap_size;
extern int pm_is_cfb(const void* p);
extern void pm_cfb_capture_prev(void);
extern void pm_cfb_capture_prev_cpu_for(const void* fill);
extern void pm_cfb_capture_prev_cpu(void);
extern void pm_zbuf_need(void);
extern unsigned pm_frame_count(void);
extern int pm_fb_native(const void* p);
extern u8 gMapShapeData[], TextureHeap[], pm_world_entity_heap[], heap_generalHead[], heap_spriteHead[];
extern unsigned short** nuGfxCfb;
extern unsigned nuGfxCfbNum;
extern int pm_wide;
#endif
