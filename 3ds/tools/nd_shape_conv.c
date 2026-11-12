/* HC4f1: host shape converter. Runs in build.sh before the device build (host C, no device code).
 *
 * For every asset table entry whose name ends in "_shape" this decodes the Yay0 data of ver/us/baserom.z64
 * and applies the same fix as pm_map.c (fix_shape and its fix_* helpers, find_asset, yay0_decode_raw), but
 * writes each pointer field as a shape offset (target - shape start) and records the field offset in a reloc
 * list. The device loader adds its shape base to every reloc field, so a null pointer (written as 0) is never
 * in the reloc list.
 *
 * Usage: nd_shape_conv <asset_base> <out_dir>
 * Output: <out_dir>/<asset>.nds =
 *   "NDSH" (4 bytes), version u32 = 2 (HC4f3: each list word is its typed unit, ND_X0/ND_X1 of nd_op.h), size u32, n_reloc u32, reloc[n_reloc] u32 (offsets into data),
 *   data[size] (native little endian; pointer fields hold shape offsets).
 * All header words are little endian. Prints "shapeconv maps N bytes B reloc R" on success.
 * Sidecars: <out_dir>/<asset>.seam (the T-point table) and <out_dir>/<asset>.dlc (S7MAP: the compiled static lists, see write_dlc).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_op.h"
#include "nd.h"

#define SHAPE_BASE 0x80210000u
#define ASSET_TABLE 0x1E40020u
#define ENTRY_SIZE 28u /* name[16], offset, compressedLength, decompressedLength */

typedef struct ShapeFix {
    u8* buf;
    u32 size;
    u8* seen;
    u32* reloc;
    u32 n_reloc;
    u32 cap_reloc;
} ShapeFix;

static u32 dlc_secs, dlc_bytes; /* S7MAP: sections and bytes written to the .dlc files */
static u32 n_nop; /* list words G_NOOP, RDPHALF_1/2, SETZIMG: typed as ND_NOP_A */

static void die(const char* msg, const char* what) {
    fprintf(stderr, "nd_shape_conv: %s%s\n", msg, what ? what : "");
    exit(1);
}

static u32 rbe(const u8* p) {
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
}

/* Native (little endian) word, as the device writes it. */
static void w32(u8* p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)((v >> 8) & 0xff);
    p[2] = (u8)((v >> 16) & 0xff);
    p[3] = (u8)((v >> 24) & 0xff);
}

/* Native (little endian) word read: the pointer fields of s->buf hold shape offsets (w32). */
static u32 le32(const u8* p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void add_reloc(ShapeFix* s, u32 field_off) {
    if (s->n_reloc == s->cap_reloc) {
        u32 cap = s->cap_reloc ? s->cap_reloc * 2 : 4096;
        u32* r = realloc(s->reloc, cap * sizeof(u32));
        if (!r) die("out of memory (reloc list)", NULL);
        s->reloc = r;
        s->cap_reloc = cap;
    }
    s->reloc[s->n_reloc++] = field_off;
}

/* A pointer field: target NULL writes 0 (no reloc); otherwise the shape offset plus a reloc entry. */
static void wptr(ShapeFix* s, u8* field, u8* target) {
    if (!target) {
        w32(field, 0);
        return;
    }
    w32(field, (u32)(target - s->buf));
    add_reloc(s, (u32)(field - s->buf));
}

static u8* in_shape(ShapeFix* s, u32 addr) {
    if (addr < SHAPE_BASE || addr >= SHAPE_BASE + s->size) return NULL;
    return s->buf + (addr - SHAPE_BASE);
}

static int claim(ShapeFix* s, u8* p) {
    u32 i;
    if (!p) return 0;
    i = (u32)(p - s->buf);
    if (i >= s->size || s->seen[i]) return 0;
    s->seen[i] = 1;
    return 1;
}

static void swap_verts(u8* p, int n) {
    int i, k;
    for (i = 0; i < n; i++) {
        for (k = 0; k < 6; k++) {
            u8* h = p + i * 16 + k * 2;
            u8 t = h[0];
            h[0] = h[1];
            h[1] = t;
        }
    }
}

static void swap_mtx(ShapeFix* s, u8* p) {
    int i;
    (void)s;
    for (i = 0; i < 16; i++) {
        u32 v = rbe(p + i * 4);
        w32(p + i * 4, v);
    }
}

static void fix_gfx(ShapeFix* s, u8* dl) {
    int n;
    if (!claim(s, dl)) return;
    for (n = 0; n < 100000; n++) {
        u32 off = (u32)(dl - s->buf);
        u32 w0, w1, op;
        u8* target = NULL;
        if (off + 8 > s->size) return;
        w0 = rbe(dl);
        w1 = rbe(dl + 4);
        op = w0 >> 24;
        if (op == 0x01 || op == 0xDA || op == 0xDE || op == 0xDC) {
            target = in_shape(s, w1);
            if (target) {
                if (op == 0x01) {
                    /* Same as pm_map.c: a G_VTX load swaps each vertex once (seen[] per vertex). */
                    int count = (w0 >> 12) & 0xff;
                    int i;
                    if (count > 64) count = 0;
                    for (i = 0; i < count; i++) {
                        u8* vtx = target + (u32)i * 16u;
                        u32 vi;
                        if ((u32)(vtx - s->buf) + 16u > s->size) break;
                        vi = (u32)(vtx - s->buf);
                        if (s->seen[vi]) continue;
                        s->seen[vi] = 1;
                        swap_verts(vtx, 1);
                    }
                } else if (op == 0xDA) {
                    if (claim(s, target)) swap_mtx(s, target);
                }
            }
        }
        if (ND_OPB(w0) == G_NOOP || ND_OPB(w0) == G_RDPHALF_1 || ND_OPB(w0) == G_RDPHALF_2 || ND_OPB(w0) == G_SETZIMG) n_nop++;
        w32(dl, ND_X0(w0, w1)); /* HC4f3: the typed unit (nd_op.h); op stays the raw op for the walk below */
        if (target) wptr(s, dl + 4, target);
        else w32(dl + 4, ND_X1(w0, w1));
        dl += 8;
        if (op == 0xDF) return;
        if (op == 0xDE && target) fix_gfx(s, target);
    }
}

static void fix_props(ShapeFix* s, u8* props, u32 count) {
    u32 i;
    if (!props || count == 0 || count > 256) return;
    if (!claim(s, props)) return;
    for (i = 0; i < count; i++) {
        u8* e = props + i * 12;
        u32 key, fmt, val;
        u8* p = NULL;
        if ((u32)(e - s->buf) + 12 > s->size) return;
        key = rbe(e);
        fmt = rbe(e + 4);
        val = rbe(e + 8);
        if (key == 0x5E || fmt == 2) p = in_shape(s, val);
        w32(e, key);
        w32(e + 4, fmt);
        if (p) wptr(s, e + 8, p);
        else w32(e + 8, val);
    }
}

static void fix_node(ShapeFix* s, u8* node);

static void fix_group(ShapeFix* s, u8* group) {
    u32 mtx, lights, nlights, nchildren, children, i;
    u8* mp;
    u8* lp;
    u8* list;
    if (!claim(s, group)) return;
    if ((u32)(group - s->buf) + 20 > s->size) return;
    mtx = rbe(group);
    lights = rbe(group + 4);
    nlights = rbe(group + 8);
    nchildren = rbe(group + 12);
    children = rbe(group + 16);
    mp = in_shape(s, mtx);
    if (mp && claim(s, mp)) swap_mtx(s, mp);
    lp = in_shape(s, lights);
    list = in_shape(s, children);
    wptr(s, group, mp);
    wptr(s, group + 4, lp);
    w32(group + 8, nlights);
    w32(group + 12, nchildren);
    wptr(s, group + 16, list);
    if (!list || nchildren > 256) return;
    if (!claim(s, list)) return;
    for (i = 0; i < nchildren; i++) {
        u8* slot = list + i * 4;
        u32 child;
        u8* node;
        if ((u32)(slot - s->buf) + 4 > s->size) return;
        child = rbe(slot);
        node = in_shape(s, child);
        wptr(s, slot, node);
        if (node) fix_node(s, node);
    }
}

static void fix_node(ShapeFix* s, u8* node) {
    u32 type, disp, nprop, props, group;
    u8* display;
    u8* dl;
    u8* pp;
    u8* gp;
    u8* disp_out = NULL;
    if (!claim(s, node)) return;
    if ((u32)(node - s->buf) + 20 > s->size) return;
    type = rbe(node);
    disp = rbe(node + 4);
    nprop = rbe(node + 8);
    props = rbe(node + 12);
    group = rbe(node + 16);
    display = in_shape(s, disp);
    if (display && claim(s, display)) {
        u32 dlp = rbe(display);
        dl = in_shape(s, dlp);
        wptr(s, display, dl);
        if (dl) fix_gfx(s, dl);
        disp_out = display;
    }
    pp = in_shape(s, props);
    fix_props(s, pp, nprop);
    gp = in_shape(s, group);
    if (gp) fix_group(s, gp);
    /* Node words are written last, as in pm_map.c. */
    w32(node, type);
    wptr(s, node + 4, disp_out);
    w32(node + 8, nprop);
    wptr(s, node + 12, pp);
    wptr(s, node + 16, gp);
}

/* True when the bytes at p are the string "db" (with its terminator) and stay inside the buffer. */
static int is_db(ShapeFix* s, u8* p) {
    if ((u32)(p - s->buf) + 3 > s->size) return 0;
    return p[0] == 'd' && p[1] == 'b' && p[2] == 0;
}

static void fix_strlist(ShapeFix* s, u8* list) {
    int n;
    if (!list) return;
    for (n = 0; n < 4096; n++) {
        u32 raw;
        u8* str;
        if ((u32)(list - s->buf) + 4 > s->size) return;
        raw = rbe(list);
        str = in_shape(s, raw);
        wptr(s, list, str);
        list += 4;
        if (str && is_db(s, str)) return;
        if (!str) return;
    }
}

/* pm_map.c fix_shape. s->buf holds the decoded big endian shape; s->size is its length. The caller frees s->reloc. */
static void fix_shape(ShapeFix* s) {
    u8* buf = s->buf;
    u32 root, vtx, models, cols, zones;
    u8* rp;
    u8* mp;
    u8* cp;
    u8* zp;
    u8* vp;
    if (s->size < 20) die("shape smaller than its 20 byte header", NULL);
    root = rbe(buf);
    vtx = rbe(buf + 4);
    models = rbe(buf + 8);
    cols = rbe(buf + 12);
    zones = rbe(buf + 16);
    rp = in_shape(s, root);
    vp = in_shape(s, vtx);
    mp = in_shape(s, models);
    cp = in_shape(s, cols);
    zp = in_shape(s, zones);
    wptr(s, buf, rp);
    wptr(s, buf + 4, vp);
    wptr(s, buf + 8, mp);
    wptr(s, buf + 12, cp);
    wptr(s, buf + 16, zp);
    fix_strlist(s, mp);
    fix_strlist(s, cp);
    fix_strlist(s, zp);
    if (rp) fix_node(s, rp);
}

/* find_asset (pm_map.c): the table is resident ROM, read big endian. Returns 0 when the name is not there. */
static int find_asset(const u8* rom, u32 rom_len, const char* name, u32* rom_off, u32* comp_len, u32* dec_len) {
    u32 bytes, i, n;
    if (ASSET_TABLE + 0x14 > rom_len) return 0;
    bytes = rbe(rom + ASSET_TABLE + 0x10);
    if (bytes < ENTRY_SIZE || bytes > 0x20000 || ASSET_TABLE + bytes > rom_len) return 0;
    n = bytes / ENTRY_SIZE;
    for (i = 0; i < n; i++) {
        const u8* e = rom + ASSET_TABLE + i * ENTRY_SIZE;
        if (strncmp((const char*)e, name, 0x10) == 0 && strlen(name) < 16) {
            *rom_off = ASSET_TABLE + rbe(e + 0x10);
            *comp_len = rbe(e + 0x14);
            *dec_len = rbe(e + 0x18);
            return 1;
        }
    }
    return 0;
}

/* yay0_decode_raw (pm_map.c). Returns 1 on success. */
static int yay0_decode_raw(const u8* src, u32 src_len, u8* dst, u32 dst_len) {
    u32 dec_size, link, count, src_pos, dst_pos, bits, mask;
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
        u32 dist, n, from;
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
            u32 pair;
            if (link + 2 > src_len) return 0;
            pair = ((u32)src[link] << 8) | src[link + 1];
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

/* pm_decode_asset (pm_map.c) for a Yay0 asset: decode, then fix_shape. Output buffer is returned in s. */
static void convert_one(const u8* rom, u32 rom_len, const char* name, ShapeFix* s) {
    u32 rom_off, comp_len, dec_len;
    u8* src;
    if (!find_asset(rom, rom_len, name, &rom_off, &comp_len, &dec_len)) die("asset not in table: ", name);
    if (dec_len < 16 || dec_len > 0x200000 || comp_len > 0x200000) die("asset size out of range: ", name);
    if (rom_off + comp_len > rom_len) die("asset past end of ROM: ", name);
    if (comp_len < 4 || rom[rom_off] != 'Y') die("asset is not Yay0: ", name);
    src = malloc(comp_len);
    s->buf = malloc(dec_len);
    if (!src || !s->buf) die("out of memory (asset)", NULL);
    memcpy(src, rom + rom_off, comp_len);
    if (!yay0_decode_raw(src, comp_len, s->buf, dec_len)) die("Yay0 decode failed: ", name);
    free(src);
    s->size = dec_len;
    s->seen = calloc(dec_len, 1);
    s->reloc = NULL;
    s->n_reloc = 0;
    s->cap_reloc = 0;
    if (!s->seen) die("out of memory (seen map)", NULL);
    fix_shape(s);
}

static u8* read_file(const char* path, u32* len) {
    FILE* f = fopen(path, "rb");
    long n;
    u8* b;
    if (!f) die("cannot open ", path);
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) die("empty file ", path);
    b = malloc((size_t)n);
    if (!b || fread(b, 1, (size_t)n, f) != (size_t)n) die("read failed ", path);
    fclose(f);
    *len = (u32)n;
    return b;
}

/* SEAMHOST: the T-point table of this shape, built here on the host by nd_seam_build (the same code the console ran at each
 * map load). Written as <dir>/<name>.seam: "NDSM", version u32 = 1, shape size u32 (the key against its .nds), r u32 (the
 * nd_seam_build result, 0 ok), nlist u32, ntp u32, the 12 NdSeamTab stats in struct order, list[nlist] (NdSeamList, addr as the
 * offset in the shape file), tp[ntp] (NdTp). Little endian: the struct records are copied as the device lays them out. */
_Static_assert(sizeof(NdSeamList) == 24 && sizeof(NdTp) == 10, "NdSeamList or NdTp layout changed");
/* S7MAP: the compiled static lists of this shape, built here by nd_dlc_compile (the code the console ran at the first draw of each list)
 * with the seam table of write_seam: one result per (list, variant). Variant 1 compiles with the T points of the list, variant 0 without. A list
 * with no points has variant 0 only, a list with points of class 0 or 1 has variant 1 only (the run-time use_tp rule of nd_shape.c dlc_build),
 * a list of class 2 with points has both (stat 0 and stat 1). A list the compiler refuses has no section (the console compiles it as before).
 * Output <dir>/<name>.dlc, little endian:
 *   "NDDC", version u32 = 2, shape size u32 (the key against the .nds), nsec u32,
 *   index[nsec] (in list order, variant): list offset u32 (shape offset), variant u32, file offset u32 of the section, section size u32,
 *   sections. A section: nops, exit_am, exit_om, ngrp, ngop, nvtx, nidx, nins, alloc_nvtx, alloc_nidx, nrun, nlit (u32 each; the alloc fields are the pool
 *   sizes the compile asks for), grp[ngrp] (NDG_GRP bytes each), gop[ngop] (u32 and-mask, u32 or-value), gat[ngop] u16 (padded to 4 bytes),
 *   run[nrun] (u32 shape offset, u32 count: the next count pool vertices are the shape Vtx from that offset with the flag word (bytes 6, 7) 0, or
 *   offset 0xffffffff: the next count pool vertices are literal, taken in order from lit), lit[nlit] (16 bytes: the vertices nd_dlc_compile made itself
 *   or that match no shape vertex; nlit is the sum of the literal run counts), idx[nidx] u16 (vertex numbers relative to the section: the console adds the first pool vertex number of its copy).
 *   A group: lo[3], hi[3] i16, nvtx, ntri, ng u16 at 0, 6, 12; tsv_exit u8 at 18; op0, op1, am, om, vbase, ibase, gbase u32 at 20..44 (vbase is
 *   relative the same way as idx). */
#define NDG_GRP 48u
static unsigned hv_req_v, hv_req_i;
static const u8* hv_base;
static u32 hv_size;
static const void* hv_vtx(uint32_t w1, unsigned n) {
    if (!n || n > 64 || w1 < (u32)(uintptr_t)hv_base || w1 - (u32)(uintptr_t)hv_base + 16u * n > hv_size) return NULL;
    return (const void*)(uintptr_t)w1;
}
static uint8_t* hv_valloc(unsigned n, unsigned* first) { hv_req_v = n; *first = 0; return (uint8_t*)malloc(n * 16u); }
static uint16_t* hv_ialloc(unsigned n) { hv_req_i = n; return (uint16_t*)malloc(n * 2u); }
static void w16(u8* p, u32 v) { p[0] = (u8)v; p[1] = (u8)(v >> 8); }
static u32 put32(u8* b, u32 pos, u32 v) { w32(b + pos, v); return pos + 4; }
typedef struct { u32 list, var, off, size; } DlcIdx;
/* the shape Vtx with the flag word cleared, as the compile copies them: hash of the 16 masked bytes over every 4 byte aligned offset */
static u32 vk_hash(const u8* v) {
    u32 h = 2166136261u, i;
    for (i = 0; i < 16; i++) h = (h ^ (i == 6 || i == 7 ? 0u : v[i])) * 16777619u;
    return h ^ (h >> 15);
}
static int vk_same(const u8* a, const u8* b) { return memcmp(a, b, 6) == 0 && memcmp(a + 8, b + 8, 8) == 0; }
static u32* vk_head;
static u32* vk_next;
#define VK_BITS 17u
static void vk_build(const u8* buf, u32 size) {
    u32 o;
    free(vk_head); free(vk_next);
    vk_head = (u32*)malloc((1u << VK_BITS) * 4u);
    vk_next = (u32*)malloc((size / 4u + 1u) * 4u);
    if (!vk_head || !vk_next) die("out of memory (dlc vtx hash)", NULL);
    memset(vk_head, 0xff, (1u << VK_BITS) * 4u);
    for (o = 0; o + 16u <= size; o += 4u) {
        u32 h = vk_hash(buf + o) & ((1u << VK_BITS) - 1u);
        vk_next[o / 4u] = vk_head[h];
        vk_head[h] = o;
    }
}
/* the longest run of pool vertices v[0..n) that equal consecutive shape Vtx; returns the count (0 = none), *off the shape offset */
static u32 vk_run(const u8* buf, u32 size, const u8* v, u32 n, u32* off) {
    u32 best = 0, c;
    for (c = vk_head[vk_hash(v) & ((1u << VK_BITS) - 1u)]; c != 0xffffffffu; c = vk_next[c / 4u]) {
        u32 len = 0;
        if (!vk_same(buf + c, v)) continue;
        while (len < n && c + 16u * (len + 1u) <= size && !v[16u * len + 6] && !v[16u * len + 7] && vk_same(buf + c + 16u * len, v + 16u * len)) len++;
        if (len > best) { best = len; *off = c; }
    }
    return best;
}
static void write_dlc(const NdSeamTab* t, const u8* buf, u32 size, u32 base, const char* dir, const char* name) {
    static uint32_t words[2 * (ND_DLC_MAXOPS + 1)];
    DlcIdx* ix = (DlcIdx*)malloc((t->nlist * 2u + 1u) * sizeof(DlcIdx));
    u8** secs = (u8**)malloc((t->nlist * 2u + 1u) * sizeof(u8*));
    u32 nsec = 0, k, v, foff, i, j;
    char out[4096];
    FILE* f;
    u8 hdr[16];
    if (!ix || !secs) die("out of memory (dlc index)", NULL);
    hv_base = buf; hv_size = size;
    vk_build(buf, size);
    for (k = 0; k < t->nlist; k++) {
        const NdSeamList* sl = &t->list[k];
        u32 off = sl->addr - base;
        unsigned nops;
        if (off >= size || (off & 7u)) continue;
        for (nops = 0; nops < ND_DLC_MAXOPS && off + 8u * (nops + 1u) <= size; nops++) {
            const u8* p = buf + off + 8u * nops;
            words[2 * nops] = le32(p); words[2 * nops + 1] = le32(p + 4);
            if ((words[2 * nops] >> 24) == NDO_END) break;
        }
        if (nops >= ND_DLC_MAXOPS || off + 8u * (nops + 1u) > size) continue; /* no end word: the console refuses it too */
        for (v = 0; v < 2; v++) {
            NdObj o;
            NdDlcEnv env;
            u8* sec;
            u32 pos, ssz, apad, nrun = 0, nlit = 0, r;
            u32* rn;
            if (v == 1 && !sl->cnt) continue;
            if (v == 0 && sl->cnt && sl->cls < 2) continue;
            env.vtx = hv_vtx; env.valloc = hv_valloc; env.ialloc = hv_ialloc;
            env.tp = v ? t->tp + sl->first : NULL; env.ntp = v ? sl->cnt : 0u;
            hv_req_v = hv_req_i = 0;
            if (nd_dlc_compile(&o, words, &env) != NDR_OK) continue;
            apad = (o.ngop * 2u + 3u) & ~3u;
            rn = (u32*)malloc((o.nvtx * 2u + 2u) * 4u); /* runs: offset, count */
            if (!rn) die("out of memory (dlc runs)", NULL);
            for (r = 0; r < o.nvtx;) {
                u32 so = 0, len = vk_run(buf, size, o.vtx + 16u * r, o.nvtx - r, &so);
                if (len) { rn[2 * nrun] = so; rn[2 * nrun + 1] = len; nrun++; r += len; continue; }
                len = 1;
                while (r + len < o.nvtx) { u32 x; if (vk_run(buf, size, o.vtx + 16u * (r + len), 1, &x)) break; len++; }
                if (nrun && rn[2 * (nrun - 1)] == 0xffffffffu) rn[2 * (nrun - 1) + 1] += len;
                else { rn[2 * nrun] = 0xffffffffu; rn[2 * nrun + 1] = len; nrun++; }
                nlit += len; r += len;
            }
            ssz = 48u + o.ngrp * NDG_GRP + o.ngop * 8u + apad + nrun * 8u + nlit * 16u + o.nidx * 2u;
            sec = (u8*)calloc(1, ssz);
            if (!sec) die("out of memory (dlc section)", NULL);
            pos = 0;
            pos = put32(sec, pos, o.nops); pos = put32(sec, pos, o.exit_am); pos = put32(sec, pos, o.exit_om); pos = put32(sec, pos, o.ngrp);
            pos = put32(sec, pos, o.ngop); pos = put32(sec, pos, o.nvtx); pos = put32(sec, pos, o.nidx); pos = put32(sec, pos, o.nins);
            pos = put32(sec, pos, hv_req_v); pos = put32(sec, pos, hv_req_i);
            pos = put32(sec, pos, nrun); pos = put32(sec, pos, nlit);
            for (i = 0; i < o.ngrp; i++) {
                const NdGrp* g = &o.grp[i];
                u8* q = sec + pos;
                for (j = 0; j < 3; j++) { w16(q + 2 * j, (u32)(uint16_t)g->lo[j]); w16(q + 6 + 2 * j, (u32)(uint16_t)g->hi[j]); }
                w16(q + 12, g->nvtx); w16(q + 14, g->ntri); w16(q + 16, g->ng);
                q[18] = g->tsv_exit;
                w32(q + 20, g->op0); w32(q + 24, g->op1); w32(q + 28, g->am); w32(q + 32, g->om);
                w32(q + 36, g->vbase); w32(q + 40, g->ibase); w32(q + 44, g->gbase);
                pos += NDG_GRP;
            }
            for (i = 0; i < o.ngop; i++) { w32(sec + pos, o.gop[i][0]); w32(sec + pos + 4, o.gop[i][1]); pos += 8; }
            for (i = 0; i < o.ngop; i++) w16(sec + pos + 2 * i, o.gat[i]);
            pos += apad;
            for (r = 0; r < nrun; r++) pos = put32(sec, put32(sec, pos, rn[2 * r]), rn[2 * r + 1]);
            for (r = 0, i = 0; r < nrun; i += rn[2 * r + 1], r++)
                if (rn[2 * r] == 0xffffffffu) { memcpy(sec + pos, o.vtx + 16u * i, rn[2 * r + 1] * 16u); pos += rn[2 * r + 1] * 16u; }
            free(rn);
            for (i = 0; i < o.nidx; i++) w16(sec + pos + 2 * i, o.idx[i]);
            ix[nsec].list = off; ix[nsec].var = v; ix[nsec].size = ssz; ix[nsec].off = 0;
            secs[nsec++] = sec;
            free(o.vtx); free(o.idx);
            o.vtx = NULL; o.idx = NULL;
            nd_dlc_free(&o);
        }
    }
    snprintf(out, sizeof out, "%s/%s.dlc", dir, name);
    f = fopen(out, "wb");
    if (!f) die("cannot write ", out);
    memcpy(hdr, "NDDC", 4);
    w32(hdr + 4, 2); w32(hdr + 8, size); w32(hdr + 12, nsec);
    fwrite(hdr, 1, 16, f);
    foff = 16u + nsec * 16u;
    for (k = 0; k < nsec; k++) { ix[k].off = foff; foff += ix[k].size; }
    for (k = 0; k < nsec; k++) {
        u8 e[16];
        w32(e, ix[k].list); w32(e + 4, ix[k].var); w32(e + 8, ix[k].off); w32(e + 12, ix[k].size);
        fwrite(e, 1, 16, f);
    }
    for (k = 0; k < nsec; k++) { fwrite(secs[k], 1, ix[k].size, f); free(secs[k]); }
    fclose(f);
    dlc_secs += nsec; dlc_bytes += foff;
    free(ix); free(secs);
}
static void write_seam(const ShapeFix* s, const char* dir, const char* name) {
    u8* tmp;
    NdSeamTab t;
    u32 base, k, stat[12];
    u8 hdr[4 + 5 * 4 + 12 * 4];
    int r;
    FILE* f;
    char out[4096];
    tmp = malloc(s->size);
    if (!tmp) die("out of memory (seam copy)", NULL);
    memcpy(tmp, s->buf, s->size);
    base = (u32)(uintptr_t)tmp;
    for (k = 0; k < s->n_reloc; k++) { /* the device loader adds its shape base to every reloc field */
        u8* field = tmp + s->reloc[k];
        w32(field, le32(field) + base);
    }
    memset(&t, 0, sizeof t);
    r = nd_seam_build(&t, tmp, s->size);
    if (r == 0) write_dlc(&t, tmp, s->size, base, dir, name);
    stat[0] = t.tp_all; stat[1] = t.tp_id; stat[2] = t.tp_int; stat[3] = t.tp_mtx; stat[4] = t.skip_ref;
    stat[5] = t.ntri; stat[6] = t.nvert; stat[7] = t.nbedge; stat[8] = t.nl_cls[0]; stat[9] = t.nl_cls[1];
    stat[10] = t.nl_cls[2]; stat[11] = t.far;
    memcpy(hdr, "NDSM", 4);
    w32(hdr + 4, 1);
    w32(hdr + 8, s->size);
    w32(hdr + 12, (u32)r);
    w32(hdr + 16, t.nlist);
    w32(hdr + 20, t.ntp);
    for (k = 0; k < 12; k++) w32(hdr + 24 + k * 4, stat[k]);
    snprintf(out, sizeof out, "%s/%s.seam", dir, name);
    f = fopen(out, "wb");
    if (!f) die("cannot write ", out);
    fwrite(hdr, 1, sizeof hdr, f);
    for (k = 0; k < t.nlist; k++) {
        NdSeamList e = t.list[k];
        e.addr -= base;
        fwrite(&e, sizeof e, 1, f);
    }
    if (t.ntp) fwrite(t.tp, sizeof(NdTp), t.ntp, f);
    fclose(f);
    nd_seam_free(&t);
    free(tmp);
}

static int ends_with_shape(const char* name) {
    size_t n = 0;
    while (n < 16 && name[n]) n++;
    return n > 6 && strncmp(name + n - 6, "_shape", 6) == 0;
}

int main(int argc, char** argv) {
    char path[4096];
    char out[4096];
    u8* rom;
    u32 rom_len, i, n;
    u32 maps = 0, total_bytes = 0, total_reloc = 0;
    u32 bytes;
    if (argc != 3) {
        fprintf(stderr, "usage: nd_shape_conv <asset_base> <out_dir>\n");
        return 2;
    }
    snprintf(path, sizeof path, "%s/ver/us/baserom.z64", argv[1]);
    rom = read_file(path, &rom_len);
    if (ASSET_TABLE + 0x14 > rom_len) die("ROM too small", NULL);
    bytes = rbe(rom + ASSET_TABLE + 0x10);
    if (bytes < ENTRY_SIZE || bytes > 0x20000 || ASSET_TABLE + bytes > rom_len) die("bad asset table size", NULL);
    n = bytes / ENTRY_SIZE;
    for (i = 0; i < n; i++) {
        const u8* e = rom + ASSET_TABLE + i * ENTRY_SIZE;
        char name[17];
        ShapeFix s;
        FILE* f;
        u32 k;
        memcpy(name, e, 16);
        name[16] = 0;
        if (!ends_with_shape(name)) continue;
        convert_one(rom, rom_len, name, &s);
        snprintf(out, sizeof out, "%s/%s.nds", argv[2], name);
        f = fopen(out, "wb");
        if (!f) die("cannot write ", out);
        {
            u8 hdr[16];
            u8* rb = malloc(s.n_reloc * 4 + 1);
            memcpy(hdr, "NDSH", 4);
            w32(hdr + 4, 2);
            w32(hdr + 8, s.size);
            w32(hdr + 12, s.n_reloc);
            fwrite(hdr, 1, 16, f);
            for (k = 0; k < s.n_reloc; k++) w32(rb + k * 4, s.reloc[k]);
            fwrite(rb, 1, s.n_reloc * 4, f);
            fwrite(s.buf, 1, s.size, f);
            free(rb);
        }
        fclose(f);
        write_seam(&s, argv[2], name);
        maps++;
        total_bytes += s.size;
        total_reloc += s.n_reloc;
        free(s.reloc);
        free(s.seen);
        free(s.buf);
    }
    printf("shapeconv maps %u bytes %u reloc %u nop %u dlc sections %u bytes %u\n", (unsigned)maps, (unsigned)total_bytes, (unsigned)total_reloc, (unsigned)n_nop, (unsigned)dlc_secs, (unsigned)dlc_bytes);
    free(rom);
    return 0;
}
