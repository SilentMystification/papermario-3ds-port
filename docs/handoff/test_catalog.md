# Test catalog (azt)

## How to use (one page)

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build <lane>` (use `-Try` for an unverified scenario). `-List` lists the scenarios. `-CheckLog <file>` checks an old log and starts nothing.
- Each call is one real Azahar start and counts against the row budget (8 starts per row, reset once by the orchestrator with `run_azahar.ps1 -ResetBudget <row>`).
- Shots of each run go to `build3ds\azt\shots\<scenario>\` (the last run of that scenario). Shots are PPM, 240 by 400.
- A log pass is not a pass. A person must open the shot and confirm the subject.
- Known noise: scattered 300 to 1300 px differences on moving sprites between runs. Judge a shot by the subject, not by pixel counts.
- Status: verified scenarios are listed in the first table. Unverified or log-only scenarios are in the second table with the reason.

### Verified (run with one command each)

yard, tour10, text_menu, battle_start, sewer, ruins, volcano, fileselect_rotate. Menu lag tests with numeric gates (picture read by a person): pause_menu, partner_menu, file_select (section MENULAG). Log-only: tour_k30 (texture counter on kmr_30, delta 0 on lane azt2 at HEAD 0738350b2; picture not applicable). Fail demos: tour_no_warp, attract_no_warp. Completion only: full411.

### Unverified (open, with the console check)

- house_sign: the sign is not readable at 240x400 in the yard (kmr_20). A camboom debug switch did not change the view and was reverted. Console check: warp to kmr_20 entry 4 (`preset=yard`), stand next to the house, face the sign, and look at the sign by eye. Note the sign text and the camera position.
- super_text: no multibounce reached. Run 1 logged actionState 5, 8 and 10 without a SUPER text. Run 2 with dense A presses logged actionState 0 only (the battle menu was not entered). Console check: start a battle with `warp=kmr_03:0 story=ch0_toad_town battle`, select Jump, press A at the landing timing of a multibounce (src/battle/move/jump/multibounce.c), and look for the SUPER text by eye.


Usage (one line): `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario>` (add `-Row <id>`, `-Build <lane>`; `-CheckLog <file>` checks an old log without a start; `-List` lists all).

Defaults: PM_ROW `azt`, build `build3ds\pm_3ds.3dsx`, cap 90 s, first marker deadline 20 s after the Azahar start. Each call is one real start (-Force) and counts against the row budget.
Exit codes: 0 log checks pass, 1 check or run failure, 2 marker missing (stopped), 3 ATTRACT LOOP (stopped), 4 unknown or unverified scenario, 5 argument or build error, 6 no new log line for 40 s (stalled, stopped), 7 until static check failed (nothing run), 9 budget used. Self-test (no start): `azt.ps1 -SelfTest`.
A log pass is not a full pass. Read the shot files and confirm the subject. Shots of each run are copied to build3ds\azt\shots\<scenario>\ (runs before this change: shots are in build3ds\shots and were overwritten).

## Verified (log checks and picture read by eye)

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| tex_kmr_11 | kmr_11 entry 0 with the TEXGARBLE log probes: texstat (stage early free count) and texbind (bind mismatch count). The shot must show Mario and the terrain wall. | `PM_ROW=TEXGARBLE powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 tex_kmr_11 -Row TEXGARBLE -Build texgarble -Try` | `warp: kmr_11 entry 0` | 45 s cap, until shot 5 frame 205 | build3ds/azt/shots/ | Unverified: first start ran to the scene f300 line (20 s), probes gave 0 early frees and 0 binds (bind probe was in the wrong function, moved). Map name kmr_11 is a guess. Entry 0 did not show the band in an earlier shot. |
| tex_kmr_12 | kmr_12 entry 0, frames 200 to 205 shots, with texstat and texbind probes (TEXGARBLE). Shows the picture of the entry and the bind mismatch count. | `PM_ROW=TEXGARBLE powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 tex_kmr_12 -Row TEXGARBLE -Build texgarble -Try` | `warp: kmr_12 entry 0` | 45 s cap, until shot 5 frame 205 | build3ds/lane-texgarble/shots/ | Unverified in catalog sense. Start 3: shot shows a sign and a grass path, no low wall with the band. The band spot needs the user to walk. |
| kmr_10_band | kmr_10 at player position -142 136 221 (pos= switch sets Mario at scene frame 30), frames 200 to 205 shots. Shows the wall band (TEXGARBLE). | `PM_ROW=TEXGARBLE powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 kmr_10_band -Row TEXGARBLE -Build texgarble -Try` | `warp: kmr_10 entry 0` | 45 s cap, until shot 5 frame 205 | build3ds/lane-texgarble/shots/ | Unverified. Picture not read yet. |
| kmr_10_ddonly | kmr_10 at -142 136 221, frame 200 drawlog window with ddonly=78:78 (only the wall chunk 78 draws). Shows whether chunk 78 alone makes the band (TEXGARBLE). | `PM_ROW=TEXGARBLE powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 kmr_10_ddonly -Row TEXGARBLE -Build texgarble -Try` | `warp: kmr_10 entry 0` | 45 s cap, until shot 5 frame 205 | build3ds/lane-texgarble/shots/ | Unverified. |
| kmr_10_dd | kmr_10 entry 0, frames 200 to 205 shots with drawlog=200:1 (chunk list of frame 200) and texstat texbind (TEXGARBLE). Names the chunk and texture of the wall in view. | `PM_ROW=TEXGARBLE powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 kmr_10_dd -Row TEXGARBLE -Build texgarble -Try` | `warp: kmr_10 entry 0` | 45 s cap, until shot 5 frame 205 | build3ds/lane-texgarble/shots/ | Unverified. |
| yard | preset=yard loads kmr_20; steady frame; shot | `yard` | `warp: kmr_20 entry 4` | 15 s | build3ds\shots (run before archive; overwritten). Picture: yard, green pipe, Mario and partner. | Shot writer took frames 200 and 204 only (not 6). |
| tour10 | tour= visits 10 maps by the real map change | `tour10` | `warp: mac_00 entry 4` | 20 s | build3ds\shots (overwritten). Picture: Mario in all 11 shots. | Route list. Phase logging on. |
| mac01 | warp to mac_01 entry 0; phase prints opsplit; shot at frames 200 and 204 | `mac01` (`-Row MODELOPS_R0`) | `warp: mac_01 entry 0` | 16 s | build3ds\azt\shots\mac01\0000_1_2.ppm, 0001_1_2.ppm. Picture: Toad Town post office, Mario and Toads. Two runs, both shots byte-identical. | Until is the first opsplit line (frame 299), not the second: a regex cannot count. The mops line (MODELOPS R0) was removed in DLX2; mac01 has not run since (until and expected events edited only). |
| census_yard | NDT: draw census of frame 200 on kmr_20, old path (log only). One `cen f200 k <key> tex <id0> <id1> tris <t> n <count>` line per distinct flush batch, and `cen f200 summary batches tris distinct over`. Run 1: 46 batches, 510 triangles, 39 distinct, 0 over. | `PM_ROW=NDT powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_yard -Row NDT -Build ndt -Try` | `warp: kmr_20 entry 4` | 21 s (until summary line) | none (log only) | Counts only. Key is a hash of the TevKey fields (no plan_gen) and the draw flags, so it is build specific. Texture id is the slot source hash, format and size. |
| pause_menu | Moved to MENULAG (lag gates, not a picture test). | `pause_menu` | `warp: mac_00 entry 4` | 15 to 17 s | build3ds\azt\shots\pause_menu (top shot frame 130) | See MENULAG. |
| yard_rp | UC3e replay: records replay at the G_DL of their group (rec line runs above 0). Top shot equals the no-replay shot. | `yard_rp` (`-Row UC3e`) | `warp: kmr_20 entry 4` | 20 s | build3dslane-UC3shots0000_0_11.ppm. Picture: Mario in the yard. | Replay gate (UC3e). Reccheck off, so the rec line bad and hit are 0 here; mac01t_rec_chk gives the compare. |
| mac01t_rp | UC3e replay on mac_01 with phase counters (same tour as mac01t_rec_chk). Runs above 0 on the f307 rec line; top shots equal HEAD. | `mac01t_rp` (`-Row UC3e`) | `warp: mac_00 entry 4` | 20 s | build3dslane-UC3shots0000_1_2.ppm. Picture: Mario in Toad Town. | Until is the f614 rec line, which has no records (runs 0 there). Read the f307 line for replays. |
| text_menu | pause dialog text, periods and spacing | `text_menu` | `warp: mac_00 entry 4` | 15 s (until shot 178 to 181; was 91 s timeout) | build3ds\azt\shots\text_menu\0002_1_1.ppm | Text read by eye. Same pause open as pause_menu, until is the first shot at 178 to 181. Picture (FINAL2 run, shot 178): pause status page, dialog text readable. |
| mapchange_hud (old text, superseded by POSHUD2) | HUDMAP: bottom screen row 1 (top row) shows `Now in mac_01 (area 1, map 2)` at frames 226 to 228 (hold of 60 steps from the change at step 170), and `Map mac_01 (area 1, map 2)` at frames 231 and 234 | `mapchange_hud -Row HUDMAP -Build hudmap -Try` | `warp: mac_00 entry 4` | 15 s | build3ds\azt\shots\mapchange_hud\bot_0228.ppm, bot_0231.ppm, bot_0234.ppm | Picture read by the code agent (run 3). Run 1 put the row at screen row 0 wrapped to the bottom, under the log; run 2 moved it to the top row. |
| exit_armed | HUDMAP: port menu (SELECT), row 3 Exit to Homebrew Menu after one A press: `A again` is red, the other rows are white | `exit_armed -Row HUDMAP -Build hudmap -Try` | `warp: mac_00 entry 4` | 15 s | build3ds\azt\shots\exit_armed\bot_0203.ppm, bot_0205.ppm | Picture read by the code agent (run 4). Until on the top shot at frame 203 to 205. |
| battle_start | battle switch starts a battle on kmr_03 | `battle_start` | `warp: kmr_03 entry 0` | 15 s | build3ds\shots (overwritten). Picture: field, then battle HUD with Mario and Goomba. | Shot line holds a burst number, not the frame index. |
| sewer | warp into tik_03 (Toad Town Tunnels) | `sewer` | `warp: tik_03 entry 0` | 15 s | build3ds\azt\shots\sewer\0000_2_2.ppm, 0001_2_2.ppm | One map. Shot at frame 200. |
| ruins | warp into isk_04 (Dry Dry Ruins) | `ruins` | `warp: isk_04 entry 0` | 16 s | build3ds\azt\shots\ruins\0000_11_3.ppm (see folder) | Story text box covers the right side. |
| volcano | warp into kzn_19 (Mt. Lavalava) | `volcano` | `warp: kzn_19 entry 0` | 15 s | build3ds\azt\shots\volcano\ (see folder) | One map. |
| attract_no_warp (fail_demo) | ATTRACT LOOP: 3 title or demo lines in a tour run | `attract_no_warp` | `boot_main` | 39 s (stopped) | none | Exit 3, `ATTRACT LOOP: 3 title or demo mode lines`. |
| tour_no_warp (fail_demo) | fail-fast: marker missing | `tour_no_warp` | `warp: [a-z0-9_]+ entry` | 21 s (killed) | none | Exit 2, `SCENARIO FAILED: marker missing`. Azahar killed, none left. |

## Unverified (run or not, with the reason)

| Scenario | Reason | Shot paths |
|---|---|---|
| yard_wide (ZOOM, unverified) | Yard with `wide`: zoomed original view, sky 240 rows, status bar at the top. `azt.ps1 yard_wide -Row ZOOM -Try`. Picture not read yet. | build3dszt\shots\yard_wide |
| yard_stereo_l, yard_stereo_r (ZOOM3D, unverified) | Yard with `wide stereo` (stereo forces the 3D slider to 1; `shoteye` dumps the right eye). Run both with `-Row ZOOM3D -Try`. Picture read by agent: both eyes draw, the right eye differs by depth, the sky has no parallax. A person must confirm on the console. | build3dszt\shots\yard_stereo_l, yard_stereo_r |
| mac01_seam (SEAM, unverified) | mac01 with `seamstat`: the line `seam: cpu path triangles, object route N, clip route M` at frame 240. Run `azt.ps1 mac01_seam -Row SEAM -Try`. Result: object route 2548, clip route 0, raw path 257137 (totals at frame 240). Shots byte-equal to before the change (Azahar computes in float32, so it cannot show the PICA seams). | build3ds\azt\shots\mac01_seam |
| nok02_tri (FLK1, unverified) | nok_02 at pos 250 0 11, frame 200 with `tritrace=200:1` (per-triangle corner w, X, Y, clip bits, cull sign and decision in draw_tri_cpu). Run `azt.ps1 nok02_tri -Row FLK1 -Build flk1 -Try`. Run 1: 10 triangles of frame 200 reach draw_tri_cpu, all `early-keep`, all w > 0, no cull or clip reject. The shot shows Mario on the path; the end piece is not confirmed by eye. | build3ds\azt\shots\nok02_tri |
| nok02_nobox (FLK1, unverified) | nok_02 at pos 250 0 11, frames 200 to 205 with debug `nobox` (bare word) (the dlc_try group box skip is off). Control for nok02_tri: a visible picture difference means the skip hides geometry at this camera. `azt.ps1 nok02_nobox -Row FLK1 -Build flk1 -Try`. | build3dszt\shots
| nok02_noboxint (FLK1, unverified) | nok_02 at pos 250 0 11, frames 200 to 205 with debug `noboxint` (bare word) (an out group runs through the CPU interpreter instead of the skip). Splits nok02_nobox: picture equal to nok02_tri in the top-right region means the skip matches the interpreter. `azt.ps1 nok02_noboxint -Row FLK1 -Build flk1 -Try`. | build3dszt\shots
ok02_noboxint |
| nok02_dlccheck (FLK1, unverified) | nok_02 at pos 250 0 11, 300 frames with debug `dlccheck` (bare word). The dlc2 line: grp, skip (box skip), missing (triangles of a skipped group the interpreter keeps), xgrp (triangles of a skipped group that use a slot an earlier group loaded). Run 8 (2026-11-03): skip 12452 of grp 25502, missing 0, xgrp 0. `azt.ps1 nok02_dlccheck -Row FLK1 -Build flk1 -Try`. | none (log only) |
| nok02_skipdump (FLK1, unverified) | nok_02 at pos 250 0 11, frames 200 to 205 with debug `skipdump` (bare word): each group logs `skipg f<frame> k op0 ntri nvtx skip plane behind scr x y box` (screen box with the viewport of pm_gbi, first rejecting clip plane). `azt.ps1 nok02_skipdump -Row FLK1 -Build flk1 -Try`. | build3dszt\shots
ok02_skipdump |
ok02_nobox |
| nok14_lake (TEX1, unverified) | nok_14 at 27 0 -44, frames 200 to 205. Shows the lake terrain for the TEX1 clamp pad (branch tex1-clamp). Run 1 (HANG1 build, the baseline): picture read, terrain shows no wrong texture in this window. Run 2 (tex1 build): only a moving sprite differs (x 100 to 181, y 279 to 314). Entry 0 and the lake position are not checked against the map. Picture read again (agent_playfix2 run): the window shows lake water, grass and the lake edge, so the window is valid. The clamp change does not change the terrain pixels: TEX1 is not merged to native (CLAUDE.md 5 and 7). Branch tex1-clamp kept. | build3ds/azt/shots/nok14_lake |
| toad_house_menu (UI1, unverified, probe) | mac_01 entry 1 with `pos=490:20:-165` next to the Toad House keeper (488 20 -174), A presses, shots 150 to 326 every 4 frames. Runs 1 and 2 walked from entry 1 and were blocked by enemies at x 479 z -50 (no door reached). Run 3: Mario settles at 493.4 20 -149.6 (outside the house wall), shots show the outside wall. Room entry not found yet. `azt.ps1 toad_house_menu -Row UI1 -Build hang1a3 -Try`. | build3ds/azt/shots/toad_house_menu |
| ui1_choice (UI1, unverified) | yard with debug hook `uishow=choice` (pm_warp.c, frame 200): opens MSG_Choice_000A through msg_get_printer_for_msg (the ShowChoice call). Shots 200 and 204: NO menu visible (field only). The printer opens (flag 2) but does not draw. Open: the draw state or position. `azt.ps1 ui1_choice -Row UI1 -Build ui1 -Try`. | build3ds/azt/shots/ui1_choice |
| nok11_splash (HANG1a, log gates pass, picture by eye pending) | nok_11 entry 0 with fxtest=89 (water splash at Mario) and grablog. Azahar stops with svcBreak in gxCmdQueueAdd at the first splash frame (frame 403, shot 12) on the build with the cap (lane hang1a) and on the baseline (lane hang1base). Run 4 (lane hang1a3): 21 frames, one grab per frame (rows 0-240), shot a at frame 412. The break is fixed by pm_gx_room (GX queue drain past 24 entries). | build3ds/lane-hang1a |
| yard_live | LIVEMSG: yard with `livelog`; the live-log status line is in log.txt after pm_log_start. Picture not yet read. | `yard_live` (`-Try`) | `warp: kmr_20 entry 4` | 20 s | build3ds\aztshotsyard_live (run and shot read by eye pending) | Azahar may not open the soc socket: a socInit failure line is the expected result there. |
| golden_multi | NDT3: six maps in one start (kmr_20 preset, mac_00, tik_03, isk_04, kzn_19, mac_01), one top shot per map at 30 frames after each map is in (`tourshot=30`). Run 2: 7 shots (shot 0 is a second kmr_20 shot at boot), 17 s from the Azahar start to shot 6. Compare two runs with `python3 -I 3ds/tools/region_diff.py <dirA> <dirB>` (small scattered differences pass, a region above 64 px fails). | `PM_ROW=NDT powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 golden_multi -Row NDT -Build ndt -Try` | `warp: kmr_20 entry 4` | 17 s (until shot 6) | build3ds/azt/shots/golden_multi/ | Unverified: the picture is not read and no HEAD run exists yet to compare with. Pause, title, fileselect and battle scenes are not in this run (they need pad script or boot states). |
| tour_k30 | Log passes (tour: done, 6 maps). Picture fails: 4 of 7 shots show a grey page, the Prologue card and a text box. The fix (shot after each map load-done plus 15 frames) is a game source change in pm_warp.c or pm_gpu.c, outside this tool. The file holds 6 maps, not 30. azt2 tried a shot burst 15 frames after each map is in (tourshot=15:21), then 30 frames (tourshot=30:9): both still show 4 of 7 non-map shots (grey page, Prologue card, dark text box). Edit reverted. Log-only since 4b0ad69d4: the kmr_30 texture counter line is checked, picture N/A (cutscene overlay on 4 maps). Re-run on the default build (binary 04:50, before the lane rebuild): delta 420. Rebuilt lane azt2 from HEAD 0738350b2 (-Build azt2, budget reset): tex fail cum 0, delta 0 on kmr_30, same args and tour file; vramfree min 2130 KB (default build: 3858 KB). The 420 came from the older default binary. Closed as log-only. | build3ds\azt\shots	our_k30 |
| partner_menu | Moved to MENULAG (lag gates; top shot frame 200 is the Party tab; frame 220 showed the Star Spirits page (FINAL4 run) and was moved to 200). | `partner_menu` | `warp: mac_00 entry 4` | 15 to 16 s | build3ds\azt\shots\partner_menu | See MENULAG. |
| file_select | Moved to MENULAG (lag gates; top shot frame 100 is the file select screen). | `file_select` | `boot_main` | 15 s | build3ds\azt\shotsile_select | See MENULAG. |
 FINAL2 run: until is shot 338 (was 3[4-9][0-9], never matched, 90 s timeout). 16 s. Picture read: File select, File 1 AAAAAAAA First Play, Select file to start.
| fileselect_rotate | Same command as file_select. File select not reached, so frames 282 to 296 are not shown. | same as file_select |
 Until changed to shot 29[0-6] (was 90 s timeout). Not re-run: budget. Status unverified.
| house_sign | Open. Warp kmr_20 entry 4 passes. Sign not readable at 240x400. A camboom switch (boom length on nextRig and prevRig) did not change the view and was reverted. | build3ds\azt\shots\house_sign |
| super_text | Open. Battle starts. Run 1 logged actionState 5, 8 and 10, no multibounce. Run 2 (dense A presses) logged actionState 0 only. No SUPER text in any shot. | build3ds\azt\shots\super_text |
| mac01t_mr0 (MR0, unverified; MR0 census) | Same view as mac01t, with the mr0 line at f899: plain static model count per frame and us per model (ops, bind, close, outside dlc, dlc). Run 2 (build with unit fixes): plain 86 per frame, per model us ops 17, bind 10, close 9, outside dlc 37 (Azahar), console x2.86 outside dlc 107. Gate 120 us outside dlc: failed. Run 1 had a dlc hook bug and was discarded. MS0 line (same run 3, until ms0 f899): opaque plain models 70 per frame, runs 13, removable closes 15 per frame (upper bound, gate 20: failed), texture changes 46. Picture: not read yet. | `azt.ps1 mac01t_mr0 -Row MR0 -Build mr0 -Try` | build3ds\azt\shots\mac01t_mr0 |
| mac01t (FINAL4, verified) | Census view. FINAL4 run: mdlsum f899 tot ops 1696 in 1623 to 2065, 6 of 6 log checks. Top shot 0000_1_2 (frame 500) shows Mario and Toads on a Toad Town street, HUD on. Census view of mac_01 reached by the tour path (`-Try`): warp mac_00 entry 4, tour step mac_01 entry 0 at about frame 209. Window is mdlsum f899 (frames 600 to 899). Gate 1175 to 1495 (1335 +/- 12 %, FINAL9; R2B measured 1335). Older gate 1623 to 2065 (1844, set after EN2, T1B, NP1B, HT1) is outdated: R5B measured 1524 and R2B 1335. Older gate 1900 to 2200 is outdated: HEAD 2315 and T1B 1844 were measured before the range was set. | build3ds\azt\shots\mac01t |
| forest_mim (FOREST diagnostic, unverified) | Warp to mim_05 entry 0 (area 12 map 4, Forever Forest). Command: `azt.ps1 forest_mim -Row FOREST -Try`. Run 1 (default build): 5 of 5 log checks. f599: opsplit ops 980, mdlsum draws 6, tris 12, dlc2 static tri 55800 of 68100. Picture read by agent: dark forest, Mario and partner. A person must confirm. No numeric gate. Scenario added for diag_forest.md. | build3ds\azt\shotsorest_mim |
| sun_sbk (SUNRAYS, unverified) | Dry Dry Desert sun overlay and ray layers (sbk_00, entry 0, story ch0_toad_town). Run with `-Try`. Debug string adds `suntile=200:1` (tile 1 and tile 0 state, triangles, sun spawn and draw; switch suntile in pm_gbi.c). Runs (SUN1): 2 starts, 3 of 3 log checks each. Picture read by eye: faint hard-edged cells in the sky strip, no sun disc (the sun base list draws no triangle). See docs/handoff/diag_sunrays.md. | build3ds\azt\shots\sun_sbk |
| yard_stc (UC2 reccheck epic, unverified) | Yard (kmr_20, entry 4) with the stprobe and reccheck switches (log only). UC2P site lines (stencil sites 1 to 3) and the UC2D line: ctx+0xF4 word (citro3d context, register 0x105 word) beside the last C3D_StencilTest word. START 10 (last start, row UC2): 3 of 3 log checks. UC2D at frame 36: ctx+0xF4 ff000010, last word105 ff000010; UC2D first differing word j 75, valid record a 00000000, frame b 00000110. Sites 1 to 3 at frame 35: ctx equals port word. Until rec f348 (frame 340s, past frame 35). Picture not read by the agent. | `azt.ps1 yard_stc -Row UC2 -Build UC2 -Try` | build3ds\azt\shots\yard_stc |
| toad_gate (TOADVIS, unverified) | Toad Town west gate view (mac_00 entry 0, preset toad_west), shots at frames 200 and 204. Three runs: working tree, 29f504d7b, final6 (3dcfc661d). Picture: west gate path, no water, no fountain in view. Budget of TOADVIS used up. See docs/handoff/diag_toadvis.md. | build3ds\aztshots	oad_gate |
| mac01s (TOADVIS, unverified) | Same view as mac01t (tour to mac_01 entry 0), until on shot line 504 (cap 40 s). Working tree build: 2 of 2 top shots byte-equal to final6 and 29f504d7b. Strip at the right edge is not confirmed as water. mac01t until mdlsum did not print on 29f504d7b (idled 121 s). | build3ds\aztshotsmac01s |
| toad_fountain (TOADFOUNT, unverified) | Mac_01 walk by padscript: entry 0 (x -600) to x -300 (seg b, frame 114), then to x -170 (seg c, frame about 158). Shots every 4 frames, frames 100 to 148 (13 shots). Until on shot frame 148 (cap 40 s). Runs: default build (HEAD f230e46c6, build 1488), `-Build final` (29f504d7b), `-Build toadf6` (final6 3dcfc661d). No fountain in view in any run: the shots show the Toad Town street and a post office side. Top shots differ in 690 to 2100 px, all at x 70 to 144 (moving NPC sprites). Next test needs a view of the candidate at about x -300, z -325 (docs/handoff/diag_toadfount.md). | build3dsaztshots	oad_fountain |
| battle_start_enc (BTLSTAR, unverified) | Battle start star transition (OVERLAY_START_BATTLE) on kmr_03. `encwarp=120` enters ENCOUNTER_STATE_PRE_BATTLE for the first world enemy (forced: the touch test did not start the battle, see docs/handoff/agent_btlstar.md). Log checks: pre-battle entered, pre-battle start, shot in window. Until: `encwarp: window end` (40 shots from the pre-battle start, 10 saved at 4-frame steps). Run 7: 4 of 4 log checks. Picture read by agent: field with Mario, a black wedge growing at the lower right at frames 174 to 210; no star shape read. Frame time in the window: gbi 140 to 177 ms per frame, fbread avg 0.2 ms, one CPU readback (D03, 8731 us). Azahar numbers only. | `azt.ps1 battle_start_enc -Row BTLSTAR -Build BTLSTAR -Try` | `warp: kmr_03 entry 0` | 16 s | build3dszt\shotsattle_start_enc | Cap 45 s. Star not confirmed. |
| battle_start_fb (BTLSTAR FB1, unverified) | battle_start_enc with the fbbind switch: one line per 16 bit RGBA bind and load (hit or miss). Run 7 (baseline build): 41 strip misses, all in frame 174 (the first transition frame, one readback D03); 1363 later strip hits; 1404 strip binds. FB1 build: 0 strip loads, 1 full-frame load (320 x 240) in frame 174, flush 71 per transition frame unchanged. Shots 0001 to 0014 differ from battle_start_enc shots by 7 pixels at x 149, y 157 to 163. | `azt.ps1 battle_start_fb -Row BTLSTAR -Build BTLSTAR -Try` | `warp: kmr_03 entry 0` (marker line logged, deadline check failed in one run) | 15 s | build3ds\lane-BTLSTAR\shotsattle_start_fb; log build3ds\lane-BTLSTAR\last_log.txt | Cap 45 s. Baseline shots overwritten by the FB1 run. Byte compare (run 9 and 10, parent 4bd4ad4f8^ vs FB1): shots 0000 to 0014 byte equal (the 7 px note above was the older battle_start_enc build, not this fix). Strip loads 1404 to 0; 1 full-frame load in frame 174. Flush 71 per transition frame (one flush per draw; normal frames 28 and 28). Readback (D03) 8.8 ms, once per transition. |
| battle_start_gp (BTLSTAR, unverified) | battle_start_enc with gbiprof: flush count by cause (PG0 report, pm_gpu.c) and phase lines for the transition frames. Used to name the 71 flushes per transition frame after FB1. | `azt.ps1 battle_start_gp -Row BTLSTAR -Build BTLSTAR -Try` | `warp: kmr_03 entry 0` | 15 s | build3ds\lane-BTLSTAR\shotsattle_start_gp; log build3ds\lane-BTLSTAR\last_log.txt | Cap 45 s. Run 15: shots 0 to 14 written, run rc 2 (crash symbols) with warp and encwarp markers in the log. No PG0 flush-cause report: gbiprof prints every 300 frames (pm_gbi.c:2266), the window is 40 frames. Not verified. Run 2 (gbiprofn=40, PG0 print added): flush calls per frame 432.55 in the transition window, of which 403 find an empty batch (about 29 do work, phase flush 29 to 71). Top cause pm_gpu_set_mode:2280 146 per frame (mostly empty). 40 consecutive texrects (rects 93, consecutive 40). Yard byte equal after the debug change. |
| merlon_ball (MERLON, unverified) | Merlon room (mac_01) with pos=-264:20:-326 (user HUD position), shots 44 to 59 with grablog (crystal ball window). | `azt.ps1 merlon_ball -Row BTLSTAR -Build BTLSTAR -Try` | `warp: mac_01 entry 0` | 15 s | build3ds\lane-BTLSTAR\shots\merlon_ball; log build3ds\lane-BTLSTAR\last_log.txt | Cap 45 s. Run 16: log checks 2 of 2. Readback: placed at -264.000 20.000 -326.000 on mac_01, settled same. grab lines in frames 43 to 49: 0 (the crystal ball window did not grab). Picture not read by agent. |
| merlon_ball60 (MERLON, unverified) | merlon_ball with texlog (debug, one line per texrect: size, cycle, render mode, z source, tile formats) and shots at frames 63 to 95 (shotwin=60:40, every 4 frames). Result: top shots 63 to 79 show a Toad Town street with a blue star sign on a wall (scene object, flat, not round); 83 to 95 show a purple room with a chest. No round crystal ball in the view. No 33 by 33 texrect (ball quad, 32 px plus one) in these frames. The 32 by 32 readback quad is not drawn in this camera view. | `azt.ps1 merlon_ball60 -Row BTLSTAR -Build BTLSTAR -Try` | `warp: mac_01 entry 0` | 15 s | build3ds\lane-BTLSTAR\shots\merlon_ball60 | Cap 45 s. The count in shotwin is 40 (frames), not a step. |
| merlon_door (MERLON, unverified) | warp mac_01, Mario placed at -203 20 -206 (settles at -203 20 -242), padscript go_-203_-280_500 then up*60 into the Merlon door, shots 130 to 189 with grablog and texlog. Result (2 runs, starts 7 and 8): Mario walks to the doorstep (frames 123 to 133) and stays outside; the room interior does not load; no 33 by 33 ball texrect, no grab lines. The padscript door entry does not work yet. | `azt.ps1 merlon_door -Row BTLSTAR -Build BTLSTAR -Try` | `warp: mac_01 entry 0` | 15 s | build3ds\lane-BTLSTAR\shots\merlon_door | Cap 45 s. |
| pause_spin (BTLSTAR, unverified) | Pause menu open, spin frames 140 to 179 (pause opened at frame 96). Shot window 40 frames. Add `grablog` to the debug string for the per-grab lines (GRAB2 switch, pm_gpu.c grab_log). Picture not read. | `azt.ps1 pause_spin -Row BTLSTAR -Build BTLSTAR -Try` | `warp: mac_00 entry 4` | 45 s | build3dszt\shots\pause_spin | Until: shot frame 176 to 181. One start not yet run. |
| pause_spin_grab (VRAM2, unverified) | pause_spin with the grablog and texstat switches. Picture: pause status screen (frame 176 area). No grab line in the pause (the pause draws with the CPU path, no copy block). | `PM_ROW=VRAM2 powershell -File 3ds/tools/azt.ps1 pause_spin_grab -Row VRAM2 -Build native -Try` | `warp: mac_00 entry 4` | 45 s | build3dszt\shots\pause_spin_grab | Texstat cap line is not reached (report at frame 300). Use yard_vramfill for the cap. |
| yard_vramfill (VRAM2, verified run 1) | yard with `vramfill=3` (three 512 KB dummy blocks in bank B, after the cache boot blocks) and texstat, until the frame 300 report. Log only. | `PM_ROW=VRAM2 powershell -File 3ds/tools/azt.ps1 yard_vramfill -Row VRAM2 -Build native -Try` | `warp: kmr_20 entry 4` | 30 s | build3ds\lane-native\last_log.txt | Run 1: `texstat f300: cap 1856KB` (constant after boot), `vramfill: block 1 of 3 failed`, no shrink line. Pass. |
| mac01_texstat (CACHE1, unverified) | mac_01 (heaviest map in the catalog) with texstat until the frame 600 report. Log only. | `PM_ROW=CACHE1 powershell -File 3ds/tools/azt.ps1 mac01_texstat -Row CACHE1 -Build native -Try` | `warp: mac_01 entry 0` | 60 s | build3ds\lane-native\last_log.txt | Run 1: live 85 slots at f300, 104 at f600 (510 KB), 0 evictions, cap 1856 KB. |
| kzn19_texstat (CACHE1, unverified) | kzn_19 with texstat until the frame 600 report. Log only. | `PM_ROW=CACHE1 powershell -File 3ds/tools/azt.ps1 kzn19_texstat -Row CACHE1 -Build native -Try` | `warp: kzn_19 entry 0` | 60 s | build3ds\lane-native\last_log.txt | Run 1: live 323 slots at f300, 459 at f600 (1099 KB), 0 evictions. 512 slot limit is 90 percent used and rising. |
| kzn19_slotpeak (SLOT1, unverified) | SLOT1: kzn_19 to the frame 1500 report, slot limit 2048 (measurement build). Peak live slots, under-1-KB count, evictions by cause. Log only. | `PM_ROW=SLOT1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 kzn19_slotpeak -Row SLOT1 -Build native -Try` | `warp: kzn_19 entry 0` | 150 (turbo, real wait 50 s) | build3dslane-native\last_log.txt | Measurement only; not a picture test. |
| battle_slotpeak (SLOT1, unverified) | SLOT1: battle switch (battlef=100) to the frame 600 report. Peak live slots in a battle. Log only. The encwarp path stalled at frame 174 (crash symbols), not used. | `PM_ROW=SLOT1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 battle_slotpeak -Row SLOT1 -Build native -Try` | `warp: kmr_03 entry 0` | 90 (turbo) | build3dslane-native\last_log.txt | Measurement only; not a picture test. |
| pause_slotpeak (SLOT1, unverified) | SLOT1: pause menu open to the frame 300 report. Peak live slots with the pause open. Log only. | `PM_ROW=SLOT1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 pause_slotpeak -Row SLOT1 -Build native -Try` | `warp: mac_00 entry 4` | 45 | build3dslane-native\last_log.txt | Measurement only; not a picture test. |
| chain_slotpeak (SLOT1, unverified) | SLOT1: tour mac_00, mac_01, kzn_19, mac_01 to the frame 1500 report. Peak live slots across map changes. Log only. | `PM_ROW=SLOT1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 chain_slotpeak -Row SLOT1 -Build native -Try` | `warp: mac_00 entry 4` | 150 (turbo) | build3dslane-native\last_log.txt | Measurement only; not a picture test. |

## POSHUD (2026-10-29): bottom screen position values

- Switch `poshud` (debug string, or options.ini `poshud=1`, Debug tab row 8 "Position HUD"). Bottom screen rows 0 and 1 show Mario x y z yaw and the camera x y z yaw. Refresh: hudint (the memory rows are replaced by these two rows while on).
- Mario yaw is gPlayerStatus.targetYaw. Camera values are gCameras[CAM_DEFAULT].lookAt_eye and curYaw.

| Scenario | Flags | Command | Result (build poshud, same code as the commit) | Status |
|---|---|---|---|---|
| yard_poshud | `poshud` | `yard_poshud -Try -Row POSHUD -Build poshud` | 3 of 3 log checks. Bottom shot bot_0259: `M x 594 y 30 z -216 yaw 180`, `C x 924 y 161 z 60 yaw 310`. Log at f300 gives Mario pos 594 30 -216. Top shot 0000_0_11 byte-equal to yard_headA (HEAD e16fee4b9). | unverified (one run, before the POSHUD2 row change; camera row removed since) |

## POSHUD2 (2026-11-03): fixed HUD rows, Mario row, stats row, map block rows 4 to 6

HUD row table (fixed rows 1 to 6 always; optional rows follow): 1 fps, 2 memory, 3 VRAM, 4 to 6 map block (during the 2 s change: old id; old area and map, `->`, new id; new area and map. Else: current id; area and map; blank). Then 7 Mario `Mario x y z yaw` (poshud), next stats (stats). Then separator, header, log window to row 26, separator 27, footer 28 to 30. Separator = 7 + (poshud) + (stats), header one below, log from 9 + (poshud) + (stats) to 26.

Cases: neither: separator 7, header 8, log 9 to 26. poshud: separator 8, header 9, log 10 to 26. both: separator 9, header 10, log 11 to 26.

| Scenario | Flags | Command | Result (build POSHUD, the commit) | Status |
|---|---|---|---|---|
| toad_pond | `poshud` | `toad_pond -Try -Row POSHUD -Build POSHUD` | Picture read, bot_0160 `Mario x -142 y 0 z -36 yaw 189`, bot_0162 `x -144 y 0 z -27 yaw 189`, bot_0199 `x -168 y 0 z 93 yaw 195`: row 5 changes between frames 2 apart. Row 4 blank, separator row 9, header row 10, footer rows 28 and 29 in place. Run 1 exit 2 (Azahar exited after 6 s while another azahar.exe ran). Run 2 exit 2 (marker check read a log.txt that the run did not write; the game log `build3ds/lane-POSHUD/last_log.txt` has the warp line). `-CheckLog` 3 of 3. | unverified (picture read, tool exit 2) |
| mapchange_poshud | `poshud`, stats off | `mapchange_poshud -Try -Row POSHUD -Build POSHUD` | 4 of 4 log checks. Picture read: bot_0227 (hold) rows 4 to 6 `mac_00` / `area 1 map 1 -> mac_01` / `area 1 map 2`, row 7 Mario, row 8 separator, row 9 header. bot_0232 (after) rows 4 to 5 `mac_01` / `area 1 map 2`, row 6 blank, row 7 Mario. Block shows until frame 231. Run 2 of this scenario. | verified (picture read, 2 runs) |
| yard_poshud (VRAM row) | `poshud` | `yard_poshud -Try -Row POSHUD -Build POSHUD` | 3 of 3 log checks. Picture read, bot_0259: row 3 `VRAM 4764/6144K tex 358/3072K`, row 7 `Mario x 594 y 30 z -216 yaw 180`. tex used 358 <= 3072, overall 4764 >= 358, overall 4764 <= 6144. | verified (picture read, 1 run) |
| mapchange_allrows | `poshud`, `stats` | `mapchange_allrows -Try -Row POSHUD -Build POSHUD` | 4 of 4 log checks. Picture read, bot_0228 (hold) and bot_0233 (after): all rows present. Rows 1 to 3 fps, memory, VRAM; 4 to 6 map block; 7 Mario; 8 stats (`t512 i322 ...`); 9 separator; 10 header; 11 to 26 log; 27 separator; 28 map; 29 apt. | verified (picture read, 1 run) |
| mapchange_hud | none (both off) | `mapchange_hud -Try -Row POSHUD -Build POSHUD` | 4 of 4 log checks. Picture read, bot_0228 (hold): rows 1 to 3 fps, memory, VRAM; 4 to 6 map block; 7 separator; 8 header; 9 to 26 log; 27 separator; 28 map; 29 apt. Replaces the old row 44 (`Now in` text, superseded). | verified (picture read, 1 run) |
| botlayout | none (poshud off) | `botlayout -Try -Row POSHUD -Build POSHUD` | 3 of 3 log checks. Pixel compare with the botlayout run (bot_0049 to bot_0054): rows 7, 8, 27, 28 and 30 byte-equal in all 6 frames. Rows 1 to 3, 9 to 17, 24 and 29 differ (fps and memory values, log text, build tag); rows 4 to 6 equal. Run before the rework (map block rows 4 to 6 are new). | unverified (compare, not a picture read; before the rework) |

## Completion only

| Scenario | Reason | Command | Runtime |
|---|---|---|---|
| full411 | One start runs tour_s420.txt (411 maps) to tour: done. Not run in this task (lane 1 runs it). Same args ran at 2026-10-29 03:51 (rc 0, 143 s). | `full411` (-Milestone, cap 1800 s) | 143 s (earlier) |

## Lane 3 notes (2026-10-29, lane3a build, rows lane3a to lane3d)

- house_sign (verified by eye, different view): `preset=intro shots shotwin=580:24`, shot 0000 (frame 580) shows the Mario house exterior with the MARIO sign on the roof, readable at 8x crop. Letters are whole and not garbled. Scene is the intro house, not the kmr_20 yard. The yard run (row lane3b, frames 200 and 204) does not show the sign readable.
- text_menu (lane3d, 4 shots, pause status page): dialog and item text read at 6x and 8x. Periods after "is." and the stat labels show no misplacement by eye. No glyph code changed. Frame-178 log check fails because the shot line is not written; the shots exist.
- intro cutscene box (lane3c, frames 580 to 600, shots every 4 frames, 6 shots): no stage box and no bounding edge visible. The scene is the Mario house with a "Mail call!" message box. Frames 602 and before 580 not captured. Not reproduced in this build.
- super_text (intro): not reached. The intro cutscene has no battle, so the multibounce path does not run. Unverified.

## Lane 3 note: intro box capture and scenario frame fix (2026-10-29)

- Scenario fix: pause_menu and text_menu waited for "shot N frame 178". Shots land on frames 170, 174, 177, 180, so frame 178 is never written. The regex also had a JSON escape error (`\b` read as backspace). Both now use `shot \d+ frame 180\b`. The regex was checked offline against a log line. The scenario is not re-run: the Azahar lock was held by a run that did not stop (see below).
- Intro boot path (no preset, lane3a, shotwin=300:400): shots every 4 frames from frame 300 to 699. Frame 300 to 326: not in the stage. Frame 327 onward: stage with grey panel, gold checker bands and red curtain. Frame 588: sky strip rises from the bottom edge (matches diag_cutscene_box.md box3). Frame 620: sky fills the panel inside the gold bands, checker walls and curtain. No border outside the stage frame was seen in these shots. Cause not decided: no N64 reference frame.
- Not captured: frames 0 to 299 and frame 700 and later.
- preset=intro (lane3a, shots every 30 calls, turbo): starts in the Mario house with the "Mail call!" box. The stage does not appear in this preset.
- Run control: the Turbo run and the shotwin run did not stop at -Timeout. A run kept Azahar alive for more than 25 minutes (frame 9905 logged) and held the lock.

## LOGCLEAN note (2026-10-29)

- yard and pause_menu debug strings now add `loglevel=debug` (3ds/tools/azt_scenarios.json). text_menu and house_sign keep the old string.
- Row LOGCLEAN, build 1417 (git 7ab19330c, uncommitted changes): yard 3 of 3 log checks, pause_menu 3 of 3 log checks. Shots written to build3ds\azt\shots\yard and build3ds\azt\shots\pause_menu. Picture not yet read by a person.
- Boot line now prints `build <n> git <hash> loglevel <n>`.

## MENULAG (2026-10-29): lag gates for the three menu scenarios

- Scope: pause_menu, partner_menu, file_select. The until-line is a `phase W` window line that starts after the menu is open (pause open at frame 96; file select at frame 15). The pause stays open: no close step. One top shot inside the menu.
- Gate fields (azt_scenarios.json): `max` = numeric limit on capture group 1 of the regex. `pick: last` = the last matching line, which is the window of the until-line.
- Mapping (no taskpl or flsplit line after LOGCLEAN, so the old baseline names do not exist in the log):
  - walk = `cpu (wall-idle-fbegin)` avg of the `phase W` line, in ms (not taskpl main us). The baseline 7634 us is taskpl main us and is not comparable.
  - closes = `flush` avg of the first `phase count` line (all closes). Tex-only closes are counted in pm_fl_n but no line prints them. The tex-only gate is not possible without a new log line.
  - ops = `gbiops` avg of the second `phase count` line (GBI commands per frame).
- Azahar, two runs each (the larger value is the base). Build 1433 (git b5c7b5226), binary build3ds\pm_3ds.3dsx, row MENULAG, 6 starts of 8.

| Scenario | Run | Wall (stop after) | Walk cpu ms | Closes | Ops | Limits set (base +15 %, rounded up) |
|---|---|---|---|---|---|---|
| pause_menu | 1 | 17 s | 11.1 | 135 | 1471 | |
| pause_menu | 2 | 15 s | 11.1 | 135 | 1471 | walk 12.8, closes 156, ops 1692 |
| partner_menu | 1 | 16 s | 9.5 | 107 | 1151 | |
| partner_menu | 2 | 15 s | 9.2 | 107 | 1151 | walk 11.0, closes 124, ops 1324 |
| file_select | 1 | 15 s | 8.1 | 88 | 1406 | |
| file_select | 2 | 15 s | 7.9 | 88 | 1406 | walk 9.4, closes 102, ops 1617 |

- Shots: one top shot per run (`build3ds\azt\shots\<scenario>`). A person must open each shot and confirm the subject (pause status page, Party tab, file select screen).
- Walk gate values are in ms in the log. Limits are in the same unit.


## Picture compare rule (NP1B, 2026-10-29)

- Tool: `python 3ds/tools/shot_cmp.py <dirA> <dirB>` (copy the shot folder of each run first, `build3ds\azt\shots\<scenario>`, because the next run of that scenario overwrites it).
- Top-screen shots (every .ppm whose name does not start with `bot_`) must be byte-equal. The tool exits 1 on any difference or on a name mismatch.
- Bottom-screen shots (`bot_*.ppm`) are not compared. HUD numbers and the refresh interval change between runs of the same binary.
- Proof: two HEAD runs of yard (build lane nphead, source at HEAD e16fee4b9) give 1 of 1 top shots byte-equal (`0000_0_11.ppm`).
- The earlier NP1 drop (18 to 140 px on yard bottom shots) was a bottom-shot difference, not a top-shot difference.

## LOGFIX (2026-10-29): cost scenarios for the phase and dcsum sets

| Scenario | Flags (after `loglevel=debug`) | Command | Result (build 1467, git 06e62c0aa) | Status |
|---|---|---|---|---|
| yard_pace | `pacestat logstat` | `yard_pace -Try` | logstat work 11.66 ms per frame (300 frames). Picture equal to yard baseline. | unverified (one run, frame-time base) |
| yard_dcsum | `pacestat logstat dcsum` | `yard_dcsum -Try` | work 11.79 ms (+0.13 ms, +1.1 %). dcsum line printed at f299. Picture equal. | unverified (one run) |
| yard_phase | `phase phsum audiostat texstat pacestat logstat` | `yard_phase -Try` | work 12.74 ms (+1.08 ms, +9.3 %). Picture equal. | unverified (one run) |

- Picture rule: top shots equal to the FINAL4 baseline (build 6e965c84b; the only later src change is LIVELOG log text). yard, yard_pace, yard_dcsum, yard_phase, mac01 (2 of 2): byte-equal. pause_menu: run 1 differed, run 2 byte-equal to baseline. Run 1 and run 2 of one binary differ, so the difference is run noise.
- The yard run without flags prints no frame-time line. Its frame time is taken from yard_pace.
- pause_menu run 1: walk gate failed (12.9 against 12.8). Run 2 passed (11.5). Gate value varies between runs.

## FSPLIT (2026-10-29): fsplit coarse split, 12 tick reads per drawn frame

- Switch `fsplit` (needs the log on and loglevel=debug). Marks at the coarse boundaries only: pm_gpu_begin (GWAIT), the step wrap (LOGIC, DL), nuGfxTaskStart (AUDIO, GBI, DL or REST), pm_gpu_end (FLUSH, SWAP), the pm_main skip path (REST). One line per 300 drawn frames. Values are ms per frame. `svc` is pm_ticks reads per frame.
- Not in the split: kick flushes (inside gbi), audio joins except the swap-task join (AUDIO), pacing idle (in rest). The 12-read budget does not allow them.
- Per-op census, per-op timers and PH_PUSH sites stay off (pm_ph_on is 0 with fsplit alone).

| Scenario | Flags | Command | Result (build with fsplit, binary sha256 8a9fdfc9e...) | Status |
|---|---|---|---|---|
| yard_pace (until: steady window, `wall 3x`) | pacestat logstat | `yard_pace -Try` | f300 (load): work 11.77 ms. f600 (steady): work 6.99 ms, wall 33.43 ms | unverified (run 3 of FSPLIT) |
| yard_fsplit (until fsplit f600) | pacestat logstat fsplit | `yard_fsplit -Try` | f300: logstat work 11.79 ms. f600: logic 0.7 dl 0.6 gbi 5.4 flush 0.1 gwait 0.0 swap 0.2 audio 0.0 rest 26.4 wall 33.4 svc 12.0. Sum of parts = logstat work 7.00 ms | unverified |
| mac01t_fsplit (until fsplit f900) | fsplit (tour to mac_01) | `mac01t_fsplit -Try` | f900 (steady): logic 1.3 dl 1.4 gbi 7.2 flush 0.1 gwait 0.0 swap 0.2 audio 0.0 rest 23.2 wall 33.4 max 34.0 over50 0 svc 12.0. f600: wall 34.8, over50 3 | unverified |
| mac01t_cap1 (CAP1, unverified) | phase fsplit (tour to mac_01, window reset at map load, pos line) | `mac01t_cap1 -Try -Row CAP1` | opsplit f483 and pos f483 after the load (`load map area 1 map 2` at frame 209). No pre-reset window line (f299, f599). Top shot 0000 (frame 500) read by eye: Mario and a Toad on the Toad Town street, HUD on. 1 start, until was wrong (cap 120 s); the run log was checked with -CheckLog (6 of 6) | unverified (1 run) |
| mac01t_rec (RK1, unverified) | rec line in window 2 (f6xx): record capture on plain opaque models (RC1 capture, RK1 bump fix) | `mac01t_rec -Try -Row RK1` (baseline `mac01t_cap1 -Try -Row RK1H -Build rc1h`) | `warp: mac_00 entry 4` | Window 2 f614: cap 0, build 0, wbad 0, rb 0. Window 1 f307: cap 303, build 79, emp 224, wbad 0 (not a gate item). Top shots 0000_1_2 and 0001_1_2 byte-equal to the HEAD run (RK1H). Yard (`-Row RK1`): rec f348 cap 48, build 9, wbad 0; top shot 0000_0_11 byte-equal to HEAD. 2 starts for the mac01t pair, 2 for yard. | unverified (1 run each side) || mac01t_prof (PF3, unverified) | prof=16 (sampled phase timers and PMU counter set m, cal line, window line) | `mac01t_prof -Try -Row PF3` | (run 1, PF3) prof cal and window line printed (6 of 6 log checks): s18, fts 29.91, wks 12.08, oh 1.52, amort 94, gbi 7.66, dlc 1.38, tex 0.02, gpu 0.30. pmu fields print, but Azahar PMU counts are tiny (pm 1000 1000 is a ratio of small numbers, not data). Shot 0000 (frame 250): Mario and a Toad on the Toad Town street, read by eye | unverified (1 run, picture read) |
| yard_pmu (CF0, unverified) | pmu line (PMU counters, 300-frame windows) with fsplit | `yard_pmu -Try -Row CF0` | `warp: kmr_20 entry 4` | 90 s cap, until pmu f300 | build3ds\azt\shots\yard_pmu | Console rc is the test: nonzero on Azahar is accepted (rule of CF0). |
| pause_fsplit (until fsplit f300) | fsplit (pause open 96 to 140) | `pause_fsplit -Try` | f300: logic 0.8 dl 0.7 gbi 7.0 flush 0.1 gwait 0.0 swap 0.3 audio 0.0 rest 25.3 wall 34.1 over50 7 svc 11.4 | unverified |

- Cost (Azahar, direction only): steady window +0.01 ms (7.00 vs 6.99 ms work). Load window +0.02 ms (11.79 vs 11.77). Limit 0.2 ms: passed.
- Pictures (shot_cmp.py, top shots, byte-equal): yard 1 of 1 (yard, yard_pace, yard_fsplit). mac01t_fsplit 2 of 2 against shots_base_mac01t (older build). pause_fsplit 1 of 1 against pause_menu (LOGFIX build). The yard_fsplit shot is from the build before the over50 fix (summary arithmetic only). A person must read the shots by eye.
- Log note: a run that stops on its until-line can lose the last lines of the on-disk log.txt. azt reads the live lines; build3ds/last_log.txt holds the copy used here.
- Mistakes: mac01t_fsplit run 1 used until `fsplit f899`. That line never prints (windows print at 300, 600, 900). The run ran to the 120 s cap. Fixed to f900. yard_pace run 2 used a window-1 regex and failed the steady-frame check. Fixed in run 3.

## PRIMZFIX (2026-10-29): toad pond depth, one unit nearer

Fix: `prim_ndc_z()` in 3ds/src/pm_gbi.c (the zmix and CPU z uses), prim_z - 1, clamped at 0. Repro switch `zround` (3ds/src/pm_gpu.c): zmix.y rounded to 2^-16 steps on the upload. Row PRIMZFIX. Builds: lane pzhead (HEAD d99eea457), pzold (HEAD plus zround only), pzfix (fix plus zround switch, same code otherwise). Shot compare: `shot_cmp.py` is not byte-equal here; see the result column.

| Scenario | What it proves | Command | Result | Status |
|---|---|---|---|---|
| toad_pond (PRIMZFIX) | Pond, mac_00 entry 4, route to x -174 z 112. HEAD run 1 and run 2 (lane pzhead): top shots not byte-equal, all 10 diffs in the NPC strip x 92 to 138. Fixed code, switch off (lane pzfix): same as HEAD outside x 92 to 138 in all 10 shots. The water at x 20 to 60 is identical in every run. | `toad_pond -Try -Row PRIMZFIX -Build pzfix` | 3 of 3 log checks, 4 runs | unverified |
| toad_pond_zround (PRIMZFIX repro) | Old code with zround (lane pzold): pond water unchanged from HEAD (teal water at x 20 to 60). Does not reproduce the console bug at the 2^-16 rounding. Fixed code with zround (lane pzfix): same picture. | `toad_pond_zround -Try -Row PRIMZFIX -Build pzold` | 3 of 3 log checks, 2 runs | unverified (does not reproduce) |

## UNTILCHK (2026-10-29): until check and stall stop (azt.ps1)

- Static check before each start (no start on a fail): the until line must hold each literal run of 4 or more letters in the C source of 3ds/src, 3ds/include or src. A frame in the until (`frame N\b`) must sit in shotwin when the debug string sets one. An `fN` frame must be at most cap times 60. A fail exits 7 with "SCENARIO REJECTED".
- Stall: no new log line for 40 s after the Azahar start stops the run and exits 6 (SCENARIO FAILED: no new log line). Not seen live: the yard run with `preset=yard` logs fs, dlc and audio lines every few seconds, so no quiet run was made. The rule itself is in the self-test.
- Self-test (no Azahar start): `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 -SelfTest`. Writes 4 scenario copies to %TEMP% with bad or good until lines and checks the stall rule. Result: SELFTEST PASS (5 of 5). Current scenarios: no NOTE line (0 of 36 rejected).

## WMODEL (2026-10-29): world model list and render task count in the fsplit line

The fsplit line (fsplit flag only) adds `wm <mean> max <max> tk <mean> max <max>` over the 300 drawn frames. wm = non-null entries of the decomp world model list (wModelList, 256 slots), counted once per frame. tk = gbi tasks walked per frame (gbi_task, key replays included). The first frame of the window is not counted.

| Scenario | What it proves | Command | Result | Status |
|---|---|---|---|---|
| yard_fsplit (WMODEL) | Yard, fsplit f600 line has the wm and tk fields. HEAD (lane pzfix) has no fields and two runs give a byte-equal top shot. | `yard_fsplit -Try -Row WMODEL -Build wmodel` | f600: wm 103.0 max 103, tk 2.00 max 2. Top shot 1 of 1 byte-equal to HEAD (HEAD run 1 and run 2 equal). logstat work at f600 window: HEAD 6.95 and 6.94 ms, new 6.96 ms (+0.01 ms, limit 0.2 ms). | unverified (3 runs) |
| mac01t_fsplit (WMODEL) | Tour to mac_01, fsplit f900 line has the wm and tk fields. | `mac01t_fsplit -Try -Row WMODEL -Build wmodel` | f900: wm 143.0 max 143, tk 2.00 max 2. Top shots 2 of 2 byte-equal to HEAD. | unverified (2 runs) |

## PAUSEFIX (2026-10-29): pause_menu gates reset on build 1517 (git 9461b95a8)

- Row PAUSE4A (background one load). pause_menu on HEAD 1514 against 1517, same scenario: closes 135 to 96, ops 1237 to 1003, walk 11.4 to 9.3 ms (window f176 to f235).
- Gates now: closes max 111 (96 + 15 %), ops max 1154 (1003 + 15 %), walk max 14.0. The walk window of the until-line is not one state: f120 to f175 mixes pause and field (13.6 to 13.9 ms), f175 to f235 is field (9.3 ms). A limit of 10.7 failed on the mixed window. Open: change the until (docs/handoff/agent_pausefix.md, item 1).
- partner_menu and file_select gates not changed (new values lower).
- Byte-equal top shots against HEAD (same run pairs): pause_menu 1 of 1, partner_menu 1 of 1, file_select 1 of 1, text_menu 3 of 3.

## PAUSE5A (2026-10-29): menu windows inside the open menu, gates at +15 %

- Build: default lane (build3ds\pm_3ds.3dsx, sha256 559728b3, git 9461b95a8 code). One run on lane pbase (HEAD 1514) was made by mistake and is not a gate value.
- Timeline (pause_menu, run 2 log): pause open at frame 96. Open spike in windows f61 to f175 (wall max 182.6 ms in f120 to f175). Steady pause from f176. Close START at frame 400 (scan 764), so the close is after the window.
- The padscript START presses: scan 146 (frame 91) and scan 764 (frame 400). Frame = 51 + (scan - 66) / 2.
- Until changed from `phase W f1[2-9]\d-` (windows that start in the spike) to `phase W f2[3-9]\d-`. The run stops at the first window that starts at frame 230 to 299. Cap 40 s (was 90).
- Window values (phase W, 60 frames, gates = larger of two runs +15 %, rounded up):

| Scenario | Run | Window | Walk cpu ms | Closes | Ops | Gate (walk, closes, ops) |
|---|---|---|---|---|---|---|
| pause_menu | 2 | f296-355 (until f296) | 9.3 | 96 | 1003 | 10.7, 111, 1154 |
| pause_menu | 3 | f236-295 (until f236) | 9.2 | 96 | 1003 | (same) |
| partner_menu | 1 | f295-354 (until f235-294) | 7.6 | 68 | 789 | 8.8, 79, 908 |
| partner_menu | 2 | f296-355 (until f236-295) | 7.6 | 68 | 789 | (same) |
| file_select | 1 | f119-178 (until) | 8.1 | 88 | 1346 | 9.5, 102, 1548 |
| file_select | 2 | f118-177 (until) | 8.2 | 88 | 1346 | (same) |

- Walk gate uses the last window in the log. For partner_menu the last window is one after the until window (both inside the menu). The scenario stops within 5 s of its until-line.
- Picture: pause_menu top shot at frame 130 (inside the open menu, the open spike period). A person must open 0000_1_1.ppm.
- Starts used on row PAUSE5A: 8 of 8 (1 pbase, 3 pause, 2 partner, 2 file). Budget reset is needed before more PAUSE5A starts.

## PAUSE5B (2026-10-29): cold timing of the pause open (fsplit, phase)

- Code: `pm_pc_begin` (state_pause.c, BEGIN_PAUSED), timers in state_pause.c (rom, pause_init, setup block) and background_gfx.c (filter), window and lines in 3ds/src/pm_phase.c. All under TARGET_3DS and the fsplit switch. No change to the picture: pause_pcopen top shot byte-equal to pause_menu top shot (PAUSE5A run 3), shot_cmp 1 of 1.
- Lines: `pcopen` (one per open: 60 drawn frames from BEGIN_PAUSED), `pcphase` (worst frame and window sums per phase), `pcgbi` (worst frame GBI sub-phases, phasegbi only). Log lines are kept under 256 bytes, so the summary is three lines.
- Build 1519 (git a02d5ac09 plus PAUSE5B), default lane.

| Scenario | Run | pcopen avg / max ms per frame | Window ms | Tex loads (KB) | Evict | Map chg | Readback ms (calls) | Filter ms | Setup ms (rom / pause_init / rest) |
|---|---|---|---|---|---|---|---|---|---|
| pause_pcopen | 1 (before the phase split) | 36.04 / 173.42 (f129) | 2162 | 96 (1385) | 0 | 0 | 8.77 (1) | 6.01 | 9.25 (0.00 / 7.30 / 1.94) |
| pause_pcopen | 2 | 35.96 / 168.61 (f128) | 2157 | 95 (1385) | 0 | 0 | 8.77 (1) | 6.01 | 9.25 (0.00 / 7.30 / 1.94) |
| pause_pcopen_gbi | 1 (phasegbi) | 36.10 / 175.31 (f128) | 2166 | 95 (1385) | 0 | 0 | 8.77 (1) | 6.00 | 9.25 (0.00 / 7.35 / 1.90) |

- Worst frame (f128, run 2 and 3): gbi 141 to 148 ms, idle 23 to 24 ms, tex 0 loads, fbread 0, gpu 0.9 ms. The GBI sub-phases of that frame (gvtx, gtri, gmtx, gop) are 0.0 ms; gtev, gbind, gst total 0.8 ms. So the GBI spike is not in any timed sub-phase.
- Window sums over 60 frames (run 2): tex 81.6 ms (95 loads, about 0.86 ms per load), gbi 538.5 ms, gpu 51.1 ms, dl 44.7 ms, other 22.1 ms, fbread 8.8 ms.
- Derived: pause steady gbi about 6.6 ms per frame ((538.5 - 147.7) / 59).
- Field, same map (`field_mac00`, Toad Town, window f241-300, no pause): closes 16 per frame (draw 16, flush 16), gbiops 465, gbi 3.2 ms avg, logic 2.3 ms, cpu 13.2 ms. Pause steady (PAUSE5A, f176-295): closes 96, gbiops 1003, cpu 9.3 ms.
- Not measured: top draw groups in the pause (frozen background, labels, icons, windows). The build has no per-group census. A census needs a new counter (diagnosis agent).
- Azahar numbers are direction only. Console: not measured.
- Starts used on row PAUSE5B: 4 of 8 (pause_pcopen 2, pause_pcopen_gbi 1, field_mac00 1).

| Scenario | What it proves | Command | First marker | Status |
|---|---|---|---|---|
| pause_pcopen | pcopen line per pause open (fsplit phase) | `pause_pcopen -Try -Row PAUSE5B` | `warp: mac_00 entry 4` | unverified (2 runs) |
| pause_pcopen_gbi | pcgbi line (phasegbi) for the worst frame | `pause_pcopen_gbi -Try -Row PAUSE5B` | `warp: mac_00 entry 4` | unverified (1 run) |
| field_mac00 | field window on mac_00 (no pause), gameplay comparison | `field_mac00 -Try -Row PAUSE5B` | `warp: mac_00 entry 4` | unverified (1 run) |
## DC6 (2026-10-29): dcsum cmd words and group counts per close

| Scenario | What it proves | Command | First marker | Status |
|---|---|---|---|---|
| mac01t_dcsum | dcsum line (cmd words per sampled close) and dcgrp line (groups per close) on the mac_01 tour view; shots at frames 500 and 504 | `azt.ps1 mac01t_dcsum -Row DC6 -Build dc6 -Try` | `warp: mac_00 entry 4` | unverified (3 runs for the log; top shots 2 of 2 byte-equal to HEAD fb9e8f0fc, picture read: Mario and a Toad on the Toad Town street, HUD on) |

- HEAD (fb9e8f0fc): cmd field reads 0 permil. The citro3d usage (C3D_GetCmdBufUsage) updates only at a split, so the delta was always 0. DC6 reads GPUCMD_GetBuffer offset instead: 80 to 82 words per sampled close (dcsum f299).
- dcgrp f299 (hundredths per close, Azahar): shadow miss 168, bind0 101, bind1 0, bindstaged 1, tev 62, palup 53, palsame 0, pmul 33, prog 12, bufinfo 12.
- palsame 0: no palette upload writes the matrix the register already holds. Shadow misses and binds are real value changes (DR1 and the TEV key already compare).
- Draw stage 14 us per close (Azahar) on both HEAD and DC6. No repeated group remains to remove; no draw change in DC6.
## SPK1 (2026-10-29): cost of pm_mem_hud (linear free and mallinfo)

- Instrument only: two tick pairs in pm_mem_hud (3ds/src/pm_mem.c), fsplit only. Line `spk1 linearfree N us, mallinfo N us` once per HUD refresh (1 per second).
- Azahar, yard_fsplit (lane spk1, runs 1 and 2): linearSpaceFree 1 us, mallinfo 4 to 5 us per refresh. Total about 6 us Azahar, about 17 us console (x2.86). Gate 3 ms: not met, so the HUD code is not changed.
- HUD values equal to HEAD in the log: `hud mem rows ready: MEM used 32.2 / 63.9 MB free 31.7 MB | VRAM min 2130K pk 4014K` (HEAD and SPK1).
- fsplit f600 over50: 0 in HEAD and SPK1. fsplit f300 (load) over50: 18 in SPK1 run 2; HEAD f300 not kept (log overwritten). Open.
- Bottom shots do not reproduce across runs on this scenario: HEAD against HEAD 667 to 670 px differ per bottom shot; SPK1 against SPK1 about 5000 px. Top shot equal HEAD against SPK1 (1 of 1). Bottom shots are not a clean gate here.
- Starts used on SPK1: 4 of 8.


## JTA (2026-10-31): jump test on Azahar

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| jump_azahar (unverified) | jumptest JT3 result lines (`jump f<frame> ok/fail/none value`) on frames 300 to 399, and the game runs past frame 330 | `azt.ps1 jump_azahar -Row JTA -Try` (debug `jumptest prof loglevel=debug`) | `jump f` | cap 45 s | none (log only, needs_picture false) | Until is the first result line at frame 300 to 399. Azahar GPU does not run jumps (docs/handoff/agent_jumptest.md). |
| jump_off_azahar (unverified) | Same boot as jump_azahar, no jumptest: the game reaches frame 330 (shot at 330) with no watchdog line (JTB) | `azt.ps1 jump_off_azahar -Row JTB -Try` (debug `shots shotwin=330:1 prof loglevel=debug`) | `boot_main` | cap 45 s | build3ds\azt\shots\jump_off_azahar | Log and shot only. Until is the frame 330 shot. |
| boot_jump_azahar (unverified) | Boot with no warp (intro plays), jumptest on: first jump result line at frame 390 or later (BOOTJ) | `azt.ps1 boot_jump_azahar -Row BOOTJ -Try` (debug `jumptest prof loglevel=debug`) | `boot_main` | cap 60 s | none (log only, needs_picture false) | Until is `jump f(39\d\|[4-9]\d\d) (ok\|fail\|none)`. Expects none: no attract check. |
| intro_jump_azahar (unverified) | Fresh boot with no warp (intro plays, game mode 16 from frame 327), jumptest on: first jump result line at frame 330 or later (INTROJ) | `azt.ps1 intro_jump_azahar -Row INTRO -Try` (debug `jumptest prof loglevel=debug`) | `boot_main` | cap 90 s | none (log only, needs_picture false) | Until is `jump f(3[3-9]\d\|[4-9]\d\d) (ok\|fail\|none)`. Expects none. |

## LOGCON (console log level, separate from the file and live log level)

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| logcon_err | `conlevel=error` with `loglevel=debug`: log.txt holds debug lines (`logstat frames 300 work`), the logstat2 line prints `console lines 0` (gate max 0). Until is the logstat2 line at frame 300. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 logcon_err -Row LOGCON -Build logcon -Try` | `warp: kmr_20 entry 4` | 20 s to the first marker, about 60 s to the until | build3ds\azt\shots\logcon_err | Run 1 (build 1670-logcon): 3 of 3 log checks. logstat2 `console lines 0`; log.txt has the `logstat frames 300` debug line. Live socket log not checked (log.txt only). Picture not read by the agent: the bottom screen must show no log text. |
| logcon_dbg | Same build with `conlevel=debug`: the logstat2 line prints `console lines` at least 1 (gate min 1). Second start of the pair. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 logcon_dbg -Row LOGCON -Build logcon -Try` | `warp: kmr_20 entry 4` | as logcon_err | build3ds\azt\shots\logcon_dbg | Run 2 (build 1670-logcon): 3 of 3 log checks. logstat2 `console lines 105` (was 0 in logcon_err). Picture not read by the agent. |

## UIANCHOR (2026-11-03): wide UI anchors, scenarios

| Scenario | Status | Command | First marker | Runtime | What it proves | Known limits |
|---|---|---|---|---|---|---|
| battle_target_wide | unverified (gate pending, UA1) | `battle_target_wide -Row UIANCHOR -Build UIANCHOR -Try` | `warp: kmr_03 entry 0` | cap 45 s | target pointer log and shot burst at target select (wide) | Wheel input does not reach target select (starts 2 to 4); see docs/handoff/agent_uianchor.md |
| talk_wide | unverified (discovery) | `talk_wide -Row UIANCHOR -Build UIANCHOR -Try` | `warp: mac_00 entry 4` | cap 30 s | NPC x y z (simstep lines, wide) | Log only. Mario does not walk by padscript (start 6). |
| battle_wide | UA4 gate: pass by eye (wheel beside Mario, wide), log `debug wheel open: base 33 183` | `battle_wide -Row UA4 -Build UIANCHOR -Try` | `warp: kmr_03 entry 0` | cap 45 s | battle wheel base follows the zoomed view (pm_wide_vx / vy) | Wide-off check: battle_start (verified) shows the wheel at the old place by eye; byte-equal to HEAD not run (wide-off code path returns the input). |
## TEXL2 (2026-11-03): volcano_l2, texture re-miss counters (unverified)

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| volcano_l2 (unverified, `-Try`) | Mt. Lavalava (kzn_19) with `texcap=256` (256 KB texture cache). The `texghost f300` line counts re-misses of evicted textures (evict-hit) and of hash-changed textures (diff-hit). Gate: evict-hit above 0. | `azt.ps1 volcano_l2 -Row TEXL2 -Build TEXL2 -Try` | `warp: kzn_19 entry 0` | 16 s | build3ds\azt\shots\volcano_l2\ | Build L2a. The `texcap=1024` value gave 0 evictions (844 KB uploaded in 300 frames). Run 7 (L2a): evict-hit 2 (32 KB), 322 upload ops, 54784 us. |
| volcano_nb (L2a gate) | Mt. Lavalava (kzn_19) with top shots at frames 196, 200, 204 (shotwin=196:9). Result (L2a vs main 7b6e3735b, 2026-11-03): frames 196, 200 and 204 equal, 0 bytes each. The spot at x166-168 y289-293 is animated (main frame 200 vs 196 and 204 differ there). | `PM_ROW=TEXL2 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 volcano_nb -Row TEXL2 -Build <lane> -Try` | `warp: kzn_19 entry 0` | 16 s | build3ds\lane-<lane>\shots\ | Shots every 4 frames only. Frame 201 is not shot. Status: verified for the L2a gate (lanes TEXL2M and TEXL2MB). |
## OPTFIX (2026-11-03): PmOpt initializer, boot option values

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| boot_opts (unverified) | Boot with no options.ini and no options.txt: the `options: boot` line prints dz_cs 0, rep_delay 500, rep_rate 200, hud_ms 1000, conlevel 1, loglevel 1, log 1 (the defaults). Log only. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 boot_opts -Row OPTFIX -Build OPTFIX -Try` | `boot_main` | cap 30 s, until at the boot line | none | The sdmc options.ini and options.txt must be moved aside for the run. The debug string loglevel=debug lets the info line print. |

## BOTLAYOUT (2026-11-03): bottom screen layout (HUD, separator, log header, scrolling log region, footer)

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| botlayout | Log scroll (BOTLAYOUT): bottom shots at frames 49 to 54 (warp to kmr_20 at frame 49; the map load writes log lines). The log region rows 9 to 26 change between frames 49 and 54 (135 of 144 pixel rows differ). The header and separator bands (rows 7 and 8) and the footer (rows 27 to 29) do not change (0 rows differ). Layout: HUD rows 1 to 4, rows 5 and 6 blank, separator 7, header 8, log 9 to 26, separator 27, row 28 `Map: kmr_20 (area 0, map 11)`, row 29 `apt: 80% O3DS #<build>`, row 30 blank. Top row shows `Now in kmr_20 (area 0, map 11)` in the 2 s hold (frame 54). | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 botlayout -Row BOTLAYOUT -Build botlayout -Try` | `warp: kmr_20 entry 4` | 18 s to the until (`padscript: done`, f259), cap 45 s | build3ds\azt\shots\botlayout | Run 5: 3 of 3 log checks, picture read. Run 4 (shots 200 to 205): the log region was identical across the frames (no new lines), so it is not a scroll proof. No earlier until line exists. |
| botlayout_menu | Menu owns the bottom screen while open; after `menu: close` the layout returns (BOTLAYOUT). Shots 84 to 129: frame 95 shows the menu (General tab, full screen). Frame 120 shows the layout: HUD rows 1 to 3, separator 7, header 8, `menu: close` in the log region, separator 27, row 28 `Map:`, row 29 `apt:`. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 botlayout_menu -Row BOTLAYOUT -Build botlayout -Try` | `warp: kmr_20 entry 4` | 22 s to the until (`padscript: done`, f130), cap 45 s | build3ds\azt\shots\botlayout_menu | Run 6: 4 of 4 log checks, picture read (frames 95 and 120). |

## POSWARP (2026-11-03): pos=<x>:<y>:<z>[:<angle>] places Mario after a warp

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| kmr10_pos (POSWARP, verified by log and picture) | pos=-142:20:221 places Mario on the kmr_10 floor. Start 8 (budget reset): readback `placed Mario at -142.000 20.000 221.000 ... settled -142.000 20.000 221.000`, 2 of 2 log checks, frame 56 shot shows Mario on the path by the fence (read by the code agent). | `azt.ps1 kmr10_pos -Row POSWARP -Build POSWARP -Try` | `warp: kmr_10 entry 0` | cap 45 s, deadline 35 s | build3dszt\shots\kmr10_pos | Gate: x and z within 0.5, y within 3 of 20. Starts 6 and 7 ended with Azahar exits (no result). Padscript token not run by this scenario. |
| kmr10_pos_hi (POSWARP, unverified) | pos=-142:136:221 (texture garble band view). Start 9: readback at frame 40 settled y 97.265 (still falling), x -142 z 221 exact, 2 of 2 log checks, frame 56 shot shows Mario on the path. Gate x and z only. | `azt.ps1 kmr10_pos_hi -Row POSWARP -Build POSWARP -Try` | `warp: kmr_10 entry 0` | cap 45 s, deadline 35 s | build3dszt\shots\kmr10_pos_hi | Y falls to the floor at 20 after the readback. Picture read by the code agent. |


## MISC1 (2026-11-03): console colour by log level, smdh metadata

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| logcolor | CONCOL: the four levels print in four colours on the bottom console (error bright red, warn yellow, info white, debug grey). Lines `lvtest: error/warn/info/debug line` at frame 100, `lvtest: done` at frame 125, bottom shots 115 to 118. Escape codes only in console bytes (log.txt is checked plain). | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 logcolor -Row MISC1 -Build MISC1 -Try` | `warp: kmr_20 entry 4` | about 20 s to the until, cap 45 s | build3ds\azt\shots\logcolor | Switch lvtest is a test aid in pm_log.c (pm_log_lvtest). The smdh is checked by 3ds/tools/smdh_dump.py run on build3ds\lane-MISC1\pm_3ds.3dsx. |
| mario_proj_wide | UA1 gate (numeric, no picture): wide on, one frame, errors -0.8 px and -1.1 px at Mario -100 and +100 (start 17, log `marlog f100 side ... err10`). Pass 1.1 px (s32 truncation of get_screen_coords is up to 1 px). | `mario_proj_wide -Row UA1 -Build UIANCHOR -Try` | `warp: mac_00 entry 4` | 30 s | get_screen_coords x (40 + x) equals the draw px away from the centre | Derived, not measured: without the fix the error is about 20 px at 100 units. |
| mario_proj_off | UA1 wide-off check (numeric): errors -0.5 px and -0.4 px (start 19) | `mario_proj_off -Row UA1 -Build UIANCHOR -Try` | `warp: mac_00 entry 4` | 30 s | wide off: get_screen_coords agrees with the draw px | Byte-equal to the previous commit not run. |
| talk_wide (UA2) | UA2 gate by eye: wide on, speech box at the top edge (flush with row 0 to 2), text inside. Speaker -1 (Mario), log `debug talk anchor`. | `talk_wide -Row UA2 -Build UIANCHOR -Try` (padscript wait300,A) | `warp: mac_00 entry 4` | 45 s | speech box top (pm_wide_top), bubble clamps (pm_wide_left/right/top/bottom), msg.c bottom 240 | Byte-equal wide off not run (helpers return the input with pm_wide 0). |

| warpfix_live (WARPFIX, unverified, log only) | A live warp (Select menu row, pm_warp_live) on a loaded save (loadslot=0) changes the map and entry only. warpitems adds 2 mushrooms and 50 coins before the warp (the test save holds none). Lines `warpstat before live` and `warpstat after live`: items, badges, coins, story, slot. Pre-fix (start 3): items 2 to 0, coins 50 to 0, story -128 to -107. Fixed (start 4 for WARPFIX lane): items 2, coins 50, story -128 unchanged. | `azt.ps1 warpfix_live -Row WARPFIX -Build WARPFIX -Try` | `warp: kmr_20 entry 4` | cap 45 s, deadline 35 s | none (log only) | Save file hash unchanged (no save written). The boot line of this save already has story -128 (the save's own value), so the boot story guard is not shown by this run. |


| warpfix_boot_save (WARPFIX2, unverified, log only) | Boot warp without loadslot, then padscript save_0: the save write is blocked (`saving disabled by boot warp`, `warp: save blocked`). Start 1: both lines printed, flash hash unchanged (850cc147...). | `azt.ps1 warpfix_boot_save -Row WARPFIX2 -Build WARPFIX2 -Try` | `warp: kmr_20 entry 4` | cap 45 s, deadline 35 s | none (log only) | Gate is the log lines and the flash hash, not a picture. |
| warpfix_fswarp_save (WARPFIX2, unverified, log only) | File select warp (pm_warp_now) at scene frame 120 on a loaded save, then padscript save_0: no write (`saving disabled by file select or title warp`). Start 2: before and after lines printed (story -128 to -107, items 0: the test save holds none), save blocked, hash unchanged. | `azt.ps1 warpfix_fswarp_save -Row WARPFIX2 -Build WARPFIX2 -Try` | `warp: kmr_20 entry 4` | cap 45 s, deadline 35 s | none (log only) | The warpitems switch is not applied on this path (only the live warp and the boss test set items). |
| warpfix_boss (WARPFIX2, unverified, log only) | Boss tab launch in the world (bosstest at frame 180), loaded save with 2 mushrooms and 50 coins, battle ends by bossend=60: `warpstat before boss` items 2, coins 50, story -128; during the fight story -16; `warpstat after boss restore` items 2, coins 50, story -128. Start 3, hash unchanged. HP and FP are restored in code but not in the log line. | `azt.ps1 warpfix_boss -Row WARPFIX2 -Build WARPFIX2 -Try` | `warp: kmr_20 entry 4` | cap 45 s, deadline 35 s | none (log only) | Restore runs 10 world frames after the battle. bossend is a test switch only. |
| battle_menu_bar (UI2, unverified) | Battle on kmr_03 with the debug switch `battlemenu` (pm_warp.c, scene frame 110): sets BS_FLAGS1_MENU_OPEN and tattles the enemies (save_tattle_flags, the real Tattle path), so the enemy HP bar draws (16C8E0.c:905, 181810.c:503). Run 1 without the tattle step: no bar. Run 2 (shot 182, build ui2): yellow bar with a 1 px dark red line at the end (PPM row 330). The red line is not in assets/us/ui/battle/hp_bar.png (32 by 16, rows 0 to 8 only). `azt.ps1 battle_menu_bar -Row UI2 -Build ui2 -Try`. | `PM_ROW=UI2 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 battle_menu_bar -Row UI2 -Build ui2 -Try` | `warp: kmr_03 entry 0` | cap 20 s (until shot 182) | build3ds/azt/shots/battle_menu_bar |
| battle_danger (UI2, unverified) | Battle on kmr_03 with Mario at HP 5 (`hp=5`, DANGER_THRESHOLD 5, battle.h:272): the danger status icon (actor_rendering.c:1596) draws. Picture read: not yet. | `PM_ROW=UI2 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 battle_danger -Row UI2 -Build ui2 -Try` | `warp: kmr_03 entry 0` | cap 20 s (until shot 182) | build3ds/azt/shots/battle_danger | Uses the existing hp= switch, no new hook. |

## AUD2 (2026-11-03): sound start retry and num_pvoice log (lane native)

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| aud2_cache (unverified, audio log only) | Yard with `dspcache=256`: sound starts that get no cache entry are retried on the next block (up to 16 blocks, dsp_voice.c AU2_START_RETRY). `audio dsp` line prints cache hits, misses, evictions and failures; the `dspaudio: num_pvoice 24` warning prints once. Run 1: failures 122 (attempts), starts 230 (successful), evictions 31, sample key 00fd6090 retried on consecutive blocks. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 aud2_cache -Row AUD2 -Build native -Try` | `warp: kmr_20 entry 4` | about 16 s, cap 25 s | none (no picture) | The log does not show whether each retry succeeded. Failures count attempts, not dropped sounds. |

## SPL1 (2026-11-03): splash grab, one per frame (lane native)

Result: no code change. HANG1a (commits e895b8b2f, 20fb1e88b) already makes one full-height grab per splash frame: pm_gbi.c grab_cur, splash branch, with splash_grab_f per frame. The row's proposed change in water_splash.c is the same path.

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| nok11_splash (SPL1 run 1, unverified) | Grab log: one `grab 0 ... rows 0-240` line per splash frame (frames 403, 407, 409, 410 ...). Shot a (file 0013, frame 412, Mario at -758 0 -49, spawn -758 25 -49) shows cyan splash drops by eye. Frame time from the log timestamps: frame 403 at 13896 ms, frame 412 at 14228 ms, frame 432 at 14959 ms, about 27 frames per second (Azahar, direction only). | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 nok11_splash -Row SPL1 -Build native -Try` | `warp: nok_11 entry 0` | about 16 s | build3ds\azt\shots\nok11_splash | No before-image: the build without the splash marks stops with svcBreak at the first splash frame, so no byte compare was made. |

## VRAM1 (2026-11-03): forced off-screen VRAM failure in the star transition (lane native): BLOCKED

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| battle_start_fbvram (VRAM1 run 1, unverified) | `vramfill=2` (two dummy 512 KB blocks in bank B at boot) makes the off-screen image alloc fail: `vram alloc fail offslot 512 KB (vram free 356 KB)` and `V05: off-screen image 0: no VRAM` (pm_gpu.c off_fail). Stop: crash symbols (rc 2) after `encwarp: window end`, 16047 unmapped reads at PC 0x00173B04. No star transition shot. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 battle_start_fbvram -Row VRAM1 -Build native -Try` | `warp: kmr_03 entry 0` | about 15 s to the until | build3ds\lane-native\shots (not copied) | The stop comes from the battle path, not from the forced VRAM failure (run 2 control). |
| battle_start_fb (VRAM1 run 2, control, no vramfill) | Same stop without `vramfill`: crash symbols (rc 2), 29660 unmapped reads at PC 0x00173B04 (script_list area), no star transition shot. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 battle_start_fb -Row VRAM1 -Build native -Try` | `warp: kmr_03 entry 0` | about 15 s to the until | build3ds\lane-native\shots | Lane native includes uncommitted WIP in the tree (pm_gpu.c, pm_warp.c, others). Not separated. |

Result: the garble test cannot run on this build. The battle start path stops with unmapped reads after the pre-battle start (frame 174), before the star transition. The VRAM failure is proven (run 1). No fix was made. Next step: a fresh investigation agent on the unmapped reads (decode PC 0x00173B04 in build3ds/lane-native/pm_3ds.map), on a clean HEAD build.
| hos01_texword (TEXD1, failed: stale-wrap hypothesis refuted) | hos_01 wall: texword=0100169c@200 dumps the first 4 binds of the I4 64x64 wall texture from frame 200 (texsrc, texgpu files and the wrap state line), drawlog=200:1 for the chunk | `PM_ROW=TEXD1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 hos01_texword -Row TEXD1 -Build native -Try` | `warp: hos_01 entry 0` | 25 | build3ds\lane-native\last_log.txt; texsrc_0100169c_*.bin, texgpu_0100169c_*.bin in the working directory | Runs 3 to 8 (TEXD1): every bind of 0100169c from frame 200 to 239 shows wrap s 3 t 3 (mirrored repeat). The stale-wrap fix (wrap set per draw tile, flush before a change) gave the same 3 3, so it was reverted. The tile 0 state at those draws is mirror. The wall chunk tw (cms 0, cmt 0, masks 5) from the earlier drawlog is not the tile that these binds sample. Dump cap 40 (texword). |
| texreg_wall (BIS1, unverified, picture not read) | hos_01 wall: texword line of 0x0100169c (the TEXD2 texreg probe was reverted in BIS1; the probe is texword, first 4 binds from frame 0). Runs to scene frame 300. | `PM_ROW=BIS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 texreg_wall -Row BIS1 -Build native -Try` | `warp: hos_01 entry 0` | 60 | build3ds\lane-native\last_log.txt; build3ds\azt\shots\texreg_wall | BIS1 run 6 on the fixed HEAD: `texword f38 img 0100169c bind 1`, wrap s 3 t 3 (mirrored repeat), shr 0 0, filter 1 (linear), slot 64x64, GPU 64x64 (pad 0 0), staged 1. Log checks 4 of 4. |
| texwrap_wall (TEXD3, unverified, picture not decisive) | hos_01 wall, texword probe of 0x0100169c at frame 200 with the switch `wrapov=repeat`: td3 line `programmed ws 2` for slot 8, shot at frame 203. Run 5 and 6 (same switch): 17755 pixels differ between the two runs; default against switch: 17355. Azahar does not show the wrap effect. | `PM_ROW=TEXD3 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 texwrap_wall -Row TEXD3 -Build native -Try` | `warp: hos_01 entry 0` | 32 (until shot frame 203) | build3ds\azt\shots\texwrap_wall\0000_5_1.ppm | The until was frame 204 in hos01_texword: never reached (runs 3 and 4). The shot writer takes frame 203. Pass needs a picture read by a person on the console. |
| hos01_texword (TEXD1/BIS1/TEXD3, unverified) | Same as texwrap_wall without the switch. Bind of 0x0100169c at frame 200: wrap s 3 t 3, filter 1 (run 3, TEXD3). | `PM_ROW=TEXD3 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 hos01_texword -Row TEXD3 -Build native -Try` | `warp: hos_01 entry 0` | 25 | build3ds\azt\shots\hos01_texword\0000_5_1.ppm | Until frame 204 is never reached within 25 s: use texwrap_wall (until frame 203, cap 32). The frame 203 shot is a plain hos_01 top view, Shooting Star Summit wall visible. |
| texreg_coin (BIS1, unverified) | tik_03 sewer coins: texword line of 0x00571b90 (first 4 binds from frame 0). | `PM_ROW=BIS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 texreg_coin -Row BIS1 -Build native -Try` | `warp: tik_03 entry 0` | 60 | build3ds\lane-native\last_log.txt | BIS1 run 7 on the fixed HEAD: no texword line for 0x00571b90 by scene frame 300 (texture not bound in entry 0). Values not obtained. Next: a position where the coins draw (texbind probe), then this scenario. |
