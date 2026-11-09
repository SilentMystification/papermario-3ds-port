# Row PAUSEFLK: flicker on the pause Party and Star Spirits tabs

## User facts
User (2026-11-09): "flickering occurs on the partner and spirit menu" (pause menu Party and Star Spirits tabs, O3DS XL, logic thread on).

## Start state
Worktree build3ds/wt/PAUSEFLK, branch PAUSEFLK from main (hash in the first commit). PM_ROW=PAUSEFLK, PM_LANE=PAUSEFLK.

## Candidates (oracle, unconfirmed, ranked)
1. gBoxQuadBuffer[21][16] (draw_box.c:16-17, 430-434): one ring for both display contexts; rotscale boxes write quads in place and the
   list reads them by pointer; with the logic thread, step N+1 overwrites entries the walker still draws when 2K > 21 (K rotscale boxes
   per frame; pause flip flags pause_main.c:59-62, windows.c:130-134). Log K and the ring index range per frame.
2. V12c ldrec lookup (nd_tex_set.c:154-158): the render tile takes ldrec[k].img for any k with tmem == tile_tmem[0] and img != timg, so
   an older load at the same TMEM can replace the sprite raster (imgfx CI4 strips imgfx.c ~1547-1630 after portrait CI8 and spirits
   background loads; D4b-1 nd_tex4_run updates ldrec; partners depth-sorted each frame, pause_partners.c:336-351). Log timg, src, ldrec.
3. Portrait decode during the list build (pause_partners.c:293-309) writes gPausePartnersPaletteBuffers while the walker draws the old
   portrait: safe only through the pm_map.c:374 drain + :385 invalidate. Party only.
4. msg_draw.c:348 reset skip (D4b-1). 5. slot_valid hashes the image only (pm_tex.c:1262), not the TLUT.

## Plan
1. Scenario pause_tab_thr_pic (threaded) and a det twin: mac_01 0 0 200, START, to Party: 10 consecutive shots steady plus the flip
   frames, then Spirits the same (read pause_main input handling for the tab keys). Scenarios and benches in one commit.
2. Flicker only threaded: class 1 or 3 (order). Both: class 2, 4 or 5 (state/cache). Trace that class, log the named value.
3. Likely fixes: (1) gBoxQuadBuffer per display context ([2][21][16], selected by gCurrentDisplayContextIndex, +5.4 KB bss, or heap at
   boot); (2) limit the ldrec lookup to loads after the last G_SETTIMG of this tile, or drop stale ldrec at each TMEM-0 load (shared
   draw code: byte gates). One fix point per cause, 2 tries per cause.

## Done criteria
nd_host PASS; memcheck OK; threaded series on Party and Spirits (10+ consecutive frames each, steady and flip) shows no frame-to-frame
difference in sprites, portraits or windows (read and diffed); pause_spin_grab_det_w300 byte-equal unless the fix changes pixels by
design (named); byte gates spot_mac01_det, battle_start_f182_cold if nd_tex_set.c changes.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c. DIRECT D4b-2 edits
draw_box.c/windows.c: merge main first.

## At the end
Delete this row file in the completing commit. SendMessage main "PAUSEFLK ready <hash>".
