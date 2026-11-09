/* nd_seam.c: the map-wide T-point table (row SEAMFIX). Pure C on the fixed shape file (pm_map.c fix_shape: host pointers, host order Vtx and Mtx words), so the
 * host test builds the same code. Cold: it runs once per shape file, at the first static list compile after pm_dlc_gen changed (nd_shape.c seam_get).
 *
 * The N64 RDP writes a pixel when any of its 8 coverage samples is inside the triangle and the VI filter heals the rest; the PICA tests the pixel centre after a
 * 1/16 pixel snap. A mesh vertex that lies inside a boundary edge of another triangle (a T-junction, or a gap of up to eps units) then shows as pinholes or a
 * background line. The table lists these points per receiver triangle edge; nd_dlc_compile splits the receiver triangle through them.
 *
 * Steps (nd_seam_soup_finish): distinct world positions (hash), the edges sorted by their vertex pair (an edge used once is a boundary edge), the boundary
 * vertices in a 64 unit grid hash, then per boundary edge the grid cells along it (samples 32 units apart) and the int64 point-on-edge test:
 * 0 < (P - A).L < L.L and |(P - A) x L|^2 <= eps^2 L.L. Coordinates beyond 8192 units are skipped (the int64 products stay in range). No double. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"
#include "nd_ops.h"

#define SEAM_MAXL 1024u
#define SEAM_FAR 8192
#define SEAM_NB 4096u /* grid buckets, power of two */

typedef struct { int16_t p[3]; uint8_t bnd; } SVert;
typedef struct { uint16_t v[3]; uint16_t list, ord; int16_t lc[3][3]; } STri; /* lc: the corners in the list's object coordinates (receiver split in its own space) */
typedef struct { uint32_t te; uint16_t v, list; uint16_t ord, edge; uint32_t dot; } SHit;

struct NdSeamSoup {
    unsigned maxtri, ntri, nv, vmax, nl, far;
    STri* tri;
    SVert* v;
    uint16_t* vh; /* vertex hash: vertex number + 1, 0 empty */
    unsigned vhmask;
    NdSeamList* l;
};

static uint32_t rd32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }

NdSeamSoup* nd_seam_soup_new(unsigned maxtri) {
    NdSeamSoup* s = (NdSeamSoup*)calloc(1, sizeof *s);
    unsigned hs = 1024;
    if (!s) return NULL;
    if (maxtri > 21000u) maxtri = 21000u; /* 3 * maxtri vertex numbers fit u16 */
    s->maxtri = maxtri; s->vmax = 3u * maxtri;
    while (hs < 2u * s->vmax) hs <<= 1;
    s->vhmask = hs - 1u;
    s->tri = (STri*)malloc((maxtri ? maxtri : 1u) * sizeof(STri));
    s->v = (SVert*)malloc((s->vmax ? s->vmax : 1u) * sizeof(SVert));
    s->vh = (uint16_t*)calloc(hs, sizeof(uint16_t));
    s->l = (NdSeamList*)calloc(SEAM_MAXL, sizeof(NdSeamList));
    if (!s->tri || !s->v || !s->vh || !s->l) { free(s->tri); free(s->v); free(s->vh); free(s->l); free(s); return NULL; }
    return s;
}

int nd_seam_soup_list(NdSeamSoup* s, uint32_t addr, int cls, const int32_t t[3], int ok) {
    if (s->nl >= SEAM_MAXL) return -1;
    NdSeamList* l = &s->l[s->nl];
    l->addr = addr; l->cls = (uint8_t)cls; l->ok = (uint8_t)ok;
    for (int q = 0; q < 3; q++) l->t[q] = cls == 1 ? t[q] : 0;
    return (int)s->nl++;
}

static int vert_id(NdSeamSoup* s, const int32_t w[3]) {
    uint32_t h = ((uint32_t)w[0] * 73856093u) ^ ((uint32_t)w[1] * 19349663u) ^ ((uint32_t)w[2] * 83492791u);
    for (unsigned i = h & s->vhmask;; i = (i + 1u) & s->vhmask) {
        unsigned k = s->vh[i];
        if (!k) {
            if (s->nv >= s->vmax) return -1;
            SVert* v = &s->v[s->nv];
            v->p[0] = (int16_t)w[0]; v->p[1] = (int16_t)w[1]; v->p[2] = (int16_t)w[2]; v->bnd = 0;
            s->vh[i] = (uint16_t)(++s->nv);
            return (int)s->nv - 1;
        }
        SVert* v = &s->v[k - 1u];
        if (v->p[0] == w[0] && v->p[1] == w[1] && v->p[2] == w[2]) return (int)k - 1;
    }
}

void nd_seam_soup_tri(NdSeamSoup* s, int list, unsigned ord, const int32_t w[3][3], const int32_t lc[3][3]) {
    int id[3];
    if (list < 0 || s->ntri >= s->maxtri) return;
    for (int c = 0; c < 3; c++)
        for (int q = 0; q < 3; q++)
            if (w[c][q] > SEAM_FAR || w[c][q] < -SEAM_FAR) { s->far++; return; }
    for (int c = 0; c < 3; c++) { id[c] = vert_id(s, w[c]); if (id[c] < 0) return; }
    if (id[0] == id[1] || id[1] == id[2] || id[2] == id[0]) return; /* degenerate: no edge, no receiver */
    STri* t = &s->tri[s->ntri++];
    t->v[0] = (uint16_t)id[0]; t->v[1] = (uint16_t)id[1]; t->v[2] = (uint16_t)id[2]; t->list = (uint16_t)list; t->ord = (uint16_t)ord;
    for (int c = 0; c < 3; c++) for (int q = 0; q < 3; q++) t->lc[c][q] = (int16_t)lc[c][q];
}

static int cmp_u64(const void* a, const void* b) { uint64_t x, y; memcpy(&x, a, 8); memcpy(&y, b, 8); return x < y ? -1 : x > y; }
static int cmp_hit(const void* a, const void* b) {
    const SHit* x = (const SHit*)a; const SHit* y = (const SHit*)b;
    if (x->list != y->list) return x->list < y->list ? -1 : 1;
    if (x->ord != y->ord) return x->ord < y->ord ? -1 : 1;
    if (x->edge != y->edge) return x->edge < y->edge ? -1 : 1;
    if (x->dot != y->dot) return x->dot < y->dot ? -1 : 1;
    return 0;
}
static int cmp_list(const void* a, const void* b) {
    uint32_t x = ((const NdSeamList*)a)->addr, y = ((const NdSeamList*)b)->addr;
    return x < y ? -1 : x > y;
}
static unsigned cell_hash(int cx, int cy, int cz) { return (((uint32_t)cx * 73856093u) ^ ((uint32_t)cy * 19349663u) ^ ((uint32_t)cz * 83492791u)) & (SEAM_NB - 1u); }

static void soup_free(NdSeamSoup* s) { free(s->tri); free(s->v); free(s->vh); free(s->l); free(s); }

int nd_seam_soup_finish(NdSeamSoup* s, NdSeamTab* t) {
    unsigned ne = 0, nb = 0, nh = 0, hcap = 0, i;
    uint64_t* e;
    uint16_t* head = NULL; uint16_t* nxt = NULL; uint32_t* stamp = NULL;
    SHit* hit = NULL;
    memset(t, 0, sizeof *t);
    free(s->vh); s->vh = NULL;
    t->ntri = s->ntri; t->nvert = s->nv; t->far = s->far;
    for (i = 0; i < s->nl; i++) t->nl_cls[s->l[i].cls < 3 ? s->l[i].cls : 2]++;
    /* boundary edges: an edge (vertex pair) that one triangle uses */
    e = (uint64_t*)malloc((3u * s->ntri + 1u) * sizeof(uint64_t));
    if (!e) { soup_free(s); return -1; }
    for (i = 0; i < s->ntri; i++)
        for (unsigned c = 0; c < 3; c++) {
            unsigned a = s->tri[i].v[c], b = s->tri[i].v[c == 2 ? 0 : c + 1];
            uint32_t key = a < b ? (a << 16 | b) : (b << 16 | a);
            e[ne++] = (uint64_t)key << 32 | (i << 2 | c);
        }
    qsort(e, ne, sizeof *e, cmp_u64);
    for (i = 0; i < ne;) {
        unsigned j = i + 1;
        while (j < ne && (e[j] >> 32) == (e[i] >> 32)) j++;
        if (j == i + 1) {
            uint32_t te = (uint32_t)e[i], key = (uint32_t)(e[i] >> 32);
            e[nb++] = te;
            s->v[key >> 16].bnd = 1; s->v[key & 0xFFFFu].bnd = 1;
        }
        i = j;
    }
    t->nbedge = nb;
    /* the boundary vertices in the grid */
    head = (uint16_t*)calloc(SEAM_NB, sizeof(uint16_t));
    nxt = (uint16_t*)malloc((s->nv + 1u) * sizeof(uint16_t));
    stamp = (uint32_t*)malloc((s->nv + 1u) * sizeof(uint32_t));
    if (!head || !nxt || !stamp) goto nomem;
    for (i = 0; i < s->nv; i++) {
        stamp[i] = 0xFFFFFFFFu;
        if (!s->v[i].bnd) continue;
        unsigned h = cell_hash(s->v[i].p[0] >> 6, s->v[i].p[1] >> 6, s->v[i].p[2] >> 6);
        nxt[i] = head[h]; head[h] = (uint16_t)(i + 1u);
    }
    /* per boundary edge: the cells along it, the point-on-edge test */
    for (unsigned j = 0; j < nb; j++) {
        uint32_t te = (uint32_t)e[j];
        const STri* tr = &s->tri[te >> 2];
        unsigned c = te & 3u, ia = tr->v[c], ib = tr->v[c == 2 ? 0 : c + 1];
        const int16_t* A = s->v[ia].p; const int16_t* B = s->v[ib].p;
        int32_t L[3] = { B[0] - A[0], B[1] - A[1], B[2] - A[2] };
        int64_t L2 = (int64_t)L[0] * L[0] + (int64_t)L[1] * L[1] + (int64_t)L[2] * L[2];
        int32_t m = 0;
        for (int q = 0; q < 3; q++) { int32_t a = L[q] < 0 ? -L[q] : L[q]; if (a > m) m = a; }
        unsigned ns = ((unsigned)m >> 5) + 1u;
        float inv = 1.0f / (float)ns;
        for (unsigned k = 0; k <= ns; k++) {
            float f = (float)k * inv;
            int32_t S[3], lo[3], hi[3];
            for (int q = 0; q < 3; q++) { float sv = (float)A[q] + (float)L[q] * f; S[q] = (int32_t)(sv < 0 ? sv - 0.5f : sv + 0.5f); lo[q] = (S[q] - 19) >> 6; hi[q] = (S[q] + 19) >> 6; }
            for (int cx = lo[0]; cx <= hi[0]; cx++)
                for (int cy = lo[1]; cy <= hi[1]; cy++)
                    for (int cz = lo[2]; cz <= hi[2]; cz++)
                        for (unsigned vn = head[cell_hash(cx, cy, cz)]; vn; vn = nxt[vn - 1u]) {
                            unsigned v = vn - 1u;
                            const int16_t* P = s->v[v].p;
                            if ((P[0] >> 6) != cx || (P[1] >> 6) != cy || (P[2] >> 6) != cz) continue;
                            if (stamp[v] == j) continue;
                            stamp[v] = j;
                            if (v == tr->v[0] || v == tr->v[1] || v == tr->v[2]) continue;
                            int64_t d0 = P[0] - A[0], d1 = P[1] - A[1], d2 = P[2] - A[2];
                            int64_t dot = d0 * L[0] + d1 * L[1] + d2 * L[2];
                            if (dot <= 0 || dot >= L2) continue;
                            int64_t c0 = d1 * L[2] - d2 * L[1], c1 = d2 * L[0] - d0 * L[2], c2 = d0 * L[1] - d1 * L[0];
                            if (4 * (c0 * c0 + c1 * c1 + c2 * c2) > (int64_t)(ND_SEAM_EPS2X * ND_SEAM_EPS2X) * L2) continue;
                            if (nh >= hcap) {
                                unsigned nc = hcap ? 2u * hcap : 1024u;
                                SHit* nhit = (SHit*)realloc(hit, nc * sizeof(SHit));
                                if (!nhit) goto nomem;
                                hit = nhit; hcap = nc;
                            }
                            SHit* h = &hit[nh++];
                            h->te = te; h->v = (uint16_t)v; h->list = tr->list; h->ord = tr->ord; h->edge = (uint16_t)c; h->dot = (uint32_t)dot;
                        }
        }
    }
    free(e); e = NULL; free(head); head = NULL; free(nxt); nxt = NULL; free(stamp); stamp = NULL;
    /* keep the points that the compiler can insert */
    t->tp_all = nh;
    {
        unsigned nk = 0;
        for (i = 0; i < nh; i++) {
            const NdSeamList* l = &s->l[hit[i].list];
            if (!l->ok) { t->skip_ref++; continue; }
            if (l->cls == 0) t->tp_id++; else if (l->cls == 1) t->tp_int++; else t->tp_mtx++;
            hit[nk++] = hit[i];
        }
        nh = nk;
    }
    qsort(hit, nh, sizeof *hit, cmp_hit);
    t->tp = nh ? (NdTp*)malloc(nh * sizeof(NdTp)) : NULL;
    t->list = (NdSeamList*)malloc((s->nl ? s->nl : 1u) * sizeof(NdSeamList));
    if ((nh && !t->tp) || !t->list) goto nomem;
    for (i = 0; i < s->nl; i++) { s->l[i].first = 0; s->l[i].cnt = 0; }
    for (i = 0; i < nh; i++) {
        NdSeamList* l = &s->l[hit[i].list];
        NdTp* p = &t->tp[i];
        if (!l->cnt) l->first = i;
        if (l->cnt < 0xFFFFu) l->cnt++;
        if (l->cls < 2) for (int q = 0; q < 3; q++) p->p[q] = (int16_t)(s->v[hit[i].v].p[q] - l->t[q]);
        else { /* receiver under its own matrix: the affine map keeps the edge parameter, so the point is the same fraction of the local edge */
            const STri* tr = &s->tri[hit[i].te >> 2];
            unsigned c = hit[i].te & 3u, cn = c == 2 ? 0u : c + 1u;
            const int16_t* A = s->v[tr->v[c]].p; const int16_t* B = s->v[tr->v[cn]].p;
            int64_t L2 = 0;
            for (int q = 0; q < 3; q++) { int64_t d = B[q] - A[q]; L2 += d * d; }
            float f = L2 ? (float)hit[i].dot / (float)L2 : 0.f;
            for (int q = 0; q < 3; q++) {
                float lp = (float)tr->lc[c][q] + f * ((float)tr->lc[cn][q] - (float)tr->lc[c][q]);
                p->p[q] = (int16_t)(int32_t)(lp < 0 ? lp - 0.5f : lp + 0.5f);
            }
        }
        p->tri = hit[i].ord; p->edge = hit[i].edge;
    }
    t->ntp = nh;
    memcpy(t->list, s->l, s->nl * sizeof(NdSeamList));
    t->nlist = s->nl;
    qsort(t->list, t->nlist, sizeof(NdSeamList), cmp_list);
    free(hit);
    soup_free(s);
    return 0;
nomem:
    free(e); free(head); free(nxt); free(stamp); free(hit);
    free(t->tp); free(t->list); t->tp = NULL; t->list = NULL; t->ntp = 0; t->nlist = 0;
    soup_free(s);
    return -1;
}

void nd_seam_free(NdSeamTab* t) { free(t->tp); free(t->list); memset(t, 0, sizeof *t); }

const NdSeamList* nd_seam_find(const NdSeamTab* t, uint32_t addr) {
    unsigned lo = 0, hi = t->nlist;
    while (lo < hi) {
        unsigned mid = (lo + hi) >> 1;
        if (t->list[mid].addr < addr) lo = mid + 1u; else hi = mid;
    }
    return lo < t->nlist && t->list[lo].addr == addr ? &t->list[lo] : NULL;
}

/* ---- the shape tree walk ---- */
typedef struct { int cls; int32_t t[3]; float f[4][4]; } SXf;
typedef struct { uint32_t addr, fh; int32_t t[3]; int16_t sid; uint8_t cls, pad; } SSeen; /* a decoded list: its transform (class, translation, hash of the float form) */
typedef struct { const uint8_t* buf; uint32_t base, size; NdSeamSoup* s; unsigned ntri, nlist; SSeen seen[SEAM_MAXL]; } SWalk;

static const uint8_t* at(const SWalk* w, uint32_t a, uint32_t n) {
    if (a < w->base || a - w->base > w->size || n > w->size - (a - w->base)) return NULL;
    return w->buf + (a - w->base);
}

/* the fixed point Mtx (16.16; words 0-7 integer halves, 8-15 fraction halves, row-vector 4 x 4): its class and float form (guMtxL2F) */
static void mtx_read(const uint8_t* m, SXf* x) {
    int32_t e[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 2; j++) {
            uint32_t ip = rd32(m + 4 * (2 * i + j)), fp = rd32(m + 32 + 4 * (2 * i + j));
            e[i][2 * j] = (int32_t)((ip & 0xFFFF0000u) | (fp >> 16));
            e[i][2 * j + 1] = (int32_t)((ip << 16) | (fp & 0xFFFFu));
        }
    int id = 1;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 4; j++) if (e[i][j] != (i == j ? 65536 : 0)) id = 0;
    if (e[3][3] != 65536) id = 0;
    for (int j = 0; j < 3; j++) if (e[3][j] & 0xFFFF) id = 0;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) x->f[i][j] = (float)e[i][j] * (1.0f / 65536.0f);
    x->t[0] = x->t[1] = x->t[2] = 0;
    if (!id) { x->cls = 2; return; }
    for (int j = 0; j < 3; j++) x->t[j] = e[3][j] >> 16;
    x->cls = (x->t[0] | x->t[1] | x->t[2]) ? 1 : 0;
}

/* child x parent (row vectors: the child matrix applies first), as guMtxCatF */
static void xf_cat(const SXf* c, const SXf* p, SXf* o) {
    SXf r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) r.f[i][j] = c->f[i][0] * p->f[0][j] + c->f[i][1] * p->f[1][j] + c->f[i][2] * p->f[2][j] + c->f[i][3] * p->f[3][j];
    if (c->cls < 2 && p->cls < 2) {
        for (int j = 0; j < 3; j++) r.t[j] = c->t[j] + p->t[j];
        r.cls = (r.t[0] | r.t[1] | r.t[2]) ? 1 : 0;
    } else { r.cls = 2; r.t[0] = r.t[1] = r.t[2] = 0; }
    *o = r;
}

static uint32_t xf_hash(const SXf* x) {
    uint32_t h = 2166136261u, u;
    for (int i = 0; i < 16; i++) { memcpy(&u, &x->f[i >> 2][i & 3], 4); h = (h ^ u) * 16777619u; }
    return h;
}
static int xf_same(const SSeen* a, const SXf* b) { return a->cls == b->cls && (b->cls == 2 ? a->fh == xf_hash(b) : !memcmp(a->t, b->t, sizeof a->t)); }

static void list_decode(SWalk* w, uint32_t addr, const SXf* x, int depth);

/* a list reached from a model node or a G_DL: decoded once; a second transform that differs makes it another-matrix list */
static void list_add(SWalk* w, uint32_t addr, const SXf* x, int depth) {
    unsigned i;
    for (i = 0; i < w->nlist; i++) if (w->seen[i].addr == addr) break;
    if (i < w->nlist) {
        if (!xf_same(&w->seen[i], x) && w->s && w->seen[i].sid >= 0) w->s->l[w->seen[i].sid].cls = 2;
        return;
    }
    if (w->nlist >= SEAM_MAXL) return;
    SSeen* e = &w->seen[w->nlist++];
    e->addr = addr; e->cls = (uint8_t)x->cls; memcpy(e->t, x->t, sizeof e->t); e->fh = xf_hash(x); e->sid = -1;
    list_decode(w, addr, x, depth);
}

static void list_decode(SWalk* w, uint32_t addr, const SXf* x, int depth) {
    int32_t pos[64][3];
    int16_t lraw[64][3]; /* the Vtx words: object coordinates of each slot */
    unsigned char live[64];
    unsigned ord = 0, i, me = w->nlist - 1u;
    int ok = 1, li = -1;
    if (depth > 8 || (addr & 7u)) return;
    memset(live, 0, sizeof live);
    if (w->s) { li = nd_seam_soup_list(w->s, addr, x->cls, x->t, 1); w->seen[me].sid = (int16_t)li; }
    for (i = 0; i < ND_DLC_MAXOPS; i++) {
        const uint8_t* p = at(w, addr + 8u * i, 8);
        if (!p) { ok = 0; break; }
        uint32_t w0 = rd32(p), w1 = rd32(p + 4);
        unsigned op = w0 >> 24;
        if (op == OP_ENDDL) break;
        switch (op) {
        case OP_VTX: {
            unsigned n = (w0 >> 12) & 0xFF, v0n = (w0 >> 1) & 0x7F, v0 = v0n >= n ? v0n - n : 0;
            const uint8_t* src = n ? at(w, w1, 16u * n) : NULL;
            if (!src || n > 64 || v0 >= 64) { ok = 0; goto done; }
            if (v0 + n > 64) n = 64 - v0;
            for (unsigned k = 0; k < n; k++) {
                int16_t ob[3];
                memcpy(ob, src + 16u * k, 6);
                for (int q = 0; q < 3; q++) lraw[v0 + k][q] = ob[q];
                if (x->cls < 2) for (int q = 0; q < 3; q++) pos[v0 + k][q] = ob[q] + x->t[q];
                else
                    for (int q = 0; q < 3; q++) {
                        float f = (float)ob[0] * x->f[0][q] + (float)ob[1] * x->f[1][q] + (float)ob[2] * x->f[2][q] + x->f[3][q];
                        pos[v0 + k][q] = (int32_t)(f < 0 ? f - 0.5f : f + 0.5f);
                    }
                live[v0 + k] = 1;
            }
            break;
        }
        case OP_TRI1: case OP_TRI2:
            for (int t = 0; t < (op == OP_TRI2 ? 2 : 1); t++) {
                uint32_t ww = t ? w1 : w0;
                unsigned ix[3] = { ((ww >> 16) & 0xFF) / 2, ((ww >> 8) & 0xFF) / 2, (ww & 0xFF) / 2 };
                unsigned o = ord++;
                if (ix[0] > 63 || ix[1] > 63 || ix[2] > 63 || !live[ix[0]] || !live[ix[1]] || !live[ix[2]]) { ok = 0; continue; }
                w->ntri++;
                if (w->s) {
                    int32_t tw[3][3], tl[3][3];
                    for (int c = 0; c < 3; c++) for (int q = 0; q < 3; q++) { tw[c][q] = pos[ix[c]][q]; tl[c][q] = lraw[ix[c]][q]; }
                    nd_seam_soup_tri(w->s, li, o, (const int32_t(*)[3])tw, (const int32_t(*)[3])tl);
                }
            }
            break;
        case OP_DL:
            ok = 0; /* nd_dlc_compile refuses OP_DL: the list runs on the interpreter, its sub list compiles on its own */
            list_add(w, w1, x, depth + 1);
            if ((w0 >> 16) & 1u) goto done; /* branch: no return */
            break;
        case OP_GEOMETRYMODE: case OP_NOOP: case OP_FULLSYNC: case OP_TILESYNC: case OP_PIPESYNC: case OP_LOADSYNC:
            break;
        default:
            ok = 0;
            break;
        }
    }
    if (i >= ND_DLC_MAXOPS) ok = 0;
done:
    if (w->s && li >= 0 && !ok) w->s->l[li].ok = 0;
}

static void node_walk(SWalk* w, uint32_t node, const SXf* x, int depth) {
    const uint8_t* n = at(w, node, 20);
    if (!n || depth > 32) return;
    uint32_t disp = rd32(n + 4), group = rd32(n + 16), nch = 0, ch = 0;
    const uint8_t* g = group ? at(w, group, 20) : NULL;
    if (g) { nch = rd32(g + 12); ch = rd32(g + 16); }
    if (g && nch && ch && nch <= 256u) {
        SXf cx = *x;
        uint32_t m = rd32(g);
        const uint8_t* mp = m ? at(w, m, 64) : NULL;
        if (mp) { SXf mx; mtx_read(mp, &mx); xf_cat(&mx, x, &cx); }
        for (unsigned i = 0; i < nch; i++) {
            const uint8_t* cp = at(w, ch + 4u * i, 4);
            if (cp) node_walk(w, rd32(cp), &cx, depth + 1);
        }
        return; /* a node with children draws its children (load_model_transforms) */
    }
    const uint8_t* d = disp ? at(w, disp, 4) : NULL;
    if (d && rd32(d)) list_add(w, rd32(d), x, 0);
}

int nd_seam_build(NdSeamTab* t, const uint8_t* buf, unsigned size) {
    SWalk* w = (SWalk*)calloc(1, sizeof *w);
    SXf id;
    memset(t, 0, sizeof *t);
    if (!w) return -1;
    memset(&id, 0, sizeof id);
    for (int i = 0; i < 4; i++) id.f[i][i] = 1.0f;
    w->buf = buf; w->base = (uint32_t)(uintptr_t)buf; w->size = size;
    uint32_t root = size >= 4 ? rd32(buf) : 0;
    node_walk(w, root, &id, 0); /* count the triangles */
    w->s = nd_seam_soup_new(w->ntri);
    if (!w->s) { free(w); return -1; }
    w->nlist = 0; w->ntri = 0;
    node_walk(w, root, &id, 0);
    int r = nd_seam_soup_finish(w->s, t);
    free(w);
    return r;
}
