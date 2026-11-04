# agent_hudb: HUD bars (row HUDB1), handoff

Branch hudb (from native 02e901772). Worktree C:\Users\Andrew\Documents\GitHub\papermario-3ds-port-hudb.
Budget: reset to 8 starts at this session. Used 6 (status_HUDB1.txt starts 7 to 12). 2 left.

## State
- Band fixed: gbi_fillrect reads the 10-bit fill fields as signed (FILL_SX10, pm_gbi.c). A left edge at x < 0 stays
  negative, so the span no longer swaps. CPUFILL loop clamps ul and uly to 0 (negative index guard).
- gbi_texrect needs no change: its 12-bit fields are already sign-extended (pm_gbi.c, the `& 0x800` lines).
- Debug switch mashx=<x> (pm_warp.c pm_mash_posx, called in draw_mash_meter): forces the meter posX. Kept as a debug tool.
- Run-away from the player menu, mashlog, fillog, texrlog switches kept (see run 6 and starts 7 to 8).

## Proven
- Band reproduced (start 7, binary before the fix, mashx=0): fillog f206 to f276 each frame, `raw ul 996,104 lr 32,109`
  (signed -28 and 32). Shot 0015 (frame 210) shows the band: image cols 130 to 135 (screen rows 104 to 109),
  one uniform colour run y 66 to 347 (281 px). The band is the subject.
- Fix (start 8, same command): fillog signed ul -28. Shot 0015 has no band (no column run over 120 px at cols 130 to 135).
  A small clipped piece remains at the top, screen x 0 to 32: the meter clipped at the left edge, as the original shows.
- Yard (start 9 base, start 10 fix, azt yard): top shot 0000_0_11 byte equal (frame 203).
  Bottom shots differ in 28 to 50 px (a 6 by 7 px light glyph, top-left of the bottom screen). Cause not found.
  Not checked against a base-vs-base run.
- Battle (start 11 base, start 8 fix): shot 0013 (frame 202) byte equal, shot 0012 equal. Shots 0003 and 0005 to 0011
  (frames 162 to 194) differ. The change cannot touch these frames: fillog finds no wrapped field before frame 206,
  and sign extension changes only fields with bit 9 set. Probably run-to-run timing. Not proven.

## Enemy HP bar (open, no port error found)
- battle_menu_bar shot 0004 (frame 182, start 12): yellow bar with a dark red line at PPM row 330 (a00000).
- Code: 16C8E0.c:905-931. HP bar = hud element HES_HPBar (32 by 16, texrect, no early return). The fill and the
  dark red line are btl_draw_prim_quad (190B20.c:3039): right = left + w, bottom = top + h, passed to gDPFillRectangle.
- The fill edge is inclusive (background_gfx.c uses SCREEN_WIDTH - 1 for a full-screen fill). So the decomp's
  quad draws w + 1 by h + 1 pixels on N64 too. The port's pm_gpu_fill_rect (px(x1 + 1)) keeps that rule.
- Result: the extra pixel is the game's own call, not a port error. A fix changes game code (decomp call) or the port
  rule for every fill. Needs a decision: match the N64 look (no change) or a 1 px smaller bar (change btl_draw_prim_quad).
  Reference image needed: the user's console or N64 frame.

## Commands
- Build: PM_LANE=hudb PM_BUILD_CAP=1500 sh 3ds/build.sh (about 2 min).
- Run band test: PM_ROW=HUDB1 PM_LANE=hudb powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/run_azahar.ps1 -Debug "warp=kmr_03:0 story=ch0_toad_town battle battlef=100 battlemenu runaway fillog mashlog texrlog mashx=0 loglevel=debug shots shotwin=150:80" -Until "shot \d+ frame 226\b" -Timeout 60 -Force
- Shots: build3ds/lane-hudb/shots/NNNN_0_2.ppm (240 wide, 400 tall: screen rotated). Shot k = frame in the log line "shot k frame F".
- Yard and bar: azt.ps1 yard | battle_menu_bar -Row HUDB1 -Build hudb (shots in build3ds/azt/shots/<scenario>/).
