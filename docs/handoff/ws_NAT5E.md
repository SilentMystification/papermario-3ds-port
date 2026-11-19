# ws_NAT5E (Haiku): core 1 writes a particle run (nd_parts_end) into the stream: a begin record, per instance its units, its snap and a draw record, then an end record. nd_parts_run, nd_parts_run_v and part_units (core 0 interpreters) go. Counter: c0gate calls -12 (14 sites in nd_walk.c:425-498 go, 2 come back in the new runners), interp 0 or -1 (part_units; report the value). Gate: drec byte-equal.
Designer: oracle NATIVE2. WAIT: none (NAT4I is on main: rec_nat nd_rec.c:408). Start commit main. Worktree build3ds/wt/NAT5E, branch NAT5E. PM_ROW=NAT5E PM_LANE=NAT5E.
Edit only: 3ds/src/nd_rec.c (nd_parts_end and the weak stubs, nothing in nd_rec_nr), 3ds/src/nd_walk.c (:415-498 only), 3ds/src/nd_geom.c (nd_part_draw :401-406), 3ds/src/nd_snap.c (one new function), 3ds/include/nd_api.h (:43-51). NAT4J and NAT4K edit other functions of nd_rec.c and nd_walk.c: merge them if git asks.
## Facts (main 8b25762dcd)
- FACT nd_parts_end (nd_rec.c:353-368) records one NAPI of all instances, then applies them to nd_ds and builds the instance snap table (vprim 1) in payload word 1. Users: effect_3D.c:238-259, embers.c:217-234.
- FACT nd_parts_run_v (nd_walk.c:474-498): start (pr_v, pm_walk_side++, nd_lsr_begin, gv_epoch++, dlc_on 1, gp_prev_op -1, pm_comb_vprim 1, sc_ok 0); per instance part_units(pre), `if (!nd_part_draw(t)) nd_ntab_run(t);`, part_units(post); end (nd_lsr_end, pm_comb_vprim 0, sc_ok 0, nd_comb_sync, pm_walk_side--, the 2 streak flushes). nd_parts_run (:425-448) is the same with nd_nr_exec for the units.
- FACT core 1 marks a PRIM or ENV on a change or on sc_lost (nd_ds.c:403-404), so `nd_ds.sc_lost = 1` at the begin gives the cuts of `sc_ok = 0` on core 0. TILESIZE is always marked (rule 6) and part_units always cuts it.
- FACT nd_part_draw takes the armed snap only with a table (nd_geom.c:402-404). draw_tri uses `nd_snap_rpend = nd_snap_arm | nd_ls_on` (:351).
## Edits (new lines end with `/* NAT5E */`)
1. nd_snap.c, after nd_lsc_begin_v (:83): `void nd_snap_vprim(unsigned v) { sn_vprim = v; sn_cmds = ~0u; } /* NAT5E: the particle plan of the next snaps; the next snap is new */`.
2. nd_rec.c: after rec_nat (:408-411) add `void nd_rec_nat(NdNapiRun run, uint32_t a, uint32_t b) { rec_open(); rec_nat(run, a, b); } /* NAT5E */`. Next to the weak stubs (:563) add `__attribute__((weak)) void nd_snap_vprim(unsigned v) { (void)v; } __attribute__((weak)) void nd_pbeg_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_pdraw_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_pend_run(const uint32_t* r) { (void)r; } /* NAT5E */` and above nd_parts_end the prototypes of these 5 names. Replace the body of nd_parts_end after `if (!b->n) return;` by:
```c
    const unsigned n = b->n, pre = b->pre, m = b->m; static const uint32_t z[2] = { 0u, 0u };
    nd_rec_napi(nd_pbeg_run, z, 1u); nd_ds.sc_lost = 1; nd_snap_vprim(1u); /* NAT5E: the run start (a command, as the old record): vprim 1, sc_ok 0 */
    for (unsigned k = 0; k < n; k++) { const uint32_t* p = b->u + 2u + 2u * m * k;
        for (unsigned j = 0; j < m; j++) { if (j == pre) { nd_snap_mesh(); nd_rec_nat(nd_pdraw_run, p[2u * j + 1u], 0u); } /* NAT5E: the instance snap and the list draw */
            else if (p[2u * j] != ND_NOP_A) nd_rec_nr(p[2u * j], p[2u * j + 1u]); } }
    nd_rec_nat(nd_pend_run, 0u, 0u); nd_snap_vprim(0u); nd_ds.sc_lost = 0; nd_ds.sc_dirty = 0; /* NAT5E: the run end (nd_comb_sync on core 0) */
    b->n = 0;
```
3. nd_walk.c: delete nd_parts_run (:425-448), the comment and body of part_units (:449-473) and nd_parts_run_v (:474-498). Keep pr_v, pr_old, pr_next, pr_cen, pr_lsr_note may go if unused (-Werror). Add:
```c
void nd_pbeg_run(const uint32_t* r) { (void)r; PM_WALK_OWN(); pr_v++; if (__builtin_expect(pm_frame_count() >= pr_next, 0)) pr_cen(); /* NAT5E: the start of the old parts runner */
    pm_walk_side++; gv_epoch++; dlc_on = 1; gp_prev_op = -1; extern int pm_comb_vprim; pm_comb_vprim = 1; sc_ok = 0; }
void nd_pdraw_run(const uint32_t* r) { const NdNDev* t = (const NdNDev*)(uintptr_t)r[0]; if (!nd_part_draw(t)) nd_ntab_run(t); } /* NAT5E: one instance list */
void nd_pend_run(const uint32_t* r) { (void)r; extern int pm_comb_vprim; pm_comb_vprim = 0; sc_ok = 0; nd_comb_sync(); pm_walk_side--; gv_streak_flush(); tex_streak_flush(); } /* NAT5E */
```
4. nd_geom.c nd_part_draw: `const unsigned char sn = nd_ls_on && nd_snap_arm;` becomes `const unsigned char sn = nd_snap_arm; /* NAT5E: the stream instance snap */`, and `nd_snap_rpend = nd_ls_on;` becomes `nd_snap_rpend = nd_snap_arm | nd_ls_on; /* NAT5E: as draw_tri */`.
5. nd_api.h: replace the 2 prototypes nd_parts_run, nd_parts_run_v (:50-51) by `void nd_pbeg_run(const uint32_t* r); void nd_pdraw_run(const uint32_t* r); void nd_pend_run(const uint32_t* r); /* NAT5E, nd_walk.c, core 0 */` and `void nd_rec_nat(NdNapiRun run, uint32_t a, uint32_t b); void nd_snap_vprim(unsigned v); /* NAT5E, core 1 */`; in the comment :45 replace `Core 0 (nd_parts_run)` by `Core 1 writes the instances into the stream (NAT5E); core 0`. Host test doubles that define nd_parts_run stay (unused).
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-5. `grep -rn "nd_parts_run\|part_units" 3ds/src 3ds/include src` prints nothing. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat5e_host.txt 2>&1`: PASS. Commit `NAT5E 1: particle runs written into the stream; core 0 parts runners deleted`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat5e_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT5E --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT5E: parts in the stream" > build3ds/out/nat5e_loop.txt 2>&1`.
4. `grep -H "partsrun\|partsnap\|c0src f\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT5E_cmp_*.txt > build3ds/out/nat5e_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT5E/pm_3ds.elf > build3ds/out/nat5e_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (4 drec equal). c0gate calls = base - 12, state 23; report interp. fxparts_drec has `partsrun` lines (the path ran). Fix rights: compile errors, unused static warnings, missing prototypes. A DIFF, a crash or a tool fault: stop, send nat5e_c0src.txt and the first DIFF line to oracle NATIVE2 with SendMessage, wait.
## Hand-back
Last commit `NAT5E 2: gate. 4 drec equal; parts in the stream; c0gate calls <c> interp <i>` with `git add -f build3ds/out/nat5e_c0src.txt build3ds/out/nat5e_loop.txt`. It deletes this page. One SubagentHandback: hash, the gate line per bench, the c0gate line.
