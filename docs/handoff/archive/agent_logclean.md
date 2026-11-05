# Handoff: LOGCLEAN (2026-10-29)

State:
- Log cleanup done. Per-frame phase split records removed (15 lines per frame at phase switch). Probe logs removed from pm_gbi.c, pm_gpu.c, pm_depth.c, pm_main.c. Comb audit removed.
- pm_log.c: no fsync in the sink thread. The writer no longer waits on an SD fsync.
- Boot line prints build, git hash, loglevel.
- Build 1417 (git 7ab19330c, uncommitted changes) compiles. Azahar yard and pause_menu log checks pass (3 of 3 each).

Open items:
- Picture check: a person reads build3ds\azt\shots\yard and build3ds\azt\shots\pause_menu.
- Hardware check: the stall fix (no SD fsync in the sink) is not measured. Run the push command after the epic check.
- Dead state with no output (remove in a follow-up row): gbiprof counters (GP_SET, GP_LAP, gp_* in pm_gbi.c and pm_gpu.c), dd capture (dd_on, dd_end, gbi_cold_dd_load, dd.*), imglog() and its switch tests, rectdbg snapshot (dbg_cmp), gbi_cold_po3a_hash, and the pm_ph_* counters that no output reads (pm_ph_kick_*, pm_ph_draws, pm_ph_vb, pm_ph_iv, pm_ph_hud_us, pm_gbi_*_ticks, pm_fl_*, pm_ec_*, pm_dc_*, pm_rs_cnt, pm_cs_cnt, pm_tex_th_frame).
- Azahar starts used for LOGCLEAN: 4 of 4 (yard before, pause_menu before, yard after, pause_menu after).

Commands:
- Build: python 3ds/tools/gen_map_restore.py, then sh 3ds/build.sh, then restore 3ds/src/map_restore_table.c from the user copy.
- Test: PM_ROW=LOGCLEAN powershell -File 3ds/tools/azt.ps1 yard -Row LOGCLEAN (and pause_menu).
- Logs: docs/handoff/log_inventory.md.
