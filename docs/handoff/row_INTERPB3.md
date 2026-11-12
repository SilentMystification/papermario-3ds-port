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

## Stop / At the end
A failed gate: commit, hand back the values. Delete this row file in the completing commit.
