# ws_STARTLR (Haiku): in the start (pause) menu 3DS L gives N64 L and 3DS R gives N64 Z
Objective (user, 2026-11-15, confirmed: "L -> L, R -> Z", only inside the start menu). FACT (code read, main 89725d888c):
the shoulders map in shoulder_map (pm_input.c:206-210). Default pm_map (pm_options.c:53): L 0x2000 (Z), R 0x2000 (Z), ZL 0x0020 (N64 L), ZR 0x0010 (N64 R).
The O3DS Z combo (X held, or Y with o3dsz=1) gives L 0x0020 and R 0x0010 (pm_input.c:207-208). Start menu = GAME_MODE_PAUSE (game_modes.h:28).
The pause menu reads Z (previous) and R (next) (pause_tabs.c:406, :417). N64 L has no pause function (no BUTTON_L in src/pause).
Held keys do not stick: b is built again each scan (pm_input.c:250-260). A key held across the mode change gives one new edge (prev_sh, :258-261).
Defaults picked: ZL and ZR keep their map in the start menu. The O3DS Z combo keeps L -> L, R -> R (on the O3DS XL, X+R is the next tab).
Alternative (1 line): ZL and ZR give nothing in the start menu, and the combo also gives L -> L, R -> Z.
Counters: none. Model: Haiku. Worktree build3ds/wt/STARTLR, branch STARTLR. PM_ROW=STARTLR PM_LANE=STARTLR on every bench call.
Files: 3ds/include/pm_shoulder.h (new), 3ds/src/pm_input.c, 3ds/src/pm_dpad.c, 3ds/tools/pm_shoulder_test.c (new), 3ds/tests/host.d/startlr.sh (new).
## Edit 1: block A as 3ds/include/pm_shoulder.h. Edit 2: block B as 3ds/tools/pm_shoulder_test.c. Edit 3: block C as 3ds/tests/host.d/startlr.sh (LF).
## Edit 4 (pm_dpad.c:18). After the closing `}` of pm_free_overworld add:
`int pm_start_menu(void) { return get_game_mode() == GAME_MODE_PAUSE; } /* STARTLR: the start (pause) menu */`
## Edit 5 (pm_input.c:206-210). Before:
`static unsigned short shoulder_map(u32 k) {` / `    if (!pm_new3ds && (k & (pm_opt.o3ds_z ? KEY_Y : KEY_X)))` / `        return ...0x0020...0x0010...;` / `    return ...pm_map[PM_IN_L]...;` / `}`
After:
```c
int pm_start_menu(void); /* pm_dpad.c */
static unsigned short shoulder_map(u32 k) {
    return pm_shoulder_bits((k & KEY_L) != 0, (k & KEY_R) != 0, (k & KEY_ZL) != 0, (k & KEY_ZR) != 0, &pm_map[PM_IN_L],
                            !pm_new3ds && (k & (pm_opt.o3ds_z ? KEY_Y : KEY_X)), pm_start_menu()); /* STARTLR */
}
```
Add `#include "pm_shoulder.h"` after `#include "pm_port.h"` (pm_input.c:2). Change the comment at :199-201: add one sentence
`STARTLR: in the start menu L gives N64 L and R gives N64 Z (pm_shoulder.h).`
## Loop (1 build, at most 2 Azahar starts; base runs not charged)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/startlr_host.txt 2>&1`. Pass: `ND HOST TESTS PASS` and `startlr ok`. Commit `STARTLR 1`.
2. One build. memcheck margin above 0.
3. Gate: `python 3ds/tools/bench.py base spot_mac01_drec`, `python 3ds/tools/bench.py run spot_mac01_drec --row STARTLR --why "STARTLR gate"`,
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row STARTLR --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal (one differing bit fails).
FACT: no bench presses a shoulder in the pause menu. The user checks the start menu on the console.
## Fix scope, stop and hand-back
A host test failure: fix only the new files. A gate that differs: write the cmp output to build3ds/out/, send the path to the oracle.
Stop: host PASS and the gate byte-equal. Final commit `STARTLR 2: gate` with the values. It deletes ws_STARTLR.md and ws_STARTLR_code.md.
Hand-back: build3ds/out/handback_STARTLR.txt (line 1 commit hash, gate value, memcheck margin, counters: none).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
