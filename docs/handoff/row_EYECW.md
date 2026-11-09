# Row EYECW: stereo convergence follows camera yaw (mim_01 ring, goomboss "wrong rotation")

## User facts
User (2026-11-09): mim_01 inner ring looks like it rotates wrong when Mario runs left and right, same as the goomboss castle (kmr_11).
Open: the user has not said if the 3D slider was up. The cause below shows only with the slider up.

## Cause (oracle, code trace)
- 3ds/src/nd_mtx.c:93 nd_mtx_combine: cw = conv * |p[2][3]| with p = nd_proj, conv = pm_eye_conv = 300 (pm_gpu.c:88).
- nd_proj is view x perspective (src/cam_main.c:84-94, loaded as G_MTX_PROJECTION), so p23 = -V22 and |p23| = |cos(yaw)cos(pitch)|.
  At yaw near +-90 cw is about 0: the convergence plane swings twice per lap, so depth flattens and returns.
- Mono (e = 0) is not affected (nd_mtx.c:92 skips the shift). 3ds/tools/nd_mtx_test.c:56-67 tests only a pure perspective.
- Cleared paths: billboards, SR2 matrix cache, MAPX key, gm_tab, box tests, model matrices, task sort, background, pm_gu math.

## Fix
1. nd_mtx.c: float nd_proj_wn = sqrtf(p03^2 + p13^2 + p23^2), recomputed in nd_mtx_load on each projection load or multiply,
   0 in nd_mtx_reset. nd_mtx_combine: cw = conv * nd_proj_wn; perspective test = nd_proj_wn != 0. Delete the old formula.
2. One sqrtf per projection load, none per G_MTX, no division (hw_guidelines).
3. nd_mtx_test.c: load nd_proj through nd_mtx_load; add test_combine_view (yaw 0/45/90/180, pitch -20): point at depth conv has
   x shift 0 (1e-4), far point near +e, near point opposite sign. Old code fails at yaw 90.
4. STEREO S1 (shader eye shift Dc = persp*cw*post_col0) uses cw = conv * nd_proj_wn.

## Gates
a. nd_host PASS with test_combine_view.
b. Mono byte-equal to main: spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det.
c. Stereo shots spot_mac01_stereo_det_l/_r change (expected): two runs of the new build match, both eyes correct.
d. New scenarios + benches in the same commit: mim01_ring_stereo_det_l/_r (warp=mim_01:1, stereo det camlog, debug log line
   "eyecw f<n> p23 <v> cw <v>" every 10 frames when e != 0, padscript go_-375_0_-375,wait10,go_-375_0_375, ~12 shots, cap 30 s)
   and mono mim01_ring_det. Pass: camlog yaw passes 90, eyecw stays about 300, Mario and trees in both eyes.
e. kmr11_cut (from commit 61d04a541) + kmr11_cut_stereo_det_l/_r: cw about 300 in f41-61 and f241-261.
f. memcheck OK. g. Console (user, slider up): mim_01 ring and the goomboss cutscene.
