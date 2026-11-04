# agent_pmod: PMOD handoff (branch nds1, worktree papermario-3ds-port-nds1)

## State
- P1 done, commit 38a57086a: NatModel table, nat_pool_put (128 KB linear pool, pm_gpu_nat_pool_init), bake call in dlc_static_build. census_toad 26/338/23/0. Host tests pass.
- P2 done, commit a77422e8f: PM2 log only. 13 DL keys, one render mode, template ids 0 to 4. Design expectation (at most 2 per render mode) not met. Coordinator decision: the template key excludes the DL (state tuple only). DL belongs to the bake key. Template key change NOT yet made.
- P3 NOT started (no code). Gate scene added: census_toad_shot (azt_scenarios.json, catalog row). Uncommitted at handoff time.
- Gate reference: build3ds/azt/shots/census_toad_shot/0000_0_8.ppm (frame 200), 0001_0_8.ppm (frame 204), made by the HEAD a77422e8f build. Viewed: Mario and the kmr_10 terrain wall, sign on the wall. No clear Toad Town house in view. Picture does not yet show a model; choose another window or position before P3 gate use.
- Shot timing: shotwin=200:6 gives shots at frames 200 and 204 only (201 to 203 never logged). Until-line must be "shot 1 frame 204".
- Azahar starts on NDS1: 2 census (P1, P2) plus 4 shot (census_toad_shot attempts). About 2 left. Budget reset by orchestrator when needed.

## Coordinator decisions (2026-11-04)
1. P3 gate: census_toad_shot, region_diff.py (3ds/tools/region_diff.py) against the reference above, view both pictures.
2. Template key = state tuple (combine pair, geom, oh, ol, z, decal, blend, atest, tile format). DL is the bake key.
3. P3 makes nat_emit the only path for covered models. Remove the recplay gate and the capture-derived nat branch (pm_gbi.c rec_done nat block, UC3e replay switch) in the P3 commit that passes the gate.

## Next steps
1. Commit azt_scenarios.json and test_catalog.md (census_toad_shot, unverified).
2. P3: mg_run arm for ok groups emits nat_emit draws (eye 0). Delete the walk for those groups in the same commit. Census must equal 26/338/23/0. Shot census_toad_shot (until shot 1 frame 204) and region_diff vs reference. View pictures.
3. P4 to P7 as in design_producer_models.md section 10.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, then `PM_LANE=nds1 PM_ROW=NDS1 sh 3ds/build.sh`, then `git checkout -- 3ds/src/map_restore_table.c`.
- Census: `PM_ROW=NDS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_toad -Row NDS1 -Build nds1`
- Host: `sh 3ds/tools/nat_host_test.sh`
- Justification line first in 3ds/logs/perf/status_NDS1.txt (gitignored).

## Round 3 (coordinator: P3 gate scene is mac01t)
- Gate scene: mac01t (verified, Toad Town mac_01, tour path from mac_00 entry 4). Shots at frames 500 and 504 (shotwin=500:6), not 501.
- Baseline from HEAD 868397b49 (same code as a77422e8f for draws): build3ds/azt/shots/mac01t/0000_1_2.ppm (frame 500) and 0001_1_2.ppm (frame 504). Viewed frame 500: Mario and partner on the path, white fence, Toad Town grass, HUD. Subject shown.
- The mdlsum checks FAILED in the baseline run (no "mdlsum f783" line before the deadline). Shots were taken. The mdlsum part is not the P3 gate; the picture gate is the shot.
- Census on mac_01: no census scenario exists for mac_01 yet. census_yard (kmr_20) and census_toad (kmr_10) exist. Add census_mac01 (preset or warp=mac_01:0 story=ch0_toad_town drawcensus=200 loglevel=debug, until "cen f200 summary") before the P3 census check.
- P3 code: not started in round 3.
- Starts on NDS1: 1 more used in round 3 (mac01t baseline). Budget reset to 8 by coordinator, so about 7 left.
