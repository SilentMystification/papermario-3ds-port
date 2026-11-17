# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 5b86d616e2, 2026-11-17. NEXT2 wrote ws_BLANKET, ws_GTSMALL, ws_FXDIG, ws_IMGFXT, ws_VCFIX. NEXT3 wrote ws_MDLTYPE, ws_BARRICADE.
Counters FACT (listcen on 3b890c6d7f, after FLTGT): build 312, gfxpos 8, data 191, data_arrays 0, gfxtype 284, recdl 11, reader 792, model 15.
Goal: elfgate.py prints func 0 data 0 gfxnames 0 and every listcen counter is 0 (include/PR/gbi.h definitions go in stage 4D).
## Pages on main (Haiku, disjoint files)
- ws_BLANKET: data -39, data_arrays -1, gfxtype -1. gen_gfx_syms.py N() image rule + twin top scan (dro_02), 11 new world twins. Coder ran.
- ws_GTSMALL: gfxtype -9 (pause labels, lava piranha byte counts, entity.c, map_api.c, nd_direct.c fill words, lkey allow line 2).
- ws_FXDIG: recdl -2 (damage_indicator.c :286 :288 entries from fxrest.d/cap.txt:144-145), new bench fxdig_drec (fxtest 129).
- ws_IMGFXT: gfxtype -9 (imgfx.c, imgfx.h, common_structs.h 3 members). Coder running; its :1115 comment check is answered.
- ws_VCFIX: bug fix. mdl_draw_vertex_copy records ModelLocalVertexCopy.dev, which no code writes (nd_vcopy_dev had no caller).
  New bench vcfix_hos01_drec (no base run: the base has the fault). verify_list has the console check.
- ws_MDLTYPE (NEXT3): gfxtype -16 (model.c 9, model.h 3, functions.h 2, HiddenPanel.c:10, tst_13 main.c:6). Starts after the VCFIX merge.
  lkey stays 3: each word walk keeps one (const uint32_t*) cast line. vcopy.sh gets a copy size check (gfxCount * 2 * sizeof(uint32_t)).
- ws_BARRICADE (NEXT3): gfxtype -6, new host test barricade.sh (maplist_test.c, seeded fault += 4). No wait, no shared file.
## Open groups, by the design they need
1-2. model.c types and map shape walkers: paged (ws_MDLTYPE, ws_BARRICADE).
3. Capture rows: status_star_shimmer.c:189 (star power scene), tst_13 main.c:306 (no Azahar route, pm_warp.c:747). Need a scene first.
4. recdl left after FXDIG (9): misc_particles.c:330 multi 30 (a per-particle value is in the tuple: move it out of the entry, design);
   green_impact :202, red_impact :209, shockwave :341 multi 2 (EFXGT owns the files; find the key of the 2 tuples in fxrest.d/cap.txt);
   chomp_drop :164 :167 unseen; disable_x :308 :310 unseen (unk_38 reaches 10 after about 90 frames, disable_x.c:150-160; the fxtest slot is 44,
   pm_fxtest.c:205: needs a longer slot for that effect); imgfx.c:1805 overlay list (monstar; capture through axns_sam2_drec).
5. gfxpos 10: main.c, main_loop.c, cam_main.c:29, variables.h:111, nuGfxTaskStart (nusys.h:790, pm_nusys.c:151): stage 4B/4C task start design.
6. 3ds/src walker gfxtype (nd_walk 33, nd_ent 29, nd_shape 20, nd_rec 15, nd_recs 15, nd_cen 7, nd_geom 1): goes with the walker deletion.
   A rename to another 8-byte type only hides the counter. Not paged.
7. include gbi.h, gbi_custom.h, gt.h, sp.h, gs2dex.h, gu.h: stage 4D.
8. reader 792 (C8) and model 15 (C9): the walker op stream (NDO_ tokens: nd_op.h 173, nd_walk.c 71, nd_rec.c 68, nd_op.c 62, nd_recs.c 48,
   nd_scomp.c 44, nd_cen.c 42, nd_tx.h 40, nd_set.c 39, nd_model.c 32, nd_dlc.c 27) and the state model definitions (nd_comb.c 3, nd_tex_set.c 3,
   nd_decide.c 2, nd_rect.c 2, nd_texwin.c 2, nd_geom.c, nd_mtx_set.c, nd_set.c 1 each). Phase E stages 1-3 (plan_endgame.md:188-196). Precondition
   C1, C2, C10 at 0: not met. No Haiku page. Design choices: nd_st.c, nd_val.c, nd_nr.h, nd_at.h write NDO_ unit words for native records
   (stage 3 needs a native record form); gen_pso_tab.py links nd_decide.c (host keeps decide: move it to 3ds/tools?); nd_cen.c is a debug census.
## Facts that cost time
- host.d scripts need gcc: run them only through nd_host_test.sh (docker). blanket.sh is pure python and runs in Git Bash.
- lkey_cen.py:71-72 fails a listed file whose reader count drops (RECBUF, GTSMALL). Each page that removes `.words.w*` reads edits allow.txt.
- Host cache fault: nmdl0.sh misses /tmp/ndcombs.c after `hostsel: hit inline`. Use PM_HOSTSEL=all on the rowloop line.
- Coders often hand back before the oracle reply arrives. Tell the orchestrator to resume them with the reply.
- gen_gfx_syms.py: IMG matched only plain names before BLANKET; only the dro_02 twin has a first line that is not its own source.
- A list without cycle bits gets an itab (nd_gfx_conv.c:863), so it can move without an entry.
- fxrest.d/cap.txt tuples can be stale: FXDIG psochk showed conv_k4 (w7) stated 0x7f, walker 0. Check w7 against a live site desc first.
- fxtest numbers are 0-based FXLIST indices (flame 27, damage_indicator_12 129). Slot 44 frames, start 400, spawn at k 2.
## What to do first
1. Answer coder questions (MDLTYPE, BARRICADE, GTSMALL, FXDIG, IMGFXT, VCFIX).
2. green_impact, red_impact, shockwave recdl: paged by DESIGN3 (ws_FXCULL, recdl -3). Do not page them again.
3. Group 5 (gfxpos 8) task start design, then group 8 needs the user's choice.
