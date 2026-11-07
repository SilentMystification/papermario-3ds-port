/* nd_tex.c: native texture descriptors and unit words (plan_native_slices S1-6).
 * A descriptor names a texture the way the gDPLoadTexture* macros do (raster, palette, format, size, width, height, tile wrap and mask). nd_tex_get returns a block id.
 * The block is the 4 words C3Di_SetTex sends for the unit: dim, param, lodParam, data address >> 3 (pm_tex.c pm_tex_slot_words). The wrap rules and the
 * parameter bit math are moved from pm_tex.c (wrap_of, the final wrap of pm_tex_bind) and from citro3d (C3D_TexSetFilter, C3D_TexSetWrap).
 * The texture cache stays pm_tex.c: a miss loads the image there, then the words are read back. */
#include <stdint.h>
#include <string.h>
#include "nd.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"

/* the counters of PG0 (pm_gbi.c gp_note) and the pipeline flush by its line (pm_gbi.c macros) */
#define GP_SET(k, same) do { if (gp_on()) gp_note(k, (same)); } while (0)
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__)

/* ---- T globals owned by nd_tex.c (plan_s10_path D10: set by the texture setters here) ---- */
unsigned st0_ep; int st0_ok; /* PG1: tile 0 wrap state of the last G_SETTILE */
struct TszLast tsz_last;     /* PG1: the last tile 0 size record (ensure_tile in pm_gbi.c reads and writes it until TX-2) */
unsigned blk_n;              /* V13: block load slot rotation */
const void* tlut_pal[16];    /* TLUT per palette slot (set by G_LOADTLUT) */

/* GPU_TEXTURE_WRAP_PARAM values */
#define NW_CLAMP_EDGE 0u
#define NW_REPEAT 2u
#define NW_MIRROR 3u

/* the RDP tile wrap of one axis: mask 0 is clamp whatever the wrap field says; cm bit 1 clamps, bit 0 mirrors */
uint32_t nd_tex_wrap_of(unsigned cm, unsigned mask) {
    if (mask == 0 || (cm & 2)) return NW_CLAMP_EDGE;
    if (cm & 1) return NW_MIRROR;
    return NW_REPEAT;
}

/* the wrap a bind programs: a texture built only mask wide (shr) lets the GPU wrap do the mask, repeat or mirror by the tile flag */
uint32_t nd_tex_wrap_final(int shr, unsigned mirror, uint32_t wrap) {
    return shr ? (mirror ? NW_MIRROR : NW_REPEAT) : wrap;
}

/* C3D_TexSetFilter and C3D_TexSetWrap on a param word: mag filter bit 1, min filter bit 2, wrap s bits 12 to 13, wrap t bits 8 to 9 */
uint32_t nd_tex_param(uint32_t param, unsigned filter, uint32_t ws, uint32_t wt) {
    param &= ~((1u << 1) | (1u << 2) | (3u << 12) | (3u << 8));
    return param | (filter & 1u) << 1 | (filter & 1u) << 2 | (ws & 3u) << 12 | (wt & 3u) << 8;
}

/* C3D_TexInit: width in the high half, height in the low half */
uint32_t nd_tex_dim(unsigned w, unsigned h) { return (uint32_t)w << 16 | h; }

/* ---- descriptor table: 128 blocks, open hash with a 256 slot index ---- */
#define ND_TEX_N 128u
#define ND_TEX_IDX 256u
typedef struct { NdTexDesc d; uint32_t w[4]; int slot; unsigned key; } NdTexEnt; /* slot: the pm_tex.c slot the image was loaded to; key: its content identity */
static NdTexEnt tex_ent[ND_TEX_N];
static uint16_t tex_idx[ND_TEX_IDX]; /* entry id + 1, 0 = empty */
static unsigned tex_n;

int pm_tex_load(const void* img, unsigned fmt, unsigned siz, int width, int height, const void* tlut, int tlut_n, int stride); /* pm_tex.h */
void pm_tex_set_wrap(unsigned cms, unsigned cmt, unsigned masks, unsigned maskt);
void pm_tex_cur_slots(int* a, int* b);
unsigned pm_tex_slot_key(int i);
unsigned pm_tex_slot_gen(int i); /* S3N-6 */
int pm_tex_slot_unit(int i, NdUnit* u);
int pm_tex_slot_find(unsigned key, unsigned fmt, unsigned siz, uint32_t dim, const void* img, const void* tlut, NdUnit* u); /* pm_tex.c, S3N-3c, TEXSTALE */
int pm_tex_slot_clean(int i, const void* img, const void* tlut); /* pm_tex.c, TEXSTALE */
int pm_tex_slot_words(int i, uint32_t* out);
void pm_tex_unit0_param(uint32_t* ws, uint32_t* wt, unsigned* filter);
void pm_tex_stale_chk(int i, const void* img, const void* pal, unsigned key); /* pm_tex.c, TEXSTALE step 1 */

static unsigned desc_hash(const NdTexDesc* d) {
    const unsigned char* b = (const unsigned char*)d;
    unsigned h = 2166136261u;
    for (unsigned i = 0; i < sizeof *d; i++) h = (h ^ b[i]) * 16777619u;
    return h;
}

void nd_tex_reset(void) { memset(tex_idx, 0, sizeof tex_idx); tex_n = 0; }

int nd_tex_get(const NdTexDesc* in) {
    NdTexDesc d = *in;
    memset(d.pad, 0, sizeof d.pad);
    unsigned h = desc_hash(&d) & (ND_TEX_IDX - 1u);
    for (;;) {
        uint16_t s = tex_idx[h];
        if (s == 0) break;
        if (memcmp(&tex_ent[s - 1u].d, &d, sizeof d) == 0) return s - 1;
        h = (h + 1u) & (ND_TEX_IDX - 1u);
    }
    /* ponytail: table full returns -1, the caller falls back to the walker. Add eviction when S3 loads more than 128 textures in a map. */
    if (tex_n >= ND_TEX_N) return -1;
    /* miss: load through the texture cache, then read the unit words of the slot it bound */
    pm_tex_set_wrap(d.cms, d.cmt, d.masks, d.maskt);
    if (!pm_tex_load(d.img, d.fmt, d.siz, d.w, d.h, d.pal, d.pal_n, d.stride)) return -1;
    int a, b;
    pm_tex_cur_slots(&a, &b);
    NdTexEnt* e = &tex_ent[tex_n];
    if (!pm_tex_slot_words(a, e->w)) return -1;
    e->slot = a; e->key = pm_tex_slot_key(a);
    uint32_t ws, wt;
    unsigned filter;
    pm_tex_unit0_param(&ws, &wt, &filter);
    e->w[1] = nd_tex_param(e->w[1], filter, ws, wt); /* the bind sets filter and wrap on the C3D_Tex before the words are sent */
    e->d = d;
    tex_idx[h] = (uint16_t)(tex_n + 1u);
    return (int)tex_n++;
}

const uint32_t* nd_tex_block(int id) { return tex_ent[id].w; }

/* S3N-3c: the unit words of block id at draw time. The slot is found from the descriptor (its content key and dim), not from the walker-bound slot.
 * A miss (the texture cache dropped the image) loads it again from the descriptor, as nd_tex_get does. 0: no slot (fallback). */
int nd_tex_unit_hint(int id, int* hs, unsigned* hg, NdUnit* u) {
    NdTexEnt* e = &tex_ent[id];
    NdUnit t;
    if (!e->key) return 0;
    /* TEXSTALE: a slot is drawn only when clean for this descriptor (live, not dirty, same img and palette, as the walker lookup). */
    /* S3N-6: the hinted slot still holds the content (its key and generation) and gives the dim: its words, no search */
    if (*hs >= 0 && pm_tex_slot_clean(*hs, e->d.img, e->d.pal) && pm_tex_slot_key(*hs) == e->key && pm_tex_slot_gen(*hs) == *hg && pm_tex_slot_unit(*hs, &t) == 1 && t.dim == e->w[0]) {
        pm_tex_stale_chk(*hs, e->d.img, e->d.pal, e->key);
        u->dim = e->w[0]; u->param = e->w[1]; u->lod = t.lod; u->addr = t.addr; u->type = t.type; u->force = 0;
        return 1;
    }
    int s = pm_tex_slot_find(e->key, e->d.fmt, e->d.siz, e->w[0], e->d.img, e->d.pal, &t);
    if (s < 0) {
        /* a miss or a dirty copy: load through the walker (pm_tex_load validates a dirty slot by its hash), then take the key of the bytes now loaded */
        const NdTexDesc* d = &e->d;
        pm_tex_set_wrap(d->cms, d->cmt, d->masks, d->maskt);
        if (!pm_tex_load(d->img, d->fmt, d->siz, d->w, d->h, d->pal, d->pal_n, d->stride)) return 0;
        int a, b;
        pm_tex_cur_slots(&a, &b);
        if (a >= 0) e->key = pm_tex_slot_key(a);
        if (!e->key) return 0;
        s = pm_tex_slot_find(e->key, e->d.fmt, e->d.siz, e->w[0], e->d.img, e->d.pal, &t);
        if (s < 0) return 0;
    }
    /* force: the unit words are sent again when the slot or its generation changed (E: a rewritten slab at the same address is not seen by the shadow) */
    u->force = s != *hs || *hg != pm_tex_slot_gen(s);
    pm_tex_stale_chk(s, e->d.img, e->d.pal, e->key);
    *hs = s; *hg = pm_tex_slot_gen(s);
    u->dim = e->w[0]; u->param = e->w[1]; u->lod = t.lod; u->addr = t.addr; u->type = t.type;
    return 1;
}
int nd_tex_unit(int id, NdUnit* u) {
    int hs = -1; unsigned hg = 0;
    return nd_tex_unit_hint(id, &hs, &hg, u);
}

/* M4: the bodies of the texture ops, one function each. tex_op_impl calls them in its switch, tb_plain (fused bind of a plain texture list) calls them in list order. The code is the code of the cases, moved. */
/* CORE-2b batch D: texture image setter (decoded fields). */
static void set_timg(u32 addr, unsigned fmt, unsigned siz, int stride) {
    nd_ws.timg = ptr_of(addr);
    if (!addr && zslot_on && siz == 2) nd_ws.timg = zb_fake; /* B5: the underwater mesh reads the z-buffer slot */
    /* D03: a texture read from a frame buffer is the last frame. Fetch it now. */
    if (nd_ws.timg && pm_is_cfb(nd_ws.timg) && !pm_is_cur_cfb(nd_ws.timg) && !pm_interp_repl) pm_cfb_capture_prev(); /* F06-5: key pass only */ /* B2: the frame buffer being drawn is a grab (T2, grab_cur) */
    nd_ws.tst1.timg_fmt = fmt;
    nd_ws.tst1.timg_siz = siz;
    nd_ws.tst1.timg_stride = stride;
    ld_valid = 0;
}
void op_settimg(u32 w0, u32 w1) { set_timg(w1, (w0 >> 21) & 7, (w0 >> 19) & 3, (int)(w0 & 0xfff) + 1); }
/* CORE-2b batch D: tile setter (decoded fields; wr = the four wrap words, sh0 and sh1 the shifts). */
static void set_tile(unsigned tile, unsigned fmt, unsigned siz, unsigned tmem, unsigned pal, const unsigned* wr, unsigned sh0, unsigned sh1) {
    int same_tile = tile == 0 && st0_ok && pm_tex_wrap_gen() == st0_ep && nd_ws.tst1.tile_fmt[0] == fmt && nd_ws.tst1.tile_siz[0] == siz && nd_ws.tst1.tile_tmem[0] == tmem && nd_ws.tst1.tile_pal[0] == pal
                  && nd_ws.tst1.tile_wrap[0][0] == wr[0] && nd_ws.tst1.tile_wrap[0][1] == wr[1] && nd_ws.tst1.tile_wrap[0][2] == wr[2] && nd_ws.tst1.tile_wrap[0][3] == wr[3]
                  && nd_ws.tst1.tile_shift[0][0] == sh0 && nd_ws.tst1.tile_shift[0][1] == sh1;
    if (tile == 0) GP_SET(3, same_tile);
    nd_ws.tst1.tile_fmt[tile] = fmt;
    nd_ws.tst1.tile_siz[tile] = siz;
    nd_ws.tst1.tile_tmem[tile] = tmem;
    nd_ws.tst1.tile_pal[tile] = pal;
    nd_ws.tst1.tile_wrap[tile][0] = wr[0];
    nd_ws.tst1.tile_wrap[tile][1] = wr[1];
    nd_ws.tst1.tile_wrap[tile][2] = wr[2];
    nd_ws.tst1.tile_wrap[tile][3] = wr[3];
    nd_ws.tst1.tile_shift[tile][0] = sh0;
    nd_ws.tst1.tile_shift[tile][1] = sh1;
    /* The render tile's wrap belongs to the triangles already queued.
     * Draw them before this tile replaces it. */
    if (tile == 0 && !same_tile) { /* PG1: an identical G_SETTILE changes nothing: no flush, no wrap call, no new texture epoch */
        int f_on = pm_ph_on && sp0_cur == 1; unsigned f0 = f_on ? pm_ticks() : 0; /* SPLIT0C2: flush time in the tex kind */
        pm_gpu_flush();
        if (f_on) sp0_F += (unsigned)(pm_ticks() - f0);
        pm_tex_set_wrap(wr[0], wr[1], wr[2], wr[3]);
        st0_ep = pm_tex_wrap_gen(); st0_ok = 1; /* the wrap state pm_tex has now is the one of tile 0: an identical G_SETTILE may skip while nothing else touched pm_tex */
    }
}
void op_settile(u32 w0, u32 w1) {
    unsigned wr[4] = { (w1 >> 8) & 3, (w1 >> 18) & 3, (w1 >> 4) & 15, (w1 >> 14) & 15 };
    set_tile((w1 >> 24) & 7, (w0 >> 21) & 7, (w0 >> 19) & 3, w0 & 0x1ff, (w1 >> 20) & 15, wr, w1 & 15, (w1 >> 10) & 15);
}
/* CORE-2b batch D2: block load setter (decoded fields: tile and texel count). */
static void set_loadblock(unsigned tile, unsigned texels) {
            unsigned tm = nd_ws.tst1.tile_tmem[tile];
            {
                /* V13: the block also covers the TMEM above it (a half-height aux tile reads there) */
                unsigned words = ((texels + 1u) << nd_ws.tst1.tile_siz[tile]) >> 4;
                for (unsigned k = 0; k < 4; k++) if (nd_ws.tst1.ldrec[k].tmem >= tm && (nd_ws.tst1.ldrec[k].tmem == tm || nd_ws.tst1.ldrec[k].tmem < tm + words)) nd_ws.tst1.ldrec[k].img = NULL;
            }
            {
                unsigned k;
                for (k = 0; k < 4 && !(nd_ws.tst1.blk[k].img && nd_ws.tst1.blk[k].tmem == tm); k++) {}
                if (k == 4) k = blk_n++ & 3;
                nd_ws.tst1.blk[k].tmem = tm; nd_ws.tst1.blk[k].img = nd_ws.timg;
                nd_ws.tst1.blk[k].bytes = ((texels + 1u) << nd_ws.tst1.tile_siz[tile]) >> 1;
            }
}
void op_loadblock(u32 w0, u32 w1) { set_loadblock((w1 >> 24) & 7, (w1 >> 12) & 0xfff); }
/* CORE-2b batch D2: palette load setter (decoded fields: tile and entry count). */
static void set_loadtlut(unsigned tile, unsigned count) {
            nd_ws.tlut = nd_ws.timg;
            nd_ws.tlut_n = (int)(count) + 1;
            {
                /* The palette slot is the 16-entry block at the load tile's TMEM address. */
                unsigned slot = (nd_ws.tst1.tile_tmem[tile] - 256u) >> 4;
                if (slot < 16) {
                    tlut_pal[slot] = nd_ws.timg;
                    /* V20: gDPLoadTextureBlock_4b sets the tile size (which binds the texture) BEFORE the palette load. A bound CI tile that
                     * uses this palette bank must take the new palette, else it keeps the one of the previous model (the brick block was
                     * drawn with a dark grey palette). */
                    tlut_v20(slot);
                }
            }
}
void op_loadtlut(u32 w0, u32 w1) { set_loadtlut((w1 >> 24) & 7, (w1 >> 14) & 0x3ff); }
/* CORE-2b batch D: tile size setter (decoded fields). */
static __attribute__((noinline)) void set_tilesize(unsigned tile, int uls, int ult, int lrs, int lrt) {
            if (((lrs - uls) >> 2) + 1 <= 0 || ((lrt - ult) >> 2) + 1 <= 0) tex_win_s25(&uls, &ult, &lrs, &lrt);
            GP_SET(4, nd_ws.tst1.tile_sz[tile][0] == uls && nd_ws.tst1.tile_sz[tile][1] == ult && nd_ws.tst1.tile_sz[tile][2] == lrs && nd_ws.tst1.tile_sz[tile][3] == lrt);
            nd_ws.tst1.tile_sz[tile][0] = uls;
            nd_ws.tst1.tile_sz[tile][1] = ult;
            nd_ws.tst1.tile_sz[tile][2] = lrs;
            nd_ws.tst1.tile_sz[tile][3] = lrt;
            if (tile == 1) { tex_tile1(tile, uls, ult, lrs, lrt); sync_combine(); } /* S25: a plan that reads TEXEL1 needs tile 1 */
            if (tile != 0) return;
            nd_ws.td[0].valid = 0;
            nd_ws.cur_tile = -1; /* GP3: defer pm_tex_load to ensure_tile(0) at first draw */
            if (dual_on) { dual_on = 0; sync_combine(); } /* S25: the plan path is off while the shaded sprite is on */
            if (m1.valid && tex_shaded0(uls, ult)) return;
            m1.valid = 0;
            int x0 = uls >> 2;
            int y0 = ult >> 2;
            int w = ((lrs - uls) >> 2) + 1;
            int h = ((lrt - ult) >> 2) + 1;
            /* FX3f, PT37: a T wrap tile too (confetti: 16 rows of a 256 row sheet moved by ult). the RDP reads TMEM row (t - SetTileSize ult) wrapped by the T mask. A window shorter than the mask period (shape_spell:
             * 65 rows of a 256 row image, ult 256) reads rows outside the window, so the whole period must be bound. */
            if (nd_ws.tst1.tile_wrap[0][3] && !(nd_ws.tst1.tile_wrap[0][1] & 2) && ld_valid) {
                int per = 1 << nd_ws.tst1.tile_wrap[0][3];
                if (per > h && per <= 256 && ld_h >= per) h = per;
            }
            if (nd_ws.tst1.tile_wrap[0][2] && !(nd_ws.tst1.tile_wrap[0][0] & 2) && ld_valid) { /* PT09: a sprite sheet read through a window narrower than the mask period (emote icons): bind the loaded width up to the period */
                int per = 1 << nd_ws.tst1.tile_wrap[0][2];
                int ldw = nd_ws.tst1.tile_siz[0] == 0 ? ld_w * 2 : ld_w; /* VIS3: a 4 bit image loads through an 8 bit tile, ld_w counts bytes (walking dust: 88 for 176 texels) */
                if (per > w && per <= 256 && ldw > w) w = ldw < per ? ldw : per;
            }
            int stride = nd_ws.tst1.timg_stride > w ? nd_ws.tst1.timg_stride : w;
            int bpp = nd_ws.tst1.tile_siz[0] >= 3 ? 4 : nd_ws.tst1.tile_siz[0] == 2 ? 2 : nd_ws.tst1.tile_siz[0] == 0 ? 0 : 1;
            const u8* src = (const u8*)nd_ws.timg;
            int r_x = ld_x, r_y = ld_y, r_str = nd_ws.tst1.timg_stride, r_v = ld_valid;
            /* V12c: the render tile shows what was loaded at its own TMEM address. A later load of another image at another
             * address (bulb glow: the frame buffer window into the upper half of TMEM, then SetTileSize of tile 0 for the
             * next strip) must not replace the image of tile 0. Tile 1 does the same lookup below. */
            for (unsigned k = 0; k < 4; k++) {
                if (nd_ws.tst1.ldrec[k].img && nd_ws.tst1.ldrec[k].tmem == nd_ws.tst1.tile_tmem[0] && nd_ws.tst1.ldrec[k].img != nd_ws.timg) {
                    src = (const u8*)nd_ws.tst1.ldrec[k].img; r_x = nd_ws.tst1.ldrec[k].x; r_y = nd_ws.tst1.ldrec[k].y; r_str = nd_ws.tst1.ldrec[k].stride; r_v = 1;
                    stride = r_str > w ? r_str : w;
                }
            }
            /* V12c: a tile window wider than the loaded image only works through the tile mask (it repeats or mirrors the image).
             * The source rows are as long as the loaded image, not as long as the window (bulb glow: 32 texel image, 65 texel window). */
            if (nd_ws.tst1.tile_wrap[0][2] && (1u << nd_ws.tst1.tile_wrap[0][2]) < (unsigned)w && r_str > 0 && r_str < w) stride = r_str;
            /* V05: the source window starts at the LoadTile origin. SetTileSize only says which texel coordinate that is
             * (draw_box loads a strip at row idx * 6 and sets the tile origin to (160 - w / 2, 0)). Both are the same
             * for gDPLoadTextureTile, so this changes no other draw. */
            if (src && bpp && r_v) src += (size_t)(r_y * stride + r_x) * (size_t)bpp;
            /* V13: no source offset for a block load. SetTileSize only moves the coordinates (the pan of a lava model). */
            else if (src && nd_ws.tst1.tile_siz[0] == 0 && r_v) {
                /* 4-bit strips (imgfx meshes) load a window of an 8-bit-wide image. The
                 * tile size carries only the scroll, so the window comes from LoadTile. */
                src += (size_t)r_y * (size_t)r_str + (size_t)r_x;
                stride = r_str * 2;
            }
            suntile_t0(src, stride, w, h, bpp, uls, ult, lrs, lrt);
            nd_ws.tile_x0 = x0;
            nd_ws.tile_y0 = y0;
            nd_ws.tile_fx = (float)(uls & 3) * 0.25f; /* PT38 */
            nd_ws.tile_fy = (float)(ult & 3) * 0.25f;
            /* PG1: the same texture again (same source, format, size, palette, stride) and nothing touched the texture state since the last load (pm_tex_epoch):
             * the load would find the same cache slot. Skip the flush and the load, the batch in flight samples the same image. A frame buffer image is
             * excluded (its content changes under the same pointer, grab_cur). */
            {
                struct TszLast* lp = &tsz_last;
                const void* palc = tlut_pal[nd_ws.tst1.tile_pal[0] & 15] ? tlut_pal[nd_ws.tst1.tile_pal[0] & 15] : nd_ws.tlut;
                int ggx = 0, ggy = 0;
                if (src && w > 0 && h > 0 && nd_ws.tst1.tile_fmt[0] == 2 && nd_ws.tst1.tile_siz[0] == 0 &&
                    pm_tex_glyph_same(src, nd_ws.tst1.tile_fmt[0], nd_ws.tst1.tile_siz[0], w, h, palc, nd_ws.tlut_n, &ggx, &ggy)) {
                    nd_ws.tile_x0 = x0 - ggx; /* same sign as ensure_tile's new-bind path (x0 - gox) */
                    nd_ws.tile_y0 = y0 - ggy;
                    nd_ws.tile_fx = (float)(uls & 3) * 0.25f;
                    nd_ws.tile_fy = (float)(ult & 3) * 0.25f;
                    nd_ws.td[0].valid = 1; nd_ws.td[0].src = src; nd_ws.td[0].fmt = nd_ws.tst1.tile_fmt[0]; nd_ws.td[0].siz = nd_ws.tst1.tile_siz[0]; nd_ws.td[0].w = w; nd_ws.td[0].h = h; nd_ws.td[0].stride = stride;
                    nd_ws.td[0].x0 = x0; nd_ws.td[0].y0 = y0; nd_ws.td[0].pal = palc; nd_ws.td[0].pal_n = nd_ws.tlut_n;
                    gp3_t0_set(); GP_SET(7, 1);
                    return;
                }
                if (lp->ok && src == lp->src && nd_ws.tst1.tile_fmt[0] == lp->fmt && nd_ws.tst1.tile_siz[0] == lp->siz && w == lp->w && h == lp->h && stride == lp->stride &&
                    palc == lp->pal && nd_ws.tlut_n == lp->pal_n && pm_tex_epoch() == lp->ep && !pm_is_cfb(src) && src && w > 0 && h > 1) {
                    nd_ws.tile_x0 = x0 - lp->ox;
                    nd_ws.tile_y0 = y0 - lp->oy;
                    nd_ws.td[0].valid = 1; nd_ws.td[0].src = src; nd_ws.td[0].fmt = nd_ws.tst1.tile_fmt[0]; nd_ws.td[0].siz = nd_ws.tst1.tile_siz[0]; nd_ws.td[0].w = w; nd_ws.td[0].h = h; nd_ws.td[0].stride = stride;
                    nd_ws.td[0].x0 = x0; nd_ws.td[0].y0 = y0; nd_ws.td[0].pal = palc; nd_ws.td[0].pal_n = nd_ws.tlut_n;
                    gp3_t0_set(); GP_SET(7, 1);
                    return;
                }
                GP_SET(7, 0);
                lp->ok = 0;
                lp->src = src; lp->fmt = nd_ws.tst1.tile_fmt[0]; lp->siz = nd_ws.tst1.tile_siz[0]; lp->w = w; lp->h = h; lp->stride = stride; lp->pal = palc; lp->pal_n = nd_ws.tlut_n;
            }
            /* GP3: pm_tex_load runs in ensure_tile(0) at the first surviving draw. */
            /* F03: a 1 row tile of the N64 z-buffer is a depth query source (pm_depth_query), not a texture (the pause background loads tall z-buffer tiles, those are textures) */
            if (src && w > 0 && h > 0 && (h > 1 || (unsigned)nd_ws.timg - (unsigned)nuGfxZBuffer >= 320u * 240u * 2u)) {
                const void* pal0 = tlut_pal[nd_ws.tst1.tile_pal[0] & 15] ? tlut_pal[nd_ws.tst1.tile_pal[0] & 15] : nd_ws.tlut;
                nd_ws.td[0].valid = 1;
                nd_ws.td[0].src = src;
                nd_ws.td[0].fmt = nd_ws.tst1.tile_fmt[0];
                nd_ws.td[0].siz = nd_ws.tst1.tile_siz[0];
                nd_ws.td[0].w = w;
                nd_ws.td[0].h = h;
                nd_ws.td[0].stride = stride;
                nd_ws.td[0].x0 = x0;
                nd_ws.td[0].y0 = y0;
                nd_ws.td[0].pal = pal0;
                nd_ws.td[0].pal_n = nd_ws.tlut_n;
                gp3_t0_set();
            }
            return;
}
__attribute__((noinline)) void op_settilesize(u32 w0, u32 w1) {
    set_tilesize((w1 >> 24) & 7, (int)((w0 >> 12) & 0xfff), (int)(w0 & 0xfff), (int)((w1 >> 12) & 0xfff), (int)(w1 & 0xfff));
}
/* CORE-2b batch D2: texture enable and scale setter (decoded fields). */
static void set_texture(unsigned on, unsigned ss, unsigned ts) {
    nd_ws.tex_on = on;
    nd_ws.tex_ss = ss;
    nd_ws.tex_ts = ts;
}
void op_texture(u32 w0, u32 w1) { set_texture((w0 & 0xff) != 0, w1 >> 16, w1 & 0xffffu); }
