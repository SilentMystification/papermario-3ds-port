/* nd_seam_test.c: seam host test (plan_native_system 9.5, gate G9). Step 1: read the N64 shapes of mac_01 and mac_02
 * (big-endian, loaded at SHAPE_BASE 0x80210000, pm_map.c:17), walk the file for G_VTX loads (F3DEX: word0 top byte 0x01,
 * n = ((w0 >> 12) & 0xFF) + 1, word1 = N64 address), read the Vtx records (16 bytes, s16 x y z at 0..5) and count the
 * distinct positions that both models have (shared edge candidates). Data words can match G_VTX, so the count is a candidate list.
 * Step 2 (S1-4): ndref kind 5 records hold the inputs (post, game matrix) of every palette product the old path made, and the kind 1 palette holds its
 * result (libcitro3d Mtx_Multiply). nd_mtx_pcomp must reproduce each result bit for bit. Then the shared positions of step 1 go through the fp24 dp4 with
 * the old matrix and the native matrix: the two must be equal, so no crack opens on a shared edge.
 * Step 3 (SEAMFIX): per shape after "--", the T-point table of nd_seam.c (shape tree, lists by matrix class, boundary edges, points inside them) and the same
 * detection on the nd_dlc_compile output of every identity and integer translation list.
 * Step 4 (SEAMFIX): mac_01 rasterized with the camera of ndref_toad (map game matrix): the PICA centre fill against an any-of-8-samples fill, holes counted.
 * Usage: nd_seam_test <mac_01_shape.bin> <mac_02_shape.bin> <ndref.bin> ... [-- <shape.bin> ...] */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "nd.h"
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_op.h"

/* pm_interp.c stubs (nd_mtx.c links them): no replacement table */
int pm_interp_repl;
const float (*pm_interp_find(const void* m))[4] { (void)m; return 0; }

#define SHAPE_BASE 0x80210000u
#define G_VTX 0x01u
#define POS_MAX 65536u

typedef struct { int16_t x, y, z; } Pos;
typedef struct { Pos* p; unsigned n; uint8_t* raw; size_t len; } Shape;

static uint32_t be32(const uint8_t* p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static int16_t be16(const uint8_t* p) { return (int16_t)(p[0] << 8 | p[1]); }
static uint32_t le32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

/* fp24: sign, 7 bit exponent, 16 bit mantissa. Round to nearest even on the float32 mantissa (drop 7 bits). Same as ndt_host_test.c fp24. */
static float fp24(float x) {
    union { float f; uint32_t u; } v; uint32_t lsb;
    v.f = x;
    if ((v.u & 0x7fffffffu) == 0) return x;
    lsb = (v.u >> 7) & 1u;
    v.u += 0x3fu + lsb;
    v.u &= ~0x7fu;
    return v.f;
}
static float dp4_fp24(const float row[4], const float v[4]) {
    float acc = 0.f;
    for (int k = 0; k < 4; k++) acc = fp24(acc + fp24(fp24(row[k]) * fp24(v[k])));
    return acc;
}

static int load(const char* path, Shape* s) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); s->len = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    s->raw = malloc(s->len);
    int ok = s->raw && fread(s->raw, 1, s->len, f) == s->len;
    fclose(f);
    s->p = malloc(POS_MAX * sizeof(Pos));
    return ok && s->p;
}

/* Every loaded record is kept. Vertex slots are reused by later display lists, so a slot is not a vertex. */
static void walk(Shape* s) {
    s->n = 0;
    for (size_t w = 0; w + 8 <= s->len; w += 8) {
        uint32_t w0 = be32(s->raw + w), w1 = be32(s->raw + w + 4);
        if ((w0 >> 24) != G_VTX) continue;
        unsigned n = ((w0 >> 12) & 0xFFu) + 1u;
        if (w1 < SHAPE_BASE) continue;
        size_t off = (size_t)(w1 - SHAPE_BASE);
        if (off + 16u * n > s->len) continue;
        for (unsigned i = 0; i < n && s->n < POS_MAX; i++) {
            const uint8_t* v = s->raw + off + 16u * i;
            s->p[s->n].x = be16(v); s->p[s->n].y = be16(v + 2); s->p[s->n].z = be16(v + 4);
            s->n++;
        }
    }
}

static int cmp_pos(const void* a, const void* b) {
    const Pos* x = a; const Pos* y = b;
    if (x->x != y->x) return x->x < y->x ? -1 : 1;
    if (x->y != y->y) return x->y < y->y ? -1 : 1;
    if (x->z != y->z) return x->z < y->z ? -1 : 1;
    return 0;
}

/* distinct positions of a model, in place. Returns the new count. */
static unsigned uniq(Pos* p, unsigned n) {
    if (!n) return 0;
    qsort(p, n, sizeof(Pos), cmp_pos);
    unsigned k = 1;
    for (unsigned i = 1; i < n; i++) if (cmp_pos(&p[i], &p[k - 1]) != 0) p[k++] = p[i];
    return k;
}

/* ---- SEAMFIX steps 3 and 4 ---- */
/* the fixed shape file as the game holds it (pm_map.c fix_shape): host pointers inside the buffer, Vtx s16 and Mtx words in host order */
typedef struct { uint8_t* buf; uint32_t size; uint8_t* seen; } Fix;
static uint8_t* fx_in(Fix* s, uint32_t a) { return a < SHAPE_BASE || a >= SHAPE_BASE + s->size ? NULL : s->buf + (a - SHAPE_BASE); }
static int fx_claim(Fix* s, uint8_t* p) { uint32_t i; if (!p) return 0; i = (uint32_t)(p - s->buf); if (i >= s->size || s->seen[i]) return 0; s->seen[i] = 1; return 1; }
static void fx_w32(uint8_t* p, uint32_t v) { memcpy(p, &v, 4); }
static void fx_mtx(uint8_t* p) { for (int i = 0; i < 16; i++) fx_w32(p + 4 * i, be32(p + 4 * i)); }
static void fx_node(Fix* s, uint8_t* node);
static void fx_gfx(Fix* s, uint8_t* dl) {
    if (!fx_claim(s, dl)) return;
    for (int n = 0; n < 100000; n++) {
        uint32_t off = (uint32_t)(dl - s->buf), w0, w1, op;
        if (off + 8 > s->size) return;
        w0 = be32(dl); w1 = be32(dl + 4); op = w0 >> 24;
        if (op == 0x01 || op == 0xDA || op == 0xDE || op == 0xDC) {
            uint8_t* tg = fx_in(s, w1);
            if (tg) {
                if (op == 0x01) {
                    int count = (w0 >> 12) & 0xff;
                    if (count > 64) count = 0;
                    for (int i = 0; i < count; i++) {
                        uint8_t* v = tg + 16u * i; uint32_t vi = (uint32_t)(v - s->buf);
                        if (vi + 16u > s->size) break;
                        if (s->seen[vi]) continue;
                        s->seen[vi] = 1;
                        for (int k = 0; k < 6; k++) { uint8_t t = v[2 * k]; v[2 * k] = v[2 * k + 1]; v[2 * k + 1] = t; }
                    }
                } else if (op == 0xDA) { if (fx_claim(s, tg)) fx_mtx(tg); }
                w1 = (uint32_t)(uintptr_t)tg;
            }
        }
        fx_w32(dl, ND_X0(w0, w1)); fx_w32(dl + 4, ND_X1(w0, w1)); /* HC4f3: typed as nd_shape_conv writes it */
        dl += 8;
        if (op == 0xDF) return;
        if (op == 0xDE) { uint8_t* nx = (uint8_t*)(uintptr_t)w1; if (nx >= s->buf && nx < s->buf + s->size) fx_gfx(s, nx); }
    }
}
static void fx_props(Fix* s, uint8_t* props, uint32_t count) {
    if (!props || count == 0 || count > 256 || !fx_claim(s, props)) return;
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* e = props + 12 * i; uint32_t key, fmt, val;
        if ((uint32_t)(e - s->buf) + 12 > s->size) return;
        key = be32(e); fmt = be32(e + 4); val = be32(e + 8);
        if (key == 0x5E || fmt == 2) { uint8_t* p = fx_in(s, val); if (p) val = (uint32_t)(uintptr_t)p; }
        fx_w32(e, key); fx_w32(e + 4, fmt); fx_w32(e + 8, val);
    }
}
static void fx_group(Fix* s, uint8_t* g) {
    uint32_t mtx, lights, nl, nc, ch;
    uint8_t* list;
    if (!fx_claim(s, g) || (uint32_t)(g - s->buf) + 20 > s->size) return;
    mtx = be32(g); lights = be32(g + 4); nl = be32(g + 8); nc = be32(g + 12); ch = be32(g + 16);
    { uint8_t* p = fx_in(s, mtx); if (p && fx_claim(s, p)) fx_mtx(p); mtx = p ? (uint32_t)(uintptr_t)p : 0; }
    { uint8_t* p = fx_in(s, lights); lights = p ? (uint32_t)(uintptr_t)p : 0; }
    list = fx_in(s, ch); ch = list ? (uint32_t)(uintptr_t)list : 0;
    fx_w32(g, mtx); fx_w32(g + 4, lights); fx_w32(g + 8, nl); fx_w32(g + 12, nc); fx_w32(g + 16, ch);
    if (!list || nc > 256 || !fx_claim(s, list)) return;
    for (uint32_t i = 0; i < nc; i++) {
        uint8_t* slot = list + 4 * i; uint8_t* node;
        if ((uint32_t)(slot - s->buf) + 4 > s->size) return;
        node = fx_in(s, be32(slot));
        fx_w32(slot, node ? (uint32_t)(uintptr_t)node : 0);
        if (node) fx_node(s, node);
    }
}
static void fx_node(Fix* s, uint8_t* node) {
    uint32_t type, disp, nprop, props, group;
    uint8_t* d;
    if (!fx_claim(s, node) || (uint32_t)(node - s->buf) + 20 > s->size) return;
    type = be32(node); disp = be32(node + 4); nprop = be32(node + 8); props = be32(node + 12); group = be32(node + 16);
    d = fx_in(s, disp);
    if (d && fx_claim(s, d)) { uint8_t* dl = fx_in(s, be32(d)); fx_w32(d, dl ? (uint32_t)(uintptr_t)dl : 0); if (dl) fx_gfx(s, dl); disp = (uint32_t)(uintptr_t)d; } else disp = 0;
    { uint8_t* p = fx_in(s, props); props = p ? (uint32_t)(uintptr_t)p : 0; fx_props(s, p, nprop); }
    { uint8_t* p = fx_in(s, group); group = p ? (uint32_t)(uintptr_t)p : 0; if (p) fx_group(s, p); }
    fx_w32(node, type); fx_w32(node + 4, disp); fx_w32(node + 8, nprop); fx_w32(node + 12, props); fx_w32(node + 16, group);
}
static uint8_t* fix_load(const char* path, uint32_t* size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    Fix s; s.size = (uint32_t)n; s.buf = malloc((size_t)n + 8); s.seen = calloc((size_t)n, 1);
    if (!s.buf || !s.seen || fread(s.buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); return NULL; }
    fclose(f);
    uint8_t* root = fx_in(&s, be32(s.buf));
    fx_w32(s.buf, root ? (uint32_t)(uintptr_t)root : 0);
    if (root) fx_node(&s, root);
    free(s.seen);
    *size = s.size;
    return s.buf;
}

/* the compile of every identity and integer translation list of a table, into a soup of the compiled triangles (world positions) */
static const uint8_t* cbuf; static uint32_t csize;
static const void* c_vtx(uint32_t w1, unsigned n) {
    uint32_t b = (uint32_t)(uintptr_t)cbuf;
    if (w1 < b || w1 - b + 16u * n > csize || n == 0 || n > 64) return NULL;
    return (const void*)(uintptr_t)w1;
}
typedef struct { int32_t w[3][3]; } WTri;
static WTri* ctri; static unsigned nctri, cctri;
static unsigned c_lists, c_ref, c_ins;
static int compile_soup(const NdSeamTab* t, NdSeamTab* out, int use_tp) {
    NdSeamSoup* s = nd_seam_soup_new(60000);
    static uint32_t lw[2 * (ND_DLC_MAXOPS + 1)];
    nctri = 0; c_lists = 0; c_ref = 0; c_ins = 0;
    for (unsigned li = 0; li < t->nlist; li++) {
        const NdSeamList* L = &t->list[li];
        if (!L->ok) continue;
        const uint8_t* p = (const uint8_t*)(uintptr_t)L->addr;
        unsigned i;
        for (i = 0; i < ND_DLC_MAXOPS; i++) { memcpy(&lw[2 * i], p + 8 * i, 8); if ((lw[2 * i] >> 24) == 0xDF || (lw[2 * i] >> 24) == NDO_END) break; }
        NdDlcEnv env; memset(&env, 0, sizeof env); env.vtx = c_vtx;
        if (use_tp && L->cnt) { env.tp = t->tp + L->first; env.ntp = L->cnt; }
        NdObj o;
        int rc = nd_dlc_compile(&o, lw, &env);
        if (rc) { c_ref++; continue; }
        c_lists++; c_ins += o.nins;
        if (L->cls == 2) { nd_dlc_free(&o); continue; } /* the soup holds world positions of the identity and translation lists only (no matrix here) */
        int sl = nd_seam_soup_list(s, L->addr, L->cls, L->t, 1);
        for (unsigned k = 0; k < o.nidx / 3u; k++) {
            int32_t w[3][3], lcn[3][3];
            for (int c = 0; c < 3; c++) { const int16_t* v = (const int16_t*)(o.vtx + 16u * (o.idx[3 * k + c] - o.vfirst)); for (int q = 0; q < 3; q++) { lcn[c][q] = v[q]; w[c][q] = v[q] + L->t[q]; } }
            nd_seam_soup_tri(s, sl, k, (const int32_t(*)[3])w, (const int32_t(*)[3])lcn);
            if (nctri >= cctri) { cctri = cctri ? 2 * cctri : 8192; ctri = realloc(ctri, cctri * sizeof *ctri); }
            memcpy(ctri[nctri++].w, w, sizeof w);
        }
        nd_dlc_free(&o);
    }
    return nd_seam_soup_finish(s, out);
}

/* step 4: the PICA fill (dp4 in fp24, divide in fp32, 1/16 pixel snap, pixel centre, top-left rule, nearest 1/w) against an any-of-8-samples fill (the N64
 * coverage that the VI filter heals). A hole is a pixel where the samples see a surface that the centre rule misses (the centre shows something farther, or
 * nothing) while its left and right (or upper and lower) neighbours show that surface: a line or a pinhole inside a surface, not a silhouette. Cause T: the
 * pixel lies within 1.5 pixels of a receiver edge of the T-point table. */
#define RW 240
#define RH 400
static float zc[RW * RH], zs[RW * RH];
static int64_t ef(int64_t ax, int64_t ay, int64_t bx, int64_t by, int64_t px, int64_t py) { return (bx - ax) * (py - ay) - (by - ay) * (px - ax); }
static int project(const float o[16], const int32_t w[3], float* sx, float* sy, float* iw) {
    float v[4] = { 1.f, (float)w[2], (float)w[1], (float)w[0] };
    float cx = dp4_fp24(o + 0, v), cy = dp4_fp24(o + 4, v), cw = dp4_fp24(o + 12, v);
    if (cw < 0.05f) return 0;
    *sx = (cx / cw * 0.5f + 0.5f) * RW; *sy = (cy / cw * 0.5f + 0.5f) * RH; *iw = 1.0f / cw;
    return !(*sx < -2048.f || *sx > 2048.f || *sy < -2048.f || *sy > 2048.f);
}
typedef struct { float ax, ay, bx, by; } Seg;
static Seg* tseg; static unsigned ntseg;
static float seg_d2(const Seg* s, float px, float py) {
    float dx = s->bx - s->ax, dy = s->by - s->ay, l2 = dx * dx + dy * dy, t = l2 > 0 ? ((px - s->ax) * dx + (py - s->ay) * dy) / l2 : 0;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    float ex = s->ax + t * dx - px, ey = s->ay + t * dy - py;
    return ex * ex + ey * ey;
}
/* the receiver edges of the table, projected: the list words decoded again (raw triangle ordinals) */
static void tedges(const NdSeamTab* t, const float o[16]) {
    ntseg = 0;
    free(tseg); tseg = malloc((t->ntp + 1) * sizeof *tseg);
    for (unsigned li = 0; li < t->nlist; li++) {
        const NdSeamList* L = &t->list[li];
        if (!L->cnt) continue;
        int32_t pos[64][3];
        unsigned ord = 0, e = L->first, end = L->first + L->cnt;
        const uint8_t* p = (const uint8_t*)(uintptr_t)L->addr;
        for (unsigned i = 0; i < ND_DLC_MAXOPS && e < end; i++) {
            uint32_t w0, w1; memcpy(&w0, p + 8 * i, 4); memcpy(&w1, p + 8 * i + 4, 4);
            { uint32_t t[2]; nd_op_unit(w0, w1, t); w0 = t[0]; w1 = t[1]; } /* HC4f3: the typed unit */
            unsigned op = w0 >> 24;
            if (op == NDO_END) break;
            if (op == NDO_VTX) {
                unsigned n = (w0 >> 12) & 0xFF, v0n = (w0 >> 1) & 0x7F, v0 = v0n >= n ? v0n - n : 0;
                for (unsigned k = 0; k < n && v0 + k < 64; k++) { const int16_t* s = (const int16_t*)(uintptr_t)(w1 + 16u * k); for (int q = 0; q < 3; q++) pos[v0 + k][q] = s[q] + L->t[q]; }
            } else if (op == NDO_TRI1 || op == NDO_TRI2) {
                for (int tt = 0; tt < (op == NDO_TRI2 ? 2 : 1); tt++) {
                    uint32_t ww = tt ? w1 : w0;
                    unsigned ix[3] = { ((ww >> 16) & 0xFF) / 2, ((ww >> 8) & 0xFF) / 2, (ww & 0xFF) / 2 };
                    while (e < end && t->tp[e].tri == ord) {
                        unsigned ed = t->tp[e].edge;
                        float ax, ay, bx, by, iw;
                        if (ix[ed] < 64 && ix[ed == 2 ? 0 : ed + 1] < 64 && project(o, pos[ix[ed]], &ax, &ay, &iw) && project(o, pos[ix[ed == 2 ? 0 : ed + 1]], &bx, &by, &iw)) {
                            Seg* s = &tseg[ntseg++]; s->ax = ax; s->ay = ay; s->bx = bx; s->by = by;
                        }
                        e++;
                    }
                    ord++;
                }
            }
        }
    }
}
static unsigned raster(const float o[16], unsigned* nskip, unsigned* ht) {
    static const int smp[8][2] = { { 1, 9 }, { 3, 3 }, { 5, 13 }, { 7, 7 }, { 9, 1 }, { 11, 11 }, { 13, 5 }, { 15, 15 } };
    memset(zc, 0, sizeof zc); memset(zs, 0, sizeof zs);
    *nskip = 0;
    for (unsigned k = 0; k < nctri; k++) {
        int64_t X[3], Y[3];
        float iw[3], fx[3], fy[3];
        int bad = 0;
        for (int c = 0; c < 3 && !bad; c++) {
            if (!project(o, ctri[k].w[c], &fx[c], &fy[c], &iw[c])) bad = 1;
            else { X[c] = (int64_t)(fx[c] * 16.f + (fx[c] < 0 ? -0.5f : 0.5f)); Y[c] = (int64_t)(fy[c] * 16.f + (fy[c] < 0 ? -0.5f : 0.5f)); }
        }
        if (bad) { (*nskip)++; continue; }
        int64_t area = ef(X[0], Y[0], X[1], Y[1], X[2], Y[2]);
        if (area == 0) continue;
        if (area < 0) { int64_t t = X[1]; X[1] = X[2]; X[2] = t; t = Y[1]; Y[1] = Y[2]; Y[2] = t; float f = iw[1]; iw[1] = iw[2]; iw[2] = f; area = -area; }
        int64_t mnx = X[0], mxx = X[0], mny = Y[0], mxy = Y[0];
        for (int c = 1; c < 3; c++) { if (X[c] < mnx) mnx = X[c]; if (X[c] > mxx) mxx = X[c]; if (Y[c] < mny) mny = Y[c]; if (Y[c] > mxy) mxy = Y[c]; }
        int x0 = (int)(mnx >> 4) - 1, x1 = (int)(mxx >> 4) + 1, y0 = (int)(mny >> 4) - 1, y1 = (int)(mxy >> 4) + 1;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > RW - 1) x1 = RW - 1;
        if (y1 > RH - 1) y1 = RH - 1;
        int tl[3];
        for (int e = 0; e < 3; e++) { int n = e == 2 ? 0 : e + 1; int64_t dx = X[n] - X[e], dy = Y[n] - Y[e]; tl[e] = (dy < 0) || (dy == 0 && dx > 0); }
        float inva = 1.0f / (float)area;
        for (int py = y0; py <= y1; py++)
            for (int px = x0; px <= x1; px++) {
                int64_t cx = 16 * px + 8, cy = 16 * py + 8;
                int64_t b0 = ef(X[1], Y[1], X[2], Y[2], cx, cy), b1 = ef(X[2], Y[2], X[0], Y[0], cx, cy), b2 = ef(X[0], Y[0], X[1], Y[1], cx, cy);
                float z = ((float)b0 * iw[0] + (float)b1 * iw[1] + (float)b2 * iw[2]) * inva; /* 1/w at the centre (the plane, also outside the triangle) */
                int in = !(b2 < 0 || (b2 == 0 && !tl[0])) && !(b0 < 0 || (b0 == 0 && !tl[1])) && !(b1 < 0 || (b1 == 0 && !tl[2]));
                int i = py * RW + px;
                if (in && z > zc[i]) zc[i] = z;
                if (z > zs[i])
                    for (int q = 0; q < 8; q++) {
                        int64_t sx = 16 * px + smp[q][0], sy = 16 * py + smp[q][1];
                        if (ef(X[0], Y[0], X[1], Y[1], sx, sy) >= 0 && ef(X[1], Y[1], X[2], Y[2], sx, sy) >= 0 && ef(X[2], Y[2], X[0], Y[0], sx, sy) >= 0) { zs[i] = z; break; }
                    }
            }
    }
    unsigned holes = 0;
    *ht = 0;
    for (int py = 1; py < RH - 1; py++)
        for (int px = 1; px < RW - 1; px++) {
            int i = py * RW + px;
            float s = zs[i];
            if (s <= 0 || zc[i] >= s * 0.97f) continue; /* the centre shows the surface the samples see */
            float lo = s * 0.97f;
            if (!((zc[i - 1] >= lo && zc[i + 1] >= lo) || (zc[i - RW] >= lo && zc[i + RW] >= lo))) continue; /* a silhouette, not a hole */
            holes++;
            for (unsigned k = 0; k < ntseg; k++) if (seg_d2(&tseg[k], px + 0.5f, py + 0.5f) <= 2.25f) { (*ht)++; break; }
        }
    return holes;
}

/* the camera of the map draws of an ndref file: the most frequent game matrix of its kind 5 records, pcomp with the post matrix */
static int ndref_camera(const char* path, float o[16]) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;
    fseek(fp, 0, SEEK_END); long sz = ftell(fp); fseek(fp, 0, SEEK_SET);
    uint8_t* buf = malloc((size_t)sz);
    if (!buf || fread(buf, 1, (size_t)sz, fp) != (size_t)sz) { fclose(fp); return 0; }
    fclose(fp);
    static unsigned cnt[64]; static float post[64][16], game[64][4][4];
    memset(cnt, 0, sizeof cnt);
    for (size_t pos = 20, end = 20 + le32(buf + 16); pos < end;) {
        uint32_t kind = le32(buf + pos), size = le32(buf + pos + 4);
        const uint8_t* body = buf + pos + 8;
        if (kind == 5) {
            unsigned np = le32(body + 64);
            for (unsigned k = 0; k < np; k++) {
                unsigned gm = le32(body + 68 + 68 * k);
                if (gm < 64) { if (!cnt[gm]) { memcpy(post[gm], body, 64); memcpy(game[gm], body + 68 + 68 * k + 4, 64); } cnt[gm]++; }
            }
        }
        pos += 8 + size;
    }
    free(buf);
    unsigned best = 0;
    for (unsigned g = 1; g < 64; g++) if (cnt[g] > cnt[best]) best = g;
    if (!cnt[best]) return 0;
    nd_mtx_pcomp(o, post[best], (const float(*)[4])game[best]);
    printf("seam step 4: camera game matrix %u (%u draws)\n", best, cnt[best]);
    return 1;
}

static int seam_steps34(int nshape, char** shapes, const char* toad) {
    int fail = 0;
    for (int a = 0; a < nshape; a++) {
        uint32_t size; uint8_t* buf = fix_load(shapes[a], &size);
        const char* nm = strrchr(shapes[a], '/'); nm = nm ? nm + 1 : shapes[a];
        if (!buf) { printf("FAIL: cannot read %s\n", shapes[a]); fail = 1; continue; }
        NdSeamTab t, c0;
        if (nd_seam_build(&t, buf, size)) { printf("FAIL: %s: nd_seam_build no memory\n", nm); fail = 1; continue; }
        printf("seam step 3: %.6s lists id %u int %u other %u, triangles %u, positions %u, boundary edges %u, far %u; T points %u: inserted id %u int %u matrix %u, refused list %u\n",
            nm, t.nl_cls[0], t.nl_cls[1], t.nl_cls[2], t.ntri, t.nvert, t.nbedge, t.far, t.tp_all, t.tp_id, t.tp_int, t.tp_mtx, t.skip_ref);
        if (t.ntri < 100 || t.nlist < 5) { printf("FAIL: %s: too few triangles or lists\n", nm); fail = 1; }
        cbuf = buf; csize = size;
        if (compile_soup(&t, &c0, 0)) { printf("FAIL: %s: compile soup no memory\n", nm); fail = 1; nd_seam_free(&t); continue; }
        printf("seam step 3: %.6s compiled %u lists (refused %u): T points in the compiled output %u\n", nm, c_lists, c_ref, c0.tp_all);
        float o[16]; unsigned nskip, ht0 = 0, h0 = 0;
        int cam = !strncmp(nm, "mac_01", 6) && toad;
        if (cam && !ndref_camera(toad, o)) { printf("FAIL: no kind 5 camera in %s\n", toad); fail = 1; cam = 0; }
        if (cam) { tedges(&t, o); h0 = raster(o, &nskip, &ht0); }
        NdSeamTab c1;
        if (compile_soup(&t, &c1, 1)) { printf("FAIL: %s: compile soup no memory\n", nm); fail = 1; nd_seam_free(&c0); nd_seam_free(&t); continue; }
        printf("seam step 3: %.6s with the T points: inserted %u of %u, T points in the compiled output %u (before %u)\n", nm, c_ins, t.ntp, c1.tp_all, c0.tp_all);
        if (c_ins != t.ntp) { printf("FAIL: %s: the compiler inserted %u of the %u table points\n", nm, c_ins, t.ntp); fail = 1; }
        if (c1.tp_all * 10u > c0.tp_all) { printf("FAIL: %s: %u T points left in the compiled output (more than 10%% of %u)\n", nm, c1.tp_all, c0.tp_all); fail = 1; }
        if (cam) {
            unsigned ht1, h1 = raster(o, &nskip, &ht1);
            printf("seam step 4: mac_01 holes before %u (on T-point edges %u, other %u), after %u (on T-point edges %u, other %u); triangles %u, near or guard skip %u, T edges %u\n",
                h0, ht0, h0 - ht0, h1, ht1, h1 - ht1, nctri, nskip, ntseg);
            if (h1 > h0 || ht1 * 2u > ht0) { printf("FAIL: mac_01 holes on T-point edges did not drop to half\n"); fail = 1; }
        }
        nd_seam_free(&c1);
        nd_seam_free(&c0);
        nd_seam_free(&t);
        free(buf);
    }
    return fail;
}

/* SEAMFIX fixture (no shape file): a receiver under its own matrix (x2 scale, object edge (0,0,0)-(50,0,0), world edge (0,0,0)-(100,0,0)) and an identity
 * list with one vertex at world (40,0,0) on that edge. Expect one T point in the receiver list at object (20,0,0) (fraction 40/100 of the object edge). */
static int seam_fixture_matrix(void) {
    NdSeamTab t; NdSeamSoup* s = nd_seam_soup_new(8); const int32_t z[3] = { 0, 0, 0 };
    int lr, lp; const NdSeamList* L; int fail = 0;
    if (!s) { puts("FAIL: fixture: no memory"); return 1; }
    lr = nd_seam_soup_list(s, 0x1000u, 2, z, 1);
    lp = nd_seam_soup_list(s, 0x2000u, 0, z, 1);
    { const int32_t w[3][3] = { { 0, 0, 0 }, { 100, 0, 0 }, { 0, 50, 0 } }, lc[3][3] = { { 0, 0, 0 }, { 50, 0, 0 }, { 0, 50, 0 } };
      nd_seam_soup_tri(s, lr, 0, w, lc); }
    { const int32_t w[3][3] = { { 40, 0, 0 }, { 40, 30, 10 }, { 60, -20, 10 } }; nd_seam_soup_tri(s, lp, 0, w, w); }
    if (nd_seam_soup_finish(s, &t)) { puts("FAIL: fixture: finish no memory"); return 1; }
    L = nd_seam_find(&t, 0x1000u);
    if (t.tp_all != 1 || t.ntp != 1 || t.tp_mtx != 1 || t.skip_ref != 0) { printf("FAIL: fixture: T points %u inserted %u matrix %u refused %u (want 1 1 1 0)\n", t.tp_all, t.ntp, t.tp_mtx, t.skip_ref); fail = 1; }
    else if (!L || L->cnt != 1 || t.tp[L->first].p[0] != 20 || t.tp[L->first].p[1] != 0 || t.tp[L->first].p[2] != 0 || t.tp[L->first].tri != 0 || t.tp[L->first].edge != 0) {
        printf("FAIL: fixture: receiver point not at object (20,0,0) tri 0 edge 0\n"); fail = 1;
    } else printf("seam fixture: matrix receiver point at object (%d,%d,%d) PASS\n", t.tp[L->first].p[0], t.tp[L->first].p[1], t.tp[L->first].p[2]);
    nd_seam_free(&t);
    return fail;
}

int main(int argc, char** argv) {
    if (argc < 4) { fprintf(stderr, "usage: nd_seam_test <mac_01_shape.bin> <mac_02_shape.bin> <ndref.bin> ... [-- <shape.bin> ...]\n"); return 2; }
    if (seam_fixture_matrix()) return 1;
    int nsh = 0; char** sh = NULL;
    for (int k = 3; k < argc; k++) if (!strcmp(argv[k], "--")) { nsh = argc - k - 1; sh = argv + k + 1; argc = k; break; }
    static Shape a, b;
    static Pos shared_pos[POS_MAX];
    if (!load(argv[1], &a) || !load(argv[2], &b)) { fprintf(stderr, "cannot read the shapes\n"); return 2; }
    walk(&a); walk(&b);
    unsigned na = uniq(a.p, a.n), nb = uniq(b.p, b.n), shared = 0;
    for (unsigned i = 0, j = 0; i < na && j < nb;) {
        int c = cmp_pos(&a.p[i], &b.p[j]);
        if (c == 0) { shared_pos[shared++] = a.p[i]; i++; j++; } else if (c < 0) i++; else j++;
    }
    printf("seam step 1: mac_01 %u loaded, %u distinct; mac_02 %u loaded, %u distinct; shared positions %u\n", a.n, na, b.n, nb, shared);

    unsigned prod = 0, prod_bad = 0, dp_checked = 0, dp_bad = 0, files = 0;
    for (int f = 3; f < argc; f++) {
        FILE* fp = fopen(argv[f], "rb");
        if (!fp) { printf("FAIL: cannot open %s\n", argv[f]); return 1; }
        fseek(fp, 0, SEEK_END); long sz = ftell(fp); fseek(fp, 0, SEEK_SET);
        uint8_t* buf = malloc((size_t)sz);
        if (!buf || fread(buf, 1, (size_t)sz, fp) != (size_t)sz) { printf("FAIL: cannot read %s\n", argv[f]); return 1; }
        fclose(fp);
        if (memcmp(buf, "NDR1", 4) != 0 || le32(buf + 4) != 4) { printf("FAIL: %s is not ndref version 4\n", argv[f]); return 1; }
        files++;
        const uint8_t* pal = NULL; unsigned np1 = 0;
        for (size_t pos = 20, end = 20 + le32(buf + 16); pos < end;) {
            uint32_t kind = le32(buf + pos), size = le32(buf + pos + 4);
            const uint8_t* body = buf + pos + 8;
            if (kind == 1) { np1 = le32(body + 520); pal = body + 524; }
            if (kind == 5) {
                float post[16];
                memcpy(post, body, 64);
                unsigned np = le32(body + 64);
                if (np != np1 || !pal) { printf("FAIL: %s: kind 5 palette count %u, kind 1 count %u\n", argv[f], np, np1); return 1; }
                for (unsigned k = 0; k < np; k++) {
                    const uint8_t* g = body + 68 + 68 * k;
                    const uint8_t* old = pal + 68 * k; /* u32 gm, then pcomp */
                    float game[4][4], o[16], oldm[16];
                    if (le32(g) != le32(old)) { printf("FAIL: %s: game matrix index differs\n", argv[f]); return 1; }
                    memcpy(game, g + 4, 64);
                    memcpy(oldm, old + 4, 64);
                    nd_mtx_pcomp(o, post, game);
                    prod++;
                    if (memcmp(o, oldm, 64) != 0) { prod_bad++; if (prod_bad < 6) printf("FAIL: %s: pcomp of game matrix %u differs from the old path\n", argv[f], le32(g)); }
                    else if ((prod & 7u) == 0) /* every 8th matrix: the shared positions through both matrices */
                        for (unsigned q = 0; q < shared; q++) {
                            float v[4] = { 1.f, (float)shared_pos[q].z, (float)shared_pos[q].y, (float)shared_pos[q].x };
                            for (int r = 0; r < 4; r++) { dp_checked++; if (dp4_fp24(o + 4 * r, v) != dp4_fp24(oldm + 4 * r, v)) dp_bad++; }
                        }
                }
            }
            pos += 8 + size;
        }
        free(buf);
    }
    printf("seam step 2: %u files, %u palette products, %u differ from the old path; %u shared position dp4 checks, %u differ\n", files, prod, prod_bad, dp_checked, dp_bad);
    if (prod == 0 || prod_bad || dp_bad || !dp_checked) { puts("seam step 2: FAIL"); return 1; }
    puts("seam step 2: PASS");
    const char* toad = NULL;
    for (int f = 3; f < argc; f++) if (strstr(argv[f], "ndref_toad")) toad = argv[f];
    if (nsh && seam_steps34(nsh, sh, toad)) { puts("seam steps 3 and 4: FAIL"); return 1; }
    if (nsh) puts("seam steps 3 and 4: PASS");
    return 0;
}
