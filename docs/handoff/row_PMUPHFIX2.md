# Row PMUPHFIX2: the phase counts re-base after the window rotation resets the counters

(Model: Haiku. Base: main. Edits 3ds/src/pm_phase.c pf_report (the PMUROT rotation block :691-693) only. Disjoint from FILLA0R. Reads
this file and CLAUDE.md.)

## User fact (XL run build 4340, pmu_xl2.txt): phase `other` 322,118,196 per frame over 4 windows; one window wraps to about 1.3e9.

## Cause (oracle code read)
- pm_pf_frame arms a sample in the else branch (`if (pm_pf_arm && --pf_cd == 0) pf_arm();`, pf_arm reads pph_last, pm_phase.c:710) and
  ends the window in the same call (`if (pf_w >= 300) pf_report();`). With profrot the window end resets the core 0 counters
  (pf_report :691-693: `pm_pf_core_setup()` sets PMN0, PMN1 and CCNT to 0, then pf_pmu_read(pf_base)) but not pph_last. When the sample
  was armed in that same call, its first push or pop computes (u32)(v - pph_last) with v near 0 and pph_last large: the wrap of about
  2^32 minus the old count (1.3e9 here). It happens only when the arm frame is the window's last frame, so some runs have none.
- The fix is the cause: re-read pph_last after the reset, as pf_base is.

## Start
`sh 3ds/tools/wt_new.sh PMUPHFIX2`. PM_ROW=PMUPHFIX2 PM_LANE=PMUPHFIX2 on every build and bench call.

## Exact edit (3ds/src/pm_phase.c pf_report, the PMUROT block)
`pf_si = (pf_si + 1) % 6; pm_pf_core_setup(); pf_pmu_read(pf_base); pm_pf_set_gen++;` becomes
`pf_si = (pf_si + 1) % 6; pm_pf_core_setup(); pf_pmu_read(pf_base); if (pf_ph_pmu && pf_samp) pph_read(pph_last); pm_pf_set_gen++; /* PMUPHFIX2: a sample armed in this call counts from the reset */`
(pf_ph_pmu, pf_samp, pph_read and pph_last are file statics above it; if pph_read is defined below pf_report, add its prototype above.)

## Gate (CLAUDE.md 2; Azahar budget 3 of 8)
- Build, memcheck OK; nd_host PASS; `bench.py base spot_mac01_det`; one list `bench.py run spot_mac01_det,pmu_rot_az --row PMUPHFIX2 --why
  "byte-equal; pmxph lines"`; `bench.py cmp spot_mac01_det --row PMUPHFIX2`: 6/6 byte-equal; pmxph a and b lines present.
- The user's console check: in pmu_report.py no phase value above the frame total in any window.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line.
