# ws_NAT4A (Haiku): measure the W3 cut rule: core 1 puts an ep mark before each combine-class stream unit that its rule cuts, core 0 counts where its own skip test disagrees (epchk). Counter: c0gate unchanged, expected `c0gate calls 122 interp 34 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE. Branch from main after `Merge NAT3D`. Worktree build3ds/wt/NAT4A, branch NAT4A. PM_ROW=NAT4A PM_LANE=NAT4A. No other row edits these files now.
Edit only: 3ds/include/nd_fs.h (line 48), 3ds/src/nd_rec.c (nd_rec_nr, main lines 405-412), 3ds/src/nd_walk.c (nr_set lines 235-244, nr_ctl line 291, nd_nr_run line 347), 3ds/src/pm_gpu.c (drec_end, after the c0diff line 634).
## Facts (main after NAT3D)
- FACT lists with a snap table already run without their combine units: ls_run skips OML, PRIM..BLEND, RARE, COMB (nd_walk.c:353), takes the cut from the core 1 ep flag (nd_lsr_tri nd_snap.c, ND_LS_EP) and the GPU calls from ls_gpu. Core 1 computes ep with nd_ds_ep_rule (nd_ds.c:402, called at :421-422).
- FACT stream state runs (runner nd_nr_run nd_walk.c:347) still run these units through nr_set: skip test nd_unit_same, else gv_epoch++ (the batch cut, nd_walk.c:235-244).
- FACT nr_ctl cuts on every ND_FS_MISC unit except MODVTX (nd_walk.c:291). The core 1 shadow (sh_drop nd_rec.c:88) resets on a MISC unit (:135), so a mark must bypass rec_op_t.
## Edits (new and changed lines end with `/* NAT4A */`)
1. nd_fs.h line 48: add `ND_FS_MISC_EP` as the last enum value (`..., ND_FS_MISC_CIMG, ND_FS_MISC_EP };`, comment `NAT4A: a core 1 cut mark, no state`).
2. nd_rec.c nd_rec_nr: replace `    nd_ds_unit(&nd_ds, w0, w1);` and `    rec_op_t(t);` with:
```c
    const unsigned id = w0 >> 24; const int lost = nd_ds.sc_lost, dirty = nd_ds.sc_dirty; /* NAT4A */
    const unsigned ch = nd_ds_apply(&nd_ds, w0, w1); /* NAT4A: nd_ds_unit's work, with the change bit */
    rec_open(); ndc_sw++; if (sh_drop(t)) return; /* rec_op_t, inline */
    if ((id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_BLEND) || id == ND_FS_RARE || id == ND_FS_COMB) && nd_ds_ep_rule(w0, ch, lost, dirty)) st_put(ND_FS_W0(ND_FS_MISC, ND_FS_MISC_EP << 16), 0u); /* NAT4A: the core 1 cut */
    st_put(t[0], t[1]);
```
3. nd_walk.c: a new line above nr_set: `unsigned nd_epk_n, nd_epk_bad, nd_epk_mk; static unsigned char ep_mark, ep_stream; /* NAT4A: epchk counters, a mark is pending, a stream run (nd_nr_run) runs */`.
   nr_set first line `    if (nd_unit_same(u)) return;` becomes:
   `    const int same = nd_unit_same(u); { const unsigned i_ = u[0] >> 24; if (ep_stream && ex_depth == 0 && (i_ == ND_FS_OML || (i_ >= ND_FS_PRIM && i_ <= ND_FS_BLEND) || i_ == ND_FS_RARE || i_ == ND_FS_COMB)) { nd_epk_n++; if ((same == 0) != (ep_mark != 0)) nd_epk_bad++; ep_mark = 0; } } /* NAT4A */`
   `    if (same) return;`
   (nr_set must sit below `static int ex_depth;` (line 195): it does.)
   nr_ctl line 291: the condition becomes `if (id != ND_FS_CALL && !(id == ND_FS_MISC && (sub == ND_FS_MISC_MODVTX || sub == ND_FS_MISC_EP))) { gv_epoch++; }` and in its `switch (sub)` add `case ND_FS_MISC_EP: ep_mark = 1; nd_epk_mk++; break; /* NAT4A: no cut yet (measure) */`.
   nd_nr_run (line 347) becomes `void nd_nr_run(const uint32_t* rec) { ep_stream = 1; nd_nr_exec(rec, (rec[-2] >> 16) & 0xffu); ep_stream = 0; } /* NAT4A */`.
4. pm_gpu.c drec_end, a new line after the c0diff line (634): `    { extern unsigned nd_epk_n, nd_epk_bad, nd_epk_mk; pm_log_evt("epchk f%u units %u bad %u marks %u", pm_frame_count(), nd_epk_n, nd_epk_bad, nd_epk_mk); } /* NAT4A */`
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat4a_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known). Commit `NAT4A 1: epchk`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat4a_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT4A --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT4A: epchk" > build3ds/out/nat4a_loop.txt 2>&1`.
4. `grep -H "epchk\|c0src f\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT4A_cmp_*.txt > build3ds/out/nat4a_epchk.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT4A/pm_3ds.elf > build3ds/out/nat4a_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (3 drec equal: a mark makes no cut). c0gate `calls 122 interp 34 state 23`. Each bench has one epchk line. Report units, bad, marks (not judged).
Fix rights: compile errors (ND_FS_W0, st_put, sh_drop, ndc_sw are nd_rec.c names). A DIFF or a tool fault: stop, send nat4a_epchk.txt to oracle NATIVE with SendMessage, wait.
## Hand-back
Last commit `NAT4A 2: gate. 3 drec equal; epchk <units/bad> x3` with `git add -f build3ds/out/nat4a_epchk.txt build3ds/out/nat4a_loop.txt`. It deletes this page. One SubagentHandback: hash, the 3 epchk lines.
