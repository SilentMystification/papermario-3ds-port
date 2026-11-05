# Diagnosis SUN4: sun ray texture breaks (no source edit, 0 Azahar starts)

## Status
- Cause not proven. The odd row word swap (LoadBlock) hypothesis is rejected [M].
- No source edit. No build. Step 4 (texel dump) not run: it needs a new probe, which is a source edit.

## Load ops, ROM base list at 0x40C490 [M]
- 0x40C4D0 FD88007F: SETTIMG I8, width 128, addr 0x09000200 (ROM 0x40BDA0).
- 0x40C4D8 F5882100: SETTILE tile 7, TMEM 256, line 16.
- 0x40C4E8 F4000000 071FE01C: LOADTILE tile 7, sl 510, tl 28. 128 by 8 texels, 1024 bytes, into TMEM 256.
- 0x40C4F8 F5802100 0100C180: SETTILE tile 1, TMEM 256, line 16, I4, cms 1 (mirror S), masks 8 (256 texels), maskt 3 (8 rows).
- 0x40C500 SETTILESIZE tile 1: 0 0 1020 28. The ray draws set the tile 1 window in C code (ray lists 0x7C0 to 0x850 have no SETTILE).
- The sun list has no LOADBLOCK. The dxt odd row swap does not apply on the N64 [M].

## Row pitch [M] [C]
- N64: line 16 = 128 bytes per TMEM row (256 I4 texels).
- Port: stride 256 texels (pm_gbi.c:3171), nibble read at src[(sy*stride+sx)>>1] (pm_tex.c:378-380; pm_tex_tx1.h tx1_n4). 128 bytes per row. Agrees with N64.
- Tile 1 draw (log build3ds/last_log.txt, "suntile t1 draw"): src 0x5046a6 = 0x504670 + 54 bytes, stride 256, w 256, h 32, x0 54.

## Break pattern in the shot [M]
- Shot: build3ds/azt/shots/sun_sbk/0000_10_0.ppm (frame 200). 4x crop: scratchpad sun4_f200_x4.png.
- Two diagonal streaks at the right edge. Each is a row of dashes: pitch about 19 px, dash about 10 px, gap about 9 px.
- The dashes lie along the streak.

## Texel row, ROM 0x40BDA0 (rows 0 to 7 byte-identical) [M]
- Rows 0 to 7 are identical. Row layout cannot make T gaps.
- Zero (transparent) texel runs, start and length: (0,4) (12,4) (176,4) (200,8) (220,8) (232,12). Texels 16 to 175 have no zero run.
- Texels 0 to 15 read 0000 cccccccc 0000: a 12 texel period exists there only.

## Offset analysis [C] [E]
- pm_gbi.c:3170 `src += (ult>>2)*l1stride + (uls>>2)` adds 54 BYTES. That is texel 108 in I4. Unit error.
- pm_gbi.c:1195 applies -54 texels in UV (td[1].x0 = 54, pm_gbi.c:3181).
- Derived read: row[(s + 54) mod 256]. Predicts texels 54 to 70 (no zero run, no dash). The shot shows dashes, so the derivation is incomplete. Not resolved. [E]

## Hypotheses (ranked)
1. Unit error at pm_gbi.c:3170 with the x0 path at pm_gbi.c:1195. Likely [E]. Explains a constant S shift, not yet the 12 texel dash period.
2. Odd row swap (LoadBlock). Rejected: LOADTILE, identical rows [M].
3. Quarter texel (uls & 3 = 3 for uls 219) not in t1_p[2]. 0.25 texel, too small for 19 px dashes [E].

## Smallest fix candidate (not applied)
- pm_gbi.c:3170: for I4 (bpp 0), add the texel offset in nibbles: byte offset (uls>>2)>>1, parity in x0. Verify first.
- Predicted if right: no dash pattern in the streak. [E]

## Next (SUN5, needs one source probe)
1. Debug probe: print the tile 1 texel row 0 (texels 0 to 255, hex) at the frame 200 tile 1 draw. One Azahar start.
2. Compare the first texel index with the ROM row. That gives the real read offset.
3. Fix pm_gbi.c:3170, retest sun_sbk. Pass: no 12 texel dash pattern in the streak.
