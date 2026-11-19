# ws_NAT4F (Haiku): OMH and TEXTURE stream units also cut only on a core 1 mark (their handler still runs when it changes a field: filter, cycle type, texture scale). Counter: c0gate unchanged, expected `c0gate calls 122 interp 34 state 23`. Gate: expected-diff (the UD-N5 epcut rule).
Designer: oracle NATIVE. WAIT: `Merge NAT4D`. Branch from main after it. Worktree build3ds/wt/NAT4F, branch NAT4F. PM_ROW=NAT4F PM_LANE=NAT4F.
Edit only: 3ds/src/nd_rec.c (nd_rec_nr, the NAT4A/NAT4B mark line), 3ds/src/nd_walk.c (nr_set, the NAT4C/NAT4D block). No other row edits these files then.
## Facts (main after NAT4D)
- FACT core 1 already has a rule for OMH and TEXTURE: nd_ds_ep_rule returns 1 on a change and 2 for OMH, TEXTURE with sc_lost or sc_dirty (nd_ds.c:402-404). The core 0 skip test of both reads sc_ok, TEXTURE also sc_sig_now() == sc_sig (nd_set.c:126, :139).
- FACT the handlers have core 0 effects that later steps read: OMH sets the filter (pm_tex_set_filter, nd_set.c:84-96) and the cycle type; TEXTURE sets tex_on, tex_ss, tex_ts (nd_tex_set.c:222-227), which the vertex load reads (nd_geom.c:498). So they still run on core 0 when they change a field.
- FACT UD-N5 (user yes): a cut that moves to the core 1 mark passes when the first DIFF frame logs `epcut f<n>` (epk_note, NAT4C).
## Edits (changed lines end with `/* NAT4F */`)
1. nd_rec.c, the mark line (NAT4B): the class test `id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_BLEND) || id == ND_FS_RARE || id == ND_FS_COMB` gets `|| id == ND_FS_OMH || id == ND_FS_TEXTURE` added.
2. nd_walk.c nr_set, the NAT4C/NAT4D block becomes (the count lines stay as they are):
```c
    { const unsigned i_ = u[0] >> 24; const int ca_ = i_ == ND_FS_OML || (i_ >= ND_FS_PRIM && i_ <= ND_FS_BLEND) || i_ == ND_FS_RARE || i_ == ND_FS_COMB, cb_ = i_ == ND_FS_OMH || i_ == ND_FS_TEXTURE;
      if (ep_stream && ex_depth == 0 && (ca_ || cb_)) {
        nd_epk_n++; if ((same == 0) != (ep_mark != 0)) { nd_epk_bad++; epk_note(i_, same, u); }
        const int mk_ = ep_mark != 0; ep_mark = 0;
        if (ca_) { if (mk_) gv_epoch++; return; } /* NAT4D: the combine class is not run */
        cut = mk_; } } /* NAT4F: OMH and TEXTURE cut on the mark and still run when they change a field */
    if (same && !cut) return;
```
   The lines after it stay: `if (cut) gv_epoch++;` and `if (!same) nd_unit_apply(u);`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat4f_host.txt 2>&1`: PASS. If a macro-against-native host test fails at a shape its seed did not ask for, add the NAT4A nd_ds restore line to its run function (geounit pattern). Commit `NAT4F 1: OMH and TEXTURE cut on the core 1 mark`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat4f_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT4F --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT4F: OMH TEXTURE core 1 cut" > build3ds/out/nat4f_loop.txt 2>&1`.
4. `grep -H "epchk\|epcut\|epbin\|c0src f\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT4F_cmp_*.txt > build3ds/out/nat4f_epchk.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT4F/pm_3ds.elf > build3ds/out/nat4f_c0gate.txt 2>&1`.
## Gate (exact, expected-diff rule)
Host PASS. For each bench: equal, or the first DIFF draw lies in a frame F with `epcut fF` (frame F = the `drec frame F seq s` line with the largest s at or below the DIFF seq). c0gate `calls 122 interp 34 state 23`.
Fix rights: compile errors, the host test restore line. A DIFF in a frame with no epcut line, a crash or a tool fault: stop, send nat4f_epchk.txt to oracle NATIVE with SendMessage, wait.
## Hand-back
Last commit `NAT4F 2: gate. expected-diff: <bench> ...` (the word `expected-diff` in it) with `git add -f build3ds/out/nat4f_epchk.txt build3ds/out/nat4f_loop.txt`. It deletes this page. One SubagentHandback: hash, the gate line per bench, the epbin lines.
