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
/* owner: nd_tex.c. The bind of tile n for the next draw (ensure_tile, the ens_ state the cache skip reads) */
const void* ens_img[2];
const void* ens_src;
const void* ens_pal;
unsigned ens_fmt, ens_siz;
unsigned ens_wrap[4];
int ens_sz[4];

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

void unpack8(unsigned c, float o[4]) {
    o[0] = pm_f255[(c >> 24) & 255u];
    o[1] = pm_f255[(c >> 16) & 255u];
    o[2] = pm_f255[(c >> 8) & 255u];
    o[3] = pm_f255[c & 255u];
}

void pal_rgba(const void* base, unsigned i, float o[4]) {
    const u8* p = (const u8*)base;
    unsigned c = 0;
    if (p) {
        p += (i & 15u) * 2u;
        c = ((unsigned)p[0] << 8) | p[1];
    }
    float r = (float)((c >> 11) & 31u) / 31.f, g = (float)((c >> 6) & 31u) / 31.f,
          b = (float)((c >> 1) & 31u) / 31.f;
    o[0] = r; o[1] = g; o[2] = b; o[3] = (c & 1u) ? 1.f : 0.f;
}

const u8* fb_prev_base(int n) {
    const u8* base;
    unsigned i;
    if (pm_wide || !nd_ws.td[n].valid || nd_ws.td[n].fmt != 0 || nd_ws.td[n].siz != 2 || nd_ws.td[n].stride != 320) return NULL;
    if (nd_ws.tst1.tile_wrap[n][2] || nd_ws.tst1.tile_wrap[n][3]) return NULL;
    base = (const u8*)nd_ws.td[n].src - ((size_t)nd_ws.td[n].y0 * 320u + (size_t)nd_ws.td[n].x0) * 2u;
    for (i = 0; i < nuGfxCfbNum; i++)
        if ((const u8*)nuGfxCfb[i] == base) break;
    if (i == nuGfxCfbNum || pm_is_cur_cfb(base)) return NULL;
    return base;
}

void ens_clear(void) { ens_src = NULL; ens_pal = NULL; }

int tile_bound(int n) {
    if (!nd_ws.td[n].valid || pm_tex_cur_idx() < 0 || !ens_src) return 0;
    if (ens_src != nd_ws.td[n].src || ens_pal != nd_ws.td[n].pal || ens_fmt != nd_ws.td[n].fmt || ens_siz != nd_ws.td[n].siz) return 0;
    if (memcmp(nd_ws.tst1.tile_wrap[n], ens_wrap, sizeof ens_wrap) || memcmp(nd_ws.tst1.tile_sz[n], ens_sz, sizeof ens_sz)) return 0;
    return 1;
}

int tile_texel(int n, int s, int t, unsigned char o[4]) {
    const u8* p = nd_ws.td[n].src;
    int w = nd_ws.td[n].w, h = nd_ws.td[n].h;
    if (n == 0) {
        s %= w; if (s < 0) s += w;
        t %= h; if (t < 0) t += h;
    } else {
        if (s < 0) s = 0; else if (s >= w) s = w - 1;
        if (t < 0) t = 0; else if (t >= h) t = h - 1;
    }
    size_t i = (size_t)t * (size_t)nd_ws.td[n].stride + (size_t)s;
    unsigned c;
    switch (nd_ws.td[n].fmt * 4 + nd_ws.td[n].siz) {
    case 4 * 4 + 0: { u8 b = p[i >> 1]; unsigned v = ((i & 1) ? (b & 15u) : (b >> 4)) * 17u; o[0] = o[1] = o[2] = o[3] = (u8)v; return 1; }
    case 4 * 4 + 1: o[0] = o[1] = o[2] = o[3] = p[i]; return 1;
    case 3 * 4 + 0: { u8 b = p[i >> 1]; unsigned v = (i & 1) ? (b & 15u) : (b >> 4); o[0] = o[1] = o[2] = (u8)((v >> 1) * 36u); o[3] = (v & 1u) ? 255 : 0; return 1; }
    case 3 * 4 + 1: o[0] = o[1] = o[2] = (u8)((p[i] >> 4) * 17u); o[3] = (u8)((p[i] & 15u) * 17u); return 1;
    case 3 * 4 + 2: o[0] = o[1] = o[2] = p[i * 2]; o[3] = p[i * 2 + 1]; return 1;
    case 0 * 4 + 2: c = ((unsigned)p[i * 2] << 8) | p[i * 2 + 1]; break;
    case 2 * 4 + 0: case 2 * 4 + 1: {
        unsigned idx;
        if (!nd_ws.td[n].pal) return 0;
        if (nd_ws.td[n].siz == 0) { u8 b = p[i >> 1]; idx = (i & 1) ? (b & 15u) : (b >> 4); } else idx = p[i];
        c = ((unsigned)((const u8*)nd_ws.td[n].pal)[idx * 2] << 8) | ((const u8*)nd_ws.td[n].pal)[idx * 2 + 1];
        break;
    }
    default: return 0;
    }
    o[0] = (u8)(((c >> 11) & 31u) * 255u / 31u);
    o[1] = (u8)(((c >> 6) & 31u) * 255u / 31u);
    o[2] = (u8)(((c >> 1) & 31u) * 255u / 31u);
    o[3] = (c & 1u) ? 255 : 0;
    return 1;
}

void ensure_tile(int n) {
    if (!nd_ws.td[n].valid) return;
    nd_ws.tile_fx = (float)(nd_ws.tst1.tile_sz[n][0] & 3) * 0.25f; /* PT38 */
    nd_ws.tile_fy = (float)(nd_ws.tst1.tile_sz[n][1] & 3) * 0.25f;
    { /* FB1 debug switch fbbind: one line per 16 bit RGBA bind (source, window, size); pm_tex.c load_impl adds hit or miss */
        static int fbon = -1;
        if (fbon < 0) fbon = pm_debug_has("fbbind");
        if (fbon && nd_ws.td[n].fmt == 0 && nd_ws.td[n].siz == 2) pm_log_d("fbbind n%d src %p xy %d,%d wh %dx%d st %d", n, (const void*)nd_ws.td[n].src, nd_ws.td[n].x0, nd_ws.td[n].y0, nd_ws.td[n].w, nd_ws.td[n].h, nd_ws.td[n].stride);
    }
    {   /* FB1: a clamp window of the previous colour buffer (the transition strips, 6 rows each) samples one texture of the whole 320 x 240
         * buffer. The key is the buffer, so every strip after the first is a bind hit (no flush, no load). Texels are absolute: tile_x0 = tile_y0 = 0. */
        const u8* fb = fb_prev_base(n);
        if (fb) {
            static const int fb_sz[4] = { -1, -1, -1, -1 }; /* a window size no SetTileSize writes: tile_bound never matches it */
            if (ens_src == fb && ens_pal == nd_ws.td[n].pal && ens_fmt == 0 && ens_siz == 2 && pm_tex_cur_idx() >= 0 &&
                !memcmp(ens_sz, fb_sz, sizeof ens_sz) && !memcmp(nd_ws.tst1.tile_wrap[n], ens_wrap, sizeof ens_wrap)) {
                nd_ws.tile_x0 = nd_ws.tile_y0 = 0;
                nd_ws.cur_tile = n;
                return;
            }
            pm_gpu_flush();
            pm_tex_set_wrap(nd_ws.tst1.tile_wrap[n][0], nd_ws.tst1.tile_wrap[n][1], nd_ws.tst1.tile_wrap[n][2], nd_ws.tst1.tile_wrap[n][3]);
            tex_gen++; /* TBREP: a load changes TMEM */
            pm_tex_load(fb, 0, 2, 320, 240, nd_ws.td[n].pal, nd_ws.td[n].pal_n, 320);
            { /* FB1 debug switch fbbind: the bind as the texture layer gives it: size (pm_tex_size), source, slot (off) or CPU texture (cur) */
                static int fbon = -1;
                int tw1 = 0, th1 = 0;
                if (fbon < 0) fbon = pm_debug_has("fbbind");
                if (fbon) { pm_tex_size(&tw1, &th1); pm_log_d("fbbind bind n%d fb %p fmt RGBA16 size %dx%d off %d cur %d", n, (const void*)fb, tw1, th1, pm_tex_off_idx(), pm_tex_cur_idx()); }
            }
            nd_ws.tile_x0 = nd_ws.tile_y0 = 0;
            ens_src = fb; ens_pal = nd_ws.td[n].pal; ens_fmt = 0; ens_siz = 2;
            memcpy(ens_wrap, nd_ws.tst1.tile_wrap[n], sizeof ens_wrap);
            memcpy(ens_sz, fb_sz, sizeof ens_sz);
            ens_img[n] = fb;
            nd_ws.cur_tile = n;
            return;
        }
    }
    if (n == 0 && !tile_bound(0) && glyph_atlas_bound()) {
        nd_ws.cur_tile = 0;
        return;
    }
    if (tile_bound(n)) {
        nd_ws.cur_tile = n;
        return;
    }
    pm_gpu_flush();
    pm_tex_set_wrap(nd_ws.tst1.tile_wrap[n][0], nd_ws.tst1.tile_wrap[n][1], nd_ws.tst1.tile_wrap[n][2], nd_ws.tst1.tile_wrap[n][3]);
    if (nd_ws.td[n].fmt == 0 && nd_ws.td[n].siz == 2) grab_cur(nd_ws.td[n].src, nd_ws.td[n].w, nd_ws.td[n].h);
    tex_gen++; /* TBREP: a load changes TMEM */
    pm_tex_load(nd_ws.td[n].src, nd_ws.td[n].fmt, nd_ws.td[n].siz, nd_ws.td[n].w, nd_ws.td[n].h, nd_ws.td[n].pal, nd_ws.td[n].pal_n, nd_ws.td[n].stride);
    {   /* V05: a window of an off-screen texture starts at (ox, oy) in it */
        int ox, oy, gox, goy;
        pm_tex_origin(&ox, &oy);
        pm_tex_glyph_ofs(&gox, &goy);
        nd_ws.tile_x0 = nd_ws.td[n].x0 - ox - (n == 0 ? gox : 0);
        nd_ws.tile_y0 = nd_ws.td[n].y0 - oy - (n == 0 ? goy : 0);
        if (n == 0) {
            tsz_last.ox = ox - gox;
            tsz_last.oy = oy - goy;
            tsz_last.ep = pm_tex_epoch();
            tsz_last.ok = pm_tex_off_idx() < 0;
        }
    }
    ens_src = nd_ws.td[n].src;
    ens_pal = nd_ws.td[n].pal;
    ens_fmt = nd_ws.td[n].fmt;
    ens_siz = nd_ws.td[n].siz;
    memcpy(ens_wrap, nd_ws.tst1.tile_wrap[n], sizeof ens_wrap);
    memcpy(ens_sz, nd_ws.tst1.tile_sz[n], sizeof ens_sz);
    ens_img[n] = nd_ws.td[n].src;
    nd_ws.cur_tile = n;
}

__attribute__((noinline)) void tex_tile1(unsigned tile, int uls, int ult, int lrs, int lrt) {
            if (tile == 1 && nd_ws.tst1.tile_fmt[1] == 2 && nd_ws.tst1.tile_siz[1] == 0 && ld_valid && nd_ws.timg) {
                /* Second tile on the same 4-bit image: remember the raster for the shaded sprite. */
                m1.valid = 1;
                m1.src = (const u8*)nd_ws.timg + (size_t)ld_y * (size_t)nd_ws.tst1.timg_stride + (size_t)ld_x;
                m1.stride = nd_ws.tst1.timg_stride * 2;
                m1.w = ((lrs - uls) >> 2) + 1;
                m1.h = ((lrt - ult) >> 2) + 1;
                m1.x0 = uls >> 2;
                m1.y0 = ult >> 2;
            }
            if (tile == 1) {
                int w = ((lrs - uls) >> 2) + 1, h = ((lrt - ult) >> 2) + 1;
                if (nd_ws.tst1.tile_wrap[1][2] && !(nd_ws.tst1.tile_wrap[1][0] & 2) && ld_valid) { /* VIS3: the same as PT09 for tile 1. The RDP reads (s - sl) through the mask, a mirrored or wrapped read of a window to the right of the image start (walking dust frame i + 1) needs the sheet from its start. */
                    int per = 1 << nd_ws.tst1.tile_wrap[1][2];
                    int ldw = nd_ws.tst1.tile_siz[1] == 0 ? ld_w * 2 : ld_w;
                    if (per > w && per <= 256 && ldw > w) w = ldw < per ? ldw : per;
                }
                int stride = nd_ws.tst1.timg_stride > w ? nd_ws.tst1.timg_stride : w;
                int bpp = nd_ws.tst1.tile_siz[1] >= 3 ? 4 : nd_ws.tst1.tile_siz[1] == 2 ? 2 : nd_ws.tst1.tile_siz[1] == 0 ? 0 : 1;
                const u8* src = (const u8*)nd_ws.timg;
                int l1x = ld_x, l1y = ld_y, l1stride = nd_ws.tst1.timg_stride, l1v = ld_valid;
                for (unsigned k = 0; k < 4; k++) {
                    if (nd_ws.tst1.ldrec[k].img && nd_ws.tst1.ldrec[k].tmem == nd_ws.tst1.tile_tmem[1]) {
                        src = (const u8*)nd_ws.tst1.ldrec[k].img; l1x = nd_ws.tst1.ldrec[k].x; l1y = nd_ws.tst1.ldrec[k].y; l1stride = nd_ws.tst1.ldrec[k].stride; l1v = 1;
                        stride = l1stride > w ? l1stride : w;
                    }
                }
                int t1x = uls >> 2; /* texel column of the window origin; the V21 branch lowers it to the loaded origin */
                int boff = 0; /* V13: tile 1 inside an earlier block load: the image start moves by the TMEM difference */
                for (unsigned k = 0; k < 4; k++)
                    if (nd_ws.tst1.blk[k].img && nd_ws.tst1.blk[k].tmem < nd_ws.tst1.tile_tmem[1] && (nd_ws.tst1.tile_tmem[1] - nd_ws.tst1.blk[k].tmem) * 8u < nd_ws.tst1.blk[k].bytes && nd_ws.tst1.blk[k].tmem == nd_ws.tst1.tile_tmem[0] && src == (const u8*)nd_ws.tst1.blk[k].img)
                        boff = (int)((nd_ws.tst1.tile_tmem[1] - nd_ws.tst1.blk[k].tmem) * 8u);
                nd_ws.td[1].valid = 0;
                if (nd_ws.cur_tile == 1) nd_ws.cur_tile = -1; /* a new tile 1 image: bind it again */
                /* The image offset is the LoadTile origin. SetTileSize uls and ult only place
                 * it: gDPLoadMultiTile (draw_box corners) loads at 0,0 and moves tiles 1 to 3
                 * to the far side of the window by SetTileSize. */
                if (src && bpp && l1v) src += (size_t)(l1y * stride + l1x) * (size_t)bpp;
                else if (src && bpp) src += (size_t)boff; /* V13: a block load puts the image at TMEM start, SetTileSize only moves the coordinates */
                else if (src && nd_ws.tst1.tile_siz[1] == 0 && !l1v) src += (size_t)boff; /* PT82: the same for a 4 bit image (gDPScrollTextureBlockHalfHeight_4b: tile 1 is the lower half of one block load, mim_darkgreen2tif) */
                else if (src && l1v) {
                    /* V21: SetTileSize uls and ult are the window origin in the image (fire_breath scrolls tile 1 by imgFrame*32+32).
                     * This branch is 4 bit only (bpp 0). uls >> 2 counts texels, two per byte, so the byte offset is half of it.
                     * A byte pointer cannot start on an odd texel: the loaded origin is the even texel at or before the window
                     * origin. td[1].x0 uses the same origin, so the UV map and the loaded data agree. */
                    t1x = (uls >> 2) & ~1;
                    src += (size_t)(ult >> 2) * (size_t)l1stride + (size_t)(t1x >> 1);
                    stride = l1stride * 2;
                }
                if (src && w > 0 && h > 0) {
                    nd_ws.td[1].valid = 1;
                    nd_ws.td[1].src = src;
                    nd_ws.td[1].fmt = nd_ws.tst1.tile_fmt[1];
                    nd_ws.td[1].siz = nd_ws.tst1.tile_siz[1];
                    nd_ws.td[1].w = w;
                    nd_ws.td[1].h = h;
                    nd_ws.td[1].stride = stride;
                    nd_ws.td[1].x0 = t1x;
                    nd_ws.td[1].y0 = ult >> 2;
                    nd_ws.td[1].pal = tlut_pal[nd_ws.tst1.tile_pal[1] & 15] ? tlut_pal[nd_ws.tst1.tile_pal[1] & 15] : nd_ws.tlut;
                    nd_ws.td[1].pal_n = nd_ws.tlut_n;
                }
            }
}

__attribute__((noinline)) void tlut_v20(unsigned slot) {
                    for (int n = 0; n < 2; n++)
                        if (nd_ws.td[n].valid && nd_ws.td[n].fmt == 2 && (nd_ws.tst1.tile_pal[n] & 15) == (int)slot && nd_ws.td[n].pal != nd_ws.timg) {
                            pm_gpu_flush();
                            nd_ws.td[n].pal = nd_ws.timg;
                            nd_ws.td[n].pal_n = nd_ws.tlut_n;
                            if (n == nd_ws.cur_tile) nd_ws.cur_tile = -1;
                        }
}

__attribute__((cold, noinline)) int tex_shaded0(int uls, int ult) {
            if (m1.valid && nd_ws.tst1.tile_fmt[0] == 2 && nd_ws.tst1.tile_siz[0] == 0 && nd_ws.tst1.tile_pal[0] != nd_ws.tst1.tile_pal[1] &&
                nd_ws.tst1.comb_a == G_CCMUX_TEXEL0 && nd_ws.tst1.comb_c == G_CCMUX_TEXEL1 &&
                m1.w > 0 && m1.h > 0) {
                /* Tile 0 starts 256 texels in (the vertices carry that) plus the light offset. */
                int dx4 = uls - (m1.x0 << 2);
                int dy4 = ult - (m1.y0 << 2);
                m1.valid = 0;
                pm_gpu_flush();
                nd_ws.tile_x0 = m1.x0;
                nd_ws.tile_y0 = m1.y0;
                nd_ws.tile_fx = nd_ws.tile_fy = 0.f;
                if (load_shaded_sprite(dx4, dy4)) {
                    dual_on = 1;
                    sync_combine();
                    return 1;
                }
            }
    return 0;
}

__attribute__((cold, noinline)) void tex_win_s25(int* p_uls, int* p_ult, int* p_lrs, int* p_lrt) {
    int uls = *p_uls, ult = *p_ult, lrs = *p_lrs, lrt = *p_lrt;
                /* S25: a window with a negative start (a scrolled texture) is 12 bit two's complement. Texel 0 is
                 * still the first texel of the loaded image, so keep the size and start at 0. */
                int su = uls & 0x800 ? uls - 0x1000 : uls, st = ult & 0x800 ? ult - 0x1000 : ult;
                int sr = lrs & 0x800 ? lrs - 0x1000 : lrs, sl = lrt & 0x800 ? lrt - 0x1000 : lrt;
                if (sr >= su && sl >= st) {
                    lrs = (su < 0 ? 0 : su) + (sr - su);
                    lrt = (st < 0 ? 0 : st) + (sl - st);
                    uls = su < 0 ? 0 : su;
                    ult = st < 0 ? 0 : st;
                }
    *p_uls = uls; *p_ult = ult; *p_lrs = lrs; *p_lrt = lrt;
}

__attribute__((noinline)) void tex_op_impl(u32 w0, u32 w1) {
    unsigned op = w0 >> 24;
    tex_gen++; /* TBREP */
    switch (op) {
        case G_SETTIMG: op_settimg(w0, w1); break;
        case G_SETTILE: op_settile(w0, w1); break;
        case G_LOADBLOCK: op_loadblock(w0, w1); break;
        case G_LOADTILE:
            ld_valid = 1;
            ld_x = (int)((w0 >> 12) & 0xfff) >> 2;
            {
                unsigned tm = nd_ws.tst1.tile_tmem[(w1 >> 24) & 7], k;
                for (k = 0; k < 4 && !(nd_ws.tst1.ldrec[k].img && nd_ws.tst1.ldrec[k].tmem == tm); k++) {}
                if (k == 4) k = ldrec_n++ & 3;
                nd_ws.tst1.ldrec[k].tmem = tm; nd_ws.tst1.ldrec[k].img = nd_ws.timg; nd_ws.tst1.ldrec[k].stride = nd_ws.tst1.timg_stride;
                nd_ws.tst1.ldrec[k].x = (int)((w0 >> 12) & 0xfff) >> 2; nd_ws.tst1.ldrec[k].y = (int)(w0 & 0xfff) >> 2;
            }
            ld_y = (int)(w0 & 0xfff) >> 2;
            ld_h = (((int)(w1 & 0xfff) - (int)(w0 & 0xfff)) >> 2) + 1;
            ld_w = ((((int)(w1 >> 12) & 0xfff) - ((int)(w0 >> 12) & 0xfff)) >> 2) + 1;
            break;
        case G_LOADTLUT: op_loadtlut(w0, w1); break;
        case G_SETTILESIZE: op_settilesize(w0, w1); break;
        case G_TEXTURE: op_texture(w0, w1); sync_combine(); break;
        case G_SETOTHERMODE_H:
        case G_SETOTHERMODE_L: op_setothermode(op, w0, w1); break;
        case G_SETPRIMCOLOR: op_setprimcolor(w0, w1); break;
        case G_SETCOMBINE: op_setcombine(w0, w1); sync_combine(); break;
    }
}

void tex_op(u32 w0, u32 w1) {
    if (!pm_ph_on) { tex_op_impl(w0, w1); return; }
    unsigned op = w0 >> 24, t0 = pm_ticks();
    unsigned long long f0 = pm_gpu_fl_acc();
    tex_op_impl(w0, w1);
    unsigned dt = (unsigned)(pm_ticks() - t0), df = (unsigned)(pm_gpu_fl_acc() - f0);
    tx_t[op] += dt - (df < dt ? df : dt); tx_f[op] += df; tx_n[op]++; /* own time without the draw closes inside the op, and the close time */
}
