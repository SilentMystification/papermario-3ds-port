# Plan: RAM-resident game on O3DS (Opus plan, saved by the orchestrator)

Goal: no asset read from the SD card during play. Console IP is 192.168.1.170 (the plan text said 168: corrected).

Change from the request: "decode everything, then drop the raw ROM image" frees no memory. Every game read goes through `pi_read_raw` (3ds/src/pm_os.c:372-430) and copies raw bytes out of `rom_bytes`. Decoding all of it costs more than it frees: maps grow from 9.75 to 21.74 MB, audio from 10.26 to about 36 MB. The packed raw image already in the heap is "play entirely from RAM". Work:
1. Confirm the console boot line says RAM resident.
2. Remove the 2.16 MB of MIPS code from the image.
3. Delete the SD streaming fallback so nothing can read the SD card during play.

## 1. Measurements (one Azahar start each, PM_ROW=RR0)
| # | quantity | command | pass line |
|---|---|---|---|
| M1 | rom mode, Azahar | `run_azahar.ps1 -Until "rom mode" -Timeout 60` | `rom mode: RAM resident 26.47 MB` |
| M2 | rom mode, O3DS console | push command (CLAUDE.md rule 6, `--ip 192.168.1.170`), then grep `rom mode\|mem split\|cap` in build3ds/hw_live_log_170.txt | `RAM resident`. If `SD STREAMING`, read the free MB |
| M3 | peak heap per map | `run_azahar.ps1 -Debug "tour=<10 heaviest maps>" -Until "tour done"`, grep `mem map heap` (pm_map.c:575) | max used KB at most 47742 + 3458, sbrk-left above 0 |
| M4 | SD reads during play | same run with `-Debug phase`, grep -c `sdrd\|sdread` | 0 lines |
| M5 | decoded texture MB | skipped: textures decode on a miss from the RAM image into the 1.5 MB VRAM cache | |
| M6 | decoded audio MB | skipped: VADPCM decodes per tick into small buffers. Keep audio raw | |

## 2. Decision tree
- M2 RAM resident and M3 fits (expected 27.1 MB of a 47.7 MB cap): do RR1 to RR3. Keep the raw image. No decoded-map preload. Keep per-map decode. Keep the texture cache at 1.5 MB.
- M2 SD STREAMING or M3 over the cap: do RR1 first (frees 2.16 MB), rerun M2. If still no fit, RR-F: per-map residency (about 60 lines in pm_os.c). At map load copy into one heap block only the keep ranges the map reads (shape, hit, tex, shared audio, sprites, msg). Load inside a 500 ms budget (the `romstall` threshold, pm_os.c:47-55). Pass: `sdrd` lines 0 outside the map-load window.

## 3. Rows
- RR1 (about 30 lines, 3ds/tools/gen_rom_remap.py, regenerated 3ds/src/rom_remap.h): drop the splat `type: code` ranges that the 3dsx links as ARM. Keep `main` (the trace reads 8,544 B there). Keep every range `rom_trace_check.py` reports as read. Steps: trace with `romfull`, `python 3ds/tools/rom_trace_check.py full.txt packed.txt`, regenerate, build. Azahar test (one quantity, packed MB): `-Until "rom mode" -Capture` shows about 24.3 MB, shot shows the title screen. Second gate: `grep -c "IN DROPPED RANGE"` is 0 over the M3 tour. Console: `rom mode` about 24.3 MB and the title draws.
- RR2 (about 40 lines, pm_os.c:250-300 and 430-440, pm_phase.c sdrd, pm_gpu.c HUD flag): if the packed image does not fit, `pm_rom_init` shows `pm_fatal_screen("Not enough memory for the ROM: need X MB, free Y MB")`. Remove the ROM FILE stream branch in `pi_read_raw`, `pm_au2_rom_lock`, `rom_sd_*` and the HUD SD flag. `romfull` becomes Azahar-only or is deleted after the RR1 trace. Azahar test (`sdrd` count): M4 with `-Capture`, 0 lines, shot shows a gameplay map. Console: `RAM resident`, no `sdread` lines after a walk of two maps.
- RR3 (about 30 lines, pm_tex.c:613-650), only if `texbaked` is kept: read texbake.pak once into one heap block after the ROM, or delete the pack path (default if no row uses `texbaked`). Azahar test: `-Debug texbaked` shot matches the M3 shot, `sdrd` 0.
- RR-F: only if the second M2 run fails.

## 4. Risks
- Data in code ranges: a splat `code` range can hold rodata read by DMA (as `main` does). A read in a dropped range is zero-filled (pm_os.c:408-409) and fails silently in the picture. Guards: trace diff and the `IN DROPPED RANGE` count. The tour must cover battle, pause and file menu.
- Low headroom: about 20.6 MB of heap left after the image before RR1. Toad Town and the Bowser castle maps can come close. M3 must include them.
- Console heap: the 45056 KB and 51200 KB lines have no console tag. Only M2 on the real O3DS settles the fit.
- Save writes: `save.flash` writes use the SD card on a worker thread. Writes are not asset reads and stay.
- Debug SD reads in pm_input.c, pm_sweep.c, pm_warp.c, pm_mapdata.c stay (switch-only).
- Lost fallback: after RR2 a console with less free heap gets a fatal screen instead of a slow game. The user must accept this.
