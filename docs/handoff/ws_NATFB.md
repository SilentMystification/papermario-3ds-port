# ws_NATFB (Sonnet, 3 files): the map shape group fallback runs on a native geometry run, not on the unit interpreter
READY. Start main 8288b39b57 or later. Oracle NATIVE10 (affae1ea40e24e4cf). Worktree build3ds/wt/NATFB, branch NATFB. PM_ROW=NATFB PM_LANE=NATFB.
Owns: 3ds/src/nd_geom.c (new function after nd_part_draw), 3ds/src/nd_shape.c (dlc_run_group, dlc_group_slow), 3ds/include/nd_int.h (1 prototype). No NAT5G file.
Counter C13: c0gate calls 82 to 80 (FACT 82: c0gate on a clean git archive of 8288b39b57). Interp 32 and state 23 stay (set_geom and nd_nr_list keep other callers: nd_set.c:165, nd_walk.c:231). Gate: drec byte-equal.
## Facts
- FACT a compiled shape list holds only VTX, TRI1, TRI2, GEOM and sync units (nd_dlc.c:81-127; any other unit gives NDR_OP). So a group (op0..op1) holds only these.
- FACT dlc_group_slow (nd_shape.c:867-872) runs a group with nd_nr_list keep 0 (nd_walk.c:397-412): list start, nd_nr_exec (sync skip, nr_geo :272-304, GEOM by nr_set :253-269), list end. ex_depth >= 1 there: the ep_stream arm of nr_set does not run.
- The debug op hook (walk_dbg_op) and ops_cen_end are prof only. Drop them, as spr_geo_walk does (nd_walk.c:414-418, NAT5W).
## Edits (new lines end with `/* NATFB */`)
1. nd_geom.c, insert before `void nd_geom_vtx(u32 w0, u32 w1, const NdW2* list)`:
```c
void nd_geo_units(const uint32_t* u, unsigned n) { /* NATFB: flat geometry units with the arms of nr_geo and the GEOM arm of nr_set (nd_walk.c) */
    for (const uint32_t* q = u, *e = u + 2u * n; q < e; q += 2) {
        const unsigned id = q[0] >> 24; const u32 a = q[0] & 0xffffffu, b = q[1];
        if (q[0] == ND_NOP_A) continue;
        if (id == ND_FS_END) break;
        if (id == ND_FS_GEOM) { const u32 g = (nd_ws.geom & a) | b; const int z = (g & G_ZBUFFER) != 0, l = (g & G_LIGHTING) != 0;
            if (g == nd_ws.geom && pm_gpu_mode_is(z, l)) continue; /* nd_unit_same */
            pm_gbi_cur_op = id; gv_epoch++; nd_ws.geom = g; pm_gpu_set_mode(z, l); continue; } /* nr_set cut, set_geom */
        pm_gbi_cur_op = id;
        if (id == ND_FS_VTX) nd_geom_vtx(a, b, (const NdW2*)q);
        else { tri_prep_ok = 0; draw_tri(a); if (id == ND_FS_TRI2) draw_tri(b); } }
    if (__builtin_expect(pm_walk_dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1); }
```
2. nd_int.h:274, after `void draw_tri(unsigned w);` add `void nd_geo_units(const uint32_t* u, unsigned n); /* NATFB, nd_geom.c */`.
3. nd_shape.c:633 (dlc_run_group, the body of set_geom nd_set.c:33-36). before: `set_geom(w0, w1);`. after: `nd_ws.geom = (nd_ws.geom & (w0 & 0xffffffu)) | w1; pm_gpu_set_mode((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0); /* NATFB: set_geom in place */`. The `gv_epoch++;` line after it stays.
4. nd_shape.c:871. before: `nd_nr_list(l + 2u * g->op0, g->op1 - g->op0, 0); /* D10b: a flat range on the op executor */`. after:
```c
    PM_WALK_OWN(); gv_epoch++; dlc_on = 1; gp_prev_op = -1; sc_ok = 0; sr_ok = 0; pm_walk_side++; /* NATFB: the list start of nd_nr_list keep 0 */
    nd_geo_units(l + 2u * g->op0, g->op1 - g->op0); /* NATFB */
    pm_walk_side--; gv_streak_flush(); tex_streak_flush(); /* NATFB: its list end */
```
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, foreground only; 1 build, at most 8 Azahar starts)
1. Edits 1-4. Commit `NATFB 1: map shape group fallback on the native geometry run`.
2. `python 3ds/tools/bench.py base spot_mac01_drec > build3ds/out/natfb_base.txt 2>&1`, then the same for entcape_kmr03_drec (append `>>`).
3. `sh 3ds/tools/rowloop.sh --row NATFB --bench spot_mac01_drec,entcape_kmr03_drec --why "NATFB: shape group fallback native" > build3ds/out/natfb_loop.txt 2>&1`.
4. `python 3ds/tools/c0gate.py build3ds/lane-NATFB/pm_3ds.elf > build3ds/out/natfb_c0.txt 2>&1`. Expect `c0gate calls 80 interp 32 state 23` and no nd_shape.c file line.
## Gate (exact): LOOP PASS (host PASS, build OK, 2 drec byte-equal) and the c0gate line of step 4. Fix rights: compile errors, missing declarations, host stubs, the count in 3ds/tests/lkey/allow.txt.
A DIFF, a crash, other c0gate numbers or a tool fault: stop. SendMessage the output path and the first DIFF line to oracle NATIVE10 (affae1ea40e24e4cf). Wait for the reply.
## Hand-back: last commit `NATFB 2: gate. <LOOP lines> <c0gate line>` with `git add -f build3ds/out/natfb_loop.txt build3ds/out/natfb_c0.txt`. It deletes this page. One SubagentHandback: hashes, cmp line per bench, c0gate line.
