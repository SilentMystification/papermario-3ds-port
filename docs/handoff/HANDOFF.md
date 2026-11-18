# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-17 19:53, main 0f39de90cb (plus docs commits). The user ended this orchestrator session. A new session starts here.
Start: read this file, docs/handoff/orchestrator_rules.md (its CORE block is printed every turn by a hook), CLAUDE.md, docs/handoff/push_runbook.md and the
memory index. Then `date`, ListAgents (status `running` is the ONLY slot count), `sh 3ds/tools/postmerge_bg.sh --status`. No agent was stopped: two coders run.

## Waiting on the user (do these first, in this order)
1. The build of main 0f39de90cb is READY (binchk OK, build key fe17e50caa46 matches, 3dsx built 19:49). It holds HWFIX1 (the rectangle fix) and the 60 FPS row.
   Write ONE line: `Build 0f39de90cb ready. Say the console.` On the user's word push at once: `powershell -NoProfile -File 3ds/go.ps1 push o -ip 192.168.1.170 -NoBuild -Seconds 75`
   (O3DS .170; n .224; XL .131 needs -ip). The auto-mode classifier denies a push without the user's word: the last orchestrator tried and was denied. Do not retry it.
2. Console check list after that push: docs/handoff/verify_list.md (intro stage, shadows, walk dust, heart box, signs, Z spin, start menu grid and menu switch,
   kkj_12 Mario, top screen HUD yellow, 60 FPS row in General tab).
3. Option rows: the user said some options in the select menu change nothing. The code read found a reader for every row. Ask the user which rows and whether
   Logging was off. HUD, Stats and Position HUD draw only on the bottom screen: ws_BOTSCR fixes them.

## Failures of 2026-11-17 and the control for each (the user's anger: read this)
1. Status report asked 3 times: hand-backs were handled first. Control: CORE item 0 (read ALL user messages of the turn, answer in this reply).
2. Console push failed 4 times: the netloader broadcast probe does not reach the consoles from this PC; the old error text blamed the console; the skill command
   builds and has no -ip. Control: push_runbook.md, go.ps1 uses the known address, the probe text is fixed. The user's word "ready" is fact.
3. Asked to push before the build was ready, then merged between the ask and the push. Control: runbook order. No AskUserQuestion popup for a push (the user
   rejects it): one plain line, then push on the user's word.
4. Ready work was held "until the console confirms": a coder sat idle. Control: CORE item 3 (queue). A merge may wait for a gate, the work never waits.
5. Double posts: the Stop hook rejects sentences over 20 words and shows the user both drafts. Control: short sentences in the FIRST draft, one reply per turn.
6. Oracles stopped at 150K: that rule is dead. Limit 500K, past 250K new work goes to a new agent (orchestrator_rules.md Agents).
7. A ScheduleWakeup did not fire for 2 hours (16:02 to 18:09). Control: read `date` every turn, set a 10 minute wakeup.
8. A design risk was named and then shipped: D1 (PSOFB fixed fallback) drew white and black rectangles on the console. Control: CORE item 8.

## Queue rule and process facts
Every ready page gets a coder at once (3 coders, 2 oracles running). Work runs on branches. A chain row starts from the branch of the row before it.
Coders cannot wait for an oracle reply: they end their turn. Resume them with the reply and tell them to finish. Merge with `sh 3ds/tools/merge_row.sh <row>`;
if the oracle edited the page on main and the branch deletes it, `git rm` the page on main first. Main must be clean: oracles leave untracked pages for
minutes (commit them or retry). The post-merge check (cold+shots) runs on every queued job while postmerge_FAIL.txt exists: purge old jobs in
build3ds/out/postmerge_q and keep the newest. Build main as ONE background job run directly (not in a subshell with `&`: it dies with the shell).

## Goal and counters (FACT, listcen at 19:52 on main 0f39de90cb)
Goal: remove the display list system. Done only when elfgate prints `func 0 data 0 gfxnames 0` and all listcen totals are 0.
Now: build 308, gfxpos 0, data 191, data_arrays 0, gfxtype 251, recdl 0, reader 769, model 15.
Session start (002498da73): 324, 22, 323, 9, 420, 11, 792, 15. 33 rows merged. model is back at 15 because HWFIX1 moved nd_decide.c back to 3ds/src (PSOFB had moved it).
build and data are the gbi.h definitions, kept until the walker is deleted (decision 4D option B). reader and model are the walker: plan_walker_delete.md
(stages S0-S8) and design_nstate.md (S2) hold the plan.
User decisions: D1 option A (fixed fallback: HWFIX1 reverted it to nd_comb_core on a miss; revisit with the S1 data), D2 option A (delete the walk census with the
walker), UD1 class core1 outside the end proof, UD2 core 0 keeps a native transform stack (orchestrator choice), UD3 one prof push at the NST1 gate (OPEN: needs a
push with `-Profile prof -GameDebug "warp=mac_01:0 pos=0:0:200"`, the bottom screen log is off then), Sonnet yes for NST1 (done), NDR0, NCONV1, NREC1.

## Console reports (O3DS .170, build 6449 = main 60182a2edd) and their rows
- White and black rectangles, grey HUD, dim Mario, black menu part: PSOFB fallback. HWFIX1 merged (0f39de90cb). Needs the user's console check.
- 60 FPS row missing: commit 1e7ddd7ea1 (before the session). ws_FPS60MENU merged (c56518f604).
- Bottom screen blank when Logging is off: pm_log.c:905 (LOG4 0c1f761665). BOTSCR is DONE (branch BOTSCR, bb7520d044, LOOP PASS, host botscr ok). Merge it AFTER the push of build 0f39de90cb: a merge changes pm_log.c and makes that built 3dsx stale (push says STALE 3DSX). Then build main again for the next push.
- Option rows that change nothing: no cause by code read (see Waiting on the user, item 3).
- The first push of build 6449 booted fine; the [CRASH] blocks in its log are an old crash.txt replay (old build numbers), not a new crash.

## Agents at handoff (ListAgents is the truth)
Coders: BOTSCR a22eb8d737b53293f is finished and waits for its merge (oracle NEXT3 a26f678cb67ee83db, 237K). Running: NST2A ae0efb252e4b0141c (relaunched on its branch from the corrected page,
budget reset; oracle NSTO a4979ad6150eacb9f, 380K; hold its MERGE until the console check, the work runs).
Oracles (all idle, resume by SendMessage): NEXT3 a26f678cb67ee83db, NSTO a4979ad6150eacb9f, PHASEE2 a2895cb81ea2b910a (360K), DIAG1 a19976ad4fb0c998a (205K).
Pages on main: ws_BOTSCR, ws_NST2B to ws_NST2F (chain after NST2A: the metric is fbad per tst1 member; start each from the branch of the row before).
NDR0 is deferred until the NST2F data exists (oracle advice). docs/handoff/oracle_handoff.md holds the oracle facts (keep it at 60 lines or less).

## Open faults
- PSOFB psomiss: one draw has tex_on 2167585 (a bad flag value); the writer is not found (oracle_handoff.md item 2). Write a page after the console check.
- The hotset line reports a stale `*(.text.nd_gx_w)` at 3ds/ld/hot_order.ld:176 (defined at 3ds/src/nd_rec.c:485). Not caused by a row. A tool or a one-line fix.
- The host test cache fails after some edits (/tmp/ndcombs.c): use PM_HOSTSEL=all PM_CACHE=off. The hostsel store rename fault is fixed (802dee1b9b).
- The user's skill command for a push (`go.ps1 push o -Seconds 75`) now works as written (go.ps1 adds the address) but still builds if the 3dsx is stale.
