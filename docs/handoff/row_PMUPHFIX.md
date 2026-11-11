# Row PMUPHFIX: pmxph phase counts: only the sampling thread counts, and the last phase of the sample is added

(Model: Haiku. Base: main. Edits 3ds/src/pm_phase.c: pph_add, pf_arm, pm_pf_frame (the sample end branch). Disjoint from EDGEAA,
TILEWIN2, SEAMHOST, DPCEN1. Reads this file and CLAUDE.md.)

## User fact (console run fxcen_pmu_o3ds.txt, pmu_report.py)
Phase `other` gets 393,697,467 e0 per sampled frame (100 %), more than the whole frame's I-miss (about 54,659); the other phases are tiny.

## Cause (oracle code read, main 0639e7a0e)
- pm_ph_push / pm_ph_pop (pm_phase.c:880-896) run on every thread except the audio threads (`pm_on_audio()` only), and add a PMU delta
  in sampled frames: `pph_add(cur)` (:884, :894) -> pph_read (two GET_VALUE svc) and `pph[k][ph] += (u32)(v[k] - pph_last[k])` (:355).
  The CP15 PMU is banked per core (pm_pf_core_setup comment, :704): a push or pop on another core (the logic thread on core 1, a loader
  or prefetch thread) reads that core's counters against pph_last of core 0, so the u32 difference wraps to about 4e9 and lands in the
  phase that thread is in (`other` for most). The tick accounting (acc[]) has no such wrap: the system tick is one clock.
- The last segment of a sample (from the last push or pop to the frame end) is never added: pm_pf_frame ends the sample
  (`pf_samp = 0`, :738) without a pph_add.

## Start
`sh 3ds/tools/wt_new.sh PMUPHFIX`. PM_ROW=PMUPHFIX PM_LANE=PMUPHFIX on every build and bench call.

## Exact edits (3ds/src/pm_phase.c)
1. Beside pph_last: `static Thread pph_th; /* PMUPHFIX: the thread that armed the sample (game thread, core 0) */`.
2. pf_arm, at `if (pf_ph_pmu) pph_read(pph_last);` (:710): `if (pf_ph_pmu) { pph_th = threadGetCurrent(); pph_read(pph_last); }`.
3. pph_add first line: `if (threadGetCurrent() != pph_th) return; /* PMUPHFIX: another core's PMU is another counter set */`.
4. pm_pf_frame, in the sample end branch (`if (cur_s) {`, :736) before `pf_samp = 0;`: `if (pf_ph_pmu) pph_add(cur); /* PMUPHFIX: the last phase of the sample */`.

## Gate (CLAUDE.md 2; Azahar budget 3 of 8)
- Build, memcheck OK; `bench.py run nd_host --row PMUPHFIX --why "pph per thread"`: PASS.
- `bench.py base spot_mac01_det`; one list `bench.py run spot_mac01_det,pmu_rot_az --row PMUPHFIX --why "byte-equal; pmxph lines"`;
  `bench.py cmp spot_mac01_det --row PMUPHFIX`: 6/6 byte-equal; pmu_rot_az has pmxph a and b lines (Azahar values may be 0).
- Console check for the user (the same push as fxcen_pmu_o3ds.txt): in pmu_report.py the phase sum of e0 is at most the frame's e0.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line.
