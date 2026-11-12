/* nd_kccmd.h: the command start decode of a GPU command buffer (FPS60W2, the K copy check in pm_gpu.c kc_build).
 * A command at word i has its header at word i + 1: bits 20-27 hold n extra words, and the next command starts at i + 2 + n + (n & 1)
 * (8-byte padding). Every start is an even word index, so one bit per even word (bit index i >> 1) marks the starts. */
#ifndef ND_KCCMD_H
#define ND_KCCMD_H
#include <stdint.h>

/* Decode the words [0, coff) of cb into the start bitmap bm (nw u32 words; the bits used are (coff / 2) at most). Returns 1 when the
 * decode ends exactly at coff, 0 when a header is missing or a command steps past coff (bm is then not valid). */
static inline int nd_kccmd_decode(const uint32_t* cb, unsigned coff, uint32_t* bm, unsigned nw) {
    unsigned i = 0;
    for (unsigned w = 0; w < nw; w++) bm[w] = 0u;
    while (i < coff) {
        if (i + 1u >= coff) return 0;
        const unsigned n = (cb[i + 1u] >> 20) & 0xFFu;
        const unsigned next = i + 2u + n + (n & 1u);
        if (next > coff) return 0;
        bm[(i >> 1) >> 5] |= 1u << ((i >> 1) & 31u);
        i = next;
    }
    return i == coff;
}

/* 1 when word off of the decoded buffer is a command start (an odd off is never one). */
static inline int nd_kccmd_is_start(const uint32_t* bm, unsigned off) {
    if (off & 1u) return 0;
    const unsigned h = off >> 1;
    return (int)((bm[h >> 5] >> (h & 31u)) & 1u);
}

#endif
