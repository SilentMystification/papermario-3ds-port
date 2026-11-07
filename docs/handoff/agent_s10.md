# Handoff S10 epic (orchestrator, 2026-11-07)

STATE
- nds1 head: see `git log -1 nds1` (this file is committed on it). pm_gbi.c after GM-3.
- Done this session (all merged, byte gates cold det): DET (det switch), sprfix + SPR4 (native sprites incl. shaded), FB-1 (grab native, chunked ring), FB-2 (noise unit 2, template LRU), FB-3 (pm5 tex1/noise, sprite pm5 uniforms), MDL-5a/5f/5e (model records fully native, walker model fallback deleted), MOVE 1-10 + MOVE-4/5, LS-4 splash record, ENCCRASH, SIZE-4 hint, SIZE-3 step 1 (sqrtf -Os hang, UB fixes), SHOT, SCN, AZT11/12, PUSHHINT.
- Open rows: GLOB (row_GLOB.md: pm_gbi.c globals to owners, then END-1; worktree glob) and S10B (row_S10B.md: S10-2 bridge delete; worktree s10b) run in parallel. Then S10-1, S10-3 (Sonnet), S10-4 (ask the user). Plan: plan_s10_path.md (rewritten 9174553bf). Merged this session: DEL+T1F c3e7afb19, TDC, LS-3 e6e757aff, RS3 fa6406ee7.
- Parked bugs: layout-dependent pictures (mac_00 sky row, nok11 shot 0016: row_UB1.md, not byte gates), SBKW sbk_56 water residual (row_SBKW.md), FX1 super block (row_FX1.md: needs a menu-walk scenario). pm5all halo dropped (switch deleted in FB-4 C1).
- Size: 3dsx 13,321,796 B (after DEL); netloader accepted 13.17 MB before. Push to O3DS XL failed at 23.84%. Options: Homebrew Launcher in title mode (hold R starting a game), SIZE-3 step 2 (-Os, 122 KB, blocked by layout bug), delete rows, SIZE-1 (parked).
- Unmerged branches with useful work: ub1 (stale unit-1 cache patch, sprnat OOB patch).
- Process rules updated: orchestrator_rules.md "Process fixes".
- Next: merge GLOB and S10B as each passes (GLOB owns pm_gbi.c, nd_cen.c, nd_walk.c; S10B owns nd_core.c, pm_nusys.c, nd.h, tools); then S10-1 row from plan_s10_path.md.
- Disk: folders build3ds/wt/fb3, fb4, tdc, ls3 are held by a process (Azahar or a shell): run wt_gc.sh --apply again when Azahar is closed (it now removes stale folders; it keeps fresh branches, use --only for a fast-forwarded one).
