# GATES1 report: pending gates of merged menu, s4 and S3L zmark WIP (nds1, HEAD 816630da9 at start)

Build: build 2249, `PM_ROW=GATES1 sh 3ds/build.sh` OK (3ds/build3ds/pm_3ds.3dsx on nds1 tree). Azahar starts used: 8 of 8 (budget reached, all gates run).

| Gate | Result | Numbers | Shot / evidence |
|---|---|---|---|
| 1 MN0 menu_shot | PASS | `menu: shot bot_m0000 tab 0 row 0`; picture shows General tab, 4 rows, tab line, bottom help line | build3ds\azt\shots\menu_shot\bot_m0000.ppm |
| 2 MN1 menu_pause | PASS | `menu: game paused at frame 94`, `menu: game resumed at frame 94` (equal) | build3ds\azt\shots\menu_pause\bot_m0000.ppm (menu shown) |
| 3 MN3 exit_armed | FAIL (no picture) | armed line OK: `menu: tab 0 row 3 Exit to Homebrew Menu = Press A again` (log #144). Frame-window shots 200 to 205 never written: the menu freeze (MN1) holds the frame at 94, log silent 10 s, run stopped. Blank line, blue label and red value not checked by eye. | none from this run. build3ds\azt\shots\exit_armed\ bot_0200..0205.ppm are from an older run (01:17), not used. |
| 4 WARPY iwa_10 | FAIL | Floor found but at y 0, gate expects y above 0. Lines (run 5, no prof=4): `poswarp probe f30 mario 0.000 -1071.668 19.600 ray y 0.000 id 28 ground y -1071.668 id -1`; `poswarp: floor y 0.000 -> 0.000`; `poswarp: placed Mario at -662.000 0.000 587.000 on map iwa_10 settled -662.000 0.000 587.000 yaw 90.00 (readback)`; `poswarp probe f60 mario -662.000 0.000 587.000 ray y 0.000 id 28 ground y 0.000 id -1 colliders 100`. No probe at f0, f120, f180 (run stopped at shot frame 111). Scene line: `warp: scene frame0 55 scan 70`, `warp: scene f300 ... pos -662 0 587`. Run 4 (prof=4): no poswarp line, no shot file (prof=4 suppresses pm_log and shot writer). Shot is frame 111 = 56 frames after frame0 55, not 60. | build3ds\shots\0000_8_5.ppm (frame 111). Picture: rock terrain and blue sky; Mario NOT visible. User also saw Mario under the world. |
| 5 S4-1 sprcheck mac_01 | FAIL | `sprcheck f300 recs 3121 eq 0 diff 2080 fb 1 why 0 dfield 1`. Gate needs eq above 0, diff 0. dfield 1 = key (spr_cmp: `a->tmpl != b->tmpl`, the template key). Values of tmpl (native and walker) are not logged: a value print is needed to name them (not added, row rule). | no picture (prof=4 run, no shot) |
| 6 S3L zmark iwa_10 | PASS (picture by eye, zmark count from run 4) | ndmcheck non-prof run: `ndmcheck f300 recs 29841 eq 1216 diff 0 fb 1453 nodraw 27172`; ndmcheck prof run: `recs 20936 eq 1102 diff 0 fb 1309`. Zmark refusal: `ndmr2 f300 c11 0 c12 0 c13 0 c14 2381 c15 0` (run 4, same build, prof=4, no ndmcheck: the ndmr line is not printed with ndmcheck on). c12 zmark 647 before, 0 now. Code 12 total (`ndm ... why 12:1365`) is a broader class (zmark, decal+blend, wide). | build3ds\shots\0000_8_5.ppm (same frame as gate 4): rock terrain, sky, no wrong depth or broken edge seen. Mario absent from frame (see gate 4), so "no missing object" is only checked for the scene. |
| Host nd_host | PASS | run 205, rc 0, `ND HOST TESTS PASS`, seam step 2 PASS. Warning only: tools/nd_cmd_test.c:60 `d_graw` unused (not fixed). | host log in bench run 205 |

## Faults for the coordinator

1. MN1 freeze vs frame-window shots (gate 3). The freeze holds frame at 94 while the menu is open, so any scenario that shoots at fixed frames while the menu is open (exit_armed, shotbottom=200:6) never fires. Fix needs a per-draw shot (shotmenu) or a scenario change. Not changed here.
2. WARPY (gate 4). Mario is not placed on a floor at iwa_10: floor query returns y 0, the picture has no Mario. Cause not found; the floor ray hits collider id 28 at y 0, the ground query returns id -1. The gate expectation (y above 0) is unverified.
3. S4-1 sprcheck (gate 5). Diff in key (template) field on mac_01: 2080 of 3121 records. Need a value print of tmpl (native, walker) to name the mismatch.
4. azt.ps1 static until check cannot read a `%s` format: scenario menu_shot had until `menu: shot bot_m0000` (rejected, exit 7). Fixed in 3ds/tools/azt_scenarios.json to `menu: shot \\w+ tab 0 row 0` (commit by itself).

## Notes

- No bench spot_iwa10_pic or WARPY / sprcheck / iwa_10 ndmcheck bench exists: run_azahar.ps1 direct, as the row allows.
- prof=4 suppresses the shot writer and all pm_log lines except prof lines, ndmcheck, sprcheck, ndmr. Use it only when no picture is needed.
- Status lines: 3ds/logs/perf/status_GATES1.txt (8 starts plus one rejected azt static check).
- Budget reached (8/8). Not reset.
