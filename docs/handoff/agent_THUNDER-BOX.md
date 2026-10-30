# Handoff: THUNDER-BOX (agent state)

## State
- Goal: stop the box around the Shy Guy when Thunder Rage lightning hits (attract-loop battle before file select). The flicker must follow the Shy Guy texture outline.
- No draw fix made yet. No source edit to the draw path. The box is not yet identified in any shot.
- Scenario `thunder_box` (3ds/tools/azt_scenarios.json): attract demo battle 12, `demoscene=12`, with `warp=kmr_03:0 story=ch0_toad_town shots shotmode=8:200 loglevel=debug`. Until: shot frame 250. Status unverified.
- Lane build: `thunder` (build3ds/lane-thunder/pm_3ds.3dsx). It includes the debug log line below.
- Budget: THUNDER-BOX used 1 of 3 after the last reset. Run 5 was the last start.

## Commits
- 7c0c0afb0: scenario thunder_box and catalog row.
- 0d33a999b: retarget to demo battle 12.
- 5d7c1562b: catalog row command fix.
- 57cc9feff: `debug lightning hit` log line in 3ds/src/effect_loads.c (fx_lightning, arg0 >= 3, GAME_MODE_BATTLE). Diagnostic only.
- 871d8d0dc: until moved to shot frame 250.

## Log markers verified (run 4 and 5, lane thunder)
- `debug demo: enter GAME_MODE_DEMO at demo scene 12` at scene frame 100 (line #123).
- `game mode 17 frame 153`, then `game mode 8 frame 153`: battle mode starts at frame 153.
- `debug lightning hit: arg0 4 pos 5 2 -20 scale 35 22` (line #227). The four hits follow (args 3, 5, 5 in run 4). Hits fall at about frame 241. The log hit line has no frame number.
- `shot 4 frame N map 0:0`: shot lines are one per frame from frame 153. The shot at frame 250 exists (run 5).

## Open items
1. Run 5 shot at frame 250 (0024_6_5.ppm) shows a curtain stage. No Shy Guys and no lightning are visible. The subject is not confirmed.
2. The Shy Guys were not located on screen. Check the battle camera or the battle start in frames 241 to 270 from the shot set in build3ds/azt/shots/thunder_box/ before another start.
3. The earlier shot at frame 180 (run 4) shows a field view with Mario. This is before the hits.
4. Shy Guy formation: BTL_DIG_FORMATION_03 (demo battle index 3, DemoScenes index 12). Verify it is four red Shy Guys (the user says so).
5. The box mechanism is not identified. The earlier diagnosis (investigate_thunder_box.md) named the quad D_09000950_3B5C90; that is wrong. The battle draw is the mesh list D_09000FF8_394098 (see docs/handoff/investigate_shockwave_callers.md). The mesh outline is not read yet.

## Commands
- Build lane thunder: `python 3ds/tools/gen_map_restore.py`, then `PM_ROW=THUNDER-BOX PM_LANE=thunder sh 3ds/build.sh`. Copy `3ds/src/map_restore_table.c` before the build and restore it after (never commit it).
- Run the scenario: `PM_ROW=THUNDER-BOX powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 thunder_box -Row THUNDER-BOX -Build thunder -Try`
- Self-test (no start): `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 -SelfTest`
- Shots: build3ds/azt/shots/thunder_box/ (PPM 240 x 400).
- Azahar log: AppData Roaming Azahar sdmc 3ds PaperMario log.txt.

## Run 6 (2026-10-29, lane thunder)
- Scenario now: `shots shotwin=241:10` (was shotmode=8:200). Until and check: shot frame 249.
- shotwin writes a shot every 4 frames: frames 241, 245, 249 (not every frame).
- Frame 249 (build3ds/azt/shots/thunder_box/0002_0_0.ppm) shows four red Shy Guys and the Thunder Rage lightning on them. Subject shown.
- Frame 249 shows no visible box around the Shy Guys. The box is not seen yet.
- Budget: 2 of 3 starts used. One start left.
- Next: one start with a window on the second hit (frame 256) and later, e.g. `shotwin=252:12`, until shot frame 260. Look for the box.

## Run 7 (lane thunder, budget 3 of 3 used)
- Scenario: `shotwin=252:12`, until and check on shot frame 260. Shots at 252, 256, 260.
- Frame 256 and frame 260 show the four red Shy Guys and the lightning.
- Box: frame 260 (0002_0_0.ppm) shows a hard-edged bright yellow rectangle behind the top Shy Guy. Frame 256 (0001_0_0.ppm) shows a similar rectangle at the top left.
- Next: no start left. Read the box source with a code agent (quad or mesh list, see investigate_shockwave_callers.md).
