# Row CONV2F: nd_draw_list(list, ND_SELF) at the call sites of self-contained v2 tables (codemod rule, pilot water_splash.c)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE2 checked the lines at 0f5d3960b,
after the CONV2D2 merge.)
## Design (fixed)
- A self-contained v2 table needs no entry. Its call site becomes `nd_draw_list(list, ND_SELF)`. The macro calls nd_rec_dl
  (3ds/src/nd_gtab.c:64-86), so core 0 takes nd_gtab_run and then the v2 run of CONV2D2. An entry other than ND_SELF is a build
  error until ENTRY1 gives the entry form (no stub, no ignored argument).
- The particle run (nd_parts_run, nd_walk.c:423-444) keeps its v1 flat tables until the native emit row (nd_part_draw reads
  NDG_VTX and NDG_TRI ops).
- The converter writes the table names to build3ds/out/ntab_tables.txt. codemod.py reads it with `--ntab <file>`.
## Facts (main 0f5d3960b)
- 3ds/tools/codemod.py:44 `'nd_rec_dl': (1, False, 'nd_draw_list', (0,)),`. The entry check is :204
  `if not why and nm in DL_NAMES and entry is None:`. The output is :223 `out.append(entry)`. The argv loop starts at :236.
- 3ds/tools/gen_gfx_tab.sh:18 calls nd_gfx_conv with 4 paths. :23 copies ntab_inherit.txt and ntab_unsup.txt to /w/build3ds/out.
- 3ds/tools/nd_gfx_conv.c:659 `if (argc < 5)`, :660 `v2_convert_all(argv[1], argv[2], argv[3], argv[4]);`, :583 the
  v2_convert_all signature. Its table loop ends before the CONV2D2 dev index print (`NdNDev ndn_dev_idx`).
- src/effects/water_splash.c:189 `nd_rec_dl(D_09000100_3BCB90);` has a v2 table (ndn_D_09000100_3BCB90). :222 calls an element
  of D_E00BEAF0, so it stays (`no-entry`).
## Start
`sh 3ds/tools/wt_new.sh CONV2F`. PM_ROW=CONV2F PM_LANE=CONV2F on every build and bench call.
## Touch only
Files: 3ds/include/nd_gtab.h (after the nd_rec_dl prototype), 3ds/tools/nd_gfx_conv.c (main argc and call lines, the
v2_convert_all signature, a write after its table loop), 3ds/tools/gen_gfx_tab.sh (:18 and :23 only), 3ds/tools/codemod.py
(argv loop, :204, :223, docstring Mapping), src/effects/water_splash.c:189.
New names: none. Host link: none (no 3ds/src file changes).
Needs: 3ds/include/nd_gtab.h has nd_ntab_find
## Exact edits
1. nd_gtab.h after the nd_rec_dl prototype: `#define ND_SELF 0` and
   `#define nd_draw_list(list, entry) do { _Static_assert((entry) == ND_SELF, "an entry needs row ENTRY1"); nd_rec_dl(list); } while (0)`.
2. nd_gfx_conv.c: v2_convert_all gets a 5th parameter `const char* tab_path`. After its table loop it writes one line per table
   (v2_tab[i]->name, table order) to tab_path. main: `argc < 6`, the usage text gains ` <ntab_tables.txt>`, the call passes argv[5].
3. gen_gfx_tab.sh:18 adds `"$T/ntab_tables.txt"` after `"$T/nd_ntab_idx.c"`. :23 loop list: `ntab_inherit.txt ntab_unsup.txt
   ntab_tables.txt`.
4. codemod.py: option `--ntab <file>` loads a global set NTAB of names (empty without it). Before :204:
   `self_ok = nm == 'nd_rec_dl' and re.sub(r'\s+', '', args[0]) in NTAB`. :204 gains `and not self_ok`. At :223 the appended
   text is `'ND_SELF' if self_ok else entry`. Docstring Mapping line:
   `nd_rec_dl(list) of a v2 table (--ntab)      -> nd_draw_list(list, ND_SELF)`.
5. Commit 1 (edits 1-4). Build (it writes build3ds/out/ntab_tables.txt). Then run
   `python 3ds/tools/codemod.py --ntab build3ds/out/ntab_tables.txt src/effects/water_splash.c`. Commit 2:
   `CONV2F: nd_draw_list(list, ND_SELF) for self-contained v2 tables, codemod --ntab, pilot water_splash.c`.
## Gate (CLAUDE.md 2: nd_host and drec byte compare only)
1. build3ds/out/ntab_tables.txt has 922 lines, one of them D_09000100_3BCB90. Build: memcheck OK. nd_host PASS.
2. Before the codemod run: `codemod.py --dry --ntab build3ds/out/ntab_tables.txt src/effects/water_splash.c` prints one keep
   line (:222 no-entry) and `value 0 keep 0 state 3 dl 2`. After it: `dl 1`. `python 3ds/tools/listcen.py --group src/effects`:
   `listcen file recdl src/effects/water_splash.c` 2 before, 1 after.
3. At commit 1: `python 3ds/tools/bench.py base fxpic_89_drec`. Seed first: water_splash.c:189 becomes
   `nd_draw_list(D_09000100_3BCB90 + 9, ND_SELF)` (the run starts at the render mode: no texture on, cycle type or combine).
   Build (the ELF sha1 changes), `bench.py run fxpic_89_drec --row CONV2F --why "CONV2F seed"`, `bench.py cmp fxpic_89_drec
   --row CONV2F`: must differ. Undo the seed.
4. After commit 2: `bench.py run fxpic_89_drec --row CONV2F --why "CONV2F drec"`, `bench.py cmp fxpic_89_drec --row CONV2F`:
   drec.txt byte-equal.
5. Budget 8 Azahar starts. Counter: listcen recdl -1 (water_splash.c).
## Stop and end
A table count other than 922, a codemod count other than the gate, a drec difference with the real edit, a seed with no
difference, a base pair that differs, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this
row file in the completing commit.
