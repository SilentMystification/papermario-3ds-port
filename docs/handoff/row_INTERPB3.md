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

## Try 3 (oracle code read of 75804ba05; the orchestrator reset the budget once more: 3 Azahar starts)
Facts of try 2: mono 12/12 byte-equal (L and K copies). Stereo left 6/12: the odd (K) shots 1, 3, 5, 7, 11 each have 102 px in
x65..190 y279..325, and shot 9 has 104 px in x65..214 y4..325. kcopy_btl f511: `kcopy 266 kwalk 34 why 0 0 8 6 0`. kcout: all 8 are
kind patch with addr 0x329xxxxx against cb 0x30189c00, at f164-f191.
Causes and edits:
1. The 8 outside patches (named): blk_build (pm_gpu.c:1157) emits a new model command block into the arena blk_ar with nd_emit_full.
   That emit runs the palette path of nd_emit.c:92-98, which calls nd_kp_vec with o in blk_ar. The patch base then points into the
   arena, not the command buffer. The cache hit copy patches its own K row (blk_put, pm_gpu.c:1185), so the arena patch is extra. It
   comes only when a block is first built: the early battle frames f164-191 with new models. Edit: in blk_build, keep
   `const unsigned kp0 = nd_kp_n;` before nd_emit_full and set `nd_kp_n = kp0;` after it. Do the same in blk_check around its
   nd_emit_draw (debug, the same leak). No other change: the emitted words stay the same.
2. The 20 walks with no counted cause: not named by code read. Every kc_build return is counted. kc_build clears kc_valid at its
   start for every frame end. The key message of step n follows step n's frame end in the walker queue (pm_main.c:497, :502), and all
   300 steps had a key frame (266 + 34). Census edit (no fix): in pm_key_show's fallback, count one of 4 slots `kfb`:
   (a) icmp on; (b) no kc_build since the last key show (a counter kc_gen++ in kc_build, read in pm_key_show); (c) kc_build ran and
   succeeded, but a later kc_build or key show cleared kc_valid before this show (keep kc_ok_gen = kc_gen at success); (d) kc_build ran
   and refused (a why slot counted). Add `kfb a b c d` to the interp line.
3. The stereo K diff, 102 px in one area of the left eye. Mono K copies are now exact, so the patch set is complete for mono. The diff
   is stereo-only. Candidates by read: CPU eye culling uses L's matrices (nd_eye_cull_e and nd_eye_cull_k, set per frame at the
   begin; a model or triangle near an eye plane is kept in one and dropped in the other); and words that differ by eye and are not in
   the patch list. Not named. Measure: new scenario `icmp_mac01_stereo` = `icmp_mac01` with `stereo` added (same until). It compares
   the patched copy with the key walk (icmpp lines, INTERPB2), now with stereo. Insert it right after `icmp_mac01` in both JSON files.
Runs (one list, 3 starts): `bench.py run spot_mac01_fps60_stereo_det,icmp_mac01_stereo,kcopy_btl --row INTERPB3 --why "try 3: arena
patches, fallback census, stereo icmp"`.
Gate of try 3:
- nd_host PASS, memcheck OK. Mono is not rerun (edits 1 and 2 do not change a mono word: 1 only drops arena patches; mono had no
  arena-patch refusal in spot_mac01_fps60_det).
- kcopy_btl: why[2] = 0 and no kcout line. Hand back the interp line with kfb.
- icmp_mac01_stereo: hand back every icmpp line (class counts U A T O, lenmis, unused, the first differing word with its site).
- spot_mac01_fps60_stereo_det: L shots stay byte-equal. Hand back the K-shot px counts (no change expected from edit 1).
Revised pass rule for the row (replaces try 2's): kwalk equals the sum of named causes (why slots plus kfb). kwalk minus 2 x cuts is
at most 3 % of the interpolated steps in mac_01 scenes and at most 5 % in battle scenes: a battle has more mode and camera events,
and each walks 2 frames. Stereo K shots byte-equal, or within the rounding rule (at most 1 step per channel) when the oracle names the
code cause from the icmpp lines. Stop after try 3 with the values; the oracle writes the next step from kfb and icmpp.

## Stop / At the end
A failed gate: commit, hand back the values. Delete this row file in the completing commit.
