# Row PMUROT: rotate the ARM11 counter sets per prof window (switch profrot), both cores

(Model: Haiku. Base: main after the FXCEN1 merge. Edits pm_phase.c, pm_logic.c lg_main (the PMU2 lines near :65-76 only, not the
CRASHEXIT2 wait loops), 3ds/tools/bench.py CORE_KEYS, azt_scenarios.json and bench_defs.json (append). Reads this file and CLAUDE.md.)

## Facts (oracle code read)
- Without profrot nothing changes: prof programs set m (INST_CACHE_MISS + DATA_CACHE_READ_MISS), or with switch profstall the old pair
  STALL_BY_LACK_OF_INST + DATA_CACHE_READ_MISS (pm_phase.c pm_pf_core_setup :706-715, pf_set :744), and prints the old line
  `prof .. pmu <m|s> e0 e1 cyc .. pm ..` (:545) that prof_by_scene.fields reads as pmu_i, pmu_d, pmu_cyc, pmu_stall.
- Core 0 counts clean frames per 300-frame window (pf_arm :665-668); core 1 counts per logic step (pm_logic.c :65-76, `core2 .. ic1 dc1 cyc1`).
- Event names: PERFCOUNTEREVT_CORE_<name> of libctru include/3ds/svc.h. The header has: INST_CACHE_MISS, STALL_BY_LACK_OF_INST,
  STALL_BY_DATA_HAZARD, INST_MICRO_TLB_MISS, DATA_MICRO_TLB_MISS, BRANCH_INST, BRANCH_NOT_PREDICTED, BRANCH_MISS_PREDICTED,
  INST_EXECUTED, FOLDED_INST_EXECUTED, DATA_CACHE_READ, DATA_CACHE_READ_MISS, DATA_CACHE_WRITE, DATA_CACHE_WRITE_MISS,
  DATA_CACHE_LINE_EVICTION, PC_CHANGED, MAIN_TLB_MISS, EXTERNAL_REQUEST, STALL_BY_LSU_FULL, STORE_BUFFER_DRAIN, MERGE_IN_STORE_BUFFER.

## Design: with profrot (and prof) the set rotates at each window end over 6 sets; both cores use the window's set
| set | e0 (name) | e1 (name) | answers |
| m | INST_CACHE_MISS (imiss) | DATA_CACHE_READ_MISS (drmiss) | does the hot code fit the 16 KB I-cache; data locality |
| s | STALL_BY_LACK_OF_INST (istall) | STALL_BY_DATA_HAZARD (dstall) | cycles lost to fetch and to loads (share of cyc) |
| b | INST_EXECUTED (inst) | BRANCH_MISS_PREDICTED (bmiss) | CPI = cyc / inst; branch cost |
| w | DATA_CACHE_WRITE_MISS (dwmiss) | DATA_CACHE_LINE_EVICTION (devict) | write traffic and cache pressure |
| q | STALL_BY_LSU_FULL (lsufull) | STORE_BUFFER_DRAIN (sbdrain) | write-buffer stalls (command buffer, copies) |
| t | MAIN_TLB_MISS (tlb) | EXTERNAL_REQUEST (extreq) | page spread of hot code and data; bus traffic |

## Start
`sh 3ds/tools/wt_new.sh PMUROT`. PM_ROW=PMUROT PM_LANE=PMUROT on every build and bench call.

## Exact edits
1. pm_phase.c: `typedef struct { char c; u32 e0, e1; const char* n0; const char* n1; } PfSet;` and `static const PfSet pf_sets[6]` with the
   rows above (PERFCOUNTEREVT_CORE_<name>); `static int pf_si, pf_rot;` and `volatile unsigned pm_pf_set_gen;`.
   pf_init (:744): `pf_rot = pm_debug_has("profrot");` after the pf_set line.
   pm_pf_core_setup: `u32 ev0, ev1;` when pf_rot: pf_sets[pf_si].e0 and .e1; else the old pair (ev0 by pf_set as now, ev1 DATA_CACHE_READ_MISS).
   pf_report: when pf_rot, after the old pmu line position print instead of it
   `prof f%u w%u pmx set %c %s %u %s %u cyc %u` (pf_sets[pf_si].c, n0, e0, n1, e1, cy); when !pf_rot print the old pmu line as now.
2. pf_report after the core2 line (:612), when pf_rot: `prof f%u w%u pmx1 set %c c1_%s %u c1_%s %u c1_cyc %u` (n0, ic1, n1, dc1, cy1).
   At the end of pf_report, before pf_reset, when pf_rot: `pf_si = (pf_si + 1) % 6; pm_pf_core_setup(); pf_pmu_read(pf_base); pm_pf_set_gen++;`.
   Core 1 changes set at its next logic step, so its first step of a window may count the old set (1 step of 300).
3. pm_logic.c: beside the prototype `void pm_pf_core_setup(void);` (:39) add `extern volatile unsigned pm_pf_set_gen;`. In lg_main beside
   lg_pmu_on add `static unsigned lg_pmu_gen;` and replace `if (pf && !lg_pmu_on) { pm_pf_core_setup(); lg_pmu_on = 1; }` by
   `if (pf && (!lg_pmu_on || lg_pmu_gen != pm_pf_set_gen)) { pm_pf_core_setup(); lg_pmu_on = 1; lg_pmu_gen = pm_pf_set_gen; } /* PMUROT */`.
4. bench.py CORE_KEYS (:336): add "imiss", "drmiss", "istall", "dstall", "inst", "bmiss", "dwmiss", "devict", "lsufull", "sbdrain", "tlb",
   "extreq", "cyc" and each of these 13 with the prefix "c1_". (prof_by_scene.fields makes a field of every name followed by a number.)
5. bench_defs.json: console bench pmu_rot_prof (console_prof, host o3ds, recipe `warp=mac_01:0 pos=0:0:200 profrot`, cap 240, until
   `prof f3[0-9]{3} w300`). Scenario and bench pmu_rot_az (azahar_stream, normal, form of kzn04_shadestat): debug
   `warp=mac_01:0 pos=0:0:200 prof=30 profrot loglevel=debug`, until `prof f(6|7)\\d\\d w\\d+ pmx set`, cap 45, deadline 40. `bench.py rebuild`.

## Gate (CLAUDE.md 2; Azahar budget 3 of 8)
- Build, memcheck OK; `bench.py run nd_host --row PMUROT --why "pmu sets"`: PASS.
- `bench.py base spot_mac01_det`; one list `bench.py run spot_mac01_det,pmu_rot_az --row PMUROT --why "byte-equal; rotation lines"`;
  `bench.py cmp spot_mac01_det --row PMUROT`: 6/6 byte-equal. The pmu_rot_az LOG: copy has two pmx lines with different set letters and
  two pmx1 lines. Values may be 0 on Azahar (it is not the console): only the lines are the gate.

## Stop and end
A failed gate, a name not in the list above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line.
