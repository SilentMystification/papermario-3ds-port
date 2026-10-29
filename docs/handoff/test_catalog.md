# Test catalog (azt)

## How to use (one page)

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build <lane>` (use `-Try` for an unverified scenario). `-List` lists the scenarios. `-CheckLog <file>` checks an old log and starts nothing.
- Each call is one real Azahar start and counts against the row budget (8 starts per row, reset once by the orchestrator with `run_azahar.ps1 -ResetBudget <row>`).
- Shots of each run go to `build3dszt\shots\<scenario>\` (the last run of that scenario). Shots are PPM, 240 by 400.
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
Exit codes: 0 log checks pass, 1 check or run failure, 2 marker missing (stopped), 3 ATTRACT LOOP (stopped), 4 unknown or unverified scenario, 5 argument or build error, 9 budget used.
A log pass is not a full pass. Read the shot files and confirm the subject. Shots of each run are copied to build3ds\azt\shots\<scenario>\ (runs before this change: shots are in build3ds\shots and were overwritten).

## Verified (log checks and picture read by eye)

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| yard | preset=yard loads kmr_20; steady frame; shot | `yard` | `warp: kmr_20 entry 4` | 15 s | build3ds\shots (run before archive; overwritten). Picture: yard, green pipe, Mario and partner. | Shot writer took frames 200 and 204 only (not 6). |
| tour10 | tour= visits 10 maps by the real map change | `tour10` | `warp: mac_00 entry 4` | 20 s | build3ds\shots (overwritten). Picture: Mario in all 11 shots. | Route list. Phase logging on. |
| mac01 | warp to mac_01 entry 0; phase prints opsplit and mops (MODELOPS R0 counters); shot at frames 200 and 204 | `mac01` (lane `mops`, `-Row MODELOPS_R0`) | `warp: mac_01 entry 0` | 16 s | build3ds\azt\shots\mac01\0000_1_2.ppm, 0001_1_2.ppm. Picture: Toad Town post office, Mario and Toads. Two runs, both shots byte-identical. | Until is the first mops line (frame 299), not the second: a regex cannot count. Baseline for MODELOPS R1 to R3. |
| pause_menu | Moved to MENULAG (lag gates, not a picture test). | `pause_menu` | `warp: mac_00 entry 4` | 15 to 17 s | build3dszt\shots\pause_menu (top shot frame 130) | See MENULAG. |
| text_menu | pause dialog text, periods and spacing | `text_menu` | `warp: mac_00 entry 4` | 15 s (until shot 178 to 181; was 91 s timeout) | build3ds\azt\shots\text_menu\0002_1_1.ppm | Text read by eye. Same pause open as pause_menu, until is the first shot at 178 to 181. Picture (FINAL2 run, shot 178): pause status page, dialog text readable. |
| battle_start | battle switch starts a battle on kmr_03 | `battle_start` | `warp: kmr_03 entry 0` | 15 s | build3ds\shots (overwritten). Picture: field, then battle HUD with Mario and Goomba. | Shot line holds a burst number, not the frame index. |
| sewer | warp into tik_03 (Toad Town Tunnels) | `sewer` | `warp: tik_03 entry 0` | 15 s | build3ds\azt\shots\sewer\0000_2_2.ppm, 0001_2_2.ppm | One map. Shot at frame 200. |
| ruins | warp into isk_04 (Dry Dry Ruins) | `ruins` | `warp: isk_04 entry 0` | 16 s | build3ds\azt\shots\ruins\0000_11_3.ppm (see folder) | Story text box covers the right side. |
| volcano | warp into kzn_19 (Mt. Lavalava) | `volcano` | `warp: kzn_19 entry 0` | 15 s | build3ds\azt\shots\volcano\ (see folder) | One map. |
| attract_no_warp (fail_demo) | ATTRACT LOOP: 3 title or demo lines in a tour run | `attract_no_warp` | `boot_main` | 39 s (stopped) | none | Exit 3, `ATTRACT LOOP: 3 title or demo mode lines`. |
| tour_no_warp (fail_demo) | fail-fast: marker missing | `tour_no_warp` | `warp: [a-z0-9_]+ entry` | 21 s (killed) | none | Exit 2, `SCENARIO FAILED: marker missing`. Azahar killed, none left. |

## Unverified (run or not, with the reason)

| Scenario | Reason | Shot paths |
|---|---|---|
| yard_live | LIVEMSG: yard with `livelog`; the live-log status line is in log.txt after pm_log_start. Picture not yet read. | `yard_live` (`-Try`) | `warp: kmr_20 entry 4` | 20 s | build3dsztshotsyard_live (run and shot read by eye pending) | Azahar may not open the soc socket: a socInit failure line is the expected result there. |
| tour_k30 | Log passes (tour: done, 6 maps). Picture fails: 4 of 7 shots show a grey page, the Prologue card and a text box. The fix (shot after each map load-done plus 15 frames) is a game source change in pm_warp.c or pm_gpu.c, outside this tool. The file holds 6 maps, not 30. azt2 tried a shot burst 15 frames after each map is in (tourshot=15:21), then 30 frames (tourshot=30:9): both still show 4 of 7 non-map shots (grey page, Prologue card, dark text box). Edit reverted. Log-only since 4b0ad69d4: the kmr_30 texture counter line is checked, picture N/A (cutscene overlay on 4 maps). Re-run on the default build (binary 04:50, before the lane rebuild): delta 420. Rebuilt lane azt2 from HEAD 0738350b2 (-Build azt2, budget reset): tex fail cum 0, delta 0 on kmr_30, same args and tour file; vramfree min 2130 KB (default build: 3858 KB). The 420 came from the older default binary. Closed as log-only. | build3dszt\shots	our_k30 |
| partner_menu | Moved to MENULAG (lag gates; top shot frame 200 is the Party tab; frame 220 showed the Star Spirits page (FINAL4 run) and was moved to 200). | `partner_menu` | `warp: mac_00 entry 4` | 15 to 16 s | build3dszt\shots\partner_menu | See MENULAG. |
| file_select | Moved to MENULAG (lag gates; top shot frame 100 is the file select screen). | `file_select` | `boot_main` | 15 s | build3dszt\shotsile_select | See MENULAG. |
 FINAL2 run: until is shot 338 (was 3[4-9][0-9], never matched, 90 s timeout). 16 s. Picture read: File select, File 1 AAAAAAAA First Play, Select file to start.
| fileselect_rotate | Same command as file_select. File select not reached, so frames 282 to 296 are not shown. | same as file_select |
 Until changed to shot 29[0-6] (was 90 s timeout). Not re-run: budget. Status unverified.
| house_sign | Open. Warp kmr_20 entry 4 passes. Sign not readable at 240x400. A camboom switch (boom length on nextRig and prevRig) did not change the view and was reverted. | build3dszt\shots\house_sign |
| super_text | Open. Battle starts. Run 1 logged actionState 5, 8 and 10, no multibounce. Run 2 (dense A presses) logged actionState 0 only. No SUPER text in any shot. | build3dszt\shots\super_text |
| mac01t (FINAL4, verified) | Census view. FINAL4 run: mdlsum f899 tot ops 1696 in 1623 to 2065, 6 of 6 log checks. Top shot 0000_1_2 (frame 500) shows Mario and Toads on a Toad Town street, HUD on. Census view of mac_01 reached by the tour path (`-Try`): warp mac_00 entry 4, tour step mac_01 entry 0 at about frame 209. Window is mdlsum f899 (frames 600 to 899). Gate 1175 to 1495 (1335 +/- 12 %, FINAL9; R2B measured 1335). Older gate 1623 to 2065 (1844, set after EN2, T1B, NP1B, HT1) is outdated: R5B measured 1524 and R2B 1335. Older gate 1900 to 2200 is outdated: HEAD 2315 and T1B 1844 were measured before the range was set. | build3ds\azt\shots\mac01t |
| forest_mim (FOREST diagnostic, unverified) | Warp to mim_05 entry 0 (area 12 map 4, Forever Forest). Command: `azt.ps1 forest_mim -Row FOREST -Try`. Run 1 (default build): 5 of 5 log checks. f599: opsplit ops 980, mdlsum draws 6, tris 12, dlc2 static tri 55800 of 68100. Picture read by agent: dark forest, Mario and partner. A person must confirm. No numeric gate. Scenario added for diag_forest.md. | build3dszt\shotsorest_mim |
| sun_sbk (SUNRAYS, unverified) | Dry Dry Desert sun overlay and ray layers (sbk_00, entry 0, story ch0_toad_town). Run with `-Try`. One run (1 of 3 starts): 3 of 3 log checks. Picture read by eye: faint hard-edged cells in the sky strip. See docs/handoff/diag_sunrays.md. | build3ds\azt\shots\sun_sbk |
| toad_gate (TOADVIS, unverified) | Toad Town west gate view (mac_00 entry 0, preset toad_west), shots at frames 200 and 204. Three runs: working tree, 29f504d7b, final6 (3dcfc661d). Picture: west gate path, no water, no fountain in view. Budget of TOADVIS used up. See docs/handoff/diag_toadvis.md. | build3dsztshots	oad_gate |
| mac01s (TOADVIS, unverified) | Same view as mac01t (tour to mac_01 entry 0), until on shot line 504 (cap 40 s). Working tree build: 2 of 2 top shots byte-equal to final6 and 29f504d7b. Strip at the right edge is not confirmed as water. mac01t until mdlsum did not print on 29f504d7b (idled 121 s). | build3dsztshotsmac01s |
| toad_fountain (TOADFOUNT, unverified) | Mac_01 walk by padscript: entry 0 (x -600) to x -300 (seg b, frame 114), then to x -170 (seg c, frame about 158). Shots every 4 frames, frames 100 to 148 (13 shots). Until on shot frame 148 (cap 40 s). Runs: default build (HEAD f230e46c6, build 1488), `-Build final` (29f504d7b), `-Build toadf6` (final6 3dcfc661d). No fountain in view in any run: the shots show the Toad Town street and a post office side. Top shots differ in 690 to 2100 px, all at x 70 to 144 (moving NPC sprites). Next test needs a view of the candidate at about x -300, z -325 (docs/handoff/diag_toadfount.md). | build3dsaztshots	oad_fountain |
| toad_water (TOADFOUNT, unverified) | Position run at the fountain candidate: warp mac_01 entry 0, padscript to (-300, -150) then (-300, -240), shots frames 140 to 176 (every 4). HEAD run 1 (build 1488): first shot shows the bulletin board, Mario and a blue stone base, no fountain. Run stopped by rule. | build3dsaztshots	oad_water |

## POSHUD (2026-10-29): bottom screen position values

- Switch `poshud` (debug string, or options.ini `poshud=1`, Debug tab row 8 "Position HUD"). Bottom screen rows 0 and 1 show Mario x y z yaw and the camera x y z yaw. Refresh: hudint (the memory rows are replaced by these two rows while on).
- Mario yaw is gPlayerStatus.targetYaw. Camera values are gCameras[CAM_DEFAULT].lookAt_eye and curYaw.

| Scenario | Flags | Command | Result (build poshud, same code as the commit) | Status |
|---|---|---|---|---|
| yard_poshud | `poshud` | `yard_poshud -Try -Row POSHUD -Build poshud` | 3 of 3 log checks. Bottom shot bot_0259: `M x 594 y 30 z -216 yaw 180`, `C x 924 y 161 z 60 yaw 310`. Log at f300 gives Mario pos 594 30 -216. Top shot 0000_0_11 byte-equal to yard_headA (HEAD e16fee4b9). | unverified (one run) |

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

- Shots: one top shot per run (`build3dszt\shots\<scenario>`). A person must open each shot and confirm the subject (pause status page, Party tab, file select screen).
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
| pause_fsplit (until fsplit f300) | fsplit (pause open 96 to 140) | `pause_fsplit -Try` | f300: logic 0.8 dl 0.7 gbi 7.0 flush 0.1 gwait 0.0 swap 0.3 audio 0.0 rest 25.3 wall 34.1 over50 7 svc 11.4 | unverified |

- Cost (Azahar, direction only): steady window +0.01 ms (7.00 vs 6.99 ms work). Load window +0.02 ms (11.79 vs 11.77). Limit 0.2 ms: passed.
- Pictures (shot_cmp.py, top shots, byte-equal): yard 1 of 1 (yard, yard_pace, yard_fsplit). mac01t_fsplit 2 of 2 against shots_base_mac01t (older build). pause_fsplit 1 of 1 against pause_menu (LOGFIX build). The yard_fsplit shot is from the build before the over50 fix (summary arithmetic only). A person must read the shots by eye.
- Log note: a run that stops on its until-line can lose the last lines of the on-disk log.txt. azt reads the live lines; build3ds/last_log.txt holds the copy used here.
- Mistakes: mac01t_fsplit run 1 used until `fsplit f899`. That line never prints (windows print at 300, 600, 900). The run ran to the 120 s cap. Fixed to f900. yard_pace run 2 used a window-1 regex and failed the steady-frame check. Fixed in run 3.
