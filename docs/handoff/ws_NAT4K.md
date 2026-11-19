# ws_NAT4K (Haiku): core 1 records no marked combine class unit (OML, PRIM, ENV, FOG, BLEND, FILL, PRIMZ, RARE, COMB). The core 1 mark alone cuts on core 0. Counter: c0gate unchanged, expected `c0gate calls 124 interp 34 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE2. WAIT: `Merge NAT4H` and `Merge NAT4J`. Branch from main after both. Worktree build3ds/wt/NAT4K, branch NAT4K. PM_ROW=NAT4K PM_LANE=NAT4K.
Edit only: 3ds/src/nd_rec.c (the NAT4G mark line), 3ds/src/nd_walk.c (nr_set class block, nr_ctl MISC_EP case), 3ds/tools/nd_st_test.c (the run check).
## Facts (main 4260a4f73b plus NAT4H, NAT4I, NAT4J)
- FACT after NAT4H the combine class `ca_` of nr_set (nd_walk.c:248) is OML, PRIM..BLEND, FILL, PRIMZ, RARE, COMB. In a stream run core 0 only cuts on its mark: `if (ca_) { if (mk_) gv_epoch++; return; }` (:252). The mark (MISC_EP) is the unit directly before (nd_rec.c:415).
- FACT nr_ctl takes MISC_EP with no gv_epoch (:309 excludes it, :328 sets ep_mark). After NAT4J the stream scalar units that reach nr_set are the combine class and TIMG..LOADTLUT; core 1 marks every tile unit (rule 6, nd_ds.c:406).
- FACT nd_r11_walk (nd_model.c:287-292) refuses a range at the first mark (MISC). So a marked unit is never read there. An unmarked unit stays recorded: nd_r11_op reads OML, OMH, COMB, PRIM, ENV, BLEND, FOG (:267-271).
- FACT the units dropped here run only nd_unit_same on core 0 (its PRIM side effect pm_gpu_set_prim, nd_set.c:134, happens again at the adopt: ls_gpu nd_snap.c:153).
## Edits (changed lines end with `/* NAT4K */`)
1. nd_rec.c nd_rec_nr: replace the NAT4G mark line (`if ((id >= ND_FS_OMH && id <= ND_FS_COMB) || (id >= ND_FS_TIMG ...` up to `/* NAT4B: the cut and its reason */`) by:
```c
    if ((id >= ND_FS_OMH && id <= ND_FS_COMB) || (id >= ND_FS_TIMG && id <= ND_FS_LOADTLUT)) { /* NAT4G */ const unsigned rc = nd_ds_ep_rule(w0, ch, lost, dirty);
        if (rc) { st_put(ND_FS_W0(ND_FS_MISC, ND_FS_MISC_EP << 16), rc | (lost ? 0x100u : 0u) | (dirty ? 0x200u : 0u) | (ch & 1u) << 10); /* NAT4B: the cut and its reason */
            if (id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_PRIMZ) || id == ND_FS_RARE || id == ND_FS_COMB) return; } } /* NAT4K: a marked combine class unit is not recorded */
```
2. nd_walk.c nr_ctl, the MISC_EP case: `case ND_FS_MISC_EP: ep_mark = 1; ep_mw = b; nd_epk_mk++; gv_epoch++; break; /* NAT4B; NAT4K: the mark is the cut */`.
3. nd_walk.c nr_set, the class block: `if (ca_) { if (mk_) gv_epoch++; return; }` becomes `if (ca_) return; /* NAT4K: the mark cut */` and `cut = mk_;` becomes `cut = 0; /* NAT4K: the mark cut */`. The rest of the block stays.
4. nd_st_test.c, the NAT4J run check: the `rm` stream is now the mark only:
```c
    if (b.nw < 4u || b.w[0] != NDC_HDR(NDC_NAPI, 1, 0) || b.w[1] != (uint32_t)(uintptr_t)nd_nr_run || b.w[2] != ND_FS_W0(ND_FS_MISC, ND_FS_MISC_EP << 16)
        || b.nu != 2u || b.u[0] != b.w[2]) { printf("stunit FAIL run\n"); return 1; } /* NAT4K: a marked OML is not recorded */
```
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat4k_host.txt 2>&1`: PASS (stunit seeds: run seed FAIL run, nat seed FAIL nat). Commit `NAT4K 1: marked combine class units not recorded; the mark cuts`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat4k_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT4K --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT4K: the mark cuts, marked combine units dropped" > build3ds/out/nat4k_loop.txt 2>&1`.
4. `grep -H "c0src f\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT4K_cmp_*.txt > build3ds/out/nat4k_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT4K/pm_3ds.elf > build3ds/out/nat4k_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (4 drec equal). c0gate `calls 124 interp 34 state 23`. Fix rights: compile errors. A DIFF, a crash or a tool fault: stop, send nat4k_c0src.txt and the first DIFF line to oracle NATIVE2 with SendMessage, wait.
## Hand-back
Last commit `NAT4K 2: gate. 4 drec equal; the mark cuts` with `git add -f build3ds/out/nat4k_c0src.txt build3ds/out/nat4k_loop.txt`. It deletes this page. One SubagentHandback: hash, the gate line per bench, the c0gate line.
