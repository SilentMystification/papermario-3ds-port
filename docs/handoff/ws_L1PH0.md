# ws_L1PH0 (Haiku): L1 misses per draw class and the unit run census (plan_l1.md section 4)
Designer: oracle L1 (plan_l1.md). Worktree build3ds/wt/L1PH0, branch L1PH0. PM_ROW=L1PH0 PM_LANE=L1PH0. Start: main dfb703014f or later.
Objective: with switch profph, each prof window prints PMN0 and PMN1 (set m: I miss, D read miss) per NDT class of the sampled steps, the sampled total, and the length of the setter unit runs. Counter: none moves (debug lines). The console capture after the merge ranks the L1 rows (plan_l1.md 5).
Line numbers are main dfb703014f. Grep each anchor again. Edit with the Edit tool (CRLF sources).
## Steps (one commit per step)
1. `L1PH0 1: class counters`. 3ds/src/pm_gpu.c. After :1300 `static int ndt_mdl;` add:
   `int pm_ndt_pmu; /* L1PH0: switch profph; the class stack also sums PMN0, PMN1, exclusive as the ticks */`
   `static uint32_t ndt_e0[4][2], ndt_ei[4][2]; static uint64_t ndt_es[NDT_N][2]; static unsigned ndt_en[NDT_N];`
   `static __attribute__((cold, noinline)) void ndt_ev(uint32_t v[2]) { u64 a = 0, b = 0; svcControlPerformanceCounter(&a, PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_0, 0); svcControlPerformanceCounter(&b, PERFCOUNTEROP_GET_VALUE, PERFCOUNTERREG_CORE_COUNT_REG_1, 0); v[0] = (uint32_t)a; v[1] = (uint32_t)b; }`
   Push :1302 before: `if (ndt_sp < 4u) { ndt_t0[ndt_sp] = ...; ndt_a[ndt_sp] = nd_altu_n; }`. After: the same, and `if (pm_ndt_pmu) { ndt_ev(ndt_e0[ndt_sp]); ndt_ei[ndt_sp][0] = ndt_ei[ndt_sp][1] = 0u; }` before its closing `}`.
   Pop: insert before :1312 `if (i) ndt_in[i - 1] += tot; ...`:
   `if (pm_ndt_pmu) { uint32_t v[2]; ndt_ev(v); ndt_en[k]++; for (int j = 0; j < 2; j++) { const uint32_t t = v[j] - ndt_e0[i][j], c = ndt_ei[i][j]; ndt_es[k][j] += t > c ? t - c : 0u; if (i) ndt_ei[i - 1][j] += t; } } /* L1PH0 */`
   After :1322 (pm_gpu_ndt_take) add `void pm_gpu_ndt_take_e(uint64_t o[NDT_N][2], unsigned n[NDT_N]) { memcpy(o, ndt_es, sizeof ndt_es); memcpy(n, ndt_en, sizeof ndt_en); memset(ndt_es, 0, sizeof ndt_es); memset(ndt_en, 0, sizeof ndt_en); } /* L1PH0 */`
   and its prototype after 3ds/include/nd.h:468.
2. `L1PH0 2: run census`. 3ds/src/nd_rec.c. Delete :610 `static unsigned char rnr_d; unsigned nd_rnr_n; /* PROF2 ... */`. Replace :215 with:
   `static unsigned char rnr_d; unsigned nd_rnr_n; /* PROF2: nd_rec_nr depth and calls (L1PH0: moved up) */`
   `unsigned nd_uqr[7], nd_uqc; static unsigned uq_run; /* L1PH0: units of the runs by length 1, 2-3, 4-7, 8-15, 16-31, 32-63, 64+, and the run ends. A run = depth-0 nd_rec_nr calls between two depth-0 rec_open calls */`
   `static __attribute__((noinline)) void uq_cut(void) { const unsigned b = 31u - (unsigned)__builtin_clz(uq_run); nd_uqr[b < 6u ? b : 6u] += uq_run; nd_uqc++; uq_run = 0; }`
   `static void rec_open(void) { if (uq_run && !rnr_d) uq_cut(); task_open(); } /* DEL3C1; L1PH0: an entry ends the unit run */`
   :613 before: `... pm_gpu_ndt_push(); rnr_d++; nd_rnr_n++; /* PROF2 */`. After: `... pm_gpu_ndt_push(); uq_run += !rnr_d; rnr_d++; nd_rnr_n++; /* PROF2; L1PH0 */`.
   3ds/src/nd_xf.c: above :29 add `unsigned nd_xf_n; /* L1PH0: transform records */`. In xf_rec :31 put `nd_xf_n++;` first.
   3ds/src/nd_gtab.c: above :148 add `unsigned nd_dl_n; /* L1PH0: list draws */`. nd_rec_dl_e body starts with `nd_dl_n++;`.
3. `L1PH0 3: lines`. 3ds/src/pm_phase.c. After :333 add `static unsigned long long pf_sm[2]; static u64 pf_sa[3]; /* L1PH0: PMN0, PMN1 of the sampled frames; the counters at the arm */`.
   After :486 add `extern unsigned nd_uqr[7], nd_uqc, nd_xf_n, nd_dl_n; static unsigned uq_m[7], uqc_m, xf_m, dl_m; /* L1PH0 */`. In pf_lg_mark :489 before the last `}` add `memcpy(uq_m, nd_uqr, sizeof uq_m); uqc_m = nd_uqc; xf_m = nd_xf_n; dl_m = nd_dl_n;`.
   :493 after the take add `{ static uint64_t e_[NDT_N][2]; static unsigned n_[NDT_N]; pm_gpu_ndt_take_e(e_, n_); } pf_sm[0] = pf_sm[1] = 0;`.
   :628 after `pm_gpu_ndt_take(ct);` add `static uint64_t l1_e[NDT_N][2], l1_e2[NDT_N][2]; static unsigned l1_n[NDT_N], l1_n2[NDT_N]; pm_gpu_ndt_take_e(l1_e, l1_n); /* L1PH0 */`. :650 after `pm_gpu_ndt_take(c2);` add `pm_gpu_ndt_take_e(l1_e2, l1_n2);`.
   After :654 (the c1d line) add, if pf_ph_pmu: a name table `static const char* const nm_[NDT_N]` in nd.h:451-453 enum order ("rec" ... "semit", 39 names) with `_Static_assert(sizeof nm_ / sizeof nm_[0] == NDT_N, "L1PH0")`. For each class with l1_n[k] != 0 take name, l1_e[k][0] / s, l1_e[k][1] / s, l1_n[k] / s (s = pf_s, 1 if 0). Print 3 classes per line, fill a short last line with "-" 0 0 0:
   `pm_log_prof_line("prof f%u w%u l1m set %c %s %u %u %u %s %u %u %u %s %u %u %u", pf_total, pf_w, st, ...)` with st = pf_rot ? pf_sets[pf_si].c : pf_set. Then
   `pm_log_prof_line("prof f%u w%u l1t set %c s %u e0 %u e1 %u cal %u %u", pf_total, pf_w, st, pf_s, pf_sm[0] / s, pf_sm[1] / s, l1_e2[NDT_ALT][0] >> 4, l1_e2[NDT_ALT][1] >> 4)` (casts to unsigned).
   After :805, if pf_ph_pmu, d = dn (1 if 0): `pm_log_prof_line("prof f%u w%u l1q u %u cut %u xf %u dl %u r %u %u %u %u %u %u %u", ...)`: r[k] = (nd_uqr[k] - uq_m[k]) / d, u = sum of r, cut = (nd_uqc - uqc_m) / d, xf, dl the same way.
   :810 after `pf_pmu_read(pf_base);` add `if (pf_samp) memcpy(pf_sa, pf_base, sizeof pf_sa);`. :819 after `pf_pmu_read(c);` add `memcpy(pf_sa, c, sizeof pf_sa);`.
   :860 before: `pf_pmu_read(pf_base); pf_run = 0;`. After: `pf_pmu_read(pf_base); pf_sm[0] += (uint32_t)(pf_base[0] - pf_sa[0]); pf_sm[1] += (uint32_t)(pf_base[1] - pf_sa[1]); pf_run = 0;`.
   :932 after `pf_ph_pmu = ...;` add `{ extern int pm_ndt_pmu; pm_ndt_pmu = pf_ph_pmu; } /* L1PH0 */`.
## Tests (output to build3ds/out/, read the last 20 lines). Gate: 1 build, 2 Azahar runs. Budget 8 Azahar starts.
1. `PM_LANE=L1PH0 sh 3ds/build.sh`: memcheck OK. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `python 3ds/tools/c0gate.py <elf>` 0 / 0 / 0.
2. `bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_drec --row L1PH0 --why "L1PH0 gate"`, `bench.py cmp spot_mac01_drec --row L1PH0`: byte-equal (C: no change).
3. `python 3ds/tools/capture_check.py --row L1PH0 --proof "l1m,l1t,l1q" warp=mac_01:0 pos=0:0:200 profdisk=1 profph profrot`: CAPTURE CHECK OK. Pass: l1m pushes > 0 for rnr, rmdl, snap. l1q u > 0. l1q u x (core2 n) <= c1d n of the same window.
4. Step 4 (measure, no gate): `PM_FNCOV=1 PM_LANE=L1PH0_fncov sh 3ds/build.sh`, `bench.py run fncov_mac01_det --row L1PH0 --why "L1 M3"` with PM_LANE=L1PH0_fncov, `PM_LANE=L1PH0 python 3ds/tools/hotset.py --fncov <log>`. Keep the `hotset M3` lines.
Fix rights: compile errors, -Werror warnings, a local name that clashes in pf_report (rename the new one), a log line over 16 words (split it), a mechanical fault of a tool command. Any DIFF in test 2: stop, send the cmp output path to the oracle.
Stop: the commit `L1PH0: gate` deletes this page. Hand back with ONE SubagentHandback: commit hashes, host, c0gate, cmp line, the l1q line, the l1m and l1t lines of one window, the M3 lines.
