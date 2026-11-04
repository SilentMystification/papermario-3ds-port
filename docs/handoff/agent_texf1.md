# agent_texf1 handoff (branch native, 2026-11-04)

## State
- Row TEXF1 committed: 6aeb0c117 on native (pm_tex.c, pm_gpu.c, azt_scenarios.json, test_catalog.md).
- Change: drop(), the stage free in pm_tex_begin_frame and the stage replace (pm_tex.c) call pf_add. pm_tex_gate runs first in pm_tex_upload_vram_before_frame and releases the list when pm_gx_done() (gxCmdQueueWait timeout 0, no wait) reports the previous frame done. A second copy into copy_blk in one frame calls copy_guard, which drains the GX queue (pm_gx_drain) first.
- Counter: texstat line "texstat pending free: now, max, ops, gate busy, max age, released, overflow, copy drains".
- Scenario hos01_texmax added with its catalog row (unverified picture, log checks pass).

## Measured (Azahar, 4 starts on row TEXF1)
- hos01_texmax, new build: evict 0, texfail none, invalidated 65, pending ops 88, max 17, gate busy 0, overflow 0.
- yard top shot 0000_0_11.ppm: byte equal, new build vs HEAD 12efe52fc.
- yard bottom shots bot_*.ppm: differ 468 to 549 bytes per frame. Baseline against baseline differs 0 to 129 bytes. Cause not decided.
- Boot vram acct line: equal.

## Open items
- Gates not run: mac_01 top shot, hos_01 wall binds (hos01_texword window), pause_spin, battle_start_fb shots 0 to 14. Azahar budget used: 4 of 8 on TEXF1.
- gxCmdQueueWait(q, 0) return value is not checked on hardware. gate busy 0 may mean the queue is always empty at the gate.
- Bottom screen diff: decide if it is noise (run the new build twice) or the change.
- Copy drain count is not in the log yet (texstat line prints it; the yard run did not reach frame 300).

## VRAM3 (not started)
- Trace of the 4 full images: full_blk[OFF_N] (pm_gpu.c:355), off_used per frame (pm_gpu.c:1695-1704). Slots are reused in turn when a frame needs more than OFF_N (V05, logs "slots are used again"). Reuse overwrites pixels a pass may still read, so OFF_N 4 to 3 needs the V05 demand count first.
- RGBA5551 (R4 in docs/handoff/inv_vram_map.md) saves 1024 KiB. Format change in tex_hdr, pm_tex bind and the snap copy.
- Shared depth buffer (R2) saves 375 KiB. Needs the depth clear at pm_gpu.c:1751 checked for the right eye.

## Commands
- Build: PM_LANE=native sh 3ds/build.sh (from the worktree root; needs 3ds/ld/map_restore.ld and 3ds/src/map_restore_table.c present).
- Baseline lane: copy build3ds/lane-texf1base/pm_3ds.3dsx (built from 12efe52fc, copies of assets/us, ver/us, map_restore.ld and map_restore_table.c were needed in the scratch tree).
- Test: PM_ROW=TEXF1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row TEXF1 -Build native -Try
- Justification lines: 3ds/logs/perf/status_TEXF1.txt (4 lines).
- Shots kept: C:\Users\Andrew\AppData\Local\Temp\claude\shots\texf1_yard and base_yard, base2_yard.

## Next steps
1. Run mac01 top, hos01 wall, pause_spin and battle_start_fb on new and baseline lanes (budget reset needed: azt.ps1 -ResetBudget TEXF1).
2. Decide the bottom screen diff.
3. VRAM3 step 1: count per-frame demand of full images (V05 log) on a pause and a battle run.

## Gate results after the first handoff (8 of 8 starts used on TEXF1)
- mac_01 seam, shots 0000_1_2 and 0001_1_2 (new vs base): byte equal.
- yard bottom, digit mask x 162 to 185, y 0 to 8 (new vs base): base-vs-base residual 0 bytes outside the mask. TEXF1-vs-base residual 468 bytes constant in every frame, box x 162 to 231, y 224 to 230. Not the digits. Cause not decided. Possibly a static bottom-screen text line changed.
- hos_01 wall texreg_wall: texword line for 0100169c missing on the TEXF1 build AND on the baseline 12efe52fc build. Not a TEXF1 regression. The scenario check fails on HEAD. Bind check not decided.
- pause_spin and battle_start_fb: NOT RUN. Azahar budget of 8 reached. Status INCOMPLETE. Needs ResetBudget TEXF1 from the orchestrator.
- VRAM3: not started.
