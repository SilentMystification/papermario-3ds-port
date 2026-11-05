# Test catalog (azt)

## How to use (one page)

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build <lane>` (use `-Try` for an unverified scenario). `-List` lists the scenarios. `-CheckLog <file>` checks an old log and starts nothing.
- Each call is one real Azahar start and counts against the row budget (8 starts per row, reset once by the orchestrator with `run_azahar.ps1 -ResetBudget <row>`).
- Shots of each run go to `build3ds\azt\shots\<scenario>\` (the last run of that scenario). Shots are PPM, 240 by 400.
- A log pass is not a pass. A person must open the shot and confirm the subject.
- Known noise: scattered 300 to 1300 px differences on moving sprites between runs. Judge a shot by the subject, not by pixel counts.
- Status: verified scenarios are listed in the first table. Unverified or log-only scenarios are in the second table with the reason.

### Verified (run with one command each)

yard, tour10, text_menu, battle_start, sewer, ruins, volcano, fileselect_rotate. Menu lag tests with numeric gates (picture read by a person): pause_menu, partner_menu, file_select (section MENULAG). Log-only: tour_k30 (texture counter on kmr_30, delta 0 on lane azt2 at HEAD 0738350b2; picture not applicable). Fail demos: tour_no_warp, attract_no_warp. Completion only: full411.

### Unverified (open, with the console check)

- house_sign: the sign is not readable at 240x400 in the yard (kmr_20). A camboom debug switch did not change the view and was reverted. Console check: warp to kmr_20 entry 4 (`preset=yard`), stand next to the house, face the sign, and look at the sign by eye. Note the sign text and the camera position.
- super_text: no multibounce reached. Run 1 logged actionState 5, 8 and 10 without a SUPER text. Run 2 with dense A presses logged actionState 0 only (the battle menu was not entered). Console check: start a battle with `warp=kmr_03:0 story=ch0_toad_town battle`, select Jump, press A at the landing timing of a multibounce (src/battle/move/jump/multibounce.c), and look for the SUPER text by eye.


Usage (one line): `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario>` (add `-Row <id>`, `-Build <lane>`; `-CheckLog <file>` checks an old log without a start; `-List` lists all).

Defaults: PM_ROW `azt`, build `build3ds\pm_3ds.3dsx`, cap 90 s, first marker deadline 20 s after the Azahar start. Each call is one real start (-Force) and counts against the row budget.
Exit codes: 0 log checks pass, 1 check or run failure, 2 marker missing (stopped), 3 ATTRACT LOOP (stopped), 4 unknown or unverified scenario, 5 argument or build error, 6 no new log line for 40 s (stalled, stopped), 7 until static check failed (nothing run), 9 budget used. Self-test (no start): `azt.ps1 -SelfTest`.
A log pass is not a full pass. Read the shot files and confirm the subject. Shots of each run are copied to build3ds\azt\shots\<scenario>\ (runs before this change: shots are in build3ds\shots and were overwritten).


## Topic files

Section text is unchanged in `test_catalog/`. Summary = first sentence, cut at 60 characters (keeps this index under 10 KB). `LANE_146-152` and `LANE_153-160` carry the old source line range (two sections start with `Lane`).

| Topic | Summary | File |
|---|---|---|
| Verified | Scenario table, 30 row(s). | [VERIFIED.md](test_catalog/VERIFIED.md) |
| Unverified | Scenario table, 43 row(s). | [UNVERIFIED.md](test_catalog/UNVERIFIED.md) |
| POSHUD | Switch `poshud` (debug string, or options.ini `poshud=1`,... | [POSHUD.md](test_catalog/POSHUD.md) |
| POSHUD2 | HUD row table (fixed rows 1 to 6 always; optional rows... | [POSHUD2.md](test_catalog/POSHUD2.md) |
| Completion only | Scenario table, 1 row(s). | [COMPLETION.md](test_catalog/COMPLETION.md) |
| Lane 3 notes | house_sign (verified by eye, different view): `preset=intro... | [LANE_146-152.md](test_catalog/LANE_146-152.md) |
| Lane 3 note | Scenario fix: pause_menu and text_menu waited for "shot N... | [LANE_153-160.md](test_catalog/LANE_153-160.md) |
| LOGCLEAN note | yard and pause_menu debug strings now add `loglevel=debug`... | [LOGCLEAN.md](test_catalog/LOGCLEAN.md) |
| MENULAG | Scope: pause_menu, partner_menu, file_select. | [MENULAG.md](test_catalog/MENULAG.md) |
| Picture compare rule | Tool: `python 3ds/tools/shot_cmp.py <dirA> <dirB>` (copy... | [PICTURE.md](test_catalog/PICTURE.md) |
| LOGFIX | Scenario table, 3 row(s). | [LOGFIX.md](test_catalog/LOGFIX.md) |
| FSPLIT | Switch `fsplit` (needs the log on and loglevel=debug). | [FSPLIT.md](test_catalog/FSPLIT.md) |
| PRIMZFIX | Fix: `prim_ndc_z()` in 3ds/src/pm_gbi.c (the zmix and CPU z... | [PRIMZFIX.md](test_catalog/PRIMZFIX.md) |
| UNTILCHK | Static check before each start (no start on a fail): the... | [UNTILCHK.md](test_catalog/UNTILCHK.md) |
| WMODEL | The fsplit line (fsplit flag only) adds `wm <mean> max... | [WMODEL.md](test_catalog/WMODEL.md) |
| PAUSEFIX | Row PAUSE4A (background one load). | [PAUSEFIX.md](test_catalog/PAUSEFIX.md) |
| PAUSE5A | Build: default lane (build3ds\pm_3ds.3dsx, sha256 559728b3,... | [PAUSE5A.md](test_catalog/PAUSE5A.md) |
| PAUSE5B | Code: `pm_pc_begin` (state_pause.c, BEGIN_PAUSED), timers... | [PAUSE5B.md](test_catalog/PAUSE5B.md) |
| DC6 | Scenario table, 1 row(s). | [DC6.md](test_catalog/DC6.md) |
| SPK1 | Instrument only: two tick pairs in pm_mem_hud... | [SPK1.md](test_catalog/SPK1.md) |
| JTA | Scenario table, 11 row(s). | [JTA.md](test_catalog/JTA.md) |
| LOGCON | Scenario table, 2 row(s). | [LOGCON.md](test_catalog/LOGCON.md) |
| UIANCHOR | Scenario table, 3 row(s). | [UIANCHOR.md](test_catalog/UIANCHOR.md) |
| TEXL2 | Scenario table, 2 row(s). | [TEXL2.md](test_catalog/TEXL2.md) |
| OPTFIX | Scenario table, 1 row(s). | [OPTFIX.md](test_catalog/OPTFIX.md) |
| BOTLAYOUT | Scenario table, 2 row(s). | [BOTLAYOUT.md](test_catalog/BOTLAYOUT.md) |
| POSWARP | Scenario table, 2 row(s). | [POSWARP.md](test_catalog/POSWARP.md) |
| MISC1 | Scenario table, 10 row(s). | [MISC1.md](test_catalog/MISC1.md) |
| AUD2 | Scenario table, 1 row(s). | [AUD2.md](test_catalog/AUD2.md) |
| SPL1 | Result: no code change. | [SPL1.md](test_catalog/SPL1.md) |
| VRAM1 | Scenario table, 8 row(s). | [VRAM1.md](test_catalog/VRAM1.md) |
| TCV1 | Scenario table, 5 row(s). | [TCV1.md](test_catalog/TCV1.md) |
| WARPMENU | Scenario table, 3 row(s). | [WARPMENU.md](test_catalog/WARPMENU.md) |

## Scenario to file

Every scenario in `3ds/tools/azt_scenarios.json`, grouped by file. NONE = the catalog text does not name it.

| File | Scenarios |
|---|---|
| WARPMENU.md | warpmenu_sub mac01_sp0 mac01_texdiff |
| VERIFIED.md | profmenu_row yard yard_shot yard_shot_rp tex_kmr_11 kmr_10_band kmr_10_dd kmr_10_ddonly tex_kmr_12 mapchange_hud exit_armed mac01t_rec_chk yard_rp mac01t_rp mac01 tour10 tour_no_warp pause_menu battle_start sewer ruins volcano attract_no_warp text_menu census_yard census_yard_rp census_toad census_toad_rp census_mac01 census_toad_shot ndref_toad |
| NONE | nok02_tri_dd nok02_stdump_on nok02_stdump_off nok02_behroute mac01_walk mac01_walkdiff mac01_walkshots mac01_ndplan ndwalk_170 ndwalk_230 ndwalk_300 ndwalk_400 mac01_ndunit kmr_10_near kmr_10_clamp kmr_10_clamp1 kmr_10_rep1 kmr_10_rt1 bowser_intro_fire intro_bowser_scout tex_kmr_10 tex_kmr_09 tex_kmr_04 tex_kmr_05 tex_kmr_06 tex_kmr_07 tex_kmr_02 tex_hos_01 yard_rec thunder_box toad_water pause_spin_wide talk_off coin_wide yard_texwin0 mac01t_texwin0 kmr_10_dd_texwin0 chapter_wide intro_native |
| UNVERIFIED.md | nok02_tri nok02_dlccheck nok02_noboxint nok02_skipdump nok02_nobox nok11_splash yard_wide yard_stereo_l yard_stereo_r mac01_seam nok14_lake yard_stc yard_live forest_mim mac01t mac01t_mr0 tour_k30 battle_start_enc battle_start_fb battle_start_gp sun_sbk partner_menu file_select fileselect_rotate pause_spin pause_spin_grab yard_vramfill kzn19_texstat mac01_texstat house_sign super_text toad_gate mac01s toad_fountain ui1_choice toad_house_menu merlon_ball merlon_ball60 merlon_door golden_multi kzn19_slotpeak battle_slotpeak pause_slotpeak chain_slotpeak |
| JTA.md | ndst_yard ndobj_mac01 mac01_texprof jump_azahar jump_off_azahar boot_noprof_azahar boot_jump_azahar intro_jump_azahar prof_boot prof_scene savefile_check |
| AUD2.md | aud2_cache |
| LOGFIX.md | yard_pace yard_phase yard_dcsum |
| DC6.md | mac01t_dcsum |
| FSPLIT.md | yard_fsplit yard_pmu mac01t_fsplit mac01t_cap1 mac01t_rec mac01t_prof pause_fsplit ndref_yard ndref_pause ndref_battle ndref_text ndref_title ndref_splash ndref_stereo |
| POSHUD.md | yard_poshud |
| POSHUD2.md | mapchange_poshud mapchange_allrows toad_pond botlayout |
| PAUSE5B.md | pause_pcopen pause_pcopen_gbi field_mac00 |
| VRAM1.md | battle_start_fbvram hos01_texword texwrap_wall texreg_wall texreg_coin hos01_texmax |
| MISC1.md | battle_menu_bar battle_danger logcolor mario_proj_off mario_proj_wide warpfix_live warpfix_boot_save warpfix_fswarp_save warpfix_boss |
| TEXL2.md | volcano_nb volcano_l2 |
| COMPLETION.md | full411 |
| PRIMZFIX.md | toad_pond_zround |
| OPTFIX.md | boot_opts |
| LOGCON.md | logcon_err logcon_dbg |
| BOTLAYOUT.md | botlayout_menu |
| POSWARP.md | kmr10_pos kmr10_pos_hi |
| UIANCHOR.md | talk_wide battle_wide battle_target_wide |
| TCV1.md | texstride_wall texdump_coin coin_kmr03 coin_dgb13 |
