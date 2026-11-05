# Diagnosis: encwarp battle start fault (row ENCD)

Worktree: C:\Users\Andrew\Documents\GitHub\papermario-3ds-port-main, branch native. HEAD at start: c1fada795.
Azahar starts used: 3 of 3. Builds used: 2 (lane native). Scenario: battle_start_fb.
Cap: 45 s (the scenario cap). azt.ps1 has no cap option, so the 25 s cap was not applied.
Local edits: reverted. Only this report is committed.
Justification lines: 3ds/logs/perf/status_ENCD.txt (not committed).

## 1. Result

- Caller of the bad script: NOT NAMED. No guard logged a bad entry pointer in any run.
- Breaking commit: NOT NAMED. The core battle start files did not change since FB1 (section 4).
- Fix: NONE PROVEN. Section 6 gives the next step.
- Star transition frames: battle_start_fb reaches them (section 7).

## 2. Runs

Abbreviations: PC = program counter. LR = link register (return address). DIAG = local log line, not committed.

| Run | Build | Guard | Outcome | DIAG line |
|---|---|---|---|---|
| 1 | start_script guard (script_list.c:238): source below 0x00100000 | start_script | rc 2 (crash), faults at PC 0x00173ACC in find_script_labels | none |
| 2 | find_script_labels entry guard (start_script guard kept) | find_script_labels entry | rc 0, stopped at the until-line "encwarp: window end" (frame 219) | none |
| 3 | same binary as run 2 | find_script_labels entry | rc 2 (crash) after the window end line, 22428 faults at PC 0x00173B78 in find_script_labels | none |

- Run 2 stopped at the until-line, so it shows no crash either way.
- Run 3 is the only run that crashed with the entry guard in place. It logged no DIAG line.
- Run 1 crashed. Its start_script guard logged no DIAG line.

## 3. What the runs prove (PROVEN from the logs)

- In run 1, no start_script call had a source below 0x00100000 before the crash.
- In run 3, no find_script_labels call had a line pointer below 0x00100000 before the crash.
  This covers every caller: start_script, start_script_in_group, start_child_script, func_802C39F8 and restart_script.
- So the first fault address 0x00000B6C in docs/handoff/diag_battle_start_crash.md is not the source pointer.
  It is a position inside the walk. The claim "source = 0x00000B6C" in that doc is wrong.
- find_script_labels (src/evt/script_list.c:136-168) walks a label table. It steps by numArgs words. It returns on type 1 and calls PANIC after 16 type-3 labels.
  A walk into low memory means a word inside a valid-entry script is not a label header.
  Two causes fit (INFERRED, not separated): the script bytes are overwritten, or the pointer points at data that is not a script.

## 4. Regression premise (PROVEN for source, INFERRED for the cause)

- No commit since 7a2cef467 (FB1) changes src/evt/script_list.c, src/evt/evt.c, src/encounter.c, src/battle/battle.c or src/battle/btl_states_actions.c.
- No commit since 7a2cef467 changes include/ battle, evt, npc or functions headers. include/gbi_custom.h has 6 added lines.
- No commit since 7a2cef467 changes 3ds/ld, 3ds/map_syms.lds, 3ds/CMakeLists.txt, 3ds/*.cmake or 3ds/stub_include.
- Changed since 7a2cef467 (3DS port, memory and draw): pm_mem.c, pm_tex.c, pm_gpu.c, pm_gbi.c, pm_warp.c, pm_wdog.c, pm_main.c, pm_route.c, pm_log.c.
  Of these, VRAM2 (commit d9c7076d8) changed the VRAM layout at boot and the texture cache size (pm_mem.c, pm_tex.c). It is the first memory-layout change in that range (INFERRED that it is the cause).
- The FB1 pass (7a2cef467) is not proof of no crash. The FB1 run stopped at the until-line, like run 2. A crash after the window end line would not show.

## 5. Correction to earlier docs (PROVEN from code)

- encwarp does NOT skip START_BATTLE. encwarp sets the pre-battle fields and then the normal substate machine runs.
  ENCOUNTER_SUBSTATE_PRE_BATTLE_LOAD (src/encounter.c:1391-1434) calls set_battle_formation(nullptr), set_battle_stage(encounter->stage) and load_battle(encounter->battle) after the fade. The ENC1 change that called these by hand was redundant.
- encounter->battle and encounter->stage come from the map NPC group at map load (src/encounter.c:2511-2512).
  The kmr_03 group is BTL_KMR_1_FORMATION_02 (src/world/area_kmr/kmr_03/npc.c:197). It is a static ID and valid.
- The battle area ROM and VRAM ranges are stubbed to ((u8*)1) (3ds/stub_include/battle_ld_addrs.h:112-114). So battle formation data is in the ELF, and a bad DMA does not explain the fault (INFERRED that the stub is used by the build).

## 6. Next step (needs a 4th Azahar start, user decision)

- Move the guard into the walk loop of find_script_labels. Before each read, if curLine is below 0x00100000, log LR (__builtin_return_address(0)) and the entry pointer, then return.
  This names the direct caller and stops the walk (no 20000 faults). It bounds the crash. It does not fix the bad script.
- A bisect is a second option. The first candidate is VRAM2 (d9c7076d8) and its parent. A bisect run must not use the until-line, or it stops before the crash.

## 7. Star transition frames (scenario command)

Command (works now, no edit needed):
`PM_ROW=ENCD powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 battle_start_fb -Row ENCD -Build native -Try`

- Run 2 log: "pre-battle start at frame 174" (line #584), "shot 5 frame 174" (line #585), "window end at frame 219".
- 15 shot files (0000 to 0014) are in build3ds/azt/shots/battle_start_fb. The picture was not read by this agent (the files are .ppm).
- The crash happens after the window end line, so the shots are written before it.

## 8. Worktree state

- src/evt/script_list.c: local DIAG edits reverted with git checkout. The Docker build copy matched HEAD plus the two DIAG guards, so no other edit was in it at build time.
- Other uncommitted files, not touched by this agent: 3ds/src/pm_tex.c, 3ds/tools/azt_scenarios.json, docs/handoff/test_catalog.md.
- 3ds/src/map_restore_table.c: restored from a copy after each build. Not committed.
