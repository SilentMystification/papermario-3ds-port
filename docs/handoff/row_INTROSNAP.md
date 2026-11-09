# Row INTROSNAP: visual snap between the stage zoom and the cutscene start (wide on)

## User facts
User (2026-11-09, console): "there's an odd visual snap in the intro cutscene between the stage zooming in and the cutscene actually
starting".

## Start state
Worktree build3ds/wt/INTROSNAP, branch INTROSNAP from main (hash in the first commit). PM_ROW=INTROSNAP, PM_LANE=INTROSNAP.

## Findings (2026-11-09, code trace and Azahar)
- Cause 1 (walker reads a changed pm_wide) cannot happen on a mode change: __wrap_set_game_mode (pm_map.c) calls pm_walk_drain("mode")
  before set_game_mode writes pm_wide; on the logic thread pm_wq_idle waits until the queue (frames and key frames, pm_wq_key) is walked.
  Debug tool in the build: PMF_WIDE (fr_push) and `wide: walk f<n> built <b> live <l>` (pm_frame_walk); the threaded Azahar run logged 0.
- Cause 2 (fps60 blend across the load): no visible effect on this route. Base fc9897f11 and the fix (pm_interp_cut at DISABLE_DRAW_FRAME
  and at an aspect change, plus pm_wide_w) gave byte-equal shots on newgame_handover (det, f400-451) and newgame_handover_thr (threaded,
  f396-459). The handover on Azahar: 4:3 stage zoom with a fade to black (f402-f413), black during the kmr_11 load, wide fade-in from
  about f434. No frame wider than its neighbours in the base. The fix was reverted (no picture change).
- Not yet tested: the console timing with fskip on (LGF_ISKIP) and without fs1off; the boot intro (path B, state_intro.c). The user's
  console run with `aspect:` and `wide: walk` lines decides the next code point.

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
