# Row BOOT: faster boot (user, 2026-11-07)

## Oracle plan BOOT
The log lines carry no tick stamps (build3ds/azt/logs/spot_mac01_pic.txt head: "#n" sequence numbers only), so the boot is not timed today. The AZT "first marker after 3.6 s" is a warm-restart number and skips the 3dsx load. The costs below are estimated from the code.
Boot order: main (pm_main.c:744: mkdir, chdir, game thread) → game_thread (pm_main.c:653). The game thread does, in order:
1. fopen debug3ds.txt, fopen options.ini;
2. pm_log_init and pm_log_start (cpu_apply 80, writer thread with a 1 s start wait at pm_log.c:952, sink thread);
3. pm_wd_start;
4. pm_options_init;
5. pm_rom_init (pm_os.c);
6. pm_game_bind;
7. boot_main (nuBoot: gfx/C3D init at pm_gpu.c:1687, shader parse at :1724, audio init at pm_audio.c:51 with ndspInit plus dspfirm.cdc, then logos and the title state).
Costs, ranked:
1. Resident ROM read, pm_os.c pm_rom_init: 26.3 MB (PM_ROM_PACKED 0x01924FA4) read by stdio fread in the pm_rom_keep ranges, synchronously on the game thread before boot_main. At an O3DS SD rate of about 4-8 MB/s through stdio this is 3-7 s, all of it before the first frame. By far the largest item.
2. The 3dsx image: build3ds/pm_3ds.3dsx is 13.5 MB, and the homebrew loader reads it all before main: about 2-3 s on O3DS. Code alone is not that large; large .rodata or .data (generated tables, the map restore table, embedded shbin or assets) is likely. Not measured yet.
3. Game-side logos: state_logos timing is game design (N64 logos, a few seconds); a skip option would be a user decision. Title assets are Yay0-decompressed and decoded and uploaded through pm_tex on the CPU (tens of ms).
4. Audio: ndspInit loads dspfirm.cdc from SD and boots the DSP, about 100-300 ms, on the boot path.
5. Small SD and thread steps, each about 10-50 ms on O3DS: debug3ds.txt, options.ini read twice (pm_main.c:659, :677, then pm_options_init), log file and crash.txt opens, mkdirs. The writer start (wait up to 1 s, normally 0 ms; the log shows "after 0 ms"). gfx/C3D init and DVLB parse: small.
Measurement (one cold Azahar boot plus one O3DS push):
- Add pm_boot_mark(name), a cold function storing svcGetSystemTick in a static table, at: main entry, game_thread entry, log started, options done, ROM open, ROM read done (with bytes and ms), gfx init done, audio init done, boot_main entry, first drawn frame, first title-state frame.
- Print one line `boot ms main X log Y opt Z rom R (MB/s) gfx G audio A first F title T` at the title.
- Loader time (launch to main) is not visible to the app: the user times the console launch to the first frame with a stopwatch once, and main→title comes from the line.
- Also `arm-none-eabi-size -A` on the elf plus the map: the 10 largest .rodata/.data symbols.
- Use a cold start in the AZT run (a warm restart hides the 3dsx load).
Fix per item:
1. ROM: stream it on core 1 in boot-use order. A loader thread (prio 0x30, core 1, after cpu_apply) reads pm_rom_keep in 1 MB chunks with FSUSER_OpenFileDirectly/FSFILE_Read (no stdio), and marks each chunk ready (a bit array plus a LightEvent). nuPiReadRom and osPiStartDma (pm_os.c:417, :463) wait on the chunk's event only when it is not ready yet, and count the waits. The header check stays synchronous (it is small). Order the chunks by first use: log the first-touch order of chunks once, with switch `romorder`, and generate the order table. RR2 still holds: the whole ROM ends resident, with no SD streaming during play.
2. 3dsx: move the big .rodata/.data tables named by the map into a data file read lazily, or derive them from the resident ROM at boot. One row per table, gated by the 3dsx size.
3. Audio: run pm_audio_init on the core 1 loader thread after the ROM chunks the logos need. The first sound waits on an event; game logic does not.
4. Options and switches: read options.ini once (merge the prof=1 pre-read into pm_options_init).
5. Logos: show the user the measured time and let them decide on a skip option; not done by default.
Gate:
- On O3DS, the `boot ms` line: title T, and first frame F, at least 3 s lower than before (expected: R leaves the critical path). The stopwatch from launch to title is better by the same amount.
- ROM wait count during the boot logged, and 0 waits after the title (no stalls in play).
- A full playthrough smoke (golden_multi) unchanged; mac_01 byte-equal.

## Rows
BOOT-1 MEASURE (start now): pm_boot_mark table and the `boot ms` line, the elf size listing (10 largest .rodata/.data/.bss symbols and their objects into docs/handoff/agent_boot.md, with the INDEX.md line), one cold Azahar boot. No behaviour change. The orchestrator pushes for the O3DS number.
BOOT-2 and later: wait for the oracle plan MEM (memory use and duplicates), which may change how the ROM stays resident; the orchestrator adds them here.

## Rules
Own worktree: git worktree add build3ds/wt/boot -b boot nds1; copy assets/us and ver/us/build; PM_LANE=boot. Code first (CLAUDE.md rule 5). Cold start for the boot run. No attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Delete this file in the last BOOT commit.
