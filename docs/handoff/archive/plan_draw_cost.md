# plan_draw_cost: cost of one draw close on the PICA200 path, mac_01 (O3DS)
Plan only. Read at HEAD b9fbebaf0. Rules: hw_guidelines.md. Tags: [M] measured, [C] code read, [L] citro3d/libctru behaviour (no source in tree), [I] inference or arithmetic.
Data source [M]: build3ds/ftp_170/hw_o3ds_log.txt, file lines 16308-34716 (scene A = mac_01), build 29f504d7b, 1120 frames with draws, medians.

## 1. One close, measured
- Draws 87 per frame. Tex-class closes 292, empty 235, non-empty 56 (dcsplit n). Empty closes make 0 svc calls now (ecsplit svc 0).
- fl tex 6285 us per frame. Each non-empty close costs about 101 us [I: (6285 - 235 x 2.6) / 56].
- Stages per close (dcsplit samples 1 in 8, x8, /56): draw 37.0, pal 16.9, st 12.0, bind 10.1, tev 10.0. Sum 86 us. About 15 us is in no field.
- Counts per frame: raw 67, tevr 35, tb0 61, palup 38, pmul 23.
- GPU time (gdraw) 8.1 ms. CPU frame (gproc) 47.8 ms. The GPU is busy about 17 % of the frame. Command words do not limit the GPU.
- Submit: 1 FrameSplit per frame (kick 1, 10 us mean, never above 1 ms), plus FrameEnd. Flush calls 5.8 per frame. No submit and no cache flush runs per draw [C pm_gpu.c:545, 261].
- gcache (GSPGPU_FlushDataCache of the vertex pool) 1.7 ms per frame [M diag_hw_log1]. Only 88 vertex slots (3.5 KB) are written [M gpusplit vb]. So the cost is the GSP IPC call, not the cache work [I].
- Texture hit lookup: 64 per frame, 17.1 us each, 1.1 ms [M thsplit]. It runs at the first draw (ensure_tile), outside G_PM_TEX.
- G_PM_TEX 10.5 ms = 56 closes x 101 us (5.7 ms) + bind body 88 x about 55 us (4.8 ms) [I arithmetic].
- dlc: 89 lists, 122 groups, 54 groups skipped by box, 68 replayed, all static (static grp = repl, fail 0), 757 tri per frame [M dlc/dlc2 f2699].
- opsplit f2482: vtx 44.6 us/op (55), tri 37 us/op (80), dlc 67 us/replay (100).

## 2. Answers
(a) The close writes few words: shadows already skip equal state (PG3 O4, DR1, DC2) [C pm_gpu.c:481-514, 873-934, 980]. DC2 removed the redundant dirty marks with no measured change [M diag_dc]. Console/Azahar per stage is 2.6 to 3.7x, above the 2.86 average: the cost is code-path length and cache misses, not register writes [I]. Removable without picture change: tick reads (about 2 us/close), the 304-byte sh5_fx memcmp and 5 uniform memcmps per pm5 draw (pal stage), the GSP IPC of the flush. Each one is small (rows below).
(b) A packed command block per (texture, combine, rendermode) key is possible only if we write raw GPUCMD words past citro3d. citro3d keeps its own copy of the state and sends a group again when its dirty flag is set or after a split [L]. A desync shows the wrong texture or blend: the same failure class as the reverted bind skip (HANDOFF.md:11). Slot addresses also move on eviction. Ceiling: tev+bind+st+half of draw (about 50 us) down to about 8 us, x 56 = 2.4 ms [I]. Gated, last.
(c) Already done. All draws of a split share one submit. Per-draw submit does not exist. Does not pay.
(d) Unsafe and does not pay. tb0 61 per 56 closes: each close is a real texture change. bind_w1 is 7 of 78 [M agent_modelops], so a sort merges at most about 7 closes (0.7 ms). It changes order at equal Z, for decals (GEQUAL plus bias), for alpha blend and for the zmark stencil passes. No row.
(e) Already done. dlc replay is 100 % static: no CPU transform and no vertex copy. The 67 us per replay is box tests, gv_matrix_id, the index write (dlc_seg, about 2270 u16 per frame) and the first-draw state work (ensure_tile, the 17 us texture hit, sync_render). A transform skip saves 0. The box test is about 1 us x 122 = 0.1 ms [I]. Does not pay.

## 3. Rows
| row | change | file, function | console ms saved, mac_01 | risk | Azahar check (azt mac01) | order |
|---|---|---|---|---|---|---|
| DS0 | Print one cold line `dcsum f` per 300 frames: non-empty closes, the 5 stage us, gcache us per flush call, flush calls, cmd words per close (C3D_GetCmdBufUsage delta, sampled close only). Use the existing pm_dc_c/pm_dc_us. LOGCLEAN removed the per-frame lines, so this is the only quantity source | pm_gpu.c pm_gpu_flush_at, gpu_flush_written; pm_gbi.c os_report | 0 | none | line present 2 times; PPM equal to build3ds/azt/shots/mac01/0000_1_2.ppm, 0001_1_2.ppm | 1 |
| DS1 | Static helper fl_dc(p, n): svcFlushProcessDataCache(CUR_PROCESS_HANDLE, p, n). On a failure result, call GSPGPU_FlushDataCache and count it. Use it at every GSPGPU_FlushDataCache call site | pm_gpu.c gpu_flush_written, pm_gpu_gstat_commit, line 77; pm_tex.c flush_pend_only | frame 0.5 to 1.5 of the 1.7 gcache; gbi 0.1 to 0.4 [I] | low-medium: the svc must accept the pseudo-handle [L]; fallback keeps the picture | fallback count 0; flush calls unchanged; PPM equal. Azahar cannot show the time: console dcsum gcache | 2 |
| DS2 | W6: compile pm_gpu.c with -Os. Move rare branches of flush_draw to cold noinline helpers: off_snapshot path, zmark pass 2, decal2, fl_count/fl_trace, dbg_cmp, nozt and imglog switch reads | 3ds/CMakeLists.txt; pm_gpu.c flush_draw, pm_tex.c pm_tex_bind | gbi 0 to 2 [I] | low (no logic change) | PPM equal; dcsum stage sum not worse than +5 %. The gain shows only on the console | 3 |
| DS3 | g_fx_gen and uvso/cmul/cadd/zmix gens: bump where pm_gpu.c writes g_fx/g_*; compare a gen, not 304 + 64 bytes | pm_gpu.c flush_draw pal block, lines 1984, 2081 | 0.2 to 0.4 [I] | low | dcsum pal us down >= 20 %; palup equal; PPM equal | 4 |
| DS4 | R4 again on console numbers: TexBind record (plan_c1 C1-2). Gate: DS0 shows bind body > 30 us per bind on the console | pm_gbi.c tb_run, case G_PM_TEX | 1 to 3 [I] of the 4.8 ms body | medium | 0a us down >= 20 x binds; tbcheck 0; PPM equal; pause and file select shots | 5 |
| DS5 | (b) packed block per key, raw GPUCMD, cache 128 keys, key holds the slot data address and the tex epoch. Only if DS2 leaves the draw+st+tev+bind sum > 60 us per close | pm_gpu.c flush_draw | up to 2.4 [I] | high | PPM equal on mac01, pause, file select, intro sign; 2 runs | 6 |

## 4. Rows that do not pay
- (c) one submit per split: already done. (d) sort by state: unsafe, <= 0.7 ms. (e) transform skip: already static.
- Empty-close skip: 0 svc now, a few us per frame. Tick-read removal (DC4): 2 ticks x 1 us x 87 = 0.2 ms.
- Palette shadow clears (DC3): 15 extra uploads per frame x about 2 us = 0.03 ms.
- TEV memo per key: <= tev 10 us x tevr 35 = 0.35 ms. PsoGpuPkt already covers compiled draws.
- Skip-if-equal state (R1 to R3): repeats are rare [M agent_modelops].
- Shader DVLB merge (pm5 into dvlb_all): the switch count is not measured. Add a count to DS0 first. Small [I].

## 5. Hardware
- DS1 removes one IPC per flush. The range and the count stay: once per split (rule 8).
- DS2 follows rules 1 and 10. Cold code leaves the hot I-cache lines.
- DS0 is cold and noinline. It reads ticks only in the 1-in-8 sample. No divide in hot code (rule 4).

## 6. First 3 rows for one Haiku code agent
1. DS0 (about 40 lines). 2. DS1 (about 20 lines). 3. DS2 (about 60 lines, CMake plus moves).
One Azahar start per row (azt mac01 -Try). A row that changes one picture byte is reverted. Console push at the end (orchestrator) with the dcsum line read.
Projected mac_01 after DS0 to DS2 [I]: gbi 31.7 ms to about 29.5 to 31.5 ms. Frame 60.6 ms to about 57 to 60 ms.
Draw-close work cannot reach 33.4 ms. All closes together are 5.7 ms. With every close at 0, gbi stays near 26 ms. The rest is the bind body, the dlc replay and the per-op walk (plan_dl_architecture).

### Critical files
- 3ds/src/pm_gpu.c (flush_draw 833-1132, gpu_flush_written 261, pm_gpu_flush_at 2241)
- 3ds/src/pm_gbi.c (os_report about 2138, tb_run 2593, case G_PM_TEX 3184, ensure_tile 2438)
- 3ds/src/pm_tex.c (pm_tex_bind 1462, load_impl 982, flush_pend_only)
- 3ds/CMakeLists.txt (DS2 -Os)
- 3ds/tools/azt_scenarios.json (mac01)
