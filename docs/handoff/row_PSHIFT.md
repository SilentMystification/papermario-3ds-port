# Row PSHIFT: the scene behind the pause menu shifts by 1 pixel

## User report (2026-11-06, Azahar and O3DS)
When the game pauses, the scene behind the pause menu sits 1 pixel off from the live frame. User guess: the buffer used for it is stale or offset.

## Facts and suspects
The pause background is a grab (copy) of the last game frame drawn as a texture rect. Recent changes in that path, all merged in nds1: OFF-2 (fd11da7e5 on branch off: grab_bind native, grab_native_unit0 words from grab_tex), OFF-1 (RECT_GM = off_proj on off-screen, rect raw px/py on off), RECT-1b (629f7c34b: native rects, pixel origin px0 = (xl+3)>>2), BG seam fix (871cc95ca: gbi_texrect origin at fractional edges). A 1 px offset fits: a half-texel or pixel-origin difference between the grab copy and the rect that draws it, a wrong grab source origin, or a stale grab image.

## Rows
PSHIFT-1 MEASURE: scenario pause_shift (mac_01 0 0 200, shot the last live frame before START and the first 3 frames with the pause menu, region outside the menu panel). Compute the best x/y offset between the live frame and the pause background (cross-correlation over the region, report dx, dy and the error at 0 and at the best offset). Then the same on build-at of 2206cd0a0 (before OFF and the RECT merges): if that build shows dx 0, bisect the merges between (RECT-1b 56ee96c93, OFF e5e7d5de5, PM5 3d29634f0, CORE e47cc7f50) with build-at. Name the commit.
PSHIFT-2 FIX: at the named code point (grab copy origin, the pause rect's px/py or s/t origin, or the grab texture offset), fix so dx = dy = 0. Two tries per cause.
Gate: pause_shift dx 0 dy 0 at 3 frames; spot_mac01_pic byte-equal; pause_spin and pause_spin_grab_w300 by eye; then the orchestrator pushes for the user check.

## Rules
Own worktree: git worktree add build3ds/wt/pshift -b pshift nds1; copy assets/us and ver/us/build; build there (PM_LANE=pshift); azt -Build <worktree 3dsx>; check each run's boot build line against your lane. Commit on branch pshift, no attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. A new scenario goes into azt_scenarios.json and bench_defs.json in the same commit (take nds1 JSON as the base, add only yours). Status line in 3ds/logs/perf/status_PSHIFT.txt before each start. Budget: reset and continue. Delete this file in the fix commit.
