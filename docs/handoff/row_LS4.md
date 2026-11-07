# Row LS4: splash mark to a native record, and the ENCCRASH walk gate

## Goal
1. LS-4 SPLASH (plan_s10_path.md, Rows): src/effects/water_splash.c:190/221 write gPmSplashMark (a G_NOOP with PM_SPLASH_ON/OFF in the low 24 bits, include/gbi_custom.h:228-232). nd_walk.c:302 reads the NOOP value and sets splash_on (nd_target.c:56, HANG1a). Replace the marks with a G_PM_ND SPLASH record (G_PM_ND is the native-only payload class, plan_s10_path.md Design / S10 model) handled in the nd module that owns splash_on; delete gPmSplashMark and the NOOP value read. Grep `gPmSplashMark` over src, 3ds, include is empty.
2. ENCCRASH walk gate (fix merged in nds1 92940f0f4: encwarp FIRST_STRIKE_NONE). Scenarios btrans_walk_enc / btrans_walk_noint / btrans_walk_fs1 (azt_scenarios.json) are status unverified, until "shot 39 frame ", cap 60, shotwin=116:40, padscript=right*200 at kmr_09. Find by code and the last log why the until-line does not match (shot count, the battle start frame, the walk path), fix the scenario (until-line from a log event of the subject, cap = subject time + 5 s), mark it verified only after a pass.

## Gates
- LS-4: nok11_splash (fb_nok11_splash_pic) by eye: the splash draws; grab line present; mac_01 byte-equal to nds1 (no splash there). The splash_on consumers (grab_cur / HANG1a path) still see the bracket: log line count of the grab equal to nds1.
- Walk: btrans_walk_noint 2 runs and btrans_walk_enc 1 run: no crash, no ENCCRASH validator line, the maphud battle line present, the shot shows the battle swirl or the battle.

## Rules
Own worktree from nds1 (build3ds/wt/ls4, branch ls4); copy assets/us and ver/us/build. Code first (CLAUDE.md rule 5). No attribution lines, never commit 3ds/src/map_restore_table.c, no git stash (shared across worktrees), keep Azahar open. Status lines in 3ds/logs/perf/status_LS4.txt. Budget: -ResetBudget LS4 at the cap. Delete this file in the last commit.
