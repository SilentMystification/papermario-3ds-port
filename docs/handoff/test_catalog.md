# Test catalog (azt)

Usage (one line): `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario>` (add `-Row <id>`, `-Build <lane>`; `-CheckLog <file>` checks an old log without a start; `-List` lists all).

Defaults: PM_ROW `azt`, build `build3ds\pm_3ds.3dsx`, cap 90 s, first marker deadline 20 s after the Azahar start. Each call is one real start (-Force) and counts against the row budget.
Exit codes: 0 log checks pass, 1 check or run failure, 2 marker missing (stopped), 3 ATTRACT LOOP (stopped), 4 unknown or unverified scenario, 5 argument or build error, 9 budget used.
A log pass is not a full pass. Read the shot files and confirm the subject. Shots of each run are copied to build3ds\azt\shots\<scenario>\ (runs before this change: shots are in build3ds\shots and were overwritten).

## Verified (log checks and picture read by eye)

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| yard | preset=yard loads kmr_20; steady frame; shot | `yard` | `warp: kmr_20 entry 4` | 15 s | build3ds\shots (run before archive; overwritten). Picture: yard, green pipe, Mario and partner. | Shot writer took frames 200 and 204 only (not 6). |
| tour10 | tour= visits 10 maps by the real map change | `tour10` | `warp: mac_00 entry 4` | 20 s | build3ds\shots (overwritten). Picture: Mario in all 11 shots. | Route list. Phase logging on. |
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
| tour_k30 | Log passes (tour: done, 6 maps). Picture fails: 4 of 7 shots show a grey page, the Prologue card and a text box. The fix (shot after each map load-done plus 15 frames) is a game source change in pm_warp.c or pm_gpu.c, outside this tool. The file holds 6 maps, not 30. azt2 tried a shot burst 15 frames after each map is in (tourshot=15:21), then 30 frames (tourshot=30:9): both still show 4 of 7 non-map shots (grey page, Prologue card, dark text box). Edit reverted, not committed. | build3ds\shots (overwritten) |
| partner_menu | Pause opens. Picture stays on the map page, not the partner tab. R*2 steps do not reach the partner page. The ui: partner line never logs. | build3ds\azt\shots\partner_menu\ (11 shots) |
| file_select | The golden fileselect padscript from boot does not reach mode 14 with -Turbo. Intro (mode 16) at frame 327. | build3ds\azt\shots\file_select\ (16 shots, intro logo) |
| fileselect_rotate | Same command as file_select. File select not reached, so frames 282 to 296 are not shown. | same as file_select |
| house_sign | Not run. No doc gives the sign position or a camera path in kmr_20. | none |
| super_text | Not run. No tested padscript makes a multibounce with jump presses. | none |

## Completion only

| Scenario | Reason | Command | Runtime |
|---|---|---|---|
| full411 | One start runs tour_s420.txt (411 maps) to tour: done. Not run in this task (lane 1 runs it). Same args ran at 2026-10-29 03:51 (rc 0, 143 s). | `full411` (-Milestone, cap 1800 s) | 143 s (earlier) |
