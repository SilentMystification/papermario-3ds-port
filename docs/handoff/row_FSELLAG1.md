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
## Gate (expected values, correction 1)
1. Build, memcheck OK. nd_host PASS. Code f90c92162 stays.
2. New scenario `fsel_pick_det_pic` in 3ds/tools/azt.d/FSELLAG1.json (all fields as fsel_det_pic): debug `fileselect freshsave
   padscript=wait400,A*6 shots shotwin=200:40 loglevel=debug det`, until `shot \d+ frame 239\b`, cap 45, deadline 25,
   first_marker `boot_main`, expects `none`, needs_picture false, expected_events [{name "file select mode", regex `game mode 14
   frame`}, {name "shot 239", regex `shot \d+ frame 239\b`}]. Bench `fsel_pick_det_pic` in bench.d/FSELLAG1.json, kind
   azahar_pic, default false, cap 45, the same until.
3. Seeded fault first (projection width + 1, as before): `bench.py base fsel_pick_det_pic` at the merge-base, run the seeded
   build, `bench.py cmp fsel_pick_det_pic --row FSELLAG1`: at least one shot differs (the scene reaches the flip). If all 40 are
   equal: stop and hand back (the pick is outside frames 200-239: hand back the `game mode` and pad lines of the log).
4. Real edit: run, cmp: 40/40 byte-equal. Budget: the orchestrator resets it (row spec fault).
5. The user checks the flip lag on the O3DS XL after the merge (one push).
## Stop and end
A shot difference with the real edit, a seeded fault that shows no difference (see gate 3), or a tool
fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
