# agent_nds_s1: S1 NDCORE handoff (2026-11-04)

## State
- Branch nds1, HEAD 3cdd0b542 at start. No S1 code written. S1-1 STOPPED at the anchor check.
- Plan rule (plan_native_slices.md, row format): an anchor that moved more than 40 lines stops the row. The planner re-anchors it.
- Native host test (sh 3ds/tools/nat_host_test.sh) passes on the baseline: template, order, layout, eye, bake, tex_id.
- nd_host_test.sh fails by design (nd_core.c missing, row S1-2).

## Anchor check (grep, 2026-11-04)
| Item | Plan line | Now | Move | Result |
|---|---|---|---|---|
| S1-1.1 pm_dlc.inc.c dlc_learn 565-573, call 639 | 565-573, 639 | 565-573, 639 | 0 | ok |
| S1-1.2 pm_gbi.c dlc_learn_begin/end | 3639 | 3647-3664 | +9 | ok |
| S1-1.2 pm_gbi.c rec_try | 3662 | 3671 | +9 | ok |
| S1-1.2 pm_gbi.c rec_done | 3765 | 3774 | +9 | ok |
| S1-1.3 pm_gpu.c REC_IDX_N arena | 2548-2560 | 2628-2670 | +80 | STOP, re-anchor |
| S1-1.3 pm_gpu.c nat_pool_set call | 2947-2952 | 3032 (in pm_gpu_nat_pool_init 3028) | +81 | STOP, re-anchor |
| S1-1.3 pm_gpu.c pm_gpu_rec_play | not given | 2748 | | add to the row |
| S1-1.4 pm_native.c nat_tmpl_strip | not given | 61 | | add to the row |
| S1-1.4 pm_native.c nat_rec_write | not given | 180 | | add to the row |
| pm_native.c nat_order, nat_eye_copy | not given | 127, 173 | | keep |

Cause of the +80: S0 added code to pm_gpu.c (S0-1 ndref hook and ndref_draw, S0-6 busy probe).

## Findings for the planner
1. S1-1 item 2 names only the nat_tmpl_strip calls in rec_try and rec_done. The replay path is wider: rec_play (pm_gbi.c 3861), the G_DL call (3945), the G_PM_MODEL call sites (4027-4030), rec_report (3843, called at 2321), and the record helpers 3555-3860. The row must name the full range.
2. pm_dlc.inc.c uses rec_on, rec_dln, rec_gin, rec_gout, rec_gdl and rec_fl at 593, 599, 606, 639, 658. These are capture-only. They go in S1-1.
3. The PMOD bake (nat_pool_put, nat_model_bake, nat_seg_add, nat_entry_state; pm_dlc.inc.c 268-369) is not named by any S1 row. It is the DLC2 static compile, not the replay. Decide keep or delete.
4. nat_host_test.c calls neither nat_tmpl_strip nor nat_rec_write. The native host test has no capture test to remove, so it stays green after S1-1 with no red step.

## Next
1. DONE (planner, 2026-11-04): S1 to S11 re-anchored by symbol at 115a06200 (plan_native_slices.md section 2, "Anchor form"). S1-1 names the full capture and replay range (findings 1, 2).
2. DONE: PMOD bake deleted in S1-1 item 2 and item 6 (finding 3). Reason: it reads walker state, and no draw uses it.
3. Code agent starts S1-1 on branch nds1, one commit for the row. A line shift is not a stop (PNS 12, anchor rule).

## Commands
- Native host test: `sh 3ds/tools/nat_host_test.sh`
- Host ND tests (docker pm3ds-host:1): `sh 3ds/tools/nd_host_test.sh`
- Build: `python 3ds/tools/gen_map_restore.py`, then `PM_ROW=S1-1 sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from the user copy. Never commit that file.
- Azahar census (S1-1 gate): `PM_ROW=S1-1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_mac01 -Row S1-1`. Write the justification line to 3ds/logs/perf/status_NDS_S1.txt first.
