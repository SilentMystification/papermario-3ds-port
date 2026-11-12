# Row S7ENT1: src/entity list calls through the S7 tables (nd_rec_dl at 14 call sites in 12 files)

(Model: Haiku. One pattern, the S7EFX step 3 rule, over a fixed list of lines. HeartBlock.c and SuperBlock.c are row S7ENT3
(after S6E3); SaveBlock.c is row S7ENTX.)

## Code read (ORACLE7, main 87a256ef5)
- nd_rec_dl(list) (3ds/src/nd_gtab.c:64-86) records the list's table as an NDC_NAPI record, or the old gSPDisplayList unit when
  the list has no table. Its core 0 runner nd_gtab_run (3ds/src/nd_walk.c:544-582) takes the same arms as walk_call
  (nd_walk.c:223-247) for a non-map list: ent_gdl first (the entity compile path of S7ENT), then dlc_try, then the ops. So an
  entity list draws the same way; the S7EFX rows proved the arm byte-equal.
- The lists named at these sites are entity model lists of the `# S7ENT` set (src/entity/model/*.c), linked in place
  (include/entity.h:64-67: ENTITY_ADDR is a cast under TARGET_3DS).

## Blocker and start
S7ENT merged (the entity tables). `sh 3ds/tools/wt_new.sh S7ENT1`; PM_ROW=S7ENT1 PM_LANE=S7ENT1. Re-grep each line first.

## Touch only (each line `gSPDisplayList(gMainGfxPos++, X);`, X unchanged)
| file | lines (main 87a256ef5) |
|---|---|
| src/entity/BlueWarpPipe.c | :158 |
| src/entity/Chest.c | :105 |
| src/entity/HiddenPanel.c | :31 |
| src/entity/ShatteringBlock_common.c | :220 |
| src/entity/WoodenCrate.c | :239 |
| src/entity/default/BoardedFloor.c | :198 |
| src/entity/default/BombableRock.c | :223 |
| src/entity/default/Padlock.c | :47 |
| src/entity/jan_iwa/ArrowSign.c | :23 |
| src/entity/jan_iwa/Plants1.c | :48, :173 |
| src/entity/sbk_omo/StarBoxLauncher.c | :43 |
| src/entity/sbk_omo/Tweester.c | :38, :56 |
Plus one include per file. No other line of these files.

## Exact edits
1. Each listed line becomes `#ifdef TARGET_3DS` / `    nd_rec_dl(X);` / `#else` / the old line / `#endif` (the same indent).
2. Include: in ShatteringBlock_common.c and WoodenCrate.c add `#include "nd_gtab.h"` inside the existing `#ifdef TARGET_3DS`
   include block (after `#include "nd_scomp.h"`); in the other files add `#ifdef TARGET_3DS` / `#include "nd_gtab.h"` / `#endif`
   after the last `#include` at the top.
3. One commit; its message lists the 14 sites as file:line.

## Gate (expected values)
- `PM_ROW=S7ENT1 PM_LANE=S7ENT1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `bench.py run nd_host --row S7ENT1 --why "S7ENT1"`: PASS (no new table: the gtab line equals main's).
- `bench.py base ent_kmr03_det` and `bench.py base spot_mac01_det`; one list `bench.py run ent_kmr03_det,spot_mac01_det --row
  S7ENT1 --why "S7ENT1: entity list calls from tables"` (2 starts). cmp both: 6/6 byte-equal.
- No det scene draws these entity setups on screen (kmr_03 has a hidden panel, closed): the proof of the swapped arm is the
  code read above plus the byte set. If the ent_kmr03_det run prints a `gtabrun` entry, hand back its tab and dl values.
- Budget 8 Azahar starts (6 planned).

## Stop
A build error, a byte difference or nd_host FAIL: stop, commit, hand back the exact output (2 tries per named cause). A line
that is not at the listed place (after the re-grep) or is already inside a TARGET_3DS branch: apply the S7EFX rule (an existing
TARGET_3DS branch takes `nd_rec_dl(X);` in place) and name it in the commit. Delete this row file in the completing commit.
