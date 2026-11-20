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
#include "nd_sv.h" /* NSET1 */

#define GP_SET(k, same) do { if (gp_on()) gp_note(k, (same)); } while (0)
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__) /* PG0: the flush counter by cause (prof mode) */
void pm_tex_set_filter(int linear);

/* ST-1: globals of the setters, cut from the former pm_gbi.c (writer: this module) */
int sci_x0, sci_y0, sci_x1 = 320, sci_y1 = 240;
int fog_fm, fog_fo, fogmix_on; /* F08: G_MW_FOG factor and offset, blender FOG/SHADE/IN/1MA active */
unsigned sr_sig;
int sr_ok;


/* Render mode bits of G_SETOTHERMODE_L, same reading as libultraship DeriveTriState: depth
 * compare and update come from Z_CMP and Z_UPD, a decal passes equal depth, blending is on when
 * the blender mixes with the frame buffer by (1 - alpha), texture edge and the alpha threshold
 * cut away near-transparent texels. */
unsigned fogc, blendc; /* V06: G_SETFOGCOLOR and G_SETBLENDCOLOR, RGBA */

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
    const int same = nd_ws.tex_on == (int)on && nd_ws.tex_ss == ss && nd_ws.tex_ts == ts; /* NWS1 */
    if (same && !mk) return; pm_gbi_cur_op = ND_FS_TEXTURE; if (mk) gv_epoch++; if (same) return;
    nd_ws.tex_on = (int)on; nd_ws.tex_ss = ss; nd_ws.tex_ts = ts; }
void nd_omh_run(const uint32_t* r) { const uint32_t m = nd_op_om_mask(r[0] & 0xffffu), v = r[1]; const int mk = (int)(r[0] >> 31); /* NAT4J: OMH, nr_set + nd_unit_apply */
    const int same = (v & m) == (nd_ws.omode_h & m); if (same) nd_ws.om_kh |= m;
    if (same && !mk) return; pm_gbi_cur_op = ND_FS_OMH; if (mk) gv_epoch++;
    if (!same) { const uint32_t sv[2] = { m, v & m }; nd_sv_omh(sv); } } /* NSET1: core 1 bit 30 is the cycle sync of set_om */
void nd_sv_nop(const uint32_t* r) { (void)r; } /* NSET1: LOADBLOCK, LOADTILE: no core 0 state */
void nd_sv_omh(const uint32_t* r) { const uint32_t m = r[0], tf = nd_ws.omode_h & (3u << 12); nd_ws.omode_h = (nd_ws.omode_h & ~m) | r[1]; nd_ws.om_kh |= m; /* NSET1: set_om H without the sync */
    if ((nd_ws.omode_h & (3u << 12)) != tf) { pm_gpu_flush(); pm_tex_set_filter((nd_ws.omode_h & (3u << 12)) != 0); } }
void nd_sv_oml(const uint32_t* r) { nd_ws.omode_l = (nd_ws.omode_l & ~r[0]) | r[1]; nd_ws.om_kl |= r[0]; } void nd_sv_geom(const uint32_t* r) { nd_ws.geom = (nd_ws.geom & r[0]) | r[1]; pm_gpu_set_mode((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0); }
void nd_sv_prim(const uint32_t* r) { GP_SET(1, nd_ws.prim == r[1] && (u8)nd_ws.prim_lod == (u8)r[0]); pm_gpu_set_prim(r[1], r[0]); nd_ws.prim = r[1]; nd_ws.prim_lod = r[0]; } void nd_sv_env(const uint32_t* r) { GP_SET(2, nd_ws.envc == r[1]); nd_ws.envc = r[1]; pm_gpu_set_env(nd_ws.envc); }
void nd_sv_fog(const uint32_t* r) { fogc = r[1]; } void nd_sv_blend(const uint32_t* r) { blendc = r[1]; } void nd_sv_fill(const uint32_t* r) { nd_ws.fill = r[1]; } void nd_sv_primz(const uint32_t* r) { nd_ws.prim_z = r[1]; { extern void pm_gpu_dbg_primz(unsigned z); pm_gpu_dbg_primz(nd_ws.prim_z); } }
void nd_sv_sci(const uint32_t* r) { sci_x0 = (int16_t)r[0]; sci_y0 = (int16_t)(r[0] >> 16); sci_x1 = (int16_t)r[1]; sci_y1 = (int16_t)(r[1] >> 16); pm_gpu_set_scissor(sci_x0, sci_y0, sci_x1, sci_y1); } void nd_sv_tex(const uint32_t* r) { nd_ws.tex_on = (int)r[0]; nd_ws.tex_ss = r[1] >> 16; nd_ws.tex_ts = r[1] & 0xffffu; }
void nd_sv_nlt(const uint32_t* r) { lit_freeze(); lt_n = r[1]; } void nd_sv_lcol(const uint32_t* r) { lt[r[0]].col[0] = (u8)(r[1] >> 24); lt[r[0]].col[1] = (u8)(r[1] >> 16); lt[r[0]].col[2] = (u8)(r[1] >> 8); } void nd_sv_fogf(const uint32_t* r) { fog_fm = (s16)(r[1] >> 16); fog_fo = (s16)r[1]; }
void nd_sv_seg(const uint32_t* r) { nd_ws.segbase[r[0]] = r[1]; } void nd_sv_conv(const uint32_t* r) { conv_k4 = r[1]; pm_comb_set_k4(conv_k4); } void nd_sv_keyr(const uint32_t* r) { keyc = (keyc & 0x00ffffffu) | r[0]; keys = (keys & 0x00ffffffu) | r[1]; }
void nd_sv_keygb(const uint32_t* r) { keyc = (keyc & 0xff000000u) | r[0]; keys = (keys & 0xff000000u) | r[1]; } void nd_sv_comb(const uint32_t* r) { nd_comb_set(r[0], r[1]); } /* NSET2: the setters of nd_unit_apply without the sync, on the core 1 values */
void nd_sv_apply(const uint32_t* s) { ((NdNapiRun)(uintptr_t)s[1])(s + 2); } /* NWS1: the sync of the old setter goes (NSET2 was the applier only) */
