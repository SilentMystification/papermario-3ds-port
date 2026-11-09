# Handoff DIRECT (epic: the game calls the native producers directly)

STATE
- Branch DIRECT (worktree build3ds/wt/DIRECT), main 8cbc32f34 merged. Plan: plan_direct.md. Row file row_DIRECT.md stays (the walker removal of plan section 1 is open).
- Done: D0-D6 records and state runs; D6b model.c, sprite.c, main.c, main_loop.c record (f15882bc5); D6c effect inline data and lists for later (nd_rec_data, NdSub; bff28f8c5); D7-D9 no task list: custom gfx lists in stream memory, NDC_LIST and mainGfx/backgroundGfx deleted (f05be775d). Each commit message has its gates.
- Memory: memcheck static 20672 KB, margin 1314 KB OK (D9 freed 1060 KB).
- Fixed on the way: mdl_get_vertex_count wrote the hidden panel vertices into gBackgroundImage (the mac_00 sky marks of D6 and D9; layout dependent).
- Open (walker still used, plan section 1 end state): nd_walk_run walks (a) the custom gfx lists that build_custom_gfx writes into stream memory (nd_rec_sub > 0: list words, G_PM_REC records, R11 parse nd_r11_walk), (b) G_DL targets that ent_gdl and dlc_try refuse (static lists, local Gfx buffers such as Block.c-style lists), (c) the sprite and entity fallback words in the NDC_SPR and NDC_ENT payloads.
- Next: D10 custom gfx builders record stream ranges (R11 pre and post as NDC_CALL ranges, nd_r11_walk reads stream commands), then a load-time compile of the static lists; then delete nd_walk_run, g1_skip, g2_same and the G_DL stack.
- Gate set per slice (one list run, normal speed, gates.sh in build3ds/lane-DIRECT): spot_mac01_det, battle_start_f182_cold, msgbox_talk_nowide, pause_spin_grab_det_w300, mdl5_iwa10_det, direct_pra13_det, menu_open_det, gameover_fps60; nok11_det turbo; fx_direct_det and lava_piranha_det (turbo) for effect changes. Base: lane build of the slice base (git diff > patch, checkout, build, run, git apply).
- Diagnostics: ndref_mac00_det and ndref_fx65_det (ndref.bin draw streams and the cen census lines: the census tex hash names a texture content change); pause_pre_det (frames before the pause capture).
- Console: not run since D4b-2 (spot_mac01_prof, spot_mac00_prof, spot_sbk56_prof, starfx_hos01_prof for the user).
