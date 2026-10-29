# Diag: menu event lines and frame cost (build 1301-ui, Azahar)

## Event lines (phase gate)
- pm_ph_ui: 3ds/src/pm_phase.c:303. Prints `ui: <what> v<n> f<frame>` only when `pm_ph_on`.
- partner menu: src/pause/pause_partners.c:629 (open), :830 (close).
- pause menu: src/state_pause.c:165 (open), :219 (close).
- file select enter: src/state_file_select.c:95. File select pick: :306 (v = filemenu_get_exit_mode(), seen v2).

## Finding 1: partner open is the pause open
- pause_main.c:645 runs fpInit of all 7 panels at pause open. The partner line fires with the pause line (f93, run C).
- Partner close never fired. Pause close fired (run A f208). Cleanup is not called at pause close.
- The partner page (tab col 3) was not reached by tab taps (runs D, E). A tab-entry hook is needed.

## Finding 2: frame cost at the events (per frame; ops = C_CMD, tri = C_TRIOUT)
Pause, event f93 (run C):
| state | ops | tri | closes | tex closes | tex miss | GBI ms | GPU ms |
|---|---|---|---|---|---|---|---|
| before f73-89 | 5474 | 371 | 72 | 311 | 0 | 11.8 | 0.5 |
| peak f95 | 3757 | 76 | 245 | 241 | 74 | 20.2 | 14.3 |
| after f100-113 | 3329 | 0 | 217 | 213 | 0-1 | 16.1 | 1.7 |
Jump: GBI +4.3 ms (+36 %), closes x3.0, ops+tri 5845 to 3329.

File select enter, event f3632 (run G):
| state | ops | tri | closes | tex closes | tex miss | GBI ms | GPU ms |
|---|---|---|---|---|---|---|---|
| before f3612-3629 (title) | 1103-1132 | 53 | 61-104 | 97-98 | 0 | 4.1-4.6 | 0.4-0.8 |
| after f3646-3652 | 2550 | 53 | 188 | 141 | 0-1 | 13.3 | 1.5 |
Jump: ops+tri 1156 to 2603 (+125 %), GBI x3.2.
File select pick, f3728: no jump (ops 1407 to 580, GBI 7.3 to 2.2 ms by f3745).
Tables: 3ds/logs/perf/ui_menu/tables.txt. Splits: 3ds/logs/perf/ui_menu/steady_split.txt.

## Finding 3: what draws per frame
- Pause: 5 of 7 windows use DRAW_FLAG_ROTSCALE (pause_main.c:45). draw_box: src/draw_box.c:430.
- Each rotating window renders into an off-screen VRAM slot (pm_gbi.c:226, pm_gpu.c:1436). OFF_N = 4 (pm_gpu.c:231).
- Steady draws per frame: pause 217 (biggest; 205 rect closes, 213 tex closes, run C f105). File select 188 (run G f3650). Title 103.
- V05 in run G (line 110856, f3674 to f3728): "more than 4 off-screen images in one frame, slots are used again". pm_gpu.c:1472 reuses a used slot. This matches bug B.

Pictures: 3ds/logs/perf/ui_menu/pause_open_shot0003.png, file_enter_shot0121.png, file_pick_prompt_shot0122.png.

Caveats: run F took 131 s wall (cap 90 s). Starts used 7 of 8. Run B failed (START too early).
