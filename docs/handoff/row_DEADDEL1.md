# Row DEADDEL1: delete the 5 dead functions with list calls that DEADSCAN found (strategy D2)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main f555551c4, DEADSCAN output and grep)
- DEADSCAN lists 5 functions that the 3DS executable does not hold and that no source file names (grep -rnw over src, include,
  3ds/src, 3ds/include finds only the definition line). Each body (LISTCEN2 rules, comments stripped):
  | file | function | lines | build | gfxpos |
  |---|---|---|---|---|
  | src/draw_img_util.c | draw_tiled_image | 216-343 | 3 | 3 |
  | src/draw_img_util.c | draw_adjustable_tiled_image | 359-530 | 5 | 5 |
  | src/background_gfx.c | func_80027BAC | 324-344 | 11 | 11 |
  | src/effect_utils.c | func_E02000AC | 60-111 | 17 | 17 |
  | src/model.c | is_model_center_visible | 4795-4897 | 20 | 20 |
  Sum: build 56, gfxpos 56. LISTCEN2 totals now: build 3768, gfxpos 3198.
- Comment lines directly above a function belong to it: src/effect_utils.c:59 (`// Unused, UB, ...`) and the comment block that
  ends at src/model.c:4794 (the depthQueryID notes). src/draw_img_util.c integer_log (:346-357) stays (other callers).
Start: `sh 3ds/tools/wt_new.sh DEADDEL1`. PM_ROW=DEADDEL1 PM_LANE=DEADDEL1 on every call.
## Touch only (places)
The 5 line ranges of the table (and the 2 comment blocks named) in src/draw_img_util.c, src/background_gfx.c,
src/effect_utils.c, src/model.c. Nothing else. (NPSO5 later edits draw_img_util.c, NBG1 background_gfx.c, NMDL rows model.c:
they wait for this merge. No open row has these places now.)
## Exact edits
1. Before any edit: `grep -rnw "draw_tiled_image\|draw_adjustable_tiled_image\|func_80027BAC\|func_E02000AC\|is_model_center_visible"
   src include 3ds/src 3ds/include` prints exactly the 5 definition lines. Any other line: stop and hand it back.
2. Delete each function from its first line to its closing brace, and the comment lines directly above it that the Facts name.
   Delete in each file from the bottom up (line numbers stay valid). Leave one blank line between the neighbours.
3. The same grep prints nothing.
4. Commit: `DEADDEL1: delete 5 dead functions with list calls (56 builder calls), DEADSCAN result`.
## Gate (expected values)
1. Count check with a seed: before any edit, `python 3ds/tools/listcen.py | grep -E "listcen total (build|gfxpos) "` must print
   build 3768 and gfxpos 3198 (it is not yet 3712: the check sees the tree). After all edits: build 3712 and gfxpos 3142.
2. `PM_ROW=DEADDEL1 PM_LANE=DEADDEL1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: build OK, memcheck OK.
3. `python 3ds/tools/deadscan.py build3ds/lane-DEADDEL1/pm_3ds.elf`: `dead 0`.
4. `python 3ds/tools/bench.py run nd_host --row DEADDEL1 --why "DEADDEL1 gate"`: PASS. No Azahar. Counter: C1 -56, C2 -56.
## Stop and end
A grep line other than the 5 definitions, a count other than the gate values, or a build error: stop, commit, hand back the exact
output (CLAUDE.md 4-5). Delete this row file in the completing commit.
