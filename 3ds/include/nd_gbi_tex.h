/* nd_gbi_tex.h: DIRECT D4b redirect of the compound texture loads and the texture rectangles (plan_direct.md section 5). A file that includes
 * it after gbi.h records gDPLoadTLUT_pal16, gDPLoadTextureBlock_4b, gDPLoadTextureTile_4b, gDPLoadTextureTile, gSPTextureRectangle and
 * gSPScisTextureRectangle as frame stream commands (NDC_TEX4, NDC_TEXRECT) instead of list words. The packet is evaluated once; it must be
 * gMainGfxPos++ (nd_rec.c checks it). Inside a list for later (nd_rec_sub > 0) the words are written as before.
 * Converted files: src/msg_draw.c, src/hud_element.c, src/draw_box.c, src/draw_img_util.c. */
#ifndef ND_GBI_TEX_H
#define ND_GBI_TEX_H
#include "nd_cmd.h"

void nd_gx_tlut16(Gfx* g, unsigned slot, const void* dram);
void nd_gx_tex4(Gfx* g, const void* timg, unsigned fmt, unsigned w, unsigned h, int uls, int ult, int lrs, int lrt, unsigned pal,
                unsigned cms, unsigned cmt, unsigned masks, unsigned maskt, unsigned shifts, unsigned shiftt, unsigned tile_form);
void nd_gx_texrect(Gfx* g, int xl, int yl, int xh, int yh, unsigned tile, int s, int t, int dsdx, int dtdy);
void nd_gx_textile(Gfx* g, const void* timg, unsigned fmt, unsigned siz, unsigned w, unsigned h, int uls, int ult, int lrs, int lrt, unsigned pal,
                   unsigned cms, unsigned cmt, unsigned masks, unsigned maskt, unsigned shifts, unsigned shiftt);
void nd_gx_scisrect(Gfx* g, int xl, int yl, int xh, int yh, unsigned tile, int s, int t, int dsdx, int dtdy);

#undef gDPLoadTLUT_pal16
#define gDPLoadTLUT_pal16(pkt, pal, dram) nd_gx_tlut16((Gfx*)(pkt), (pal), (dram))
#undef gDPLoadTextureBlock_4b
#define gDPLoadTextureBlock_4b(pkt, timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt) \
    nd_gx_tex4((Gfx*)(pkt), (timg), (fmt), (width), (height), 0, 0, 0, 0, (pal), (cms), (cmt), (masks), (maskt), (shifts), (shiftt), 0)
#undef gDPLoadTextureTile_4b
#define gDPLoadTextureTile_4b(pkt, timg, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) \
    nd_gx_tex4((Gfx*)(pkt), (timg), (fmt), (width), (height), (uls), (ult), (lrs), (lrt), (pal), (cms), (cmt), (masks), (maskt), (shifts), (shiftt), 1)
#undef gDPLoadTextureTile
#define gDPLoadTextureTile(pkt, timg, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt)     nd_gx_textile((Gfx*)(pkt), (const void*)(timg), (fmt), (siz), (width), (height), (uls), (ult), (lrs), (lrt), (pal), (cms), (cmt), (masks), (maskt), (shifts), (shiftt))
#undef gSPScisTextureRectangle
#define gSPScisTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)     nd_gx_scisrect((Gfx*)(pkt), (xl), (yl), (xh), (yh), (tile), (s), (t), (dsdx), (dtdy))
#undef gSPTextureRectangle
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    nd_gx_texrect((Gfx*)(pkt), (xl), (yl), (xh), (yh), (tile), (s), (t), (dsdx), (dtdy))
#endif
