# agent_sr1: SR1 (version fields and counters, no reader)

Plan: docs/handoff/plan_stateful_records.md row SR1. Base: HEAD 67078df7d (DLX2 closed).

## State

- Model fields: pmRecVer (u32, offset 0x14), pmSlot (u8, 0xAA), pmPad (0xAB), pmMtxVer (u16, 0xAC), pmPad2 (0xAE). Size 0xB0 unchanged (layout checked by offset arithmetic, no static assert added).
- Global pm_rec_seq (defined src/model.c, extern and PM_REC_BUMP macro in include/model.h).
- PM_REC_BUMP calls: 17. src/model.c 11 (creation, 2841 IGNORE_MATRIX clear, enable and disable transform group, mdl_group_set_custom_gfx x3, set_tex_panner, set_mdl_custom_gfx_set, mdl_make_local_vertex_copy, parade_wide_setup). src/evt/map_api.c 6 (SetTexPanner, SetCustomGfxEnabled, SetModelCustomGfx, SetModelTexVariant, EnableTexPanning, SetRenderMode).
- pmSlot writes: creation (modelIdx + 1), group child copy (newModel.pmSlot = 0).
- tb_gen: u32 tb_gen[128] in 3ds/src/pm_gbi.c, bumped in pm_texbind_set after the list reset. Declared u32, not u16 (the key in plan a1 may truncate it later).
- gstat_gen: unsigned in 3ds/src/pm_gpu.c, bumped in pm_gpu_gstat_reset (after the if, so every reset bumps).
- Not in SR1 (plan row SR1 excludes them): savedMtx (2817, SR2), pm_fog_ver, pm_tint_ver, pm_rec_ctx.

## Deviation

- src/battle/level_up.c (5 sites, flags TRANSFORM_GROUP_MEMBER): not changed. Those writes go to EntityModel (include/entity.h, size 0x68, no padding). Entity models never reach a record (records key on gCurrentModels). Adding a field would change EntityModel size. No edit made.

## Gate (SR1)

- Build: lane srhead (HEAD 67078df7d) and lane sr1. No warning line in sr1 that srhead lacks (unique warning lines compared). map_restore_table.c restored from its kept copy after each build, not committed.
- HEAD against HEAD: mac01t_cap1 (-Try) 0000_1_2 and 0001_1_2 equal in 2 runs. yard 0000_0_11 equal in 2 runs.
- SR1 against HEAD: mac01t_cap1 0000_1_2 and 0001_1_2 equal. yard 0000_0_11 equal. Log checks 6 of 6 and 3 of 3.
- Azahar starts on row SR1: 6 of 6 (budget used). Justifications in 3ds/logs/perf/status_SR1.txt.

## Open items

- SR2 (plan row SR2) reads pmMtxVer and sets the header w1. Not started.
- The picture was byte-equal, so the counters do not change pixels. A person has not looked at the shots.

## Next

- SR2 starts after this commit. Read the plan row SR2 first. Budget for SR2: reset with run_azahar.ps1 -ResetBudget SR1 only if the orchestrator asks.

## Commands

- Build: `python 3ds/tools/gen_map_restore.py`, `PM_LANE=<lane> sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from 3ds/src/map_restore_table.c.bak.
- Picture: `powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row SR1 -Build <lane> -Try` and `... yard -Row SR1 -Build <lane>`.
