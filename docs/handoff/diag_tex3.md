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

## Update (row tex4, lane tex3, 2 of 5 starts used, same binary 07ee16b3)

### Fileselect (new frame 400 vs reference, diff 2.46 percent)
- The diff sits in one strip: the File 1 name text (x 160 to 200) and the box edge at x 100 to 140. Other boxes and labels match.
- Reference reads "File 1 AAAAAAAA". New frame reads "File 1 AAA" then other glyphs, then "000".
- Verdict: REGRESSION SUSPECT, not intended. The name glyphs changed and no fix in this branch explains why. Likely cause: fix B slot reuse (the name may draw from an off-screen slot) or fix A glyph read. Not proven. Next check: a run on the base binary (before a03daae9c) with the same switches, and compare the name strip.

### Intro (diff 14.75 percent over the frame, mean 4.79)
- The sign reads MARIO in both frames: intended.
- The diff is not a shift (best shift 0,0 is the minimum) and not a brightness fade (new/ref brightness ratio 1.002). Mean signed diff per channel: R -1.6, G -2.9, B -6.4.
- The reference intro.png was last saved at commit 8a25a4583 (G01g), many commits before this branch. Verdict: STALE REFERENCE LIKELY, the whole-frame change is not from the texture fixes alone. Not proven: no base build was run.

### File select flip (rotation check, window 152 to 331, one shot per frame)
- Frames 152 to 159: blank. 160 to 178: the title logo fades in. 179 to 262: title with PRESS START.
- 263 to 273: the logo turns out. 282 to 296: the boxes turn in with their contents.
- Zoom check (frames 287, 290, 293, 296): the labels (File 3, First Play, File 1 name, Select file to start, Stereo, Delete File, Copy File, Options, Cancel, Mono, File 2, File 4, NEW) stay on their boxes during the turn.
- Verdict: PASS. The box contents turn with their boxes. No text shows at the top left.
- The window 330 to 389 from the coordinator is static (menu fully open), so the turn is not in that window.
- Note: the earlier window 150 to 152 was blank because the title screen had not started.

### File 1 name (row tex4, 4 of 5 starts used)
- Old golden (2026-10-19): "File 1 AAAAAAAA", 8 legible, evenly spaced A glyphs. New frame 400: "File 1 AAA", then 4 glyphs that are not legible A letters, then "000", uneven spacing.
- Verdict: NEW IS WORSE. The golden is not updated.
- The name is CI4 text (imglog glyphbind: tile fmt 2 CI, siz 0). It does not use the IA4 nibble path, so fix A (87b05658d) does not change it.
- Test: the glyph-same skip (pm_tex_glyph_same branch in pm_gbi.c, commit 8c021240a sign change to x0 - ggx) was disabled in lane tex4. The name still differs (2.53 percent diff vs old golden). So that skip is not the cause. The test edit is reverted, not committed.
- Not found: the draw that makes the wrong glyphs. The imglog glyphbind lines print x0 0 y0 0 and do not give the letter index.
- Not tested: fix B (off-screen slot reuse).

### Intro reference
- No base statehash for the intro is recorded, so the intro cannot be compared to a base. The intro golden is not regenerated.
