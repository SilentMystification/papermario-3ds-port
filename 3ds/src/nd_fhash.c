/* nd_fhash.c: FNV-1a 64 over PICA command writes, address registers masked (FRAMEHASH1). Pure C: no division, no double, no device header. */
#include <stdint.h>
#include "nd_fhash.h"

uint64_t nd_fh_bytes(uint64_t h, const void* p, uint32_t n) {
    const uint8_t* b = (const uint8_t*)p;
    for (uint32_t i = 0; i < n; i++) {
        h ^= b[i];
        h *= 0x100000001b3ull;
    }
    return h;
}

/* address registers: 0x085-0x08A, 0x095, 0x09D, 0x11C, 0x11D, 0x200, 0x203 + 3 i (i 0..11), 0x23A, 0x23B */
uint32_t nd_fh_keep(unsigned reg, uint32_t v) {
    if (reg == 0x227) return v & 0x80000000u; /* INDEXBUFFER_CONFIG: the index format stays, the offset is masked */
    if (reg >= 0x085 && reg <= 0x08A) return 0;
    if (reg == 0x095 || reg == 0x09D || reg == 0x11C || reg == 0x11D || reg == 0x200) return 0;
    if (reg >= 0x203 && reg <= 0x224 && (reg - 0x203u) % 3u == 0) return 0;
    if (reg == 0x23A || reg == 0x23B) return 0;
    return v;
}

/* one command: [value][header][extra values][pad when the extra count is odd]. Header: bits 0-15 register, 16-19 mask, 20-27 extra count, bit 31 consecutive */
int nd_fh_words(uint64_t* h, const uint32_t* w, uint32_t n) {
    uint32_t i = 0;
    while (i < n) {
        if (i + 2u > n) return 0;
        uint32_t hdr = w[i + 1];
        unsigned reg = hdr & 0xFFFFu;
        unsigned mask = (hdr >> 16) & 0xFu;
        uint32_t extra = (hdr >> 20) & 0xFFu;
        int consec = (hdr >> 31) != 0;
        uint32_t len = 2u + extra + (extra & 1u);
        if (i + len > n) return 0;
        for (uint32_t j = 0; j <= extra; j++) {
            uint32_t v = (j == 0) ? w[i] : w[i + 1 + j];
            unsigned r = (consec && j >= 1) ? reg + j : reg;
            uint32_t k = nd_fh_keep(r, v);
            uint8_t rec[7];
            rec[0] = (uint8_t)(r & 0xFF);
            rec[1] = (uint8_t)(r >> 8);
            rec[2] = (uint8_t)mask;
            rec[3] = (uint8_t)(k & 0xFF);
            rec[4] = (uint8_t)((k >> 8) & 0xFF);
            rec[5] = (uint8_t)((k >> 16) & 0xFF);
            rec[6] = (uint8_t)(k >> 24);
            *h = nd_fh_bytes(*h, rec, 7);
        }
        i += len;
    }
    return 1;
}

/* FRAMEHASH2: nd_fh_frame. The last value of each register below 0x300 (zero at the call start), read by the draws. */
static uint32_t fh_tab[0x300];
static const uint8_t fh_bpt[14] = { 32, 24, 16, 16, 16, 16, 16, 8, 8, 8, 4, 4, 4, 8 }; /* bits per texel, format 0..13 */

static uint64_t fh_u32(uint64_t h, uint32_t v) {
    uint8_t b[4];
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)((v >> 8) & 0xFF);
    b[2] = (uint8_t)((v >> 16) & 0xFF);
    b[3] = (uint8_t)(v >> 24);
    return nd_fh_bytes(h, b, 4);
}

/* n bytes at physical pa; when map gives NULL, 0xDEADBEEF and n instead */
static uint64_t fh_data(uint64_t h, const NdFhEnv* e, uint32_t pa, uint32_t n) {
    const void* p = e->map(pa, n, e->ctx);
    if (p) return nd_fh_bytes(h, p, n);
    h = fh_u32(h, 0xDEADBEEFu);
    return fh_u32(h, n);
}

/* the vertex bytes of a draw: each enabled buffer i over the vertex range lo..hi */
static uint64_t fh_vert(uint64_t h, const NdFhEnv* e, uint32_t lo, uint32_t hi) {
    uint32_t base = fh_tab[0x200] << 3;
    for (unsigned i = 0; i < 12; i++) {
        uint32_t cfg = fh_tab[0x205 + 3 * i];
        if ((cfg >> 28) == 0) continue;
        uint32_t stride = (cfg >> 16) & 0xFFu;
        uint32_t pa = base + fh_tab[0x203 + 3 * i] + lo * stride;
        h = fh_u32(h, 0xD0000000u | i);
        h = fh_data(h, e, pa, (hi + 1u - lo) * stride);
    }
    return h;
}

/* the texel bytes of a draw: each enabled unit u (bits 0-2 of 0x080) */
static uint64_t fh_tex(uint64_t h, const NdFhEnv* e) {
    static const unsigned dimr[3] = { 0x082, 0x092, 0x09A };
    static const unsigned addrr[3] = { 0x085, 0x095, 0x09D };
    static const unsigned typer[3] = { 0x08E, 0x096, 0x09E };
    for (unsigned u = 0; u < 3; u++) {
        if (((fh_tab[0x080] >> u) & 1u) == 0) continue;
        uint32_t w = (fh_tab[dimr[u]] >> 16) & 0x7FFu;
        uint32_t ht = fh_tab[dimr[u]] & 0x7FFu;
        uint32_t fmt = fh_tab[typer[u]] & 0xFu;
        uint32_t bits = fmt < 14u ? fh_bpt[fmt] : 0u;
        uint32_t n = (w * ht * bits) >> 3;
        h = fh_u32(h, 0xD2000000u | u);
        h = fh_u32(h, w);
        h = fh_u32(h, ht);
        h = fh_u32(h, fmt);
        h = fh_data(h, e, fh_tab[addrr[u]] << 3, n);
    }
    return h;
}

/* one draw (reg 0x22E arrays, 0x22F indexed). Its feeds: index bytes, vertex bytes, texel bytes. */
static uint64_t fh_draw(uint64_t h, const NdFhEnv* e, unsigned reg) {
    uint32_t count = fh_tab[0x228];
    uint32_t base = fh_tab[0x200] << 3;
    uint32_t lo = 0, hi = 0;
    int any = 0;
    if (reg == 0x22F) {
        uint32_t cfg = fh_tab[0x227];
        uint32_t size = (cfg >> 31) ? 2u : 1u;
        uint32_t idx = base + (cfg & 0x0FFFFFFFu);
        uint32_t n = count * size;
        const void* p = e->map(idx, n, e->ctx);
        h = fh_u32(h, 0xD1000000u);
        if (p) {
            const uint8_t* b = (const uint8_t*)p;
            for (uint32_t k = 0; k < count; k++) {
                uint32_t v = (size == 2u) ? ((uint32_t)b[2 * k] | ((uint32_t)b[2 * k + 1] << 8)) : (uint32_t)b[k];
                if (k == 0 || v < lo) lo = v;
                if (k == 0 || v > hi) hi = v;
            }
            h = nd_fh_bytes(h, b, n);
            any = count != 0;
        } else {
            h = fh_u32(h, 0xDEADBEEFu);
            h = fh_u32(h, n);
        }
    } else {
        lo = fh_tab[0x22A];
        hi = lo + count - 1u;
        any = count != 0;
    }
    if (any) h = fh_vert(h, e, lo, hi);
    return fh_tex(h, e);
}

int nd_fh_frame(uint64_t* h, const uint32_t* w, uint32_t n, const NdFhEnv* e) {
    uint32_t i = 0;
    for (unsigned r = 0; r < 0x300; r++) fh_tab[r] = 0;
    while (i < n) {
        if (i + 2u > n) return 0;
        uint32_t hdr = w[i + 1];
        unsigned reg = hdr & 0xFFFFu;
        unsigned mask = (hdr >> 16) & 0xFu;
        uint32_t extra = (hdr >> 20) & 0xFFu;
        int consec = (hdr >> 31) != 0;
        uint32_t len = 2u + extra + (extra & 1u);
        if (i + len > n) return 0;
        for (uint32_t j = 0; j <= extra; j++) {
            uint32_t v = (j == 0) ? w[i] : w[i + 1 + j];
            unsigned r = (consec && j >= 1) ? reg + j : reg;
            uint32_t k = nd_fh_keep(r, v);
            uint8_t rec[7];
            rec[0] = (uint8_t)(r & 0xFF);
            rec[1] = (uint8_t)(r >> 8);
            rec[2] = (uint8_t)mask;
            rec[3] = (uint8_t)(k & 0xFF);
            rec[4] = (uint8_t)((k >> 8) & 0xFF);
            rec[5] = (uint8_t)((k >> 16) & 0xFF);
            rec[6] = (uint8_t)(k >> 24);
            *h = nd_fh_bytes(*h, rec, 7);
            if (r < 0x300) fh_tab[r] = v;
            if (r == 0x11C || r == 0x11D) {
                *h = fh_u32(*h, 0xD3000000u | (r & 0xFFu));
                *h = fh_u32(*h, e->role(v << 3, e->ctx));
            } else if (r == 0x22E || r == 0x22F) {
                *h = fh_draw(*h, e, r);
            }
        }
        i += len;
    }
    return 1;
}
