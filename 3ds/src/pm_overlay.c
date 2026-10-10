#include "common.h"

static s32 overlay_type[2] = { OVERLAY_NONE, OVERLAY_NONE };
static f32 overlay_alpha[2] = { -1.f, -1.f };
static u8 overlay_r[2], overlay_g[2], overlay_b[2];

void set_screen_overlay_params_front(u8 type, f32 zoom) {
    overlay_type[SCREEN_LAYER_FRONT] = (s8)type;
    overlay_alpha[SCREEN_LAYER_FRONT] = zoom;
}

void set_screen_overlay_params_back(u8 type, f32 zoom) {
    overlay_type[SCREEN_LAYER_BACK] = (s8)type;
    overlay_alpha[SCREEN_LAYER_BACK] = zoom;
}

void set_screen_overlay_color(s32 layer, u8 r, u8 g, u8 b) {
    if (layer != SCREEN_LAYER_FRONT && layer != SCREEN_LAYER_BACK) return;
    overlay_r[layer] = r;
    overlay_g[layer] = g;
    overlay_b[layer] = b;
}

void get_screen_overlay_params(s32 layer, u8* type, f32* zoom) {
    if (layer != SCREEN_LAYER_FRONT && layer != SCREEN_LAYER_BACK) {
        if (type) *type = (u8)OVERLAY_NONE;
        if (zoom) *zoom = 0.f;
        return;
    }
    if (type) *type = (u8)overlay_type[layer];
    if (zoom) *zoom = overlay_alpha[layer] > 0.f ? overlay_alpha[layer] : 0.f;
}

static void emit_color_overlay(s32 layer) {
    s32 type = overlay_type[layer];
    f32 alpha = overlay_alpha[layer];
    if (type != OVERLAY_SCREEN_COLOR && type != OVERLAY_VIEWPORT_COLOR) return;
    if (alpha <= 0.f) return;
    gDPPipeSync(gMainGfxPos++);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
    gDPSetCombineMode(gMainGfxPos++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gMainGfxPos++, 0, 0, overlay_r[layer], overlay_g[layer], overlay_b[layer], (u8)alpha);
    gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (type == OVERLAY_SCREEN_COLOR) {
        gDPFillRectangle(gMainGfxPos++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    } else {
        Camera* cam = &gCameras[CAM_DEFAULT];
        s32 x1 = cam->viewportStartX + cam->viewportW - 1;
        s32 y1 = cam->viewportStartY + cam->viewportH - 1;
        if (x1 < cam->viewportStartX) x1 = cam->viewportStartX;
        if (y1 < cam->viewportStartY) y1 = cam->viewportStartY;
        gDPFillRectangle(gMainGfxPos++, cam->viewportStartX, cam->viewportStartY, x1, y1);
    }
}

void clear_screen_overlays(void) {
    overlay_type[SCREEN_LAYER_FRONT] = OVERLAY_NONE;
    overlay_type[SCREEN_LAYER_BACK] = OVERLAY_NONE;
    overlay_alpha[SCREEN_LAYER_FRONT] = -1.f;
    overlay_alpha[SCREEN_LAYER_BACK] = -1.f;
}

void render_screen_overlay_backUI(void) { emit_color_overlay(SCREEN_LAYER_BACK); }
void render_screen_overlay_frontUI(void) { emit_color_overlay(SCREEN_LAYER_FRONT); }

/* The theater mesh lives in curtains.c, which needs extracted PNGs we do not
 * link. This is the same opening: scale 2 is fully open, scale 1 frames the
 * intro viewport (29, 28, 262x162) so the cutscene sits inside the curtain. */
static f32 curtain_scale = 2.f;
static f32 curtain_scale_goal = 2.f;
static f32 curtain_fade = 0.f;
static f32 curtain_fade_goal = 0.f;
static void (*curtain_draw)(void) = NULL;

void initialize_curtains(void) {
    curtain_draw = NULL;
    curtain_scale = 2.f;
    curtain_scale_goal = 2.f;
    curtain_fade = 0.f;
    curtain_fade_goal = 0.f;
}

void update_curtains(void) {}

void set_curtain_scale_goal(f32 scale) { curtain_scale_goal = scale; }

void set_curtain_scale(f32 scale) {
    curtain_scale_goal = scale;
    curtain_scale = scale;
}

void set_curtain_fade_goal(f32 fade) { curtain_fade_goal = fade; }

void set_curtain_fade(f32 fade) {
    curtain_fade_goal = fade;
    curtain_fade = fade;
}

void set_curtain_draw_callback(void (*callback)(void)) { curtain_draw = callback; }

void render_curtains(void) {
    f32 open;
    int shade, left, top, right, bottom;
    int r, g, b;

    if (curtain_scale_goal != curtain_scale) {
        curtain_scale += (curtain_scale_goal - curtain_scale) * 0.1f;
    }
    if (curtain_fade_goal != curtain_fade) {
        curtain_fade += (curtain_fade_goal - curtain_fade) * 0.03f;
    }

    if (curtain_scale < 1.9f) {
        open = (1.9f - curtain_scale) / 0.9f;
        if (open < 0.f) open = 0.f;
        if (open > 1.f) open = 1.f;
        left = (int)(29.f * open);
        top = (int)(28.f * open);
        right = SCREEN_WIDTH - (int)(29.f * open);
        bottom = SCREEN_HEIGHT - (int)(50.f * open);
        shade = (int)(255.f - curtain_fade * 255.f);
        if (shade < 0) shade = 0;
        if (shade > 255) shade = 255;
        r = shade;
        g = shade * 0x28 / 255;
        b = shade * 0x18 / 255;
        gDPPipeSync(gMainGfxPos++);
        gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
        gDPSetCombineMode(gMainGfxPos++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gDPSetPrimColor(gMainGfxPos++, 0, 0, r, g, b, 255);
        if (left > 0) gDPFillRectangle(gMainGfxPos++, 0, 0, left - 1, SCREEN_HEIGHT - 1);
        if (right < SCREEN_WIDTH) gDPFillRectangle(gMainGfxPos++, right, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
        if (top > 0) gDPFillRectangle(gMainGfxPos++, left, 0, right - 1, top - 1);
        if (bottom < SCREEN_HEIGHT) gDPFillRectangle(gMainGfxPos++, left, bottom, right - 1, SCREEN_HEIGHT - 1);
    }

    if (curtain_draw) curtain_draw();
}
