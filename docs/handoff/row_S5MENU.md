# Row S5MENU: pause and file menu contents as native records (NATIVEAPI S5; two rows S5MENU-P and S5MENU-F, one worktree each)

Model: Sonnet (user decides).
- Why not Haiku: these contents are drawn inside windows, many inside ROTSCALE off-screen passes, so their target is an
  off-screen image. The direct path needs the off-screen answer of S6T (design question 3). The file menu draws characters
  through its own filemenu_draw_char (not msg_draw).
- Why not Opus: the helpers exist when it starts.

## Blocker and base
S6T merged (with its off-screen answer), S2W and S3MSG merged. S5MENU-P: `sh 3ds/tools/wt_new.sh S5MENUP`. S5MENU-F:
`sh 3ds/tools/wt_new.sh S5MENUF`. They edit different directories, so both can run at once.

## Code read (gbi calls per function)
- Pause: pause_badges_draw_contents 22, pause_tutorial_draw_contents 12, pause_map_draw_contents 11,
  pause_partners_draw_contents 10, pause_spirits_draw_contents 9, pause_textbox_draw_contents 3, pause_items_draw_contents 3,
  pause_draw_menu_label 1, pause_draw_rect 1.
- File menu: filemenu_draw_contents_copy_arrow 21, filemenu_draw_char 17, func_filemenu_8024D710 13 (select language),
  filemenu_update_show_name_confirm 5, filemenu_update_hidden_name_confirm 5, filemenu_draw_contents_choose_name 3,
  filemenu_draw_rect 1.

## Design
- Each texture rectangle uses the HUD pattern. Each fill uses nd_fill_run. Each quad uses nd_direct_quad (S5CHUD).
- The off-screen target comes from S6T's rule.
- filemenu_draw_char uses NAPI3B's glyph path.
- The two update functions (name confirm) set window flags and matrices: classify each op as state, value or draw in the commit.
- Fallback counted.

## Files and places
S5MENU-P: src/pause/*.c (the functions above). S5MENU-F: src/filemenu/*.c (the functions above).

## Gate (expected values, per row)
- Build, memcheck OK; nd_host PASS.
- P: pause_tab_det_pic and pause_spin_grab_det_w300, with bases. F: a det file select scene (pre-flight; else add
  `fsel_det` from fsel_fncov_idle with `shots shotwin=400:2` instead of fncov) and pause_tab_det_pic. 2 starts each. Every shot
  byte-equal.
- listcen: src/pause or src/filemenu build count 0 except the counted fallbacks. Budget 6 each.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit (the second row to finish deletes it).
