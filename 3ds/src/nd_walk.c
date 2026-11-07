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
    case G_SETPRIMCOLOR: return nd_ws.prim == w1 && aud_prim_lod == (u8)(w0 & 0xff) && sc_ok;
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
    dlc_on = dlc_enabled();
    Gfx* stack[12];
    int sp = 0;
    Gfx* pc = (Gfx*)list;
    Gfx* end = (nbytes >= 8) ? pc + nbytes / 8 : NULL;
    unsigned guard = 0;
    u32 pc_lo = 0, pc_hi = 0;
    gp_prev_op = -1; /* PG4: the time between two lists is not an op cost */
    sc_ok = 0; sr_ok = 0; /* DIP2: a new display list starts with a full sync */
    if (!pc) { gv_streak_flush(); tex_streak_flush(); return; }
    dd_end();
    if (dd_on()) { dd_idx = 0; dd_dl0 = dd_dl1 = 0; }
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
                dd_end();
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
        case G_PM_SPR: { /* S4-1, S4-3: producer record of a sprite component (sprite.c). A native sprite (pm_gpu_spr_native) draws here; its vertex and triangle words (header offset) become G_NOOP, the state words after the record still run. Else the walker draws the old words. */
            Gfx* srec = pc - 1;
            if (pm_ph_on) pk_rec[pmx_cur]++; /* WCEN2 */
            if (pm_gpu_spr_native((const uint32_t*)srec)) {
                if (pm_ph_on) pk_nat[pmx_cur]++;
                const u32 k = srec[0].words.w0 & 0xffffu;
                srec[k].words.w0 = 0; srec[k].words.w1 = 0;
                srec[k + 1].words.w0 = 0; srec[k + 1].words.w1 = 0;
            }
            pc += G_PM_SPR_WORDS - 1;
            break;
        }
        case G_PM_ENT: /* S6-1: producer record of an entity model draw (entity_model.c). The record is not run; the old words draw. S6-2a: the end record (G_PM_ENT_END, entcheck only) checks the tile 0 state. */
            if (pm_ph_on) pk_rec[pmx_cur]++; /* WCEN2 */
            if (w0 & G_PM_ENT_END) { pm_gbi_ent_texchk((const uint32_t*)(const void*)(uintptr_t)w1); pm_gbi_ent_segchk((const uint32_t*)(const void*)(uintptr_t)w1); }
            else if ((w0 & G_PM_ENT_DRAW) && ((pm_tex_tagi = 4, ent_draw(pc - 1)) ? 1 : (ent_skipgen = 1, 0))) { if (pm_ph_on) pk_nat[pmx_cur]++; pc += G_PM_ENT_WORDS; break; } /* S6-2c-3: the list drew from the pools, its G_DL after the record is skipped */
            pc += G_PM_ENT_WORDS - 1;
            break;
        case G_PM_TEXI: /* GLYPH: the texture-load words after this header run in tb_inline. */
            tb_inline(pc, w0 & 0xffffu);
            pc += w0 & 0xffffu;
            break;
        case G_PM_RM: /* C2: a render mode list, applied in place (rm_apply). */
            if (rm_apply(w1)) break;
            /* fallback: the list has another op, run it as G_DL */
            w0 = (u32)G_DL << 24; /* G_DL_PUSH */
            /* fall through */
        case G_DL: {
            Gfx* next = (Gfx*)ptr_of(w1);
            if (next && !is_host((u32)next)) {
                gbi_cold_bad_next(w1, w0, (u32)list);
                break;
            }
            if (next) { __builtin_prefetch(next); __builtin_prefetch(next + 4); } /* H4: the G_DL target */
            if (dd_on()) { dd_dl1 = dd_dl0; dd_dl0 = (u32)next; if (sp == 0) dd_top = (u32)(pc - 1); }
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
        case G_VTX: {
            unsigned gv_t0 = gp_on() ? pm_ticks() : 0;
            unsigned n = (w0 >> 12) & 0xff;
            unsigned v0n = (w0 >> 1) & 0x7f;
            unsigned v0 = (v0n >= n) ? v0n - n : 0;
            Vtx* src = (Vtx*)ptr_of(w1);
            if (src) __builtin_prefetch(src + n); /* H4: the next vertex block */
            if (src && !is_host((u32)src)) {
                gbi_cold_bad_vtx(w1, w0, (u32)list);
                src = NULL;
            }
            if (src && n <= 64) {
                for (unsigned pk = 0; pk < n * 16u; pk += 32u) __builtin_prefetch((const char*)src + pk);
                if (v0 >= 64) n = 0; /* S30: a vertex load that starts past the buffer (garbage list) copied 4 GB */
                else if (v0 + n > 64) n = 64 - v0;
                PH_ADD(C_VTXLOAD, n);
                if (dd_on()) memcpy(&verts[v0], src, n * sizeof(Vtx)); /* PG4: verts is read only by the drawlog statistics */
                /* A load at slot 0 starts a new mesh. Later loads in the same
                 * list append above it and keep the earlier slots. */
                if (v0 == 0 && tri_since_vtx) memset(vert_live, 0, sizeof vert_live);
                tri_since_vtx = 0;
                if (v0 == 0 && dd_on()) gbi_cold_dd_load(src);
                int gring0 = -1;
                unsigned gmi = GV_NONE;
                int gv = 0;
                const int lit_ld = (nd_ws.geom & G_LIGHTING) != 0;
                if (n) { /* GV1: a load goes to the ring as it is (GV5: also a lit load, the GPU lights it). A full ring takes the old code. */
                    if (lit_ld && !gv_lit_ok()) gv_fb[GVR_LITLOAD]++;
                    else if (!dd_on() && !xt_on() && !rmlog_on()) {
                        gmi = gv_matrix_id(lit_ld);
                        if (gmi != GV_NONE) gring0 = pm_gpu_gvtx_load(src, n);
                        if (gring0 >= 0) gv = 1; else { gmi = GV_NONE; gv_fb[GVR_RING]++; }
                    }
                }
                if (gv) { gv_load_n += n; PH_ADD(C_GVLOAD, n); }
                int gp1_cls = GP1_CROSS;
                unsigned char gp1_p = 0;
                s16 gp1_lo[3] = { 32767, 32767, 32767 }, gp1_hi[3] = { -32768, -32768, -32768 };
                if (gv) {
                    for (unsigned j = 0; j < n; j++)
                        for (int q = 0; q < 3; q++) {
                            s16 o = src[j].v.ob[q];
                            if (o < gp1_lo[q]) gp1_lo[q] = o;
                            if (o > gp1_hi[q]) gp1_hi[q] = o;
                        }
                    gp1_cls = gp1_box_class(gp1_lo, gp1_hi, combined, &gp1_p);
                    if (gp1_cls == GP1_IN) { gp1_ld_in++; gp1_vx_in += n; }
                    else if (gp1_cls == GP1_OUT) { gp1_ld_out++; gp1_vx_out += n; }
                    else { gp1_ld_cross++; gp1_vx_cross += n; }
                }
                for (unsigned i = 0; i < n; i++) {
                    vert_ring[v0 + i] = gring0 >= 0 ? (unsigned short)(gring0 + (int)i) : GV_NONE;
                    vert_gmi[v0 + i] = (unsigned short)gmi;
                    vert_tss[v0 + i] = ((unsigned)nd_ws.tex_ss << 16) | ((unsigned)nd_ws.tex_ts & 0xffffu);
                    vert_live[v0 + i] = 1;
                    vert_scr[v0 + i] = 0;
                    vert_litf[v0 + i] = (u8)(gv && lit_ld ? 1 | ((nd_ws.geom & G_TEXTURE_GEN) ? 2 : 0) : 0);
                    if (gv && lit_ld) lit_pending = 1;
                    if (gv && gp1_cls == GP1_IN) {
                        vert_cb[v0 + i] = 0;
                        vert_gvf[v0 + i] = 0;
                        continue;
                    }
                    if (gv && gp1_cls == GP1_OUT) {
                        vert_cb[v0 + i] = gp1_p;
                        vert_gvf[v0 + i] = 0;
                        continue;
                    }
                    if (gv) { /* GP1 cross: per-vertex product and clip bits */
                        const float(*m)[4] = combined;
                        float x = (float)src[i].v.ob[0], y = (float)src[i].v.ob[1], z = (float)src[i].v.ob[2];
                        float* p = vert_p[v0 + i];
                        p[0] = m[0][0] * x + m[1][0] * y + m[2][0] * z + m[3][0];
                        p[1] = m[0][1] * x + m[1][1] * y + m[2][1] * z + m[3][1];
                        p[2] = m[0][2] * x + m[1][2] * y + m[2][2] * z + m[3][2];
                        p[3] = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];
                        vert_cb[v0 + i] = (unsigned char)clip_bits4(p);
                        vert_gvf[v0 + i] = 1;
                        st_vx++;
                        continue;
                    }
                    gbi_vtx_cpu1(&src[i], v0 + i);
                }
            }
            if (gp_on()) gp_acc[7] += pm_ticks() - gv_t0;
            break;
        }
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
        case G_SETPRIMCOLOR: /* HT1: the prim body is in tex_op, so a G_PM_TEXI run can hold it */
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
