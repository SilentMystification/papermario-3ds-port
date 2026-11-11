# Row HC5: combine decision off core 0 (plan_hotcode.md 5.3, class B)

(Model: Opus, user approved 2026-11-11. Reason: the work is a protocol design across two threads (which combine inputs the recorder
knows, the T_PLAN / T_SYNC units, the plan table publish) with choices Haiku cannot make. The Opus agent writes Haiku rows for the
mechanical parts it finds (it may hand them back instead of coding them).)
## Facts (141c5e371)
- Class B (combine sync): M1 7.2 KB, M1-hot 7.2 KB; target <= 3 KB on core 0. Roots decide, pm_comb_eval, pm_gpu_set_plan.
- Core 0 path: sync_combine (3ds/src/nd_comb.c:566-584): sc_sig_now (:466) skip test, scm memo (16 entries, scm_sig :435, scm_find
  :443, scm_store :448, scm_replay :455), else sync_combine_work (:541-564): sync_legacy, decide, pm_comb_colours, pm_gpu_plan_key and
  pm_gpu_set_plan (pm_gpu.c:3653).
- Inputs (plan_hotcode 5.3): comb_w0/w1, omode_h cycle bits, prim, prim_lod low byte, env, keyc, keys, conv_k4 (op values: the recorder
  shadow nd_rec.c:53-66 holds omh/oml, prim, comb, env; not keyc/keys/conv_k4 (NDO_RARE), not prim_lod); tex_on, use_t1, comb_set,
  comb_alpha_shade (op decode); td[1].valid and dual_on (texture model, walker); box_prod, box_pat (outputs, nd_comb.c:176, :187);
  box_folded (tile 0 texels at a draw: core 0 only). After NDC_MODEL all are unknown (hit vs miss) until HC6.
## Design to deliver (then code it)
The recorder runs the memo on its shadow and writes T_PLAN (plan id in a core 1 plan table published with the frame) when every input
is known and box_prod is 0, else T_SYNC (core 0 runs today's sync, cold). Census switch c5cen (T_PLAN vs T_SYNC per frame, cold,
one line per 300 frames): class B <= 3 KB on core 0 needs T_SYNC < 5 % of syncs on mac_01.
## Places (other rows)
nd_comb.c (sync_combine, sync_combine_work, the memo), pm_comb.c (pm_comb_eval), nd_rec.c (the shadow struct :53-66 and rec_op_t),
pm_gpu.c pm_gpu_set_plan (:3653) and pm_gpu_plan_key only. HC8 edits pm_gpu.c pm_gpu_spr_build (:3796) and pm_gpu_grange (:2924),
nd_model.c :389-410, nd_ent.c :378, nd_shape.c :283/:608; SYNC60 edits pm_gpu.c pm_gpu_begin_wait: do not edit those. HC6 also edits
nd_rec.c (the record path nd_rec_model): coordinate by place; `git merge main` before the completing commit.
## Gates
nd_host PASS; memcheck OK; byte-equal spot_mac01_det and mdl5_iwa10_det (bases at b8bc06613; battle_start_f182_cold is not a gate: its
cold runs differ, row LOADMTX); hotset_size M1-hot class B <= 3 KB; the c5cen line on mac_01 (one Azahar list: spot_mac01_det,
mdl5_iwa10_det, and the c5cen scenario on one slot). Console prof push mac_01 0 0 200 (lgms grows <= 2 ms): the user's check.
## Start, stop, end
`sh 3ds/tools/wt_new.sh HC5`; PM_ROW=PM_LANE=HC5. CLAUDE.md 1-10. A try with no change in class B is not repeated; hand back the facts.
Delete this row file in the completing commit.
