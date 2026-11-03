# agent_grab3: epic GRAB, row BTLSTAR (state at wind-down)

## Budget
- BTLSTAR: 8 of 8 Azahar starts used since the last reset. No start, no build left for this round. Reset needed for any test.

## Commits (branch btlstar)
- 4bd4ad4f8 FB1: previous-frame strips bind one 320x240 texture (fb_prev_base, pm_gbi.c ensure_tile). Strip loads 1404 to 0. Transition shots 0000-0014 byte-equal to the parent (baseline). Yard byte-equal. Merged to main.
- 3eecc8857, 708224743: gbiprof flush-cause report prints (PG0), gbiprofn=<frames> period (debug).
- eb5b9e324, df25cc39f: main merged.
- 2d266a831, 6a8e4bc4d, f0c130508: merlon_ball, merlon_ball60 (texlog), merlon_door scenarios and catalog rows.
- 045d52514: fbbind bind line for the FB1 texture (size, slot or CPU).

## Findings
- Transition (console garble, still open): FB1 did not fix it on the console (build 1792). Candidate: the GPU copy slot is 512x256 physical but pm_tex_size reports 320x240 (pm_tex.c pm_tex_size, pm_gpu.c off_begin). Not confirmed. Test: fbbind bind line (new) on battle_start_fb.
- Transition readback: the one CPU readback (8.8 ms, frame 174) stays (decision). A GPU-copy request in the pre-battle state (FB2) gave 0 readbacks and shot 0000 byte-equal, but shots 0001-0014 differ (28-37 px). Reverted. Copy-once variant not tested.
- Empty flush: already early-returns (flush_draw nverts 0). No change.
- Flush count: 432 flush calls per transition frame, 403 empty; about 29 do work. Top cause pm_gpu_set_mode:2280.
- Hit effects (console: too large): FB1 catches every clamp window of the previous colour buffer. Callers: screen_render_util.c:125 via screen_overlays.c:345 (battle start), screen_overlays.c:352 (another overlay), battle/move/item/dizzy_dial.c:12 (battle dizzy dial), effects/firework_rocket.c:247. Narrowing not done: the four callers share one routine; needs a caller tag or a per-strip flag. Battle hit scenario not run.
- Merlon blue square: not reproduced. The blue panel in the shots (frames 63-79) is a Toad Town star sign (scene object). No round ball in view. The 32x32 ball quad (crystal_ball_gfx.c:108-116) did not draw in frames 60-99 (no 33x33 texrect).
- Merlon door: placed at -203 20 -206 settles at -203 20 -242. Padscript go_-203_-280_500 reaches the doorstep (frames 123-133), stays outside; up*60 does not enter. Room does not load. Blocked on a door interaction (A press or other).
- Quizmo stretched texture (console): no frame copy on the Quizmo path (quizmo_*.c, Quizmo.inc.c). FB1 cannot catch it. Candidate: stage light display list D_09001518_39A0D8 (64x16 CI texture, SetTileSize 252x60, T clamp): the window loads 60 rows from a 16-row texture (pm_gbi.c G_SETTILESIZE, ensure_tile). Not confirmed.
- VRAM (console "vram alloc fail"): none in the logs I have (hw_live_log_131, _170, _224 (empty)). The 131 log: map load A and B each used 2007 KB, 1065 KB free per bank; four full images (512 KB each, two per bank, frame 876) leave about 41 KB per bank (matches the 82 KB minimum reported). Runtime users: copy block (512 KB, copy_get), full off-screen images (512 KB, OFF_N=4), texture slab extension. Lazy at runtime, freed after 60 idle frames (IDLE_FREE). On failure: copy_get returns NULL (grab falls back to CPU readback); off-screen image sets off_fail, the pass draws on the screen (likely source of stretched or garbled pictures).
- VRAM allocation table: boot = texcache 3072 KB (two halves 1536), targets 4x384 KB, small arena 12x16 KB (about 4800 KB). Runtime pool about 1344 KB. Runtime demand copy 512 + 4 full images 2048 = 2560 KB.

## Open items / next commands (need a reset first)
1. Reset BTLSTAR (-ResetBudget, worktree).
2. VRAM fix candidates: (a) texcache halves 1536 -> 1024 KB per bank (pool 1577 KB per bank: copy 512 + 2 full 1024); or (b) OFF_N 4 -> 2. Test with vramfill=<KB> (pm_tex.c dummy blocks) on battle_start_fb and yard; check vram alloc fail lines and transition shots.
3. battle_start_fb with the FB1 bind line (fbbind): size, slot or CPU per bind (console garble check).
4. Battle scene with a landed hit (new scenario) with fbbind and drawlog: which draws take FB1.
5. merlon_door: find the door interaction (A press at the door, or padscript token A after go_) and retry; then merlon_ball60 for the ball.
6. Quizmo scenario (talk to Quizmo or script switch), one shot, texlog on the stage light draw (D09001518 tile 252x60).
7. Fix candidate for the stage light: clamp the tile window to the texture height when T is clamp (pm_gbi.c G_SETTILESIZE / ensure_tile); byte-equal check on the stage scene.

## Commands
- Build: `cp 3ds/src/map_restore_table.c <keep>; python 3ds/tools/gen_map_restore.py; PM_LANE=BTLSTAR sh 3ds/build.sh; cp <keep> 3ds/src/map_restore_table.c`
- Test: `PM_ROW=BTLSTAR powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row BTLSTAR -Build BTLSTAR -Try`
- Lane log: build3ds/lane-BTLSTAR/last_log.txt (the Roaming log.txt is shared with other agents; do not read it for a run).
