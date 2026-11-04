# agent_nds1_e5: handoff after E5-1 and E5-2 (branch nds1)

## State
- E5-1 committed (1624da33c): NatKey has rm[4] (render mode at entry) and sl[2] (tex slot pair at entry). nat_entry_state() runs before each dlc_seg. nat_seg_add sets NatRec.flags bit 0 for a draw whose render mode at exit differs from entry. Gate: census_mac01 64 / 810 / 53 PASS. nat_host_test PASS. ndt_host_test PASS. NATB line not compared with a baseline. No host test case added (nat_seg_add is in pm_dlc.inc.c, not host-built).
- E5-2 committed (1821dbc49): dlc_learn in pm_dlc.inc.c runs one capture per map group with g->tid1 == 0 (not learned). Capture functions dlc_learn_begin / dlc_learn_end in pm_gbi.c (after rec_g0_set). Template id stored as DlcGrp.tid1 (0 not learned, 0xFFFF refused, else id + 1). Gate: census_mac01 64 / 810 / 53 PASS. nat_host_test PASS.
- LRN log (build3ds/lane-nds1/last_log.txt, first 40 lines only): 26 learned (tid 0 x16, 7 x2, 8 x7, 9 x1), 14 refused at pm_gpu_rec_end (rr 0). Not all 65 groups logged (cap 40).
- E5-3, E5-4, E5-5, E5-6 not started.

## Starts used (row NDS1, budget 8)
- E5-1 census_mac01: 1. E5-2 census_mac01: 1. Budget reset done once before E5-1.

## Open items for E5-3 (replay of map groups)
- Per-draw census tuples: cen_put(h, id0, id1, tris) (pm_gpu.c:486). The replay path (pm_gpu_rec_play, pm_gpu.c:2622) puts the cen tuples given by the caller. The map path has no per-draw tuple yet. Find how the walker sets id0/id1 at flush (pm_gpu.c ~1053 pm_tex_cur_slots, ~505).
- Tex block of each draw: nat_emit reads nat_tex[tex0] (pm_native.c:163). The live tex block of the slot at draw queue time is not yet taken. Needs a function that returns the 4 words of a bound slot.
- Index base: nat_emit writes nat_idx_base + idx_off (pm_native.c:165). The map pool base must match pm_gpu_rec_ia_phys(). Not checked.
- 14 of 40 logged groups have no template (pm_gpu_rec_end refused). Reason not found.

## Next steps
1. Read pm_gpu.c cen_put callers and pm_tex slot block access.
2. E5-3 on a branch: dlc_static_group takes the replay when g->tid1 > 1 and no NatRec draw has flags bit 0. Refused draws keep dlc_seg.
3. Gates: census_mac01 64 / 810 / 53, census_toad 26 / 338 / 23, tex+tri multiset equal to the walk.

## Commands
- Build: `/c/Python314/python 3ds/tools/gen_map_restore.py`, `PM_LANE=nds1 PM_ROW=NDS1 sh 3ds/build.sh`, then `git checkout -- 3ds/src/map_restore_table.c`.
- Census: `PM_ROW=NDS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_mac01 -Row NDS1 -Build nds1 -Try`.
- Host: `sh 3ds/tools/nat_host_test.sh`, `sh 3ds/tools/ndt_host_test.sh`.
