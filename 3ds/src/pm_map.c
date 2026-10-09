#include "common.h"
#include "model.h"
#include "camera.h"
#include "pm_port.h"

int strcmp(const char* a, const char* b);
void bcopy(const void* src, void* dst, unsigned int n);

extern EvtScript hos_05_EVS_Intro_Main;

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

static void* shape_keep;

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
                    int count = (w0 >> 12) & 0xff;
                    if (claim(s, target) && count > 0 && count <= 64) swap_verts(target, count);
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

static int yay0_decode(const u8* src, u32 src_len, u8* dst, u32 dst_len) {
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

static int find_asset(const char* name, u32* rom_off, u32* comp_len, u32* dec_len) {
    AssetEntry first;
    AssetEntry* table;
    u32 bytes, i, n;
    dma_copy((u8*)ASSET_TABLE, (u8*)ASSET_TABLE + sizeof(first), &first);
    swap_u32s(&first.offset, 3);
    bytes = first.offset;
    if (bytes < sizeof(first) || bytes > 0x20000) return 0;
    table = general_heap_malloc(bytes);
    if (!table) return 0;
    dma_copy((u8*)ASSET_TABLE, (u8*)ASSET_TABLE + bytes, table);
    n = bytes / sizeof(AssetEntry);
    for (i = 0; i < n; i++) {
        swap_u32s(&table[i].offset, 3);
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

void load_map_by_IDs(s16 areaID, s16 mapID, s16 loadType) {
    const char* shape_name = NULL;
    const char* tex_name = "hos_tex";
    u32 rom_off, comp_len, dec_len, tex_off, tex_comp, tex_dec;
    void* compressed;
    u8* shape;
    ModelNode* root;
    Camera* cam;
    s32 models = 0;
    s32 i;

    (void)loadType;
    if (mapID == 5) shape_name = "hos_05_shape";
    else if (mapID == 4) shape_name = "hos_04_shape";
    else if (areaID == AREA_KMR && mapID == 11) {
        shape_name = "kmr_20_shape";
        tex_name = "kmr_tex";
    } else if (areaID == AREA_KMR && mapID == 1) {
        shape_name = "kmr_02_shape";
        tex_name = "kmr_tex";
    } else {
        pm_log("map %d not loaded", mapID);
        return;
    }

    general_heap_create();
    clear_render_tasks();
    clear_script_list();
    clear_worker_list();
    clear_model_data();
    create_cameras();

    if (!find_asset(shape_name, &rom_off, &comp_len, &dec_len) || dec_len < 32 || dec_len > 0x80000 || comp_len > 0x80000) {
        pm_log("map %s missing", shape_name);
        return;
    }
    compressed = general_heap_malloc(comp_len);
    shape = general_heap_malloc(dec_len);
    if (!compressed || !shape) {
        pm_log("map %s alloc failed", shape_name);
        return;
    }
    dma_copy((u8*)rom_off, (u8*)rom_off + comp_len, compressed);
    if (!yay0_decode(compressed, comp_len, shape, dec_len)) {
        pm_log("map %s yay0 failed", shape_name);
        general_heap_free(compressed);
        return;
    }
    general_heap_free(compressed);
    root = fix_shape(shape, dec_len);
    if (!root) {
        pm_log("map %s fix failed", shape_name);
        return;
    }
    shape_keep = shape;

    if (find_asset(tex_name, &tex_off, &tex_comp, &tex_dec)) {
        load_data_for_models(root, tex_off, tex_comp);
    } else {
        load_data_for_models(root, 0, 0);
    }

    for (i = 0; i < MAX_MODELS; i++) {
        if ((*gCurrentModels)[i] != NULL) models++;
    }

    cam = &gCameras[CAM_DEFAULT];
    cam->flags &= ~CAMERA_FLAG_LEAD_PLAYER;
    cam->flags &= ~CAMERA_FLAG_DISABLED;
    cam->nearClip = 16;
    cam->farClip = 4096;
    cam->vfov = 25.f;
    cam->updateMode = CAM_UPDATE_FROM_ZONE;
    cam->needsInit = false;
    cam->needsReinit = true;
    cam->yinterpRate = 3.f;
    cam->params.world.zoomPercent = 100;
    gCurrentCameraID = CAM_DEFAULT;

    if (mapID == 5) {
        /* Intro_Main loads IntroCamSettings and pans. Zone update turns that
         * boom into the eye. Minimal mode was ignoring it. */
        cam->controlSettings.type = CAM_CONTROL_FIXED_ORIENTATION;
        cam->controlSettings.boomLength = 130.4f;
        cam->controlSettings.boomPitch = 12.4f;
        cam->controlSettings.viewPitch = -16.8f;
        cam->controlSettings.flag = false;
        cam->controlSettings.points.two.Ax = 0.f;
        cam->controlSettings.points.two.Ay = -1.f;
        cam->controlSettings.points.two.Az = 0.f;
        cam->controlSettings.points.two.Bx = -433.0127f;
        cam->controlSettings.points.two.By = -1.f;
        cam->controlSettings.points.two.Bz = -250.f;
        cam->movePos.x = 0.f;
        cam->movePos.y = 157.f;
        cam->movePos.z = 0.f;
        cam->targetPos = cam->movePos;
        cam->followPlayer = true;
        cam->panActive = true;
        set_cam_viewport(CAM_DEFAULT, 29, 28, 262, 162);
        {
            Evt* script = start_script_in_group(&hos_05_EVS_Intro_Main, EVT_PRIORITY_0, 0, EVT_GROUP_NEVER_PAUSE);
            if (script) gGameStatusPtr->mainScriptID = script->id;
            pm_log("intro script %d", gGameStatusPtr->mainScriptID);
        }
    } else {
        /* No kmr_20 script yet, so there is no zone to enter. A fixed boom
         * behind the door (yaw 90 faces +X) is the stand-in until one loads. */
        cam->controlSettings.type = CAM_CONTROL_FIXED_ORIENTATION;
        cam->controlSettings.boomLength = 480.f;
        cam->controlSettings.boomPitch = 18.f;
        cam->controlSettings.viewPitch = -5.f;
        cam->controlSettings.flag = false;
        cam->controlSettings.points.two.Ax = 0.f;
        cam->controlSettings.points.two.Ay = 0.f;
        cam->controlSettings.points.two.Az = 0.f;
        cam->controlSettings.points.two.Bx = 50.f;
        cam->controlSettings.points.two.By = 0.f;
        cam->controlSettings.points.two.Bz = 0.f;
        cam->followPlayer = false;
        cam->targetPos.x = 240.f;
        cam->targetPos.y = 30.f;
        cam->targetPos.z = -80.f;
        set_cam_viewport(CAM_DEFAULT, 0, 0, 320, 240);
        gPlayerStatus.pos.x = 240.f;
        gPlayerStatus.pos.y = 30.f;
        gPlayerStatus.pos.z = -80.f;
        gPlayerStatus.curYaw = 90.f;
        update_cameras();
        pm_log("world cam");
    }
    pm_log("map %s models %d", shape_name, models);
}

static Npc intro_npcs[16];

Npc* get_npc_safe(s32 npcId) {
    if (npcId < 0 || npcId >= 16) return &intro_npcs[0];
    return &intro_npcs[npcId];
}

Npc* resolve_npc(Evt* script, s32 npcIdOrPtr) {
    (void)script;
    if (npcIdOrPtr == NPC_SELF) return &intro_npcs[0];
    if (npcIdOrPtr >= EVT_LIMIT) return get_npc_safe(npcIdOrPtr);
    return (Npc*)(unsigned long)npcIdOrPtr;
}
