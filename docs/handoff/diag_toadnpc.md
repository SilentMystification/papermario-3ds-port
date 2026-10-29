# TOADNPC: Toad Town cost against walk maps

Row TOADNPC (diagnostic). No source edits. Azahar starts: 2 of 3 (mac01, yard). Build 29f504d7b, map file build3ds/lane-final/pm_3ds.map.
## 1. Answer
- Toad Town (mac_01) runs at 60.6 ms per frame (16.5 fps). Walk maps run at 33.7-35.3 ms (28.3-29.7 fps).
- NPC draws are not a top cause. NPC sprite ops are 5.7 % of mac_01 ops.
- Top causes of the extra 25.6 ms: model draws (+12.7 ms gbi), texture closes (+5.3 ms, inside gbi), GPU draw side (+5.6 ms). A further +5.5 ms in "other" has no counter.
## 2. Per map, console (wall and gbi: phase windows, weighted by frames)
| map (FTP lines) | frames | wall ms | fps | gbi ms | ops/frame | draws/frame | tex closes (non-empty) | tex flush us | NPC spawn |
|---|---|---|---|---|---|---|---|---|---|
| A mac_01 (16308-34716) | 1143 | 60.6 | 16.5 | 31.5 | 2156 | 86.8 | 289 (56) | 7464 | 23 |
| B area1 map3 (34717-43053) | 981 | 35.0 | 28.6 | 18.8 | 1395 | 37.1 | 192 (19) | 2203 | 9 |
| C area0 map9 (43054-45129) | 300 | 33.7 | 29.7 | 16.4 | 1116 | 34.9 | 126 (21) | 2702 | 2 |
| D area0 map10 (45130-53296) | 1198 | 35.3 | 28.3 | 13.5 | 703 | 53.2 | 65 (27) | 3046 | 1 |
| E maps 6,5,6 (53297-59537) | 877 | 34.6 | 28.9 | 15.4 | 848 | 49.9 | 75 (29) | 3033 | 0, 2, 0 |
Ops: taskpl per frame. Draws: gpusplit per frame. Closes: flsplit (non-empty = closes minus empty). Scene A includes menu periods (FTP 22489-33092), not split.
## 3. Producers (ops per frame, opsplit windows, 2-4 per map)
| producer (key) | source | A | B | C | D | E |
|---|---|---|---|---|---|---|
| appendGfx_model 0x161328 | src/model.c:1384 | 1899 | 866 | 582 | 121 | 86 |
| appendGfx_model_group 0x164d8c | src/model.c | 44 | 8 | 24 | 116 | 164 |
| appendGfx_entity_model 0x212878 | src/entity_model.c | - | 389 | 324 | 95 | 197 |
| sprite_component (NPC, sprite draws) | src/sprite.c:443 | 114 | 161 | 130 | 41 | 10 |
| appendGfx_npc 0x1d0178 (own ops) | src/npc.c:835 | 8 | - | - | - | - |
| walking_dust 0x1c6f00 | src/effects/walking_dust.c | - | 12 | 33 | 14 | 24 |
| effect_renderUI | src/effects.c:165 | - | - | - | 177 | - |
| WorkerBackUI (HUD) | hud | - | 22 | 93 | 31 | 111 |
| total (prod line) | | 2145 | 1573 | 1270 | 752 | 895 |
"-" means not in the top 8 of a window. B total 1573 is 13 % above taskpl 1395. Not resolved.
## 4. NPC share and counts, mac_01
- sprite_component 113.7 ops per frame (5.3 %). appendGfx_npc own 8.3 ops (0.4 %). NPC share 5.7 %.
- Sampled us: 2101 + 114 = 2215 us per frame. Producer sum 39.9 ms against gbi 31.5 ms. Scaled NPC cost: 1.8 ms (5.6 %).
- spawn npcs 23 (FTP line 16486). Spawn counts do not count visible NPCs.
- sprite_component is 114 ops in A and 161 in B. A has 23 spawned NPCs against 9 in B, and fewer sprite ops.
- No counter: visible NPCs per frame, sprite draws per frame, entities per frame. The sprite key covers NPC, player and battle sprites.
## 5. Azahar cross-check (lane mops)
- mac01 (f299, one window): ops 1165, binds 78, model 755, sprite_component 247 (21 %), spawn npcs 5, story -128. Console story at load: -107. NPC sets differ.
- yard: spawn npcs 3. The scenario does not turn phase on, so no ops data.
- Shots written: build3ds/azt/shots/mac01/0000_1_2.ppm, build3ds/azt/shots/yard/0000_0_11.ppm. Not opened by this agent.
## 6. Top 3 causes (mac_01 minus area1 map3, wall +25.6 ms)
1. Model draws. appendGfx_model 1899 ops against 866 (+1033). Binds 88 against 67. Ops per bind 24.6 against 20.8. gbi +12.7 ms.
2. Texture closes. Flush 7.5 ms against 2.2 ms per frame (+5.3 ms). Inside gbi, so it overlaps cause 1.
3. GPU draw side. Draws 86.8 against 37.1 per frame. fbegin +2.2 ms, gpu +2.0 ms, gdraw +1.4 ms.
NPC sprite ops are not in the top 3.
## 7. Next step
- NPC is not a cause. No NPC plan row.
- Plan row TOADMDL1 (not started): key the appendGfx_model producer by model ID (src/model.c:1384, phase only). Count draws per model in mac_01. One Azahar start (azt mac01). Name the top model IDs before any edit.
- Add counters before the next diagnosis: sprite draws per frame (src/sprite.c spr_draw_component), visible NPCs per frame (src/npc.c appendGfx_npc), entities per frame (src/entity.c:392).
