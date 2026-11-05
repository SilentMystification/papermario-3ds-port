# Log inventory (row LOGCLEAN, 2026-10-29)

## 1. Call sites in 3ds/src (before the row)

Level: pm_log_e 0, pm_log_w 1, pm_log and pm_log_i 2, pm_log_d and pm_log_hw 3. The push runs loglevel=debug, so every level prints.

| Source file | Call sites (approx.) | Main content |
|---|---|---|
| pm_gbi.c | 53 | per-op probes (mtxlog, pipelog, modifyvtx, gbi skip), comb audit, dd/imglog, gbiprof |
| pm_gpu.c | 43 | box probes (boxlog), gbiprof flush, rectdbg, shot, VRAM, GPU limit |
| pm_warp.c | 38 | warp and scene lines |
| pm_tex.c | 33 | texture cache, texstat |
| pm_phase.c | 31 | phase windows (kept), per-frame split records (removed) |
| pm_main.c | 26 | boot, pace, fps, logflood (removed) |
| pm_options.c | 17 | options |
| pm_mem.c | 15 | MEM and VRAM |
| pm_dspaud.c, pm_audio.c, pm_au2.c | 37 | audio and dsp summaries |
| other | about 90 | tests (fxtest, dsptest, dbggame), input, os, flash, interp |

Total: 379 call sites. 163 are at debug level (pm_log_d or pm_log_hw).

## 2. Lines per category in a debug log

Source A: O3DS hardware log, build3ds/ftp_170/hw_o3ds_log.txt (62660 lines, about 3874 frames, before the row). Source B: Azahar yard and pause_menu logs with loglevel=debug and no push flags (before and after the row).

| Category | HW lines (A) | Per frame (A) | Status after the row |
|---|---|---|---|
| tilesplit, tbsplit, tbdbg, taskpl, rsplit, rmsplit, gbisplit, gabsplit, dlcsplit, combsplit, flsplit, ecsplit, dcsplit, thsplit, gpusplit (15 kinds) | 58097 | 15.0 | removed (pm_phase.c ph_format, PhRec ring) |
| phase, phs, phh, phc, spike | about 2760 | 0.7 | kept |
| audio, pace, prod, b7 | about 880 | 0.2 | kept |
| texstat, tx1, dr1, dlc, dlc2, pdl, gv1, gv1pal, gp1, gp3 | about 260 | 0.07 | kept (texstat); others kept, see section 4 |
| menu, mem, warp, game, opsplit, logstat, logstat2, logtop, rom, vram | about 390 | 0.1 | kept |
| phrec dropped (writer behind) | 23 events | | removed (no frame records) |

Azahar, same switches (A = after, B = before):

| Run | Lines before | Lines after | Frames |
|---|---|---|---|
| yard (loglevel=debug) | 178 | 172 | 389 |
| pause_menu (loglevel=debug) | 264 | 263 | 209 |

The Azahar runs have no phase switch, so the 15 split kinds did not appear in them. The change in those runs is small (6 lines in yard, 1 line in pause_menu).

## 3. Deleted code (this row)

- pm_phase.c: PhRec ring, ph_format (15 per-frame lines), pm_ph_drain, phasecsv path (pc and pcx lines).
- pm_log.c: per-second fsync in the sink thread (see section 5); pm_ph_drain calls.
- pm_gbi.c: comb audit (comb new, comb text, comb dbg, comb count, comb total, PO2, audit table and counters); per-op probes (dd, ddtex, ddv, ddverb, rm, imglog, pipemtx, mtx, mtxv, pipelog, gbi skip, gbi ignore moveword and movemem, modifyvtx logs, B6 key, settdbg, glyphbind, xt, xt fillrect, po3a and gbiprof summary lines, is_host bound); cold helpers gbi_cold_pipelog, gbi_cold_mtxlog, gbi_cold_xt_fill, gbi_cold_movemem, gbi_cold_keylog; mtxlog_on and mtxlog_ok.
- pm_gpu.c: flush_boxlog (box raw, box tex, box draw), box fill, X02, flushlog, gbiprof flush and texrect lines, rectdbg line.
- pm_depth.c: F03 query line.
- pm_main.c: logflood loop.

## 4. Kept (and why)

- Boot: build id, git hash, loglevel (new line 8 of every log), MODE, rom, mem boot, vram boot.
- Per-window and per-second summaries: phase W and phase ms/count lines, phs, phh, phc, pace, texstat, opsplit, prod, VRAM and MEM, spike.
- Faults and fatal lines, ui markers, texfail, shot lines, warp lines.
- Still present, not removed in this row: gbiprof counters (GP_SET, GP_LAP and gp_* arrays, no output now), dd capture (dd_on, dd_end, gbi_cold_dd_load, dd.* fields, no output now), imglog() and the imglog switch tests in grab_cur and draw_tri (no output now), rectdbg snapshot (dbg_cmp, no output now), gbi_cold_po3a_hash (no output now). Each has no log output, but the state code still runs under its switch. Follow-up row: remove these after the picture check.
- b7 (per 30 frames) and audio summaries are kept because they are not per-frame.

## 5. Writer stall (pm_log.c)

- The sink thread held fsink_lock during a once-per-second fsync. The writer (fb_put) takes the same lock when fb is full (128 KB). An SD fsync takes seconds, so the writer waited.
- Hardware log: writer fbuf max 6.7 to 7.1 s; fb 122137 of 131072 bytes; phrec dropped 23 times.
- Fix: the sink thread no longer fsyncs. fb_sync_all (drain, stop) and the crash path still fsync.
- Not measured on hardware. The fix is the expected cause of the fbuf stall, based on the code path and the log values.

## 6. Tests

- yard, pause_menu, both with -Row LOGCLEAN, loglevel=debug added to both scenario debug strings in 3ds/tools/azt_scenarios.json.
- Log checks: yard 3 of 3 pass, pause_menu 3 of 3 pass.
- Walk ms per frame (from warp scene lines, scene frame difference / time difference):
  - yard f100 to f300: before 34.0 ms, after 34.4 ms.
  - pause_menu f2 to f182: before 34.7 ms, after 34.6 ms.
- Picture: the shot files were written to build3ds/azt/shots/yard and pause_menu. A person must read them and confirm the subject.
