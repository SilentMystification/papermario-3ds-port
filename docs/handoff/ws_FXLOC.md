# ws_FXLOC (Haiku): effect list handles take the type NdLst (C7 gfxtype -100)
Designer: the oracle (SendMessage). Worktree build3ds/wt/FXLOC, PM_ROW=FXLOC PM_LANE=FXLOC. Main at release: bd56f6cec9. Start: now.
Shared files: bombette_breaking.c, flower_splash.c, flower_trail.c with ENTSELF. FACT: the hunks are 6 or more lines apart (:160 vs :166,
:170 vs :187, :215 vs :241), so git merges both in any order.
## Objective
- FACT (listcen --group src/effects, main 023db9ca81): gfxtype 165 in src/effects/*.c. FACT (oracle classification of the 165):
  100 are list handles: 47 pointer arrays `Gfx* X[] = { NDN(..) }` and 53 locals `Gfx* dlist = X[i]` that only nd_draw_list,
  gSPDisplayList or NDCTL_CALL use. 28 are `extern Gfx X[]` of unmoved effects/gfx lists (BACKLOG2). 28 are list builders
  (savedGfxPos, nd_rec_data vertex buffers) and 9 other: not in this page.
- FACT (code): NdLst is `const void*` (3ds/include/nd_gtab.h:66). nd_draw_list takes `const void*` (nd_gtab.h:89-90).
  gSPDisplayList casts its list to `unsigned int` (include/PR/gbi.h:1685). The type change keeps every pointer value.
- FACT (oracle dry run on 023db9ca81): `fxtype arrays ok 47 other 3 files 39 locals 53`. listcen src/effects gfxtype 167 -> 67
  (gfxtype total -100). Other counters: 0 change. fxloc.sh and fxtype.sh: `fxloc test ok`, `fxtype test ok`.
## Design (docs/handoff/ws_FXLOC.patch: fxtype.py --local, new fixture 3ds/tests/fxloc/a.c, new host test 3ds/tests/host.d/fxloc.sh)
The tool rewrites the 39 files of src/effects/*.c. Edit shape (src/effects/smoke_burst.c:13 and :112):
Before: `Gfx* D_E004E5E0[] = {`   After: `NdLst D_E004E5E0[] = {`
Before: `    Gfx* dlist = D_E004E5E0[data->unk_00];`   After: `    NdLst dlist = D_E004E5E0[data->unk_00];`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_FXLOC.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. `python 3ds/tools/fxtype.py --local src/effects/*.c > build3ds/out/fxloc_tool.txt`. Pass: the last line is
   `fxtype arrays ok 47 other 3 files 39 locals 53`. Another line: stop. Send the file to the oracle.
3. `git diff --stat -- src | tail -1` gives `39 files changed, 100 insertions(+), 100 deletions(-)`.
4. Commit the 3 patch files and the 39 sources: `FXLOC 1: effect list arrays and handle locals take NdLst (fxtype.py --local)`.
5. `python 3ds/tools/bench.py base fxr_drec > build3ds/out/fxloc_base1.txt 2>&1` and the same for pbuf_fx_drec (fxloc_base2.txt).
6. `sh 3ds/tools/rowloop.sh --row FXLOC --bench fxr_drec,pbuf_fx_drec --why "FXLOC gate" > build3ds/out/fxloc_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS (the host log has `fxloc test ok`), build rc 0, both cmp lines byte-equal.
- `python 3ds/tools/listcen.py --group src/effects | grep " gfxtype " | awk '{s+=$5} END {print s}'` gives 67.
## What you may edit to fix a fail
- A compile error in a source of step 2: change that one line back to `Gfx*` and its array or local partner too. Write the line in
  the commit. 2 tries per cause. Then send build3ds/out/loop_FXLOC_build.txt to the oracle.
- A base run that fails or a DIFF: stop. Send build3ds/out/loop_FXLOC_cmp_<bench>.txt to the oracle. Budget: 1 build, 4 Azahar starts.
## Stop and hand-back
Last commit `FXLOC 2: gate. fxr_drec, pbuf_fx_drec byte-equal; src/effects gfxtype 167 -> 67` deletes ws_FXLOC.md and the patch.
One SubagentHandback: commit hashes, the LOOP lines, the gfxtype value.
