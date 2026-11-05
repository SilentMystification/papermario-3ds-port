# Handoff: finish the native draw epic (for the next orchestrator)

Written 2026-11-05 at the end of the code session on branch nds1. Another handoff exists: docs/handoff/start_here_native_draw.md and docs/handoff/agent_nds_s1e.md (the running log of the code session). Read this file first. It says what is true now, what is open, and how to work. Do not overwrite the other two files.

Plan documents: docs/handoff/plan_native_slices.md (the slice table, rows S0 to S11), docs/handoff/plan_native_system.md (the design, the seam rules in section 13), docs/handoff/design_native_draw.md (the fidelity rule). Project rules are in CLAUDE.md and the memory index. The rules that matter most here: no feature toggles, no stubs, no git push, no attribution lines in commits, never commit or discard 3ds/src/map_restore_table.c, commit after each passed row, the user judges speed on the console.

## 1. Goal

Replace the N64 display list walker (3ds/src/pm_gbi.c, pm_gpu.c, pm_dlc.inc.c) with native 3DS draw records, templates and emit. The gain is frame time on the O3DS XL. Target: 30 fps. Today Toad Town (mac_01) runs about 25 to 27 fps in prof mode.

## 2. State at the end of the session

Branch nds1. The tree is clean except the user files 3ds/src/map_restore_table.c and 3ds/logs/perf/pushed_builds.txt (do not commit the first).

What is done and committed:
- S0 test tools. S1 native core: nd_tmpl.c, nd_mtx.c, nd_tex.c, nd_core.c, nd_state.c, nd_dlc.c, nd_emit.c, with host tests (sh 3ds/tools/nd_host_test.sh, all must pass).
- S2 bridge: nd_gbi.h leaf macros write the old words and decode them into nd_st. No native draw reads nd_st yet.
- S3 M1 and M2: nd_dlc compiles map shape lists into static vertex and index pools. M3: nd_raw_native in pm_gpu.c draws raw pm4 batches (unlit palette program, main target, at most one texture unit, no stencil pass) from nd_emit words through GPUCMD_AddRawCommands. About 90 percent of Toad Town batches take it.
- M4 step 1: bind lists and render mode lists run fused (tb_ops, op functions without dispatch). Switch texdiff (scenarios mac01_texdiff, mac01_walkdiff) runs each bind both ways from the same start state and compares the state. 36000 binds and 66500 render mode lists, 0 diffs.
- Console stability and texture fixes (this session, all in the log of commits bc5e16a8d, 21e4a0b10, 21632dce4, 78beb2a36, 18d49a20c): see section 5.
- Tools: prof mode with scene tag, prof_record.py and docs/handoff/perf_history.csv, savetool, Profiling row and Warp submenu in the debug menu, watchdog line with breadcrumbs, switches ndplan and ndunit, ndref files of moving frames in 3ds/tests/ndwalk.

NOT verified on the console: the last build (commit 18d49a20c, fix for flat and wrong textures). It was pushed to the O3DS XL with prof mode (build number 2095 or later) and the user was asked to check the post office in mac_01 while moving. The result was not reported when this file was written. First task of the next session: ask the user for that result. If the flat textures remain, use the ndunit switch idea (section 6) on the next suspect.

Measured numbers (console prof, mac_01, docs/handoff/perf_history.csv): native close off 23.4 fps 42.6 ms (walking), native close on 24.9 fps 40.0 ms, with the fused bind 25.7 fps 38.7 ms (Mario standing at pos -264 20 -326). Walking runs are not comparable to each other (Mario takes another path each time). Use the padscript switch for a fixed route (section 7).

## 3. What is left (by slice, in the order I recommend)

Slice status from plan_native_slices.md: S0 and S1 done. S2 bridge only. S3 partly done. S4 to S11 not started. Rough share done: under one fifth.

1. Finish S3 (map models). It covers most draws of the frame.
   - Verify the console result of commit 18d49a20c (section 2).
   - M4 step 2: producer record. appendGfx_model (src/model.c, under TARGET_3DS) writes one record for a plain opaque model: texture handle, render mode index, lights group, matrix, shape list. A consumer sets the walker state from it. Proof method agreed with the user: a debug switch that runs the old words and the native setter from the same start state and compares the state (the texdiff harness in pm_gbi.c is the model: ts_save, ts_load, TS_FIELDS), plus the picture. Do not try a memo of the bind result again (section 5, item 8).
   - Cover the batches that still use the old close: lit models (pm5, nd_lights is ready and has a known-answer test only, the ndref scenes hold no lit draw), two texture units, off-screen targets, zmark and decal passes, the CPU vertex path.
   - S3-5: sort and merge with a per draw matrix (a raw batch holds one game matrix at palette entry 0 today, so sorting by texture does not reduce closes).
2. S4 sprites, S5 HUD and text, S6 entities, S7 world and battle, S8 effects, S9 screens. Each slice takes one content type out of the walker. See the rows in plan_native_slices.md. Each slice needs its own ndref oracle files (the S0 tools), its own host test, and a console prof run.
3. S10 delete the walker (only after S4 to S9). S11 tuning.

Order rule from the plan: no culling of any kind (memory feedback_no-culling). Only opaque depth tested records may be reordered. The seam rules in plan_native_system.md section 13 apply to every row: one fp32 product post times game (nd_mtx_pcomp), s16 positions raw, no vertex shader change, post/viewport/tilt unchanged. Gate G9 is nd_seam_test plus mac01_seam.

## 4. How to work (process rules that bit us)

- One code agent at a time, in the main folder on branch nds1, no worktrees. Agents are Haiku per CLAUDE.md. A hard problem (the hang hunt) needed a stronger model: the Opus review agent found the real bugs. Use a stronger model for review and diagnosis when the user allows it.
- Test through the scenario catalog: powershell -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Try. Set PM_ROW. A scenario needs a cap and an until-line that the build prints. A new scenario goes into 3ds/tools/azt_scenarios.json and docs/handoff/test_catalog.md in the same commit. The Azahar start budget per row is a repeat guard only: reset it with powershell -File 3ds/run_azahar.ps1 -ResetBudget <row> and go on.
- Azahar is not deterministic in frame numbering. Two runs of one walk differ in timing, so frame by frame picture diffs of a walk are useless. Standing shots at fixed frames are stable (mac01_seam compared with region_diff: 3ds/tools/region_diff.py). Azahar also cannot show GPU texture cache or register hazards. It shows the command stream you wrote: use stream checks (ndunit, ndplan, nd_emit_test over ndref files).
- Console flow: the user must open netloader before every push (it closes after each push). Push with: PM_ROW=PUSH sh 3ds/push_3ds.sh --no-build --ip 192.168.1.131 -t 30 prof. O3DS XL is 192.168.1.131 (the default of go.ps1 pull o is .168: pass -ip 192.168.1.131). Pull logs and Luma dumps with powershell -NoProfile -File 3ds/go.ps1 pull o -ip 192.168.1.131: the user must start ftpd (port 5000) first. If the push stops with ARCHIVE FAILED: the build number did not change but the file did: commit and rebuild, then push. Ask with the AskUserQuestion tool (netloader and results), never in chat text.
- Performance comparisons use prof mode only (memory feedback_prof-mode-for-perf-tests), the same switches in both runs, a fixed padscript route, then python 3ds/tools/prof_record.py add <log> --label "..." and the trend command. Phase, pmu and sp0c switches only split a cost.
- Build: sh 3ds/build.sh (docker, about 35 s incremental). Host tests: sh 3ds/tools/nd_host_test.sh (docker image pm3ds-host:1; gcc is not on the Windows path).
- Decode a Luma crash dump: python 3ds/tools/decode_dump.py <dump> and addr2line from the container: MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd)/3ds/logs/builds:/elfdir" devkitpro/devkitarm sh -c '/opt/devkitpro/devkitARM/bin/arm-none-eabi-addr2line -f -C -e /elfdir/pm_3ds_<build>.elf <addresses>'. The ELF must be the archived one of the build that wrote the dump.
- The watchdog (3ds/src/pm_wdog.c) ends the process after 10 s without game loop progress. Its line prints the step_game_loop call number, the walker op, the last flush, the native draw counters and the last script step. It writes stall notes to crash.txt on the SD card every second from the third second.

Pitfalls of the tools in this environment:
- The Bash tool collapses a double backslash. Python edit scripts with C strings that hold backslash-n came out with real newlines three times. Write the script to a file with the Write tool and build the sequence with chr(92).
- Source files use CRLF. A multi line replace must convert the newlines. Scripts used a helper that detects the line ending.
- The JSON scenario regexes need a doubled backslash in the file.
- Files in the scenarios and catalog: LF is replaced by CRLF on commit (a warning, harmless).

## 5. Findings of the session that the next session must not rediscover

1. Hard hang that needs a power cycle = the GPU, not the game thread. Watchdog line then shows walker op df and last flush pm_gpu_end (the last frame ended). Four real defects were found by review and fixed: a 512 word stack buffer in nd_raw_native (worst case 620 words, now ND_EMIT_MAX 768, proven by nd_emit_test); C3DiF_DrawUsed not set after native words; stale uniform shadows after native draws (now raw1_reset in nd_leave_native); stencil effect copy not turned off. The hang was not seen again after these fixes, but the proof is only absence over short runs.
2. C3D_TexBind(1, NULL) faults in citro3d (it reads the type of the null texture for units above 0). Only unit 0 can be unbound. A fix of mine crashed on this (data abort in drop, decoded with decode_dump.py). Check every citro3d call with a null argument in the libcitro3d disassembly: docker run ... arm-none-eabi-objdump -d libcitro3d.a (a copy of the output may sit in build3ds/c3d.dis).
3. Flat and wrong textures while moving (only with the native close): after a slot drop citro3d holds a null unit 0. C3Di_UpdateContext rebuilds the texture unit enable word from the bound units and writes unit 0 off whenever a texture bind is dirty. The native shadow still said on. Fix: invalidate the native unit shadow when the context flags (offset +32, mask 0x3800000) show a dirty texture bind. The ndunit switch proves it (negative test reproduced it).
4. Native unit words equal what citro3d writes: 9000 draws, all fields, 0 mismatches (ndunit scan with masks and the list split tails). The first ndunit run showed false mismatches because the command buffer tail is lost at a split: scan before every C3D_FrameSplit and C3D_FrameEnd.
5. The old path leaves unit 0 stale in about 2 of 100 draws (pm_tex_bind skips a rebind its tb_last shadow believes done). The native path draws the current texture there. This difference was NOT the cause of the flicker.
6. Plan layout of the template key equals the plan of the old path (ndplan switch, 12000 draws).
7. -Os for the walker files shrank hot code by 26 to 39 percent and was not faster on the console. The instruction cache is not the whole story. Reverted.
8. A bind memo (record the bind calls and the end state, replay on the same start state) passed 36000 binds in the diff harness and gave no console gain; the console build hung. Reverted (commits 798655867 and its revert). Fused ops without a memo are kept.
9. Texture flicker only in the left eye: a symptom of the missing cache clear and unit shadow staleness, not a stereo bug.
10. Azahar ran a 30 map tour for over 330000 frames without a crash. The console faults were GPU and hardware state faults that Azahar does not model.

## 6. Test and debug switches that exist (all debug only, all off by default)

texdiff (bind and render mode list diff, scenarios mac01_texdiff and mac01_walkdiff), ndplan (scenario mac01_ndplan), ndunit (scenario mac01_ndunit: decodes the command buffer into a register shadow and logs a textured native draw that ends with unit 0 off), ndref=<frame> (reference draw stream, scenarios ndwalk_170 and others, files in 3ds/tests/ndref and 3ds/tests/ndwalk, format in 3ds/tests/ndref/FORMAT.md), prof, phase, sp0c, opsplit, mr0 (walker split lines), padscript=go_<x>_<z>_<frames>,wait<n> (a fixed walk), pos=x:y:z, warp=<map>:<entry>, story=<name>, savefile=, wdog=<seconds>.
A stream check is the right tool for a GPU state bug: write a debug function that decodes what you wrote into a register shadow and asserts the invariant, then prove it with a negative test (remove the fix, see the check fail).

## 7. Procedure for the first sessions

1. Ask the user (AskUserQuestion) for the console result of build commit 18d49a20c: flat textures gone? hangs? Then record the answer in docs/handoff/start_here_native_draw.md.
2. Get a fair baseline: push with the same fixed route. Use padscript=go_91_87_900,wait60,go_-300_-300_700 after warp=mac_01:0 story=ch0_toad_town (note: Mario stopped at x -221 z -223 on that route, choose a route that finishes). Record with prof_record.py.
3. Start the code agent on the producer record row (section 3, item 1). Row detail goes into 3ds/SLICES.md. The agent prompt is 10 lines or fewer. Suggested text: read this file and docs/handoff/start_here_native_draw.md; row M4-2 producer record for plain opaque models; proof is a diff switch from the same start state plus the picture; no toggles, no stubs, no push, no attribution; commit after the row passes; write handoff docs/handoff/agent_<epic>.md at the end.
4. After every row that changes draw code: host tests, mac01_seam picture, a long Azahar tour (go.ps1 push a -NoBuild -Interactive -GameDebug "tour=@tour_k30.txt loglevel=debug" and let it run), then one console push and a play test with the intro. The console is where GPU faults show.

## 8. Risks

- GPU hazards the emulator cannot show: texture cache clears, unit enable words, shadows of registers that citro3d or the native words changed. Every new native word path needs a stream check.
- Memory safety of fixed buffers sized from game data: size from the worst case and prove with a host test.
- The user dislikes stops: do not end a turn with a report when a next step exists. Report at a real fork or a hardware need (netloader, ftpd) and keep working on code meanwhile.
- The user wants numbers recorded (perf_history.csv) and findings written to docs, not kept in chat.
