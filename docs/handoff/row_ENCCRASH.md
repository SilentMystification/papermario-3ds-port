# Row ENCCRASH: crash after a standing encwarp encounter with interp off

## Facts (BTRANS agent, 2026-11-06)
Scenarios btrans_stand_enc / btrans_stand_noint on branch btrans (7b08de8d4): with interp off, standing encwarp runs end rc 1 "crash symbols" after the window end, on the base (caplog only) and the fixed build. Not in the capture code (unverified).

## Route (oracle, code first, CLAUDE.md rule 5)
1. Take the symbolized PC/LR and the last 30 log lines from the AZT crash report ("crash symbols").
2. Read the faulting function, and the frame, game mode and battle-load phase from the pm_wd phase line.
3. Compare with docs/handoff/archive/diag_encwarp.md and diag_battle_start_crash.md (earlier encwarp and battle-start faults: warp-into-battle state and load order).
4. Name the field or pointer at the faulting line from the code before any further run. One confirming run at most. Then fix; gate: 3 standing encwarp runs interp off with no crash, the battle starts.

## Rules
Own worktree from nds1 (branch enccrash) or the btrans worktree if the scenarios are needed (cherry-pick 90157a60e). No attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Delete this file in the fix commit.

## State (2026-11-07, paused)
- Crash (1 of 1 unguarded run, btrans_stand_noint, interp off, build 2580 = nds1 + btrans): run_azahar "crash symbols", top PC 0x00193B70 = find_script_labels, src/evt/script_list.c:155 (addr2line, lane ELF).
- Unmapped report (last_unmapped.txt, 355k lines): Read32 at PC 0x00193B54/60/70 of 0x0006015C, 0x00020C94, 0x0006D714, 0x00014E54, 0x0004F304.
- Game log: encwarp pre-battle start frame 172, window end 217, then no game lines (loop from ~frame 232).
- Faulting code: find_script_labels walks bytecode from script->ptrNextLine (type, numArgs, label; curLine += numArgs), returns on type 1, no bound on the walk. Callers: start_script, start_script_in_group, start_child_script, func_802C39F8, func_802C3C10, restart_script.
- Candidate field (not confirmed): numArgs word at curLine+1 of the started script's bytecode; script bytecode overwritten or freed while the start pointer stays valid.
- Diagnostic build (walk guard, uncommitted): 4 standing interp-off runs, all pass, guard never fired. Crash not reproduced on that build.
- Battle: pre-battle start logged; maphud battle not in the window.
- Starts used: 6 of 8.
- Next: reproduce on the guarded build, read the ENCCRASH line (script id, steps, end ptr, caller), addr2line the caller, fix there.

## State 2 (resumed 2026-11-07, paused)
- Rebased on nds1 3775f1303. Guarded build: src/evt/script_list.c walk guard (uncommitted diagnostic; logs ENCCRASH id/steps/end/caller). Build 2650.
- Code (callers of find_script_labels in the encwarp path): start_script (script_list.c:288, enemy aiScript and hitBytecode/defeatBytecode starts), start_child_script (430, evt_handle_exec_wait evt.c:978), func_802C39F8 (498, evt.c:1219 thread), restart_script (571, evt.c:987 evt_handle_jump: ptrFirstLine = a script variable value).
- Dangling candidates: enemy->aiScript / hitScript / auxScript are raw Evt* (encounter.c:2554, 2666, 2767); script structs are heap_malloc'd (script_list.c:270/339/409/478) and heap_free'd on kill (script_list.c:706). encounter_api.c:419-435 (AI replace) reads enemy->aiScript->groupFlags after kill_script_by_ID in a path that may run while the old script is gone.
- Named candidate, not confirmed: a Jump/restart that takes ptrFirstLine from a variable holding a stale pointer, or an enemy aiScript pointer read after its script was freed.
- Reproduction: guarded run 1 (standing, interp off) passed, no ENCCRASH line. Crash seen 1 of 1 on the unguarded build (run 1 in State 1). Starts used: 7 of 8.
- Next: repeat the guarded standing interp-off run until the crash returns; the ENCCRASH line gives the script id and the caller address (addr2line on docs/handoff's lane ELF). Fix at the caller that sets the pointer.

## Oracle plan (2026-11-07, code trace)
Trace: encwarp (pm_warp.c:1096 encwarp_tick) enters ENCOUNTER_STATE_PRE_BATTLE directly with hitType 0 and FIRST_STRIKE_PLAYER. It skips the START_BATTLE block of update_encounters_neutral (encounter.c:957-1010). Missing side effects of a real touch: suspend_all_script(curEnemy->aiScriptID) and auxScriptID (~962-967), the hit-script start, disable_player_input / partner_disable_input, PS_FLAG_ENTERING_BATTLE. The touched enemy's AI script keeps running into battle against the battle NPC list. The fault addresses (0x00014E54 .. 0x0006D714) are neither heap nor 3dsx pointers: a script started or restarted from a value that is not a bytecode pointer.
Candidates: 1 (top) the unsuspended AI script starts a child or restarts (evt.c:986-987 restart_script, start_child_script) from a variable the battle overwrote. 2 a real-game Jump/Exec from a non-pointer variable. 3 stale aiScript after kill_script_by_ID: ruled out for US (encounter_api.c groupFlags read is JP/PAL only).
Measurement (1 run, guarded build plus validator): in start_script, start_child_script, func_802C39F8, restart_script check the bytecode pointer lies in the 3dsx .data/.rodata range or the heap. On failure log once: __builtin_return_address(0), parent script id, its ptrCurLine and opcode, owner1.enemy and npcID, parent == curEnemy aiScript and its suspend state, game mode, encounter state.
Fix: cause 1: a port-side function in pm_warp.c that mirrors encounter.c:960-1010 for ENCOUNTER_TRIGGER_NONE (hitType = ENCOUNTER_TRIGGER_NONE, FIRST_STRIKE_NONE, suspend AI and aux, hit scripts, disable player and partner input, PS_FLAG_ENTERING_BATTLE). Do not change the game file. The validator stays as a cold counted check with a log line. Cause 2: fix the variable producer the log names.
Gate: 5 standing encwarp runs interp off plus 2 walking runs: no crash, no validator line, maphud battle line appears.
