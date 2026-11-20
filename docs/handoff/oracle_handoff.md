# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main c23333bb73 plus the pages of oracle NATIVE15, 2026-11-20.
## Goal
- User: better frame time on the O3DS, work spread about evenly over core 0 and core 1, core 0 decides nothing, good ARM11 code, good PICA use.
- The plan: docs/handoff/plan_perf.md (findings with file:line, ranked rows, measurement plan). Old plans: plan_native.md, design_native_draw.md.
- Numbers (FACT capture build 9f6be238aa, build3ds/hw_o3ds_log.txt): ft 46 to 51 ms per step, about 21 steps per second (fps counts interpolated presents).
  Core 1 lgms 44 to 50 ms, core 0 load 18 to 25 ms, core 0 waits 15 to 17 ms per step. c0gate 0 / 0 / 1 (state nd_c0).
## Tools on main
- `python 3ds/tools/c0gate.py <elf>`: calls / interp / state. Run on the ELF of the same tree (a clean worktree or git archive).
- `bench.py base <bench> --commit $(git merge-base HEAD main)`, `bench.py run a,b --row <id>`, `bench.py cmp <bench> --row <id> --commit ...`.
- Gate benches (drec, byte-equal shots): spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec, hudfb_drec (debug words in 3ds/tools/azt.d/*.json).
- Prof lines (pm_phase.c:575-703): fps, core2 lgms, c1n, c1t, cls, ndc, pmu. PERF0 adds c1s (core 1 logic and list ms) and c1a (alt words, GPU draw words).
## Open work (order)
1. Merged up to c4648e7561 (NC0A, C1MIC). Console capture of build 7569 is in plan_perf.md section 6. No push until the user wakes.
2. Pages on main, parallel: ws_C1HOT, ws_PROF2.
3. Next pass: Sonnet design pages NC0B (VTX record carries geom, tss), NC0R (target role from the snap), then NC0C, MCOPY1, MCOPY2 (plan_perf.md 6a).
## Facts that cost time
- Sources are CRLF. Git Bash `sed -i` turns them into LF: pages say `sed -b -i` or the Edit tool.
- Host -Werror units link some files: nd_rec.c with nd_ds.c (geounit, hunit, nat5a, nmdl0, ns3unit, stunit), nd_ds.c alone (nst1, nset1),
  nd_direct.c with 3ds/tools/nd_direct_test.c (it defines nd_ap). nd_snap.c, nd_walk.c, nd_geom.c, nd_comb.c, nd_tex_set.c are not host-linked.
- Host seeds sed nd_rec.c text: `|| c == 1u;` (nat5a.sh), `rec_nat(nd_epm_run, 0u,` and `p[0] = NDC_HDR(NDC_NAPI, 1, 1);` (stunit.sh). Keep those strings.
- The Bash default timeout is 120 s. nd_host_test.sh needs `timeout: 600000`. bench.py run does not build: `PM_LANE=<id> sh 3ds/build.sh` first.
- The stream lives as long as its display context (2 chains, nd_rec.c:32-35, rec_ctx_check :167). A core 0 pointer into the stream must not outlive the walk.
- The c1t timers run in sampled steps only. The alt timer is per unit: subtract altu x cal ticks (cal 303 ticks per pair) for real ms.
- mx in the core2 wait line is mixer wall time (pm_au2.c:168-176), not CPU.
- Model cache hit path (nd_recs.c:251-276) runs no rec_apply, no nd_msnap_use: core 0 state on a hit comes from hit_apply only.
- altw copy words count words written since CNTFIX (b17ad66e08); before it a range over a chunk jump counted the heap distance.
- nd_img_role (nd_tgt.h:38-42) reads nd_c0.omode_l and omode_h: 14 callers, some with no live snap.
- PERF0 altw (drec end) and PERF1 snapi lines are cumulative from boot: compare a tip run with its base log, never with a steady-state rate.
- Snap memo: a new snap needs a new command between draws (sn_cmds, nd_snap.c:53). nd_snap_alt and nd_snap_now force the next snap new.
- Core 1 runs while core 0 walks. Det runs are serial: a race does not show in a bench.
- A SendMessage to a coder that handed back resumes it. Never start a second coder on the same worktree.
- Oracles cannot start coders (permission denied): the orchestrator starts them.
## What to do first
1. Read plan_perf.md and the ws_PERF pages on main. `git log --oneline -15`. Answer coder questions (SendMessage, 10 lines).
2. When PERF0 hands back: read its altw lines. Write ALTM (model alt ranges as a cached core 1 block) or drop it if c0 m is near c1 m.
