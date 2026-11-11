/* YAY0FAST: Yay0 decode. The back reference copies a run at once when the source and the destination do not overlap; an
 * overlapping run repeats its pattern byte by byte. Output and failure cases match the former byte loop (pm_map.c). */
#include <stdint.h>
#include <string.h>
#include "nd_yay0.h"

static uint32_t yay0_rbe(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

int nd_yay0_raw(const uint8_t* src, uint32_t src_len, uint8_t* dst, uint32_t dst_len) {
    uint32_t dec_size, link, count, src_pos, dst_pos, bits, mask;
    if (src_len < 16 || src[0] != 'Y' || src[1] != 'a' || src[2] != 'y' || src[3] != '0') return 0;
    dec_size = yay0_rbe(src + 4);
    link = yay0_rbe(src + 8);
    count = yay0_rbe(src + 12);
    if (dec_size != dst_len || link >= src_len || count >= src_len) return 0;
    src_pos = 16;
    dst_pos = 0;
    bits = 0;
    mask = 0;
    while (dst_pos < dec_size) {
        uint32_t dist, n;
        if (mask == 0) {
            if (src_pos + 4 > src_len) return 0;
            bits = yay0_rbe(src + src_pos);
            src_pos += 4;
            mask = 0x80000000u;
        }
        if (bits & mask) {
            if (count >= src_len) return 0;
            dst[dst_pos++] = src[count++];
        } else {
            uint32_t pair;
            if (link + 2 > src_len) return 0;
            pair = ((uint32_t)src[link] << 8) | src[link + 1];
            link += 2;
            dist = pair & 0xFFFu;
            n = pair >> 12;
            if (n == 0) {
                if (count >= src_len) return 0;
                n = src[count++] + 0x12;
            } else {
                n += 2;
            }
            if (dist > dst_pos) return 0;
            if (dist == dst_pos) return 0;
            if (n > dec_size - dst_pos) return 0;
            {
                uint8_t* d = dst + dst_pos;
                const uint8_t* s = d - dist - 1;
                if (dist + 1 >= n) {
                    memcpy(d, s, n);
                } else {
                    uint32_t k;
                    for (k = 0; k < n; k++) d[k] = s[k];
                }
                dst_pos += n;
            }
        }
        mask >>= 1;
    }
    return 1;
}
