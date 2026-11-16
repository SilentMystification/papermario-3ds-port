/* pm_tex_offpick.h: the off-screen slot that holds an image address (WAVEWIDE). pm_tex.c load_inner and pm_tex_off_rewin use it.
 * The two frame buffers are adjacent (gFrameBuf1 = gFrameBuf0 + 153600 bytes) and a full slot spans 256 rows of 640 bytes, so the slot
 * of gFrameBuf0 also holds the first 16 rows of gFrameBuf1. The slot with the nearest base (the smallest offset) is the image of the address. */
#ifndef PM_TEX_OFFPICK_H
#define PM_TEX_OFFPICK_H
typedef struct { const unsigned char* base; unsigned stride; void* tex; int siz, w, h; } PmOffT;
/* gs: 3 = 32 bit, 2 = 16 bit, 1 = 8 bit. Returns the slot index or -1. *d_out gets the byte offset of img in that slot. */
static inline int pm_offt_pick(const PmOffT* t, int n, const void* img, unsigned gs, unsigned* d_out) {
    const unsigned char* p = (const unsigned char*)img;
    const unsigned bpp = gs == 3 ? 4u : gs;
    int best = -1;
    unsigned bd = 0;
    for (int i = 0; gs && i < n; i++) {
        if (!t[i].base || (unsigned)t[i].siz != gs || p < t[i].base) continue;
        const unsigned d = (unsigned)(p - t[i].base);
        if (d >= t[i].stride * bpp * (unsigned)t[i].h) continue;
        if (best < 0 || d < bd) { best = i; bd = d; }
    }
    *d_out = bd;
    return best;
}
#endif
