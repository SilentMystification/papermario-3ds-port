# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 3d552f33b3, 2026-11-17. Oracle ENTRY (this file) wrote the ENTRY pages below. Counters (FACT listcen 9bc7a6f298, src not
changed since): build 386 (include 308, src 78), gfxpos 89, data 1207, data_arrays 92, gfxtype 580, recdl 11, reader 789, model 15.
Goal: remove the N64 display list system. Done when 3ds/tools/elfgate.py prints func 0 data 0 gfxnames 0 and every listcen counter is 0.
Scope (orchestrator 2026-11-17): at most 2 oracles. assets/us/effects/gfx belongs to the effects oracles (a-m, n-z). FXSEED gap a,
mdl_get_copied_gfx and the converter-refused lists belong to the other oracle. The loop-fix files are not ours (3ds/build.sh,
nd_host_test.sh, bench.py, merge_row.sh, post_merge.sh, postmerge_bg.sh, push_3ds.sh, gen_gfx_tab.sh, rowloop.sh, run_azahar.ps1, CMake, .ld).
## Ready pages (each: page + patch, `git apply --check` OK on main at release)
- ws_ENTSRC (Haiku, start when the loop fix leaves gen_gfx_tab.sh): switch entcapt (nd_walk.c entcap_rec takes the key), entapply
  SITE any src/, gen_ntab_entries.py extra files `<path>[=NS]` from 3ds/tools/ntab_ent.d/*.txt (gen_gfx_tab.sh:28), host test entsrc.sh.
- ws_ENTSELF (Haiku, start now): 6 sites of self-contained tables to nd_draw_list(X, ND_SELF). C1 -6, C2 -6. New bench entself_fx_drec.
- ws_ENTCAPE (after Merge ENTSRC): 10 entity sites, capture tour entcape_cap (5 spots, GUESS in view), gate entcape_kmr03_drec.
- ws_ENTCAPR (after Merge ENTSRC): curtains:155, filemenu_common:996 :1018, npc_merlee:248 :259. Captures reuse curt/fsel/gmerlee words.
- ws_ENTCAPD (after Merge ENTSRC): screen_render_util:157 :181 (kgr_01 darkness), gate entcapd_kgr01_drec.
## Open work, in this order (orchestrator 2026-11-17)
1. SaveBlock: one gset.py line (src/entity/model/SaveBlock.c) moves Entity_SaveBlock_RenderFaces: data -33, data_arrays -1, gfxtype -1
   (ASSET_C oracle dry run). Gate bench spot_mac01_drec. Not yet paged.
2. listcen.py rule: do not count an asset that no source includes (blanket.gfx.inc.c 39 data, world/dead/lava_piranha/vine.gfx.inc.c
   17 data), LCNOBUILD shape (NOBUILD_RULE, 3ds/tests/host.d/lcnobuild.sh). Dry run it, then page.
3. entity_model.c (build 4, gfxpos 4, data_arrays 2, gfxtype 8; :470 :492 :774 :786 model->gfx.displayList): page it only if closed.
4. Phase E pages, each with its trigger (listcen totals and a grep), function names to delete, terms that must return nothing:
   1A-1C need build 0, gfxpos 0, recdl 0 (1C also: no device caller of nd_dlc_compile; FACT nd_ent.c:311 is the last one); 1D needs
   data 0, data_arrays 0 and NS3; stage 2 needs stage 1 and PSOCAP; the user decided (2026-11-17): a draw with no table entry uses a
   fixed fallback mode and writes one log line, nd_comb_core does not stay; stage 5 needs data 0; stage 3 needs 1, 2, 5 and recdl 0;
   stage 4 needs data 0 and stage 3. Terms per stage: design_lookahead.md section 7. Not written.
## src build/gfxpos sites by group (FACT grep, 023db9ca81; build 78, gfxpos 89 in src)
- ENTRY pages above: 23 sites. Not paged, no scene known: status_star_shimmer.c:189 (itab), screen_overlays.c:309 D_8014EA48 (itab).
- Other oracle: copied gfx 21 (hos_01.inc 4, Snowflakes 1, flo_14 2, hos_02 4, hos_03 3, kgr_01 1, rowf 1, club64 1, pra_29 2, Whale 2),
  model node lists 3 (flo_10 waterfx:231, mac_00 water_gfx:20, crystal_ball_gfx:95), model.c:4341, entity_model.c 4 (item 3 above).
- Converter refused (other oracle): curtains:136 :180 (TheaterInitGfx), :183 NoControllerGfx RECT, screen_overlays 7 PRIMDEPTH,
  hos_05 intro 2 SCISSOR, kmr_20 records 1 RECT, mgm_00 scoreboard 1 RECT, state_title_screen 3 and hud_element 2 (state lists with
  SCISSOR, not in the converter set yet). Effects oracles: flame.c 4 (gap e). tst_13 main.c:307 (list writer).
- gfxpos only (11): cam_main.c:29, main.c:151, main_loop.c:178 :195 (frame task: S9DEL4B), model.c:4259 :4274, msg_draw.c:334 :1692,
  filemenu_msg.c:184 :209 (stream position compares), draw_box.c:462 nd_gx_w. No page yet.
## Facts that cost time
- entcap logs a list only when it has no v2 table (nd_walk.c:620 `!nt`). Every itab list has an NdNDev (IENTRY), so before ENTSRC no
  itab site can be captured. ND_SELF is a null entry: nd_draw_list(X, ND_SELF) == nd_rec_dl(X) on the device (nd_gtab.h:88, nd_gtab.c:132).
- gen_ntab_entries.py names(): a member access (data->gfx2) or a function parameter (HeartBlock arg1) gives no pair. Not a fail.
- GENKEY (branch): a new file in a traced tar path changes K0 (readkey.sh header), so a new ntab_ent.d/<ROW>.txt rebuilds gfx_tab.
- Writing Python through a Bash heredoc on this host mangles backslashes (\b, \r, \n became control chars): use the Edit tool or chr(92).
- Tour items `map:entry@x:y:z` run in one Azahar start (pm_warp.c:368-374), end line `tour: done items <n>` (:1462). Cap 45 s: 5 items.
- Scratch worktree for patches: create one in the scratchpad with `git worktree add --detach`, remove it when done.
## What to do first
1. Answer coder questions of ENTSRC, ENTSELF, ENTCAP* (SendMessage, file:line).
2. Open work items 1 to 4 in order. Commit each page alone (`git add -f`).
