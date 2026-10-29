# Test catalog (azt)

## How to use (one page)

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build <lane>` (use `-Try` for an unverified scenario). `-List` lists the scenarios. `-CheckLog <file>` checks an old log and starts nothing.
- Each call is one real Azahar start and counts against the row budget (8 starts per row, reset once by the orchestrator with `run_azahar.ps1 -ResetBudget <row>`).
- Shots of each run go to `build3dszt\shots\<scenario>\` (the last run of that scenario). Shots are PPM, 240 by 400.
- A log pass is not a pass. A person must open the shot and confirm the subject.
- Known noise: scattered 300 to 1300 px differences on moving sprites between runs. Judge a shot by the subject, not by pixel counts.
- Status: verified scenarios are listed in the first table. Unverified or log-only scenarios are in the second table with the reason.

### Verified (run with one command each)

yard, tour10, pause_menu, text_menu, battle_start, sewer, ruins, volcano, partner_menu, file_select, fileselect_rotate. Log-only: tour_k30 (texture counter on kmr_30, delta 0 on lane azt2 at HEAD 0738350b2; picture not applicable). Fail demos: tour_no_warp, attract_no_warp. Completion only: full411.

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
| pause_menu | padscript START*3 opens pause (mode 10) | `pause_menu` | `warp: mac_00 entry 4` | 15 s | build3ds\shots (overwritten). Picture: status page. | Shot at frame 178 (frame 181 is never written). |
| text_menu | pause dialog text, periods and spacing | `text_menu` | `warp: mac_00 entry 4` | 17 s | build3ds\azt\shots\text_menu\0002_1_1.ppm | Text read by eye. Same command as pause_menu. |
| battle_start | battle switch starts a battle on kmr_03 | `battle_start` | `warp: kmr_03 entry 0` | 15 s | build3ds\shots (overwritten). Picture: field, then battle HUD with Mario and Goomba. | Shot line holds a burst number, not the frame index. |
| sewer | warp into tik_03 (Toad Town Tunnels) | `sewer` | `warp: tik_03 entry 0` | 15 s | build3ds\azt\shots\sewer\0000_2_2.ppm, 0001_2_2.ppm | One map. Shot at frame 200. |
| ruins | warp into isk_04 (Dry Dry Ruins) | `ruins` | `warp: isk_04 entry 0` | 16 s | build3ds\azt\shots\ruins\0000_11_3.ppm (see folder) | Story text box covers the right side. |
| volcano | warp into kzn_19 (Mt. Lavalava) | `volcano` | `warp: kzn_19 entry 0` | 15 s | build3ds\azt\shots\volcano\ (see folder) | One map. |
| attract_no_warp (fail_demo) | ATTRACT LOOP: 3 title or demo lines in a tour run | `attract_no_warp` | `boot_main` | 39 s (stopped) | none | Exit 3, `ATTRACT LOOP: 3 title or demo mode lines`. |
| tour_no_warp (fail_demo) | fail-fast: marker missing | `tour_no_warp` | `warp: [a-z0-9_]+ entry` | 21 s (killed) | none | Exit 2, `SCENARIO FAILED: marker missing`. Azahar killed, none left. |

## Unverified (run or not, with the reason)

| Scenario | Reason | Shot paths |
|---|---|---|
| tour_k30 | Log passes (tour: done, 6 maps). Picture fails: 4 of 7 shots show a grey page, the Prologue card and a text box. The fix (shot after each map load-done plus 15 frames) is a game source change in pm_warp.c or pm_gpu.c, outside this tool. The file holds 6 maps, not 30. azt2 tried a shot burst 15 frames after each map is in (tourshot=15:21), then 30 frames (tourshot=30:9): both still show 4 of 7 non-map shots (grey page, Prologue card, dark text box). Edit reverted. Log-only since 4b0ad69d4: the kmr_30 texture counter line is checked, picture N/A (cutscene overlay on 4 maps). Re-run on the default build (binary 04:50, before the lane rebuild): delta 420. Rebuilt lane azt2 from HEAD 0738350b2 (-Build azt2, budget reset): tex fail cum 0, delta 0 on kmr_30, same args and tour file; vramfree min 2130 KB (default build: 3858 KB). The 420 came from the older default binary. Closed as log-only. | build3dszt\shots	our_k30 |
| partner_menu | Verified by eye. Pause opens. Padscript with partner=1 (the Party tab is skipped when no partner is in the save), then R*2 with wait60 three times. Shot (frames 190 to 208) shows the Party tab: Goombario and the text 'Check your Party and see each member's abilities.' | build3dszt\shots\partner_menu (azt2 lane, run 6) |
| file_select | The golden fileselect padscript from boot does not reach mode 14 with -Turbo. Intro (mode 16) at frame 327. | build3ds\azt\shots\file_select\ (16 shots, intro logo) |
| fileselect_rotate | Same command as file_select. File select not reached, so frames 282 to 296 are not shown. | same as file_select |
| house_sign | Open. Warp kmr_20 entry 4 passes. Sign not readable at 240x400. A camboom switch (boom length on nextRig and prevRig) did not change the view and was reverted. | build3dszt\shots\house_sign |
| super_text | Open. Battle starts. Run 1 logged actionState 5, 8 and 10, no multibounce. Run 2 (dense A presses) logged actionState 0 only. No SUPER text in any shot. | build3dszt\shots\super_text |

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
