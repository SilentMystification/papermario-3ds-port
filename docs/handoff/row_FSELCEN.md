# Row FSELCEN: walker census of the file select flip and the pause flip (measurement, no code edit)

(Model: Haiku: 4 scenario copies and a census read, no code. Runs in parallel with S4E3D, S4SNAK, S4WBLK: it appends at the end of
the two JSON lists; the S4 rows insert after named fxpic/fxfncov entries.)

## Why (user fault: file select entry is slow, the flip on a file pick is very laggy; oracle code read)
Both flips draw windows with DRAW_FLAG_ROTSCALE (filemenu_common.c:648, :669; up to 11 windows at once: filemenu_main.c:1108-1118,
filemenu_createfile.c:589-599; entry: windows.c:130-134 appear flags; pause: pause_main.c:59-62 the same path). Per window per frame
(draw_box.c:432-830): an off-screen pass for the contents (draw_box.c:770-786 -> nd_target.c:92 -> pm_gpu_off_begin pm_gpu.c:2022:
flush, a full 512 x 256 slot clear :2089, C3D_FrameDrawOn, rebuild_3d; pm_gpu_off_end :2383: flush, rebuild_3d, bind_main), then
height/6 strips (draw_box.c:801-820), each a texture load of the slot, a tile size, a matrix multiply and one 2-triangle draw. Only 3
full slots exist (pm_gpu.c:2059-2068: the 4th and later images reuse a slot, pm_log_e "V05 demand"). This row counts which part
costs on core 0 before a fix row is written.

## Start
`sh 3ds/tools/wt_new.sh FSELCEN`. PM_ROW=FSELCEN PM_LANE=FSELCEN. Build the census lane: `PM_FNCOV=1 PM_ROW=FSELCEN PM_LANE=FSELCEN
sh 3ds/build.sh`.

## Touch only
3ds/tools/azt_scenarios.json and 3ds/tools/bench_defs.json: 4 entries each, appended at the end of each list. One commit.

## Scenarios and benches (kind azahar_stream like fxfncov_50, no shots, until `fncov f\d+ done`, cap 45, deadline 40)
1. `fsel_fncov_rot`: copy of `fileselect_rotate` with debug `fileselect fncov=289 loglevel=debug det` (the entry turn, frames 282-296).
2. `fsel_fncov_idle`: the same with `fncov=400` (file select at rest).
3. `pause_fncov_flip`: debug `preset=toad padscript=wait80,START*3 fncov=99 loglevel=debug det` (the pause opens at frame 96).
4. `pause_fncov_idle`: the same with `fncov=200`.

## Commands
- `python 3ds/tools/bench.py run fsel_fncov_rot,fsel_fncov_idle,pause_fncov_flip,pause_fncov_idle --row FSELCEN --lane FSELCEN_fncov
  --why "file select and pause flip walker census"` (one list, 4 starts).
- `python 3ds/tools/hotset.py --fncov <log> --fncovfn --lane FSELCEN` on each of the 4 logs, output to build3ds/out/ of the worktree.

## Gate (measurement)
- All 4 logs have the `fncov f<n> done` line and the census lines.
- fsel_fncov_rot has a pm_gpu_off_begin count of 1 or more. If it is 0, run `fsel_fncov_rot` once more with `fncov=285` (one try). The
  same for pause_fncov_flip with `fncov=97`. Still 0: stop and hand back.

## Try 2 (oracle code read; the orchestrator reset the budget once)
- Facts of try 1: fsel_fncov_rot f285 and f289, and fsel_fncov_idle f400: the same draw set, with pm_gpu_off_begin 0. pause_fncov_flip
  f99: 2069 calls against 63556 at rest.
- Cause: a fixed fncov frame misses the flip. The entry appear is 4 frames of ROTSCALE (windows.c:189-195, counter 0-3) at a frame no
  code fixes. The pick flip runs only after an A press on an empty file (filemenu_main.c:1094-1118, hidden_with_rotation for 11
  windows, 10 frames: filemenu_common.c:667). f99 in the pause is not a valid flip window: 2069 calls is a frame before the pause
  panel draws. Only frames with a ROTSCALE box whose contents go to an off-screen pass (draw_box.c:768-782) are the subject.
- Tool edit (debug, census only):
  1. 3ds/src/pm_fncov.c: switch `fncovrot=<min frame>`. In pm_fncov_walk's init: if the switch is set, fn_on = 1,
     fn_min = atoi(value), fn_want = 0xFFFFFFFFu. New `void pm_fncov_event(unsigned f)`:
     `if (fn_on && !fn_active && !fn_done && fn_want == 0xFFFFFFFFu && f >= fn_min) fn_want = f;` (mark it no_instrument_function
     like the other hooks).
  2. src/draw_box.c, inside `if (quads != nullptr) {` of the contents branch (:769-771), before mdl_get_next_texture_address:
     `#ifdef TARGET_3DS` `{ extern void pm_fncov_event(unsigned); extern unsigned pm_frame_n; pm_fncov_event(pm_frame_n); }` `#endif`.
     pm_frame_n during the build is the frame the walker entries of this step carry (nd_wq.c:124), so the census arms at the walk
     of the frame that holds the flip. With det the step is built before its walk (serial mode), so the arm comes in time.
- Scenarios (replace the 4 entries of try 1, the same names; det kept; cap 45):
  1. `fsel_fncov_rot`: `fileselect fncovrot=1 loglevel=debug det` (the first rotating box with contents: the entry appear).
  2. `fsel_fncov_pick`: `fileselect freshsave padscript=wait400,A fncovrot=400 loglevel=debug det` (File 1 is empty with
     freshsave; the cursor starts on File 1; the A press starts the 11-window flip; the first rotating content box at or after f400).
  3. `fsel_fncov_idle`: unchanged (`fncov=400`).
  4. `pause_fncov_flip`: `preset=toad padscript=wait80,START*3 fncovrot=90 loglevel=debug det`; `pause_fncov_idle` unchanged.
  Until for all: `fncov f\d+ done`. Rename nothing else. hotset: `--lane FSELCEN` (the normal lane map, as the coder found).
- Runs: one list of 4 (rot, pick, flip, plus one spare). Gate: each census shows pm_gpu_off_begin >= 1 (rot and pick at least 1 per
  rotating window with contents: about 11 in pick). Expected in pick: off_begin about 11, rebuild_3d about 22, ensure_tile_slow and
  pm_tex_load grow with the strips (height/6 per window). 0 in pick: stop and hand back the log tail (the A press may not have
  reached the menu).

## Hand back (values only, no fix)
- Per pair (rot minus idle, flip minus idle): the 25 largest call deltas by function, and the counts of pm_gpu_off_begin,
  pm_gpu_off_end, flush, flush_draw, rebuild_3d, bind_main, ensure_tile_slow, pm_tex_load, gv_tri, nd_ops_run in each log.
- The count of "V05 demand" lines in the fsel_fncov_rot log.
- Commit the scenarios and benches (the census files stay in build3ds/out of the worktree). Delete this row file in the completing commit. Budget: 6 starts.
