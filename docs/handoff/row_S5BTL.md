# Row S5BTL: battle draws left after S5PQUAD (tattle camera, menu content, bonk) as native records (NATIVEAPI S5)

(Oracle 5 split: row_S5BTL1.md (Haiku) takes the two tattle camera state ranges of 16C8E0.c; row_S5PQUAD2.md takes
190B20.c btl_appendGfx_prim_quad. This Sonnet row keeps the tattle background strips (16C8E0.c :780-823, CI8 strips with a
scroll, COPY cycle) and the FILL-cycle fills (:849, :856), and starts after S5BTL1 merges. The small battle files (lava_piranha 5,
hos_01 stage 4, popup_messages, the item moves, Snowflakes: 1 each) are S7 data rows.)
Model: Sonnet (user decides).
- Why not Haiku: tattle_cam_pre_render (48 gbi calls, src/battle/16C8E0.c) sets up the tattle camera view (viewport, scissor,
  projection, fills), and its state is inherited by the world draws that follow. Each op must be classed: a value op stays, a
  state range is compiled (S6S), a fill becomes nd_fill_run.
- Why not Opus: no new helper.

## Blocker and base
S5PQUAD and S6S merged. `sh 3ds/tools/wt_new.sh S5BTL`; PM_ROW=S5BTL PM_LANE=S5BTL.

## Code read (gbi calls per function)
- src/battle/16C8E0.c tattle_cam_pre_render 48;
- src/battle/btl_states_menus.c: btl_menu_moves_draw_content 1, btl_menu_strats_draw_content 1;
- src/battle/popup_messages.c btl_bonk_setup_gfx 1;
- src/battle/190B20.c: btl_appendGfx_prim_quad is S5PQUAD's.
- napicen3 btl: battle/190B20.o 32 (S5PQUAD), effects/disable_x.o 28 (an effect: S7EFX).

## Design
- tattle_cam_pre_render: the static part becomes an S6S range; the per-frame values (viewport, scissor, matrices) stay as value
  ops in the record; fills use nd_fill_run.
- The 3 single calls follow the same rules.
- Hand back the class of each of the 48 ops in the commit message (state, value or draw).

## Files and places
src/battle/16C8E0.c (tattle_cam_pre_render), src/battle/btl_states_menus.c (the 2 functions), src/battle/popup_messages.c
(btl_bonk_setup_gfx).

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- One list with bases: battle_wheel_nowide_det, plus a tattle scene. Pre-flight: grep the scenarios for "tattle". If there is
  none, add `btl_tattle_det` from battle_wheel_nowide_det with a padscript that opens tattle; take the press sequence from an
  existing tattle route if one exists, else hand back before adding. 2 starts. Every shot byte-equal.
- napicen_btl: the moved functions' ops 0. Budget 6.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit.
