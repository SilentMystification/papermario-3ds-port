#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "pm_tex.h"
#include "pm_port.h"

enum { FMT_RGBA, FMT_YUV, FMT_CI, FMT_IA, FMT_I };
enum { SIZ_4, SIZ_8, SIZ_16, SIZ_32 };

#define SLOTS 24
#define BUDGET (4u * 1024u * 1024u)
#define MAX_DIM 256

typedef struct {
    const void* img;
    unsigned fmt, siz;
    int w, h;
    unsigned bytes;
    unsigned stamp;
    int live;
    C3D_Tex tex;
} Slot;

static Slot slots[SLOTS];
static unsigned used;
static unsigned stamp;
static u8* scratch;
static int cur = -1;
static int have;

static int pot_at_least_8(int n) {
    int p = 8;
    while (p < n && p < MAX_DIM) p <<= 1;
    return p;
}

static unsigned rd16(const u8* p) { return ((unsigned)p[0] << 8) | p[1]; }

static GPU_TEXCOLOR gpu_fmt(unsigned fmt, unsigned siz) {
    if (fmt == FMT_RGBA && siz == SIZ_32) return GPU_RGBA8;
    if (fmt == FMT_IA && siz == SIZ_16) return GPU_LA8;
    if (fmt == FMT_IA && siz == SIZ_8) return GPU_LA8;
    if (fmt == FMT_IA && siz == SIZ_4) return GPU_LA8;
    if (fmt == FMT_I && (siz == SIZ_8 || siz == SIZ_4)) return GPU_L8;
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

static int expand(const u8* src, unsigned fmt, unsigned siz, int w, int h,
                  const u8* tlut, int tlut_n, int pw, int ph, GPU_TEXCOLOR gf) {
    unsigned out_bpp = (gf == GPU_RGBA8) ? 4 : (gf == GPU_L8 || gf == GPU_L4 || gf == GPU_LA4) ? 1 : 2;
    if ((unsigned)pw * (unsigned)ph * out_bpp > 256u * 256u * 4u) return 0;
    memset(scratch, 0, (size_t)pw * (size_t)ph * out_bpp);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (fmt == FMT_CI) {
                unsigned idx = 0;
                if (siz == SIZ_4) {
                    unsigned byte = src[(y * w + x) >> 1];
                    idx = (x & 1) ? (byte & 15) : (byte >> 4);
                } else {
                    idx = src[y * w + x];
                }
                unsigned pix = (tlut && (int)idx < tlut_n) ? rd16(tlut + idx * 2) : 0xffffu;
                write5551(scratch + ((y * pw + x) * 2), pix);
            } else if (fmt == FMT_RGBA && siz == SIZ_16) {
                write5551(scratch + ((y * pw + x) * 2), rd16(src + (y * w + x) * 2));
            } else if (fmt == FMT_RGBA && siz == SIZ_32) {
                const u8* s = src + (y * w + x) * 4;
                u8* d = scratch + (y * pw + x) * 4;
                d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
            } else if (fmt == FMT_IA && siz == SIZ_16) {
                unsigned c = rd16(src + (y * w + x) * 2);
                u8* d = scratch + (y * pw + x) * 2;
                d[0] = (u8)(c >> 8);
                d[1] = (u8)(c & 255);
            } else if (fmt == FMT_IA && siz == SIZ_8) {
                unsigned c = src[y * w + x];
                unsigned i = (c >> 4) * 17;
                unsigned a = (c & 15) * 17;
                u8* d = scratch + (y * pw + x) * 2;
                d[0] = (u8)i;
                d[1] = (u8)a;
            } else if (fmt == FMT_IA && siz == SIZ_4) {
                unsigned byte = src[(y * w + x) >> 1];
                unsigned nib = (x & 1) ? (byte & 15) : (byte >> 4);
                unsigned i = (nib >> 1) * 36;
                unsigned a = (nib & 1) ? 255 : 0;
                u8* d = scratch + (y * pw + x) * 2;
                d[0] = (u8)i;
                d[1] = (u8)a;
            } else if (fmt == FMT_I && siz == SIZ_8) {
                scratch[y * pw + x] = src[y * w + x];
            } else if (fmt == FMT_I && siz == SIZ_4) {
                unsigned byte = src[(y * w + x) >> 1];
                unsigned nib = (x & 1) ? (byte & 15) : (byte >> 4);
                scratch[y * pw + x] = (u8)(nib * 17);
            } else {
                write5551(scratch + ((y * pw + x) * 2), 0xffffu);
            }
        }
    }
    return 1;
}

static void drop(Slot* s) {
    if (!s->live) return;
    C3D_TexDelete(&s->tex);
    if (used >= s->bytes) used -= s->bytes;
    else used = 0;
    s->live = 0;
}

static void evict_until(unsigned need) {
    while (used + need > BUDGET) {
        int oldest = -1;
        for (int i = 0; i < SLOTS; i++) {
            if (!slots[i].live) continue;
            if (oldest < 0 || slots[i].stamp < slots[oldest].stamp) oldest = i;
        }
        if (oldest < 0) return;
        if (oldest == cur) cur = -1;
        drop(&slots[oldest]);
    }
}

void pm_tex_init(void) {
    scratch = (u8*)linearAlloc(256 * 256 * 4);
}

int pm_tex_load(const void* img, unsigned fmt, unsigned siz, int width, int height,
                const void* tlut, int tlut_n) {
    have = 0;
    cur = -1;
    if (!img || width < 1 || height < 1 || !scratch) return 0;
    if ((unsigned)img < 0x00100000u) {
        static int once;
        if (!once) {
            once = 1;
            pm_log("tex ptr %08x %dx%d fmt %u", (unsigned)img, width, height, fmt);
        }
        return 0;
    }
    if (width > MAX_DIM || height > MAX_DIM) {
        pm_log("tex %dx%d skipped\n", width, height);
        return 0;
    }
    for (int i = 0; i < SLOTS; i++) {
        Slot* s = &slots[i];
        if (s->live && s->img == img && s->fmt == fmt && s->siz == siz && s->w == width && s->h == height) {
            s->stamp = ++stamp;
            cur = i;
            have = 1;
            return 1;
        }
    }
    int pw = pot_at_least_8(width);
    int ph = pot_at_least_8(height);
    GPU_TEXCOLOR gf = gpu_fmt(fmt, siz);
    if (fmt == FMT_CI) gf = GPU_RGBA5551;
    if (!expand((const u8*)img, fmt, siz, width, height, (const u8*)tlut, tlut_n, pw, ph, gf)) return 0;
    unsigned bpp = (gf == GPU_RGBA8) ? 4u : (gf == GPU_L8) ? 1u : 2u;
    unsigned bytes = (unsigned)pw * (unsigned)ph * bpp;
    int slot = -1;
    for (int i = 0; i < SLOTS; i++) if (!slots[i].live) { slot = i; break; }
    if (slot < 0) {
        evict_until(BUDGET);
        for (int i = 0; i < SLOTS; i++) if (!slots[i].live) { slot = i; break; }
    }
    if (slot < 0) return 0;
    evict_until(bytes);
    Slot* s = &slots[slot];
    if (!C3D_TexInit(&s->tex, (u16)pw, (u16)ph, gf)) {
        pm_log("tex init failed %dx%d\n", pw, ph);
        return 0;
    }
    C3D_TexSetFilter(&s->tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&s->tex, GPU_REPEAT, GPU_REPEAT);
    const void* upload = scratch;
    if (bpp == 2u) {
        u8* tiled = scratch + (256 * 256 * 2);
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
    }
    C3D_TexLoadImage(&s->tex, upload, GPU_TEXFACE_2D, 0);
    s->img = img;
    s->fmt = fmt;
    s->siz = siz;
    s->w = width;
    s->h = height;
    s->bytes = bytes;
    s->stamp = ++stamp;
    s->live = 1;
    used += bytes;
    cur = slot;
    have = 1;
    return 1;
}

int pm_tex_bind(void) {
    if (!have || cur < 0 || !slots[cur].live) return 0;
    slots[cur].stamp = ++stamp;
    C3D_TexBind(0, &slots[cur].tex);
    return 1;
}

void pm_tex_size(int* w, int* h) {
    int pw = 1, ph = 1;
    if (have && cur >= 0) {
        pw = pot_at_least_8(slots[cur].w);
        ph = pot_at_least_8(slots[cur].h);
    }
    if (w) *w = pw;
    if (h) *h = ph;
}
