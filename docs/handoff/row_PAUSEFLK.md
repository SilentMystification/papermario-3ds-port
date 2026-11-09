# Row PAUSEFLK: flicker on the pause Party and Star Spirits tabs

## User facts
User (2026-11-09): "flickering occurs on the partner and spirit menu" (pause menu Party and Star Spirits tabs, O3DS XL, logic thread on).

## Start state
Worktree build3ds/wt/PAUSEFLK, branch PAUSEFLK from main (hash in the first commit). PM_ROW=PAUSEFLK, PM_LANE=PAUSEFLK.

## State (2026-11-09, branch PAUSEFLK)
- Benches: pause_tab_thr_pic (threaded, wide), pause_tab_det_pic (det twin), pause_tab_c2haz (det census), pause_tab_c2haz_thr (census,
  logic thread). Route: mac_01, all partners and 7 spirits (debug switches allpartners, spmax=<n> at the warp, pm_warp.c), Party panel
  with two wheel turns (frames about 150 to 190), Star Spirits panel from about frame 205.
- Azahar does not show the flicker: threaded and det series show only the sprite animation, the cursor and the portrait scroll from
  frame to frame (no frame of a panel differs from its neighbours otherwise). Azahar runs the logic thread interleaved with the walker,
  not in parallel, so a race cannot show there.
- c2haz census, det and threaded, on this route: 0 rewritten addresses (Party and Spirits panels and the tab flips). The portrait
  decodes drain the walker (drains 298 -> 330 during the wheel turns). So candidates 1 (gBoxQuadBuffer ring: at most 6 rotscale boxes
  per frame, 2K < 21) and 3 (portrait decode) do not hold on this route; 2, 4 and 5 are deterministic and would show in the det series.
- Not covered by the census: writes past the first 64 bytes of a pointer, transient writes inside a step, native cache reads.
- Fixed (8ad76ecff): draw_box rotscale corners read and write quads[idx*4] of their slot (decomp SHIFT form). The US form moved quads
  by 4, 8 and 12, so corner 3 used slot + 24..27 of a 16-vertex slot; slot 20 overran gBoxQuadBuffer into InspectIcon and ISpyData.
  Byte-equal to the base on pause_tab_det_pic 101/101 and pause_spin_grab_det_w300 40/40, so it is not the flicker in Azahar.

## Next
Console run (the console decides): push the pause_tab_c2haz_thr switches to the O3DS XL with the logic thread and read the c2haz new
lines (a parallel walk shows what Azahar cannot). Ask the user whether the flicker shows with the logic thread off, and whether it is
steady or only while the wheel turns or the tab flips.

## Done criteria
nd_host PASS; memcheck OK; threaded series on Party and Spirits (10+ consecutive frames each, steady and flip) shows no frame-to-frame
difference in sprites, portraits or windows (read and diffed); pause_spin_grab_det_w300 byte-equal unless the fix changes pixels by
design (named); byte gates spot_mac01_det, battle_start_f182_cold if nd_tex_set.c changes.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c. DIRECT D4b-2 edits
draw_box.c/windows.c: merge main first.

## At the end
Delete this row file in the completing commit. SendMessage main "PAUSEFLK ready <hash>".
