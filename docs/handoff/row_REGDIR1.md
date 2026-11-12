# Row REGDIR1: the gfx table source set as one file per row (3ds/tools/gconv_src.d/<row>.txt)

(Model: Haiku. One script region and a file migration; the generated tables must stay byte-identical.)

## Code read (ORACLE7, corrected after the coder's facts; main f792550fe)
- 3ds/tools/gen_gfx_tab.sh:7-19 builds GCONV_SRC from 13 lines. Lines with entries: :7 base (6 names), :8 S7EFX1 (13), :9 S7EFX2 (14),
  :10 S7EFX3 (13), :11 S7EFX4 (14), :12 S7EFX5 (14), :14 S7EFX6 (14), :15 S7EFX7 (14: squirt .. throw_spiny, merged 1b990e9a9),
  :17 S7ENT (49 .c paths): 151 entries. Empty lines: :13 (a second S7EFX5 tag), :16 S7EFX8, :18 S7MAP, :19 S7MDL. :26 maps each
  entry to a path (`*.c` as given, else src/effects/gfx/<name>.c).
- The order of GCONV_SRC is the include order of gconv_in.c and the table order of the generated nd_gfx_tab.c.
- The main folder's 3ds/src/nd_gfx_tab.c is stale (an older set): it is no reference. The reference is a fresh run of the unchanged
  script on this row's base.

## Start
Worktree build3ds/wt/REGDIR1 (branch REGDIR1). First `git merge main` (so S7EFX8's line is in if it merged). PM_ROW=REGDIR1 PM_LANE=REGDIR1.

## Touch only
3ds/tools/gen_gfx_tab.sh (:7-19 only), new directory 3ds/tools/gconv_src.d/. Not build.sh (REGDIR3 adds gconv_src.d to the stamp).

## Exact edits
0. Before any edit, after the merge: `PM_ROW=REGDIR1 PM_LANE=REGDIR1 sh 3ds/build.sh > build3ds/out/build_pre.txt 2>&1` (it runs
   gen_gfx_tab in docker pm3ds-host:1 and writes the worktree's 3ds/src/nd_gfx_tab.c); copy that file to build3ds/out/gfx_tab_pre.c
   and note `sha1sum` and the `gen_gfx_syms: <n> sources <m> symbols` entry of build_pre.txt. (Your run at dc5ea1dfe gave 151
   sources, 2485 symbols, sha1 f4acd4b3469437fec4524d573c62d0e788eb6be0: still valid if the merge brought no gen_gfx_tab.sh change.)
1. New files, one entry per line (blank lines and `#` comments allowed), each with exactly the names of its line in order:
   `000_base.txt` (:7), `S7EFX1.txt` (:8), `S7EFX2.txt` (:9), `S7EFX3.txt` (:10), `S7EFX4.txt` (:11), `S7EFX5.txt` (:12),
   `S7EFX6.txt` (:14), `S7EFX7.txt` (:15), `S7EFX8.txt` (:16, only if the merge filled it), `S7ENT.txt` (:17). No file for an empty
   line. LC_ALL=C sort of the names keeps today's order (000_base, S7EFX1 .. S7EFX8, S7ENT).
2. gen_gfx_tab.sh :7-19 become: `GCONV_SRC=$(cat $(ls 3ds/tools/gconv_src.d/*.txt | LC_ALL=C sort) | sed 's/#.*//' | tr -s ' \t\r\n' ' ')`
   and the comment "a row adds its sources as 3ds/tools/gconv_src.d/<row>.txt; no shared line".
3. If S7EFX8 merges after this row: its merge moves its names from the line into gconv_src.d/S7EFX8.txt (the orchestrator does it
   at that merge; this row's commit message says so).

## Gate (expected values)
- `PM_ROW=REGDIR1 PM_LANE=REGDIR1 sh 3ds/build.sh > build3ds/out/build_post.txt 2>&1`: gen_gfx_tab ran; its `gen_gfx_syms` entry
  equals the pre run (151 sources 2485 symbols, or 159 sources if S7EFX8 merged first); `cmp 3ds/src/nd_gfx_tab.c
  build3ds/out/gfx_tab_pre.c` prints nothing and the two sha1 values are equal (exact).
- nd_host PASS with the same `gtab lists <n>` value as the pre build. No Azahar start. Budget 0.

## Stop
A table difference: stop, commit, hand back the first differing line. Delete this row file in the completing commit.
