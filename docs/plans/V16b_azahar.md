# V16b: white box on a flashing enemy sprite (read-only study, no cause proven)

## Code path (all read)
1. src/battle/actor_rendering.c:3492 part_flash_on sets flashEnabled (light 1, medium 8, heavy 14 frames).
2. actor_rendering.c:2041 make_flash_palettes writes white into copiedPalettes[1][i], keeps the alpha bit (PAL_MEM, macros.h:314).
3. actor_rendering.c:2126 spr_draw_npc_sprite(OVERRIDE_PALETTES, flashPalettes). It only fills flashPalettes[i] when adjustedPalettes[i] is not null.
4. src/sprite.c:459 spr_draw_component takes palettes[paletteIdx]. sprite.c:235 gDPLoadTLUT_pal16 loads 16 entries from that pointer.
5. 3ds/src/pm_gbi.c:2029 G_LOADTLUT stores tlut pointer and tlut_n. pm_gbi.c:445-495 sync_render: opaque sprite = CVG_X_ALPHA, no blend, alpha test 0x30 (pm_gpu.c:389 GPU_GREATER). Shaded sprite uses load_shaded_sprite (pm_gbi.c:1436), a 16x16 lut, RGBA8.
6. 3ds/src/pm_tex.c:152 expand: CI4 texel = palette entry, alpha = bit 0. Cache key = img, tlut pointer, FNV of palette (pm_tex.c:392-398).

## Suspected causes (ranked, none proven)
A. pm_tex.c:152: a texel with tlut == NULL or index >= tlut_n becomes 0xFFFF = opaque white. One bad palette pointer or count gives a full white rectangle with the sprite size. This matches "box". Azahar may hide it if the stale pointer still holds valid memory.
B. actor_rendering.c:2054 + 2058: flashPalettes[i] stays stale or null when adjustedPalettes[i] is null. The sprite then loads a null or old TLUT (sprite.c:235). Needs a log, not a guess.
C. The alpha test at pm_gbi.c:489 (0x30) is the only thing that removes alpha 0 texels in the opaque path. If PICA hardware reads the 5551 alpha bit or the filtered alpha differently, transparent texels (now white RGB) draw as a white box. Azahar may differ on bilinear alpha at the edge.
D. O3DS texture cache pressure (pm_tex.c:431 "tex cache full"): a failed load must skip the draw. A stale bound texture would draw white.

## Azahar tests (no code change first)
T1: forced heavy flash on omo Shy Guys (BTL_OMO_FORMATION_03) with Debug "rmoff" (blend always on, no alpha test) and again with "nocomb". A box that shows here proves C.
T2: same run with Debug "depthview=2" and "rmlog". Read the draw index of the sprite quad in the flash frames. Name the quad that is white.
T3: add a log (one line per run) in pm_tex.c:152 when tlut is null or idx >= tlut_n, and in make_flash_palettes when adjustedPalettes[i] is null. Run the forced heavy flash on kmr Monty Mole. Any log line proves A or B.
T4: shrink the texture cache (a debug cap in pm_tex.c) to the O3DS size and run T1. A box proves D.

## Next step
Run T3 first (1 build, 1 start). It is the cheapest test and it checks the only path that makes opaque white with the sprite size.
