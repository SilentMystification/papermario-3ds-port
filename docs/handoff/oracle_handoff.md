# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main d9038b0907, 2026-11-17. Oracle NEXT2 wrote ws_BLANKET, ws_GTSMALL, ws_FXDIG, ws_IMGFXT, ws_VCFIX.
Counters FACT (listcen on 5e7ac3c413): build 314, gfxpos 10, data 279, data_arrays 3, gfxtype 297, recdl 11, reader 792, model 15.
Goal: elfgate.py prints func 0 data 0 gfxnames 0 and every listcen counter is 0 (include/PR/gbi.h definitions go in stage 4D).
## Pages on main (Haiku, disjoint files)
- ws_BLANKET: data -39, data_arrays -1, gfxtype -1. gen_gfx_syms.py N() image rule + twin top scan (dro_02), 11 new world twins. Coder ran.
- ws_GTSMALL: gfxtype -9 (pause labels, lava piranha byte counts, entity.c, map_api.c, nd_direct.c fill words, lkey allow line 2).
- ws_FXDIG: recdl -2 (damage_indicator.c :286 :288 entries from fxrest.d/cap.txt:144-145), new bench fxdig_drec (fxtest 129).
- ws_IMGFXT: gfxtype -9 (imgfx.c, imgfx.h, common_structs.h 3 members). Coder running; its :1115 comment check is answered.
- ws_VCFIX: bug fix. mdl_draw_vertex_copy records ModelLocalVertexCopy.dev, which no code writes (nd_vcopy_dev had no caller).
  New bench vcfix_hos01_drec (no base run: the base has the fault). verify_list has the console check.
- FLTGT (branch): flame data -49, data_arrays -2.
## Open groups, by the design they need
1. model.c types (model.c 7 tokens, model.h :48 :112 :146, functions.h:509): Haiku page after VCFIX merges (model.c). Facts:
   gfxCopy as uint32_t* changes `sizeof(*copy->gfxCopy[i])` (model.c:3690) from 8 to 4: write `gfxCount * 2 * sizeof(uint32_t)`.
   mdl_local_gfx_update_vtx_pointers (:3647) index rule w0 -> [0], w1 -> [1], ++ -> += 2; lkey allow `src/model.c 3` -> 2.
   mdl_project_tex_coords has 2 more prototypes: HiddenPanel.c:10, tst_13 main.c:6. set_custom_gfx callers pass void* (map_api.c after GTSMALL).
2. barricade.c:44-55 and GetFirstTriangleNormal.inc.c:17-48 (gfxtype 6): map shape list readers. u32 index rewrite, lkey MAP lines change. After group 1.
3. Capture rows: status_star_shimmer.c:189 (star power scene), tst_13 main.c:306 (no Azahar route, pm_warp.c:747). Need a scene first.
4. recdl left after FXDIG (9): misc_particles.c:330 multi 30 (a per-particle value is in the tuple: move it out of the entry, design);
   green_impact :202, red_impact :209, shockwave :341 multi 2 (EFXGT owns the files; find the key of the 2 tuples in fxrest.d/cap.txt);
   chomp_drop :164 :167 unseen; disable_x :308 :310 unseen (unk_38 reaches 10 after about 90 frames, disable_x.c:150-160; the fxtest slot is 44,
   pm_fxtest.c:205: needs a longer slot for that effect); imgfx.c:1805 overlay list (monstar; capture through axns_sam2_drec).
5. gfxpos 10: main.c, main_loop.c, cam_main.c:29, variables.h:111, nuGfxTaskStart (nusys.h:790, pm_nusys.c:151): stage 4B/4C task start design.
6. 3ds/src walker gfxtype (nd_walk 33, nd_ent 29, nd_shape 20, nd_rec 15, nd_recs 15, nd_cen 7, nd_geom 1): goes with the walker deletion.
   A rename to another 8-byte type only hides the counter. Not paged.
7. include gbi.h, gbi_custom.h, gt.h, sp.h, gs2dex.h, gu.h: stage 4D.
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
1. Answer coder questions (IMGFXT, GTSMALL, FXDIG, VCFIX, BLANKET).
2. After VCFIX merges, page group 1 (model.c types). Then group 2.
3. Then the recdl keys of green_impact, red_impact, shockwave (after EFXGT merges): read fxrest.d/cap.txt for the 2 tuples.
