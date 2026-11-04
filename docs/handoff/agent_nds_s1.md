# agent_nds_s1: S1 NDCORE handoff (2026-11-04, second session)

## State
- Branch nds1. Re-anchored plan: docs/handoff/plan_native_slices.md at 10b3239e3 (symbol anchors, line numbers are hints).
- S1-1 code: commit 3e6b19bc2. Deleted the replay range, the record capture, the PMOD bake (nat_pool_*, nat_model_*) and nat_tmpl_strip, nat_rec_write, pm_gpu_rec_*, pm_tex_rec_use, pm_gpu_pal_fill, pm_gpu_stc_ctx_log, pm_gpu_nat_pool_init, gstat_gen, tb_gen, rec_on and the rec_* statics.
- S1-1 gates:
  - Host (sh 3ds/tools/nat_host_test.sh): PASS.
  - ARM build (docker pm3ds-build:1, PM_ROW=S1-1 sh 3ds/build.sh): PASS.
  - Azahar census_mac01 (1 start of 8, -Try): PASS, batches 64, tris 810, distinct 53, over 0.
  - Picture gate (census_toad_shot region_diff against the HEAD build shot): PENDING. Needs one HEAD build (lane) and one run of each build.
  - mem line (PM_LINEAR_FIXED_KB, expected down about 256 KB): not read yet.
- Deviations from the row text:
  - test_layout (nat_host_test.c) no longer checks the captured template words (they came from nat_tmpl_strip). Eye copy and palette patch checks stay. The 100-word small-output cap is 40 now (the template-free output is 76 words).
  - test_bake deleted with the pool. Its call in main deleted.
  - pm_eye_idx and nat_mtx are kept (S1-4 decides them). pal_mul kept.
  - pm_native.h keeps nat_tex_id, nat_order, nat_emit, nat_eye_copy, nat_gpu_addr, NatRec, NatTmpl (S1-4 removes the files).
- map_restore_table.c: saved in the scratchpad before the build and restored byte-equal (md5 437b90efe5f32b82394ce2ece094210e). Never committed.

## Next
1. S1-1 picture gate. Build HEAD (3e6b19bc2^) in a lane, run census_toad_shot with each build (-Try, budget 8 starts), region_diff.py on the two shots. Pass: scattered one-pixel diffs only.
2. S1-2 (nd.h, nd_core.c, pm_nusys.c gbi_task, CMakeLists): host test case "legacy only" first, then the code.
3. S1-3 to S1-6 in order.

## Commands
- Native host test: `sh 3ds/tools/nat_host_test.sh`
- ND host tests (docker pm3ds-host:1): `sh 3ds/tools/nd_host_test.sh`
- Build: `python 3ds/tools/gen_map_restore.py`, `PM_ROW=S1-1 sh 3ds/build.sh`, then restore map_restore_table.c byte-equal from the scratchpad copy (md5 437b90ef...).
- Azahar: write the justification line to 3ds/logs/perf/status_NDS_S1.txt first, then `PM_ROW=S1-1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_toad_shot -Row S1-1 -Try`.
