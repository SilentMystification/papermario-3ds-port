# ws_DEL1C (Haiku): nd_dlc.c reads only ND_FS units (nd_fs.h ids). dlc_op and the raw gbi op arms go; the host test types its raw words. Counter: C8 reader nd_dlc.c 27 -> 0, total 387 -> 360 (FACT listcen e3d2f6523b). Gate: 2 drec byte-equal, the nd_dlc host lines equal the start commit.
Designer: oracle DEL helper. Start commit main e3d2f6523b. No WAIT. Overlaps: none (no running row edits these files). Worktree build3ds/wt/DEL1C, branch DEL1C. PM_ROW=DEL1C PM_LANE=DEL1C.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_dlc.c, 3ds/tools/nd_dlc_test.c.
## Facts (code read, main e3d2f6523b)
- FACT every device and converter input of nd_dlc.c is typed: nd_ent.c (ND_FS units of k->fw), the .dlc build of nd_shape_conv.c (ND_X0 per word, nd_shape_conv.c:156), nd_seam_test.c:128 and mapconv_test.c:106 (NDO_END ends the words). Only nd_dlc_test.c feeds raw gbi words.
- FACT ND_FS ids equal the NDO ids (nd_fs.h:1-3). dlc_op (nd_dlc.c:12-30) maps a typed id to the raw OP_ id, so each `case OP_x` has one typed id: VTX, TRI1, TRI2, GEOM (OP_GEOMETRYMODE), TIMG, TILE, TILESIZE, LOADBLOCK, LOADTILE, LOADTLUT, END (OP_ENDDL), ND_NOP_A (OP_NOOP).
- FACT ent_known (nd_dlc.c:361-377) on a typed id after dlc_op is 1 exactly for ND_NOP_A, ND_FS_BASE..ND_FS_COMB and ND_FS_TIMG..ND_FS_MOVEMEM. `*bad` stays the typed id. tex_decode_n bad_op takes the FS id (only the texbad prof line logs it, nd_ent.c:43).
## Edits
1. nd_dlc.c includes: delete `#include "nd_ops.h"` (a compile error for a name of nd_ops.h: keep it), add `#include "nd_fs.h"`. Delete dlc_op (12-30, with its comment 10-11) and `#define OP_PM_TEXI 0x0B` (360).
2. Replace every `dlc_op(w0)` with `(w0 >> 24)` (lines 100, 187, 202, 280, 433, 487). Then rename each case and test (Before -> After): OP_ENDDL -> ND_FS_END, OP_VTX -> ND_FS_VTX, OP_TRI1 -> ND_FS_TRI1, OP_TRI2 -> ND_FS_TRI2, OP_GEOMETRYMODE -> ND_FS_GEOM, OP_SETTIMG -> ND_FS_TIMG, OP_SETTILE -> ND_FS_TILE, OP_SETTILESIZE -> ND_FS_TILESIZE, OP_LOADBLOCK -> ND_FS_LOADBLOCK, OP_LOADTLUT -> ND_FS_LOADTLUT, OP_LOADTILE -> ND_FS_LOADTILE.
3. nd_dlc_compile pass 1 (:145-148). Before: `case OP_NOOP: case OP_FULLSYNC: ... break;` and `default: return NDR_OP;` After: `default: if (w0 == ND_NOP_A) break; return NDR_OP; /* DEL1C: a sync or no-op unit */`.
4. ent_known. Replace the function (:361-377) with:
```c
static int ent_known(uint32_t w0) { /* DEL1C: the state, texture, geometry, matrix and no-op units (the typed rule of nd_rec.c st_kind) */
    const unsigned id = w0 >> 24;
    return w0 == ND_NOP_A || (id >= ND_FS_BASE && id <= ND_FS_COMB) || (id >= ND_FS_TIMG && id <= ND_FS_MOVEMEM);
}
```
   nd_ent_segments: `if (!ent_known(op))` -> `if (!ent_known(w0))`. The neutral test (:461) -> `} else if (w0 == ND_NOP_A) {`.
5. delta_apply (:381-421): keep only the typed switch with ND_FS names (NDO_GEOM -> ND_FS_GEOM, NDO_OMH -> ND_FS_OMH, NDO_OML -> ND_FS_OML, NDO_COMB -> ND_FS_COMB, NDO_COMB_W0 -> ND_FS_COMB_W0, NDO_TEXTURE -> ND_FS_TEXTURE, NDO_PRIM -> ND_FS_PRIM, NDO_ENV -> ND_FS_ENV). Delete the `if (nd_op_typed(op)) {` wrapper, its `return;` and the raw switch (401-420).
6. nd_ent_strip (:491): `NDO_W0(NDO_END, 0)` -> `ND_FS_W0(ND_FS_END, 0)`. `git grep -n "NDO_\|OP_\|dlc_op" -- 3ds/src/nd_dlc.c > build3ds/out/del1c_grep.txt`: empty.
7. nd_dlc_test.c: add `#include "nd_op.h"` and `#include "nd_fs.h"`. Add after vcopy:
   `static void ty(uint32_t (*L)[2], uint32_t (*T)[2], unsigned n) { for (unsigned i = 0; i < n; i++) { const uint32_t a = L[i][0], b = L[i][1]; T[i][0] = ND_X0(a, b); T[i][1] = ND_X1(a, b); } } /* DEL1C: raw words to units (nd_shape_conv form) */`
   ent_seg_test: add `uint32_t T[8][2];`. Before each of the 7 `nd_ent_segments(&L[0][0], ...)` calls put `ty(L, T, 8);` and pass `&T[0][0]`. :122 `bad == OP_DL` -> `bad == ND_FS_CALL`.
   main, the read loop (:161-164): keep the raw OP_ENDDL test. After the loop and the :165 test add `for (unsigned q = 0; q <= i; q++) { const uint32_t a = lw[2 * q], b = lw[2 * q + 1]; lw[2 * q] = ND_X0(a, b); lw[2 * q + 1] = ND_X1(a, b); } /* DEL1C */`. check_obj (:71-73): OP_VTX, OP_TRI1, OP_TRI2 -> ND_FS_VTX, ND_FS_TRI1, ND_FS_TRI2.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Before any edit: `sh 3ds/tools/nd_host_test.sh > build3ds/out/del1c_host0.txt 2>&1`. `grep "^nd_dlc" build3ds/out/del1c_host0.txt > build3ds/out/del1c_dlc0.txt`.
2. Edits 1-7. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del1c_host.txt 2>&1`: PASS. `grep "^nd_dlc" build3ds/out/del1c_host.txt > build3ds/out/del1c_dlc1.txt`. Commit `DEL1C 1: nd_dlc.c reads ND_FS units only`.
3. `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main) > build3ds/out/del1c_base1.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row DEL1C --bench spot_mac01_drec --no-host --why "DEL1C typed dlc" > build3ds/out/del1c_loop1.txt 2>&1`.
4. The same 2 commands for entcape_kmr03_drec (del1c_base2.txt, del1c_loop2.txt). Then `python 3ds/tools/listcen.py > build3ds/out/del1c_listcen.txt`.
## Gate (exact)
Host PASS. del1c_dlc1.txt equals del1c_dlc0.txt (`nd_dlc ent: PASS`, the `lists compiled` counts). Both rowloops exit 0. The grep of edit 6 is empty. listcen total reader 360, no nd_dlc.c reader line; the other totals equal the start commit except core1 (data).
## Fix rights and stop
Fix rights: the 2 files, compile errors only. Stop: a DIFF, a host line that differs, a crash, a tool fault. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time. One bench per rowloop call.
## Hand-back
Last commit `DEL1C 2: gate. spot_mac01 and entcape drec equal; reader nd_dlc.c 27 -> 0` deletes this page. One SubagentHandback: commit hash, the 2 nd_dlc lines, the listcen reader and core1 totals.
