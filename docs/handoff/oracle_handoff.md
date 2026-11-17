# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 3ad3164cdd, 2026-11-17. Oracle DESIGN1 wrote ws_NMDL6 and ws_VSLOT3 (groups 3 and 4 below).
Counters on 358070e292 (FACT listcen): build 326 (include 308, src 18), gfxpos 29, data 339, data_arrays 10, gfxtype 424, recdl 11,
reader 792, model 15. After GFXPOS7 + THEATVP (FACT, oracle listcen of main + both patches): build 324, gfxpos 22, data 323,
data_arrays 9, gfxtype 420.
Goal: elfgate.py prints func 0 data 0 gfxnames 0 and every listcen counter is 0 (include/gbi.h definitions go in stage 4D).
## Ready pages (Haiku, start now, disjoint files; page + patch, apply --check OK on 358070e292, one host run PASS on all 3)
- ws_GFXPOS7: msg_draw.c:334 :1692, filemenu_msg.c:184 :209 drop the gMainGfxPos half; draw_box.c:462 -> nd_pso_unit. gfxpos -5.
  Gate wscmb_pause_drec, wscmb_fsel_drec.
- ws_THEATVP: GS_VP symbol kind (gen_gfx_syms VP rule, nd_gfx_conv.c viewport lookup), TheaterInitGfx moves, curtains.c:133 :177
  ND_SELF. build -2 gfxpos -2 data -16. Gate curt_title_drec.
- ws_BASEKEEP2 (tool fault, no counter): bench.py base_check_shots removed row bases (stored() with the defs of the postmerge
  checkout). FACT post_merge_f1216481d1.txt:8978. Host test basecheck.sh with 2 seeded faults. No build, no Azahar.
## Remaining src build 16 and gfxpos 22 after the 3 pages, by group (FACT grep 358070e292)
1. flame.c:188 :190 :197 :211 (build 4, gfxpos 4, data gfx/flame.c 49). :190 D_090009E0 unit 15 is gsDPLoadTextureTile of
   OS_K0_TO_PHYSICAL(&D_800B3AE0), a render target buffer (gfx/flame.c:47), and unit 0 is gsDPSetColorImage of it: NTGT form, not a
   symbol rule. D_09000800 unit 12 the same kind (ntab_unsup). :188 :211 need captured entries (fxtest of EFFECT_FLAME). :197 after them.
2. screen_overlays.c:264 D_8014EA48 (data 40 with D_8014E8F0/E9A8): its 11 tile loads read 160x120 RGBA16 from `BSS s32 D_80156910`
   (screen_overlays.c:21, 4 bytes). A symbol rule needs an image extent decision: user or Sonnet. Not paged.
3. filemenu_common.c:996 :1018 D_8024B708: PAGED ws_VSLOT3 (Sonnet, user yes 2026-11-17; patch, apply --check OK on 3ad3164cdd).
4. entity_model.c:490 :772 :784, model.c:4340, flo_10 waterfx.c:231, mac_00 water_gfx.c:20, mac_01 crystal_ball_gfx.c:95: PAGED ws_NMDL6
   (Sonnet, user yes 2026-11-17; patch, apply --check OK). Oracle host run of both patches (PM_CACHE=off): ND HOST TESTS PASS.
5. status_star_shimmer.c:189 D_801097D8 itab: an entcap capture (star power scene OPEN).
6. tst_13 main.c:306: an entry needs the state of mdl_draw_hidden_panel_surface (main.c:302); no Azahar capture (pm_warp.c:747).
7. gfxpos only after GFXPOS7: cam_main.c:29 (definition), main.c:151, main_loop.c:178 :195 (task start: stage 4B), model.c:4258
   :4273 (effcen probe ec_g0).
## Remaining data 323 after the 3 pages (FACT): include/PR/gbi.h 191 (stage 4D), gfx/flame.c 49 (group 1), screen_overlays.c 40
(group 2), blanket.gfx.inc.c 39 (the StaticAnimatorNode .displayList fields of ToadHouseBlanketAnim.inc.c: animator form, NMDL), filemenu_gfx.c 4.
## Phase E: no trigger holds within the next 15 rows (groups 1-4 need NTGT, Sonnet or user designs). No phase E page.
## Facts that cost time
- Host test with the cache on: nmdl0.sh failed on a missing /tmp/ndcombs.c after a src/model.c edit (hostsel skips the inline prep). PM_CACHE=off passes. Tool fault, not paged.
- Scratch worktree: `git worktree add --detach <scratchpad>/wt main` took 5 s this time. Remove it with the full path.
- Git Bash PATH: a C:/ path breaks PATH (colon). Use /c/... A python3 shim dir is needed to run host.d/*.sh locally.
- Source files are CRLF in the worktree: edit with a CRLF-aware script; git diff gives LF patches.
- gset.py holds a list named with `&X` (gset.py:173). A new file needs gconv_src.d/<row>.txt.
- ws_FXLIT.md, ws_FXLT.md, ws_POSTASYNC.md (+ patches, ws_DLCCHK.patch) are still on main. GUESS: done rows (NDN_TSZ is in
  nd_gtab.h, postmerge_bg.sh exists). The orchestrator decides the delete.
## What to do first
1. Answer coder questions of GFXPOS7, THEATVP, BASEKEEP2 (SendMessage, file:line).
2. Ask the user for the Sonnet designs of groups 1 (NTGT flame), 3 (mixed vertex slots) and 4 (NMDL6/7).
3. Then page the flame entry capture (:188 :211) and the shimmer capture.
