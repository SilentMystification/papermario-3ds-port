# PT41, PT49, PT50, PT51, PT52: causes and fix text (INVHUD)

Build tested: build3ds/pm_3ds.3dsx of 2026-10-21 16:14 (copy in 3ds/logs/perf/invhud/pm_3ds.3dsx). Azahar: 3 starts, PM_ROW=INVHUD. Shots and logs: 3ds/logs/perf/invhud/r1, r2, r3 (git-ignored). No source file was edited.

## PT41: orange background of choice boxes (PROVEN by code and arithmetic, no shot)
- The boxes with a choice (shop "buy, sell, check", the save prompt) are message boxes with style MSG_STYLE_CHOICE. They are not popup menus. The VIS-1 search of WINDOW_STYLE_1 and WINDOW_STYLE_3 looked at the wrong code. The window style table is correct (src/windows.c:34 to :49: index 13 is the unused slot with style 1, index 14 WIN_POPUP_CONTENT has style 3).
- Cause: src/msg_draw.c:2093 to :2095 read the box fill colour with `((u16*)ui_msg_palettes)[4]`. ui_msg_palettes (src/msg_img.c:42) is a byte array in N64 big-endian order. On the 3DS the u16 read swaps the bytes. Bytes 222,245 give 0xDEF5 on the N64: (27,27,26) * 8 = (216,216,208), a light grey. The 3DS reads 0xF5DE: (30,23,15) * 8 = (240,184,120), orange.
- The same class of fault was fixed for sprite palettes with PAL_MEM (include/macros.h:312 to :317).
- Fix text (src/msg_draw.c:2093 to :2095): wrap each read in PAL_MEM:
  `r = UNPACK_PAL_R(PAL_MEM(((u16*)ui_msg_palettes)[4])) * 8;` and the same for g and b.
- Test: any ShowChoice box. No scene was reached in Azahar (no choice NPC within reach of a warp). Check the fill (216,216,208) in a shot of the Toad Town shop or `warp=kmr_02` Goomba Village talk.
- A/B switches (nogv1, noraw1, nofl1, nodr1 and the rest) do not touch this. The colour comes from the CPU read.

## PT52: garbage pixels at the top left when pausing (PROVEN by shot)
- Repro: `fast warp=mac_00:4 story=ch0_toad_town sp=55 fixedseed loglevel=debug shots shotwin=45:20 shotmode=10:50 padscript=wait160,START*2,wait300`. Shots r1/shots/0020 to 0069 (pause from shot 22).
- Evidence: in every pause shot, row 0 of the picture, game x 0 to 15, holds coloured pixels, for example (87,118,3) and (3,26,56). The rest of row 0 and all other rows are clean (row 0 against row 2 differs in 7 pixels, all in x 0 to 15 of the game area).
- Cause: gfx_transfer_frame_to_depth (src/background_gfx.c:248 to :279) fills the z-buffer image with the filtered frame for y 1 to 238 and x 2 to 317 only. Row 0, row 239 and the columns 0, 1, 318, 319 are never written. The comment at line 274 says the edges are copied, but the loop never visits them. On the N64 the overscan hides row 0. On the 3DS the image lives in memory from pm_zbuf_need (3ds/src/pm_main.c:188 to :195): malloc, not cleared, and the darkness stencil (X02) writes a 64 x 64 image at its start. So row 0 shows stale bytes.
- The pause filter already uses the CPU path (pm_gbi.c:2579, pm_cfb_capture_prev_cpu). The prevcpu switch has no effect on it. The GPU grab (PT12) is not involved.
- Fix text (src/background_gfx.c, end of gfx_transfer_frame_to_depth, and the same in func_80027774): after the loops add, under TARGET_3DS:
  `for (y = 0; y < SCREEN_HEIGHT; y++) for (x = 0; x < SCREEN_WIDTH; x++) if (y == 0 || y == SCREEN_HEIGHT - 1 || x < 2 || x >= SCREEN_WIDTH - 2) zBuffer[SCREEN_WIDTH * y + x] = frameBuffer0[SCREEN_WIDTH * y + x] | 1;`
  It copies the unfiltered edge pixel, as the comment says. Test: the same repro, row 0 of shot 0040 equals row 1.

## PT50: garbled text while the item box enters and leaves (LIKELY, partial)
- Repro: `fast warp=mac_00:4 story=ch0_toad_town testitem=m fixedseed loglevel=debug shots shotwin=100:120 padscript=wait100,right*120,wait300`. Garble in r2/shots/0023 to 0027 (frames 123 to 127), the box is clean from 0028. Sheet: scratchpad copy r2enter.png.
- What the shot shows: the header box ("You got the Mushroom!") is empty while it rotates in. The text of the description ("A regular Mushroom. Restores 5 HP.") is drawn a second time as smeared glyphs at the upper left of the screen. The description box at the bottom is correct.
- Evidence of the mechanism (xtrace and offdump at frame 124, r3/log.txt lines 507 and 607, r3/shots/off124_0.ppm): both rotating windows use the same colour image 0xff76d4. draw_box (src/draw_box.c:770, :772) takes it from mdl_get_next_texture_address (src/model.c:4621), which does not reserve memory, so every rotating window of a frame gets the same address. First fill is 280 x 32 with prim e2e1d8 (description), second fill is 136 x 40 with prim c8bfa4 (header). Both contents land in one slot: the off-screen dump shows the header text and icon on top of the description text, and the description text is visible to the right of the 136 pixel fill. pm_gpu_off_begin (3ds/src/pm_gpu.c:1117 to :1190) finds the slot by (base, siz) and reuses it, so the slot is never a separate image per window.
- Steady state is not affected because a window with scale 1 and no rotation takes the direct path (flags without DRAW_FLAG_ROTSCALE). Only the animation uses the off-screen path.
- Not proven: why the header strips show nothing and the second copy of the description text appears on the screen. A/B switches were not run (start budget used). The cause chain "one slot for two windows" is likely, the exact draw order is a guess.
- Fix text: in pm_gpu_off_begin, a begin for a base whose slot was already sampled in this frame (a strip load named it) takes a new slot (generation counter in the key) and clears it. Then each window has its own image, as on the N64 where the second window overwrites the memory after the first window has been drawn. Raise OFF_N if two windows need three slots. Alternative with no port change: in draw_box, `#ifdef TARGET_3DS` advance the texture heap position by width * height * 2 after each rotating window (call a function that moves TextureHeapPos), so the addresses differ. Check the heap is reset at frame start.
- Test: the repro above, shots 0023 to 0027 show the text inside the header. Run the exit part with `A*2` after wait: `wait100,right*120,wait80,A*2,wait200` and shots 190:60.
- A/B switches to run first on the next start (all one change per run): nobox1, nodr1, nomerge, t1foldall.

## PT49: black pixels near the star point count (NO REPRO, GUESS)
- The status bar did not appear in any shot of the three runs (the bar stays hidden after the pipe scene until Mario idles 240 steps or HP, FP, coins change). The pause stats page with `sp=55` (r1/shots/0040) shows the star point row clean at 1x.
- Checked by code: the star point items are 1-cycle texrects, not COPY mode (src/hud_element.c:589 to :593, filter flag cleared at src/inventory.c:456, :464). So the PT06 COPY rule (pm_gbi.c:2647 to :2653) does not apply to them. The shine image (assets/us/ui/status/star_point_shine.png, IA8 24 x 24, drawn with PM_CC_47) has no black texel with alpha above 0 (checked: 0 of 576). The asset is not the source.
- Open candidates, not tested: the star icon and digits are CI4 with a clamped tile (hud_element.c:480 to :560). A bilinear or tile edge read of index 0 would show a black pixel. The pixels "appear and disappear" with the shine animation and the blink (inventory.c:829 to :856).
- Next test (1 start): add `hp=7` and a coin pick up (`testitem=c`) so the bar shows, then `shotwin=<f>:30` on the bar, and compare with `nogv1 noraw1 nofl1`. Or ask the user whether the pixels are on the overworld bar or on the pause stats page.

## PT51: text on the bottom screen when the pause menu opens (GUESS)
- Azahar, loglevel=debug, pause entry: no line that the console prints at the default level (warn and error) appears between "game mode 10" (frame 136) and frame 190. Lines in that range are info or debug only ("D03: z-buffer image allocated" is info). So the O3DS shows lines that Azahar does not produce.
- Mechanism: every log line of level warn or error goes to the bottom console (3ds/src/pm_log.c:303, :418) next to the HUD lines 1 and 2 (pm_gpu_hud). Pause entry on the O3DS is the heaviest memory step. Candidate warn lines that only the console can produce: "FL1: slab full ... evicted one texture" (3ds/src/pm_tex.c:759, up to 8 lines), "GPU LIMIT NEAR" (pm_gpu.c:1399), "V16b2: CI texel idx" (pm_tex.c:183, :782), "ROM READ" (pm_os.c:275), "B2: no VRAM for the 256 x 512 grab copy" and V05 off-screen errors (pm_gpu.c:1145, :1153, :1205).
- Needed: the O3DS log.txt of one pause (skill pull-o). Command: grep the lines after "game mode 10".
- Fix text, independent of the line: print warn lines to the console only when the debug HUD option is off or `loglevel` was set by hand. Simplest: in pm_log.c:303 and :418 test `pm_opt.dbg_hud == 0 || lv <= 0` (errors only while the HUD shows). The lines stay in log.txt.

## Not done
- Credits shortcuts (message from the coordinator): not part of this row. The INVEND agent owns the credits chain (status file 3ds/logs/perf/status_INVEND.txt: `warp=end_00:0 story=96`, then end_01, kmr_30, `kkj_00:6`). No PT33_40_fix.md was written here because no Azahar start was left and the per-scene entries were not measured.
- Azahar run times (fast mode -F, wall): run 1 about 8 s, run 2 8 s, run 3 8 s.
