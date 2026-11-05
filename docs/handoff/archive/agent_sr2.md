# agent_sr2: SR2 (matrix version in the header, load_mtx cache)

Plan: docs/handoff/plan_stateful_records.md row SR2. Base: HEAD db05981f1 (SR1 committed).

## State

- src/model.c appendGfx_model (1514 to 1518): header w1 = pmSlot (bits 0 to 7) | static bit 0x100 (finalMtx == &savedMtx and not BILLBOARD) | pmMtxVer << 16.
- src/model.c: global `u16 pm_mtx_seq` (2621). Bumps `model->pmMtxVer = ++pm_mtx_seq` at the two savedMtx writes that matter: creation (before PM_REC_BUMP, covers 2697 and 2700 branches) and the savedMtx refresh in mdl_update_transform_matrices (2822 to 2825).
- 3ds/src/pm_gbi.c: mtx_cache[256][16] float and mtx_tag[256] u32 (valid = 0x10000 | pmMtxVer). load_mtx(w0, w1, hdr) takes the header word. When the static bit is set, slot is not 0, and no fps60 entry (lm == 0): hit copies the cached matrix; miss unpacks and stores. mg_run passes its header w1. The walker G_MTX passes 0 (no cache).
- Static G_MTX inside a model group: only gSPMatrix(model->finalMtx) at model.c 1988 and 1999. The billboard G_MTX (2039) is outside the cache because the static bit is 0 for BILLBOARD.

## Deviation from the plan text

- Version is a global u16 counter (pm_mtx_seq), not model->pmMtxVer++. A per-model counter repeats: a new model at the same slot and heap address would start at the same value as an old one, and the cache would return the old matrix. Ceiling: the u16 wraps after 65536 bumps. A wrap can match an old entry for the same slot only if that slot was not refilled in between. Upgrade path: keep the source address in the tag too (mtx_src[slot]).
- The yard HEAD baseline is the SR1 start 6 shot (lane sr1, 02:56), not a new start. Budget.

## Gate (SR2)

- Build: `PM_LANE=sr2 sh 3ds/build.sh` after gen_map_restore.py, build OK (exit 0). Warnings in pm_gbi.o and model.o come only from included headers (aPoleFilter, nullptr). None on changed lines. map_restore_table.c restored from a kept copy, not committed.
- HEAD against HEAD: mac01t_cap1 runs 2 and 3 (lane sr1): 0000_1_2 and 0001_1_2 byte-equal.
- SR2 against HEAD: mac01t_cap1 (lane sr2, start 4): 0000_1_2 and 0001_1_2 byte-equal to HEAD. Log checks 6 of 6.
- SR2 against HEAD: yard (lane sr2, start 5): 0000_0_11 byte-equal to SR1 start 6 (lane sr1). Log checks 3 of 3.
- Azahar starts on row SR2: 5 of 6. Start 1 failed (until was the pos line at f483, shot window 500 to 505: no shot). Its until changed to `shot \d+ frame 504\b` in azt_scenarios.json (mac01t_cap1). Justifications in 3ds/logs/perf/status_SR2.txt.
- Not run: mac01t_prof (prof line, direction only). The cache hit count is not measured: no counter in the build. The picture gate proves equal output, not that the cache hits.
- A person has not looked at the shots. The script does not read the picture.

## Open items

- Cache hit count and the prof line for SR2 are not measured. SR3 needs the record counters first.
- The version wrap ceiling (above).

## Next

- SR3 (plan row SR3). Read the plan row first.

## Commands

- Build: `python 3ds/tools/gen_map_restore.py`, `PM_LANE=sr2 sh 3ds/build.sh`, restore 3ds/src/map_restore_table.c from a kept copy.
- Picture: `PM_ROW=SR2 powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row SR2 -Build sr2 -Try` and `... yard -Row SR2 -Build sr2`.
