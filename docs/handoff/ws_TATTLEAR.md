# ws_TATTLEAR (Haiku): wide tattle view fills the 2D window 1:1 (user bug; no counter move)
Designer: oracle DIAG1. Worktree build3ds/wt/TATTLEAR, branch TATTLEAR from main. PM_ROW=TATTLEAR PM_LANE=TATTLEAR. Edit only
src/cam_main.c, src/battle/16C8E0.c; add 3ds/tests/host.d/tattlear.sh, 3ds/tests/tattlear/tattlear_test.c.
## Cause (code read, main e1e9ff16ac)
Wide: a 3D draw puts game x at 1.25 x (pm_gpu.c:230-231, :1131-1132), a 2D draw at 40 + x (pm_gpu_rect.c:161). The window frame
(tattle_window.c:238-243) and the view fills (16C8E0.c:837, :843) are 2D. CAM_TATTLE (viewport 137 95 138 99, goombario.c:1448) is
3D: the picture is 171..344 on screen, the window is 177..315, and the aspect stays 138/99 (cam_main.c:84: 1.25 only for
i <= CAM_BATTLE). That is the 1.25 x stretch. Fix: 3D x = (x + 40) * 4 / 5 for CAM_TATTLE in vp, vpAlt (sprite.c:306-307 read
both) and the 3D scissor. Do not edit cam_main.c:84: aspect 138/99 is then right.
## Edits
1. cam_main.c after pm_wide_draw_x (:509-511):
```c
/* TATTLEAR: the 3D game x that lands on the 2D game x with wide (3D 1.25 x, pm_gpu.c:230; 2D 40 + x, pm_gpu_rect.c:161). up 1 rounds up. */
s32 pm_wide_x3d(s32 x, s32 up) {
    return ((x + 40) * 4 + (up ? 4 : 0)) / 5;
}
```
2. cam_main.c set_cam_viewport, after :480 `    camera->vpAlt.vp.vtrans[3] = 0;`:
```c
    if (pm_wide && id == CAM_TATTLE) { /* TATTLEAR: the 3D view fills the 2D tattle window (40 + x, 1:1): 3D x = (x + 40) * 4 / 5 */
        camera->vp.vp.vscale[0] = (s16) (camera->viewportW * 8 / 5);
        camera->vp.vp.vtrans[0] = (s16) ((camera->viewportStartX * 2 + camera->viewportW + 80) * 8 / 5);
        camera->vpAlt.vp.vscale[0] = camera->vp.vp.vscale[0];
        camera->vpAlt.vp.vtrans[0] = gGameStatusPtr->altViewportOffset.x + camera->vp.vp.vtrans[0];
    }
```
3. 16C8E0.c after :735 `#include "nd_tx.h"`: `extern int pm_wide;` and `s32 pm_wide_x3d(s32 x, s32 up);`. Before :846 `    nd_xf_pnorm(cam->perspNorm);`:
```c
    if (pm_wide) { /* TATTLEAR: the scissor of the 3D draws; the 2D fills above keep the scissor of :821 */
        nd_pso_scis(G_SC_NON_INTERLACE, pm_wide_x3d(cam->viewportStartX, 1), cam->viewportStartY,
                    pm_wide_x3d(cam->viewportStartX + cam->viewportW, 0), cam->viewportStartY + cam->viewportH);
    }
```
4. tattlear.sh in the form of host.d/barricade.sh: awk-extract `s32 pm_wide_x3d(` and `void set_cam_viewport(` (to `^}`) of src/cam_main.c
   into ext.c after `#include "common.h"` and `extern int pm_wide;`. The test defines pm_wide, gCameras[4], gGameStatusPtr. Seeded fault:
   sed `s/\* 8 \/ 5/* 2/g` on ext.c must print `tattlear FAIL` first. Pass prints `tattlear ok 7`.
## Test cases (exact; v = gCameras[id].vp.vp, a = vpAlt.vp, scr(n) = (n * 5 + 8) / 16)
1. wide 1, (CAM_TATTLE,137,95,138,99): v.vscale 220 198, v.vtrans 787 576, a.vscale[0] 220, a.vtrans[0] 787. 2. As 1, altViewportOffset.x 8: a.vtrans[0] 795.
3. wide 1, (CAM_TATTLE,37,95,138,99): v.vtrans[0] 467, v.vscale[0] 220. 4. wide 0, case 1 args: v.vscale[0] 276, v.vtrans[0] 824.
5. wide 1, (CAM_BATTLE,0,0,320,240): v.vscale[0] 640, v.vtrans[0] 640. 6. pm_wide_x3d (137,1) 142, (275,0) 252, (37,1) 62, (175,0) 172; each * 5 / 4 = 40 + x.
7. scr(vtrans[0] - vscale[0]), scr(vtrans[0] + vscale[0]): case 1 gives 177 315, case 3 gives 77 215.
## Gates (1 build, 1 Azahar start; base not charged)
`python 3ds/tools/bench.py base axns_sam2_drec --row TATTLEAR --commit $(git merge-base HEAD main)`, then `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh
--row TATTLEAR --bench axns_sam2_drec --why "TATTLEAR gate" > build3ds/out/tattlear_loop.txt 2>&1`. `LOOP PASS`: host PASS with `tattlear ok 7`,
build rc 0, axns_sam2_drec byte-equal (a battle without tattle is unchanged).
## Fix rights: only the 4 files. A failed case, a DIFF or a compile error in a game file: stop, send the output path to DIAG1.
## Stop: last commit `TATTLEAR 2: gate. tattlear ok 7, axns_sam2_drec byte-equal` deletes this page. One SubagentHandback.
