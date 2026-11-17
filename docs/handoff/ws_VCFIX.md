# ws_VCFIX (Haiku): vertex copy draws fill their device entry (bug fix, no counter; expected draw change)
Designer: oracle NEXT2 (SendMessage). Worktree build3ds/wt/VCFIX, branch VCFIX from main, PM_ROW=VCFIX PM_LANE=VCFIX.
Main at release: fcf18f4daf. Edit only src/model.c, 3ds/tests/host.d/vcopy.sh, new 3ds/tools/azt.d/VCFIX.json, new 3ds/tools/bench.d/VCFIX.json.
Shares no file with GTSMALL, FXDIG, IMGFXT, BLANKET, FLTGT, EFXGT. Model: Haiku (3 inserts, one test block, one scenario copy).
## Cause (FACT, code)
- mdl_draw_vertex_copy (model.c:3728) records `mlvc->dev[selector]`. No code writes ModelLocalVertexCopy.dev (model.h:115; grep: only :3728).
- mdl_make_local_vertex_copy (:3689-3693) fills gfxCopy[i] and vtxCopy[i] only. nd_vcopy_dev (3ds/include/nd_vcopy.h:9) has no caller.
- So every vertex copy draw (hos_01 battle stars, mac_01 rug, kgr_01, pra_29, flo_14, hos_02, hos_03, mac_05 sign, Snowflakes) records a heap
  NdNDev that nobody set. FACT (nd_gtab.h:121-124): a NULL list in an NdNDev record gave ent_gdl(NULL) before.
## Edits (src/model.c)
1. After :14 `#include "nd_vcount.h"` add `#include "nd_vcopy.h"`.
2. After :3693 `            mdl_local_gfx_copy_vertices(baseVtx, numVertices, copy->vtxCopy[i]);` add
   `            nd_vcopy_dev(&copy->dev[i], copy->gfxCopy[i]); /* VCFIX: the entry that mdl_draw_vertex_copy records */`.
3. After :3700 `            copy->vtxCopy[i] = nullptr;` add `            copy->dev[i] = (NdNDev){ 0 };`.
4. In mdl_draw_vertex_copy, before :3728 `    nd_rec_ndev(&mlvc->dev[selector]);` add:
```c
    if (mlvc->gfxCopy[selector] == nullptr) {
        return; /* VCFIX: a copy made without lists (MakeLocalVertexCopy false) records nothing */
    }
    { static unsigned vcn; if (vcn < 4u) { extern void pm_log(const char*, ...); vcn++; /* VCFIX proof line (debug tool) */
        pm_log("vcopy draw %d nu %u ntri %u", copyIndex, (unsigned)mlvc->dev[selector].nu, (unsigned)mlvc->dev[selector].ntri); } }
```
5. 3ds/tests/host.d/vcopy.sh: before the last line `$N/t || exit 1` add a check. Extract `mdl_make_local_vertex_copy` with the awk form of :6 into $N/mk.c.
   `grep -q 'nd_vcopy_dev(&copy->dev\[i\], copy->gfxCopy\[i\]);' $N/mk.c || { echo "vcopy FAIL dev not filled"; exit 1; }`. Seeded fault: a sed copy
   without that call ($N/seed/mk.c) must fail the same grep, else `echo "vcopy seeded fault not seen (dev)"; exit 1`.
6. New azt.d/VCFIX.json: scenario `vcfix_hos01_drec`, a copy of `axns_kzn2_drec` (azt.d/AXNS.json) with `battle=BTL_HOS_FORMATION_01:BTL_HOS_STAGE_01`
   (Ember x2, stage hos_01, FACT pm_battle_table.h:70). Subject, proves, reason name VCFIX. New bench.d/VCFIX.json: bench `vcfix_hos01_drec` (copy of the
   axns_kzn2_drec bench in bench.d/AXNS.json). Commit 6 with 1-5.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-6. Commit `VCFIX 1: vertex copy entry filled (nd_vcopy_dev), null copy skipped, proof log; vcopy.sh check; vcfix_hos01_drec`.
2. `python 3ds/tools/bench.py base sbk14_drec --row VCFIX` (base run, not charged; sbk_14 has no vertex copy, FACT grep MakeLocalVertexCopy).
3. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row VCFIX --bench sbk14_drec --why "VCFIX control" > build3ds/out/vcfix_loop.txt 2>&1`.
4. `python 3ds/tools/bench.py run vcfix_hos01_drec --row VCFIX --why "VCFIX proof"` twice. Copy drec.txt of run 1 to build3ds/out/vcfix_a.txt.
## Tests that decide
- `LOOP PASS` of step 3 (host PASS with `vcopy ok`, build rc 0, sbk14_drec byte-equal).
- Step 4: both runs reach the until line; `cmp build3ds/out/vcfix_a.txt build3ds/azt/shots/vcfix_hos01_drec/VCFIX/drec.txt` equal; the run log has
  `vcopy draw` lines (1 to 4) with nu and ntri above 0. No base run of vcfix_hos01_drec: the base build has the fault. Budget: 1 build, 4 Azahar starts.
## Fix rights: only the 4 files. A crash, a missing `vcopy draw` line, nu 0, a cmp DIFF or a host FAIL: stop, send the path to the oracle, wait.
## Stop and hand-back
Last commit `VCFIX 2: gate. sbk14_drec byte-equal; vcfix_hos01_drec 2 runs equal, vcopy draw nu <n> ntri <n>` deletes this page. One SubagentHandback: hashes, LOOP line, vcopy draw lines.
