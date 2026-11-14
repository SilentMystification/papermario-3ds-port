# ws_GKEY2: a list with a device entry runs from the entry on every device path (key = NdNDev.list; blocker of GSET: C5, C6, C12)
Model: Haiku. PM_ROW, PM_LANE = GKEY2. Budget: 2 Azahar runs. Start after `Merge GKEY1` and `Merge EMIT4` show in `git log --oneline main`. Branch GKEY2.
Files: 3ds/include/nd_gtab.h, 3ds/src/nd_gtab.c, 3ds/src/nd_walk.c, 3ds/src/nd_rec.c, 3ds/src/pm_main.c. No counter moves. Before GSET u is the list and nu the words
to its stop (ws_EMIT), so every edit reads the same words in the same order: the drec gate proves it.
## Facts (main 3eba999c4)
- Device reads of a list's words by its address: nd_rec_dl gt_state_only (nd_gtab.c:102), walk_call depth 0 `nd_ops_run(next, ND_LIST_MAX, 1)` (nd_walk.c:246),
  nd_lstcen (:245, debug census), rec_inline (nd_rec.c:390-403, a gSPDisplayList of a state-only list), the ent path (EMIT4 moves it to the entry).
- nd_walk_state stops at NDO_END and after a branch (nd_walk.c:365-382), so nd_walk_state(list, MAX) and nd_walk_state(u, nu) run the same units.
- nd_ntab_find sorts on its first call. GKEY2 adds core 1 callers (nd_rec_dl, rec_inline), so both indices sort before the logic thread starts (pm_main.c:539).
## Group 1 (one build, one gate, one commit `GKEY2: ...`)
1. nd_gtab.h after `void nd_gtab_run_e(const uint32_t* rec);` add `void nd_ntab_rec(const uint32_t* rec); void nd_ntab_rec_e(const uint32_t* rec); /* GKEY2: rec[0] = const NdNDev* */`.
2. nd_gtab.c nd_rec_dl. `static const NdGTab* memo_t[8];` -> `static const void* memo_t[8]; /* GKEY2: NdNDev* when memo_f bit 1, else NdGTab* */`. `const NdGTab* t = NULL;` -> `const void* t = NULL;`.
   Before `t = nd_gtab_find(list);` and the `if (t) { fl = ...` line After:
   `const NdNDev* nt = nd_ntab_find(list); if (nt) { t = nt; fl = nt->fl | 2u; } else { const NdGTab* g = nd_gtab_find(list); if (g) { t = g; fl = nd_gt_state_only((const uint32_t*)g->list); } }`
   `if (t) { memo_l[mi] = list; memo_t[mi] = t; memo_f[mi] = fl; mi = (mi + 1u) & 7u; }`. After `gtr_tab++;` add `const int v2 = (fl & 2u) != 0; fl &= 1u;`. In the r4 line
   `nd_rec_napi(dl_ent ? nd_gtab_run_e : nd_gtab_run, r4, 2u)` -> `nd_rec_napi(v2 ? (dl_ent ? nd_ntab_rec_e : nd_ntab_rec) : (dl_ent ? nd_gtab_run_e : nd_gtab_run), r4, 2u)`
   and `nd_rec_napi(nd_gtab_run, rec, 1u);` -> `nd_rec_napi(v2 ? nd_ntab_rec : nd_gtab_run, rec, 1u);`.
3. nd_walk.c. Rename `void nd_gtab_run(const uint32_t* rec) {` to `static void gt_run(const void* key, const NdGTab* t, const NdNDev* nt, const uint32_t* rec) {` and in its body:
   delete the `const NdGTab* t = ...` and `const NdNDev* nt = nd_ntab_find(t->list);` lines; `PMDL("entcap"), 0)) entcap_rec` -> `PMDL("entcap") && !nt, 0)) entcap_rec`;
   `(uint32_t)(uintptr_t)t->list` -> `(uint32_t)(uintptr_t)key`; `const Gfx* next = (const Gfx*)t->list;` -> `const Gfx* next = (const Gfx*)key;`. Keep PM_WALK_OWN() as its first line. After it add:
   `void nd_gtab_run(const uint32_t* rec) { const NdGTab* t = (const NdGTab*)(uintptr_t)rec[0]; gt_run(t->list, t, NULL, rec); }`
   `unsigned nd_gtr_rec; /* GKEY2: the runs of nd_ntab_rec */`
   `void nd_ntab_rec(const uint32_t* rec) { const NdNDev* nt = (const NdNDev*)(uintptr_t)rec[0]; nd_gtr_rec++; gt_run(nt->list, NULL, nt, rec); }`
   `void nd_ntab_rec_e(const uint32_t* rec) { nd_psochk_draw((const NdPsoDesc*)(uintptr_t)rec[3]); nd_ntab_rec(rec); }`
   walk_call :245-246 Before `if (__builtin_expect(nd_lc_frame != -1, 0)) nd_lstcen(next, ex_depth ? 2u : 1u);` and `if (!ex_depth) nd_ops_run(next, ND_LIST_MAX, 1);`
   After `if (__builtin_expect(nd_lc_frame != -1, 0) && !nt) nd_lstcen(next, ex_depth ? 2u : 1u);` and `if (!ex_depth) { if (nt) nd_ops_run(nt->u, nt->nu, 1); else nd_ops_run(next, ND_LIST_MAX, 1); }`.
4. nd_gtab.c gtr_cen: add `extern unsigned nd_gtr_rec;` next to `extern unsigned nd_gtr_ntab;`, `gtr_pr` to the static list, the ntabrun line becomes
   `pm_log_evt("ntabrun f%u runs %u rec %u", f, nd_gtr_ntab - gtr_pn, nd_gtr_rec - gtr_pr);` and add `gtr_pr = nd_gtr_rec;` after `gtr_pn = nd_gtr_ntab;`.
5. nd_rec.c rec_inline. After the first `if (a < 0x00100000u ...) return 0;` line: `const Gfx* l = (const Gfx*)(uintptr_t)a;` -> `const NdNDev* nt = nd_ntab_find((const void*)(uintptr_t)a);
   const uint32_t* w = nt ? nt->u : (const uint32_t*)(uintptr_t)a; /* GKEY2: the words of a list with a device entry come from the entry, its end at nu */`. In the first loop add
   `if (nt && n == nt->nu) break;` after `if (n == 64u) return 0;`, and `st_kind(l[n].words.w0)` -> `st_kind(w[2u * n])`. In the last loop `st_kind(l[i].words.w0) == 1u) nd_rec_state(l[i].words.w0, l[i].words.w1)`
   -> `st_kind(w[2u * i]) == 1u) nd_rec_state(w[2u * i], w[2u * i + 1u])`. Add `#include "nd_gtab.h"` if `grep -n nd_gtab.h 3ds/src/nd_rec.c` is empty.
6. pm_main.c:539 before `pm_lg_start();` add `{ void nd_tab_init(void); nd_tab_init(); } /* GKEY2: sort the list indices before core 1 and core 0 both look up */`.
## Gate (1 build, 2 Azahar runs)
nd_host PASS. Build rc 0, memcheck margin 310 KB or more. `bench.py run` then `cmp --commit <merge-base>` byte-equal: gold_12_drec, fxpic_91_drec. One run log has an
`ntabrun ... rec <n>` line with n above 0 (the new path ran). Commit `GKEY2: ...` with that line. The commit deletes this page.
Fix rights: the five files. A drec difference or rec 0: no commit, write build3ds/out/gkey2.txt, send the path to the oracle (CLAUDE.md rule 5).
Hand back: 8 lines (hash, ntabrun line, drec results, margin; details build3ds/out/handback_GKEY2.txt).
