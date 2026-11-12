# Row INTERPB3: design B step 3: show the patched copy as the key frame (no second walk), stereo per stream

(Model: Sonnet (user approved). After INTERPB2 is merged; same coder. Design: row_ORACLE_NEXT.md, option B and the stereo note.)

## Design
- pm_step_run (pm_main.c:496, :502) queues the key frame as today, but the walker message runs pm_key_show instead of pm_key_walk
  (pm_nusys.c:191-207) when the L frame of the step built a valid K copy (INTERPB2: no overflow, no lost entry, patch list complete):
  pm_gpu_begin(1) as pm_key_walk does, queue the K copy's GX entries in the recorded order (the stereo replay's queue form, sr_replay
  pm_gpu.c:2111-2135: command lists from the K buffer, fills and copies as recorded), pm_gpu_end. An invalid copy falls back to
  pm_key_walk for that step (count it).
- Stereo: the L frame's recorded entries include its right-eye replay (bind_main swap :2066-2089, sr_map :2091); the K copy is taken
  after that replay, so K's queue holds both eyes with K's patched lists; the right-eye bind lists carry only the eye uniform and
  targetR (no matrix), so they need no patch.
- Lifetime: the vertex ring chunks and the command list memory of L stay until the K frame is done (ring stamps cover two frames:
  nb.ring_fr and ring_done, pm_gpu_rect.c:28-58; the K buffer is its own memory, two of them alternate).
- INTERP1 limits (pm_main.c interp_heavy): the walk now runs once per step: the walk limit becomes 26 ms (calm under 20 ms); the GPU
  limit stays 13 ms (two GPU frames per step). Census line interp gets `kcopy <n> kwalk <n>`.

## From INTERPB2 (the copy is built in INTERPB3, INTERPB2 only patched per list while reading)
- The K copy needs the whole frame: the queued lists and the tail. Build it at the L frame end after C3D_FrameEnd has closed the last
  list and before the command buffer is reused: record every GX entry of the L frame as it is taken (the icmp_take / sr_take point,
  before each mid-frame clear by pm_gx_room, pm_gx_drain and the KICK1 kicks), copy the words of each command list entry into the K
  buffer, apply the patches by their offset in the frame's command buffer, and queue the K frame from the recorded entries with the
  list addresses moved into the K buffer (fills, copies and transfers as recorded: INTERP2M T 0 shows the same targets).
- The patch offsets are relative to the frame's command buffer; a frame whose command buffer wrapped or was split into a second
  buffer (cmd_split_forced) takes the fallback (pm_key_walk) in this row; count it.

- Scene changes (INTERPB2 decision b): the frames around a cut differ in structure (mac01 at the warp: 14139 against 10545 words).
  Fall back to pm_key_walk for the 2 steps after a map change, a game mode change or a camera cut: a counter set in pm_ph_scene_map and
  pm_ph_scene_mode (pm_phase.c:70-75) and in pm_interp_dont_camera (pm_interp.c:79), counted as kwalk in the interp census.
- While the camera moves the copy shows L's draw set with K matrices (CPU culls on L matrices): accepted design limit; the user judges
  the edges on the console.

## Start
`sh 3ds/tools/wt_new.sh INTERPB3` (from main with INTERPB2). PM_ROW=INTERPB3 PM_LANE=INTERPB3.

## Touch only
3ds/src/pm_nusys.c (pm_key_walk, the new pm_key_show), 3ds/src/pm_gpu.c (the K queue, fallback test, lifetimes), 3ds/src/pm_main.c
(interp_heavy limits, census fields), 3ds/src/nd_wq.c only if the walker message needs a new kind.

## Gate (CLAUDE.md 2-3; console in the batch run)
- Build, memcheck OK; nd_host PASS.
- Scenario spot_mac01_fps60_stereo_det: copy of spot_mac01_fps60_det with the switch `stereo` (forced, both eyes: shots of both
  targets as the nok11_det_stereo_l/_r benches take them). Bases stored on main for spot_mac01_fps60_det and the new scene.
- One list: `bench.py run spot_mac01_fps60_det,spot_mac01_fps60_stereo_det,icmp_btl --row INTERPB3 --why "key frame from the patched
  copy, no second walk"`. Pass: both scenes byte-equal to their bases (the K shots now come from the copy); the interp census shows
  kwalk 0 and kcopy = the interpolated steps on icmp_btl after frame 300. Budget: 6 Azahar starts.
- Console (orchestrator): the batch XL run with fps60 on: interp lines (key_us 0, kcopy) and the frame time against the INTERP1 run.

## Try 2 (oracle code read of 23e092261 and 60bbc5601; budget reset once by the orchestrator: 3 Azahar starts)
Facts of try 1: kcopy_btl `kcopy 267 kwalk 34 why 0 0 8 6`; spot_mac01_fps60_det 0/12 (about 91750 px, whole screen, even shots
too); spot_mac01_fps60_stereo_det 6/12 (odd shots only, 102 px, x65..190 y279..325 of the rotated shot); stereo_det_r 11/12 (1 px).
Causes (code read):
1. Mono L frames differ, stereo L frames are equal. The only head change that runs in mono L and not in stereo L is the new
   `if (kc_arm && !stereo_frame) sr_force_all();` in pm_gpu_twin_begin (pm_gpu.c:198). It runs at the walk start, after the frame
   begin made bind_main and the PT12 grab draw (pm_gpu_begin_wait). sr_force_all marks every citro3d vertex uniform dirty
   (C3D_FVUnifDirty) and resets the native shadow. The next citro3d draw then uploads citro3d's CPU copy of the uniforms, which is
   stale for registers the grab draw and the native path wrote. Stereo calls the same function in sr_frame_begin, before bind_main
   and the grab, and its L shots are byte-equal: that is the place where the force is safe.
2. The K copy frame skips pm_gpu_begin_wait, so the two noise_next() calls of a frame begin (PT07) do not run. The noise sequence of
   every later frame shifts against the base, and the det line's noise value differs. This is not the cause of the shot diffs here,
   but it is a real divergence.
3. The odd (K) stereo shots differ only in one object (102 px). The K copy has L's words wherever the patch list has no K word. In
   INTERPB2 the cmul and cadd sites were left without a twin, and those values depend on the matrices. That class is the expected
   cause, but this try does not prove it (step 3 below only measures).
4. why[2] 8 (a list or a patch outside the copied range): no cause is found by code read. GPUCMD_Split, the stereo bind lists and
   the off-screen passes all stay in the frame's command buffer. These frames fall back to the key walk, so the picture is right
   and only the cost is wrong. This try measures it.
5. kwalk 0 is the wrong pass rule. A scene cut walks 2 key frames by design (kc_cut), so kwalk >= 2 x cuts. Also kwalk 34 against
   why sum 14 leaves 20 walks with no counted cause: an L frame end with kc_rec or kc_arm 0.
Edits:
a. pm_gpu.c: delete the sr_force_all call in pm_gpu_twin_begin. In sr_frame_begin replace `if (!stereo_frame) return;` by
   `if (!stereo_frame) { if (pm_interp_on > 0) sr_force_all(); return; }` (the same point as stereo; every fps60 mono frame).
b. pm_gpu_key_show: after C3D_FrameBegin, call noise_next() twice, as a frame begin does.
c. Census: pm_kc_why gets a fifth slot, "no armed L frame": counted in kc_build at the `!kc_rec` and `!kc_arm` returns, only when
   pm_icmp_kind == 1. Add it to the interp line. For why[2], log the first 8 per run with pm_log_hw: `kcout f<n> kind list|patch i
   <index> addr <a> n <words> cb <cb> coff <coff> split <cmd_split_n - kc_split0>`.
Gate of try 2 (one list, 3 starts: `bench.py run spot_mac01_fps60_det,spot_mac01_fps60_stereo_det,kcopy_btl --row INTERPB3 --why "try 2:
force at frame begin"`; base 20d613506 as before):
- nd_host PASS, memcheck OK.
- Mono and stereo: every even shot (the L frames) is byte-equal. Odd shots (K copies): at most 150 px per shot, all inside the area
  of try 1 (x65..190, y279..325 in stereo). Hand back the diff counts and bounding boxes per shot.
- kcopy_btl: kwalk = why[2] + why[3] + why[4], and why[0] = why[1] = 0. Hand back the interp line and the kcout lines.
- The pass rule for the row (replaces kwalk 0): kwalk equals the sum of the named causes, and kwalk minus 2 x cuts is at most 3 % of
  the interpolated steps.
Stop after try 2 with the values in all cases. The oracle then writes the K-site step (the patch for the class that the icmp census
names for the odd-shot area) or a fix for the outside lists from the kcout lines.

## Stop / At the end
A failed gate: commit, hand back the values. Delete this row file in the completing commit.
