# Handoff S10 epic (orchestrator, 2026-11-07)

STATE
- nds1 head: see `git log -1 nds1` (this file is committed on it). pm_gbi.c after GM-3.
- Done this session (all merged, byte gates cold det): DET (det switch), sprfix + SPR4 (native sprites incl. shaded), FB-1 (grab native, chunked ring), FB-2 (noise unit 2, template LRU), FB-3 (pm5 tex1/noise, sprite pm5 uniforms), MDL-5a/5f/5e (model records fully native, walker model fallback deleted), MOVE 1-10 + MOVE-4/5, LS-4 splash record, ENCCRASH, SIZE-4 hint, SIZE-3 step 1 (sqrtf -Os hang, UB fixes), SHOT, SCN, AZT11/12, PUSHHINT.
- Open rows: none in S10. S10-4: no full-catalog tool exists (bench.py runs one bench per call; sweep.py is cutscenes only; golden.py forbidden), so S10-4 was the console prof push only. Merged this session: DEL+T1F, TDC, LS-3, RS3, S10B, S10C, GLOB (END-1 closed, no delete), S10-1, S10-3.
- Parked bugs: layout-dependent pictures (mac_00 sky row, nok11 shot 0016: row_UB1.md, not byte gates), SBKW sbk_56 water residual (row_SBKW.md), FX1 super block (row_FX1.md: needs a menu-walk scenario). pm5all halo dropped (switch deleted in FB-4 C1).
- Size: 3dsx 13,316,040 B (build 2952, 7be88729b). Push to the O3DS XL works: 3dslink prints the compressed size ("3160810 sent (23.74%)" = zlib ratio, not a stopped upload); the earlier "failed at 23.84%" was the same ratio.
- Unmerged branches with useful work: ub1 (stale unit-1 cache patch, sprnat OOB patch).
- Process rules updated: orchestrator_rules.md "Process fixes".
- Next: console result at mac_01 pos 0 0 200 (run 25044, O3DS XL, 7be88729b): fps 28.0, gbi 37.4, o34 22. Last same-spot console run (342, O3DS .170, a3c5ec093, 2026-11-06): fps 29.3, gbi 30.2, o34 8. Different console: a push of a3c5ec093 to the O3DS XL tells if the +7 ms gbi is the build or the device (user decides).
- Disk: folders build3ds/wt/fb3, fb4, tdc, ls3 are held by a process (Azahar or a shell): run wt_gc.sh --apply again when Azahar is closed (it now removes stale folders; it keeps fresh branches, use --only for a fast-forwarded one).
