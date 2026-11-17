# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 808bd98f15, 2026-11-17. Oracle ENDGAME1 (the only oracle) wrote the 4 pages below.
Counters on 52f9b15c42 (FACT listcen): build 341 (include 308 = gbi.h 245, gbi_custom.h 61, gt.h 2; src 33), gfxpos 44, data 482,
data_arrays 15, gfxtype 428, recdl 11, reader 792, model 15. After the 4 pages (FACT, oracle listcen of main + all 4 patches):
build 326 (src 18), gfxpos 29, data 339, data_arrays 10, gfxtype 424, recdl 11.
Goal: elfgate.py prints func 0 data 0 gfxnames 0 and every listcen counter is 0.
Not ours: 3ds/build.sh, nd_host_test.sh, bench.py, merge_row.sh, post_merge.sh, postmerge_bg.sh, rowloop.sh, gen_gfx_tab.sh, run_azahar.ps1, CMake, .ld.
## Ready pages (Haiku, start now, disjoint files; page + patch, apply --check OK on 52f9b15c42, one host run PASS on all 4)
- ws_ENTSELF (row ENTSELF2): 6 ND_SELF sites incl. pause_gfx.c:425. build -6 gfxpos -6. New bench entself_fx_drec.
- ws_TSTSHK (row TSTSHK2): tst_13 shockwave gset, entity_model.c:470 dead arm. Gate decision in the page (tst13 blocked, host proof).
- ws_ENTCAPR2: curtains.c:151 and npc_merlee.c:248 :259 entries. build -3. Gate curt_title_drec, gmerlee_drec. Old ws_ENTCAPR goes.
- ws_INITGS: HudElemBack/FrontInitGfx, TitleSetupGfx gset, 5 ND_SELF sites. build -5 data -28.
## Remaining src build 18 and gfxpos 29 after the 4 pages, by group, with the move (FACT grep 52f9b15c42 unless marked)
1. Converter refusals ("unit address outside the symbol map", FACT ntab_unsup.txt of the oracle host run):
   screen_overlays.c:264 D_8014EA48 (unit 15 loads `BSS s32 D_80156910`, screen_overlays.c:21, a screen copy buffer; also itab),
   effects flame.c:190 D_090009E0 (unit 15, kind OPEN), curtains.c:132 :176 TheaterInitGfx (unit 0 gsSPViewport(&TheaterViewport)).
   Move: a converter symbol rule row (RAWIMG form: gen_gfx_syms + host test with seeded fault), then gset, then entries. Not paged.
2. itab with no capture yet: status_star_shimmer.c:189 D_801097D8, flame.c:188 :211 (D_09000918, D_090008F8 itab). Move: entcap
   capture (ENTCAPE form). Scene OPEN for the shimmer (star power use). flame: fxtest scene of the flame effect (id OPEN).
3. flame.c:197 preset->dlist (handle of the flame lists): after 1 and 2, nd_draw_list with the entry of each preset. Data: gfx/flame.c 49.
4. entity_model.c:492 (reflect), :774 :786 (draw_entity_model_E); model.c:4340; world node lists flo_10 waterfx.c:231,
   mac_00 water_gfx.c:20, mac_01 crystal_ball_gfx.c:95. Move: NMDL6/NMDL7 (entity and model record form). Design OPEN (Sonnet needs the user).
5. filemenu_common.c:996 :1018 D_8024B708 (+ filemenu_gfx.c data 4): it draws vertex slots 0-1 of D_8024B6F0 under another matrix.
   Move: a design (both quads on the CPU, or one list per arrow). Not Haiku until the oracle writes it.
6. tst_13 main.c:307 gSPDisplayList(NDN(N(shockwave_gfx))) after TSTSHK2: needs an entry; no Azahar capture (pm_warp.c:747).
   Move: an entry from the code read of main.c:302-305 (cycle 1, AA_ZB_OPA_SURF, mdl_draw_hidden_panel_surface state), host proof only.
7. gfxpos only (11): cam_main.c:29 (definition), main.c:151, main_loop.c:178 :195 (task start: stage 4B), model.c:4258 :4273 (effcen
   probe ec_g0), msg_draw.c:334 :1692 and filemenu_msg.c:184 :209 (gMainGfxPos half of a stream position compare), draw_box.c:462
   (nd_gx_w writes words to gMainGfxPos). Move after build 0: one Haiku page drops the gMainGfxPos halves (only nd_rec_task moves
   it then: GUESS until build is 0), draw_box.c:462 needs a read of nd_gx_w (OPEN).
## Remaining data 339 after the 4 pages (FACT)
include/PR/gbi.h 191 (macro definitions: they go with stage 4D, never by a row), blanket.gfx.inc.c 39 (named by
ToadHouseBlanketAnim.inc.c, included by dro_02/npc.c:59, hos_03/npc.c:7, jan_03/npc.c:20: gset include mode of those 3 files, not
checked), gfx/flame.c 49 (group 1-3), screen_overlays.c 40 (group 1), curtains.c 16 (TheaterInitGfx, group 1), filemenu_gfx.c 4 (group 5).
## Phase E triggers (plan_endgame.md section 6). FACT: include build 308 and include data 191 are gbi.h macro definitions. They reach
0 only in stage 4D. So read every "build 0" and "data 0" trigger as "src and assets 0" (oracle rule, 2026-11-17).
- Stage 1A-1C: src build 0, gfxpos 0 (except cam_main.c:29 and the task lines of group 7), recdl 0, no device caller of nd_dlc_compile.
  Holders: groups 1-7 and the recdl 11 (effects 10, imgfx.c 1). Not within the next 15 rows: group 4 and 5 need designs.
- Stage 5: data 0 outside gbi.h. Holders: groups 1, 3, 5 and blanket. Not within 15 rows (group 5 design).
- No phase E page is written: no trigger can hold within the next 15 rows.
## Facts that cost time
- Write patches in a scratch worktree (`git worktree add --detach <scratchpad>/wt main`); the add takes over 2 min (run it alone).
  Remove it with the full path. Source files are CRLF: replace with a newline-aware script, git diff gives LF patches.
- gset.py holds a list that its device file names with `&X` (hold_line, gset.py:173). Change `&X` to `X` first (same address).
- A new file needs gconv_src.d/<row>.txt to reach the converter: gset_ok.txt lists only lists of gconv sources.
- host.d/buildlock.sh failed once in the oracle run (`docker engine not answering`, race), passed on the rerun: a tool fault, not a page fault.
- entcap logs a list only when it has no v2 table (nd_walk.c `!nt`). ND_SELF is the null entry (nd_gtab.h:88, nd_gtab.c:136).
- Writing Python through a Bash heredoc mangles backslashes: use the Edit tool or chr(92).
## What to do first
1. Answer coder questions of ENTSELF2, TSTSHK2, ENTCAPR2, INITGS (SendMessage, file:line).
2. Page group 1 (converter symbol rule for D_80156910, TheaterViewport, flame unit 15), then group 2 captures, then group 7.
