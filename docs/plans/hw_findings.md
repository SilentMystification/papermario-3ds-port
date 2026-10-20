# O3DS hardware findings (investigation HWLOG)

Sources: 3ds/logs/perf/hw_pull/hw_o3ds_log.txt (2406 lines, build 891, O3DS, switches `--verbose loglevel=debug`, no phase switches), 3ds/logs/perf/hw_pull/hw_o3ds_dumps/ (37 dumps, 00000000 to 00000036), source code at main c98477fdc. No new run, no console access, no Azahar start.

Ranking by user impact:
1. The live log does not work on this build (item E). Cause is in code. Fix is small.
2. The audio job on core 1 makes the game thread wait 6.7 to 16.7 ms per step on the O3DS (item A). Azahar shows 0.5 to 3.7 ms audio cost per tick.
3. The save writes 40 files open/close cycles on the game thread (item D).
4. The scene change with 11 NPCs costs 6 steps on Azahar and more on hardware (item C).
5. The crash dumps cannot be mapped to code (item B). No dump shows a fault in the HOME/sleep hook.

## A. Hardware performance

What the log holds: boot lines, 89 `game mode` lines, 89 `audio cost` lines, 5 `audio wait` lines, 18 `mem map` and 8 `mem tick` blocks, 8 `log writer heartbeat` lines, 1061 `b7` lines, comb lines. Run length: last game step is frame 31919, that is 3.9 cycles of the 8233 step demo loop (game mode 2 at f3937, f8233, f9057, f10199).

What the log does not hold: no `pc` lines (no `phase phasecsv`), no `pace: wall` lines (no `pacestat`), no `phase` lines, no per-step wall time, no time stamp at all. hw_seg.py stops with "no pc lines or no game mode lines". So per-segment pace, late steps, cpu ms and gbi ms per step are not available. The log has no total run time. A weak bound only: the writer heartbeat grows by 845 per 3600 steps in the demo loop and the writer wakes every 250 ms or less, so 3600 steps took at most 211 s (at least 17 steps per second). Do not use it as a pace.

CPU limit lines: `cpu limit: set 80 (start) result 00000000`, `game value 80`. At the end `cpu limit: set 30 (HOME menu opens) result 00000000`, then `boot_main returned`. There is no "HOME menu closed" line: the user pressed HOME and closed the game. No error result on any cpu limit call.

Audio (the one data set that works on hardware), per 60 Hz tick:
- `audio cost avg`, O3DS: intro 4381 to 11084 us (mean 7709, 11 samples), demo 7741 to 8812 us, later cycles 6296 to 9832 us. Max per tick 11.9 to 24.2 ms. A 60 Hz tick is 16.7 ms.
- Azahar (3ds/logs/perf/full/cur_normal.txt): intro 472 to 5221 us (mean 3154). Boot logos 472 us. So the O3DS audio cost is 2 to 10 times the Azahar value in the same segment.
- `audio wait` (game thread blocked in the join, core 0): avg 6666, 16663, 12296, 10582, 10742 us, max 32.7 to 60.0 ms. A step is 33.3 ms. The game thread waits 20 to 50 percent of a step for the audio thread on core 1. Job avg 2.8 to 24.4 ms, max up to 80 ms (80 ms is 2.4 steps). Join site counts: all joins are at the `main` site (pm_audio.c:194 `pm_audio_join_at`). The `audio wait` line stops after join 3000 (about f8200) in this log. The reason is not known (the print is every 600 joins, pm_audio.c:205).
- Azahar has no `audio wait` line in the logs in 3ds/logs/perf. No comparison.

Memory (O3DS 64 MB cap): linear peak 11466 KB of 14208 KB, 0 texture failures, VRAM free min 4370 KB of 6144 KB, texture cache peak 4096 KB (full at the budget, so cache evictions happen in the busy scenes), vertex slots max 5196 of 32768, command buffer max 3 percent, 0 vertex drops, game stack high-water 4716 of 261632 B, audio stack 1844 of 65024 B. Heap free-in-arena 2 to 9 KB on every map line: the main heap is full (27556 of 32032 KB used, sbrk-left 4472 KB). Log writer dropped 0 lines.

Compare with Azahar (3ds/logs/perf/speed_ref_860.txt, baseline_azahar_840.txt): Azahar numbers are host time (Ryzen 7 PRO 5850U), not O3DS time: gbi 9.64 ms per step, 133 steps over 33.3 ms for the whole cycle at "fast". They cannot be compared with hardware ms. Counters (draws, texmiss, texKB) are equal between targets, so only a hardware run with phase switches can give the O3DS ms.

Ranking of where the O3DS time goes, as far as the data allows:
1. Audio: the game thread waits for core 1 (avg 6.7 to 16.7 ms per step, 20 to 50 percent of the step budget). This is the only large, measured hardware cost.
2. Texture cache full (peak 4096 KB at budget 4096 KB): texture decode on the game thread in busy scenes. Count only, no ms.
3. GBI and rest: unknown on hardware.

New run needed (command H2 in 3ds/logs/perf/commands_hw_891.txt): `-t 330 audiostat pacestat logstat phase phasecsv loglevel=debug fixedseed statehash=10`, then FTP pull and `python3 3ds/tools/hw_seg.py <pulled log> "build NNN O3DS H2"`. Without `phase phasecsv` there is no per-step data. `pacestat` gives the late-step count. The live log must work first (item E) or the pull must be used (the pull has all lines).

## B. Crash dumps

Format (checked on all 37 files): Luma exception dump v3.1. Header 40 bytes: magic DEADC0DE DEADCAFE, version, processor word (0x0b, or 0x1000b), type (3 = data abort, 2 = prefetch abort: all type-2 dumps have a bad pc 0x0, 0x40800000 or 0xa67e0000, all type-3 dumps have a fault address), total size, register dump 92 bytes, code dump 96 bytes, stack dump, 16 bytes extra = process name (8 bytes) + title id (8 bytes). Registers: r0-r12, sp, lr, pc, cpsr, DFSR, IFSR, FAR, FPEXC, FPINST, FPINST2.

The 244 byte dumps (00000016, 18, 21, 22, 24, 25, 27, 35, 36) are not notes. They are full dumps with stack dump size 0 (and 21 with code size 0). The stack dump is missing because the stack pointer was on an unreadable page or the thread stack was not mapped for Luma.

List (number, size, process, type, pc, lr, sp, FAR):
- 0000, 0002 to 0006 (1588 B): rehid (title 0004013000001d02, system module), data abort, pc 00101e68, lr 00101e10, sp 00118ac0, FAR d8e007f7. Same crash 6 times.
- 0001 (916 B): MarioKar (0004000000030800), pc 00420988, other game. Not this port.
- 0007 (780 B), 0008 (332), 0009 (448), 0010 (504), 0011 (924), 0012 (2644), 0015 (436): 3dsx_app (host title 000400000d921e00 = Homebrew Launcher host), sp 0x0800xxxx, small pc values 0x1020xx to 0x1d2548. Homebrew from the old layout. Not matched to a build.
- 0013 (1012 B), 0014 (1676 B): nwm (0004013000002d02, Wi-Fi system module), pc fff1c1d8 and 001204c8.
- 0016 to 0036: 3dsx_app. 0016, 0018 are small pc (1076a0, 1045dc). 0017: prefetch abort, pc 0. 0019, 0020, 0028 (sp 0x0801bc60). 0021: prefetch abort, pc 40800000, lr 00132294. 0022 to 0027, 0029, 0030, 0035: pc in 0x3f7bb4 to 0x43e768, sp 0x30e1xxxx. 0031 to 0034 (3356 B): prefetch abort, pc a67e0000, lr = sp (stack smash). 0036: pc 003acd68, lr 003b47b8, sp 328ae5e8, FAR 328ae5e4, DFSR 805 (write, translation fault), type 3.

Newest dumps (36, 35, 34 to 31): 
- 0036 (244 B): data abort at pc 3acd68. FAR = sp - 4, DFSR 0x805: a push to an unmapped page below the stack, so a stack overflow or a corrupt sp. r0 01393748, r2 01390484 are heap/static pointers.
- 0035 (244 B): data abort, pc 43e768, lr 43e760, FAR = sp + 4 (30e1a0cc, sp 30e1a0c8), DFSR 5 (read, translation fault). In build 891 the address 0x43e768 is in .data (starts 0x435638), so the pc is not code of build 891.
- 0031 to 0034: prefetch abort at pc a67e0000, lr and sp 30e223d8 or 30e523d8, 4 times, near equal. A return to a smashed stack.

Mapping to functions: no ELF exists (`build3ds/*.elf` absent, no .elf in the tree). Only build3ds/pm_3ds.map and pm_3ds.3dsx of build 891 (14:11 today, .text 0x100000 to 0x3deea8, .rodata 0x3df000, .data 0x435638, .bss 0xcd8388) exist. I checked the code dump words of dump 36 against the 891 3dsx at its pc: they differ (891 has `0b20d015` near pc, the dump has `10402de9 704f1dee ...`). The same holds for dumps 22 to 35 (pc is beyond the 891 .text end or in .data). Result: none of the dumps comes from build 891 or its map. The addresses are valid only for the build that wrote the dump, and no ELF of those builds is kept. A map lookup in 891 gives wrong names (for dump 36: entity_ItemBlock_setupGfx+0x60 and C3Di_FrameBufBind+0x154, two unrelated functions, so it is a false match). I do not give function names.

Which dump is the HOME or sleep crash: not decidable from the files. The dump number order is the time order. A "hard crash" of the whole console (not only the app) comes from a system module death: the candidates are nwm 0013/0014 (Wi-Fi module, active at HOME and sleep) and rehid 0000 to 0006 (input module, 6 identical crashes). These two are not this port and not in this port's code. The game dumps (0007 to 0036) are app crashes with a stack or pc fault and none has a link to the APT hook (no pc in pm_log.c in a matching build). The newest dump 0036 is a stack write fault, not HOME related by content.

Does the crash predate the APT hook (ba090e9ea CPULIM-UI, 2026-10-20 12:23)? The user says "a while ago". The hook is 30 commits before HEAD and was added today. HOME/sleep crash reports "a while ago" predate it. The last game log (build 891, with the hook) shows `set 30 (HOME menu opens) result 00000000` and a clean `boot_main returned`: HOME works on 891 in this one run. So the hook cannot have caused the old crash, and one HOME press in 891 did not crash. Not tested: sleep (ONSLEEP/ONWAKEUP) and return from HOME (ONRESTORE).

Code review of the hook (3ds/src/pm_log.c:466 to 515, pm_main.c:289):
- `cpu_hook` runs on the game thread inside `aptMainLoop` (pm_main.c:289). It calls `APT_SetAppCpuTimeLimit` and `pm_log` (non-blocking ring, no lock held). Both are legal there. No finding of a deadlock.
- Risks (not proven): (1) the hook stays installed after exit paths that skip `pm_cpu_limit_exit`; (2) at ONSLEEP the audio thread (core 1) and log writer thread (core 1) keep running while the app suspends (the writer does `fsync` on sdmc once per second: pm_log.c writer loop). SD access during sleep can fail or block. (3) After ONSUSPEND the app has lost the GPU right: the game thread is blocked in aptMainLoop, so no GPU command is sent; the audio job does not use the GPU. I found no GSP use after suspend in the hook path. A test needs HOME and sleep in one run and the dump check after.
- The `3dslink` push script in 3ds/push_3ds.sh warns (lines 77 to 90) that a force-kill of a running netload left both consoles executing corrupted code with a data abort and a wrong pc. That is a second candidate for several dumps with garbage pc (0031 to 0034, 0021): killed or failed netload pushes. In this session the pushes failed with "Connection reset by peer" (3ds/logs/perf/hw_891_H1_push.out).

## C. Intro NPC spawn hitch

Scene: intro map hos_05 (area 5 = AREA_HOS, map 5), entered at step 327 to 331 after game mode 16. `spawn npcs 11` is logged at step 331 (src/encounter.c:2804). Second intro scene hos_04 (`load map area 5 map 4`, `spawn npcs 1`) at step 3484 (Azahar numbering), before the title at f3937. In the O3DS log the same lines are at `#86 load map area 5 map 5`, `#103 spawn npcs 11` (frame 329 to 335).

Azahar per-step data (3ds/logs/perf/full/cur_normal.txt, normal speed, ms are host ms, budget 33.3 ms), phase columns from pm_phase.c:
- Step 331: wall 211.8 ms. logic 50.5, ROM 118.5 (6946 reads, 578204 bytes, avg 83 bytes), Yay0 9.4, fbegin 16.2.
- Step 332: wall 124.4: GBI 35.3, texture decode 61.1, Yay0 15.1. Step 333: 51.7. Step 336: 76.2. Step 337: 135.1 (texture decode 83.8, GBI 38.4). Steps 331 to 340 add to 781 ms for 10 steps (333 ms budget), so about 450 ms are over budget (about 14 lost steps).
- Step 3484 (hos_04, one NPC): wall 421.6 ms, ROM 334.8 (5899 reads, 526300 bytes), logic 44.3, Yay0 9.1. This is the worst step of the intro after boot (step 1 is 5497 ms, boot).
- Step 3938 (title): 98.7 ms, texture 40.9.
Which phase spikes: ROM reads (118 and 335 ms), then texture decode on a miss (61 to 84 ms in the 6 steps after the spawn), then logic (45 to 50 ms), Yay0 (9 to 15 ms). Audio load is not visible in the step data. Azahar time is host time: the O3DS is much slower per instruction, so the real hitch is longer. No O3DS number exists.

Loading path (all on the game thread, core 0):
- src/101b90_len_8f0.c:159 `spr_load_sprite`: per NPC sprite: 2 ROM reads (`nuPiReadRom`), `general_heap_malloc` of the compressed size, `pm_yay0_decode` of up to 0x40000 bytes, `spr_host_endian` over the whole result, a swizzle loop up to 512 raster entries. Called from src/sprite.c:1075 `spr_load_npc_sprite`.
- 3ds/src/pm_map.c:393 `find_asset`: every `load_asset_by_name` reads the whole asset table (up to 0x20000 bytes) from ROM into a malloc buffer and scans it with strcmp for each asset (map shape, hit, texture, background).
- 3ds/src/pm_os.c:279 `pi_read_raw`: per read: a loop over `pm_rom_drop` (about 60 entries, rom_remap.h:129 to 253) and `pm_rom_keep` (about 30 entries, rom_remap.h:5 to 129), then `pm_tex_invalidate(buf, size)` (pm_os.c:333 and pm_tex.c:368), which loops over up to 512 texture slots (`slot_hi`) for every read. The ROM itself is resident in RAM (log: `rom resident 27106KB`), so a read is a memcpy, not an SD access. With 6946 reads in one step, the per-read overhead is the ROM cost: 118.5 ms / 6946 = 17 us per read (step 3484: 57 us per read). On the O3DS this overhead is multiplied.
- Texture misses (`tx`): decode on the game thread at first use, 371 to 657 triangles with misses per step after the spawn.

Cheapest fix (about 20 lines, no behavior change): in `pm_tex_invalidate` keep the lowest and highest live image/palette address and return at once when `[ptr, ptr+size)` does not touch that range; in `pi_read_raw` skip the invalidate when `slot_hi == 0`. Then measure `ro` in step 331 and 3484 again with `phase phasecsv` (Azahar, one start). Second fix: merge the small reads of `spr_load_sprite` and `find_asset` (cache the asset table once: it is read-only after boot, so `find_asset` can keep one copy instead of a malloc+memcpy per asset). Third (larger, needs a plan): decode the Yay0 sprites of the next map on core 1 during the fade out (known at `goto_map`), or load NPC sprites one per step (staggered). Texture decode misses could be prefetched the same way. The last two change the load order and need a statehash check.

## D. Save hitch

Path: game code `fio_save_game` (src/fio.c:268) -> `fio_erase_flash` (src/fio.c:480) -> `fio_write_flash` (src/fio.c:452) -> port shim in 3ds/src/pm_os.c:427 `flash_sync`. All on the game thread (core 0), synchronous. No backup file copy is made by the shim. The game keeps 2 copies of the globals (GLOBALS_PAGE_1 and 2) and the save slot copies in the 128 KB chip image.

Blocking calls per save of one slot (SaveData is 0x1380 = 4992 bytes = 39 pages of 128 bytes):
- `osFlashSectorErase`: pm_os.c:473, `flash_sync(off, 16384)`: `fopen("save.flash","r+b")`, `fseek`, `fwrite` 16384 bytes, `fclose` (pm_os.c:429 to 433). 1 open/close.
- `fio_write_flash` loop (src/fio.c:466 to 473): per 128 byte page one `osFlashWriteArray` (pm_os.c:466 to 470), each calls `flash_sync(page, 128)` with its own `fopen`, `fseek`, `fwrite` 128 bytes, `fclose`. 39 open/close cycles.
- Total per slot save: 40 fopen, 40 fclose, 5008 bytes plus 16384 erase bytes written (about 21 KB), 40 separate commits. `fclose` flushes and on sdmc commits the file (FSFILE_Close). No CRC loop on the shim side. The game computes the CRC in `fio_calc_file_checksum` over 4992 bytes (negligible, a few thousand adds).
- Saving the globals (src/filemenu/filemenu_yesno.c:444 and 471, `fio_save_globals`, src/fio.c:123): 2 erases (2 x 16 KB) and 2 x 1 page: 4 more open/close cycles.
- PAL builds write and verify 4 times (src/fio.c:289 to 296): not our build (US).
Estimate (no measurement exists in any log): an sdmc open+write+close on the O3DS costs about 5 to 30 ms (card dependent). 40 to 44 cycles = 0.2 to 1.3 s. This matches "hitch when saving". Add: the game thread cannot run the audio join in that time.

Fix that keeps the save safe (no user decision needed):
1. `flash_sync` only copies into `flash_mem` (already done by the callers) and marks a dirty range (min offset, max end). No file access on the game thread.
2. A flush on core 1 (the log writer thread is on core 1 and exists; or a small own thread with the same libctru natives): wait until no `flash_sync` call came for 2 steps (the game writes all pages in one burst), then `fopen("save.flash","r+b")`, one `fwrite` of the dirty span, `fclose` once. Result: 1 open, 1 write of up to 21 KB, 1 close.
3. Join (wait until flushed) in: the next `flash_sync` call, `pm_cpu_limit_exit`/normal exit, and the APT hooks ONSUSPEND, ONSLEEP, ONEXIT (the existing hook `cpu_hook`, pm_log.c:477).
4. Keep the existing behavior on a write error (log, keep RAM copy).
Atomic rename: the file is 128 KB, but libctru `rename` on sdmc cannot replace an existing file, so a temp-file plus rename would need an unlink first (a window with no file). Not recommended. The game already writes two copies of the globals and checks CRC on load (src/fio.c:108 to 116), so a torn write is detected.
What can go wrong: power loss or HOME-then-quit within the flush window (about 2 steps plus the write, below 0.2 s): the new save is lost, the old data stays (erase and write are already torn today: the current code erases the sector first and writes 39 times, a power cut in the 0.2 to 1.3 s window loses the slot; the fix makes the window shorter, not longer). Suspend during the write: the join in ONSUSPEND/ONSLEEP closes the gap. A crash of the game in the window: the in-RAM copy is lost (the crash handler could call the flush: pm_main crash path).

## E. Live log (TCP 17492)

Code: 3ds/src/pm_log.c:177 to 209 `pm_log_init(live)`: `socInit` with the buffer from `pm_soc_buffer` (pm_mem.c:112), `socket`, `SO_REUSEADDR`, `bind` 0.0.0.0:17492, `listen(1)`, non-blocking. The server starts at boot (pm_main.c:441, `pm_log_init` runs before everything else), well before the PC connects. `accept_client` (pm_log.c:65) runs in the writer thread on core 1 every 250 ms or less (pm_log.c:426 to 428), or in `pm_log_poll` on the game thread before the writer runs. On connect the game replays the last 32768 bytes of the ring (RING, pm_log.c:16), then sends new lines. If the send fails, the client is dropped and a new accept is possible (one client only, a new client replaces the old one). Queue full: the new line is dropped and counted (`q_dropped`, a numbered `lines dropped` line follows, pm_log.c:296, 385). The file log log.txt is written from the same batch, so the file has all lines.
The PC side (3ds/tools/livelog.py) retries the connect every second until the end of the time and reconnects after a lost link, skipping lines by `#seq` and printing GAP lines. 3ds/push_3ds.sh runs 3dslink with game args `-- --verbose <switches>`: `--verbose` here is a game argument, not the 3dslink stdio option. The game never uses the netload stdio redirect on port 17491, so the 3dslink stdio server and the game server do not meet. There is no stdio conflict.

Evidence from this run (build 891, started by 3dslink: the arguments `--verbose loglevel=debug` are in the log, so `netloaded` was true):
- `#0 live log: off (buffer 0x0, 0 bytes, socInit ffffffff)`. The server did not start. `socInit` was never called with a buffer: the buffer pointer is NULL.
- The memory split shows the reservation: linear 14208 KB + heap 32032 KB = 46240 KB = offered 46368 KB - 128 KB. So `started_by_netload()` returned true (pm_mem.c:85, PM_SOC = 0x20000) and 128 KB were taken away. The later `svcControlMemory(&soc_base, 0, 0, soc, MEMOP_ALLOC, ...)` (pm_mem.c:101) failed, `soc_buf` stayed NULL, and the Result is not logged. Probable cause: `MEMOP_ALLOC` with address 0 is not valid on the real kernel (libctru allocates the app heap with an explicit address in 0x08000000 to 0x0E000000). The Azahar note in docs/HISTORY.md line 212 says the path was never run with a real buffer on any target ("forced test with a heap buffer gave socInit e0e01bf5", socket path "NOT run"). Since the first commit (46e73cb81) no log shows a live log from hardware: build3ds/hw_live_log_1.txt (2026-10-20 00:40) and hw_live_log_180.txt (15:21) have 0 bytes. The 3ds/logs directory has no live log from a console.
- The pushes by the orchestrator at 14:18 and 15:19 failed (`Connection reset by peer`, hw_891_H1_push.out, build3ds/.netload_attempt_870226_3.log), so no live read was tried in this session.
Verdict: does NOT work on build 891 (proved by log line #0, not by guess). Nothing in the PC script or the network blocks it: the game side never opens the port. The unverified part: whether `socInit` and the accept path work once the buffer exists (never run).
Fix (row HW1): log the `svcControlMemory` Result; take the 128 KB soc buffer (4 KB aligned) out of the end of the linear pool block that is already allocated (reduce `ours`/the heap by 128 KB and set `soc_buf` to that address) instead of a second `MEMOP_ALLOC` with address 0, or call it with the heap end address. Then `socInit` result goes to the `live log:` line (it already does).
Cheapest test, one push by the user with a named switch: build with the fix, then `sh 3ds/push_3ds.sh --no-build --ip 192.168.1.180 -t 60 loglevel=debug` after the user opens the Homebrew Launcher and presses Y. Look for these lines in the output: `[livelog] connected`, `live log: port 17492` and the build line `build NNN`. If `live log: off (buffer ..., socInit ...)` shows again, the result code names the failure. The FTP pull of log.txt gives the same lines when the live link fails.

## Open items that need the user
- An O3DS run with H2 switches (A) after HW1. One HOME press and one sleep and wake in that run test the hook (B).
- No other user decision is needed for HW1 to HW3.
