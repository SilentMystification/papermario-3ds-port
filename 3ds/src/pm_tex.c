#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "pm_tex.h"
#include "pm_port.h"

enum { FMT_RGBA, FMT_YUV, FMT_CI, FMT_IA, FMT_I };
enum { SIZ_4, SIZ_8, SIZ_16, SIZ_32 };

/* One frame can draw more distinct textures than the N64 TMEM suggests: a Bowser silhouette (many
 * background strips) plus a line of message glyphs went past 128 and the glyph quads drew white (V03). */
#define SLOTS 512
#define BUDGET (4u * 1024u * 1024u)
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
    int live;
    int stale; /* invalidated while this frame's draws still use it: freed at the next frame start */
    C3D_Tex tex;
} Slot;

static Slot slots[SLOTS];
static unsigned used;
static int slot_hi; /* every live slot index is below this, so scans stay short while few slots are live */
static unsigned used_peak; /* M02 measurement */
static unsigned map_peak;
/* V13c: bytes of the slots that a draw touched in this frame (the working set that eviction must keep), highest since the last map. */
static unsigned frame_bytes, map_wmax;
#define TOUCH(s) do { if ((s)->epoch != epoch) { frame_bytes += (s)->bytes; if (frame_bytes > map_wmax) map_wmax = frame_bytes; } (s)->epoch = epoch; } while (0)
static unsigned stamp;
static unsigned epoch;
static u8* scratch;
static int cur = -1;
static int aux = -1; /* S25: the second texture (TEXEL1), bound at unit 1 */
static GPU_TEXTURE_WRAP_PARAM aux_ws = GPU_REPEAT, aux_wt = GPU_REPEAT;
static int have;
/* V05: off-screen images of this frame (render textures owned by pm_gpu.c) */
static struct { const u8* base; unsigned stride; C3D_Tex* tex; int siz, w, h; } offt[8]; /* siz as pm_gpu_off_begin */
static int offt_n, off_cur = -1, off_ox, off_oy;
static int aux_off = -1, aux_ox, aux_oy; /* V12c: TEXEL1 is an off-screen slot (the grab of the frame buffer) */
void pm_tex_set_offscreen(int i, const void* base, unsigned stride, void* tex, int siz, int w, int h) {
    offt[i].siz = siz;
    offt[i].w = w;
    offt[i].h = h;
    offt[i].base = (const u8*)base;
    offt[i].stride = stride;
    offt[i].tex = (C3D_Tex*)tex;
    if (i >= offt_n) offt_n = i + 1;
}
void pm_tex_clear_offscreen(void) { for (int k = 0; k < 8; k++) offt[k].base = NULL; offt_n = 0; off_cur = -1; aux_off = -1; }
void pm_tex_origin(int* x, int* y) {
    *x = (have && off_cur >= 0) ? off_ox : 0;
    *y = (have && off_cur >= 0) ? off_oy : 0;
}
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

void pm_tex_set_wrap(unsigned cms, unsigned cmt, unsigned masks, unsigned maskt) {
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
    if (fmt == FMT_IA && siz == SIZ_8) return GPU_LA8;
    if (fmt == FMT_IA && siz == SIZ_4) return GPU_LA8;
    /* The RDP gives an I texel an alpha equal to its intensity. PICA L8 has alpha 1. */
    if (fmt == FMT_I && (siz == SIZ_8 || siz == SIZ_4)) return GPU_LA8;
    return GPU_RGBA5551;
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

static int expand(const u8* src, unsigned fmt, unsigned siz, int w, int h, int stride,
                  const u8* tlut, int tlut_n, int pw, int ph, GPU_TEXCOLOR gf) {
    unsigned out_bpp = (gf == GPU_RGBA8) ? 4 : (gf == GPU_L8 || gf == GPU_L4 || gf == GPU_LA4) ? 1 : 2;
    if ((unsigned)pw * (unsigned)ph * out_bpp > 256u * 256u * 4u) return 0;
    memset(scratch, 0, (size_t)pw * (size_t)ph * out_bpp);
    int mw = (mk_on && mk_ws && (int)mk_ws < w) ? (int)mk_ws : 0;
    int mh = (mk_on && mk_wt && (int)mk_wt < h) ? (int)mk_wt : 0;
    for (int y = 0; y < h; y++) {
        int sy = y;
        if (mh) { int r = mk_mt ? y % (2 * mh) : y % mh; sy = (mk_mt && r >= mh) ? 2 * mh - 1 - r : r; }
        for (int x = 0; x < w; x++) {
            int sx = x;
            if (mw) { int r = mk_ms ? x % (2 * mw) : x % mw; sx = (mk_ms && r >= mw) ? 2 * mw - 1 - r : r; }
            if (fmt == FMT_CI) {
                unsigned idx = 0;
                if (siz == SIZ_4) {
                    unsigned byte = src[(sy * stride + sx) >> 1];
                    idx = (sx & 1) ? (byte & 15) : (byte >> 4);
                } else {
                    idx = src[sy * stride + sx];
                }
                unsigned pix = (tlut && (int)idx < tlut_n) ? rd16(tlut + idx * 2) : 0xffffu;
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
            } else if (fmt == FMT_IA && siz == SIZ_8) {
                unsigned c = src[sy * stride + sx];
                unsigned i = (c >> 4) * 17;
                unsigned a = (c & 15) * 17;
                u8* d = scratch + (y * pw + x) * 2;
                d[0] = (u8)a;
                d[1] = (u8)i;
            } else if (fmt == FMT_IA && siz == SIZ_4) {
                unsigned byte = src[(sy * stride + sx) >> 1];
                unsigned nib = (sx & 1) ? (byte & 15) : (byte >> 4);
                unsigned i = (nib >> 1) * 36;
                unsigned a = (nib & 1) ? 255 : 0;
                u8* d = scratch + (y * pw + x) * 2;
                d[0] = (u8)a;
                d[1] = (u8)i;
            } else if (fmt == FMT_I && siz == SIZ_8) {
                u8 c = src[sy * stride + sx];
                u8* d = scratch + (y * pw + x) * 2;
                d[0] = c;
                d[1] = c;
            } else if (fmt == FMT_I && siz == SIZ_4) {
                unsigned byte = src[(sy * stride + sx) >> 1];
                unsigned nib = (sx & 1) ? (byte & 15) : (byte >> 4);
                u8* d = scratch + (y * pw + x) * 2;
                d[0] = (u8)(nib * 17);
                d[1] = (u8)(nib * 17);
            } else {
                write5551(scratch + ((y * pw + x) * 2), 0xffffu);
            }
        }
    }
    /* Pad to the power-of-two size with the last texel, so a clamped coordinate gets the edge. */
    for (int y = 0; y < ph; y++) {
        int sy = y < h ? y : h - 1;
        for (int x = (y < h ? w : 0); x < pw; x++) {
            int sx = x < w ? x : w - 1;
            memcpy(scratch + ((size_t)y * pw + x) * out_bpp, scratch + ((size_t)sy * pw + sx) * out_bpp, out_bpp);
        }
    }
    return 1;
}

static int nib(const u8* src, int stride, int x, int y) {
    unsigned byte = src[(y * stride + x) >> 1];
    return (x & 1) ? (byte & 15) : (byte >> 4);
}

static int floor4(int v) { return v >= 0 ? v / 4 : -((-v + 3) / 4); }

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

static void drop(Slot* s) {
    if (!s->live) return;
    if (aux == (int)(s - slots)) aux = -1;
    C3D_TexDelete(&s->tex);
    if (used >= s->bytes) used -= s->bytes;
    else used = 0;
    s->live = 0;
    s->stale = 0;
    while (slot_hi > 0 && !slots[slot_hi - 1].live) slot_hi--;
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
    drop(&slots[oldest]);
    return 1;
}

void pm_tex_init(void) {
    /* Linear image, then a second copy in PICA tile order. */
    scratch = (u8*)linearAlloc(256 * 256 * 4 * 2);
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

void pm_tex_begin_frame(void) {
    epoch++;
    frame_bytes = 0;
    /* The previous frame has finished on the GPU (C3D_FRAME_SYNCDRAW), so deferred frees are safe now. */
    for (int i = 0; i < slot_hi; i++)
        if (slots[i].live && slots[i].stale) drop(&slots[i]);
}

void pm_tex_invalidate(const void* ptr, unsigned size) {
    unsigned start, end;
    int i;
    if (!ptr || !size) return;
    start = (unsigned)ptr;
    end = start + size;
    if (end < start) end = 0xffffffffu;
    for (i = 0; i < slot_hi; i++) {
        Slot* s = &slots[i];
        unsigned img, pal;
        if (!s->live) continue;
        img = (unsigned)s->img;
        pal = (unsigned)s->tlut;
        if (!((img >= start && img < end) || (pal && pal >= start && pal < end))) continue;
        if (aux == i) aux = -1;
        if (cur == i) {
            cur = -1;
            have = 0;
        }
        if (s->epoch == epoch) {
            /* A draw of this frame may still sample it (the GPU runs at FrameEnd). Hide it from the cache and free it later. */
            s->img = NULL;
            s->tlut = NULL;
            s->stale = 1;
            continue;
        }
        drop(s);
    }
}

static int ph_miss;
static int load_inner(const void* img, unsigned fmt, unsigned siz, int width, int height,
                      const void* tlut, int tlut_n, int stride, const unsigned* lut, int dx4, int dy4,
                      unsigned dkey);
/* PT01: a cache hit is only counted. A miss (convert, tile, upload) runs under the tex phase. */
static int load_impl(const void* img, unsigned fmt, unsigned siz, int width, int height,
                     const void* tlut, int tlut_n, int stride, const unsigned* lut, int dx4, int dy4,
                     unsigned dkey) {
    int r;
    ph_miss = 0;
    PH_ADD(C_TEXCALL, 1);
    r = load_inner(img, fmt, siz, width, height, tlut, tlut_n, stride, lut, dx4, dy4, dkey);
    if (ph_miss) PH_POP();
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
                off_oy = (int)(d / row);
                off_ox = (int)((d % row) / bpp);
                off_cur = i;
                if (offt[i].w == 64) { static int n; if (n++ < 6) pm_log("X02: overlay reads 64x64 slot %d bits %d addr %p", i, gs == 3 ? 32 : gs == 2 ? 16 : 8, img); }
                have = 1;
                return 1;
            }
        }
    }
    if ((unsigned)img < 0x00100000u) {
        static int once;
        if (!once) {
            once = 1;
            pm_log("tex ptr %08x %dx%d fmt %u", (unsigned)img, width, height, fmt);
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
            pm_log("tex %dx%d skipped\n", width, height);
        }
        return 0;
    }
    unsigned phash = 0;
    if (fmt == FMT_CI && tlut) {
        phash = 2166136261u;
        for (int i = 0; i < tlut_n * 2; i++) phash = (phash ^ ((const u8*)tlut)[i]) * 16777619u;
    }
    for (int i = 0; i < slot_hi; i++) {
        Slot* s = &slots[i];
        if (s->live && s->img == img && s->tlut == tlut && s->phash == phash && s->fmt == fmt && s->siz == siz &&
            s->w == width && s->h == height && s->stride == stride && s->dkey == dkey) {
            s->stamp = ++stamp;
            TOUCH(s);
            cur = i;
            have = 1;
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
    if (lut) {
        if (!expand_dual((const u8*)img, width, height, stride, lut, dx4, dy4, pw, ph)) return 0;
    } else if (!expand((const u8*)img, fmt, siz, width, height, stride, (const u8*)tlut, tlut_n, pw, ph, gf)) return 0;
    unsigned bpp = (gf == GPU_RGBA8) ? 4u : (gf == GPU_L8) ? 1u : 2u;
    unsigned bytes = (unsigned)pw * (unsigned)ph * bpp;
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
                pm_log("tex cache full %dx%d, draws using it are skipped", width, height);
            }
            return 0;
        }
        slot = -1;
    }
    Slot* s = &slots[slot];
    if (!C3D_TexInit(&s->tex, (u16)pw, (u16)ph, gf)) {
        pm_log("tex init failed %dx%d\n", pw, ph);
        fail_last = 1;
        fail_n++;
        return 0;
    }
    C3D_TexSetFilter(&s->tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&s->tex, GPU_REPEAT, GPU_REPEAT);
    const void* upload = scratch;
    /* Scratch is two 256KB halves. The linear image fills the first, the
     * Morton copy the second. A 512x256 RGBA5551 background is 256KB, so the
     * old 128KB tile buffer skipped tiling and the GPU scattered the pixels. */
    if (bpp == 2u && (unsigned)pw * (unsigned)ph * 2u <= 256u * 256u * 4u) {
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
    C3D_TexLoadImage(&s->tex, upload, GPU_TEXFACE_2D, 0);
    s->img = img;
    s->tlut = tlut;
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
    frame_bytes += bytes;
    if (frame_bytes > map_wmax) map_wmax = frame_bytes;
    s->live = 1;
    if (slot >= slot_hi) slot_hi = slot + 1;
    used += bytes;
    if (used > used_peak) used_peak = used;
    if (used > map_peak) map_peak = used;
    cur = slot;
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
void pm_tex_set_filter(int linear) { tex_filt = linear ? GPU_LINEAR : GPU_NEAREST; }

int pm_tex_bind(void) {
    if (have && off_cur >= 0) {
        C3D_TexBind(0, offt[off_cur].tex);
        return 1;
    }
    if (!have || cur < 0 || !slots[cur].live) return 0;
    slots[cur].stamp = ++stamp;
    C3D_TexSetFilter(&slots[cur].tex, tex_filt, tex_filt);
    C3D_TexSetWrap(&slots[cur].tex, shr_s ? (mk_ms ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : wrap_s, shr_t ? (mk_mt ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : wrap_t);
    C3D_TexBind(0, &slots[cur].tex);
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
    C3D_TexSetFilter(&slots[aux].tex, tex_filt, tex_filt);
    C3D_TexSetWrap(&slots[aux].tex, aux_shr_s ? (mk_ms ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : aux_ws, aux_shr_t ? (mk_mt ? GPU_MIRRORED_REPEAT : GPU_REPEAT) : aux_wt);
    C3D_TexBind(1, &slots[aux].tex);
    return 1;
}

void pm_tex_unbind_aux(void) { C3D_TexBind(1, NULL); }

void pm_tex_aux_size(int* w, int* h) {
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
