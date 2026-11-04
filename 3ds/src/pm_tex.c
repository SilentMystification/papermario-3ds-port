#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm_tex.h"
#include "pm_texbake.h"
#include "pm_port.h"
void pm_gpu_rect_sync(void); /* pm_gpu.c */

enum { FMT_RGBA, FMT_YUV, FMT_CI, FMT_IA, FMT_I };
enum { SIZ_4, SIZ_8, SIZ_16, SIZ_32 };
#include "pm_tex_tx1.h" /* TX1 */
static unsigned tx1_miss, tx1_kb, tx1_ticks, tx1_mismatch; /* per 300 frames, see tc_report */
static u8* tx1_chk; /* switch tx1check: the new result, compared with the old path */

/* One frame can draw more distinct textures than the N64 TMEM suggests: a Bowser silhouette (many
 * background strips) plus a line of message glyphs went past 128 and the glyph quads drew white (V03). */
#define SLOTS 1024 /* SLOT1: peak live 497 (kzn_19, frame 1500, uncapped build); 497 x 1.5 = 746, next power of two */
#define TEX_SCRATCH_PLANE (1024u * 512u * 2u) /* one linear or tiled plane (1024x512 RGBA5551) */
typedef struct {
    const void* img;
    const void* tlut;
    unsigned fmt, siz;
    int w, h, stride;
    unsigned bytes;
    unsigned phash; /* V06: FNV-1a of the CI palette, so an in-place palette edit misses */
    unsigned dkey; /* 0 for a plain texture, else the hash of a shaded one */
    unsigned shash; /* tex_inv1: FNV-1a of the source bytes read at load, over slen bytes */
    unsigned slen;
    unsigned tlen; /* bytes of the palette at tlut (0 for none): the full extent is tested for overlap, not its start */
    int dirty; /* tex_inv2: invalidated, but the source is kept; slot_valid hashes it at the next lookup */
    int native; /* tex_inv2: frame buffer source, always dropped on invalidate */
    unsigned stamp;
    unsigned epoch;
    unsigned sgen; /* C1: bumped at drop and invalidation; TEX records compare this */
    unsigned uses; /* TC1: hits since the upload */
    unsigned last_frame; /* TC1: frame of the last hit, bind or upload */
    int live;
    int slab; /* FL1: the image lives in the texture slab */
    int vram_up; /* C3D_TexLoadImage pending before FrameBegin */
    u8* vram_stage;
    u8* tex_vram_ptr; /* slab VRAM; tex.data may point at vram_stage while staging */
    unsigned up_frame; /* row 1: tc_frame of the last C3D_TexLoadImage; vram_stage is freed two frames later */
    unsigned stage_n; /* row 1 measure: bytes of vram_stage (0 when freed) */
    int stale; /* invalidated while this frame's draws still use it: freed at the next frame start */
    C3D_Tex tex;
} Slot;

static Slot slots[SLOTS];
/* CF7: one bit per slot. vup_bits mirrors vram_up (upload pending). att_bits: stale is set, or a vram_stage is held.
 * The two per-frame passes visit only the set bits, in slot order, instead of every slot below slot_hi. */
#define SBIT_W (SLOTS / 32)
static unsigned vup_bits[SBIT_W], att_bits[SBIT_W];
static unsigned live_bits[SBIT_W]; /* SLOT1: one bit per live slot; the free slot search reads these words, not the records */
static void sbit_set(unsigned* b, int i) { b[i >> 5] |= 1u << (i & 31); }
static void sbit_clr(unsigned* b, int i) { b[i >> 5] &= ~(1u << (i & 31)); }
static void stale_mark(Slot* s) { s->stale = 1; sbit_set(att_bits, (int)(s - slots)); }
static int slot_valid(Slot* s);
static unsigned used;
#define RECENT_N 12
static short recent_slot[RECENT_N] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static unsigned recent_n;
static int slot_hi; /* every live slot index is below this, so scans stay short while few slots are live */
static unsigned slot_nlive, slot_peak; /* SLOT1: live slots now and the highest count of this run */
static unsigned used_peak; /* M02 measurement */
static unsigned map_peak;
/* V13c: bytes of the slots that a draw touched in this frame (the working set that eviction must keep), highest since the last map. */
static unsigned frame_bytes, map_wmax;
#define TOUCH(s) do { if ((s)->epoch != epoch) { frame_bytes += (s)->bytes; if (frame_bytes > map_wmax) map_wmax = frame_bytes; } (s)->epoch = epoch; } while (0)
static unsigned stamp;
static unsigned epoch;
/* TC1: cache statistics. The counters are plain increments. The report line (every 300 frames) prints with switch texstat or loglevel=debug. */
static unsigned tc_frame, tc_hits, tc_miss, tc_evict, tc_evict_ev0, tc_inval, tc_up_bytes, tc_evict_bytes, tc_ev0_bytes;
static unsigned tot_hit, tot_miss, tot_inval, tot_evict, tot_up; /* lifetime (up in bytes), read per map by pm_tex_map_delta */
static unsigned tot_inv_t, tot_hash_t; /* tex_inv3: ticks in pm_tex_invalidate and in the lookup hash (slot_valid), read per map */
static unsigned gh_up_n, gh_up_t; /* TEXL2 L2a: upload ops and their ticks, per texghost window */
static unsigned tot_same, tot_diff; /* tex_inv1: at invalidation, the source bytes equal (same) or differ from (diff) the bytes at load */
static int tc_on = -1;
/* Every hit (load, bind, glyph, aux, bind_slot) stamps the slot with the use counter. Eviction takes the lowest stamp. */
#define TC_HIT(s) do { (s)->stamp = ++stamp; (s)->uses++; (s)->last_frame = tc_frame; tc_hits++; tot_hit++; } while (0)
static u8* scratch;
static int cur = -1;
static int aux = -1; /* S25: the second texture (TEXEL1), bound at unit 1 */
static GPU_TEXTURE_WRAP_PARAM aux_ws = GPU_REPEAT, aux_wt = GPU_REPEAT;
static int have;
/* V05: off-screen images of this frame (render textures owned by pm_gpu.c) */
static struct { const u8* base; unsigned stride; C3D_Tex* tex; int siz, w, h; } offt[10]; /* siz as pm_gpu_off_begin */
static int offt_n, off_cur = -1, off_ox, off_oy;
static int aux_off = -1, aux_ox, aux_oy; /* V12c: TEXEL1 is an off-screen slot (the grab of the frame buffer) */
unsigned pm_tex_epoch_v; /* PG1: bumped by every call that changes what pm_tex_bind or pm_tex_size would give, so a caller can skip a repeated identical load */
unsigned pm_tex_epoch(void) { return pm_tex_epoch_v; }
unsigned pm_tex_wrap_gen_v; /* OPT-TS: bumped only when the wrap state changes (pm_tex_set_wrap) and at a frame start. A texture load does not change it. */
unsigned pm_tex_wrap_gen(void) { return pm_tex_wrap_gen_v; }
void pm_tex_set_offscreen(int i, const void* base, unsigned stride, void* tex, int siz, int w, int h) {
    pm_tex_epoch_v++;
    offt[i].siz = siz;
    offt[i].w = w;
    offt[i].h = h;
    offt[i].base = (const u8*)base;
    offt[i].stride = stride;
    offt[i].tex = (C3D_Tex*)tex;
    if (i >= offt_n) offt_n = i + 1;
}
int pm_tex_i8(int aux) {
    if (aux) return aux_off >= 0 && aux_off < offt_n && offt[aux_off].siz == 1;
    if (have && off_cur >= 0 && off_cur < offt_n && offt[off_cur].siz == 1) return 1;
    return have && off_cur < 0 && cur >= 0 && cur < slot_hi && slots[cur].live && slots[cur].fmt == FMT_I && slots[cur].siz == SIZ_8;
}
int pm_tex_off_idx(void) { return (have && off_cur >= 0) ? off_cur : -1; }
int pm_tex_cur_idx(void) { return (have && off_cur < 0 && cur >= 0) ? cur : -1; }
int pm_tex_aux_off_idx(void) { return (aux_off >= 0 && aux_off < offt_n) ? aux_off : -1; }
void pm_tex_clear_offscreen(void) { for (int k = 0; k < 10; k++) offt[k].base = NULL; offt_n = 0; off_cur = -1; aux_off = -1; }
void pm_tex_origin(int* x, int* y) {
    pm_tex_epoch_v++;
    *x = (have && off_cur >= 0) ? off_ox : 0;
    *y = (have && off_cur >= 0) ? off_oy : 0;
}
static const C3D_Tex* tb_last; /* DR1 bind shadow, see pm_tex_bind */
static int tb_f, tb_ws, tb_wt;
static const C3D_Tex* au_last; /* DC2 bind shadow of unit 1, see pm_tex_bind_aux */
static int au_f, au_ws, au_wt;
static GPU_TEXTURE_WRAP_PARAM wrap_s = GPU_REPEAT;
static GPU_TEXTURE_WRAP_PARAM wrap_t = GPU_REPEAT;
static int shr_s, shr_t, aux_shr_s, aux_shr_t; /* V12c: the texture is only mask wide, the GPU wrap does the mask (set per load) */

static GPU_TEXTURE_WRAP_PARAM wrap_of(unsigned mode, unsigned mask) {
    /* mask 0 is clamp on the RDP, whatever the wrap field says. */
    if (mask == 0 || (mode & 2)) return GPU_CLAMP_TO_EDGE;
    if (mode & 1) return GPU_MIRRORED_REPEAT;
    return GPU_REPEAT;
}

/* X01: the tile mask. A tile larger than its mask (the star and circle wipes load a 32 texel wide image into a 64 texel
 * tile, mask 5, mirror) reads the image through the mask: s is masked before the texel address, so the part past the
 * mask mirrors or repeats the image. The texture is built that way (a copy of the 64 wide window, clamped at its edge). */
static unsigned mk_ws, mk_wt;  /* mask width in texels, 0 = none */
static unsigned mk_ms, mk_mt;  /* mirror flags */
static int mk_on;              /* the next load is a render tile load (pm_tex_load), not the shaded or rgba8 loads */

/* C7: one GPU texture per message font sheet; consecutive glyphs keep the same bind for DR1 rectangle batches. */
extern u8 MsgCharImgNormal[];
extern u8 MsgCharImgTitle[];
extern u8 MsgCharImgSubtitle[];
typedef struct {
    const u8* base;
    unsigned size;
    int gw, gh, raster, cols;
    unsigned id;
} GlyphSheet;

static const GlyphSheet glyph_sheets[] = {
    { MsgCharImgNormal, 0x5100u, 16, 16, 128, 16, 1u },
    { MsgCharImgTitle, 0xF60u, 12, 15, 96, 16, 2u },
    { MsgCharImgSubtitle, 0xB88u, 12, 12, 72, 16, 3u },
};

#define GLYPH_DKEY(id) (0xC7000000u | (id))

static unsigned rd16(const u8* p);
static void write5551(u8* dst, unsigned c);

static int glyph_ox, glyph_oy;

static int glyph_resolve(const u8* img, int w, int h, unsigned fmt, unsigned siz, const GlyphSheet** sh, int* idx) {
    unsigned i;
    if (fmt != FMT_CI || siz != SIZ_4) return 0;
    for (i = 0; i < sizeof glyph_sheets / sizeof glyph_sheets[0]; i++) {
        const GlyphSheet* g = &glyph_sheets[i];
        const u8* p = (const u8*)img;
        if (w != g->gw || h != g->gh || p < g->base || p >= g->base + g->size) continue;
        if (((unsigned)(p - g->base)) % (unsigned)g->raster) continue;
        *sh = g;
        *idx = (int)((p - g->base) / (unsigned)g->raster);
        return 1;
    }
    return 0;
}

static int glyph_atlas_dims(const GlyphSheet* g, int* aw, int* ah) {
    int n = (int)(g->size / (unsigned)g->raster);
    int rows = (n + g->cols - 1) / g->cols;
    *aw = g->cols * g->gw;
    *ah = rows * g->gh;
    return n;
}

static int glyph_build_atlas(const GlyphSheet* g, const u8* tlut, int tlut_n, int pw, int ph) {
    int aw, ah, ng, i;
    ng = glyph_atlas_dims(g, &aw, &ah);
    if (pw < aw || ph < ah) return 0;
    memset(scratch, 0, (size_t)pw * (size_t)ph * 2u);
    for (i = 0; i < ng; i++) {
        const u8* gsrc = g->base + (size_t)i * (size_t)g->raster;
        int gx = (i % g->cols) * g->gw;
        int gy = (i / g->cols) * g->gh;
        int y;
        for (y = 0; y < g->gh; y++) {
            int x;
            for (x = 0; x < g->gw; x++) {
                unsigned byte = gsrc[(y * g->gw + x) >> 1];
                unsigned idx = (x & 1) ? (byte & 15u) : (byte >> 4);
                unsigned pix = (tlut && (int)idx < tlut_n) ? rd16(tlut + idx * 2) : 0xffffu;
                write5551(scratch + ((gy + y) * pw + (gx + x)) * 2, pix);
            }
        }
    }
    return 1;
}

static int glyph_remap(const void** img, int* width, int* height, int* stride, unsigned* dkey,
                       unsigned fmt, unsigned siz) {
    const GlyphSheet* g;
    int idx;
    int aw, ah;
    if (!glyph_resolve((const u8*)*img, *width, *height, fmt, siz, &g, &idx)) {
        { static int onf = -1; if (onf < 0) onf = pm_debug_has("imglog");
          if (onf && fmt == FMT_CI && siz == SIZ_4 && *width == 16 && *height == 16) {
              unsigned k; long off = -1; for (k = 0; k < sizeof glyph_sheets / sizeof glyph_sheets[0]; k++) if ((const u8*)*img >= glyph_sheets[k].base && (const u8*)*img < glyph_sheets[k].base + glyph_sheets[k].size) off = (long)((const u8*)*img - glyph_sheets[k].base);
              pm_log_d("glyphmiss src %p off %ld stride %d", (void*)*img, off, *stride); } }
        glyph_ox = glyph_oy = 0;
        return 0;
    }
    glyph_atlas_dims(g, &aw, &ah);
    glyph_ox = (idx % g->cols) * g->gw;
    glyph_oy = (idx / g->cols) * g->gh;
    { static int on = -1; if (on < 0) on = pm_debug_has("imglog"); /* glyph log: source char index, atlas cell, source address */
      if (on) pm_log_d("glyphmap sheet %u idx %d cell %d,%d src %p w %d h %d stride %d", g->id, idx, glyph_ox, glyph_oy, (void*)*img, *width, *height, *stride); }
    *img = g->base;
    *width = aw;
    *height = ah;
    if (*stride < aw) *stride = aw;
    *dkey = GLYPH_DKEY(g->id);
    return 1;
}

void pm_tex_glyph_ofs(int* x, int* y) {
    if (x) *x = glyph_ox;
    if (y) *y = glyph_oy;
}

int pm_tex_glyph_same(const void* img, unsigned fmt, unsigned siz, int w, int h, const void* tlut, int tlut_n, int* gx, int* gy) {
    const GlyphSheet* g;
    int idx, aw, ah;
    unsigned phash = 0;
    Slot* s;
    if (!glyph_resolve((const u8*)img, w, h, fmt, siz, &g, &idx)) return 0;
    if (!have || cur < 0 || cur >= SLOTS) return 0;
    s = &slots[cur];
    glyph_atlas_dims(g, &aw, &ah);
    if (fmt == FMT_CI && tlut) {
        phash = 2166136261u;
        { const u16* pl = (const u16*)tlut; int i = 0;
          for (; i + 1 < tlut_n; i += 2) phash = (phash ^ ((unsigned)pl[i] | ((unsigned)pl[i + 1] << 16))) * 16777619u;
          if (i < tlut_n) phash = (phash ^ pl[i]) * 16777619u; }
    }
    if (!s->live || s->img != g->base || s->dkey != GLYPH_DKEY(g->id) || s->fmt != fmt || s->siz != siz || s->w != aw || s->h != ah ||
        s->tlut != tlut || s->phash != phash || !slot_valid(s)) return 0;
    glyph_ox = (idx % g->cols) * g->gw;
    glyph_oy = (idx / g->cols) * g->gh;
    if (gx) *gx = glyph_ox;
    if (gy) *gy = glyph_oy;
    TC_HIT(s);
    TOUCH(s);
    return 1;
}

void pm_tex_set_wrap(unsigned cms, unsigned cmt, unsigned masks, unsigned maskt) {
    pm_gpu_rect_sync(); /* DR1: a queued rectangle batch is drawn with the texture state it was queued in */
    pm_tex_epoch_v++;
    pm_tex_wrap_gen_v++;
    mk_ws = masks ? 1u << masks : 0;
    mk_wt = maskt ? 1u << maskt : 0;
    mk_ms = cms & 1u;
    mk_mt = cmt & 1u;
    wrap_s = wrap_of(cms, masks);
    wrap_t = wrap_of(cmt, maskt);
}

static int pot_at_least_8(int n) {
    int p = 8;
    while (p < n) p <<= 1;
    return p;
}

static unsigned rd16(const u8* p) { return ((unsigned)p[0] << 8) | p[1]; }
extern int pm_fb_native(const void* p);
static int src_native; /* D03: the image is a frame buffer or the saved frame (native u16) */
static unsigned rd16s(const u8* p) { return src_native ? ((unsigned)p[1] << 8) | p[0] : rd16(p); }

static GPU_TEXCOLOR gpu_fmt(unsigned fmt, unsigned siz) {
    if (fmt == FMT_RGBA && siz == SIZ_32) return GPU_RGBA8;
    if (fmt == FMT_IA && siz == SIZ_16) return GPU_LA8;
    if (fmt == FMT_IA && (siz == SIZ_8 || siz == SIZ_4)) return GPU_LA4;
    if (fmt == FMT_I && siz == SIZ_8) return GPU_L8;
    /* I4 masks (shadows, message silhouettes): intensity is alpha on the RDP; LA4 carries it in the alpha nibble. */
    if (fmt == FMT_I && siz == SIZ_4) return GPU_LA4;
    return GPU_RGBA5551;
}

static unsigned tex_gpu_bytes(int pw, int ph, GPU_TEXCOLOR gf) {
    if (gf == GPU_RGBA8) return (unsigned)pw * (unsigned)ph * 4u;
    if (gf == GPU_L4) return (unsigned)pw * (unsigned)ph / 2u;
    if (gf == GPU_L8 || gf == GPU_LA4) return (unsigned)pw * (unsigned)ph;
    return (unsigned)pw * (unsigned)ph * 2u;
}

static void write5551(u8* dst, unsigned c) {
    dst[0] = (u8)(c & 255);
    dst[1] = (u8)((c >> 8) & 255);
}

/* PICA stores 16-bit texels in 8x8 Morton tiles. C3D_TexLoadImage copies
 * the bytes through, so the scratch has to already be in that order. */
static unsigned tiled16(int x, int y, int pw) {
    unsigned mx = (unsigned)x & 7u;
    unsigned my = (unsigned)y & 7u;
    unsigned morton = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) |
                      ((mx & 4u) << 2) | ((my & 4u) << 3);
    unsigned tile = ((unsigned)y >> 3) * ((unsigned)pw >> 3) + ((unsigned)x >> 3);
    return (tile * 64u + morton) * 2u;
}

/* P13: y % m for y >= 0 and m >= 1. A power of two m needs no divide. Same result as the % operator. */
static inline int wrap_mod(int y, int m) { return (m & (m - 1)) == 0 ? (y & (m - 1)) : y % m; }

static void td3_parse(void); /* TEXD3, defined before pm_tex_bind */
static int td3_padoff;
static int expand(const u8* src, unsigned fmt, unsigned siz, int w, int h, int stride,
                  const u8* tlut, int tlut_n, int pw, int ph, GPU_TEXCOLOR gf) {
    if ((fmt == FMT_I && (siz == SIZ_8 || siz == SIZ_4)) || (fmt == FMT_IA && (siz == SIZ_8 || siz == SIZ_4)))
        return 0;
    unsigned out_bpp = (gf == GPU_RGBA8) ? 4 : (gf == GPU_L8 || gf == GPU_L4 || gf == GPU_LA4) ? 1 : 2;
    if ((unsigned)pw * (unsigned)ph * out_bpp > TEX_SCRATCH_PLANE) return 0;
    memset(scratch, 0, (size_t)pw * (size_t)ph * out_bpp);
    int mw = (mk_on && mk_ws && (int)mk_ws < w) ? (int)mk_ws : 0;
    int mh = (mk_on && mk_wt && (int)mk_wt < h) ? (int)mk_wt : 0;
    for (int y = 0; y < h; y++) {
        int sy = y;
        if (mh) { int r = wrap_mod(y, mk_mt ? 2 * mh : mh); sy = (mk_mt && r >= mh) ? 2 * mh - 1 - r : r; }
        for (int x = 0; x < w; x++) {
            int sx = x;
            if (mw) { int r = wrap_mod(x, mk_ms ? 2 * mw : mw); sx = (mk_ms && r >= mw) ? 2 * mw - 1 - r : r; }
            if (fmt == FMT_CI) {
                unsigned idx = 0;
                if (siz == SIZ_4) {
                    unsigned byte = src[(sy * stride + sx) >> 1];
                    idx = (sx & 1) ? (byte & 15) : (byte >> 4);
                } else {
                    idx = src[sy * stride + sx];
                }
                unsigned pix = (tlut && (int)idx < tlut_n) ? rd16(tlut + idx * 2) : 0xffffu;
                if (!(tlut && (int)idx < tlut_n)) { static int nb; if (nb++ < 8) pm_log_w("V16b2: CI texel idx %d tlut %p n %d, drawn white", (int)idx, (const void*)tlut, tlut_n); }
                write5551(scratch + ((y * pw + x) * 2), pix);
            } else if (fmt == FMT_RGBA && siz == SIZ_16) {
                write5551(scratch + ((y * pw + x) * 2), rd16s(src + (sy * stride + sx) * 2));
            } else if (fmt == FMT_RGBA && siz == SIZ_32) {
                const u8* s = src + (sy * stride + sx) * 4;
                u8* d = scratch + (y * pw + x) * 4;
                d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
            } else if (fmt == FMT_IA && siz == SIZ_16) {
                unsigned c = rd16(src + (sy * stride + sx) * 2);
                u8* d = scratch + (y * pw + x) * 2;
                /* PICA LA8 is alpha, then luminance. */
                d[0] = (u8)(c & 255);
                d[1] = (u8)(c >> 8);
            } else {
                write5551(scratch + ((y * pw + x) * 2), 0xffffu);
            }
        }
    }
    /* Pad to the power-of-two size with the last texel, so a clamped coordinate gets the edge. */
    /* A tile mask makes the image repeat or mirror past the window (bulb glow: window 65 in a 128 texture, mask 64, mirror),
     * so the pad follows the mask. */
    td3_parse(); /* TEXD3 switch padov=off: no mask follow in the pad */
    int pmh = td3_padoff ? 0 : mh, pmw = td3_padoff ? 0 : mw;
    for (int y = 0; y < ph; y++) {
        int sy = y < h ? y : h - 1;
        if (pmh) { int r = wrap_mod(y, mk_mt ? 2 * pmh : pmh); sy = (mk_mt && r >= pmh) ? 2 * pmh - 1 - r : r; }
        for (int x = (y < h ? w : 0); x < pw; x++) {
            int sx = x < w ? x : w - 1;
            if (pmw) { int r = wrap_mod(x, mk_ms ? 2 * pmw : pmw); sx = (mk_ms && r >= pmw) ? 2 * pmw - 1 - r : r; }
            memcpy(scratch + ((size_t)y * pw + x) * out_bpp, scratch + ((size_t)sy * pw + sx) * out_bpp, out_bpp);
        }
    }
    return 1;
}

static int nib(const u8* src, int stride, int x, int y) {
    unsigned byte = src[(y * stride + x) >> 1];
    return (x & 1) ? (byte & 15) : (byte >> 4);
}

/* P14: arithmetic shift right is floor(v / 4) for every int v (same as the old sign split with two divides). */
static int floor4(int v) { return v >> 2; }

static int expand_dual(const u8* src, int w, int h, int stride, const unsigned* lut, int dx4, int dy4,
                       int pw, int ph) {
    if ((unsigned)pw * (unsigned)ph * 4u > TEX_SCRATCH_PLANE) return 0;
    memset(scratch, 0, (size_t)pw * (size_t)ph * 4u);
    for (int y = 0; y < h; y++) {
        int sy = floor4(4 * y + 2 - dy4);
        if (sy < 0) sy = 0;
        if (sy > h - 1) sy = h - 1;
        for (int x = 0; x < w; x++) {
            int sx = floor4(4 * x + 2 - dx4);
            if (sx < 0) sx = 0;
            if (sx > w - 1) sx = w - 1;
            unsigned c = lut[nib(src, stride, x, y) * 16 + nib(src, stride, sx, sy)];
            u8* d = scratch + (y * pw + x) * 4;
            d[0] = (u8)(c >> 24);
            d[1] = (u8)(c >> 16);
            d[2] = (u8)(c >> 8);
            d[3] = (u8)c;
        }
    }
    return 1;
}

/* HW3: pm_tex_invalidate runs for every ROM read (6946 in one step at the NPC spawn). A count per hashed 4 KB page of the
 * start addresses (img, tlut) of the live slots lets a read that touches none of them return before the slot scan.
 * A hash collision only gives a false yes. The count of an address is removed when the slot drops or goes stale. */
#define ADDR_HASH 4096u
static unsigned short addr_cnt[ADDR_HASH];
static unsigned addr_live;
static inline unsigned addr_h(unsigned a) { return ((a >> 12) * 2654435761u) >> 20; }
/* Every 4 KB page of [p, p+n) is counted, so a write into the middle of a slot finds it. d is +1 (add) or -1 (sub). */
static void addr_span(const void* p, unsigned n, int d) {
    if (!p || !n) return;
    unsigned a = (unsigned)p, e = a + n - 1u;
    for (unsigned pg = a >> 12; pg <= (e >> 12); pg++) {
        unsigned h = addr_h(pg << 12);
        if (d > 0) { addr_cnt[h]++; addr_live++; }
        else { if (addr_cnt[h]) addr_cnt[h]--; if (addr_live) addr_live--; }
    }
}
/* The counted extent of a slot is its source bytes and its palette. Lengths are fixed between the add and the sub. */
static void slot_span(const Slot* s, int d) {
    addr_span(s->img, s->slen, d);
    addr_span(s->tlut, s->tlen, d);
}


/* Texture pool in VRAM (GPU path). A miss writes its tiled image into this block. One merged
 * GSPGPU_FlushDataCache per span before each command list submit (pm_tex_flush_pending). */
#define SLAB_EXT 640
#define PEND_N 24
#define PEND_GAP (64u * 1024u)
/* VRAM2: the cache is one block per bank, sized at boot from what the bank has left after every boot reservation (pm_mem.c
 * pm_vram_bank_free_kb). tex_cap is the sum that came up (0: no cache). It does not change after boot. */
static unsigned tex_cap;
static unsigned slab_segs; /* VRAM blocks that came up at boot */
static unsigned slab_n; /* slab_ext entries in use */
static struct { u8* p; unsigned len; } slab_ext[SLAB_EXT]; /* free extents, sorted by address */
static struct { u8* p; unsigned len; } pend[PEND_N]; static int pend_n;
static unsigned fl1_srchash(const void* img, int w, int h, int stride, unsigned siz) { if (siz != 2) return 0; unsigned x = 2166136261u; const u8* q = (const u8*)img; for (int y = 0; y < h; y++) for (int i = 0; i < w * 2; i++) x = (x ^ q[y * stride * 2 + i]) * 16777619u; return x; }
static int fl1_state = -1; static unsigned fl1_sum, fl1_n;
static int fl1_on(void) { return slab_segs > 0; }
void pm_vram_note(void); /* pm_mem.c */
void pm_vram_alloc_fail(const char* what, unsigned kb); /* pm_mem.c */
void pm_vram_reg(const void* p, unsigned bytes); /* pm_mem.c */
int pm_vram_bank_of(const void* p);
void pm_vram_unreg(const void* p, unsigned bytes); /* pm_mem.c */
int pm_debug_value(const char* key, char* out, unsigned n);
unsigned pm_vram_bank_free_kb(int b); /* pm_mem.c */
static void slab_init(void) {
    /* Bank A block first, then bank B: slab_ext stays sorted by address. Each block takes the bank's free space in 32 KB steps. A block
     * that does not come up is logged, and it takes one step less until it does. */
    static const vramAllocPos pos[2] = { VRAM_ALLOC_A, VRAM_ALLOC_B };
    for (unsigned k = 0; k < 2; k++) {
        unsigned kb = pm_vram_bank_free_kb((int)k) & ~31u;
        u8* p = NULL;
        while (kb >= 32u && !(p = (u8*)vramMemAlignAt(kb * 1024u, 0x80, pos[k]))) kb -= 32u;
        if (!p) {
            pm_vram_alloc_fail("slab", 0);
            pm_log_e("texcache bank %c: no VRAM left (free %uKB)", k ? 'B' : 'A', pm_vram_bank_free_kb((int)k));
            continue;
        }
        pm_vram_reg(p, kb * 1024u);
        pm_vram_note();
        slab_ext[slab_n].p = p; slab_ext[slab_n].len = kb * 1024u; slab_n++;
        tex_cap += kb * 1024u; slab_segs++;
        pm_log_w("texcache bank %c: %uKB at %p, cap %uKB", k ? 'B' : 'A', kb, (void*)p, tex_cap >> 10);
    }
    if (!slab_segs) pm_log_e("texcache: no VRAM left, textures are skipped");
    /* debug switches (test tools, may stay): vramfill=N and vramfilla=N make N dummy 512 KB blocks in bank B or bank A at boot, never freed.
     * They make a bank small so a full-buffer failure, and the shrink path, can be forced. */
    for (unsigned b = 0; b < 2; b++) {
        char v[16];
        const char* key = b ? "vramfill" : "vramfilla";
        unsigned nf = pm_debug_value(key, v, sizeof v) ? (unsigned)atoi(v) : 0;
        for (unsigned k = 0; k < nf; k++) {
            u8* d = (u8*)vramMemAlignAt(512u * 1024u, 0x80, b ? VRAM_ALLOC_B : VRAM_ALLOC_A);
            if (!d) { pm_log_e("%s: block %u of %u failed", key, k + 1, nf); break; }
            pm_vram_reg(d, 512u * 1024u);
            pm_log_w("%s: block %u of %u in bank %c", key, k + 1, nf, b ? 'B' : 'A');
        }
    }
}
static unsigned slab_size(unsigned bytes) { bytes = (bytes + 127u) & ~127u; return bytes < 512u ? 512u : bytes; } /* the GPU reads whole 8 x 8 tiles */
/* Level-0 byte count (same formula as citro3d C3D_TexInitWithParams). */
static unsigned tex_fmt_bits(GPU_TEXCOLOR gf) {
    if (gf == GPU_RGBA8) return 32;
    if (gf == GPU_L8 || gf == GPU_LA4 || gf == GPU_ETC1A4) return 8;
    if (gf == GPU_L4 || gf == GPU_ETC1) return 4;
    return 16;
}
static unsigned tex_gpu_object_size(int pw, int ph, GPU_TEXCOLOR gf) {
    if (pw < 8 || ph < 8 || pw > 1024 || ph > 1024) return 0;
    return tex_fmt_bits(gf) * (unsigned)pw * (unsigned)ph / 8u;
}
static unsigned tex_slab_reserve(unsigned gpu_size) { return slab_size(gpu_size); }
static u8* slab_alloc(unsigned bytes0) {
    unsigned bytes = slab_size(bytes0);
    int best = -1;
    for (unsigned i = 0; i < slab_n; i++) if (slab_ext[i].len >= bytes && (best < 0 || slab_ext[i].len < slab_ext[best].len)) best = (int)i;
    if (best < 0) return NULL;
    u8* r = slab_ext[best].p;
    slab_ext[best].p += bytes; slab_ext[best].len -= bytes;
    if (!slab_ext[best].len) { for (unsigned i = (unsigned)best; i + 1 < slab_n; i++) slab_ext[i] = slab_ext[i + 1]; slab_n--; }
    return r;
}
static unsigned slab_lost; /* bytes of VRAM the extent table could not take back (row 1 measure) */
static unsigned stage_freed; /* row 1 measure: vram_stage bytes freed by pm_tex_begin_frame, cumulative */
static void slab_free(void* p, unsigned bytes) {
    bytes = slab_size(bytes);
    unsigned i = 0;
    while (i < slab_n && (u32)slab_ext[i].p < (u32)p) i++;
    if (i > 0 && slab_ext[i - 1].p + slab_ext[i - 1].len == (u8*)p && pm_vram_bank_of(p) == pm_vram_bank_of(slab_ext[i - 1].p)) { slab_ext[i - 1].len += bytes; i--; }
    else if (slab_n < SLAB_EXT) { for (unsigned k = slab_n; k > i; k--) slab_ext[k] = slab_ext[k - 1]; slab_ext[i].p = p; slab_ext[i].len = bytes; slab_n++; }
    else { /* extent table full: the block is lost until the next boot. Counted and logged once. */
        static int lost_once;
        slab_lost += bytes;
        if (!lost_once) { lost_once = 1; pm_log_w("texcache: extent table full, %u B lost", bytes); }
        return;
    }
    if (i + 1 < slab_n && slab_ext[i].p + slab_ext[i].len == slab_ext[i + 1].p && pm_vram_bank_of(slab_ext[i].p) == pm_vram_bank_of(slab_ext[i + 1].p)) { slab_ext[i].len += slab_ext[i + 1].len; for (unsigned k = i + 1; k + 1 < slab_n; k++) slab_ext[k] = slab_ext[k + 1]; slab_n--; }
}
static void pend_flush_one(u8* p, unsigned len) { pm_fl_dc(p, len); PH_ADD(C_FLCALL, 1); PH_ADD(C_FLKB, len >> 10); }
static int slot_draws_stage(const Slot* s) {
    return s->vram_up && s->vram_stage && s->tex.size;
}

static void upload_vram_slot(Slot* s) {
    unsigned n = s->tex.size;
    if (!s->vram_up || !s->vram_stage || !n) return;
    if (s->tex_vram_ptr) s->tex.data = s->tex_vram_ptr;
    pm_fl_dc(s->vram_stage, n);
    extern void pm_gx_room(void);
    pm_gx_room(); /* HANG1a: GX queue room before the copy */
    C3D_TexLoadImage(&s->tex, s->vram_stage, GPU_TEXFACE_2D, 0);
    s->vram_up = 0;
    sbit_clr(vup_bits, (int)(s - slots));
    s->up_frame = tc_frame;
}
static unsigned pm_th_dc_n, pm_th_dc_us; /* PI3: pend_flush_one calls and us this drawn frame */
static void flush_pend_only(void) {
    unsigned t0 = pm_ph_on ? pm_ticks() : 0;
    for (int i = 0; i < pend_n; i++) pend_flush_one(pend[i].p, pend[i].len);
    if (pm_ph_on && pend_n) { pm_th_dc_n += pend_n; pm_th_dc_us += (pm_ticks() - t0) / 268u; }
    pend_n = 0;
}
int pm_tex_vram_pending(void) {
    for (int w = 0; w < SBIT_W; w++)
        if (vup_bits[w]) return 1;
    return 0;
}
int pm_tex_cur_vram_pending(void) {
    return have && cur >= 0 && cur < SLOTS && slots[cur].live && slots[cur].vram_up;
}
int pm_tex_aux_vram_pending(void) { /* UC3b: unit 1 slot is in its load frame (draws its linear stage copy) */
    return have && aux >= 0 && aux < SLOTS && slots[aux].live && slots[aux].vram_up;
}
void pm_tex_gate(void);
void pm_tex_upload_vram_before_frame(void) {
    unsigned t0 = tc_on > 0 ? pm_ticks() : 0;
    pm_tex_gate(); /* TEXF1: before this frame's copies join the GX queue, so the queue still holds the previous frame only */
    for (int w = 0; w < SBIT_W; w++)
        for (unsigned m = vup_bits[w]; m; m &= m - 1) { upload_vram_slot(&slots[w * 32 + (int)__builtin_ctz(m)]); gh_up_n++; }
    if (t0) gh_up_t += pm_ticks() - t0;
}
void pm_tex_drain_vram(void) { /* uploads run before C3D_FrameBegin and at bind */ }
void pm_tex_flush_pending(void) { flush_pend_only(); }
void pm_tex_flush_pend_only(void) { flush_pend_only(); }
static void pend_add(u8* p, unsigned len) {
    for (int i = 0; i < pend_n; i++) { /* merge with a span that is near */
        u8* a = pend[i].p; u8* e = a + pend[i].len;
        if (p <= e + PEND_GAP && p + len + PEND_GAP >= a) {
            u8* lo = p < a ? p : a; u8* hi = (p + len) > e ? (p + len) : e;
            pend[i].p = lo; pend[i].len = (unsigned)(hi - lo);
            return;
        }
    }
    if (pend_n == PEND_N) pm_tex_flush_pending();
    pend[pend_n].p = p; pend[pend_n].len = len; pend_n++;
}

/* tex_inv2: a dirty slot has its addresses removed from the page filter. Put them back before the slot goes on. */
static void undirty(Slot* s) {
    if (!s->dirty) return;
    slot_span(s, 1);
    s->dirty = 0;
}

/* probe (log only, TEXGARBLE): a staging copy freed less than 2 frames after its upload was queued. The GPU may still read it. */
static unsigned early_free_n, early_free_kb;
static void stage_probe(const Slot* s) {
    if (!s->vram_up && tc_frame < s->up_frame + 2) { early_free_n++; early_free_kb += s->stage_n >> 10; }
}
/* TEXL2 L2a (measure only): ghost table of contents that left the cache. Key = the slot identity plus the source hash. A miss whose key is here
 * was resident less than GH_AGE_KB KB of puts ago. cause 0: evicted, 1: source hash changed (slot_valid). */
#define GH_N 1024
#define GH_AGE_KB 4096u
typedef struct { unsigned key, cum; } GhEnt; /* cum: put KB clock at the put, bit 31 = cause 1 */
static GhEnt gh[GH_N];
static unsigned gh_clock_kb, gh_ev_hit, gh_ev_kb, gh_df_hit, gh_df_kb;
static unsigned gh_key(const void* img, const void* tlut, unsigned phash, unsigned fmt, unsigned siz, int w, int h, int stride, unsigned dkey, unsigned sh) {
    unsigned v[10] = { (unsigned)(uintptr_t)img, (unsigned)(uintptr_t)tlut, phash, fmt, siz, (unsigned)w, (unsigned)h, (unsigned)stride, dkey, sh };
    unsigned x = 2166136261u;
    for (int k = 0; k < 10; k++) x = (x ^ v[k]) * 16777619u;
    return x | 1u;
}
static void gh_put(const Slot* s, int cause) {
    if (!s->img || s->native || !s->slen) return;
    unsigned key = gh_key(s->img, s->tlut, s->phash, s->fmt, s->siz, s->w, s->h, s->stride, s->dkey, s->shash);
    gh_clock_kb += s->bytes >> 10;
    gh[key & (GH_N - 1)].key = key;
    gh[key & (GH_N - 1)].cum = gh_clock_kb | (cause ? 0x80000000u : 0u);
}
static void gh_check(unsigned key, unsigned bytes) {
    const GhEnt* e = &gh[key & (GH_N - 1)];
    unsigned c = e->cum & 0x7fffffffu;
    if (e->key != key || gh_clock_kb - c >= GH_AGE_KB) return;
    if (e->cum >> 31) { gh_df_hit++; gh_df_kb += bytes >> 10; }
    else { gh_ev_hit++; gh_ev_kb += bytes >> 10; }
}

/* TEXF1: a staging copy or a slab extent is freed only after the GPU has finished the frames that sampled it. drop, the stage free
 * and the stage replace call pf_add. pm_tex_gate runs before C3D_FrameBegin: when the GX queue of the previous frame is done (pm_gx_done,
 * no wait) the whole list is released. The GPU runs the queue in order, so a done queue covers every entry added before it. */
enum { PF_N = 128 };
typedef struct { u8* p; unsigned bytes; unsigned frame; int slab; } PendFree;
static PendFree pf_ent[PF_N];
static unsigned pf_n, pf_max, pf_ops, pf_gate_busy, pf_max_age, pf_rel, pf_over;
extern int pm_gx_done(void);
extern void pm_gx_drain(void);
static void pf_release_all(void) {
    for (unsigned k = 0; k < pf_n; k++) {
        unsigned age = tc_frame - pf_ent[k].frame;
        if (age > pf_max_age) pf_max_age = age;
        if (pf_ent[k].slab) slab_free(pf_ent[k].p, pf_ent[k].bytes);
        else linearFree(pf_ent[k].p);
    }
    pf_rel += pf_n;
    pf_n = 0;
}
static void pf_add(u8* p, unsigned bytes, int slab) {
    if (!p) return;
    if (pf_n == PF_N) { /* full: wait for the queue once, then release all. Rare. */
        pm_gx_drain();
        pf_over++;
        pf_release_all();
    }
    pf_ent[pf_n].p = p; pf_ent[pf_n].bytes = bytes; pf_ent[pf_n].frame = tc_frame; pf_ent[pf_n].slab = slab;
    pf_n++;
    if (pf_n > pf_max) pf_max = pf_n;
    pf_ops++;
}
void pm_tex_gate(void) {
    if (!pf_n) return;
    if (!pm_gx_done()) { pf_gate_busy++; return; }
    pf_release_all();
}

static void drop(Slot* s) {
    if (!s->live) return;
    undirty(s);
    slot_span(s, -1);
    if (aux == (int)(s - slots)) aux = -1;
    if (s->vram_stage) {
        stage_probe(s);
        pf_add(s->vram_stage, 0, 0);
        s->vram_stage = NULL;
        s->stage_n = 0;
    }
    s->vram_up = 0;
    sbit_clr(vup_bits, (int)(s - slots));
    if (s->slab) {
        pf_add(s->tex_vram_ptr ? s->tex_vram_ptr : s->tex.data, s->bytes, 1);
        s->tex_vram_ptr = NULL;
        s->slab = 0;
        memset(&s->tex, 0, sizeof s->tex);
    }
    tb_last = NULL; au_last = NULL;
    if (used >= s->bytes) used -= s->bytes;
    else used = 0;
    s->sgen++;
    s->live = 0;
    sbit_clr(live_bits, (int)(s - slots));
    slot_nlive--;
    s->stale = 0;
    while (slot_hi > 0 && !slots[slot_hi - 1].live) slot_hi--;
}

unsigned pm_tex_slot_gen(int i) {
    if (i < 0 || i >= SLOTS || !slots[i].live) return 0;
    return slots[i].sgen;
}
/* RK2: the cache slots bound on unit 0 and unit 1 (-1 when none). The record key compares their generations. */
void pm_tex_cur_slots(int* a, int* b) { *a = cur; *b = aux; }
/* CEN1: texture identity for the draw census (pm_gpu.c cen_add). The slot source hash (shash) with format and size, so the id matches across builds. 0 none; 0x40000000 and 0x40000100 mark the off-screen image of unit 0 and unit 1. */
static unsigned slot_census_id(const Slot* s) {
    unsigned h = 2166136261u;
    h = (h ^ s->shash) * 16777619u; h = (h ^ s->fmt) * 16777619u; h = (h ^ s->siz) * 16777619u;
    h = (h ^ (unsigned)s->w) * 16777619u; h = (h ^ (unsigned)s->h) * 16777619u;
    return h | 1u;
}
unsigned pm_tex_census_id(int unit) {
    if (unit == 0) {
        if (have && off_cur >= 0) return 0x40000000u | (unsigned)off_cur;
        return cur < 0 || !slots[cur].live ? 0u : slot_census_id(&slots[cur]);
    }
    if (aux_off >= 0 && aux_off < offt_n) return 0x40000100u | (unsigned)aux_off;
    return aux < 0 || !slots[aux].live ? 0u : slot_census_id(&slots[aux]);
}
/* UC3e: 1 when a record may draw slot s (-1 none): the slot is live and not in its load frame. Touches it as a bind does. */
int pm_tex_rec_use(int s) {
    if (s < 0) return 1;
    if (s >= SLOTS || !slots[s].live || slot_draws_stage(&slots[s])) return 0;
    TOUCH(&slots[s]);
    return 1;
}

static const C3D_Tex* tex_for_draw(Slot* s) {
    if (slot_draws_stage(s)) {
        pm_fl_dc(s->vram_stage, s->tex.size);
        s->tex.data = s->vram_stage;
        return &s->tex;
    }
    if (s->tex_vram_ptr) s->tex.data = s->tex_vram_ptr;
    return &s->tex;
}

/* probe (log only, TEXGARBLE, switch texbind): each bind logs the slot key (source address, length, sgen, frame) for the first
 * 200 binds. A bind of a clean slot whose source bytes no longer hash to shash counts as a mismatch: the bytes changed under
 * a key that was not invalidated. Dirty and native slots are skipped (their change is expected). */
static int bind_probe = -1; static unsigned bind_n, bind_mis, bind_skip;
static unsigned probe_fnv(const u8* p, unsigned n) { unsigned x = 2166136261u; for (unsigned k = 0; k < n; k++) x = (x ^ p[k]) * 16777619u; return x; }
/* TEXD1 debug switch texword=<hex image address>: the first 4 binds of that image write the source bytes (texsrc_<addr>_<n>.bin, slen bytes)
 * and the converted PICA bytes (texgpu_<addr>_<n>.bin, the slot's bytes in VRAM or the staged copy) to the working directory, and log the layout. */
static GPU_TEXTURE_FILTER_PARAM tex_filt; /* BIS1: tentative, the definition with the value is below (log only) */
static void texword_dump(const Slot* s) {
    static long want = -2;
    static unsigned minf; /* texword=<hex>@<frame>: the dumps start at that frame */
    static int ndump;
    if (want == -2) {
        char tv[32];
        if (pm_debug_value("texword", tv, sizeof tv)) {
            char* at = strchr(tv, '@');
            if (at) { minf = (unsigned)strtoul(at + 1, NULL, 10); *at = 0; }
            want = strcmp(tv, "shape") == 0 ? -3 : (long)strtoul(tv, NULL, 16);
        } else want = -1;
    }
    if (want == -1 || tc_frame < minf || (want == -3 ? !(s->fmt == 4 && s->siz == 0 && s->w == 64 && s->h == 64) : (long)(uintptr_t)s->img != want) || ndump >= 4) return;
    int staged = s->vram_stage && s->vram_up;
    const u8* g = staged ? s->vram_stage : (s->tex_vram_ptr ? s->tex_vram_ptr : (const u8*)s->tex.data);
    unsigned gn = staged ? s->stage_n : (unsigned)s->tex.size;
    char fn[48];
    FILE* f;
    unsigned hh = 2166136261u;
    ndump++;
    long addr = (long)(uintptr_t)s->img;
    snprintf(fn, sizeof fn, "texsrc_%08lx_%d.bin", addr, ndump);
    f = fopen(fn, "wb");
    if (f) { fwrite(s->img, 1, s->slen, f); fclose(f); }
    snprintf(fn, sizeof fn, "texgpu_%08lx_%d.bin", addr, ndump);
    f = fopen(fn, "wb");
    if (f && g) fwrite(g, 1, gn, f);
    if (f) fclose(f);
    if (g) for (unsigned k = 0; k < gn; k++) hh = (hh ^ g[k]) * 16777619u;
    pm_log("texword f%u img %08lx bind %d: slot %d %dx%d fmt %u siz %u stride %d slen %u gpu %ux%u gn %u staged %d wrap s %d t %d shr %d %d filter %d hash %08x gpu0 %02x%02x%02x%02x",
           (unsigned)tc_frame, addr, ndump, (int)(s - slots), s->w, s->h, s->fmt, s->siz, s->stride, s->slen, (unsigned)s->tex.width, (unsigned)s->tex.height, gn, staged,
           (int)wrap_s, (int)wrap_t, shr_s, shr_t, (int)tex_filt, hh, g ? g[0] : 0, g ? g[1] : 0, g ? g[2] : 0, g ? g[3] : 0);
}

static void bind_probe_one(int i, const Slot* s) {
    if (bind_n < 200) pm_log_d("texbind f%u slot %d img %p slen %u sgen %u %dx%d fmt %u siz %u dirty %d native %d", (unsigned)tc_frame, i, s->img, s->slen, s->sgen, s->w, s->h, s->fmt, s->siz, s->dirty, s->native);
    bind_n++;
    if (s->dirty || s->native || !s->shash || !s->img) { bind_skip++; return; }
    unsigned h = probe_fnv((const u8*)s->img, s->slen);
    if (h != s->shash) {
        if (bind_mis < 8) pm_log_w("texbind MISMATCH f%u slot %d img %p slen %u shash %08x now %08x sgen %u %dx%d fmt %u siz %u", (unsigned)tc_frame, i, s->img, s->slen, s->shash, h, s->sgen, s->w, s->h, s->fmt, s->siz);
        bind_mis++;
    }
}

int pm_tex_bind_slot(int i) {
    if (i < 0 || i >= SLOTS || !slots[i].live) return 0;
    cur = i;
    TOUCH(&slots[i]);
    TC_HIT(&slots[i]);
    texword_dump(&slots[i]);
    if (bind_probe < 0) bind_probe = pm_debug_has("texbind");
    if (bind_probe) bind_probe_one(i, &slots[i]);
    const C3D_Tex* tp = tex_for_draw(&slots[i]);
    pm_dc_c[DC_TB0]++;
    C3D_TexBind(0, tp);
    tb_last = slot_draws_stage(&slots[i]) ? NULL : &slots[i].tex;
    pm_tex_epoch_v++;
    return 1;
}

/* A texture bound in the current command list has to stay alive until the next
 * frame begins. Deleting it earlier makes the GPU sample freed VRAM. */
/* CACHE1: the current and the previous frame are protected. cause: 0 no free slot (slot limit), 1 cache cap, 2 no extent (slab) */
static unsigned tc_ev_cause[3];
static int evict_oldest_done(int cause) {
    int oldest = -1;
    for (int i = 0; i < slot_hi; i++) {
        if (!slots[i].live || slots[i].epoch == epoch || slots[i].epoch + 1u == epoch) continue;
        if (oldest < 0 || (int)(slots[i].stamp - slots[oldest].stamp) < 0) oldest = i; /* wrap-safe: stamp is a use counter, lowest = least recent */
    }
    if (oldest < 0) return 0;
    if (oldest == cur) cur = -1;
    tc_ev_cause[cause]++;
    tc_evict++;
    tot_evict++;
    tc_evict_bytes += slots[oldest].bytes;
    if (slots[oldest].uses == 0) { tc_evict_ev0++; tc_ev0_bytes += slots[oldest].bytes; }
    gh_put(&slots[oldest], 0);
    drop(&slots[oldest]);
    return 1;
}

/* PG2L: baked textures (docs/plans/PG2_loader.md). Switch texdump: write every plain miss as a request to texreq.bin. The pack
 * texbake.pak is not read by any shipped path (RR3), so the switches texbaked and texbakecheck are gone. A plain texture is one without lut, not a frame buffer image. */
enum { TBM_DUMP = 1 };
static int tb_mode;
static FILE *tb_dump;
static unsigned tb_dumped;
static uint64_t* tb_seen; /* texdump: keys already written (open addressing, 0 = empty) */
#define TB_SEEN_N 16384u

static void tb_init(void) {
    tb_mode = pm_debug_has("texdump") ? TBM_DUMP : 0;
    if (tb_mode & TBM_DUMP) {
        tb_dump = fopen("texreq.bin", "wb");
        tb_seen = (uint64_t*)calloc(TB_SEEN_N, sizeof(uint64_t));
        if (tb_dump && tb_seen) { uint32_t h[2] = { TB_REQ_MAGIC, 0xffffffffu }; fwrite(h, 4, 2, tb_dump); fflush(tb_dump); }
        else { pm_log_w("PG2L: texdump cannot start"); if (tb_dump) fclose(tb_dump); tb_dump = NULL; tb_mode &= ~TBM_DUMP; }
    }
}

static void tb_dump_req(uint64_t key, const TbReq* q, const u8* tl, const u8* src) {
    unsigned i = (unsigned)(key >> 7) & (TB_SEEN_N - 1);
    while (tb_seen[i]) { if (tb_seen[i] == key) return; i = (i + 1) & (TB_SEEN_N - 1); }
    if (tb_dumped * 2u >= TB_SEEN_N) return; /* table half full: stop writing, the list is long enough to report */
    tb_seen[i] = key;
    TbReq r = *q; r.src_len = tb_src_len(q);
    fwrite(&r, sizeof r, 1, tb_dump);
    if (q->tlut_n) fwrite(tl, 2, q->tlut_n, tb_dump);
    fwrite(src, 1, r.src_len, tb_dump);
    fflush(tb_dump);
    tb_dumped++;
}

void pm_tex_init(void) {
    /* Linear image, then a second copy in PICA tile order. */
    scratch = (u8*)linearAlloc(TEX_SCRATCH_PLANE * 2u);
    slab_init();
    { /* TEXL2 debug switch texcap=<KB> (test tool, may stay): cap the cache so one map evicts */
        char v[16];
        unsigned kb = pm_debug_value("texcap", v, sizeof v) ? (unsigned)atoi(v) : 0;
        if (kb && kb * 1024u < tex_cap) tex_cap = kb * 1024u;
    }
    tb_init();
}

unsigned pm_tex_used(void) { return used; }
unsigned pm_tex_budget(void) { return tex_cap; }
/* H03: the last load failed (cache full or no VRAM). The draw skips the batch, no white quad. */
static int fail_last, fail_n;
int pm_tex_failed(void) { return fail_last; }
unsigned pm_tex_fail_count(void) { return (unsigned)fail_n; }
/* sweep420: live slot count and lifetime evictions, read at each map load. */
void pm_tex_map_state(unsigned* out_n, unsigned* out_ev) {
    unsigned n = 0;
    for (int i = 0; i < slot_hi; i++) if (slots[i].live) n++;
    *out_n = n;
    *out_ev = tot_evict;
}
/* row 1 measure: live vram_stage KB, cumulative freed KB, and extent bytes lost to a full table. */
void pm_tex_stage_state(unsigned* live_kb, unsigned* freed_kb, unsigned* lost_b) {
    unsigned live = 0;
    for (int i = 0; i < slot_hi; i++) live += slots[i].stage_n;
    *live_kb = live >> 10;
    *freed_kb = stage_freed >> 10;
    *lost_b = slab_lost;
}
unsigned pm_tex_peak(void) { return used_peak; }
/* V13c: highest cache use since the last call, then restart from the current use. */
unsigned pm_tex_map_peak_take(unsigned* wmax) { unsigned p = map_peak; *wmax = map_wmax; map_peak = used; map_wmax = 0; return p; }
/* texmap (debug): cache counts since the previous call, at each map load: hits, misses, uploaded KB, invalidated, evicted. */
void pm_tex_map_delta(unsigned* o) {
    static unsigned prev[9];
    unsigned now[9] = { tot_hit, tot_miss, tot_up >> 10, tot_inval, tot_evict, tot_same, tot_diff, tot_inv_t, tot_hash_t };
    for (int k = 0; k < 9; k++) { o[k] = now[k] - prev[k]; prev[k] = now[k]; }
}

static void tc_report(void) {
    unsigned n = 0, bytes = 0, u0n = 0, u0b = 0, rec_b = 0, rec_n = 0;
    unsigned an[5] = { 0 }, ab[5] = { 0 }; /* age in frames: <=1, <=10, <=100, <=300, older */
    unsigned small_n = 0; /* SLOT1: live slots under 1 KB */
    int top[3] = { -1, -1, -1 };
    for (int i = 0; i < slot_hi; i++) {
        Slot* s = &slots[i];
        unsigned age, b;
        if (!s->live) continue;
        age = tc_frame - s->last_frame;
        b = s->bytes;
        n++;
        bytes += b;
        if (b < 1024) small_n++;
        if (s->uses == 0) { u0n++; u0b += b; }
        if (age <= 300) { rec_n++; rec_b += b; }
        int k = age <= 1 ? 0 : age <= 10 ? 1 : age <= 100 ? 2 : age <= 300 ? 3 : 4;
        an[k]++;
        ab[k] += b;
        for (int t = 0; t < 3; t++)
            if (top[t] < 0 || b > slots[top[t]].bytes) {
                for (int u = 2; u > t; u--) top[u] = top[u - 1];
                top[t] = i;
                break;
            }
    }
    pm_log_d("texstat f%u: cap %uKB hits %u miss %u evict %u (%uKB) never hit before evict %u (%uKB) invalidated %u uploaded %uKB",
             tc_frame, tex_cap >> 10, tc_hits, tc_miss, tc_evict, tc_evict_bytes >> 10, tc_evict_ev0, tc_ev0_bytes >> 10, tc_inval, tc_up_bytes >> 10);
    pm_log_d("texstat stage early free %u (%uKB), freed at age 2 or more %uKB", early_free_n, early_free_kb, stage_freed >> 10);
    extern unsigned pm_copy_drain;
    pm_log_d("texstat pending free: now %u, max %u, ops %u, gate busy %u, max age %u frames, released %u, overflow %u, copy drains %u",
             pf_n, pf_max, pf_ops, pf_gate_busy, pf_max_age, pf_rel, pf_over, pm_copy_drain);
    pm_log_d("texstat evict by cause: slot %u cap %u extent %u (since last report)", tc_ev_cause[0], tc_ev_cause[1], tc_ev_cause[2]);
    tc_ev_cause[0] = tc_ev_cause[1] = tc_ev_cause[2] = 0;
    pm_log_d("texbind binds %u mismatch %u skipped %u", bind_n, bind_mis, bind_skip);
    pm_log_d("texstat live: %u slots %uKB, never hit %u (%uKB), used in last 300 frames %u (%uKB)", n, bytes >> 10, u0n, u0b >> 10, rec_n, rec_b >> 10);
    pm_log_d("texstat slots: live %u peak %u limit %u under 1 KB %u, byte cap %uKB", slot_nlive, slot_peak, SLOTS, small_n, tex_cap >> 10);
    pm_log_d("texstat age (frames since last use) <=1: %u/%uKB, <=10: %u/%uKB, <=100: %u/%uKB, <=300: %u/%uKB, older: %u/%uKB",
             an[0], ab[0] >> 10, an[1], ab[1] >> 10, an[2], ab[2] >> 10, an[3], ab[3] >> 10, an[4], ab[4] >> 10);
    for (int t = 0; t < 3; t++)
        if (top[t] >= 0) pm_log_d("texstat largest %d: %dx%d fmt %u siz %u %uKB uses %u age %u", t, slots[top[t]].w, slots[top[t]].h, slots[top[t]].fmt, slots[top[t]].siz,
                                  slots[top[t]].bytes >> 10, slots[top[t]].uses, tc_frame - slots[top[t]].last_frame);
    pm_log_hw("tx1 f%u miss %u KB %u us %u ns/B %u%s", tc_frame, tx1_miss, tx1_kb, tx1_ticks / 268u, tx1_kb ? (unsigned)((unsigned long long)tx1_ticks * 1000ull / 268ull / (tx1_kb * 1024ull)) : 0u, "");
    if (PMDL("tx1check")) pm_log_hw("tx1check mismatch %u", tx1_mismatch);
    tx1_miss = tx1_kb = tx1_ticks = 0;
    pm_log_hw("texghost f%u: evict-hit %u/%uKB diff-hit %u/%uKB upload ops %u us %u", tc_frame, gh_ev_hit, gh_ev_kb, gh_df_hit, gh_df_kb, gh_up_n, gh_up_t / 268u);
    gh_ev_hit = gh_ev_kb = gh_df_hit = gh_df_kb = gh_up_n = gh_up_t = 0;
    tc_hits = tc_miss = tc_evict = tc_evict_ev0 = tc_inval = tc_up_bytes = tc_evict_bytes = tc_ev0_bytes = 0;
}

void pm_tex_begin_frame(void) {
    pm_tex_epoch_v++;
    pm_tex_wrap_gen_v++;
    tc_frame++;
    if (tc_on < 0) tc_on = pm_debug_has("texstat") || pm_ph_on; /* texture timers: texstat or phase only, not loglevel */
    if (tc_on && tc_frame % 300 == 0) tc_report();
    epoch++;
    frame_bytes = 0;
    /* Frees of a sampled block go through pf_add (TEXF1): the GPU has not waited for the previous frame by default. */
    for (int w = 0; w < SBIT_W; w++)
        for (unsigned m = att_bits[w]; m; m &= m - 1) {
            int i = w * 32 + (int)__builtin_ctz(m);
            Slot* s = &slots[i];
            if (s->live && s->stale) drop(s);
            /* row 1: the linear staging copy is only read by the upload, two frames before it can go */
            if (s->live && s->vram_stage && !s->vram_up && tc_frame >= s->up_frame + 2) {
                pf_add(s->vram_stage, 0, 0);
                s->vram_stage = NULL;
                stage_freed += s->stage_n;
                s->stage_n = 0;
            }
            if (!(s->live && s->vram_stage)) sbit_clr(att_bits, i);
        }
}

extern unsigned pm_dlc_gen; /* DLC1: a write into the map shape buffer (ROM read, Yay0 decode) makes the compiled display lists stale */
extern u8 TextureHeap[], pm_world_entity_heap[], heap_generalHead[], heap_spriteHead[];
extern unsigned char gMapShapeData[];
/* tex_inv1: bytes the expander reads for a w x h source, stride in texels, siz = SIZ_4..SIZ_32 (4 << siz bits per texel) */
static unsigned src_len(int w, int h, int stride, unsigned siz) {
    unsigned bits = (unsigned)((h - 1) * stride + w) << (siz + 2);
    return (bits + 7) >> 3;
}
static unsigned src_hash(const void* p, unsigned n) {
    const u8* q = (const u8*)p;
    unsigned h = 2166136261u;
    for (unsigned i = 0; i < n; i++) h = (h ^ q[i]) * 16777619u;
    return h;
}
static void tex_invalidate_body(const void* ptr, unsigned size) {
    pm_tex_epoch_v++;
    if (ptr && size && (unsigned)ptr < (unsigned)gMapShapeData + 0x40000u && (unsigned)ptr + size > (unsigned)gMapShapeData) pm_dlc_gen++;
    unsigned start, end;
    int i;
    if (!ptr || !size) return;
    start = (unsigned)ptr;
    end = start + size;
    if (end < start) end = 0xffffffffu;
    if (addr_live == 0) return;
    if (size < (64u << 12)) {
        unsigned a;
        int hit = 0;
        for (a = start >> 12; a <= (end - 1) >> 12; a++)
            if (addr_cnt[addr_h(a << 12)]) { hit = 1; break; }
        if (!hit) return;
    }
    for (i = 0; i < slot_hi; i++) {
        Slot* s = &slots[i];
        unsigned img, pal;
        int img_hit, pal_hit;
        if (!s->live) continue;
        img = (unsigned)s->img;
        pal = (unsigned)s->tlut;
        /* Overlap of the full extents: [img, img+slen) and [pal, pal+tlen) against [start, end). */
        img_hit = s->img && img < end && img + (s->slen ? s->slen : 1u) > start;
        pal_hit = s->tlut && pal < end && pal + (s->tlen ? s->tlen : 1u) > start;
        if (!img_hit && !pal_hit) continue;
        if (img_hit && !s->native && s->slen) {
            /* tex_inv2: keep the slot. Its bytes are hashed at the next lookup, and only a changed hash drops it. */
            tc_inval++;
            tot_inval++;
            if (!s->dirty) {
                slot_span(s, -1);
                s->dirty = 1;
            }
            if (aux == i) aux = -1;
            if (cur == i) {
                cur = -1;
                have = 0;
            }
            s->sgen++;
            continue;
        }
        undirty(s);
        pm_gpu_rect_sync(); /* DR1: draw a queued rectangle batch before its texture goes away */
        tc_inval++;
        tot_inval++;
        if (aux == i) aux = -1;
        if (cur == i) {
            cur = -1;
            have = 0;
        }
        if (s->epoch == epoch) {
            /* A draw of this frame may still sample it (the GPU runs at FrameEnd). Hide it from the cache and free it later. */
            slot_span(s, -1);
            s->img = NULL;
            s->tlut = NULL;
            s->sgen++;
            stale_mark(s);
            continue;
        }
        drop(s);
    }
}

void pm_tex_invalidate(const void* ptr, unsigned size) {
    unsigned t0 = tc_on > 0 ? pm_ticks() : 0;
    tex_invalidate_body(ptr, size);
    if (t0) tot_inv_t += pm_ticks() - t0;
}

/* tex_inv2: 1 when the slot may be used. A dirty slot whose source hash changed leaves the cache (stale or drop) and 0 is returned. */
static int slot_valid(Slot* s) {
    int i;
    unsigned t0, h;
    if (!s->dirty) return 1;
    t0 = tc_on > 0 ? pm_ticks() : 0;
    h = src_hash(s->img, s->slen);
    if (t0) tot_hash_t += pm_ticks() - t0;
    if (h == s->shash) {
        undirty(s);
        tot_same++;
        return 1;
    }
    tot_diff++;
    gh_put(s, 1);
    i = (int)(s - slots);
    pm_gpu_rect_sync(); /* DR1 */
    if (aux == i) aux = -1;
    if (cur == i) {
        cur = -1;
        have = 0;
    }
    if (s->epoch == epoch) {
        undirty(s);
        slot_span(s, -1);
        s->img = NULL;
        s->tlut = NULL;
        s->sgen++;
        stale_mark(s);
        return 0;
    }
    drop(s);
    return 0;
}

static int ph_miss;
#define LIDX_N 256
static short lidx[LIDX_N]; /* OPT-LK */
static int load_inner(const void* img, unsigned fmt, unsigned siz, int width, int height,
                      const void* tlut, int tlut_n, int stride, const unsigned* lut, int dx4, int dy4,
                      unsigned dkey);
/* PT01: a cache hit is only counted. A miss (convert, tile, upload) runs under the tex phase. */
static unsigned tprof_hit_t, tprof_hit_n, tprof_miss_t, tprof_miss_n;
void pm_tex_prof_report(unsigned frames) {
    pm_log("gbiprof tex per frame: hit calls %u ticks-us %u (%u us per call), miss calls %u ticks-us %u (%u us per call)", tprof_hit_n / frames, tprof_hit_t / 268 / frames,
           tprof_hit_n ? tprof_hit_t / 268 / tprof_hit_n : 0, tprof_miss_n / frames, tprof_miss_t / 268 / frames, tprof_miss_n ? tprof_miss_t / 268 / tprof_miss_n : 0);
    if (tb_mode) pm_log("PG2L: requests written %u", tb_dumped);
    tprof_hit_t = tprof_hit_n = tprof_miss_t = tprof_miss_n = 0;
}
/* PI3: this drawn frame's counts: texture hits, hit us, misses, miss us, pend flush calls, pend flush us. Resets them. */
void pm_tex_th_frame(unsigned* o) {
    o[0] = tprof_hit_n; o[1] = tprof_hit_t / 268u; o[2] = tprof_miss_n; o[3] = tprof_miss_t / 268u; o[4] = pm_th_dc_n; o[5] = pm_th_dc_us;
    tprof_hit_t = tprof_hit_n = tprof_miss_t = tprof_miss_n = pm_th_dc_n = pm_th_dc_us = 0;
}
static int load_impl(const void* img, unsigned fmt, unsigned siz, int width, int height,
                     const void* tlut, int tlut_n, int stride, const unsigned* lut, int dx4, int dy4,
                     unsigned dkey) {
    pm_gpu_rect_sync(); /* DR1 */
    pm_tex_epoch_v++;
    int r;
    static int tp = -1;
    if (tp < 0) tp = pm_debug_has("gbiprof");
    unsigned t0 = (tp || pm_ph_on) ? pm_ticks() : 0; /* PI3: timed with phase on too */
    ph_miss = 0;
    PH_ADD(C_TEXCALL, 1);
    if (!lut && dkey == 0) {
        const void* gimg = img;
        int gw = width, gh = height, gstride = stride;
        unsigned gdkey = dkey;
        if (glyph_remap(&gimg, &gw, &gh, &gstride, &gdkey, fmt, siz))
            img = gimg, width = gw, height = gh, stride = gstride, dkey = gdkey;
    } else
        glyph_ox = glyph_oy = 0;
    r = load_inner(img, fmt, siz, width, height, tlut, tlut_n, stride, lut, dx4, dy4, dkey);
    if (ph_miss) PH_POP();
    if (tp || pm_ph_on) { /* PG2 measure: ticks inside pm_tex_load, hit and miss, per 300 frames (reported by pm_tex_prof_report) */
        unsigned dt = pm_ticks() - t0;
        if (ph_miss) { tprof_miss_t += dt; tprof_miss_n++; } else { tprof_hit_t += dt; tprof_hit_n++; }
    }
    { /* FB1 debug switch fbbind: the load after its fbbind line, hit or miss */
        static int fbon = -1;
        if (fbon < 0) fbon = pm_debug_has("fbbind");
        if (fbon && fmt == FMT_RGBA && siz == SIZ_16) pm_log_d("fbload img %p %dx%d st %d %s", img, width, height, stride, ph_miss ? "miss" : "hit");
    }
    return r;
}

static __attribute__((noinline)) int load_miss(const void* img, unsigned fmt, unsigned siz, int width, int height,
                      const void* tlut, int tlut_n, int stride, const unsigned* lut, int dx4, int dy4,
                      unsigned dkey, unsigned phash, unsigned lk) {
    src_native = pm_fb_native(img);
    PH_PUSH(PH_TEX);
    PH_ADD(C_TEXMISS, 1);
    const unsigned slen0 = src_len(width, height, stride, siz); /* TEXL2: the source hash is computed once here (also the slot hash) */
    const unsigned shash0 = src_hash(img, slen0);
    int pw = pot_at_least_8(width);
    int ph = pot_at_least_8(height);
    GPU_TEXCOLOR gf = gpu_fmt(fmt, siz);
    if (fmt == FMT_CI) gf = GPU_RGBA5551;
    if (lut) gf = GPU_RGBA8;
    unsigned gpu_size;
    for (;;) {
        gpu_size = tex_gpu_object_size(pw, ph, gf);
        if (!gpu_size) {
            if (pw <= ph && pw < 1024) {
                pw <<= 1;
                continue;
            }
            if (ph < 1024) {
                ph <<= 1;
                continue;
            }
            return 0;
        }
        if (gpu_size >= tex_gpu_bytes(pw, ph, gf)) break;
        if (pw <= ph && pw < 1024)
            pw <<= 1;
        else if (ph < 1024)
            ph <<= 1;
        else
            return 0;
    }
    gh_check(gh_key(img, tlut, phash, fmt, siz, width, height, stride, dkey, shash0), gpu_size);
    if (tb_mode && !lut && !src_native) {
        TbReq q;
        memset(&q, 0, sizeof q);
        q.fmt = (u8)fmt; q.siz = (u8)siz; q.ms = (u8)mk_ms; q.mt = (u8)mk_mt; q.w = (u16)width; q.h = (u16)height; q.stride = (u16)stride;
        q.mw = (u16)((mk_on && mk_ws && (int)mk_ws < width) ? mk_ws : 0); q.mh = (u16)((mk_on && mk_wt && (int)mk_wt < height) ? mk_wt : 0);
        q.tlut_n = (u16)((fmt == FMT_CI && tlut) ? tlut_n : 0);
        uint64_t key = tb_key(&q, (const u8*)tlut, (const u8*)img);
        if (tb_mode & TBM_DUMP) tb_dump_req(key, &q, (const u8*)tlut, (const u8*)img);
    }
    /* TX1: one pass expand and tile into the texture. Switch tx1check runs the old expand as well and compares (a test tool). */
    int native_tex = (gf == GPU_L8 || gf == GPU_L4 || gf == GPU_LA4);
    unsigned bpp0 = (gf == GPU_RGBA8) ? 4u : native_tex ? 1u : 2u;
    unsigned tex_bytes_need = tex_gpu_bytes(pw, ph, gf);
    int tx1 = tex_bytes_need <= TEX_SCRATCH_PLANE && pw <= TX1_MAXD && ph <= TX1_MAXD;
    int tx1_chkon = tx1 && PMDL("tx1check");
    if (tx1_chkon && !tx1_chk) { tx1_chk = (u8*)malloc(512 * 1024); if (!tx1_chk) { tx1 = 0; tx1_chkon = 0; } }
    unsigned tx1_t0 = tc_on > 0 ? pm_ticks() : 0;
    if (tx1 && !tx1_chkon && (dkey & 0xFF000000u) != 0xC7000000u) {
        /* TX1: Morton-tiled upload buffer filled below, no linear expand */
    } else if ((dkey & 0xFF000000u) == 0xC7000000u) {
        const GlyphSheet* gs = NULL;
        unsigned k;
        for (k = 0; k < sizeof glyph_sheets / sizeof glyph_sheets[0]; k++)
            if (glyph_sheets[k].id == (dkey & 0xffu)) { gs = &glyph_sheets[k]; break; }
        if (!gs || !glyph_build_atlas(gs, (const u8*)tlut, tlut_n, pw, ph)) return 0;
        gf = GPU_RGBA5551;
    } else if (lut) {
        if (!expand_dual((const u8*)img, width, height, stride, lut, dx4, dy4, pw, ph)) return 0;
    } else if (!native_tex && !expand((const u8*)img, fmt, siz, width, height, stride, (const u8*)tlut, tlut_n, pw, ph, gf))
        return 0;
    unsigned tex_bytes = gpu_size;
    unsigned reserve = tex_slab_reserve(gpu_size);
    unsigned bpp = (gf == GPU_RGBA8) ? 4u : (gf == GPU_L8 || gf == GPU_LA4) ? 1u : (gf == GPU_L4) ? 1u : 2u;
    int slot = -1;
    for (;;) {
        slot = -1;
        for (int w = 0; w < SBIT_W; w++) if (~live_bits[w]) { slot = w * 32 + (int)__builtin_ctz(~live_bits[w]); break; }
        if (slot >= 0 && used + reserve <= tex_cap) break;
        if (!evict_oldest_done(slot < 0 ? 0 : 1)) {
            static int once;
            fail_last = 1;
            fail_n++;
            if (!once) {
                once = 1;
                pm_log_w("tex cache full %dx%d, draws using it are skipped", width, height);
            }
            return 0;
        }
        slot = -1;
    }
    Slot* s = &slots[slot];
    tb_last = NULL; au_last = NULL; /* DR1 */
    s->slab = 0;
    if (!fl1_on()) {
        static int once;
        fail_last = 1;
        fail_n++;
        if (!once) {
            once = 1;
            pm_log_e("texcache VRAM pool missing");
        }
        return 0;
    }
    u8* sp_keep = NULL;
    { /* image in the VRAM pool; header from a per-format template, data in the slab */
        u8* sp = slab_alloc(reserve);
        while (!sp && evict_oldest_done(2)) {
            static int nev;
            if (nev++ < 8) pm_log_w("texcache VRAM full for %u bytes (used %u), evicted one texture", reserve, used);
            sp = slab_alloc(reserve);
        }
        if (!sp) {
            static int once, dg_n;
            fail_last = 1;
            fail_n++;
            if (!once) {
                once = 1;
                pm_log_w("texcache VRAM full %dx%d", width, height);
            }
            if (dg_n++ < 16) { /* row 0 diagnosis: why no extent fits */
                unsigned live = 0, this_ep = 0, evictable = 0, free_b = 0, hole = 0;
                for (int k = 0; k < slot_hi; k++) {
                    if (!slots[k].live) continue;
                    live++;
                    if (slots[k].epoch == epoch) this_ep++; else evictable++;
                }
                for (unsigned k = 0; k < slab_n; k++) {
                    free_b += slab_ext[k].len;
                    if (slab_ext[k].len > hole) hole = slab_ext[k].len;
                }
                pm_log_w("texfail slab: want %u B, live %u, this-epoch %u, evictable %u, free %u B, largest hole %u B, extents %u, used %u B",
                         reserve, live, this_ep, evictable, free_b, hole, slab_n, used);
            }
            return 0;
        }
        memset(&s->tex, 0, sizeof s->tex);
        if (!C3D_TexInit(&s->tex, (u16)pw, (u16)ph, gf)) {
            slab_free(sp, reserve);
            fail_last = 1;
            fail_n++;
            return 0;
        }
        void* init_data = s->tex.data;
        s->tex.data = s->tex_vram_ptr = sp;
        if (init_data) {
            C3D_Tex tmp = s->tex;
            tmp.data = init_data;
            C3D_TexDelete(&tmp);
        }
        s->slab = 1;
        sp_keep = sp;
    }
    C3D_TexSetFilter(&s->tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&s->tex, GPU_REPEAT, GPU_REPEAT);
    if (tx1) { /* TX1: tile in linear scratch; VRAM slot is filled by C3D_TexLoadImage (O3DS cannot CPU-store into vramMemAlign) */
        u8* t1dst = tx1_chkon ? tx1_chk : (scratch + TEX_SCRATCH_PLANE);
        if ((dkey & 0xFF000000u) == 0xC7000000u) {
            for (int y = 0; y < ph; y++) {
                int sy = ph - 1 - y;
                for (int x = 0; x < pw; x++) {
                    unsigned s0 = ((unsigned)sy * (unsigned)pw + (unsigned)x) * 2u;
                    unsigned d0 = tiled16(x, y, pw);
                    t1dst[d0] = scratch[s0];
                    t1dst[d0 + 1] = scratch[s0 + 1];
                }
            }
        } else if (lut) tx1_build_dual(t1dst, (const u8*)img, width, height, stride, lut, dx4, dy4, pw, ph);
        else {
            int mw1 = (mk_on && mk_ws && (int)mk_ws < width) ? (int)mk_ws : 0;
            int mh1 = (mk_on && mk_wt && (int)mk_wt < height) ? (int)mk_wt : 0;
            if (native_tex)
                tx1_build_native(t1dst, gf, fmt, siz, (const u8*)img, width, height, stride, pw, ph, mw1, mh1, (int)mk_ms, (int)mk_mt);
            else {
                tx1_build(t1dst, (const u8*)img, fmt, siz, width, height, stride, (const u8*)tlut, tlut_n, pw, ph, (int)bpp0, mw1, mh1,
                          (int)mk_ms, (int)mk_mt, src_native);
                if (tx1_bad_ci) {
                    static int nb;
                    if (nb++ < 8)
                        pm_log_w("V16b2: %u CI texels with an index outside the palette (tlut %p n %d), drawn white", tx1_bad_ci,
                                 (const void*)tlut, tlut_n);
                }
            }
        }
    }
    const u8* staged = scratch + TEX_SCRATCH_PLANE;
    if (!tx1 || tx1_chkon) {
        const void* upload = scratch;
        /* Scratch is two 256KB halves. The linear image fills the first, the
         * Morton copy the second. A 512x256 RGBA5551 background is 256KB, so the
         * old 128KB tile buffer skipped tiling and the GPU scattered the pixels. */
        if (bpp == 2u && (unsigned)pw * (unsigned)ph * 2u <= TEX_SCRATCH_PLANE) {
            u8* tiled = scratch + TEX_SCRATCH_PLANE;
            /* PICA samples t = 0 from the last row. Store row 0 there so the
             * game's top-left UV is the top of the image. */
            for (int y = 0; y < ph; y++) {
                int sy = ph - 1 - y;
                for (int x = 0; x < pw; x++) {
                    unsigned s0 = ((unsigned)sy * (unsigned)pw + (unsigned)x) * 2u;
                    unsigned d0 = tiled16(x, y, pw);
                    tiled[d0] = scratch[s0];
                    tiled[d0 + 1] = scratch[s0 + 1];
                }
            }
            upload = tiled;
        } else if (bpp == 4u && (unsigned)pw * (unsigned)ph * 4u <= TEX_SCRATCH_PLANE) {
            u8* tiled = scratch + TEX_SCRATCH_PLANE;
            for (int y = 0; y < ph; y++) {
                int sy = ph - 1 - y;
                for (int x = 0; x < pw; x++) {
                    unsigned s0 = ((unsigned)sy * (unsigned)pw + (unsigned)x) * 4u;
                    unsigned d0 = (tiled16(x, y, pw) / 2u) * 4u;
                    /* Same byte order citro3d's RGBA8 tiles use: A, B, G, R. */
                    tiled[d0] = scratch[s0 + 3];
                    tiled[d0 + 1] = scratch[s0 + 2];
                    tiled[d0 + 2] = scratch[s0 + 1];
                    tiled[d0 + 3] = scratch[s0];
                }
            }
            upload = tiled;
        } else if (bpp == 1u && (unsigned)pw * (unsigned)ph <= TEX_SCRATCH_PLANE) {
            u8* tiled = scratch + TEX_SCRATCH_PLANE;
            for (int y = 0; y < ph; y++) {
                int sy = ph - 1 - y;
                for (int x = 0; x < pw; x++) {
                    unsigned mx = (unsigned)x & 7u;
                    unsigned my = (unsigned)y & 7u;
                    unsigned morton = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) |
                                      ((mx & 4u) << 2) | ((my & 4u) << 3);
                    unsigned tile = ((unsigned)y >> 3) * ((unsigned)pw >> 3) + ((unsigned)x >> 3);
                    tiled[tile * 64u + morton] = scratch[(unsigned)sy * (unsigned)pw + (unsigned)x];
                }
            }
            upload = tiled;
        }
        staged = (const u8*)upload;
    }
    if (s->vram_stage) { stage_probe(s); pf_add(s->vram_stage, 0, 0); }
    s->vram_stage = (u8*)linearAlloc(gpu_size);
    sbit_set(att_bits, (int)(s - slots));
    if (!s->vram_stage) {
        slab_free(sp_keep, reserve); /* row 1: the VRAM extent taken above goes back, or it is lost */
        s->tex_vram_ptr = NULL; s->slab = 0; memset(&s->tex, 0, sizeof s->tex);
        fail_last = 1;
        fail_n++;
        return 0;
    }
    memcpy(s->vram_stage, staged, gpu_size);
    s->stage_n = gpu_size;
    pend_add(s->vram_stage, gpu_size);
    s->bytes = reserve;
    s->vram_up = 1;
    sbit_set(vup_bits, (int)(s - slots));
    if (PMDL("fl1log")) { unsigned h = 2166136261u; const u8* q = (const u8*)s->tex.data; for (unsigned i = 0; i < reserve; i++) h = (h ^ q[i]) * 16777619u; fl1_sum += h * (fl1_n + 1u); fl1_n++; if (fl1_n <= 400) pm_log_w("fl1log up %u slot %d %dx%d fmt %u siz %u bytes %u hash %08x src %dx%d stride %d img %p srchash %08x", fl1_n, slot, pw, ph, fmt, siz, reserve, h, width, height, stride, img, fl1_srchash(img, width, height, stride, siz)); }
    if (tx1_chkon && memcmp(tx1_chk, s->vram_stage, gpu_size) != 0) {
        if (tx1_mismatch++ < 8) pm_log_w("tx1check: bytes differ fmt %u siz %u %dx%d stride %d lut %d", fmt, siz, width, height, stride, lut != NULL);
    }
    if (tx1_t0) { tx1_miss++; tx1_kb += reserve >> 10; tx1_ticks += pm_ticks() - tx1_t0; }
    s->img = img;
    s->tlut = tlut;
    s->slen = slen0;
    s->tlen = tlut ? (tlut_n > 0 ? (unsigned)tlut_n * 2u : 2u) : 0u;
    slot_span(s, 1);
    s->phash = phash;
    s->fmt = fmt;
    s->siz = siz;
    s->w = width;
    s->h = height;
    s->stride = stride;
    s->dkey = dkey;
    s->shash = src_hash(img, s->slen); /* after the conversion, as at HEAD: the pre-conversion hash differs for some sources */
    s->dirty = 0;
    s->native = src_native;
    PH_ADD(C_TEXBYTE, reserve >> 10);
    s->stamp = ++stamp;
    s->epoch = epoch;
    s->uses = 0;
    s->last_frame = tc_frame;
    tc_miss++;
    tot_miss++;
    tot_up += reserve;
    tc_up_bytes += reserve;
    frame_bytes += reserve;
    if (frame_bytes > map_wmax) map_wmax = frame_bytes;
    s->live = 1;
    sbit_set(live_bits, slot);
    if (++slot_nlive > slot_peak) slot_peak = slot_nlive;
    if (slot >= slot_hi) slot_hi = slot + 1;
    used += reserve;
    if (used > used_peak) used_peak = used;
    if (used > map_peak) map_peak = used;
    cur = slot;
    recent_slot[recent_n++ % RECENT_N] = (short)slot; /* PG2 */
    lidx[lk] = (short)slot;
    have = 1;
    return 1;
}

static int load_inner(const void* img, unsigned fmt, unsigned siz, int width, int height,
                      const void* tlut, int tlut_n, int stride, const unsigned* lut, int dx4, int dy4,
                      unsigned dkey) {
    have = 0;
    cur = -1;
    off_cur = -1;
    fail_last = 0;
    if (!img || width < 1 || height < 1 || !scratch) return 0;
    {
        /* V05: a window of an off-screen image drawn this frame: sample the render texture, no copy. The load must have the
         * pixel size of the image (X02: the same memory is a 32 bit and an 8 bit image in one frame). */
        unsigned gs = (fmt == FMT_RGBA && siz == SIZ_32) ? 3u : (fmt == FMT_RGBA && siz == SIZ_16) ? 2u : (fmt == FMT_I && siz == SIZ_8) ? 1u : 0u;
        for (int i = 0; gs && i < offt_n; i++) {
            if (!offt[i].base) continue;
            unsigned bpp = gs == 3 ? 4u : gs, d = (unsigned)((const u8*)img - offt[i].base), row = offt[i].stride * bpp;
            if ((unsigned)offt[i].siz == gs && (const u8*)img >= offt[i].base && d < row * (unsigned)offt[i].h) {
                /* P15: a power of two row (and bpp) needs no divide. Same result. */
                if ((row & (row - 1)) == 0) {
                    off_oy = (int)(d >> __builtin_ctz(row));
                    off_ox = (int)((d & (row - 1)) / bpp);
                } else {
                    off_oy = (int)(d / row);
                    off_ox = (int)((d % row) / bpp);
                }
                off_cur = i;
                if (offt[i].w == 64) { static int n; if (n++ < 6) pm_log_d("X02: overlay reads 64x64 slot %d bits %d addr %p", i, gs == 3 ? 32 : gs == 2 ? 16 : 8, img); }
                have = 1;
                return 1;
            }
        }
    }
    if ((unsigned)img < 0x00100000u) {
        static int once;
        if (!once) {
            once = 1;
            pm_log_d("tex ptr %08x %dx%d fmt %u", (unsigned)img, width, height, fmt);
        }
        return 0;
    }
    shr_s = shr_t = 0;
    if (mk_on && lut == NULL) {
        int mw = (mk_ws && (int)mk_ws < width) ? (int)mk_ws : 0, mh = (mk_wt && (int)mk_wt < height) ? (int)mk_wt : 0;
        /* V12c: a tile window far larger than its mask (fright jar: window 501, mask 128, SetTileSize to 2000 only lifts the clamp) cannot be
         * expanded (512 x 512 is over the build size) and the load failed. The RDP applies the mask after the clamp, so a window larger than
         * the mask repeats (or mirrors) the mask sized image. Build that image, the GPU wrap repeats it. */
        if ((mw || mh) && pot_at_least_8(width) * pot_at_least_8(height) > 128 * 1024) {
            if (mw) { width = mw; shr_s = 1; mw = 0; }
            if (mh) { height = mh; shr_t = 1; mh = 0; }
        }
        if (mw || mh) dkey = (unsigned)(mw | mh << 8 | (int)mk_ms << 16 | (int)mk_mt << 17) | 0x80000000u;
        if (mw && stride < mw) stride = mw; /* the source row is the mask width, not the tile width */
        else if (!mw && stride < width) stride = width;
    } else if (stride < width) stride = width;
    unsigned phash = 0;
    if (fmt == FMT_CI && tlut) {
        phash = 2166136261u;
        { /* PG2: two palette entries per step (the key is internal, only its stability matters). Any in-place edit still changes it. */
            const u16* pl = (const u16*)tlut;
            int i = 0;
            if (!((unsigned)(uintptr_t)tlut & 3u)) { /* OPT-PH: the same value as the u16 pairs below (little endian), one word load per step, four steps per loop */
                const u32* pw = (const u32*)tlut;
                int nw = tlut_n >> 1, k = 0;
                for (; k + 4 <= nw; k += 4) {
                    phash = (phash ^ pw[k]) * 16777619u;
                    phash = (phash ^ pw[k + 1]) * 16777619u;
                    phash = (phash ^ pw[k + 2]) * 16777619u;
                    phash = (phash ^ pw[k + 3]) * 16777619u;
                }
                for (; k < nw; k++) phash = (phash ^ pw[k]) * 16777619u;
                i = nw * 2;
            }
            for (; i + 1 < tlut_n; i += 2) phash = (phash ^ ((unsigned)pl[i] | ((unsigned)pl[i + 1] << 16))) * 16777619u;
            if (i < tlut_n) phash = (phash ^ pl[i]) * 16777619u;
        }
    }
    /* OPT-LK: one probe of a direct mapped index from the key to the slot, before the recent list and the scan. The slot is compared in full, so a stale entry only misses. */
    const unsigned lk = (((unsigned)(uintptr_t)img >> 2) ^ ((unsigned)(uintptr_t)tlut >> 3) ^ (unsigned)(width * 31 + height * 131 + (int)fmt * 7 + (int)siz * 3 + stride * 17) ^ dkey) & (LIDX_N - 1);
    {
        int i = lidx[lk];
        Slot* s = &slots[i];
        if (i < slot_hi && s->live && s->img == img && s->tlut == tlut && s->phash == phash && s->fmt == fmt && s->siz == siz &&
            s->w == width && s->h == height && s->stride == stride && s->dkey == dkey && slot_valid(s)) {
            TC_HIT(s);
            TOUCH(s);
            cur = i;
            have = 1;
            return 1;
        }
    }
    /* PG2: most loads repeat one of the last few textures. Check those slots first (same full compare), then the whole table. */
    for (int k = 0; k < RECENT_N; k++) {
        int i = recent_slot[k];
        if (i < 0 || i >= slot_hi) continue;
        Slot* s = &slots[i];
        if (s->live && s->img == img && s->tlut == tlut && s->phash == phash && s->fmt == fmt && s->siz == siz &&
            s->w == width && s->h == height && s->stride == stride && s->dkey == dkey && slot_valid(s)) {
            TC_HIT(s);
            TOUCH(s);
            cur = i;
            have = 1;
            return 1;
        }
    }
    for (int i = 0; i < slot_hi; i++) {
        Slot* s = &slots[i];
        if (s->live && s->img == img && s->tlut == tlut && s->phash == phash && s->fmt == fmt && s->siz == siz &&
            s->w == width && s->h == height && s->stride == stride && s->dkey == dkey && slot_valid(s)) {
            TC_HIT(s);
            TOUCH(s);
            cur = i;
            have = 1;
            recent_slot[recent_n++ % RECENT_N] = (short)i;
            lidx[lk] = (short)i;
            return 1;
        }
    }
    ph_miss = 1;
    return load_miss(img, fmt, siz, width, height, tlut, tlut_n, stride, lut, dx4, dy4, dkey, phash, lk);
}

int pm_tex_load(const void* img, unsigned fmt, unsigned siz, int width, int height,
                const void* tlut, int tlut_n, int stride) {
    int r;
    mk_on = 1;
    r = load_impl(img, fmt, siz, width, height, tlut, tlut_n, stride, NULL, 0, 0, 0);
    mk_on = 0;
    return r;
}

int pm_tex_load_shaded(const void* img, int width, int height, int stride, const void* spal,
                       const unsigned* lut, int dx4, int dy4) {
    unsigned h = 2166136261u;
    for (int i = 0; i < 256; i++) h = (h ^ lut[i]) * 16777619u;
    h = (h ^ (unsigned)dx4) * 16777619u;
    h = (h ^ (unsigned)dy4) * 16777619u;
    return load_impl(img, FMT_CI, SIZ_4, width, height, spal, 16, stride, lut, dx4, dy4, h | 1u);
}

/* D03: a texture the port composed on the CPU (RGBA8, w x h, tightly packed in rgba). key names the content.
 * rgba NULL: only look for a cached copy and bind it. Returns 1 if bound. */
int pm_tex_load_rgba8(const void* buf, unsigned key, int w, int h, const unsigned char* rgba) {
    pm_gpu_rect_sync(); /* DR1: a queued rectangle batch is drawn with the texture state it was queued in */
    pm_tex_epoch_v++;
    key |= 1u;
    if (!rgba) {
        have = 0;
        cur = -1;
        off_cur = -1;
        for (int i = 0; i < slot_hi; i++) {
            Slot* s = &slots[i];
            if (s->live && s->img == buf && s->dkey == key && s->fmt == FMT_RGBA && s->siz == SIZ_32 && s->w == w &&
                s->h == h && s->stride == w && slot_valid(s)) {
                TC_HIT(s);
                TOUCH(s);
                cur = i;
                have = 1;
                return 1;
            }
        }
        return 0;
    }
    (void)buf;
    return load_impl(buf, FMT_RGBA, SIZ_32, w, h, NULL, 0, w, NULL, 0, 0, key);
}

static GPU_TEXTURE_FILTER_PARAM tex_filt = GPU_NEAREST; /* V15: from omh G_TF bits, set sampler state at bind, not in the cache key */
void pm_tex_set_filter(int linear) {
    pm_gpu_rect_sync(); /* DR1 */ tex_filt = linear ? GPU_LINEAR : GPU_NEAREST; }

/* DR1: citro3d sends the texture unit again at the next draw after every C3D_TexBind, C3D_TexSetFilter and C3D_TexSetWrap call. The shadow below
 * skips the three calls when the texture, filter and wrap are what the last bind set. Anything that can change the unit or the texture
 * object in another way clears it (frame and target changes through pm_tex_bind_reset, a dropped or loaded slot, the aux bind). */
void pm_tex_bind_reset(void) { tb_last = NULL; au_last = NULL; }
/* TEXD3 console switches (debug string): wrapov=repeat|mirror|clamp|edge forces the GPU wrap of unit 0 in pm_tex_bind; texnearest=1
 * forces nearest filter on unit 0; padov=off pads the texture with the plain window edge (no mask follow). Parsed when the switch
 * string changes (pm_dbg_gen). Absent switches change nothing. */
extern unsigned pm_dbg_gen;
int pm_debug_value(const char* key, char* out, unsigned n);
static unsigned td3_gen;
static int td3_wrap = -1, td3_near, td3_nlog;
static void td3_parse(void) {
    char v[16];
    if (td3_gen == pm_dbg_gen) return;
    td3_gen = pm_dbg_gen;
    td3_wrap = -1;
    if (pm_debug_value("wrapov", v, sizeof v)) {
        if (!strcmp(v, "repeat")) td3_wrap = GPU_REPEAT;
        else if (!strcmp(v, "mirror")) td3_wrap = GPU_MIRRORED_REPEAT;
        else if (!strcmp(v, "clamp")) td3_wrap = GPU_CLAMP_TO_BORDER;
        else if (!strcmp(v, "edge")) td3_wrap = GPU_CLAMP_TO_EDGE;
    }
    td3_near = pm_debug_value("texnearest", v, sizeof v) && v[0] == '1';
    td3_padoff = pm_debug_value("padov", v, sizeof v) && !strcmp(v, "off");
}
int pm_tex_bind(void) {
    if (have && off_cur >= 0) {
        if (tb_last == offt[off_cur].tex && tb_f == -2) return 1;
        pm_dc_c[DC_TB0]++;
        C3D_TexBind(0, offt[off_cur].tex);
        tb_last = offt[off_cur].tex; tb_f = -2;
        return 1;
    }
    if (!have || cur < 0 || !slots[cur].live) return 0;
    slots[cur].stamp = ++stamp;
    slots[cur].last_frame = tc_frame;
    GPU_TEXTURE_WRAP_PARAM ws = shr_s ? (mk_ms ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : wrap_s, wt = shr_t ? (mk_mt ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : wrap_t;
    td3_parse();
    if (td3_wrap >= 0) ws = wt = (GPU_TEXTURE_WRAP_PARAM)td3_wrap;
    GPU_TEXTURE_FILTER_PARAM ff = td3_near ? GPU_NEAREST : tex_filt;
    if (!slot_draws_stage(&slots[cur]) && tb_last == &slots[cur].tex && tb_f == (int)ff && tb_ws == (int)ws && tb_wt == (int)wt) return 1;
    texword_dump(&slots[cur]); /* TEXD1: pm_tex_bind is the path the draws use (pm_tex_bind_slot is not) */
    if (bind_probe < 0) bind_probe = pm_debug_has("texbind");
    if (bind_probe) bind_probe_one(cur, &slots[cur]);
    const C3D_Tex* tp = tex_for_draw(&slots[cur]);
    /* TEXGARBLE test switches (scratch): texnear = point filter, texclamp = clamp to edge, both for every bind of unit 0 */
    if (PMDL("texclamp")) ws = wt = GPU_CLAMP_TO_EDGE;
    C3D_TexSetFilter(tp, PMDL("texnear") ? GPU_NEAREST : ff, PMDL("texnear") ? GPU_NEAREST : ff);
    C3D_TexSetWrap(tp, ws, wt);
    if ((td3_wrap >= 0 || td3_near) && td3_nlog < 16) { td3_nlog++; pm_log_d("td3 bind unit0 slot %d programmed ws %d wt %d filter %d", cur, (int)ws, (int)wt, (int)ff); } /* first 16 binds only */
    pm_dc_c[DC_TB0]++; if (slot_draws_stage(&slots[cur])) pm_dc_c[DC_TBS]++;
    C3D_TexBind(0, tp);
    {   /* imglog: the GPU texture the draw binds (slot size, GPU size and address) */
        static int on = -1;
        if (on < 0) on = pm_debug_has("imglog");
        if (on) pm_log_d("imglog f%d gpubind slot w %d h %d stride %d gpu w %d h %d data %p fmt %u", (int)tc_frame,slots[cur].w, slots[cur].h, slots[cur].stride, (int)tp->width, (int)tp->height, (void*)tp->data, (unsigned)slots[cur].fmt);
    }
    tb_last = slot_draws_stage(&slots[cur]) ? NULL : &slots[cur].tex;
    tb_f = (int)ff; tb_ws = (int)ws; tb_wt = (int)wt;
    return 1;
}

void pm_tex_size(int* w, int* h) {
    int pw = 1, ph = 1;
    if (have && off_cur >= 0) {
        pw = offt[off_cur].w;
        ph = offt[off_cur].h;
    } else if (have && cur >= 0) {
        pw = pot_at_least_8(slots[cur].w);
        ph = pot_at_least_8(slots[cur].h);
    }
    if (w) *w = pw;
    if (h) *h = ph;
}

/* boxlog: the bound unit 0 slot, its size and format, and the raw bytes of 5 texels (4 corners and the centre) from the source image. */
void pm_tex_boxinfo(char* out, unsigned n) {
    static const int px[5] = { 0, -1, 0, -1, -2 }, py[5] = { 0, 0, -1, -1, -2 };
    const Slot* s;
    unsigned len, i;
    if (!out || n == 0) return;
    out[0] = 0;
    if (!have || off_cur >= 0 || cur < 0 || !slots[cur].live) { snprintf(out, n, "tex none"); return; }
    s = &slots[cur];
    if (!s->img) { snprintf(out, n, "tex %d no img", cur); return; }
    len = (unsigned)snprintf(out, n, "tex %d w %d h %d fmt %u siz %u stride %d", cur, s->w, s->h, s->fmt, s->siz, s->stride);
    for (i = 0; i < 5 && len + 40 < n; i++) {
        int x = px[i] < 0 ? (px[i] == -2 ? s->w / 2 : s->w + px[i]) : px[i];
        int y = py[i] < 0 ? (py[i] == -2 ? s->h / 2 : s->h + py[i]) : py[i];
        unsigned off = (unsigned)(y * s->stride), k, bpp = s->siz == 32 ? 4 : s->siz == 16 ? 2 : 1;
        const unsigned char* b = (const unsigned char*)s->img;
        if (s->siz == 4) off += (unsigned)(x / 2); else off += (unsigned)(x * (int)bpp);
        if (s->bytes && off + bpp > s->bytes) { len += (unsigned)snprintf(out + len, n - len, " (%d,%d)=oob", x, y); continue; }
        len += (unsigned)snprintf(out + len, n - len, " (%d,%d)=", x, y);
        for (k = 0; k < bpp && len + 3 < n; k++) len += (unsigned)snprintf(out + len, n - len, "%02x", b[off + k]);
    }
    if (s->fmt == 2 && s->tlut && len + 80 < n) { /* CI: the first 16 palette entries, 16 bit each (RGBA5551 on N64) */
        const unsigned char* t = (const unsigned char*)s->tlut;
        len += (unsigned)snprintf(out + len, n - len, " tlut");
        for (i = 0; i < 16 && len + 6 < n; i++) len += (unsigned)snprintf(out + len, n - len, " %04x", (unsigned)(t[2 * i] << 8 | t[2 * i + 1]));
    }
}

/* S25: the texture just loaded becomes TEXEL1 (unit 1). Wrap is that tile's own. */
int pm_tex_set_aux(unsigned cms, unsigned cmt, unsigned masks, unsigned maskt) {
    pm_gpu_rect_sync(); /* DR1: a queued rectangle batch is drawn with the texture state it was queued in */
    pm_tex_epoch_v++;
    aux_off = -1;
    if (have && off_cur >= 0) { aux = -1; aux_off = off_cur; aux_ox = off_ox; aux_oy = off_oy; return 1; }
    if (!have || cur < 0 || !slots[cur].live) { aux = -1; return 0; }
    aux = cur;
    aux_ws = wrap_of(cms, masks);
    aux_wt = wrap_of(cmt, maskt);
    aux_shr_s = shr_s;
    aux_shr_t = shr_t;
    return 1;
}

/* DC2: unit 1 keeps the texture of the last aux bind, as unit 0 does (tb_last). A staged slot flushes its data at every bind, so it never skips. */
int pm_tex_bind_aux(void) {
    if (aux_off >= 0 && aux_off < offt_n) {
        if (au_last == offt[aux_off].tex && au_f == -2) return 1;
        pm_dc_c[DC_TB1]++; C3D_TexBind(1, offt[aux_off].tex);
        au_last = offt[aux_off].tex; au_f = -2;
        return 1;
    }
    if (aux < 0 || !slots[aux].live) return 0;
    slots[aux].stamp = ++stamp;
    TOUCH(&slots[aux]);
    GPU_TEXTURE_WRAP_PARAM ws = aux_shr_s ? (mk_ms ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : aux_ws;
    GPU_TEXTURE_WRAP_PARAM wt = aux_shr_t ? (mk_mt ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : aux_wt;
    const int staged = slot_draws_stage(&slots[aux]);
    if (!staged && au_last == &slots[aux].tex && au_f == (int)tex_filt && au_ws == (int)ws && au_wt == (int)wt) return 1;
    tb_last = NULL; /* DR1: this changes the filter and wrap of a texture that unit 0 may use */
    const C3D_Tex* tp = tex_for_draw(&slots[aux]);
    if (PMDL("texclamp") || PMDL("texclamp1")) ws = wt = GPU_CLAMP_TO_EDGE; /* TEXGARBLE test switches: texclamp1 = aux unit only */
    if (PMDL("texrep1")) wt = GPU_REPEAT; /* TEXGARBLE test switch: aux unit t repeats (the N64 clamp of t off) */
    C3D_TexSetFilter(tp, PMDL("texnear") ? GPU_NEAREST : tex_filt, PMDL("texnear") ? GPU_NEAREST : tex_filt);
    C3D_TexSetWrap(tp, ws, wt);
    pm_dc_c[DC_TB1]++;
    C3D_TexBind(1, tp);
    au_last = staged ? NULL : tp; au_f = (int)tex_filt; au_ws = (int)ws; au_wt = (int)wt;
    return 1;
}

void pm_tex_unbind_aux(void) {
    pm_gpu_rect_sync(); /* DR1 */ au_last = NULL; C3D_TexBind(1, NULL); }

void pm_tex_aux_size(int* w, int* h) {
    pm_tex_epoch_v++;
    int pw = 1, ph = 1;
    if (aux_off >= 0 && aux_off < offt_n) {
        pw = offt[aux_off].w;
        ph = offt[aux_off].h;
    } else if (aux >= 0 && slots[aux].live) {
        pw = pot_at_least_8(slots[aux].w);
        ph = pot_at_least_8(slots[aux].h);
    }
    if (w) *w = pw;
    if (h) *h = ph;
}

int pm_tex_aux_valid(void) { return (aux_off >= 0 && aux_off < offt_n) || (aux >= 0 && slots[aux].live); }
void pm_tex_aux_origin(int* x, int* y) { *x = aux_off >= 0 ? aux_ox : 0; *y = aux_off >= 0 ? aux_oy : 0; }
