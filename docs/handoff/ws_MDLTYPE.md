# ws_MDLTYPE (Haiku): model list members and model list functions without the Gfx type (C7 gfxtype -16)
Designer: oracle NEXT3 (SendMessage). Worktree build3ds/wt/MDLTYPE, branch MDLTYPE from main AFTER the VCFIX merge, PM_ROW=MDLTYPE PM_LANE=MDLTYPE.
Edit only src/model.c, include/model.h, include/functions.h (:509), src/entity/HiddenPanel.c (:10), src/world/area_tst/tst_13/main.c (:6),
3ds/tests/vcopy/vcopy_test.c, 3ds/tests/host.d/vcopy.sh. Shares no file with GTSMALL, FXDIG, IMGFXT. Model: Haiku (type edits, 2 word loops).
## Objective and counters
- FACT (listcen 3b890c6d7f): gfxtype model.c 9, model.h 3, functions.h 2, HiddenPanel.c 1 of 3 (:10), tst_13 main.c 1. Move: gfxtype -16.
- Line numbers are main 3b890c6d7f. VCFIX adds 1 line after :14 and 7 lines in :3693-3728. Match the text, not the number.
- FACT: no `+`, `-`, `[ ]` or `++` on displayData->displayList outside model.c:3775 (grep). The other readers pass it to const void* or Gfx* params.
- lkey stays `src/model.c 3`: each word walk keeps one `(const uint32_t*)` cast line (lkey_cen.py R_CAST). No allow.txt edit.
## Edits
1. model.h:48 `Gfx* displayList;` -> `void* displayList; /* map shape list, unit word pairs */`. :112 `Gfx* gfxCopy[2];` -> `uint32_t* gfxCopy[2];`.
   :146 `typedef Gfx* ModelCustomGfxList[32];` -> `typedef void* ModelCustomGfxList[32];`.
2. functions.h:509 and model.c:3520: `Gfx* pre, Gfx* post` -> `void* pre, void* post`. model.c:3535 `const Gfx* l =` -> `const void* l =`.
3. model.c:3634 `mdl_get_vertex_count(Gfx* gfx,` -> `mdl_get_vertex_count(const void* gfx,`. :3641 stays (the cast line).
4. model.c:3647-3663 (mdl_local_gfx_update_vtx_pointers). Signature -> `(const void* nodeDlist, Vtx* baseVtx, uint32_t* arg2, Vtx* arg3)`.
   Before `u32 w0;` add `const uint32_t* src = (const uint32_t*)nodeDlist; /* MDLTYPE: unit word pairs (lkey MAP reader) */`.
   :3650-3652 -> `do {` (drop t[2] and its 2 reads). :3653 `w0 = t[0];` -> `w0 = src[0];`. :3654 `t[1]` -> `src[1]`.
   :3658 -> `arg2[0] = w0;`. :3659 -> `arg2[1] = (u32)w1;`. :3660 `nodeDlist++;` -> `src += 2;`. :3661 `arg2++;` -> `arg2 += 2;`.
5. model.c:3677 `Gfx* nodeDlist;` -> `void* nodeDlist;`. :3690 `heap_malloc(gfxCount * sizeof(*copy->gfxCopy[i]))` -> `heap_malloc(gfxCount * 2 * sizeof(uint32_t))`.
6. model.c:3731 `Gfx* outGfx` -> `const void* outGfx`. :3767 `Gfx* dlist;` -> `const uint32_t* dlist;`.
   :3775 -> `dlist = (const uint32_t*)model->modelNode->displayData->displayList; /* MDLTYPE: unit word pairs (lkey MAP reader) */`.
   :3778 -> `cmd = dlist[0] >> 0x18; tempVert = (Vtx*)dlist[1]; /* HC4e3 */`. :3786 `dlist++;` -> `dlist += 2;`.
7. HiddenPanel.c:10 and tst_13 main.c:6: `Gfx* destGfx` -> `const void* destGfx`.
8. vcopy_test.c:7 -> `void mdl_local_gfx_update_vtx_pointers(const void* nodeDlist, Vtx* baseVtx, uint32_t* arg2, Vtx* arg3);`.
   :34 -> `mdl_local_gfx_update_vtx_pointers(src, base, a, nv);`.
9. vcopy.sh: after the VCFIX dev check (it writes $N/mk.c), before `$N/t || exit 1`, add the size check and its seeded fault:
   `grep -q 'heap_malloc(gfxCount \* 2 \* sizeof(uint32_t))' $N/mk.c || { echo "vcopy FAIL copy size"; exit 1; }`
   `sed 's/gfxCount \* 2 \*/gfxCount */' $N/mk.c > $N/seed/mk2.c; grep -q 'gfxCount \* 2 \* sizeof' $N/seed/mk2.c && { echo "vcopy seeded fault not seen (size)"; exit 1; }`
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edit. `grep -nw Gfx src/model.c include/model.h` prints nothing. `python 3ds/tools/lkey_cen.py 3ds/tests/lkey/allow.txt .` prints `lkey census ok`.
2. Commit `MDLTYPE 1: model list members, custom gfx slots and vertex copy words without the Gfx type; vcopy size check`.
3. `python 3ds/tools/bench.py base <b> --row MDLTYPE` for b = vcfix_hos01_drec, entcape_kmr03_drec (base runs, not charged).
4. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row MDLTYPE --bench vcfix_hos01_drec,entcape_kmr03_drec --why "MDLTYPE gate" > build3ds/out/mdltype_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS with `vcopy ok 6`, build rc 0, 2 cmp lines byte-equal. Budget: 1 build, 4 Azahar starts.
- `python 3ds/tools/listcen.py | grep "^listcen total gfxtype"`: 16 lower than on `$(git merge-base HEAD main)` (read both values).
## Fix rights: only the 7 files. A compile error, an lkey line, a host FAIL or a cmp DIFF: stop, send the path to the oracle, wait.
## Stop and hand-back
Last commit `MDLTYPE 2: gate. vcfix_hos01_drec, entcape_kmr03_drec byte-equal; gfxtype <n>` deletes this page. One SubagentHandback: hashes, LOOP line, gfxtype.
