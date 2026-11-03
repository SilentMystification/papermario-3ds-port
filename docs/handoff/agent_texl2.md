blocked on texgarble fix (L2b not started: branch texgarble has no fix commit, only probe 9ca2ec747)

## State
- Branch texl2 (worktree ..\papermario-3ds-port-texl2, from main 7d19c5808). Plan: docs/handoff/plan_tex_l2.md (uc2c d6f57e4e8).
- Row L2a done in code (counters and debug switch only). Commit: see `git log texl2 -1`.
- Row L2b to L2e NOT started. L2b is blocked on the TEXGARBLE fix: branch texgarble has only probe commit 9ca2ec747 (no fix). Check `git log texgarble` for a fix commit and docs/handoff/agent_texgarble.md before L2b.

## L2a picture gate: NOT passed (measured 2026-11-03)
- HEAD vs HEAD (two volcano runs, shot 0000_18_13): 0 bytes differ. Run noise is ruled out.
- Every L2a build differs from HEAD volcano in the same 20 bytes (208594-211475, x166-168, y289-293):
  (a) L2a as committed (source hash before the conversion): 20 bytes.
  (b) HEAD plus only the early source hash: 20 bytes.
  (c) L2a with the slot hash after the conversion (HEAD order): 20 bytes.
  (d) L2a without the ghost hash and check: 20 bytes.
- Conclusion: the 20 bytes follow the code build, not the ghost logic and not the hash order. Cause not found. Possible: layout or timing sensitivity in the emulator. Not proven.
- Pre-conversion source hash differs from the post-conversion hash for some sources (variant b). The L2 key must use one hash order (post-conversion, as HEAD).
- yard: top shot 0000_0_11 byte-equal to HEAD (L2a as committed). Bottom shots differ from frame 210 (bottom screen, not gated).

## L2a result (Azahar, build L2a, lane TEXL2)
- volcano_l2 (texcap=256): texghost f300 evict-hit 2/32KB, diff-hit 0/0KB, upload ops 322, upload us 54784. Counters above 0: PASS.
- Picture, open: volcano_l2 and volcano (default cap, build L2a) differ from HEAD volcano in 20 bytes at x166-168, y289-293 (same bytes in all three runs). Default-cap L2a volcano run equals volcano_l2, so the cap is not the cause. The L2a code does not change pixels by design. Cause not proven: possibly run noise. Test that: one HEAD rerun of volcano (1 start).
- Picture, open: yard L2a top shot 0000_0_11 byte-equal to HEAD. Bottom shots bot_0200 equal; bot_0202 to bot_0259 differ, mostly in the strip y10-29 (x2-239). Not explained. The L2a code does not touch the bottom screen.
- No person has read the shots yet. A picture check by eye is still needed.
- Budget: 8 of 8 starts used for row TEXL2 (3ds/logs/test_budget.csv). Orchestrator reset needed for more starts.

## Notes for the next agent
- The yard scenario log check fails at `warp: scene f300` (the run stops by stall at about 21 s). The shots are valid. Use the shots.
- texcap=1024 gives no eviction on volcano. Use texcap=256.
- Build: `PM_LANE=TEXL2 sh 3ds/build.sh` in the worktree. Needs assets/us and ver/us/build copied from the main folder (ignored by git).
- Baseline references: scratchpad base_yard (HEAD yard shots), base_volc (HEAD volcano shots, log.txt). Scratchpad path: C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\bf7242ad-7188-4cf3-8ec3-af3ba8c281b2\scratchpad\
- Azahar runs collided with other agents' runs at 02:12 to 02:16 (Azahar exits when another instance runs). Check azahar.exe with Get-CimInstance before each start.

## Next steps
1. Orchestrator: reset the TEXL2 budget (run_azahar.ps1 -ResetBudget TEXL2) and approve one HEAD volcano rerun to test run noise.
2. Person: read build3ds\azt\shots\volcano_l2\ and yard shots by eye.
3. L2b only after a TEXGARBLE fix commit. Then put by stage move (plan 2.3 step 2), per plan section 4.

## Commands
- Scenario: `PM_ROW=TEXL2 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 volcano_l2 -Row TEXL2 -Build TEXL2 -Try`
- Justification line: 3ds/logs/perf/status_TEXL2.txt (before each start).

## L2a bisection (2026-11-03, agent texl2b)
- Base check: parent 7d19c5808 built in lane TEXL2BASE equals the old HEAD baseline (0 bytes). The base is not the cause.
- Layout control: parent plus an unused 4 KB static array (lane TEXL2PAD): 0 bytes against HEAD. A bss layout shift alone is not the cause.
- Part A (load_miss source hash moved to the top, gh_check): removing it from L2a changes nothing. Not run alone; see split below.
- Split: L2a with only the counters (ghost table, texcap switch, upload timer, report line; lane TEXL2BK): 20 bytes against HEAD, 0 bytes against L2a. The counters part produces the 20 bytes.
- Open: which line in the counters part. Suspects: the upload timer loop body, the texcap pm_debug_value call in pm_tex_init. Hypothesis, not tested: a timing-dependent texture race (TEXGARBLE stage free) moved by any instruction change.
- Budget: 7 of 8 starts used for TEXL2 after the reset (ledger). One start left. Gate L2a: NOT passed.
- Note: a failed azt marker check (TEXL2PAD, TEXL2BK) leaves build3ds/azt/shots unchanged. Read the lane folder build3ds/lane-<lane>/shots/ instead.
