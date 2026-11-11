# Row PMUPH: counter events per phase in the prof samples (switch profph) and the ranked PMU report

(Model: Haiku. Waits: PMUROT merged (same file pm_phase.c). Branch from main after it. Edits pm_phase.c pm_ph_push, pm_ph_pop,
pf_arm, pf_report, pf_reset; new 3ds/tools/pmu_report.py. Reads only this file and CLAUDE.md.)

## Facts (oracle code read, main 81c729091)
- The counters are window totals: no count is split by code. The phase stack (pm_phase.c pm_ph_push :813, pm_ph_pop :822, phases
  PH_* pm_port.h:87: logic, dl, gbi, gpu, tex, audio, rom, yay0, fbegin, fend, idle and the g_* draw parts) times one sampled frame in
  30 (pf_arm :652-669 sets pm_ph_on; the game thread, core 0; the sample step runs inline). The ph line prints ms per phase (:540).
- A counter read is one svc per register (pf_pmu_read :325, cost on the `prof cal` line). Two reads per push or pop in sampled frames
  only keep the clean frames free; the cost shows in the existing oh and amort fields.

## Design
With profph (and prof): at each push and pop of a sampled frame read PMN0 and PMN1 (not CCNT: the ticks already give time) and add
the delta to the phase being left, as acc[] does for ticks. Window sums per phase; one line per window with the 6 largest phases by e0.

## Start
`sh 3ds/tools/wt_new.sh PMUPH`. PM_ROW=PMUPH PM_LANE=PMUPH on every build and bench call.

## Exact edits
1. pm_phase.c: `static int pf_ph_pmu; static u64 pph_last[2]; static unsigned long long pph[2][PH_N];` pf_init: pf_ph_pmu =
   pm_debug_has("profph"). A helper `static void pph_add(int ph)`: two GET_VALUE svc (REG_0, REG_1) into v[2]; pph[k][ph] += (u32)(v[k] -
   pph_last[k]); pph_last = v. pm_ph_push and pm_ph_pop: after `acc[cur] += now - last;` add `if (pf_ph_pmu && pf_samp) pph_add(cur);`.
   pf_arm: after `pm_ph_on = 1;` read pph_last (the same two svc). pf_reset: clear pph.
2. pf_report after the pmx line: `prof f%u w%u pmxph set %c` followed by up to 6 ` <phase name> <e0> <e1>` pairs (pname[], the phases with
   the largest e0 sum, values per sampled frame: sum / pf_s), one line (keep it under 160 characters: names are short).
3. New 3ds/tools/pmu_report.py <log> [--label L]: reads the prof lines (reuse prof_by_scene.parse_windows), groups windows by pmx set,
   averages per set: core 0 and core 1 e0, e1, cyc; derived: CPI = cyc / inst (set b), I-miss and D-read-miss per 1000 instructions
   (needs a set-b window of the same scene: use the run average), istall and dstall share of cyc (set s), bmiss per 1000 inst; and from
   pmxph the per-phase share of e0 and e1. Writes build3ds/out/pmu_<label>.txt: one table per core, then phases ranked by each event.
   First line: `console values only; Azahar does not count`.

## Gate (CLAUDE.md 2; Azahar budget 3 of 8)
- Build, memcheck OK; `bench.py run nd_host --row PMUPH --why "pm_phase.c"`: PASS.
- `bench.py base spot_mac01_det`; one list `bench.py run spot_mac01_det,pmu_rot_az --row PMUPH --why "byte-equal; pmxph lines"`
  (pmu_rot_az from PMUROT; add `profph` to its debug string in this row); `bench.py cmp spot_mac01_det --row PMUPH`: 6/6 byte-equal;
  the pmu_rot_az LOG: copy has pmxph lines; `python 3ds/tools/pmu_report.py <that log>` writes the table without an error.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line, table path.
