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
static unsigned prim = 0xffffffffu;
static unsigned fill = 0xffffffffu;
static int tex_on;
static const void* timg;
static unsigned timg_fmt, timg_siz;
static int timg_stride;
static int tile_x0, tile_y0;
static const void* tlut;
static int tlut_n;
static unsigned tile_fmt[8], tile_siz[8];
static u32 segbase[16];
static Vtx verts[64];
/* A display list may only draw vertices its own G_VTX loaded. Slots left by
 * the previous model otherwise connect the two meshes into long triangles. */
static u8 vert_live[64];
/* The RSP transforms a vertex when it is loaded. A later matrix does not move it. */
static float vert_m[64][4][4];
static u8 seen[256];
/* i = fully in front. c = cut where it crossed the camera. d = fully outside.
 * x is the first vertex's N64 ndc * 100. */
static int n_cmd, n_in, n_cut, n_drop, sample_x, have_sample;
static char stats_line[48];
static unsigned comb_a, comb_b, comb_c;
static int comb_set;

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

static void sync_combine(void) {
    int wants_tex = tex_on && (comb_a == G_CCMUX_TEXEL0 || comb_b == G_CCMUX_TEXEL0 || comb_c == G_CCMUX_TEXEL0);
    int wants_prim = comb_a == G_CCMUX_PRIMITIVE || comb_b == G_CCMUX_PRIMITIVE || comb_c == G_CCMUX_PRIMITIVE;
    int mode = 0;
    if (wants_tex) mode = 2;
    else if (wants_prim && !comb_set) mode = 1;
    else if (wants_prim && comb_a != G_CCMUX_SHADE) mode = 1;
    pm_gpu_set_combine(mode);
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

static float plane_dist(const ClipV* v, int plane) {
    switch (plane) {
    case 0: return v->w + v->x;
    case 1: return v->w - v->x;
    case 2: return v->w + v->y;
    case 3: return v->w - v->y;
    case 4: return v->w + v->z;
    default: return v->w - v->z;
    }
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

/* Azahar does not clip a corner that passes behind the camera, so that corner
 * is pulled through the eye and the face becomes a streak. Cut it here. */
static int clip_poly(ClipV* poly, int n) {
    ClipV tmp[12];
    int plane;
    for (plane = 0; plane < 6; plane++) {
        int m = 0;
        int i;
        if (n < 3) return 0;
        for (i = 0; i < n; i++) {
            ClipV* cur = &poly[i];
            ClipV* prev = &poly[(i + n - 1) % n];
            float dc = plane_dist(cur, plane);
            float dp = plane_dist(prev, plane);
            int ic = dc >= 0.f;
            int ip = dp >= 0.f;
            if (ic != ip && (dp - dc) != 0.f && m < 12)
                lerp_clip(&tmp[m++], prev, cur, dp / (dp - dc));
            if (ic && m < 12) tmp[m++] = *cur;
        }
        for (i = 0; i < m; i++) poly[i] = tmp[i];
        n = m;
    }
    return n;
}

static int in_front(const ClipV* v) {
    int plane;
    if (!(v->w > 0.05f)) return 0;
    for (plane = 0; plane < 6; plane++) {
        if (plane_dist(v, plane) < 0.f) return 0;
    }
    return 1;
}

/* Clip positions, not object positions. The identity matrix leaves x,y,z,w
 * alone and the viewport places them. A corner behind the camera is cut off
 * first so it cannot be divided through the eye. */
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
    for (i = 0; i < 3; i++) xform_vtx(vert_m[idx[i]], &verts[idx[i]], &poly[i]);
    if (!have_sample && poly[0].w != 0.f) {
        sample_x = (int)(poly[0].x / poly[0].w * 100.f);
        have_sample = 1;
    }
    if (in_front(&poly[0]) && in_front(&poly[1]) && in_front(&poly[2])) {
        n_in++;
        n = 3;
    } else {
        n = clip_poly(poly, 3);
        if (n < 3) {
            n_drop++;
            return;
        }
        n_cut++;
    }
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

void pm_gbi_end_frame(void) {
    /* One second of presented frames, summed. Identical seconds are not written. */
    static int frames, acc_t, acc_i, acc_c, acc_d, acc_x;
    static char prev[48];
    snprintf(stats_line, sizeof stats_line, "t%d i%d c%d d%d x%d", n_cmd, n_in, n_cut, n_drop, have_sample ? sample_x : 0);
    acc_t += n_cmd;
    acc_i += n_in;
    acc_c += n_cut;
    acc_d += n_drop;
    if (have_sample) acc_x = sample_x;
    frames++;
    if (frames >= 30) {
        char line[48];
        snprintf(line, sizeof line, "t%d i%d c%d d%d x%d", acc_t, acc_i, acc_c, acc_d, acc_x);
        if (strcmp(line, prev) != 0) {
            pm_log("stats %s", line);
            snprintf(prev, sizeof prev, "%s", line);
        }
        frames = acc_t = acc_i = acc_c = acc_d = 0;
    }
    n_cmd = n_in = n_cut = n_drop = 0;
    have_sample = 0;
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

void pm_gbi_run(void* list, unsigned nbytes) {
    Gfx* stack[12];
    int sp = 0;
    Gfx* pc = (Gfx*)list;
    Gfx* end = (nbytes >= 8) ? pc + nbytes / 8 : NULL;
    unsigned guard = 0;
    if (!pc) return;
    while (pc && guard++ < 200000) {
        if (end && sp == 0 && pc >= end) break;
        u32 w0 = pc->words.w0;
        u32 w1 = pc->words.w1;
        unsigned op = w0 >> 24;
        pc++;
        switch (op) {
        case G_ENDDL:
            if (sp == 0) return;
            pc = stack[--sp];
            break;
        case G_DL: {
            Gfx* next = (Gfx*)ptr_of(w1);
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
            if (src && n <= 64) {
                if (v0 + n > 64) n = 64 - v0;
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
            break;
        }
        case G_SETCOMBINE:
            comb_a = (w0 >> 20) & 0xf;
            comb_c = (w0 >> 15) & 0x1f;
            comb_b = (w1 >> 28) & 0xf;
            comb_set = 1;
            sync_combine();
            break;
        case G_SETPRIMCOLOR:
            prim = w1;
            pm_gpu_set_prim(prim);
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
            int tw = 1, th = 1;
            pm_tex_size(&tw, &th);
            float s0 = ((float)(s16)(hs >> 16) / 32.f - (float)tile_x0) / (float)tw;
            float t0 = ((float)(s16)(hs & 0xffff) / 32.f - (float)tile_y0) / (float)th;
            float dsdx = (float)(s16)(hd >> 16) / 1024.f;
            float dtdy = (float)(s16)(hd & 0xffff) / 1024.f;
            float width = (float)(xh - xl) / 4.f;
            float height = (float)(yh - yl) / 4.f;
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
            break;
        case G_SETTILE: {
            unsigned tile = (w1 >> 24) & 7;
            tile_fmt[tile] = (w0 >> 21) & 7;
            tile_siz[tile] = (w0 >> 19) & 3;
            break;
        }
        case G_LOADTLUT:
            tlut = timg;
            tlut_n = (int)((w1 >> 14) & 0x3ff) + 1;
            break;
        case G_SETTILESIZE: {
            unsigned tile = (w1 >> 24) & 7;
            if (tile != 0) break;
            int uls = (int)((w0 >> 12) & 0xfff);
            int ult = (int)(w0 & 0xfff);
            int lrs = (int)((w1 >> 12) & 0xfff);
            int lrt = (int)(w1 & 0xfff);
            int x0 = uls >> 2;
            int y0 = ult >> 2;
            int w = ((lrs - uls) >> 2) + 1;
            int h = ((lrt - ult) >> 2) + 1;
            int stride = timg_stride > w ? timg_stride : w;
            int bpp = tile_siz[0] >= 3 ? 4 : tile_siz[0] == 2 ? 2 : tile_siz[0] == 0 ? 0 : 1;
            const u8* src = (const u8*)timg;
            if (src && bpp) src += (size_t)(y0 * stride + x0) * (size_t)bpp;
            tile_x0 = x0;
            tile_y0 = y0;
            if (src && w > 0 && h > 0)
                pm_tex_load(src, tile_fmt[0], tile_siz[0], w, h, tlut, tlut_n, stride);
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
        case G_SETCIMG:
        case G_SETZIMG:
        case G_RDPHALF_1:
        case G_RDPHALF_2:
            break;
        default:
            note(op);
            break;
        }
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
