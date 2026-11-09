# Row ROOMTEX: texture data loaded per scene on the bottom-screen HUD (user, 2026-11-09)

## User facts
User: "I want a breakdown of how much data is loaded per room / scene. Each time we enter a room or change a scene (a battle, or a
transition to somewhere else) we should see what chunk of the texture cache was just filled with new textures, and what percentage
of that room's loaded assets were pulled from RAM vs already in the VRAM texture cache." Tab the new line in so its numbers line
up with the VRAM row above it; align the MEM line too if possible.

## Goal
A third bottom-screen memory row, for example:
  MEM  used 12.3/ 64.0 MB free  51.7 MB
  VRAM used 5000/6144K  tex 3000/3000K
  ROOM new  1234K (78%) kept 340K (22%)
new = KB uploaded from RAM into the texture cache since the scene start (misses). kept = KB of textures used in this scene that were
already in VRAM (hits, counted once per slot per scene, by slot bytes). Percentages by bytes. The first number of each row starts at
the same column (fixed-width fields, %4u style). Exact labels are free if the columns line up; each row at most 39 characters.

## Start state
`sh 3ds/tools/wt_new.sh ROOMTEX`, branch ROOMTEX. PM_ROW=ROOMTEX, PM_LANE=ROOMTEX. One Haiku agent.

## Facts (code read by the orchestrator)
- HUD rows: pm_mem_hud(char out[2][40]) in 3ds/src/pm_mem.c:228-247; caller pm_main.c:447-448, :672 (rows drawn there).
- Scene reset today: pm_mem_scene_reset (pm_mem.c:225), called from pm_mem_log("map") (pm_mem.c:250).
- Texture counters: pm_tex.c:105 (tot_hit, tot_miss, tot_up ...), pm_tex_map_delta (pm_tex.c:1016), the per-map "texmap" log
  (pm_mem.c:252-255).

## Work
1. Scene counters in pm_tex.c: new_bytes and kept_bytes since the scene start. "kept once per slot per scene": a per-slot scene
   stamp (u16 beside the slot) compared to a scene number; no per-frame loop over slots. Integer adds only on the hit/miss paths.
2. Scene start = map load (exists) plus battle start, battle end and any other game-mode change that loads a new scene (find the
   one place where the game mode changes; name file:line). At each scene start, log one line with the final numbers of the scene
   that ends: "roomtex <scene> new %uK kept %uK new %u%%".
3. HUD: out becomes [3][40]; the three rows aligned as above; caller in pm_main.c draws three rows.

## Touch only
3ds/src/pm_tex.c (counters), pm_mem.c (HUD rows, reset), pm_main.c (row count, the draw), the one game-mode change site.

## Done criteria
1. nd_host PASS. Build OK. memcheck OK.
2. One Azahar run (debug HUD on, cap 30 s): a map load and a battle start (an existing battle scenario with the HUD switch). Shots:
   the three rows aligned after the map load and after the battle start; the log has two roomtex lines.
3. Commit; delete this row file in the completing commit. Budget: 3 Azahar starts.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
