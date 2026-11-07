# Handoff S10 epic (orchestrator, 2026-11-07)

STATE
- nds1 head: see `git log -1 nds1` (this file is committed on it). pm_gbi.c after GM-3.
- Done this session (all merged, byte gates cold det): DET (det switch), sprfix + SPR4 (native sprites incl. shaded), FB-1 (grab native, chunked ring), FB-2 (noise unit 2, template LRU), FB-3 (pm5 tex1/noise, sprite pm5 uniforms), MDL-5a/5f/5e (model records fully native, walker model fallback deleted), MOVE 1-10 + MOVE-4/5, LS-4 splash record, ENCCRASH, SIZE-4 hint, SIZE-3 step 1 (sqrtf -Os hang, UB fixes), SHOT, SCN, AZT11/12, PUSHHINT.
- Open rows: RS3 (row_RS3.md: RECT-3 then ST-3 move, worktree rs3). LS-3 merged e6e757aff. ST-3 and RECT-3, then END-1 and S10-1..4 (plan_s10_path.md). DEL + T1F merged c3e7afb19 (TRI-4, FB-4 C1-C3, GM-3, per-triangle aux TEXEL1 fold).
- Parked bugs: layout-dependent pictures (mac_00 sky row, nok11 shot 0016: row_UB1.md, not byte gates), SBKW sbk_56 water residual (row_SBKW.md), FX1 super block (row_FX1.md: needs a menu-walk scenario). pm5all halo dropped (switch deleted in FB-4 C1).
- Size: 3dsx 13,321,796 B (after DEL); netloader accepted 13.17 MB before. Push to O3DS XL failed at 23.84%. Options: Homebrew Launcher in title mode (hold R starting a game), SIZE-3 step 2 (-Os, 122 KB, blocked by layout bug), delete rows, SIZE-1 (parked).
- Unmerged branches with useful work: ub1 (stale unit-1 cache patch, sprnat OOB patch).
- Process rules updated: orchestrator_rules.md "Process fixes".
- Next: merge RS3; then END-1 and S10-1..4 (plan_s10_path.md), oracle plan first. TDC closed: no dropped triangles.
- Disk: folders build3ds/wt/fb3, fb4, tdc, ls3 are held by a process (Azahar or a shell): run wt_gc.sh --apply again when Azahar is closed (it now removes stale folders; it keeps fresh branches, use --only for a fast-forwarded one).
