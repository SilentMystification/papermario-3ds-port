# Row HC6: model hit path (plan_hotcode.md 5.4, classes C and D)

(Model: Opus, user approved 2026-11-11. Reason: hit-state rule, key A/B split between the cores and the pre-built GPU command block are
design choices with picture risk that Haiku cannot make. The Opus agent writes Haiku rows for the mechanical parts it finds.)
## Facts (141c5e371)
- Classes C (model record hit) M1-hot 11.3 KB and D (native submit and emit) 8.8 KB; target C+D <= 8 KB.
- A hit skips rec_apply (3ds/src/nd_recs.c:71-102), so the walker keeps the pre-record omode, combine, prim, fog and tile state; a miss
  keeps the record's. pm_gpu_model_key (pm_gpu.c, "Premise 2" of plan 5.4) reads record words, nd_ws fields and core 0 state.
- rec_matrix (nd_recs.c:65-70) runs on a cache hit (S3N-6); the load_mtx guard is row LOADMTX (nd_mtx_set.c only).
## Design to deliver (then code it), in plan order
1. Hit state equal to the miss state (typed copy of the record's scalar state on a hit, no GPU call); census hitst (cold switch) of
   walker draws after a hit that read a field the hit left stale; the byte gate decides.
2. Key A (record part, hashed on core 1 into the payload) and key B (core 0 render shadow); switch mkchk compares A+B with the old key
   on every record: 0 differences.
3. rec_run hit fast path <= 600 B (memo by record address, key B compare, visibility, matrix, submit loop); everything else in a cold
   rec_miss (rec_apply, ndm, waterlog, wskip, R11, POND, stale check).
4. Pre-built GPU command block per cached draw with patch offsets (pal0 rows, zmark row, mix_st, unit 0 address and dim, template
   program); switch blkchk compares the hit's command span old vs block: 0 differences outside the filtered state words.
5. HC5 (combine plan on core 1, plan_hotcode 5.3) on top of steps 1-2: with the record's state known after NDC_MODEL, rerun the
   census c5cen of branch HC5 (ba484c66e: merge or cherry-pick it into HC6 first) and add T_PLAN/T_SYNC only if T_SYNC < 5 % of syncs
   on mac_01 f600-f900; else hand back the census line. Use the Opus coder a2112b69ed01fcddd (HC5 context) for this row.
## Places (other rows)
nd_recs.c (rec_apply, rec_matrix, the hit path), pm_gpu.c (pm_gpu_model_key, pm_gpu_model_hit, rec_run, pm_gpu_native_submit_draw),
nd_model.c (the native model cache, not :389-410), nd_emit.c, nd_rec.c nd_rec_model (:208-219) only. WAITING for HC8 to merge
(pm_gpu.c and nd_model.c). HC5 edits nd_rec.c's shadow and rec_op_t, nd_comb.c, pm_comb.c, pm_gpu_set_plan; SYNC60 edits
pm_gpu_begin_wait; LOADMTX edits nd_mtx_set.c: do not edit those places. `git merge main` before the completing commit.
## Gates
nd_host PASS; memcheck OK; byte-equal spot_mac01_det, mdl5_iwa10_det, spot_mac00_det (bases at b8bc06613 or stored at the
merge-base); hotset_size M1-hot C+D <= 8 KB. One Azahar list per step (the three scenes, plus the census scene of the step).
Console prof push mac_01 0 0 200 (pmu I-miss, cycles, lgms): the user's check.
## Start, stop, end
After HC8 merges: `sh 3ds/tools/wt_new.sh HC6`; PM_ROW=PM_LANE=HC6. CLAUDE.md 1-10. A try with no change in C+D is not repeated;
hand back the facts. Delete this row file in the completing commit.
