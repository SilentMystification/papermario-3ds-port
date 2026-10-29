# Handoff: DRAWCOST epic, agent_drawcost

## State
- DS0 committed cbab0e929: dcsum line every 300 drawn frames (pm_gpu.c, pm_gbi.c hook, pm_gpu.h). Azahar mac01: picture byte-equal to baseline (build3ds/lane-mops/shots), dcsum line present (f299).
- DS1 committed cd248cbc8: pm_fl_dc() svc flush with GSP fallback, 9 call sites (pm_gpu.c, pm_tex.c, decl pm_port.h). Azahar mac01: picture byte-equal, fallbacks 0.
- DS2 reverted: -Os for pm_gpu.c gave equal picture, but dcsum stage sum 24 -> 26 us per close (+8 %, gate +5 %, one run, 1 us steps). Cold-branch moves not done.
- HUD (uncommitted in tree): pm_mem.c (rows 1 and 2 now VRAM: min free and peak used since map load, tex fill, A/B used and free, evictions per s), pm_main.c (refresh gate at PmOpt.hud_ms), pm_options.c (options.ini key hudint=<ms>: 250, 500, 1000 default, 2000, 5000; no panel row), pm_port.h (PmOpt.hud_ms). Build ok. azt yard and pause_menu: log checks pass. Bottom-screen picture NOT read: the azt shots are the top screen (400 wide). The log line "hud mem rows ready" prints once per run, so no second value exists to compare.
  Values read (yard, first refresh): row 1 "VRAM min 2130K pk 4014K tex 0/3072K"; row 2 "A 2007u/1065f B 2007u/1065f ev 0/s". Same in pause_menu. Linear/heap text was removed from HUD row 1.

## Open items
- HUD bottom-screen check: needs a bottom-screen shot at two refreshes one interval apart. Options: orchestrator decides a scenario (shotbottom switch at a fixed frame writes one frame only).
- HUD row not committed: picture check not complete.
- Panel row for hudint not added (ini key only). Decide if the panel must show it.
- Accidents: `git checkout` ran twice on 3ds/src/map_restore_table.c (DS0 and DS1 build loops) and once on 3ds/CMakeLists.txt (DS2 revert). The table was regenerated with gen_map_restore.py. Its first line differs from docs/handoff/map_restore_table.c.bak, so the content may not equal the user's pre-session version. CMakeLists.txt matches HEAD; a concurrent edit made before the checkout would be lost.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_BUILD_CAP=900 sh 3ds/build.sh
- Test: PM_ROW=DRAWCOST_<row> powershell -File 3ds/tools/azt.ps1 mac01 -Row DRAWCOST_<row> -Try (mac01); yard; pause_menu.
- Starts used this run: DS0 1, DS1 1, DS2 1, HUD 2.
