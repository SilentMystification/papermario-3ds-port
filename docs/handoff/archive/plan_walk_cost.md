# plan_walk_cost: split and cut the 34.9 ms console walk (O3DS yard)
(Opus plan, saved by the orchestrator. Tags: [M] measured, [C] read from code at HEAD, [I] inference.)

## W0 first (protects the baseline)
- [C] main still has the failed M1 census: `if (dlc_on && pm_ph_on) dlr_count(next);` at pm_gbi.c:2542 and pm_dlc.inc.c:1385. Every push runs with `phase`, so every console build pays for it. Build 1227 (91b225755) does not contain it [M]. Azahar cost: main walk 9.53 to 11.43 ms (+1.9) [M diag_dlr]. Console about 5 ms [I, x2.7].
- Row W0: delete both calls, `dlr_count`, `DlrEnt`, `dlr_tab`, `dlr_c`, `dlr_buf` and the `dlr f` print (about 60 lines). Gate: Azahar taskpl main within 0.3 ms of 9.53 ms. Picture: Mario, ground, shadow.

## W1 measurement: sampled time per op (about 50 lines, pm_gbi.c, pm_port timer)
- Only with `phase` on (pm_ph_on). No new switch.
- Every op: `op_n[op]++` (exact count, no timer read).
- Sample when `((n_ops + os_off) & 63) == 0`: store `os_op = op`, `os_t0 = pm_ticks()`, and the flsplit flush-tick total as `os_fl0`. Close at the top of the next op, at the ENDDL sp==0 return and at the `end` break: `os_tk[os_op] += pm_ticks() - os_t0`, `os_sn[os_op]++`, `os_fl[os_op] += fl_now - os_fl0`.
- `os_off = rm_frame * 37 & 63` (varies the phase each frame).
- In G_DL, when `dlc_try` hits set `os_op = 0xFF` (pseudo-op "dlc").
- Free counters: `tk` (pm_ticks calls per frame), `gms` (memcmp steps in gv_matrix_id), `gmn`.
- Estimate per op: `os_tk/os_sn * op_n`. Cost about 71 timer pairs x 2.6 us = 0.18 ms per frame [I].
- Report every 300 frames from pm_gbi_end_frame, via pm_log_hw: `opsplit f%d ops %u | vtx mtx tri tex cc geo dl dlc sync oth: us/n each | top %02x:%u x6 | fl %u | tk %u | gms %u gmn %u`. Classes from `u8 os_cls[256]`: tex = the 7 ops at pm_gbi.c:56; cc = SETCOMBINE, PRIM, ENV, OTHERMODE_H/L, FOG; geo = GEOMETRYMODE, MOVEWORD, MOVEMEM, SCISSOR; mtx = MTX, POPMTX; sync = RDP syncs and NOOP (dispatch cost alone).
- Azahar (diagnostic budget 3): `PM_LANE=wc PM_ROW=W1 powershell -File 3ds/run_azahar.ps1 -Timeout 90 -LockWait 180 -Debug "fast preset=yard phase fixedseed shotmode=4:80"`. Until-line: first `opsplit f` after f599.
- PASS: `ops` within 1% of taskpl ops; class sum within 15% of taskpl bg+main; at least 2 steady yard lines; shot shows Mario, ground, shadow. Table to 3ds/logs/perf/status_W1.txt.
- Console: push command (rule 6), read `opsplit f` in build3ds/hw_live_log_170.txt.

## Candidate rows (gain [I] until W1; each gated on W1, 30 to 60 lines, one quantity)
| # | Row | Gate (W1) | Expected | Risk |
|---|---|---|---|---|
| W2 | Sample phase instruments 1 in 8 (streak edges pm_gbi.c:42-60, flush_at timing, sync_combine_work_timed), as DC1 does | tk x 1.3 us >= 1.5 ms | 1 to 3 ms | none |
| W3 | gv_matrix_id: 64-entry hash (FNV of the 16 words, exact memcmp on hit) replaces the linear scan of up to 256 x 64 B (gm_tab 16 KB = all of L1D). `combined` lazy: MTX/POPMTX set a dirty bit, first reader calls upload_mvp | vtx+mtx >= 3 ms or gms > 2000 | 0.5 to 2 ms | low (hash), medium (lazy combined: every reader through the getter) |
| W4 | Dispatch prologue (pm_gbi.c:2489-2514): one `u8 opf[256]` flag table (TEX, TRI, NOEPOCH) replaces the two streak calls and the 8-way compare; one local `dbg` word read once per pm_gbi_run | sync class >= 1.5 us/op | 0.3 to 1 ms | low |
| W5 | Per-close cost: DC1 field for the 31 us per close no field covers; split draw into UpdateContext and DrawElements; upload palette rows only when changed (pm_gpu.c:805) | draw or untimed >= 20 us/close | 0.5 to 1.5 ms | medium (stale uniform) |
| W6 | I-cache: compile pm_gbi.c and pm_gpu.c with -Os (set_source_files_properties); mark pipelog/po3a/dd/mtxlog blocks cold | W2-W4 leave per-op > 5 us | 0 to 3 ms, console only | low; Azahar cannot show it |
- Batching more draws per close is not a row (67 non-empty closes; merge at most 0.88 ms; empty closes 0.7 ms). The lever is cost per close (W5).
- Do not repeat: combiner-flush skip, texture-bind skip, per-op timers, DL replay classes 2/3.

## Hardware [I]
- O3DS ARM11 268 MHz, 16 KB L1I, 16 KB L1D (4-way, 32 B lines), no L2. A miss goes to FCRAM at 30 to 60+ cycles.
- 7.6 us/op is about 2040 cycles: tens of misses per op, not branch cost. The hot path (pm_gbi.c handlers + sync_combine + flush + citro3d) is far over 16 KB, so a change of op class probably evicts handler code. Fits the 2.7x console/Azahar ratio (Azahar does not model these caches). W6 tests this.
- Scanned tables of 16 KB or more (gm_tab) flush L1D. Keep per-op state in few lines.
- linearAlloc rings (pm_gpu.c:1165) are cached and flushed once per split; no change needed.
- One core does the work; no row moves walk work to core 1.

## Order and stop rule
- W0, W1 (console), then W2 to W6 each only if its W1 gate holds, in table order.
- Each code row: one Azahar start for the quantity plus the picture (Mario, ground, shadow); console push at the end of the epic (rule 6). A miss is reverted (rule 7).
- Stop after two rows in a row that each give less than 1 ms on the console wall, or when W1 shows no gated row.
- Sum of W2-W6 about 3 to 10 ms: about 48 to 55 ms, not 33.4 ms.
- After the stop, give the user the architecture decision: walk on N3DS core 2 with one frame of latency, or a geometry cache for classes 4-6.

Files: pm_gbi.c (dispatch 2449-2560, streaks 35-60, gv_matrix_id 1367, load_mtx 1990, report 2107), pm_dlc.inc.c (dlr_count 939, 1385), pm_gpu.c (palette 771-860, flush_at, rings 1165), pm_phase.c (ph_format 316-333, dcsplit), 3ds/CMakeLists.txt (W6).
