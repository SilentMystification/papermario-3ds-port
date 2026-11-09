# Row INTROSNAP: visual snap between the stage zoom and the cutscene start (wide on)

## User facts
User (2026-11-09, console): "there's an odd visual snap in the intro cutscene between the stage zooming in and the cutscene actually
starting".

## Start state
Worktree build3ds/wt/INTROSNAP, branch INTROSNAP from main (hash in the first commit). PM_ROW=INTROSNAP, PM_LANE=INTROSNAP.

## Causes (oracle)
Path A (new game, most likely): state_file_select.c:530 set_curtain_scale_goal(2.0f) zooms the stage; :543 set_game_mode(ENTER_WORLD)
sets pm_wide = 1 (game_modes.c:280) on the logic thread and DISABLE_DRAW_FRAME (state_map_transitions.c:40).
1. Walker race: the walker reads the global pm_wide at walk time (pm_gpu.c:169-170 viewport x1.25, :919-929 scissor, :1887 offscreen
   base -80, pm_gpu_rect.c:147-148, :218, nd_rect.c:103, nd_tex_set.c:321, pm_main.c:181), so frame Z+11 (built 4:3, frustum +-80) can be
   walked with wide = 1: a 25% stretch that stays on screen during the kmr_20 load. Timing dependent: not in det/serial runs.
2. Interpolation across the cut: at the first drawn step after the load, pm_interp_lerp(0.5) (pm_nusys.c:158) pairs new leaves with the
   stale pleaves of the step before the load (gfx_curtains node): one 60 Hz frame 11% wide. No skip triggers (LGF_ISKIP not set;
   pm_interp_dont_camera has no reader). Path B (boot intro parts, state_intro.c:148/232) has cause 2 at each part load.

## Plan
0. Debug tools (stay): in set_game_mode when pm_wide changes `aspect: wide %d -> %d mode %d`; in pm_frame_walk (pm_nusys.c:121) a cold
   noinline log of the first 8 frames whose build-time wide differs from live: `wide: walk f%u built %d live %d`; `cut=%u` on the f06
   line (pm_interp.c:183).
1. Fix 1: pm_port.h:126 PMF_WIDE 2u; fr_push (pm_nusys.c:133) sets it when pm_wide (every push, also pm_gfx_bg_flush; the key frame
   inherits through frame order, comment it; PmKey stays 32 B); pm_gpu.c `int pm_wide_w` (walker side, init :1434, extern in
   pm_gpu_int.h); pm_frame_walk first line pm_wide_w = (f->flags & PMF_WIDE) != 0; replace walker reads of pm_wide with pm_wide_w in
   pm_gpu.c (not 150/151/1434), pm_gpu_rect.c:147/148/218, nd_rect.c:103, nd_tex_set.c:321, pm_main.c:181; game side stays on pm_wide.
   Confirm each changed function runs on the walker; report any that does not.
2. Fix 2: pm_interp.c `void pm_interp_cut(void) { pnleaf = pnnode = 0; ncut++; }` (logic thread); call it in src/main_loop.c:212 (the
   DISABLE_DRAW_FRAME branch: every map load, intro part, battle) and in set_game_mode when pm_wide changed.
hw_guidelines: same loads on the walk path, one OR per push, O(1) cut, cold debug logs, static +8 B.

## Done criteria
nd_host PASS; byte-equal (det/serial) intro_native_pic and one wide det map scene vs build-at base; new scenario+bench newgame_handover
(fileselect, First Play, name, yes; wide fps60 det shots over the last ~4 steps before game mode 7 and ~6 after the draw resumes; until
`game mode 4 frame` or the first shot after the load; cap 25 s): no frame wider than its neighbours (before: the first lerp frame 11%
wider). Console (user): non-det fps60 wide new game, `wide: walk` mismatch count, eye check of the handover.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c. DIRECT/STEREO edit pm_gpu.c
and pm_nusys.c in parallel: merge main before each fix.

## At the end
Delete this row file in the completing commit. SendMessage main "INTROSNAP ready <hash>".
