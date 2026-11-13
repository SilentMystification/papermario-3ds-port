# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-13, main 061d75ad0 and later doc commits. Main is clean. No agent runs (the user ordered STOP ALL WORK; both
listed subagents are completed). Nothing starts until the user says "start".
Read first, in this order: this file, orchestrator_rules.md (its Core block is printed every turn by the hook), CLAUDE.md (39 lines),
plan_endgame.md (v3, accepted). Then .claude/agents/oracle.md and coder.md (the new process, committed in a80c08261 and 061d75ad0).

## Goal and done
Remove the display list system completely. Done only when `3ds/tools/elfgate.py` prints `func 0 data 0 gfxnames 0` and all LISTCEN2
counters C1..C12 are 0. Never say done before. Print the counters only when the user asks or a merge changes them.
FACT (last LISTCEN2 run): C1 3612 (start 3820), C2 3042 (3254), C3 272, C4 280, C5 22103 (unchanged), C6 2108, C7 2845, C8 732, C9 15,
C10 361. ELF gate last measured func 50, data 4152, gfxnames 1806 (no delete row merged since).
FACT (codemod dry run over src, 240 files with calls): value 1605 (44 %), state 1515 (41 %), dl 483 (13 %), keep 81. State calls:
899 sit in root files (hud, model, draw_img). Converter: 922 of 1372 static lists convert, 426 inherit caller state (ENTRYCAP),
24 unsupported. So about 57 % of the call sites are mechanical; the state calls need a designed idiom component first (GUESS: the long pole).

## Why the process changed (user stop, 2026-11-13)
One-file rows cost 20 to 57 min and removed 6 to 10 sites each. About 12 of 24 rows stopped on row-text faults. The old oracle used 1521 calls and
717M cache-read tokens. Coders polled with ping and sleep loops. FACTs measured: coder start 28.3K tokens; a tools line does not cut it; a coder run
is 158 to 305 calls and 16M to 32M cache-read tokens; the harness backgrounds any command after 180 s; the completion notice arrives about 0 s after
the job ends (14 of 14 jobs); subagent cache TTL is 5 min. New design: 3 workstreams, one Haiku coder each, host unit tests as the main proof,
emulator once per group, the coder asks the oracle directly (SendMessage, untested), fresh oracle from oracle_handoff.md past 150K context.
None of the new rules has run yet. Whether Haiku can run a whole directory workstream is OPEN.

## Next steps (in order, after the user says start)
1. Start a fresh Opus oracle (about 12K start). Tasks: (a) write docs/handoff/oracle_handoff.md (60 lines or fewer) from plan_endgame.md;
   (b) HUNIT goal page: one host test per codemod value call shape (macro stream vs native record through the walker and nd_r11_walk, seeded
   fault). FACT: no test file references nd_val_prim, nd_val_env, nd_xf_push or nd_val_seg today. (c) a design task for the state-call idioms.
2. Goal pages ws_A (value calls C1/C2, codemod on whole directories), ws_B (static data C5/C6: CONV2F, ENTRYCAP, then groups; memcheck margin
   310 KB or more), ws_C (N64 branches C3/C4, then state calls, readers, recdl). Each 40 lines or fewer with one before and after example per edit shape.
3. Pilot: one Haiku coder on one workstream. Measure sites removed, tokens, minutes, stops (transcript timing script). Stop test (GUESS): after 3
   merged groups C1 must fall by 200 or more, else stop and ask the user.
4. Then up to 3 workstreams plus 1 tools coder. At most 4 agents at once. A Sonnet or Opus coder needs the user's yes.

## Unmerged work (worktrees build3ds/wt/<id>; ahead = commits over main; dirty = uncommitted entries)
- CODEMODW1: ahead 5, dirty 2. Fix of the water: nd_val.c records its typed unit with nd_rec_state (cause: nd_val_prim records an NDC_NAPI header
  that nd_r11_walk, nd_model.c:296 `default: return 0;`, rejects in model custom gfx ranges). Cause is a code read, not confirmed by a test. Gate not run:
  base fxpic_105, seed blue 128 to 64, spot_mac00w_det and fxpic_105 byte-equal.
- N64DEL2: ahead 1, dirty 56 (55 edited files, 613 deletions uncommitted). Gate not run. 14 .inc.c compare failures seen; cut the gate to changed files.
- ENTRYCAP: ahead 2, dirty 2. CONV2F: ahead 2, dirty 2. CONV2MIX: ahead 1, dirty 1. SEEDPROBE1: ahead 1, dirty 8 (cancelled chain, delete it).
- BOTHUD: ahead 1, dirty 1, ON HOLD (needs the user's answer). FPS60FL: ahead 0, dirty 3. FSELLAG: ahead 0, dirty 1 (file select flip, console check owed).
- Rows CODEMODB1-B11 and CODEMODW1-W4 exist; the workstream design supersedes the B batch rows.
Dirty worktrees: a coder or the oracle reads each before any merge. Never merge without a gate.

## Console and the 60 fps freeze (user fact, still open)
Build #5018 with 60 fps on froze the O3DS .170 in Toad Town (mac_01). The game thread waits in phase 7 (gpu end), step 555, pm_gpu_end_w:2986.
FPS60W2 fixed one defect and did not remove the freeze. Open path: the key frame queues the L frame's fills and VRAM copies again (pm_gpu.c:2946-2955).
The oracle reads what the two GX queue entries contain. Details: row_FPS60W.md, memory project_fps60-freeze-open. FPS60FL is a branch on this.
Console owed by the user: AMBTITLE, AMBWARP, FSELLAG1/2, the v2 runner (CONV2D2). GOLDWLD0 needs the user's fortune scene.
Consoles: O3DS XL 192.168.1.131, N3DS .224, O3DS .170. Push only on the user's word: `powershell -NoProfile -File 3ds/go.ps1 push o -ip <ip> -Interactive -Profile prof warp=mac_01:0 pos=0:0:200`
in the background, build first with plain `sh 3ds/build.sh`. Ask the console with a multi-select (memory feedback_push-selector).
After a freeze: power cycle, FTP on the console, `go.ps1 pull o -ip <ip>`; dumps with 3ds/tools/decode_dump.py.

## Tools and facts that cost time
- Hook no_poll_hook.js (PreToolUse, Bash) blocks sleep, ping -n/-c, tail --pid, timeout /t, Start-Sleep for all agents. FACT: a coder test (HOOKTEST) was blocked. OPEN: a wait hidden inside a script file; coder SendMessage to a live oracle.
- Merge: `sh 3ds/tools/merge_row.sh <id> <msgfile>` (main must be clean; steps in orchestrator_rules.md). Rebuild main in the background.
- runs.jsonl: a worktree copy can be huge: `python -I 3ds/tools/dedupe_runs.py <file>`, commit with `git add -f`.
- Lane build (PM_LANE=<row>) writes build3ds/lane-<row>/, not build3ds/pm_3ds.3dsx. Build main plain before a push.
- The HBL netloader accepts one connection and stops listening after minutes. Never test port 17491. Never build between the question and the push.
- Hook no_guess_hook.js blocks messages with guess-and-check phrases (for example "log line"): describe a read, not a run.
- Memcheck: static 21668 KB, margin 318 KB (CONV2D2). Tables grow it.
- Repo repair: local history was rewritten (blobs over 100 MB removed). The push to GitHub is the user's action. origin/main is an ancestor of main.
  Backups: C:\Users\Andrew\Documents\papermario-3ds-port-backup-2026-11-12.git and the old-branches bundle. refs/original/* stay until the user
  confirms the push. Before a push, check for no blob over 100 MB (`git rev-list --objects <branch> --not --remotes=origin | git cat-file --batch-check`).

## User communication rules (memory feedback files hold the why)
No flattery, no apologies. ASD-STE100 in every word. Label numbers FACT (source), GUESS or OPEN. First check the facts, then ask only a decision.
Take agent states from ListAgents each turn. Edit rule files in place, never append. Table only when a state changed. A user-reported bug is fact.
Never: toggles, stubs, git push, git stash, a build or push in the foreground of the chat. Never launch an agent when the user says not to.
