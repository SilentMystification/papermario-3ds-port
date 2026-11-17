# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 3ad3164cdd, 2026-11-17. Oracle DESIGN1 wrote ws_NMDL6 and ws_VSLOT3 (groups 3 and 4 below).
Oracle DESIGN2 wrote ws_FLTGT, ws_OVLIMG (groups 1-2) and ws_POSTFAIL2 (gbi_viewport vp 1 fault, page + patch each).
Counters on 358070e292 (FACT listcen): build 326 (include 308, src 18), gfxpos 29, data 339, data_arrays 10, gfxtype 424, recdl 11,
reader 792, model 15. After GFXPOS7 + THEATVP (FACT, oracle listcen of main + both patches): build 324, gfxpos 22, data 323,
data_arrays 9, gfxtype 420.
Goal: elfgate.py prints func 0 data 0 gfxnames 0 and every listcen counter is 0 (include/gbi.h definitions go in stage 4D).
## Ready pages (Haiku, start now, disjoint files)
- ws_FLTGT: CIMG_UNIT in nd_gfx_conv.c, twin buffers D_800B32E0/D_800B3AE0 GS_IMG, D_09000800 + D_090009E0 move, flame.c:190 :197 ND_SELF.
  build -2 gfxpos -2 data -49 data_arrays -2 gfxtype -4 (FACT listcen). Gate fxops_drec (fxtest 27 = flame).
- ws_OVLIMG: D_80156910 = unsigned char[0x9600] (160 x 120 RGBA16, ALIGNED(8)), D_8014EA48 itab + move (gset.d OVLIMG), :264 ND_SELF.
  build -1 gfxpos -1 data -40 data_arrays -1 gfxtype -1. OVERLAY_TYPE_2 has no caller (FACT grep): gate spot_mac01_drec + ovlimg.sh.
- ws_POSTFAIL2: bad viewport log (gbi_cold_bad_vp, nd_stream_cur, nd_rec_where), up to 3 starts. Read its hit line: unit ctx = rec ctx
  means core 1 rewrote a chain that core 0 still runs (recycled stream buffer); cmd w1 is the NAPI runner (resolve with the ELF).
- ws_BASEKEEP2 (tool fault, no counter, page of BACKLOG5): its state is with the orchestrator.
Both FLTGT + OVLIMG: build 321 gfxpos 19 data 234 data_arrays 6 gfxtype 415 (FACT oracle listcen on 3ad3164cdd + both patches).
## Remaining src build 13 and gfxpos 19 after the 3 FLTGT/OVLIMG edits, by group (FACT grep 358070e292, groups 1-2 now paged)
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
- host: a change of nd_rec.c fails geounit.sh under hostsel (/tmp/ndcombs.c of the inline unit). Use PM_HOSTSEL=all.
- hostsel store can fail with WinError 5 (hostsel.py:123 rename) while every unit is ok. GUESS: a race on build3ds/hostsel.
- Git Bash PATH: a C:/ path breaks PATH (colon). Use /c/... A python3 shim dir is needed to run host.d/*.sh locally.
- Source files can be CRLF after checkout in a scratch worktree, LF in others: python on Windows writes CRLF with write_text. Use newline=''.
- gset.py holds a list named with `&X` (gset.py:173). A new file needs gconv_src.d/<row>.txt.
- ws_FXLIT.md, ws_FXLT.md, ws_POSTASYNC.md (+ patches, ws_DLCCHK.patch) are still on main. GUESS: done rows (NDN_TSZ is in
  nd_gtab.h, postmerge_bg.sh exists). The orchestrator decides the delete.
## What to do first
1. Answer coder questions of GFXPOS7, THEATVP, BASEKEEP2 (SendMessage, file:line).
2. Groups 1-4 have pages (FLTGT, OVLIMG, VSLOT3, NMDL6). Read the POSTFAIL2 hit line when it comes.
3. Then page the flame entry capture (:188 :211) and the shimmer capture.
