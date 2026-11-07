# Agent boot (row BOOT, epic BOOT)

STATE (BOOT-1, branch boot)
- Branch boot, row file docs/handoff/row_BOOT.md (not deleted: BOOT-1 gate open).
- Done: pm_boot_mark marks (pm_main.c, pm_os.c, pm_nusys.c), `boot ms` line at first title or intro step, bench boot_ms.
- Open: cold boot number. Runs 3627 (warm) and the first cold run (no line) are recorded.
- Cold start blocked: both shared slots (build3ds/azslots/slot1, slot2) hold kept Azaharas; a cold run needs one closed.
- Next: orchestrator closes one slot or waits for azreap (15 min idle), then run `bench.py run boot_ms --row BOOT --lane boot`.
- Next (BOOT-2): O3DS push for the stopwatch and boot ms on hardware.

## Boot line (BOOT-1)
`boot ms main X log Y opt Z rom R (M MB/s) gfx G audio A first F title T`, printed once by pm_boot_title_check (pm_main.c) at the first step of game mode 2 (title) or 16 (intro). X, Y, Z, G, A, F, T are ms after main() (X = game thread entry). R is the ROM read in ms (pm_rom_init open to read done) with the rate.
Reason for mode 16: the no-warp boot skips the title (log: game mode 1 logos, then game mode 16 at frame 327, no mode 2).

## Warm run results (build 2583, lane boot, commit d08155c4b plus BOOT-1; NOT cold)
boot ms main 208 log 461 opt 770 rom 4985 (5.0 MB/s) gfx 5801 audio 5911 first 6259 title 16681
Run 3627, warm (kept Azahar restarted, first marker 3.1 s). Warm does not show the 3dsx load (launch to main).

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
- Cold: the first run with a new lane shows "cold (restart failed)" in slot 2 (log line 1). It reached first marker at 17.7 s and the until line was not printed in 45 s (title never reached; trigger fixed, see above).
- A boot that skips the title (no warp) is the default boot_ms path.
