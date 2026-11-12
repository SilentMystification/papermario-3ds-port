# Row SCOMP1: state compiler: core 0 entries (a def without a record) and a hashed 128-slot entry table

(Model: Haiku. Exact code below. nd_scomp_find, nd_scomp_run and the header only; SCOMP0 edits other functions of nd_scomp.c in
parallel. No caller changes, so the picture cannot change.)

## Why (oracle 4 code read, main eacd96ed9)
- nd_scomp.h has only nd_scomp_rec (core 1) and nd_scomp_run(rec). S6E runs entity gaps (static list data) on core 0: it needs a
  call that analyses a def at first use and runs it with its own value units (exec_own) or given ones (exec), s6chk included.
- nd_scomp_find (3ds/src/nd_scomp.c:85-96): 32 slots, linear scan, round-robin replace: entity gaps would thrash it (a cold
  nd_comb_decide per miss). 128 slots (about 40 KB heap, calloc at first run), FNV hash of the key, 8-slot probe.

## Start
`sh 3ds/tools/wt_new.sh SCOMP1`; PM_ROW=SCOMP1 PM_LANE=SCOMP1 on every build and bench call.

## Touch only
3ds/src/nd_scomp.c: nd_scomp_find (:85-96), nd_scomp_run (:280-286). 3ds/include/nd_scomp.h: ND_SC_N, two declarations.

## Exact edits
1. nd_scomp_find: keep its first two lines (the `last` hint); replace the rest of the body with:
   ```
       uint32_t h = 2166136261u; unsigned fr; int fr_ok = 0;
       for (unsigned k = 0; k < ND_SC_KEYW; k++) h = (h ^ key[k]) * 16777619u;
       i = (h ^ (h >> 15)) & (ND_SC_N - 1u); fr = i;
       for (unsigned p = 0; p < 8u; p++) {
           const unsigned j = (i + p) & (ND_SC_N - 1u);
           if (!t->used[j]) { if (!fr_ok) { fr = j; fr_ok = 1; } continue; }
           if (t->key[j][0] == key[0] && !memcmp(t->key[j], key, sizeof t->key[j])) { t->last[def_id & 15u] = (uint8_t)j; *miss = 0; return j; }
       }
       memcpy(t->key[fr], key, sizeof t->key[fr]); /* a miss: the first free probe slot, else the home slot */
       t->used[fr] = 1; t->last[def_id & 15u] = (uint8_t)fr; *miss = 1;
       return fr;
   ```
2. Replace nd_scomp_run with:
   ```
   static int sc_chk = -1;
   static void sc_go(const NdSDef* d, const uint32_t* v) {
       if (__builtin_expect(sc_chk < 0, 0)) sc_chk = pm_debug_has("s6chk") ? 1 : 0;
       if (__builtin_expect(sc_chk, 0)) s6_run(d, v); else sc_exec(d, v);
   }
   void nd_scomp_run(const uint32_t* rec) { sc_go((const NdSDef*)(uintptr_t)rec[0], rec + 2); }
   int nd_scomp_exec(NdSDef* d, const uint32_t* v, unsigned nv) {
       if (__builtin_expect(!d->st, 0)) d->st = nd_scomp_analyse(d) ? 1u : 2u;
       if (d->st != 1u || nv != d->nv) return 0;
       sc_go(d, v);
       return 1;
   }
   int nd_scomp_exec_own(NdSDef* d) {
       uint32_t v[2u * 32u]; const uint32_t* u = (const uint32_t*)d->u; unsigned j = 0;
       if (__builtin_expect(!d->st, 0)) d->st = nd_scomp_analyse(d) ? 1u : 2u;
       if (d->st != 1u) return 0;
       for (unsigned k = 0; k < d->n; k++) if (d->vmask >> k & 1u) { v[2u * j] = u[2u * k]; v[2u * j + 1u] = u[2u * k + 1u]; j++; }
       return nd_scomp_exec(d, v, j);
   }
   ```
3. nd_scomp.h: `#define ND_SC_N 32u` -> `128u`. After the nd_scomp_run declaration add
   `int nd_scomp_exec(NdSDef* d, const uint32_t* v, unsigned nv); /* core 0: run d with nv value units; 0 = refused (run the ops) */`
   and `int nd_scomp_exec_own(NdSDef* d); /* core 0: run d whose value units are its own (static list data); 0 = refused */`.

## Gate (expected values)
- `PM_ROW=SCOMP1 PM_LANE=SCOMP1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row SCOMP1 --why "SCOMP1 core 0 entries"`: PASS, `SCOMP TEST PASS`.
- `PM_ROW=SCOMP1 PM_LANE=SCOMP1 python 3ds/tools/bench.py base spot_mac01_det`; `python 3ds/tools/bench.py run
  spot_mac01_det,s6chk_mac01 --row SCOMP1 --why "SCOMP1: no caller change"` (2 starts); `bench.py cmp spot_mac01_det --row
  SCOMP1`: every shot byte-equal; s6chk `bad 0`. Budget 4 starts.

## Stop
A FAIL line, a byte difference, `bad` above 0, an edit that does not fit, or a test-tool fault: commit, hand back the output.

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate values, the memcheck line.
