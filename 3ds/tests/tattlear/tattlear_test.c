/* TATTLEAR host test: the tattle camera view in wide. set_cam_viewport and pm_wide_x3d (cam_main.c, extracted by
 * tattlear.sh) are the real code. Prints "tattlear ok 7" or "tattlear FAIL <what>" (first line) and exits 1. */
#include <stdio.h>
#include "common.h"

extern int pm_wide;
s32 pm_wide_x3d(s32 x, s32 up);
void set_cam_viewport(s16 id, s16 x, s16 y, s16 width, s16 height);

int pm_wide;
Camera gCameras[4];
GameStatus gStatus;
GameStatus* gGameStatusPtr = &gStatus;

static int chk(const char* what, long got, long want) {
    if (got != want) {
        printf("tattlear FAIL %s got %ld want %ld\n", what, got, want);
        return 1;
    }
    return 0;
}

/* screen x of a 3D vtrans/vscale value in 1/4 units (scr(n) = (n * 5 + 8) / 16, as in the page) */
static int scr(int n) {
    return (n * 5 + 8) / 16;
}

int main(void) {
    Camera* c = &gCameras[CAM_TATTLE];
    s32 i;

    /* 1: wide, tattle window 137 95 138 99 */
    pm_wide = 1;
    gStatus.altViewportOffset.x = 0;
    set_cam_viewport(CAM_TATTLE, 137, 95, 138, 99);
    if (chk("1 vscale", c->vp.vp.vscale[0], 220)) return 1;
    if (chk("1 vscale y", c->vp.vp.vscale[1], 198)) return 1;
    if (chk("1 vtrans", c->vp.vp.vtrans[0], 787)) return 1;
    if (chk("1 alt vscale", c->vpAlt.vp.vscale[0], 220)) return 1;
    if (chk("1 alt vtrans", c->vpAlt.vp.vtrans[0], 787)) return 1;

    /* 2: as 1, alt viewport offset x 8 */
    gStatus.altViewportOffset.x = 8;
    set_cam_viewport(CAM_TATTLE, 137, 95, 138, 99);
    if (chk("2 alt vtrans", c->vpAlt.vp.vtrans[0], 795)) return 1;
    gStatus.altViewportOffset.x = 0;

    /* 3: wide, tattle window 37 95 138 99 */
    set_cam_viewport(CAM_TATTLE, 37, 95, 138, 99);
    if (chk("3 vtrans", c->vp.vp.vtrans[0], 467)) return 1;
    if (chk("3 vscale", c->vp.vp.vscale[0], 220)) return 1;

    /* 4: not wide, args of 1 */
    pm_wide = 0;
    set_cam_viewport(CAM_TATTLE, 137, 95, 138, 99);
    if (chk("4 vscale", c->vp.vp.vscale[0], 276)) return 1;
    if (chk("4 vtrans", c->vp.vp.vtrans[0], 824)) return 1;

    /* 5: wide, CAM_BATTLE full view is not changed */
    pm_wide = 1;
    set_cam_viewport(CAM_BATTLE, 0, 0, 320, 240);
    if (chk("5 battle vscale", gCameras[CAM_BATTLE].vp.vp.vscale[0], 640)) return 1;
    if (chk("5 battle vtrans", gCameras[CAM_BATTLE].vp.vp.vtrans[0], 640)) return 1;

    /* 6: pm_wide_x3d, and each value * 5 / 4 = 40 + x */
    {
        static const s32 xin[4] = { 137, 275, 37, 175 };
        static const s32 upin[4] = { 1, 0, 1, 0 };
        static const s32 want[4] = { 142, 252, 62, 172 };
        for (i = 0; i < 4; i++) {
            s32 v = pm_wide_x3d(xin[i], upin[i]);
            if (chk("6 pm_wide_x3d", v, want[i])) return 1;
            if (chk("6 back to 2D", v * 5 / 4, 40 + xin[i])) return 1;
        }
    }

    /* 7: screen edges of the tattle view, wide */
    pm_wide = 1;
    set_cam_viewport(CAM_TATTLE, 137, 95, 138, 99);
    if (chk("7 case1 left", scr(c->vp.vp.vtrans[0] - c->vp.vp.vscale[0]), 177)) return 1;
    if (chk("7 case1 right", scr(c->vp.vp.vtrans[0] + c->vp.vp.vscale[0]), 315)) return 1;
    set_cam_viewport(CAM_TATTLE, 37, 95, 138, 99);
    if (chk("7 case3 left", scr(c->vp.vp.vtrans[0] - c->vp.vp.vscale[0]), 77)) return 1;
    if (chk("7 case3 right", scr(c->vp.vp.vtrans[0] + c->vp.vp.vscale[0]), 215)) return 1;

    printf("tattlear ok 7\n");
    return 0;
}
