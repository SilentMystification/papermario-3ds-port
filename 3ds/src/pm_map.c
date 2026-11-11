#include <string.h>
#include <stdio.h>
#include "common.h"
#include "model.h"
#include "sprite.h"
#include "camera.h"
#include "game_modes.h"
#include "pm_port.h"
#include "battle/battle.h"

void partner_enable_input(void);
int strcmp(const char* a, const char* b);
unsigned int strlen(const char* s);
char* strncpy(char* d, const char* s, unsigned int n);
void bcopy(const void* src, void* dst, unsigned int n);
void bzero(void* p, unsigned int n);

#define ASSET_TABLE 0x1E40020u

typedef struct AssetEntry {
    char name[16];
    u32 offset;
    u32 compressedLength;
    u32 decompressedLength;
} AssetEntry;

void* pm_decode_asset(const char* name, unsigned* out_size);
/* Real map shapes are up to 157 KB. heaps.c only has 32 KB, so this is the buffer that the real code uses as gMapShapeData. */
u8 pm_shape_buf[0x40000] __asm__("gMapShapeData") __attribute__((aligned(16)));
/* D02: heaps.c has D_80200000 as 16 KB, but the pause menu (0x38000) and file select (0x20000) use it as the hud element aux cache.
 * On the N64 it is the start of the expansion RAM. Here the next symbol is AuHeapBase (audio heap), so the cache overran it. */
u8 pm_hud_aux_cache[0x38000] __asm__("D_80200000") __attribute__((aligned(0x1000)));

static const void* last_asset_src;
static char last_asset_name[24];
char pm_shape_name[24] = "none"; /* SEAMFIX: the asset name of the shape file in pm_shape_buf */
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

/* HC4f2: the shape loader. romfs:/shape/<name>.nds is written by nd_shape_conv.c (HC4f1): "NDSH", version u32 = 2,
 * size u32, n_reloc u32, reloc[n_reloc] u32 (offsets into data), data[size]. Pointer fields hold shape offsets, so the
 * loader copies data into buf and adds buf to every reloc field. Returns 0 when the file is missing or does not fit buf. */
static int shape_load_romfs(u8* buf, u32 size, const char* name) {
    char path[64];
    u8 hdr[16];
    u32 dsize, n_reloc, k;
    FILE* f;
    snprintf(path, sizeof path, "romfs:/shape/%s.nds", name);
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fread(hdr, 1, 16, f) != 16 || memcmp(hdr, "NDSH", 4) != 0) goto bad;
    if (hdr[4] != 2 || hdr[5] || hdr[6] || hdr[7]) goto bad; /* HC4f3: version 2, typed list words */
    dsize = (u32)hdr[8] | ((u32)hdr[9] << 8) | ((u32)hdr[10] << 16) | ((u32)hdr[11] << 24);
    n_reloc = (u32)hdr[12] | ((u32)hdr[13] << 8) | ((u32)hdr[14] << 16) | ((u32)hdr[15] << 24);
    if (dsize != size) goto bad;
    if (fseek(f, (long)(16 + n_reloc * 4), SEEK_SET) != 0) goto bad;
    if (fread(buf, 1, size, f) != size) goto bad;
    if (fseek(f, 16, SEEK_SET) != 0) goto bad;
    for (k = 0; k < n_reloc; ) { /* the reloc list in chunks of 64 words: no heap, no full copy */
        u32 rb[64];
        u32 c = n_reloc - k < 64 ? n_reloc - k : 64;
        u32 j;
        if (fread(rb, 4, c, f) != c) goto bad;
        for (j = 0; j < c; j++) {
            u32 r = rb[j];
            u32 v;
            if (r > size - 4) goto bad;
            memcpy(&v, buf + r, 4);
            v += (u32)(unsigned long)buf;
            memcpy(buf + r, &v, 4);
        }
        k += c;
    }
    fclose(f);
    return 1;
bad:
    fclose(f);
    return 0;
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

int pm_yay0_raw(const unsigned char* src, unsigned src_len, unsigned char* dst, unsigned dst_len) { return yay0_decode_raw(src, src_len, dst, dst_len); } /* MAPX-4: the core-1 prefetch */
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
    { void pm_walk_drain(const char* why); pm_walk_drain("decode"); } /* CORE2-3b: the decode writes RAM that the last walk may have read */
    unsigned xt0 = pm_xf.on ? pm_ticks() : 0u;
    { /* MAPX-4: the core-1 prefetch decoded this asset (same name and size) at the start of the map change: its bytes, else the decode here */
        extern int pm_prefetch_take(const void* src, void* dst, unsigned dec);
        int pre = pm_prefetch_take(s, dst, dec);
        if (pre) pm_xf.yay_pre++;
        else if (!yay0_decode(s, bound, dst, dec)) return;
    }
    if (pm_xf.on) { pm_xf.yay_n++; pm_xf.yay_b += dec; pm_xf.yay_t += pm_ticks() - xt0; } /* MAPX-1 */
    /* The decode overwrote RAM that cached PICA textures were built from (the background raster and palette sit at one
     * fixed address for every map). The cache keys on address and size, so drop what the decode replaced. */
    pm_tex_invalidate(dst, dec);
    /* ROM data is big-endian. The real loaders read these two kinds of file in place, so convert them here, once. */
    if (dst == (void*)pm_shape_buf) {
        { extern char wMapShapeName[]; memcpy(pm_shape_name, src == last_asset_src ? last_asset_name : wMapShapeName, sizeof pm_shape_name); } /* SEAMFIX: the seamfix log line names the shape (world.c calls its own load_asset_by_name, not the wrap) */
        if (!shape_load_romfs(dst, dec, pm_shape_name)) pm_log_evt("shape load fail %s", pm_shape_name);
        { extern unsigned pm_dlc_gen; pm_dlc_gen++; } /* DLC1: the shape file is final now */
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
    /* HW3: the table is resident ROM. Scan the names in place, no 128 KB copy and no chunked reads per asset. */
    {
        const u8* hdr = pm_rom_ptr(ASSET_TABLE, sizeof(first));
        if (hdr) {
            bytes = rbe(hdr + 0x10);
            const u8* t = (bytes >= sizeof(first) && bytes <= 0x20000) ? pm_rom_ptr(ASSET_TABLE, bytes) : NULL;
            if (t) {
                n = bytes / sizeof(AssetEntry);
                for (i = 0; i < n; i++) {
                    const u8* e = t + i * sizeof(AssetEntry);
                    if (strncmp((const char*)e, name, 0x10) == 0 && strlen(name) < 16) {
                        *rom_off = ASSET_TABLE + rbe(e + 0x10);
                        *comp_len = rbe(e + 0x14);
                        *dec_len = rbe(e + 0x18);
                        return 1;
                    }
                }
                return 0;
            }
        }
    }
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
int pm_debug_has(const char* word);
extern void* __real_load_asset_by_name(const char* name, u32* decompressedSize);
void* __wrap_load_asset_by_name(const char* name, u32* decompressedSize) {
    void* ret;
    if (pm_debug_has("assets")) pm_log_d("load_asset_by_name %s", name);
    ret = __real_load_asset_by_name(name, decompressedSize);
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
            pm_log_d("evt %d cur %d fn %p cl %d first %p flags %x blk %d off %d wait %d", e->id, e->curOpcode, e->callFunction, (s32)(e->ptrCurLine - e->ptrFirstLine), e->ptrFirstLine, e->stateFlags, e->blocked,
                (s32)(e->ptrNextLine - e->ptrFirstLine), e->loopDepth);
        }
    }
}

/* S30b: all 421 maps are linked, the GotoMap guard is gone. The wraps log the map change (warp hook, shot mode, data probe). */
/* MAPX-4: post the Yay0 decode of the target map's shape and hit (the names load_map_by_IDs builds, world.c) to the core-1 prefetch. */
static void map_prefetch(void) {
    extern void pm_prefetch_post(const char* shape, const unsigned char* ssrc, unsigned scl, unsigned sdec, const char* hit, const unsigned char* hsrc, unsigned hcl, unsigned hdec);
    s32 a = gGameStatusPtr->areaID, m = gGameStatusPtr->mapID;
    if (a < 0 || a >= (s32)ARRAY_COUNT(gAreas) || m < 0 || m >= gAreas[a].mapCount) return;
    char sn[24], hn[24];
    const unsigned char* src[2] = { NULL, NULL };
    u32 dec[2] = { 0, 0 }, clen[2] = { 0, 0 };
    snprintf(sn, sizeof sn, "%s_shape", gAreas[a].maps[m].id);
    snprintf(hn, sizeof hn, "%s_hit", gAreas[a].maps[m].id);
    for (int i = 0; i < 2; i++) {
        u32 off, cl, dl;
        if (!find_asset(i ? hn : sn, &off, &cl, &dl)) continue;
        src[i] = pm_rom_ptr(off, cl);
        dec[i] = dl; clen[i] = cl;
    }
    pm_prefetch_post(sn, src[0], clen[0], dec[0], hn, src[1], clen[1], dec[1]);
}
/* BTLWIPE F5: post the Yay0 decode of the battle stage's shape and hit when the star wipe has closed (PRE_BATTLE_LOAD, encounter.c, the
 * first step with the screen black), about 16 steps before BTL_SUBSTATE_NORMAL_START_INIT decodes them (btl_states_actions.c). Not at the
 * start of the wipe: the decode is one long call without a system call, and Azahar did not switch core 1 back to the logic thread inside
 * it (the longest wipe step went 4.0 -> 28.3 ms). battleID and stageID are the ones PRE_BATTLE_LOAD gives
 * load_battle and set_battle_stage (no formation override there); the stage is the one load_battle_section and that state pick. The
 * battle area tables are linked (static). */
void pm_btl_prefetch(s32 battleID, s32 stageID) {
    extern void pm_prefetch_post(const char* shape, const unsigned char* ssrc, unsigned scl, unsigned sdec, const char* hit, const unsigned char* hsrc, unsigned hcl, unsigned hdec);
    u32 area = UNPACK_BTL_AREA(battleID), idx = UNPACK_BTL_INDEX(battleID);
    if (area >= (u32)ARRAY_COUNT(gBattleAreas)) return;
    BattleArea* ba = &gBattleAreas[area];
    Battle* battle = ba->battles ? &(*ba->battles)[idx] : NULL;
    Stage* stage = stageID >= 0 && ba->stages ? (*ba->stages)[stageID].stage : battle ? battle->stage : NULL;
    if (!stage || !stage->shape) return;
    const char* nm[2] = { stage->shape, stage->hit };
    const unsigned char* src[2] = { NULL, NULL };
    u32 dec[2] = { 0, 0 }, clen[2] = { 0, 0 };
    for (int i = 0; i < 2; i++) {
        u32 off, cl, dl;
        if (!nm[i] || !find_asset(nm[i], &off, &cl, &dl)) continue;
        src[i] = pm_rom_ptr(off, cl);
        dec[i] = dl; clen[i] = cl;
    }
    pm_prefetch_post(nm[0], src[0], clen[0], dec[0], nm[1] ? nm[1] : "", src[1], clen[1], dec[1]);
}
extern void __real_load_map_by_IDs(s16 areaID, s16 mapID, s16 loadType);
extern void __real_set_game_mode(s16 mode);

s16 pm_warp_hook(s16 mode);
void pm_walk_drain(const char* why); /* nd_walk.c, CORE2-3b */
void pm_tex_scene_begin(const char* name); /* ROOMTEX: pm_tex.c */
void __wrap_set_game_mode(s16 mode) {
    pm_walk_drain("mode"); /* a mode change loads and frees scene data */
    mode = pm_warp_hook(mode); /* slice W01 debug switches warp, story, preset */
    if (mode == GAME_MODE_ENTER_WORLD) pm_cheat_enter_world(); /* CHEATSAVE: the slot's cheat file (pm_cheatsave.c) */
    {
        extern unsigned pm_frame_count(void);
        char sm[16];
        pm_log_d("game mode %d frame %u", mode, pm_frame_count());
        pm_ph_scene_mode(mode);
        if (pm_debug_value("shotmode", sm, sizeof sm) && atoi(sm) == mode) { /* X01: shotmode=<mode>:<n> the window of n frames from the change to <mode>; one shot file, its last frame (SHOT-1, pm_gpu.c dump_shot) */
            const char* c = strchr(sm, ':');
            extern void pm_gpu_shot_burst(int n);
            pm_gpu_shot_burst(c ? atoi(c + 1) : 60);
        }
    }
    if (mode == GAME_MODE_BATTLE) pm_tex_scene_begin("battle"); /* ROOMTEX: scene starts at a battle start (battle.c:149) and at END_BATTLE (btl_states_actions.c:2168, :4499) */
    else if (mode == GAME_MODE_END_BATTLE) pm_tex_scene_begin("battle end");
    if (mode == GAME_MODE_CHANGE_MAP) { pm_xfer_map_begin(); map_prefetch(); } /* MAPX-1: the fade out starts the xfer line; MAPX-4: the target's shape and hit decode on core 1 */
    if (mode == GAME_MODE_WORLD) {
        extern void pm_warp_world_mode(unsigned frame);
        extern unsigned pm_frame_count(void);
        pm_warp_world_mode(pm_frame_count());
    }
    __real_set_game_mode(mode);
}

void __wrap_load_map_by_IDs(s16 areaID, s16 mapID, s16 loadType) {
    unsigned xt0 = pm_xf.on ? pm_ticks() : 0u; /* MAPX-1: the load and audio stage ticks of the xfer line */
    pm_walk_drain("map");
    pm_audio_join_at(0); /* AU2; MAPX-5b: the control job joins (it may read what the load writes); the mixer render queue is not drained: it reads only
                          * its own command ring and the sample data (docs/plans/AU2_design.md), and the ROM is in RAM. The drain waited about 100 ms on Azahar. */
    if (pm_xf.on) pm_xf.au_t += pm_ticks() - xt0;
    {
        extern unsigned pm_pad_scans;
        pm_log_d("load map area %d map %d scan %u", areaID, mapID, pm_pad_scans);
        pm_ph_scene_map(areaID, mapID);
    }
    pm_mem_log("map");
    {
        extern void pm_mapdata_probe(const char* id);
        s16 pa = loadType == LOAD_FROM_MAP ? areaID : gGameStatusPtr->areaID, pmap = loadType == LOAD_FROM_MAP ? mapID : gGameStatusPtr->mapID;
        { /* ROOMTEX: a map load is a scene start, named by its area:map ids */
            extern void pm_tex_scene_begin(const char* name);
            char nm[24];
            snprintf(nm, sizeof nm, "map %d:%d", (int)pa, (int)pmap);
            pm_tex_scene_begin(nm);
        }
        pm_mapdata_probe(gAreas[pa].maps[pmap].id); /* slice MS01 debug switch dataprobe */
    }
    __real_load_map_by_IDs(areaID, mapID, loadType);
    if (pm_xf.on) pm_xf.load_t += pm_ticks() - xt0;
    pm_xfer_map_load(loadType == LOAD_FROM_MAP ? areaID : gGameStatusPtr->areaID, loadType == LOAD_FROM_MAP ? mapID : gGameStatusPtr->mapID);
}

/* BTLWIPE: the load step of the xfer battle line */
extern void __real_load_battle_section(void);
void __wrap_load_battle_section(void) {
    unsigned t0 = pm_ticks();
    __real_load_battle_section();
    pm_xfer_btl_load(pm_ticks() - t0);
}

/* MS01: a linked map has PM_MAP_DMA_TAG as dmaStart and dmaEnd (3ds/stub_include/map_rom_ranges.h), so load_map_by_IDs calls
 * dma_copy for it, at the place where the N64 copied the overlay. The overlay has no ROM data here. The call restores the static data of
 * the map instead (pm_mapdata.c), like the overlay copy did. gMapConfig is set by then. */
extern MapConfig* gMapConfig;
extern u32 __real_dma_copy(u8* romStart, u8* romEnd, void* vramDest);
u32 __wrap_dma_copy(u8* romStart, u8* romEnd, void* vramDest) {
    if (romStart == (u8*)0x70000001 && romEnd == romStart) {
        extern void pm_mapdata_enter(const char* id);
        pm_mapdata_enter(gMapConfig->id);
        return 0;
    }
    return __real_dma_copy(romStart, romEnd, vramDest);
}
