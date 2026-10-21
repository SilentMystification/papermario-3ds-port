# CRASH1011: O3DS build 1011 data abort at boot

Log: 3ds/logs/perf/hw_pull6/hw_o3ds_log.txt. ELF: 3ds/logs/builds/pm_3ds_1011.elf. Read only investigation, no run.

## Mapping (arm-none-eabi-addr2line, 1011 ELF)
- pc 00175068: au_engine_init, src/audio/core/engine.c:254 (au_fetch_SBN_file inlined, first SBN lookup after au_load_BK_headers).
- lr 001766a0: au_load_BK_to_bank, engine.c:1150 (the BK header read at engine.c:1140 got zeros).
- Stack: 0017b10c create_audio_system (system.c:101), 0017b5a8 nuAuDmaNew (system.c:264), 001487f0 boot_main (main.c:96), 00101920 game_thread (pm_main.c:540).

## Cause (proven)
1. The ROM is not resident. Boot log line 15: "rom streaming (41943040 bytes, cap 27068KB)". Build 970 had "rom resident 27106KB of 40960KB (packed)".
2. 3ds/src/pm_os.c:230-233: the packed image needs PM_ROM_PACKED 0x01A78A6E = 27106 KB (3ds/src/rom_remap.h:3). The cap is heap minus 4 MB (pm_os.c:234-237). Heap is 31164 KB in 1011 and 31344 KB in 970. Cap 27068 KB < 27106 KB, short by 38 KB. In 970 the cap was 27248 KB (142 KB spare).
3. The heap is what is left of the commit after the image: pm_mem.c:78-106 gives all commit (65536 KB) minus 14 MB linear pool to the heap. The image grew 183664 B (arm-none-eabi-size: bss +159704 B, text +23904 B; "used-by-image" 19856 to 20036 KB). So the heap shrank 180 KB.
4. Bug in the streaming branch: pm_os.c:250 sets rom_size only in the resident branch. In the streaming branch (pm_os.c:261-264) rom_size stays 0. pi_read_raw then takes pm_os.c:335 "addr >= rom_size" for every read: logs PAST END OF ROM, fills the buffer with zeros, and never reads the file. The log shows only 2 lines because rom_suspect (pm_os.c:270-276) logs one per distinct address and the crash came at the third read. Offsets 0x00f00000 and 0x01e40030 are only the first reads of the audio init (SBN header, BK header), not special assets. They are not past the real end (0x2800000).
5. The zero data gives a null file entry pointer in au_fetch_SBN_file: far 0, r0 0000ffff. The code that streams was never run since commit a7ad808596 added the rom_size check (the streaming path has been broken since then; every run so far had resident ROM).
6. Azahar: heap is larger (commit limit 98304 KB), the ROM is resident, so the path does not run there.

## commit used 65536 KB of 65536 KB
Not an out of memory sign. __system_allocateHeaps (pm_mem.c:98) takes the whole commit at boot as one linear block, so used equals limit in every build. No NULL from an allocation. The FL1 slab, the MEM1 command buffer and the dsp cache come out of the linear pool or heap after this and are not the cause: 970 had the same sbrk-left (3860 KB at boot with ROM resident). The 180 KB image growth is the cause. The dspaudio line (cache 1536 KB) is printed before the first read and did not fail.

## Fix
- Minimal: pm_os.c:30-38 area: lower `leave` (pm_os.c:234) from 4 MB to 3.5 MB. Cap becomes 27644 KB > 27106 KB. Free heap after the ROM is 4058 KB at boot, more than 970 had (3860 KB). Better as well: cut 40 KB or more of image bss (the largest 1011 bss growth), then keep `leave`.
- Also fix the real bug: set rom_size = (u32)sz in the streaming branch before pm_os.c:261, so a console with less heap reads the file instead of zeros.
- Add a check in pm_os.c that logs one error line and stops (pm_fatal_stop) when a read passes the end with the file open.

## Switches
No runtime switch avoids it. The decision happens at pm_rom_init before any switch allocates, and the image size is fixed at build time. dspaudio off, abtog without dlc, dspcache and nofl1 do not change image or heap. romfull makes the need larger. Next push: build with the `leave` change, keep the same switches.
