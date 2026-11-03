# agent_grab2: epic GRAB (frame grab and readback path), row BTLSTAR

## State
- Branch btlstar, worktree C:/Users/Andrew/Documents/GitHub/papermario-3ds-port-btlstar. Base: agent_btlstar at 178e47040.
- Commit 740594342 (3ds/src/pm_gpu.c, untested): K1 and K2 and grablog. Build: PM_LANE=BTLSTAR sh 3ds/build.sh, exit 0, no warning from pm_gpu.c.
- Azahar starts on row BTLSTAR: 7 of 8 used (agent_btlstar). No start in this round. 1 left. The row needs an orchestrator reset (-ResetBudget) to run the open tests.
- Result: INCOMPLETE. No picture check of K1 or K2 yet.

## Commits
- 740594342: pm_gpu.c, 11 insertions, 5 deletions.
  - K1 (pm_gpu.c grab_impl): the reuse past GRAB_MAX (8) is gone. A dirtied copy is copied again. The cap only logs B2 once.
  - K2 (pm_gpu.c pm_gpu_eye): the eye switch sets grab_dirty = 1, so pass 2 (right eye) never reuses the pass 1 (left eye) copy.
  - grablog (debug switch, pm_gpu.c grab_log): one debug line per grab call: id, source, rows, frame, reuse yes or no.

## Findings (read in this round)
- Shot 0005 (frame 174) and shot 0014 (frame 210) at full size: the field and Mario, no star shape. A dark wedge grows at the lower right (0014). The star hole is not in the picture.
- Transition frame counters (btlstar_run7 log, phase lines): draw 71 and flush 71 and texcall 65 in the worst transition frame, against draw 28, flush 28, texcall 24 in normal frames. So the transition adds about 40 strip draws and 40 texture binds per frame (screen_render_util.c:30-77).
- Strip loads read the previous colour buffer (src = prevGfxCfb). grab_cur (pm_gbi.c:251) returns early when the source is not the current buffer (pm_is_cur_cfb). So the strips do not grab. Each strip load reads the previous frame from VRAM through pm_tex_load (texKB up to 321 KB per frame in the window).
- Readback is not the cost: fbread average 0.2 ms, one CPU readback 8731 us. The gbi phase is 140 to 177 ms in the worst frames, average 19.8 ms over the window, p95 150.9 ms.
- Cause of the 140 to 177 ms is NOT named. The phase counters do not name the flush cause. The gbiprof switch (PG0, pm_gbi.c pm_gpu_flush_at) gives flush causes by line. One run with gbiprof on the transition is needed.
- Stale reuse, found in code: tile_bound (pm_gbi.c:2452) skips grab_cur entirely for a frame-buffer texture with the same pointer, so a second 32 by 32 readback in the same frame or the next frame does not reach grab_impl. Not changed (open item 3).
- Stereo right eye (symptom 4): K2 fixes the cross-eye reuse found in code (pm_gpu.c grab_impl reuse path). The shared copy block (copy_get) is still one block for both eyes. Not changed.

## Open items
1. Test K1 and K2 with one start each (picture and grablog). Needs the reset. Scenario needed: battle_start_enc (exists, BTLSTAR, unverified) for K1 frames 174 to 219, pause scenario with stereo for K2 (no such scenario yet; yard_stereo_l and yard_stereo_r exist, ZOOM3D).
2. Byte-equal check on yard for K1 and K2 (rule: one per fix). Not run.
3. tile_bound and frame-buffer textures (pm_gbi.c:2452, ensure_tile pm_gbi.c:2490): a frame-buffer tile with the same pointer skips the grab. Fix candidate: call grab_cur on that path too, and reload only when grab_impl copies. Not made (needs the test in 1).
4. Transition lag (symptom 1): 40 strip loads and flushes per frame (screen_render_util.c:30-77). Fix candidate: one copy of the previous frame per frame into a grab slot, one texrect. Not made. Needs the gbiprof run to confirm the flush cause.
5. Pause spin scenario (symptom 2): not added.
6. Merlon room scenario (symptom 3): blocked. The POSWARP row (branch poswarp) adds the pos=<x>:<y>:<z> switch. This agent did not write a pos switch (coordinator message). Use pos= after the POSWARP commit is merged to main, and cherry-pick it into btlstar for this scenario.

## Next steps
1. Orchestrator: -ResetBudget for row BTLSTAR when open tests start.
2. Run battle_start_enc with grablog and phase, one start, justification line in 3ds/logs/perf/status_BTLSTAR.txt first. Quantity: grab lines per frame in the transition, and the picture at frames 174 to 219.
3. Run gbiprof on battle_start_enc (one start): flush causes by line in the transition.

## Commands
- Build: copy 3ds/src/map_restore_table.c from the main folder, run `python 3ds/tools/gen_map_restore.py`, then `PM_LANE=BTLSTAR sh 3ds/build.sh`. Restore the worktree copy of map_restore_table.c afterwards. Never commit it.
- Test: `PM_ROW=BTLSTAR powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 battle_start_enc -Row BTLSTAR -Build BTLSTAR -Try`
- Grab log: add `grablog` to the debug string. Lines: `grab <id> src <ptr> rows a-b frame n reuse yes|no`.

## Notes
- Acronyms: GRAB (frame grab and readback epic), K1, K2 (grab_impl and pm_gpu_eye fixes of this round), B2 (T2 grab role), T3 (previous-frame grab role), VRAM (video RAM), TMEM (texture memory), DL (display list), GPU (graphics processing unit), CPU (central processing unit), POSWARP (debug position warp row), PG0 (flush counter by cause), gbi (graphics binary interface, the display list phase), fps (frames per second).
