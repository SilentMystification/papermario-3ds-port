# Row FSELLAG1: a ROTSCALE window's target switch keeps the native and citro3d state (no full sh_clear), file select lag P2

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## User fact (2026-11-12)
The file select screen is laggy while the save rectangles rotate (build 5101, O3DS XL).
## Code read (ORACLE8, main 4404177f0)
- Each ROTSCALE window draws its contents into an off-screen slot every frame and switches the target twice: pm_gpu_off_begin
  (pm_gpu.c:2051) ends with `sh_clear(); C3D_FrameDrawOn(gs.offs[i].rt);` (:2134), pm_gpu_off_end (:2420-2428) ends with
  `bind_main();`, and bind_main (:2345-2349) does `sh_clear(); C3D_FrameDrawOn(target);` in mono.
- sh_clear (:639) = nd_leave_native() + `sh.has = 0` + pm_tex_bind_reset(). nd_leave_native (:633-638) marks the citro3d
  effect, texenv and texture unit state dirty and resets the program binding (raw1_reset): the next draw sends all of it again.
  With 11 windows that is 22 full re-sends per frame.
- A target switch changes only the frame buffer, the viewport and the scissor (C3D_FrameDrawOn sets the frame buffer and the
  viewport, the viewport call turns the scissor off) and the projection (rebuild_3d, already called at :2133 and :2426). The
  TEV, effect, texture unit and program registers stay as they are on the GPU. The comment at :601-602: sh.has is cleared where
  citro3d may change the state itself. Of the sh fields only the scissor (SH_SC) is changed by the switch.
- bind_main also runs at the frame begin (:2820), where the full reset stays. Only the two switch sites change.
Start: `sh 3ds/tools/wt_new.sh FSELLAG1`. PM_ROW=FSELLAG1 PM_LANE=FSELLAG1 on every call.
## Touch only (places)
3ds/src/pm_gpu.c: line :2134 in pm_gpu_off_begin and the last line `bind_main();` of pm_gpu_off_end (:2427). Not bind_main,
not draw_box.c (plan row NBOX waits), not nd_native_submit (HOSTCORE2, another place: both may run).
## Exact edits
1. :2134: replace `sh_clear(); C3D_FrameDrawOn(gs.offs[i].rt);` with
   `sh.has &= ~SH_SC; pm_tex_bind_reset(); C3D_FrameDrawOn(gs.offs[i].rt); /* FSELLAG1: a target switch changes only the scissor of the sh state */`
2. pm_gpu_off_end: replace `bind_main();` with
   `if (!sr_on || sr_over) { sh.has &= ~SH_SC; pm_tex_bind_reset(); C3D_FrameDrawOn(target); if (eye_gpu != eye_e) eye_put(eye_e); } else bind_main(); /* FSELLAG1 */`
3. Commit: `FSELLAG1: off-screen target switches keep the native and citro3d state, only the scissor shadow is cleared`.
## Correction 1 (ORACLE8, after f90c92162, seeded run 839150): the gate scenes did not reach the rotating windows
- fsel_det_pic is the idle file select (debug `fileselect freshsave shots shotwin=300:6`): no pick, no flip, no ROTSCALE pass.
  pause_spin_grab_det_w300 shoots frames 140-179, after the flip (START at about 80-110, pause_fncov_flip counts 3 off_begin
  calls on f101). So the seed (projection width + 1 in pm_gpu_off_begin) showed no difference: the byte gate saw nothing.
- The pick flip: scene fsel_fncov_pick (`fileselect freshsave padscript=wait400,A*6`), census pick frame f208 with 86
  pm_gpu_off_begin calls per frame (row_ORACLE_NEXT.md, FSELCEN).
- What each proof can show: pixels (shots) show any state that the narrowed switch fails to send again and that changes a
  pixel. drec (HOSTCORE2) records NdDraw at nd_native_submit: the target switch changes no NdDraw, so drec is equal with or without
  the bug: no proof here. Command words (FRAMEHASH3) are not on main. Azahar shows no GPU or CPU cost: the console decides the lag.
## Correction 2 (ORACLE8, after bd352acc8 and the seeded run): the honest gate
- The seed changed off_proj (Mtx_Ortho in pm_gpu_off_begin). off_proj reaches only draws through the post matrix (rebuild_3d,
  pm_gpu.c:223-246, gs.off_active). The window contents (fill, glyphs, rectangles) draw without it: no pixel of the flip changed,
  so the seed did not prove that the scene reaches the path. A pixel seed inside the target switch is not known by code read.
- The change keeps pixels by design (it removes repeated state sends). So the proof is: no pixel changes on the flip frames (done:
  fsel_pick_det_pic 40/40 byte-equal, base 2c32f79f3 two runs equal), plus a reach proof that the flip runs pm_gpu_off_begin with
  this build, plus the user's console check of the lag. No Azahar seed for this row (and FSELLAG2).
## Gate (expected values, correction 2)
1. Done: build, memcheck, nd_host PASS, fsel_pick_det_pic 40/40 and pause_spin_grab_det_w300 40/40 byte-equal (f90c92162).
2. Reach: build the census lane first, `PM_FNCOV=1 PM_ROW=FSELLAG1 PM_LANE=FSELLAG1 sh 3ds/build.sh` (lane FSELLAG1_fncov,
   build.sh:24), then `PM_ROW=FSELLAG1 PM_LANE=FSELLAG1 python 3ds/tools/bench.py run fsel_fncov_pick --row FSELLAG1 --lane FSELLAG1_fncov --why "FSELLAG1 reach"`
   (the census lane of this build: the same pad input as fsel_pick_det_pic). Expected: the census lines of the pick window list
   pm_gpu_off_begin with a count above 0 (oracle census: 86 on f208). Hand back the pm_gpu_off_begin and nd_native_submit lines.
3. After the merge the user checks the file select flip lag on the O3DS XL (one push). Budget: 2 Azahar starts.
## Stop and end
A shot difference, a census with pm_gpu_off_begin 0 in the pick window (the scene does not reach the path), or a tool
fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
