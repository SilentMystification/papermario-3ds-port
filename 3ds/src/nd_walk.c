/* nd_walk.c: the list walker (WK-1, plan_s10_path group W). nd_walk_run is the pc loop of pm_gbi.c, moved without change: the
 * stack, the host range check, the g1 and g2 skip tables and the cold handlers of bad lists. The op handlers stay in pm_gbi.c
 * until their groups move (nd_int.h declares them). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "pm_gbi.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
#include "nd_int.h"

int host_range(u32 p, u32* lo, u32* hi) {
    u32 heap = __ctru_heap, hend = __ctru_heap + __ctru_heap_size, lin = __ctru_linear_heap, lend = lin + __ctru_linear_heap_size;
    if (p >= heap && p < hend) { *lo = heap; *hi = hend; return 1; }
    if (p >= lin && p < lend) { *lo = lin; *hi = lend; return 1; }
    if (p >= 0x00100000u && p < (u32)(uintptr_t)__end__) { *lo = 0x00100000u; *hi = (u32)(uintptr_t)__end__; return 1; }
    return 0;
}

int pm_gpu_spr_native(const uint32_t* rec); /* SPR4 */
int pm_gpu_spr_checking(void); /* SPR4 */
void pm_gpu_spr_flush(void); /* SPR4 sprcheck: the flush at the sprite triangle; counts whether the sprite was checked by it */
static Gfx* spr_at; /* SPR4: vertex word of the sprite record to draw natively (G_PM_SPR arm sets it, G_VTX arm uses it) */
static Gfx* spr_rec; /* SPR4: the sprite record of spr_at */
static Gfx* spr_flush_at; /* SPR4 sprcheck: the triangle word of a walker-drawn sprite; the batch closes after it, so the compare sees the draw state */

const u8 g1_skip[256] = { [G_SPNOOP] = 1, [G_RDPPIPESYNC] = 1, [G_RDPTILESYNC] = 1, [G_RDPLOADSYNC] = 1, [G_RDPFULLSYNC] = 1 };

void g2_note(unsigned op, u32 w0) {
    if (op == G_SETPRIMCOLOR) { nd_ws.prim_lod = w0 & 0xffffu; return; }
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

void nd_walk_run(void* list, unsigned nbytes) {
    gv_epoch++; /* GV1b */
    dlc_on = 1;
    Gfx* stack[12];
    int sp = 0;
    Gfx* pc = (Gfx*)list;
    Gfx* end = (nbytes >= 8) ? pc + nbytes / 8 : NULL;
    unsigned guard = 0;
    u32 pc_lo = 0, pc_hi = 0;
    gp_prev_op = -1; /* PG4: the time between two lists is not an op cost */
    sc_ok = 0; sr_ok = 0; /* DIP2: a new display list starts with a full sync */
    if (!pc) { gv_streak_flush(); tex_streak_flush(); return; }
    while (pc && guard++ < 200000) {
        if (__builtin_expect(spr_flush_at != NULL, 0) && pc == spr_flush_at + 1) { spr_flush_at = NULL; pm_gpu_spr_flush(); } /* SPR4 sprcheck: close the batch of the sprite */
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
        if (pm_ph_on) { gv_streak_op(op); tex_streak_op(op); }
        pc++;
        if (!((u32)pc & 31u)) { __builtin_prefetch(pc + 4); __builtin_prefetch(pc + 8); }
        n_ops++;
        if (gp_on()) {
            unsigned t = pm_ticks();
            if (gp_prev_op >= 0) { gp_opt[gp_prev_op] += t - gp_opt_last; gp_opn[gp_prev_op]++; }
            gp_opt_last = t;
            gp_prev_op = (int)op;
        }
        if (pm_ph_fine) {
            /* only these three: the thousands of state ops stay in the gbi phase (a tick read costs 1.3 us in Azahar) */
            fp = op == G_VTX ? PH_GVTX : (op == G_TRI1 || op == G_TRI2) ? PH_GTRI : op == G_MTX ? PH_GMTX : 0;
            if (fp) pm_ph_push(fp);
        }
        if (op != G_VTX && op != G_TRI1 && op != G_TRI2 && op != G_MTX && op != G_POPMTX && op != G_DL && op != G_ENDDL && op != G_MODIFYVTX) {
            gv_epoch++; /* GV1b */
        }
        if (pm_ph_on) os_op_hook(pc - 1, sp, op); /* W1: after the prologue, so a sample is the handler only */
        if (pm_ph_on) mdc_op(pc - 1, sp, op, w0, w1); /* MDL1: model census, counts only */
        switch (op) {
        case G_ENDDL:
            if (sp == 0) {
                if (gp_on() && gp_prev_op >= 0) { gp_opt[gp_prev_op] += pm_ticks() - gp_opt_last; gp_opn[gp_prev_op]++; gp_prev_op = -1; }
                if (pm_ph_on) mr0_walk_end(); /* MR0 */
                if (fp) pm_ph_pop();
                gv_streak_flush();
                tex_streak_flush();
                return;
            }
            pc = stack[--sp];
            break;
        case G_PM_REC: /* M4-2, S3F-4: producer record of a model (model.c) */
            if (pm_ph_on) pk_rec[pmx_cur]++; /* WCEN2 */
            pc += rec_run(pc - 1);
            if (pm_ph_on && rec_nat_last) pk_nat[pmx_cur]++;
            break;
        case G_PM_SPR: { /* S4-1, S4-3, SPR4: producer record of a sprite component (sprite.c). The record is not run here: the state words after it run on the walker, and the final draw is built at its vertex word (G_VTX arm below), where the live state is the draw state. */
            Gfx* srec = pc - 1;
            if (pm_ph_on) pk_rec[pmx_cur]++; /* WCEN2 */
            spr_rec = srec;
            spr_at = srec + (srec[0].words.w0 & 0xffu); /* header bits 0 to 7: the offset of the final gSPVertex word */
            pc += G_PM_SPR_WORDS - 1;
            break;
        }
        case G_PM_ENT: /* S6-1: producer record of an entity model draw (nd_ent.c). The record is not run; the old words draw unless nd_ent_case draws it. */
            if (pm_ph_on) pk_rec[pmx_cur]++; /* WCEN2 */
            pc += G_PM_ENT_WORDS - 1 + nd_ent_case(pc - 1, w0, w1); /* the drawn record skips its own words and its G_DL */
            break;
            pc += w0 & 0xffffu;
            break;
            /* fall through */
        case G_DL: {
            Gfx* next = (Gfx*)ptr_of(w1);
            if (next && !is_host((u32)next)) {
                gbi_cold_bad_next(w1, w0, (u32)list);
                break;
            }
            if (next) { __builtin_prefetch(next); __builtin_prefetch(next + 4); } /* H4: the G_DL target */
            __builtin_prefetch(next); __builtin_prefetch(next + 4);
            if (ent_skipgen) ent_skipgen = 0;
            else if (next && dlc_on && !((u32)next - (u32)gMapShapeData < 0x40000u) && (pm_tex_tagi = 5, ent_gdl(next))) { /* SGEN-1: any other static list, the entity path */
                if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) pc = (Gfx*)dlc_end;
                break;
            }
            if (dlc_on && dlc_try(next)) { /* DLC1: a compiled map shape list. A G_DL_NOPUSH ends the list that holds it, as G_ENDDL. */
                if (pm_ph_on) { os_dlc_hit(); mr0_dlc_hit = 1; }
                if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) pc = (Gfx*)dlc_end;
                break;
            }
            if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) { pc = next; }
            else if (sp < 12) {
                stack[sp++] = pc;
                pc = next;
            }
            break;
        }
        case G_VTX:
            if (pc - 1 == spr_at) { /* SPR4: the final draw of a sprite record. Native: the vertex and triangle words are skipped. Else the walker loads and draws them. */
                spr_at = NULL;
                if (pm_gpu_spr_native((const uint32_t*)spr_rec)) {
                    if (pm_ph_on) pk_nat[pmx_cur]++;
                    pc += 1;
                    break;
                }
                if (pm_gpu_spr_checking()) spr_flush_at = pc; /* the triangle word: the walker draw ends its batch there */
            }
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
        case G_NOOP:
            if ((w0 & 0xffffffu) == PM_SPLASH_ON) splash_on = 1;
            else if ((w0 & 0xffffffu) == PM_SPLASH_OFF) splash_on = 0;
            break;
        case G_SPNOOP:
        case G_RDPFULLSYNC:
        case G_RDPTILESYNC:
        case G_RDPPIPESYNC:
        case G_RDPLOADSYNC:
        case G_SETZIMG:
        case G_RDPHALF_1:
        case G_RDPHALF_2:
            break;
        default:
            note(op);
            break;
        }
        if (fp) pm_ph_pop();
    }
    gv_streak_flush();
    tex_streak_flush();
}

/* ---- MV-7: walk core, cut from pm_gbi.c (no change): host range, pointer resolve, init, cold modifyvtx, lt compare, run entry, fallback ---- */
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
void pm_gbi_init(void) {
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
    if (where == G_MWO_POINT_XYSCREEN && vi < 64) {
        vert_sx[vi] = (s16)(w1 >> 16);
        vert_sy[vi] = (s16)(w1 & 0xffffu);
        vert_scr[vi] = 1;
    }
}
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
