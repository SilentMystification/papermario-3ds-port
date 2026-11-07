# Row S101: S10-1, delete pm_gbi.c and pm_gbi.h

## Goal
pm_gbi.c (comment blocks only after GLOB) and pm_gbi.h are deleted. Names pm_gbi_* stay (no rename). Pixels: layout only.

## Start state
Worktree build3ds/wt/s101, branch s101 from nds1 (after GLOB merge fe1c73c0d and this row file). Copy assets/us and ver/us/build into it.

## Premise and its check (oracle plan 2026-11-07)
Premise: pm_gbi.c has no definitions. Every function pm_gbi.h declares (pm_gbi_init, run, fallback, end_frame, eye_changed, stats) is defined in nd_walk.c, nd_cen.c or nd_mtx_set.c.
Check first: `grep -cE "^[a-zA-Z]" 3ds/src/pm_gbi.c` shows only #include lines (0 others); for each prototype in pm_gbi.h, grep its definition outside pm_gbi.c. Any miss: stop and report.

## Steps (one commit)
1. Move the pm_gbi.h prototypes and any types/macros it holds into nd.h (or nd_int.h when only nd_* files use them). Change every `#include "pm_gbi.h"` (about 17) to the header that now holds them.
2. Delete 3ds/src/pm_gbi.c, its CMakeLists.txt line, and 3ds/include/pm_gbi.h. Fix nd_host_test.sh and any tool that lists pm_gbi.c.
3. Delete the unreferenced globals seam_obj, seam_clip, st_cor if `grep -rnw` finds no reader (only their definition and writes); otherwise keep them and say so.
4. Fix comments that name pm_gbi.c as a file that holds code (grep -rn "pm_gbi\.c" 3ds/src 3ds/include 3ds/tools 3ds/CMakeLists.txt).

## Gate
build; nd_host PASS; `git ls-files 3ds/src/pm_gbi.c 3ds/include/pm_gbi.h` empty; byte gates (cold det, cmp against base shots you take first on lane s101): spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015, pause_spin_grab_det_w300.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=S101 PM_LANE=s101 on every build.sh and bench call. Budget 8 starts. No git stash. Never commit runs.jsonl or 3ds/src/map_restore_table.c. Wait for runs in the foreground. The commit deletes this row file.
