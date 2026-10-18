# Orchestrator state snapshot

Date: 2026-10-18 (end of first orchestrator chat).
Use this file to resume the orchestrator in a new chat. Read it first.

## 1. Running work at this moment

No agent runs. All agents finished. Check `git status` in both trees before you launch anything.
Main tree: ..\papermario-3ds-port (branch main). Lane B: ..\papermario-3ds-port-laneB (branch lane-b, PM_LANE=laneB, rules in docs/LANES.md).
The V09 kmr_22 fix is committed on lane-b (378f2879f). Check `git -C ..\papermario-3ds-port-laneB status` for stray edits.

## 2. Done this session

| Item | Commit | Branch |
|---|---|---|
| G01g golden frames | 8a25a4583 | main |
| H03 memory split and graceful texture failure | 7a56321c7 | main |
| V08 save block verdict (no code change) | f737c96a8 | main |
| T02 run_azahar mutex and config restore | 695964d4c | main |
| X02 darkness overlay = slice B0 (kgr_01, Watt, omo_12, omo_14 by pixel check; kzn_08 not tested) | 9b122e395 | main |
| S31 crash screen closed (verdict b) | 1a23cbe64 | main |
| S18, S19c (poly_scan.py thresholds too loose, pass by eye) | 5beb113c1 | main |
| S30b all 421 maps linked, guard removed | d7d626ea7 | lane-b |
| F01 kmr_23 (tour harness) | 04ae78a19 | lane-b |
| F01 tst_12 measured, moved to F10 | 57a23316f | lane-b |
| F01 nok_01 (tour harness: bgm_set_song(1, AU_SONG_NONE)) | 6ead17dbd | lane-b |
| F01 trd_06 (tour uses partner 1) | f49ac2251 | lane-b |
| V11 closed: real geometry (25 high ledge needs a full jump) | ea0342683 | lane-b |
| V09 kmr_22 fix: tile 1 source per TMEM address in pm_gbi.c | 378f2879f | lane-b |

Plans in docs/plans/: PO3, AU1, MA1_list, F01_causes, F03_F08, S31, S28_prep, S18_S19c_commands, render_gaps, buffer_bridge, X02_darkness_notes. Memory budget: docs/MEMORY_BUDGET.md.
Note for the merge: V09 kmr_22 changes pm_gbi.c on lane-b and X02 changed pm_gbi.c on main. Expect a conflict. Re-run the X02 check (3ds/tools/x02_check.py) and golden.py after the merge.

## 3. Open queue, in order

| # | Row | Blocker |
|---|---|---|
| 1 | B4 (= F03) depth readback | none |
| 2 | V06 outside of a building is black | row 1 (graphics, one at a time) |
| 3 | V12 damage star zoomed and clipped, plus test of other battle effects | row 2 |
| 4 | B1 | row 3 |
| 5 | B2 mid-frame frame grab | row 4 |
| 6 | B3 | row 5 |
| 7 | B5 | row 6 |
| 8 | B6 | row 7 |
| 9 | F08 fog factor | row 8 |
| 10 | V10 Strategies box checkerboard | row 9 |
| 11 | Merge lane-b into main (conflict in pm_gbi.c expected). Close F01 and the kmr_23 part | rows 1 to 10 may run before or after. Merge early if a graphics agent needs the 421 maps. |
| 12 | Test kzn_08 darkness (X02) after the merge | row 11 |
| 13 | One full tour with -FaultStop | row 12 |
| 14 | One golden run | row 13 |
| 15 | H05 hardware-readiness audit (see 3ds/SLICES.md) | row 14 |
| 16 | S28 hardware run | row 15, H05 findings fixed or accepted by the user, user input below |
| 17 | PO2, PO3, AU1, MA1, B7 (optimization) | S28 |
| 18 | F06, F07, F09, F10 (tst_12 loop bug, last) | rows above |

Only one graphics agent at a time. Other lanes may run non-graphics rows.

## 4. User decisions (2026-10-18)

| Decision | Effect |
|---|---|
| V07 and V09 intro tints are shelved. | Branch shelf/v09-nok01. Not in the queue. The user reports them again if seen. |
| All 421 maps linked and resident. | No SD streaming during play. |
| tst_12 stays linked and warpable (future scene selector). | Loop bug is row F10, last. |
| Optimization starts only after S28. Planning is allowed. | PO2, PO3, AU1, MA1 wait. |
| S28 only after all Azahar-verifiable work and H05. | No early boot test. |
| Narrow tests, tight prompts, token efficiency. | Agents read 3ds/AGENT_RULES.md (45 lines). Batch docs edits. The orchestrator may edit docs itself (small edits). |
| No flattery. Replies in ASD-STE100. | Every reply. |
| Orchestrator calls out its own bloat and hands off. | This file is the handoff. |
| User wants one general solution for N64 memory image effects. | docs/plans/buffer_bridge.md. Check PO3 risk R7 at every PO3 slice. |
| Usage-limit auto stop: not wanted. | None. |
| User confirmed faults to fix: V12 (damage star). Check other battle effects for the same cause. | Row 3. |

## 5. Pending questions for the user

| Question | Recommendation |
|---|---|
| Crash text on the bottom screen. | No (decide after S28). |
| S28 readiness: O3DS on at 192.168.1.168, dspfirm.cdc, papermario.z64 on the SD card, FTP app, Luma dump path. | Answer before row 16. |
| PO2 (after S28). | Yes, after S28 timing. |
| Does the X02 darkness effect look right by eye? Command: `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -Debug "fast warp=kgr_01:0 partner=6 shots"` (partner=1 for the plain circle). | The user confirms. |

## 6. How to resume

1. Read this file.
2. Read 3ds/AGENT_RULES.md for the rules that agents follow. Read 3ds/ORCHESTRATOR.md only for orchestrator rules (model rule, section 1).
3. Run `git status` in the main tree and in ..\papermario-3ds-port-laneB. Read the tail of 3ds/PROGRESS.log.
4. Continue at the first open row in section 3. Launch with the slim prompt in ORCHESTRATOR.md section 5. Use `sonnet` for code and investigation, `haiku` for mechanical work, `opus` for planning only. Never `fable`.
5. Keep your own chat small. Hand off again when it grows long.
