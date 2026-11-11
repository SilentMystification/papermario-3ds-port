# Row BTLDBG: debug-menu battle during an NPC talk (logic thread null read) and the logic-thread stack walk

(Model: Haiku. Base: main 5d26c84a2. Disjoint from MAPROMFS. Reads only this file and CLAUDE.md.)

## User facts
O3DS XL, build 4195 (dd4720d43): the user started a battle from the debug menu and at the same moment talked to a Toad. Crash:
`data abort thread=logic pc=00198644 lr=0101ce30 far=00000000 r0=r1=r2=0 r4=r5=0101cdd0 r8=0058f7d8 r9=00197098 r10=010a8c50`,
after `seamfix map isk_bt04_shape` (the battle stage loaded); `sp 327ebed8 is outside the known thread stacks, no stack walk`.

## Cause (oracle code read, build3ds/pm_3ds.map of build 4195)
- pc 0x198644 = find_script_labels+0x3c (src/evt/script_list.c:150-153): `curLine = script->ptrNextLine` is NULL, so a script was started
  with a NULL source (start_script). r8 = &UniqueScriptCounter and r10 = gScriptIdList (start_script's registers), r9 = 0x197098 inside
  evt_execute_next_command: an EVT exec op of a battle script ran a NULL script pointer. The exact op is not known: no stack walk.
- Path A (the trigger): the debug battle start (3ds/src/pm_warp.c battle_tick :1332 and boss_tick :1238, run after the step's game logic,
  pm_warp_tick :1963) checks only the game mode, then forces `gEncounterState = ENCOUNTER_STATE_NONE` and load_battle. The game itself
  never starts a battle during a talk: the talk sets ENCOUNTER_STATE_CONVERSATION (src/encounter.c:1199), disables input, starts the
  NPC script with gCurrentEncounter set for a talk (curEnemy, encountered = CONVERSATION). The battle then starts on that state.
- Path B (the tool): the crash handler looks for the stack only in recs[0] and recs[1] (pm_main.c:149 `for (int i = 0; i < 2; i++)`);
  the logic thread is recs[3] (pm_logic.c:56), the render thread recs[2]: their crashes never get a stack walk, so the caller of a
  crash on the logic thread is lost.

## Start
`sh 3ds/tools/wt_new.sh BTLDBG`. PM_ROW=BTLDBG PM_LANE=BTLDBG on every build and bench call.

## Touch only
3ds/src/pm_warp.c: battle_tick, boss_tick. 3ds/src/pm_main.c: crash_handler (one loop bound).

## Exact edits
1. pm_warp.c battle_tick, after `if (get_game_mode() != GAME_MODE_WORLD) { bt_pending = -1; return; }` add:
   `if (gEncounterState != ENCOUNTER_STATE_NEUTRAL || (gPlayerStatus.flags & PS_FLAG_INPUT_DISABLED)) { bt_frames = 0; return; } /* BTLDBG: wait while a talk, an encounter or a script holds the world */`
2. pm_warp.c boss_tick, after the line that sets `boss_frames = 0; return; }` for the mode and map test, add the same test with
   `boss_frames = 0;`.
3. pm_main.c crash_handler: `for (int i = 0; i < 2; i++)` -> `for (int i = 0; i < (int)(sizeof recs / sizeof recs[0]); i++)` with the
   comment `/* BTLDBG: every registered stack (game, audio, render, logic) */`.

## Gate (CLAUDE.md 2; Azahar budget 2 of 8)
- `PM_ROW=BTLDBG PM_LANE=BTLDBG sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `bench.py run nd_host --row BTLDBG --why "pm_main.c crash handler"`: PASS.
- `bench.py base spot_mac01_det`; `bench.py run spot_mac01_det --row BTLDBG --why "no draw change"`; `bench.py cmp spot_mac01_det --row BTLDBG`: 6/6 byte-equal.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the completing commit. Hand back: the hash, the gate results, the memcheck line. The user's console check:
the same menu battle while talking to a Toad starts only after the talk ends.
