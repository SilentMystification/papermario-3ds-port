/* nd_walk.c: the replay side of the frame stream commands (DIRECT) and the op executor (nd_walk_state): the stream state runs, flat op ranges
 * (nd_nr_list) and lists (nd_list_run, nr_call) run on it. D10c: the list walker of the former pm_gbi.c (nd_walk_run: its pc loop, stack,
 * host range check and g1 skip table) is deleted. The op handlers live in nd_int.h's groups. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "nd_fs.h"
#include "pm_port.h"
#include "nd_cmd.h"
#include "variables.h"
#include "nd_psochk.h"
#include "nd_int.h"
#include "nd_ds.h" /* NST2D: nd_ds_rpl */
#include "nd_op.h"
#include "nd_api.h"
#include "nd_snap.h"
#include "nd_gtab.h"
#include "nd_lkey.h"
#include "nd_gbi.h" /* ND_W0_D2 of the fallback matrix unit */
#include "nd_nr.h"

/* GLOB: definitions moved here from the former pm_gbi.c (layout only). */
unsigned pm_gbi_cur_op; /* PI2: the unit id (ND_FS_*) of the step now running, or a record op G_PM_* (DEL1D1; one store per op) */

int pm_gpu_spr_native(const uint32_t* rec); /* SPR4 */


void nd_bad_next(u32 w1, u32 w0, u32 list) {
    static int bad; if (bad++ < 8) pm_log_e("bad dl target %08x w0 %08x list %08x", w1, w0, list);
}

void nd_bad_vtx(u32 w1, u32 w0, u32 list) {
    static int bad; if (bad++ < 8) pm_log_e("bad vtx %08x w0 %08x list %08x", w1, w0, list);
}

/* POSTFAIL2 (debug tool): a viewport unit whose address is not host memory (one Azahar run read vp 0x00000001, postfail1_trace.txt).
 * One line per case (4 at most): the unit, its neighbours, the stream chain and chunk that hold it, the stream command that runs (nd_stream_cur)
 * and the chain the recorder (core 1) writes now. A unit in the chain that core 1 writes is a recycled stream buffer. */
extern const uint32_t* nd_stream_cur;
int nd_rec_where(const void* p, unsigned* chunk);
int nd_rec_ctx(void);
extern s32 gCurrentDisplayContextIndex;
static __attribute__((cold, noinline)) void nd_bad_vp(const uint32_t* u) {
    static unsigned n;
    if (n++ >= 4u) return;
    const int pv = is_host((u32)(uintptr_t)(u - 4)), nx = is_host((u32)(uintptr_t)(u + 3));
    const uint32_t* s = nd_stream_cur;
    unsigned uc = 0, sc = 0;
    const int uk = nd_rec_where(u, &uc), sk = s ? nd_rec_where(s, &sc) : -1;
    pm_log_evt("POSTFAIL2 bad vp f%u rm %d: unit %p %08x %08x prev %08x %08x %08x %08x next %08x %08x; unit ctx %d chunk %u; cmd %p ctx %d chunk %u h %08x w1 %08x; rec ctx %d dctx %d",
        pm_frame_count(), rm_frame, (const void*)u, (unsigned)u[0], (unsigned)u[1],
        pv ? (unsigned)u[-4] : 0u, pv ? (unsigned)u[-3] : 0u, pv ? (unsigned)u[-2] : 0u, pv ? (unsigned)u[-1] : 0u,
        nx ? (unsigned)u[2] : 0u, nx ? (unsigned)u[3] : 0u, uk, uc, (const void*)s, sk, sc, s ? (unsigned)s[0] : 0u, s ? (unsigned)s[1] : 0u,
        nd_rec_ctx(), (int)gCurrentDisplayContextIndex);
}

void nd_bad_mtx(u32 w0, u32 w1, NdW2* list, NdW2* pc) {
    static unsigned bad; if (bad++ < 6) pm_log_w("gbi: matrix w0 %08x w1 %08x outside host memory, list %p at op %d, frame %d, previous ops %08x %08x %08x %08x", w0, w1, (void*)list, (int)(pc - list), rm_frame,
        pc > list ? pc[-1].words.w0 : 0, pc > list ? pc[-1].words.w1 : 0, pc > list + 1 ? pc[-2].words.w0 : 0, pc > list + 1 ? pc[-2].words.w1 : 0);
}

/* FX1-3 orbdbg (debug tool): a display list an effect registers (pm_gbi_dbg_reg, energy_orb_wave.c) gets a tag. DIRECT D10c: nr_call keeps the tag of the list it runs
 * (orb_tag, restored at its end). Under the orbdbg switch a tagged list runs on the op executor, not the ent or DLC path, so orb_log (pm_gpu.c)
 * sees its draws. Nothing is tagged without the switch. */
static const void* orb_lst[32];
static unsigned char orb_lst_tag[32];
static int orb_lst_n;
static unsigned orb_tag;
void pm_gbi_dbg_reg(const void* l, unsigned tag) {
    for (int i = 0; i < orb_lst_n; i++) if (orb_lst[i] == l) { orb_lst_tag[i] = (unsigned char)tag; return; }
    if (orb_lst_n < 32) {
        orb_lst[orb_lst_n] = l; orb_lst_tag[orb_lst_n] = (unsigned char)tag; orb_lst_n++;
        pm_walk_dbg_set();
        pm_log("orbdbg reg tag %u ptr %p", tag, l);
    }
}
static unsigned orb_tag_of(const void* l) {
    for (int i = 0; i < orb_lst_n; i++) if (orb_lst[i] == l) return orb_lst_tag[i];
    return 0;
}
unsigned pm_gbi_dbg_orb_tag(void) { return orb_tag; }
/* PROF2-5: the walker debug word (pm_port.h, nd.h WD_*). Set where its inputs change, read once per list by nd_walk_run: a clean frame tests one
 * register per op and calls nothing. The debug work runs in cold functions out of the hot span. */
unsigned pm_walk_dbg;
void pm_walk_dbg_set(void) {
    pm_walk_dbg = (pm_ph_cen ? WD_CEN : 0u) | (pm_ph_fine ? WD_FINE : 0u) | (pm_ph_on ? WD_PH : 0u) | (orb_lst_n ? WD_ORB : 0u);
}
/* The op prologue of a debug walk: the op count (PH_ADD C_CMD, opsplit sample), the streak and per-op timers of the phase census, the phasegbi timer
 * (returns its phase id, popped after the handler), the opsplit sample and the model census. */
/* PERF2 step 6: the op class of each op for the class timers (nd.h NDT_TEX..NDT_LIST); unlisted ops are list class */
static uint8_t walk_cls_tab[256];
static void walk_cls_init(void) {
    static const uint8_t tex[] = { ND_FS_TIMG, ND_FS_TILE, ND_FS_LOADTILE, ND_FS_LOADBLOCK, ND_FS_TILESIZE, ND_FS_LOADTLUT, ND_FS_TEXTURE };
    static const uint8_t comb[] = { ND_FS_COMB, ND_FS_ENV, ND_FS_PRIM, ND_FS_BLEND, ND_FS_FOG, ND_FS_OMH, ND_FS_OML, ND_FS_PRIMZ };
    static const uint8_t geo[] = { ND_FS_VTX, ND_FS_TRI1, ND_FS_TRI2, ND_FS_GEOM };
    static const uint8_t mtx[] = { ND_FS_MTX, ND_FS_POPMTX, ND_FS_MW, ND_FS_MOVEMEM };
    static const uint8_t rect[] = { ND_FS_RECT, ND_FS_FILL, ND_FS_SCISSOR };
    for (unsigned i = 0; i < 256u; i++) walk_cls_tab[i] = NDT_LIST;
    for (unsigned i = 0; i < sizeof tex; i++) walk_cls_tab[tex[i]] = NDT_TEX;
    for (unsigned i = 0; i < sizeof comb; i++) walk_cls_tab[comb[i]] = NDT_COMB;
    for (unsigned i = 0; i < sizeof geo; i++) walk_cls_tab[geo[i]] = NDT_GEO;
    for (unsigned i = 0; i < sizeof mtx; i++) walk_cls_tab[mtx[i]] = NDT_MTX;
    for (unsigned i = 0; i < sizeof rect; i++) walk_cls_tab[rect[i]] = NDT_RECT;
    walk_cls_tab[0] = NDT_TEX; /* the table is built: entry 0 (G_NOOP) holds a non-list class only as the init mark */
}
static __attribute__((cold, noinline)) int walk_dbg_op(unsigned dbg, const NdW2* at, int sp, unsigned op, u32 w0, u32 w1) {
    int fp = 0;
    if (dbg & WD_PH) { if (walk_cls_tab[0] != NDT_TEX) walk_cls_init(); pm_gpu_ndt_cls(op ? walk_cls_tab[op] : NDT_LIST); }
    n_ops++;
    if (dbg & WD_CEN) {
        gv_streak_op(op); tex_streak_op(op);
        unsigned t = pm_ticks();
        if (gp_prev_op >= 0) { gp_opt[gp_prev_op] += t - gp_opt_last; gp_opn[gp_prev_op]++; }
        gp_opt_last = t;
        gp_prev_op = (int)op;
    }
    if (dbg & WD_FINE) {
        /* only these three: the thousands of state ops stay in the gbi phase (a tick read costs 1.3 us in Azahar) */
        fp = op == ND_FS_VTX ? PH_GVTX : (op == ND_FS_TRI1 || op == ND_FS_TRI2) ? PH_GTRI : op == ND_FS_MTX ? PH_GMTX : 0;
        if (fp) pm_ph_push(fp);
    }
    (void)at; (void)sp; (void)w0; (void)w1; /* DELCEN: the op and model census are deleted */
    return fp;
}
static __attribute__((cold, noinline)) void walk_dbg_end(unsigned dbg) { /* the end of the outer list */
    if (dbg & WD_PH) pm_gpu_ndt_cls(-1); /* PERF2: the class span ends with the outer list */
    if (!(dbg & WD_CEN)) return;
    if (gp_prev_op >= 0) { gp_opt[gp_prev_op] += pm_ticks() - gp_opt_last; gp_opn[gp_prev_op]++; gp_prev_op = -1; }
}
/* The model record arm (G_PM_REC in a list, NDC_MODEL in the frame stream): the record words past the header. */
static unsigned rec_case(unsigned dbg, const NdW2* q) {
    NDT_BD(tr, dbg);
    unsigned n = rec_run(q);
    NDT_E(tr, NDT_REC);
    return n;
}
/* DIRECT D1: NDC_MODEL of the frame stream (nd_rec.c), with the walker's op prologue of G_PM_REC (gv_epoch, the census op hook). D10a: the
 * only model record path (no list holds a record). */
void nd_walk_rec_model(const NdW2* q) {
    const unsigned dbg = pm_walk_dbg;
    dlc_on = 1;
    pm_gbi_cur_op = G_PM_REC;
    gv_epoch++; /* GV1b, as the walker's prologue of G_PM_REC; NST2B */
    int fp = 0;
    if (__builtin_expect(dbg, 0)) fp = walk_dbg_op(dbg, q, 0, G_PM_REC, q->words.w0, q->words.w1);
    rec_case(dbg, q);
    if (__builtin_expect(fp, 0)) pm_ph_pop();
    if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1); /* the class span ends with the command */
}
static void spr_geo_walk(const uint32_t* g); static void ent_dl_walk(unsigned dbg, const NdW2* c); /* NAT5W */
/* DIRECT D2: NDC_SPR (nd_rec.c): the sprite record and its vertex and triangle words, at the point of its final draw (the walker state is the
 * draw state). Native, or else the walker loads and draws the two words, as the G_VTX arm. */
void nd_walk_spr(const NdW2* q) {
    const unsigned dbg = pm_walk_dbg;
    pm_gbi_cur_op = G_PM_SPR;
    int fp = 0;
    if (__builtin_expect(dbg, 0)) { fp = walk_dbg_op(dbg, q, 0, G_PM_SPR, q->words.w0, q->words.w1); }
    NDT_BD(ts, dbg);
    int sh = pm_gpu_spr_native((const uint32_t*)q);
    NDT_E(ts, NDT_SPR);
    if (__builtin_expect(fp, 0)) pm_ph_pop();
    if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1);
    if (sh) { extern unsigned char nd_snap_arm; nd_snap_arm = 0; /* NDR4: the native sprite took its snap */ return; }
    { extern int nd_spr_walk; nd_spr_walk = 1; spr_geo_walk((const uint32_t*)(q + G_PM_SPR_WORDS)); nd_spr_walk = 0; } /* WALLSHADE; NAT5W */
}
static __attribute__((cold, noinline)) void ent_lsr_note(void) { static unsigned char seen; if (!seen) { seen = 1; pm_log_evt("lsnap ent f%u", pm_frame_count()); } } /* NREC2 */
/* DIRECT D3: NDC_ENT (nd_rec.c): an entity record. A DRAW record that the native path refuses leaves its G_DL to the walker (ent_skipgen,
 * as the walker's G_PM_ENT arm and the G_DL after it). */
void nd_walk_ent(const NdW2* q, unsigned words) {
    const unsigned dbg = pm_walk_dbg;
    pm_gbi_cur_op = G_PM_ENT;
    const uint32_t* const lsb = (const uint32_t*)(uintptr_t)((const uint32_t*)q)[-1]; nd_lsr_begin(lsb); if (lsb) ent_lsr_note(); /* NREC2: the list snap table (nd_rec_ent_draw) */
    gv_epoch++; /* GV1b, as the walker's prologue of G_PM_ENT */
    int fp = 0;
    if (__builtin_expect(dbg, 0)) { fp = walk_dbg_op(dbg, q, 0, G_PM_ENT, q->words.w0, q->words.w1); }
    NDT_BD(te, dbg);
    const uint32_t* const et = words > G_PM_ENT_WORDS + 1u ? (const uint32_t*)(uintptr_t)((const uint32_t*)q)[2u * G_PM_ENT_WORDS + 3u] : NULL; /* NAT5A */
    int drawn = nd_ent_case(q, q->words.w0, q->words.w1, et);
    NDT_E(te, NDT_ENT);
    if (__builtin_expect(fp, 0)) pm_ph_pop();
    if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1);
    if (!drawn && words > G_PM_ENT_WORDS) ent_dl_walk(dbg, q + G_PM_ENT_WORDS); /* NAT5W */ /* D10c: its G_DL on the op executor (nr_call) */
    nd_lsr_end(); /* NREC2 */
}
/* DIRECT D10c: the list executor. A list (a G_DL target, a render mode list) runs on nd_walk_state from its first op to its G_ENDDL (no length:
 * ND_LIST_MAX bounds a list without one, as the walker's op guard). ex_depth counts the lists running now: a G_DL inside a list runs its
 * target nested (no list start or end rules, as the walker's G_DL push), at most ND_LIST_DEPTH deep (the walker's stack of 12; a deeper G_DL is
 * dropped as the walker dropped it). */
#define ND_LIST_MAX 200000u
#define ND_LIST_DEPTH 12
static int ex_depth;
static unsigned char alt_mode; static void ls_run(const uint32_t* q, unsigned n); /* NAT5F; NAT5N: 0 stream, 1 nd_nr_exec, 2 ls_run */
static __attribute__((cold, noinline)) void orb_enter(unsigned t) {
    if (t != orb_tag) pm_gpu_flush();
    orb_tag = t;
    pm_log("orbdbg enter f%u tag %u", pm_frame_count(), t);
}
/* DIRECT D5, D10c: a gSPDisplayList (push form, or a branch inside a list: nd_walk_state ends that list after it). As the walker's G_DL arm: a
 * static list goes to the entity path (ent_gdl) or a compiled map shape (nd_sg_try); else the list runs on the op executor with the sync state
 * kept: from a stream command as a list of its own (nd_nr_list), inside a list nested. */
static void nr_call(unsigned dbg, const NdW2* at, u32 w0, u32 w1, const uint32_t* ab) { /* NAT5N: ab NULL: the static table */
    NdW2* next = (NdW2*)ptr_of(w1);
    if (!next) return;
    if (!is_host((u32)next)) { nd_bad_next(w1, w0, (u32)(uintptr_t)at); return; }
    __builtin_prefetch(next);
    const NdNDev* nt = nd_ntab_find(next);
    const unsigned ot = orb_tag;
    int ow = 0; /* FX1-3: a tagged list under orbdbg runs on the executor (no ent or DLC path) */
    if (__builtin_expect(dbg & WD_ORB, 0) && PMDL("orbdbg")) { unsigned t = orb_tag_of(next); if (t) { orb_enter(t); ow = 1; } }
    int ring = 0; /* PBATCH1: a particle list (ent_gdl -1) runs on the op executor, not nd_sg_try */
    if (ent_skipgen) ent_skipgen = 0;
    else if (!ow && dlc_on && !(nt && !nt->ntri) && !((u32)next - (u32)gMapShapeData < 0x40000u)) {
        NDT_BD(te, dbg); pm_tex_tagi = 5; nd_ent_src = ab ? 2u : 1u; /* NATEF */ int eh = ent_gdl(next, ab ? (const uint32_t*)(uintptr_t)ab[3] : NULL); NDT_E(te, NDT_ENT);
        if (eh > 0) return;
        if (eh < 0) ring = 1;
    }
    if (!ow && dlc_on && !ring) {
        NDT_BD(td, dbg); int dh = nd_sg_try(next); NDT_E(td, NDT_DLC);
        if (dh) return;
    }
    if (ab) { const unsigned char m = alt_mode; /* NAT5N: the nested alt range, with the executor the static table took */
        if (!ex_depth) { PM_WALK_OWN(); gv_epoch++; dlc_on = 1; gp_prev_op = -1; pm_walk_side++; ex_depth = 1; alt_mode = 1u; nd_alt_run(ab); ex_depth = 0; pm_walk_side--; gv_streak_flush(); tex_streak_flush(); } /* nd_nr_list keep 1 */
        else if (ex_depth < ND_LIST_DEPTH) { ex_depth++; if (nt && nd_ls_on) nd_lsr_geo(); alt_mode = (nt && nd_ls_on) ? 2u : 1u; nd_alt_run(ab); ex_depth--; }
        alt_mode = m; }
    else if (!ex_depth) { if (nt) nd_nr_list(nt->u, nt->nu, 1); else nd_nr_list(next, ND_LIST_MAX, 1); } /* GKEY2: a list with an entry runs its unit copy */
    else if (ex_depth < ND_LIST_DEPTH) { ex_depth++; if (nt) nd_ntab_run(nt); else nd_nr_exec((const uint32_t*)next, ND_LIST_MAX); ex_depth--; }
    if (__builtin_expect(dbg & WD_ORB, 0) && orb_tag != ot) { pm_gpu_flush(); orb_tag = ot; } /* FX1-3 */
}
/* DIRECT D4b-3, D5: native record run (nd_rec.c): a run of ops recorded by the nd_gbi.h redirect. Each op runs as the walker runs it inside a
 * list: its skip test (typed ops: nd_unit_same, the light memory test of nr_geo), else the op prologue (pm_gbi_cur_op, gv_epoch but not for vertex, triangle, matrix and list ops) and the
 * same handler as the walker's arm. The recorder dropped the RDP syncs; the branch form of G_DL stays a list word.
 * D10b: also the op executor of a flat op range (nd_nr_list): an RDP sync or G_SPNOOP is skipped before the prologue, a texture rectangle
 * takes its two RDPHALF units, G_ENDDL ends the range (the flat ranges hold no branch). */
unsigned nd_epk_n, nd_epk_bad, nd_epk_mk; static unsigned char ep_mark, ep_stream; /* NAT4A: epchk counters, a mark is pending, a stream run (nd_nr_run) runs */ static uint32_t ep_mw; static unsigned epk_h[64][2], epk_nl; /* NAT4B */
/* NAT4B: one disagreement: the unit id, the direction (1: a mark and core 0 skips, 0: core 0 cuts with no mark), the mark reason word and the core 0 skip inputs. */
int pm_drec_now(void); extern int sc_ok; extern unsigned sc_sig, comb_gen, cmb_gen_seen; unsigned sc_sig_now(void); extern int box_folded;
static __attribute__((cold, noinline)) void epk_note(unsigned id, int same, const uint32_t* u) { /* NAT4B: one disagreement */
    const unsigned dir = same ? 1u : 0u; /* 0: core 0 cuts with no mark, 1: a mark and core 0 skips */
    epk_h[id & 63u][dir]++;
    { static unsigned fr = ~0u; if (pm_drec_now() && pm_frame_count() != fr) { fr = pm_frame_count(); pm_log_evt("epcut f%u", fr); } } /* NAT4C: one line per drec frame with a changed cut */
    if (epk_nl < 24u && pm_drec_now()) { epk_nl++; pm_log_evt("epbad f%u id %u dir %u mw %03x sc_ok %d sig %d gen %d fold %d a %06x b %08x", pm_frame_count(), id, dir, ep_mark ? (unsigned)ep_mw : 0u,
        sc_ok, sc_sig_now() == sc_sig, cmb_gen_seen == comb_gen, box_folded, (unsigned)(u[0] & 0xffffffu), (unsigned)u[1]); }
}
void nd_epk_log(void) { for (unsigned i = 0; i < 64u; i++) for (unsigned d = 0; d < 2u; d++) if (epk_h[i][d]) pm_log_evt("epbin id %u dir %u n %u", i, d, epk_h[i][d]); } /* NAT4B */
static unsigned char c6_mark, c6_on; unsigned nd_c6_bad; static __attribute__((cold, noinline)) void c6_note(void) { static unsigned fr = ~0u; nd_c6_bad++; if (pm_drec_now() && pm_frame_count() != fr) { fr = pm_frame_count(); pm_log_evt("c6cut f%u", fr); } } /* NAT6B */
/* HC4a: one typed scalar op (nd_op.h) with the walker's rules: the skip test, then the prologue (pm_gbi_cur_op, gv_epoch, the debug op hook)
 * and the setter. */
static void nr_set(unsigned dbg, const uint32_t* u, const void* at) {
    const int same = nd_unit_same(u); int cut = !same; /* NAT4C: the cut of this unit */ if (c6_on && !(ep_stream && ex_depth == 0)) { cut = c6_mark != 0; if (cut == same) c6_note(); } /* NAT6B: a copy unit cuts on its core 1 mark */
    { const unsigned i_ = u[0] >> 24; const int ca_ = i_ == ND_FS_OML || (i_ >= ND_FS_PRIM && i_ <= ND_FS_BLEND) || i_ == ND_FS_RARE || i_ == ND_FS_COMB || i_ == ND_FS_FILL || i_ == ND_FS_PRIMZ /* NAT4H */, cb_ = !ca_ && ((i_ >= ND_FS_OMH && i_ <= ND_FS_COMB) || (i_ >= ND_FS_TIMG && i_ <= ND_FS_LOADTLUT)); /* NAT4G all other scalar units */
      if (ep_stream && ex_depth == 0 && (ca_ || cb_)) {
        if (cb_) { nd_epk_n++; if ((same == 0) != (ep_mark != 0)) { nd_epk_bad++; epk_note(i_, same, u); } } /* NAT4F: count only OMH, TEXTURE (the combine class fields are stale on core 0 since NAT4D) */
        const int mk_ = ep_mark != 0; ep_mark = 0;
        if (ca_) return; /* NAT4K: the mark cut */
        cut = 0; /* NAT4K: the mark cut */ } } /* NAT4F: OMH and TEXTURE cut on the mark and still run when they change a field */
    if (same && !cut) return;
    const unsigned id = u[0] >> 24;
    pm_gbi_cur_op = id; /* DEL1D1: the unit id */
    if (cut) gv_epoch++; /* GV1b; NST2H source; NAT4C: the cut */
    int fp = 0;
    if (__builtin_expect(dbg, 0)) fp = walk_dbg_op(dbg, (const NdW2*)at, 0, id, u[0], u[1]);
    if (!same) nd_unit_apply(u); /* NAT4C */
    if (__builtin_expect(fp, 0)) pm_ph_pop();
}
/* HC4d: one typed geometry or matrix op (nd_op_geo) with the walker's rules: no skip test but the light memory one (R5B), the prologue
 * (pm_gbi_cur_op; gv_epoch only for G_MOVEMEM, as the old prologue), the handler. at: the op's place in its list (logs). */
static __attribute__((noinline)) void nr_geo(unsigned dbg, const uint32_t* u, const void* at) {
    const unsigned id = u[0] >> 24;
    const u32 a = u[0] & 0xffffffu, b = u[1];
    if (id == ND_FS_MOVEMEM) {
        if ((a & 0xffu) == G_MV_LIGHT && lt_mem_same(a, b)) return;
        gv_epoch++; /* GV1b; NST2H source */
    }
    pm_gbi_cur_op = id; /* DEL1D1: the unit id */
    int fp = 0;
    if (__builtin_expect(dbg, 0)) fp = walk_dbg_op(dbg, (const NdW2*)at, 0, id, u[0], u[1]);
    switch (id) {
    case ND_FS_VTX: nd_geom_vtx(a, b, (const NdW2*)at); break;
    case ND_FS_TRI1:
        tri_prep_ok = 0;
        draw_tri(a);
        break;
    case ND_FS_TRI2:
        tri_prep_ok = 0;
        draw_tri(a);
        draw_tri(b);
        break;
    case ND_FS_MTX:
        if (!is_host((u32)(uintptr_t)ptr_of(b))) nd_bad_mtx(a, b, (NdW2*)at, (NdW2*)at + 1);
        nd_xf_load(a, b, 0);
        break;
    case ND_FS_POPMTX: nd_xf_popb(b); break;
    default:
        if ((a & 0xffu) == G_MV_VIEWPORT && !is_host(b)) nd_bad_vp(u); /* POSTFAIL2 (debug tool) */
        nd_mv_mem(a, b);
        break;
    }
    if (__builtin_expect(fp, 0)) pm_ph_pop();
}
/* HC4d2: one control op (nd_op_ctl) with the walker's prologue (pm_gbi_cur_op; gv_epoch but not for a list call or G_MODIFYVTX; the debug
 * op hook), then its handler. u: the typed unit, at: its place in the run (a texture rectangle reads the b of the two units after it).
 * Returns the units it took after u (2 for a texture rectangle), or ~0u when a branch ends the run. */
static __attribute__((noinline)) unsigned nr_ctl(unsigned dbg, const uint32_t* u, const uint32_t* at, const uint32_t* e) {
    const unsigned id = u[0] >> 24;
    const u32 a = u[0] & 0xffffffu, b = u[1];
    const unsigned sub = a >> 16;
    /* DEL1D1: the unit id; a list mark keeps its record op (G_PM_LST, G_PM_ND) */
    const unsigned op = id != ND_FS_MISC ? id : sub == ND_FS_MISC_LST ? (unsigned)G_PM_LST : sub == ND_FS_MISC_ND ? (unsigned)G_PM_ND : (unsigned)ND_FS_MISC;
    unsigned r = 0;
    pm_gbi_cur_op = op;
    if (id != ND_FS_CALL && !(id == ND_FS_MISC && (sub == ND_FS_MISC_MODVTX || sub == ND_FS_MISC_EP)) && !(id == ND_FS_TXR && ep_stream && ex_depth == 0)) { gv_epoch++; } /* NAT5T: the mark cut a stream tile size */ /* GV1b; no cut for a list call or a vertex modify */
    int fp = 0;
    if (__builtin_expect(dbg, 0)) fp = walk_dbg_op(dbg, (const NdW2*)at, 0, op, u[0], u[1]);
    switch (id) {
    case ND_FS_CALL:
        nr_call(dbg, (const NdW2*)at, a & ~2u, (a & 2u) ? ((const uint32_t*)(uintptr_t)b)[0] : b, (a & 2u) ? (const uint32_t*)(uintptr_t)b + 1 : NULL); /* NAT5N */
        if (a & 1u) r = ~0u; /* D10c: a branch ends the list that holds it */
        break;
    case ND_FS_TXR: /* NAT5T */
        if (ep_stream && ex_depth == 0) { nd_epk_n++; ep_mark = 0; } /* NAT5T: the mark of nr_set */
        pm_gbi_cur_op = ND_FS_TILESIZE; nd_tx_tszr((const NdTxR*)(uintptr_t)b); break; /* NAT5T */
    case ND_FS_RECT:
        
        if (!(b & ND_FS_RECT_TEX)) nd_frect_draw(a, b);
        else if (at + 4 < e) { nd_trect_draw(a, b & 0x07ffffffu, at[3], at[5]); r = 2; } /* D10b: the op and its two RDPHALF units */
        break;
    default:
        switch (sub) {
        case ND_FS_MISC_LST: ent_skipgen = 0; gp_prev_op = -1; break; /* D11a: an in-line static state list (nd_rec.c rec_inline): its list start as nr_call and nd_nr_list */
        case ND_FS_MISC_ND: nd_native_rec((a >> 8) & 0xffu, a & 0xffu); break; /* D9: the native-only payload (the water splash bracket; PAUSEFADE: the low byte, the gutter fade 0..128, splash and grab hold pass 0 or 1) */
        case ND_FS_MISC_MODVTX: nd_modvtx((a >> 8) & 0xffu, a & 0xffu); break;
        case ND_FS_MISC_CIMG: nd_cimg_set(a, b); break;
        case ND_FS_MISC_EP: ep_mark = 1; ep_mw = b; nd_epk_mk++; gv_epoch++; break; /* NAT4B; NAT4K: the mark is the cut */
        default: break;
        }
        break;
    }
    if (__builtin_expect(fp, 0)) pm_ph_pop();
    return r;
}
/* An op with no typed form (not one the game writes): the walker's unknown op note. */
static __attribute__((cold, noinline)) void nr_unknown(unsigned dbg, const uint32_t* q) {
    const unsigned op = q[0] >> 24;
    pm_gbi_cur_op = op;
    gv_epoch++; /* GV1b */
    int fp = 0;
    if (__builtin_expect(dbg, 0)) fp = walk_dbg_op(dbg, (const NdW2*)q, 0, op, q[0], q[1]);
    note(op);
    if (__builtin_expect(fp, 0)) pm_ph_pop();
}
/* HC4d2: the op executor runs typed ops only. HC4e4: every list is typed; the RDP syncs and G_SPNOOP are the typed no-op (ND_NOP_A);
 * ND_FS_END ends the run. */
void nd_nr_exec(const uint32_t* q, unsigned n) { /* DEL1D2: was nd_walk_state */
    const unsigned dbg = pm_walk_dbg;
    for (const uint32_t* e = q + 2u * n; q < e; q += 2) {
        const uint32_t* u = q;
        unsigned id = q[0] >> 24;
        if (q[0] == ND_NOP_A) continue; /* HC4f3: a typed sync or no-op, skipped as a raw sync: no nr_ctl, no gv_epoch */
        if (__builtin_expect(!nd_op_typed(id), 0)) { nr_unknown(dbg, q); continue; } /* HC4e4: every list is typed */
        if (nd_op_geo(id)) nr_geo(dbg, u, q);
        else if (nd_op_ctl(id)) {
            const unsigned r = nr_ctl(dbg, u, q, e);
            if (r == ~0u) q = e - 2;
            else q += 2u * r;
        }
        else if (id == ND_FS_END) break;
        else nr_set(dbg, u, q);
    }
    if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1); /* the class span ends with the command */
}
void nd_nr_run(const uint32_t* rec) { const unsigned n = (rec[-2] >> 16) & 0xffu; if (alt_mode == 2u) ls_run(rec, n); else { ep_stream = alt_mode == 0u; nd_nr_exec(rec, n); ep_stream = 0; } } /* NAT4A; NAT5F; NAT5N */
void nd_geo_rec(const uint32_t* rec) { const unsigned dbg = pm_walk_dbg, n = (rec[-2] >> 16) & 0xffu, m = alt_mode; /* NAT5G: nr_geo per unit, as nd_nr_exec and ls_run */
    for (unsigned k = 0; k < n; k++) nr_geo(dbg, rec + 2u * k, rec + 2u * k);
    if (__builtin_expect(dbg & WD_PH, 0) && m != 2u) pm_gpu_ndt_cls(-1); }
void nd_ctl_rec(const uint32_t* rec) { const unsigned dbg = pm_walk_dbg, n = (rec[-2] >> 16) & 0xffu, m = alt_mode; const uint32_t* const e = rec + 2u * n; /* NAT5G: nr_ctl per unit */
    if (m != 2u) ep_stream = m == 0u; /* as nd_nr_run */
    for (const uint32_t* q = rec; q < e; q += 2) { if (q[0] == ND_NOP_A) continue; const unsigned r = nr_ctl(dbg, q, q, e); if (r == ~0u) break; q += 2u * r; }
    if (m != 2u) { ep_stream = 0; if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1); } }
void nd_txs_run(const uint32_t* r) { /* NAT6A: a stream texture unit with its mark: the mark (nr_ctl MISC_EP), then nr_set at ep_stream and ex_depth 0 */
    nd_epk_mk++; gv_epoch++; nd_epk_n++; ep_mark = 0; pm_gbi_cur_op = r[0] >> 24; nd_unit_apply(r); }
void nd_oml_run(const uint32_t* r) { const uint32_t m = nd_op_om_mask(r[0]); if ((r[1] & m) == (nd_ws.omode_l & m)) nd_ws.om_kl |= m; ep_mark = 0; } /* NAT6A: nd_unit_same OML, then nr_set returns */
void nd_prim_run(const uint32_t* r) { if (nd_ws.prim == r[1] && (uint8_t)nd_ws.prim_lod == (uint8_t)r[0] && sc_ok) { pm_gpu_set_prim(nd_ws.prim, r[0]); nd_ws.prim_lod = r[0]; } ep_mark = 0; } /* NAT6A: nd_unit_same PRIM */
void nd_epm_run(const uint32_t* r) { pm_gbi_cur_op = ND_FS_MISC; ep_mark = 1; ep_mw = r[1]; nd_epk_mk++; gv_epoch++; } /* NAT6C: nr_ctl MISC_EP */
void nd_cbd_run(const uint32_t* r) { (void)r; ep_mark = 0; } /* NAT6C: nr_set of a pure combine unit at ep_stream, ex_depth 0 */
void pm_tex_set_filter(int linear); /* pm_tex.c */ static void ls_omh(const uint32_t* q) { const uint32_t m = nd_op_om_mask(q[0] & 0xffffffu), tf = nd_ws.omode_h & (3u << 12); /* NCONV2D: set_om H (nd_set.c:84-91) without the sync */
    nd_ws.omode_h = (nd_ws.omode_h & ~m) | (q[1] & m); nd_ws.om_kh |= m; if ((nd_ws.omode_h & (3u << 12)) != tf) { pm_gpu_flush(); pm_tex_set_filter((nd_ws.omode_h & (3u << 12)) != 0); } }
static void ls_run(const uint32_t* q, unsigned n) { /* NCONV2D: the unit copy of a list with a snap table: nd_walk_state without the combine units */
    const unsigned dbg = pm_walk_dbg;
    for (const uint32_t* e = q + 2u * n; q < e; q += 2) { const unsigned id = q[0] >> 24;
        if (q[0] == ND_NOP_A || id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_BLEND) || id == ND_FS_RARE || id == ND_FS_COMB) continue;
        if (id == ND_FS_OMH) ls_omh(q); else if (id == ND_FS_TEXTURE) nd_tx_texon(q[0] & 0xffffffu, q[1]); else if (id == ND_FS_GEOM) { if (!nd_unit_same(q)) nd_unit_apply(q); }
        else if (__builtin_expect(!nd_op_typed(id), 0)) nr_unknown(dbg, q); else if (nd_op_geo(id)) nr_geo(dbg, q, q);
        else if (nd_op_ctl(id)) { const unsigned r = nr_ctl(dbg, q, q, e); if (r == ~0u) q = e - 2; else q += 2u * r; }
        else if (id == ND_FS_END) break; else nr_set(dbg, q, q);
    } }
void nd_cpy_run(const uint32_t* rec) { const unsigned dbg = pm_walk_dbg, n = (rec[-2] >> 16) & 0xffu, m = alt_mode; const uint32_t* const e = rec + 2u * n; /* NAT6B: a copy state record, a slot before each unit */
    if (m != 2u) ep_stream = m == 0u; /* as nd_nr_run */
    for (const uint32_t* q = rec; q < e; q += 2) { if (q[0] == ND_C6_MARK || q[0] == ND_NOP_A) { c6_mark = q[0] == ND_C6_MARK; continue; }
        c6_on = 1; if (m == 2u) ls_run(q, 1u); else nr_set(dbg, q, q); c6_on = 0; c6_mark = 0; }
    if (m != 2u) { ep_stream = 0; if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1); } }

/* DIRECT D10b: a flat op range (no branch, no nested list: the sprite fallback geometry, the pieces of a flattened entity list, the R13 words
 * of a model record, an interpreted map shape group) on the op executor (nd_walk_state), with the list start and end rules of nd_walk_run:
 * gv_epoch, dlc_on, a full combine and render sync unless keep (PERF2: a piece of one list), the vertex and texture streak flush. */
static __attribute__((cold, noinline)) void ops_cen_end(void) {
    if (pm_ph_cen) walk_dbg_end(pm_walk_dbg);
}
/* DEL1A: was nd_nr_list */
void nd_nr_list(const void* l, unsigned n, int keep) {
    PM_WALK_OWN(); /* CORE2-3a: a run is walker-side only */
    const int d = ex_depth, cen = __builtin_expect(pm_walk_dbg & WD_CEN, 0) && d == 0;
    gv_epoch++; /* GV1b */
    dlc_on = 1;
    gp_prev_op = -1; /* PG4: the time between two lists is not an op cost */
    if (!keep) { sc_ok = 0; sr_ok = 0; } /* DIP2: a new list starts with a full sync (not a piece of one flattened list) */
    pm_walk_side++;
    ex_depth = d + 1;
    nd_nr_exec((const uint32_t*)l, n);
    ex_depth = d;
    pm_walk_side--;
    gv_streak_flush();
    tex_streak_flush();
    if (cen) ops_cen_end();
}
/* NAT5W: the sprite fallback words VTX(4, 0) and TRI2 (sprite.c:314-317): the list start and end of nd_nr_list (keep 1), the arms of nr_geo. */
static void spr_geo_walk(const uint32_t* g) {
    PM_WALK_OWN(); const int d = ex_depth; gv_epoch++; dlc_on = 1; gp_prev_op = -1; pm_walk_side++; ex_depth = d + 1;
    pm_gbi_cur_op = ND_FS_VTX; nd_geom_vtx(g[0] & 0xffffffu, g[1], (const NdW2*)g);
    pm_gbi_cur_op = ND_FS_TRI2; tri_prep_ok = 0; draw_tri(g[2] & 0xffffffu); draw_tri(g[3]);
    ex_depth = d; pm_walk_side--; gv_streak_flush(); tex_streak_flush(); }
/* NAT5W: the G_DL unit of a refused entity record: the list start and end of nd_nr_list (keep 1), the CALL arm of nr_ctl. */
static void ent_dl_walk(unsigned dbg, const NdW2* c) {
    PM_WALK_OWN(); const int d = ex_depth; gv_epoch++; dlc_on = 1; gp_prev_op = -1; pm_walk_side++; ex_depth = d + 1;
    const uint32_t* u_ = (const uint32_t*)c; pm_gbi_cur_op = ND_FS_CALL; nr_call(dbg, c, u_[0] & 0xffffffu, u_[1], NULL); /* NAT5W; NAT5N */
    ex_depth = d; pm_walk_side--; gv_streak_flush(); tex_streak_flush(); }
/* NAT5A: the units of a cut raw alt range ab (nd_alt_eput), with the list start and end of nd_nr_list (keep 1). NATEF */
void nd_ent_slice(const uint32_t* ab) {
    PM_WALK_OWN();
    const int d = ex_depth, cen = __builtin_expect(pm_walk_dbg & WD_CEN, 0) && d == 0;
    gv_epoch++; dlc_on = 1; gp_prev_op = -1;
    pm_walk_side++; ex_depth = d + 1;
    const unsigned char m = alt_mode; alt_mode = 1u; nd_alt_run(ab); alt_mode = m;
    ex_depth = d; pm_walk_side--;
    gv_streak_flush(); tex_streak_flush();
    if (cen) ops_cen_end();
}
/* NAT5E: a particle run is records in the stream (nd_parts_end, nd_rec.c): the begin record, per instance its units (the list drawn
 * natively by nd_part_draw), the end record. The units are the stream units of core 0 (nd_rec_nr). */
static unsigned pr_v, pr_next = 300u;
static __attribute__((cold, noinline)) void pr_cen(void) {
    const unsigned f = pm_frame_count();
    pr_next = (f / 300u + 1u) * 300u;
    pm_log_evt("partsrun f%u v %u", f, pr_v);
}
void nd_pbeg_run(const uint32_t* r) { (void)r; PM_WALK_OWN(); pr_v++; if (__builtin_expect(pm_frame_count() >= pr_next, 0)) pr_cen(); /* NAT5E: the start of the old parts runner */
    pm_walk_side++; gv_epoch++; dlc_on = 1; gp_prev_op = -1; extern int pm_comb_vprim; pm_comb_vprim = 1; sc_ok = 0; }
void nd_pdraw_run(const uint32_t* r) { const NdNDev* t = (const NdNDev*)(uintptr_t)r[0]; if (!nd_part_draw(t)) { if (nd_ls_on) nd_lsr_geo(); nd_geo_units(t->u, t->nu); } } /* NAT5E: one instance list */ /* NATFB2: a refused particle list on the native geometry run */
void nd_pend_run(const uint32_t* r) { (void)r; extern int pm_comb_vprim; pm_comb_vprim = 0; sc_ok = 0; nd_comb_sync(); pm_walk_side--; gv_streak_flush(); tex_streak_flush(); } /* NAT5E */
/* ENTRYCAP: the device switch entcap (PMDL). A static list without a v2 table logs its walker entry description (nd_walker_desc) once per
 * distinct key: FNV-1a 32 over the list address, the call site (nd_rec_dl's return address) and the 44 bytes of the description. 4096 keys. */
static uint32_t entcap_fnv(uint32_t h, const void* p, unsigned n) {
    const uint8_t* b = (const uint8_t*)p;
    for (unsigned i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
    return h;
}
static __attribute__((cold, noinline)) void entcap_rec(const void* list, uint32_t site, uint32_t ent) { /* a static list call with no stated entry */
    static uint32_t keys[4096];
    static unsigned nkeys, over;
    void nd_walker_desc(NdPsoDesc* d);
    if (!PMDL("entcapt") && nd_ntab_find(list)) return; /* a v2 table: not a list without one (ENTSRC: the switch entcapt logs it too) */
    if (ent) return; /* FXENT2: a call with a stated entry (nd_draw_list with an ND_PSO_DESC); ND_SELF and nd_rec_dl calls are logged */
    NdPsoDesc d;
    nd_walker_desc(&d);
    d.geom = 0; d.key_c = 0; d.key_s = 0; d.plod = 0; /* FXREST2: draw values, not entry state */
    const uint32_t la = (uint32_t)(uintptr_t)list;
    uint32_t h = entcap_fnv(2166136261u, &la, 4u);
    h = entcap_fnv(h, &site, 4u);
    h = entcap_fnv(h, &d, sizeof d);
    for (unsigned i = 0; i < nkeys; i++) if (keys[i] == h) return;
    if (nkeys == 4096u) { if (!over) { over = 1; pm_log_evt("entcap over"); } return; }
    keys[nkeys++] = h;
    const uint32_t* w = (const uint32_t*)&d;
    pm_log_evt("entcap l %08x s %08x d %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x", la, site, w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7], w[8], w[9], w[10]);
}
/* S7RUN (nd_gtab.h nd_rec_dl): the core 0 runner of one static list call from its table. rec[0] = the table, rec[1] = the state-only flag.
 * Arm rule (the commit names it): a state-only list (nd_rec.c rec_inline: fewer than 64 units, all state ops) never reached the walker's G_DL arms,
 * so it runs as G_PM_LST and its ops (the list start of that unit: gv_epoch, ent_skipgen, gp_prev_op); any other list takes the arms of nr_call
 * in its order (ent_skipgen, ent_gdl, nd_sg_try) and, when neither draws it, the list start of nd_nr_list (keep) and its table ops. A def whose value
 * units all precede the statics (nv == pre, the S6E rule of ent_gap) runs compiled, else as units. NAT5T: a debug walk takes the same arms (its
 * unit runs keep the census hooks). Vertex and triangle ops are the walker's typed units (nr_geo), so clip, cull, lit loads and the
 * ring stay the walker's. */
unsigned nd_gtr_ent, nd_gtr_dlc, nd_gtr_ops, nd_gtr_ntab;
/* CONV2D2: the words of a v2 list (nd_gtab.h NdNDev) run with nd_walk_state: NOPs skipped there, the stop at n. */
void nd_ntab_run(const NdNDev* e) { if (nd_ls_on) { nd_lsr_geo(); ls_run(e->u, e->nu); } else nd_nr_exec(e->u, e->nu); } /* NCONV2D: a list with a snap table runs without its combine units */
static void gt_body(const NdNDev* nt, const uint32_t* ab) { nd_gtr_ntab++; if (!ab) { nd_ntab_run(nt); return; } if (nd_ls_on) nd_lsr_geo(); const unsigned char m = alt_mode; alt_mode = nd_ls_on ? 2u : 1u; nd_alt_run(ab); alt_mode = m; } /* NAT5T: ab the alt range bounds, NULL: the static table */ /* NAT5F: as nd_ntab_run, from the stream copy; NAT5N */
static __attribute__((cold, noinline)) void entcap_so(const void* l, uint32_t site) { static uint32_t k[256]; static unsigned n;
    const uint32_t a = (uint32_t)(uintptr_t)l; for (unsigned i = 0; i < n; i++) if (k[i] == (a ^ site * 2654435761u)) return; if (n == 256u) return;
    k[n++] = a ^ site * 2654435761u; pm_log_evt("entcap so l %08x s %08x", a, site); }
static void gt_run0(const void* key, const NdNDev* nt, const uint32_t* rec, const uint32_t* ab);
static void gt_run(const void* key, const NdNDev* nt, const uint32_t* rec, const uint32_t* ab) { nd_lsr_begin((rec[1] & 2u) ? (const uint32_t*)(uintptr_t)rec[4] : NULL); gt_run0(key, nt, rec, ab); nd_lsr_end(); } /* NAT5T: ab */ /* NCONV1D1: the snap table of a drawing list (rec[1] bit 1) */
static void gt_run0(const void* key, const NdNDev* nt, const uint32_t* rec, const uint32_t* ab) {
    PM_WALK_OWN();
    if (rec[1] & 1u) { /* state-only */
        if (__builtin_expect(PMDL("entcap"), 0)) entcap_so(key, rec[2]);
        pm_walk_side++;
        pm_gbi_cur_op = G_PM_LST; gv_epoch++; ent_skipgen = 0; gp_prev_op = -1; /* NAT5W: the MISC_LST unit (nr_ctl) */
        gt_body(nt, ab);
        pm_walk_side--;
        return;
    }
    if (__builtin_expect(PMDL("entcap") && (!nt || PMDL("entcapt")), 0)) entcap_rec(key, rec[2], rec[3]); /* ENTSRC: entcapt adds the lists with a v2 table */
    const NdW2* next = (const NdW2*)key; /* NAT5T: a debug walk (pm_walk_dbg) takes these arms too: the static table has no tile window */
    pm_gbi_cur_op = ND_FS_CALL;
    int ring = 0;
    if (ent_skipgen) ent_skipgen = 0;
    else if (dlc_on && !(nt && !nt->ntri) && !((u32)next - (u32)gMapShapeData < 0x40000u)) {
        nd_ent_src = 3u; /* NATEF */
        const int eh = ent_gdl(next, ab ? (const uint32_t*)(uintptr_t)ab[-1] : NULL); /* NAT5A; NAT5T: rec[5], rec[9] 0 for a device record */
        if (eh > 0) { nd_gtr_ent++; return; }
        if (eh < 0) ring = 1;
    }
    if (dlc_on && !ring && nd_sg_try(next)) { nd_gtr_dlc++; return; }
    nd_gtr_ops++;
    const int d = ex_depth;
    gv_epoch++; /* GV1b */
    dlc_on = 1;
    gp_prev_op = -1;
    pm_walk_side++;
    ex_depth = d + 1;
    gt_body(nt, ab);
    ex_depth = d;
    pm_walk_side--;
    gv_streak_flush();
    tex_streak_flush();
}
unsigned nd_gtr_rec; /* GKEY2: the runs of nd_ntab_rec */
void nd_ntab_rec(const uint32_t* rec) { const NdNDev* nt = (const NdNDev*)(uintptr_t)rec[0]; nd_gtr_rec++; gt_run(nt->list, nt, rec, (rec[1] & 4u) ? rec + 6 : NULL); }
void nd_ntab_rec_e(const uint32_t* rec) { nd_psochk_draw((const NdPsoDesc*)(uintptr_t)rec[3]); nd_ntab_rec(rec); }
void nd_ndev_rec(const uint32_t* rec) { NdNDev d; nd_ndev_unpack(&d, rec); nd_gtr_rec++; gt_run(d.list, &d, rec, rec + 10); } /* NTABFIND: the record-time copy (nd_gtab.h) */
/* DIRECT D10a: a custom gfx range of the frame stream (model record R11, build_custom_gfx), run to its NDC_END as the walker ran the list it
 * replaces: the list start (gv_epoch, dlc_on; the caller resets the skip state: nd_recs.c rec_range, NATRS) and the list end (the vertex and texture streak flush). */
void nd_range_run(const uint32_t* a) {
    PM_WALK_OWN();
    const int d = ex_depth;
    ex_depth = 0; /* the stream level: a G_DL of a state run is a list of its own */
    gv_epoch++; /* GV1b */
    dlc_on = 1;
    gp_prev_op = -1;
    pm_walk_side++;
    nd_ds_rpl++; /* NST2D */
    nd_stream_run(a);
    nd_ds_rpl--;
    ex_depth = d;
    pm_walk_side--;
    if (__builtin_expect(pm_walk_dbg, 0)) walk_dbg_end(pm_walk_dbg);
    gv_streak_flush();
    tex_streak_flush();
}
void nd_mls_run(const uint32_t* r) { (void)r; PM_WALK_OWN(); gv_epoch++; dlc_on = 1; gp_prev_op = -1; sc_ok = 0; sr_ok = 0; pm_walk_side++; ex_depth++; } /* NAT5B: nd_nr_list keep 0 start */
void nd_mle_run(const uint32_t* r) { (void)r; ex_depth--; pm_walk_side--; gv_streak_flush(); tex_streak_flush(); if (__builtin_expect(pm_walk_dbg & WD_CEN, 0) && ex_depth == 0) ops_cen_end(); } /* NAT5B: its end */
void nd_alt_run_x(const uint32_t* ab) { const unsigned char m = alt_mode; alt_mode = 1u; nd_alt_run(ab); alt_mode = m; } /* NAT5T: the fallback range of a record, as the static table ran it */
void nd_mdl_alt(const uint32_t* ab) { const unsigned char m = alt_mode; alt_mode = 1u; nd_alt_run(ab); alt_mode = m; } /* NAT5B: a model alt range, as rec_top ran it */
/* ---- MV-7: walk core, cut from the former pm_gbi.c (no change): host range, pointer resolve, init, cold modifyvtx, lt compare, run entry, fallback ---- */
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

static inline void* ptr_res(u32 w);
void* ptr_of(u32 w) { return ptr_res(w); }
static inline void* ptr_res(u32 w) {
    if (!w) return NULL;
    if (is_host(w)) return (void*)w;
    unsigned seg = (w >> 24) & 0xff;
    if (seg < 16 && nd_ws.segbase[seg]) return (void*)(nd_ws.segbase[seg] + (w & 0x00ffffffu));
    /* SW1: an N64 K0 or K1 address (OS_PHYSICAL_TO_K0 of a static buffer: the camera projection matrix of the credits parade) names host memory with the top bits set */
    if ((w & 0xc0000000u) == 0x80000000u && is_host(w & 0x1fffffffu)) return (void*)(w & 0x1fffffffu);
    return (void*)w;
}
void* pm_gbi_ptr_of(u32 w) { return ptr_of(w); } /* MDL-1: the walker's segment resolver, for the R13 texture block (nd_model.c nd_r13_desc) */
void pm_gbi_init(void) {
    if (pm_c2own < 0) pm_c2own = pm_debug_has("c2own"); /* CORE2-3a: ownership check (nd_wq.c) */
    nd_mtx_reset();
    memset(vert_live, 0, sizeof vert_live);
    nd_ws.geom = G_SHADE | G_SHADING_SMOOTH;
    upload_mvp();
}
/* R5B: a G_MOVEMEM light that writes the values already in lt[slot] changes nothing. The same op also runs lit_freeze, which
 * acts when a lit vertex is pending, so the skip needs lit_pending == 0 (then lit_freeze returns at once). */
int lt_mem_same(u32 w0, u32 w1) {
    int slot = (int)(((w0 >> 8) & 0xff) * 8 / 24) - 2;
    const u8* l = (const u8*)ptr_of(w1);
    return l && slot >= 0 && slot < 9 && !lit_pending && lt[slot].col[0] == l[0] && lt[slot].col[1] == l[1] && lt[slot].col[2] == l[2] &&
           lt[slot].dir[0] == (s8)l[8] && lt[slot].dir[1] == (s8)l[9] && lt[slot].dir[2] == (s8)l[10];
}
void nd_modvtx(unsigned where, unsigned vi) { /* B5 (HC4d2: typed ND_FS_MISC fields). Only G_MWO_POINT_XYSCREEN is used by the game (underwater.c) */
    /* the screen position (b) is not used: a triangle with such a vertex is dropped (nd_gv_tri GVR_SCR) */
    if (where == G_MWO_POINT_XYSCREEN && vi < 64) vert_scr[vi] = 1;
}
/* CORE2-3b: the game thread is about to write memory that the walk of the last frame may have read: a ROM read into RAM, a decode, a map
 * load, a game mode change (load writes, the CORE2-2 census hazard 4). The walker must be idle from here on: wait until the walker queue
 * (nd_wq.c) is empty. Calls made on the
 * walker side (the walker's own reads) do nothing. */
void pm_wq_idle(void); /* nd_wq.c: CORE2C-2, return when the walker queue is empty */
void pm_walk_drain(const char* why) {
    /* PAUSEC: pm_walk_side is the game thread's count of the walk it runs. A logic thread drain must wait for that walk too (the
     * queue entry is popped after it runs), so only the walker's own calls return here. */
    if (pm_walk_side && !pm_wq_on_logic()) return;
    unsigned xt0 = pm_xf.on ? pm_ticks() : 0u; /* BTLWIPE: the dr stage of the xfer line */
    pm_wq_idle();
    if (pm_xf.on) { pm_xf.dr_n++; pm_xf.dr_t += pm_ticks() - xt0; }
}
void pm_gbi_fallback(void) { /* NATFB2 (user yes): the boot picture is one native fill, as the old FILL rectangle; no test triangle */
    pm_gpu_set_replace(1);
    pm_gpu_fill_rect(0, 0, 319, 239, rgba_from_5551(0x2229u));
    pm_gpu_set_replace(0);
}
