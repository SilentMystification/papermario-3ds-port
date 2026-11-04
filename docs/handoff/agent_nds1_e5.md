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

## E5-3 reads (second pass, not implemented)
- Census tuple per draw: cen_add (pm_gpu.c:496) puts h (TevKey hash, z, graw, prog, cull), id0 = pm_tex_census_id(0), id1 = pm_tex_census_id(1), tris (pm_gpu.c:1085). id0/id1 are read from the live slot state at flush (pm_tex.c:681). h is constant per template key. tris is per draw.
- Index address: nat_idx_base can be set to nat_gpu_addr(osConvertVirtToPhys(pool base)) (pm_native.c:227, same formula as pm_gpu_rec_ia_phys, pm_gpu.c:2534). Pool base set by nat_pool_set (pm_native.c:222). Feasible.
- Tex block per draw: nat_emit reads nat_tex[tex0] (filled by nat_tex_id from the 4 GPU words of unit 0 / unit 1). No pm_tex accessor returns those 4 words for a slot. Missing: an accessor for the unit config words of the current slot at emit time.
- Slot per draw: the flush reads pm_tex_cur_slots at flush time (pm_gpu.c:1053), after queueing. The E5-1 key (sl[], entry) is not proven to equal the flush-time slot. Needs a check before E5-3 can use it.
- Blocker: E5-3 needs the tex-words accessor and a proof that the entry slot equals the flush slot. Neither is implemented. No code change in this pass.

## E5-3 step 1 blocked (third pass)
- pm_tex_slot_words needs the 4 GPU unit words of a slot. Their format (0x081 / 0x091 data words 22..25) is set by citro3d at flush. citro3d source is not on disk (docs/handoff/inv_citro3d_format.md). No pm_tex function gives these words. A word layout guessed from the header would not be checked until a shot, so no accessor was written.
- Option for the coordinator: capture the 4 words per slot from the command buffer after a live flush (a per-slot cache keyed by slot generation). That is a new mechanism, not a copy of existing code.
- Pool index base (nat_gpu_addr(osConvertVirtToPhys(pool))) and census tuple sources are known (see above). They are not enough without the tex words.

## TEXW result (fourth pass)
- Source: citro3d texture.c:258 to 289 (C3Di_SetTex): unit 0 words dim, param, lodParam, phys(data) >> 3. Template grp 22 to 25 match this layout where the bound tex is the same.
- census_mac01 TEXW: ok 28 / bad 29. Logged mismatches swap the slot tex and the off-screen tex (dim 0x00400020 and 0x00200020, addresses 0305a700 and 0305bf80). The words are read at learn end, not at the window's first flush, so the check compares different textures.
- Not run: E5-3 emit, E5-4, E5-5, E5-6. Option A (capture the 4 words at flush per slot) is the next step.
- Starts on NDS1: 5 of 8.

## TEXW option A result (fifth pass, handoff at token limit)
- pm_gpu.c flush_draw snapshots unit 0 words (pm_tex_slot_words of the bound slot) at each flush of a learn window (rec_fw[16][4], rec_fwn). pm_gpu_rec_fw reads them. dlc_learn_end compares template grp 22 to 25 with any snapshot.
- census_mac01 TEXW: ok 56 / bad 1. The bad case has one snapshot (nf 1). Template words 00200020 00002206 00000000 0305a940; snapshot 00200040 00000006 00000000 0305c380. The template words come from a flush that the draw-time snapshot does not show (not resolved).
- Census 64/810/53 PASS with this code.
- Not done: E5-3 emit (needs per-draw words at emit, the unresolved bad case must be explained first), E5-4, E5-5, E5-6.
- Starts on NDS1: 8 of 8 used after the reset? Count: E5-1, E5-2, TEXW, TEXW2, TEXW3, TEXW4, TEXW5 = 7 of 8 (budget was reset before this pass).
- Next: find which flush emitted template words 00200020/0305a940 in the bad case (log the flush address of each snapshot and the 0x081 header order in the window), then E5-3 per the coordinator steps.

## E5-3 learned replay (sixth pass): census FAILED, code reverted
- Patch kept: docs/handoff/nds1_e53_wip.patch (learn end writes NatRec + pool index copy + census tuples; dlc_nat_group replays with nat_emit; NAT_TEX_TMPL=0xFF keeps template words; learn refuses when any unit 0 flush differs from the template).
- Build: ok. Host tests nat_host_test and ndt_host_test: pass (with the patch).
- census_mac01 with the patch: 71 batches / 810 tris / 54 distinct. HEAD (same build lane, same scenario): 64 / 810 / 52. Gate is 64 / 810. FAIL.
- Only 1 learned replay in the run (log line E5-3 learned replay draws 1). So the replay is refused for almost all groups. The reason is not logged. The +7 batches are not explained: the replay adds at most 1 cen_put in this run.
- Next (new diagnostic agent, 3 starts max): add refusal counters to dlc_nat_group (view, wind, interp, lit/fog, box, tex_use, slot words, pal, emit, rec_play) and log the census lines of frame 200 with the replay on and off. Find the source of the +7 batches before any emit change.
- Starts on NDS1 in this pass: 2 (E5-3 census, HEAD census). Budget reset before this pass.
