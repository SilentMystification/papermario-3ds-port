#include "common.h"
#include "game_modes.h"
#include "fio.h"
#include "battle/battle.h"

/* Set by load_map_by_IDs (pm_map.c). The real change_map and enter_world load the map, so state_init_world must not load it again. */
int pm_map_loaded;

/* US title_data, after the four big-endian offsets at the front. */
#define TITLE_LOGO_W 200
#define TITLE_LOGO_H 112
#define TITLE_LOGO_X 60
#define TITLE_LOGO_Y 15
#define PRESS_W 128
#define PRESS_H 32

extern void* pm_decode_asset(const char* name, unsigned* out_size);
extern void pm_log(const char* fmt, ...);
void init_hud_element_list(void);
void init_item_entity_list(void);
void hud_element_clear_cache(void);

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

void state_init_title_screen(void) {
    gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
    gOverrideFlags |= GLOBAL_OVERRIDES_DISABLE_RENDER_WORLD;
    clear_screen_overlays();
    gGameStatusPtr->introPart = INTRO_PART_NONE;
    gGameStatusPtr->startupState = 2;
    /* Leaving the intro without a map load (hos_04 is not in gAreas): drop every list that points into the general heap. */
    clear_npcs();
    clear_render_tasks();
    clear_script_list();
    clear_worker_list();
    clear_model_data();
    clear_animator_list();
    clear_entity_models();
    clear_encounter_status();
    clear_effect_data();
    clear_entity_data(true);
    general_heap_create();
    /* The real file select and startup states set these lists up before the first map load. */
    init_hud_element_list();
    init_item_entity_list();
    hud_element_clear_cache();
    load_title_images();
    reset_background_settings();
    load_map_bg("title_bg");
    set_background(&gBackgroundImage);
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
        /* Same calls as the real new-file path (filemenu_yesno.c FM_CONFIRM_CREATE). Without
         * STORY_INTRO, mac_00 builds DefaultNPCs but runs the intro scene on them. */
        clear_player_data();
        clear_saved_variables();
        evt_set_variable(nullptr, GB_StoryProgress, STORY_INTRO);
        pm_log("file %d", file_slot + 1);
        pm_map_loaded = 0;
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
    if (pm_map_loaded) {
        pm_map_loaded = 0;
        gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_RENDER_WORLD;
        return;
    }
    gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME;
    gOverrideFlags &= ~GLOBAL_OVERRIDES_DISABLE_RENDER_WORLD;
    /* The intro fade is a full-screen plate. It stays up after the title
     * unless this mode clears it, which is the blank screen on a file slot. */
    clear_screen_overlays();
    gGameStatusPtr->context = CONTEXT_WORLD;
    load_map_by_IDs(gGameStatusPtr->areaID, gGameStatusPtr->mapID, LOAD_FROM_MAP);
}

int pm_debug_has(const char* word);
void state_step_world(void) {
    gCameras[CAM_DEFAULT].flags &= ~CAMERA_FLAG_DISABLED;
    update_encounters();
    update_npcs();
    update_player();
    update_effects();
    iterate_models();
    update_cameras();
    /* Debug switch "battle": start the first KMR battle 600 world frames after the first world frame. No game logic. */
    if (pm_debug_has("battle")) {
        static int bf;
        if (++bf == 600) {
            pm_log("debug battle: load_battle(BTL_KMR_1_FORMATION_00)");
            set_battle_stage(BTL_KMR_1_STAGE_01);
            load_battle(BTL_KMR_1_FORMATION_00);
        }
    }
    /* Debug switch "demobattle": enter the real demo state at scene 1, which calls load_demo_battle(0) (dig area, Fuzzies). */
    if (pm_debug_has("demobattle")) {
        static int df;
        if (++df == 600) {
            pm_log("debug demobattle: enter GAME_MODE_DEMO at demo scene 1 (battle index 0)");
            gGameStatusPtr->demoState = DEMO_STATE_ACTIVE;
            gGameStatusPtr->nextDemoScene = 1;
            set_game_mode(GAME_MODE_DEMO);
        }
    }
    if (pm_debug_has("player")) {
        static int n;
        if (++n % 10 == 0) {
            pm_log("plr %d %d %d as %d fl %x floor %d", (s32)gPlayerStatus.pos.x, (s32)gPlayerStatus.pos.y, (s32)gPlayerStatus.pos.z,
                   gPlayerStatus.actionState, gPlayerStatus.flags, gCollisionStatus.curFloor);
            for (s32 i = 0; i < MAX_NPCS; i++) {
                Npc* npc = get_npc_by_index(i);
                if (npc != nullptr && npc->flags != 0) {
                    pm_log("npc%d %d %d %d spd %d floor %d fl %x", i, (s32)npc->pos.x, (s32)npc->pos.y, (s32)npc->pos.z,
                           (s32)npc->moveSpeed, npc->curFloor, npc->flags);
                }
            }
        }
    }
}

/* Storage only. The real definition (with FILENAME_ERROR defaults) is in src/state_title_screen.c, which the port replaces
 * with the code above until the title slice. fio.c needs the symbol. Remove when state_title_screen.c is built. */
SaveMetadata gSaveSlotMetadata[4];
