# Ranking of the levers left after DLC2 (lane B, 2026-10-22)

Basis: console build 970 town (hw_p4_findings.md): lo 8.4, dl 3.2, gb 40.4 (about 31 with DLC1 and DLC2 by the Azahar x3.5 estimate), gp+gd+gc 9.9, ot 2.7, fe 1.2, au 0.8, tx 0.2 ms. Gains are console ms per drawn step. Data letters: M measured on the console, A measured in Azahar, E estimate.

| Rank | Lever | Gain (ms) | Risk | Basis | Status |
|---|---|---|---|---|---|
| 1 | PF1: software prefetch (PLD) in the interpreter, G_DL targets, G_VTX data and DLC group data. Azahar has no cache model, so only the console shows it | 0.5 to 1.5 (E) | none for the picture (a hint instruction); 0 if the stalls are not memory bound | M: 5.8 us per gbi op on the console against 1.5 us in Azahar. Streaming data per step: display list 46 KB, vertices 38 KB, so 2600 cache lines, at most 0.8 ms of misses (E) | Done in this row (switch `nopf`, abtog state `nopf`) |
| 2 | GC1: fewer GSPGPU_FlushDataCache calls (6 per step, 60 KB, gc 1.6 ms on the console) | 0.5 to 0.8 (E) | medium | M: gc 1.6 ms. Merge the ring, index and pool flushes with the KICK1 split | Not started, console only |
| 3 | Lean draw (DLC5, pre-built register packets) | 2 to 4 (E), unproven | high | A: gd 15 us per draw, gp 8.6 us. DR1 and RAW1 did not get below 79 us on the console. The cost is inside C3D_DrawElements | Not started. Needs the citro3d source to see which dirty flags set per draw |
| 4 | Texture hit path (hash index) | 0.2 to 0.3 | low | A: 162 hits per step, 68 by the recent list, 93 by scan, 5019 slot checks per step = 50 k cycles = 0.2 ms | Too small to do |
| 5 | Fewer draws (merge draws with equal state) | 0.2 (up to 0.8 if colours could be dynamic) | high | A: 3.6 percent of the 8203 draws per 300 steps have a state equal to the draw before (16 percent when prim, env, matrix and colour are ignored). The flush sites that draw are G_SETTILE (35 per step) and G_LOADTLUT (20 per step): real texture changes | Dropped. An atlas design is out of scale for 0.2 ms |
| 6 | DLC3 kind S (skip repeated state lists) | 0 | n/a | A: 0 skip candidates in 87614 G_DL targets (DLC.md section 7) | Dropped |
| 7 | AU1b to AU1d (audio to core 1) | 0 | n/a | M: AU2 already runs the render on core 1. Audio join is 0.6 to 0.8 ms, no more to gain | Superseded by AU2 (done) |
| 8 | TC1 (cache full in 4 scenes), O13 (VRAM tier), HW6 (NPC spawn reads) | under 0.3 steady | medium | M: texture evictions 0 in the whole 270 s town run, tx 0.2 ms. They act on map load spikes (3 per 270 s) | Not worth a row now |
| 9 | MA1 logic sites (lo 8.4 ms) | 0 to 0.5 | game logic: needs a Q-MA1 entry | parked | No entry exists. Not touched |

Reading: after DLC2 no Azahar-measurable lever above 1 ms is left. The remaining CPU part is spread over the decomp game code (lo, dl, ot = 14.3 ms), the gbi ops that must run (state ops 4.6 ms in Azahar) and the citro3d draw call. The next real information comes from the console: build 1035 (DLC2) with `phase phsum abtog=base,dlc1,dlc` (hw_batch.md 8.0e) and then `abtog=base,nopf` for PF1.
