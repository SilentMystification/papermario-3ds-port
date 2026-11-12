# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-12 about 16:30, main 3148af0a9 or later. Read first, in this order: this file, orchestrator_rules.md, CLAUDE.md,
plan_endgame.md (v2), test_catalog.md. The oracle (Opus, agent a113b2781e2728d61, ORACLE7) is idle with about 540K tokens; per the
rule of 2026-11-12 it is NOT replaced on a token count (only when forced or for a new work area). No coder is running; worktrees of
finished or stopped rows still exist (see "Open rows").

## The goal and the one rule
Remove the display list system completely: no list builders, readers, walker state model, N64 list words (orchestrator_rules.md
"Definition of done"). The epic is NOT DONE until the old system is gone from the source or proven absent from the final 3ds
executable: `3ds/tools/elfgate.py` (row ELFGATE, not written yet) prints `func 0 data 0 gfxnames 0` and all source counters
(LISTCEN2) are 0. Nobody says "done" before that; every status starts with the line "Display list code remaining" in counts.
Census at main 235bb8721: 3820 active builder calls (+648 N64-only), 3254 gMainGfxPos uses (+715), 22103 list data macros in 2107 Gfx
arrays, 714 reader hits in 22 3ds/src modules, 15 state-model functions. The user has wanted this for weeks and was told "done" at
stage ends while the list code stayed; the stage gates only measured their own rows.

## Where the plan stands (user decision pending)
plan_endgame.md v2 (100 rows: Haiku 71, Sonnet 27, Opus 2; 45-60 h honest) is written but NOT accepted. The user answered "Changes
needed" with these instructions, which come first:
1. Write this handoff and update all processes from this session (done in the same commit as this file).
2. The next session works in PARALLEL but without merge conflicts: many agents may edit the same file, never the same area of code.
   No extra work for conflict handling. See "Parallel work without merge conflicts" below. The first rows of the next session must
   remove the shared registries that cause conflicts (rows REGDIR1-3, to be written by the oracle) before many rows run together.
3. After that the plan v2 is shown again for acceptance. Opus for NPSO and CONV2 is approved. The exact-compare rule is in
   (CLAUDE.md 4: one differing bit fails, no rounding exception). The new CLAUDE.md rule 2 wording (golden draw records and frame
   hash, no shot compare) is confirmed by the user but NOT yet edited: edit it when the golden-master harness (HOSTCORE, GMHARN,
   GMCOV) and FRAMEHASH exist. Until then bench.py cmp byte compares stay the only runtime gate.
The user asked for testing that is "efficient and effective" and not screenshot based: row_TESTIDEAS.md holds 12 gate ideas and the
unit / golden-master evaluation (about 73 % of the 3356 call sites can run as host golden-master tests; the large groups model,
HUD, messages, imgfx, sprites, item entities need psochk plus frame hash in scenes that the coverage map shows run their sites).

## Open rows (branches and worktrees in build3ds/wt)
Passed their gates, not merged (the user paused all merges until the plan is accepted; merge them with sh 3ds/tools/merge_row.sh):
S1TSTEN 71fc0c37c, S7ENT cc55b8a38, S7EFX7 branch tip 5f7584b1a, S7EFX8 branch tip ff04c4481. Their S7/S6 table forms are deleted again
by S9DEL5; the user decides whether they still merge.
Stopped: S7FLAME (real fix 83a09c842 without stub; byte gate failed on fxpic_27: 1205 px, x 100..162 y 182..216; the plan closes
the row), S6E3 (code 52687d1a7, base queue), S7TEX2 (no edit; the oracle rewrote it as option b; the plan closes it), FRAMEHASH
(worktree only, row written, on hold). Old worktrees HC5-8, NAPICEN2, PBATCH2, PBFLUSH and .claude/worktrees/agent-* are leftovers:
run `sh 3ds/tools/wt_gc.sh --apply` after checking they are merged or closed.

## Console and the 60 fps bug (user fact)
O3DS (.170) got the main build at about 15:55 (push OK, no-progress limit worked). Turning on 60 fps mode then froze the console
(power cycle needed), either at the toggle or frames later. The live log was cut mid-line; FTP on the .170 times out. The oracle's
trace: stuck GPU; the paths that only run at 60 fps and never ran on hardware are the key frame copy shown again (pm_gpu.c:2936-2963,
C3D_FrameBegin(0) result at :2941 unchecked), the twin patch words (kc_build :2917-2927) and the key walk fallback (pm_nusys.c:209-222).
It cannot pick one. NEXT STEP: the user power cycles the .170 and boots the game once (START+SELECT 1 s if it freezes), then pull
crash.txt by FTP (`powershell -NoProfile -File 3ds/go.ps1 pull o -ip 192.168.1.170`; 3ds/tools/decode_dump.py for Luma dumps) and
give it to the oracle for one fix row. options.ini may hold fps60=1 and hang the next boot too.
The XL (.131) panic of the session start was the Homebrew Launcher netloader killed or starved while taking the 51 MB 3dsx, not the
game. The 3dsx is 38 MB now (S7MAP2); the S7MAP .dlc sidecars were 18 MB.

## Tools and what broke (facts and fixes; details in git log)
- runs.jsonl: old merge_row.sh doubled repeated note lines at every merge: 3.5 GB, lock waits of 900 s. Fixed: TOOLB4 (merge_row
  appends a note only if main lacks it; resolves a runs.jsonl conflict with main's version), TOOLDEDUP (main file 7.5 MB, user approved),
  lock wait 900 s. A worktree copy of runs.jsonl can still be huge: run `python -I 3ds/tools/dedupe_runs.py 3ds/logs/perf/runs.jsonl`
  in that worktree (keeps every run_id line and the first copy of each other line), commit it with `git add -f`.
- Base runs: the two runs of a base must run on one slot (TOOLB4, TOOLB5: the queue grants the oldest entry for slot k first).
  A base is keyed by commit; main moves with every merge, so every lane builds its own base (10+ min, one build holds a lock):
  this was the largest time sink. The planned gates (golden masters, hashes) remove base builds.
- Push: PUSHFIX (go.ps1 prints push_3ds.sh output; --no-build refuses a stale 3dsx; detached start with PUSH STARTED / PUSH END and
  build3ds/out/push_last.txt) and PUSHFIX2 (a started upload is killed only after NOGROW_SECS=60 without output growth or at a size
  cap 15 s per MB, min 120 s; the old fixed 60 s kill cut every 38-51 MB send at a random percentage and likely crashed the netloader).
  OPEN TOOL FAULT (row not written): the stale check compares the build's git hash with HEAD, so any commit (even docs) makes a
  correct 3dsx stale; `go.ps1 push` without -NoBuild works (build.sh is a 24 s no-op when the content key is unchanged). The check
  must use build.sh's content key. A lane build (PM_LANE=main) writes build3ds/lane-main/, NOT build3ds/pm_3ds.3dsx: build main
  with plain `sh 3ds/build.sh` before a push. The HBL netloader accepts one connection and gives up after minutes: never test
  port 17491 before a push and never build between asking the user to open it and the push; check pings only.
- gen_gfx_tab.sh: every S7EFX/S7ENT/S7MAP row edited one line next to the others: a merge conflict at nearly every merge (coders had
  to merge main and resolve). Shell scripts must stay LF (.gitattributes 3ds/**/*.sh eol=lf).
- merge_row.sh needs a clean main (commit docs with `git add -A -f docs/handoff 3ds/logs/perf/pushed_builds.txt` first). When it
  merges a branch that changes merge_row.sh itself, run a copy of the script. Worktree removal can fail while a shell sits in it.

## Parallel work without merge conflicts (user order, 2026-11-12)
Rule: several agents may work in one file, never in the same area of code. A merge conflict is a planning fault and costs a coder
an hour. How:
1. Every row names its PLACES: file plus function or region names it edits (oracle.md). The orchestrator keeps the table of open rows
   and their places (docs/handoff/places.md, edited in place) and launches a row only if no open row owns one of its places; a
   rename, move or reformat of a shared region makes the later row wait.
2. Shared registries are not edited by rows: per-row files instead. REGDIR rows (first of the next session): (a) GCONV_SRC lines of
   3ds/tools/gen_gfx_tab.sh become one file per row, 3ds/tools/gconv_src.d/<row>.txt, read by the script; (b) scenarios and benches
   (azt_scenarios.json, bench_defs.json) become per-row files 3ds/tools/azt.d/<row>.json and bench.d/<row>.json merged by the tools at
   load; (c) pushed_builds.txt and runs.jsonl are written by the tools only and merge_row resolves them; (d) build.sh want= lists and
   CMake source lists use globs or per-directory files. Until REGDIR is merged, rows that must touch a registry run one at a time.
3. Coders do not merge main into their branch to "stay current": they branch from the main they were created on and the
   orchestrator merges immediately after the hand-back (one at a time, git merge-tree check first). A coder merges main only when
   the merge-tree check shows a conflict, then resolves it in the registry file only.
4. Bases and byte compares at a fixed wave base commit (cmp --commit <wave base>) so a merge of another row does not force a new base.

## First steps of the next session
1. Read the files above, run `git status`, `sh 3ds/tools/wt_gc.sh` (dry), check docker (`docker ps`; never restart Docker while a build runs).
2. Pull the 60 fps crash.txt when the user has it (see above) and send it to the oracle (one fix row, Haiku).
3. Ask the oracle for: row TOOLFIX (stale check by content key; go.ps1 push builds main in the default lane), rows REGDIR1-3, the
   places table, then the missing plan rows. Show the user plan v2 with the changes and get the acceptance.
4. Only then launch rows, at most 6 coders, one status table per turn (State | Agent | Row | Summary) with the first line "Display
   list code remaining".
5. Merge or close the four passed rows as the user decides.

## User communication rules (all from 2026-11-12; memory feedback files hold the why)
No flattery and no apologies ("STOP GLAZING ME"); answer the question that was asked, with the number; the status summary is the Agent
description word for word and says what the row changes; read a failing tool's source first (timeouts, kills, swallowed output)
before re-running or reporting symptoms; never run a push, build or long command in the foreground of the chat (detached or
background, one line at start and end); decisions go through AskUserQuestion; never say the epic is done; no stubs, no toggles.
