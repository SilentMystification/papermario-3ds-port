# Row BOOTSTATE: one log entry of the global draw inputs at the shot frames, to name the input that differs between base runs

(Model: Haiku. 50 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main of the time, BASEKEEP evidence)
- `bench.py base fxpic_89` at db17115ac: two runs differ, every one of the 6 shots by the same 84455 pixels, bbox x 0..220,
  y 0..399 of 240 x 400: a global difference (layout, camera, a full-screen colour), the same in every frame. spot_mac01_det
  and spot_mac01_fps60_det failed the same way before (0000_1_2 first) and stored equal pairs at a3ffbbe8b.
- Read and ruled out: options.ini (run_azahar.ps1:231 copies the user's master file into the slot for every run, the master is
  unchanged since 2026-11-08: wide=1 fps60=0), the boot order (pm_options_init pm_main.c:944 runs before the game thread
  and pm_gpu_init, pm_nusys.c:29), the seed (pm_os.c:55: det sets salt 0), the shot frames (walker frames). No producer found.
- The kept folders may not be read as pictures (CLAUDE.md 2). So the game prints the inputs, and the two runs are compared as
  text: an exact compare that names the input.
Start: `sh 3ds/tools/wt_new.sh BOOTSTATE`. PM_ROW=BOOTSTATE PM_LANE=BOOTSTATE on every call.
## Touch only (places)
3ds/src/pm_main.c: one new block next to the shotbottom probe (:813-818). Nothing else.
## Exact edits
1. pm_main.c probe, debug switch `bootstate` (read once): at every frame of the shot window (shot_w0 is in pm_gpu.c: use the
   frames given by the switch value `bootstate=<first>:<count>`), print one entry:
   `pm_log_evt("bootstate f%u wide %d stretch %d interp %d optwide %d fps60 %d seed %08x cam %d %d %d tgt %d %d %d mario %d %d %d
   mode %d", frame, pm_wide, pm_stretch, pm_interp_on, pm_opt.wide, pm_opt.fps60, (unsigned)gRandSeed, (int)gCameras[0].lookAt_eye.x,
   (int)gCameras[0].lookAt_eye.y, (int)gCameras[0].lookAt_eye.z, (int)gCameras[0].lookAt_obj.x, (int)gCameras[0].lookAt_obj.y,
   (int)gCameras[0].lookAt_obj.z, (int)gPlayerStatus.pos.x, (int)gPlayerStatus.pos.y, (int)gPlayerStatus.pos.z, get_game_mode());`
   (declare what the file lacks with extern lines, as pm_main.c does for other globals).
2. Commit: `BOOTSTATE: bootstate probe of the global draw inputs (layout, interp, seed, camera, Mario)`.
## Gate (expected values)
1. Build, memcheck OK. nd_host PASS.
2. A per-row scenario copy of fxpic_89 named fxpic_89_bs with `bootstate=412:4` added (azt.d/BOOTSTATE.json, bench.d/BOOTSTATE.json,
   all fields as fxpic_89). Run it 3 times on one slot: `bench.py run fxpic_89_bs --row BOOTSTATE --why "BOOTSTATE run <k>"`, and
   after each run copy build3ds/azt/logs/fxpic_89_bs.txt to build3ds/out/bs_run<k>.txt (the next run overwrites it). Then
   `grep bootstate build3ds/out/bs_run*.txt` and `bench.py base fxpic_89_bs --commit <this row's commit> --row BOOTSTATE`. Hand
   back exactly: the 12 bootstate lines (3 runs x 4 frames) and the BASEKEEP result line. A field that differs between runs names
   the input. All equal: hand back that too.
3. Seeded fault (a value change, first): print `frame + 1` in place of `frame`: the lines must show f413..f416. Then remove it.
4. Budget 6 Azahar starts.
## Stop and end
A tool fault, or a line that does not match the text above: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this
row file in the completing commit.
