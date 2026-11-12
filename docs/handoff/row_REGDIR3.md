# Row REGDIR3: no shared source list edit for a new file (CMake PORT_SRC by glob, build.sh stamps by directory)

(Model: Haiku. Two regions of two build files; the build output must stay the same.)

## Code read (ORACLE7, main 5be532ef9)
- 3ds/CMakeLists.txt:343-430 `set(PORT_SRC ...)` lists every 3ds/src/*.c by hand: each new port file edits it (S7TEX1, S7CONV, NAPI1).
  Compared with the directory: nd_op.c (host only: HC4e4) and pm_fncov.c (added at :462-464 only with FNCOV) are not listed;
  pm_audio_gate.c is listed but is not in 3ds/src (find where it comes from before the change and keep it as an explicit item).
- 3ds/build.sh:68 (gen_shapes `want=`: git hash-object of named files) and :86 (gen_gfx_tab `want=`: cat of named files): a new
  input needs a shared line edit, and today the entity model sources and the gconv_src.d files are missing from :86.
- GAME_SRC (:32-46) and the generated effects.cmake / maps.cmake are game sources that do not grow: unchanged.

## Start
`sh 3ds/tools/wt_new.sh REGDIR3`. PM_ROW=REGDIR3 PM_LANE=REGDIR3.

## Touch only
3ds/CMakeLists.txt (:343-430 PORT_SRC only), 3ds/build.sh (:68 and :86 only). TOOLFIX does not edit build.sh.

## Exact edits
1. CMakeLists: `file(GLOB PORT_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)`, then `list(REMOVE_ITEM PORT_SRC
   .../src/nd_op.c .../src/pm_fncov.c)` (each with a one-line reason), then the explicit item for pm_audio_gate.c as found.
   Keep every other property line that names PORT_SRC.
2. build.sh :86: `want=` = sha1 of `git ls-files -s -- $GT_IN; git diff HEAD -- $GT_IN; git ls-files -o --exclude-standard -- $GT_IN |
   xargs -r sha1sum` with `GT_IN="3ds/tools/gen_gfx_tab.sh 3ds/tools/gen_gfx_syms.py 3ds/tools/nd_gfx_conv.c 3ds/tools/nd_gconv.h
   3ds/tools/gconv_inc 3ds/tools/gconv_src.d 3ds/src/nd_scomp.c 3ds/src/nd_texwin.c 3ds/include src/effects/gfx src/entity"`
   (the key_now form of :119-126).
3. build.sh :68: the same form with `SH_IN="3ds/tools/nd_shape_conv.c 3ds/src/nd_seam.c 3ds/src/nd_dlc.c 3ds/include"` plus the
   names :68 has today that are not under these paths (keep each).

## Gate (expected values)
- A clean lane build: the link map lists the same object files as main's build3ds/pm_3ds.map (compare the sorted object lists:
  equal), memcheck OK, nd_host PASS. The generated nd_gfx_tab.c equals main's (cmp). No Azahar start. Budget 0.
- A second build with no change prints `gen_gfx_tab: up to date` and `gen_shapes: up to date`; touching one file under
  src/entity/model makes gen_gfx_tab run (revert after).

## Try 2 (ORACLE7, after 53971954e: row spec fault, budget reset once)

- Facts accepted: pm_audio_gate.c is tracked (keep it as the explicit item); PORT_SRC is now in glob order (alphabetical): the sorted
  object lists are equal, so the link has the same objects (the order of hot code is set by hot_order.ld, not by PORT_SRC).
- The table compare was against a stale main copy. REGDIR3 does not change gen_gfx_tab.sh, so the generated table depends only on
  the tree; what this row can break is the stamp (a table left stale). Corrected gate, in the worktree after `git merge main`:
  1. Fresh table: delete build3ds/lane-REGDIR3/gfx_tab.stamp and 3ds/src/nd_gfx_tab.c, build
     (`PM_ROW=REGDIR3 PM_LANE=REGDIR3 sh 3ds/build.sh > build3ds/out/b1.txt 2>&1`), copy 3ds/src/nd_gfx_tab.c to
     build3ds/out/tab_fresh.c, sha1sum it.
  2. Build again with no change: b2.txt shows `gen_gfx_tab: up to date` and `gen_shapes: up to date`; the table is cmp-identical.
  3. Add one comment line at the end of src/entity/model/Chest.c, build: b3.txt shows gen_gfx_tab ran; remove the line, build:
     gen_gfx_tab ran again and the table is cmp-identical to tab_fresh.c (equal sha1). No commit of the comment line.
  4. The object list and memcheck as before; nd_host PASS. Budget 0 Azahar starts.
- REGDIR1 (running) changes gen_gfx_tab.sh :7-19 and adds 3ds/tools/gconv_src.d/: GT_IN already names both; before REGDIR1 merges,
  `git ls-files 3ds/tools/gconv_src.d` is empty, which is valid. No overlap with REGDIR1's lines (this row edits build.sh and
  CMakeLists only). Merge order: either.

## Stop
A different object list or a table difference: stop, commit, hand back. Delete this row file in the completing commit.
