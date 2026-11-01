# agent_sr3: SR3 on the copy path: stopped before code

Task: plan_stateful_records.md row SR3, changed to the copy path (decision_jump_closed.md). Base: HEAD 92f917ffb.
No source file changed. No build, no Azahar start. No justification line written (no start).

## Why stopped

1. Uniform writes in the old path depend on draw history, so a copied record is not a function of its key.
   - 3ds/src/pm_gpu.c 915 to 918 and 937 to 939: palette matrix uploads (C3D_FVUnifMtx4x4 with pcomp[gm]) are skipped when
     sh_pal_ok, sh_pal_gm and sh_pal_gen show the register block holds that value. The palette slot k depends on the order
     of earlier draws.
   - pm_gpu.c 949 to 951: projection upload is skipped when proj_last equals the matrix (DR1 skip). proj_last is history.
   - pm_gpu.c 894 to 900 and 926 to 929: uvso, cmul, cadd, zmix uniforms are skipped by memcmp against the last value (sh5_*, sh_*).
   A record copied from one frame writes the uniform values of that frame. A different frame needs other values. The
   copied words also skip the shadow updates, so later old-path draws compare against the wrong state.
2. Matrix values change with the camera. The per-frame matrix and projection cannot stay in a copied record. The plan
   (SR4 glue) cuts them out per frame. That cut needs the uniform emission of flush_draw to be rewritten, not parsed.
3. Header format is not verified. libctru gpu.h (in image pm3ds-build:1) gives GPUCMD_HEADER(incremental, mask, reg)
   with no count field. The extra-parameter count of a write is not read from the repo. citro3d headers were not found in
   the build image (agent_sr3 earlier stop, still open).

## Result

A byte-equal copy draw needs a per-frame uniform cut. That means a change to flush_draw's uniform path (not a copy of
words). Azahar budget 6 starts stays unused for SR3.

## Next (for the orchestrator, Opus plan)

- Plan row: flush_draw uniform path as a key-pure function. Each uniform write becomes a per-frame glue entry, and the
  shadow compares run in the glue writer, not in the record.
- Verify the PICA header count layout (libctru gpu.h, GPUCMD_AddIncrementalWrites in libctru gpu.c) in the build image.
- Read citro3d (C3D_UpdateUniforms, C3D_DrawElements) from the image before a plan names it.
