# Row CORE2C: game logic on the second core, walker stays on core 0 (oracle plan 2026-11-08)

## User facts
User: use the second core more; "cache pressure might also be reduced if we spread workload to 2 cores as we'll have 2x the L1 cache". Each ARM11 core has its own 16 KB L1I and L1D.

## Oracle findings (plan, not measured)
- Walker on O3DS core 1: likely slower. The app gets at most 80% of core 1 (APT_SetAppCpuTimeLimit 80, pm_log.c:1203-1216); audio, AU2 render, log writer/sink, flash, watchdog use part of it (core1_busy 30 before PERF1); so the walker would get about 50-70% of a core. GSP/DSP run there too. svcFlushProcessDataCache must run on the core that wrote the data. Verdict: walker stays on core 0 on the O3DS.
- Cache: today the game code evicts the walker's lines once per frame (about 0.1-0.5 ms refill); most of the 48k-80k I-misses are the walk evicting itself (24.7 KB hot span). A large cache gain needs the walk itself split (producers on one core, emit/submit on the other): that is DIRECT's seam, measured there.
- Gain of logic on core 1: frame = max(walk, (logic + list build) / s1), s1 = core 1 share. Saves the logic + list build (1.3-5.3 ms) plus 0.1-0.5 ms misses: about 10-15% in mac_04/mac_05.
- Order: probe now; the thread split after PERF2 merges and before DIRECT. DIRECT's design adds the producer / emit-submit core split as an option chosen by console I/D-miss data.

## Rows (one Opus agent, worktree build3ds/wt/core2c, branch core2c, PM_LANE=core2c)
1. C1P PROBE (now): switch `c1probe` (prof mode, 10 s): (a) share: a thread on core 1 at the planned logic priority runs a fixed integer loop in 1 ms chunks for 2 s while the game draws mac_01; iterations vs the same loop timed on core 0 at boot give s1; with audio active; (b) wake latency: the main thread signals a LightEvent each frame, the core-1 thread records tick(wake) - tick(signal), max and p95; (c) throttle period: gaps > 0.5 ms between chunk end and next chunk start give the period and length of the pause. One prof line. Bench c1probe_o3ds (console_prof). Gates: build, nd_host. Then STOP and report: the user pushes once. Pass: s1 >= 0.5 and p95 wake < 1 ms. P2 code read in the same report: every pm_gpu/pm_tex call the game step makes outside the walk (nd_wq.c routes them), and the main-loop pieces that stay on the main thread (aptMainLoop, pad scan, pacing, pm_wq drain). The probe is deleted after the decision.
2. (after PERF2 merges; the orchestrator tells you) CORE2-3c-1: frame queue in nd_wq.c, depth 1, payload struct independent of lists vs records (background list, main list, cfb, flags), waiting drains (pm_walk_drain nd_walk.c:414 and pm_wq_call wait for the queue to empty: map, battle, save, HOME, sleep, exit, pause capture, depth read), det = serial. Still one thread. Gates: build; nd_host; spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015 byte-equal; tour10_det windows; c2haz_mac01 no new address.
3. CORE2-3c-2: the logic thread (step_game_loop + list build of N+1) on core 1 (O3DS) or core 2 (N3DS), one code path, core from APT_CheckNew3DS; the main thread (core 0) is the walker + system (aptMainLoop, pad scan, pacing, frame begin, posted nd_wq messages, walk and frame end of N); pad state to logic through the pad buffer, read once per step; audio priority above logic; the push waits only if N is not walked yet. A new prof field: walk-wait (time the walker waits for the next frame). The serial game-thread path is deleted (det is the serial queue mode). Gates: the 4 byte gates under det; c2haz_mac01 and c2haz_battle without det (no new address at steady state); menu_open_det; tour10_det.
4. Console proof (the user pushes): spot_mac01_prof and a mac_05 spot, prof=30. Pass: clean wk falls by about the logic + list-build time; aujoin 0; AU2 underrun count 0; walk-wait about 0 when the walk is the bound. N3DS later with the same benches.

## Rules
CLAUDE.md rules 5, 7, 9, 11, docs/handoff/hw_guidelines.md, docs/handoff/plan_core2.md. PM_LANE=core2c PM_ROW=core2c on every build.sh and bench call. No git stash. No toggles, no stubs. Commit after each passed step. Azahar timing is direction only; the console decides.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=core2c PM_LANE=core2c sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=core2c PM_LANE=core2c python 3ds/tools/bench.py run <bench> --row core2c --lane core2c --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "core2c ready <hash>".
