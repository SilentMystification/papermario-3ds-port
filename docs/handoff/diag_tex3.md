# diag_tex3: verify texture fixes A and B (row tex3, lane tex3)

Branch worktree-agent-ac8950c0d9c243322. Build 1187-tex3 (lane tex3, 3dsx sha256 07ee16b3...). Azahar starts used: 6 of 6 (budget file 3ds/logs/test_budget.csv in this worktree). No starts left.

## 1. golden.py RUN FAILED without an Azahar start
Cause: run_azahar.ps1 has a budget cache. For the same row, 3dsx sha, and args with an earlier rc 0 end, it prints "cached ok" and exits 0 without starting Azahar and without a shot. golden.py deletes the shot before the run, finds no "matched" line, and reports RUN FAILED. A scene that differed is not in 3ds/logs/golden_ok.json, so a rerun of it hits this cache.
Check: `powershell -File 3ds/run_azahar.ps1 -Turbo -FaultStop -Timeout 90 -Until "shot 2 frame" -Debug "fast preset=toad battle fixedseed shots shotwin=300:3 statehash=100"` printed "cached ok (same row, 3dsx and args, rc 0)" and "budget: row tex3 run 2/8 (not run, use -Force to rerun)".
Fix: 3ds/tools/golden.py passes -Force. A golden call is a real run, and golden_ok.json still prevents reruns of passed scenes.
Not changed: the lock wait is the script default (900 s). golden.py does not pass -LockWait, so the 3 minute wait in the task is not applied.

## 2. statehash (base from diag_tex2: toad f100 94083528, battle f200 dda688fa, battle f300 6b6fe648)
| run | frame | value | base | result |
|---|---|---|---|---|
| battle (`preset=toad battle`) | f100 | 94083528 | 94083528 | match |
| battle | f200 | dda688fa | dda688fa | match |
| battle | f300 | 6b6fe648 | 6b6fe648 | match |
| toad (`preset=toad`) | f100 | 94083528 | 94083528 | match |
| toad | f200 | 83b272fe | not recorded | no base |
| toad | f300 | aed188c8 | not recorded | no base |

The statehash is game state, not pixels. All base values match, so the game state did not change.

## 3. golden.py (reference 3ds/golden, tolerance 8, fail above 0.5 percent, battle above 3.0 percent)
| scene | status | % px > 8 | max | note |
|---|---|---|---|---|
| battle | ok | 2.47 | 189 | below the battle limit of 3.0 |
| toad (statehash extra) | ok | 0.01 | 148 | |
| fileselect | DIFF | 2.46 | 222 | see below |
| intro | DIFF | 14.75 | 253 | mean 4.79, see below |

### Intro (Mario House sign)
- The sign reads MARIO in the reference frame and in the new frame (crop of the sign, both rotated and legible).
- The difference is not the sign only. Differing pixels cover x 20 to 219 and y 52 to 347 (most of the frame, not a sign-sized area). A shift of 1 to 3 pixels in x or y does not lower the difference. Cause not found.

### File select
- At frame 400 the boxes hold their contents: File 1 (First Play), File 2, File 3, File 4 (NEW), and the side labels Stereo, Delete File, Copy File, Options, Cancel, Mono, Select file to start.
- The "File 1" name text differs. The reference shows eight A glyphs. The new frame shows A, A, then other glyphs, then 000. The save data on the SD card may differ between the reference run and this run. Cause not found.
- Rotation check NOT DONE. Frames 150, 151, and 152 are identical and show a blank screen, so the menu is not open at that window. The window was the last start. A different window needs a new start, and the budget is used up.
- The window 150 log (last_log.txt of that run) has no "V05" line. The fileselect run at 400 was not checked for V05, because its log was overwritten.

### Multibounce SUPER text: NOT REACHED
- No preset reaches it. The presets are in 3ds/src/pm_warp.c (intro, yard, toad, toad_east, toad_west, osr, kmr02, credits). None starts a battle with a jump input. `preset=toad battle` starts a battle with no input script.
- The text code is fx_attack_result_text (ver/us/symbol_addrs.txt 0x80071390), with the multibounce move in battle_move_multibounce_* (0x802A1000). The multibounce code is not in 3ds/src.
- Missing: a battle switch with a jump script (padscript with A presses) that makes a multibounce, and a log line that shows the text draw.

## Next
- Run the file select flip. Frames 150 to 152 show a blank screen, so take a later window. The frame of the menu is not measured yet.
- Run a battle with a jump input (padscript) and a log line for the multibounce text.
- Decide whether the intro diff (14.75 percent over the whole frame) is a timing difference. Compare with a second run of the reference build.
