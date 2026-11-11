# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-10 night, main 86c3de04c or later. No agent runs. Read orchestrator_rules.md and CLAUDE.md first.

## At handoff
- Oracle batch 10 done: row_HC4f3.md (52 lines) and row_HC4e4.md (50 lines) are full Haiku rows. Worktree build3ds/wt/HC4f3 exists (from 384c404c2: merge main first).
- FFOG1 merged (pm5 fog fix). The user checks mim_11 on the XL after a push.
- build3ds/wt/TOOLB1: merged; its folder may be locked; the next wt_gc sweep removes it.

## Queue, in order
1. TOOLB2 (row_TOOLB2.md): items 1-3 merged (cb5659dad). Open: item 4 (shot copy at timeout) and gate (b), with the oracle.
2. ESCAPE2 (row_ESCAPE2.md, oracle first): START+SELECT exit faults the system; debug menu choice hbmenu / quit / HOME Menu.
3. HC4f3, then HC4e4. Then HOTORD (no row yet): PM_FNCOV=1 lane build, fncov_mac01_det/kmr10_det/mac00_det, hot_order.py.
4. STEREOFX (row_STEREOFX.md): the 3D slider hang on the XL. Evidence: a log-on push, slider on, wait 12 s, START+SELECT, pull
   crash.txt (GX queue dump). The kgr_01 hang did not return on 86004672a (cause not proven).
5. KZN04 GPU shading row (Sonnet, user OK; non-byte-equal shading accepted). HC7 step 7, HC8, SHEEP, DEADCODE.

## Waiting on the user (console, O3DS XL .131)
- 3D on or switched: omo dark room (Watt), Merlee, pause menu, a battle, a door. Coins and Star Piece. Heart Block HP count-up.
- mim_11 / Forever Forest floor after FFOG1 merges.

## Merged on 2026-11-10 (details in git log)
STEREOFX1, STEREOCRASH2, SEAM2, COINSTAR, PAUSEFADE, HEARTBLK, HC3 (-Os, console prof not worse), HC7 step 2, HC4e1, HC4e2a/b/c,
HC4e3 (all 3 groups), HC4f1, HC4f2, ESCAPE, N64PEEK (tool, parked), AZTSTEREO, BUILDCLEAN, BASEVERIFY, BASEFIX, TOOLB1.

## Rules changed on 2026-11-10 (now in CLAUDE.md and orchestrator_rules.md)
Fault is fact; code read only (no repro, no debug-log, no visual checks); tool faults stop the gate; the oracle writes row files;
one oracle per session start (others only on a user yes); session prompts at 300K then each 100K; base --fill after each merge; main stays clean; rows run in parallel only without merge conflicts; instruction files rewritten in STE (1da69d775).

## Tooling note
The scratchpad merge helper of the last session appended runs.jsonl lines but did not commit them. Commit runs.jsonl and
pushed_builds.txt with `git add -f` (3ds/logs is git-ignored, these two are tracked) so main stays clean (CLAUDE.md 10).
