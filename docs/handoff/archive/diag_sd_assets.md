# SD card vs RAM: asset loading on 3DS (read-only check)

Answer: the ROM is loaded fully into RAM at boot when it fits. Otherwise the port streams it from the SD card during play. No romfs is used.

## 1. Where the data lives
- ROM file: `sdmc:/3ds/PaperMario/papermario.z64` (or `baserom.us.z64`), cwd set at `3ds/src/pm_main.c:658-660`.
- Full ROM: `PM_ROM_FULL` = 0x02800000 = 40 MiB (`3ds/src/rom_remap.h:2`).
- Packed image: `PM_ROM_PACKED` = 0x01A78A6E = 27,757,166 bytes = 27,107 KB (26.47 MiB) (`rom_remap.h:3`). 121 keep ranges (`rom_remap.h:5`), 0x00000000 to end of data, dropped ranges zeroed.
- Save: `save.flash` on SD (`3ds/src/pm_flash.c:39-41`), RAM copy `flash_mem` of `FLASH_SIZE` = 8 x `FLASH_SECTOR` (`3ds/src/pm_os.c:523-524`).
- Options: `options.ini` on SD (`3ds/src/pm_options.c:66`, `:86`).
- No `romfs` or `FSFILE` use in `3ds/src` or `3ds/include`. `FSUSER_*` is used only for the debug log (`3ds/src/pm_mem.c:56-62`).

## 2. Load mode
- `pm_rom_init()` at `3ds/src/pm_os.c:208-281` checks size, magic and FNV head hash, then tries a packed RAM copy.
- Packed copy: `memalign(16, PM_ROM_PACKED)` at `pm_os.c:255-263`. It needs `need <= cap`, where `cap = heap - 3458 KB` (`pm_os.c:250-252`).
- If the copy fails, the port falls back to a file stream (`pm_os.c:275-279`). The log line is `rom streaming`. Reads use `fseek` and `fread` at `pm_os.c:405`.
- Memory-mapping is not used.
- Heap size is not a constant in code. It is `__ctru_heap_size` (`3ds/src/pm_mem.c:104`, `:131`). The boot line `rom resident %uKB of %uKB` (`pm_os.c:269`) gives the real value. An older log in `3ds/logs/builds` shows `rom resident 32519KB of 40960KB (packed)`. That is from an older packed size, not the current 27,107 KB.
- Other budgets: FCRAM cap 64 MiB (`pm_mem.c:18`), linear fixed 4968 KB (`pm_mem.c:20`), texture budget 1536 KB (`src/pm_tex.c:22`).

## 3. Reads during play (game side, all go through `pi_read_raw`, `pm_os.c:333`)
- Entry points: `nuPiReadRom` `pm_os.c:438`, `osPiStartDma` `:484`, `osEPiStartDma` `:491`.
- Shared DMA helper: `dma_copy` `src/43F0.c:439-453`, chunk 0x2000 (`src/43F0.c:57`).
- Map load: `load_asset_by_name` `src/world/world.c:271-291`, called from `src/world/world.c:152` and `src/state_battle.c:181`, `:199` (`get_asset_offset`).
- Textures: `src/model.c:2053`, `:2085`, `:2097`, `:2142`, `:2277`.
- Battle: `src/battle/181810.c:323`, `:325`. Battle stage: `src/battle/btl_states_actions.c:226`, `:233`.
- Audio samples: `src/audio/core/engine.c:1421`, `:1428` (0x2000 chunks).
- Actions and partners: `src/world/actions.c:197`, `src/world/partners.c:564`.
- Items, HUD, msg: `src/item_entity.c:329`, `:362`; `src/hud_element.c:201`, `:237`; `src/msg.c:223`, `:831`.
- Pause and overlays: `src/state_pause.c:146-159`, `:249`.
- Title and logos: `src/state_title_screen.c:190`, `src/state_logos.c:81`.
- Save: no ROM read. Writes go to `save.flash` on a worker thread (`pm_flash.c:76` mark, `:39-41` write). Flush on HOME, sleep, exit (`src/pm_log.c` `cpu_hook`, `pm_main.c:635`, `pm_warp.c:443`).
- Texture pack `texbake.pak` is opened lazily at `src/pm_tex.c:635-638`. The exact first-use trigger is not traced.

## 4. Sizes and heap budget
- Resident case: packed ROM 26.47 MiB sits in the heap, next to the 4968 KB linear fixed pool, 1536 KB texture budget and the game heap left after the 3458 KB reserve.
- Streaming case: only the 0x10000 head is read at boot. Every `pi_read_raw` call that hits the SD card adds to `romstall` logs (`pm_os.c:47-55`, threshold 500 ms per frame).
- Debug-only SD reads (not assets): `pm_input.c:131`, `pm_sweep.c:162`, `pm_warp.c:144`, `:153`, `pm_mapdata.c:28`, `pm_main.c:578`.

## Not verified
- Exact heap size on O3DS and N3DS (not in source).
- Whether the packed copy fits on O3DS now (needs a boot log line `rom resident` or `rom streaming`).
- `.claude/worktrees/` copies were skipped as out of scope.
