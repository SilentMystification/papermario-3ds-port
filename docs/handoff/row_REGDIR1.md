# Row REGDIR1: the gfx table source set as one file per row (3ds/tools/gconv_src.d/<row>.txt)

(Model: Haiku. One script region and a file migration; the generated tables must stay byte-identical.)

## Code read (ORACLE7, main 5be532ef9)
- 3ds/tools/gen_gfx_tab.sh:7-19 builds GCONV_SRC from 13 lines, one per row (base, S7EFX1-6 with names, S7EFX7, S7EFX8, S7MAP, S7MDL
  empty, S7ENT with .c paths); :26 maps each entry to a path (`*.c` as given, else src/effects/gfx/<name>.c). Every new row edits a
  shared line region: the merge-conflict source the user named.
- The order of GCONV_SRC is the include order of gconv_in.c and the table order of the generated nd_gfx_tab.c.

## Start
`sh 3ds/tools/wt_new.sh REGDIR1`. PM_ROW=REGDIR1 PM_LANE=REGDIR1.

## Touch only
3ds/tools/gen_gfx_tab.sh (:7-19 only), new directory 3ds/tools/gconv_src.d/. Not build.sh (REGDIR3 adds gconv_src.d to the stamp).

## Exact edits
1. New files, one entry per line (blank lines and `#` comments allowed): `000_base.txt` (sparkles effect_3D snowfall snaking_static
   water_block embers), `S7EFX1.txt` .. `S7EFX6.txt` (the names of each line, in its order), `S7ENT.txt` (the .c paths). The empty
   lines (S7EFX7, S7EFX8, S7MAP, S7MDL) get no file. File names sort in today's line order (000_base < S7EFX1 < ... < S7ENT).
2. gen_gfx_tab.sh :7-19 become: `GCONV_SRC=$(cat $(ls 3ds/tools/gconv_src.d/*.txt | LC_ALL=C sort) | sed 's/#.*//' | tr -s ' \t\r\n' ' ')`
   and a comment: "a row adds its sources as 3ds/tools/gconv_src.d/<row>.txt; no shared line".
3. Before the edit, keep a copy of the generated 3ds/src/nd_gfx_tab.c (build3ds/out/gfx_tab_before.c).

## Gate (expected values)
- Build: gen_gfx_tab runs (its script changed); `cmp 3ds/src/nd_gfx_tab.c build3ds/out/gfx_tab_before.c`: identical (exact).
- nd_host PASS with the same `gtab lists <n>` value as before. No Azahar start (the tables are byte-identical). Budget 0.

## Stop
A table difference: stop, commit, hand back the first differing line. Delete this row file in the completing commit.
