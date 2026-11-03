# Handoff: TEXGARBLE after the wall fix (2026-11-03)

Branch texgarble. Head: see git log. Main has the TEXWIN wall fix (1d6beb014, merged). Azahar budget for TEXGARBLE: 7 of 8 starts used in the current ledger, 1 left.

## Done
- Wall band (kmr_10, Mario at -142 136 221, scenario kmr_10_band): cause is the TEXEL1 (aux unit, tile 1) t clamp. Tile 1 is 32 by 64 CI4, cms mirror mask 5, cmt clamp mask 6, window t 16.25 to 79.25. A clamp axis whose window is shorter than its mask period now repeats (tile_wrap_fix, pm_gbi.c, TEXWIN). Before/after: stripes gone, wall pattern kept.
- Gate: yard top shot (0000_0_11) byte-equal with TEXWIN on and off. mac01t top shots byte-equal (1d6beb014). kmr_10 entry view (kmr_10_dd vs kmr_10_dd_texwin0): one column changed (portrait x 219, y 285 to 348) next to the black screen edge, values differ by 1 to 6. Explained as the window edge on the wall texel.
- Switches (debug, committed): texnear, texclamp, texclamp1, texrep1, texrt1, texwin0. drawlog writes dd and dd1 lines per chunk (tile 0 and tile 1 state).

## Not done
- Unit 0 part of TEXWIN: not applied. Rule for it is untested.
- Bowser fire line: not found. Scouts: bowser_intro_fire (battle switch from kmr_03: no battle, Peach dialogue) and intro_bowser_scout (preset=intro, padscript wait40,A*2 x15, fast; reached Mario's house and the letter from Peach at 84 shots, Bowser not reached).
- Sky row (1 px, BGROW): parked, not visible on Azahar (console only). Numeric method: take the top shot, rotate to 400 by 240, for each row y compute mean absolute difference to rows y-1 and y+1 over the sky columns, score = up + down - 2 * (neighbour-to-neighbour difference). A 1 px garbled row has a large positive score. On the yard top shot every row scored 0 or less, so no such row. Script: scratchpad, python with PIL and numpy.
- Yard bottom shots (bot_*.ppm) differ between TEXWIN on and off from frame 202. Not explained.
- mac01t scenario reports 2 failed checks both before and after. Not explained.

## Needed for the Bowser fire
- The exact story and warp state of the intro Bowser fight. Start from preset=intro and press A through the Peach letter (long dialogue). The fight starts after Bowser arrives. The script event name is in the decomp intro map script (src/world/area_kmr, look for Bowser and EVS intro). The console owner can check the log line for that event.
- Then drawlog=<frame>:1 on the flame frame, and ddonly bisect, then TEXWIN unit 0 test.

## Commands
- Scenario run: PM_ROW=TEXGARBLE powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row TEXGARBLE -Build texgarble -Try
- Justification line in 3ds/logs/perf/status_TEXGARBLE.txt before each start.
