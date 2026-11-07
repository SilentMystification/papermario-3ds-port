# Handoff S10 epic (orchestrator, 2026-11-07)

STATE
- nds1 head: see `git log -1 nds1` (this file is committed on it). pm_gbi.c 938 lines.
- Done this session (all merged, byte gates cold det): DET (det switch), sprfix + SPR4 (native sprites incl. shaded), FB-1 (grab native, chunked ring), FB-2 (noise unit 2, template LRU), FB-3 (pm5 tex1/noise, sprite pm5 uniforms), MDL-5a/5f/5e (model records fully native, walker model fallback deleted), MOVE 1-10 + MOVE-4/5, LS-4 splash record, ENCCRASH, SIZE-4 hint, SIZE-3 step 1 (sqrtf -Os hang, UB fixes), SHOT, SCN, AZT11/12, PUSHHINT.
- Open rows (row files in docs/handoff): DEL (branch fb4: TRI-4, FB-4, GM-3; see row_FB.md "DEL row"), LS-3 (rec_* to nd_recs.c, row_MOVE.md), ST-3 and RECT-3 (after DEL), then END-1 and S10-1..4 (plan_s10_path.md).
- Parked bugs: mac_00 layout-dependent sky row (row_FB.md MAC00 sections; not a valid byte gate), SBKW sbk_56 water residual (row_SBKW.md), pm5all halo (row_FB.md FB-3), FX1 super block (row_FX1.md: broken only after the upgrade menu confirm; needs a menu-walk scenario).
- Size: 3dsx 13,359,084 B; netloader accepted 13.17 MB before. Push to O3DS XL failed at 23.84%. Options: Homebrew Launcher in title mode (hold R starting a game), SIZE-3 step 2 (-Os, 122 KB, blocked by layout bug), delete rows, SIZE-1 (parked).
- Unmerged branches with useful work: ub1 (stale unit-1 cache patch, sprnat OOB patch), fb4 (DEL in progress).
- Process rules updated: orchestrator_rules.md "Process fixes".
- Next command: read row_FB.md "DEL row" and the DEL agent's last state; merge fb4 when its gates pass; then LS-3 from row_MOVE.md.
- Disk: build3ds is 181 GB (about 70 worktrees, about 210 lanes); clean merged worktrees with user approval.
