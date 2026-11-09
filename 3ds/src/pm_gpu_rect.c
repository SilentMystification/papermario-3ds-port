/* pm_gpu_rect.c: the vertex ring (chunks, wrap) and the rectangle queue (fill and texture rectangles as raw batches), moved from pm_gpu.c without change (C8).
 * The batch state is NdBatch nb and the render state PmGpuSt gs (pm_gpu_int.h). */
#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <string.h>
#include "nd.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "pm_gpu_int.h"
#define flush() pm_gpu_flush_from(__func__, __LINE__)

/* FB-1 ring_wrap: the raw vertex and index ring has no room for the next rectangle or grab quad. The limits are GRING_VTX and GRING_IDX (the ringsmall
 * switch was deleted in DEL). The walker paths keep the real limits. A wrap closes the open
 * batch, submits the queued part (split_frame) and moves the ring to a chunk the GPU has finished, else to a new one. No wait for the GPU happens here: the
 * mid-frame gspWaitForP3D hung on a list that citro3d had not started (FB-1 oracle decision). */
static unsigned ring_wraps;
/* the chunk c becomes the ring. The caller has flushed the open batch. */
static void ring_set(int c) {
    nb.ring_cur = c;
    nb.gring = nb.rchunk[c].vtx; nb.gidx = nb.rchunk[c].idx;
    nb.gring_n = nb.gidx_n = nb.gring_fl = nb.gidx_fl = nb.gi_start = nb.gni = 0;
    nb.buf_cur = 0;
}
/* a frame begins on chunk 0 with empty fills */
void ring_frame_begin(void) {
    nb.ring_fr++;
    nb.rchunk[0].stamp = nb.ring_fr;
    ring_set(0);
}
/* FB-1: the open batch takes kind k. A batch of the other kind is closed first, off chunk 0 only (on chunk 0 both kinds share the vertex base). */
void gi_kind_set(int k) {
    if (nb.gni == 0) nb.gi_kind = k;
    else if (nb.ring_cur && nb.gi_kind != k) { flush(); nb.gi_kind = k; }
}
/* a chunk other than the one in use: finished on the GPU (stamp < ring_done), or never used. A new chunk is linearAlloc'd (counted, logged). -1 when none. */
static __attribute__((cold, noinline)) int ring_pick(void) {
    for (int c = 0; c < nb.rchunk_n; c++) {
        if (c == nb.ring_cur || (nb.rchunk[c].used && nb.rchunk[c].stamp >= nb.ring_done)) continue;
        nb.rchunk[c].used = 1; nb.rchunk[c].stamp = nb.ring_fr;
        return c;
    }
    if (nb.rchunk_n >= RING_CHUNK_MAX) return -1;
    unsigned char* v = (unsigned char*)linearAlloc(GRING_VTX * 16u);
    unsigned short* x = (unsigned short*)linearAlloc(GRING_IDX * 2u);
    if (!v || !x) {
        if (v) linearFree(v);
        if (x) linearFree(x);
        pm_log_e("ring: chunk %d failed (linear)", nb.rchunk_n);
        return -1;
    }
    const int c = nb.rchunk_n++;
    nb.rchunk[c].vtx = v; nb.rchunk[c].idx = x; nb.rchunk[c].used = 1; nb.rchunk[c].stamp = nb.ring_fr;
    pm_log_w("ring: chunk %d allocated, %u chunks in use (%u KB each)", c, (unsigned)nb.rchunk_n, (unsigned)((GRING_VTX * 16u + GRING_IDX * 2u) >> 10));
    return c;
}
static __attribute__((cold, noinline)) int ring_wrap(void) {
    const int was_off = gs.off_active, prev = gs.off_cur;
    flush();
    if (was_off) pm_gpu_off_end(); /* the split runs on the main target, as grab_impl does */
    pm_gx_room();
    split_frame();
    const int c = ring_pick();
    if (c >= 0) {
        ring_set(c);
        ring_wraps++;
        pm_log_d("ringwrap f%u wraps %u chunk %d", pm_frame_count(), ring_wraps, c);
    }
    if (was_off) pm_gpu_off_begin(gs.offs[prev].base, gs.offs[prev].w == 64 ? 64 : 320, gs.offs[prev].siz);
    return c >= 0;
}
/* FB-1: 1 when nv vertices and ni indices fit the ring, after a ring_wrap when they do not. 0 when they cannot fit an empty chunk or no chunk is free. */
int ring_need(unsigned nv, unsigned ni) {
    if (nb.gring_n + nv <= GRING_VTX && nb.gidx_n + ni <= GRING_IDX) return 1;
    if (nv > GRING_VTX || ni > GRING_IDX) return 0;
    return ring_wrap();
}

/* Game pixels are y-down. The ortho matrix is y-up, and 320px is centered in 400. */
static float px(int x) { return gs.off_active ? (float)x : 40.f + (float)x; } /* F07-3: 2D is 1:1 and centered, also with wide */
static float py(int y) { return gs.off_active ? (float)y : 240.f - (float)y; }

static float fill_z = 0.5f;
/* RECT-1a: fill rectangles as raw Vtx batches (agent_s6.md RECT-1). Four Vtx in the ring: x, y screen pixels, z 0, s t 0, colour in the N64 Vtx layout (r in byte 12).
 * Palette entry RECT_GM (pcomp_calc) is the projection with the depth folded in. The batch closes through nd_raw_native with
 * rect_2d set (flush_impl_body rect branch). Merge key as rect_raw_open, plus the depth. rect_c: fill, tex, native, merged, cpu, ring high mark. */
static unsigned rect_c[6];
static unsigned rr_why[8]; /* RECT-2b diag: why a textured or fill rectangle is refused (dropped, rect_c[4]): 1 gpu, 2 off, 3 grab, 4 zmark, 5 ring full, 6 norectraw, 7 texcoord range */
static int rect_raw_ok(void) {
    if (!(gs.gpu_vtx_on && gs.gpu_ok && gs.gv4_on)) { rr_why[1]++; return 0; }
    if (!ring_need(4, 6)) { rr_why[5]++; return 0; }
    return 1;
}
void rect_raw_open(int ec, float fz, const float uvso[4]) {
    int rtx = pm_tex_cur_idx();
    rq_cur = rd_tex ? rtx : -1; /* RECT diag: the queue-time slot of this rectangle */
    if (nb.rect_open && nb.gni > 0 && nb.rect_ec == ec && nb.rect_zb == nb.rect_z && nb.rect_tex == rtx && fz == nb.rect_pal_z && !memcmp(nb.g_uvso, uvso, sizeof(nb.g_uvso)) && !PMDL("rectnomerge")) { dr1_merged++; rect_c[3]++; rect_sig_check("merge"); return; }
    if (nb.rect_open) dr1_breaks++;
    flush();
    nb.gi_kind = 0;
    nb.draw_3d = 0;
    nb.rect_ec = ec; nb.rect_zb = nb.rect_z; nb.rect_tex = rtx; nb.rect_open = 1; rq_n = 0;
    if (fz != nb.rect_pal_z) { /* the palette entry and the register caches hold the matrix of the old depth */
        nb.rect_pal_z = fz; gs.pcomp_gen[RECT_GM] = 0;
    }
    memcpy(nb.g_uvso, uvso, sizeof(nb.g_uvso));
    nb.g_vp[0] = gs.vp_sx; nb.g_vp[1] = gs.vp_sy; nb.g_vp[2] = gs.vp_tx; nb.g_vp[3] = gs.vp_ty;
    for (int i = 0; i < 4; i++) { nb.g_cmul[i] = 1.f; nb.g_cadd[i] = 0.f; }
    nb.g_zmix[0] = 1.f; nb.g_zmix[1] = 0.f;
    nb.g_ct = 0; nb.g_cullm = GPU_CULL_NONE; nb.g_pal = 1; nb.g_use5 = 0;
    nb.pal_n = 1; nb.pal_g[0] = RECT_GM; nb.pal_gen++;
    if (PMDL("rectsig")) rect_sig_fill(&rsig_open);
}
static void rect_raw_quad(int xa, int ya, int xb, int yb, short s0, short t0, short s1, short t1, unsigned c) {
    short v[32];
    const int px_[4] = { xa, xb, xb, xa }, py_[4] = { ya, ya, yb, yb };
    const short ps[4] = { s0, s1, s1, s0 }, pt[4] = { t0, t0, t1, t1 };
    for (int i = 0; i < 4; i++) {
        short* q = v + 8 * i;
        unsigned char* cb = (unsigned char*)q;
        q[0] = (short)px_[i]; q[1] = (short)py_[i]; q[2] = 0; q[3] = 0; q[4] = ps[i]; q[5] = pt[i];
        cb[12] = (unsigned char)(c >> 24); cb[13] = (unsigned char)(c >> 16); cb[14] = (unsigned char)(c >> 8); cb[15] = (unsigned char)c;
    }
    const int b = pm_gpu_gvtx_load(v, 4);
    unsigned short* ix = nb.gidx + nb.gidx_n;
    ix[0] = (unsigned short)b; ix[1] = (unsigned short)(b + 1); ix[2] = (unsigned short)(b + 2);
    ix[3] = (unsigned short)b; ix[4] = (unsigned short)(b + 2); ix[5] = (unsigned short)(b + 3);
    nb.gidx_n += 6; nb.gni += 6;
    if (rq_n < RQ_MAX) { rq_slot[rq_n] = rq_cur; rq_b[rq_n] = b; rq_st[rq_n][0] = s0; rq_st[rq_n][1] = t0; rq_st[rq_n][2] = s1; rq_st[rq_n][3] = t1; }
    rq_n++;
    rect_c[2]++;
    if ((unsigned)nb.gring_n > rect_c[5]) rect_c[5] = (unsigned)nb.gring_n;
}
void rect_log(unsigned f) { if (PMDL("rectsig")) pm_log_d("rsig sum f%u chk %u diff %u", f, rsig_chk, rsig_diff); pm_log_d("rect f%u fill %u tex %u native %u merged %u drop %u ring %u", f, rect_c[0], rect_c[1], rect_c[2], rect_c[3], rect_c[4], rect_c[5]); pm_log_d("rectwhy f%u gpu %u off %u grab %u zmark %u ring %u norect %u range %u", f, rr_why[1], rr_why[2], rr_why[3], rr_why[4], rr_why[5], rr_why[6], rr_why[7]); memset(rr_why, 0, sizeof rr_why); memset(rect_c, 0, sizeof rect_c);
    nd_tridrop_log(f);
}
void pm_gpu_fill_rect(int x0, int y0, int x1, int y1, unsigned rgba) {
    if (PMDL("zmforce")) { gs.z_on = 1; nb.rect_z = 1; gs.r_zmark = 1; gs.r_zmark_zn = 0.5f; } /* FB-1 ZM forcing: every rectangle is a zmark rectangle (CPU and native alike) */
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    rect_c[0]++;
    rd_tex = 0;
    if (rect_raw_ok()) {
        static const float uv0[4] = { 1.f, 1.f, 0.f, 0.f };
        /* F07-3: a fill from the game edge reaches the screen edge. OFF-1: an off-screen pass is px and py (no wide, no 40 pixel margin) */
        int xa = gs.off_active ? x0 : (pm_wide && x0 <= 0) ? 0 : 40 + x0;
        int xb = gs.off_active ? x1 + 1 : (pm_wide && x1 >= 319) ? 400 : 40 + x1 + 1;
        int ya = gs.off_active ? y0 : 240 - y0, yb = gs.off_active ? y1 + 1 : 240 - (y1 + 1);
        rect_raw_open(0, fill_z, uv0);
        rect_raw_quad(xa, ya, xb, yb, 0, 0, 0, 0, rgba);
        if (!gs.off_active) gs.grab_dirty = 1; /* a fill draws rows the grab does not track (rect_y0 and rect_y1 are for texture rectangles): the next grab copies */
        return;
    }
    rect_c[4]++; /* no CPU rectangle path: a refused rectangle is dropped and counted (rect log: drop) */
    return;
}

void pm_gpu_fill_rect_z(int x0, int y0, int x1, int y1, unsigned rgba, float d) {
    fill_z = d; /* the batch in flight keeps its own depth state: rect_raw_open draws it first when rect_z differs */
    nb.rect_z = 1;
    pm_gpu_fill_rect(x0, y0, x1, y1, rgba);
    nb.rect_z = 0;
    fill_z = 0.5f;
}

/* B3: a fill into the z-buffer image: depth d in the rectangle, colour untouched, no depth test (r_nocol, r_zupd, no r_zcmp). */
void pm_gpu_fill_depth(int x0, int y0, int x1, int y1, float d) {
    int s_nc = gs.r_nocol, s_zc = gs.r_zcmp, s_zu = gs.r_zupd, s_bl = gs.r_blend, s_at = gs.r_atest, s_de = gs.r_decal, s_z = gs.z_on, s_mix = gs.r_mix;
    flush();
    int s_zm = gs.r_zmark; gs.r_zmark = 0;
    gs.r_nocol = 1; gs.r_zcmp = 0; gs.r_zupd = 1; gs.r_blend = 0; gs.r_atest = -1; gs.r_decal = 0; gs.z_on = 1; gs.r_mix = 0;
    pm_gpu_fill_rect_z(x0, y0, x1, y1, 0, d);
    flush(); /* DR1: the rectangle is drawn with the temporary render state */
    gs.r_zmark = s_zm;
    gs.r_nocol = s_nc; gs.r_zcmp = s_zc; gs.r_zupd = s_zu; gs.r_blend = s_bl; gs.r_atest = s_at; gs.r_decal = s_de; gs.z_on = s_z; gs.r_mix = s_mix;
}

/* RECT-1b: a texture rectangle as a raw batch (agent_s6.md RECT-1b). s and t are normalized: the ring gets shorts s * sc, the batch uvso is {1 / sc, 1 / sc, 0, 0} (pica uv = tc * uvso.xy + uvso.zw).
 * sc is 4096, 1024 or 256 by the largest |s| or |t| (the short range): above 127 the piece is refused (returns 0, rr_why 7). */
static short tex_st(float v, float sc) { float f = v * sc; return (short)(f >= 0.f ? f + 0.5f : f - 0.5f); }
/* SC-1 (FB-1): one textured piece as a raw batch. Coordinates at or above 127 take the base form: the shorts hold s - bs and uvso.zw = bs (pica uv = tc * uvso.xy + uvso.zw),
 * bs = floor of the lower s (bt for t). Below 127 the base is 0 and the batch is as before. sc follows the largest value that the shorts hold.
 * Returns 0 when the piece is still too wide (tex_raw_rect splits first, so this is not reached in practice). */
static int tex_raw_piece(int xa, int ya, int xb, int yb, float s0, float t0, float s1, float t1, int y0, int y1) {
    const int saved = gs.combine;
    const int ec = (saved == 4 || saved == 5 || saved == 6 || saved == 7 || saved == 8) ? saved : 2; /* MSGBOX 2 (UI1): class 4 (PM_CC_07 drop shadow, constant colour times texel alpha) keeps its class; as class 2 it drew the glyph texels */
    float mx = 0.f;
    const float av[4] = { s0 < 0.f ? -s0 : s0, s1 < 0.f ? -s1 : s1, t0 < 0.f ? -t0 : t0, t1 < 0.f ? -t1 : t1 };
    for (int i = 0; i < 4; i++) if (av[i] > mx) mx = av[i];
    float bs = 0.f, bt = 0.f;
    if (mx >= 127.f) { bs = floorf(s0 < s1 ? s0 : s1); bt = floorf(t0 < t1 ? t0 : t1); }
    const float r0s = s0 - bs, r1s = s1 - bs, r0t = t0 - bt, r1t = t1 - bt;
    const float rv[4] = { r0s < 0.f ? -r0s : r0s, r1s < 0.f ? -r1s : r1s, r0t < 0.f ? -r0t : r0t, r1t < 0.f ? -r1t : r1t };
    float rmx = 0.f;
    for (int i = 0; i < 4; i++) if (rv[i] > rmx) rmx = rv[i];
    float sc, inv;
    if (rmx < 7.9f) { sc = 4096.f; inv = 0.000244140625f; }
    else if (rmx < 31.9f) { sc = 1024.f; inv = 0.0009765625f; }
    else if (rmx < 127.f) { sc = 256.f; inv = 0.00390625f; }
    else { rr_why[7]++; return 0; }
    unsigned c = (saved == 5 || saved == 7) ? gs.env_col : (saved == 8 && gs.plan.vtxname != PM_NM_SHADE) ? gs.plan.vtx : 0xffffffffu;
    const float uvso[4] = { inv, inv, bs, bt };
    rect_raw_open(ec, fill_z, uvso);
    rect_raw_quad(xa, ya, xb, yb, tex_st(r0s, sc), tex_st(r0t, sc), tex_st(r1s, sc), tex_st(r1t, sc), c);
    { /* R13c: the rows this rectangle draws (the close reads them for the grab) */
        int ry0 = y0 < 0 ? 0 : y0, ry1 = y1 + 1;
        if (nb.rect_y1 > nb.rect_y0) { if (ry0 < nb.rect_y0) nb.rect_y0 = ry0; if (ry1 > nb.rect_y1) nb.rect_y1 = ry1; }
        else { nb.rect_y0 = ry0; nb.rect_y1 = ry1; }
    }
    return 1;
}
/* RECT-1b: a texture rectangle as a raw batch (agent_s6.md RECT-1b). s and t are normalized: the ring gets shorts s * sc, the batch uvso is {1 / sc, 1 / sc, bs, bt}.
 * A rectangle whose s or t span needs more than about 120 texels is split into equal pieces along that axis (SC-1). */
static int tex_raw_rect(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1) {
    if (!rect_raw_ok()) return 0;
    float xa = px(x0), xb = px(x1 + 1);
    if (pm_wide && !gs.off_active && xb > xa && x1 - x0 + 1 >= 160 && !pm_tex_cell_bound()) { /* F07-7: a rectangle on the game edge reaches the screen edge. HUDTRAIL: only a screen-covering rectangle (>= 160 game px) does; a HUD piece on game x 0 or 319 repeated its texture across the gutter. MSGBOX 4: not a glyph: a glyph at game x 0 (wide bubble text) drew 40 px of the cells left of it */
        float ds = (s1 - s0) / (xb - xa);
        if (x0 == 0) { s0 -= ds * xa; xa = 0.f; }
        if (x1 == 319) { s1 += ds * (400.f - xb); xb = 400.f; }
    }
    const float ya = py(y0), yb = py(y1 + 1);
    const int ixa = (int)xa, ixb = (int)xb, iya = (int)ya, iyb = (int)yb;
    /* split count: the base form is needed when the magnitude is at or above 127; a piece holds a span of about 120 texels */
    float mx = 0.f;
    const float av[4] = { s0 < 0.f ? -s0 : s0, s1 < 0.f ? -s1 : s1, t0 < 0.f ? -t0 : t0, t1 < 0.f ? -t1 : t1 };
    for (int i = 0; i < 4; i++) if (av[i] > mx) mx = av[i];
    int nx = 1, ny = 1;
    if (mx >= 127.f) {
        const float spanS = s0 < s1 ? s1 - s0 : s0 - s1, spanT = t0 < t1 ? t1 - t0 : t0 - t1;
        if (spanS >= 120.f || spanT >= 120.f) { nx = 1 + (int)(spanS / 120.f); ny = 1 + (int)(spanT / 120.f); }
    }
    if (nx == 1 && ny == 1) return tex_raw_piece(ixa, iya, ixb, iyb, s0, t0, s1, t1, y0, y1);
    const int W = ixb - ixa, H = iyb - iya;
    if (W == 0 || H == 0 || (W < 0 ? -W : W) < nx || (H < 0 ? -H : H) < ny) { rr_why[7]++; return 0; }
    if (!ring_need(4u * (unsigned)nx * (unsigned)ny, 6u * (unsigned)nx * (unsigned)ny)) { rr_why[5]++; return 0; } /* the pieces must fit the ring together */
    for (int j = 0; j < ny; j++) {
        const int ja = iya + H * j / ny, jb = iya + H * (j + 1) / ny;
        const float ta = t0 + (t1 - t0) * (float)(ja - iya) / (float)H, tb = t0 + (t1 - t0) * (float)(jb - iya) / (float)H;
        for (int i = 0; i < nx; i++) {
            const int ia = ixa + W * i / nx, ib = ixa + W * (i + 1) / nx;
            const float sa = s0 + (s1 - s0) * (float)(ia - ixa) / (float)W, sb = s0 + (s1 - s0) * (float)(ib - ixa) / (float)W;
            tex_raw_piece(ia, ja, ib, jb, sa, ta, sb, tb, y0, y1);
        }
    }
    return 1;
}
void pm_gpu_tex_rect(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1) {
    if (PMDL("zmforce")) { gs.z_on = 1; nb.rect_z = 1; gs.r_zmark = 1; gs.r_zmark_zn = 0.5f; } /* FB-1 ZM forcing (as pm_gpu_fill_rect) */
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; float s = s0; s0 = s1; s1 = s; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; float s = t0; t0 = t1; t1 = s; }
    fl_rect_note();
    rect_c[1]++;
    rd_tex = 1;
    if (tex_raw_rect(x0, y0, x1, y1, s0, t0, s1, t1)) { fl_rect_done(); return; }
    rect_c[4]++; /* no CPU rectangle path: a refused rectangle is dropped and counted (rect log: drop) */
    fl_rect_done();
}

void pm_gpu_tex_rect_z(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1, float d) {
    fill_z = d;
    nb.rect_z = 1;
    pm_gpu_tex_rect(x0, y0, x1, y1, s0, t0, s1, t1);
    nb.rect_z = 0;
    fill_z = 0.5f;
}

