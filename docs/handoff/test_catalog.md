# Test catalog (azt)

Run a scenario with `3ds/tools/azt.ps1`. Scenario data: `3ds/tools/azt_scenarios.json`.
Do not hand-build `run_azahar.ps1` commands for a test. The script sets the switches, the until line and the cap.

    powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> [-Row <id>] [-Build <lane>]
    powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 -List
    powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -CheckLog <file>   (no Azahar start)

Defaults: PM_ROW `azt`, build `build3ds\pm_3ds.3dsx`, cap 90 s, first marker deadline 20 s after the Azahar start.
Every call passes `-Force`, so a call is always a real start and counts against the row budget (8 starts).
Exit codes: 0 log checks pass, 1 check or run failure, 2 marker missing (run stopped), 3 ATTRACT LOOP (run stopped),
4 unknown or unverified scenario, 5 argument or build error, 9 row budget used up.
A log pass is not a full pass. Open the shot files that the script lists and confirm the subject.

| Scenario | Status | What it proves | Command (azt.ps1 ...) | First marker | Runtime (last run) | Known limits |
|---|---|---|---|---|---|---|
| yard | verified | preset start loads kmr_20 and runs steady frames. Picture: yard, green pipe, Mario and partner sprite. | `yard` | `warp: kmr_20 entry 4` | 15 s (stop at `warp: scene f300`) | shotwin=200:6 writes 2 shots (frames 200 and 204), the writer takes one shot every 4 frames. |
| tour10 | verified | tour= visits 10 maps by the real map change path. Picture: Mario visible in all 11 shots. | `tour10` | `warp: mac_00 entry 4` | 20 s | Uses the route_test list. The list starts and ends on mac_00. Phase logging is on (slower). |
| tour_k30 | unverified | Log: warp to arn_02, then the tour file runs to `tour: done, 6 maps`. | `tour_k30` | `warp: arn_02 entry 0` | 17 s | The tour file tour_k30.txt has 6 maps, not 30. 4 of 7 shots show no map view (grey page, Prologue title card, text box). Picture check fails. |
| tour_no_warp | fail_demo | Fail-fast. A tour without warp=, preset= or story= never loads a map. The script stops the run at the deadline, exit 2, `SCENARIO FAILED: marker missing`. | `tour_no_warp` | `warp: [a-z0-9_]+ entry` | 21 s (killed) | Demo only. Not a map test. The ATTRACT LOOP check did not fire in this run, the marker check stopped it first. |
| pause_menu | verified | padscript opens the pause menu in mac_00 (pause mode is entered). Picture: pause status page with the dialog "Check here to see what Mario's current status is." | `pause_menu` | `warp: mac_00 entry 4` | 15 s (stop at shot 178) | Until and expected shot use frame 178. Frame 181 is never written by the shot writer. |
| battle_start | verified | The battle switch starts a real battle on kmr_03 (formation 0, stage 1). Picture: field, then the battle HUD with Mario and a Goomba (the first frame is a load card). | `battle_start` | `warp: kmr_03 entry 0` | 15 s (stop at shot 182) | Shot numbers in the shot line are a burst number (shot 4), not the frame index. Run 7 used the wrong until (shot 29) and timed out at 90 s. |
| sewer | unverified | Would prove a warp into tik_03 (Toad Town Tunnels) and steady frames. | `sewer` | `warp: tik_03 entry 0` | not run | Not run in this task (row budget 8 of 8). Use the orchestrator reset (run_azahar.ps1 -ResetBudget azt) to run it. |
| ruins | unverified | Would prove a warp into isk_04 (Dry Dry Ruins) and steady frames. | `ruins` | `warp: isk_04 entry 0` | not run | Not run in this task (budget). |
| volcano | unverified | Would prove a warp into kzn_19 (Mt. Lavalava) and steady frames. | `volcano` | `warp: kzn_19 entry 0` | not run | Not run in this task (budget). |
| partner_menu | unverified | Not defined. | `partner_menu` | `warp: mac_00 entry 4` | not run | No tested command. The partner tab index in the pause tabs is not verified (docs/handoff/diag_pause_closes.md). |
| file_select | unverified | Not defined. | `file_select -Try` | `game mode 1 frame` | not run | No tested command. The title and file select are not reached in 90 s, and the intro takes about 2 min (docs/handoff/route_test.md). |
| full411 | completion_only | One start runs all 411 maps of tour_s420.txt to `tour: done`. No picture check per map. | `full411` (Milestone, 1800 s cap) | `warp: arn_02 entry 0` | 143 s (earlier run, same args, 2026-10-29 03:51) | Not run again in this task (budget). |

## Scenario status meaning

- verified: the script ran the scenario, every expected event passed, and the shot was read by eye.
- unverified: not run, or run with a failing check. The reason is in the scenario file.
- fail_demo: a scenario that fails on purpose, to show the fail-fast path.
- completion_only: a long run that checks only the end of the run.
