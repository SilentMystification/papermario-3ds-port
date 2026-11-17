# ws_BARRICADE (Haiku): 2 map shape list walkers read u32 word pairs, not Gfx (C7 gfxtype -6)
Designer: oracle NEXT3 (SendMessage). Worktree build3ds/wt/BARRICADE, branch BARRICADE from main, PM_ROW=BARRICADE PM_LANE=BARRICADE.
Main at release: c9540957ad. Edit only src/world/area_omo/omo_02/barricade.c, src/world/common/util/GetFirstTriangleNormal.inc.c,
new 3ds/tests/maplist/maplist_test.c, new 3ds/tests/host.d/barricade.sh. Shares no file with MDLTYPE, VCFIX, GTSMALL, FXDIG, IMGFXT. No wait. Model: Haiku (closed spec).
## Objective and counters
- FACT (listcen c9540957ad): gfxtype barricade.c 3 (:44 :46 :55), GetFirstTriangleNormal.inc.c 3 (:17 :19 :48). Move: gfxtype -6.
- FACT: the callers (barricade.c:114, mim_01/03/06 hint.c:13) pass displayData->displayList. A const void* param takes Gfx* and void*.
- FACT: no bench runs these walkers (barricade blow-up event, mim laughing flowers event). The host test is the proof.
- lkey stays 1 per file: each walker keeps one `(const uint32_t*)` cast line (lkey_cen.py R_CAST). No allow.txt edit.
## Edits (same words, same order; one unit = 2 words)
1. barricade.c:44 `(Gfx* displayList, s32* outDist)` -> `(const void* displayList, s32* outDist)`.
   :46 `Gfx* dlist = displayList;` -> `const uint32_t* dlist = (const uint32_t*)displayList; /* BARRICADE: unit word pairs (lkey MAP) */`.
   :50-51 -> `u32 w0 = dlist[0]; u32 w1 = dlist[1];`. :55 `((Gfx*) w1, outDist)` -> `((const void*)(uintptr_t)w1, outDist)`. :78 `dlist++;` -> `dlist += 2;`.
2. GetFirstTriangleNormal.inc.c:17 `(Gfx* inGfx,` -> `(const void* inGfx,`.
   :19 `Gfx* gfx = inGfx;` -> `const uint32_t* gfx = (const uint32_t*)inGfx; /* BARRICADE: unit word pairs (lkey MAP) */`.
   :39-40 -> `u32 w0 = gfx[0]; u32 w1 = gfx[1];`. :48 `((Gfx* ) w1,` -> `((const void*)(uintptr_t)w1,`. :87 `gfx++;` -> `gfx += 2;`.
3. host.d/barricade.sh (copy the frame of host.d/vcopy.sh). Flags: vcopy.sh F plus `-msse2 -mfpmath=sse -ffp-contract=off`, link `-lm`. N=/tmp/barricade.
   $N/ext.c = `#define NAMESPACE t` + `#include "common.h"` + (barricade.c through `tr -d '\r'`, awk: print `/^#define BAR_/` lines and
   `void N(DetermineSphericalSize)` to its first `^}`) + all of GetFirstTriangleNormal.inc.c (tr -d '\r').
   Seeded fault: `sed 's/+= 2;/+= 4;/' $N/ext.c > $N/seed/ext.c`; cmp equal -> `barricade seeded fault not made`, exit 1. The seed binary
   must print a first line that starts `barricade FAIL`, else `barricade seeded fault not seen`, exit 1. Then build and run the real one.
4. maplist_test.c: `#include "common.h"`, prototypes `void t_DetermineSphericalSize(const void*, s32*);`
   `void t_GetFirstTriangleNormal(const void*, f32*, f32*, f32*);`. Lists are `uint32_t` arrays of unit pairs. VTX of n vertices:
   w0 `((uint32_t)NDO_VTX << 24) | (n << 12) | (n << 1)`, w1 the array address. CALL: w0 `(uint32_t)NDO_CALL << 24`, w1 the sub list. END: `NDO_END << 24`, 0.
   Case 1: Vtx va, vb, vc with v.ob (3,4,0), (6,8,0), (0,0,12). B = {VTX(1) vb, VTX(1) vc, END}, A = {VTX(1) va, CALL B, END}.
   r must be 12, else print `barricade FAIL radius <r>` (seed walk gives 5).
   Case 2: Vtx_t v[4] ob (0,0,0), (1,0,0), (0,0,1), (5,5,5). B2 = {TRI1 w0 `NDO_TRI1 << 24`, w1 `(0 << 16) | (2 << 8) | 4`, END},
   A2 = {VTX(4) v, CALL B2, END}. Set nx = ny = nz = 9.0f. Result must be nx 0, ny 1, nz 0, else `barricade FAIL normal` (seed leaves 9).
   Pass: print `barricade ok 2`, exit 0.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edit. `grep -nw Gfx` on the 2 game files prints nothing. `python 3ds/tools/lkey_cen.py 3ds/tests/lkey/allow.txt .` prints `lkey census ok`.
2. Commit `BARRICADE 1: omo_02 barricade and GetFirstTriangleNormal walk u32 unit pairs; host test barricade.sh`.
3. `python 3ds/tools/bench.py base spot_mac01_drec --row BARRICADE` (base run, not charged).
4. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row BARRICADE --bench spot_mac01_drec --why "BARRICADE gate" > build3ds/out/barricade_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS with `barricade ok 2`, build rc 0, spot_mac01_drec byte-equal (control). Budget: 1 build, 2 Azahar starts.
- `python 3ds/tools/listcen.py | grep "^listcen total gfxtype"`: 6 lower than on `$(git merge-base HEAD main)`.
## Fix rights: only the 4 files. A compile error, an lkey line, a host FAIL or a cmp DIFF: stop, send the path to the oracle, wait.
## Stop and hand-back
Last commit `BARRICADE 2: gate. barricade ok 2; spot_mac01_drec byte-equal; gfxtype <n>` deletes this page. One SubagentHandback: hashes, LOOP line, gfxtype.
