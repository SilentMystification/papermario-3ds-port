# Row CONV2F: nd_draw_list(list, ND_SELF) at the call sites of self-contained v2 tables (codemod rule, pilot water_splash.c)

Blocked by CONV2D2 (the runner and nd_ntab_find; the same v2_convert_all). Release it after the CONV2D2 merge and re-check the lines.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Design (ORACLE8, from row_CONV2D.md and row_CONV2D2.md; fixed)
- A self-contained v2 table needs no entry. Its call site becomes `nd_draw_list(list, ND_SELF)`. The macro records as nd_rec_dl
  does (nd_gtab.c:64-86), so core 0 takes nd_gtab_run and then nd_ntab_run (CONV2D2). An entry other than ND_SELF is a build
  error until ENTRY1 gives the entry form. Reason: no stub, and no ignored argument.
- Plan change: "nd_part_draw on v2" leaves this row. The particle run (nd_parts_run, nd_walk.c:415-485) keeps its v1 flat
  tables until the native emit row, because nd_part_draw reads NdGTab NDG_VTX and NDG_TRI ops (nd_geom.c:354-404).
- The converter writes the table names to build3ds/out/ntab_tables.txt. codemod.py reads it with `--ntab <file>`.
## Facts (ORACLE8, main 0250d5221)
- codemod.py: MAP 'nd_rec_dl' -> 'nd_draw_list' (:44). The entry check is at :204 (`entry is None` gives keep `no-entry`). The
  output is built at :221-224 (`out.append(entry)`). The argv loop is at :236-243.
- gen_gfx_tab.sh:18 calls nd_gfx_conv with 4 paths. :23 copies ntab_inherit.txt and ntab_unsup.txt to /w/build3ds/out.
  nd_gfx_conv.c main :603-604 (argc < 5 check). v2_convert_all :534.
- src/effects/water_splash.c:189 `nd_rec_dl(D_09000100_3BCB90);`. That list has a v2 table: it is not in ntab_inherit.txt and not
  in ntab_unsup.txt. :222 calls an element of the array D_E00BEAF0, so it stays (`no-entry`).
Start: `sh 3ds/tools/wt_new.sh CONV2F`. PM_ROW=CONV2F PM_LANE=CONV2F on every call.
## Touch only (places)
3ds/include/nd_gtab.h (after the nd_rec_dl prototype), 3ds/tools/nd_gfx_conv.c (main argc line, v2_convert_all signature and
its table loop), 3ds/tools/gen_gfx_tab.sh (:18 and :23 only), 3ds/tools/codemod.py (argv loop, the check at :204, the output
at :223), src/effects/water_splash.c:189.
## Exact edits
1. nd_gtab.h: `#define ND_SELF 0` and
   `#define nd_draw_list(list, entry) do { _Static_assert((entry) == ND_SELF, "an entry needs row ENTRY1"); nd_rec_dl(list); } while (0)`.
2. nd_gfx_conv.c: v2_convert_all gets a 5th path `tab_path`. It writes one table name per line (v2_tab[i]->name, in table order).
   main: `argc < 6`, the usage line gains `<ntab_tables.txt>`, and the call passes argv[5].
3. gen_gfx_tab.sh:18 adds `"$T/ntab_tables.txt"` after `"$T/nd_ntab_idx.c"`. :23 loop list: `ntab_inherit.txt ntab_unsup.txt
   ntab_tables.txt`.
4. codemod.py: the argv option `--ntab <file>` loads a set NTAB of names (empty without it). At :204: a call is self_ok when
   `nm == 'nd_rec_dl'` and its argument without blanks is in NTAB. A self_ok call is never kept for the entry. At :223 the
   output is `'ND_SELF' if self_ok else entry`. The docstring Mapping gains the line
   `nd_rec_dl(list) of a v2 table (--ntab) -> nd_draw_list(list, ND_SELF)`.
5. Build (it writes build3ds/out/ntab_tables.txt). Then run
   `python 3ds/tools/codemod.py --ntab build3ds/out/ntab_tables.txt src/effects/water_splash.c`.
6. Commit: `CONV2F: nd_draw_list(list, ND_SELF) for self-contained v2 tables, codemod --ntab, pilot water_splash.c`.
## Gate (expected values)
1. build3ds/out/ntab_tables.txt has 923 lines, and D_09000100_3BCB90 is one of them. Build, memcheck OK. nd_host PASS.
2. `codemod.py --dry --ntab build3ds/out/ntab_tables.txt src/effects/water_splash.c` before edit 5:
   `value 0 keep 0 state 3 dl 2` and one keep line (:222 no-entry). After: `dl 1`. listcen --group src/effects: recdl of
   water_splash.c 2 -> 1.
3. `bench.py base fxpic_89` at the merge-base, then `bench.py run fxpic_89 --row CONV2F --why "CONV2F byte gate"` and
   `bench.py cmp fxpic_89 --row CONV2F`: every shot byte-equal. Seed first (a value change, then undo):
   `nd_draw_list(D_09000100_3BCB90 + 1, ND_SELF)`. The cmp must differ.
4. Budget 8 Azahar starts. Counter: listcen recdl -1 (water_splash.c).
## Stop and end
A table count other than 923, a codemod count other than the gate, a cmp difference with the real edit, a seed with no
difference, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file at the end.
