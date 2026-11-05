# Diagnosis: battle start crash at PC 0x00173B04

Worktree: C:\Users\Andrew\Documents\GitHub\papermario-3ds-port-main, branch native, HEAD 55f3148b5.
Build: build3ds/lane-native (pm_3ds.map, pm_3ds.elf, last_unmapped.txt, last_log.txt).
Azahar starts used: 0 of 3 (no run was needed for the decode, and no code was changed).

## 1. Decode of PC 0x00173B04 (PROVEN)

- pm_3ds.map: `find_script_labels` at 0x00173AD8, size 0x8C (ends 0x00173B64).
- Source: src/evt/script_list.c lines 136 to 168.
- Loop body: lines 151 to 166. The loads are at lines 152 (`type = *curLine++`),
  153 (`numArgs = *curLine++`), 154 (`label = *curLine`), 155 (`curLine += numArgs`).
- Caller: `start_script` (src/evt/script_list.c:238), which calls `find_script_labels(newScript)` at line 288.
  `start_script` stores the source pointer in `ptrNextLine` at line 260 before the call.
- last_unmapped.txt: 29660 faults, all at PC 0x00173B04, all Read32, all distinct, all 4-byte aligned.
  Address range 0x00000000 to 0x0001CF6C. First fault 0x00000B6C.

## 2. Why the reads are unmapped (INFERRED)

- The 3DS maps no memory below 0x00100000 for this game image. Every fault is below 0x0001CF6C, so every fault is a bogus address.
- The first fault (0x00000B6C) is the first load from `curLine`, which equals the `source` pointer. So `source` is 0x00000B6C (INFERRED from fault order).
- Later faults are not sequential. The walk reads `numArgs` from unmapped memory and steps by garbage values. This part is not explained.
- The loop has no bound except `type == 1` or 16 labels. Bogus data therefore walks until the emulator stops it. That gives the 16k to 30k faults.

## 3. Decode caveat

- No addr2line, objdump, capstone or devkitPro toolchain exists on this machine (/opt/devkitpro is absent).
- The ARM words were read from the ELF PT_LOAD segment with Python. The word at 0x00173B04 is `mov r0,#0`, not a load.
  Azahar reports a block-level PC, so the faulting load is in the loop (word 0x00173B14, `ldr r1,[r2]`). INFERRED.

## 4. Which path starts the bad script (INFERRED, the caller is not named)

- `start_script` is called with a source from the battle state (src/battle/btl_states_actions.c:321, 333, 432):
  `stage->preBattle`, `battle->onBattleStart`. Static Stage and Battle data in the binary has valid pointers (checked for kmr_04: src/battle/common/stage/area_kmr/kmr_04.inc.c).
  A small value in those fields is not expected from static data. The source of the bad pointer is not proven.
- Scenario difference (PROVEN from scenario definitions and catalog):
  - battle_start (verified, passes): debug switch `battle` loads a formation with `load_battle` directly. No `encwarp`.
  - battle_start_enc, battle_start_fb, battle_start_fbvram (crash): all use `encwarp=120`.
    battle_start_fbvram debug string is in 3ds/tools/azt_scenarios.json. battle_start_fb is defined as "same as battle_start_enc" in docs/handoff/test_catalog.md.
- So the common factor is `encwarp`, not the `fbbind` or `vramfill` switches. The fb switches only add log lines. The control run without vramfill crashes too.
- encwarp path (3ds/src/pm_warp.c:802 to 845): it sets the pre-battle fields by hand (firstStrikeType, hitType, scriptedBattle, fadeOutAmount, substateDelay, gEncounterState, gEncounterSubState).
  It skips the START_BATTLE code in src/encounter.c:1431 to 1434, which calls `set_battle_formation(nullptr)`, `set_battle_stage(encounter->stage)` and `load_battle(encounter->battle)`.
  So the battle ID and stage ID come from stale state, not from the world enemy. (PROVEN from code. Effect on the pointer: INFERRED.)

## 5. Regression or property (PROVEN for the window, cause INFERRED)

- Catalog: battle_start_fb FB1 run (commit 7a2cef467, parent 4bd4ad4f8^) reached the transition frame 174 with shots 0000 to 0014 and no crash.
  So the encwarp path worked at that point. The crash is a regression between 7a2cef467 and HEAD, or the path was never valid and the FB1 run did not reach `start_script` the same way.
- Commits in that range that touch battle, encounter, evt or warp code: 22ad77382 (UI2 step 1, pm_warp.c), 6b3b0eec5 (UI1 hook), adeeb38e3 (UI2 step 2, star depth in initialize_battle, 16C8E0.c; only argument changes, no pointer write), a4d3a2aed and 508b9b00d (HANG1a, gx queue; no battle code), 3f27dcfee (UI1 cc07_dbg; sync_combine_work), WARPFIX and WARPFIX2 (b3d631735, 368800bff; world warp state, which changes the encounter state that encwarp reads).
- The commit that introduced the crash is not named. The HANG1 commit 4d68fae60 does not touch battle, evt or encounter files (checked with git show --stat).
- Not run: a worktree build of 7a2cef467 or 4d68fae60^. No devkitPro toolchain is present here, so the build was not started.

## 6. Proposed fix

1. Make encwarp run the real START_BATTLE setup (set_battle_formation(nullptr), set_battle_stage(encounter->stage), load_battle(encounter->battle)) instead of writing the pre-battle fields by hand. This is a debug-tool fix in 3ds/src/pm_warp.c only.
2. Add one guard in `start_script` (src/evt/script_list.c:238): log and return when `source` is below 0x00100000, with the call site. This names the caller on the next run and stops the unbounded walk in find_script_labels.
3. Bisect 7a2cef467 to HEAD only if fix 1 does not stop the crash. Each step needs one build and one Azahar start (PM_ROW=BSC, justification line first).

## Open items

- Name the caller of the bad source pointer. Needs one run with fix 2 (one Azahar start, PM_ROW=BSC).
- Confirm the decode with a toolchain (addr2line on pm_3ds.elf at 0x00173B04) when devkitPro is available.
