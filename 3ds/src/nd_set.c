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
extern int pm_wide; /* WIDE4: set_scissor reads signed x edges with wide on */

#define GP_SET(k, same) do { if (gp_on()) gp_note(k, (same)); } while (0)
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__) /* PG0: the flush counter by cause (prof mode) */
void pm_tex_set_filter(int linear);

/* ST-1: globals of the setters, cut from the former pm_gbi.c (writer: this module) */
int sci_x0, sci_y0, sci_x1 = 320, sci_y1 = 240;
int fog_fm, fog_fo, fogmix_on; /* F08: G_MW_FOG factor and offset, blender FOG/SHADE/IN/1MA active */
unsigned sr_sig;
int sr_ok;
unsigned mw_cnt[256];


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

/* CORE-2b: G_SETPRIMCOLOR, one owner (the walker switch and the record both run it). */
void op_setprimcolor(u32 w0, u32 w1) {
    GP_SET(1, nd_ws.prim == w1 && (u8)nd_ws.prim_lod == (u8)(w0 & 0xff));
    pm_gpu_set_prim(w1, w0 & 0xffffu); /* closes the open batch before the change (SPR4-prim) */
    nd_ws.prim = w1;
    nd_ws.prim_lod = w0 & 0xffffu; /* S3N-3d: the prim LOD word (pm_gpu key) */
    sync_combine();
}

void op_setothermode(unsigned op, u32 w0, u32 w1) {
            unsigned len = (w0 & 0xff) + 1;
            unsigned field = (w0 >> 8) & 0xff;
            unsigned sft = 32 - field - len;
            unsigned mask = (len >= 32) ? 0xffffffffu : (((1u << len) - 1u) << sft);
            if (op == G_SETOTHERMODE_H) {
                unsigned old_tf = nd_ws.omode_h & (3u << 12);
                nd_ws.omode_h = (nd_ws.omode_h & ~mask) | (w1 & mask);
                nd_ws.om_kh |= mask; /* B8 known bits */
                if ((nd_ws.omode_h & (3u << 12)) != old_tf) { /* V15: G_MDSFT_TEXTFILT, POINT is 0, BILERP 2, AVERAGE 3 */
                    pm_gpu_flush();
                    pm_tex_set_filter((nd_ws.omode_h & (3u << 12)) != 0);
                }
            }
            else { nd_ws.omode_l = (nd_ws.omode_l & ~mask) | (w1 & mask); nd_ws.om_kl |= mask; }
            /* S25: the cycle type decides 1 or 2 cycles of the combiner. */
            if (op == G_SETOTHERMODE_H && sft <= G_MDSFT_CYCLETYPE && sft + len > G_MDSFT_CYCLETYPE) sync_combine();
}

/* S3F-2b: G_SETENVCOLOR, one function for the walker and the record. */
void gbi_setenv(u32 w1) {
    GP_SET(2, nd_ws.envc == w1);
    nd_ws.envc = w1;
    pm_gpu_set_env(nd_ws.envc);
    sync_combine();
}

void gbi_moveword(u32 w0, u32 w1) {
    unsigned index = (w0 >> 16) & 0xff;
    unsigned offset = w0 & 0xffff;
    if (index == G_MW_NUMLIGHT) { nd_light_count(w1); return; } /* L01 */
    if (index == G_MW_LIGHTCOL && offset / 24 < 9 && (offset % 24) == 0) { unsigned k = offset / 24; lt[k].col[0] = w1 >> 24; lt[k].col[1] = w1 >> 16; lt[k].col[2] = w1 >> 8; return; } /* L01 */
    if (index == G_MW_FOG) { fog_fm = (s16)(w1 >> 16); fog_fo = (s16)w1; return; } /* F08 */
    if (index != G_MW_SEGMENT) {
        unsigned n = ++mw_cnt[index];
    }
    if (index == G_MW_SEGMENT && (offset / 4) < 16) {
        unsigned seg = offset / 4;
        nd_ws.segbase[seg] = (u32)ptr_of(w1);
        if (!nd_ws.segbase[seg]) nd_ws.segbase[seg] = w1;
    }
}

/* V06 rule, one owner (sync_render and the native model state, pm_gpu.c model_draw): the first blender cycle P * A_FOG + IN * (1 - A_FOG), P = BL or FOG.
 * 1 with *col = the mix colour (RGB of P, alpha = the fog colour alpha), else 0. */
int nd_mix1(unsigned ol, unsigned* col) {
    unsigned p = (ol >> 30) & 3u;
    if ((p != 2u && p != 3u) || ((ol >> 26) & 3u) != 1u || ((ol >> 22) & 3u) != 0u || ((ol >> 18) & 3u) != 0u) return 0;
    *col = ((p == 2u ? blendc : fogc) & 0xffffff00u) | (fogc & 0xffu);
    return 1;
}
void nd_sr_forget(void) { sr_ok = 0; }
void sync_render(void) {
    unsigned sg = nd_ws.omode_l ^ (nd_ws.omode_h * 2654435761u) ^ nd_ws.geom ^ (unsigned)img_role(0);
    if (sr_ok && sg == sr_sig) {
        GP_SET(6, 1);
        return;
    }
    sr_sig = sg;
    sr_ok = 1;
    nd_ent_rfull += nd_ent_in;
    GP_SET(6, 0);
    unsigned m = nd_ws.omode_l;
    /* V06: the first blender cycle mixes the colour with the blend colour (m1a 2) or the fog colour (m1a 3) by the fog
     * colour alpha: (P * A + M * (1 - A)), P = BL or FOG, A = A_FOG, M = IN, B = 1MA. The room shroud (PM_RM_SHROUD) fades the
     * world to black this way. Other blenders are not touched. */
    {
        unsigned p = (m >> 30) & 3u, a = (m >> 26) & 3u, mm = (m >> 22) & 3u, b = (m >> 18) & 3u, mixc;
        if (nd_mix1(m, &mixc))
            pm_gpu_set_mix(1, mixc);
        else if (!PMDL("nofog") && p == 3u && a == 2u && mm == 0u && b == 0u) {
            /* F08: G_RM_FOG_SHADE_A, the weight is the shade alpha (the vertex fog factor, see draw_tri) */
            pm_gpu_set_mix(2, (fogc & 0xffffff00u) | 0xffu);
            fogmix_on = 1;
            goto mixed;
        } else
            pm_gpu_set_mix(0, 0);
        fogmix_on = 0;
    }
mixed:
    NdRender rs;
    nd_render_state(nd_ws.omode_h, m, img_role(0) == ROLE_DEPTH, &rs);
    pm_gpu_set_nocolor(rs.nocol);
    if (rs.set_memb) pm_gpu_set_memblend(rs.memb);
    pm_gpu_set_render(rs.zcmp, rs.zupd, rs.blend, rs.atest, rs.decal);
    pm_gpu_dbg_mode(m); /* FX1-2b: orbdbg log of the glow blend, set after the flush of the old state */
    if (m == 0x00504B50u && PMDL("orbdbg")) { /* FX1-2b: the render state the glow's draws get, once per frame */
        static unsigned orb_sf = ~0u;
        extern unsigned pm_frame_count(void);
        extern void pm_log(const char* fmt, ...);
        if (orb_sf != pm_frame_count()) {
            orb_sf = pm_frame_count();
            pm_log("orbdbg state f%u blend %d atest %d zcmp %d zupd %d decal %d memb %d nocol %d mix %d fogmix %d", orb_sf, rs.blend, rs.atest,
                   rs.zcmp, rs.zupd, rs.decal, rs.memb, rs.nocol, (int)0, (int)fogmix_on);
        }
    }
    pm_gpu_render_src(nd_ws.omode_h, m, rs.depth);
}
