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
