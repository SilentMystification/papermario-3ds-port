# Handoff S10 epic (orchestrator, 2026-11-07)

STATE
- nds1 head: see `git log -1 nds1` (this file is committed on it). pm_gbi.c after GM-3.
- Done this session (all merged, byte gates cold det): DET (det switch), sprfix + SPR4 (native sprites incl. shaded), FB-1 (grab native, chunked ring), FB-2 (noise unit 2, template LRU), FB-3 (pm5 tex1/noise, sprite pm5 uniforms), MDL-5a/5f/5e (model records fully native, walker model fallback deleted), MOVE 1-10 + MOVE-4/5, LS-4 splash record, ENCCRASH, SIZE-4 hint, SIZE-3 step 1 (sqrtf -Os hang, UB fixes), SHOT, SCN, AZT11/12, PUSHHINT.
- Open rows: AUDIT (one Opus agent, row_AUDIT.md, report docs/handoff/audit_system.md, then its own fixes in worktree audit; no subagents). B2 (row_MENUB.md: warp-tab noise, console run needed; check commit on branch grey). S10 is complete. Merged 2026-11-08: CFG-3 4b8f2896c, GREY ae8176ad6 (no console check, user decision), CFG-1/CFG-2/B1 61c952e64.
- Parked bugs: layout-dependent pictures (mac_00 sky row, nok11 shot 0016: row_UB1.md, not byte gates), SBKW sbk_56 water residual (row_SBKW.md), FX1 super block (row_FX1.md: needs a menu-walk scenario). pm5all halo dropped (switch deleted in FB-4 C1).
- Size: 3dsx 13,316,040 B (build 2952, 7be88729b). Push to the O3DS XL works: 3dslink prints the compressed size ("3160810 sent (23.74%)" = zlib ratio, not a stopped upload); the earlier "failed at 23.84%" was the same ratio.
- Unmerged branches with useful work: ub1 (stale unit-1 cache patch, sprnat OOB patch).
- Process rules updated: orchestrator_rules.md "Process fixes".
- Open regression (user: leave open, 2026-11-08): same O3DS XL, mac_01 0 0 200 prof=4: a3c5ec093 run 25158 gbi 30.2 ms, fps 29.4, o34 2-9; 7be88729b run 25044 gbi 37.4 ms, fps 28.0, o34 18-30 (oh 8.0 -> 12.8, amort 2000 -> 3200). 407 first-parent commits between them (77 merges). Options: console bisect (about 9 pushes) or an oracle code read of the prof fields first.
- Disk: folders build3ds/wt/fb3, fb4, tdc, ls3 are held by a process (Azahar or a shell): run wt_gc.sh --apply again when Azahar is closed (it now removes stale folders; it keeps fresh branches, use --only for a fast-forwarded one).

## Questions for the user (collected overnight 2026-11-08)
1. Push target: memory says O3DS .170 since 2026-11-05; this session pushed to O3DS XL .131. Which one from now on?
2. Console prof push of nds1 (audit W1-W6 merged): does the 7.2 ms gbi regression (runs 25158 vs 25044) close?
3. CORE2-0/CORE2-1 console measurement (aujoin, core1 busy, ft): push when ready.
4. GREY, B2 and the args-vs-debug3ds order: one console session with debug logging (quick Select taps, warp tab scroll).
5. toad_standing recipe still has loglevel=debug: drop it?
6. CORE2-0 console run: include one prof run without the core-1 probe thread? (if the kernel counts the probe against the core-1 limit, it can slow audio; only that pair shows it)

