#include "common.h"
#include "game_modes.h"

/* US title_data, after the four big-endian offsets at the front. */
#define TITLE_LOGO_W 200
#define TITLE_LOGO_H 112
#define TITLE_LOGO_X 60
#define TITLE_LOGO_Y 15
#define PRESS_W 128
#define PRESS_H 32

extern void* pm_decode_asset(const char* name, unsigned* out_size);
extern void pm_log(const char* fmt, ...);

static u8* title_blob;
static u8* logo;
static u8* press;
static int show_press = 1;
static int blink;
static int file_slot;

static u32 read_be(const u8* p) {
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
}

static void load_title_images(void) {
    unsigned size = 0;
    u32 logo_off, press_off;

    title_blob = pm_decode_asset("title_data", &size);
    logo = NULL;
    press = NULL;
    if (!title_blob || size < 16) {
        pm_log("title_data missing");
        return;
    }
    logo_off = read_be(title_blob);
    press_off = read_be(title_blob + 8);
    if (logo_off + TITLE_LOGO_W * TITLE_LOGO_H * 4u <= size) logo = title_blob + logo_off;
    if (press_off + PRESS_W * PRESS_H <= size) press = title_blob + press_off;
    pm_log("title logo %08x press %08x", logo_off, press_off);
}

static void draw_logo(void) {
    if (!logo) return;
    gDPPipeSync(gMainGfxPos++);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
    gDPSetCombineMode(gMainGfxPos++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    gSPTexture(gMainGfxPos++, -1, -1, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureTile(gMainGfxPos++, logo, G_IM_FMT_RGBA, G_IM_SIZ_32b, TITLE_LOGO_W, TITLE_LOGO_H,
                       0, 0, TITLE_LOGO_W - 1, TITLE_LOGO_H - 1, 0,
                       G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gSPScisTextureRectangle(gMainGfxPos++,
                            TITLE_LOGO_X << 2, TITLE_LOGO_Y << 2,
                            (TITLE_LOGO_X + TITLE_LOGO_W) << 2, (TITLE_LOGO_Y + TITLE_LOGO_H) << 2,
                            G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    gDPPipeSync(gMainGfxPos++);
}

static void draw_press_start(void) {
    if (!press || !show_press) return;
    gDPPipeSync(gMainGfxPos++);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
    gDPSetCombineMode(gMainGfxPos++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    gSPTexture(gMainGfxPos++, -1, -1, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlock(gMainGfxPos++, press, G_IM_FMT_IA, G_IM_SIZ_8b, PRESS_W, PRESS_H, 0,
                        G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gSPTextureRectangle(gMainGfxPos++, 96 << 2, 137 << 2, (96 + PRESS_W) << 2, (137 + PRESS_H) << 2,
                        G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    gDPPipeSync(gMainGfxPos++);
}

/* Background files store N64 pointers relative to gBackgroundImage at 0x80200000. */
#define BG_BASE 0x80200000u

static u8* bg_blob;
static u8* bg_raster;
static u8* bg_pal;
static int bg_x, bg_y, bg_w, bg_h;

static void load_background(const char* name) {
    unsigned size = 0;
    u8* blob;
    u32 raster, pal;

    bg_blob = NULL;
    bg_raster = NULL;
    bg_pal = NULL;
    bg_w = bg_h = 0;
    blob = pm_decode_asset(name, &size);
    if (!blob || size < 16) {
        pm_log("bg %s missing", name);
        return;
    }
    raster = read_be(blob);
    pal = read_be(blob + 4);
    bg_x = (blob[8] << 8) | blob[9];
    bg_y = (blob[10] << 8) | blob[11];
    bg_w = (blob[12] << 8) | blob[13];
    bg_h = (blob[14] << 8) | blob[15];
    if (raster >= BG_BASE && raster - BG_BASE < size) bg_raster = blob + (raster - BG_BASE);
    if (pal >= BG_BASE && pal - BG_BASE + 512u <= size) bg_pal = blob + (pal - BG_BASE);
    bg_blob = blob;
    pm_log("bg %s %dx%d", name, bg_w, bg_h);
}

void pm_draw_loaded_background(void) {
    int w, h;

    if (!bg_raster || !bg_pal || bg_w < 8 || bg_h < 8 || !gMainGfxPos) return;
    w = bg_w > 320 ? 320 : bg_w;
    h = bg_h > 240 ? 240 : bg_h;
    gDPPipeSync(gMainGfxPos++);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
    gDPSetCombineMode(gMainGfxPos++, G_CC_DECALRGB, G_CC_DECALRGB);
    gSPTexture(gMainGfxPos++, -1, -1, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTLUT_pal256(gMainGfxPos++, bg_pal);
    gDPLoadTextureTile(gMainGfxPos++, bg_raster, G_IM_FMT_CI, G_IM_SIZ_8b, bg_w, h,
                       0, 0, w - 1, h - 1, 0,
                       G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gSPTextureRectangle(gMainGfxPos++, bg_x << 2, bg_y << 2, (bg_x + w) << 2, (bg_y + h) << 2,
                        G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    gDPPipeSync(gMainGfxPos++);
}

void state_init_title_screen(void) {
    gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
    gOverrideFlags |= GLOBAL_OVERRIDES_DISABLE_RENDER_WORLD;
    clear_screen_overlays();
    gGameStatusPtr->introPart = INTRO_PART_NONE;
    gGameStatusPtr->startupState = 2;
    general_heap_create();
    load_title_images();
    load_background("title_bg");
    show_press = 1;
    blink = 0;
    pm_log("title screen");
}

void state_step_title_screen(void) {
    u32 pressed = gGameStatusPtr->pressedButtons[0];

    blink++;
    if (blink >= 20) {
        blink = 0;
        show_press ^= 1;
    }
    if (pressed & (BUTTON_A | BUTTON_START)) {
        set_game_mode(GAME_MODE_FILE_SELECT);
    }
}

void state_drawUI_title_screen(void) {
    draw_logo();
    draw_press_start();
}

void state_init_file_select(void) {
    gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
    gOverrideFlags |= GLOBAL_OVERRIDES_DISABLE_RENDER_WORLD;
    clear_screen_overlays();
    file_slot = 0;
    pm_log("file select");
}

void state_step_file_select(void) {
    u32 pressed = gGameStatusPtr->pressedButtons[0];

    if (pressed & (BUTTON_C_UP | BUTTON_D_UP | BUTTON_STICK_UP)) {
        if (file_slot > 0) file_slot--;
    }
    if (pressed & (BUTTON_C_DOWN | BUTTON_D_DOWN | BUTTON_STICK_DOWN)) {
        if (file_slot < 2) file_slot++;
    }
    if (pressed & (BUTTON_A | BUTTON_START)) {
        /* kmr_maps[11] is kmr_20, Mario's house. Entry 0 faces into the room. */
        gGameStatusPtr->areaID = AREA_KMR;
        gGameStatusPtr->mapID = 11;
        gGameStatusPtr->entryID = 0;
        pm_log("file %d", file_slot + 1);
        set_game_mode(GAME_MODE_WORLD);
    }
}

void state_drawUI_file_select(void) {
    s32 i;

    gDPPipeSync(gMainGfxPos++);
    gDPSetCycleType(gMainGfxPos++, G_CYC_FILL);
    gDPSetFillColor(gMainGfxPos++, GPACK_RGBA5551(32, 32, 64, 1) << 16 | GPACK_RGBA5551(32, 32, 64, 1));
    gDPFillRectangle(gMainGfxPos++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    for (i = 0; i < 3; i++) {
        unsigned shade = (i == file_slot) ? GPACK_RGBA5551(240, 208, 96, 1) : GPACK_RGBA5551(80, 80, 112, 1);
        s32 y = 48 + i * 48;
        gDPPipeSync(gMainGfxPos++);
        gDPSetFillColor(gMainGfxPos++, shade << 16 | shade);
        gDPFillRectangle(gMainGfxPos++, 40, y, 280, y + 32);
    }
    gDPPipeSync(gMainGfxPos++);
}

void state_init_world(void) {
    gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
    gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_RENDER_WORLD;
    /* The intro fade is a full-screen plate. It stays up after the title
     * unless this mode clears it, which is the blank screen on a file slot. */
    clear_screen_overlays();
    load_map_by_IDs(gGameStatusPtr->areaID, gGameStatusPtr->mapID, LOAD_FROM_MAP);
    load_background("kmr_bg");
}

void state_step_world(void) {
    Camera* cam = &gCameras[CAM_DEFAULT];

    /* The zone controller reads this. Forcing minimal mode here threw away
     * every boom and pan the map script had set. */
    cam->flags &= ~CAMERA_FLAG_DISABLED;
    if (!(gPlayerStatus.flags & PS_FLAG_CAMERA_DOESNT_FOLLOW)) {
        cam->targetPos.x = gPlayerStatus.pos.x;
        cam->targetPos.y = gPlayerStatus.pos.y;
        cam->targetPos.z = gPlayerStatus.pos.z;
    }
    update_cameras();
}
