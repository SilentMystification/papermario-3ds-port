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

static void emit_color_overlay(s32 layer) {
    s32 type = overlay_type[layer];
    f32 alpha = overlay_alpha[layer];
    if (type != OVERLAY_SCREEN_COLOR && type != OVERLAY_VIEWPORT_COLOR) return;
    if (alpha <= 0.f) return;
    gDPPipeSync(gMainGfxPos++);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
    gDPSetCombineMode(gMainGfxPos++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gMainGfxPos++, 0, 0, overlay_r[layer], overlay_g[layer], overlay_b[layer], (u8)alpha);
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

void render_screen_overlay_backUI(void) { emit_color_overlay(SCREEN_LAYER_BACK); }
void render_screen_overlay_frontUI(void) { emit_color_overlay(SCREEN_LAYER_FRONT); }
