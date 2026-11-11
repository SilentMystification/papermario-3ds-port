# Row SPLASHPREV: the water splash refracts the previous frame (no mid-frame split); Sushie Squirt lag

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## User facts and decision (2026-11-11)
"Sushi's squirt water splash that happens on the enemy is laggy." Decision: refract the previous frame's picture (the splash lags one
frame behind the scene; the picture change is accepted).
## Cause (oracle code read)
- The hit effect is fx_water_splash type 0 (src/battle/1A5830.c:328-329): 21 parts for 24 frames. Each part loads a 32x32 window of
  the frame being drawn: src/effects/water_splash.c:209 `gDPLoadMultiTile(..., OS_K0_TO_PHYSICAL(nuGfxCfb_ptr), ...)`.
- A load from the current frame buffer is a grab of the picture drawn so far (3ds/src/nd_target.c grab_cur :65-87; with the splash
  bracket one full-height grab per frame, :75-80): pm_gpu_grab splits the frame on the GPU and copies 320x240 mid-frame.
- A load from the other (older) frame buffer takes the previous-frame path instead (3ds/src/nd_tex_set.c set_timg :47:
  pm_cfb_capture_prev -> pm_gpu_grab_prev, which sets prev_want = 2; the next frame begins copy the old picture as the first queue
  entry, before the clear (pm_gpu.c, PT12 "prev_want && grab_tex_init"): no mid-frame split while the splash keeps asking.
## Start
`sh 3ds/tools/wt_new.sh SPLASHPREV` (build3ds/wt/SPLASHPREV, branch SPLASHPREV from main). PM_ROW=SPLASHPREV PM_LANE=SPLASHPREV.
Running rows edit other files: HC8 (pm_gpu.c :3796 :2924, nd_model.c, nd_ent.c, nd_shape.c), SYNC60 (pm_gpu.c sync60_pace),
HCCUT1 (pm_gpu.c rect_sig_check, ndm_cmp, pm_gpu_ndm_end), KZN04 (nd_tex_set.c tex_shaded0, pm_tex.c pm_tex_load_shaded). No overlap.
## Touch only
src/effects/water_splash.c: the gDPLoadMultiTile call in the parts loop (:209-211) and two lines above it. No other file.
## Exact edit
Directly above `gDPLoadMultiTile(gMainGfxPos++, OS_K0_TO_PHYSICAL(nuGfxCfb_ptr), ...` (inside the loop, after the temp_t3 block) add:
```
#ifdef TARGET_3DS
        u16* refrBuf = nuGfxCfb[0] == nuGfxCfb_ptr ? nuGfxCfb[1] : nuGfxCfb[0]; /* SPLASHPREV: the previous frame (nuGfxCfbNum 2): no mid-frame grab */
#else
        u16* refrBuf = nuGfxCfb_ptr;
#endif
```
and in the gDPLoadMultiTile call replace `OS_K0_TO_PHYSICAL(nuGfxCfb_ptr)` with `OS_K0_TO_PHYSICAL(refrBuf)`. Nothing else changes
(nuGfxCfb is declared in include/nu/nusys.h:674, included at :2). The gPmNdSplash bracket (:190-192, :223-225) stays.
## Gate (CLAUDE.md 2)
- `PM_ROW=SPLASHPREV PM_LANE=SPLASHPREV sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row SPLASHPREV --why "SPLASHPREV"`: PASS.
- Byte-equal on two scenes with no water splash: spot_mac01_det and mdl5_iwa10_det: `bench.py base <bench> --row SPLASHPREV` (fresh
  bases after LOADMTX, test_catalog.md 6a), run, `bench.py cmp <bench> --row SPLASHPREV`: 6/6 each. One Azahar list, 2 starts.
- The splash itself (Sushie Squirt on an enemy, the look and the fps): the user's console check.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
