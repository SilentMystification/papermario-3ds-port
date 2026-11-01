# agent_uc2: UC2 (reccheck, patch-masked compare of a valid record with this frame's words): FAIL, reverted

Plan: docs/handoff/plan_uniform_cut.md row UC2 (and plan_record_reuse.md section 2 `miss`). Base: HEAD 271b205f4 (source d463ff287, UC1).
No commit. Source reverted (git apply -R of the UC2 patch: scratchpad uc2_change.patch). Status lines: 3ds/logs/perf/status_UC2.txt (4 starts).

## Change tried (reverted)
- pm_gbi.c: switch `reccheck` (pm_debug_has). rec_try: on a key hit with state 1, the capture runs again (old path still draws) and rec_chk = 1.
  rec_done: a check returns before the key is written (the record is kept). rec_check: compares with pm_gpu_rec_same. Counters hit, bad, miss on the rec line.
- pm_gpu.c: pm_gpu_rec_same(a, na, b, nb, &bad, &miss). Walks the stored record and this frame's words with pm_gpu_rec_walk. Masks the patch data words
  (split entry: off, off+2, off+3, off+4; else off..off+3). bad: unmasked words that differ, or lengths differ. miss: patch tables differ. pt and pk restored.
- pm_gpu.h: prototype.
- azt_scenarios.json: mac01t_rec debug + `reccheck`; new yard_rec. test_catalog.md: row. All reverted.

## Gate result (window 2 = the f6xx line of mac01t_rec, the f34x line of yard)
| Run | Lane | Scenario | Line | Counters | Shots |
|---|---|---|---|---|---|
| 1 | UC2H (HEAD) | mac01t_rec -Try | f614 cap 4 build 0 emp 4 wbad 0 pt 0 pk 0 mt 4 | n/a | 0000_1_2, 0001_1_2 saved |
| 2 | UC2H (HEAD) | yard -Try | f349 cap 48 build 9 emp 39 wbad 0 pt 72 pk 36 | n/a | 0000_0_11 saved |
| 3 | UC2 | mac01t_rec -Try (reccheck) | f614 cap 4 build 0 emp 4 wbad 0 | hit 5000 bad 12427 miss 300 (f307: hit 4790 bad 9992 miss 360) | 0000_1_2, 0001_1_2 byte-equal to run 1 |
| 4 | UC2 | yard_rec -Try (reccheck) | f348 cap 48 build 9 emp 39 wbad 0 pt 72 pk 36 | hit 2375 bad 4800 miss 0 | 0000_0_11 byte-equal to run 2 |

- Picture: equal on both scenes. The check does not change the drawn output.
- Check counters: NOT 0. Gate fails on bad (both scenes) and miss (mac01t_rec). wbad 0 on both.
- Run 4 did not stop at its until. The until was `rec f349 `, the yard window line in this run was f348. The run ran to the 90 s cap (a scenario error, the scenario is reverted).

## Cause (inference, not verified)
- bad: about 2 words per compare on yard (4800 / 2375). The index buffer address (gidx frame ring, pm_gpu.c 96 and 279; plan_stateful_records a2) is a F value
  that is not a patch slot. The plan UC2 text does not mask it. Plan_record_reuse section 2 excludes it, so UC2 must mask it too.
- miss: 300 of 5000 compares on mac01t_rec, 0 on yard. Likely a moved model with a different pal count per draw (plan_uniform_cut 2c, k5). Not verified.
- Next step: a fresh investigation agent names the first differing word (register, index in the group) on mac01t_rec with the 4 counters. No code agent from this guess (rule 5/7).

## UC2F (diag_uc2_mismatch change): step 1 committed, step 2 gate FAIL, reverted

Base HEAD 41a82a858 (commit A). Status lines: 3ds/logs/perf/status_UC2F.txt (5 starts, one repeat without a justification line).

### Commit A 41a82a858: reccheck without the 0x227 mask (uc2_change.patch, 3 files)
- Build lane UC2F: ok, no warning in pm_gpu.c or pm_gbi.c.
- Picture: byte-equal to the UC2H HEAD shots on mac01t_rec_chk (0000_1_2, 0001_1_2) and yard_rec (0000_0_11).
- Counters, window 2: mac f614 hit 5000 bad 12427 miss 300. Yard f348 hit 2375 bad 4800 miss 0. Same as the earlier UC2 run.

### Step 2: 0x227 index address mask (reverted, not committed)
- Change: pm_gpu_rec_walk records the offset of each 0x227 param word (rec_ix, cap 512). pm_gpu_rec_same sets those words in rec_msk.
- Build: ok. Picture: byte-equal (mac 0000_1_2, 0001_1_2; yard 0000_0_11).
- Counters, window 2: mac f614 hit 5000 bad 9700 miss 300 (bad was 12427). Yard f349 hit 2375 bad 4222 miss 0 (bad was 4800 at f348).
- Gate (bad 0 per window): FAIL on both scenes. Miss stays at 300 on mac, 0 on yard.
- Reverted by git apply -R. Diff kept at scratchpad uc2_mask_B.patch (49 lines).
- Result: the index address words explain part of bad (about 2727 of 12427 on mac, about 578 on yard). They do not explain all of it. The next diagnosis names the first unmasked word that differs (register, group index, frame) on yard.

### Scenario state
- New scenarios in 3ds/tools/azt_scenarios.json (uncommitted, the file also holds other uncommitted edits): mac01t_rec_chk (mac01t_rec + reccheck) and yard_rec (yard + reccheck, until `rec f34\d `).
- Catalog row for the two scenarios: not added (test_catalog.md has other uncommitted edits).

## Open items
- Plan UC2 must mask the index buffer address words before reccheck can give 0.
- The yard_rec until is `rec f348 ` in this build (baseline f349). Use `rec f34\d ` if the scenario is kept.
- Starts used on UC2: 4 of 8 (status_UC2.txt). UC2H baseline lane: build3ds/lane-UC2H. UC2 lane: build3ds/lane-UC2.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, `PM_LANE=<lane> sh 3ds/build.sh`, then `cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c`.
- Start: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 <scene> -Row <row> -Build <lane> -Try`.
- Rec line: the lane log `build3ds/lane-<lane>/last_log.txt` (build3ds/last_log.txt is not the lane log).
- Shots of the baseline: scratchpad uc2h folder (0000_1_2, 0001_1_2, 0000_0_11).
