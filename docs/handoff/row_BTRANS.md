# Row BTRANS: battle entry swirl intermittently black or missing

## User report (2026-11-06)
The star/swirl transition when entering a battle is sometimes corrupted: parts go black or the effect is missing for some frames. Definitely on O3DS hardware, maybe in Azahar. Seems to happen when Mario is moving while entering the battle. Investigate the CODE of the whole sequence (CLAUDE.md rule 5, INVESTIGATE BY CODE): snapshots are the last resort.

## Oracle plan BTRANS
The capture path in code: the swirl strips read the old frame buffer. pm_gbi.c:3985 (SETTIMG of a cfb that is not the current one, key pass only, `!pm_interp_repl`) calls pm_cfb_capture_prev, which runs cfb_capture_prev(0) (pm_main.c ~233). That tries the GPU path pm_gpu_grab_prev, then grab_impl(prev_frame=1) at pm_gpu.c:2346-2372. grab_impl returns 2 only when prev_pre is set, i.e. the previous drawn frame asked (prev_want) and pm_gpu_begin queued grab_queue_copy before the clear (pm_gpu.c:2505-2507). Otherwise it returns 0, and cfb_capture_prev falls back to pm_gpu_read_frame (pm_main.c ~263), a CPU read of the live colour buffer.
Causes, ranked:
1. (top) The fallback reads the colour buffer after this frame's clear. pm_gpu_read_frame (pm_gpu.c:2133) waits only for the previous frame's GPU tail. kick_check (pm_gpu.c:727) has already queued `C3D_RenderTargetClear` and part 1 of this frame at the KICK1 split once draws_frame reaches kick_at. A frame with a T2 grab does the same via grab_split. So the CPU read gets a cleared or half-drawn buffer: black areas, or a frame with no swirl content.
   Why moving: a moving camera plus the walking dust and NPCs raises draws_frame past kick_at before the overlay (the overlay is drawn last), so the split comes before the capture. Standing still often stays under kick_at. That is the "when Mario moves" link.
   The fallback is taken when:
   - (a) First transition frame: no prev_want from the frame before, so prev_pre = 0, every encounter.
   - (b) Interp on: the key pass sets prev_want. The next pm_gpu_begin is the t = 0.5 replay draw; it queues the copy and clears prev_want, but the replay never captures. The next key pass then has prev_pre = 0, so it falls back on every key frame. pm_interp_skip (F06-5, pm_nusys.c:113), more likely when the frame is heavy (moving), flips between the two patterns, so the black is intermittent.
   - (c) An off_snapshot of a full-size slot in the frame voids the request (`prev_pre = 0`, pm_gpu.c ~396), or a T2 grab ran first (grab_n).
   - (d) pm_gpu_off_begin finds no free slot (pm_gpu.c ~2374). OFF-1..3 now keep more off-screen slots alive at battle load.
2. Missing strips on the fallback frames: 33-40 uploads of 320x6 per frame (PT12: 32 misses, 255 KB a frame) under VRAM pressure at battle load. pm_tex fails, the H03 skip drops the batch, and the effect is missing.
3. Wrong buffer role: pm_is_cur_cfb (pm_main.c ~192, the wide +80 rule) decides T2 against T3. If an FS1 skip or an interp key pass changes which buffer is current, the swirl's SETTIMG takes the T2 grab (grab_cur) of the frame being built, which is lower risk.
Measurement (confirms cause 1 only): switch `caplog`, one line per cfb_capture_prev call: frame, pm_interp_repl, the key/replay pass, the path (gpu 2 / cpu fallback / done), the reason for 0 (prev_pre, grab_n, off_begin fail, grab_tex_init), kick_done, grab_split, want_clear, draws_frame, and the frame's pm_tex failed count.
- Scenario: kmr_09 Goomba, 10 walk-ins with the stick held and 10 standing, interp on and off, a shot every transition frame.
- Confirmed if the black frames equal the cpu-fallback calls with kick_done > 0 (or grab_split > 0).
Fix:
1. pm_gpu.c pm_gpu_begin: keep prev_want across a replay draw. Clear it only in a frame whose pass captured, and keep it set for 2 drawn frames after the last request. Then the copy is queued at begin for every key frame (fixes a and b).
2. cfb_capture_prev / pm_gpu_read_frame: when grab_impl returns 0 and this frame has already split (kick_done || grab_split || !want_clear), do not read the colour buffer. Keep the last good capture in the slot (no CPU read) and count it.
3. First frame: encounter start (the encounter.c OVERLAY_START_BATTLE set) calls a port hook that sets prev_want one frame early, or kick_check holds KICK1 while a capture request is pending and no copy is queued (fixes a).
4. If cause 2 is confirmed: draw the fallback through one full-screen off-screen slot (PT12 fix step 2), not 40 uploads.
Gate:
- 20 walk-in plus 20 standing encounters, interp on and off: 0 black frames.
- Pixel test: no transition frame with more than 5% pure black outside the swirl mask compared with the frame before.
- caplog: cpu-after-split = 0; pause filter (golden pause1-4) right.
- One O3DS console push, the user walks into 5 encounters.

## Oracle decision BTRANS
Keep fixes 1-3 and verify on the console. They guard code paths that still exist on nds1:
- cfb_capture_prev (pm_main.c) still falls back to pm_gpu_read_frame;
- prev_want is still cleared at every pm_gpu_begin, including the interp replay draw (pm_gpu.c, the prev_pre/prev_want lines);
- kick_check still queues the clear at the first split.
Of the 18 commits between efaca195d and 5ec146f06, none touches those lines. They are CORE-3 (nd_rect.c), MDL, the PSHIFT grab texel row and AZT9 pool/warm-restart changes.
Nothing on nds1 hides the fallback. The evidence is not comparable:
- the old 38/38 run was a walk-in (most likely interp on: every key frame falls back, cause b);
- the only nds1 base run was standing;
- the 8 fixed-build runs show cpu 0 because the fix removes the fallback, so they prove nothing about the base.
Next run: one nds1-base (caplog only) walk-in with interp on.
- cpu_after_split > 0: the cause is shown on nds1.
- cpu 0: also run a walk-in with interp off, and one with the cpulimit and frame pacing of O3DS (the `fs1force` switch to force FS1 skips). This covers the O3DS-only paths: slow frames give an early KICK1 and FS1 skips.
Then push the fixed build to the O3DS console. Gate: the user does 5 walk-in encounters, with no black frames, and crash.txt/log.txt have the caplog lines.
The crash (standing encwarp, interp off, base and fixed): give it its own row; it is not BTRANS.
1. Code-trace route: take the symbolized PC/LR and the last 30 log lines from the AZT crash report ("crash symbols").
2. Read the faulting function, and the frame, game mode and battle-load phase from the pm_wd phase line.
3. Compare with docs/handoff/archive/diag_encwarp.md and diag_battle_start_crash.md (earlier encwarp and battle-start faults: warp-into-battle state and the load order).
4. Name the field or pointer at the faulting line from the code before any further run. One confirming run at most.

## Rules
Own worktree: git worktree add build3ds/wt/btrans -b btrans nds1; copy assets/us and ver/us/build; build there (PM_LANE=btrans); azt -Build <worktree 3dsx>; check each run's boot build line. The caplog counters are the main proof; shots only for the final picture gate (keep the encounter count of the gate, but read black frames from caplog plus one shot pass). Commit on branch btrans, no attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. New scenarios go into azt_scenarios.json and bench_defs.json in the same commit (nds1 JSON as the base, add only yours). Status line in 3ds/logs/perf/status_BTRANS.txt before each start. Budget: reset and continue. Two tries per named cause. Delete this file in the last fix commit; the orchestrator pushes to the console for the user check.
