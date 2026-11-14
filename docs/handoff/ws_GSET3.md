# ws_GSET3: the entity model lists leave the device: gset.py block mode for lists written in the .c text (moves C5, C6, C12)
Model: Haiku. PM_ROW, PM_LANE = GSET3. Budget: 4 Azahar runs. Start after `Merge GSET2` shows in `git log --oneline main` (same tool, same twin folder). Branch GSET3.
Files: 3ds/tools/gset.py (block mode only), new 3ds/tests/gset/case_blk/ (fixture), 3ds/tests/host.d/gset.sh (one more case), new 3ds/tools/gset.d/GSET3.txt,
3ds/tools/gfxin/src/entity/model/ (twins), src/entity/model/*.c and the src lines gset.py rewrites.
FACT (grep, main 1149b5513): 57 files in src/entity/model hold 735 lines `Gfx <name>[] = {` at column 0, none static. 3ds/tools/gconv_src.d/S7ENT.txt lists 49 of them (the converter
set). Users outside: src/entity/<area>/*.c `extern Gfx X[];` and `STANDARD_ENTITY_MODEL_SCRIPT(X, ...)` (jan_iwa/ArrowSign.c:6, :54). Arrays reference Vtx and images of their own file.
## Group 1: block mode (host test first, commit `GSET3 1: ...`)
1. gset.py: a set-file line `src/entity/model/<x>.c` takes block mode (a line under src/effects/gfx keeps the include mode of ws_GSET1). Block of X = from the line
   `^Gfx (\w+)\[\] = \{` to the first later line that is exactly `};`. Move the block when X is in gset_ok.txt and the file has no `\bX\b` outside the blocks it moves; else
   print `gset keep <file>:<line>` and leave it. The twin is 3ds/tools/gfxin/<path>: first line `#include "<path>"` (create when missing), then the moved blocks in file order.
   In the device file the block becomes the one line `NDN_DECL(X);`. Add `#include "nd_gtab.h"` after the first #include of the device file. Users in other files: the
   ws_GSET1 rewrite rule, but a user file listed in a gconv_src.d row prints `gset keep` (the host link has no ndd_ symbol).
   Example (src/entity/model/ArrowSign.c:81-84) Before `Gfx Entity_ArrowSign_Render[] = {` .. `};` After (device) `NDN_DECL(Entity_ArrowSign_Render);` and the block in
   3ds/tools/gfxin/src/entity/model/ArrowSign.c. Before (jan_iwa/ArrowSign.c:6) `extern Gfx Entity_ArrowSign_Render[];` After `NDN_DECL(Entity_ArrowSign_Render);`;
   Before (:54) `STANDARD_ENTITY_MODEL_SCRIPT(Entity_ArrowSign_Render, RENDER_MODE_SURFACE_OPA)` After `STANDARD_ENTITY_MODEL_SCRIPT(NDN(Entity_ArrowSign_Render), RENDER_MODE_SURFACE_OPA)`.
2. Fixture case_blk: a model file with 3 blocks (A ok, B ok but used by a non-moved line in the file, C not ok), a user file, gset_ok.txt (A, B), the expected device file,
   twin and log written by hand from the rule. gset.sh runs it and cmp; seeded fault: a copy of gset.py whose block end is the first line that starts with `}` must differ
   (put a line `    }` inside block A's initializer). `sh 3ds/tools/nd_host_test.sh`: PASS with `gset ok`.
## Group 2: the entity set (1 build, 2 runs, commit `GSET3 2: ...`)
3. `grep -v '^#' 3ds/tools/gconv_src.d/S7ENT.txt > 3ds/tools/gset.d/GSET3.txt`. `sh 3ds/tools/nd_host_test.sh`, `python 3ds/tools/listcen.py > build3ds/out/lc_gset3_before.txt`.
4. `python 3ds/tools/gset.py 3ds/tools/gset.d/GSET3.txt > build3ds/out/gset3.txt`. nd_host PASS (`ntab moved` rises, every `bad 0`). A `gconv: moved <X>` exit or a link
   error naming X: put X's block back in its device file in place of its NDN_DECL line, rerun.
5. Build rc 0, memcheck margin above 0 KB. Gate: `bench.py run spot_mac01_drec --row GSET3 --why "GSET3 2"`, `bench.py cmp spot_mac01_drec --row GSET3 --commit $(git merge-base HEAD main)`;
   same for sbk14_drec: both byte-equal. listcen after: data and data_arrays fall, build and gfxpos unchanged. The commit deletes this page.
Fix rights: the files above. A cmp difference or a host FAIL: 2 tries per named cause, then commit the work in progress (no gate claim), build3ds/out/gset3.txt, send the path to the oracle.
Hand-back file, first 8 lines: hashes, gset moved and keep, ntab moved, listcen data and data_arrays before and after, cmp results, margin.
