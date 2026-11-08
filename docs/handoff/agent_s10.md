# Handoff S10 epic (orchestrator, 2026-11-07)

STATE
- nds1 head: see `git log -1 nds1` (this file is committed on it). pm_gbi.c after GM-3.
- Done this session (all merged, byte gates cold det): DET (det switch), sprfix + SPR4 (native sprites incl. shaded), FB-1 (grab native, chunked ring), FB-2 (noise unit 2, template LRU), FB-3 (pm5 tex1/noise, sprite pm5 uniforms), MDL-5a/5f/5e (model records fully native, walker model fallback deleted), MOVE 1-10 + MOVE-4/5, LS-4 splash record, ENCCRASH, SIZE-4 hint, SIZE-3 step 1 (sqrtf -Os hang, UB fixes), SHOT, SCN, AZT11/12, PUSHHINT.
- Open rows: AUDIT (Opus, worktree audit: W1-W10, C4, B2, C8 merged up to 0ec3ded3f; now A5 close path under 16 KB, batch-state struct, rest of the pm_gpu.c split). CORE2 (Opus, worktree core20: CORE2-0/1/2 merged; hazard fixes now; CORE2-3 waits for an N3DS run). B2 menu noise (row_MENUB.md, console). Merged 2026-11-08: CFG-3, GREY, CFG-1/2+B1, HUD2, audit W1-W10/C4/B2/C8, CORE2-0/1/2. main = nds1.
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
7. HUD2 (merged): the status bar is edge to edge in wide, items at 4:3 size. The left end column is extended by the wide edge rule: check the bar ends by eye on the console. Spread the items across the bar too?
8. AUDIT: one console_prof pair at spot_mac01_prof, base 53e5a553e vs nds1 with W1-W10 (Azahar shows no change: 5.9 ms both; it has no cache model).
9. AUDIT: should the stereo second walk become a replay of the first walk (pm_nusys.c:94)? It changes how eye 2 draws.
10. CORE2-1 (merged 114e6ae0a): console prof spot_mac01_prof and spot_iwa10_prof on O3DS, 6ac37d50c vs 6001f31f5: aujoin_us must drop, ft_ms and o34 must not rise.
11. CORE2-2 (merged 554b9b941): one N3DS prof run (core2 probe line, ft_ms) decides CORE2-3 (walker thread on core 2).
12. audiohash is a near-equality check (+-2 commands run to run, DSP voice ends feed back): accept, or make voice ends deterministic under det?
13. AUDIT B2 (merged): mac_00 pond blocker now draws (prim depth fix). Look at build3ds/audit_shots/b2_pond_before_after.png: the screen bottom is now flat dark blue where the hedge texture showed. Is that the correct look (N64)? If not, revert merge of 899127979.
(Decided by the orchestrator overnight: pause_spin_grab_det_w300 accepted as layout-sensitive with the CORE2-1 control build (row_UB1.md); B2 depth readback in Azahar yes; delete 66 unused debug switches per D9 yes; split pm_gpu.c as a pure move yes.)

