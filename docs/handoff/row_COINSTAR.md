# Row COINSTAR: coins after an enemy dies and the hos_01 Star Piece render garbled (user, 2026-11-10)

(Model: Haiku. START ONLY AFTER HC7 IS MERGED: this row edits 3ds/src/nd_tex_set.c.)

## User facts
"Coins are not rendering correctly after an enemy dies. The Star Piece that is in hos_01 has the same problem." GARBLED, O3DS XL.
Reproducing scenario: coinstar_hos01 (add it, below).

## Cause (oracle, code read, main fe241dc42)
In a map with sprite shading (gSpriteShadingProfile->flags != 0: hos_01 main.c:117, all of kzn; the kzn fire bars drop coins), the item draw
(src/item_entity.c:983-1003 and 1022-1040) loads the CI4 icon with an 8-bit LoadTile at origin 0 and draws tile 1 (combine PM_CC_WINDOW_5 =
TEXEL1) with SetTileSize 0x400,0x400: a coordinate offset of 256 texels. func_801491E4 sets no tile 0 size.
tex_win1 (3ds/src/nd_tex_set.c:453), branch V21 (:487-495, commit 64929e264, 2026-10-24, "Scroll the fire tile from the tile-size window"),
takes the origin as an image offset: src += (ult>>2)*l1stride + (t1x>>1), i.e. 256 rows + 128 bytes past a 12x24 byte raster. Result: garbage.
Tile 0 (tex_win0 :169-174, V13) uses the LoadTile origin. No recent merge (HC4c/d, I8AUX, SHROUD, NEGPAN, TEXKEY2) is the cause.
Premise: the item's tile 1 window source is offset by 3200 bytes (24x24, stride 12) or 4224 bytes (32x32, stride 16).

## Start state
After the HC7 merge: `sh 3ds/tools/wt_new.sh COINSTAR` from the main of that time. PM_ROW=PM_LANE=COINSTAR.

## Touch only
3ds/src/nd_tex_set.c (tex_win1, tex_tile1), azt_scenarios.json, bench_defs.json.

## Step 1: check line (commit it)
In tex_tile1 (:513), after tex_win1(...):
  if (__builtin_expect(PMDL("t1win"), 0)) { static unsigned n_; if (n_ < 8u) { n_++; pm_log("t1win f%u uls %d ult %d w %d h %d ldw %d ldh %d off %d", pm_frame_count(), uls, ult, wn.w, wn.h, nd_tm.ld_w, nd_tm.ld_h, wn.src && nd_ws.timg ? (int)(wn.src - (const u8*)nd_ws.timg) : -1); } }
Scenario coinstar_hos01: "warp=hos_01:0 pos=-300:0:-220 t1win shots shotwin=200:2 loglevel=debug", until "shot \\d+ frame 201\\b", cap 30,
first_marker "warp: hos_01 entry 0", needs_picture true (Star Piece at -350 0 -275, entity.c:7). Bench coinstar_hos01_pic (azahar_pic).
Run 1: expect a line `uls 1024 ult 1024 w 24|32 ... off 3200|4224` and a garbled Star Piece in the shot. If off is 0 or no line prints: hand back with the lines (premise false).

## Step 2: the edit (tex_win1, insert directly before `else if (src && l1v) {` of V21, :487)
    else if (src && l1v && ((uls >> 2) >= m->ld_w * 2 || (ult >> 2) >= m->ld_h)) {
        /* COINSTAR: a window origin outside the loaded 4-bit image is a coordinate offset only (item icons in a shaded map: tile 1 at 256,256
         * over a 24 x 24 load, item_entity.c:990). The source is the LoadTile origin, as tile 0 (V13); x0 keeps uls >> 2. */
        src += (size_t)(l1y * l1stride + l1x);
        stride = l1stride * 2;
    }
(fire_breath keeps V21: its origin imgFrame*32+32 is inside the loaded image.)

## Done criteria
1. Run 2 of coinstar_hos01_pic: t1win shows off 0. The shot shows a clean Star Piece. The Star Piece is the subject; counters are not a pass.
2. Shared draw code: `bench.py base wall_hos01_pic`, `bench.py base sprshade_hos01_pic`, then `bench.py cmp <bench> --row COINSTAR` for both.
   Expected byte-equal, or differences only inside the Star Piece box (report the count and the bbox).
3. nd_host PASS. memcheck OK. Commit.
Affected scenarios: coinstar_hos01, wall_hos01_pic, sprshade_hos01_pic.

## Stop
8 Azahar starts. 2 tries per named cause. No change in the picture after the edit: discard it and hand back with the t1win lines. STOP RULE: 90 min or 150 tool calls.

## Rules
CLAUDE.md rules 1-7. PM_LANE=COINSTAR. No git stash. Foreground waits. The t1win debug line stays.

## Commands
- Build: PM_ROW=COINSTAR PM_LANE=COINSTAR sh 3ds/build.sh
- Test: PM_ROW=COINSTAR PM_LANE=COINSTAR python 3ds/tools/bench.py run coinstar_hos01_pic --row COINSTAR --lane COINSTAR --why "<text>"
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget COINSTAR

## At the end
Delete this row file in the completing commit. Hand back "COINSTAR ready <hash>". Console look on the XL (kzn fire bar coins) by the user.
