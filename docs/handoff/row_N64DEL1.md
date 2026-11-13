# Row N64DEL1: delete the N64-only branches of src/effects (and write the tools that N64DEL2 and N64DEL3 reuse)

(Model: Haiku. 60 lines or fewer. Decision 2026-11-12: the port stops building for N64. Tag pre-list-removal keeps the old tree.
Plan: plan_endgame.md v3 phase H, moves C3 build_n64 and C4 gfxpos_n64 of src/effects to 0.)
## Code read (ORACLE7, corrected by ORACLE8 on main adfaf6187 with the merged LISTCEN2 tool)
- src/effects/*.c: 121 files with 424 blocks `#ifdef TARGET_3DS` (421) or `#ifndef TARGET_3DS` (3). No `#if defined(TARGET_3DS)`,
  `#if !defined`, `#elif` or compound condition names TARGET_3DS there. No file uses __LINE__.
- `python 3ds/tools/listcen.py` on main: group src/effects build 1337, build_n64 430, gfxpos 1314, gfxpos_n64 489, gfxtype_n64 2,
  recdl_n64 1. Totals build_n64 702, gfxpos_n64 769.
- The device defines and include paths: 3ds/tools/gen_gfx_tab.sh:12 (DEF) and :14 (INC). assets/us and ver/us/build are in
  the main folder only (3ds/build.sh:15-17), so the preprocessor compare runs in docker with the nd_host_test.sh mounts (:15-20).
Start: `sh 3ds/tools/wt_new.sh N64DEL1`. PM_ROW=N64DEL1 PM_LANE=N64DEL1 on every call.
## Touch only (places)
New 3ds/tools/unifdef3ds.py, new 3ds/tools/ppcmp.sh. src/effects/*.c: only the TARGET_3DS directive lines and the N64-only lines
they guard. Nothing else.
## Exact edits
1. unifdef3ds.py `<file>...`: rewrites each file in place, line based, comments untouched. For a block opened by `#ifdef
   TARGET_3DS` or `#if defined(TARGET_3DS)` (no `&&`/`||`): keep the lines up to its `#else`/`#endif`, drop the `#else` part,
   drop the directive lines. For `#ifndef TARGET_3DS` or `#if !defined(TARGET_3DS)`: drop the lines up to `#else`/`#endif`,
   keep the `#else` part, drop the directives. Nested blocks of other conditions inside are copied as they are (kept or dropped
   with their parent part). A block with `#elif` or a compound condition naming TARGET_3DS is left unchanged and printed as
   `unifdef3ds: skip <file>:<line>`. Prints `unifdef3ds: <file> blocks <n>` per changed file and `unifdef3ds: total blocks <n>`.
   Idempotent (a second run changes nothing).
2. ppcmp.sh `<outdir> <file>...` (LF): AR = the main folder (`git rev-parse --path-format=absolute --git-common-dir` + /..).
   One `docker run` of pm3ds-host:1 with the mounts and /work links of nd_host_test.sh:15-20. Inside, per file:
   `gcc -E -P -w <DEF of gen_gfx_tab.sh:12> <INC of gen_gfx_tab.sh:14> <file> | grep -v '^[[:space:]]*$'` into
   `<outdir>/<file with / replaced by _>.i`. Exit 1 when a gcc call fails.
3. Before the edit: `sh 3ds/tools/ppcmp.sh build3ds/out/pp_before src/effects/*.c`. Run unifdef3ds.py on src/effects/*.c. After:
   `sh 3ds/tools/ppcmp.sh build3ds/out/pp_after src/effects/*.c`. Commit the two tools and the files in one commit. The message
   lists the skip lines (expected none) and the total blocks.
## Gate (expected values)
- `unifdef3ds: total blocks 424`, no skip line. A second run: no file changes (`git status` clean).
- `diff -r build3ds/out/pp_before build3ds/out/pp_after` prints nothing (the compiled 3DS code is the same, plan section 5).
  Seeded fault first: append the line `int ppcmp_seed;` to one pp_after file, `diff -r` must name that file, then delete the line.
- `python 3ds/tools/listcen.py --group src/effects`: no `_n64` line for any file. Group src/effects build 1337 and gfxpos 1314
  (unchanged). Totals build_n64 272 (702 - 430) and gfxpos_n64 280 (769 - 489).
- `PM_ROW=N64DEL1 PM_LANE=N64DEL1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. `python 3ds/tools/bench.py run
  nd_host --row N64DEL1 --why "N64DEL1 gate"`: PASS. Budget 0 Azahar starts.
## Stop and end
A build error, a skip line, an active count that changed, or a preprocessor difference: stop, commit, hand back the exact output
(2 tries per named cause). Delete this row file in the completing commit.
