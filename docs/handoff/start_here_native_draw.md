# Start here: native draw system (code session handoff)

Read this file first. It points to the plan and lists the next steps. You are the code session. You work until a row set passes or a gate fails validly. A written status message ends your turn, so write one only at that point.

Startup cost note: a code agent reads its row file (docs/handoff/row_<id>.md, from row_TEMPLATE.md) first and opens only what that file names. This file is the long-form handoff for a user-started session.

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

## State (2026-11-04, end of the long session)
- S0, S1 (S1-1 to S1-6), S2-1 and the S2 force include, S3-1 (nd_dlc) done. Details: docs/handoff/agent_nds_s1e.md.
- S3 M1 and M2: shape lists compile with nd_dlc into the static vertex and index pools, dlc uses them (old dlc_compile and dlc_static_build deleted).
- S3 M3: nd_emit.c plus nd_raw_native in pm_gpu.c: raw pm4 batches (unlit, main target, at most one texture unit, no stencil pass) draw from native command words. Host test nd_emit_test equals the real command stream. Azahar pictures equal. Console prof A/B (Toad Town): fps 23.4 to 24.9, frame 42.6 to 40.0 ms.
- Console history: docs/handoff/perf_history.csv, python 3ds/tools/prof_record.py add|trend. Use prof mode for every comparison (memory feedback_prof-mode-for-perf-tests).
- ndref is format v4 (kinds 4, 5, 6). Host tests: sh 3ds/tools/nd_host_test.sh.

- M4 step 1 (commits 507add26f, 8e50af7f3): texture bind lists run fused (tb_ops, op functions without dispatch, one combine sync for SETCOMBINE+TEXTURE). Proof: switch texdiff (scenario mac01_texdiff) runs each bind both ways from the same start state and compares the tile, combine and mode state: 6000 binds, 0 diffs. mac_01 picture pixel equal. Console prof, Toad Town standing at pos -264 20 -326: 26.2 fps, 37.9 ms (3 windows, Mario did not walk; earlier runs walked, so the delta of +1.3 fps is a direction). Next in M4: lights, render mode list and matrix words of a model group; precomputed per list bind data.

- 2026-11-05 console stability and texture findings (commits bc5e16a8d, 21e4a0b10, 21632dce4, 18d49a20c): (1) nd_raw_native wrote its words into a 512 word stack buffer, the worst case draw is 620: now a static ND_EMIT_MAX 768 buffer, nd_emit_test proves the bound. (2) The review of the native state handling (agent report) gave six fixes: C3DiF_DrawUsed set after native words, uniform shadows (raw1_reset) cleared on leaving native, stencil effect copy off, texture cache clear when the texture, slot generation or staging changes (NdUnit.force), buffer colour in the emit fast path, command buffer room check. (3) C3D_TexBind(1, NULL) faults in citro3d (reads the type of the null texture for units above 0): drop() unbinds unit 0 only. (4) Flat and wrong textures while moving: when a slot is dropped citro3d holds a null unit 0, C3Di_UpdateContext then rebuilds the texture unit enable word as unit 0 off, the native shadow still said on. The native shadow of the unit blocks and the enable word is invalidated whenever a texture bind is dirty. Proof: switch ndunit (scenario mac01_ndunit) decodes the command buffer into a register shadow (also before every list split) and logs a textured native draw that ends with unit 0 off: 0 in 12000 draws with the fix, many without it. (5) Switch ndplan (mac01_ndplan) checks the plan layout of the template key against the plan of the old path: 0 mismatches in 12000 draws. (6) 3ds/tests/ndwalk: four ndref files of moving frames (old path stream); nd_emit_test passes on them, 4 of 225 draws show a stale unit 0 in the old path (citro3d skips a rebind the tb_last shadow believes done). (7) Watchdog line: step_game_loop call, walker op, last flush, last script step; stall notes every second in crash.txt. A hard console hang needs a power cycle: it was the GPU, not the game thread. Open: the console run of build with fix (4) was not run yet (user not ready); the earlier crash decode used 3ds/tools/decode_dump.py with the ELF of the same build (docker addr2line).

## What is left (S3 on)
1. M4 producer records: appendGfx_model writes one record for a plain opaque model (texture handle, render mode index, lights group, matrix, shape list) and the consumer sets the walker state directly. This removes the 5 to 8 texture words, the combine sync and the light words per model: the walker spends 211 us per plain model on the console (phase on), the close only 37 us. Proof method decided with the user: a debug switch that runs the old words and the native setter from the same start state and logs any difference in the texture, tile, combine and other mode state (state-diff), plus the picture.
2. Cover the batches that still use the old close: lit (pm5 light block, nd_lights is ready), two texture units, off-screen targets, zmark and decal passes.
3. S3-5: sort and merge needs a per draw matrix: the nd_emit record already carries pal entries per draw.
4. S4 to S10 as in plan_native_slices.md.

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
