# DIAG FOREST: Forever Forest (mim_05, area 12 map 4)

Sources: build3ds/hw_live_log_170.txt (O3DS, build 1467, ids #5200 to #5813). Azahar: scenario forest_mim, 1 of 3 starts (row FOREST). Tags: [M] measured, [C] code, [E] estimate.

## 1. Corrections
- Mode 4 is GAME_MODE_WORLD. Mode 7 is ENTER_WORLD, 8 BATTLE, 9 END_BATTLE (src/game_modes.h). HWLOG4 labels mode 4 as ENTER. [C]
- The 51.7 ms is one 217-frame window, f27826 to f28043, 5 frames after END_BATTLE. It is not steady forest. [M]
- The 27 closes line (dcsum f28338) covers f28038 to f28338. Battle runs from f28043, so 295 of 300 frames are battle. No dcsum or logstat line covers the post-battle window. [M]
- The 1645 Toad ops in the brief are not in this log. Azahar gives 1732 (f599) and 1844 (f899). [M]

## 2. Segments (pace lines, wall ms per drawn frame) [M]
| Segment | Frames | Wall ms | Logstat work / wall (window) | Closes |
|---|---|---|---|---|
| A calm WORLD | f27266-27553 | 39.6 | 11.9 / 34.5 (f27438-27738) | 9 |
| B BATTLE | f27553-27821 | 34.6 | mixed window | mixed |
| C post-battle WORLD | f27826-28043 | 51.7 | none | none |
| D BATTLE | f28043 on | 34.1 | 32.0 / 38.9 (f28038-28338) | 27 |

Toad Town steady (HWLOG3, console): wall 48.5 ms, work 48.2 ms, 76.6 closes, draw 30.6 us per close.
## 3. Azahar mim_05 against mac01t, f599 (direction only) [M]
- Ops 980 against 1732. Draws 6 against 58. Tris 12 against 202 (mdlsum).
- Binds 73 against 85. Closes 28 against 32. Models 91 (static 91) against 89 (static 80).
- dlc2 per 300 frames: 68100 tri, 82 % static, against 91040 tri, 100 % static.
- Logstat work 11.97 ms, wall 33.90 ms. Console calm work 11.9 ms matches. [E]
- Picture: dark forest, Mario and partner, HUD on. Agent read only. A person must confirm.

## 4. Triangles per frame (dlc2 tri is a 300-frame sum; gv1 tri delta agrees) [M]
- Static tri per frame: f26999 58, f27299 187, f27599 30, f27899 277, f28199 552 (post-battle), f28499 605 (battle).
- Toad Town console: 923 per frame (277040 / 300), steady.
- Visible tris do not explain the extra cost. Post-battle forest has fewer tris than Toad. [E]

## 5. Source facts for mim_05 [C]
- SetForeverForestFog: fog 990 to 1000. Camera far is 650. Fog starts past the far plane. Gv1 fog count is 0 in all windows. [M]
- hint.c SetupExitHint: 17 models get texture panners. Top 12 mdlsum rows show pan 0, static 299/299. [M]
- npc.c: 2 piranha plants. gate.c and entity.c are empty. [C]
- Battle stage of mim maps sets fog 950 to 1000 (src/battle/common/stage/area_mim/mim_01.inc.c). END_BATTLE does not reset it. [C]
- END_BATTLE (src/state_battle.c, state_step_end_battle) reloads the shape (decode_yay0, bumps pm_dlc_gen), collision, map BG, all textures, NPCs. [C]
- DL compile count per 300 frames: 0 (f27599), 168 (f27299), 203 (f27899), 30 (f28199), 131 (f28499). [M]
## 6. Ruled out [M]
- Fog (gv1 fog 0), grabs and splits (b7 0 on 714 lines), texture misses and failures (0), copy buffer (2 events), lit fallbacks (about 0.3 per frame).

## 7. Top 3 candidates (not verified) [E]
1. Post-battle reload and tri rise. Extra about 17 ms over segment C. Static tris rise from 30 to 190 per frame to 550 to 590. Smallest fix: none yet. Measure first: phase on, 300 frames after END_BATTLE. Risk: none.
2. DL recompile after reload. decode_yay0 bumps pm_dlc_gen, so every DL entry rebuilds. Compile count 131 to 203 per 300 frames after battle, 0 in calm. Estimate 0 to 1 ms per frame, not measured. Smallest fix: skip the gen bump when the shape hash is unchanged (src/state_battle.c, 3ds/src/pm_dlc.inc.c). Risk: stale DL if the hash misses a change.
3. Texture re-upload after reload (mdl_load_all_textures). Forest VRAM bank A at load: 3031 KB used, 41 KB free. Misses stay 0. Smallest fix: none until texstat runs on segment C. Cost unknown.

## 8. Not measured
- Ops, dlc and CPU split in segment C. Azahar cannot reach it: a battle win is needed, and azt battle switches do not win.
- Next: one Azahar run with a battle driver and phase, or one console run with phase on the post-battle window.
