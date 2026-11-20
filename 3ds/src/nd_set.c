/* nd_set.c: the native state setters of the walker (group S, ST-1, cut from the former pm_gbi.c). 3DS only: they call pm_gpu_* and pm_tex_*.
 * nd_state.c keeps NdWs, nd_words and the field decoders (host-built). */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
#include "nd_ops.h"
#include "nd_op.h"
#include "nd_fs.h"
extern int pm_wide; /* WIDE4: set_scissor reads signed x edges with wide on */

#define GP_SET(k, same) do { if (gp_on()) gp_note(k, (same)); } while (0)
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__) /* PG0: the flush counter by cause (prof mode) */
void pm_tex_set_filter(int linear);

/* ST-1: globals of the setters, cut from the former pm_gbi.c (writer: this module) */
int sci_x0, sci_y0, sci_x1 = 320, sci_y1 = 240;
int fog_fm, fog_fo, fogmix_on; /* F08: G_MW_FOG factor and offset, blender FOG/SHADE/IN/1MA active */
unsigned sr_sig;
int sr_ok;


/* CORE-2b: the geometry mode setter. Every G_GEOMETRYMODE path writes through here (gv_epoch stays with the callers that bumped it). */
void set_geom(u32 w0, u32 w1) {
    nd_ws.geom = (nd_ws.geom & (w0 & 0xffffffu)) | w1;
    pm_gpu_set_mode((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0);
}

/* Render mode bits of G_SETOTHERMODE_L, same reading as libultraship DeriveTriState: depth
 * compare and update come from Z_CMP and Z_UPD, a decal passes equal depth, blending is on when
 * the blender mixes with the frame buffer by (1 - alpha), texture edge and the alpha threshold
 * cut away near-transparent texels. */
unsigned fogc, blendc; /* V06: G_SETFOGCOLOR and G_SETBLENDCOLOR, RGBA */
/* CORE-2b: the fog and blend colour setters (fogc, blendc; the fog factors stay with G_MOVEWORD). */
void set_fogcolor(u32 w1) { fogc = w1; }

/* CORE-2b: scissor (the walker's sci_* and the GPU scissor), prim depth and fill colour. */
void set_scissor(u32 w0, u32 w1) {
    int ulx = (int)((w0 >> 12) & 0xfff);
    int uly = (int)(w0 & 0xfff);
    int lrx = (int)((w1 >> 12) & 0xfff);
    int lry = (int)(w1 & 0xfff);
    /* WIDE4: with wide on a window in the left margin (battle Moves/Strats list at game x -20) sets a scissor with a negative left edge; gDPSetScissor
     * packs it as an unsigned 12-bit field (-18 -> 1006 px), which swapped the edges and cut the list text. The x fields are signed 10.2 then. */
    if (pm_wide) {
        if (ulx & 0x800) ulx |= ~0xfff;
        if (lrx & 0x800) lrx |= ~0xfff;
    }
    sci_x0 = ulx >> 2;
    sci_y0 = uly >> 2;
    sci_x1 = lrx >> 2;
    sci_y1 = lry >> 2;
    pm_gpu_set_scissor(ulx >> 2, uly >> 2, lrx >> 2, lry >> 2);
}

void set_primdepth(u32 w1) {
    nd_ws.prim_z = (w1 >> 16) & 0x7fffu;
    { extern void pm_gpu_dbg_primz(unsigned z); pm_gpu_dbg_primz(nd_ws.prim_z); } /* POND-LOG: waterlog line of pm_gpu.c */
}

void set_fillcolor(u32 w1) { nd_ws.fill = w1; }

void set_blendcolor(u32 w1) { blendc = w1; }

/* CORE-2b: G_SETPRIMCOLOR, one owner (the walker and the record both run it). a = the LOD word, b = the colour. */
void nd_set_prim(u32 a, u32 b) {
    GP_SET(1, nd_ws.prim == b && (u8)nd_ws.prim_lod == (u8)(a & 0xff));
    pm_gpu_set_prim(b, a & 0xffffu); /* closes the open batch before the change (SPR4-prim) */
    nd_ws.prim = b;
    nd_ws.prim_lod = a & 0xffffu; /* S3N-3d: the prim LOD word (pm_gpu key) */
    nd_comb_sync();
}

/* HC4a: the othermode setter on the typed fields (h: the H word; mask from shift and length, nd_op.h). */
static void set_om(int h, unsigned sft, unsigned len, unsigned mask, u32 v) {
            if (h) {
                unsigned old_tf = nd_ws.omode_h & (3u << 12);
                nd_ws.omode_h = (nd_ws.omode_h & ~mask) | (v & mask);
                nd_ws.om_kh |= mask; /* B8 known bits */
                if ((nd_ws.omode_h & (3u << 12)) != old_tf) { /* V15: G_MDSFT_TEXTFILT, POINT is 0, BILERP 2, AVERAGE 3 */
                    pm_gpu_flush();
                    pm_tex_set_filter((nd_ws.omode_h & (3u << 12)) != 0);
                }
            }
            else { nd_ws.omode_l = (nd_ws.omode_l & ~mask) | (v & mask); nd_ws.om_kl |= mask; }
            /* S25: the cycle type decides 1 or 2 cycles of the combiner. */
            if (h && sft <= G_MDSFT_CYCLETYPE && sft + len > G_MDSFT_CYCLETYPE) nd_comb_sync();
}

/* S3F-2b: G_SETENVCOLOR, one function for the walker and the record. */
void nd_envc_set(u32 w1) {
    GP_SET(2, nd_ws.envc == w1);
    nd_ws.envc = w1;
    pm_gpu_set_env(nd_ws.envc);
    nd_comb_sync();
}

/* HC4a: G_MOVEWORD by its typed kind (nd_op.c decodes the index and offset). */
static void set_mw(unsigned kind, unsigned arg, u32 w1) {
    switch (kind) {
    case ND_FS_MW_NUMLIGHT: nd_light_count(w1); break; /* L01 */
    case ND_FS_MW_LIGHTCOL: lt[arg].col[0] = w1 >> 24; lt[arg].col[1] = w1 >> 16; lt[arg].col[2] = w1 >> 8; break; /* L01 */
    case ND_FS_MW_FOG: fog_fm = (s16)(w1 >> 16); fog_fo = (s16)w1; break; /* F08 */
    case ND_FS_MW_SEG:
        nd_ws.segbase[arg] = (u32)ptr_of(w1);
        if (!nd_ws.segbase[arg]) nd_ws.segbase[arg] = w1;
        break;
    default: break;
    }
}

/* HC4a: the typed scalar ops (nd_op.h). nd_unit_same: 1 when the walker skips the op (the old g2_same case; it runs the old g2_note part);
 * nd_unit_apply: the handler of the op (the walker's arm). */
int nd_unit_same(const uint32_t* u) {
    const u32 a = u[0] & 0xffffffu, b = u[1];
    switch (u[0] >> 24) {
    case ND_FS_OMH: { const u32 m = nd_op_om_mask(a); if ((b & m) != (nd_ws.omode_h & m) || !sc_ok) return 0; nd_ws.om_kh |= m; return 1; }
    case ND_FS_OML: { const u32 m = nd_op_om_mask(a); if ((b & m) != (nd_ws.omode_l & m)) return 0; nd_ws.om_kl |= m; return 1; }
    case ND_FS_GEOM: {
        unsigned g = (nd_ws.geom & a) | b;
        return g == nd_ws.geom && pm_gpu_mode_is((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0);
    }
    case ND_FS_PRIM:
        if (nd_ws.prim != b || (u8)nd_ws.prim_lod != (u8)a || !sc_ok) return 0;
        pm_gpu_set_prim(nd_ws.prim, a); nd_ws.prim_lod = a; /* SPR4-prim: LOD change closes the open batch */
        return 1;
    case ND_FS_ENV: return nd_ws.envc == b && sc_ok;
    case ND_FS_FOG: return fogc == b;
    case ND_FS_BLEND: return blendc == b;
    case ND_FS_TEXTURE: return nd_ws.tex_on == (int)a && nd_ws.tex_ss == (b >> 16) && nd_ws.tex_ts == (b & 0xffffu) && sc_ok && sc_sig_now() == sc_sig;
    case ND_FS_COMB: /* R2B: the handler's writes equal the state and nd_comb_sync does nothing (sc_ok and the same input hash) */
        return comb_set && nd_ws.comb_w0 == ND_FS_COMB_W0(a) && nd_ws.comb_w1 == b && box_folded == 0 && cmb_gen_seen == comb_gen && sc_ok &&
               sc_sig_now() == sc_sig;
    case ND_FS_MW: /* R5B: the handler of G_MW_LIGHTCOL writes lt[k].col only (no other effect) */
        return (a >> 16) == ND_FS_MW_LIGHTCOL && lt[a & 0xffffu].col[0] == (u8)(b >> 24) && lt[a & 0xffffu].col[1] == (u8)(b >> 16) &&
               lt[a & 0xffffu].col[2] == (u8)(b >> 8);
    default: return 0;
    }
}
static __attribute__((cold, noinline)) void set_rare(unsigned k, u32 w1) {
    if (k == ND_FS_RARE_CONV) { conv_k4 = (w1 >> 9) & 0xffu; pm_comb_set_k4(conv_k4); }
    else if (k == ND_FS_RARE_KEYR) {
        keyc = (keyc & 0x00ffffffu) | ((w1 >> 8 & 0xffu) << 24);
        keys = (keys & 0x00ffffffu) | ((w1 & 0xffu) << 24);
    } else {
        keyc = (keyc & 0xff000000u) | ((w1 >> 24 & 0xffu) << 16) | ((w1 >> 8 & 0xffu) << 8);
        keys = (keys & 0xff000000u) | ((w1 >> 16 & 0xffu) << 16) | ((w1 & 0xffu) << 8);
    }
    nd_comb_sync();
}
void nd_unit_apply(const uint32_t* u) {
    const u32 a = u[0] & 0xffffffu, b = u[1];
    switch (u[0] >> 24) {
    case ND_FS_OMH: set_om(1, a & 0xffu, (a >> 8) & 0xffu, nd_op_om_mask(a), b); break;
    case ND_FS_OML: set_om(0, a & 0xffu, (a >> 8) & 0xffu, nd_op_om_mask(a), b); break;
    case ND_FS_GEOM: set_geom(a, b); break;
    case ND_FS_PRIM: nd_set_prim(a, b); break;
    case ND_FS_ENV: nd_envc_set(b); break;
    case ND_FS_FOG: set_fogcolor(b); break;
    case ND_FS_BLEND: set_blendcolor(b); break;
    case ND_FS_FILL: set_fillcolor(b); break;
    case ND_FS_PRIMZ: set_primdepth(b); break;
    case ND_FS_SCISSOR: set_scissor(a, b); break;
    case ND_FS_TEXTURE: nd_tx_texon(a, b); nd_comb_sync(); break;
    case ND_FS_MW: set_mw(a >> 16, a & 0xffffu, b); break;
    case ND_FS_RARE: set_rare(a, b); break;
    case ND_FS_COMB: nd_comb_set(ND_FS_COMB_W0(a), b); nd_comb_sync(); break;
    case ND_FS_TIMG: nd_tx_timg(a, b); break; /* HC4c: the texture image, tile and load ops (nd_tex_set.c) */
    case ND_FS_TILE: nd_tx_tile(a, b); break;
    case ND_FS_TILESIZE: nd_tx_tsz(a, b); break;
    case ND_FS_LOADTLUT: nd_tx_ldtlut(b); break;
    default: break;
    }
}

/* V06 rule, one owner (nd_rnd_sync and the native model state, pm_gpu.c model_draw): the first blender cycle P * A_FOG + IN * (1 - A_FOG), P = BL or FOG.
 * 1 with *col = the mix colour (RGB of P, alpha = the fog colour alpha), else 0. */
extern int pm_nofog; /* MENUGEN part 3: Disable Fog (pm_options.c) */
/* MENUGEN part 3: the room shroud is the blend colour mix (P = BL), so Disable Fog (the FOG mix) never turns it off (macros.h:319, nd_recs.c:95, :178) */
_Static_assert((((PM_RM_SHROUD) >> 30) & 3u) == 2u, "PM_RM_SHROUD must not be the FOG mix");
int nd_mix1(unsigned ol, unsigned* col) {
    unsigned p = (ol >> 30) & 3u;
    if ((p != 2u && p != 3u) || ((ol >> 26) & 3u) != 1u || ((ol >> 22) & 3u) != 0u || ((ol >> 18) & 3u) != 0u) return 0;
    if (p == 3u && pm_nofog) return 0; /* FOG blend: no fog. Caveat: the room shroud (PM_RM_SHROUD, nd_recs.c:95, :178) uses this mix too */
    *col = ((p == 2u ? blendc : fogc) & 0xffffff00u) | (fogc & 0xffu);
    return 1;
}
void nd_sr_forget(void) { sr_ok = 0; }
static __attribute__((cold, noinline)) void sr_orb_log(unsigned m, const NdRender* rs) {
    static unsigned orb_sf = ~0u;
    extern unsigned pm_frame_count(void);
    extern void pm_log(const char* fmt, ...);
    (void)m;
    if (orb_sf != pm_frame_count()) {
        orb_sf = pm_frame_count();
        pm_log("orbdbg state f%u blend %d atest %d zcmp %d zupd %d decal %d memb %d nocol %d mix %d fogmix %d", orb_sf, rs->blend, rs->atest,
               rs->zcmp, rs->zupd, rs->decal, rs->memb, rs->nocol, (int)0, (int)fogmix_on);
    }
}
int nd_sr_same(unsigned sg) { /* NDR0F2: 1 when the GPU state of signature sg is the one set (nd_rnd_sync skip) */
    if (sr_ok && sg == sr_sig) {
        GP_SET(6, 1);
        return 1;
    }
    nd_ent_rfull += nd_ent_in;
    GP_SET(6, 0);
    return 0;
}
void nd_render_apply(const NdRender* rs, int mixk, unsigned mixc, unsigned omh, unsigned oml, unsigned sg) { /* NDR0F2: the GPU part of nd_rnd_sync */
    sr_sig = sg;
    sr_ok = 1;
    pm_gpu_set_mix(mixk, mixc); /* V06 / F08: mixk 1 colour mix, 2 fog shade alpha mix, 0 none */
    fogmix_on = mixk == 2;
    pm_gpu_set_nocolor(rs->nocol);
    if (rs->set_memb) pm_gpu_set_memblend(rs->memb);
    pm_gpu_set_render(rs->zcmp, rs->zupd, rs->blend, rs->atest, rs->decal);
    pm_gpu_set_zmark(0, 0.f); /* MERLON: zmark lives one render sync; gv_epoch_enter turns it on after this for a depth-image model */
    pm_gpu_dbg_mode(oml); /* FX1-2b: orbdbg log of the glow blend, set after the flush of the old state */
    if (oml == 0x00504B50u && PMDL("orbdbg")) sr_orb_log(oml, rs); /* FX1-2b: the render state the glow's draws get, once per frame. HCCUT3: cold */
    pm_gpu_render_src(omh, oml, rs->depth);
}
/* NAT4I: the native state records of nd_rec.c rec_nat (core 1 values after the unit; bit 31 = the core 1 mark) */
void nd_sci_run(const uint32_t* r) { pm_gbi_cur_op = ND_FS_SCISSOR; gv_epoch++; sci_x0 = (int16_t)r[0]; sci_y0 = (int16_t)(r[0] >> 16); sci_x1 = (int16_t)r[1]; sci_y1 = (int16_t)(r[1] >> 16); pm_gpu_set_scissor(sci_x0, sci_y0, sci_x1, sci_y1); }
void nd_geo_run(const uint32_t* r) { const unsigned g = r[0]; const int mk = (int)(r[1] >> 31);
    const int same = g == nd_ws.geom && pm_gpu_mode_is((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0);
    if (same && !mk) return; pm_gbi_cur_op = ND_FS_GEOM; if (mk) gv_epoch++;
    if (!same) { nd_ws.geom = g; pm_gpu_set_mode((g & G_ZBUFFER) != 0, (g & G_LIGHTING) != 0); } }
void nd_mw_run(const uint32_t* r) { const unsigned k = (r[0] >> 16) & 0xffu, g = r[0] & 0xffffu; const uint32_t v = r[1]; const int mk = (int)(r[0] >> 31);
    const int same = k == ND_FS_MW_LIGHTCOL && g < 9u && lt[g].col[0] == (u8)(v >> 24) && lt[g].col[1] == (u8)(v >> 16) && lt[g].col[2] == (u8)(v >> 8);
    if (same && !mk) return; pm_gbi_cur_op = ND_FS_MW; if (mk) gv_epoch++; if (same) return;
    if (k == ND_FS_MW_NUMLIGHT) { lit_freeze(); lt_n = v; } else if (k == ND_FS_MW_LIGHTCOL && g < 9u) { lt[g].col[0] = (u8)(v >> 24); lt[g].col[1] = (u8)(v >> 16); lt[g].col[2] = (u8)(v >> 8); }
    else if (k == ND_FS_MW_FOG) { fog_fm = (s16)(v >> 16); fog_fo = (s16)v; } else if (k == ND_FS_MW_SEG && g < 16u) nd_ws.segbase[g] = v; }
void nd_tex_run(const uint32_t* r) { const unsigned on = r[0] & 0xffffffu, ss = r[1] >> 16, ts = r[1] & 0xffffu; const int mk = (int)(r[0] >> 31); /* NAT4J: TEXTURE, nr_set + nd_unit_apply */
    const int same = nd_ws.tex_on == (int)on && nd_ws.tex_ss == ss && nd_ws.tex_ts == ts && sc_ok && sc_sig_now() == sc_sig;
    if (same && !mk) return; pm_gbi_cur_op = ND_FS_TEXTURE; if (mk) gv_epoch++; if (same) return;
    nd_ws.tex_on = (int)on; nd_ws.tex_ss = ss; nd_ws.tex_ts = ts; nd_comb_sync(); }
void nd_omh_run(const uint32_t* r) { const uint32_t m = nd_op_om_mask(r[0] & 0xffffu), v = r[1]; const int mk = (int)(r[0] >> 31); /* NAT4J: OMH, nr_set + nd_unit_apply */
    const int same = (v & m) == (nd_ws.omode_h & m) && sc_ok; if (same) nd_ws.om_kh |= m;
    if (same && !mk) return; pm_gbi_cur_op = ND_FS_OMH; if (mk) gv_epoch++;
    if (!same) set_om(1, r[0] & 0xffu, (r[0] >> 8) & 0xffu, m, v); }
