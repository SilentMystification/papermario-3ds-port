# Row POND: Toad Town pond shows flat blue, the water texture is missing

## User facts (bug)
Trigger: walk to the Toad Town pond (mac_00, Mario near x -174 z 112). First wrong frame: every frame at the pond. Seen on the console (O3DS XL, build 5c84d16e7). Expected look: the water texture drawn on top of the pond. Now: flat blue (the blocker colour). Before the B2 fix (audit, nd_prim_clip_z, merged 899127979) the hedge behind showed through: the user calls the B2 change an improvement, keep it. Reproducing scenarios: toad_pond (azt_scenarios.json:4484), pond_mac00_wlog (the B2 shot, frame 204).

## Goal
The water layer(s) of the pond draw over the prim-depth blocker as on N64. Investigate by code first (CLAUDE.md rule 5 INVESTIGATE BY CODE): name the pond's draw order (blocker, water surface, any scrolling texture layers), each draw's render mode (Z_CMP/Z_UPD, G_ZS_PRIM or pixel depth, blend, decal), and the depth value each one gets on the PICA. The likely cause: a later water layer fails the depth test or is culled against the blocker depth that B2 now writes or tests, or the water layer itself is a G_ZS_PRIM draw whose prim depth goes through another path (rect vs tri) or a decal mode (ZMODE_DEC) with no PICA polygon offset. Name the file, line and value before any edit.

## Start state
Worktree build3ds/wt/pond, branch pond from nds1 (current). Copy assets/us and ver/us/build into it. audit_system.md section B (B2) and its result line 156 give the depth formulas.

## Touch only
The native draw path that the trace names (nd_model.c nd_prim_clip_z and callers, nd_rect.c prim depth, pm_gpu.c depth/zmix state, the decal path). No change to the B2 formula unless the trace proves it wrong.

## Done criteria
1. Code trace written into this row's handback: each pond draw, its mode and PICA depth, and the failing one.
2. Build; nd_host PASS.
3. Picture gate: pond_mac00_wlog (or toad_pond) shot shows the water texture over the pond, not flat blue and not the hedge.
4. spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015 byte-equal to the base, unless the trace shows the changed path is in them (then name the pixels).
Affected scenarios (no others): pond_mac00_wlog, toad_pond, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=pond PM_ROW=pond on every build.sh and bench call. No git stash. No toggles, no stubs. 2 tries per named cause.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=pond PM_LANE=pond sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=pond PM_LANE=pond python 3ds/tools/bench.py run <bench> --row pond --lane pond --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "pond ready <hash>".
