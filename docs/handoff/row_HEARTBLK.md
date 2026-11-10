# Row HEARTBLK: lag while HP/FP count up after a Heart Block (user, 2026-11-09)
## Paths that can cause the lag (code, main 7f7eaa26f), ranked
1. The status bar is up for the whole count-up. src/inventory.c:609-628 sets showTimer 70 while displayHP/FP != cur. The draw at :753-860 is HP/FP/coin/star icons plus status_bar_draw_number, one hud rect per digit (:470-560). Console fact (row HC7, XL): about 20 extra text rects cost about 5 ms and 15k I-misses per frame. This is the largest named cost.
2. Two sounds every 4 frames: SOUND_HEART_PICKUP and SOUND_FLOWER_PICKUP (inventory.c:650 and :669), 15 sound starts a second on the logic core.
3. Heart Block state 3: fx_sparkles(FX_SPARKLES_3) every 2nd frame (src/entity/HeartBlock.c:238). The heart model is drawn 3 times with alpha, each with its own combine and prim (HeartBlock.c:28-72).
## Fix rows
- Path 1 is the HC7 rect path (row HC7 step 2 below, then step 7). No separate row.
- Next oracle step after HC7 merges: read pm_tex.c glyph_resolve/cell_load. Goal: put the HUD digit rasters into the glyph atlas cells, so the digits of one number share one texture and merge into one rect batch. That fix row comes after that read.
- Paths 2 and 3: no code fix without the console prof line of path 1 first. One XL prof push after HC7 merges (scene kmr_03, hp=1, a jump under the block at 130 60 0). This is a measurement of the fix, not a repro.
