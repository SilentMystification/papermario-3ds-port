#include "common.h"
#include "model.h"
#include "sprite.h"
#include "camera.h"
#include "game_modes.h"
#include "pm_port.h"

int strcmp(const char* a, const char* b);
unsigned int strlen(const char* s);
char* strncpy(char* d, const char* s, unsigned int n);
void bcopy(const void* src, void* dst, unsigned int n);
void bzero(void* p, unsigned int n);

/* Shape files are built for gMapShapeData at this N64 address. */
#define SHAPE_BASE 0x80210000u
#define ASSET_TABLE 0x1E40020u

typedef struct AssetEntry {
    char name[16];
    u32 offset;
    u32 compressedLength;
    u32 decompressedLength;
} AssetEntry;

typedef struct ShapeFix {
    u8* buf;
    u32 size;
    u8* seen;
} ShapeFix;

void* pm_decode_asset(const char* name, unsigned* out_size);
/* Real map shapes are up to 157 KB. heaps.c only has 32 KB, so this is the buffer that the real code uses as gMapShapeData. */
u8 pm_shape_buf[0x40000] __asm__("gMapShapeData") __attribute__((aligned(16)));
/* D02: heaps.c has D_80200000 as 16 KB, but the pause menu (0x38000) and file select (0x20000) use it as the hud element aux cache.
 * On the N64 it is the start of the expansion RAM. Here the next symbol is AuHeapBase (audio heap), so the cache overran it. */
u8 pm_hud_aux_cache[0x38000] __asm__("D_80200000") __attribute__((aligned(0x1000)));

static const void* last_asset_src;
static char last_asset_name[24];
static void swap_hit_file(u8* hit, u32 size);
static u32 rbe(const u8* p) {
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
}

static void w32(u8* p, u32 v) {
    bcopy(&v, p, 4);
}

static void swap_u32s(void* p, int n) {
    u8* b = p;
    int i;
    for (i = 0; i < n; i++) {
        u32 v = rbe(b + i * 4);
        w32(b + i * 4, v);
    }
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
    int i, s;
    for (i = 0; i < n; i++) {
        for (s = 0; s < 6; s++) {
            u8* h = p + i * 16 + s * 2;
            u8 t = h[0];
            h[0] = h[1];
            h[1] = t;
        }
    }
}

static void swap_mtx(u8* p) {
    int i;
    for (i = 0; i < 16; i++) {
        u32 v = rbe(p + i * 4);
        w32(p + i * 4, v);
    }
}

static void fix_gfx(ShapeFix* s, u8* dl);
static void fix_node(ShapeFix* s, u8* node);

static void fix_gfx(ShapeFix* s, u8* dl) {
    int n;
    if (!claim(s, dl)) return;
    for (n = 0; n < 100000; n++) {
        u32 off = (u32)(dl - s->buf);
        u32 w0, w1, op;
        if (off + 8 > s->size) return;
        w0 = rbe(dl);
        w1 = rbe(dl + 4);
        op = w0 >> 24;
        if (op == 0x01 || op == 0xDA || op == 0xDE || op == 0xDC) {
            u8* target = in_shape(s, w1);
            if (target) {
                if (op == 0x01) {
                    /* A later G_VTX often starts inside an earlier load. claim()
                     * only marks the first byte, so that overlap was swapped
                     * twice and those corners landed tens of thousands of units away. */
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
                    if (claim(s, target)) swap_mtx(target);
                }
                w1 = (u32)(unsigned long)target;
            }
        }
        w32(dl, w0);
        w32(dl + 4, w1);
        dl += 8;
        if (op == 0xDF) return;
        if (op == 0xDE) {
            u8* next = (u8*)(unsigned long)w1;
            if (next >= s->buf && next < s->buf + s->size) fix_gfx(s, next);
        }
    }
}

static void fix_props(ShapeFix* s, u8* props, u32 count) {
    u32 i;
    if (!props || count == 0 || count > 256) return;
    if (!claim(s, props)) return;
    for (i = 0; i < count; i++) {
        u8* e = props + i * 12;
        u32 key, fmt, val;
        if ((u32)(e - s->buf) + 12 > s->size) return;
        key = rbe(e);
        fmt = rbe(e + 4);
        val = rbe(e + 8);
        if (key == 0x5E || fmt == 2) {
            u8* p = in_shape(s, val);
            if (p) val = (u32)(unsigned long)p;
        }
        w32(e, key);
        w32(e + 4, fmt);
        w32(e + 8, val);
    }
}

static void fix_group(ShapeFix* s, u8* group) {
    u32 mtx, lights, nlights, nchildren, children, i;
    u8* list;
    if (!claim(s, group)) return;
    if ((u32)(group - s->buf) + 20 > s->size) return;
    mtx = rbe(group);
    lights = rbe(group + 4);
    nlights = rbe(group + 8);
    nchildren = rbe(group + 12);
    children = rbe(group + 16);
    {
        u8* p = in_shape(s, mtx);
        if (p && claim(s, p)) swap_mtx(p);
        mtx = p ? (u32)(unsigned long)p : 0;
    }
    {
        u8* p = in_shape(s, lights);
        lights = p ? (u32)(unsigned long)p : 0;
    }
    list = in_shape(s, children);
    children = list ? (u32)(unsigned long)list : 0;
    w32(group, mtx);
    w32(group + 4, lights);
    w32(group + 8, nlights);
    w32(group + 12, nchildren);
    w32(group + 16, children);
    if (!list || nchildren > 256) return;
    if (!claim(s, list)) return;
    for (i = 0; i < nchildren; i++) {
        u8* slot = list + i * 4;
        u32 child;
        u8* node;
        if ((u32)(slot - s->buf) + 4 > s->size) return;
        child = rbe(slot);
        node = in_shape(s, child);
        w32(slot, node ? (u32)(unsigned long)node : 0);
        if (node) fix_node(s, node);
    }
}

static void fix_node(ShapeFix* s, u8* node) {
    u32 type, disp, nprop, props, group;
    u8* display;
    u8* dl;
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
        w32(display, dl ? (u32)(unsigned long)dl : 0);
        if (dl) fix_gfx(s, dl);
        disp = (u32)(unsigned long)display;
    } else {
        disp = 0;
    }
    {
        u8* p = in_shape(s, props);
        props = p ? (u32)(unsigned long)p : 0;
        fix_props(s, p, nprop);
    }
    {
        u8* p = in_shape(s, group);
        group = p ? (u32)(unsigned long)p : 0;
        if (p) fix_group(s, p);
    }
    w32(node, type);
    w32(node + 4, disp);
    w32(node + 8, nprop);
    w32(node + 12, props);
    w32(node + 16, group);
}

static void fix_strlist(ShapeFix* s, u8* list) {
    int n;
    if (!list) return;
    for (n = 0; n < 4096; n++) {
        u32 raw, host;
        u8* str;
        if ((u32)(list - s->buf) + 4 > s->size) return;
        raw = rbe(list);
        str = in_shape(s, raw);
        host = str ? (u32)(unsigned long)str : 0;
        w32(list, host);
        list += 4;
        if (str && strcmp((char*)str, "db") == 0) return;
        if (!str) return;
    }
}

static ModelNode* fix_shape(u8* buf, u32 size) {
    ShapeFix s;
    u32 root, vtx, models, cols, zones;
    u8* seen = general_heap_malloc(size);
    u32 zi;
    if (!seen) return NULL;
    for (zi = 0; zi < size; zi++) seen[zi] = 0;
    s.buf = buf;
    s.size = size;
    s.seen = seen;
    root = rbe(buf);
    vtx = rbe(buf + 4);
    models = rbe(buf + 8);
    cols = rbe(buf + 12);
    zones = rbe(buf + 16);
    (void)vtx;
    w32(buf, in_shape(&s, root) ? (u32)(unsigned long)in_shape(&s, root) : 0);
    w32(buf + 4, in_shape(&s, vtx) ? (u32)(unsigned long)in_shape(&s, vtx) : 0);
    w32(buf + 8, in_shape(&s, models) ? (u32)(unsigned long)in_shape(&s, models) : 0);
    w32(buf + 12, in_shape(&s, cols) ? (u32)(unsigned long)in_shape(&s, cols) : 0);
    w32(buf + 16, in_shape(&s, zones) ? (u32)(unsigned long)in_shape(&s, zones) : 0);
    fix_strlist(&s, in_shape(&s, models));
    fix_strlist(&s, in_shape(&s, cols));
    fix_strlist(&s, in_shape(&s, zones));
    if (in_shape(&s, root)) fix_node(&s, in_shape(&s, root));
    root = rbe(buf); /* already native */
    {
        u32 native;
        bcopy(buf, &native, 4);
        root = native;
    }
    general_heap_free(seen);
    return (ModelNode*)(unsigned long)root;
}

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

/* PT01: phase and counters around the decoder. */
static int yay0_decode(const u8* src, u32 src_len, u8* dst, u32 dst_len) {
    int r;
    PH_PUSH(PH_YAY0);
    r = yay0_decode_raw(src, src_len, dst, dst_len);
    PH_ADD(C_YAYN, 1);
    PH_ADD(C_YAYB, dst_len >> 10);
    PH_POP();
    return r;
}

/* Retail decode_yay0 core, also used by spr_load_sprite. Header fields are big-endian. */
int pm_yay0_decode(const void* src, u32 src_len, void* dst, u32 dst_len) {
    return yay0_decode(src, src_len, dst, dst_len);
}

extern void pm_tex_invalidate(const void* ptr, unsigned size);

void decode_yay0(void* src, void* dst) {
    u8* s = src;
    u32 dec, link, count, bound;
    if (!src || !dst) return;
    if (s[0] != 'Y' || s[1] != 'a' || s[2] != 'y' || s[3] != '0') return;
    dec = rbe(s + 4);
    link = rbe(s + 8);
    count = rbe(s + 12);
    /* No source length is passed. The streams are valid, so the decoder reads only what it needs. The old bound
     * (the farther table offset) cut the literal bytes at the end and every decode failed. */
    bound = 0x100000u;
    if (dec > 0x40000u || link < 16u || count < 16u) return;
    if (!yay0_decode(s, bound, dst, dec)) return;
    /* The decode overwrote RAM that cached PICA textures were built from (the background raster and palette sit at one
     * fixed address for every map). The cache keys on address and size, so drop what the decode replaced. */
    pm_tex_invalidate(dst, dec);
    /* ROM data is big-endian. The real loaders read these two kinds of file in place, so convert them here, once. */
    if (dst == (void*)pm_shape_buf) {
        fix_shape(dst, dec);
    } else if (src == last_asset_src) {
        u32 n = strlen(last_asset_name);
        if (n > 4 && strcmp(last_asset_name + n - 4, "_hit") == 0) swap_hit_file(dst, dec);
        /* title_data starts with TitleDataStruct (4 big-endian offsets), read in place by state_title_screen.c. Image data stays as is. */
        if (strcmp(last_asset_name, "title_data") == 0) {
            u32* w = dst;
            for (int i = 0; i < 4; i++) w[i] = __builtin_bswap32(w[i]);
        }
    }
    last_asset_src = NULL;
}

static int find_asset(const char* name, u32* rom_off, u32* comp_len, u32* dec_len) {
    AssetEntry first;
    AssetEntry* table;
    u32 bytes, i, n;
    dma_copy((u8*)ASSET_TABLE, (u8*)ASSET_TABLE + sizeof(first), &first);
    bytes = first.offset;
    if (bytes < sizeof(first) || bytes > 0x20000) return 0;
    table = general_heap_malloc(bytes);
    if (!table) return 0;
    dma_copy((u8*)ASSET_TABLE, (u8*)ASSET_TABLE + bytes, table);
    n = bytes / sizeof(AssetEntry);
    for (i = 0; i < n; i++) {
        if (strcmp(table[i].name, name) == 0) {
            *rom_off = ASSET_TABLE + table[i].offset;
            *comp_len = table[i].compressedLength;
            *dec_len = table[i].decompressedLength;
            general_heap_free(table);
            return 1;
        }
    }
    general_heap_free(table);
    return 0;
}

static void swap_u16s(u8* p, int n) {
    int i;
    for (i = 0; i < n; i++) {
        u8 t = p[i * 2];
        p[i * 2] = p[i * 2 + 1];
        p[i * 2 + 1] = t;
    }
}

/* Hit files are big-endian N64 data read in place. Layout follows load_hit_data in collision.c. */
static void swap_hit_section(u8* hit, u32 size, u32 off) {
    u8* h;
    s32 numColliders, numVerts, numBoxes, i;
    u32 collOff, vertOff, boxOff;
    if (!off || off + 0x18 > size) return;
    h = hit + off;
    numColliders = (s16)((h[0] << 8) | h[1]);
    collOff = rbe(h + 4);
    numVerts = (s16)((h[8] << 8) | h[9]);
    vertOff = rbe(h + 0xC);
    numBoxes = (s16)((h[0x10] << 8) | h[0x11]);
    boxOff = rbe(h + 0x14);
    if (collOff + numColliders * 12u > size || vertOff + numVerts * 6u > size || boxOff + numBoxes * 4u > size) return;
    for (i = 0; i < numColliders; i++) {
        u8* c = hit + collOff + i * 12;
        u32 triOff = rbe(c + 8);
        s32 numTris = (s16)((c[6] << 8) | c[7]);
        if (numTris > 0 && triOff + numTris * 4u <= size) swap_u32s(hit + triOff, numTris);
        swap_u16s(c, 4);
        swap_u32s(c + 8, 1);
    }
    swap_u16s(hit + vertOff, numVerts * 3);
    swap_u32s(hit + boxOff, numBoxes);
    swap_u16s(h, 1);
    swap_u32s(h + 4, 1);
    swap_u16s(h + 8, 1);
    swap_u32s(h + 0xC, 1);
    swap_u16s(h + 0x10, 1);
    swap_u32s(h + 0x14, 1);
}

/* The hit file is decoded in place by the real load_map_hit_asset. Swap it once, before the real code reads it. */
static void swap_hit_file(u8* hit, u32 size) {
    u32 collOff = rbe(hit);
    u32 zoneOff = rbe(hit + 4);
    swap_hit_section(hit, size, collOff);
    swap_hit_section(hit, size, zoneOff);
    swap_u32s(hit, 2);
}

/* Records the last asset the real load_asset_by_name returned (linked with --wrap, see CMakeLists.txt). decode_yay0 then
 * knows which kind of big-endian file it just decoded. */
extern void* __real_load_asset_by_name(const char* name, u32* decompressedSize);
void* __wrap_load_asset_by_name(const char* name, u32* decompressedSize) {
    void* ret = __real_load_asset_by_name(name, decompressedSize);
    last_asset_src = ret;
    strncpy(last_asset_name, name, sizeof(last_asset_name) - 1);
    last_asset_name[sizeof(last_asset_name) - 1] = 0;
    return ret;
}

void* pm_decode_asset(const char* name, unsigned* out_size) {
    u32 rom_off, comp_len, dec_len;
    void* compressed;
    u8* decoded;

    if (out_size) *out_size = 0;
    if (!find_asset(name, &rom_off, &comp_len, &dec_len) || dec_len < 16 || dec_len > 0x200000 || comp_len > 0x200000) {
        return NULL;
    }
    compressed = general_heap_malloc(comp_len);
    decoded = general_heap_malloc(dec_len);
    if (!compressed || !decoded) return NULL;
    dma_copy((u8*)rom_off, (u8*)rom_off + comp_len, compressed);
    if (comp_len >= 4 && ((u8*)compressed)[0] == 'Y') {
        if (!yay0_decode(compressed, comp_len, decoded, dec_len)) {
            general_heap_free(compressed);
            general_heap_free(decoded);
            return NULL;
        }
    } else {
        u32 n = comp_len < dec_len ? comp_len : dec_len;
        bcopy(compressed, decoded, n);
    }
    general_heap_free(compressed);
    if (out_size) *out_size = dec_len;
    return decoded;
}

/* Debug switch "scripts": log every live script's position. */
extern ScriptList* gCurrentScriptListPtr;
void pm_dump_scripts(void) {
    s32 i;
    for (i = 0; i < MAX_SCRIPTS; i++) {
        Evt* e = (*gCurrentScriptListPtr)[i];
        if (e && e->id) {
            pm_log("evt %d cur %d fn %p cl %d first %p flags %x blk %d off %d wait %d", e->id, e->curOpcode, e->callFunction, (s32)(e->ptrCurLine - e->ptrFirstLine), e->ptrFirstLine, e->stateFlags, e->blocked,
                (s32)(e->ptrNextLine - e->ptrFirstLine), e->loopDepth);
        }
    }
}

/* Map guard (linked with --wrap, see CMakeLists.txt). gAreas has MAP_ID rows with null settings for maps that are not
 * in the image. The real load_map_by_IDs would read through that null pointer. goto_map ignores the result of
 * get_map_IDs_by_name and then calls set_game_mode(GAME_MODE_CHANGE_MAP), so refuse there and keep the current map. */
extern s32 __real_get_map_IDs_by_name(const char* mapName, s16* areaID, s16* mapID);
extern void __real_load_map_by_IDs(s16 areaID, s16 mapID, s16 loadType);
extern void __real_set_game_mode(s16 mode);
static s16 cur_area = -1, cur_map = -1;

s32 __wrap_get_map_IDs_by_name(const char* mapName, s16* areaID, s16* mapID) {
    s32 ok = __real_get_map_IDs_by_name(mapName, areaID, mapID);
    if (!ok) {
        pm_log("map %s not in gAreas", mapName);
        *areaID = -1;
        *mapID = -1;
    }
    return ok;
}

static int map_is_linked(s16 areaID, s16 mapID) {
    if (areaID < 0 || areaID >= ARRAY_COUNT(gAreas) - 1 || gAreas[areaID].maps == NULL) return 0;
    if (mapID < 0 || mapID >= gAreas[areaID].mapCount) return 0;
    return gAreas[areaID].maps[mapID].settings != NULL;
}

s16 pm_warp_hook(s16 mode);
void __wrap_set_game_mode(s16 mode) {
    mode = pm_warp_hook(mode); /* slice W01 debug switches warp, story, preset */
    if (mode == GAME_MODE_CHANGE_MAP && !map_is_linked(gGameStatusPtr->areaID, gGameStatusPtr->mapID)) {
        pm_log("GotoMap refused: area %d map %d is not linked", gGameStatusPtr->areaID, gGameStatusPtr->mapID);
        gGameStatusPtr->areaID = cur_area;
        gGameStatusPtr->mapID = cur_map;
        return;
    }
    pm_log("game mode %d", mode);
    if (mode == GAME_MODE_WORLD) {
        extern void pm_warp_world_mode(unsigned frame);
        extern unsigned pm_frame_count(void);
        pm_warp_world_mode(pm_frame_count());
    }
    __real_set_game_mode(mode);
}

void __wrap_load_map_by_IDs(s16 areaID, s16 mapID, s16 loadType) {
    if (loadType == LOAD_FROM_MAP && !map_is_linked(areaID, mapID)) {
        pm_log("load refused: area %d map %d is not linked", areaID, mapID);
        return;
    }
    cur_area = areaID;
    cur_map = mapID;
    {
        extern unsigned pm_pad_scans;
        pm_log("load map area %d map %d scan %u", areaID, mapID, pm_pad_scans);
    }
    pm_mem_log("map");
    __real_load_map_by_IDs(areaID, mapID, loadType);
}
