# agent_nds_s1d: S1-3 items 1 to 7 not closed, S1-4 not started (2026-11-04)

## State
- Branch nds1. HEAD before this file: 9fcfee63c (S1-3 partial). No code changed in this run.
- Host test nd_host_test.sh: unchanged. Fails at nd_mtx (S1-4), as before.
- Azahar: 0 starts in this run. Status file not written (no start).

## Verified register layouts (libcitro3d.a disassembly, objdump of docker image pm3ds-build:1)
- C3D_StencilTest: word 0x105 = en | ((fn << 4) & 0x70) | ref << 16 | wm << 8 | im << 24. Matches the comment in pm_gpu.c stc_set.
- C3D_StencilOp: word 0x106 = sfail | zfail << 4 | zpass << 8 (args: sfail, zfail, zpass). Both stores set ctx dirty bit (ctx+32, 0x10).
- C3D_EarlyDepthTest: sets ctx bytes +234 (en), +235 (func), +236 (u32 ref). The register write (0x61, 0x62, 0x6A, 0x118) is NOT found: no read of ctx+234..236 in libcitro3d.a by immediate offset. The flush reads these fields by another form. Open.

## Open for S1-3 (unchanged blockers)
1. TEV words: the packing of C3D_TexEnv (texenv.h: srcRgb, srcAlpha u16; opAll; funcRgb, funcAlpha u16; color; scaleRgb, scaleAlpha u16) into TEV registers is not verified. Register base: GPUREG_TEXENV0_OPERAND 0xC1, TEXENV0_COLOR 0xC3, TEXENV0_SCALE 0xC4 (libctru registers.h). The write function is not yet disassembled.
2. Stencil: 0x105 and 0x106 layouts verified (above). Template key needs a stencil mode field (stc_set 5, 6 and the decal stencil sites 1 to 4). No ndref field holds the stencil mode: needs re-capture.
3. Early depth: register not located (above). sh_early is called with (false, GREATER, 0) at pm_gpu.c L1164.
4. COLOR_OPERATION 0x100 blend bits: read-modify-write in sync_render (pm_gbi.c L647) and rm_apply (pm_gbi.c L2840). The base bits come from game state, so the template needs that base as a key input. Not read in this run.
5. z 1 depth flags (zmark, zcmp, zupd, nocol): ndref has no flags. Re-capture needed (game hook in ndref_draw, pm_gpu.c; FORMAT.md; nd_ref_dump.py). Azahar start needed, justification line first.
6. f32tof24: host stub only checks zero or non-zero. Real libctru f32tof24 not reproduced.
7. blend_off path: not in template.
- TEV words also need w0, w1, two, t1only in the key (decided). ndref has no w0 or w1: the same re-capture as item 5 is needed.

## Not started
- S1-4 (nd_mtx). Plan rows at docs/handoff/plan_native_slices.md L161-167.

## Next steps
1. Extend the ndref hook (pm_gpu.c ndref_draw) to write z flags, stencil mode, w0, w1, two, t1only. Bump FORMAT.md. Re-capture 8 files. One Azahar run per scene, status line first.
2. Locate the TEV register packing: disassemble the function that writes GPUREG_TEXENV0_* in libcitro3d.a (search for the 0xC1 or 0xC3 immediate in the flush).
3. Locate the early depth register writer (search for ctx+234 reads in other forms, or the register 0x61 immediate).
4. Then items 1 to 7, commit. Then S1-4.

## Commands
- Host: sh 3ds/tools/nd_host_test.sh (fails at nd_mtx until S1-4).
- Disassembly: docker run --rm pm3ds-build:1 sh -c '$DEVKITPRO/devkitARM/bin/arm-none-eabi-objdump -d --no-show-raw-insn $DEVKITPRO/libctru/lib/libcitro3d.a' (about 8300 lines; write to scratchpad, grep).
- Headers: $DEVKITPRO/libctru/include/c3d/*.h and include/3ds/gpu/registers.h (same image).
- Build: /c/Python314/python 3ds/tools/gen_map_restore.py; PM_ROW=<row> sh 3ds/build.sh; restore map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). Never commit it.

## Addendum (step 2 probe)
- No citro3d texenv.c in the repo or the docker image (pm3ds-build:1). Only headers. TEV packing must come from disassembly of the TEV register writer in libcitro3d.a.
- Register map (registers.h): TEXENV0_SOURCE 0xC0, OPERAND 0xC1, COLOR 0xC3, SCALE 0xC4. FUNC is assumed 0xC2 (not confirmed in the grep output). Stage stride not yet confirmed.
- Step 1 (ndref hook, Azahar re-capture) and step 4 (S1-4) not started.
