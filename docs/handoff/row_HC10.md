# Row HC10: regenerate the hot link order from a fresh census (plan_hotcode HC10). Starts after HCCUT2, HCCUT3, HCCUT4, HCCUT5 merge.

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts
- 3ds/ld/hot_order.ld was made by HOTORD (094ce15dc) from the fncov census with 3ds/tools/hot_order.py; since then HC4e4, HC6, HC6B,
  KZN04 and HCCUT1-5 moved functions to .text.unlikely, inlined or cloned them (load_impl.constprop.0). hotset_size fails on a name
  the map lost (stale) and the order no longer follows what runs (M3CEN 29bfe4297).
- The census benches: fncov_mac01_det, fncov_kmr10_det, fncov_mac00_det (lane <L>_fncov, PM_FNCOV=1; no base, never a byte gate).
- The word-path deletions of HC10 in plan_hotcode (NDC_SPR/NDC_ENT fallback words) depend on DIRECT D11 (b): not in this row.
## Start
`sh 3ds/tools/wt_new.sh HC10` from main after the four rows above merged; PM_ROW=HC10 PM_LANE=HC10.
## Steps
1. `PM_ROW=HC10 PM_LANE=HC10 PM_FNCOV=1 sh 3ds/build.sh > build3ds/out/build_fncov.txt 2>&1`: rc 0.
2. One list: `PM_ROW=HC10 python 3ds/tools/bench.py run fncov_mac01_det,fncov_kmr10_det,fncov_mac00_det --row HC10 --lane HC10_fncov --why "census for the hot order"`.
   If azt_scenarios.json gets an "unverified" mark: `git checkout 3ds/tools/azt_scenarios.json`. Copy the three logs to build3ds/out/.
3. `python 3ds/tools/hot_order.py --help`, then run it on the three logs as HOTORD did (094ce15dc message names the command) to write
   3ds/ld/hot_order.ld. In 3ds/tools/hotset.json keep only roots that exist in the map (hotset prints "roots missing": delete those).
4. `PM_ROW=HC10 PM_LANE=HC10 sh 3ds/build.sh > build3ds/out/build.txt 2>&1` (normal lane): memcheck OK.
## Gate (CLAUDE.md 2)
- `bench.py run hotset_size --row HC10 --lane HC10 --why "HC10 order"`: rc 0, stale 0, no roots missing; write M1-hot per class.
- `bench.py run nd_host --row HC10 --why "HC10"`: PASS.
- `bench.py base` mdl5_iwa10_det and spot_mac01_det; one `run mdl5_iwa10_det,spot_mac01_det`; `cmp` each: 6/6 (link order only).
- Console (user): prof push mac_01 0 0 200 (pmu_i, cycles): the orchestrator parks it in row_ORACLE_NEXT.md.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
