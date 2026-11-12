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

## History (tries 1-3, code 540e96f5f)
- Try 1-2: sr_force_all moved to sr_frame_begin (every fps60 frame), noise_next twice in pm_gpu_key_show, census slots why[0..4] and
  kcout. Mono spot_mac01_fps60_det 12/12 byte-equal since try 2.
- Try 3: arena patches dropped in blk_build and blk_check (why[2] 0 now), kfb census, scenario icmp_mac01_stereo.
- Facts of try 3: kcopy_btl f510 `kcopy 274 kwalk 26 why 0 0 0 6 0 kfb 0 0 0 26`. spot_mac01_fps60_stereo_det 6/12: even (L) shots
  equal, odd (K) shots 102 or 104 px in x65..190 y279..325. icmp_mac01_stereo printed no icmpp line.

## Try 4 (oracle 3 code read of 540e96f5f; the orchestrator resets the budget once: 3 Azahar starts)
Model: Sonnet. This try has a design part with choices inside the gate: the unpatched copy rule, the eyd twin and the icmp prefix
compare. Haiku is not enough: the edits touch the frame record lifetime in three files, and the coder must keep the walker and
stereo invariants that this row lists.

### Cause A: the 20 uncounted walks (named)
- A step whose game draw is off (GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME: battle entry state_battle.c:47-105, battle exit :129-145, map
  transitions state_map_transitions.c:40/145/224, pause state_pause.c:113/234) draws only the background task.
  gfx_draw_frame returns at main_loop.c:225-228, so no swap task is started. gfx_task_background still calls
  nuGfxTaskStart(no swap). That puts the stream into key_list (pm_nusys.c:176, nkey 1). pm_gfx_bg_flush (pm_nusys.c:193, from
  retrace_call :251) then pushes a frame with no PMF_REPL.
- That frame's walk sets pm_icmp_kind 0 (pm_nusys.c:139), and nd_twin_frame_begin(0) does not call pm_gpu_twin_begin, so kc_arm
  stays 0. At the frame end kc_build returns at `!kc_arm` without a count, because kind is not 1 (pm_gpu.c:2878).
- pm_interp_key_take returns 1 (nkey 1, pm_main.c:496), so pm_key_show runs. kc_valid is 0, g != seen, and kc_ok_gen != g, so the
  show counts kfb d and walks the key frame.
- In this window that is about 20 steps: the battle entry (5 to 6 steps with the draw off) plus the map-load steps of the
  kmr_03 warp.
- No other path gives an uncounted refusal followed by a key show:
  - the key walk's own frame end (kind 2) runs after its show decision;
  - ISKIP steps clear nkey at the swap (pm_nusys.c:183), and their background task comes before the swap, so they have no key.
- In such a step the L frame walked the same streams the key walk would walk, with no replacement (no PMF_REPL, so
  pm_interp_repl is 0). The K picture therefore equals the L picture: the copy needs no patch.

### Cause B: the stereo K diff (one class named; the rest needs the repaired measure)
- The eyd uniform has no K twin in stereo. eyd_sync (pm_gpu.c:846-850) writes (gv_post col 0, cw = eye_conv x nd_proj_wn) through
  eye_vec, a plain C3D_FVUnifSet. With nd_kp_on it does not go through u5row_k, so u5kpv[u_eyd] is never set. kp_scan
  (pm_gpu.c:1069-1077) then has nothing to patch, and the K copy keeps L's cw.
- In mono, cw is 0 in both frames, which is why mono is exact.
- In stereo, cw differs between L and K in two cases:
  - nd_proj_wn of the draw differs (a projection that L interpolates);
  - pm_eye_conv changes in the step. The L begin reads it before the step's camera update in serial mode
    (pm_gpu_begin_w -> pm_gpu_begin_wait from pm_step_run :486, before pm_gfx_retrace). The key walk reads it after
    (cam_main.c:78).
- Not proven to be the 102 px. The measure was blind for two reasons:
  - the until `twinchk f\d+ entries` ends the run before a window;
  - ic_cmp (pm_gpu.c:2215-2217) returns at a length mismatch before it counts a pair, so a stereo window never reaches 300 pairs.

### Edits
1. pm_main.c wq_step_end (:473): before `wq_frame_end()`, call `pm_gpu_kc_want(e->drew && e->key)`. New in pm_gpu.c:
   `static int kc_want; void pm_gpu_kc_want(int k) { kc_want = k; }`, with its declaration in pm_port.h next to pm_gpu_kc_cut.
   pm_key_walk's frame end never sets it, so it stays 0 there.
2. pm_gpu.c kc_build, at the top after `kc_valid = 0; kc_gen++;`: `const int want = kc_want; kc_want = 0;`. Then:
   - `!want`: `kc_rec = 0; return;` with no count. No key show follows, which saves the copy on ISKIP, NOKEY and key-walk frame
     ends.
   - want and `!kc_rec`: why[4]++ (now counted for every kind) and return.
   - want, kc_rec, and `!kc_arm`: an unpatched copy. Run the same buffer checks (tb < cb, kc_over, split, size, lists inside),
     but not the kc_cut test (do not consume kc_cut) and not nd_kp_lost. Use a patch count of 0, not nd_kp_n (nd_kp_n holds the
     last L frame's list: it is reset only in pm_gpu_twin_begin). Count `pm_kc_plain++` (a new unsigned, printed in the interp
     line as `kplain <n>` after kfb and reset with the others at pm_main.c:519).
   - armed: as today.
   - Write it as one body with a local `np = kc_arm ? nd_kp_n : 0`, and test kc_cut and nd_kp_lost only when kc_arm.
3. pm_gpu.c eyd K twin. In eyd_sync, when `nd_kp_on && u_eyd >= 0`:
   - L value: `l = { cw, gv_post.r[2].x, gv_post.r[1].x, gv_post.r[0].x }` (memory order w z y x, as u5vec does).
   - K value: the same with `cw_k = pm_eye_conv_live x wn_k`. wn_k is `sqrtf(nd_proj_k[0][3]^2 + nd_proj_k[1][3]^2 +
     nd_proj_k[2][3]^2)` (the nd_proj_wn formula of pm_key_show, pm_nusys.c:234). pm_eye_conv_live is `stereo_frame ?
     pm_eye_conv : 0.f`, the value the key walk's begin will read.
   - Then `u5row_k(u_eyd, l, k)` instead of eye_vec. u5row_k does the C3D_FVUnifSet, the u5kh record and the u5kp stage, and
     kp_scan patches the words. eyd_sync runs before the c3d_upd_needed test (:1216), so the uniform goes out in the scanned
     update.
   - Keep `C3D_FVUnifDirty[u_eye] = false` as today: the eye register itself is not touched.
   - Mono stays byte-equal by construction: cw and cw_k are 0, so u5kpv is 0.
4. pm_gpu.c icmp measure (debug, cold, under the icmp switch only):
   - ic_cmp: on nl != nk, count lenmis, then compare the first min(nl, nk) pairs as for equal lengths (pairs and class counts).
   - Keep the index, register and both values of the first differing pair of the window in IcC (fields fi, fr, fl, fk; fi
     = ~0u when none).
   - Print them in the icmpp line: ` first <i> r<reg> l <v> k <v>`.
   - Add one cold line for the first 8 length mismatches of the run: `icmpl f%u wl %u wk %u first %u r%03x l %08x k %08x`.
5. azt_scenarios.json icmp_mac01_stereo: until `icmpp f(2\d\d|[3-9]\d\d|\d{4,}) ` (the first window at or after frame 200, when
   the scene is at rest). Set cap 45, deadline 40, and set the same until in its bench_defs entry and in expected_events.
   run_azahar.ps1 matches the first hit only, and this regex has no earlier hit.

### Invariants the coder keeps (any break is a stop)
- The walker state after a key show must equal the state after a key walk. pm_key_show copies nd_mv and nd_proj from the twins.
  For an unpatched copy the twins are L's own: nd_mtx_twin_sync ran at the last kind-1 begin, but a kind-0 frame walks without
  twins. So in pm_key_show, when the shown copy was unpatched, do not copy nd_mv_k or nd_proj_k: keep the state the L walk left.
  Keep a flag kc_plain_shown set in kc_build and read it in pm_gpu_key_show, through an accessor, as pm_key_show needs it.
- Lifetimes and noise: no change (the copy rule of try 1 holds for unpatched copies too).
- No new per-frame cost on the mono or stereo path outside nd_kp_on, except the kc_want store.

### Runs (one list, 3 starts)
`bench.py run kcopy_btl,icmp_mac01_stereo,spot_mac01_fps60_stereo_det --row INTERPB3 --why "try 4: unpatched copy for draw-off
steps, eyd K twin, icmp prefix compare"`. The bases are those of try 3 (20d613506). Mono is not rerun: edits 1-2 change no mono
word of spot_mac01 (that scene has no draw-off step), and edit 3 writes the same L word in mono.

### Gate of try 4 (expected values)
- nd_host PASS, memcheck OK.
- kcopy_btl, interp line:
  - why[0] = why[1] = why[2] = why[4] = 0;
  - kfb a = b = c = 0;
  - kwalk = why[3] = kfb d (expected 6);
  - kplain > 0 (expected about 20);
  - kcopy + kwalk = 300.
  - Pass rule: kwalk - why[3] <= 15 (5 % of 300 steps; expected 0).
- spot_mac01_fps60_stereo_det: L shots stay 6/6 equal. K shots byte-equal, or within the rounding rule (at most 1 step per channel)
  with the cause named by the icmpp first-pair fields.
- icmp_mac01_stereo: an icmpp line at f200 or later, with the first differing pair. If the K shots still differ, hand back the
  icmpp and icmpl lines: they name the register (U uniform location, A, T, O), and the oracle writes the patch.

### Stop
A failed gate or a broken invariant: commit, hand back the values. No fifth try without an oracle step.

## Stop / At the end
A failed gate: commit, hand back the values. Delete this row file in the completing commit.
