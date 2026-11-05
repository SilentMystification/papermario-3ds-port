# Orchestrator handoff (2026-10-29, late evening)

For the next orchestrator. Read CLAUDE.md first (rules 1 to 9), then this file, then docs/handoff/work_queue.md. You orchestrate. Agents (Haiku code and investigation, Opus plans) do the work. Chat in ASD-STE100, minimal, status each turn: RUNNING / READY-NOT-LAUNCHED / BLOCKED-ON-USER.

## 1. State
- Branch main, HEAD 3b7bc8a20. 123 commits ahead of origin/main (the user pushes; agents never push). Only one local branch (main). All old worktrees and branches were deleted on 2026-10-29 (patches and branch tips are in C:\Users\Andrew\Documents\GitHub\_port_cleanup_patches).
- Nothing is running. No agent is live. No Azahar process.
- Uncommitted in the tree: 3ds/src/map_restore_table.c (the user's file, never commit, never discard; .bak next to it), 3ds/logs/perf/pushed_builds.txt (ignored by git on purpose), docs/handoff/test_catalog.md (stale hunks from concurrent agents; the user has not decided to restore it from HEAD; do not commit it blindly), docs/handoff/agent_model.md (untracked, safe to commit).
- Goal: O3DS reaches 30 fps (33.4 ms). No culling, no lower effect rates. src/ edits allowed with TARGET_3DS guards. Console is the O3DS at 192.168.1.170. The user judges speed there. Azahar shows direction only.

## 2. Builds
- Push candidate: build 1526, git 542f2d72d, archive 3ds/logs/builds/pm_3ds_1526_542f2d72d.3dsx, sha256 2ed9666be4437388..., equal to build3ds/pm_3ds.3dsx (verified). It holds: lazy FrameBegin (LB1), equal-light skip (R5B), equal-state skip (R2B), the entity/sprite/HUD texture ops (EN2, NP1B, HT1), the second kick split (CROSS2), svc cache flush (DS1), warp/spike log trim (WARPLOG2), the pond depth fix (PRIMZFIX, prim_z - 1), UNTILCHK, the model count in the fsplit line (WMODEL), the pause background fix (PAUSE4A), Position HUD (POSHUD), frame split flag fsplit, dcsum, hudint.
- Commit 6571d97d5 (ROMKEEP) is on main but is NOT in build 1526 and is not archived. It keeps 499 KiB more ROM data (effect gfx read set, entity models, kzn anim) that the packer dropped. O3DS margin after it: 1,202 KiB. Its checks are unfinished (see section 4).
- Push command (the script archives the 3dsx itself as pm_3ds_<build>_<hash>.3dsx and writes pushed_builds.txt): PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 pacestat logstat dcsum fsplit loglevel=debug
- The live log goes to build3ds/hw_live_log_170.txt (working since build 1467). The user allowed FTP pulls on request: py -3 -u 3ds/tools/ftppull.py 192.168.1.170 o <out dir>.
- Always copy the build you mean to push to build3ds/pm_3ds.3dsx and compare sha256 with the archive. Once an agent left a debug binary there.

## 3. Console results so far (O3DS)
- Toad Town (area 1 map 2 = mac_01): 60.6 ms (29f504d7b) -> 57.0 ms (full phase set) -> 48.5 ms (build 1467, cheap flags). Ops 2,156 -> about 1,645. Target 33.4 ms. 15 ms left.
- Where the time goes (plan_toad_33.md): gbi 26 to 29 ms, closes 5.3 ms (76.6 per frame at about 69 us), GPU wait at frame start 3.5 to 5 ms, logic 3.3 ms, display list build 3.8 ms. CPU is the limit; GPU is busy about 17 %.
- Logging itself cost 4 to 5 ms or more in Toad Town (20 fps unlogged vs 15 to 17 logged, user report). The cheap flag set is pacestat logstat dcsum fsplit loglevel=debug. Do not use phase or phsum for timing runs.
- Forever Forest calm is 34.5 ms; the lag is after a battle (51.7 ms; cause not found; needs the model count line wm/tk from a console run after a battle).
- Pause menu: +11 ms over gameplay on the console (48.0 vs 37.4). PAUSE4A cut closes 135 -> 96 on Azahar; open spike is 95 texture loads (81.6 ms) plus readback 8.8 ms.
- gcache flush: 31 us -> 1 us median after DS1.

## 4. Open items (section order = value)
1. Console push of build 1526, then read the live log: the fsplit line (gwait, logic, gbi, flush, wm, tk, spike count), pond at x -174 y 0 z 112 in the toad map (use the Position HUD, Debug tab row 8), pause feel, warp window line count, Toad Town frame time vs 48.5 ms. Everything depends on this data.
2. ROMKEEP (6571d97d5): the packer was dropping data the game reads (78 reads: 38 effect gfx, 20 entity models, plus world_model_anim_kzn). The fix is committed. Under the new TEST SCOPE rule it needs only: boot/memory line (margin >= 1.0 MB), one heavy map load, and sun_sbk. Then rebuild, archive via the push script, push. Do not run the catalog or full411 for it. Fallback if memory is short: keep only the 252 KB effect gfx read set.
3. Sun rays (Dry Dry Desert, effect fx_sun in 3ds/src/effect_loads.c:739): the user says the rays should be straight lines or triangles but the texture has breaks. Azahar sun_sbk shows dashed streaks and hard-edged cells (about 19 px pitch). Ruled out: blend, depth, tile 1 mirror vs clamp (it is mirror, port matches), odd-row word swap (texture loads by LOADTILE, 8 identical rows), tile 0 stride gather, the dropped ROM block alone. Likely fault, not proven (diag_sun4.md): pm_gbi.c:3170 adds the tile 1 texel offset uls>>2 = 54 in bytes; for I4 that is texel 108. Row SUN5: make it an I4 nibble offset, judge sun_sbk crops at 4x, check other I4 draws with nonzero uls (text glyphs) stay equal. The user will judge on the console. Stop rule: two unchanged shots end the series.
4. Pond / fountain water (toad map, x -174 y 0 z 112): background shows through on the console. Azahar shows a correct pool. PRIMZFIX is in build 1526 (diag_primz.md). The user checks. Ground flicker in Toad Town: no cause (diag_toadvis.md); needs the user's photo or a texstat run.
5. Post-battle slowdown: needs a console run after a battle with the fsplit line (wm/tk fields).
6. Speed route from plan_toad_33.md: LB1 done; R5/R2 done; left: MR1 retained model op (gate was 120 us per model outside dlc; census gave 107 us console estimate, 86 models, so MR1 was not built; reopen only if the console fsplit shows gbi is 55 % or more of the frame), CORE1 second-core pipeline (high risk, 2 to 4 ms O3DS), FPS20 stable 20 fps cap (user decision). Dropped with valid gates: DS2 (-Os), DS3, DS4, DS5, CROSS1, MS1 (15 removable closes), DC6 (no repeated groups), SPK1 (17 us), PAUSE4B (9 textures not 111).
7. The user's decisions still pending: the 3 s steady-frame wait in the yard/sewer/ruins/volcano scenarios; whether the status bar should show after the pause closes; the LOGCLEAN picture check; cutscene box; volcano fog room; house_sign and super_text pictures; the optional git push origin --delete dlc-compiler-c1c2 (user runs it); restore of test_catalog.md/azt_scenarios.json (git restore --staged --worktree on both; the classifier denied it once).

## 5. How the user wants work done (hard lessons from this session)
- Tests: test only what a change can affect. Name the affected scenarios in the agent prompt. No per-row catalog runs, no unrelated-scene sweeps, no old-build comparisons unless the row changes pixels there. A full test (whole catalog or full411) runs only when an epic merges and only if valuable; for a single feature ask the user first. The catalog (3ds/tools/azt.ps1, azt_scenarios.json, test_catalog.md) is a library of targeted tests and the reproducible way to create new ones. CLAUDE.md rule 5 has these words.
- No idle tests: a test ends within 5 s of its until-line, the until-line must be a line the build prints (UNTILCHK guards this), cap 40 s. Tests must put the subject in the picture: the fountain/pond was never in a shot for hours; use the Position HUD coordinates from the user.
- The Azahar budget only stops repeat shots. When an agent reports the budget used with gates left, mark INCOMPLETE and reset (run_azahar.ps1 -ResetBudget <row>) the same turn. Never drop a row because the budget ended.
- A byte-equal gate needs a HEAD-against-HEAD run first. The NP1 and LB1 drops were bad comparisons (battle swirl frames and the bottom screen are not deterministic; NPC sprites move in Toad Town). Compare top-screen frames HEAD reproduces, mask moving regions.
- Rotate an agent at 150k tokens. A fresh agent starts from its docs/handoff/agent_<epic>.md with a prompt of 10 lines or fewer.
- One code agent at a time. Investigations can run beside it but not on Azahar at the same time as another agent (check Get-Process azahar).
- Do not edit game source yourself. Do not git push. No attribution in commits. Never commit 3ds/src/map_restore_table.c. Agents must not run git checkout or git restore; undo with Edit. The classifier denies git restore, kill commands and bulk deletes outside the user's approval: do not work around a denial; ask the user.
- Do not run a loop of scenarios in the background: the last one kept launching Azahar after the agent was stopped. If you must stop agent work, also stop its loops (they were bash loops under scratchpad/romkeep/cmp).
- The user pays attention to the screen: explain what a running test is for in one or two sentences when asked. If you cannot say why a test helps the console, do not run it.
- Prefer a photo or Position HUD coordinates from the console to guessing at visual bugs on Azahar.

## 6. Key documents
- Plans: docs/handoff/plan_toad_33.md (route to 33.4 ms), plan_draw_cost.md, plan_model_ops.md, plan_toad_model.md, plan_entity_npc.md, plan_dl_architecture.md, plan_walk_cost.md, plan_source_opts.md, work_queue.md.
- Console diagnoses: diag_hwlog3.md (build 1467 cheap set), diag_hwlog4.md (pause, forest, desert), diag_hw_log1.md, diag_logcost.md, diag_cpu_gpu_crossing.md, diag_abcmp.md (before/after unproven; yard A/B procedure), diag_warpnoise.md.
- Visual bugs: diag_sunrays.md, diag_sun4.md, diag_primz.md, diag_toadfount.md, diag_toadvis.md, diag_romdrop.md.
- Agent handoffs: agent_lb1a.md, agent_logtrim.md, agent_pausefix.md, agent_model.md, agent_draw.md, agent_romkeep.md (if written), agent_nphud.md, agent_entnpc.md, agent_crossfix.md.
- Rules for code: docs/handoff/hw_guidelines.md (O3DS cache and ARM11 rules). Test catalog: docs/handoff/test_catalog.md. FINAL_WRITEUP.md holds the older console checklist and push notes.
- Memory: C:\Users\Andrew\.claude\projects\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\memory\MEMORY.md (feedback: no-attribution, no-stubs, model-roles, minimal-chat, no-feature-toggles, no-idle-tests, budget-not-a-gate, test-scope; project: perf-targets).

## 7. First moves for the next orchestrator
1. Report status. Ask the user for the console log of build 1526 (or the push result) and, for the sun, a photo or Position HUD values.
2. If the user agrees: launch SUN5 (one row, one scenario sun_sbk) and the targeted ROMKEEP check (boot/memory line, one heavy map, sun_sbk). One code agent, prompt of 10 lines or fewer, affected scenarios named.
3. Build the next push, archive via push_3ds.sh, give the user the push command only after the epic is complete and the targeted check passed.
4. Use the fsplit numbers from the console to choose between the speed rows in section 4 item 6. Do not start MR1 or CORE1 without those numbers.
