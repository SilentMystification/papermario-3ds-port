# ws_EMIT4: the entity path takes a v2 list's words from its device entry (NEMIT part 2; blocker of GSET: C5, C6, C12)
Model: Haiku. PM_ROW, PM_LANE = EMIT4. Budget: 4 Azahar runs. Start after `Merge EMIT` shows in `git log --oneline main` (E1 gives NdNDev the fields u, nu).
Branch EMIT4 (`sh 3ds/tools/wt_new.sh EMIT4`). Files: 3ds/tools/nd_gfx_conv.c, 3ds/tools/nd_gtab_test.c, 3ds/tests/host.d/conv2c.sh, 3ds/include/nd_gtab.h,
3ds/src/nd_gtab.c, 3ds/src/nd_ent.c, 3ds/include/nd_int.h. CONV2C2 starts after `Merge EMIT4` (same converter file). No counter moves: GSET then sets u to the copy.
## Facts (code read, main 911cc3cec). The device entry u is the list until GSET (ws_EMIT step 2), so each edit reads the same words.
- Readers of a list's words on the entity path: ent_flat (nd_ent.c:133-149, from ent_compile :264), ent_hash_walk (:112-125, from :333 and ent_run :487),
  ent_tdwalk on the v1 table (:222-251, from :305; the pieces point into the list, nd_gfx_conv.c:138 and :153). ent_draw :449 and ent_gdl :416 use the address only.
- No game file writes into a static Gfx array (grep `\]\.words\.w[01] *=` in src: record buffers and the imgfx heap buffer only), so only the vertex span changes ent_hash.
- A gap with no table pieces runs as ent_sdef on k->fw (nd_ent.c:324-325), byte-equal to the pieces form (S7ENT gate, 6cca554d5).
## Group 1 (host test first, then the device code, one build, one commit `EMIT4: ...`)
1. nd_gfx_conv.c v2_units_top: delete the line `if (w0 == ND_NOP_A) continue;` (:414 only; :429 in v2_walk stays). v2_unit_add after `cx->ue[i][0] = 0;`
   (:387) add `if (w0 == ND_NOP_A) { if ((w1 >> 24) >= 16u) v2_unsup("no-op unit with an address", k); return; }`. ndnu now equals the list words to the stop.
   nd_gtab_test.c v2_units: delete `if (w0 == ND_NOP_A) continue;` (:484 only; :506 in v2_dev stays).
3. nd_gtab.h after `void nd_ntab_run(const NdNDev* e);` add: `int nd_ntab_flat(const NdNDev* e, void* out, unsigned* n, unsigned max, int depth, int
   (*call)(uint32_t w1, unsigned* n, int depth)); /* EMIT4: the ent_flat words of a v2 list from e->u (1 done, 0 refused) */`. In the NDN_UNIT comment
   replace `NOPs and the end left out` with `NOPs kept, the end left out`. 4. nd_gtab.c: after `#include "nd_gtab.h"` (:5) add `#include <string.h>` and `#include "nd_op.h"`. After nd_gtab_find (:27, before `#ifndef ND_SCOMP_HOST`) add:
   `int nd_ntab_flat(const NdNDev* e, void* out, unsigned* n, unsigned max, int depth, int (*call)(uint32_t w1, unsigned* n, int depth)) { for (unsigned i = 0;
   i < e->nu; i++) { const uint32_t* u = e->u + 2u * i; if ((u[0] >> 24) == NDO_CALL) { if (!call(u[1], n, depth)) return 0; if (u[0] & 1u) return 1; continue; }
   if (*n >= max) return 0; memcpy((char*)out + 8u * *n, u, 8u); (*n)++; } return e->nu < max; }`
5. nd_gtab_test.c (inside `#ifdef NTAB_V2`, before v2_compare): f_list = a copy of ent_flat (nd_ent.c:133-149) on `const uint32_t* l` writing into an
   `uint32_t* o` (a call target t is `(w1 >> 24) >= 16u`, else return 0). f_call(w1, n, depth): `(w1 >> 24) < 16u || depth >= 5` returns 0. Find the NdNTab of
   w1 in ndn_all (t->list == w1). Found: build `NdNDev c = { t->list, 0, (const uint32_t*)t->ops[0].p, t->ops[0].b, 0 }` (field order of E1) and return
   nd_ntab_flat(&c, ffb, n, ND_DLC_MAXOPS, depth + 1, f_call). Not found: f_words++ and return f_list(w1, ffb, n, depth + 1). v2_flat(t, name, fault): the same
   copy-form entry for t (nu - 1 when fault), ra = f_list(list, ffa, &na, 0), rb = nd_ntab_flat(&e, ffb, &nb, ND_DLC_MAXOPS, 0, f_call). Fail `flat words` when
   ra != rb, or ra and (na != nb or memcmp of 8 * na bytes). Count f_v2 (f_words 0) and f_w. Buffers ffa, ffb: `static uint32_t [2u * ND_DLC_MAXOPS]`.
   Call `v2_flat(t, name, fault);` after `v2_dev(t, name);` (:526). After the `ntab dev entries` printf add `printf("ntab flat v2 %u words %u bad %u\n", ...)`.
6. conv2c.sh after the existing fault check add: `if grep -q "^ntab flat v2 [0-9]* words [0-9]* bad 0$" /tmp/ndntab_fault.txt; then echo "seeded flat
   fault not seen"; exit 1; fi`. Run `sh 3ds/tools/nd_host_test.sh`: `ntab flat ... bad 0`, the fault seen, `ntab tables` equal to the value before step 1.
7. nd_int.h after the `u8 useseg;` line (:362) add `u8 v2; /* EMIT4: flat words from device entries only: the hash covers the vertex span */`. nd_ent.c: :128 Before `u32 h = ent_hash_walk(l, 2166136261u, 0);` After `u32 h = l ? ent_hash_walk(l, 2166136261u, 0) : 2166136261u;`. After ent_flat (:149):
   `static unsigned ent_flat_w, ent_v2_log;` and `static int ent_flat_call(uint32_t w1, unsigned* n, int depth) { const Gfx* t = (const Gfx*)ptr_of(w1); if (!t
   || !is_host((u32)t) || depth >= 5) return 0; const NdNDev* c = nd_ntab_find(t); if (c) return nd_ntab_flat(c, ent_flat_buf, n, ND_DLC_MAXOPS, depth + 1,
   ent_flat_call); ent_flat_w++; return ent_flat(t, n, depth + 1); }`. :264 Before `if (!ent_flat(l0, &nops, 0)) { k->why = 5; return; }` After `const NdNDev*
   nt = nd_ntab_find(l0); ent_flat_w = 0; const int fl = nt ? nd_ntab_flat(nt, ent_flat_buf, &nops, ND_DLC_MAXOPS, 0, ent_flat_call) : ent_flat(l0, &nops, 0);
   k->v2 = (u8)(nt && !ent_flat_w); if (!fl) { k->why = 5; return; }`. :300 After `const NdGTab* gt = k->v2 ? NULL : nd_gtab_find(l0);`. :333 After
   `k->hash = ent_hash(k->v2 ? NULL : l0, k->vp, k->vlen);` and after :334 `k->st = ENT_OK;` add `if (k->v2 && !ent_v2_log) { ent_v2_log = 1; pm_log_evt("entv2 l
   %08x n %u", (unsigned)k->list, nops); }`. :487 After `if (ent_hash(k->v2 ? NULL : l, k->vp, k->vlen) != k->hash) ent_run_recompile(k, l);`.
## Gate (1 build, 2 Azahar runs). nd_host PASS (step 6 lines), memcheck margin 310 KB or more, `bench.py run` then `cmp --commit` byte-equal: gold_12_drec (930de29e1), fxpic_91_drec (d29deacc3).
One of the two run logs has an `entv2 l` line (proof that the v2 path ran). Commit `EMIT4: ...` with the `ntab flat` line and the memcheck margin.
Fix rights: the seven files. A FAIL `flat words`, a smaller `ntab tables` count, a drec difference or no `entv2` line: no commit, write
build3ds/out/emit4.txt, send the path to the oracle (CLAUDE.md rule 5). Hand back: 8 lines (hash, `ntab flat` line, drec results, margin).
