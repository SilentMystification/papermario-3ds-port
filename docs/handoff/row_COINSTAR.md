# Row COINSTAR: coins after an enemy dies and the hos_01 Star Piece render wrong (user, 2026-11-10)

## User facts
"Coins are not rendering correctly after an enemy dies. The Star Piece that is in hos_01 has the same problem."
Look not described yet: ask the user (garbled, missing, wrong colour, flat) before a run.

## Leads (orchestrator)
Both are item entities (item_entity.c, ITEM_COIN / ITEM_STAR_PIECE) drawn as sprites with animated frames (coin spin, star piece
shine) through the item entity draw (CI4/CI8 rasters with palettes, set per frame). Recent changes in that area: I8AUX (unit-1 I8
alpha, pm_tex.c pm_tex_i8), SHROUD (sprite mix from the own othermode, pm_gpu_spr_build), NEGPAN (negative tile start),
TEXKEY2 (palette key colours patched at ROM read, map textures only), HC4c/HC4d typed ops (texture and control ops).
## Step 1 (queue, one agent at a time; Opus weekly limit hit 2026-11-10): code read of the item entity draw path after HC4d, then
one Azahar run of a coin drop (battle win) and hos_01 with shots; bisect the recent merges by code if a shot shows it.
