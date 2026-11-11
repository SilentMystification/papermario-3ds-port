# Row TILEWIN: a scrolling 4-bit tile 1 window loads a new texture every frame (effect_63, fire_breath, whirlwind)

(Model: Haiku. Base: main after FXCEN3. Edits 3ds/src/nd_tex_set.c tex_win1 only: disjoint from R13NOP (nd_rec.c), PMUPH (pm_phase.c),
FXCEN3 (tool). Reads this file and CLAUDE.md.)

## User facts (console fxtest, O3DS, cost per frame above the 32.8 ms base)
effect_63 +30.5 ms (texmiss 6.4 per frame), fire_breath +17.7 ms (7 draws only), whirlwind +15.7 ms (texmiss 6.8), firework_rocket +5.0 ms.

## Cause (oracle code read)
- effect_63 (src/effects/effect_63.c:263) moves tile 1 per part with `gDPSetTileSize(1, tempX, tempY, ...)`; fire_breath scrolls tile 1
  by imgFrame*32+32 (named in nd_tex_set.c V21 comment); whirlwind sets tile 0 and tile 1 sizes per ring segment (whirlwind.c:283-284).
- tex_win1 (nd_tex_set.c:473-519): a non-4-bit tile 1 and a whole-period window (WALLT1, :501-507) keep the source at the LoadTile origin
  (SetTileSize only moves the coordinates, td[1].x0/y0). The 4-bit V21 branch (:514-522) moves the source by the window origin
  (ult rows, uls/2 bytes) whenever the window is smaller than the mask period: every scroll step is another source pointer, so another
  texture key: a pm_tex miss, a 4-bit decode and an upload per part per frame (texmiss 6-7, flkb up to 133 KB per frame).
- On the RDP a wrapped tile reads (s - sl) & mask from what LoadTile put in TMEM, so binding the whole mask period at the LoadTile origin
  and moving only the coordinates (as WALLT1 does) is the N64 rule and gives one texture for all scroll steps.

## Start
`sh 3ds/tools/wt_new.sh TILEWIN`. PM_ROW=TILEWIN PM_LANE=TILEWIN on every build and bench call.

## Exact edit (3ds/src/nd_tex_set.c tex_win1)
Directly before the branch `else if (src && l1v) { /* V21: ...` add:
`else if (src && l1v && t->tile_wrap[1][2] && t->tile_wrap[1][3] && (1 << t->tile_wrap[1][2]) <= m->ld_w * 2 && (1 << t->tile_wrap[1][3]) <= m->ld_h) {`
`    /* TILEWIN: a window inside one wrap period of a 4-bit image loaded whole (effect_63, fire_breath, whirlwind tile 1 scroll): bind the`
`     * whole period at the LoadTile origin; SetTileSize only moves the coordinates (x0 = uls >> 2, td[1].y0 = ult >> 2), as WALLT1.`
`     * Before, the source moved with the window: a new texture for every scroll step. */`
`    w = 1 << t->tile_wrap[1][2]; h = 1 << t->tile_wrap[1][3];`
`    src += (size_t)(l1y * l1stride + l1x); stride = l1stride * 2;`
`}`
No other line. t1x stays uls >> 2 for this branch.

## Gate (CLAUDE.md 2; Azahar budget 5 of 8)
- Build, memcheck OK; `bench.py run nd_host --row TILEWIN --why "tex_win1 wrap window"`: PASS.
- `bench.py base spot_mac01_det`; one list `bench.py run spot_mac01_det,fxcen_c3,fxcen_c5,fxcen_c6 --row TILEWIN --why "byte-equal;
  texmiss of fire_breath, effect_63, whirlwind"`; `bench.py cmp spot_mac01_det --row TILEWIN`: 6/6 byte-equal.
- `python 3ds/tools/fxcen_report.py <the three fxcen LOG: copies> --label tilewin`: the texmiss column of fire_breath, effect_63 and
  whirlwind is below 0.5 (the FXCEN2 Azahar table had 4.8 and 6.8 for effect_63 and whirlwind). Write the three rows in the commit.
  The look of these effects can change at the window edges: the user checks fire_breath and effect_63 on the console.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line and the three texmiss rows.
