# Old branches: merge, redo or drop (ORACLE7, code read of git log main..<branch> and git diff main...<branch>, main f73457b88)

The orchestrator applies this list (git branch -D and wt_gc after the user's yes), then deletes this file.

| branch | holds (outside 3ds/logs) | gate | verdict and reason |
|---|---|---|---|
| FPS60W | pm_gpu.c gx_wait_or_reset and the C3D_FrameBegin checks, pm_wdog.c reboot, azt.d/bench.d FPS60W | none passed | drop: replaced by FPS60W2 (the cause fix in kc_build); the user rejected the timeout-and-reset form |
| HC5 | nd_rec.c c5cen census (109 lines, older form), c5cen_mac01 scenario | failed (T_SYNC 22689 of 22939) | drop: a census, its result is recorded (row_ORACLE_NEXT.md HC7b note); HC6 carried the later c5cen |
| HC6 | only the c5cen scenario and bench entries and old row files (main...HC6 has no 3ds/src diff: the HC6B command block is on main) | step 4 passed and merged; step 5 census failed (T_SYNC 99.3 %) | drop: the code is on main; the rest is a census scenario and deleted row files |
| PBATCH2 | nd_geom.c gv_tri routed vertex colour (+10), pm_gpu.c gvtx colour and plan compare (+7) | passed at 5e6e5c6c7 (686 commits behind): nd_host, hotset_size, spot_mac01_det and mdl5_iwa10_det 6/6 | drop: a speed change inside the gv walker path that plan_endgame.md deletes (S9DEL2) and that the native particle path (S4) replaced for effects; merging needs a new gate on today's main for code that is scheduled for deletion |
| PBFLUSH | PBATCH2's commits plus the fxflush_pink census scenario | census ran (run 504363) | drop: the census result is recorded (pink_sparkles 53.8 plan flushes per frame, row_ORACLE_NEXT.md); the scenario has no later user |
| S6E3 | HeartBlock.c, SuperBlock.c S6T compiled ranges (+70), s6chk_sbk56 scenario | not passed (the spot_sbk56_det base was never stored: two base runs differed) | drop: S6 compiled defs are list forms (plan_endgame.md section 9); the two files are in the src/entity golden-master family (GOLD-ENT, NENT0) |
| S7FLAME | flame.c nd_rec_dl (4 sites), the two buffers moved to src/effects/gfx/flame.c, build.sh stamp, GCONV flame line, fxpic_27 | failed (fxpic_27 1 of 7 shots equal) | drop: plan_endgame.md converts flame through CONV2/GSET (native tables); keep the finding for GSET1: a buffer a gfx source names moves to that source (the real definition, no stub) |

Worktrees with no commit off main: FRAMEHASH (on hold, only .wt_keep), HC7, HC8, S7TEX2 (only .wt_keep): remove with wt_gc.
NAPICEN2: its staged runs.jsonl holds 3 run lines (napicen_mac01, napicen_talk, napicen_btl, 2026-11-12T01:10-01:11); all 3 are
already in main's runs.jsonl: nothing to keep; remove. hud1 (.claude/worktrees/agent-ada36f387b16d4402, ddc31e84f): an old agent
worktree, 0 commits off main: remove with `git worktree remove`.
Nothing here is merged. No place of places.md is touched by a merge.
