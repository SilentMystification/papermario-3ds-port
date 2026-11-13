/* nd_texwin.c: S7TEX1: the pure tile 0 window (tex_win0) and the descriptor of a tile load (nd_tex_win_desc), moved from nd_tex_set.c unchanged so the host table tool can link them. */
#include <stdint.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_int.h"
#include "nd_api.h"

/* D4a-1: the tile 0 window of a SetTileSize (source, size, stride): a pure function of the tile state t, the texture image timg and the
 * load model m (no global writes). set_tilesize applies it (td[0], the origin, the repeated-load skips). */
void tex_win0(const NdTst* t, const NdTexM* m, const void* timg, int uls, int ult, int lrs, int lrt, NdWin* o) {
    int w = ((lrs - uls) >> 2) + 1;
    int h = ((lrt - ult) >> 2) + 1;
    /* FX3f, PT37: a T wrap tile too (confetti: 16 rows of a 256 row sheet moved by ult). the RDP reads TMEM row (t - SetTileSize ult) wrapped by the T mask. A window shorter than the mask period (shape_spell:
     * 65 rows of a 256 row image, ult 256) reads rows outside the window, so the whole period must be bound. */
    if (t->tile_wrap[0][3] && !(t->tile_wrap[0][1] & 2) && m->ld_valid) {
        int per = 1 << t->tile_wrap[0][3];
        if (per > h && per <= 256 && m->ld_h >= per) h = per;
    }
    if (t->tile_wrap[0][2] && !(t->tile_wrap[0][0] & 2) && m->ld_valid) { /* PT09: a sprite sheet read through a window narrower than the mask period (emote icons): bind the loaded width up to the period */
        int per = 1 << t->tile_wrap[0][2];
        int ldw = t->tile_siz[0] == 0 ? m->ld_w * 2 : m->ld_w; /* VIS3: a 4 bit image loads through an 8 bit tile, m->ld_w counts bytes (walking dust: 88 for 176 texels) */
        if (per > w && per <= 256 && ldw > w) w = ldw < per ? ldw : per;
    }
    if (t->tile_wrap[0][2] && !(t->tile_wrap[0][0] & 2) && m->ld_valid) { const int per = 1 << t->tile_wrap[0][2]; const int ldw = t->tile_siz[0] == 0 ? m->ld_w * 2 : m->ld_w; if (per <= 256 && ldw >= per) w = per; }
    if (t->tile_wrap[0][3] && !(t->tile_wrap[0][1] & 2) && m->ld_valid) { const int per = 1 << t->tile_wrap[0][3]; if (per <= 256 && m->ld_h >= per) h = per; }
    /* TILEWIN2: a wrapping axis reads (s - sl) & mask from TMEM: one period is bound, whatever the window (effect_63, whirlwind) */
    int stride = t->timg_stride > w ? t->timg_stride : w;
    int bpp = t->tile_siz[0] >= 3 ? 4 : t->tile_siz[0] == 2 ? 2 : t->tile_siz[0] == 0 ? 0 : 1;
    const u8* src = (const u8*)timg;
    int r_x = m->ld_x, r_y = m->ld_y, r_str = t->timg_stride, r_v = m->ld_valid;
    /* V12c: the render tile shows what was loaded at its own TMEM address. A later load of another image at another
     * address (bulb glow: the frame buffer window into the upper half of TMEM, then SetTileSize of tile 0 for the
     * next strip) must not replace the image of tile 0. tex_win1 does the same lookup for tile 1. */
    for (unsigned k = 0; k < 4; k++) {
        if (t->ldrec[k].img && t->ldrec[k].tmem == t->tile_tmem[0] && t->ldrec[k].img != timg) {
            src = (const u8*)t->ldrec[k].img; r_x = t->ldrec[k].x; r_y = t->ldrec[k].y; r_str = t->ldrec[k].stride; r_v = 1;
            stride = r_str > w ? r_str : w;
        }
    }
    /* V12c: a tile window wider than the loaded image only works through the tile mask (it repeats or mirrors the image).
     * The source rows are as long as the loaded image, not as long as the window (bulb glow: 32 texel image, 65 texel window). */
    if (t->tile_wrap[0][2] && (1u << t->tile_wrap[0][2]) < (unsigned)w && r_str > 0 && r_str < w) stride = r_str;
    /* V05: the source window starts at the LoadTile origin. SetTileSize only says which texel coordinate that is
     * (draw_box loads a strip at row idx * 6 and sets the tile origin to (160 - w / 2, 0)). Both are the same
     * for gDPLoadTextureTile, so this changes no other draw. */
    if (src && bpp && r_v) src += (size_t)(r_y * stride + r_x) * (size_t)bpp;
    /* V13: no source offset for a block load. SetTileSize only moves the coordinates (the pan of a lava model). */
    else if (src && t->tile_siz[0] == 0 && r_v) {
        /* 4-bit strips (imgfx meshes) load a window of an 8-bit-wide image. The
         * tile size carries only the scroll, so the window comes from LoadTile. */
        src += (size_t)r_y * (size_t)r_str + (size_t)r_x;
        stride = r_str * 2;
    }
    o->src = src; o->w = w; o->h = h; o->stride = stride; o->x0 = uls >> 2;
}

/* NAPI2B: the descriptor of tile 0 after a gDPLoadTextureTile of the stated image, as set_tilesize and tex_win0 make it (the same function): the image
 * (timg, its row stride in texels of the load size), the tile format and size, the four wrap words, the LoadTile window (ld*: texel origin and size) and the
 * SetTileSize window (10.2). pal, pal_n: the palette for a CI tile (else NULL, 0). Returns 0 when set_tilesize would leave tile 0 invalid (empty window, no source). */
int nd_tex_win_desc(NdBindDesc* o, const void* timg, int timg_stride, unsigned fmt, unsigned siz, const unsigned wrap[4], int ldx, int ldy, int ldw, int ldh,
                    int uls, int ult, int lrs, int lrt, const void* pal, int pal_n) {
    NdTst t;
    NdTexM m;
    NdWin wn;
    memset(&t, 0, sizeof t);
    memset(&m, 0, sizeof m);
    t.timg_stride = timg_stride;
    t.tile_fmt[0] = fmt; t.tile_siz[0] = siz;
    memcpy(t.tile_wrap[0], wrap, sizeof t.tile_wrap[0]);
    m.ld_valid = 1; m.ld_x = ldx; m.ld_y = ldy; m.ld_w = ldw; m.ld_h = ldh;
    if (((lrs - uls) >> 2) + 1 <= 0 || ((lrt - ult) >> 2) + 1 <= 0) return 0;
    tex_win0(&t, &m, timg, uls, ult, lrs, lrt, &wn);
    if (!wn.src || wn.w <= 0 || wn.h <= 0 || (wn.h <= 1 && (unsigned)(uintptr_t)timg - (unsigned)(uintptr_t)nuGfxZBuffer < 320u * 240u * 2u)) return 0;
    o->src = wn.src; o->pal = pal; o->pal_n = pal_n; o->fmt = fmt; o->siz = siz;
    o->stride = wn.stride; o->x0 = wn.x0; o->y0 = ult >> 2; o->w = wn.w; o->h = wn.h;
    memcpy(o->wrap, wrap, sizeof o->wrap);
    o->sz[0] = uls; o->sz[1] = ult; o->sz[2] = lrs; o->sz[3] = lrt;
    return 1;
}

/* D4a-1: the tile 1 window of a SetTileSize (source, size, stride, origin column): pure, as tex_win0. */
void tex_win1(const NdTst* t, const NdTexM* m, const void* timg, int uls, int ult, int lrs, int lrt, NdWin* o) {
    int w = ((lrs - uls) >> 2) + 1, h = ((lrt - ult) >> 2) + 1;
    if (t->tile_wrap[1][2] && !(t->tile_wrap[1][0] & 2) && m->ld_valid) { /* VIS3: the same as PT09 for tile 1. The RDP reads (s - sl) through the mask, a mirrored or wrapped read of a window to the right of the image start (walking dust frame i + 1) needs the sheet from its start. */
        int per = 1 << t->tile_wrap[1][2];
        int ldw = t->tile_siz[1] == 0 ? m->ld_w * 2 : m->ld_w;
        if (per > w && per <= 256 && ldw > w) w = ldw < per ? ldw : per;
    }
    if (t->tile_wrap[1][2] && !(t->tile_wrap[1][0] & 2) && m->ld_valid) { const int per = 1 << t->tile_wrap[1][2]; const int ldw = t->tile_siz[1] == 0 ? m->ld_w * 2 : m->ld_w; if (per <= 256 && ldw >= per) w = per; }
    if (t->tile_wrap[1][3] && !(t->tile_wrap[1][1] & 2) && m->ld_valid) { const int per = 1 << t->tile_wrap[1][3]; if (per <= 256 && m->ld_h >= per) h = per; }
    /* TILEWIN2: a wrapping axis reads (s - sl) & mask from TMEM: one period is bound, whatever the window (effect_63, whirlwind) */
    int stride = t->timg_stride > w ? t->timg_stride : w;
    int bpp = t->tile_siz[1] >= 3 ? 4 : t->tile_siz[1] == 2 ? 2 : t->tile_siz[1] == 0 ? 0 : 1;
    const u8* src = (const u8*)timg;
    int l1x = m->ld_x, l1y = m->ld_y, l1stride = t->timg_stride, l1v = m->ld_valid;
    for (unsigned k = 0; k < 4; k++) {
        if (t->ldrec[k].img && t->ldrec[k].tmem == t->tile_tmem[1]) {
            src = (const u8*)t->ldrec[k].img; l1x = t->ldrec[k].x; l1y = t->ldrec[k].y; l1stride = t->ldrec[k].stride; l1v = 1;
            stride = l1stride > w ? l1stride : w;
        }
    }
    int t1x = uls >> 2; /* texel column of the window origin; the V21 branch lowers it to the loaded origin */
    int boff = 0; /* V13: tile 1 inside an earlier block load: the image start moves by the TMEM difference */
    for (unsigned k = 0; k < 4; k++)
        if (t->blk[k].img && t->blk[k].tmem < t->tile_tmem[1] && (t->tile_tmem[1] - t->blk[k].tmem) * 8u < t->blk[k].bytes && t->blk[k].tmem == t->tile_tmem[0] && src == (const u8*)t->blk[k].img)
            boff = (int)((t->tile_tmem[1] - t->blk[k].tmem) * 8u);
    /* The image offset is the LoadTile origin. SetTileSize uls and ult only place
     * it: gDPLoadMultiTile (draw_box corners) loads at 0,0 and moves tiles 1 to 3
     * to the far side of the window by SetTileSize. */
    if (src && bpp && l1v) src += (size_t)(l1y * stride + l1x) * (size_t)bpp;
    else if (src && bpp) src += (size_t)boff; /* V13: a block load puts the image at TMEM start, SetTileSize only moves the coordinates */
    else if (src && t->tile_siz[1] == 0 && !l1v) src += (size_t)boff; /* PT82: the same for a 4 bit image (gDPScrollTextureBlockHalfHeight_4b: tile 1 is the lower half of one block load, mim_darkgreen2tif) */
    else if (src && l1v && t->tile_wrap[1][2] && t->tile_wrap[1][3] &&
             w == (1 << t->tile_wrap[1][2]) && h == (1 << t->tile_wrap[1][3])) {
        /* WALLT1: the window is the whole mask period of the loaded image (aux pan: hos_01 walls; model aux offset: kmr_10 cliffs).
         * The RDP reads (s - sl) & mask from what LoadTile put in TMEM: the source is the LoadTile origin, and SetTileSize
         * only moves the coordinates (o->x0 and y0 in tex_tile1, as V13). Moving the source by the window origin read past the raster. */
        src += (size_t)(l1y * l1stride + l1x);
        stride = l1stride * 2;
    }
    else if (src && l1v && ((uls >> 2) >= m->ld_w * 2 || (ult >> 2) >= m->ld_h)) {
        /* COINSTAR: a window origin outside the loaded 4-bit image is a coordinate offset only (item icons in a shaded map: tile 1 at 256,256
         * over a 24 x 24 load, item_entity.c:990). The source is the LoadTile origin, as tile 0 (V13); x0 keeps uls >> 2. */
        src += (size_t)(l1y * l1stride + l1x);
        stride = l1stride * 2;
    }
    else if (src && l1v && t->tile_wrap[1][2] && t->tile_wrap[1][3] && (1 << t->tile_wrap[1][2]) <= m->ld_w * 2 && (1 << t->tile_wrap[1][3]) <= m->ld_h) {
        /* TILEWIN: a window inside one wrap period of a 4 bit image loaded whole (effect_63, fire_breath, whirlwind tile 1 scroll): bind the
         * whole period at the LoadTile origin; SetTileSize only moves the coordinates (x0 = uls >> 2, td[1].y0 = ult >> 2), as WALLT1.
         * Before, the source moved with the window: a new texture for every scroll step. */
        w = 1 << t->tile_wrap[1][2]; h = 1 << t->tile_wrap[1][3];
        src += (size_t)(l1y * l1stride + l1x); stride = l1stride * 2;
    }
    else if (src && l1v) {
        /* V21: SetTileSize uls and ult are the window origin in the image (fire_breath scrolls tile 1 by imgFrame*32+32).
         * This branch is 4 bit only (bpp 0). uls >> 2 counts texels, two per byte, so the byte offset is half of it.
         * A byte pointer cannot start on an odd texel: the loaded origin is the even texel at or before the window
         * origin. td[1].x0 uses the same origin, so the UV map and the loaded data agree. */
        t1x = (uls >> 2) & ~1;
        src += (size_t)(ult >> 2) * (size_t)l1stride + (size_t)(t1x >> 1);
        stride = l1stride * 2;
    }
    o->src = src; o->w = w; o->h = h; o->stride = stride; o->x0 = t1x;
}
