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
