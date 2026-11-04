# TRACE QUIZMO AND JR TROOPA (row QJT), 2026-11-03

Branch qjt (worktree papermario-3ds-port-qjt, from native ca4ea0f46). No source change, no debug hook committed.
Azahar starts used: 3 of 6 (QJT runs 1 to 3). Status file: 3ds/logs/perf/status_QJT.txt (untracked).
Binary: copy of the main folder build3ds/pm_3ds.3dsx (same commit ca4ea0f46). Switch: fxtest (src/pm_fxtest.c).
Shots: build3ds/shots/ in the worktree (untracked).

## Item 2: Jr Troopa transform, pink effect. PARTLY PROVEN, edge cause NOT PROVEN
- Trigger: src/battle/area/kmr_part_3/actor/jr_troopa.c:486 to 489. In PHASE_ENEMY_BEGIN, when HP is 1 or less, the script calls PlayEffect(EFFECT_GATHER_ENERGY_PINK) with type 0 and type 1, scale 1, duration 30. The same call is in egg, mage, para and final Jr Troopa files.
- Dispatch: src/evt/fx_api.c:422 to 423, then src/effect_loads.c:93 to 95, then src/effects/gather_energy_pink.c.
- Draw (gather_energy_pink.c, appendGfx): setup list D_E00269F8[type], geometry list D_E00269F0[type]. Type 0 uses setup D_09001138 (I4 64x64 texture D_09000400, wrap, 2 cycle, RM_CLD_SURF2) and geometry D_090010B0 (26 vertices, 16 triangles, source assets/us/effects/gfx/D_090010B0_33CC80.gfx.inc.c).
- Alpha: vertex alpha is 0 at the base ring (z 0) and 255 at the far ring (z -2000). Source: assets/us/effects/gfx/D_09000F10_33CAE0.vtx.inc.c. Alpha ramp in update: unk_24 rises over 10 frames and falls over the last 6.
- Texture coordinates: s runs to 8192 (1/32 texel units, 256 texels), 4 wraps round the ring at a 64 texel mask. Wrap is G_TX_WRAP, so a seam appears only if the 64x64 texture is not tileable at its edge (not checked).
- Scroll: tile 0 origin is unk_40 mod 256 (quarter texel). Tile 1 origin unk_38 grows by 96 per frame for type 1 (no wrap in update). The port reads SetTileSize uls and lrs as 12 bit fields: src/pm_gbi.c:3069 to 3071. Tex window clamp for negative starts: src/pm_gbi.c:2873 to 2885.
- Shot (run 2, fxtest=14 type 0, mac_01, Mario at -264 20 -326, spawn frame 402, shot a frame 412): build3ds/shots/0013_1_2.ppm. By eye: a ring of hard-edged magenta blocks with radial streaks. Zoom: scratchpad qjt_r2_f412_zoom.png. No smooth gradient read at the ring edge.
- Not proven: the 16 triangle draw of D_090010B0 is not in the frame 412 drawlog (chunks 69 to 183 only, no tri 16 line). No texel dump. Type 1 (the tile 1 growth path) was NOT run: a new switch entry needs a binary build, not done in this row.
- Run 1 (kmr_20, fxtest=14): no effect visible. Mario was not settled (spawn y -975). Shot shows yard only.

## Item 1: Chuck Quizmo stretched geometry. NOT REACHED (Quizmo scene not run)
- Quizmo effect run (run 3, fxtest=72 quizmo_stage at mac_01, same position, shot a frame 412): picture shows the purple wall and a flat dark block in the lower half. No stretched shape read by eye.
- A stretched triangle is in the frame 412 drawlog of run 2 and run 3: chunk tri 2, screen box -223 207 to 3740 8291, clip w range 5.29 to 291.43, texture timg 0x0100141c (16x16, fmt 2). The w value 5.29 is near the near plane, so the perspective divide blows up the box. It is a mac_01 world draw, not a Quizmo draw. Source file not mapped (display list address 012dffb0 in the drawlog).
- Not reached: the Quizmo NPC scenes (dro_01, dro_02, end_00, hos_03, mac_02) and the Quizmo show. The quiz start needs a hook or the NPC talk path. Lead from agent_grab3.md: stage light D_09001518_39A0D8 (SetTileSize 252 by 60, T clamp). Not seen in the mac_01 shot.

## Fix proposal (not applied, no code change in this row)
- Item 2: run fxtest for type 1 with a debug entry appended after gather_energy_pink in src/pm_fxtest.c:46 (uncommitted), drawlog on frame 412, and dump the tri 16 chunk. If the edge is the tile 1 origin (unk_38 grows without bound), the fix is a mod 256 on the origin in gather_energy_pink.c update (one line, src/effects/gather_energy_pink.c, near the unk_38 update). Test: type 1 shot on mac_01, picture of the ring edge.
- Item 1: find the source of the chunk at drawlog frame 412 (mac_01, dl 012dffb0). A near plane clip for tri 2 draws (pm_gbi.c or pm_gpu.c vertex path) is the candidate. Not proven.

## Budget
- QJT starts used: 3 of 6. Remaining: 3 (not used).
