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

## Enemy HP bar and fill edge (fixed in the next commit, see status)
- Cycle type: the HP bar fills (btl_draw_prim_quad, 190B20.c:3039) run in G_CYC_1CYCLE (cyc 0, fillcyc log).
  Screen clears run in G_CYC_FILL (cyc 3, e.g. ul 0,0 lr 319,239).
- Before: gbi_fillrect gave every cycle the inclusive lower right (pm_gpu_fill_rect px(x1 + 1)). A 1-cycle fill was 1 px
  too wide and 1 px too tall. Frame 182 bar: red fill ul 277,184 lr 277,188 and dark line ul 277,183 lr 277,184 at width 0
  drew a 1 px column (5 rows and 2 rows).
- Change: for cyc 0 and 1, lrx-- and lry--, empty rectangle returns. cyc 2 (COPY) and cyc 3 (FILL) stay inclusive.
- Shot proof (start 14 vs start 13, battle_menu_bar, frame 182): the 1 px column at the bar end is gone; rows 331 to 335 unchanged.
  Whole-shot region diff fails (2479 px, 3 regions) from Mario and Goomba motion, which is run-to-run timing (see below).
- Byte equal: battle frames 153 to 162 (shots 0000 to 0003) equal between start 13 and 14. Yard top shot (start 15) equal to start 10.
  Yard bottom shots differ (60 of 60, 146 to 231 px scattered, not a fill edge line). Cause not isolated.
- Title screen: not checked. The boot path reaches game mode 2 late in the log (start 16 hit the 150 s cap at shot frame 704).
- Run-to-run timing: battle shots 0003 and 0005 to 0011 differ between two runs of the same binary pair (start 8 and start 11),
  with no wrapped fill before frame 206. Not proven as timing; the cause is outside the fill change.

## Commands
- Build: PM_LANE=hudb PM_BUILD_CAP=1500 sh 3ds/build.sh (about 2 min).
- Run band test: PM_ROW=HUDB1 PM_LANE=hudb powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/run_azahar.ps1 -Debug "warp=kmr_03:0 story=ch0_toad_town battle battlef=100 battlemenu runaway fillog mashlog texrlog mashx=0 loglevel=debug shots shotwin=150:80" -Until "shot \d+ frame 226\b" -Timeout 60 -Force
- Shots: build3ds/lane-hudb/shots/NNNN_0_2.ppm (240 wide, 400 tall: screen rotated). Shot k = frame in the log line "shot k frame F".
- Yard and bar: azt.ps1 yard | battle_menu_bar -Row HUDB1 -Build hudb (shots in build3ds/azt/shots/<scenario>/).

## Full-screen fills and edge check (after a677ebc76)
- Decomp callers of gDPFillRectangle with a full-screen lower right (cycle set in the display list):
  - 1-cycle, lr SCREEN_WIDTH - 1 / SCREEN_HEIGHT - 1: screen_overlays.c:222 (OVERLAY_SCREEN_COLOR, the screen colour
    fade). This is the one that now stops one column and one row short (x 319, y 239 stay unfaded).
  - 1-cycle, lr SCREEN_WIDTH (exclusive style): background_gfx.c:362. The new rule now covers exactly the screen.
  - FILL cycle (cyc 3, inclusive, unchanged): background_gfx.c:398, 467, 548, 553, 558, 563; state_logos.c:275;
    draw_box.c:778; 16C8E0.c:849 and 856; hud_element.c:1809; screen_render_util.c:212.
  - Other callers (43F0.c, tattle_window.c, virtual_entity.c, msg_draw.c, kmr_20 records, mgm_00 scoreboard, screen_overlays
    :238 viewport) use the cycle set elsewhere or box edges; not all checked.
- Decision: the strip stays (exclusive, the RDP rule given by the coordinator). The N64 cannot be observed here. The
  port hid the strip before because pm_gpu_fill_rect extends fills that reach x 319 to the 400 px edge (F07-3). After the
  change x1 = 318, so that extension no longer fires for this fade. Open for the user: keep the strip (RDP rule) or
  keep full cover for OVERLAY_SCREEN_COLOR (one line in gbi_fillrect or pm_gpu_fill_rect).
- Shot check of the 1-cycle fade: not obtained. The warp start and the battle start fade in cyc 3 (FILL), and the
  battlemenu run did not reproduce the cyc 0 full-screen fade in starts 19 and 20. Budget left: 1 start.
- Warp start fade (FILL, cyc 3), frames 14 to 22: 9 top shots byte equal, refA (before) against B (after) (starts 17, 18).
- Title screen: not checked. The boot path is too slow (start 16 reached only shot frame 704 in 150 s).
- Yard bottom screen (320 by 240, bot_ shots, start 15 against start 10): 60 of 60 differ. Two kinds:
  (a) a 6 by 7 px glyph at x 178 to 183, y 0 to 6 changes between frames of one run (frame or timer digit), 4 px per frame step;
  (b) 1 px edge pixels in clusters at x 140 to 230, y 60 to 70 and x 140 to 150, y 220 to 230 (8962 px in total).
  Kind (b) follows the exclusive rule (1-cycle bottom screen fills). Mask for the glyph: x 162 to 185, y 0 to 8 (4738 px outside it).

## Decision (orchestrator): full cover for OVERLAY_SCREEN_COLOR
- screen_overlays.c:222 now fills 0,0 to SCREEN_WIDTH, SCREEN_HEIGHT under TARGET_3DS (exclusive 1-cycle rule covers the
  whole screen). The N64 path keeps SCREEN_WIDTH - 1. Build check only (no Azahar start). Not shot.
- HP bar rule (1-cycle exclusive) unchanged.
