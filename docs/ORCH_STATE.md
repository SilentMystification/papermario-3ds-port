# Orchestrator state snapshot

Date: 2026-10-18 (end of first orchestrator chat).
Use this file to resume the orchestrator in a new chat. Read it first.

## 1. Running work at this moment

No agent runs. The merge of lane-b into main is DONE (93cab18e2, docs b44eba7f2). Lane B is rebased onto main. Tag lane-b-pre-merge keeps the old lane-b tip. Main tree: ..\papermario-3ds-port (branch main). Lane B: ..\papermario-3ds-port-laneB (branch lane-b, PM_LANE=laneB, rules in docs/LANES.md).
Merge checks passed: build links, 3dsx 13,013,472 B, x02_check kgr_01 (corners 0, centre 87), one tour over kpa_03, kzn_03, flo_03, jan_03, sam_03, pra_03, kzn_08 (rc 0, no fault). golden.py 9 scenes 0.00 to 0.01 percent (battle about 1 percent, known).
Not checked after the merge: the tour chunk trd_05 to nok_04 (stopped at trd_10, rc 0, no "tour: done" line, not re-run), the full tour. kzn_08 loads but draws no darkness (alpha stays 0, the fog script is unused there), so it is not a darkness fault.
Main tree clean.

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
| 3a | V13 lava texture in the volcano (docs/plans/V12_V13_diag.md) | row 3, one graphics agent at a time |
| 4 | B1 | row 3 |
| 5 | B2 mid-frame frame grab | row 4 |
| 6 | B3 | row 5 |
| 7 | B5 | row 6 |
| 8 | B6 | row 7 |
| 9 | F08 fog factor | row 8 |
| 10 | V10 Strategies box checkerboard | row 9 |
| 11 | DONE 93cab18e2 (merge lane-b; F01 closed) | rows 1 to 10 may run before or after. Merge early if a graphics agent needs the 421 maps. |
| 12 | DONE: kzn_08 loads and renders, darkness alpha stays 0 (fog script unused in kzn_08), no fault | row 11 |
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
| Every H05 finding goes into the active queue as a row and is done BEFORE S28. Then repeat the affected checks. | S28 does not start while an H05 row is open (user decision 2026-10-18). |
| Narrow tests, tight prompts, token efficiency. | Agents read 3ds/AGENT_RULES.md (45 lines). Batch docs edits. The orchestrator may edit docs itself (small edits). |
| No flattery. Replies in ASD-STE100. | Every reply. |
| Orchestrator calls out its own bloat and hands off. | This file is the handoff. |
| User wants one general solution for N64 memory image effects. | docs/plans/buffer_bridge.md. Check PO3 risk R7 at every PO3 slice. |
| ALL functionality is ported before S28 (user decision 2026-10-18). | S28 and later: hardware, missed fixes, optimization only. No new functionality after S28. H05 Part C (docs/plans/H05.md) audits this. |
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

## 7. PAUSE 2026-10-18 (usage limit). Newer than sections 1 to 3. Read this first.

All agents were stopped by the orchestrator. Nothing runs. The main tree holds UNCOMMITTED half-done work from the stopped agents (all in one tree, mixed):
| Stopped agent | Row | Uncommitted files |
|---|---|---|
| R13b | black blobs on water_splash and motion_blur_flame | 3ds/src/pm_comb.c, pm_gbi.c (half way: a fold call and a helper, pm_gpu.c cleanup not done) |
| V12d | 12 effect gfx data layout | 3ds/CMakeLists.txt, 3ds/src/pm_v12d_tmp.c (temp file, delete or finish) |
| V13c | texture cache peak | 3ds/src/pm_tex.c, pm_mem.c (maybe a log), tour script in 3ds/logs |
| F11-2 | Options item on file select | include/enums.h, filemenu.h, variables.h, src/filemenu/filemenu_common.c, filemenu_main.c, filemenu_msg.c, src/windows.c |
| FX1a | effect test harness | 3ds/src/pm_fxtest.c, 3ds/tools/fxtest_report.py, docs/plans/FX1_table.md, pm_main.c hunk |
| C2F to C2I | tour faults | only reading so far; 3ds/src/pm_warp.c hunk is from an earlier agent |
Resume plan: run `git diff` per file group. Relaunch each row with a fresh agent and tell it to continue from the uncommitted diff (or `git stash` the group). Do NOT run git checkout on these files without looking. Remove the git worktree 3ds/logs/wt_battle if it is not needed.
Done since section 3 (all local commits, nothing pushed): F03, V06, V12, V12b, V12c, V13, V13b, V10, B1, B2, B3, B3b, B5, B6, F08, L01, L01b, GP1, T01, T02, T01b, R05, R13 (checks), C3A, C3B, C3C, C4A to C4E, F11-1, plans F06 F07 F09 F11, V6 to V14 diagnoses.
Open rows now (3ds/SLICES.md is the truth): R13b, V12d, V13c, F11-2, F11-3, FX1a, FX1b, V14, C2F to C2I, C2R rest, C4F, F07-1 to F07-5, F06-1 to F06-6, then tour (13), golden (14), H05 re-check (15), S28.
New user decisions today: V12d yes; F09 US only; F06 and F07 are toggles on N3DS and O3DS, built before S28, default off, like PaperBoat; no 10 KB memory cap (revisit only if memory runs out); H05 findings that need no user decision become rows and are worked at once; C4E static map (D-pad, X, Y, C-stick = C; L and R = Z; N3DS ZL, ZR = N64 L, R); Options menu F11 before S28 for F11-1 to F11-3.
Open questions for the user: O3DS has no ZR (N64 R loses a button: stopgap?), F11 decisions D1, D2, D4, D5, what looks wrong on the fortune ball and wedge (V14), S28 hardware readiness.

## 8. PAUSE 2 (2026-10-18 evening, usage limit). Newer than section 7. Read this first.
Section 7 is obsolete: all those agents finished and committed. When FX2a to FX2g reports, the orchestrator stops launching. Nothing else runs.
Done since section 7: R13b, R13c, R13d, V12d, V13c, V14 (no code change), V15 (bilinear filter), V05b, F11-2, F11-3, FX1a, FX1b, C2F to C2I, C3A, C3B, C4E (static map in pm_input.c), the test budget mechanism (PM_ROW, 4 starts per row, exit 9, golden max 3 scenes, cached reruns, tour over 40 maps needs -Milestone).
Open queue (3ds/SLICES.md is the truth): FX2a to FX2g (running at the pause), C4F, F07-1 to F07-5, F06-1 to F06-6, C2R rest (63 battle stages, effects tour), one pre-S28 pass (full tour with -FaultStop and full golden, once, only when needed), H05 final-state steps, S28 (needs the user and the O3DS).
Rules to keep: one code agent per build tree. Set PM_ROW in every test shell. Golden only on touched scenes. Relay in 3 lines. No per-slice full tour or golden. No memory cap. H05 findings that need no user decision become rows at once.
Open questions for the user: O3DS has no ZR (N64 R loses a button), F11 decisions D1, D2, D4, D5, look at the effect sheets in 3ds/logs/fx, check the toad and yard golden references after V15 (updated without a look), S28 hardware readiness.

## 9. HANDOFF STATE (2026-10-19, written for a new orchestrator chat). Newest section. Sections 7 and 8 are obsolete.
Start here: read this section, then 3ds/AGENT_RULES.md, then 3ds/SLICES.md (the truth for open rows). Read 3ds/ORCHESTRATOR.md section 1 for the rules.
Running at the time of writing: V23 (pause menu party picture cut and map circles) agent aff30e9704f868bf5. After it reports, nothing runs. Check `git status` first: uncommitted files from older agents are line-ending noise (3ds/CMakeLists.txt moved pm_fxtest line, 3ds/src/pm_warp.c, 3ds/tools/fxtest_report.py, docs/plans/FX1_table.md).
How to work (user rules, keep them):
- Reply in ASD-STE100, no praise, relay each agent report in 3 lines or fewer. Do not answer late or duplicate agent notices.
- One code agent per build tree and only one using Azahar at a time (read-only agents may run in parallel but must not use Azahar). Group related rows into one agent. Do tiny edits inline.
- Set PM_ROW=<row> in every test shell. The budget (3ds/run_azahar.ps1) allows 8 Azahar starts per row. Only the orchestrator resets it (`powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>`), at most once per row. golden.py: 1 to 3 named scenes, --update runs the scene. No full tour or full golden per slice: only when a change needs them, and once before S28.
- Agent models: sonnet for code, haiku for mechanical work, opus only for plans. Never fable. Agents do not push. No Claude attribution in commits.
- H05 findings that need no user decision become rows and are worked at once. No per-slice memory cap (revisit only if the O3DS runs out).
- The classifier blocks edits of settings files (self-modification). The user added `.claude/settings.local.json` allow rules for go.ps1 and run_azahar.ps1 by hand.
Done this session (local commits, nothing pushed): F03, V06, V10, V12 to V12d, V13, V13b, V14 (no code change), V15 bilinear, V21 fire, V05b, B1 to B6, B3b, F08, L01, L01b, GP1, G01R, T01, T01b, T02, R05, R13 to R13d, FX1a, FX1b, FX2b, FX2d to FX2f, C2F to C2I, C3A to C3C, C4A to C4F, F07-1 to F07-6 (widescreen, switch `wide`), F06-1 to F06-4 (60 fps, switch `fps60`), F11-1 to F11-3 (options.txt and the Options panel, Select button), V22 (5 effect files added to the no-end list), H06a, H06c (one global rule -fno-toplevel-reorder, the per-file lists are gone, tool 3ds/tools/check_data_order.py), the test budget mechanism.
Open rows to work next, in order: V23 (finish), F06-5 and F06-6 (overload skip, last 60 fps slices; the fps60 pace in Azahar is 11 to 14 steps/s so F06-5 matters), F07-7, F11-4 (wire fps60 and wide to the menu rows, part of F06/F07), then group: V16+V17 (box around flickering sprites and star boxes, intro partial fade at f3626: leads in docs/plans/V16_diag.md and V17_V21_diag.md), V18 (road flicker on camera tilt: not reproduced, maybe hardware only), V19 (Sushie shadows, scene jan_04 demo, `demoscene=<n>` switch needs checked indices), V20 (nok_12 brick block, probably fixed by H06c, user must confirm), FX2a (fxtest flame), V22b (D8014EA48), H06a2 (is_host margin), H06a3 (warning flags), H06b (combiner and format coverage tour, one Azahar run), H06d (tour name cut iwa_0), C2R (63 battle stages and effects tour). Then the H05 final-state steps and one full tour plus golden if a change needs it. Then S28.
Needs the user: (1) look at effect sheets 3ds/logs/fx/sheet_1..11.png, (2) check the golden toad and yard pictures (re-saved by G01R), (3) confirm V20 and V18 and V19 on a test build, (4) the O3DS has no ZR, so N64 R loses a button (stopgap?), (5) F11 decisions D1 (place of the Options item, plan default taken), D2 (Select at any time or file select only), D4 (debug toggles), D5 (rebind scope), (6) S28: O3DS on at 192.168.1.168, dspfirm.cdc and papermario.z64 on the SD card, FTP app, Luma dump path.
Decisions of this session: F09 US only (languages from a PAL ROM later, no 25/50 Hz). F06 and F07 are toggles on both models, built before S28, default off, 4:3 at 30 fps default. Controls: D-pad, X (C-up), Y (C-down), C-stick (N3DS) = C buttons; L and R = Z; ZL and ZR (N3DS) = N64 L and R; Select opens the Options panel. Rebinding (F11-5) and Select at any time (F11-6) come after S28. The 3dsx holds code and code-side data only. The ROM papermario.z64 must be on the SD card at sdmc:/3ds/PaperMario/.
Build number (BN1): V0 + commit count - C0, constants in 3ds/build_base.txt, set by 3ds/build.sh, header 3ds/include/pm_build.h. Suffix -laneB only (no -dirty since 2026-10-19).

## 9a. V23 result (added at the pause)
V23 stays open (commits 3dcc56320, 1de1f7643). Map: G_TEXRECT now honours its tile argument (path dots tile 1). Not seen in a shot: new row V23b (debug switch for GF_MAP_* flags). Party portrait cut (about 30 of 150 px): NOT found. Ruled out: box panels, scissor, image 2, texrect uv, portrait bytes, palette. Suspect: GPU upload or sampling of the 256x16 CI8 texture, or the slot cache in pm_tex. Next step: one start that dumps the uploaded texel row in pm_tex load_inner (reset the V23 budget first). A user screenshot of the cut party screen would help. All agents are stopped at this point. Nothing runs.

## 9b. Notes for the new chat (added at the end)
- Nothing is running. No agent, no Azahar, no pending cron job. Closing the old chat kills nothing, because nothing is left. A cron job lives only in the chat that made it.
- V23 repro command and next start are in the V23 row in 3ds/SLICES.md (HANDOFF 2026-10-19 text at the end of the row). The party portrait is drawn in 12-row strips (tile 0 is 150x12): check the strip loop and the line offset between strips.
- The first thing to do in the new chat: `git status`, then `powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget V23` once, then relaunch V23 with a fresh sonnet agent.

## 10. HANDOFF STATE (2026-10-19 evening, written for a new orchestrator chat). Newest section. Section 9 is obsolete.
Start here: read this section, then docs/handoff/ORCH_RULES.txt (eleven rules), 3ds/AGENT_RULES.md, and 3ds/SLICES.md (truth for open rows). Check `git status` first. CLAUDE.md in the main folder is untracked on purpose. Nothing is running. No agent works. No Azahar window is open.

### User style (hard rules, the user has shouted about these)
- Rule 10: before an Agent call, look for a running or resumable agent. Use SendMessage when one exists. A row of about 30 lines or less never gets its own agent. The handoff is written by the orchestrator or the finishing agent, never by a new agent.
- Rule 11: never open with agreement or praise. State the action and the result. If the plan changed, say so in the first line.
- Decide small things yourself. Do not ask the user what you can work out. Explain things in plain words with the file link (markdown link) and what to look for.
- Test with the user together. The user plays in Azahar and reports. Open it for the user: `powershell -NoProfile -File 3ds/run_azahar.ps1 -Interactive` (run in the background). Lane B: `cd ..\papermario-3ds-port-laneB; $env:PM_LANE="laneB"; powershell -File 3ds/run_azahar.ps1 -Interactive`. Close Azahar before an agent needs the mutex.
- Do not read agent transcripts. Read 3ds/PROGRESS.log (last 12 lines) if a check is needed. The user dislikes questions about this.

### Done since section 9 (all local commits, nothing pushed)
V26 (Mario speed floats read as neighbours, `(&DefaultMoveSpeed)[-1]` in src/7B440.c), V25 (rect_2d blending by combiner alpha in pm_gpu.c), V24 (PAL_MEM byte swap for sprite palettes), V24b (poison tint check), V27 (clip_near at true near plane in pm_gbi.c, water surface), V20 (G_LOADTLUT updates the bound tile), V16 (hit flash palette, Goomba only), V23 (GPU scissor axis mirrored, pm_gpu.c flush), V17 (no fault), V22b, FX2a (flame glow, partial), FX3a (sheets at full size, 80-effect sweep), FX3b and FX3d (snowfall list fall-through), FX3c (effect 93, planerr 90), FX3e, F07-7 (400 column grab, overlays reach both edges), F07-8, F06-5, F06-6 (backgroundGfx 0x800), F11-4, C-O3DS (on the O3DS, X is Z; held with L or R it gives N64 L and R; options.txt `o3dsz=1` uses Y), L02 (log levels, switch `loglevel`, default warn, `log=off`), F11-8 (Options rows Logging and Log level), R-SEED (random seed per boot; switch `fixedseed` for tests; run_azahar.ps1 -Smoke and golden.py add it), H06a2 to H06a6 (sp9C = 0 in water_fountain.c accepted by the user), H06b1 to H06b3, H06b5, H06d, H06e, H06f, C2R, C2J, V23b (switch `mapflags`), BN1 and BS1 (below), rules 10 and 11.
Lane-b was merged into main at commit 07c1d7ab1. Goldens after the merge: battle 1.07%, toad 0.00%, yard 0.00%. No golden was updated for the title (lane B showed 37% because the old golden contains the ghost image) or for intro (11.2%, the user said the saved golden files are correct, keep them).

### Open rows
- V16b: white box around the enemy sprite while it is hit and flashes: Thunder Rage on Shy Guys, Parakarry 7 damage, jump on Monty Mole. The Goomba flash is fixed. Lane B agent a230bb016a984a094 had just started it (merge main into lane-b first). Check every sprite palette write, the flash path and sprite format. 
- V18: sanctuary walkway flicker at the end of the star spirit intro pan. Not seen in Azahar. The V27 near-plane fix may help. User still sees it in the merged build.
- V19: Sushie shadow flicker when she dives under the log in the jungle. Not seen in Azahar. Same state as V18. Possible shared cause: geometry flicker with camera angle (depth range, z-fight, clipping).
- V23b: the user sees two hand cursors on the map. The agent saw one.
- Fall-through fix: fire_breath D_09000B88_373F68 to gsSPBranchList(D_09000BA8_373F88) and peach_star_beam D_09005090_3EAFC0 to gsSPBranchList(D_090050F0_3EB020). Plan in docs/plans/fallthrough_sweep.md. Not started.
- FX3f: 14 effects show nothing in the shots (7, 9, 11, 13, 14, 30, 34, 36, 53, 100, 102, 116, 122, 126). Fall-through is ruled out. Check spawn arguments and life time against the 44 frame slot.
- F11-7: Options panel debug row "Fixed seed" (switch exists). F11-5 (rebinding) and F11-6 (Select at any time) come after S28.
- Not tested: O3DS shoulder map on hardware, Options panel A press on the Log level row, glow within 40 px of the screen edge in `wide`, darkness stencil at omo_14 against an older build, V16 on Parakarry and Monty Mole, golden battle after V20 in lane B (merged main golden passed), paralyze, static and dizzy palettes.
- Then: the H05 final steps, one full tour plus golden if a change needs them. Then S28 (the user sets up the O3DS when it is time. Do not start S28).

### Decisions of the user (keep)
F11 D1 plan default, D2 Select at any time, D4 choice A (four debug rows), D5 choice A (all 18 inputs). F07-7 option A (400 columns). O3DS Z key: the agent chose X (changeable by `o3dsz=1`). Fixed seed and logging become Options rows. Test budget raised from 4 to 8 starts per row. The intro golden stays as saved.

### Build and budget
- Build number = 797 + (commit count - 3172), constants in 3ds/build_base.txt (tracked). Suffix -laneB only (the -dirty suffix was removed by the user on 2026-10-19). The boot log, the bottom screen and the Options footer show it. HEAD had 3173 commits, so the next clean build prints 798. The old Docker counters are not used any more.
- Budget: 8 Azahar starts per row. The count is in 3ds/logs/test_budget.csv of each folder. Lane B has its own copy in papermario-3ds-port-laneB: run `-ResetBudget <row>` from that folder. Only the orchestrator resets, once per row (the user approved extra resets for V23, FX2a, V25).
- Docker commands in Git Bash need `MSYS_NO_PATHCONV=1`.
- Lane B is behind main after the merge. Merge main into lane-b before any lane B work. Lane B agent id a230bb016a984a094 can be resumed with SendMessage. The session rate limit stopped it once (reset 3:40pm Eastern). Container awesome_johnson is gone. Agent a78e72286b2383811 was stopped by the user (it had finished BN1, BS1, rules 10 and 11).

### Files for the user to look at
Effect sheets 3ds/logs/fx/sheet_1.png to sheet_11.png (checked, mostly correct). Golden pictures 3ds/golden/toad.png, yard.png, intro.png (checked, correct). Plain questions: docs/plans/user_questions_plain.md.

### Start prompt for the new chat
Read docs/ORCH_STATE.md section 10, then docs/handoff/ORCH_RULES.txt, 3ds/AGENT_RULES.md and 3ds/ORCHESTRATOR.md section 1. You are the orchestrator only. Reply in ASD-STE100 English, no praise, no agreement. Nothing runs now. Step 1: print the eleven rules in 11 short lines and one status block (RUNNING, READY-NOT-LAUNCHED with the conflict, BLOCKED-ON-USER). Step 2: resume agent a230bb016a984a094 with SendMessage (merge main into lane-b first) for V16b, V18, V19. Launch one main-tree code agent for the fall-through fix and FX3f. Ask the user to test with you when a merged build is ready. Do not start S28.

## 11. HANDOFF STATE (2026-10-20, written for a new orchestrator chat). Newest section. Section 10 is obsolete.
Start here: read this section, then docs/handoff/ORCH_RULES.txt (eleven rules), 3ds/AGENT_RULES.md, 3ds/SLICES.md (truth for open rows). Check `git status` first. CLAUDE.md in the main folder is untracked on purpose. All commits are local. Nothing is pushed. No agent runs. No Azahar window is open. The user runs the performance test on the O3DS next. Work is paused until the user sends the numbers.

### Build number
837 on the build made before commit c69750409 (BN1b). The next build prints 838. No suffix, not even for changed tracked files. Only `-laneB` stays (lane-b branch or PM_LANE set). The number is 797 + (commit count - 3172), constants in 3ds/build_base.txt.

### S28
The user booted the game on a console. S28 is done (marked in docs/SLICES_DONE.md). Optimization and F11-5, F11-6 rows are open.

### Done since section 10 (all local commits)
FALLTHRU (c07ea8805), FX3f (48e01329a), FX3g (a1e52dbac), FX3h (no code fault), F11-7 Fixed seed row (9cf0ff1c8), F11-9 Options stick dead zone and repeat (188abb388), F11-10 live apply of 60 FPS, Widescreen, No light (3b39d3d99), V24c palettes ok, DARK darkness stencil mirror fold in pm_gbi.c (23cf1cdf6), WIDEHUD wide HP label (5f43bb0a4), F11-5 and F11-6 Select menu with controls rows and options.txt `map.*` keys (11cc3fffa), PO2 vertex transform shared per vertex (27.9M vs 50.5M corners at f12000 preset=toad, speed not measured on O3DS), decalbias switch (default 0.001), AU1a (7022d34c6), PERF0 (merge bd39c87dc, lane-b through e0f697d45), B7-0 debug log lines, BN1b (c69750409), plans: V16b_azahar.md, V18_V19_azahar.md, V23c_azahar.md, open_rows_azahar.md, post_s28_optimization.md (7200e5852), post_s28_gaps.md (989261035). Goldens after the merge: toad 0.00%, yard 0.00%, battle 1.07%.

### Performance run (the user does this now)
Checklist: docs/plans/perf_run.md. One command: 3ds/tools/perf_capture.ps1. It runs A, B, C, D, F, E1, E2 and G (G is B7) and writes 3ds/logs/perf/baseline_<build>.txt. The parse and the writer were checked, not on a console. The baseline includes PO2. Runs A and C need `loglevel=debug`. The user sends the baseline file back.

### Open rows
- V23c: second hand cursor on the map. Likely cause: drop shadow flag in src/pause/pause_main.c:625 (+2,+2 dark copy, original game does it). Test not run: fps60 + wide + 3D, pause map, 2x shot. Plan docs/plans/V23c_azahar.md.
- V16b: white box on the hit enemy sprite. Not seen in Azahar. Suspects: 3ds/src/pm_tex.c:152 (CI texel to opaque white when tlut NULL), make_flash_palettes (src/battle/actor_rendering.c:2041-2058). Next step T3: log lines there, forced heavy flash on kmr Monty Mole. Plan docs/plans/V16b_azahar.md.
- V18 and V19: flicker. Azahar shows no difference for decalbias 0.001, 0.00003, 0. Device test: `-GameDebug decalbias=0.00003` in jan_04 (V19) and the star spirit intro pan (V18).
- AU1b to AU1d (audio to core 1). AU1b was reverted. AU1a stays. MA1 step 0 (`statehash=` switch in pm_warp.c), then MA1 sites. PO3 (needs golden.py --extra, PO3b). WIDEHUD2 (message box, pause text, Options in wide). Merge order is in docs/plans/post_s28_gaps.md.
- Questions for the user in 3ds/QUESTIONS.md: Q-F10 (endless script on tst_12), Q-PO2 (A do PO2, B skip, C decide after O3DS numbers, recommend A). Each MA1 logic change needs a Q-MA1-<site> yes.
- F09: no console test (US only). B7, V07 and V09: B7 needs the O3DS, V07 and V09 are shelved by the user.
- Not tested: B7 non-zero counters (debug battle has no swirl), F11-5 with real key presses and the bottom screen, V16c on Parakarry, glow exactly at the screen edge in wide, turning 60 FPS off again.

### Agents
All idle or stopped. Lane B agent a71be838a065f75fb is resumable (lane B at e0f697d45, merged into main). Main-tree agent adf26294238ad81da was stopped by the user on 2026-10-20. Resume agents by SendMessage. Read-only planner agents are finished.

### User style (hard rules)
Rules 10 and 11 in docs/handoff/ORCH_RULES.txt. Do not over-test: a text-only or number-only change gets a build check only. Golden runs only for renderer changes, on touched scenes. Never wait on an agent when the user said to do a step here. Close Azahar yourself when an agent needs the lock. Ask the user nothing that you can work out. Reply in ASD-STE100 English, no praise, no agreement.

### Start prompt for the new chat
Read docs/ORCH_STATE.md section 11, then docs/handoff/ORCH_RULES.txt, 3ds/AGENT_RULES.md and 3ds/ORCHESTRATOR.md section 1. You are the orchestrator only. Reply in ASD-STE100 English, no praise, no agreement. Nothing runs now. Step 1: print the eleven rules in 11 short lines and one status block. Step 2: ask the user for the O3DS baseline file (3ds/logs/perf/baseline_*.txt), the Q-PO2 and Q-F10 answers, and the console results for V16b, V18, V19. Step 3: when the baseline arrives, resume the main-tree work: V23c, V16b, AU1b to AU1d, MA1 step 0 by SendMessage to the right agents.

## 12. STATE UPDATE (2026-10-20). Newest section. Section 11 stays as written.

### User decisions 2026-10-20
- H05 and V17 are done. Their rows moved to docs/SLICES_DONE.md.
- H06a, H06b and H06d belong to the optimization work. Their rows in 3ds/SLICES.md carry the tag.
- V19b is later.
- The O3DS baseline is dropped. The console Wi-Fi is flaky and the push failed.
- The baseline now comes from Azahar in the O3DS config: 3ds/logs/perf/baseline_azahar_840.txt. It covers the full intro and demo cycle, 8232 frames. Pre-PO2 and current: all counters equal. PO2 transforms about 55 percent of the old vertex work. Host time changes by -0.7 percent of wall time.

### PERF-BASE tooling (commits d7bfe2d9c, 00b89ecc6, e021d8ce4, f19883e59)
- push_3ds.sh builds only when stale. It has a send timeout and prints PUSH OK or PUSH FAILED.
- run_azahar.sh runs Azahar as a flatpak in fast mode (-F).
- perf_az_full.py writes the per segment table.

### Status of optimization rows
| Row | Status | Commit | Note |
|---|---|---|---|
| PO2 | DONE 2026-10-20 | see docs/SLICES_DONE.md | 12.98M vertices transformed vs 23.81M corners read |
| AU1b to AU1d | DONE | 5ae29eac3, e02051bfa | Audio ticks on a core 1 thread. Switch cpulimit=<5..80>, default 30. PCM hash equals the AU1b reference. |
| AU1e | OPEN, needs the user | none | O3DS test at cpulimit 30, 50, 80 |
| MA1 step 0 | DONE | e52bf5929 | Switch statehash=<n> (FNV-1a, default every 10 steps). 858 samples, 0 differences in 2 cycles. |
| PO3a | DONE | 5a98914d5 | Log 3ds/logs/perf/po3a_azahar_845.txt. Next step: the PO3a gates (user instruction). |
| PO3b | WAITS | none | Needs an O3DS GBI-interpret number after PO2. User decision pending. |

AU1 numbers (Azahar): at cpulimit=80, core 0 audio time is 8.68 to 0.74 ms per step. Audio posts 21120 against 22200 (wall ticks). Default cpulimit 30 is slower than the overlap on Azahar. Not tested: crashtest_audio on core 1, warp=nok_01:0.
Benchmark runs use cpulimit=80. With cpulimit=30 the game timeline differs from the baseline timeline.

PO3a numbers (Azahar): mixed matrix ids 0.0 percent. 20.7 distinct matrix ids per frame against 136 CPU draws per frame. 85.8 percent plain triangles. The CPU rejects 63.7 percent of triangle commands.

MA1 open questions in 3ds/QUESTIONS.md: Q-MA1-D8, Q-MA1-D9, Q-MA1-D10, Q-MA1-D11-D12, P16.

### Running now
- Lane B row MA1-R: MA1 render sites P13 to P18 and P20.
- Read-only audit H06a: docs/plans/H06a.md.

### Rules that stay
- Do not start S28.
- Do not push.

## 13. HANDOFF TO ANOTHER MACHINE (2026-10-22). Newest section.
Read docs/handoff/HANDOFF_2026-10-22.md first (state, trees, builds, consoles, user rules, performance, PT table, pending requests, resume steps, START PROMPT file docs/handoff/START_PROMPT_2026-10-22.txt). Rules 1 to 16: docs/handoff/ORCH_RULES.txt.
Everything is local (nothing pushed): the package /home/andrew/papermario_handoff_2026-10-22/ (all.bundle, files archive, memory, README.txt) carries it. Lane C holds the unmerged SWEEP commit ffc2b722a. The user file 3ds/src/map_restore_table.c stays uncommitted (patch in docs/handoff/).
Build 1015 is on both consoles (O3DS .180, N3DS .181), pushed with Push PLAY. Next: FTP pull of both logs. New rows: PT67a to PT67d in 3ds/SLICES.md.


## 14. HANDOFF 2026-10-23. Newest section.
Read docs/handoff/HANDOFF_2026-10-23.md first, then docs/handoff/START_PROMPT_2026-10-23.txt. Everything is local. No agent runs. CRASH83 fix (src/evt/map_api.c) is written but not built or tested. The compiler work (docs/plans/full_gpu_path.md) has not started.
