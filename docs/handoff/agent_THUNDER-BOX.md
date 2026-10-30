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

## Run 8 (budget reset, lane thunder, 1 start used)
- Tried: D_09000FF8_394098 line 36 (group 2, `G_RM_AA_ZB_XLU_SURF`) alpha combine changed from `0,0,0,PRIMITIVE` to `TEXEL0,0,PRIMITIVE,0`. Built and shot.
- Result: the yellow rectangle is still at frames 256 and 260 (top left of the top Shy Guy). The change is not the cause. Edit reverted by hand (the asset file is not tracked by git, so git checkout does not work). Not committed.
- Texture check: D_09000000_3930A0 is a bolt, zero outside the bolt. Not the source of a full rectangle.
- Mesh UVs (D_09000CF8_393D98, D_090009F8_393A98) reach about 60 texels (1916/32). Texture load uses MIRROR|WRAP, mask 5 (D_09000950_3939F0.gfx.inc.c:15). Mirror path: pm_tex.c:261-270, 318-371, 1473.
- Next: find the draw that writes the rectangle. Candidates not yet tested: the mirror wrap state (pm_tex.c:1473, `shr_s` path), or a non-mesh draw in the same frame (lightning world or FadeBackgroundDarken).

## Run 9 (2 starts left after this run, lane thunder)
- Confirmed: D_09000950_3939F0 (src/effects/flashing_box_shockwave.c:183-184) has no triangles. It only sets state. It draws no pixels.
- The battle branch (src/effects/flashing_box_shockwave.c:176-178) never sets the primitive colour. Only the world type sets 255,255,0. So battle PRIM is stale.
- Tried: skip D_09000950_3939F0 for the battle type. Built and shot at frame 260. The rectangle stays at the top left of the top Shy Guy, and new yellow blocks appear at the top and bottom of the frame. Result: not the cause.
- Edit reverted in src/effects/flashing_box_shockwave.c. Not committed.

## Run 10 (Test A, 1 start, budget 1 left)
- Edit: skip D_09000FF8_394098 for the battle type (src/effects/flashing_box_shockwave.c, after the quad line). Kept the quad line.
- Shot at frame 260 (0002_0_0.ppm): the yellow rectangle is still at the top left, next to the top Shy Guy. Result: the shockwave mesh is not the writer.
- Source edit reverted. Not committed.
- Next: Test B (investigate_thunder_box_3.md): skip the bolt triangles gSPDisplayList(dlist) at src/effects/lightning.c:279 for unk_00 >= 3. One start left.

## Run 11 (Test B2, last start, budget 0 left)
- Edit: skip the bolt triangle draw gSPDisplayList(dlist) for unk_00 < 3 is false, that is for unk_00 >= 3 (src/effects/lightning.c:279 region).
- Shot at frame 260 (0002_0_0.ppm): the bolt is gone. The pale yellow panel is still at the top left. Result: the bolt triangles are not the writer.
- Source edit reverted. Not committed.
- Next: Test B1 is not run (no start left). The writer is still open. Candidates: the bolt tile 1 seam (lightning.c:278), the reflected bolt copy (lightning.c:213-218), squirt.c, and the fill path (3ds/src/pm_gbi.c:3774-3807).

## Run 12 (sparkles skip test, lane thunder, 1 start, budget 2 left)
- Dump (commit 04d6a1acd, frames 259-260): frame 260 has 1258 GBI ops in build3ds/azt/thunder_box_gbidump.txt. No fill rect is pale yellow. Full-screen fill fffc (alpha 0), black borders 0001. The pale yellow PRIM (213,254,180 cycling to 254,172,213) sets a sparkle draw in src/effects/sparkles.c:280-282 (table D_E0022CF0).
- Edit: skip gSPDisplayList(D_E0022CD0[unk_2C & 7]) at src/effects/sparkles.c:282 in battle mode (get_game_mode() != GAME_MODE_BATTLE). Nothing else changed.
- Shot at frame 260 (0002_0_0.ppm), viewed: Shy Guys and lightning are shown. The hard-edged pale yellow rectangle at the top left of the top Shy Guy is not visible. A yellow hit burst remains there. No before-shot was kept for a pixel compare.
- Source edit reverted (git checkout of src/effects/sparkles.c).
- Next: confirm with a second look at the earlier shot set if needed, or test the next candidate (tile 1 span, lightning.c:278).

## Run 13 (temp sparkle log, last start, budget 0 left)
- Log at pm_gbi.c gv_params_calc (a255 gate, line 1279) and at the SETCOMBINE alpha stage (comb_alpha_shade, line 3391), frame 260. Log: build3ds/azt/thunder_box_sparkle_log.txt (40 lines).
- Palette prims (d5feb4ff, fefeb4ff, fed5acff, feacacff, feacd5ff) never reached gv_params_calc. No palette-prim line was logged. The sparkle draw takes another path (the CPU draw path near pm_gbi.c:1976), which was not logged.
- SETCOMBINE lines at frame 260 show alpha_shade 1 for most combines, and 0 for two combines (log lines 221, 223). Plan_active 1 for one draw (log line 228, prim ffffffff).
- Log edit reverted (git checkout of pm_gbi.c, the gbidump commit 04d6a1acd stays).
- Next: log the CPU draw path (pm_gbi.c near 1976) for prim d5feb4ff, frame 260: plan_active, a255, and the alpha stage. No starts left.

## Run 14 (temp CPU sparkle log, budget reset, 1 start)
- Log at pm_gbi.c, after the vertex alpha block (near line 1976), frame 260, prim d5feb4ff.
- Result: 0 log lines. No draw at frame 260 with prim d5feb4ff reaches this function, and none reached gv_params_calc (run 13). The palette prim is set (dump #1083), but the triangles that follow it take another path.
- Next: find the draw path that consumes the G_TRI2 after the palette prim (the streak or batch path, pm_gbi.c near 3583 and gv_streak_op), and log there for prim d5feb4ff.
- Log edit reverted (git checkout of pm_gbi.c).

## Run 15 (temp triangle log in draw_tri, frame 260, prim d5feb4ff)
- Log: build3ds/azt/thunder_box_sparkle_tri_log.txt (2 lines, the two triangles of the sprite quad at log #1109 and #1110).
- Vertex colour alpha (cn[3]) = 0 for all three vertices. Lighting is off (geom 00200005 has no G_LIGHTING bit), so the vertex alpha is not used by the colour path.
- Blend: omL 00553048, omH 00082cf0, 1-cycle (omH cycle bits 0). tex_on 1.
- Texture coords read 0,0 for all vertices. The verts[] array is not filled on the ring path (pm_gpu_gvtx_load), so the tc values are not valid. Texel coverage is not logged. Use the asset coordinates instead: sprite tc s 2112..2816 and t 0..704 (D_09001088_339048), which is texels x 66..88, y 0..22.
- Log edit reverted (git checkout of pm_gbi.c). Commit adds only the handoff note.

## Run 16 (box source log, lane thunder, 1 start)
- Temp log at src/effects/flashing_box_shockwave.c appendGfx (after the local declarations): type, pm_frame_count, lifetime time, primCol, unk_18. Reverted by git checkout after the shot. Not committed.
- Shots: 0000 (frame 252), 0001 (frame 256), 0002 (frame 260) in build3ds/azt/shots/thunder_box/. All three are even frames. No odd frame was shot.
- Shot-window fact: the shot frames are the only frames that render in this window. The log has box lines only at 252, 256, 260 (until 263), and every frame from 264 on. Shot frames are 4 apart.
- Log: type 3 (battle lightning) draws every frame from 242. Type 0 (shock hit) draws at 252, 256, 260, then every frame from 264. PRIM is always 255 255 0. alpha 255 in most lines.
- Type 0 lifetime at the shot frames is 1, 5, 9 (always odd). The time % 2 gate at line 162 passes on every shot frame here, so the gate does not explain an odd or even difference in this run.
- Picture: frame 256 and frame 260 show a pale yellow hard-edged block at the top Shy Guy. Frame 252 is not clear.
- Box source by elimination: type 3 mesh skips (runs 9 and 10) did not remove the block. Type 0 draw lists D_09000600_3936A0 and D_090008E8_393988 are not yet skip-tested. Next: skip type 0 at flashing_box_shockwave.c:162 to 169 in battle, one start, frame 260.
- Budget: 1 start used in this run.

## Run 17 (type 0 skip test, lane thunder, 1 start, budget 1 left)
- Edit: skip the type 0 draw (D_09000600_3936A0 and D_090008E8_393988) in battle. Condition at src/effects/flashing_box_shockwave.c:162 now requires `get_game_mode() != 8` for type 0. Type 1 gate unchanged.
- Shot at frame 260 (build3ds/azt/shots/thunder_box/0002_0_0.ppm), viewed. Shy Guys and lightning are shown.
- Yellow box: not seen. No hard-edged yellow rectangle at the top Shy Guy in a 4x crop. No before-shot was kept, so this is not a pixel compare.
- Hit burst: still present. Yellow star burst at the top Shy Guy and a yellow glow at the centre.
- Source edit reverted (git checkout of flashing_box_shockwave.c). Map table restored from backup. Not committed.
- Next: a before-shot at frame 260 on the same binary (unedited) to confirm the box is gone. Burst source not yet found.
