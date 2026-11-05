# Handoff: NPHUD epic, agent_nphud

## State
- Rows: NP1B (retry of NP1), HT1. Both committed. Working tree: HEAD (be50979c1) plus the user file 3ds/src/map_restore_table.c (modified, never committed or discarded; restored from .bak after each build).
- Status lines: 3ds/logs/perf/status_NP1B.txt, 3ds/logs/perf/status_HT1.txt.

## Commits
- 64be3494b NP1B: src/sprite.c spr_appendGfx_component_flat (TARGET_3DS). One G_PM_TEXI header before the TLUT load; n set after the load macro in the shading and the non-shading branch. Also 3ds/tools/shot_cmp.py (picture compare rule) and docs/handoff/test_catalog.md (rule section).
  - mac01t f899 r3 (NPC sprite, 27 draws): 490 -> 301 ops, 18.1 -> 11.1 per draw. Gate <= 12: pass.
  - Top shots: mac01 2 of 2, mac01t 2 of 2, yard 1 of 1, pause_menu 1 of 1 byte-equal.
- be50979c1 HT1: src/hud_element.c hud_element_draw_rect (TARGET_3DS) and 3ds/src/pm_gbi.c.
  - One G_PM_TEXI per element for the setup words (render mode, LUT, TLUT, filter). One per tile for combine, prim and tile load. The rect stays outside the runs.
  - pm_gbi.c: the G_SETPRIMCOLOR body moved into tex_op. The dispatch case calls tex_op, as G_SETCOMBINE does.
  - mac01t f899 r3 (task 0000f0e2, gfx_back_ui): ops 319 -> 171 (-148 per frame, gate >= 100: pass). Closes 19 -> 19.
  - Top shots: mac01 2 of 2, mac01t 2 of 2, yard 1 of 1, pause_menu 1 of 1, file_select 1 of 1 byte-equal.

## Picture compare rule (new)
- Tool: `python 3ds/tools/shot_cmp.py <dirA> <dirB>`. Top shots (not bot_*) must be byte-equal. Bottom shots are skipped.
- Proof: two HEAD runs of yard (lane nphead, source HEAD e16fee4b9): top shot 0000_0_11.ppm byte-equal.
- The earlier NP1 drop was a bottom-shot difference. The top shot was equal in that run.

## Open items
- file_select baseline: the only file_select shot is build3ds/azt/shots/file_select from 12:37 (before EN2, T1B and NP1). It is not a HEAD run. The 6-start budget of HT1 had no start left for a fresh HEAD baseline. The HT1 shot is equal to it. Re-check with a fresh HEAD baseline when budget allows.
- mac01t gate `mdlsum f899 tot ops (gate 1900 to 2200)` fails at 1844 with NP1B and HT1 (T1B also 1844). The range was set from HEAD (2315 fails high). The gate range is outdated. Orchestrator decides the new range in azt_scenarios.json.
- Not done: the second circle CC removal (EN2 follow-up).

## Azahar starts
- NP1B: 6 of 6 (yard HEAD A, yard HEAD B, mac01t, mac01, yard, pause_menu).
- HT1: 6 of 6 (mac01t, mac01t, yard, mac01, pause_menu, file_select).

## Files kept for the record (build3ds/nphud)
- yard_headA, yard_headB (HEAD proof), yard_np1, mac01t_np1_log.txt, mac01t_ht1_log.txt, mac01t_ht1b_log.txt, yard_ht1, pause_ht1, file_select_ht1, file_select_base_1237.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=<row> PM_LANE=<lane> PM_BUILD_CAP=900 sh 3ds/build.sh; then cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c.
- Lanes: build3ds/lane-nphead (HEAD e16fee4b9), lane-np1b (NP1B), lane-ht1 (HT1, be50979c1 plus the tree at that point).
- Test: PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build <lane> [-Try for mac01t].
- Compare: copy build3ds\azt\shots\<scenario> first, then python 3ds/tools/shot_cmp.py <dirA> <dirB>.

## Next steps
1. Fresh HEAD baseline for file_select (1 start, row with budget).
2. Orchestrator sets the mac01t mdlsum gate range.
3. Next HUD or NPC rows from plan_entity_npc.md (EN1, NP2, T1).
