# Orchestrator state snapshot

Date: 2026-10-18.
Use this file to resume the orchestrator in a new chat. Read it first.

## 1. Running work at this moment

| Item | Where | Status at snapshot | How to check |
|---|---|---|---|
| X02 slice B0 | Main tree. Uncommitted edits in 3ds/src/pm_gpu.c, pm_gbi.c, pm_main.c, pm_tex.c, 3ds/include/pm_gpu.h, pm_tex.h | Running. Not committed. | `git status`. Tail of 3ds/PROGRESS.log. |
| F01 trd_06 faults | Lane B: ..\papermario-3ds-port-laneB, branch lane-b | Running. | `git -C ..\papermario-3ds-port-laneB status`. Tail of 3ds/PROGRESS.log. |

Agents may have stopped. Check the agent list before you wait for a result.
A row is done only when its PROGRESS.log line and its 3ds/SLICES.md row agree.

## 2. Done this session

| Item | Commit | Branch |
|---|---|---|
| G01g golden frames | 8a25a4583 | main |
| H03 memory split and graceful texture failure | 7a56321c7 | main |
| V08 save block verdict (no code change) | f737c96a8 | main |
| T02 run_azahar mutex and config restore | 695964d4c | main |
| S30b 154 id-only maps | d7d626ea7 | lane-b |
| F01 kmr_23 cause | 04ae78a19 | lane-b |
| F01 tst_12 measure | 57a23316f | lane-b |
| F01 nok_01 (tour harness: bgm_set_song(1, AU_SONG_NONE) in tour_step) | 6ead17dbd | lane-b |
| S31 crash screen closed | 1a23cbe64 | main |
| S18 done (rows in docs/SLICES_DONE.md) | see docs/SLICES_DONE.md | main |
| S19c done (poly_scan.py) | 5beb113c1 | main |

S19c scan thresholds are too loose (flags natural dirt). The pass was by eye on 3 shots. Low priority tune.

## 3. Open queue, in order

Each row waits for the row above it, unless the blocker column says otherwise.

| # | Row | Blocker |
|---|---|---|
| 1 | X02 (slice B0) | Running. Finish and commit first. |
| 2 | V06 | Row 1 |
| 3 | B1 | Row 2 |
| 4 | B2 | Row 3 |
| 5 | B3 | Row 4 |
| 6 | B4 (= F03) | Row 5 |
| 7 | B5 | Row 6 |
| 8 | B6 | Row 7 |
| 9 | F08 | Row 8 |
| 10 | V10 | Row 9 |
| 11 | V11 | Row 10 |
| 12 | V09 kmr_22 | Row 11 |
| 13 | F01 trd_06 | Lane B run in progress |
| 14 | Merge lane-b into main | Row 13 and any other lane-b row |
| 15 | One full tour | Row 14 |
| 16 | One golden run | Row 15 |
| 17 | S28 hardware run | Row 16. User input: O3DS on at 192.168.1.168, dspfirm.cdc on the SD card, papermario.z64, FTP app, Luma dump path. |
| 18 | PO2 | Row 17 (S28) |
| 19 | PO3 | PO2 and the S28 gate |
| 20 | AU1 | Row 17 (S28) |
| 21 | MA1 | Row 17 (S28) |
| 22 | B7 | Row 17 (S28) |
| 23 | F06 | Row 22 |
| 24 | F07 | Row 23 |
| 25 | F09 | Row 24 |
| 26 | F10 (tst_12 loop bug, last) | Row 25 |

Only one build or Azahar agent runs at a time in the main tree.

## 4. User decisions (2026-10-18)

| Decision | Effect |
|---|---|
| V07 and V09 intro tints are shelved. | Work is on branch shelf/v09-nok01. Not in the queue. |
| All 421 maps are linked and resident. | No SD streaming during play. |
| tst_12 stays linked. | Its loop bug is row F10 (queue row 26), last. |
| AGENT_RULES.md (new rule). | Slice agents read 3ds/AGENT_RULES.md, not ORCHESTRATOR.md. Keep it at most 45 lines. |
| Optimization starts only after S28. | Rows 18 to 22 wait for S28. |
| S28 starts only after all Azahar-verifiable work. | S28 is the gate. |
| Narrow tests. Tight prompts. | Use small test scopes and short agent prompts. |
| No flattery. Replies in ASD-STE100. | Apply to every reply. |
| The orchestrator may ask an agent through SendMessage. | Check agent state with a read-only agent when the user asks what an agent does. |

## 5. Pending questions for the user

| Question | Recommendation |
|---|---|
| Crash text on the bottom screen. | No. |
| S28 readiness: O3DS on, dspfirm.cdc, papermario.z64, FTP app, Luma dump path. | Answer before row 17. |
| PO2 (after S28). | Decide before row 18. |

## 6. How to resume

1. Read this file.
2. Read 3ds/ORCHESTRATOR.md and HANDOFF.md.
3. List the agents. Check the state of any agent that matters.
4. Run `git status` in the main tree and in ..\papermario-3ds-port-laneB. Read the tail of 3ds/PROGRESS.log.
5. Relaunch any slice that stopped. Mark its row as not done.
6. Continue at the first open row in section 3.
