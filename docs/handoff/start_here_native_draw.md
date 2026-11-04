# Start here: native draw system (code session handoff)

Read this file first. It points to the plan and lists the next steps. You are the code session. You work until a row set passes or a gate fails validly. A written status message ends your turn, so write one only at that point.

## You are not the orchestrator
CLAUDE.md opens with orchestrator rules ("you are the orchestrator only, agents do all work"). They do not apply to you. You write the game source yourself. You run the tests, reset Azahar budgets, commit, and decide when to continue. Do not spawn subagents for the code work. Use CLAUDE.md rules 5 (tests), 7 (code) and 9 (platform) only. Rule 6 (console) applies: push only when the user asks.

## Goal
Replace the N64 display list walker (3ds/src/pm_gbi.c, pm_gpu.c, pm_dlc.inc.c) with native 3DS draw records. Producers (src/model.c and others) write native data. The walker and interpreter are deleted in slice S10. Target: 60 fps on Old 3DS (30 fps floor), 16.7 ms per frame.

## Where things are
- Repo: C:\Users\Andrew\Documents\GitHub\papermario-3ds-port. This is the only folder. No worktrees. Branch: nds1 (stay on it).
- Design: docs/handoff/plan_native_system.md (sections: ARM11, PICA, TDD, constraints carried, earlier-plan table, game source edits, rule 12 on symbol anchors, section 14 open questions).
- Rows: docs/handoff/plan_native_slices.md. Anchors are symbols plus a grep string. A line shift never stops a row. Grep the symbol and continue.
- Budget: docs/handoff/frame_budget.md (per-stage targets and the O3DS XL baseline).
- Latest state: docs/handoff/agent_nds_s1d.md (S1-3 blockers and probes), agent_nds_s1c.md, agent_nds_s1b.md, agent_nds_s0.md. Picture diff note: diag_s1_1_picture.md.
- Rules: CLAUDE.md rules 5 (tests), 7 (code) and 9 (platform). Read only those.

## State
- S0 done: host tests, ndref reference streams (3ds/tests/ndref, 8 files), prof stage line, frame_budget.md.
- S1-1 done (3e6b19bc2): replay range and PMOD bake deleted. S1-2 done (389d7c16d): nd_core frame driver and LEGACY bridge.
- S1-3 partial (9fcfee63c): nd_tmpl render state (cull, depth map, alpha test, blend, depth test) passes nd_tmpl_test on 8 files.
- Not started: S1-4 (nd_mtx, the only red host test), S1-5, S1-6, then S2 to S11.

## Next steps (S1-3 finish, then S1-4)
1. Extend the ndref hook (pm_gpu.c ndref_draw) to also record: z flags (zmark, zcmp, zupd, nocol), stencil mode (stc_set 5/6 and decal sites 1 to 4), w0, w1, two, t1only, and the register writes the old path issues for TEV, early depth and COLOR_OPERATION per draw (register id and value). That register list is the host test ORACLE: you do not need to disassemble libcitro3d.a. Bump FORMAT.md and nd_ref_dump.py, re-capture the 8 files (one Azahar run per scene, justification line first).
2. Write the failing host test: nd_tmpl output words equal the oracle words for all 8 files. Then add TEV, stencil, early depth and COLOR_OPERATION words to nd_tmpl.c. The template key gains w0, w1, two, t1only. Words are built once at template creation by calling pm_comb_build and the sh_* writes, never per draw. A game pair that pm_comb_build refuses is built from the old path result, not a fallback.
3. Hints, UNVERIFIED (check against the oracle): TEXENV stage i at register 0xC0+8*i for stages 0 to 3 and 0xF0+8*(i-4) for stages 4 and 5. Offsets: +0 source, +1 operand, +2 combiner, +3 color, +4 scale. Stencil op words verified earlier: 0x105 and 0x106 (C3D_StencilOp = sfail | zfail<<4 | zpass<<8). Headers: $DEVKITPRO/libctru/include/c3d/*.h and 3ds/gpu/registers.h inside docker image pm3ds-build:1.
4. S1-4 nd_mtx: host test first (red now), then code. Then S1-5, S1-6 per the plan rows.
5. After S1: S2 to S11 in order from plan_native_slices.md. Each row: host test first and shown failing, code, ARM build, row gate, commit.

## Gates (every row)
Picture equal first (region_diff, VIEW the picture), triangles equal, batches not above the old path, host tests, hot loop under 8 KB, prof line shows the stage. A picture difference needs a stated cause. Azahar is for pictures and crashes. Speed comes from the console `prof` line only.

## Commands
- Host tests: sh 3ds/tools/nd_host_test.sh, sh 3ds/tools/nat_host_test.sh
- Build: /c/Python314/python 3ds/tools/gen_map_restore.py; PM_ROW=<row> sh 3ds/build.sh; then restore 3ds/src/map_restore_table.c. User file: never commit it, never discard it (md5 437b90efe5f32b82394ce2ece094210e).
- Scenario: powershell -File 3ds/tools/azt.ps1 <scenario> -Row <row>. Write one justification line to 3ds/logs/perf/status_<row>.txt before each Azahar start.
- Azahar budget: 8 starts per row. It is never a gate. If it ends with gates unrun: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row> and continue.
- Console push (only when the user asks): PM_ROW=PUSH sh 3ds/push_3ds.sh --no-build --ip 192.168.1.131 -t 150 prof (O3DS XL, netloader on; the log lands in build3ds/hw_live_log_131.txt). Do not challenge that IP.

## Rules that matter
- No feature toggles, no stubs, no git push, no attribution in commits. Finished work replaces the old path in the same row. The legacy walker stays only as the bounded bridge the plan names. S10 deletes it.
- Commit after each passed row (git add the changed files only).
- ARM11 and PICA rules: docs/handoff/hw_guidelines.md and plan sections ARM11 and PICA. No division, no double, 32-byte records, sequential access.
- Work does not stop for a token count. A background command is not a stop: wait for it inside the turn (poll its output file) and continue.
