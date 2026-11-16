# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-14, main cdba8653d. Main is clean. No coder runs. The user asked for a stopping point and a process audit; both are done
(docs/handoff/audit_process.md, fixes applied in cdba8653d). Start by reading: this file, orchestrator_rules.md (its Core block is printed
every turn by the hook), CLAUDE.md. Then ListAgents, then the free pages below.

## Goal and done
Remove the display list system completely. Done only when `3ds/tools/elfgate.py build3ds/pm_3ds.elf` prints `func 0 data 0 gfxnames 0` and all
LISTCEN2 counters C1..C12 are 0. Never say done before. Print counters only when the user asks.
FACT (listcen and elfgate, 2026-11-14, main 139abd3c3 build): C1 846 (start of the 2026-11-13 session 3612), C2 315 (3042), C3 0 (272), C4 0 (280),
C5 data 19190 (22103), C6 arrays 1778 (2108), C7 gfxtype 2493 (2845), C8 readers 766 (732), C9 model 15, C10 recdl 42 (361).
ELF gate: func 48 (50), data 3260 (4152), gfxnames 1496 (1806). Memcheck margin 358 KB (the real limit is a margin above 0 KB, soft).

## How the work runs now (user decisions 2026-11-13 and 14)
- Haiku coders get closed chunks (exact files, edit shape with a real example, gate, stop condition, no open question). The oracle (Opus) owns the design.
- The oracle gets a short brief in its prompt (task, merges since its last run, files to check). It does not read the process files. FACT: the audit run cost 51K tokens, earlier runs 160K to 350K.
- Start an oracle only when no closed page exists for a free slot. A coder asks the oracle that wrote its page (its id is in the launch prompt), at most 2 rounds per cause, commits and ends its turn, the reply wakes it.
- A coder hands back only through build3ds/out/handback_<id>.txt (first 8 lines) and one short last line. The facts hook shows the file. The orchestrator merges with `sh 3ds/tools/merge_row.sh <branch> <msgfile>`.
- Gates: exact byte compare, base = `git merge-base HEAD main`, memcheck margin above 0 KB. No made-up gate numbers.
- Hooks: orch_facts.js (facts from disk each turn, and a Stop hook), orchestrator_rules.md Core (567 characters), no_poll_hook.js, no_guess_hook.js.
- FACT (2026-11-13 17:00 to 2026-11-14 05:00): 43 merges in 11 h 41 min, coders about 2.9 M tokens, oracles about 2.3 M (GUESS: sums of the last notice values).

## Next steps (in order)
1. Free pages, both unblocked: ws_FXENT2.md (the 42 kept effect sites, after NSTREAM2: merged) and ws_GSET3.md (entity lists, after GSET2: merged). A coder starts each with the oracle id that wrote it.
2. Oracle tasks (brief them): NSTREAM3 (core 0 local buffers in hud_element.c, msg_draw.c, sprite.c, screen_overlays.c, nd_direct.c; its page waited for TEX4: merged);
   GCONVW (world, model.c, battle, pause and UI lists are not in the converter set: design_lookahead.md section 5); the stage rows of phase E with their triggers (design_lookahead.md sections 7 and 8).
3. Open facts: nd_gfx_conv.c:800 has_ent is false after the twin move (GSET2 left 22 lists with no device entry out); src/sprite.c:315-336 older setters record from core 0 (unchecked);
   spiky_white_aura.c:8 extern Gfx* stays. The decision for a draw with no pso table entry after S9DEL2A: a fixed fallback mode with a log line (user, 2026-11-14).
4. Phase E (deletes) waits for C1, C2, C10 at 0 and the NSTREAM stages (design_lookahead.md section 7). Before S9DEL1A tag main `pre-list-removal`.

## Parked branches (nothing deleted; each waits on the user)
BOTHUD (ahead 1, ON HOLD, needs the user's answer), CONV2MIX (ahead 1, the plan calls MIX closed), FPS60FL (ahead 1, the 60 fps freeze), FSELLAG (ahead 0, file select flip, console check owed),
SEEDPROBE1 (ahead 2, cancelled seed-probe chain, superseded by host unit tests; the user said keep unless resumed).

## Console and the 60 fps freeze (user fact, still open)
Build #5018 with 60 fps on froze the O3DS .170 in Toad Town (mac_01). The game thread waits in phase 7 (gpu end), step 555, pm_gpu_end_w:2986.
FPS60W2 fixed one defect and did not remove the freeze. Open path: the key frame queues the L frame's fills and VRAM copies again (pm_gpu.c:2946-2955).
Details: memory project_fps60-freeze-open. Console owed by the user: AMBTITLE, AMBWARP, FSELLAG1/2, the v2 runner. GOLDWLD0 needs the user's fortune scene.
Consoles: O3DS XL 192.168.1.131, N3DS .224, O3DS .170. Push only on the user's word: `powershell -NoProfile -File 3ds/go.ps1 push o -ip <ip> -Interactive -Profile prof warp=mac_01:0 pos=0:0:200`
in the background, build first with plain `sh 3ds/build.sh`. Ask the console with a multi-select (memory feedback_push-selector).
After a freeze: power cycle, FTP on the console, `go.ps1 pull o -ip <ip>`; dumps with 3ds/tools/decode_dump.py. Nothing of this session was run on a console (OPEN).

## Tools and facts that cost time
- A coder's command that runs over about 180 s goes to the background and the coder ends its turn; the completion notice wakes it. Interim notices are normal.
- merge_row.sh refuses a dirty main: the oracle writes pages into main while it runs, so commit its files first (git add docs/handoff) and merge in the same command. INDEX.md is gone.
- runs.jsonl: a worktree copy can be huge: `python -I 3ds/tools/dedupe_runs.py <file>`, commit with `git add -f`. A coder's worktree on a branch with another name than its id: pass the branch name to merge_row.
- Lane build (PM_LANE=<row>) writes build3ds/lane-<row>/, not build3ds/pm_3ds.3dsx. Build main plain before a push, in the background.
- The HBL netloader accepts one connection and stops listening after minutes. Never test port 17491. Never build between the question and the push.
- Repo repair: local history was rewritten (blobs over 100 MB removed). The push to GitHub is the user's action. origin/main is an ancestor of main.
  Backups: C:\Users\Andrew\Documents\papermario-3ds-port-backup-2026-11-12.git and the old-branches bundle. Before a push, check for no blob over 100 MB.

## User communication rules (memory feedback files hold the why)
No flattery, no apologies. ASD-STE100 in every word. Label numbers FACT (source), GUESS or OPEN. First check the facts, then ask only a decision. Object first when a directive looks wrong.
Take agent states from ListAgents each turn. Edit rule files in place, never append. Table only when a state changed. A user-reported bug is fact. Never invent a rule or a gate number.
Never: toggles, stubs, git push, git stash, a build or push in the foreground of the chat. Never launch an agent when the user says not to.
