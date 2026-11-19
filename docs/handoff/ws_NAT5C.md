# ws_NAT5C (Haiku): core 1 records a compiled def (nd_scomp_rec) as stream units in the order of nd_scomp_run, plus one cut record. nd_scomp_run (core 0 interpreter of the def) goes. Counter: c0gate calls -3, interp -1 (from 122 34 23 on main 4260a4f73b: `c0gate calls 119 interp 33 state 23`; with NAT4J merged first: `calls 121 interp 33`). Gate: drec byte-equal.
Designer: oracle NATIVE2. WAIT: none. Start commit main. Worktree build3ds/wt/NAT5C, branch NAT5C. PM_ROW=NAT5C PM_LANE=NAT5C.
Edit only: 3ds/src/nd_scomp.c (the target part, from `#ifndef ND_SCOMP_HOST` :118), 3ds/include/nd_scomp.h. No other row edits these files. Every host build of nd_scomp.c sets ND_SCOMP_HOST (nd_host_test.sh:242, conv2c.sh, fltgt.sh, modelrm.sh, vrom.sh): the target part is not in a host link.
## Facts (main 4260a4f73b)
- FACT nd_scomp_rec (nd_scomp.c:142-153): a def the compiler takes (st 1) is one NDC_NAPI record of nd_scomp_run (:150) and core 1 applies its units with nd_ds_scomp (:152). A refused def goes to rec_ops (:128-141): each unit by nd_rec_nr (that also applies it to nd_ds).
- FACT nd_scomp_run (:159-167) runs on core 0: the pre value units (nd_nr_exec v, d->pre), each static unit in def order (`!(d->vmask >> k & 1u)`), then `sr_ok = 0; gv_epoch++; pm_gbi_cur_op = d->last_op;`, then the post value units. nd_nr_exec skips a unit whose w0 is ND_NOP_A (nd_walk.c:353).
- FACT a def holds no draw, list or rectangle unit (unit_class :16-28 refuses them). Users: ND_RM_REC (nd_rmdef.h:75, nv 0) in animator.c, cam_main.c, entity_model.c, item_entity.c, status_icons.c.
- FACT nd_rec_napi (nd_rec.c:283-292) is a command: it resets the shadow, disarms the snap and changes nd_rec_cmds, as the old record did. So the next triangle takes a new snap, as before.
## Edits (new lines end with `/* NAT5C */`)
1. nd_scomp.c nd_scomp_rec: delete the line `uint32_t p[2u + 2u * 32u];`. Replace the lines from `p[0] = (uint32_t)(uintptr_t)d;` to the `nd_ds_scomp(...)` line (:147-152, keep the `scomp rec` log line) by:
```c
    for (unsigned j = 0; j < d->pre; j++) if (v[2u * j] != ND_NOP_A) nd_rec_nr(v[2u * j], v[2u * j + 1u]); /* NAT5C: the order of the old nd_scomp_run: the pre value units */
    { const uint32_t* t = (const uint32_t*)d->u; for (unsigned k = 0; k < d->n; k++) if (!(d->vmask >> k & 1u) && t[2u * k] != ND_NOP_A) nd_rec_nr(t[2u * k], t[2u * k + 1u]); } /* NAT5C: the static units */
    { const uint32_t c[2] = { (uint32_t)d->last_op, 0u }; nd_rec_napi(nd_scut_run, c, 1u); } /* NAT5C: the cut and the render resync of the old run (a command, as the old record) */
    for (unsigned j = d->pre; j < nv; j++) if (v[2u * j] != ND_NOP_A) nd_rec_nr(v[2u * j], v[2u * j + 1u]); /* NAT5C: the post value units */
```
   The units go through nd_rec_nr, which applies them to nd_ds: do not call nd_ds_scomp here.
2. nd_scomp.c core 0 part: replace `sr_note` (:158) and the whole `void nd_scomp_run(...) { ... }` (:159-167) by:
```c
void nd_scut_run(const uint32_t* r) { sr_ok = 0; gv_epoch++; pm_gbi_cur_op = r[0]; } /* NAT5C: the cut and the render resync of the old compiled run (DEL3B) */
```
3. nd_scomp.h: replace `void nd_scomp_run(const uint32_t* rec);` (:41) by `void nd_scut_run(const uint32_t* r); /* NAT5C, core 0: the cut record of a compiled def */`. In the comment at :5 replace `core 0 (nd_scomp_run, an NDC_NAPI runner)` by `core 0 (its units in the stream, then nd_scut_run)`.
4. `grep -rn "nd_scomp_run" 3ds/src 3ds/include src` must print nothing (c0gate_names.txt keeps the name: its interp count drops to 0 for it).
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat5c_host.txt 2>&1`: PASS. Commit `NAT5C 1: compiled defs as stream units and a cut record; nd_scomp_run deleted`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat5c_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT5C --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT5C: defs as stream units" > build3ds/out/nat5c_loop.txt 2>&1`.
4. `grep -H "scomp rec\|c0src f\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT5C_cmp_*.txt > build3ds/out/nat5c_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT5C/pm_3ds.elf > build3ds/out/nat5c_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (4 drec equal). c0gate calls = your base calls - 3, interp = base interp - 1, state 23. Report the `scomp rec` lines (they prove the path ran). Fix rights: compile errors, missing prototypes. A DIFF, a crash or a tool fault: stop, send nat5c_c0src.txt and the first DIFF line to oracle NATIVE2 with SendMessage, wait.
## Hand-back
Last commit `NAT5C 2: gate. 4 drec equal; nd_scomp_run deleted; c0gate calls <c> interp <i>` with `git add -f build3ds/out/nat5c_c0src.txt build3ds/out/nat5c_loop.txt`. It deletes this page. One SubagentHandback: hash, the gate line per bench, the c0gate line, the scomp rec lines.
