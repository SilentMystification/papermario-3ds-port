# Row PMUROT: rotate the ARM11 counter sets per prof window (switch profrot), both cores

(Model: Haiku. Base: main 81c729091. Edits pm_phase.c, pm_logic.c lg_main (the PMU2 lines :65-76 only, not the CRASHEXIT2 wait loops),
3ds/tools/bench.py CORE_KEYS, bench_defs.json (append). Disjoint from FXCEN1, CRASHEXIT2, LIBTHR, MAPROMFS. Reads this file and CLAUDE.md.)

## Facts (oracle code read)
- Prof mode programs two events and the cycle counter per core (pm_phase.c pm_pf_core_setup :706-715): set m = INST_CACHE_MISS +
  DATA_CACHE_READ_MISS, or set s (switch profstall, :744) = STALL_BY_LACK_OF_INST + DATA_CACHE_READ_MISS. Core 0 counts clean frames
  per 300-frame window (pf_arm :665-668, line `prof .. pmu <m|s> e0 e1 cyc .. pm ..` :545); core 1 counts per logic step (pm_logic.c
  :65-76, `core2 .. ic1 dc1 cyc1`). Switch pmu (:242-281, log on, no prof) alternates I-miss/D-miss with main TLB miss/inst stall.
- Not used: data hazard stalls, instructions executed (CPI), branch mispredicts, D write misses, D line writebacks, external requests,
  micro TLB misses. One run gives one set only; no line names the events of e0 and e1.

## Design
Switch profrot (with prof): the set index rotates at each window end over 5 sets; both cores use the window's set; a new line names it.
| set | e0 | e1 | answers |
| m | CORE_INST_CACHE_MISS | CORE_DATA_CACHE_READ_MISS | does the hot code fit the 16 KB I-cache; data locality |
| s | CORE_STALL_BY_LACK_OF_INST | CORE_STALL_BY_DATA_HAZARD | cycles lost to fetch and to loads (share of cyc) |
| b | CORE_INST_EXECUTED | CORE_BRANCH_MISS_PREDICTED | CPI = cyc / inst; dispatch and loop branch cost |
| w | CORE_DATA_CACHE_WRITE_MISS | CORE_DATA_CACHE_LINE_WRITEBACK | write traffic (command buffer, copies) |
| t | CORE_MAIN_TLB_MISS | CORE_EXTERNAL_REQUEST | page spread of hot code and data; bus traffic |
Names: PERFCOUNTEREVT_<name> of libctru include/3ds/svc.h. A missing name: stop, hand back the enum list of that header.

## Start
`sh 3ds/tools/wt_new.sh PMUROT`. PM_ROW=PMUROT PM_LANE=PMUROT on every build and bench call.

## Exact edits
1. pm_phase.c: a static table `pf_sets[5] = { {'m', ev0, ev1, "imiss", "drmiss"}, ... }` as above; `static int pf_si;` (index), `static int pf_rot;`
   (pm_debug_has("profrot") in pf_init next to the pf_set line :744; profstall keeps set s as the start index). `volatile unsigned pm_pf_set_gen;`.
   pm_pf_core_setup: ev0/ev1 from pf_sets[pf_si]. pf_set becomes pf_sets[pf_si].c.
   pf_report, after the `pmu` line (:545): print `prof f%u w%u pmx set %c c0 %s %u %s %u cyc %u` (e0, e1, cy per clean frame, the set's
   two names); print the old `pmu` line only for sets m and s (its readers know only those). At the end of pf_report (before pf_reset):
   if pf_rot: pf_si = (pf_si + 1) % 5; pm_pf_core_setup(); pf_pmu_read(pf_base); pm_pf_set_gen++.
2. pf_report core2 line (:612): after it print `prof f%u w%u pmx1 set %c c1 %s %u %s %u cyc %u` with ic1, dc1, cy1 and the same names.
   Note: core 1's first steps of a window may still count the old set until its next step (one step of 300 frames).
3. pm_logic.c lg_main: `static unsigned lg_pmu_gen;` beside lg_pmu_on; replace `if (pf && !lg_pmu_on) { pm_pf_core_setup(); lg_pmu_on = 1; }`
   by `if (pf && (!lg_pmu_on || lg_pmu_gen != pm_pf_set_gen)) { pm_pf_core_setup(); lg_pmu_on = 1; lg_pmu_gen = pm_pf_set_gen; } /* PMUROT */`
   and declare `extern volatile unsigned pm_pf_set_gen;` beside the pm_pf_core_setup prototype (:39).
4. The pmx1 line prints its names with a `c1_` prefix (`c1_imiss`, ..., `c1_cyc`) so the generic parser (prof_by_scene.fields: every
   name followed by a number) keeps both cores. bench.py CORE_KEYS (:336): add "imiss", "drmiss", "istall", "dstall", "inst", "bmiss",
   "dwmiss", "dwb", "tlb", "extreq", "cyc" and the same ten plus "cyc" with the c1_ prefix.
5. bench_defs.json: console bench pmu_rot_prof (console_prof, host o3ds, recipe `warp=mac_01:0 pos=0:0:200 profrot`, cap 240, until
   `prof f3[0-9]{3} w300`). Scenario and bench pmu_rot_az (azahar_stream, normal, form of kzn04_shadestat): debug
   `warp=mac_01:0 pos=0:0:200 prof=30 profrot loglevel=debug`, until `prof f(6|7)\\d\\d w\\d+ pmx set`, cap 45, deadline 40. `bench.py rebuild`.

## Gate (CLAUDE.md 2; Azahar budget 3 of 8)
- Build, memcheck OK; `bench.py run nd_host --row PMUROT --why "pmu sets"`: PASS.
- `bench.py base spot_mac01_det`; one list `bench.py run spot_mac01_det,pmu_rot_az --row PMUROT --why "byte-equal; rotation lines"`;
  `bench.py cmp spot_mac01_det --row PMUROT`: 6/6 byte-equal. The pmu_rot_az LOG: copy has two pmx lines with different set letters
  and two pmx1 lines. Values may be 0 on Azahar (it is not the console): only the lines are the gate.

## Stop and end
A failed gate, a missing event name, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the
completing commit. Hand back the hash, gates, memcheck line.
