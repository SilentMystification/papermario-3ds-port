# Agent boot (row BOOT, epic BOOT)

STATE (row BOOT done, branch BOOT)
- ROM streamed at boot (3ds/src/pm_romload.c): header check, image allocation, then a loader thread; the game starts at once.
  Readers: pi_read_raw reads bytes the stream has not reached straight from the file (exact size); pm_rom_ptr waits for or reads
  the 128 KB chunk. Bench boot_ms (gate rdone > 0: the stream ends before the title, so no wait after it), boot_logo_pic, boot_romorder.
- Azahar (warm): base run 80966, stream run 81566 (whole chunks), step B run 81869 (boot order, not kept), final runs 82172 and 84827.
- BOOT2 base (Azahar, cold, 73575adc0): first draw 1836 ms after main, title 12763; 59 chunks and 37 direct reads before the first draw.
  The bank loads (au_load_BK_to_bank) copy through au_read_rom -> pm_rom_ptr (src/audio/core/engine.c:1428): each touch demands a whole
  128 KB chunk. Deferring create_audio_system to the first draw does not work as is: the first step (set_game_mode path, sound resets
  before GAME_MODE_STARTUP) calls snd_set_bgm_volume (src/audio/bgm_control.c:64) and starts the engine on the game thread before frame 0.
- Open (user, console): boot ms before/after on O3DS XL and N3DS; HOME and sleep during the logos (io_gate waits for the stream read
  in flight); the cost of the 37 direct reads and 42 chunk demand reads before the first draw on a real SD card.

## Boot line
`boot ms main X log Y opt Z rom R (M MB/s) gfx G audio A first F title T rdone D waits W wms V`, printed once by pm_boot_title_check
(pm_main.c) at the first step of game mode 2 (title) or 16 (intro; the no-warp boot skips the title). Times are ms after main()
(X = game thread entry). R: ROM open to stream start. M: stream rate. D: whole image in RAM (0 = not yet). W, V: reads that
blocked (direct reads included) and their ms. The loader prints `rom stream done ms .. MB/s .. demand .. waits .. wait_ms .. direct ..`.
Switch romorder (debug): first touch of each 128 KB chunk (`romorder c<idx> f<frame> t<ms>`), counts after the boot line.

## Boot ROM reads (romorder, run 81263)
59 of 320 chunks before the first frame, the same 59 before the title: 57 audio-bank chunks (SBN, small header and table reads of
au_load_BK_to_bank), then c8, c15, c16 at logo frame 15. No ROM read during the logos (frames 15 to 328). Map reads start after.

## Size of the elf (build3ds/lane-boot/pm_3ds.elf, arm-none-eabi-size -A)
.text 3,326,908 B; .rodata 203,668 B; .data 9,060,724 B; .bss 9,816,280 B; .mapstat 226,868 B; debug sections not counted.
The 3dsx (13.6 MB) carries .text, .rodata, .data: the .data bulk (9 MB) is loaded before main. Measured as one 3dsx size only.
Largest initialized data and rodata (nm -S, D or r):
1. .LC2 (r) 133,696 B
2. pause_world_map_png 102,400 B
3. dgb_01_AS_SmashBridges 64,076 B
4. AS_ToyTank_MoveForward 33,710 B
5. b_area_flo2_huff_n_puff_EVS_Attack_TuffPuffSwarm 28,992 B
6. kmr_11_AnimScript_Fortress 27,420 B
7. b_area_flo2_huff_n_puff_EVS_Attack_GroupSlam 23,260 B
8. Entity_MunchlesiaReset1_vertexArray 19,152 B
9. Entity_MunchlesiaSpitOut_vertexArray 19,152 B
10. Entity_MunchlesiaBeginChew_vertexArray 19,152 B
Largest bss: heap_generalHead 1,572,864 B; D_80164000 1,344,608 B; heap_spriteHead 1,048,576 B; mc_tab 622,592 B; TextureHeap 425,984 B; mc_pool 413,696 B; AuHeapBase 352,256 B; gBackgroundImage 262,144 B; gMapShapeData 262,144 B; slots 262,144 B.
The rest of .data is 29,600 small symbols (D and r together 9.47 MB).

## Notes
- Warm and cold: a warm restart in a kept Azahar does not show the 3dsx load (launch to main).
