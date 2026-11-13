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
## Gate (expected values: exact, rule 2)
1. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row FSELLAG1 --why "FSELLAG1"`: PASS.
2. `python 3ds/tools/bench.py base fsel_det_pic` and `base pause_spin_grab_det_w300` at the merge-base, then
   `bench.py run fsel_det_pic,pause_spin_grab_det_w300 --row FSELLAG1 --why "FSELLAG1 byte gate"` (one list, one slot), then
   `bench.py cmp` of each: every shot byte-equal. A difference: the switch needs more state than the scissor: stop and hand back
   the differing shots (names and counts only, no picture read).
3. Seeded fault (a value change, shown first and then removed): in pm_gpu_off_begin change `(float)off_w` in the Mtx_Ortho line
   (:2131) to `(float)(off_w + 1)` (the slot contents scale by one pixel): cmp of fsel_det_pic must show a difference. This
   proves the scene draws through the changed function. Budget 8 Azahar starts.
4. Azahar shows no GPU or CPU cost. After the merge the user checks the file select flip on the O3DS XL (one push).
## Stop and end
A shot difference with the real edit, a seeded fault that shows no difference (the scene does not reach the path), or a tool
fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
