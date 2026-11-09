/* nd_walk.c: the list walker (WK-1, plan_s10_path group W). nd_walk_run is the pc loop of the former pm_gbi.c, moved without change: the
 * stack, the host range check, the g1 and g2 skip tables and the cold handlers of bad lists. The op handlers stay in the former pm_gbi.c
 * until their groups move (nd_int.h declares them). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "nd_cmd.h"
#include "variables.h"
#include "nd_int.h"

extern int nd_c2_on; /* CORE2-2 census (nd_c2haz.c, switch c2haz) */
void nd_c2_rec(const Gfx* rec, unsigned words);
void nd_c2_src(const Gfx* at);
/* GLOB: definitions moved here from the former pm_gbi.c (layout only). */
unsigned pm_gbi_cur_op; /* PI2: op of the display list step now running (one store per op) */

int host_range(u32 p, u32* lo, u32* hi) {
    u32 heap = __ctru_heap, hend = __ctru_heap + __ctru_heap_size, lin = __ctru_linear_heap, lend = lin + __ctru_linear_heap_size;
    if (p >= heap && p < hend) { *lo = heap; *hi = hend; return 1; }
    if (p >= lin && p < lend) { *lo = lin; *hi = lend; return 1; }
    if (p >= 0x00100000u && p < (u32)(uintptr_t)__end__) { *lo = 0x00100000u; *hi = (u32)(uintptr_t)__end__; return 1; }
    return 0;
}

int pm_gpu_spr_native(const uint32_t* rec); /* SPR4 */

const u8 g1_skip[256] = { [G_SPNOOP] = 1, [G_RDPPIPESYNC] = 1, [G_RDPTILESYNC] = 1, [G_RDPLOADSYNC] = 1, [G_RDPFULLSYNC] = 1 };

void g2_note(unsigned op, u32 w0) {
    if (op == G_SETPRIMCOLOR) { pm_gpu_set_prim(nd_ws.prim, w0 & 0xffffu); nd_ws.prim_lod = w0 & 0xffffu; return; } /* SPR4-prim: LOD change closes the open batch */
    if (op != G_SETOTHERMODE_H && op != G_SETOTHERMODE_L) return;
    unsigned len = (w0 & 0xff) + 1, field = (w0 >> 8) & 0xff, sft = 32 - field - len;
    unsigned mask = (len >= 32) ? 0xffffffffu : (((1u << len) - 1u) << sft);
    if (op == G_SETOTHERMODE_H) nd_ws.om_kh |= mask; else nd_ws.om_kl |= mask;
}

int g2_same(unsigned op, u32 w0, u32 w1) {
    switch (op) {
    /* R2B: each case skips only when the handler's writes equal the current state and its calls do nothing (sync_combine: sc_ok and
     * the same input hash; pm_gpu_set_mode: the mode already set). */
    case G_SETCOMBINE:
        return comb_set && nd_ws.comb_w0 == w0 && nd_ws.comb_w1 == w1 && box_folded == 0 && cmb_gen_seen == comb_gen && sc_ok &&
               sc_sig_now() == sc_sig;
    case G_TEXTURE:
        return nd_ws.tex_on == ((w0 & 0xff) != 0) && nd_ws.tex_ss == (w1 >> 16) && nd_ws.tex_ts == (w1 & 0xffffu) && sc_ok && sc_sig_now() == sc_sig;
    case G_GEOMETRYMODE: {
        unsigned g = (nd_ws.geom & (w0 & 0xffffffu)) | w1;
        return g == nd_ws.geom && pm_gpu_mode_is((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0);
    }
    case G_MOVEMEM: return (w0 & 0xff) == G_MV_LIGHT && lt_mem_same(w0, w1);
    case G_MOVEWORD: { /* R5B: the handler of G_MW_LIGHTCOL writes lt[k].col only (no other effect) */
        unsigned index = (w0 >> 16) & 0xff, offset = w0 & 0xffff, k = offset / 24;
        return index == G_MW_LIGHTCOL && k < 9 && (offset % 24) == 0 && lt[k].col[0] == (u8)(w1 >> 24) && lt[k].col[1] == (u8)(w1 >> 16) &&
               lt[k].col[2] == (u8)(w1 >> 8);
    }
    case G_SETPRIMCOLOR: return nd_ws.prim == w1 && (u8)nd_ws.prim_lod == (u8)(w0 & 0xff) && sc_ok;
    case G_SETENVCOLOR: return nd_ws.envc == w1 && sc_ok;
    case G_SETFOGCOLOR: return fogc == w1;
    case G_SETBLENDCOLOR: return blendc == w1;
    case G_SETOTHERMODE_L:
    case G_SETOTHERMODE_H: {
        unsigned len = (w0 & 0xff) + 1, field = (w0 >> 8) & 0xff;
        unsigned sft = 32 - field - len;
        unsigned mask = (len >= 32) ? 0xffffffffu : (((1u << len) - 1u) << sft);
        unsigned old = op == G_SETOTHERMODE_H ? nd_ws.omode_h : nd_ws.omode_l;
        return (w1 & mask) == (old & mask) && (op == G_SETOTHERMODE_L || sc_ok);
    }
    default: return 0;
    }
}

void gbi_cold_bad_dl(u32 pc, u32 list, unsigned nbytes, int sp) {
    static int bad; if (bad++ < 6) pm_log_e("bad dl pc %08x list %08x n %u sp %d", pc, list, nbytes, sp);
}

void gbi_cold_bad_next(u32 w1, u32 w0, u32 list) {
    static int bad; if (bad++ < 8) pm_log_e("bad dl target %08x w0 %08x list %08x", w1, w0, list);
}

void gbi_cold_bad_vtx(u32 w1, u32 w0, u32 list) {
    static int bad; if (bad++ < 8) pm_log_e("bad vtx %08x w0 %08x list %08x", w1, w0, list);
}

void gbi_cold_bad_mtx(u32 w0, u32 w1, Gfx* list, Gfx* pc) {
    static unsigned bad; if (bad++ < 6) pm_log_w("gbi: G_MTX w0 %08x w1 %08x outside host memory, list %p at op %d, frame %d, previous ops %08x %08x %08x %08x", w0, w1, (void*)list, (int)(pc - list), rm_frame,
        pc > list ? pc[-1].words.w0 : 0, pc > list ? pc[-1].words.w1 : 0, pc > list + 1 ? pc[-2].words.w0 : 0, pc > list + 1 ? pc[-2].words.w1 : 0);
}

/* FX1-3 orbdbg (debug tool): a display list an effect registers (pm_gbi_dbg_reg, energy_orb_wave.c) gets a tag. The walker keeps the tag of the list it runs
 * in (orb_tag, saved with the pc on the stack). Under the orbdbg switch a tagged list runs in the walker, not the ent or DLC path, so orb_log (pm_gpu.c)
 * sees its draws. Nothing is tagged without the switch. */
static const void* orb_lst[32];
static unsigned char orb_lst_tag[32];
static int orb_lst_n;
static unsigned orb_tag, orb_tag_stk[12];
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
    pm_walk_dbg = (pm_ph_cen ? WD_CEN : 0u) | (pm_ph_fine ? WD_FINE : 0u) | (nd_c2_on > 0 ? WD_C2 : 0u) | (pm_ph_on ? WD_PH : 0u) | (orb_lst_n ? WD_ORB : 0u);
}
/* The op prologue of a debug walk: the op count (PH_ADD C_CMD, opsplit sample), the streak and per-op timers of the phase census, the phasegbi timer
 * (returns its phase id, popped after the handler), the opsplit sample and the model census. */
/* PERF2 step 6: the op class of each op for the class timers (nd.h NDT_TEX..NDT_LIST); unlisted ops are list class */
static uint8_t walk_cls_tab[256];
static void walk_cls_init(void) {
    static const uint8_t tex[] = { G_SETTIMG, G_SETTILE, G_LOADTILE, G_LOADBLOCK, G_SETTILESIZE, G_LOADTLUT, G_TEXTURE };
    static const uint8_t comb[] = { G_SETCOMBINE, G_SETENVCOLOR, G_SETPRIMCOLOR, G_SETBLENDCOLOR, G_SETFOGCOLOR, G_SETOTHERMODE_H, G_SETOTHERMODE_L, G_RDPSETOTHERMODE, G_SETPRIMDEPTH };
    static const uint8_t geo[] = { G_VTX, G_MODIFYVTX, G_TRI1, G_TRI2, G_QUAD, G_GEOMETRYMODE, G_CULLDL };
    static const uint8_t mtx[] = { G_MTX, G_POPMTX, G_MOVEWORD, G_MOVEMEM };
    static const uint8_t rect[] = { G_TEXRECT, G_TEXRECTFLIP, G_FILLRECT, G_SETFILLCOLOR, G_SETSCISSOR, G_SETCIMG, G_SETZIMG };
    for (unsigned i = 0; i < 256u; i++) walk_cls_tab[i] = NDT_LIST;
    for (unsigned i = 0; i < sizeof tex; i++) walk_cls_tab[tex[i]] = NDT_TEX;
    for (unsigned i = 0; i < sizeof comb; i++) walk_cls_tab[comb[i]] = NDT_COMB;
    for (unsigned i = 0; i < sizeof geo; i++) walk_cls_tab[geo[i]] = NDT_GEO;
    for (unsigned i = 0; i < sizeof mtx; i++) walk_cls_tab[mtx[i]] = NDT_MTX;
    for (unsigned i = 0; i < sizeof rect; i++) walk_cls_tab[rect[i]] = NDT_RECT;
    walk_cls_tab[0] = NDT_TEX; /* the table is built: entry 0 (G_NOOP) holds a non-list class only as the init mark */
}
static __attribute__((cold, noinline)) int walk_dbg_op(unsigned dbg, const Gfx* at, int sp, unsigned op, u32 w0, u32 w1) {
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
        fp = op == G_VTX ? PH_GVTX : (op == G_TRI1 || op == G_TRI2) ? PH_GTRI : op == G_MTX ? PH_GMTX : 0;
        if (fp) pm_ph_push(fp);
    }
    if (dbg & WD_CEN) { os_op_hook(at, sp, op); mdc_op(at, sp, op, w0, w1); } /* W1: after the prologue, so a sample is the handler only; MDL1: model census, counts only */
    return fp;
}
static __attribute__((cold, noinline)) void walk_dbg_end(unsigned dbg) { /* the end of the outer list */
    if (dbg & WD_PH) pm_gpu_ndt_cls(-1); /* PERF2: the class span ends with the outer list */
    if (!(dbg & WD_CEN)) return;
    if (gp_prev_op >= 0) { gp_opt[gp_prev_op] += pm_ticks() - gp_opt_last; gp_opn[gp_prev_op]++; gp_prev_op = -1; }
    mr0_walk_end(); /* MR0 */
}
static __attribute__((cold, noinline)) void walk_dbg_rec(unsigned dbg, const Gfx* rec, unsigned words) { /* a producer record: CORE2-2 census, WCEN2 count */
    if (dbg & WD_C2) nd_c2_rec(rec, words);
    if (dbg & WD_CEN) pk_rec[pmx_cur]++;
}
static __attribute__((cold, noinline)) void walk_dbg_nat(void) { pk_nat[pmx_cur]++; } /* WCEN2: a record drawn natively */
static __attribute__((cold, noinline)) void walk_dbg_dlc(void) { os_dlc_hit(); mr0_dlc_hit = 1; }
/* The model record arm (G_PM_REC in a list, NDC_MODEL in the frame stream): the record words past the header. */
static unsigned rec_case(unsigned dbg, const Gfx* q) {
    if (__builtin_expect(dbg, 0)) walk_dbg_rec(dbg, q, G_PM_REC_WORDS);
    NDT_BD(tr, dbg);
    unsigned n = rec_run(q);
    NDT_E(tr, NDT_REC);
    if (__builtin_expect(dbg & WD_CEN, 0) && rec_nat_last) walk_dbg_nat();
    return n;
}
/* DIRECT D1: NDC_MODEL of the frame stream (nd_rec.c), with the walker's op prologue of G_PM_REC (gv_epoch, the census op hook). D10a: the
 * only model record path (no list holds a record). */
void nd_walk_rec_model(const Gfx* q) {
    const unsigned dbg = pm_walk_dbg;
    dlc_on = 1;
    pm_gbi_cur_op = G_PM_REC;
    gv_epoch++; /* GV1b, as the walker's prologue of G_PM_REC */
    int fp = 0;
    if (__builtin_expect(dbg, 0)) fp = walk_dbg_op(dbg, q, 0, G_PM_REC, q->words.w0, q->words.w1);
    rec_case(dbg, q);
    if (__builtin_expect(fp, 0)) pm_ph_pop();
    if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1); /* the class span ends with the command */
}
/* DIRECT D2: NDC_SPR (nd_rec.c): the sprite record and its vertex and triangle words, at the point of its final draw (the walker state is the
 * draw state). Native, or else the walker loads and draws the two words, as the G_VTX arm. */
void nd_walk_spr(const Gfx* q) {
    const unsigned dbg = pm_walk_dbg;
    pm_gbi_cur_op = G_PM_SPR;
    int fp = 0;
    if (__builtin_expect(dbg, 0)) { fp = walk_dbg_op(dbg, q, 0, G_PM_SPR, q->words.w0, q->words.w1); walk_dbg_rec(dbg, q, G_PM_SPR_WORDS); }
    NDT_BD(ts, dbg);
    int sh = pm_gpu_spr_native((const uint32_t*)q);
    NDT_E(ts, NDT_SPR);
    if (__builtin_expect(fp, 0)) pm_ph_pop();
    if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1);
    if (sh) { if (__builtin_expect(dbg & WD_CEN, 0)) walk_dbg_nat(); return; }
    nd_walk_ops((void*)(q + G_PM_SPR_WORDS), 16u);
}
/* DIRECT D3: NDC_ENT (nd_rec.c): an entity record. A DRAW record that the native path refuses leaves its G_DL to the walker (ent_skipgen,
 * as the walker's G_PM_ENT arm and the G_DL after it). */
void nd_walk_ent(const Gfx* q, unsigned words) {
    const unsigned dbg = pm_walk_dbg;
    pm_gbi_cur_op = G_PM_ENT;
    gv_epoch++; /* GV1b, as the walker's prologue of G_PM_ENT */
    int fp = 0;
    if (__builtin_expect(dbg, 0)) { fp = walk_dbg_op(dbg, q, 0, G_PM_ENT, q->words.w0, q->words.w1); walk_dbg_rec(dbg, q, G_PM_ENT_WORDS); }
    NDT_BD(te, dbg);
    int drawn = nd_ent_case(q, q->words.w0, q->words.w1);
    NDT_E(te, NDT_ENT);
    if (__builtin_expect(fp, 0)) pm_ph_pop();
    if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1);
    if (!drawn && words > G_PM_ENT_WORDS) nd_walk_ops((void*)(q + G_PM_ENT_WORDS), 8u);
}
void nd_walk_ops(void* list, unsigned nbytes);
/* DIRECT D5: a recorded gSPDisplayList (push form). As the walker's G_DL arm: a static list goes to the entity path (ent_gdl) or a compiled map
 * shape (dlc_try); else the walker runs the list to its G_ENDDL with the sync state kept (nd_walk_ops). */
static void walk_call(unsigned dbg, const Gfx* at, u32 w0, u32 w1) {
    (void)w0;
    if (__builtin_expect(dbg & WD_C2, 0)) nd_c2_src(at);
    Gfx* next = (Gfx*)ptr_of(w1);
    if (!next) return;
    if (!is_host((u32)next)) { gbi_cold_bad_next(w1, w0, (u32)(uintptr_t)at); return; }
    __builtin_prefetch(next);
    if (ent_skipgen) ent_skipgen = 0;
    else if (dlc_on && !((u32)next - (u32)gMapShapeData < 0x40000u)) {
        NDT_BD(te, dbg); pm_tex_tagi = 5; int eh = ent_gdl(next); NDT_E(te, NDT_ENT);
        if (eh) return;
    }
    if (dlc_on) {
        NDT_BD(td, dbg); int dh = dlc_try(next); NDT_E(td, NDT_DLC);
        if (dh) { if (__builtin_expect(dbg & WD_CEN, 0)) walk_dbg_dlc(); return; }
    }
    nd_walk_ops(next, 0);
}
/* DIRECT D4b-3, D5: NDC_STATE (nd_rec.c): a run of ops recorded by the nd_gbi.h redirect. Each op runs as the walker runs it inside a
 * list: g2_same skips it (g2_note), else the op prologue (pm_gbi_cur_op, gv_epoch but not for vertex, triangle, matrix and list ops) and the
 * same handler as the walker's arm. The recorder dropped the RDP syncs (g1_skip); G_ENDDL and the branch form of G_DL stay list words. */
void nd_walk_state(const uint32_t* q, unsigned n) {
    const unsigned dbg = pm_walk_dbg;
    for (const uint32_t* e = q + 2u * n; q < e; q += 2) {
        const u32 w0 = q[0], w1 = q[1];
        const unsigned op = w0 >> 24;
        if (g2_same(op, w0, w1)) { g2_note(op, w0); continue; }
        pm_gbi_cur_op = op;
        if (op != G_VTX && op != G_TRI1 && op != G_TRI2 && op != G_MTX && op != G_POPMTX && op != G_DL && op != G_MODIFYVTX) gv_epoch++; /* GV1b */
        int fp = 0;
        if (__builtin_expect(dbg, 0)) fp = walk_dbg_op(dbg, (const Gfx*)q, 0, op, w0, w1);
        switch (op) {
        case G_VTX: nd_geom_vtx(w0, w1, (const Gfx*)q); break;
        case G_TRI1:
            tri_prep_ok = 0;
            draw_tri(w0);
            break;
        case G_TRI2:
            tri_prep_ok = 0;
            draw_tri(w0);
            draw_tri(w1);
            break;
        case G_MTX:
            if (!is_host((u32)(uintptr_t)ptr_of(w1))) gbi_cold_bad_mtx(w0, w1, (Gfx*)q, (Gfx*)q + 1);
            load_mtx(w0, w1, 0);
            break;
        case G_POPMTX: gbi_popmtx(w1); break;
        case G_MOVEMEM: gbi_movemem(w0, w1); break;
        case G_MOVEWORD: gbi_moveword(w0, w1); break;
        case G_MODIFYVTX: gbi_cold_modifyvtx(w0, w1); break;
        case G_DL: walk_call(dbg, (const Gfx*)q, w0, w1); break;
        case G_SETCONVERT:
            conv_k4 = (w1 >> 9) & 0xffu;
            pm_comb_set_k4(conv_k4);
            sync_combine();
            break;
        case G_SETKEYR:
            keyc = (keyc & 0x00ffffffu) | ((w1 >> 8 & 0xffu) << 24);
            keys = (keys & 0x00ffffffu) | ((w1 & 0xffu) << 24);
            sync_combine();
            break;
        case G_SETKEYGB:
            keyc = (keyc & 0xff000000u) | ((w1 >> 24 & 0xffu) << 16) | ((w1 >> 8 & 0xffu) << 8);
            keys = (keys & 0xff000000u) | ((w1 >> 16 & 0xffu) << 16) | ((w1 & 0xffu) << 8);
            sync_combine();
            break;
        case G_GEOMETRYMODE: set_geom(w0, w1); break;
        case G_TEXTURE:
        case G_SETOTHERMODE_H:
        case G_SETOTHERMODE_L:
        case G_SETCOMBINE:
        case G_SETPRIMCOLOR:
        case G_SETTIMG:
        case G_SETTILE:
        case G_LOADBLOCK:
        case G_LOADTILE:
        case G_LOADTLUT:
        case G_SETTILESIZE: tex_op(w0, w1); break;
        case G_SETSCISSOR: set_scissor(w0, w1); break;
        case G_SETFOGCOLOR: set_fogcolor(w1); break;
        case G_SETBLENDCOLOR: set_blendcolor(w1); break;
        case G_SETENVCOLOR: gbi_setenv(w1); break;
        case G_SETCIMG: gbi_setcimg(w0, w1); break;
        case G_SETPRIMDEPTH: set_primdepth(w1); break;
        case G_SETFILLCOLOR: set_fillcolor(w1); break;
        case G_FILLRECT: nd_fillrect(w0, w1); break;
        case G_PM_ND: nd_native_rec(w0); break; /* D9: the native-only payload (the water splash bracket) */
        case G_NOOP:
        case G_SETZIMG: break;
        default: note(op); break;
        }
        if (__builtin_expect(fp, 0)) pm_ph_pop();
    }
    if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1); /* the class span ends with the command */
}
static int walk_keep; /* PERF2: set by nd_walk_ops for the next nd_walk_run only */
/* DIRECT D10a: a custom gfx range of the frame stream (model record R11, build_custom_gfx), run to its NDC_END as the walker ran the list it
 * replaces: the list start (gv_epoch, dlc_on, a full combine and render sync: DIP2) and the list end (the vertex and texture streak flush). */
void nd_range_run(const uint32_t* a) {
    PM_WALK_OWN();
    gv_epoch++; /* GV1b */
    dlc_on = 1;
    gp_prev_op = -1;
    sc_ok = 0; sr_ok = 0;
    pm_walk_side++;
    nd_stream_run(a);
    pm_walk_side--;
    if (__builtin_expect(pm_walk_dbg, 0)) walk_dbg_end(pm_walk_dbg);
    gv_streak_flush();
    tex_streak_flush();
}
void nd_walk_run(void* list, unsigned nbytes) {
    const unsigned dbg = pm_walk_dbg; /* PROF2-5: once per list */
    gv_epoch++; /* GV1b */
    dlc_on = 1;
    Gfx* stack[12];
    int sp = 0;
    Gfx* pc = (Gfx*)list;
    Gfx* end = (nbytes >= 8) ? pc + nbytes / 8 : NULL;
    unsigned guard = 0;
    u32 pc_lo = 0, pc_hi = 0;
    gp_prev_op = -1; /* PG4: the time between two lists is not an op cost */
    if (!walk_keep) { sc_ok = 0; sr_ok = 0; } /* DIP2: a new display list starts with a full sync (not the pieces of one flattened list, nd_walk_ops) */
    walk_keep = 0;
    if (!pc) { gv_streak_flush(); tex_streak_flush(); return; }
    while (pc && guard++ < 200000) {
        if (end && sp == 0 && pc >= end) break;
        if ((u32)pc < pc_lo || (u32)pc >= pc_hi) { /* PG3 A15: the range of pc is looked up when pc leaves the last range (list entry, G_DL, return), not at every op */
            if (!host_range((u32)pc, &pc_lo, &pc_hi)) { pc_lo = pc_hi = 0;
            gbi_cold_bad_dl((u32)pc, (u32)list, nbytes, sp);
            gv_streak_flush();
            tex_streak_flush();
            return;
            }
        }
        u32 w0 = pc->words.w0;
        u32 w1 = pc->words.w1;
        unsigned op = w0 >> 24;
        if (g1_skip[op]) { pc++; continue; }
        if (g2_same(op, w0, w1)) { g2_note(op, w0); pc++; continue; }
        pm_gbi_cur_op = op;
        int fp = 0;
        pc++;
        if (!((u32)pc & 31u)) { __builtin_prefetch(pc + 4); __builtin_prefetch(pc + 8); }
        if (op != G_VTX && op != G_TRI1 && op != G_TRI2 && op != G_MTX && op != G_POPMTX && op != G_DL && op != G_ENDDL && op != G_MODIFYVTX) {
            gv_epoch++; /* GV1b */
        }
        if (__builtin_expect(dbg, 0)) fp = walk_dbg_op(dbg, pc - 1, sp, op, w0, w1); /* PROF2-5: phase census, phasegbi, op count */
        switch (op) {
        case G_ENDDL:
            if (sp == 0) {
                if (__builtin_expect(dbg, 0)) walk_dbg_end(dbg);
                if (__builtin_expect(fp, 0)) pm_ph_pop();
                gv_streak_flush();
                tex_streak_flush();
                return;
            }
            pc = stack[--sp];
            if (__builtin_expect(dbg & WD_ORB, 0) && orb_tag != orb_tag_stk[sp]) pm_gpu_flush(); /* FX1-3 */
            orb_tag = orb_tag_stk[sp];
            break;
        case G_DL: {
            if (__builtin_expect(dbg & WD_C2, 0)) nd_c2_src(pc - 1);
            Gfx* next = (Gfx*)ptr_of(w1);
            if (next && !is_host((u32)next)) {
                gbi_cold_bad_next(w1, w0, (u32)list);
                break;
            }
            if (next) { __builtin_prefetch(next); __builtin_prefetch(next + 4); } /* H4: the G_DL target */
            unsigned ot_sv = orb_tag; /* FX1-3: the tag of the list this G_DL runs in */
            int ow_ = 0; /* FX1-3: a tagged list under orbdbg runs in the walker (no ent or DLC path) */
            if (__builtin_expect(dbg & WD_ORB, 0) && PMDL("orbdbg")) {
                unsigned t_ = orb_tag_of(next);
                if (t_) { if (t_ != orb_tag) pm_gpu_flush(); orb_tag = t_; ow_ = 1; pm_log("orbdbg enter f%u tag %u ptr %p", pm_frame_count(), t_, (void*)next); }
            }
            if (ent_skipgen) ent_skipgen = 0;
            else if (!ow_ && next && dlc_on && !((u32)next - (u32)gMapShapeData < 0x40000u)) { /* SGEN-1: any other static list, the entity path */
                NDT_BD(te, dbg); pm_tex_tagi = 5; int eh = ent_gdl(next); NDT_E(te, NDT_ENT);
                if (eh) { if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) pc = (Gfx*)dlc_end; break; }
            }
            if (!ow_ && dlc_on) { /* DLC1: a compiled map shape list. A G_DL_NOPUSH ends the list that holds it, as G_ENDDL. */
                NDT_BD(td, dbg); int dh = dlc_try(next); NDT_E(td, NDT_DLC);
                if (dh) {
                    if (__builtin_expect(dbg & WD_CEN, 0)) walk_dbg_dlc();
                    if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) pc = (Gfx*)dlc_end;
                    break;
                }
            }
            if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) { pc = next; }
            else if (sp < 12) {
                orb_tag_stk[sp] = ot_sv; /* FX1-3 */
                stack[sp++] = pc;
                pc = next;
            } else orb_tag = ot_sv;
            break;
        }
        case G_VTX:
            nd_geom_vtx(w0, w1, list);
            break;
        case G_MODIFYVTX:
            gbi_cold_modifyvtx(w0, w1);
            break;
        case G_TRI1:
            tri_prep_ok = 0;
            draw_tri(w0);
            break;
        case G_TRI2:
            tri_prep_ok = 0;
            draw_tri(w0);
            draw_tri(w1);
            break;
        case G_MTX:
            if (!is_host((u32)(uintptr_t)ptr_of(w1))) gbi_cold_bad_mtx(w0, w1, (Gfx*)list, pc);
            load_mtx(w0, w1, 0);
            break;
        case G_SETSCISSOR: set_scissor(w0, w1); break;
        case G_MOVEMEM:
            gbi_movemem(w0, w1);
            break;
        case G_POPMTX:
            gbi_popmtx(w1);
            break;
        case G_GEOMETRYMODE:
            set_geom(w0, w1);
            break;
        case G_TEXTURE:
        case G_SETOTHERMODE_H:
        case G_SETOTHERMODE_L:
        case G_SETCOMBINE:
        case G_SETPRIMCOLOR: /* HT1: the prim body is in tex_op, so a texture run can hold it */
            tex_op(w0, w1);
            break;
        case G_SETCONVERT:
            conv_k4 = (w1 >> 9) & 0xffu;
            pm_comb_set_k4(conv_k4);
            sync_combine();
            break;
        case G_SETKEYR:
            keyc = (keyc & 0x00ffffffu) | ((w1 >> 8 & 0xffu) << 24);
            keys = (keys & 0x00ffffffu) | ((w1 & 0xffu) << 24);
            sync_combine();
            break;
        case G_SETKEYGB:
            keyc = (keyc & 0xff000000u) | ((w1 >> 24 & 0xffu) << 16) | ((w1 >> 8 & 0xffu) << 8);
            keys = (keys & 0xff000000u) | ((w1 >> 16 & 0xffu) << 16) | ((w1 & 0xffu) << 8);
            sync_combine();
            break;
        case G_SETFOGCOLOR: set_fogcolor(w1); break;
        case G_SETBLENDCOLOR: set_blendcolor(w1); break;
        case G_SETENVCOLOR:
            gbi_setenv(w1);
            break;
        case G_SETCIMG:
            gbi_setcimg(w0, w1);
            break;
        case G_SETPRIMDEPTH: set_primdepth(w1); break;
        case G_SETFILLCOLOR: set_fillcolor(w1); break;
        case G_FILLRECT: {
            nd_fillrect(w0, w1);
            break;
        }
        case G_TEXRECT:
        case G_TEXRECTFLIP: {
            if (!pc || (end && sp == 0 && pc + 1 >= end)) break;
            u32 hs = pc->words.w1;
            pc++;
            u32 hd = pc->words.w1;
            pc++;
            nd_texrect(w0, w1, hs, hd);
            break;
        }
        case G_SETTIMG:
        case G_SETTILE:
        case G_LOADBLOCK:
        case G_LOADTILE:
        case G_LOADTLUT:
        case G_SETTILESIZE:
            tex_op(w0, w1);
            break;
        case G_MOVEWORD:
            gbi_moveword(w0, w1);
            break;
        case G_PM_ND: /* LS-4: native-only payload record (gbi_custom.h), dispatched by kind in nd_target.c */
            nd_native_rec(w0);
            break;
        case G_NOOP: /* G_SPNOOP and the RDP syncs never get here: g1_skip skips them */
        case G_SETZIMG:
        case G_RDPHALF_1:
        case G_RDPHALF_2:
            break;
        default:
            note(op);
            break;
        }
        if (__builtin_expect(fp, 0)) pm_ph_pop();
    }
    if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1); /* PERF2: a list that ends at its byte count (no G_ENDDL) closes its class span too */
    gv_streak_flush();
    tex_streak_flush();
}

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

void nd_c2_note(const void* p);
void nd_c2_task(const void* list);
int nd_c2_init(void);
static inline void* ptr_res(u32 w);
void* ptr_of(u32 w) {
    void* r = ptr_res(w);
    if (__builtin_expect(nd_c2_on > 0, 0)) nd_c2_note(r);
    return r;
}
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
    nd_c2_init();
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
void gbi_cold_modifyvtx(u32 w0, u32 w1) { /* B5: F3DEX2 w0 = op, where, vertex * 2. Only G_MWO_POINT_XYSCREEN is used by the game (underwater.c) */
    unsigned where = (w0 >> 16) & 0xff, vi = (w0 & 0xffffu) / 2;
    /* w1 (the screen position) is not used: a triangle with such a vertex is dropped (gv_tri GVR_SCR) */
    if (where == G_MWO_POINT_XYSCREEN && vi < 64) vert_scr[vi] = 1;
}
/* W1/P1: the outer call sets the producer cursor state. Nested calls (dlc replay) run inside the outer op. */
static void gbi_run1(void* list, unsigned nbytes);
void nd_c2_walked(void);
void nd_c2_drain(const char* why);
void pm_gbi_run(void* list, unsigned nbytes) {
    PM_WALK_OWN(); /* CORE2-3a: a walk runs on the walker side only (gbi_task, a posted message) */
    pm_walk_side++;
    if (nd_c2_on > 0) { nd_c2_task(list); gbi_run1(list, nbytes); nd_c2_walked(); }
    else gbi_run1(list, nbytes);
    pm_walk_side--;
}
/* PERF2: the ops of a flattened entity list (ent_run, nd_ent.c) between its native segments. The pieces are one list: the walker's combine and
 * render shadows (sc_ok, sr_ok) stay valid across them (a native segment keeps them as dlc_try does inside a walk), so the first combine or
 * render op of a piece is not a full sync. ent_run made 37 walks and 40 full combine syncs per frame on mac_01 (entsync line). */
void nd_walk_ops(void* list, unsigned nbytes) { walk_keep = 1; pm_gbi_run(list, nbytes); }
/* CORE2-3b: the game thread is about to write memory that the walk of the last frame may have read: a ROM read into RAM, a decode, a map
 * load, a game mode change (load writes, the CORE2-2 census hazard 4). The walker must be idle from here on: wait until the walker queue
 * (nd_wq.c) is empty. The census (switch c2haz) checks the writes made before this point and starts the frame clean. Calls made on the
 * walker side (the walker's own reads) do nothing. */
void pm_wq_idle(void); /* nd_wq.c: CORE2C-2, return when the walker queue is empty */
void pm_walk_drain(const char* why) {
    /* PAUSEC: pm_walk_side is the game thread's count of the walk it runs. A logic thread drain must wait for that walk too (the
     * queue entry is popped after it runs), so only the walker's own calls return here. */
    if (pm_walk_side && !pm_wq_on_logic()) return;
    unsigned xt0 = pm_xf.on ? pm_ticks() : 0u; /* BTLWIPE: the dr stage of the xfer line */
    pm_wq_idle();
    if (nd_c2_on > 0) nd_c2_drain(why);
    if (pm_xf.on) { pm_xf.dr_n++; pm_xf.dr_t += pm_ticks() - xt0; }
}
static void gbi_run1(void* list, unsigned nbytes) {
    if (!pm_ph_cen) { nd_walk_run(list, nbytes); return; }
    if (os_nest == 0) { pre_w0 = (const Gfx*)list; pmx_main = 0; /* DIRECT D9: no main task list (the producer census by list position has no main list) */
        pmk_n = 0; /* WCEN3: the walk copy holds the marks of this list only */
        pre_mkn = pmk_n; pre_mk0 = pmk_n ? pmk[0].p : NULL; pmx_cur = pmx_main ? PK_PRE : pk_slot((unsigned)(uintptr_t)list); /* WCEN2: a non-main list has its own key (its address) */ pmx_next = 0; mdcm_next = 0; mdcm_cur = -1; }
    os_nest++;
    nd_walk_run(list, nbytes);
    if (os_open && os_nest == 1) os_close();
    os_nest--;
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
