# Row HC9C: the cached model draw (NdNatC) gets a 64-byte hit head (plan_hotcode HC9, data layout; byte-equal)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (oracle code read, main 012ab5762)
- 3ds/include/nd.h:391-392 `typedef struct __attribute__((aligned(32))) NdNatC { NdDraw d; const uint16_t* sd; unsigned nidx; float zn;
  uint8_t zp; int8_t mix_st; NdTmplKey tk; uint32_t boff; int bt; uint16_t blen, bgen; NdBlkOff bo; } NdNatC;`
- A cache hit (pm_gpu_model_hit loop and blk_put, pm_gpu.c) reads per draw: zp, zn, mix_st, sd, nidx, bgen, bt, bo, boff, blen and the
  NdDraw hot header (d: the first 64 bytes, nd.h:242-261 PERF2 step 3) plus d.tev_col. Today sd..mix_st sit after the whole NdDraw and
  the block fields after tk: three or four 32-byte lines per draw beyond the NdDraw header. tk is read only when the template ids are
  rebuilt (tgen moved).
- Users by field name only (no offsetof, no sizeof assert on NdNatC; nd_model.c:592 memmoves by sizeof): a field order change is
  byte-equal by construction. mc_pool[NDMC_POOL] (nd_model.c:472) grows by the alignment padding: memcheck shows it.
- The other HC9 items: NdShadow generation ids: not worth it after HC6B (a block hit resets the shadow; the compares run only on the old
  path); mc_keyw/mc_tab one-line probes: no row until a console pmu_d reading shows the probe (parked in row_ORACLE_NEXT.md).
## Start
`sh 3ds/tools/wt_new.sh HC9C`; PM_ROW=HC9C PM_LANE=HC9C. No running row edits nd.h:391-392. `git merge main` before the completing commit.
## Exact edit
1. nd.h:391-392 becomes (same fields, new order, 64-byte alignment):
   `typedef struct __attribute__((aligned(64))) NdNatC {`
   `    uint32_t boff; int bt; uint16_t blen, bgen; NdBlkOff bo; const uint16_t* sd; unsigned nidx; float zn; uint8_t zp; int8_t mix_st; /* HC9C: the hit head (first 64 bytes) */`
   `    NdDraw __attribute__((aligned(64))) d; /* its own 64-byte hot header (PERF2 step 3) on the next line */`
   `    NdTmplKey tk; } NdNatC; /* HC6 step 4 block fields and the hit fields first (HC9C); tk only for a template rebuild */`
   and directly below: `_Static_assert(offsetof(NdNatC, d) == 64, "HC9C: the hit head is one 64-byte line");`
2. Keep the existing comment text of :392 after the struct.
## Gate (CLAUDE.md 2)
- build: memcheck OK (write the static delta of mc_pool). `bench.py run hotset_size --row HC9C --lane HC9C --why "layout"`: rc 0.
- `bench.py run nd_host --row HC9C --why "HC9C"`: PASS.
- `bench.py base` mdl5_iwa10_det and spot_mac01_det; one `run mdl5_iwa10_det,spot_mac01_det`; `cmp` each: 6/6.
- Console (user, parked): prof push mac_01 0 0 200, pmu_d per frame against the last push.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
