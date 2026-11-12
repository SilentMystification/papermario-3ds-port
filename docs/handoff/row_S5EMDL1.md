# Row S5EMDL1: the 62 render-mode lists Gfx_RM* as S6T defs, and entity_model.c calls them as compiled ranges

(Model: Haiku. Mechanical: one new header, one block in model.c, one macro and 44 line swaps in entity_model.c. Row S5EMDL2
(the other entity_model.c state lines) and S5EMDL3 (animator.c) wait for this merge.)

## Code read (main 5c3865487)
- src/model.c:648-1259: 62 lists Gfx_RM1_*, Gfx_RM2_*, Gfx_RM3_* (names: `grep -n "^Gfx Gfx_RM[0-9]_" src/model.c`). Their units
  are only SetRenderMode, SetCycleType, ClearGeometryMode, SetGeometryMode, SetCombineMode (2 lists) and End: all static classes
  of nd_scomp.c:16-27, so every list compiles with nv 0. Each unit is one Gfx: n = (array size) - 1 (End left out).
- src/entity_model.c: appendGfx_entity_model :376-454 and draw_entity_model_E :718-795 call them with
  `gSPDisplayList(gMainGfxPos++, Gfx_RMk_NAME);` (44 lines). The externs are at entity_model.c:30-.
- The model path (model.c, nd_rm_tab) and animator.c are not touched here.

## Start
`sh 3ds/tools/wt_new.sh S5EMDL1`. PM_ROW=S5EMDL1 PM_LANE=S5EMDL1 on every call.

## Touch only
New 3ds/include/nd_rmdef.h; src/model.c (one block right after the `};` of Gfx_RM3_CLOUD_NO_ZB, :1259); src/entity_model.c (a
macro block after its Gfx_RM externs, and the 44 call lines).

## Exact edits
1. 3ds/include/nd_rmdef.h:
   ```c
   /* nd_rmdef.h: the render-mode lists of src/model.c (Gfx_RM*) as S6T defs (nd_scomp.h). ND_RM_REC(Gfx_RMk_NAME) records the
    * compiled range of that list instead of a gSPDisplayList (S5EMDL1). */
   #ifndef ND_RMDEF_H
   #define ND_RMDEF_H
   #include "nd_scomp.h"
   #define ND_RM_LIST(X) X(Gfx_RM1_SURFACE_OPA) X(Gfx_RM1_DECAL_OPA) ... <all 62 names in the order of model.c>
   #define ND_RM_EXT(n) extern NdSDef n##_def;
   ND_RM_LIST(ND_RM_EXT)
   #define ND_RM_REC(n) nd_scomp_rec(&n##_def, NULL, 0)
   #endif
   ```
   Write the 62 names out (one `X(...)` each, several per line with ` \` line ends).
2. model.c after :1259: `#ifdef TARGET_3DS` / `#include "nd_rmdef.h"` /
   `#define ND_RM_DEF(n) NdSDef n##_def = { n, sizeof n / sizeof n[0] - 1 };` / `ND_RM_LIST(ND_RM_DEF)` / `#endif`.
3. entity_model.c after the Gfx_RM extern block:
   `#ifdef TARGET_3DS` / `#include "nd_rmdef.h"` / `#define EM_RM(l) ND_RM_REC(l)` / `#else` /
   `#define EM_RM(l) gSPDisplayList(gMainGfxPos++, l)` / `#endif`.
4. entity_model.c: replace each `gSPDisplayList(gMainGfxPos++, Gfx_RM` by `EM_RM(Gfx_RM` (44 lines; the `)` and `;` stay):
   `sed -i 's/gSPDisplayList(gMainGfxPos++, Gfx_RM/EM_RM(Gfx_RM/' src/entity_model.c`, then `grep -c "EM_RM(Gfx_RM"`: 44.
   No other line of the file changes.

## Gate (expected values)
- `sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. `bench.py run nd_host --row S5EMDL1 --why "S5EMDL1 nd_host"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det,s6chk_mac01
  --row S5EMDL1 --why "S5EMDL1: entity render-mode lists as defs"` (3 starts). cmp: 6/6 and 6/6 byte-equal. s6chk_mac01: `bad 0`
  and the `ranges` value next to main's last s6chk_mac01 run in runs.jsonl (reach report, not a stop: S6M1 merged with ranges 681 equal to main, as its records ran after f200; hand back both values).
- No `scomp: def` line in the 3 logs. Budget 8 Azahar starts.

## Stop
A byte difference, `bad` above 0, or a scomp line: stop, commit, hand back (2 tries per named cause).
Delete this row file in the completing commit.
