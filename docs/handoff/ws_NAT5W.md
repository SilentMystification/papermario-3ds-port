# ws_NAT5W (Haiku): three nd_walk.c runs of one or two known units go native: the sprite fallback words, the G_DL unit of a refused entity record, the list mark of a state-only list. Counter: c0gate calls -3 (nd_walk.c:169, :186, :475). Gate: drec byte-equal.
Designer: oracle NATIVE3 (agent a86ed4bedc3a8575e). WAIT: none. Start commit main. Worktree build3ds/wt/NAT5W, branch NAT5W. PM_ROW=NAT5W PM_LANE=NAT5W. Edit only: 3ds/src/nd_walk.c. NAT5R runs at the same time on other files.
## Facts (main 648d3d0335)
- FACT the sprite fallback runs `nd_nr_list(q + G_PM_SPR_WORDS, 2u, 1)` (nd_walk.c:169). The 2 units are always `ND_OP_VTX_W0(4, 0)`, vertices and `ND_FS_W0(ND_FS_TRI2, ...)`, a second index word (src/sprite.c:314-317).
- FACT nd_nr_list with keep 1 (:387-401): PM_WALK_OWN, gv_epoch++, dlc_on = 1, gp_prev_op = -1, pm_walk_side++, ex_depth = d + 1, nd_nr_exec, ex_depth = d, pm_walk_side--, gv_streak_flush, tex_streak_flush. nr_geo (:265-296): pm_gbi_cur_op = id; VTX: nd_geom_vtx(a, b, at); TRI2: tri_prep_ok = 0, draw_tri(a), draw_tri(b) (a = w0 & 0xffffff, b = w1). nr_ctl CALL (:301-316): pm_gbi_cur_op = ND_FS_CALL, no gv_epoch, nr_call(dbg, at, a, b).
- FACT the entity fallback (:186) runs the one CALL unit (nd_rec.c rec_ent_draw: ND_OP_CALL_W0) with nd_nr_list keep 1. The state-only list (:475) runs the unit MISC_LST: nr_ctl sets pm_gbi_cur_op = G_PM_LST, gv_epoch++, ent_skipgen = 0, gp_prev_op = -1 (:306-324).
- FACT nr_call (:204) and ex_depth (:195) are defined after nd_walk_ent (:174): the new helpers go after nd_nr_list (:401), with prototypes above nd_walk_spr (:158).
## Edits (new lines end with `/* NAT5W */`)
1. After nd_nr_list add the two helpers. They are the steps of nd_nr_list keep 1 with the arm of the unit inline:
```c
/* NAT5W: the sprite fallback words VTX(4, 0) and TRI2 (sprite.c:314-317): the list start and end of nd_nr_list (keep 1), the arms of nr_geo. */
static void spr_geo_walk(const uint32_t* g) {
    PM_WALK_OWN(); const int d = ex_depth; gv_epoch++; dlc_on = 1; gp_prev_op = -1; pm_walk_side++; ex_depth = d + 1;
    pm_gbi_cur_op = ND_FS_VTX; nd_geom_vtx(g[0] & 0xffffffu, g[1], (const NdW2*)g);
    pm_gbi_cur_op = ND_FS_TRI2; tri_prep_ok = 0; draw_tri(g[2] & 0xffffffu); draw_tri(g[3]);
    ex_depth = d; pm_walk_side--; gv_streak_flush(); tex_streak_flush(); }
/* NAT5W: the G_DL unit of a refused entity record: the list start and end of nd_nr_list (keep 1), the CALL arm of nr_ctl. */
static void ent_dl_walk(unsigned dbg, const NdW2* c) {
    PM_WALK_OWN(); const int d = ex_depth; gv_epoch++; dlc_on = 1; gp_prev_op = -1; pm_walk_side++; ex_depth = d + 1;
    pm_gbi_cur_op = ND_FS_CALL; nr_call(dbg, c, c->words.w0 & 0xffffffu, c->words.w1);
    ex_depth = d; pm_walk_side--; gv_streak_flush(); tex_streak_flush(); }
```
2. Above nd_walk_spr add `static void spr_geo_walk(const uint32_t* g); static void ent_dl_walk(unsigned dbg, const NdW2* c); /* NAT5W */`.
3. Before and after, nd_walk.c:169:
```c
    { extern int nd_spr_walk; nd_spr_walk = 1; nd_nr_list(q + G_PM_SPR_WORDS, 2u, 1); nd_spr_walk = 0; } /* WALLSHADE */
    { extern int nd_spr_walk; nd_spr_walk = 1; spr_geo_walk((const uint32_t*)(q + G_PM_SPR_WORDS)); nd_spr_walk = 0; } /* WALLSHADE; NAT5W */
```
4. nd_walk.c:186: `nd_nr_list(q + G_PM_ENT_WORDS, 1u, 1);` becomes `ent_dl_walk(dbg, q + G_PM_ENT_WORDS); /* NAT5W */`.
5. gt_run0 state-only block (:473-475): delete `static const uint32_t lst[2] = ...;` and replace `nd_nr_exec(lst, 1u);` by `pm_gbi_cur_op = G_PM_LST; gv_epoch++; ent_skipgen = 0; gp_prev_op = -1; /* NAT5W: the MISC_LST unit (nr_ctl) */`.
The walker debug hooks of these units (walk_dbg_op, the class span end) go with them: prof attribution only, no draw.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-5. `grep -n "nd_nr_list(q + \|nd_nr_exec(lst" 3ds/src/nd_walk.c` prints nothing. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat5w_host.txt 2>&1`: PASS. Commit `NAT5W 1: sprite, entity G_DL and state-only list mark run natively`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat5w_base_<B>.txt 2>&1`. 3. `sh 3ds/tools/rowloop.sh --row NAT5W --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT5W: native unit runs" > build3ds/out/nat5w_loop.txt 2>&1`.
4. `grep -H "drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT5W_cmp_*.txt > build3ds/out/nat5w_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT5W/pm_3ds.elf > build3ds/out/nat5w_c0gate.txt 2>&1` (base: the same command on the start commit in the main folder).
## Gate (exact): host PASS. Rowloop exit 0 (4 drec equal). c0gate calls = base - 3, interp and state as base. Fix rights: compile errors, unused statics, prototypes. A DIFF, a crash or a tool fault: stop, send nat5w_c0src.txt and the first DIFF line to oracle NATIVE3 (a86ed4bedc3a8575e) with SendMessage, wait.
## Hand-back: last commit `NAT5W 2: gate. 4 drec equal; native unit runs` with `git add -f build3ds/out/nat5w_c0src.txt build3ds/out/nat5w_loop.txt`. It deletes this page. One SubagentHandback: hash, gate line per bench, c0gate line.
