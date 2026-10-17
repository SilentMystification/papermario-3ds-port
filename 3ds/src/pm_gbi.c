#include <stdio.h>
#include <string.h>
#include "ultra64.h"
#include "pm_gbi.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"

extern unsigned int __ctru_heap;
extern unsigned int __ctru_heap_size;
extern unsigned int __ctru_linear_heap;
extern unsigned int __ctru_linear_heap_size;

static float proj[4][4];
static float mv[10][4][4];
static float combined[4][4];
static int mv_sp;
/* Viewport scale/translate after the 2-bit fraction. Full 320x240 until a display list sets one. */
static float vp_sx = 160.f, vp_sy = 120.f, vp_sz = 127.75f;
static float vp_tx = 160.f, vp_ty = 120.f, vp_tz = 127.75f;
static unsigned geom;
static unsigned omode_h;
static unsigned omode_l = 0x00000030u; /* Z_CMP | Z_UPD until a list sets a mode */
static int rm_frame;

static unsigned prim = 0xffffffffu;
static unsigned fill = 0xffffffffu;
static int tex_on;
static const void* timg;
static unsigned timg_fmt, timg_siz;
static int timg_stride;
static int ld_valid, ld_x, ld_y; /* last LoadTile window, in 8-bit texels of the source image */
static int tile_x0, tile_y0;
static const void* tlut;
static int tlut_n;
/* RDP state the shaded-sprite path needs: color image, TLUT per palette slot, per-tile TMEM
 * address and palette, tile sizes, scissor, env color and the second combiner cycle. */
extern int pm_is_cfb(const void* p);
static void* cimg;
static int cimg_stride = 320;
static const void* tlut_pal[16];
static unsigned tile_tmem[8], tile_pal[8];
static int tile_sz[8][4];
static int sci_x0, sci_y0, sci_x1 = 320, sci_y1 = 240;
static unsigned envc = 0xffffffffu;
static unsigned comb2_a, comb2_b, comb2_c, comb2_d, comb2_aa, comb2_ab, comb2_ac, comb2_ad;
static unsigned comb_aa, comb_ab, comb_ac, comb_ad;
/* The sprite raster captured when tile 1 got its size. A shaded sprite sets two tiles on the
 * same 4-bit image (tile 0 reads the shading palette, tile 1 the sprite palette). */
static struct { int valid; const u8* src; int stride, w, h, x0, y0; } m1;
static int dual_on;
/* A texture tile as the RDP holds it. Tile 0 is the render tile. A combiner that reads only TEXEL1
 * samples tile 1 (draw_box takes its window body from tile 1). One texture is bound at a time,
 * so the tile the next draw needs is bound just before it. */
static struct {
    int valid;
    const u8* src;
    unsigned fmt, siz;
    int w, h, stride, x0, y0, pal_n;
    const void* pal;
} td[2];
static unsigned tile_wrap[8][4];
static int cur_tile;
static int use_t1;     /* the combiner reads TEXEL1 only: draw with tile 1 (TEXEL1 is renamed TEXEL0) */
static int comb_mode5; /* (PRIM - ENV) * TEXEL + ENV */
static float dual_gain[3];
static void unpack8(unsigned c, float o[4]);
static void ensure_tile(int n);
static unsigned tile_fmt[8], tile_siz[8];
static u32 segbase[16];
static Vtx verts[64];
/* A display list may only draw vertices its own G_VTX loaded. Slots left by
 * the previous model otherwise connect the two meshes into long triangles. */
static u8 vert_live[64];
/* The RSP transforms a vertex when it is loaded. A later matrix does not move it. */
static float vert_m[64][4][4];
static u8 seen[256];
/* i = every corner in front of the eye, drawn where it projected.
 * c = a corner behind the eye was cut. d = nothing left in front.
 * x is the first vertex's N64 ndc * 100. */
static int n_cmd, n_in, n_cut, n_drop, n_out, n_keep, sample_x, have_sample;
static char stats_line[64];
static unsigned comb_a, comb_b, comb_c, comb_d;
static int comb_set;
/* 1 when the alpha combiner reads shade alpha. DECAL and MODULATE modes do not, and imgfx
 * meshes leave cn[3] at 0, so passing it through made them vanish. */
static int comb_alpha_shade = 1;

static int is_host(u32 p) {
    unsigned int heap = __ctru_heap;
    unsigned int hend = __ctru_heap + __ctru_heap_size;
    unsigned int lin = __ctru_linear_heap;
    unsigned int lend = lin + __ctru_linear_heap_size;
    if (p >= heap && p < hend) return 1;
    if (p >= lin && p < lend) return 1;
    if (p >= 0x00100000u && p < 0x01400000u) return 1;
    return 0;
}

static void* ptr_of(u32 w) {
    if (!w) return NULL;
    if (is_host(w)) return (void*)w;
    unsigned seg = (w >> 24) & 0xff;
    if (seg < 16 && segbase[seg]) return (void*)(segbase[seg] + (w & 0x00ffffffu));
    return (void*)w;
}

static void ident(float m[4][4]) {
    memset(m, 0, sizeof(float) * 16);
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
}

static void mul(float c[4][4], const float a[4][4], const float b[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            c[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
        }
    }
}

static float elem(const Mtx* src, int row, int col) {
    const u32* m = (const u32*)src->m;
    int idx = row * 2 + (col >> 1);
    u32 sh = (col & 1) ? 0 : 16;
    s16 ip = (s16)((m[idx] >> sh) & 0xffff);
    s16 fp = (s16)((m[idx + 8] >> sh) & 0xffff);
    s32 fixed = ((s32)ip << 16) | (u16)fp;
    return (float)fixed / 65536.f;
}

static void unpack(const Mtx* src, float d[4][4]) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) d[r][c] = elem(src, r, c);
}

static void upload_mvp(void) {
    /* Row-vector matrices, same as transform_point: modelview first, then projection. */
    mul(combined, mv[mv_sp], proj);
    pm_gpu_set_mvp(combined, vp_sx, vp_sy, vp_tx, vp_ty);
}

/* Combiner input as a constant, or -1 when it needs a texel or the shade, -2 for COMBINED.
 * slot 0..3 is A, B, C, D of the colour equation. */
static float cc_const_in(int slot, unsigned sel, int ch, const float* pr, const float* en) {
    if (sel == 0) return -2.f;
    if (sel == 3) return pr[ch];
    if (sel == 5) return en[ch];
    if (slot == 0 && sel == 6) return 1.f;
    if (slot == 3 && sel == 6) return 1.f;
    if (slot == 3 && sel == 7) return 0.f;
    if (slot == 1 && sel >= 7) return 0.f;
    if (slot == 0 && sel >= 8) return 0.f;
    if (slot == 2 && sel == 10) return pr[3];
    if (slot == 2 && sel == 12) return en[3];
    if (slot == 2 && sel >= 16) return 0.f;
    return -1.f;
}

/* Alpha of one cycle as K * TEXEL0 alpha. Returns K, or -1 if the form is not that.
 * (T1 - T0) * E + T0 reads two textures. Tile 1 is not loaded, so TEXEL0 stands for both. */
static float cc_alpha_k(unsigned aa, unsigned ab, unsigned ac, unsigned ad, const float* pr, const float* en) {
    if (ab == 7 && ad == 7 && aa == 1 && (ac == 3 || ac == 5)) return ac == 3 ? pr[3] : en[3];
    if (ab == 7 && ad == 7 && ac == 1 && (aa == 3 || aa == 5)) return aa == 3 ? pr[3] : en[3];
    if (aa == 2 && ab == 1 && ad == 1 && (ac == 3 || ac == 5)) return 1.f;
    return -1.f;
}

/* A combiner whose colour needs no texel and no shade, and whose alpha is a constant times
 * TEXEL0 alpha: a flat colour through a texture's alpha mask (a drop shadow is black this way).
 * Returns 1 and the RGBA constant, the alpha byte being the factor on texture alpha. */
static int const_rgb_tex_alpha(unsigned* out) {
    float pr[4], en[4], rgb[3], k, k2;
    int ch;
    int two = ((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 1;
    if (((omode_h >> G_MDSFT_CYCLETYPE) & 3) > 1 || !comb_set || !tex_on) return 0;
    unpack8(prim, pr);
    unpack8(envc, en);
    for (ch = 0; ch < 3; ch++) {
        float a, b, c, d;
        if (two) {
            a = cc_const_in(0, comb2_a, ch, pr, en); b = cc_const_in(1, comb2_b, ch, pr, en);
            c = cc_const_in(2, comb2_c, ch, pr, en); d = cc_const_in(3, comb2_d, ch, pr, en);
        } else {
            a = cc_const_in(0, comb_a, ch, pr, en); b = cc_const_in(1, comb_b, ch, pr, en);
            c = cc_const_in(2, comb_c, ch, pr, en); d = cc_const_in(3, comb_d, ch, pr, en);
            if (a == -2.f) a = 0.f; /* COMBINED is 0 in the only cycle */
            if (b == -2.f) b = 0.f;
            if (c == -2.f) c = 0.f;
            if (d == -2.f) d = 0.f;
        }
        if (a < 0.f || b < 0.f || c < 0.f || d < 0.f) return 0;
        rgb[ch] = (a - b) * c + d;
        rgb[ch] = rgb[ch] < 0.f ? 0.f : rgb[ch] > 1.f ? 1.f : rgb[ch];
    }
    k = cc_alpha_k(comb_aa, comb_ab, comb_ac, comb_ad, pr, en);
    if (k < 0.f) return 0;
    if (two) {
        /* Second cycle alpha: COMBINED * constant, or COMBINED passed through. */
        if (comb2_aa == 0 && comb2_ab == 7 && comb2_ad == 7 && (comb2_ac == 3 || comb2_ac == 5)) k2 = comb2_ac == 3 ? pr[3] : en[3];
        else if (comb2_aa == 7 && comb2_ab == 7 && comb2_ac == 7 && comb2_ad == 0) k2 = 1.f;
        else return 0;
        k *= k2;
    }
    *out = ((unsigned)(rgb[0] * 255.f + .5f) << 24) | ((unsigned)(rgb[1] * 255.f + .5f) << 16) |
           ((unsigned)(rgb[2] * 255.f + .5f) << 8) | (unsigned)(k * 255.f + .5f);
    return 1;
}

static void sync_combine(void) {
    unsigned cc;
    if (const_rgb_tex_alpha(&cc)) {
        pm_gpu_set_const(cc);
        pm_gpu_set_combine(4);
        return;
    }
    /* Decal modes put TEXEL0 in the addend, which is mux d, not a/b/c.
     * Missing it draws the sprite quad's vertex colors: a grey gradient. */
    int wants_tex = tex_on && (comb_a == G_CCMUX_TEXEL0 || comb_b == G_CCMUX_TEXEL0 ||
        comb_c == G_CCMUX_TEXEL0 || comb_d == G_CCMUX_TEXEL0 || comb_c == G_CCMUX_TEXEL0_ALPHA);
    int wants_prim = comb_a == G_CCMUX_PRIMITIVE || comb_b == G_CCMUX_PRIMITIVE || comb_c == G_CCMUX_PRIMITIVE;
    int mode = 0;
    /* Mode 3 = texture colour, but alpha only from the vertex: the alpha combiner does not read TEXEL0. */
    int alpha_tex = comb_aa == 1 || comb_ab == 1 || comb_ac == 1 || comb_ad == 1 ||
                    comb_aa == 2 || comb_ab == 2 || comb_ac == 2 || comb_ad == 2;
    comb_mode5 = 0;
    if (wants_tex && comb_a == G_CCMUX_PRIMITIVE && comb_b == G_CCMUX_ENVIRONMENT && comb_c == G_CCMUX_TEXEL0 &&
        comb_d == G_CCMUX_ENVIRONMENT && alpha_tex) {
        comb_mode5 = 1;
        pm_gpu_set_env(envc);
        pm_gpu_set_combine(5);
        return;
    }
    if (wants_tex) mode = alpha_tex ? 2 : 3;
    else if (wants_prim && !comb_set) mode = 1;
    else if (wants_prim && comb_a != G_CCMUX_SHADE) mode = 1;
    pm_gpu_set_combine(mode);
}

/* Render mode bits of G_SETOTHERMODE_L, same reading as libultraship DeriveTriState: depth
 * compare and update come from Z_CMP and Z_UPD, a decal passes equal depth, blending is on when
 * the blender mixes with the frame buffer by (1 - alpha), texture edge and the alpha threshold
 * cut away near-transparent texels. */
static void sync_render(void) {
    unsigned m = omode_l;
    if (pm_debug_has("rmoff")) { /* A/B check: the old always-blend, always-write state */
        pm_gpu_set_render(1, 1, 1, -1, 0);
        return;
    }
    int blend = ((m & (3u << 20)) == (1u << 20) && (m & (3u << 16)) == 0) ||
                ((m & (3u << 22)) == (1u << 22) && (m & (3u << 18)) == 0);
    int atest = -1;
    if (m & 0x1000u) { /* CVG_X_ALPHA, texture edge */
        if (blend) atest = 0x08;
        else atest = 0x30;
    } else if ((m & 3u) == 1u) { /* G_AC_THRESHOLD */
        atest = 0x08;
    }
    pm_gpu_set_render((m & 0x10u) != 0, (m & 0x20u) != 0, blend, atest, (m & 0xc00u) == 0xc00u);
}

static unsigned rgba_from_5551(unsigned c) {
    unsigned r = (c >> 11) & 31;
    unsigned g = (c >> 6) & 31;
    unsigned b = (c >> 1) & 31;
    unsigned a = (c & 1) ? 255 : 0;
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    return (r << 24) | (g << 16) | (b << 8) | a;
}

static unsigned vert_color(const Vtx* v) {
    if (geom & G_LIGHTING) {
        /* Normals share the color bytes. One light from above-front until the
         * display list's lights are applied. */
        float nx = (float)(signed char)v->v.cn[0] / 127.f;
        float ny = (float)(signed char)v->v.cn[1] / 127.f;
        float nz = (float)(signed char)v->v.cn[2] / 127.f;
        float d = nx * 0.25f + ny * 0.85f + nz * 0.45f;
        if (d < 0.f) d = 0.f;
        if (d > 1.f) d = 1.f;
        unsigned c = (unsigned)((0.35f + 0.65f * d) * 255.f);
        return (c << 24) | (c << 16) | (c << 8) | 255u;
    }
    return ((unsigned)v->v.cn[0] << 24) | ((unsigned)v->v.cn[1] << 16) |
           ((unsigned)v->v.cn[2] << 8) | (unsigned)v->v.cn[3];
}

static void vert_uv(const Vtx* v, float* u, float* vcoord) {
    int tw = 1, th = 1;
    pm_tex_size(&tw, &th);
    if (tw < 1) tw = 1;
    if (th < 1) th = 1;
    /* SetTileSize's origin is the start of the uploaded window. Absolute
     * vertex UVs have to be moved into that window, same as a texrect. */
    *u = ((float)v->v.tc[0] / 32.f - (float)tile_x0) / (float)tw;
    *vcoord = ((float)v->v.tc[1] / 32.f - (float)tile_y0) / (float)th;
}

typedef struct {
    float x, y, z, w, u, v, r, g, b, a;
} ClipV;

static unsigned pack_color(const ClipV* v) {
    int r = (int)v->r, g = (int)v->g, b = (int)v->b, a = (int)v->a;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (a < 0) a = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (a > 255) a = 255;
    return ((unsigned)r << 24) | ((unsigned)g << 16) | ((unsigned)b << 8) | (unsigned)a;
}

static const float ident_m[4][4] = {
    { 1.f, 0.f, 0.f, 0.f },
    { 0.f, 1.f, 0.f, 0.f },
    { 0.f, 0.f, 1.f, 0.f },
    { 0.f, 0.f, 0.f, 1.f },
};

/* The RSP multiplies when the vertex is loaded. Later matrices do not move it. */
static void xform_vtx(const float m[4][4], const Vtx* v, ClipV* o) {
    float x = (float)v->v.ob[0];
    float y = (float)v->v.ob[1];
    float z = (float)v->v.ob[2];
    unsigned c = vert_color(v);
    o->x = m[0][0] * x + m[1][0] * y + m[2][0] * z + m[3][0];
    o->y = m[0][1] * x + m[1][1] * y + m[2][1] * z + m[3][1];
    o->z = m[0][2] * x + m[1][2] * y + m[2][2] * z + m[3][2];
    o->w = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];
    vert_uv(v, &o->u, &o->v);
    o->r = (float)((c >> 24) & 255u);
    o->g = (float)((c >> 16) & 255u);
    o->b = (float)((c >> 8) & 255u);
    o->a = (float)(c & 255u);
}

static void lerp_clip(ClipV* o, const ClipV* a, const ClipV* b, float t) {
    o->x = a->x + (b->x - a->x) * t;
    o->y = a->y + (b->y - a->y) * t;
    o->z = a->z + (b->z - a->z) * t;
    o->w = a->w + (b->w - a->w) * t;
    o->u = a->u + (b->u - a->u) * t;
    o->v = a->v + (b->v - a->v) * t;
    o->r = a->r + (b->r - a->r) * t;
    o->g = a->g + (b->g - a->g) * t;
    o->b = a->b + (b->b - a->b) * t;
    o->a = a->a + (b->a - a->a) * t;
}

/* Cut only corners that passed behind the eye. A corner that is merely off the
 * side of the screen keeps its projected position, so a pan cannot swap the
 * triangle for a different polygon. */
static int clip_near(ClipV* poly, int n) {
    ClipV tmp[12];
    int m = 0;
    int i;
    if (n < 3) return 0;
    for (i = 0; i < n; i++) {
        ClipV* cur = &poly[i];
        ClipV* prev = &poly[(i + n - 1) % n];
        float dc = cur->w - 0.05f;
        float dp = prev->w - 0.05f;
        int ic = dc >= 0.f;
        int ip = dp >= 0.f;
        if (ic != ip && m < 12) {
            float denom = dp - dc;
            float t = (denom != 0.f) ? dp / denom : -1.f;
            if (t >= 0.f && t <= 1.f) lerp_clip(&tmp[m++], prev, cur, t);
        }
        if (ic && m < 12) tmp[m++] = *cur;
    }
    for (i = 0; i < m; i++) poly[i] = tmp[i];
    return m;
}

/* Same bits as libultraship, except the near plane, which they leave commented out.
 * A triangle is dropped only when every corner is off the same side. */
static int clip_bits(const ClipV* p) {
    int b = 0;
    float w = p->w;
    if (p->x < -w) b |= 1;
    if (p->x > w) b |= 2;
    if (p->y < -w) b |= 4;
    if (p->y > w) b |= 8;
    if (p->z > w) b |= 32;
    return b;
}

/* Clip positions, not object positions. The identity matrix leaves x,y,z,w
 * alone and the viewport places them. */
static void draw_tri(unsigned w) {
    int i0 = (int)((w >> 16) & 0xff) / 2;
    int i1 = (int)((w >> 8) & 0xff) / 2;
    int i2 = (int)(w & 0xff) / 2;
    int idx[3];
    ClipV poly[12];
    int n, i;
    n_cmd++;
    if (i0 < 0 || i1 < 0 || i2 < 0 || i0 > 63 || i1 > 63 || i2 > 63) {
        n_drop++;
        return;
    }
    if (!vert_live[i0] || !vert_live[i1] || !vert_live[i2]) {
        n_drop++;
        return;
    }
    idx[0] = i0;
    idx[1] = i1;
    idx[2] = i2;
    ensure_tile(use_t1 ? 1 : 0);
    for (i = 0; i < 3; i++) {
        xform_vtx(vert_m[idx[i]], &verts[idx[i]], &poly[i]);
    }
    if (!have_sample && poly[0].w != 0.f) {
        sample_x = (int)(poly[0].x / poly[0].w * 100.f);
        have_sample = 1;
    }
    if (clip_bits(&poly[0]) & clip_bits(&poly[1]) & clip_bits(&poly[2])) {
        n_out++;
        return;
    }
    {
        int k;
        for (k = 0; k < 3; k++) {
            float pw = poly[k].w;
            if (pw > 0.05f && poly[k].x >= -pw && poly[k].x <= pw && poly[k].y >= -pw && poly[k].y <= pw)
                n_keep++;
        }
    }
    if (poly[0].w > 0.05f && poly[1].w > 0.05f && poly[2].w > 0.05f) {
        n_in++;
        n = 3;
    } else {
        n = clip_near(poly, 3);
        if (n < 3) {
            n_drop++;
            return;
        }
        n_cut++;
    }
    if (pm_debug_has("rmlog") && (rm_frame % 30) == 1) {
        static unsigned lo, lg, la, lb, lc, ld, lh, lp, lf;
        if (lo != omode_l || lg != (geom & G_ZBUFFER) || la != comb_a || lb != comb_b || lc != comb_c || ld != comb_d ||
            lh != omode_h || lp != prim || lf != tile_fmt[0] * 4 + tile_siz[0]) {
            lp = prim; lf = tile_fmt[0] * 4 + tile_siz[0];
            lo = omode_l; lg = geom & G_ZBUFFER; la = comb_a; lb = comb_b; lc = comb_c; ld = comb_d; lh = omode_h;
            pm_log("rm f%d oml %08x omh %08x zb %d comb %x %x %x %x a %x %x %x %x tex %d fmt %u/%u prim %08x env %08x xy %d %d w %d", rm_frame, lo, lh, lg != 0, la, lb, lc, ld, comb_aa, comb_ab, comb_ac, comb_ad, tex_on, tile_fmt[0], tile_siz[0], prim, envc,
                   (int)(poly[0].x / poly[0].w * vp_sx + vp_tx), (int)(vp_ty - poly[0].y / poly[0].w * vp_sy), (int)poly[0].w);
        }
    }
    if (comb_mode5) {
        float en[4];
        unpack8(envc, en);
        for (i = 0; i < n; i++) { poly[i].r = en[0] * 255.f; poly[i].g = en[1] * 255.f; poly[i].b = en[2] * 255.f; poly[i].a = en[3] * 255.f; }
    }
    sync_render();
    if (!comb_alpha_shade) {
        for (i = 0; i < n; i++) poly[i].a = 255;
    }
    {
        int boost = dual_on && comb2_a == G_CCMUX_SHADE && comb2_b == G_CCMUX_ENVIRONMENT &&
                    comb2_c == G_CCMUX_COMBINED && comb2_d == G_CCMUX_COMBINED;
        pm_gpu_set_rgb_scale(boost ? 2 : 1);
        if (!boost) goto no_boost;
    }
    {
        /* Cycle 2 is (SHADE - ENV) * COMBINED + COMBINED = COMBINED * (SHADE - ENV + 1), up to 2.
         * The texture stage multiplies by 2, so the vertex carries half of it. */
        float en[4];
        unpack8(envc, en);
        for (i = 0; i < n; i++) {
            float* ch[3] = { &poly[i].r, &poly[i].g, &poly[i].b };
            for (int k = 0; k < 3; k++) {
                float f = (*ch[k] / 255.f - en[k] + 1.f) / 2.f;
                *ch[k] = (f < 0.f ? 0.f : f > 1.f ? 1.f : f) * 255.f;
            }
        }
    }
no_boost:
    pm_gpu_set_mvp(ident_m, vp_sx, vp_sy, vp_tx, vp_ty);
    for (i = 1; i < n - 1; i++) {
        if (!(poly[0].w > 0.05f && poly[i].w > 0.05f && poly[i + 1].w > 0.05f)) continue;
        pm_gpu_vert_clip(poly[0].x, poly[0].y, poly[0].z, poly[0].w, poly[0].u, poly[0].v, pack_color(&poly[0]));
        pm_gpu_vert_clip(poly[i].x, poly[i].y, poly[i].z, poly[i].w, poly[i].u, poly[i].v, pack_color(&poly[i]));
        pm_gpu_vert_clip(poly[i + 1].x, poly[i + 1].y, poly[i + 1].z, poly[i + 1].w,
                         poly[i + 1].u, poly[i + 1].v, pack_color(&poly[i + 1]));
    }
}

static void load_mtx(u32 w0, u32 w1) {
    unsigned flags = (w0 & 0xff) ^ G_MTX_PUSH;
    Mtx* src = (Mtx*)ptr_of(w1);
    if (!src) return;
    float in[4][4];
    unpack(src, in);
    int is_proj = (flags & G_MTX_PROJECTION) != 0;
    if (!is_proj && (flags & G_MTX_PUSH) && mv_sp + 1 < 10) {
        memcpy(mv[mv_sp + 1], mv[mv_sp], sizeof(mv[0]));
        mv_sp++;
    }
    float (*dst)[4] = is_proj ? proj : mv[mv_sp];
    if (flags & G_MTX_LOAD) {
        memcpy(dst, in, sizeof(in));
    } else {
        /* Row vector v * incoming * current. The RSP and PaperBoat's
         * interpreter both premultiply; the other order leaves a child
         * under the wrong parent. */
        float cur[4][4];
        memcpy(cur, dst, sizeof(cur));
        mul(dst, in, cur);
    }
    upload_mvp();
}

static void note(unsigned op) {
    if (seen[op]) return;
    seen[op] = 1;
    pm_log("gbi skip %02x\n", op);
}

static unsigned n_ops; /* PT01: display list ops run this frame */

void pm_gbi_end_frame(void) {
    /* One second of presented frames, summed. Identical seconds are not written. */
    static int frames, acc_t, acc_i, acc_c, acc_d, acc_o, acc_k, acc_x;
    static char prev[64];
    snprintf(stats_line, sizeof stats_line, "t%d i%d c%d d%d o%d k%d x%d",
             n_cmd, n_in, n_cut, n_drop, n_out, n_keep, have_sample ? sample_x : 0);
    PH_ADD(C_CMD, n_ops);
    n_ops = 0;
    PH_ADD(C_TRI, n_cmd);
    PH_ADD(C_TRIOUT, n_in + n_cut);
    acc_t += n_cmd;
    acc_i += n_in;
    acc_c += n_cut;
    acc_d += n_drop;
    acc_o += n_out;
    acc_k += n_keep;
    if (have_sample) acc_x = sample_x;
    frames++;
    if (frames >= 30) {
        char line[80];
        snprintf(line, sizeof line, "t%d i%d c%d d%d o%d k%d x%d", acc_t, acc_i, acc_c, acc_d, acc_o, acc_k, acc_x);
        if (strcmp(line, prev) != 0) {
            if (pm_debug_has("stats")) pm_log("stats %s", line); /* L01: per second line, only with the stats switch */
            snprintf(prev, sizeof prev, "%s", line);
        }
        frames = acc_t = acc_i = acc_c = acc_d = acc_o = acc_k = 0;
    }
    n_cmd = n_in = n_cut = n_drop = n_out = n_keep = 0;
    have_sample = 0;
    rm_frame++;
}

const char* pm_gbi_stats(void) { return stats_line; }

void pm_gbi_init(void) {
    ident(proj);
    ident(mv[0]);
    mv_sp = 0;
    memset(vert_live, 0, sizeof vert_live);
    geom = G_SHADE | G_SHADING_SMOOTH;
    upload_mvp();
}


/* ---- RDP color combiner, one cycle, for work the display list does off screen ---- */
static void unpack8(unsigned c, float o[4]) {
    o[0] = (float)((c >> 24) & 255u) / 255.f;
    o[1] = (float)((c >> 16) & 255u) / 255.f;
    o[2] = (float)((c >> 8) & 255u) / 255.f;
    o[3] = (float)(c & 255u) / 255.f;
}

static float cc_rgb_in(unsigned sel, int ch, const float* t0, const float* t1, const float* pr,
                       const float* en, int a_ok) {
    switch (sel) {
    case 1: return t0[ch];
    case 2: return t1[ch];
    case 3: return pr[ch];
    case 4: return 1.f; /* SHADE: none off screen */
    case 5: return en[ch];
    case 6: return a_ok ? 1.f : 0.f;
    default: return 0.f;
    }
}

/* (A - B) * C + D, cycle 1 only. COMBINED reads as 0 there. */
static void cc_cycle1(const float* t0, const float* t1, float out[4]) {
    float pr[4], en[4];
    unpack8(prim, pr);
    unpack8(envc, en);
    for (int ch = 0; ch < 3; ch++) {
        float a = cc_rgb_in(comb_a, ch, t0, t1, pr, en, 1);
        float b = cc_rgb_in(comb_b, ch, t0, t1, pr, en, 0);
        float c;
        switch (comb_c) {
        case 8: c = t0[3]; break;
        case 9: c = t1[3]; break;
        case 10: c = pr[3]; break;
        case 11: c = 1.f; break;
        case 12: c = en[3]; break;
        default: c = cc_rgb_in(comb_c, ch, t0, t1, pr, en, 0); break;
        }
        float d = cc_rgb_in(comb_d, ch, t0, t1, pr, en, 1);
        float v = (a - b) * c + d;
        out[ch] = v < 0.f ? 0.f : v > 1.f ? 1.f : v;
    }
    {
        unsigned sel[4] = { comb_aa, comb_ab, comb_ac, comb_ad };
        float v[4];
        for (int i = 0; i < 4; i++) {
            switch (sel[i]) {
            case 1: v[i] = t0[3]; break;
            case 2: v[i] = t1[3]; break;
            case 3: v[i] = pr[3]; break;
            case 4: v[i] = 1.f; break;
            case 5: v[i] = en[3]; break;
            case 6: v[i] = (i == 2) ? 0.f : 1.f; break;
            default: v[i] = 0.f; break;
            }
        }
        float r = (v[0] - v[1]) * v[2] + v[3];
        out[3] = r < 0.f ? 0.f : r > 1.f ? 1.f : r;
    }
}

static void pal_rgba(const void* base, unsigned i, float o[4]) {
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

/* A texture rectangle into an off-screen color image, sampled from a TLUT held in TMEM. The
 * shaded-sprite code draws its 16-entry shadow/highlight palette this way. Returns 1 if done. */
static int rect_to_image(int xl, int yl, int xh, int yh, unsigned tile, u32 hs, u32 hd) {
    if (!cimg || pm_is_cfb(cimg) || tile >= 8 || tile_tmem[tile] < 256) return 0;
    int x0 = xl >> 2, y0 = yl >> 2, x1 = xh >> 2, y1 = yh >> 2;
    if (x0 < sci_x0) x0 = sci_x0;
    if (y0 < sci_y0) y0 = sci_y0;
    if (x1 > sci_x1) x1 = sci_x1;
    if (y1 > sci_y1) y1 = sci_y1;
    float s0 = (float)(s16)(hs >> 16) / 32.f;
    float t0 = (float)(s16)(hs & 0xffff) / 32.f;
    float dsdx = (float)(s16)(hd >> 16) / 1024.f;
    float dtdy = (float)(s16)(hd & 0xffff) / 1024.f;
    float sl = (float)tile_sz[tile][0] / 4.f, tl = (float)tile_sz[tile][1] / 4.f;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            float s = s0 + dsdx * (float)(x - (xl >> 2)) - sl;
            float t = t0 + dtdy * (float)(y - (yl >> 2)) - tl;
            int ts = (int)s;
            (void)t;
            if (ts < 0) ts = 0;
            /* A TLUT in TMEM holds each entry four times in a row. One row only. */
            unsigned word = tile_tmem[tile] + (unsigned)(ts >> 2);
            unsigned entry = word - 256u;
            if (entry > 255u) continue;
            float tx[4], none[4] = { 0, 0, 0, 0 }, o[4];
            pal_rgba(tlut_pal[entry >> 4], entry & 15u, tx);
            cc_cycle1(tx, none, o);
            unsigned r = (unsigned)(o[0] * 31.f + .5f), g = (unsigned)(o[1] * 31.f + .5f),
                     b = (unsigned)(o[2] * 31.f + .5f), a = o[3] >= .5f;
            unsigned c = (r << 11) | (g << 6) | (b << 1) | a;
            u8* d = (u8*)cimg + ((size_t)y * (size_t)cimg_stride + (size_t)x) * 2u;
            d[0] = (u8)(c >> 8);
            d[1] = (u8)c;
        }
    }
    pm_tex_invalidate(cimg, 64);
    return 1;
}

/* Tile 0 reads the shading palette, tile 1 the sprite palette, both through the same 4-bit
 * indices, and the first combiner cycle multiplies them. Build the 16 x 16 result the
 * combiner would give, and let pm_tex apply it per texel. Returns 1 if the texture is bound. */
static int load_shaded_sprite(int dx4, int dy4) {
    const void* sp = tlut_pal[tile_pal[1] & 15];
    const void* sh = tlut_pal[tile_pal[0] & 15];
    unsigned lut[256];
    float env[4];
    if (!sp || !sh) return 0;
    unpack8(envc, env);
    /* Second cycle (SHADE - ENV) * COMBINED + COMBINED is linear in the vertex shade. It
     * can brighten past 1, so fold the largest factor into the texture and the rest into the
     * vertex color (see draw_tri). */
    (void)env;
    dual_gain[0] = dual_gain[1] = dual_gain[2] = 1.f;
    for (unsigned i = 0; i < 16; i++) {
        float t1[4];
        pal_rgba(sp, i, t1);
        for (unsigned j = 0; j < 16; j++) {
            float t0[4], o[4];
            pal_rgba(sh, j, t0);
            cc_cycle1(t0, t1, o);
            unsigned c[4];
            for (int k = 0; k < 4; k++) {
                float v = o[k] * (k < 3 ? dual_gain[k] : 1.f);
                if (v > 1.f) v = 1.f;
                c[k] = (unsigned)(v * 255.f + .5f);
            }
            lut[i * 16 + j] = (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3];
        }
    }
    return pm_tex_load_shaded(m1.src, m1.w, m1.h, m1.stride, sp, lut, dx4, dy4);
}

/* Bind the texture of tile n (0 or 1) for the next draw. */
static void ensure_tile(int n) {
    if (n == cur_tile || !td[n].valid) return;
    pm_gpu_flush();
    pm_tex_set_wrap(tile_wrap[n][0], tile_wrap[n][1], tile_wrap[n][2], tile_wrap[n][3]);
    pm_tex_load(td[n].src, td[n].fmt, td[n].siz, td[n].w, td[n].h, td[n].pal, td[n].pal_n, td[n].stride);
    tile_x0 = td[n].x0;
    tile_y0 = td[n].y0;
    cur_tile = n;
}

void pm_gbi_run(void* list, unsigned nbytes) {
    Gfx* stack[12];
    int sp = 0;
    Gfx* pc = (Gfx*)list;
    Gfx* end = (nbytes >= 8) ? pc + nbytes / 8 : NULL;
    unsigned guard = 0;
    if (!pc) return;
    while (pc && guard++ < 200000) {
        if (end && sp == 0 && pc >= end) break;
        if (!is_host((u32)pc)) {
            static int bad_dl;
            if (bad_dl++ < 6) pm_log("bad dl pc %08x list %08x n %u sp %d", (u32)pc, (u32)list, nbytes, sp);
            return;
        }
        u32 w0 = pc->words.w0;
        u32 w1 = pc->words.w1;
        unsigned op = w0 >> 24;
        int fp = 0;
        pc++;
        n_ops++;
        if (pm_ph_fine) {
            /* only these three: the thousands of state ops stay in the gbi phase (a tick read costs 1.3 us in Azahar) */
            fp = op == G_VTX ? PH_GVTX : (op == G_TRI1 || op == G_TRI2) ? PH_GTRI : op == G_MTX ? PH_GMTX : 0;
            if (fp) pm_ph_push(fp);
        }
        switch (op) {
        case G_ENDDL:
            if (sp == 0) {
                if (fp) pm_ph_pop();
                return;
            }
            pc = stack[--sp];
            break;
        case G_DL: {
            Gfx* next = (Gfx*)ptr_of(w1);
            if (next && !is_host((u32)next)) {
                static int bad_next;
                if (bad_next++ < 8) pm_log("bad dl target %08x w0 %08x list %08x", w1, w0, (u32)list);
                break;
            }
            if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) pc = next;
            else if (sp < 12) {
                stack[sp++] = pc;
                pc = next;
            }
            break;
        }
        case G_VTX: {
            unsigned n = (w0 >> 12) & 0xff;
            unsigned v0n = (w0 >> 1) & 0x7f;
            unsigned v0 = (v0n >= n) ? v0n - n : 0;
            Vtx* src = (Vtx*)ptr_of(w1);
            if (src && !is_host((u32)src)) {
                static int bad_vtx;
                if (bad_vtx++ < 8) pm_log("bad vtx %08x w0 %08x list %08x", w1, w0, (u32)list);
                src = NULL;
            }
            if (src && n <= 64) {
                if (v0 >= 64) n = 0; /* S30: a vertex load that starts past the buffer (garbage list) copied 4 GB */
                else if (v0 + n > 64) n = 64 - v0;
                PH_ADD(C_VTXLOAD, n);
                memcpy(&verts[v0], src, n * sizeof(Vtx));
                /* A load at slot 0 starts a new mesh. Later loads in the same
                 * list append above it and keep the earlier slots. */
                if (v0 == 0) memset(vert_live, 0, sizeof vert_live);
                for (unsigned i = 0; i < n; i++) {
                    vert_live[v0 + i] = 1;
                    memcpy(vert_m[v0 + i], combined, sizeof combined);
                }
            }
            break;
        }
        case G_TRI1:
            draw_tri(w0);
            break;
        case G_TRI2:
            draw_tri(w0);
            draw_tri(w1);
            break;
        case G_MTX:
            load_mtx(w0, w1);
            break;
        case G_SETSCISSOR: {
            int ulx = (int)((w0 >> 12) & 0xfff);
            int uly = (int)(w0 & 0xfff);
            int lrx = (int)((w1 >> 12) & 0xfff);
            int lry = (int)(w1 & 0xfff);
            sci_x0 = ulx >> 2;
            sci_y0 = uly >> 2;
            sci_x1 = lrx >> 2;
            sci_y1 = lry >> 2;
            pm_gpu_set_scissor(ulx >> 2, uly >> 2, lrx >> 2, lry >> 2);
            break;
        }
        case G_MOVEMEM: {
            if ((w0 & 0xff) == G_MV_VIEWPORT) {
                Vp* vp = (Vp*)ptr_of(w1);
                if (vp) {
                    vp_sx = (float)vp->vp.vscale[0] / 4.f;
                    vp_sy = (float)vp->vp.vscale[1] / 4.f;
                    vp_sz = (float)vp->vp.vscale[2] / 4.f;
                    vp_tx = (float)vp->vp.vtrans[0] / 4.f;
                    vp_ty = (float)vp->vp.vtrans[1] / 4.f;
                    vp_tz = (float)vp->vp.vtrans[2] / 4.f;
                    upload_mvp();
                }
            }
            break;
        }
        case G_POPMTX: {
            unsigned count = w1 / 64;
            if (count == 0) count = 1;
            while (count-- && mv_sp > 0) mv_sp--;
            upload_mvp();
            break;
        }
        case G_GEOMETRYMODE:
            geom = (geom & (w0 & 0xffffffu)) | w1;
            pm_gpu_set_mode((geom & G_ZBUFFER) != 0, (geom & G_LIGHTING) != 0);
            break;
        case G_TEXTURE:
            tex_on = (w0 & 0xff) != 0;
            sync_combine();
            break;
        case G_SETOTHERMODE_H:
        case G_SETOTHERMODE_L: {
            unsigned len = (w0 & 0xff) + 1;
            unsigned field = (w0 >> 8) & 0xff;
            unsigned sft = 32 - field - len;
            unsigned mask = (len >= 32) ? 0xffffffffu : (((1u << len) - 1u) << sft);
            if (op == G_SETOTHERMODE_H) omode_h = (omode_h & ~mask) | (w1 & mask);
            else omode_l = (omode_l & ~mask) | (w1 & mask);
            break;
        }
        case G_SETCOMBINE:
            comb_a = (w0 >> 20) & 0xf;
            comb_c = (w0 >> 15) & 0x1f;
            comb_b = (w1 >> 28) & 0xf;
            comb_d = (w1 >> 15) & 7;
            comb_set = 1;
            comb_aa = (w0 >> 12) & 7;
            comb_ac = (w0 >> 9) & 7;
            comb_ab = (w1 >> 12) & 7;
            comb_ad = (w1 >> 9) & 7;
            comb2_a = (w0 >> 5) & 0xf;
            comb2_c = w0 & 0x1f;
            comb2_b = (w1 >> 24) & 0xf;
            comb2_d = (w1 >> 6) & 7;
            comb2_aa = (w1 >> 21) & 7;
            comb2_ac = (w1 >> 18) & 7;
            comb2_ab = (w1 >> 3) & 7;
            comb2_ad = w1 & 7;
            /* TEXEL1 with no TEXEL0 anywhere: the texture is tile 1. Rename it so one binding serves. */
            {
                int t0 = comb_a == 1 || comb_b == 1 || comb_d == 1 || comb_c == 1 || comb_c == 8 ||
                         comb_aa == 1 || comb_ab == 1 || comb_ac == 1 || comb_ad == 1;
                int t1 = comb_a == 2 || comb_b == 2 || comb_d == 2 || comb_c == 2 || comb_c == 9 ||
                         comb_aa == 2 || comb_ab == 2 || comb_ac == 2 || comb_ad == 2;
                int c2 = comb2_a == 1 || comb2_b == 1 || comb2_d == 1 || comb2_c == 1 || comb2_c == 8 ||
                         comb2_aa == 1 || comb2_ab == 1 || comb2_ac == 1 || comb2_ad == 1 ||
                         comb2_a == 2 || comb2_b == 2 || comb2_d == 2 || comb2_c == 2 || comb2_c == 9 ||
                         comb2_aa == 2 || comb2_ab == 2 || comb2_ac == 2 || comb2_ad == 2;
                use_t1 = 0;
                if (t1 && !t0 && !c2) {
                    use_t1 = 1;
                    if (comb_a == 2) comb_a = 1;
                    if (comb_b == 2) comb_b = 1;
                    if (comb_d == 2) comb_d = 1;
                    if (comb_c == 2) comb_c = 1;
                    if (comb_c == 9) comb_c = 8;
                    if (comb_aa == 2) comb_aa = 1;
                    if (comb_ab == 2) comb_ab = 1;
                    if (comb_ac == 2) comb_ac = 1;
                    if (comb_ad == 2) comb_ad = 1;
                }
            }
            comb_alpha_shade = ((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 ||
                               ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4;
            sync_combine();
            break;
        case G_SETPRIMCOLOR:
            prim = w1;
            pm_gpu_set_prim(prim);
            sync_combine();
            break;
        case G_SETENVCOLOR:
            envc = w1;
            pm_gpu_set_env(envc);
            sync_combine();
            break;
        case G_SETCIMG:
            cimg = ptr_of(w1);
            cimg_stride = (int)(w0 & 0xfff) + 1;
            break;
        case G_SETFILLCOLOR:
            fill = w1;
            break;
        case G_FILLRECT: {
            int lrx = (int)((w0 >> 14) & 0x3ff);
            int lry = (int)((w0 >> 2) & 0x3ff);
            int ulx = (int)((w1 >> 14) & 0x3ff);
            int uly = (int)((w1 >> 2) & 0x3ff);
            /* FILL cycle uses the fill register. 1-cycle fills (screen fades) use primitive color. */
            unsigned color = (((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 3)
                ? rgba_from_5551(fill & 0xffff) : prim;
            pm_gpu_fill_rect(ulx, uly, lrx, lry, color);
            break;
        }
        case G_TEXRECT:
        case G_TEXRECTFLIP: {
            if (!pc || (end && sp == 0 && pc + 1 >= end)) break;
            u32 hs = pc->words.w1;
            pc++;
            u32 hd = pc->words.w1;
            pc++;
            int xh = (int)((w0 >> 12) & 0xfff);
            int yh = (int)(w0 & 0xfff);
            int xl = (int)((w1 >> 12) & 0xfff);
            int yl = (int)(w1 & 0xfff);
            if (xh & 0x800) xh |= ~0xfff;
            if (yh & 0x800) yh |= ~0xfff;
            if (xl & 0x800) xl |= ~0xfff;
            if (yl & 0x800) yl |= ~0xfff;
            if (rect_to_image(xl, yl, xh, yh, (w1 >> 24) & 7, hs, hd)) break;
            ensure_tile(use_t1 ? 1 : 0);
            int tw = 1, th = 1;
            pm_tex_size(&tw, &th);
            float s0 = ((float)(s16)(hs >> 16) / 32.f - (float)tile_x0) / (float)tw;
            float t0 = ((float)(s16)(hs & 0xffff) / 32.f - (float)tile_y0) / (float)th;
            float dsdx = (float)(s16)(hd >> 16) / 1024.f;
            float dtdy = (float)(s16)(hd & 0xffff) / 1024.f;
            float width = (float)(xh - xl) / 4.f;
            float height = (float)(yh - yl) / 4.f;
            if (((omode_h >> G_MDSFT_CYCLETYPE) & 3) == 2) {
                /* COPY mode: the RDP writes 4 pixels per cycle, so dsdx counts 4 texels per pixel (4.0 = 1:1).
                 * The lower right corner is inclusive, one more pixel (as in libultraship). */
                dsdx /= 4.f;
                width += 1.f;
            }
            float s1 = s0 + dsdx * width / (float)tw;
            float t1 = t0 + dtdy * height / (float)th;
            pm_gpu_tex_rect(xl / 4, yl / 4, xh / 4, yh / 4, s0, t0, s1, t1);
            break;
        }
        case G_SETTIMG:
            timg = ptr_of(w1);
            timg_fmt = (w0 >> 21) & 7;
            timg_siz = (w0 >> 19) & 3;
            timg_stride = (int)(w0 & 0xfff) + 1;
            ld_valid = 0;
            break;
        case G_SETTILE: {
            unsigned tile = (w1 >> 24) & 7;
            tile_fmt[tile] = (w0 >> 21) & 7;
            tile_siz[tile] = (w0 >> 19) & 3;
            tile_tmem[tile] = w0 & 0x1ff;
            tile_pal[tile] = (w1 >> 20) & 15;
            tile_wrap[tile][0] = (w1 >> 8) & 3;
            tile_wrap[tile][1] = (w1 >> 18) & 3;
            tile_wrap[tile][2] = (w1 >> 4) & 15;
            tile_wrap[tile][3] = (w1 >> 14) & 15;
            /* The render tile's wrap belongs to the triangles already queued.
             * Draw them before this tile replaces it. */
            if (tile == 0) {
                pm_gpu_flush();
                pm_tex_set_wrap((w1 >> 8) & 3, (w1 >> 18) & 3, (w1 >> 4) & 15, (w1 >> 14) & 15);
            }
            break;
        }
        case G_LOADTILE:
            ld_valid = 1;
            ld_x = (int)((w0 >> 12) & 0xfff) >> 2;
            ld_y = (int)(w0 & 0xfff) >> 2;
            break;
        case G_LOADTLUT:
            tlut = timg;
            tlut_n = (int)((w1 >> 14) & 0x3ff) + 1;
            {
                /* The palette slot is the 16-entry block at the load tile's TMEM address. */
                unsigned slot = (tile_tmem[(w1 >> 24) & 7] - 256u) >> 4;
                if (slot < 16) tlut_pal[slot] = timg;
            }
            break;
        case G_SETTILESIZE: {
            unsigned tile = (w1 >> 24) & 7;
            int uls = (int)((w0 >> 12) & 0xfff);
            int ult = (int)(w0 & 0xfff);
            int lrs = (int)((w1 >> 12) & 0xfff);
            int lrt = (int)(w1 & 0xfff);
            tile_sz[tile][0] = uls;
            tile_sz[tile][1] = ult;
            tile_sz[tile][2] = lrs;
            tile_sz[tile][3] = lrt;
            if (tile == 1 && tile_fmt[1] == 2 && tile_siz[1] == 0 && ld_valid && timg) {
                /* Second tile on the same 4-bit image: remember the raster for the shaded sprite. */
                m1.valid = 1;
                m1.src = (const u8*)timg + (size_t)ld_y * (size_t)timg_stride + (size_t)ld_x;
                m1.stride = timg_stride * 2;
                m1.w = ((lrs - uls) >> 2) + 1;
                m1.h = ((lrt - ult) >> 2) + 1;
                m1.x0 = uls >> 2;
                m1.y0 = ult >> 2;
            }
            if (tile == 1) {
                int w = ((lrs - uls) >> 2) + 1, h = ((lrt - ult) >> 2) + 1;
                int stride = timg_stride > w ? timg_stride : w;
                int bpp = tile_siz[1] >= 3 ? 4 : tile_siz[1] == 2 ? 2 : tile_siz[1] == 0 ? 0 : 1;
                const u8* src = (const u8*)timg;
                td[1].valid = 0;
                if (src && bpp) src += (size_t)((uls >> 2) + (ult >> 2) * stride) * (size_t)bpp;
                else if (src && ld_valid) {
                    src += (size_t)ld_y * (size_t)timg_stride + (size_t)ld_x;
                    stride = timg_stride * 2;
                }
                if (src && w > 0 && h > 0) {
                    td[1].valid = 1;
                    td[1].src = src;
                    td[1].fmt = tile_fmt[1];
                    td[1].siz = tile_siz[1];
                    td[1].w = w;
                    td[1].h = h;
                    td[1].stride = stride;
                    td[1].x0 = uls >> 2;
                    td[1].y0 = ult >> 2;
                    td[1].pal = tlut_pal[tile_pal[1] & 15] ? tlut_pal[tile_pal[1] & 15] : tlut;
                    td[1].pal_n = tlut_n;
                }
            }
            if (tile != 0) break;
            td[0].valid = 0;
            cur_tile = 0;
            dual_on = 0;
            if (m1.valid && tile_fmt[0] == 2 && tile_siz[0] == 0 && tile_pal[0] != tile_pal[1] &&
                comb_a == G_CCMUX_TEXEL0 && comb_c == G_CCMUX_TEXEL1 &&
                m1.w > 0 && m1.h > 0) {
                /* Tile 0 starts 256 texels in (the vertices carry that) plus the light offset. */
                int dx4 = uls - (m1.x0 << 2);
                int dy4 = ult - (m1.y0 << 2);
                m1.valid = 0;
                pm_gpu_flush();
                tile_x0 = m1.x0;
                tile_y0 = m1.y0;
                if (load_shaded_sprite(dx4, dy4)) {
                    dual_on = 1;
                    break;
                }
            }
            m1.valid = 0;
            int x0 = uls >> 2;
            int y0 = ult >> 2;
            int w = ((lrs - uls) >> 2) + 1;
            int h = ((lrt - ult) >> 2) + 1;
            int stride = timg_stride > w ? timg_stride : w;
            int bpp = tile_siz[0] >= 3 ? 4 : tile_siz[0] == 2 ? 2 : tile_siz[0] == 0 ? 0 : 1;
            const u8* src = (const u8*)timg;
            if (src && bpp) src += (size_t)(y0 * stride + x0) * (size_t)bpp;
            else if (src && tile_siz[0] == 0 && ld_valid) {
                /* 4-bit strips (imgfx meshes) load a window of an 8-bit-wide image. The
                 * tile size carries only the scroll, so the window comes from LoadTile. */
                src += (size_t)ld_y * (size_t)timg_stride + (size_t)ld_x;
                stride = timg_stride * 2;
            }
            tile_x0 = x0;
            tile_y0 = y0;
            /* One batch can only sample one image. The last load of the frame
             * was being bound for every surface, so the house wore one fence. */
            pm_gpu_flush();
            if (src && w > 0 && h > 0) {
                const void* pal0 = tlut_pal[tile_pal[0] & 15] ? tlut_pal[tile_pal[0] & 15] : tlut;
                pm_tex_load(src, tile_fmt[0], tile_siz[0], w, h, pal0, tlut_n, stride);
                td[0].valid = 1;
                td[0].src = src;
                td[0].fmt = tile_fmt[0];
                td[0].siz = tile_siz[0];
                td[0].w = w;
                td[0].h = h;
                td[0].stride = stride;
                td[0].x0 = x0;
                td[0].y0 = y0;
                td[0].pal = pal0;
                td[0].pal_n = tlut_n;
            }
            break;
        }
        case G_MOVEWORD: {
            unsigned index = (w0 >> 16) & 0xff;
            unsigned offset = w0 & 0xffff;
            if (index == G_MW_SEGMENT && (offset / 4) < 16) {
                unsigned seg = offset / 4;
                segbase[seg] = (u32)ptr_of(w1);
                if (!segbase[seg]) segbase[seg] = w1;
            }
            break;
        }
        case G_NOOP:
        case G_SPNOOP:
        case G_RDPFULLSYNC:
        case G_RDPTILESYNC:
        case G_RDPPIPESYNC:
        case G_RDPLOADSYNC:
        case G_SETZIMG:
        case G_RDPHALF_1:
        case G_RDPHALF_2:
            break;
        default:
            note(op);
            break;
        }
        if (fp) pm_ph_pop();
    }
}

static void mtx_elem(Mtx* dst, int r, int c, float v) {
    s32 fixed = (s32)(v * 65536.f);
    u32* m = (u32*)dst->m;
    int idx = r * 2 + (c >> 1);
    u32 sh = (c & 1) ? 0u : 16u;
    u32 mask = 0xffffu << sh;
    m[idx] = (m[idx] & ~mask) | ((((u32)(fixed >> 16)) & 0xffffu) << sh);
    m[idx + 8] = (m[idx + 8] & ~mask) | (((u32)fixed & 0xffffu) << sh);
}

void pm_gbi_fallback(void) {
    static int ready;
    static Gfx dl[8];
    static Vtx fv[3];
    static Mtx fm;
    if (pm_debug_has("nofallback")) return;
    if (!ready) {
        memset(&fm, 0, sizeof(fm));
        mtx_elem(&fm, 0, 0, 1.f / 160.f);
        mtx_elem(&fm, 1, 1, 1.f / 120.f);
        mtx_elem(&fm, 2, 2, 1.f);
        mtx_elem(&fm, 3, 3, 1.f);
        short xy[3][2] = { { -50, -40 }, { 50, -40 }, { 0, 50 } };
        u8 rgb[3][3] = { { 255, 80, 80 }, { 80, 255, 80 }, { 80, 80, 255 } };
        for (int i = 0; i < 3; i++) {
            fv[i].v.ob[0] = xy[i][0];
            fv[i].v.ob[1] = xy[i][1];
            fv[i].v.ob[2] = 0;
            fv[i].v.cn[0] = rgb[i][0];
            fv[i].v.cn[1] = rgb[i][1];
            fv[i].v.cn[2] = rgb[i][2];
            fv[i].v.cn[3] = 255;
        }
        Gfx* g = dl;
        gDPSetCycleType(g++, G_CYC_FILL);
        gDPSetFillColor(g++, 0x22292229);
        gDPFillRectangle(g++, 0, 0, 319, 239);
        gDPSetCycleType(g++, G_CYC_1CYCLE);
        gSPMatrix(g++, &fm, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPVertex(g++, fv, 3, 0);
        gSP1Triangle(g++, 0, 1, 2, 0);
        gSPEndDisplayList(g++);
        ready = 1;
    }
    pm_gbi_run(dl, 0);
}
