# Row PAUSEC: pause crash (data abort, NULL z-buffer in the pause filter)

## User facts
User (2026-11-09, O3DS XL, main 60b1dc9ac build 3353, prof mode, fps30): "pausing crashed the game." Log build3ds/hw_live_log_131.txt:
[CRASH] data abort thread=other pc=00195910 (gfx_frame_filter_pass_1 +0x2bc) far=00000284 fsr=00000805 (write) r10=00000284, sp outside
the known thread stacks (the logic thread, heap stack). The last map change was mac_00 -> mac_01.

## Cause (orchestrator trace, confirm by code first)
far 0x284 = 2 * 322 = &zBuffer[320*1 + 2], the first filtered pixel: zBuffer (nuGfxZBuffer) is NULL in gfx_transfer_frame_to_depth
(src/background_gfx.c:378, case BACKGROUND_RENDER_STATE_FILTER_PAUSED). nuGfxZBuffer is allocated only by pm_zbuf_need (3ds/src/pm_main.c:186),
called by the WALKER in img_role (3ds/src/nd_target.c:35) when it walks the VISCVG fill of the previous state (background_gfx.c:362-369).
With the logic thread (CORE2C-3) the FILTER_PAUSED step runs on core 1 right after that step; pm_walk_drain("cfb") (nd_walk.c:513:
pm_wq_idle + nd_c2_drain) may return before the walk of the previous frame has run img_role (ring empty but the frame popped and still
being walked, or depth-1 push order). Then zBuffer is still NULL -> data abort. In serial mode (Azahar det, prof sample frames) the order
holds, so the gates did not see it.

## Start state
Worktree build3ds/wt/PAUSEC, branch PAUSEC from main (hash in the first commit). PM_ROW=PAUSEC, PM_LANE=PAUSEC.

## Plan
1. Confirm by code: what pm_wq_idle waits for when called on the logic thread (nd_wq.c), and whether frame N (the VISCVG fill) is walked
   before it returns. Name file:line and the condition. Status line.
2. Fix at the shared point: pm_walk_drain on the logic thread must wait until every pushed frame is walked (wq_fr_walked == wq_fr_push,
   the CORE2I/C1WAIT wait kind), not only until the ring is empty. This also covers the other drain callers (pm_os.c:433 ROM read,
   pm_map.c decode, background_gfx.c:271). Keep the C1WAIT timing.
3. Allocation must not depend on walk order: in background_gfx.c (TARGET_3DS) call pm_zbuf_need() before gfx_transfer_frame_to_depth on the
   game side; pm_zbuf_need's pm_tex_invalidate touches walker state: post it through pm_wq_post when called on the logic thread (or move the
   invalidate to the walker). If the malloc fails (O3DS XL commit used 65536/65536 KB at the crash), skip the filter and log once (no crash).
4. Commit with the hw note.

## Done criteria
1. nd_host PASS. memcheck OK.
2. Azahar: a threaded (non-det, prof sample off) pause scenario: pause from the field 3 times (padscript Start, wait, Start), until the pause
   menu line the build prints, cap 20; the shot shows the pause menu over the filtered field; no CRASH line. Use the existing pause scenario
   (grep pause in azt_scenarios.json) with the logic thread on; add one if none is threaded.
3. pause det scenario (existing) byte-equal to base.
Affected scenarios (no others): the pause ones.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c. Padscripts in the azslots sdmc.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "PAUSEC ready <hash>".
