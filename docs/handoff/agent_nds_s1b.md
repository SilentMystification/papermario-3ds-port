# agent_nds_s1b: S1 NDCORE handoff, after S1-2 (2026-11-04, third session)

## State
- Branch nds1, main folder only. Plan: docs/handoff/plan_native_slices.md section S1 (symbol anchors).
- Done and committed:
  - 3e6b19bc2 S1-1 (delete learn-and-replay, record capture, PMOD bake).
  - f6675237d, 388210583 picture gate fixes (baseline shots, regex).
  - 389d7c16d S1-2 (nd.h, nd_core.c, nd_core_test.c, pm_nusys.c gbi_task, CMakeLists, nd_host_test.sh).
- NOT started: S1-3, S1-4, S1-5, S1-6.

## Step 0 result (S1-1 picture, 300 px, 42 regions)
- Two shots of the same S1-1 binary: 0 px diff (2 of 2 identical). Shot-to-shot noise is 0.
- The 300 px vs the S0 baseline is deterministic. Viewed: scattered single-pixel changes on wood and grass texture edges. No object moved or changed. Gate PASS (max region 27 px, 0.31 %).
- Noise floor set: 0 px shot-to-shot. Re-check the 300 px at S1-6 (texture descriptors).
- Crop image: scratchpad s1/d0000.png (diff script: scratchpad s1/pdiff.py).

## S1-2 gates
- Host legacy-only test (3ds/tools/nd_core_test.c, docker pm3ds-host:1): PASS.
- ARM build (PM_ROW=S1-2 sh 3ds/build.sh): PASS.
- G5 size (sh 3ds/tools/nd_size_test.sh 3ds/src/nd_core.c): PASS, nd_emit + nd_legacy 216 B (limit 8192).
- census_toad_shot on S1-2 vs S1-1 shots: 0 px (2 of 2).
- yard_shot on S1-2 vs lane-base (-Build build3ds/lane-base/pm_3ds.3dsx): 0 px (2 of 2).
- Full nd_host_test.sh: FAIL only at "nd_mtx missing (row S1-4)". Expected until S1-4.
- Mem line: PM_LINEAR_FIXED_KB is still 4968 KB (constant, pm_mem.c line 20). The mem line shows linear 14208 KB in both builds. The planned drop of about 256 KB is not visible. Open: change the constant when the drop is measured.

## Azahar starts on row S1-2 (budget 8)
- census_toad_shot 1, yard_shot 1, yard_shot lane-base 1. Total 3 of 8.
- Row S1-1: 4 of 8 (census_mac01, baseline, S1-1 census, noise run B).

## Deviations and open items
- NdRec has only the LEGACY fields (kind, a, b) and reserved words. The draw fields come with the rows that use them.
- nd_bridge_cut does not read gMainGfxPos yet. No producer calls it except gbi_task. Add the gMainGfxPos cut when the first producer (S3 or S4) calls nd_* draws.
- Full array (2048 records) runs the span at once, out of order. ponytail comment in nd_core.c. Add a split before producers fill it.
- Record array is static BSS, 64 KB. Plan said heap. Check the linear and heap split at S1-6 close.
- pm_pf_nd_add is called per emit when pm_ph_on. Its prototype is in nd_core.c (not in a header).
- Commit rule: map_restore_table.c is never committed. It is restored byte-equal (md5 437b90efe5f32b82394ce2ece094210e) after each gen_map_restore.py run.

## Next (S1-3 first)
1. S1-3 NDTMPL (plan section S1-3). Host test first: for each ndref draw, nd_tmpl_build(key) words decode to the ndref TEV, depth, blend, alpha and cull fields (tool: 3ds/tools/nd_pica_decode.c).
2. The sh_* setters are in pm_gpu.c, grep `static void sh_depth_test(bool en` (about L629-662). They call C3D_* functions. The template words must equal the register writes these functions make. Read the C3D_* register encoding before the code. Do not stub.
3. Then S1-4 (nd_mtx.c, deletes pm_native.c, pm_native.h, nat_host_test.*), S1-5, S1-6 in order.

## Commands
- Host: sh 3ds/tools/nd_host_test.sh (fails at nd_mtx until S1-4).
- Build: python 3ds/tools/gen_map_restore.py, save and restore map_restore_table.c byte-equal, then PM_ROW=<row> sh 3ds/build.sh.
- Azahar: write the justification line to 3ds/logs/perf/status_NDS_S1.txt first, then `PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Try`. Use `-Build build3ds/lane-base/pm_3ds.3dsx` for the S0 baseline.
- Python: use /c/Python314/python (python3 is a Windows stub).
