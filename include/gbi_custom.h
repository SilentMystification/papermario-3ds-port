#ifndef _GBI_CUSTOM_H_
#define _GBI_CUSTOM_H_

#include "ultra64.h"

#define	gDPScrollMultiTile2_4b(pkt, timg, fmt, width, height,	\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt,\
    scrolls, scrollt)			\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width)>>1), timg);	\
	gDPSetTile(pkt, fmt, G_IM_SIZ_8b, 			        \
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		   shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(ult)<<(G_TEXTURE_IMAGE_FRAC),			\
			(lrs)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(lrt)<<(G_TEXTURE_IMAGE_FRAC));			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_RENDERTILE + 1, 0, cmt, maskt, shiftt, cms, masks,		\
		   shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE + 1,					\
			(uls + scrolls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult + scrollt)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs + scrolls)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt + scrollt)<<G_TEXTURE_IMAGE_FRAC)			\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_RENDERTILE + 0, 1, cmt, maskt, shiftt, cms, masks,		\
		   shifts);						\
}


#define	gDPScrollTextureTile_4b(pkt, timg, fmt, width, height,	\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt, \
    scrolls, scrollt)			\
							\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width)>>1), timg);	\
	gDPSetTile(pkt, fmt, G_IM_SIZ_8b, 			        \
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		   shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(ult)<<(G_TEXTURE_IMAGE_FRAC),			\
			(lrs)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(lrt)<<(G_TEXTURE_IMAGE_FRAC));			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_RENDERTILE, 0, cmt, maskt, shiftt, cms, masks,		\
		   shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE,					\
			(uls + scrolls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult + scrollt)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs + scrolls)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt + scrollt)<<G_TEXTURE_IMAGE_FRAC)			\


#define	gDPScrollTextureBlock_4b(pkt, timg, fmt, width, height,		\
		pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt)		\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_16b, 1, timg);		\
	gDPSetTile(pkt, fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0,	\
		cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0,				\
		(((width)*(height)+3)>>2)-1,				\
		CALC_DXT_4b(width)); 					\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0,	\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE,\
        (scrolls) << G_TEXTURE_IMAGE_FRAC,\
        (scrollt) << G_TEXTURE_IMAGE_FRAC,			\
		((width)-1 + scrolls) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1 + scrollt) << G_TEXTURE_IMAGE_FRAC)			\
}

#define	gDPScrollMultiTile_4b(pkt, timg, tmem, rtile, fmt, width, height,	\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt,\
    scrolls, scrollt)			\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width)>>1), timg);	\
	gDPSetTile(pkt, fmt, G_IM_SIZ_8b, 			        \
		   (((((lrs)-(uls)+1)>>1)+7)>>3), tmem,			\
		   G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		   shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(ult)<<(G_TEXTURE_IMAGE_FRAC),			\
			(lrs)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(lrt)<<(G_TEXTURE_IMAGE_FRAC));			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), tmem,			\
		   rtile, pal, cmt, maskt, shiftt, cms, masks,		\
		   shifts);						\
	gDPSetTileSize(pkt, rtile,					\
			((uls)<<G_TEXTURE_IMAGE_FRAC) + scrolls,			\
			((ult)<<G_TEXTURE_IMAGE_FRAC) + scrollt,			\
			((lrs)<<G_TEXTURE_IMAGE_FRAC) + scrolls,			\
			((lrt)<<G_TEXTURE_IMAGE_FRAC) + scrollt);      \
}

#define	gDPScrollMultiTile(pkt, timg, tmem, rtile, fmt, siz, width, height,\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt)			\
{									\
	gDPSetTextureImage(pkt, fmt, siz, width, timg);			\
	gDPSetTile(pkt, fmt, siz,					\
		(((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), tmem,	\
		G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC);			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, siz,					\
		(((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), tmem,	\
		rtile, pal, cmt, maskt, shiftt, cms, masks,		\
		shifts);						\
	gDPSetTileSize(pkt, rtile,					\
			((uls)<<G_TEXTURE_IMAGE_FRAC) + scrolls,			\
			((ult)<<G_TEXTURE_IMAGE_FRAC) + scrollt,			\
			((lrs)<<G_TEXTURE_IMAGE_FRAC) + scrolls,			\
			((lrt)<<G_TEXTURE_IMAGE_FRAC) + scrollt);      \
}

#define	gDPScrollTextureBlockHalfHeight(pkt, timg, fmt, siz, width, height,		\
		pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt, shifts2, shiftt2)		\
{									\
	gDPSetTextureImage(pkt, fmt, siz##_LOAD_BLOCK, 1, timg);	\
	gDPSetTile(pkt, fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 	\
		0 , cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0, 				\
		(((width)*(height) + siz##_INCR) >> siz##_SHIFT) -1,	\
		CALC_DXT(width, siz##_BYTES)); 				\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, siz,					\
		(((width) * siz##_LINE_BYTES)+7)>>3, 0,			\
		G_TX_RENDERTILE, pal, cmt, maskt - 1, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0,			\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		(((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC)			\
    gDPSetTile(pkt, fmt, siz,					\
		(((width) * siz##_LINE_BYTES)+7)>>3, \
        (((width) * ((height) >> 1) * siz##_LINE_BYTES)+7)>>3,			\
		G_TX_RENDERTILE + 1, pal, cmt, maskt - 1, shiftt2, cms, masks,	\
		shifts2);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE + 1,			\
        (scrolls),\
        (scrollt),			\
		(((width)-1) << G_TEXTURE_IMAGE_FRAC) + (scrolls),			\
		((((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC)  + (scrollt)	);		\
}

#define	gDPScrollTextureBlockHalfHeight_4b(pkt, timg, fmt, width, height,		\
		pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt, shifts2, shiftt2)		\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_16b, 1, timg);		\
	gDPSetTile(pkt, fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0,	\
		cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0,				\
		(((width)*(height)+3)>>2)-1,				\
		CALC_DXT_4b(width)); 					\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0,	\
		G_TX_RENDERTILE, pal, cmt, maskt - 1, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0,			\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		(((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC)			\
    gDPSetTile(pkt, fmt, G_IM_SIZ_4b,					\
		(((width)>>1)+7)>>3, \
        (((width) * ((height) >> 1) / 2)+7)>>3,			\
		G_TX_RENDERTILE + 1, pal, cmt, maskt - 1, shiftt2, cms, masks,	\
		shifts2);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE + 1,			\
        scrolls,\
        scrollt,			\
		(((width)-1) << G_TEXTURE_IMAGE_FRAC) + scrolls,			\
		((((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC)  + scrollt)		\
}


#define G_PM_REC 0x0D /* M4-2, S3F-4: producer record of a model (model.c), pm_gbi.c rec_run */
/* S4-1: producer record of one sprite component (sprite.c spr_appendGfx_component, TARGET_3DS): 4 Gfx words after the header, the old words follow.
  * w0 (header): G_PM_SPR << 24 | flags << 16 | alpha << 8 | offset (bits 0 to 7: words from the record to the final gSPVertex). w1: the render mode list (Gfx*).
  * word 1: w0 raster, w1 palette. word 2: w0 matrix (the matrixStack entry), w1 vertices (Quad*). word 3: w0 width << 16 | height. */
#define G_PM_SPR 0x0F
#define G_PM_SPR_WORDS 4
/* S6-1: producer record of one entity model draw (entity_model.c appendGfx_entity_model, TARGET_3DS): 4 Gfx words before the draw's old words.
 * w0 (header): G_PM_ENT << 24 | render mode index << 16 | fog bit 0. w1: the model display list (Gfx*).
 * word 1: w0 segment id (vertex segment, 0 when none), w1 physical vertex array (0 when none). word 2: w0 matrix flags, w1 the matrixStack entry (Mtx*).
 * word 3: w1 the render mode list (Gfx*, the G_DL the switch emitted, 0 when none). The old words still draw (the dispatch skips the record). */
#define G_PM_ENT 0x11
#define G_PM_ENT_WORDS 4
/* S6-2a: w0 bit 8 on a record written after the entity model display list (entcheck only): the walker checks its tile 0 state against nd_dlc_tex_decode of that list. */
#define G_PM_ENT_END 0x100u
/* S6-2c-3: w0 bit 9 on a record written right before the model display list G_DL (w1 = the list, word 1 as the first record). The walker draws the list natively from the static pools and skips that G_DL, or leaves it to run. */
#define G_PM_ENT_DRAW 0x200u
/* S6-3: the draw record of a static list drawn with its own G_DL (item entities): writes the 4 words at pkt, the caller advances gMainGfxPos by G_PM_ENT_WORDS. segid and varray: the vertex segment and its array (0 when the list holds direct pointers). */
#define gPmEntDrawRecord(pkt, list, segid, varray) do { \
    Gfx* _pmr = (pkt); \
    _pmr[0].words.w0 = ((u32)G_PM_ENT << 24) | (u32)G_PM_ENT_DRAW; _pmr[0].words.w1 = (u32)(uintptr_t)(list); \
    _pmr[1].words.w0 = (u32)(segid); _pmr[1].words.w1 = (u32)(varray); \
    _pmr[2].words.w0 = _pmr[2].words.w1 = _pmr[3].words.w0 = _pmr[3].words.w1 = 0; \
} while (0)
#define G_PM_REC_WORDS 14 /* S3F-2c: words of one producer record (R0 to R10). S3G-1: R11 custom gfx pre (w0) and post (w1), R12 geometry (w0) and pop flag (w1). S3G-2: R13 w0 = words after the record (special tile list or untextured ops), they run in place of the bind */

/* LS-4: G_PM_ND, the native-only payload class (no N64 encoding, plan_s10_path.md). One word: w0 = G_PM_ND << 24 | kind << 16 | arg, w1 unused.
 * G_PM_ND_SPLASH (arg 1 opens, 0 closes) brackets the water splash draws (water_splash.c). Between the brackets a colour buffer grab is one
 * full-height copy per frame (nd_target.c grab_cur). The walker reads it in nd_walk.c and calls nd_native_rec (nd_target.c). */
#define G_PM_ND 0x12
#define G_PM_ND_SPLASH 1
/* SBLK2 B1: G_PM_ND_GRABHOLD (arg 1 opens, 0 closes) brackets one motion_blur_flame draw (motion_blur_flame.c). Inside the bracket the first
 * colour buffer grab of the frame copies the frame and the later grabs of the same frame reuse that copy (nd_target.c grab_cur). */
#define G_PM_ND_GRABHOLD 2
/* PAUSEOPEN: G_PM_ND_GUTTER (arg 1 opens, 0 closes) brackets the two wide gutter loads of the pause picture (background_gfx.c). Inside it the pause
 * slot (the last frame's grab) is windowed 400 columns wide (pm_gpu.c pm_gpu_pause_win), so the loads find columns 0-39 and 360-399. */
#define G_PM_ND_GUTTER 3

/* DIRECT D11a: G_PM_LST, the start of a static state list recorded in line (nd_rec.c rec_inline). The replay runs it as the list start of the
 * G_DL it replaces (nd_walk.c: the state epoch, the entity skip mark consumed). w1 unused. */
#define G_PM_LST 0x13
#define gPmNdSplash(pkt, on) { Gfx *_g = (Gfx *)(pkt); _g->words.w0 = ((u32)G_PM_ND << 24) | ((u32)G_PM_ND_SPLASH << 16) | ((on) ? 1u : 0u); _g->words.w1 = 0; }
#define gPmNdGutter(pkt, on) { Gfx *_g = (Gfx *)(pkt); _g->words.w0 = ((u32)G_PM_ND << 24) | ((u32)G_PM_ND_GUTTER << 16) | ((on) ? 1u : 0u); _g->words.w1 = 0; }
#define gPmNdGrabHold(pkt, on) { Gfx *_g = (Gfx *)(pkt); _g->words.w0 = ((u32)G_PM_ND << 24) | ((u32)G_PM_ND_GRABHOLD << 16) | ((on) ? 1u : 0u); _g->words.w1 = 0; }

/* CORE2-3: a camera viewport (Camera.vp, Camera.vpAlt) is rewritten in place by the game. A list takes a copy made at list build
 * (pm_nusys.c pm_vp_snap, one pool per display context), so a walk of the list one frame later reads the value of its own frame. */
#ifdef TARGET_3DS
Vp* pm_vp_snap(const Vp* v);
#define PM_VP(v) pm_vp_snap(v)
#else
#define PM_VP(v) (v)
#endif

#endif
