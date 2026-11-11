/* YAY0FAST: host equality test of the Yay0 decoders. For every Yay0 entry of the ROM asset table (the find and iterate code
 * of nd_shape_conv.c), decode with the former byte loop (yay0_old, copied from nd_shape_conv.c) and with nd_yay0_raw
 * (3ds/src/nd_yay0.c); both must return 1 and give the same bytes.
 *
 * Usage: yay0_test <asset_base>
 * Output: "yay0_test: assets N equal N" (N = Yay0 entries). Exit 1 on any difference.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "nd_yay0.h"

#define ASSET_TABLE 0x1E40020u
#define ENTRY_SIZE 28u /* name[16], offset, compressedLength, decompressedLength */

static uint32_t rbe(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

/* yay0_decode_raw as it was in pm_map.c and nd_shape_conv.c. Returns 1 on success. */
static int yay0_old(const uint8_t* src, uint32_t src_len, uint8_t* dst, uint32_t dst_len) {
    uint32_t dec_size, link, count, src_pos, dst_pos, bits, mask;
    if (src_len < 16 || src[0] != 'Y' || src[1] != 'a' || src[2] != 'y' || src[3] != '0') return 0;
    dec_size = rbe(src + 4);
    link = rbe(src + 8);
    count = rbe(src + 12);
    if (dec_size != dst_len || link >= src_len || count >= src_len) return 0;
    src_pos = 16;
    dst_pos = 0;
    bits = 0;
    mask = 0;
    while (dst_pos < dec_size) {
        uint32_t dist, n, from;
        if (mask == 0) {
            if (src_pos + 4 > src_len) return 0;
            bits = rbe(src + src_pos);
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
            from = dst_pos - dist;
            while (n--) {
                if (dst_pos >= dec_size || from == 0) return 0;
                dst[dst_pos++] = dst[from - 1];
                from++;
            }
        }
        mask >>= 1;
    }
    return 1;
}

static uint8_t* read_rom(const char* path, long* len) {
    FILE* f = fopen(path, "rb");
    uint8_t* b;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    fseek(f, 0, SEEK_SET);
    b = malloc((size_t)*len);
    if (!b || fread(b, 1, (size_t)*len, f) != (size_t)*len) { fclose(f); free(b); return NULL; }
    fclose(f);
    return b;
}

int main(int argc, char** argv) {
    char path[4096];
    uint8_t* rom;
    long rom_len;
    uint32_t bytes, i, n, assets = 0, equal = 0;
    if (argc != 2) {
        fprintf(stderr, "usage: yay0_test <asset_base>\n");
        return 2;
    }
    snprintf(path, sizeof path, "%s/ver/us/baserom.z64", argv[1]);
    rom = read_rom(path, &rom_len);
    if (!rom) { fprintf(stderr, "yay0_test: cannot read %s\n", path); return 2; }
    if (ASSET_TABLE + 0x14 > (uint32_t)rom_len) { fprintf(stderr, "yay0_test: ROM too small\n"); return 2; }
    bytes = rbe(rom + ASSET_TABLE + 0x10);
    if (bytes < ENTRY_SIZE || bytes > 0x20000 || ASSET_TABLE + bytes > (uint32_t)rom_len) { fprintf(stderr, "yay0_test: bad asset table size\n"); return 2; }
    n = bytes / ENTRY_SIZE;
    for (i = 0; i < n; i++) {
        const uint8_t* e = rom + ASSET_TABLE + i * ENTRY_SIZE;
        uint32_t rom_off = ASSET_TABLE + rbe(e + 0x10);
        uint32_t comp_len = rbe(e + 0x14);
        uint32_t dec_len = rbe(e + 0x18);
        uint8_t* src;
        uint8_t* a;
        uint8_t* b;
        int ra, rb;
        if (comp_len < 4 || rom_off > (uint32_t)rom_len || comp_len > (uint32_t)rom_len - rom_off) continue;
        if (rom[rom_off] != 'Y' || rom[rom_off + 1] != 'a' || rom[rom_off + 2] != 'y' || rom[rom_off + 3] != '0') continue;
        if (dec_len < 16 || dec_len > 0x200000) continue;
        assets++;
        src = malloc(comp_len);
        a = malloc(dec_len);
        b = malloc(dec_len);
        if (!src || !a || !b) { fprintf(stderr, "yay0_test: out of memory\n"); return 2; }
        memcpy(src, rom + rom_off, comp_len);
        memset(a, 0, dec_len);
        memset(b, 0, dec_len);
        ra = yay0_old(src, comp_len, a, dec_len);
        rb = nd_yay0_raw(src, comp_len, b, dec_len);
        if (ra == 1 && rb == 1 && memcmp(a, b, dec_len) == 0) {
            equal++;
        } else {
            fprintf(stderr, "yay0_test: differs at entry %u (old %d new %d)\n", (unsigned)i, ra, rb);
        }
        free(src);
        free(a);
        free(b);
    }
    printf("yay0_test: assets %u equal %u\n", (unsigned)assets, (unsigned)equal);
    free(rom);
    return equal == assets && assets > 0 ? 0 : 1;
}
