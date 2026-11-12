# Row S7ENT3: HeartBlock and SuperBlock content list calls through the S7 tables (5 call sites)

(Model: Haiku. The S7ENT1 pattern in the two files row S6E3 edits; it waits for S6E3 so the lines do not conflict.)

## Code read (ORACLE7, main 87a256ef5)
- src/entity/HeartBlock.c:50 and :66, src/entity/SuperBlock.c:146, :162 and :178: `gSPDisplayList(gMainGfxPos++, dlist);`.
  After S6E3 they sit between nd_scomp_rec blocks (TARGET_3DS) and the old lines (#else), outside any branch. dlist is a model
  list of HeartBlockContent.c / SuperBlockContent.c (in the `# S7ENT` set).
- nd_rec_dl takes the walk_call arms (row_S7ENT1.md code read), so the draw is the same.
- kmr_03 has a heart block (src/world/area_kmr/kmr_03: Entity_HeartBlock), so ent_kmr03_det draws HeartBlock content;
  spot_sbk56_det draws the super block content (row S6E3 try 2).

## Blocker and start
S6E3 and S7ENT merged. `sh 3ds/tools/wt_new.sh S7ENT3`; PM_ROW=S7ENT3 PM_LANE=S7ENT3. Re-grep the five lines first.

## Touch only
The five lines and one include in each file (inside the existing `#ifdef TARGET_3DS` block that includes nd_scomp.h, added by S6E3).

## Exact edits
1. Each line becomes `#ifdef TARGET_3DS` / `nd_rec_dl(dlist);` / `#else` / the old line / `#endif`. A line that S6E3 already put
   inside a TARGET_3DS branch takes `nd_rec_dl(dlist);` in place.
2. `#include "nd_gtab.h"` after `#include "nd_scomp.h"` in each file.

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S7ENT3 --why "S7ENT3"`: PASS.
- `bench.py base ent_kmr03_det` and `bench.py base spot_sbk56_det`; one list `bench.py run ent_kmr03_det,spot_sbk56_det --row
  S7ENT3 --why "S7ENT3: block content lists from tables"` (2 starts). cmp both 6/6 byte-equal. Hand back the `gtabrun` tab and
  dl values of both logs (tab above 0 where the content draws; dl 0).
- Budget 8 Azahar starts (6 planned).

## Stop
A byte difference, nd_host FAIL or dl above 0: stop, commit, hand back the exact output (2 tries per named cause). Delete this
row file in the completing commit.
