# agent_dlf: epic DLF state

Branch at run: thunder-box (not main; no checkout allowed). Commits: 008ea433f DLF1.

## State
- DLF1 done: gbidump removed, streak calls gated on pm_ph_on. Test mac01t_fsplit (try): gbidump lines 0, shot subject shown.
- DLF2 reverted. Deleted pdl0, pdl_try_m, pdl_tpl blocks. Gate failed: gbi_run_body 20084 -> 20988 B (gate 17400 or less).
  Cause: pdl_try (its last caller now gbi_run_body) is inlined. Its out-of-line copy existed in the DLF1 map.
  Ops gate: mac01t mdlsum tot ops 1305 (range 1175-1495). No DLF1 baseline ops run.
- DLF3 not started: it deletes the pdl_try chain that DLF2 unhooks. Rows DLF4 to DLF6 not started (order).

## Open items
- Plan defect: DLF2 gate (bytes) cannot pass before DLF3. Merge the DLF2 and DLF3 gates, or add a noinline step, as a plan decision.
- Patch of the failed DLF2 edit: scratchpad dlf2_attempt.patch (reapply with git apply after the plan change).

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=<row> sh 3ds/build.sh; restore 3ds/src/map_restore_table.c from a copy.
- Test: PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 <scenario> -Row <row> [-Try].
