# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-12 about 18:35, main 2fe6c42ac and later doc commits. No agent runs. All coders handed back and are merged or closed.
Read first, in this order: this file, orchestrator_rules.md (its Core block is the per-turn hook text: two top rules, do not glaze
the user and write everything in ASD-STE100, plus the process rule "first check, then ask, never guess and check"), CLAUDE.md,
plan_endgame.md (v2), places.md, test_catalog.md. The oracle is agent a113b2781e2728d61 (Opus, ORACLE7). It is idle. Its context
size is not a blocker and it is not replaced on a token count (user, 2026-11-12). If SendMessage cannot resume it, start one oracle
and give it this file.

## The goal and the one rule
Remove the display list system completely: no list builders, readers, walker state model, N64 list words (orchestrator_rules.md
"Definition of done"). The epic is NOT DONE until the old system is gone from the source or proven absent from the final 3ds
executable: `3ds/tools/elfgate.py` (row ELFGATE, not written yet) prints `func 0 data 0 gfxnames 0` and all source counters
(LISTCEN2) are 0. Nobody says "done" before that. Every status starts with the line "Display list code remaining" in counts.
Census at main 235bb8721: 3820 active builder calls (+648 N64-only), 3254 gMainGfxPos uses (+715), 22103 list data macros in 2107 Gfx
arrays, 714 reader hits in 22 3ds/src modules, 15 state-model functions. The user was told "done" at stage ends while the list code
stayed. The stage gates only measured their own rows.

## Where the plan stands (user decision pending)
plan_endgame.md v2 (100 rows; 45-60 h honest) is written and NOT accepted. The user answered "Changes needed". Done since then:
the handoff, the process updates, the no-conflict order (REGDIR1-3 and TOOLFIX are merged). New order from the user (2026-11-12):
most rows, if not all, must be Haiku-sized. A row that Haiku cannot do gets a spec for the model that can. No hard rule picks the
model. No stronger model without value (oracle.md item 4). Plan v2 still counts 71 Haiku, 27 Sonnet, 2 Opus rows: the oracle re-cuts
the Sonnet and Opus rows before the plan is shown again. Opus for NPSO and CONV2 was approved earlier; the re-cut may remove the need.
Still open: the user's acceptance. CLAUDE.md rule 2 (golden draw records and frame hash, no shot compare) is confirmed by the user and
NOT yet edited: edit it when HOSTCORE, GMHARN, GMCOV and FRAMEHASH exist. Until then bench.py cmp byte compares stay the runtime gate.
The user wants gates that are exact (one differing bit fails), efficient and not screenshot based: row_TESTIDEAS.md holds 12 gate ideas
(about 73 % of the 3356 call sites can run as host golden-master tests; large groups need psochk plus frame hash).

## Merged and closed this session (hashes changed by the repo repair: find rows with `git log --grep <row>`)
Merged: S6M6, S5SPR3, S6E, S7CONV, S5EMDL2, S5EMDL3, S3MSG, S5ITEM1, S7RUN, S6VORD, S7TEX1, S5TTL, TOOLB4, TOOLB5, S5CHUDB, S5MENU0,
S5BTL1, S7MAP, S7MAP2, S6KEY, S6SYNC, PUSHFIX, PUSHFIX2, LISTCEN, S7EFX1-8, S7ENT, S1TSTEN, TOOLDEDUP, REGDIR1, REGDIR2, REGDIR3,
TOOLFIX, FPS60W2. Memcheck: static 21648 KB, margin 338 KB (shrinks as S7 tables grow).
Closed (the oracle read each, row_BRANCHES.md): FPS60W (timeouts and reset: replaced by FPS60W2), HC5, HC6, PBATCH2, PBFLUSH, S6E3, S7FLAME,
S7TEX2. Their worktrees and branches are deleted. A bundle of the 7 branches with commits is in
C:\Users\Andrew\Documents\papermario-old-branches-2026-11-12.bundle. Row files of closed rows still in docs/handoff (HC8,
PBFLUSH, S6E3, S7FLAME, S7TEX2): delete them in the commit that accepts the plan. FRAMEHASH: row written, worktree deleted, on hold.
Memory files hold the why of the user rules (feedback_*.md in the memory folder).

## Console and the 60 fps freeze (user fact, still open)
Build #5018 (main with FPS60W2), 60 fps on: the O3DS .170 froze in Toad Town (mac_01) after some time. The picture froze and the
console needed a power cycle. FPS60W2 fixed one real defect (kc_build patch base not a command start) and is merged. It did not remove
this freeze. crash.txt (pulled by FTP, file build3ds/hw_o3ds_crash.txt): the game thread waits in phase 7 (gpu end: stream walk, submit,
frame end), step 555, pm_gpu_end_w:2986, native draws 20258 refused 0. GX queue n 4 cur 4 last 1: e1 id 01 30189c00 size 6350,
e2 id 01 3018ff50 size 2a80 (starts where e1 ends), e3 id 03 1f000000 3008ca00 019000f0. Build #4931 froze the same way (steps 652,
:2981). The oracle ruled out (row_FPS60W.md, last section): stream chain reuse, idle passes, kc_buf and vertex ring reuse, texture
uploads, GX queue depth, scene change refusal. Open path: the key frame queues the L frame's memory fills and VRAM copies again
(pm_gpu.c:2946-2955), targets not read. The live log was empty because the push used prof (log=off, pm_log.c:655-661).
First step of the next session (user order): the oracle reads what the two list entries contain and why the GPU cannot finish them.
FPS60VAL (Sonnet row, host validator of the key frame copy) is written, not started. The user decides after the next console test.
Console actions: push needs `-ip 192.168.1.170` (without -ip the netloader broadcast got no answer twice). Start the Homebrew
Launcher netloader (Y) before the push. After a freeze: power cycle, start FTP on the console, then
`powershell -NoProfile -File 3ds/go.ps1 pull o -ip 192.168.1.170` (crash.txt, dumps). Luma dumps: 3ds/tools/decode_dump.py.
options.ini may hold fps60=1 and hang the next boot (START+SELECT 1 s). The XL (.131) panic earlier was the netloader killed while
it took a 51 MB 3dsx. The 3dsx is 38 MB now.

## Tools and what broke (facts; details in git log)
- Push: go.ps1 prints push_3ds.sh output; detached start with PUSH STARTED and PUSH END, log build3ds/out/push_o_<time>.txt. A started
  upload is killed only after 60 s without growth or at 15 s per MB (min 120 s). TOOLFIX is merged: the --no-build stale check
  compares the folder's own build3ds/build_key.txt with that folder's key (PM_LANE= PM_KEY_ONLY=1 sh 3ds/build.sh), not the git hash.
  A lane build (PM_LANE=<row>) writes build3ds/lane-<row>/, not build3ds/pm_3ds.3dsx: build main with plain `sh 3ds/build.sh` before a push.
  The HBL netloader accepts one connection and stops listening after minutes: never test port 17491 before a push and never build
  between asking the user to open it and the push.
- Registries are per-row files now (REGDIR1-3 merged): effect table sources 3ds/tools/gconv_src.d/<row>.txt (gen_gfx_tab.sh reads
  them sorted), scenarios and benches 3ds/tools/azt.d/<row>.json and bench.d/<row>.json (merged at load), source files by glob in
  CMakeLists.txt and build.sh. Rows add files; they do not edit a shared list.
- runs.jsonl: merge_row.sh appends a note only if main lacks it and resolves a runs.jsonl conflict with main's version. Main file is
  7.7 MB. A worktree copy can be huge: `python -I 3ds/tools/dedupe_runs.py 3ds/logs/perf/runs.jsonl`, commit with `git add -f`.
- Bases: two runs on one slot; a base is keyed by commit, so every lane builds its own base (10+ min). The planned gates (golden
  masters, hashes) remove base builds.
- merge_row.sh needs a clean main. When a branch deleted a row file that main edited later, delete the file on main first (FPS60W2).
  Shell scripts stay LF (.gitattributes). A hook (no_guess_hook.js) blocks messages with guess-and-check phrases: describe a read, not a run.

## Parallel work without merge conflicts (user order, 2026-11-12)
Several agents may work in one file, never in the same area of code. A merge conflict is a planning fault. How: (1) every row names its
PLACES (file plus function or region) and places.md lists the open ones. A row launches only if no open row owns one of its places.
(2) Rows never edit a shared registry (see Tools). (3) Coders do not merge main into their branch. The orchestrator merges at once
after the hand-back, one at a time. (4) Bases and byte compares use a fixed wave base commit (cmp --commit).

## Repo repair: DONE, the push to GitHub is the user's action (2026-11-12)
Cause: 53 blobs over 100 MB in local history (3ds/logs/perf/runs.jsonl, 1.7 to 3.5 GB each). Done: `git filter-branch --index-filter`
on the 522 local commits that origin lacks (main and 18 small branches). A runs.jsonl over 90 MB became the 7.7 MB file. The hashes of
those commits changed. origin/main is an ancestor of main (512 commits ahead): a push adds commits and needs no force. The main tree
equals the pre-rewrite tree (59259a6f). Branch dlc-compiler-c1c2 was left out (no runs.jsonl, old upstream history, 3611 commits origin
lacks). Backups: mirror clone C:\Users\Andrew\Documents\papermario-3ds-port-backup-2026-11-12.git, the bundle above, and
refs/original/* in the repo (remove them with `git for-each-ref --format="%(refname)" refs/original | xargs -n1 git update-ref -d`,
then `git gc --prune=now`, only after the user confirms the push). Why filter-repo failed: it strips the PGP signatures of old
upstream commits and changes every hash. Before a push, `git rev-list --objects <branch> --not --remotes=origin | git cat-file
--batch-check='%(objectsize) %(rest)'` must show no blob over 100 MB. Remote: https://github.com/SilentMystification/papermario-3ds-port.git.

## First steps of the next session
1. Read the files above. Run `git status`, `docker ps`, `sh 3ds/tools/wt_gc.sh` (dry). Never restart Docker while a build runs.
2. Start the oracle read of the freeze (Console and the 60 fps freeze). It writes a row. Haiku-sized pieces. Merge as the coders hand back.
3. Ask the user: did the push to GitHub work, is the freeze fixed on the console, is FPS60VAL needed.
4. Have the oracle re-cut the Sonnet and Opus rows of plan v2 to Haiku-sized rows. Show the plan to the user. Get the acceptance. Then the
   oracle writes the missing plan rows (NOLIST, HOSTCORE, GMHARN, GMCOV, PICAVAL, COVMAP, GOLD rows).
5. Only then launch plan rows: at most 6 coders, places.md checked first, one status table per turn with the first line "Display
   list code remaining". Edit CLAUDE.md rule 2 when HOSTCORE, GMHARN, GMCOV and FRAMEHASH exist.

## User communication rules (all from 2026-11-12; memory feedback files hold the why)
No flattery and no apologies. Answer the question that was asked, with the number. The status summary is the Agent description word for
word and says what the row changes. First check the facts (this file, the tool help or source, git, the code), then ask. Ask only
a decision or a fact no read gives. Never guess and check. Read a failing tool's source before re-running. Never run a push, build
or long command in the foreground of the chat. Decisions go through AskUserQuestion. Never say the epic is done. No stubs, no toggles.
Never launch an agent when the user says not to. An oracle's context size is no reason to stop or to ask for a new oracle.
