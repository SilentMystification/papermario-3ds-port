# Row HCCUT1: three debug functions out of the hot code (classes E and C), byte-equal

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (main map build3ds/pm_3ds.map 2026-11-11, hotset.py classes, oracle code read)
Class E M1-hot 20.1 KB has ndm_cmp 6672 B and rect_sig_check 2140 B; class C has pm_gpu_ndm_end 728 B. All three are debug paths:
- rect_sig_check (3ds/src/pm_gpu.c:674-684): the whole body runs only with switch rectsig (`if (!PMDL("rectsig")) return;` first),
  but the function is called on every rect batch close (:687) and merge (pm_gpu_rect.c:101), so all 2140 B sit in the hot group
  (3ds/ld/hot_order.ld:107).
- ndm_cmp (pm_gpu.c:3520, prototype :969): called only under `if (ndm_live)` (:1380, switch ndmcheck); attribute noinline, not cold.
- pm_gpu_ndm_end (pm_gpu.c:3498): called only under `if (ndm)` (nd_recs.c:194, switch ndmcheck).
## Start
`sh 3ds/tools/wt_new.sh HCCUT1` (build3ds/wt/HCCUT1, branch HCCUT1 from main). PM_ROW=HCCUT1 PM_LANE=HCCUT1 on every call.
Other rows edit pm_gpu.c in other functions: HC8 (pm_gpu_spr_build, pm_gpu_grange), SYNC60 (pm_gpu_begin_wait and a new function
above it), HC5 (pm_gpu_set_plan, pm_gpu_plan_key). Do not edit or move those; `git merge main` before the completing commit.
## Touch only
3ds/src/pm_gpu.c: rect_sig_check (:674-684), the ndm_cmp prototype (:969) and definition line (:3520), the pm_gpu_ndm_end definition
line (:3498). No other line.
## Exact edits
1. rect_sig_check: rename the existing function to `static __attribute__((cold, noinline)) void rect_sig_check_on(const char* where)`
   and delete its first line `if (!PMDL("rectsig")) return;`. Directly below it add:
   `void rect_sig_check(const char* where) { if (__builtin_expect(PMDL("rectsig"), 0)) rect_sig_check_on(where); } /* HCCUT1: the check body is cold */`
2. ndm_cmp: prototype (:969) `static void ndm_cmp(...)` -> `static __attribute__((cold, noinline)) void ndm_cmp(...)`; definition
   (:3520) `static __attribute__((noinline)) void ndm_cmp(` -> `static __attribute__((cold, noinline)) void ndm_cmp(`.
3. pm_gpu_ndm_end (:3498): `void pm_gpu_ndm_end(void) {` -> `__attribute__((cold, noinline)) void pm_gpu_ndm_end(void) {`.
## Gate (CLAUDE.md 2)
- Bases: LOADMTX is merged: `bench.py base` stores fresh bases again (test_catalog.md 6a).
- `PM_ROW=HCCUT1 PM_LANE=HCCUT1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `PM_LANE=HCCUT1 python 3ds/tools/bench.py run hotset_size --row HCCUT1 --lane HCCUT1 --why "HCCUT1 sizes"`: rc 0 (0 stale; if
  hot_order.ld names a function the map lost, hand back), M1hot E about 11.3 KB (20.1 minus 8.8), C about 10.6 KB. Write both.
- `python 3ds/tools/bench.py run nd_host --row HCCUT1 --why "HCCUT1"`: PASS.
- Byte compare (one list): spot_mac01_det and mdl5_iwa10_det (bases stored at b8bc06613): base, run, cmp, 6/6 each.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
