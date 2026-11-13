# ws_B: static lists: CONV2F and ENTRYCAP gates, then the entry form (counter C10; prepares C5, C6)

Model: Haiku. PM_ROW and PM_LANE = the branch name of the group. Memcheck margin 310 KB or more on every build (FACT: 318 at
CONV2D2, HANDOFF.md). C5 22103 and C6 2108 (FACT, listcen at b2442aa97) move only in the GSET groups after CONV2C2: not this page.
## Group 1: CONV2F (worktree build3ds/wt/CONV2F, branch CONV2F, head 00f72db89)
- Commit 1 (fbac2c7b7) and commit 2 (cfcf63ce3, water_splash.c:189) exist. First: `git merge main`. Then run row_CONV2F.md
  "Gate" steps 1-4 as written (922 tables, codemod dry counts, seed `+ 9` must differ, fxpic_89_drec byte-equal). Also
  `sh 3ds/tests/codemod/run.sh`: PASS. Commit runs.jsonl, delete row_CONV2F.md, hand the hash to the orchestrator (merge).
## Group 2: ENTRYCAP (worktree build3ds/wt/ENTRYCAP, branch ENTRYCAP, head 8a01f500e)
- Commit 1 (9276f8bda, scene fxpic_20_entcap) and commit 2 (a165990e8: nd_gtab.c:109, nd_walk.c entcap_rec, entcap.py) exist.
  First: `git merge main`. Run row_ENTRYCAP.md "Gate" steps 1-3. Save the entcap.py output to build3ds/out/entcap_fx20.txt
  (group 3 needs the D_090008F0_344860 line). Commit, delete row_ENTRYCAP.md, hand the hash over.
## Group 3: the entry form (new branch ENTRY1 from main after groups 1 and 2 merged; `sh 3ds/tools/wt_new.sh ENTRY1`)
Edit shape a, 3ds/include/nd_gtab.h:75-76 (CONV2F). Before:
  `#define ND_SELF 0` / `#define nd_draw_list(list, entry) do { _Static_assert((entry) == ND_SELF, "an entry needs row ENTRY1"); nd_rec_dl(list); } while (0)`
  After: `#define ND_SELF (*(const NdPsoDesc*)0)` / `void nd_rec_dl_e(const void* list, const NdPsoDesc* e);` /
  `#define nd_draw_list(list, entry) nd_rec_dl_e((list), &(entry))` (ND_SELF gives a null e; nd_gtab.h:10 includes nd_api.h).
Edit shape b, 3ds/src/nd_gtab.c. Add `static const NdPsoDesc* dl_ent;` and after nd_rec_dl:
  `void nd_rec_dl_e(const void* list, const NdPsoDesc* e) { dl_ent = e; nd_rec_dl(list); dl_ent = NULL; }`.
  The entcap line (:109 on ENTRYCAP). Before: `if (__builtin_expect(PMDL("entcap"), 0)) { const uint32_t r4[4] = { ..., 0u }; nd_rec_napi(nd_gtab_run, r4, 2u); return; }`
  After: `if (dl_ent || __builtin_expect(PMDL("entcap"), 0)) { const uint32_t r4[4] = { ..., (uint32_t)(uintptr_t)dl_ent }; nd_rec_napi(dl_ent ? nd_gtab_run_e : nd_gtab_run, r4, 2u); return; }`
Edit shape c, 3ds/src/nd_walk.c after nd_gtab_run: `void nd_gtab_run_e(const uint32_t* rec) { nd_psochk_draw((const NdPsoDesc*)(uintptr_t)rec[3]); nd_gtab_run(rec); }`
  (`#include "nd_psochk.h"`, prototype in nd_gtab.h next to nd_gtab_run :71). nd_psochk_draw: nd_comb.c:219 (one test when off).
Edit shape d, the pilot src/effects/explosion.c:195. Before: `nd_rec_dl(D_090008F0_344860);` After: `nd_draw_list(D_090008F0_344860, pe_explosion_0);`
  and at file scope after the externs (:13): `ND_PSO_DESC(pe_explosion_0, w0, ..., w7, b0, ..., b11);` from the entcap line:
  w0-w7 = the first 8 of the 11 words after `d`, b0-b11 = the bytes of the last 3 words, low byte first (nd_api.h:25-33).
New scene fxpic_20_psochk (azt.d/ENTRY1.json, bench.d/ENTRY1.json, copy of fxpic_20_entcap with `psochk` for `entcap`, until
`psochk f5\d\d draws \d+ bad \d+`). One commit with the scene, one with edits a-d.
## Group 3 gate (1 build, 2 runs plus the stored base pair)
1. nd_host PASS (psodesc.sh and psotab.sh see the new desc). Build: memcheck margin 310 or more. gen_pso_tab: no duplicate name.
2. Seed: one byte of pe_explosion_0 changed (b0 tex_on flipped). Run fxpic_20_psochk: the line shows `bad` 1 or more. Undo.
3. Real: fxpic_20_psochk shows `draws` 1 or more and `bad 0`. `bench.py cmp fxpic_89_drec --row ENTRY1`: byte-equal (ND_SELF path).
4. Counter: listcen recdl -1 (explosion.c:195). C1 unchanged.
## What you may edit to fix a failing test
Groups 1-2: only what the row file names. Group 3: the four edit places above and the two scene files. Never a ws_A directory
file other than explosion.c, and explosion.c only at :13 and :195.
## Stop and hand-back
Stop after group 3. The oracle writes the ENTRY group page (all 426 inheriting lists) from the group 3 result. Failures: CLAUDE.md
5 (2 tries per cause, then build3ds/out/wsb_<g>.txt to the oracle). Hand-back (8 lines): hashes, gate values, memcheck line.
