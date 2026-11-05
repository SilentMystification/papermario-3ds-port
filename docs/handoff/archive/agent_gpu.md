# GPU path epic (display-list + GPU moves)

Branch **dlc-compiler-c1c2** at git **473538b5d** (committed tip). All GPU-path code is **uncommitted** in the working tree. Never commit `3ds/src/map_restore_table.c`.

## Done (working tree, Azahar)
| Stage | State |
|---|---|
| C0–C7 | In tree; compiler path through templates (C5), register packets (C6), glyph atlas (C7) |
| GP1 | Hybrid G_VTX box (in tree) |
| GP2 | PICA user clip: `gp2_clip_dat[3] = f32tof24(1.f)` in `pm_gpu.c` — do not revert |
| GP3 | Lazy texture bind (`pdl_lazy_tile`: return when `td[0].valid`) — do not revert |
| GP5 | **Skipped** (no palflush rate in logs) |
| TX-B | Restored: I→L8/L4, IA→LA4, native Morton upload; LA8 expand removed for those formats |
| VRAM cache | **1536 KB** in VRAM via `vramMemAlign` at **0x1f05dc00** (was **4.75 MB** FCRAM linear slab + per-tex `C3D_TexInit` fallback; both removed) |

Picture (toad frame 251 vs `3ds/golden/toad.png`, tol 8): TEXPLACE **0.20%**, TX-B **0.21%**, VRAM **0.21%** — town geometry and textures on correct surfaces.

Status files: `3ds/logs/perf/status_texplace.txt`, `status_txb.txt`, `status_vram.txt`, `status_pic3.txt`.

## Archive
Copy before any console push: `3ds/logs/builds/pm_3ds_1088_gpu.3dsx` (GPU-path snapshot; pair with `pm_3ds_1088.elf` / `.map` in same folder if present).

## Open
- **TX-A** blocked on plan section 13 **question 5** (texture miss cost after FL1 / DisplayTransfer gate).
- **Core split** (producer/consumer): needs user decision on **one drawn step of input latency** (HANDOFF section 5a, plan section 2.10 / item 6).
- **Hardware test** not run for this epic (no console push, no live log).

## Next
1. User runs **hardware test** on O3DS (archive above, then push only when they want — orchestrator gives push per CLAUDE.md rule 6; **do not run push from this handoff**).
2. **TX-A** only if the user answers question 5 and wants DisplayTransfer tiling.

## Commands (Azahar only)
Before build: `python 3ds/tools/gen_map_restore.py`, then restore `3ds/src/map_restore_table.c` from its `.bak`.

One toad shot (example row `VRAM`; use `VRAM2` if `VRAM` is 8/8 in `3ds/logs/test_budget.csv`):
```text
PM_ROW=VRAM powershell -NoProfile -File 3ds/run_azahar.ps1 -FaultStop -Timeout 120 -Until "shot 0 frame" -Debug "fast preset=toad fixedseed dlc shots shotwin=251:1"
```
Compare `build3ds/shots/0000.ppm` to `3ds/golden/toad.png`. Do **not** run the console push (`3ds/push_3ds.sh`) until the user asks.

Plan: `docs/plans/full_gpu_path.md`.
