# TRBZ: Buzzar pick-up shows the wind bar (battle, 2026-11-03)

Branch native. Read-only trace. No Azahar start (0 of 2). No source edit.

## 1. Cause: PROVEN (port code path), INFERRED (N64 call target)

Pick-up script, src/battle/area/iwa/actor/buzzar.c:943-945:

    Call(LoadActionCommand, ACTION_COMMAND_BREAK_FREE)   // 943: DMA break_free overlay
    Call(action_command_whirlwind_init)                  // 944: no args, wrong name
    Call(SetupMashMeter, 1, 30, 0, 0, 0, 0)              // 945

- Line 944 blame: 9574baebaf (2024-01-21, upstream). The port adds no commits to these files (git log on break_free.c, whirlwind.c, action_cmd.c, buzzar.c shows upstream only).
- The port resolves `Call(name)` by the C symbol name. So line 944 runs whirlwind.c `N(init)` (whirlwind.c:74-127).
- whirlwind.c:89 sets `actionCommandID = ACTION_COMMAND_WHIRLWIND`. whirlwind.c:102-124 creates AButton, BlueMeter, Whirlwind7 digit, Bubble. These are the wind bar elements.
- break_free.c:19-71 (the RunAway and OK elements) never runs. `action_command_break_free_init` has 0 callers.
- break_free.c:73-106 (`break_free_start`, called at buzzar.c:955) does not set `actionCommandID`. So `action_command_update` (src/battle/action_cmd.c, switch on actionCommandID, case WHIRLWIND) keeps running the wind update.
- Result on screen: the wind bar with the wind digits and bubble. Matches the report.

## 2. N64 intent (INFERRED)

- ver/us/symbol_addrs.txt: every `action_command_*_init` sits at VRAM 0x802A9000. break_free_init rom 0x4233F0 (line 9095). whirlwind_init rom 0x423C70 (line 9100).
- So the ROM call target 0x802A9000 holds break_free code after `LoadActionCommand BREAK_FREE`. The decomp name `whirlwind_init` is an alias for the same slot. The N64 pick-up runs break_free init.
- break_free.h declares break_free_init with "args: none". This matches line 944 (no args).
- Not checked: the script bytecode itself (no ROM image in the worktree).

## 3. Args side effect (PROVEN code, value NOT measured)

- whirlwind.c:88 reads `evt_get_variable(script, *args++)` with args = script->ptrReadPos. evt.c:866-910 (`evt_handle_call`) does not advance ptrReadPos before the callable runs. So the variation reads the next opcode byte (SetupMashMeter).
- Effect: actionProgress 0 or 3 (whirlwind.c:94-98). Does not change which bar shows.

## 4. Scan for the same bug elsewhere

- Pairs of LoadActionCommand with the next init (4-line window, 23 pairs in src): one mismatch, buzzar.c:943-944.
- Run-away: player_events.c:1245-1246 loads FLEE and calls flee_init. Match.
- Bombette: bombette.c:1022 calls bomb_init after the BOMB load. Match.
- So the Bombette and run-away bars do not share this cause. The window scan is not exhaustive for cases where the load and init are not within 4 lines.
- hud_element.c recycling: not involved. No element is reused here. The wind elements are created and never freed, and the break-free elements are never created.

## 5. Fix (proposal, not applied)

- buzzar.c:944: `Call(action_command_whirlwind_init)` to `Call(action_command_break_free_init)`.
- break_free.h is included at buzzar.c:3. The fix is one line.

## 6. Test

- No catalog scenario covers Buzzar (grep test_catalog.md and azt_scenarios.json: no match).
- diag_battle_start_crash.md: `encwarp` crashes. The `battle` switch loads a formation directly (battle_start verified). A Buzzar formation needs a new scenario and a pick-up hook.
- Pass check after the fix: the pick-up bar shows the RunAway and OK icons. The wind digits and bubble do not show.
- Not run. Current worktree has uncommitted edits in src/battle/action_cmd.c and 3ds/src/pm_*.c from other work. Not touched.
