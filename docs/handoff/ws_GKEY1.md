# ws_GKEY1: one standalone device entry ndd_<list> per v2 list, a pointer index, the state-only flag in the entry (blocker of GSET: C5, C6, C12)
Model: Haiku. PM_ROW, PM_LANE = GKEY1. Budget: 2 Azahar runs. Start after `Merge CONV2C2` shows in `git log --oneline main` (same converter and test files; `grep -n v2_dup 3ds/tools/nd_gfx_conv.c` must hit). Branch GKEY1 (`sh 3ds/tools/wt_new.sh GKEY1`).
Files: 3ds/include/nd_gtab.h, 3ds/src/nd_gtab.c, 3ds/tools/nd_gfx_conv.c, 3ds/tools/nd_gtab_test.c, 3ds/tests/host.d/conv2c.sh. No counter moves. Device draws stay the same (same lookup result, same words).
## Facts (main 3eba999c4)
- NdNDev readers and writers (grep): nd_gtab.h:103 type, :104 `extern NdNDev ndn_dev_idx[];`; nd_gtab.c:42-62 nt_sort and nd_ntab_find (device part); nd_gfx_conv.c:616-619 the index printf;
  nd_gtab_test.c:499 v2_dev search, :510-511 the n checks, :548 notri loop `ndn_dev_idx[i].ntri`; EMIT4 adds positional `NdNDev c = { ... }` copies (5 fields, no edit).
- The field n has no device reader (grep `->n\b` in nd_walk.c, nd_gtab.c, nd_ent.c: none). fl takes its place, so the struct size stays 20 bytes.
- gt_state_only (nd_gtab.c:81-88, with gt_kind :74-80) reads the list words on core 1. After GSET the words are gone, so the converter writes the flag.
## Group 1 (host test first, then one build, one gate, one commit `GKEY1: ...`)
1. nd_gtab.h. Before `typedef struct NdNDev { const void* list; uint32_t n; const uint32_t* u; uint32_t nu; uint32_t ntri; } NdNDev;`
   After `typedef struct NdNDev { const void* list; uint32_t fl; const uint32_t* u; uint32_t nu; uint32_t ntri; } NdNDev; /* GKEY1: fl = nd_gt_state_only of the list (converter) */`.
   Before `extern NdNDev ndn_dev_idx[];` After `extern const NdNDev* ndn_dev_idx[]; /* GKEY1: &ndd_<list> per entry, sorted by ->list at the first nd_ntab_find */`.
   After `void nd_ntab_run(const NdNDev* e);` add `unsigned nd_gt_state_only(const uint32_t* l); /* 1: the end within 64 units, every unit before it a state op or a sync */`.
2. nd_gtab.c. Move gt_kind, gt_state_only, nt_sorted, nt_sort and nd_ntab_find from the device part to the line before `#ifndef ND_SCOMP_HOST`. If
   `#include "nd_op.h"` is not above that line, add it after `#include "nd_gtab.h"`. Rename `static unsigned gt_state_only(` to `unsigned nd_gt_state_only(`
   and its call in nd_rec_dl. In nt_sort: `const NdNDev e = ndn_dev_idx[i];` -> `const NdNDev* e = ndn_dev_idx[i];`, `ndn_dev_idx[j - 1u].list > (uintptr_t)e.list`
   -> `ndn_dev_idx[j - 1u]->list > (uintptr_t)e->list`. In nd_ntab_find: `ndn_dev_idx[mid].list` -> `ndn_dev_idx[mid]->list`, `return &ndn_dev_idx[mid];` -> `return ndn_dev_idx[mid];`.
   After nd_ntab_find add: `void nd_tab_init(void) { if (!gt_sorted) gt_sort(); if (!nt_sorted) nt_sort(); } /* GKEY2 calls it before the logic thread starts */`.
3. nd_gfx_conv.c. Before `static V2C v2c_mem;` add `static unsigned v2_state_only(const uint32_t* l)`: the body of nd_gt_state_only with gt_kind written in line
   (copy both functions, rename, no other change). Replace the block from the comment `/* CONV2D2: the device index of the tables` to the printf of
   `const unsigned ndn_dev_n` with: a loop over i < v2_n_tab that skips `v2_dup[i]` and prints `const NdNDev ndd_%s = { (const void*)%s, %uu, (const uint32_t*)%s, %uu, %uu };\n`
   with (name, name, v2_state_only((const uint32_t*)v2_tab[i]->addr), name, v2_nk[i], v2_ntri[i]) and counts ndev; then `const NdNDev* ndn_dev_idx[%u] = {\n` (ndev, 1 when 0),
   one `    &ndd_%s,\n` per non-dup i, `    0\n` when ndev is 0, then `};\nconst unsigned ndn_dev_n = %uu;\n` with ndev. Example ndd line: `const NdNDev ndd_D_09000100_3BCB90 = { (const void*)D_09000100_3BCB90, 0u, (const uint32_t*)D_09000100_3BCB90, 12u, 4u };`.
4. nd_gtab_test.c v2_dev. Before `for (unsigned i = 0; i < ndn_dev_n; i++) if (ndn_dev_idx[i].list == t->list) dv = &ndn_dev_idx[i];` After `dv = nd_ntab_find(t->list);`.
   Delete the line `if (dv->n != ks) ...`. In the next line `dv->nu != dv->n + (fault ? 1u : 0u)` -> `dv->nu != ks + (fault ? 1u : 0u)`. After that line add
   `if (dv->fl != nd_gt_state_only(L) + (fault ? 1u : 0u)) { v2_fl_bad++; v2_fail("device index state-only flag", name, 0); }` with `static unsigned v2_fl_bad;` next to v2_dev_bad.
   :548 `ndn_dev_idx[i].ntri` -> `ndn_dev_idx[i]->ntri`. After the `ntab dev entries` printf add `printf("ntab dev flag bad %u\n", v2_fl_bad);`. In main after the
   `nd_gtab_find(not_a_list)` line add (inside `#ifdef NTAB_V2`) `if (nd_ntab_find(not_a_list)) { cur = "all"; fail("nd_ntab_find of a non-list", 0); }`.
5. conv2c.sh after the dev fault line add `grep -q "^ntab dev flag bad [1-9]" /tmp/ndntab_fault.txt || { echo "seeded flag fault not seen"; exit 1; }`. The normal run must print `ntab dev flag bad 0`.
## Gate (1 build, 2 Azahar runs)
`sh 3ds/tools/nd_host_test.sh` PASS with `ntab dev entries <n> bad 0`, `ntab dev flag bad 0`, the three seeded faults seen, `ntab tables` equal to the value before the edit.
Build rc 0, memcheck margin 310 KB or more (FACT EMIT merge 322 KB; GUESS +4 KB for the pointer index). `bench.py run` then `cmp --commit <merge-base>` byte-equal:
gold_12_drec, fxpic_91_drec. Commit `GKEY1: ...` with the ntab dev lines and the margin. The commit deletes this page.
Fix rights: the five files. A FAIL, a drec difference or a margin below 310 KB: no commit, write build3ds/out/gkey1.txt, send the path to the oracle (CLAUDE.md rule 5).
Hand back: 8 lines (hash, ntab dev lines, drec results, margin; details build3ds/out/handback_GKEY1.txt).
