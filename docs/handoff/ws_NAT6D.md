# ws_NAT6D (DRAFT, Sonnet): copy units apply with no core 0 skip test; then the interpreter body goes
DRAFT: no coder starts this page. The oracle releases it after the merges below, with exact lines and before:/after: blocks from main. Designer: oracle NATIVE9.
## WAIT (real data dependencies)
- `Merge NAT6B`. NAT6D reads its gate data: the c6cut count per bench and the expected-diff result. The nd_cpy_run record, ND_C6_MARK and nd_alt_mark come from NAT6B. NAT6D changes nd_cpy_run.
- `Merge NAT6C`. After NAT6A and NAT6C the stream holds no state unit for nr_set (FACT plan: only END and DROP stay in NRUN). NAT6D deletes the stream path of nr_set (the ep block, nd_walk.c:255-260 on e5ae433d74) only then.
- `Merge NATRS`. NATRS deletes nd_list_run and changes nd_range_run (nd_walk.c :402, :546-555). The NAT6D deletion list must not hold them twice.
## Design (from the code on main e5ae433d74; numbers GUESS until the refresh)
1. NAT6D-1 (expected-diff UD-N5, marker `c6app f<n>`). In a copy, core 1 decides the apply too. After the apply in nd_alt_mark, core 1 writes a second slot bit: apply when nd_ds_apply gave a change bit (ch & 1), or when rule 2 (lost or dirty) fired. nd_cpy_run then calls nd_unit_apply when that bit is set, and the cut from the mark. It does not call nd_unit_same. For 1 row, core 0 still computes nd_unit_same in a cold compare only, and logs `c6app f<n>` once per drec frame where it differs. The next row deletes the compare.
2. NAT6D-2 (byte-equal). Delete the compare of step 1, the epk counters and logs (epk_note, nd_epk_log, epbad, epcut) and the ep block of nr_set. nr_set then serves only the static table runs.
3. Outside NAT6D: the static table runs still call the interpreter. FACT call sites on e5ae433d74: nr_call nd_walk.c:231 (nd_nr_list), :232 (nd_ntab_run, nd_nr_exec), gt_body :502 (nd_ntab_run), nd_nr_list :413 and nd_ent_units :438 (nd_nr_exec), nd_ent.c:417 (nd_ent_units), nd_pdraw_run (nd_ntab_run). These need a copy (an alt range) before nd_nr_exec, ls_run, nr_set, nd_unit_same and nd_unit_apply can go. That is a separate row: the fallback-runner chain (NATFB, NATIVE10, NATIVE11).
4. The end state of W5 (plan_native.md): nd_ws loses geom, tss, segbase, lights and matrices to a native struct, and the F3 globals go with nd_comb_sync. Those rows follow 3.
## Counter (GUESS)
C13 calls: NAT6D-1 about +1 (nd_unit_apply in nd_cpy_run), NAT6D-2 about -3 (nd_unit_same and the 2 epk sites). interp and state do not change until 3 is done.
## Gate (planned)
NAT6D-1: host PASS (nat5a case: the apply bit after an unchanged unit is 0). entcape_kmr03_drec and fxparts_drec are byte-equal, or the first DIFF frame F has a `c6app fF` line. NAT6D-2: byte-equal on the same 2 benches.
