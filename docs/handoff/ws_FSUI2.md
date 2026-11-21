# ws_FSUI2 (Haiku): Fullscreen game reads of the Widescreen output rules (3D to 2D x, screen bounds, tattle view, box frustum, effects)
Designer: the oracle in the launch prompt. Worktree build3ds/wt/FSUI2, branch FSUI2. PM_ROW=FSUI2 PM_LANE=FSUI2. Start: the FSUI1 gate commit or main after `Merge FSUI1`.
Counter: none of C1 to C12. Objective (plan_widewin.md section 6): game code that places 2D against the 3D picture (3D 1.25 x, 2D 40 + x) does it in Fullscreen too. Layout reads (pm_wide_left(54), wide boxes, field of view) stay pm_wide. With pm_stretch 0 every edit gives the old value. Grep the old text before each edit. Use the Edit tool (CRLF).
## Directory order (then edits): include/pm_wide.h, include/PR/gbi.h, src/cam_main.c, src/screen_overlays.c, src/screen_render_util.c, src/draw_box.c, src/hud_element.c, src/msg_draw.c,
src/battle/16C8E0.c, src/effects/bulb_glow.c, src/effects/star_spirits_energy.c, src/effects/underwater.c, 3ds/tests/host.d/tattlear.sh, 3ds/tests/tattlear/tattlear_test.c. No other file.
1. pm_wide.h, after `extern int pm_wide;` (:4), add 3 lines: `extern int pm_wout; /* FSUI1: pm_gpu.c, Widescreen or Fullscreen output (3D 1.25 x, 2D 1:1 at 40 + x) */`,
   `static inline int pm_scr_left(void) { return pm_wout ? -40 : 0; } /* FSUI2: game x of the left screen edge */` and `static inline int pm_scr_right(void) { return pm_wout ? 360 : 320; } /* FSUI2: the right edge (exclusive) */`
2. Bounds shape: `pm_wide_left(0)` becomes `pm_scr_left()`, `pm_wide_right(SCREEN_WIDTH)` becomes `pm_scr_right()`. Example draw_box.c:436 before
   `if (posX >= pm_wide_right(SCREEN_WIDTH) || posY >= SCREEN_HEIGHT) {` after `if (posX >= pm_scr_right() || posY >= SCREEN_HEIGHT) {`. Sites: draw_box.c:436, :440;
   hud_element.c:804 (2 tokens), :808, :809, :813; msg_draw.c:344, :346; screen_render_util.c:229, :230, :255, :256, :264, :265. Do not edit inventory.c (layout reads).
3. Rename shape: the token `pm_wide` becomes `pm_wout`. Example draw_box.c:469 before `guFrustumF(mtx1, pm_wide ? -100.0f : -80.0f, pm_wide ? 100.0f : 80.0f, ...` after the same line with
   `pm_wout` twice. Sites: draw_box.c:469 (2); screen_render_util.c:228, :251, :255, :256 (`pm_wide && (`); 16C8E0.c:849 `if (pm_wide) {` (add `extern int pm_wout;` after :737);
   bulb_glow.c:202-203 and star_spirits_energy.c:391-392 (`extern int pm_wide;` and `if (pm_wide) {`); underwater.c:172, :194, :196; cam_main.c:478 `if (pm_wide && id == CAM_TATTLE)`;
   gbi.h:2511-2514 (the extern and the 3 PM_SCIS macros: 4 tokens).
4. cam_main.c:9: after `extern int pm_wide; /* F07-2 */` add `extern int pm_wout, pm_stretch; /* FSUI2: pm_gpu.c */`. After the `}` of pm_wide_cam (:14) add:
```c
s32 pm_wout_cam(Camera* c) { /* FSUI2: the 3D draw is at 1.25 x and a 2D draw at 40 + x: the wide full view, or the main and battle views in Fullscreen */
    return pm_wide_cam(c) || (pm_stretch && (c == &gCameras[CAM_DEFAULT] || c == &gCameras[CAM_BATTLE]));
}
```
   cam_main.c:504 `if (pm_wide_cam(camera)) {` and :513 `return pm_wide_cam(camera) ?`: write `pm_wout_cam` for `pm_wide_cam`. Keep :16 (the lead ratio). screen_overlays.c:14: after `extern s32 pm_wide_cam(Camera* c);` add `extern s32 pm_wout_cam(Camera* c);`; :379 `if (pm_wide_cam(camera)) {`: write `pm_wout_cam` for `pm_wide_cam`.
5. tattlear.sh: in the printf prefix add `extern int pm_wout, pm_stretch;` after `extern int pm_wide;`. Before the pm_wide_x3d awk line add 3 awk lines of the same form for
   `^s32 pm_wide_cam\(`, `^s32 pm_wout_cam\(`, `^s32 pm_wide_draw_x\(`. `mkdir -p $N/seed` becomes `mkdir -p $N/seed $N/seed2`. After the first seed check add seed 2:
   `sed 's/(pm_stretch && (c ==/(0 \&\& (c ==/' $N/ext.c > $N/seed2/ext.c`, a cmp check (`tattlear seed 2 not made`), a gcc of `$N/seed2_t` as for seed 1, and
   `case "$($N/seed2_t | head -1)" in "tattlear FAIL 9"*) ;; *) echo "tattlear seed 2 not seen"; exit 1;; esac`.
6. tattlear_test.c: after `int pm_wide;` add `int pm_wout; int pm_stretch;` and `s32 pm_wide_draw_x(Camera* camera, s32 x);`. On the 4 old lines `pm_wide = 1;` or `pm_wide = 0;`
   (cases 1, 4, 5, 7) append ` pm_wout = pm_wide;`. Before the printf add cases 8 and 9; the printf becomes `tattlear ok 9`. Name `tattlear ok 9` and 2 seeds in both header comments.
```c
    pm_wide = 0; pm_stretch = 1; pm_wout = 1; set_cam_viewport(CAM_TATTLE, 137, 95, 138, 99); /* 8: Fullscreen tattle window of case 1 */
    if (chk("8 vscale", c->vp.vp.vscale[0], 220) || chk("8 vtrans", c->vp.vp.vtrans[0], 787)) return 1;
    set_cam_viewport(CAM_DEFAULT, 12, 20, 296, 200); if (chk("9 view x", gCameras[CAM_DEFAULT].viewportStartX, 12) || chk("9 draw_x 32", pm_wide_draw_x(&gCameras[CAM_DEFAULT], 32), 0) ||
        chk("9 draw_x 300", pm_wide_draw_x(&gCameras[CAM_DEFAULT], 300), 335) || chk("9 tattle x", pm_wide_draw_x(&gCameras[CAM_TATTLE], 100), 100)) return 1;
    pm_stretch = 0; pm_wout = 0; if (chk("9 native x", pm_wide_draw_x(&gCameras[CAM_DEFAULT], 32), 32)) return 1;
```
## Tests (output to build3ds/out/ of the worktree, read the last 20 lines). 1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS with `tattlear ok 9`. 2. `PM_LANE=FSUI2 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: calls 0 interp 0 state 0.
3. Native: `bench.py base msgbox_talk_nowide --commit $(git merge-base HEAD main)`, `bench.py run msgbox_talk_nowide --row FSUI2 --why "native unchanged"`, `bench.py cmp msgbox_talk_nowide --row FSUI2`:
   byte-equal; the log holds `gpu: wide=0 stretch=0`.
4. Fullscreen: `bench.py base msgw_full_drec --commit $(git merge-base HEAD main)`, `bench.py run msgw_full_drec --row FSUI2 --why "fullscreen 3d to 2d x"`, `bench.py cmp msgw_full_drec --row FSUI2 --expect fsui`:
   `expected-diff PASS`; the log holds `gpu: wide=0 stretch=1` and every `clipg f330` line has `out 0`.
Fix rights: the 14 files. A compile fault from a page edit: fix it, note it in the commit. Any other FAIL or DIFF: send the output path to the oracle in the launch prompt. Stop: commit `FSUI2 1: gate` with this page deleted. Hand-back: one SubagentHandback: commit hash, host result, c0gate, both cmp lines, the clipg f330 lines.
