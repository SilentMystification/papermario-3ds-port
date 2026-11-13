# Row COVMAP: site coverage of game files from the fncov census lane (no game source edit), tool covsites.py

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Plan v3 COVMAP, cut to the coverage map that
the GOLD rows need. ORACLE8 2026-11-13.)
## Design (fixed)
- The fncov census lane (PM_FNCOV=1, 3ds/CMakeLists.txt:383-391, 3ds/src/pm_fncov.c) counts the calls per function of the
  -finstrument-functions TUs. The switch is `fncov=<frame>`. The lines are `fncov f<n> <addr>:<calls> ...`, then
  `fncov f<n> done`. Today only 3ds/src TUs are instrumented (:387). This row adds game files from per-row list files
  3ds/tools/cov.d/<row>.txt (as gconv_src.d). It writes no line in a shared registry.
- A site is a line of an active branch (the TARGET_3DS rule of listcen.py) that matches
  `\bg(DP|SP)\w+\(|\bnd_(rec_dl|scomp_rec|draw_list|val_\w+|xf_\w+)\(`. The sites of a function count as covered when the
  function has calls > 0. This is exact for straight-line functions. A row with branches inside a function says so in its row.
## Facts (ORACLE8, main 0250d5221)
- Game sources are `${DECOMP}/src/...` in CMake (CMakeLists.txt:7 `set(DECOMP "${CMAKE_CURRENT_SOURCE_DIR}/..")`, effects.cmake,
  maps.cmake). set_property(SOURCE ...) needs that exact path form.
- hotset.py: lane_dir(lane, fncov=True) (:32) and parse_map(path) (:49) give {addr: {name, size, obj}} of the census map.
  hot_order.py:28-44 reads the fncov lines (regex `fncov f(\d+) ((?:[0-9a-f]{8}:\d+ ?)+)`).
- Bench fxfncov_89 (`warp=mac_01:0 pos=0:0:200 fxtest=89 fncov=420 loglevel=debug det`, until `fncov f\d+ done`) exists.
  bench.py `--lane L` runs build3ds/lane-L/pm_3ds.3dsx (bench.py:10).
- Expected, computed by the oracle with the site rule: src/effects/water_splash.c has 11 sites, all in water_splash_appendGfx.
Start: `sh 3ds/tools/wt_new.sh COVMAP`. PM_ROW=COVMAP PM_LANE=COVMAP on every call.
## Touch only (places)
3ds/CMakeLists.txt inside `if(PM_FNCOV)` (:386-391) only; new 3ds/tools/cov.d/README.txt and cov.d/COVMAP.txt; new
3ds/tools/covsites.py.
## Exact edits
1. CMakeLists.txt, after the FNCOV_TUS GLOB line (:387):
   `file(GLOB COV_LISTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/cov.d/*.txt)`, then for each list
   `set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${cl})` and
   `file(STRINGS ${cl} cov_files REGEX "^src/")`. Each line becomes `list(APPEND FNCOV_TUS ${DECOMP}/${cf})`.
2. cov.d/README.txt: one game source per line (repo-relative, starts with src/), one file per row, read only by the fncov lane.
   cov.d/COVMAP.txt: `src/effects/water_splash.c`.
3. covsites.py `<fncov log> --lane <L> <src file>...`: the calls per address (union of all fncov frames of the log). The map is
   parse_map(lane_dir(L, True) / "pm_3ds.map"). The functions of each file are definitions at column 0, with the body to the
   first `}` at column 0. `N(x)` matches the map name that ends with `_x` and whose obj contains the file stem. It prints per
   file `cov <file> sites <hit>/<total> funcs <hit>/<n>` and one `cov miss <file> <func> sites <k>` per function with sites
   and 0 calls.
4. Commit: `COVMAP: fncov lane instruments the game files of tools/cov.d, covsites.py gives site coverage`.
## Gate (expected values)
1. `PM_FNCOV=1 PM_LANE=COVMAP sh 3ds/build.sh` OK, and the normal build OK (no change: the lists are read only under PM_FNCOV).
2. `bench.py run fxfncov_89 --row COVMAP --lane COVMAP_fncov --why "COVMAP coverage"`, then
   `python 3ds/tools/covsites.py <its log> --lane COVMAP src/effects/water_splash.c`:
   `cov src/effects/water_splash.c sites 11/11 funcs 1/1`.
3. Seed first (a value change, then undo): in covsites.py count a function as hit when calls > 1000000. Then
   `sites 0/11` and one `cov miss ... water_splash_appendGfx sites 11`.
4. Budget 8 Azahar starts. Counter: none moves. This is the coverage tool of phase G (GMHARN and GMCOV become its use per row).
## Stop and end
A build error, a hit count other than 11/11, a seed that still shows 11/11, or a tool fault: stop, commit, hand back the exact
output (CLAUDE.md 4-5). Delete this row file in the completing commit.
