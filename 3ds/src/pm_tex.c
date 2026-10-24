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
#define SLOTS 512
#define BUDGET (1536u * 1024u) /* VRAM hot set (was 4 MB + 768 KB linear slab) */
#define MAX_DIM 512

typedef struct {
    const void* img;
    const void* tlut;
    unsigned fmt, siz;
    int w, h, stride;
    unsigned bytes;
    unsigned phash; /* V06: FNV-1a of the CI palette, so an in-place palette edit misses */
    unsigned dkey; /* 0 for a plain texture, else the hash of a shaded one */
    unsigned stamp;
    unsigned epoch;
    unsigned sgen; /* C1: bumped at drop and invalidation; TEX records compare this */
    unsigned uses; /* TC1: hits since the upload */
    unsigned last_frame; /* TC1: frame of the last hit, bind or upload */
    int live;
    int slab; /* FL1: the image lives in the texture slab */
    int vram_up; /* tiled bytes in vram_stage; draw binds tex_lin until upload_vram_slot before FrameBegin */
    u8* vram_stage;
    u8* tex_lin_heap; /* linearAlloc from C3D_TexInit(tex_lin); data points at vram_stage instead */
    int stale; /* invalidated while this frame's draws still use it: freed at the next frame start */
    C3D_Tex tex;
    C3D_Tex tex_lin;
} Slot;

static Slot slots[SLOTS];
static unsigned used;
#define RECENT_N 12
static short recent_slot[RECENT_N] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static unsigned recent_n;
static int slot_hi; /* every live slot index is below this, so scans stay short while few slots are live */
static unsigned used_peak; /* M02 measurement */
static unsigned map_peak;
/* V13c: bytes of the slots that a draw touched in this frame (the working set that eviction must keep), highest since the last map. */
static unsigned frame_bytes, map_wmax;
#define TOUCH(s) do { if ((s)->epoch != epoch) { frame_bytes += (s)->bytes; if (frame_bytes > map_wmax) map_wmax = frame_bytes; } (s)->epoch = epoch; } while (0)
static unsigned stamp;
static unsigned epoch;
/* TC1: cache statistics. The counters are plain increments. The report line (every 300 frames) prints with switch texstat or loglevel=debug. */
static unsigned tc_frame, tc_hits, tc_miss, tc_evict, tc_evict_ev0, tc_inval, tc_up_bytes, tc_evict_bytes, tc_ev0_bytes;
static int tc_on = -1;
#define TC_HIT(s) do { (s)->uses++; (s)->last_frame = tc_frame; tc_hits++; } while (0)
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
    return have && off_cur < 0 && cur >= 0 && cur < slot_hi && slots[cur].live && slots[cur].fmt == FMT_I;
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
        glyph_ox = glyph_oy = 0;
        return 0;
    }
    glyph_atlas_dims(g, &aw, &ah);
    glyph_ox = (idx % g->cols) * g->gw;
    glyph_oy = (idx / g->cols) * g->gh;
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
        s->tlut != tlut || s->phash != phash) return 0;
    glyph_ox = (idx % g->cols) * g->gw;
    glyph_oy = (idx / g->cols) * g->gh;
    if (gx) *gx = glyph_ox;
    if (gy) *gy = glyph_oy;
    s->stamp = ++stamp;
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
    while (p < n && p < MAX_DIM) p <<= 1;
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
    if (fmt == FMT_I && siz == SIZ_4) return GPU_L4;
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

static int expand(const u8* src, unsigned fmt, unsigned siz, int w, int h, int stride,
                  const u8* tlut, int tlut_n, int pw, int ph, GPU_TEXCOLOR gf) {
    if ((fmt == FMT_I && (siz == SIZ_8 || siz == SIZ_4)) || (fmt == FMT_IA && (siz == SIZ_8 || siz == SIZ_4)))
        return 0;
    unsigned out_bpp = (gf == GPU_RGBA8) ? 4 : (gf == GPU_L8 || gf == GPU_L4 || gf == GPU_LA4) ? 1 : 2;
    if ((unsigned)pw * (unsigned)ph * out_bpp > 256u * 256u * 4u) return 0;
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
    for (int y = 0; y < ph; y++) {
        int sy = y < h ? y : h - 1;
        if (mh) { int r = wrap_mod(y, mk_mt ? 2 * mh : mh); sy = (mk_mt && r >= mh) ? 2 * mh - 1 - r : r; }
        for (int x = (y < h ? w : 0); x < pw; x++) {
            int sx = x < w ? x : w - 1;
            if (mw) { int r = wrap_mod(x, mk_ms ? 2 * mw : mw); sx = (mk_ms && r >= mw) ? 2 * mw - 1 - r : r; }
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
    if ((unsigned)pw * (unsigned)ph * 4u > 256u * 256u * 4u) return 0;
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
static void addr_add(const void* p) {
    if (!p) return;
    addr_cnt[addr_h((unsigned)p)]++;
    addr_live++;
}
static void addr_sub(const void* p) {
    unsigned h;
    if (!p) return;
    h = addr_h((unsigned)p);
    if (addr_cnt[h]) addr_cnt[h]--;
    if (addr_live) addr_live--;
}


/* Texture pool in VRAM (GPU path). A miss writes its tiled image into this block. One merged
 * GSPGPU_FlushDataCache per span before each command list submit (pm_tex_flush_pending). */
#define SLAB_BYTES BUDGET
#define SLAB_EXT 640
#define PEND_N 24
#define PEND_GAP (64u * 1024u)
static u8* slab; static unsigned slab_n; /* slab_n: extents in use */
static struct { unsigned off, len; } slab_ext[SLAB_EXT];
static struct { u8* p; unsigned len; } pend[PEND_N]; static int pend_n;
static C3D_Tex slab_proto[16];
static unsigned fl1_srchash(const void* img, int w, int h, int stride, unsigned siz) { if (siz != 2) return 0; unsigned x = 2166136261u; const u8* q = (const u8*)img; for (int y = 0; y < h; y++) for (int i = 0; i < w * 2; i++) x = (x ^ q[y * stride * 2 + i]) * 16777619u; return x; }
static int fl1_state = -1; static unsigned fl1_sum, fl1_n;
static int fl1_on(void) { return slab != NULL; }
static void slab_init(void) {
    unsigned free0 = (unsigned)vramSpaceFree();
    slab = (u8*)vramMemAlign(SLAB_BYTES, 0x80);
    if (slab) {
        slab_ext[0].off = 0;
        slab_ext[0].len = SLAB_BYTES;
        slab_n = 1;
        pm_log_w("texcache VRAM %uKB at %p (vram free was %uKB now %uKB)", SLAB_BYTES >> 10, (void*)slab, free0 >> 10,
                 (unsigned)vramSpaceFree() >> 10);
    } else
        pm_log_e("texcache VRAM %uKB failed (vram free %uKB)", SLAB_BYTES >> 10, free0 >> 10);
}
static unsigned slab_size(unsigned bytes) { bytes = (bytes + 127u) & ~127u; return bytes < 512u ? 512u : bytes; } /* the GPU reads whole 8 x 8 tiles */
static u8* slab_alloc(unsigned bytes0) {
    unsigned bytes = slab_size(bytes0);
    int best = -1;
    for (unsigned i = 0; i < slab_n; i++) if (slab_ext[i].len >= bytes && (best < 0 || slab_ext[i].len < slab_ext[best].len)) best = (int)i;
    if (best < 0) return NULL;
    u8* r = slab + slab_ext[best].off;
    slab_ext[best].off += bytes; slab_ext[best].len -= bytes;
    if (!slab_ext[best].len) { for (unsigned i = (unsigned)best; i + 1 < slab_n; i++) slab_ext[i] = slab_ext[i + 1]; slab_n--; }
    return r;
}
static void slab_free(void* p, unsigned bytes) {
    bytes = slab_size(bytes);
    unsigned off = (unsigned)((u8*)p - slab), i = 0;
    while (i < slab_n && slab_ext[i].off < off) i++;
    if (i > 0 && slab_ext[i - 1].off + slab_ext[i - 1].len == off) { slab_ext[i - 1].len += bytes; i--; }
    else if (slab_n < SLAB_EXT) { for (unsigned k = slab_n; k > i; k--) slab_ext[k] = slab_ext[k - 1]; slab_ext[i].off = off; slab_ext[i].len = bytes; slab_n++; }
    else return; /* extent table full: the block is lost until the next boot */
    if (i + 1 < slab_n && slab_ext[i].off + slab_ext[i].len == slab_ext[i + 1].off) { slab_ext[i].len += slab_ext[i + 1].len; for (unsigned k = i + 1; k + 1 < slab_n; k++) slab_ext[k] = slab_ext[k + 1]; slab_n--; }
}
static void pend_flush_one(u8* p, unsigned len) { GSPGPU_FlushDataCache(p, len); PH_ADD(C_FLCALL, 1); PH_ADD(C_FLKB, len >> 10); }
static int tex_dim_pot(int n) { return n > 0 && (n & (n - 1)) == 0; }

static void upload_vram_slot(Slot* s) {
    if (!s->vram_up || !s->vram_stage || !s->bytes) return;
    int w = s->w;
    int h = s->h;
    if (!tex_dim_pot(w) || !tex_dim_pot(h)) {
        pm_log_w("texupload skip PICA copy %dx%d", w, h);
        return;
    }
    GSPGPU_FlushDataCache(s->vram_stage, s->bytes);
    C3D_TexLoadImage(&s->tex, s->vram_stage, GPU_TEXFACE_2D, 0);
    s->vram_up = 0;
}
static void flush_pend_only(void) {
    for (int i = 0; i < pend_n; i++) pend_flush_one(pend[i].p, pend[i].len);
    pend_n = 0;
}
int pm_tex_vram_pending(void) {
    for (int i = 0; i < slot_hi; i++)
        if (slots[i].vram_up) return 1;
    return 0;
}
int pm_tex_cur_vram_pending(void) {
    return have && cur >= 0 && cur < SLOTS && slots[cur].live && slots[cur].vram_up;
}
void pm_tex_upload_vram_before_frame(void) {
    for (int i = 0; i < slot_hi; i++) upload_vram_slot(&slots[i]);
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

static void drop(Slot* s) {
    if (!s->live) return;
    addr_sub(s->img);
    addr_sub(s->tlut);
    if (aux == (int)(s - slots)) aux = -1;
    if (s->vram_stage) {
        linearFree(s->vram_stage);
        s->vram_stage = NULL;
    }
    if (s->tex_lin_heap) {
        linearFree(s->tex_lin_heap);
        s->tex_lin_heap = NULL;
    }
    memset(&s->tex_lin, 0, sizeof s->tex_lin);
    s->vram_up = 0;
    if (s->slab) {
        slab_free(s->tex.data, s->bytes);
        s->slab = 0;
        memset(&s->tex, 0, sizeof s->tex);
    }
    tb_last = NULL;
    if (used >= s->bytes) used -= s->bytes;
    else used = 0;
    s->sgen++;
    s->live = 0;
    s->stale = 0;
    while (slot_hi > 0 && !slots[slot_hi - 1].live) slot_hi--;
}

unsigned pm_tex_slot_gen(int i) {
    if (i < 0 || i >= SLOTS || !slots[i].live) return 0;
    return slots[i].sgen;
}

static void slot_tex_lin_build(Slot* s, GPU_TEXCOLOR gf, int pw, int ph, unsigned bytes) {
    if (s->tex_lin_heap) {
        linearFree(s->tex_lin_heap);
        s->tex_lin_heap = NULL;
    }
    memset(&s->tex_lin, 0, sizeof s->tex_lin);
    if (!C3D_TexInit(&s->tex_lin, (u16)pw, (u16)ph, gf)) return;
    s->tex_lin_heap = (u8*)s->tex_lin.data;
    s->tex_lin.data = s->vram_stage;
    s->tex_lin.size = bytes;
    s->tex_lin.width = (u16)pw;
    s->tex_lin.height = (u16)ph;
}

static const C3D_Tex* tex_for_draw(Slot* s) {
    if (s->vram_up && s->vram_stage && s->bytes && s->tex_lin.data == s->vram_stage) {
        GSPGPU_FlushDataCache(s->vram_stage, s->bytes);
        return &s->tex_lin;
    }
    return &s->tex;
}

int pm_tex_bind_slot(int i) {
    if (i < 0 || i >= SLOTS || !slots[i].live) return 0;
    cur = i;
    TOUCH(&slots[i]);
    TC_HIT(&slots[i]);
    const C3D_Tex* tp = tex_for_draw(&slots[i]);
    C3D_TexBind(0, tp);
    tb_last = (slots[i].vram_up && slots[i].vram_stage) ? NULL : &slots[i].tex;
    pm_tex_epoch_v++;
    return 1;
}

/* A texture bound in the current command list has to stay alive until the next
 * frame begins. Deleting it earlier makes the GPU sample freed VRAM. */
static int evict_oldest_done(void) {
    int oldest = -1;
    for (int i = 0; i < slot_hi; i++) {
        if (!slots[i].live || slots[i].epoch == epoch) continue;
        if (oldest < 0 || slots[i].stamp < slots[oldest].stamp) oldest = i;
    }
    if (oldest < 0) return 0;
    if (oldest == cur) cur = -1;
    tc_evict++;
    tc_evict_bytes += slots[oldest].bytes;
    if (slots[oldest].uses == 0) { tc_evict_ev0++; tc_ev0_bytes += slots[oldest].bytes; }
    drop(&slots[oldest]);
    return 1;
}

/* PG2L: baked textures (docs/plans/PG2_loader.md). Switches: texdump (write every plain miss as a request to texreq.bin), texbaked (read the pack
 * texbake.pak on a miss), texbakecheck (as texbaked, but decode too and compare). A plain texture is one without lut, not a frame buffer image. */
enum { TBM_DUMP = 1, TBM_USE = 2, TBM_CHECK = 4 };
static int tb_mode;
static FILE *tb_pak, *tb_dump;
static TbRec* tb_idx;
static unsigned tb_n, tb_hits, tb_miss, tb_bad, tb_dumped;
static u8* tb_cmp;
static int tb_fd = -1; /* data reads use read(): one file system call for the whole texture (stdio reads in small chunks) */
static uint64_t* tb_seen; /* texdump: keys already written (open addressing, 0 = empty) */
#define TB_SEEN_N 16384u

static void tb_init(void) {
    tb_mode = (pm_debug_has("texdump") ? TBM_DUMP : 0) | (pm_debug_has("texbaked") ? TBM_USE : 0) | (pm_debug_has("texbakecheck") ? TBM_CHECK : 0);
    if (tb_mode & TBM_DUMP) {
        tb_dump = fopen("texreq.bin", "wb");
        tb_seen = (uint64_t*)calloc(TB_SEEN_N, sizeof(uint64_t));
        if (tb_dump && tb_seen) { uint32_t h[2] = { TB_REQ_MAGIC, 0xffffffffu }; fwrite(h, 4, 2, tb_dump); fflush(tb_dump); }
        else { pm_log_w("PG2L: texdump cannot start"); if (tb_dump) fclose(tb_dump); tb_dump = NULL; tb_mode &= ~TBM_DUMP; }
    }
    if (tb_mode & (TBM_USE | TBM_CHECK)) {
        TbHead h;
        tb_pak = fopen("texbake.pak", "rb");
        if (tb_pak && fread(&h, sizeof h, 1, tb_pak) == 1 && h.magic == TB_MAGIC && h.version == TB_VERSION && h.count && h.count < 200000u) {
            tb_idx = (TbRec*)malloc(h.count * sizeof(TbRec));
            if (tb_idx && fread(tb_idx, sizeof(TbRec), h.count, tb_pak) == h.count) tb_n = h.count;
        }
        if (tb_mode & TBM_CHECK) tb_cmp = (u8*)malloc(256 * 256 * 4);
        if (!tb_n || ((tb_mode & TBM_CHECK) && !tb_cmp)) {
            pm_log_w("PG2L: texbake.pak not usable, decode as before");
            if (tb_pak) fclose(tb_pak);
            tb_pak = NULL; tb_mode &= ~(TBM_USE | TBM_CHECK); tb_n = 0;
        } else { tb_fd = open("texbake.pak", O_RDONLY); if (tb_fd < 0) { pm_log_w("PG2L: texbake.pak not usable, decode as before"); tb_mode &= ~(TBM_USE | TBM_CHECK); tb_n = 0; } else pm_log_d("PG2L: pack %u textures", tb_n); }
    }
}

static const TbRec* tb_find(uint64_t key) {
    unsigned lo = 0, hi = tb_n;
    while (lo < hi) {
        unsigned m = (lo + hi) / 2;
        if (tb_idx[m].key < key) lo = m + 1; else hi = m;
    }
    return (lo < tb_n && tb_idx[lo].key == key) ? &tb_idx[lo] : NULL;
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
    scratch = (u8*)linearAlloc(256 * 256 * 4 * 2);
    slab_init();
    tb_init();
}

unsigned pm_tex_used(void) { return used; }
unsigned pm_tex_budget(void) { return BUDGET; }
/* H03: the last load failed (cache full or no VRAM). The draw skips the batch, no white quad. */
static int fail_last, fail_n;
int pm_tex_failed(void) { return fail_last; }
unsigned pm_tex_fail_count(void) { return (unsigned)fail_n; }
unsigned pm_tex_peak(void) { return used_peak; }
/* V13c: highest cache use since the last call, then restart from the current use. */
unsigned pm_tex_map_peak_take(unsigned* wmax) { unsigned p = map_peak; *wmax = map_wmax; map_peak = used; map_wmax = 0; return p; }

static void tc_report(void) {
    unsigned n = 0, bytes = 0, u0n = 0, u0b = 0, rec_b = 0, rec_n = 0;
    unsigned an[5] = { 0 }, ab[5] = { 0 }; /* age in frames: <=1, <=10, <=100, <=300, older */
    int top[3] = { -1, -1, -1 };
    for (int i = 0; i < slot_hi; i++) {
        Slot* s = &slots[i];
        unsigned age, b;
        if (!s->live) continue;
        age = tc_frame - s->last_frame;
        b = s->bytes;
        n++;
        bytes += b;
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
    pm_log_d("texstat f%u: hits %u miss %u evict %u (%uKB) never hit before evict %u (%uKB) invalidated %u uploaded %uKB",
             tc_frame, tc_hits, tc_miss, tc_evict, tc_evict_bytes >> 10, tc_evict_ev0, tc_ev0_bytes >> 10, tc_inval, tc_up_bytes >> 10);
    pm_log_d("texstat live: %u slots %uKB, never hit %u (%uKB), used in last 300 frames %u (%uKB)", n, bytes >> 10, u0n, u0b >> 10, rec_n, rec_b >> 10);
    pm_log_d("texstat age (frames since last use) <=1: %u/%uKB, <=10: %u/%uKB, <=100: %u/%uKB, <=300: %u/%uKB, older: %u/%uKB",
             an[0], ab[0] >> 10, an[1], ab[1] >> 10, an[2], ab[2] >> 10, an[3], ab[3] >> 10, an[4], ab[4] >> 10);
    for (int t = 0; t < 3; t++)
        if (top[t] >= 0) pm_log_d("texstat largest %d: %dx%d fmt %u siz %u %uKB uses %u age %u", t, slots[top[t]].w, slots[top[t]].h, slots[top[t]].fmt, slots[top[t]].siz,
                                  slots[top[t]].bytes >> 10, slots[top[t]].uses, tc_frame - slots[top[t]].last_frame);
    pm_log_hw("tx1 f%u miss %u KB %u us %u ns/B %u%s", tc_frame, tx1_miss, tx1_kb, tx1_ticks / 268u, tx1_kb ? (unsigned)((unsigned long long)tx1_ticks * 1000ull / 268ull / (tx1_kb * 1024ull)) : 0u, "");
    if (PMDL("tx1check")) pm_log_hw("tx1check mismatch %u", tx1_mismatch);
    tx1_miss = tx1_kb = tx1_ticks = 0;
    tc_hits = tc_miss = tc_evict = tc_evict_ev0 = tc_inval = tc_up_bytes = tc_evict_bytes = tc_ev0_bytes = 0;
}

void pm_tex_begin_frame(void) {
    pm_tex_epoch_v++;
    pm_tex_wrap_gen_v++;
    tc_frame++;
    if (tc_on < 0) tc_on = pm_debug_has("texstat") || pm_log_level() >= 3;
    if (tc_on && tc_frame % 300 == 0) tc_report();
    epoch++;
    frame_bytes = 0;
    /* The previous frame has finished on the GPU (C3D_FRAME_SYNCDRAW), so deferred frees are safe now. */
    for (int i = 0; i < slot_hi; i++)
        if (slots[i].live && slots[i].stale) drop(&slots[i]);
}

extern unsigned pm_dlc_gen; /* DLC1: a write into the map shape buffer (ROM read, Yay0 decode) makes the compiled display lists stale */
extern u8 TextureHeap[], pm_world_entity_heap[], heap_generalHead[], heap_spriteHead[];
unsigned pm_pdl_heap_gen = 1; /* C1: TextureHeap writes bump this; texheap state-list entries recompile */
unsigned pm_pdl_entity_gen = 1;
unsigned pm_pdl_genheap_gen = 1;
unsigned pm_pdl_sprite_gen = 1;

void pm_pdl_genheap_bump(void) { pm_pdl_genheap_gen++; }

void pm_pdl_touch_range(const void* ptr, unsigned size) {
    unsigned s, e;
    if (!ptr || !size) return;
    s = (unsigned)ptr;
    e = s + size;
    if (e < s) e = 0xffffffffu;
    if (s - (unsigned)pm_world_entity_heap < 0x18000u && e > (unsigned)pm_world_entity_heap) pm_pdl_entity_gen++;
    if (s - (unsigned)heap_generalHead < 0x180000u && e > (unsigned)heap_generalHead) pm_pdl_genheap_gen++;
    if (s - (unsigned)heap_spriteHead < 0x100000u && e > (unsigned)heap_spriteHead) pm_pdl_sprite_gen++;
}
extern unsigned char gMapShapeData[];
void pm_tex_invalidate(const void* ptr, unsigned size) {
    pm_tex_epoch_v++;
    if (ptr && size && (unsigned)ptr < (unsigned)gMapShapeData + 0x40000u && (unsigned)ptr + size > (unsigned)gMapShapeData) pm_dlc_gen++;
    unsigned start, end;
    int i;
    if (!ptr || !size) return;
    if ((unsigned)ptr - (unsigned)TextureHeap < 0x68000u && (unsigned)ptr + size > (unsigned)TextureHeap) pm_pdl_heap_gen++;
    pm_pdl_touch_range(ptr, size);
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
        if (!s->live) continue;
        img = (unsigned)s->img;
        pal = (unsigned)s->tlut;
        if (!((img >= start && img < end) || (pal && pal >= start && pal < end))) continue;
        pm_gpu_rect_sync(); /* DR1: draw a queued rectangle batch before its texture goes away */
        tc_inval++;
        if (aux == i) aux = -1;
        if (cur == i) {
            cur = -1;
            have = 0;
        }
        if (s->epoch == epoch) {
            /* A draw of this frame may still sample it (the GPU runs at FrameEnd). Hide it from the cache and free it later. */
            addr_sub(s->img);
            addr_sub(s->tlut);
            s->img = NULL;
            s->tlut = NULL;
            s->sgen++;
            s->stale = 1;
            continue;
        }
        drop(s);
    }
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
    if (tb_mode) pm_log("PG2L: baked hits %u, not in pack %u, check mismatches %u, requests written %u", tb_hits, tb_miss, tb_bad, tb_dumped);
    tprof_hit_t = tprof_hit_n = tprof_miss_t = tprof_miss_n = 0;
}
static int load_impl(const void* img, unsigned fmt, unsigned siz, int width, int height,
                     const void* tlut, int tlut_n, int stride, const unsigned* lut, int dx4, int dy4,
                     unsigned dkey) {
    pm_gpu_rect_sync(); /* DR1 */
    pm_tex_epoch_v++;
    int r;
    static int tp = -1;
    if (tp < 0) tp = pm_debug_has("gbiprof");
    unsigned t0 = tp ? pm_ticks() : 0;
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
    if (tp) { /* PG2 measure: ticks inside pm_tex_load, hit and miss, per 300 frames (reported by pm_tex_prof_report) */
        unsigned dt = pm_ticks() - t0;
        if (ph_miss) { tprof_miss_t += dt; tprof_miss_n++; } else { tprof_hit_t += dt; tprof_hit_n++; }
    }
    return r;
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
    if (width > MAX_DIM || height > MAX_DIM) {
        static int once;
        if (!once) {
            once = 1;
            pm_log_w("tex %dx%d skipped\n", width, height);
        }
        return 0;
    }
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
            s->w == width && s->h == height && s->stride == stride && s->dkey == dkey) {
            s->stamp = ++stamp;
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
            s->w == width && s->h == height && s->stride == stride && s->dkey == dkey) {
            s->stamp = ++stamp;
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
            s->w == width && s->h == height && s->stride == stride && s->dkey == dkey) {
            s->stamp = ++stamp;
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
    src_native = pm_fb_native(img);
    PH_PUSH(PH_TEX);
    PH_ADD(C_TEXMISS, 1);
    int pw = pot_at_least_8(width);
    int ph = pot_at_least_8(height);
    GPU_TEXCOLOR gf = gpu_fmt(fmt, siz);
    if (fmt == FMT_CI) gf = GPU_RGBA5551;
    if (lut) gf = GPU_RGBA8;
    const TbRec* tr = NULL; /* PG2L: the baked record of this texture, tb_use = its bytes are in the upload buffer and the decode is skipped */
    int tb_use = 0;
    if (tb_mode && !lut && !src_native) {
        TbReq q;
        memset(&q, 0, sizeof q);
        q.fmt = (u8)fmt; q.siz = (u8)siz; q.ms = (u8)mk_ms; q.mt = (u8)mk_mt; q.w = (u16)width; q.h = (u16)height; q.stride = (u16)stride;
        q.mw = (u16)((mk_on && mk_ws && (int)mk_ws < width) ? mk_ws : 0); q.mh = (u16)((mk_on && mk_wt && (int)mk_wt < height) ? mk_wt : 0);
        q.tlut_n = (u16)((fmt == FMT_CI && tlut) ? tlut_n : 0);
        uint64_t key = tb_key(&q, (const u8*)tlut, (const u8*)img);
        if (tb_mode & TBM_DUMP) tb_dump_req(key, &q, (const u8*)tlut, (const u8*)img);
        if (tb_mode & (TBM_USE | TBM_CHECK)) {
            unsigned bb = (gf == GPU_RGBA8) ? 4u : 2u;
            tr = tb_find(key);
            if (tr && (tr->pw != pw || tr->ph != ph || tr->bytes != (unsigned)pw * (unsigned)ph * bb || tr->gf != (gf == GPU_RGBA8 ? TB_GF_RGBA8 : gf == GPU_LA8 ? TB_GF_LA8 : TB_GF_RGBA5551))) tr = NULL;
            if (tr) {
                u8* dst = (tb_mode & TBM_CHECK) ? tb_cmp : scratch + (256 * 256 * 4);
                if (lseek(tb_fd, (off_t)tr->off, SEEK_SET) == (off_t)tr->off && read(tb_fd, dst, tr->bytes) == (ssize_t)tr->bytes) tb_use = !(tb_mode & TBM_CHECK);
                else tr = NULL;
            }
            if (tr) tb_hits++; else tb_miss++;
        }
    }
    /* TX1: one pass expand and tile into the texture. Switch tx1check runs the old expand as well and compares (a test tool). */
    int native_tex = (gf == GPU_L8 || gf == GPU_L4 || gf == GPU_LA4);
    unsigned bpp0 = (gf == GPU_RGBA8) ? 4u : native_tex ? 1u : 2u;
    unsigned tex_bytes_need = tex_gpu_bytes(pw, ph, gf);
    int tx1 = !tb_use && !tr && !tb_mode && tex_bytes_need <= 256u * 256u * 4u && pw <= TX1_MAXD && ph <= TX1_MAXD;
    int tx1_chkon = tx1 && PMDL("tx1check");
    if (tx1_chkon && !tx1_chk) { tx1_chk = (u8*)malloc(512 * 1024); if (!tx1_chk) { tx1 = 0; tx1_chkon = 0; } }
    unsigned tx1_t0 = tc_on > 0 ? pm_ticks() : 0;
    if (tb_use) {
        /* the bytes are in the second scratch half already */
    } else if (tx1 && !tx1_chkon && (dkey & 0xFF000000u) != 0xC7000000u) {
        /* TX1: decoded after the texture exists */
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
    unsigned bytes = tex_gpu_bytes(pw, ph, gf);
    unsigned bpp = (gf == GPU_RGBA8) ? 4u : (gf == GPU_L8 || gf == GPU_LA4) ? 1u : (gf == GPU_L4) ? 1u : 2u;
    int slot = -1;
    for (;;) {
        for (int i = 0; i < SLOTS; i++) if (!slots[i].live) { slot = i; break; }
        if (slot >= 0 && used + bytes <= BUDGET) break;
        if (!evict_oldest_done()) {
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
    tb_last = NULL; /* DR1 */
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
    { /* image in the VRAM pool; header template per format */
        u8* sp = slab_alloc(bytes);
        while (!sp && evict_oldest_done()) {
            static int nev;
            if (nev++ < 8) pm_log_w("texcache VRAM full for %u bytes (used %u), evicted one texture", bytes, used);
            sp = slab_alloc(bytes);
        }
        if (!sp) {
            static int once;
            fail_last = 1;
            fail_n++;
            if (!once) {
                once = 1;
                pm_log_w("texcache VRAM full %dx%d", width, height);
            }
            return 0;
        }
        if (!slab_proto[gf].data) {
            if (!C3D_TexInit(&slab_proto[gf], 8, 8, gf)) {
                slab_free(sp, bytes);
                fail_last = 1;
                fail_n++;
                return 0;
            }
        }
        s->tex = slab_proto[gf];
        s->tex.data = sp;
        s->tex.width = (u16)pw;
        s->tex.height = (u16)ph;
        s->tex.size = bytes;
        s->slab = 1;
    }
    C3D_TexSetFilter(&s->tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&s->tex, GPU_REPEAT, GPU_REPEAT);
    if (tx1) { /* TX1: tile in linear scratch; VRAM slot is filled by C3D_TexLoadImage (O3DS cannot CPU-store into vramMemAlign) */
        u8* t1dst = tx1_chkon ? tx1_chk : (scratch + (256 * 256 * 4));
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
                tx1_build_native(t1dst, gf, (const u8*)img, width, height, stride, pw, ph, mw1, mh1, (int)mk_ms, (int)mk_mt);
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
    const u8* staged = scratch + (256 * 256 * 4);
    if (!tx1 || tx1_chkon) {
        const void* upload = scratch;
        /* Scratch is two 256KB halves. The linear image fills the first, the
         * Morton copy the second. A 512x256 RGBA5551 background is 256KB, so the
         * old 128KB tile buffer skipped tiling and the GPU scattered the pixels. */
        if (tb_use) {
            upload = scratch + (256 * 256 * 4);
        } else if (bpp == 2u && (unsigned)pw * (unsigned)ph * 2u <= 256u * 256u * 4u) {
            u8* tiled = scratch + (256 * 256 * 4);
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
        } else if (bpp == 4u && (unsigned)pw * (unsigned)ph * 4u <= 256u * 256u * 4u) {
            u8* tiled = scratch + (256 * 256 * 4);
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
        } else if (bpp == 1u && (unsigned)pw * (unsigned)ph <= 256u * 256u * 4u) {
            u8* tiled = scratch + (256 * 256 * 4);
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
        if (tr && !tb_use && memcmp(upload, tb_cmp, tr->bytes) != 0) { /* texbakecheck */
            if (tb_bad++ < 8) pm_log_w("PG2L: baked bytes differ fmt %u siz %u %dx%d stride %d", fmt, siz, width, height, stride);
        }
        staged = (const u8*)upload;
    }
    if (s->vram_stage) linearFree(s->vram_stage);
    s->vram_stage = (u8*)linearAlloc(bytes);
    if (!s->vram_stage) {
        fail_last = 1;
        fail_n++;
        return 0;
    }
    memcpy(s->vram_stage, staged, bytes);
    pend_add(s->vram_stage, bytes);
    s->bytes = bytes;
    slot_tex_lin_build(s, gf, pw, ph, bytes);
    s->vram_up = 1;
    if (PMDL("fl1log")) { unsigned h = 2166136261u; const u8* q = (const u8*)s->tex.data; for (unsigned i = 0; i < bytes; i++) h = (h ^ q[i]) * 16777619u; fl1_sum += h * (fl1_n + 1u); fl1_n++; if (fl1_n <= 400) pm_log_w("fl1log up %u slot %d %dx%d fmt %u siz %u bytes %u hash %08x src %dx%d stride %d img %p srchash %08x", fl1_n, slot, pw, ph, fmt, siz, bytes, h, width, height, stride, img, fl1_srchash(img, width, height, stride, siz)); }
    if (tx1_chkon && memcmp(tx1_chk, s->vram_stage, bytes) != 0) {
        if (tx1_mismatch++ < 8) pm_log_w("tx1check: bytes differ fmt %u siz %u %dx%d stride %d lut %d", fmt, siz, width, height, stride, lut != NULL);
    }
    if (tx1_t0) { tx1_miss++; tx1_kb += bytes >> 10; tx1_ticks += pm_ticks() - tx1_t0; }
    s->img = img;
    s->tlut = tlut;
    addr_add(img);
    addr_add(tlut);
    s->phash = phash;
    s->fmt = fmt;
    s->siz = siz;
    s->w = width;
    s->h = height;
    s->stride = stride;
    s->dkey = dkey;
    s->bytes = bytes;
    PH_ADD(C_TEXBYTE, bytes >> 10);
    s->stamp = ++stamp;
    s->epoch = epoch;
    s->uses = 0;
    s->last_frame = tc_frame;
    tc_miss++;
    tc_up_bytes += bytes;
    frame_bytes += bytes;
    if (frame_bytes > map_wmax) map_wmax = frame_bytes;
    s->live = 1;
    if (slot >= slot_hi) slot_hi = slot + 1;
    used += bytes;
    if (used > used_peak) used_peak = used;
    if (used > map_peak) map_peak = used;
    cur = slot;
    recent_slot[recent_n++ % RECENT_N] = (short)slot; /* PG2 */
    lidx[lk] = (short)slot;
    have = 1;
    return 1;
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
                s->h == h && s->stride == w) {
                s->stamp = ++stamp;
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
void pm_tex_bind_reset(void) { tb_last = NULL; }
int pm_tex_bind(void) {
    if (have && off_cur >= 0) {
        if (tb_last == offt[off_cur].tex && tb_f == -2) return 1;
        C3D_TexBind(0, offt[off_cur].tex);
        tb_last = offt[off_cur].tex; tb_f = -2;
        return 1;
    }
    if (!have || cur < 0 || !slots[cur].live) return 0;
    slots[cur].stamp = ++stamp;
    slots[cur].last_frame = tc_frame;
    GPU_TEXTURE_WRAP_PARAM ws = shr_s ? (mk_ms ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : wrap_s, wt = shr_t ? (mk_mt ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : wrap_t;
    if (!slots[cur].vram_up && tb_last == &slots[cur].tex && tb_f == (int)tex_filt && tb_ws == (int)ws && tb_wt == (int)wt) return 1;
    const C3D_Tex* tp = tex_for_draw(&slots[cur]);
    C3D_TexSetFilter(tp, tex_filt, tex_filt);
    C3D_TexSetWrap(tp, ws, wt);
    C3D_TexBind(0, tp);
    tb_last = (slots[cur].vram_up && slots[cur].vram_stage) ? NULL : &slots[cur].tex;
    tb_f = (int)tex_filt; tb_ws = (int)ws; tb_wt = (int)wt;
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

int pm_tex_bind_aux(void) {
    if (aux_off >= 0 && aux_off < offt_n) { C3D_TexBind(1, offt[aux_off].tex); return 1; }
    if (aux < 0 || !slots[aux].live) return 0;
    slots[aux].stamp = ++stamp;
    TOUCH(&slots[aux]);
    tb_last = NULL; /* DR1: this changes the filter and wrap of a texture that unit 0 may use */
    const C3D_Tex* tp = tex_for_draw(&slots[aux]);
    C3D_TexSetFilter(tp, tex_filt, tex_filt);
    C3D_TexSetWrap(tp, aux_shr_s ? (mk_ms ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : aux_ws, aux_shr_t ? (mk_mt ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : aux_wt);
    C3D_TexBind(1, tp);
    return 1;
}

void pm_tex_unbind_aux(void) {
    pm_gpu_rect_sync(); /* DR1 */ C3D_TexBind(1, NULL); }

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
