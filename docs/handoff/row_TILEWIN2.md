# Row TILEWIN2: a wrapping tile binds exactly one mask period (effect_63, whirlwind: the window size changes every frame)

(Model: Haiku. Waits: TILEWIN merged (same function tex_win1). Edits 3ds/src/nd_tex_set.c tex_win0 and tex_win1 (their first size
blocks) only. Reads this file and CLAUDE.md.)

## Facts (TILEWIN run e80047038: fire_breath texmiss 0.05 fixed; effect_63 4.21, whirlwind 3.79, firework_rocket 1.00 per frame)
## Cause (oracle code read)
- effect_63 tile 1: gsDPLoadMultiTile_4b of a 32 x 128 I4 image, wrap, masks 5 and 7 (effect gfx D_09001A08). Per part
  `gDPSetTileSize(1, tempX, tempY, (tempX + 0x3F) * 4, (tempY + 0xF) * 4)` (effect_63.c:263): uls is tempX in 10.2 units but lrs is
  (tempX + 63) * 4, so the window width ((lrs - uls) >> 2) + 1 changes with tempX every frame.
- whirlwind tile 0 and tile 1: a 64 x 32 I4 image, wrap, masks 6 and 5 (gsDPLoadTextureTile_4b, gsDPLoadMultiTile_4b). Per segment
  `gDPSetTileSize(0, spB0 + i * 16, 0, (spB0 + i * 16 + 63) << 2, 31 << 2)` (whirlwind.c:283-284): the same mixed units, a window
  wider than the 64 texel period that changes per segment and per frame.
- tex_win0 (nd_tex_set.c:137) and tex_win1 (:473) size the bound texture by the window (only widened up to the period when it is
  narrower, PT09 / VIS3 / PT37). The texture key holds w and h, so every new width is a new texture: a miss and an upload.
- The RDP reads a wrapping axis as (s - sl) & mask from what LoadTile put in TMEM: the texels seen are one period, whatever the window.
  So for a wrapping axis whose load covers the period, the bound size is exactly the period; the window origin stays a coordinate
  (x0, y0) as now. With w and h equal to the periods, tex_win1's WALLT1 branch (:501-507) takes the LoadTile origin as the source.

## Start
`sh 3ds/tools/wt_new.sh TILEWIN2` after the TILEWIN merge. PM_ROW=TILEWIN2 PM_LANE=TILEWIN2 on every build and bench call.

## Exact edits (3ds/src/nd_tex_set.c)
1. tex_win0, after the two existing wrap blocks (the PT37 T block and the PT09 S block, before `int stride = ...`):
   `if (t->tile_wrap[0][2] && !(t->tile_wrap[0][0] & 2) && m->ld_valid) { const int per = 1 << t->tile_wrap[0][2]; const int ldw = t->tile_siz[0] == 0 ? m->ld_w * 2 : m->ld_w; if (per <= 256 && ldw >= per) w = per; }`
   `if (t->tile_wrap[0][3] && !(t->tile_wrap[0][1] & 2) && m->ld_valid) { const int per = 1 << t->tile_wrap[0][3]; if (per <= 256 && m->ld_h >= per) h = per; }`
   `/* TILEWIN2: a wrapping axis reads (s - sl) & mask from TMEM: one period is bound, whatever the window (effect_63, whirlwind) */`
2. tex_win1, after the existing VIS3 block (before `int stride = ...`): the same two lines with index [1] in place of [0].

## Gate (CLAUDE.md 2; Azahar budget 7 of 8)
- Build, memcheck OK; `bench.py run nd_host --row TILEWIN2 --why "wrap period binding"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det,fxcen_c3,fxcen_c5,fxcen_c6
  --row TILEWIN2 --why "byte-equal maps; texmiss of effect_63, whirlwind"`; `bench.py cmp` of both maps: write the byte-equal counts (a
  map texture with a window wider than its period would change: name it from the diff, no pass/fail on pixels).
- `fxcen_report.py` on the three fxcen LOG: copies: texmiss below 0.5 for effect_63 and whirlwind, fire_breath stays below 0.5; write
  their rows and the bulb_glow and firework_rocket rows in the commit. The look is the user's console check.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line, the rows.
