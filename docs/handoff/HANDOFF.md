# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-16, main d947f064a5, clean (GSETR, STALEOBJ, FSKIP2 and GBIHDR merged). The user ended the session on purpose: start NO new work in the old session.
Start of the new session: read this file, docs/handoff/orchestrator_rules.md (Core block is printed each turn), CLAUDE.md, the memory index.
Then ListAgents (status `running` is the only slot count), then merge every hand-back message that arrived, then the list below.

## Goal and done
Remove the display list system completely. Done only when `python 3ds/tools/elfgate.py build3ds/pm_3ds.elf` prints `func 0 data 0 gfxnames 0`
and all `3ds/tools/listcen.py` counters are 0. Never say done before.
FACT (listcen, main ef3afb3fc9): build 722, gfxpos 201, data 2804, data_arrays 251, gfxtype 858, recdl 15, reader 785, model 15.
FACT (oracle run, earlier today, not re-run): elfgate func 48, data 807, gfxnames 246. Memcheck margin 434 KB (soft limit, above 0).

## Agents at the moment of handoff (a notice or hand-back message from each may arrive in the new session)
| Row | Agent id | Model, type | State | Page |
|---|---|---|---|---|
| HOLD1 (release 4 sources to gset, data -61) | a6baed310287d06b8 | Haiku coder | running | ws_HOLD1 |
| PRA31 (Matrix4s scan, pra_31 lists, data -311 GUESS) | ad285a439e033e8a8 | Haiku coder | running | ws_PRA31 |
| FLIPLOG (per-frame flip log row, switch flipfps) | a69f7a550bfc7c3dc | Haiku coder | running | ws_FLIPLOG(+_code) |
Oracles (all finished, resume the one that wrote a page when its coder asks): DL3 ac7ba50e4977ada89 (GBIHDR, HOLD1, PRA31), bug oracle 39
ae0d7e8991193de1b (FSKIP2, capture_flip.md), bug oracle 40 af53b666eeb2c5ea1 (FLIPLOG, verify_list.md).
GBIHDR deleted 132 unused gbi macros (listcen now build 566, data 2611, gfxtype 826). A branch that was built before that merge and uses a deleted macro fails to compile: after each of HOLD1, PRA31 and FLIPLOG, build main and restore the block from `git show 71ea9565ca:include/PR/gbi.h`.
Merge: `sh 3ds/tools/merge_row.sh <row>` (message is built from the branch commits). When main holds the oracle's page of the same row, `git rm` it first.

## Ready pages, not started (GSETR is merged, nothing blocks them)
- ws_CGFX (Haiku, 7 lists, new kkj bench), ws_DLSITE1 (Haiku, 21 sites in 13 files; resume the GSETR coder ace9cc3ba6cd67c64, it knows gset output).
- ws_MODELRM (Sonnet, user approved earlier) after CGFX merges (both edit nd_rec.c).
- Waiting rows with no page: A(x) namespace (needs GSETR and PRA31 merged), npc_merlee (gset.py), tst_13 (after MODELRM). Readers 785: phase E with the walker.
- FXSEED (Sonnet, 198 effect lists with no seed, 552 data): the user has NOT answered. Ask once.

## User-reported console faults (FACT, user)
Confirmed fixed by the user: select menu flash, pipe animation, map change crash.
Merged, NOT yet seen on the console (build 5877 was stale for the first two): moon jump (MOON7), start menu L/R (STARTLR2), Tidal Wave in wide (WAVEWIDE),
map change lag (MAPLAG; the startup menu entry has no hold, a hold there changes every drec bench base: needs the user).
Open: file select flip and start menu pane flip lag (cause not named: capture with flipfps after FLIPLOG, plan in capture_flip.md);
frame skip 3 modes (FSKIP2 running; FSKIP put them on the wrong row); pause background, black coins, chapter_change, Munchlesia (verify_list.md, binchk lines).

## Next push (the user names the console, XL .131 is the main one)
1. FSKIP2 and STALEOBJ are merged (3 frame skip modes on the Frame skip row, build.sh keeps no stale objects). Merge the other finished rows first. 2. Build main. 3. `python 3ds/tools/binchk.py build3ds/pm_3ds.elf <string of each merged row>` (strings in verify_list.md).
4. ONLY THEN ask the console selector (AskUserQuestion) and push with `--no-build`. Never ask before the build is finished (user, angry, twice).
5. Test list for the user: only unconfirmed items from verify_list.md. Words: `--profile log log=on moonjump` (add `-GameDebug flipfps` after FLIPLOG).
Capture rule: run the same words in Azahar first (3ds/tools/capture_check.py), read the real command in push_3ds.sh, never the echo line.

## Documents (cleaned 2026-11-16)
docs/handoff holds 40 files (was 143): the rules, the hooks, the live pages (ws_*), the architecture plans the epic still uses and the capture, verify and push lists. The old rows, agent notes, diagnostics, audits and archive are gone (git history keeps them). 3ds/ORCHESTRATOR.md, 3ds/AGENT_RULES.md and docs/ORCH_STATE.md are gone. `sh 3ds/tools/rules_lint.sh` (run by the Stop hook) stops a rule file that grows past its cap. `sh 3ds/tools/epic_status.sh` prints the epic totals.

## Process rules the user set today (all are in orchestrator_rules.md or memory)
- Every reply: short ASD-STE100, no agreement or apology phrases (global style hook `~/.claude/hooks/style_guard.js` blocks them), FACT/GUESS/OPEN on numbers.
- Agent label: `[Model type] ROW - task` in every Agent description and state table. Resume a finished agent when its context helps.
- Slots: count only status `running`. Start the oracle when all closed pages are blocked. Coder launch prompt names the oracle's agent id.
- A state the user reports is fact. Never probe the console. A bug: read the code to file, line and value.
- Hand-backs arrive as SubagentHandback messages. The disk hand-back check is gone.

## Process decisions for the new session (user: "Lets fix those", 1 to 5 of the token audit)
1. Interim wake-ups: let a coder wait for a background job inside ONE foreground call (`until test -f <done file>; do sleep 10; done`, timeout 590000), see CLAUDE.md rule 6.
2. This handoff replaces a long session. Start the next session fresh.
3. Smaller coder runs: pages touch few files, patch files for mechanical edits.
4. Resume relevant agents and reuse oracles (user decision).
5. Pages as patch files: the oracle writes `docs/handoff/ws_<id>.patch` for mechanical edits, the coder runs `git apply` and the gates.
