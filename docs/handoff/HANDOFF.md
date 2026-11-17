# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-16 evening, main ed22eb88f1, clean. Start of the new session: read this file, docs/handoff/orchestrator_rules.md (Core block is printed each turn), CLAUDE.md, the memory index.
Then ListAgents (status `running` is the ONLY slot count, user rule: never count from files, Docker or old text). Resume idle coders (`completed`) whose job is unfinished.

## Goal and counters (FACT, epic_status.sh, main ed22eb88f1)
Goal: remove the display list system. Done only when elfgate prints `func 0 data 0 gfxnames 0` and all listcen totals are 0.
Now: build 405, gfxpos 126, data 1207, data_arrays 92, gfxtype 583, recdl 10, reader 789, model 15; elfgate func 48 data 629 gfxnames 87.
Session start (dea919f9d1): build 566, gfxpos 201, data 2239, data_arrays 211, gfxtype 822, gfxnames 218. Memcheck margin 446 KB.
Sizing (oracle, build3ds/out/sizing_endgame.txt in the main folder): GUESS 62 Haiku rows + 4 Sonnet rows, longest chain 17 rows. Build 405 includes about 308 macro definitions in include/ that phase E deletes.

## Merged this session (all gated byte-equal unless noted)
LCNOBUILD, CGFX, DLSITE1, AXNS, FXHOLD, FXREN (exact expected diff gate, fire_breath fix, user accepted), CURT, DLSITE2, MODELRM, WSCMB, GMERLEE, FXWRAP, MAPCONV, NMDL0, EVAR, PBUF, DLCCHK, VINE, ROOTREST.
Pipeline rows merged: BENCHSPEED (host test), BASEBIN, VOLGEN, GFXINC, GFXINC2. build_slot.sh: 4 slots (PM_BUILD_SLOTS, memlog peak 2.89 GiB per build, WSL cap 14 GB).

## Agents at handoff (ListAgents)
Running: oracle a0c6529f765ca5d19 (GENSPEED: why the gen stage takes 260 s, writes ws_GENSPEED.md), oracle a0b1f51ef8deebbd9 (resumed on MAINBIN, its context is very large: start a fresh oracle for new work).
Coder a989e26b373144922 (MAINBIN, Haiku): branch MAINBIN has 2 commits. The oracle sent fix "MAINBIN 1c" (base_check skips basebin entries older than VOLGEN 1, 5f0da7a0fc). The rerun of post_merge.sh step 4 must print `post_merge ok`. Then gate and merge.
Idle and done: a4f2ded3c4729fa08, ad433f22f062b9e41, a3826c55761c6a920 (their rows are merged).

## Ready work
No ws_ page is free. ws_MAINBIN waits on its coder. ws_GENSPEED comes from the oracle.
Not paged (need an oracle design first): FXSEED gap a (31 quizmo lists, Sonnet, device design for lit draws, the user has not said yes for gap a), NTGT (Sonnet, approved), NIMGM (Sonnet, approved), 4 lists the converter refuses (screen_overlays PRIMDEPTH, kmr_20/mgm_00 RECT, hos_05 SCISSOR), 22 mdl_get_copied_gfx sites (after NMDL), ENTRY group (about 30 + 10 entity sites, needs per-scene captures), readers 789 and model 15 (phase E).
Phase E: stage 1C is blocked (nd_ent.c:311 still calls nd_dlc_compile on the device, build and gfxpos must be 0). Stage 5 needs data 0.

## Pipeline facts (FACT, user priority: no step may take minutes)
- A row took 42 to 115 min before. It took 32 to 33 min after the pipeline rows (EVAR, PBUF).
- Build of main after a source change: gen 260 s, sync-host 8 s, compile-link 90 s (build3ds/out/main_build_nmdl0.txt). gen is the largest part: GENSPEED.
- Root causes found: image .Id changed on every docker build (cold ccache), a base build per bench, host test twice, lane name and time tag in the build number (a base_check must build in the base lane).
- Cache safety rules (binding, plan_buildspeed.md): content-hash key of every input, recheck on every hit, seeded-fault self-test, atomic write, cachecheck at merge, cold gate at phase ends.
- PM_POSTMERGE_EVERY sets how often post_merge.sh runs. The user has NOT chosen N (I recommended every 3rd merge). Default 1 costs about 16 min per merge.
- Page rule: a page runs the host test once, after the change. Rows start in waves from one main commit, so they share one base (BASEBIN store).
- Coders end their turn while a job runs (status `completed`). Give each launch prompt the exact wait form: ONE Bash call that holds only `until test -f <abs done file>; do sleep 10; done` (timeout 590000), repeated. The harness returns it about every 180 s. The no-poll hook blocks any other form, also inside a heredoc.

## User-reported console faults and tests
Confirmed fixed by the user: select menu flash, pipe animation, map change crash.
Merged, not yet seen on the console: moon jump (MOON7), start menu L/R (STARTLR2), Tidal Wave in wide (WAVEWIDE), map change lag (MAPLAG), pause background, black coins, chapter title, Munchlesia, VINE (lava piranha vine in kzn_19 and kmr_22, no bench reaches them), fire_breath now draws 4 triangles once (FXREN visible fix).
Open: file select flip and start menu pane flip lag (capture with flipfps, docs/handoff/capture_flip.md), frame skip 3 modes.

## Next push (the user names the console, XL .131 is the main one)
Main built rc 0 at 8fdf72d126 (memcheck margin 446 KB). Main moved since: build main again and read the result. Run binchk with docs/handoff/push_strings.txt. THEN ask the console selector (AskUserQuestion). Then push with `--no-build`. Never ask before the build is finished.

## Process rules the user set this session (all in memory or orchestrator_rules.md)
ListAgents only for agent state. A state the user reports (nothing runs, console online) is fact. Short ASD-STE100 replies with sentences of 20 words or fewer (the style guard blocks longer ones, also in tables and bullets). FACT/GUESS/OPEN on numbers. Merge with sh 3ds/tools/merge_row.sh <row> (main clean, a branch that deletes a page main changed: git rm the page on main first). Tracked __pycache__ files were untracked (79ee809270).
