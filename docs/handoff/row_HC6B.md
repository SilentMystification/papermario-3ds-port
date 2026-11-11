# Row HC6B: model hit path, steps 4 and 5 (plan_hotcode.md 5.4 and 5.3, classes C, D and B)

(Model: Opus, user approved for HC6 on 2026-11-11; the same coder a2112b69ed01fcddd continues (HC5 and HC6 context). Reason: the
pre-built command block (which words are per-frame patches) and the T_PLAN/T_SYNC protocol between the cores are design choices with
picture risk.)

## Facts
- HC6 steps 1-3 are on branch HC6 (ready 45066aa71): hit state copy (hitst), key A on core 1 in the NDC_MODEL header plus key B on core 0
  (mkchk diff 0), rec_run hit fast path 464 B with a cold rec_miss. hotset at 0865df7c0: M1-hot C 7752 B, D 8792 B (C+D 16.5 KB; target
  C+D <= 8 KB). Gates of steps 1-3: spot_mac01_det and mdl5_iwa10_det 6/6 byte-equal vs 6c18e508b, nd_host PASS.
- Start only after the orchestrator merges HC6 steps 1-3 into main (row_HC6.md is deleted by that merge).
## Design to deliver (then code it), in order
4. Pre-built GPU command block per cached draw (plan 5.4 last item): the miss records the command words of pm_gpu_native_submit_draw
   for each NdNatC into a linear block with the full state (no redundant-state filter) and the offsets of the per-frame words: palette
   rows (pal0), the zmark row, the shroud TEV colour (mix_st), unit 0 address and dim when e->tex, the template program when tgen moved.
   A hit copies the block (aligned) into the command buffer and patches the offsets. Check switch blkchk (cold): the hit's command span
   old vs block per draw, 0 differences outside the filtered state words. Memory: the block pool on the heap at first use; memcheck margin.
5. HC5 on top of steps 1-2: merge or cherry-pick the c5cen census of branch HC5 (ba484c66e), rerun it on mac_01 f600-f900; add T_PLAN
   and T_SYNC (plan 5.3) only if T_SYNC < 5 % of syncs; else hand back the census line and stop step 5.
## Places (other rows)
Step 4: pm_gpu.c (pm_gpu_native_submit_draw, pm_gpu_model_hit), nd_emit.c, nd_model.c (the native cache entry). Step 5: nd_rec.c (the
recorder shadow and rec_op_t, not stream_run), nd_comb.c (sync_combine, the memo), pm_comb.c, pm_gpu.c pm_gpu_set_plan/pm_gpu_plan_key.
Rows that may run at the same time, other places: WALLSHADE (pm_gpu.c pm_gpu_spr_build :3869, nd_geom.c, nd_walk.c nd_walk_spr), UB1
(pm_gpu.c spr_nat_fb :3750, :3787-3788, :3796; nd_model.c mf_tot :227), HC9B (nd_rec.c stream_run), HCCUT2 (nd_rect.c, nd_tmpl.c
nd_tmpl_build). Do not edit those places; `git merge main` before each completing commit.
## Gates
nd_host PASS; memcheck OK; byte-equal spot_mac01_det, mdl5_iwa10_det, spot_mac00_det against stored bases at the merge-base
(`bench.py base`, run, `cmp`); blkchk 0 (step 4); hotset_size M1-hot C+D written (target <= 8 KB) and class B (step 5, target <= 3 KB).
One Azahar list per step (the three scenes plus the census scene of the step). Budget: 8 Azahar starts for this row (new budget).
Console prof push mac_01 0 0 200 (pmu I-miss, cycles, lgms): the user's check.
## Start, stop, end
`sh 3ds/tools/wt_new.sh HC6B` from main after the HC6 merge; PM_ROW=PM_LANE=HC6B. CLAUDE.md 1-10. A try with no change in C+D is not
repeated; hand back the facts. Commit each step after its gates. Delete this row file in the completing commit.
