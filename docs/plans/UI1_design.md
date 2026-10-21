# UI1 design: Options tabs, D-pad modes (rows UI1a, UI1c)

## Existing code (file:line)
- Options panel (file select only): `src/filemenu/filemenu_options.c`. Window 200x214 at `filemenu_options_open` (line 56). Draw: `filemenu_draw_contents_options_panel` (line 100), 10 rows, 17 px pitch, label at baseX+24, value at baseX+140, labels are message arrays `filemenu_msg_opt_*` in `src/filemenu/filemenu_msg.c` (line 442 on) with ids `FILE_MESSAGE_OPT_*` in `include/enums.h` (line 5414). Input: `filemenu_options_handle_input` (line 150). F11-9: stick gate 56 of 80 and a 6 update move gap. F11-10: live apply in `pm_options_flip`.
- Settings: `3ds/src/pm_options.c`. `pm_opt` (`PmOpt` in `3ds/include/pm_port.h` line 122), `pm_options_get`/`pm_options_flip` (row numbers 0 to 8), `pm_options_write`, `pm_options_init`, `pm_options_close`, `pm_options_build_msg` (text to game message, BN1 footer).
- options.ini keys today: `version fps60 wide hud stats nofog nolight o3dsz log loglevel fixedseed` and `map.<A B X Y L R ZL ZR START DUP DDOWN DLEFT DRIGHT CSUP CSDOWN CSLEFT CSRIGHT>=<NONE A B Z START L R CUP CDOWN CLEFT CRIGHT>` (written only when different from the default). The key names are the debug switch names (`nofog`, `nolight`, `hud`, `stats`, `log`, `loglevel`, `fixedseed`).
- Select menu (port text menu, bottom screen): `3ds/src/pm_options_ui.c`. It uses `pm_options_get/flip`, `pm_map[]`, `pm_n64_next`, `pm_map_reset`.
- Input layer: `3ds/src/pm_input.c`, `pm_pad_scan` (line 120, called each retrace from `pm_main.c` line 295). `hidScanInput`, `hidKeysHeld/Down`, `in_key[]` table, `pm_map[]` loop, circle pad `hidCircleRead` and `clamp_stick` (v*80/154, clamp 80). The N64 has one stick. The "C-stick" of the game is the four N64 C buttons. The 3DS C-stick (N3DS) is `KEY_CSTICK_*`. Default map: D-pad and C-stick and X, Y give N64 C buttons. `pm_new3ds` (`pm_main.c` line 436, `APT_CheckNew3DS`) is the device test, also used by the HUD.
- Test input: `padscript` (`pm_input.c` line 31). It already gives N64 buttons and stick values. It cannot press 3DS keys, so UI1 adds tokens `k_dup k_ddown k_dleft k_dright k_L k_R` (3DS keys, before the mapping).
- Debug switches: `hud`, `stats`, `nofog`, `nolight`, `log`, `loglevel`, `fixedseed` already have options keys (rows 2 to 8). Warp: `3ds/src/pm_warp.c`, `presets[]` (line 29), `pm_warp_active` (line 89, reads switches `preset`, `warp`, `story`), `pm_warp_hook` (line 165, called once at GAME_MODE_LOGOS).

## Options placement (no option disappears)
- General: 60 FPS (row 0), Widescreen (row 1), Back.
- Controls: D-pad mode (new, `dpad_mode`), 17 rebind rows, Reset controls, Back.
- Debug: HUD, Stats, No fog, No light, Logging, Log level, Fixed seed, Warp preset, Warp now, Back.
- `o3dsz` stays an options.ini key only (it has no row today). The Select menu keeps all rows and gets the D-pad mode row.
- New keys: `dpad_mode=multi|cstick|analog` (default multi, O3DS has no `analog`), `warp=<preset name>` (absent = off; warps at boot like `preset=`; a switch wins).

## Panel model
- One row table in `pm_options.c`: tab, row count, label, value, action. The game panel draws strings through a text to message encoder (`pm_options_msg`), the Select menu uses the same value and action functions. One storage, two views.
- Tabs: L and R of the 3DS (raw keys, `pm_pad_take_tab`) and N64 L/R. A tab strip at the top of the panel, the active tab in a second palette.
- The Controls tab has 19 rows and the window shows 10: the panel scrolls with the cursor.

## Multifunction D-pad
- `dpad_mode`: 0 multi, 1 cstick, 2 analog (N3DS only; on O3DS a stored `analog` reads as multi).
- In `pm_pad_scan` the D-pad keys whose `pm_map` value is a C button are the "C-stick target". Targets: C buttons (`cbtn` = mode is not analog) and stick emulation (`emu` = mode analog, or multi and not free overworld). A D-pad key mapped to a non-C button always works as mapped and never emulates.
- Emulation: full value 80 per axis, diagonals set both axes, the circle pad wins when it is moved more. The game turns the stick into direction buttons with the same dead zone and repeat as the circle pad (F11-9 gate and `input.c` repeat), so menus repeat the same way.
- Diagonal in a four direction menu: both axes are sent, the menu code takes the stronger axis it tests first. Default, see `3ds/QUESTIONS.md`.

## Test for the free overworld (`pm_free_overworld`, `3ds/src/pm_dpad.c`)
Free overworld = all of:
- `get_game_mode() == GAME_MODE_WORLD` (pause, battle, file select, intro, map change, game over, demo are other modes, so they count as menu).
- `gPlayerStatus.flags` has none of `PS_FLAG_INPUT_DISABLED` (set by cutscenes and scripts that take the control), `PS_FLAG_CUTSCENE_MOVEMENT`, `PS_FLAG_PAUSED`, `PS_FLAG_ENTERING_BATTLE`, `PS_FLAG_HAS_CONVERSATION_NPC`.
- No message printer is active (`pm_msg_active` in `src/msg.c`, flag `MSG_STATE_FLAG_2`): a message box counts as menu.
- No popup menu: `gPopupMenu != NULL && gPopupMenu->result == POPUP_RESULT_CHOOSING` (shops, item and partner popups).
Not covered: a world screen that is not one of these (none known). A script that shows a custom window without input disable would count as free overworld.

## Tests
- Azahar cannot press keys. `padscript` tokens `k_*` inject 3DS keys. Shots: tabs (normal, wide), L/R, D-pad in file select, pause menu, a battle menu, overworld camera (C button). Statehash and counters against `baseline_azahar_840.txt` in a run without inputs.

## Select menu mirror (PT59)
- The Select menu (`3ds/src/pm_options_ui.c`) and the Options panel of the file select (`src/filemenu/filemenu_options.c`) show the same three tabs: General, Controls, Debug. L and R change the tab. Both menus read one row table in `3ds/src/pm_options.c` (`pm_optui_rows`, `pm_optui_label`, `pm_optui_value`, `pm_optui_step`, `pm_optui_act`). Each function takes `mode`: 0 is the file select panel, 1 is the Select menu.
- The two menus keep no row code of their own. They only draw and read the pad. A change in one menu is the same change in the other: both write `pm_opt`, `pm_map` and options.ini (saved when the menu closes), and the live apply code in `pm_options_flip` runs in both.
- Mode 1 adds rows to the General tab: `Return to Title Screen` and `Exit to Homebrew` (A twice). In mode 1 `Warp now` changes the map at once (`pm_warp_live`). In mode 0 it leaves the file select. The Warp choice is not saved.
- Test: padscript tokens `SELECT` (opens the menu), `ddown`, `dup`, `dleft`, `dright`, `A`, `B`, `L`, `R` act on the menu. The log shows `menu: tab <t> row <r> <label> = <value>` at debug level.
