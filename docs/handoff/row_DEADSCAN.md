# Row DEADSCAN: list the source functions with list calls that the 3DS executable does not hold (dead code, strategy D2)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Strategy pick A + C + D2, user 2026-11-12.)
## Facts (ORACLE8, main 344822275)
- 3ds/tools/elfgate.py parse_elf(path) (:71-102) returns ([(name, value, size, type, section)], count). type 2 = FUNC.
  GCC clones keep the base name before the first "." (elfgate.py base_name :41).
- 3ds/tools/listcen.py: strip_comments (:55), line_flags (:126, True = active on the 3DS), the build class regex
  `\bg[DS]P[A-Za-z0-9_]*\s*\(`.
- The link drops unused functions (the build uses -ffunction-sections and --gc-sections: the main ELF of 2026-11-12 17:18 has no
  symbol draw_tiled_image, and src/draw_img_util.c:216 and :359 define draw_tiled_image and draw_adjustable_tiled_image, which
  no source file calls).
- A static function can be absent because GCC inlined it into a live caller. So absent is not dead by itself.
Start: `sh 3ds/tools/wt_new.sh DEADSCAN`. PM_ROW=DEADSCAN PM_LANE=DEADSCAN on every call. No blocker.
## Touch only (places)
New files 3ds/tools/deadscan.py, 3ds/tests/deadscan/run.sh, 3ds/tests/deadscan/fix_a.c, 3ds/tests/deadscan/test_deadscan.py.
## Exact edits
1. deadscan.py `<elf> [--src <dir>]` (default src): import listcen and elfgate from its own folder. For every .c file under
   the dir: strip comments, find each function definition (a line at column 0 matching `^[A-Za-z_][\w \*]*\b(\w+)\s*\(` that
   is not a prototype: the next `{` comes before the next `;`), its body to the matching `}`. Count the active build-class calls
   in the body. For functions with 1 or more calls: elf = 1 when a FUNC symbol has the name or base name, refs = the number of
   `\bNAME\b` matches in all .c and .h files under src and include outside the function's own definition line.
   dead = elf 0 and refs 0. Print per function `deadscan fn <file> <name> calls <n> elf <0|1> refs <r>` and at the end
   `deadscan total funcs <n> calls <c> absent <a> absent_calls <ac> dead <d> dead_calls <dc>`. Exit 0.
2. fix_a.c (fixture, never built): three functions with list calls: `live_fn` (2 calls), `inl_fn` (static, 1 call, called by
   live_fn), `dead_fn` (3 calls, no caller). test_deadscan.py runs deadscan's functions on fix_a.c with the symbol set
   {live_fn}: expected live_fn elf 1, inl_fn elf 0 refs 1 (not dead), dead_fn elf 0 refs 0 (dead), total dead 1 dead_calls 3.
   Print `PASS: <case>` / `FAIL: <case>`, exit 1 on a FAIL. run.sh: `cd "$(dirname "$0")/../../.." && python
   3ds/tests/deadscan/test_deadscan.py`.
3. Commit: `DEADSCAN: deadscan.py lists dead functions with list calls (ELF symbols and source references), fixture test`.
## Gate (expected values)
1. Seeded fault: in deadscan.py ignore refs (dead = elf 0 only). `sh 3ds/tests/deadscan/run.sh`: expected a FAIL for inl_fn.
   Restore: all PASS, exit 0.
2. `PM_ROW=DEADSCAN PM_LANE=DEADSCAN sh 3ds/build.sh > build3ds/out/build.txt 2>&1` (memcheck OK), then
   `python 3ds/tools/deadscan.py build3ds/lane-DEADSCAN/pm_3ds.elf > build3ds/out/deadscan.txt`. Expected: the lines of
   draw_tiled_image and draw_adjustable_tiled_image with elf 0 refs 0. Hand back the total line (the number is the result of
   this row: it decides the DEADDEL rows). No Azahar. Counter: none moved here (the DEADDEL rows move C1, C2).
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
