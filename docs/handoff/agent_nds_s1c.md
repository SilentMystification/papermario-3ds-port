# agent_nds_s1c: S1-3 partial handoff (2026-11-04)

## State
- Branch nds1. Commit: S1-3 partial (render state template). TEV words NOT done.
- Done: 3ds/include/nd.h (NdTmplKey, NdTmplWord, nd_tmpl_build, nd_tmpl_words), 3ds/src/nd_tmpl.c (256 entries, 512 index, linear probe), 3ds/tools/nd_tmpl_test.c, nd_host_test.sh line, CMakeLists src/nd_tmpl.c.
- Template words (libctru reg numbers): CULL 0x40, DM_SCALE 0x4D, DM_OFFSET 0x4E, DM_ENABLE 0x6D, BLEND_FUNC 0x101, ALPHA_TEST 0x104, DEPTH_COLOR_MASK 0x107. Packed layouts taken from the disassembly of libcitro3d.a (C3D_DepthTest, C3D_AlphaTest, C3D_AlphaBlend, C3D_CullFace, C3D_DepthMap).

## Gates
- Host nd_tmpl_test on the 8 ndref files: PASS. 674 draws: 398 z0 checked, 276 z1 skipped, 673 blend checked (1 off_active skipped).
- Test was first run with no nd_tmpl.c: link FAIL (undefined nd_tmpl_build). Then PASS.
- ARM build (PM_ROW=S1-3 sh 3ds/build.sh): PASS, nd_tmpl.o linked.
- Azahar: 0 starts. Template is not called by any draw (S3 wires it). No picture change expected.
- Full nd_host_test.sh: FAIL only at nd_mtx (row S1-4), as expected.

## Open for S1-3 (row not complete)
1. TEV words (TevKey -> stage words). The key has no PmPlan or w0/w1, so pm_comb_build output cannot be rebuilt from the key. Decision needed: add PmPlan (or w0,w1,two,t1only) to the key, or keep TEV out of nd_tmpl.
2. Stencil (zmark pass: stc_set 5/6, C3D_StencilOp) not in the template.
3. Early depth (sh_early) not in the template. Register packing not yet checked (ctx offsets 234..236).
4. COLOR_OPERATION 0x100 blend-mode bits (sync_render / rm_apply) not in the template: the write is a read-modify-write of unknown base bits.
5. z 1 depth enable (zmark/zcmp/zupd) is not verified: the ndref has no flags. Re-capture needed for a full check.
6. f32tof24 offset bits are not checked (host stub). Only zero and non-zero.
7. Blend off_active path (blend_off) not in template.

## Not started
- S1-4 (nd_mtx), S1-5 (interp), S1-6 (tex descriptors). Plan row lines 161-175 of docs/handoff/plan_native_slices.md.
- diag_s1_1_picture.md: not present at this handoff.
- Mem line: PM_LINEAR_FIXED_KB still 4968 KB (open from S1-2).

## Commands
- Host: sh 3ds/tools/nd_host_test.sh (fails at nd_mtx until S1-4).
- Build: /c/Python314/python 3ds/tools/gen_map_restore.py; PM_ROW=<row> sh 3ds/build.sh; restore map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). Never commit it.
