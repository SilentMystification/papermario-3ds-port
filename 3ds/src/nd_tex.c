/* nd_tex.c: native texture descriptors and unit words (plan_native_slices S1-6).
 * A descriptor names a texture the way the gDPLoadTexture* macros do (raster, palette, format, size, width, height, tile wrap and mask). nd_tex_get returns a block id.
 * The block is the 4 words C3Di_SetTex sends for the unit: dim, param, lodParam, data address >> 3 (pm_tex.c pm_tex_slot_words). The wrap rules and the
 * parameter bit math are moved from pm_tex.c (wrap_of, the final wrap of pm_tex_bind) and from citro3d (C3D_TexSetFilter, C3D_TexSetWrap).
 * The texture cache stays pm_tex.c: a miss loads the image there, then the words are read back. */
#include <stdint.h>
#include <string.h>
#include "nd.h"

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
int pm_tex_slot_unit(int i, NdUnit* u);
int pm_tex_slot_find(unsigned key, unsigned fmt, unsigned siz, uint32_t dim, NdUnit* u); /* pm_tex.c, S3N-3c */
int pm_tex_slot_words(int i, uint32_t* out);
void pm_tex_unit0_param(uint32_t* ws, uint32_t* wt, unsigned* filter);

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
int nd_tex_unit(int id, NdUnit* u) {
    NdTexEnt* e = &tex_ent[id];
    NdUnit t;
    if (!e->key) return 0;
    int s = pm_tex_slot_find(e->key, e->d.fmt, e->d.siz, e->w[0], &t);
    if (s < 0) {
        const NdTexDesc* d = &e->d;
        pm_tex_set_wrap(d->cms, d->cmt, d->masks, d->maskt);
        if (!pm_tex_load(d->img, d->fmt, d->siz, d->w, d->h, d->pal, d->pal_n, d->stride)) return 0;
        s = pm_tex_slot_find(e->key, e->d.fmt, e->d.siz, e->w[0], &t);
        if (s < 0) return 0;
    }
    u->dim = e->w[0]; u->param = e->w[1]; u->lod = t.lod; u->addr = t.addr; u->type = t.type; u->force = 0;
    return 1;
}
