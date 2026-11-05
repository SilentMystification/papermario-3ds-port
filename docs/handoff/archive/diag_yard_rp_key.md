# Diagnosis: yard census, 3 entries with a different template key under recplay

Branch nds1, worktree `papermario-3ds-port-nds1`, base HEAD ba5d4ffa3. Lane build `nds1` (debug prints, reverted). Azahar starts: 2 (status lines `diag YRP-A` and `diag YRP-B` in `3ds/logs/perf/status_NDS1.txt`).

## Verdict

(b). The census key artifact. The differing field is `prim` (primary colour, set by `gDPSetPrimColor`). The emitted PICA words do not differ for these draws.

## Fact 1: the census of the two runs

| Run | Batches | Tris | Distinct | Note |
|---|---|---|---|---|
| YRP-A census_yard_rp (replay on, prints) | 46 | 510 | 39 | equal to the fact |
| YRP-B census_yard (replay off, `reccheck` on, prints) | 54 | 744 | 46 | not the control |

`reccheck` changes the batch split. The record compare flushes the command buffer (`pm_gpu_rec_begin`), so YRP-B has more batches. YRP-B is still valid for the word compare (Fact 4). It is not valid for the batch count. The control without `reccheck` was not run.

## Fact 2: the three draws

| Tex0 | Tris | Replay key (YRP-A, capture) | Live key (YRP-B, frame 200) |
|---|---|---|---|
| 72ecc073 | 17 | 64e9a795 | e4cc9a15 |
| cfffd811 | 2 | 64e9a795 | e4cc9a15 |
| f4b0d655 | 6 | 64e9a795 | e4cc9a15 |

Record id, group and model of the three draws: NOT obtained. The YRP-A log was overwritten by the YRP-B run (both write `build3ds/lane-nds1/last_log.txt`). The `RPL f200` lines of YRP-A are lost. The live `TK` lines of the three draws are also not printed. The debug print in `cen_add` skips draws inside a record window (`rec_cap`), and these draws are inside one.

Records compared at frame 200 in YRP-B (state 1, reccheck): slots 4, 23, 21, 38, 52, 50, 53, 51, 39, 54, 33, 24. Whether the three draws belong to these slots is INFERRED from the suppressed live lines, not proven.

## Fact 3: the two TevKey structs

Source: `3ds/src/pm_gpu.c:474` (struct `TevKey`). Hash in `cen_add` (`pm_gpu.c:495`): all words except `plan_gen`, then `z`, `graw`, `prog`, `cull`.

Replay key 64e9a795 (capture prints, 284 lines, all identical):

| Field | Value |
|---|---|
| combine | 2 |
| tex | 1 |
| tex1 | 0 |
| i8_0 | 0 |
| i8_1 | 0 |
| rgb_scale | 1 |
| r_mix | 0 |
| draw_3d | 1 |
| rect_2d | 0 |
| dview | 0 |
| off_active | 0 |
| off_i8 | 0 |
| r_memblend | 0 |
| r_replace | 0 |
| r_blend | 0 |
| **prim** | **0x00000000** |
| cconst | 0x00000080 |
| mixcol | 0x00000000 |
| plan_gen | 0 (not hashed) |
| z, graw, prog, cull | 1, 1, 4, 2 |

Live key e4cc9a15 (6 live prints at frame 200, identical; not the three draws themselves):

| Field | Value |
|---|---|
| combine ... r_blend | same as above (all 15 fields equal) |
| **prim** | **0x00000080** |
| cconst | 0x00000080 |
| mixcol | 0x00000000 |
| plan_gen | 0 (not hashed) |
| z, graw, prog, cull | 1, 1, 4, 2 |

Only `prim` differs between the two field sets. The hash is FNV over these words, so `prim` explains the hash difference.

## Fact 4: the emitted words (record against live, frame 200)

YRP-B ran `reccheck`. `rec_check` (`3ds/src/pm_gbi.c`, debug print `RC f200`) compared each record with the live words of the same frame. Full word equality, no mask.

- 12 records compared at frame 200. Each has 264 words.
- Each record has exactly one differing word: register 0x227 (index address, patch slot). The TEV words (0x0C0 to 0x0FF), colour words, texture words and palette words are equal in all 12.
- The rule for 0x227 is the arena rewrite of `pm_gpu_rec_idx`. It is not a state word.

## Fact 5: where prim reaches the words

`prim` is read at these places in `3ds/src/pm_gpu.c`:
- 698: `C3D_TexEnvColor(env, abgr(prim))`, branch `tex && combine == 6`.
- 706, 712: `C3D_TexEnvColor`, branch `tex && (combine == 5 || combine == 7)`.
- 739: `C3D_TexEnvColor`, branch `combine == 1` (no texture).
- 2441 to 2442: `pm_gpu_set_prim` flushes only for combine 5, 6, 7.

The branch for combine 2 with texture (pm_gpu.c, `else if (tex)`) uses GPU_PRIMARY_COLOR and never calls `C3D_TexEnvColor`. The draws of this key are combine 2 with texture, so `prim` does not reach their TEV words.

## Fix

Census only (this key artifact): in `cen_add` (`3ds/src/pm_gpu.c:495`), exclude `prim` from the hash unless `combine` is 1, 5, 6 or 7. For those combines, keep the mismatch. It is a real signal there.

Replay correctness (separate, NOT measured): `RecKey` does not hold `prim`. `rec_g0_set` (`3ds/src/pm_gbi.c:~3610`) stores `comb_w0`, `comb_w1` and the mode words, not `prim`. For combine 1, 5, 6 and 7, a replayed record would write the TEV constant of the capture time. Adding `prim` to `rec_g0` does not fully fix it. `prim` is set by commands outside the record, so the draw-level value can differ from the value at group start. The clean fix is to store the TEV constant as a per-draw patch word, or to refuse replay when the combine is 1, 5, 6 or 7 and `prim` changes. Decide with a test. Not started.

## Region diff: yard with and without recplay

Command: `python -I 3ds/tools/region_diff.py build3ds/azt/shots/census_yard build3ds/azt/shots/census_yard_rp`

- `0000_0_11.ppm` (frame 200): PASS, 0 px differ (0.00 %), 0 regions.
- `0001_0_11.ppm` (frame 204): PASS, 0 px differ (0.00 %), 0 regions.
- 2 of 2 shots pass.

YRP-B had `reccheck` on, so its frames came from the old path with compare only. The shot check does not prove the replay path is correct on other maps.

## Not done

- Record id, group and model of the three draws (log lost, see Fact 2).
- A control run of census_yard without `reccheck`.
- The picture is not read by a person. The shot files are in `build3ds/azt/shots/census_yard_rp/` and `build3ds/azt/shots/census_yard/`.
- The prim-colour replay risk (combine 1, 5, 6, 7) is not measured.

## Commands

- Census replay on: `PM_ROW=NDS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_yard_rp -Row NDS1 -Build nds1 -Try`
- Census replay off: `PM_ROW=NDS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_yard -Row NDS1 -Build nds1`
- Debug edits used (reverted): `DBG-YRP` prints in `pm_gpu.c` (`cen_add`, `pm_gpu_rec_play`) and `pm_gbi.c` (`rec_play`, `rec_check`). Scenario debug strings changed for the two runs (reverted).
