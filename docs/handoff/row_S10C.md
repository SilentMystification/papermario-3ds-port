# Row S10C: S10B follow-ups (dead nd_mtx_pool, nok11_det stop after until)

## Goal
1. Delete nd_mtx_pool (nd_mtx.c ~13, 64 KB .bss): after S10-2 it has no users.
2. nok11_det ends about 28 s after its until-line (until at ~16 s, end ~44 s, cap 45). CLAUDE.md rule 5 TIME: a test ends within 5 s of its until-line. Find why the run does not stop at the until-line and fix the stop.

## Start state
Worktree build3ds/wt/s10c, branch s10c from nds1 36048ea64. Copy assets/us and ver/us/build into it. Row GLOB runs in parallel (pm_gbi.c, nd_cen.c, nd_walk.c, nd_geom.c, nd_comb.c, nd_tex_set.c, nd_target.c, nd_mtx_set.c, nd_rect.c, nd_int.h): do not edit those.

## Checks first (0 starts)
1. `grep -rn "nd_mtx_pool" 3ds/src 3ds/include` shows only the definition (and maybe an extern). Otherwise stop and report the user.
2. Read the until/stop logic in 3ds/tools/azt.ps1 (and what it calls) for scenario nok11_det (azt_scenarios.json: until "fxtest: shot 1 effect 89 water_splash", debug "... shots ... offdump=410 ..."). Name the file, line and value that keep the run alive after the until-line matches (for example a wait for shot files, a deadline, or a shot count). Fix the stop at that point, in the tool, not by a shorter cap. The fix must not cut the shots the gate needs (nok11_det 0000-0015) or the shots of other scenarios: if the stop waits for shot files on purpose, limit the wait to the shots written up to the until frame plus 5 s.

## Gate
- build; nd_host PASS; 3dsx size smaller than before (report bytes); boot memory line (bss) smaller by about 64 KB.
- nok11_det: end_after_until_s (runs.jsonl) 5 s or less; shots 0000-0015 byte-equal to the base copy C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\48214233-5ad6-4a37-b750-8bdf4cfcc789\scratchpad\head\nok11_det (S10B head shots) or, if missing, a base run you take at 36048ea64.
- spot_mac01_det 6/6 and battle_start_f182_cold 1/1 byte-equal to a base run (the bss move changes layout).

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=S10C PM_LANE=s10c on every build.sh and bench call. Budget 8 starts. No git stash. Never commit runs.jsonl or 3ds/src/map_restore_table.c. Wait for runs in the foreground. Two commits (pool delete; stop fix). The second deletes this row file.
