# Post-S28 gaps (Q-PO2, O3DS baseline, MA1 replay, merge order, console tests)

Written 2026-10-19. Read-only plan. Sources: post_s28_optimization.md, S28_prep.md, PO2.md, MA1.md, MA1_list.md, B7.md, V16b_azahar.md, V18_V19_azahar.md, 3ds/SLICES.md, HISTORY.md:1548. `/push-o X` = `powershell -NoProfile -File 3ds/go.ps1 push o X`. `/pull-o` = `go.ps1 pull o`.

## 1. Q-PO2 (now in 3ds/QUESTIONS.md)
HISTORY.md:1548 says "PO2 ... Not in QUESTIONS.md yet. Ask the user." The question: do the "each corner once" speed-up?
- A: yes, after S28, lane B, golden 0 difference. B: no, PO3 only. C: only if the O3DS shows GBI interpret 5 ms or more per step.
- Recommendation: A. CPU only, 2 files, same pictures, PO3 needs it.

## 2. O3DS baseline (user steps, O3DS at 192.168.1.168)
1. Put `dspfirm.cdc` in `sdmc:/3ds/`. Put `papermario.z64` in `sdmc:/3ds/PaperMario/`. Delete old `debug3ds.txt` files.
2. Before each run: open Homebrew Launcher, press Y. After each run: HOME, close the game.
3. Run A boot: `/push-o -GameDebug "loglevel=debug" -Seconds 75`. (The `mem` lines are debug lines. Without `loglevel=debug` they do not show.)
4. Run B title: `/push-o -GameDebug "audiostat pacestat logstat" -Seconds 60`. Do not press keys.
5. Run C toad: `/push-o -GameDebug "fast preset=toad audiostat pacestat logstat loglevel=debug" -Seconds 90`.
6. Run D yard: `/push-o -GameDebug "fast preset=yard audiostat pacestat" -Seconds 60`.
7. Run F battle: `/push-o -GameDebug "fast preset=yard battle audiostat pacestat" -Seconds 60`.
8. Run E1 phase toad: `/push-o -GameDebug "fast preset=toad phase phasecsv" -Seconds 60`. Start the FTP app. `/pull-o`. Then `python 3ds/tools/phase_report.py build3ds/hw_o3ds_log.txt`.
9. Run E2 phase yard: same as step 8 with `preset=yard`. Never put `phase` in the same run as `audiostat` or `pacestat`.
10. After each run copy `build3ds/hw_live_log_168.txt` to `build3ds/s28_o_<A|B|C|D|F|E1|E2>.txt`. Tell the orchestrator "baseline done". An agent reads the files.
Numbers to copy back (table in S28_prep.md section 7): `mem commit limit` (A), `mem split H03` heap KB (A), `APT_SetAppCpuTimeLimit` result (A), `audio cost avg/max us` and `audio posts` dropped and underrun (B, C, D, F), `pace: steps/s` and slow intervals 3 and 4+ (B, C, D, F), `mem ... linear peak, texture failures` (C), phase wall, cpu, gbi, fbegin ms and over33 count (E1, E2).

## 3. MA1 replay design
- Step 0 code: switch `statehash=<N>` (default 30). New static function `state_hash()` in 3ds/src/pm_warp.c, called from `pm_warp_tick(frame)` every N frames, and once more at smoke end next to `pm_audio_hash_log()` (pm_warp.c:429). Log line: `statehash f <frame> fnv <8 hex>`. About 30 lines. No other file.
- Hash covers (FNV-1a, floats as raw 32 bit words): gRandSeed. Area and map id. Player position, yaw, action state, flags, speed. Player data HP, FP, coins, star points. Every live NPC in the current list: position, yaw, flags. Default camera eye and target. In battle: player, partner and each enemy actor position, yaw, HP. Check the field names in include/ before code.
- Not covered: render state, texture cache, audio (audiohash does that), wall-clock timers.
- Replay runs (switches `fast fixedseed audiolock statehash=30`, Azahar `-Turbo -Smoke -FaultStop`): toad `preset=toad autoa smokeframes=600`, yard `preset=yard autoa smokeframes=600`, battle `preset=toad battle autoa smokeframes=330`. Add `demoscene=5 preset=toad smokeframes=330` (jan_04, NPC and camera math) for D8 and D11.
- Baseline: 2 runs per scene (6 starts). The second run adds an unused token `run2` so the cache does not skip it. The 2 runs must give equal lines. If not, step 0 finds the input that differs first. No MA1 change before that.
- Identical = the same number of `statehash` lines, the same frame and fnv in every line, the same final line. For P20 also the same `audiohash ... (final)` line. Render sites: golden 0 difference. No tolerance for game logic.
- Approval rule for a game-logic math change (D8 to D12, D16, P16 if it moves NPCs): (1) statehash identical in every replay scene. (2) A measured gain of 0.2 ms per step or more on the touched phase. (3) The agent writes `Q-MA1-<site>` in 3ds/QUESTIONS.md with the old line, the new line, the hash result and the gain. (4) No merge before the user writes yes. A change with any hash difference is dropped, not sent to the user.

## 4. Merge order and file ownership
| Row | Tree | Files | Merge order | Azahar slot |
|---|---|---|---|---|
| MA1 step 0 | lane B, same agent as PO2 | pm_warp.c | 1 | 1 (6 starts, baseline hashes) |
| AU1b to AU1d (AU1a done, 7022d34c6) | main | pm_audio.c, pm_os.c, pm_main.c, pm_phase.c (`__wrap_step_game_loop`), pm_log.c | 2 | 2 |
| PO2 (after Q-PO2 = A) | lane B | pm_gbi.c (G_VTX, `draw_tri`, `xform_vtx`, `vert_color`, `vert_uv`), pm_phase.c (timer) | 3, rebase on AU1 | 3 |
| Hardware session 1 [USER] | O3DS | none | after 3 | AU1e, PO2 phase |
| MA1 render sites P13 to P17 | lane B | pm_tex.c, pm_gu.c | 4 | 4 |
| PO3a to PO3g | main | pm_gbi.c, pm_gpu.c, pm_gpu.h, 3ds/shaders, CMakeLists.txt, tools/golden.py | 5 | 5 |
| MA1 P18, P20 | lane B | pm_os.c, pm_audio.c | 6 (after AU1d) | 6 |
| MA1 P1 to P12, P7 | main | pm_gbi.c, pm_gpu.c | 7 (after PO3g) | 7 |
| MA1 logic sites | lane B | src/ files per site | 8, one per user yes | 8 |
| Hardware session 2 [USER] | O3DS | none | after 7 | PO3f, MA1 |
- The main tree has uncommitted edits in pm_main.c and pm_nusys.c now. Commit or clear them before AU1b.
- One Azahar user at a time. 8 starts per row.

## 5. Console test sheet (one console session, after S28)
| Row | Command | Scene | Look for | Send back |
|---|---|---|---|---|
| B7 | `/push-o -GameDebug "fast preset=toad battle phase phasecsv loglevel=debug" -Seconds 60`, then `/pull-o`, `phase_report.py` | Toad Town, then the battle start swirl | nothing by eye | "B7 done". Gap: no log line exists for grabs and splits per frame, VRAM use, swirl capture time or F03 query time. Row B7-0 must add them before B7. |
| F09 | none | none | none | nothing. US only (user decision 2026-10-18). No console test. |
| V16b | `/push-o -GameDebug "fast fixedseed preset=toad battle=BTL_IWA_FORMATION_02:BTL_KMR_1_STAGE_01 flashheavy=250" -Seconds 60`, then `/pull-o` | Monty Mole battle. The moles flash every 40 frames from about 8 s. | a white square around a mole (fault) or a white mole shape (good) | "box" or "no box", a photo if box. The agent greps `V16b2:` in the log. |
| V18 | `/push-o -GameDebug "fast fixedseed" -Seconds 150`. Then again with `fast fixedseed nozclip`. Then `fast fixedseed depthview=3` only if it still blinks. | Intro end: the camera tilts up over the road to the sanctuary (about 112 to 118 s) | road or walkway parts that blink in and out | "blinks" or "no blink" per run. A phone video of the depthview run. |
| V19 | `/push-o -GameDebug "fast fixedseed demoscene=5 preset=toad" -Seconds 30`. Then again with `nozt`. Then `depthview=3`. | Sushie dives under the log (about 7 to 8 s) | shadows that blink near the log and water | "blinks" or "no blink" per run, and which shadow (Mario, Sushie, other). |
- Gap: the `decalbias` switch from V18_V19_azahar.md is not in the source. Add it (pm_gpu.c:387, about 5 lines) before the session. Then V18 and V19 get a 4th run with `decalbias=0.00003`.

## Decisions only the user can make
- Q-PO2 (A, B or C).
- Each MA1 game-logic change (Q-MA1-<site>).
- S28 start and the O3DS session times (sections 2 and 5).
