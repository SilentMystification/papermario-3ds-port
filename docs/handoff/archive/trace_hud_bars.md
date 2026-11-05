# TRHB: HUD bar trace (battle, 2026-11-03)

Branch native. Trace build: lane trhb, source HEAD adeeb38e3 plus temporary logs (reverted; not committed).
Azahar starts: 2 of 8 (status_TRHB.txt). Runs used direct run_azahar.ps1 (no catalog scenario; the tests need debug switches the catalog does not hold).

## 1. Enemy HP bar 1 px too tall and wide: PARTLY PROVEN
- PROVEN (HUD side, run 2, log lines #2224 and #875): HES_HPBar draw tex 32x16, draw 32x16, off -16 -8, base 348 191, z 10 (render depth 10 from 16C8E0.c:913).
- NOT PROVEN: the GPU rectangle of the bar. No pm_gpu_tex_rect line with px 348..379 appears in 800 rect lines (run 2). The bar does not reach pm_gpu_tex_rect in this path. Cause not found.
- Source: hud_element.c:433 (16-row rule), hud_element.c:466-471 (lrx = baseX + drawSizeX - 1 for width 32), pm_gbi.c gbi_texrect (ex = xh/4 - 1 in 1-cycle mode: the 31 px width is one pixel narrower than the 32 px image, so the 1 px wide report needs the GPU side).
- Next: log in gbi_texrect before rect_to_image (pm_gbi.c) and in box_pattern_rect to see which early return drops the bar.

## 2. Bombette charge bar and run-away bar: NOT PROVEN, NOT REACHED
- Run-away state set by hook (f110, log "debug runaway: run state set f110"). No flee draw: 0 "trhb mash" lines and 0 "copyrect" lines in frames 110 to 248.
- Shot 182 (start 1): screen dark (fade), no bar.
- Hypothesis (source only): flee.c:48 hudPosX = -48, action_cmd.c:130 posX -= 28. Negative x goes into gDPFillRectangle (include/PR/gbi.h:4325, 10-bit fields, wraps to 1024 + x). pm_gpu.c:2753 swaps inverted bounds, so a wrapped left edge gives a span from about 40 to 1019: a long horizontal line at the bar row. Consistent with the report, not proven.
- Next: a run-away hook that enters the real flee action (the menu path also calls btl_state_update_player_menu, which the hook skips), plus the trhb mash and copyrect logs.

## 3. Star points bar depth: PROVEN for the 2D path, the 20 to 0 change has no effect on GPU Z
- All HUD rectangles reach pm_gpu_tex_rect with fill_z = 0.500 (log: "z 0.500", 800 lines, run 2). Star point rects (comb 8, px 182..344, run 1 and run 2) also show z 0.500.
- The render depth is worldPosOffset.z. It sets the draw sort only (hud_element.c:1233, :1353). It reaches a 3D Z only for transformed elements (hud_element.c:1502, z = -z/10). The battle star points use draw_hud_element_internal (hud_element.c:1885), the 2D path, so depth 20 and 0 give the same GPU Z.
- Source: pm_gpu.c:2741 (fill_z = 0.5f default), pm_gpu.c tex_rect uses fill_z for every vertex.
- So the star bar "too far back" is not caused by its render depth. Cause still open. Check draw order against the 3D pass (the bar is drawn before or after the battle actors) on the console with stereo on and off.

## 4. Toad House menu and save dialog, second text and glove: NOT REACHED
- No start. The toad_house_menu route (azt_scenarios.json:2537) runs past the 25 s cap (cap 60 s in the catalog entry). Needs a route or a shorter hook.
- msg.c:2213-2262 (msg_draw_choice_pointer): the glove is drawn twice on purpose (shadow at +2 px, alpha 72, PM_CC_07 at msg.c:2255 with draw_image_with_clipping at posX + 2, posY + 2, then the opaque glove). The second copy seen on the console needs a drawcensus of that frame.

## Trace facts (use)
- Log switches used: trhb=<lo>:<hi> (frame window, temporary pm_trhb_on), hud draw line, rect line (pm_gpu_tex_rect), fill line (gbi_fillrect), copyrect line (startup_draw_prim_rect_COPY), mash line (draw_mash_meter). All temporary.
- Coordinates: screen x = pixel x + 40 (fill px 0..319 maps to screen 40..360). The rect y order is inverted in the log (ya > yb), which is the port y flip.
- Shots: build3ds/lane-trhb/shots/ (run 2, 0000 to 0027 at frames 110 to 248). Run 1 log was overwritten by run 2.

## Commands
- Build: py 3ds/tools/gen_map_restore.py; PM_LANE=trhb PM_BUILD_CAP=1500 sh 3ds/build.sh (cold build about 25 min). map_restore_table.c sha 92645afb unchanged.
- Run: PM_ROW=TRHB PM_LANE=trhb powershell -File 3ds/run_azahar.ps1 -Debug "warp=kmr_03:0 story=ch0_toad_town battle battlef=100 battlemenu runaway shots shotwin=110:182 shotmode=8:30 loglevel=debug trhb=110:182" -Until "shot \d+ frame 182\b" -Timeout 40 -Force

## Open items
1. GPU rect of HES_HPBar: find the early return in gbi_texrect (pm_gbi.c).
2. Flee action entry: hook into the real flee path, then trhb mash and copyrect lines.
3. Star bar: draw order against the 3D pass (console).
4. Glove and text: drawcensus in a menu frame.
