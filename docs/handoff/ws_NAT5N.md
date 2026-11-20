# ws_NAT5N (Sonnet: 3 files, a recursive range layout on core 1 and the same arms on core 0): a list called from inside a raw list alt range (NAT5F) gets its own nested raw alt range. Core 0 runs that range with today's executor in place of the static table. After this row, no list that a recorded list calls runs from a static table. Counter: c0gate calls 0 (enabler of NAT5T). Gate: drec byte-equal.
Designer: oracle NATIVE3 (agent a86ed4bedc3a8575e). WAIT: none. Start commit main. Worktree build3ds/wt/NAT5N, branch NAT5N. PM_ROW=NAT5N PM_LANE=NAT5N. Edit only: 3ds/src/nd_ds.c (ds_pairs), 3ds/src/nd_rec.c (after nd_alt_close), 3ds/include/nd_api.h (one line), 3ds/src/nd_walk.c (alt_raw, nd_nr_run, nr_call, nr_ctl CALL case, the NAT5W helper ent_dl_walk, gt_body). Owned lines: these functions only. NAT5A (nd_ent.c) and NAT5B (nd_recs.c, nd_bind_run) run in parallel; merge if git asks.
## Facts (main 0874e769f2)
- FACT ds_pairs (nd_ds.c:413-428) puts only depth 0 units into the raw alt range (:418). A CALL unit recurses with ds_list_at (:420), which takes the v2 entry nt->u, or the raw list to its END (:426-433).
- FACT nr_call (nd_walk.c:205-228) runs a called list from its static table: at ex_depth 0 `nd_nr_list(nt->u, nt->nu, 1)` or `nd_nr_list(next, ND_LIST_MAX, 1)`; deeper `nd_ntab_run(nt)` (ls_run with a table, else nd_nr_exec) or `nd_nr_exec(next, ND_LIST_MAX)` (no table: always nd_nr_exec). Its arms ent_gdl and nd_sg_try come first. A branch (a & 1) ends the run (:316). ND_OP_CALL_W0 is `ND_FS_W0(ND_FS_CALL, 0)` (nd_op.h:22): no other bit of a is set.
- FACT alt_raw (nd_walk.c:367) is declared after nr_call, and nd_nr_run (:368) takes ls_run when alt_raw && nd_ls_on. gt_body (:476) runs a top list alt.
## Edits (new lines end with `/* NAT5N */`)
1. nd_rec.c, after nd_alt_close:
```c
/* NAT5N, core 1: a call unit inside a raw alt range. A block {list word, a, b} in stream data, the unit CALL | 2 with w1 = the block,
 * then a jump over the nested range that ds_pairs fills with the called list; nd_alt_call_end closes it (blk[3] keeps the jump). */
uint32_t* nd_alt_call(uint32_t w0, uint32_t w1) {
    uint32_t* const blk = (uint32_t*)nd_rec_data_st(16u); blk[0] = w1; blk[1] = blk[2] = 0u;
    nd_alt_put(w0 | 2u, (uint32_t)(uintptr_t)blk); blk[3] = (uint32_t)(uintptr_t)nd_alt_open(); return blk; }
void nd_alt_call_end(uint32_t* blk) { nd_alt_close((uint32_t*)(uintptr_t)blk[3], blk + 1); }
```
   nd_api.h after the NAT5F line: `uint32_t* nd_alt_call(uint32_t w0, uint32_t w1); void nd_alt_call_end(uint32_t* blk); /* NAT5N */`.
2. nd_ds.c ds_pairs. :418 becomes `if (nd_alt_raw_on && (w0 >> 24) != ND_FS_CALL) nd_alt_put(w0, w1); /* NAT5F; NAT5N: every depth, a call opens its own range */`. The CALL branch becomes:
```c
        if ((w0 >> 24) == ND_FS_CALL) {
            uint32_t* const ac = nd_alt_raw_on ? nd_alt_call(w0, w1) : NULL; /* NAT5N */
            ds_list_at(s, w1, depth + 1u);
            if (ac) nd_alt_call_end(ac); /* NAT5N */
            if (w0 & 1u) return;
```
   Add a weak `uint32_t* nd_alt_call(uint32_t w0, uint32_t w1) { (void)w0; (void)w1; return NULL; } void nd_alt_call_end(uint32_t* b) { (void)b; }` next to the weak nd_alt_put (:16), both `__attribute__((weak))`.
3. nd_walk.c. Move the alt state above nr_call (after ex_depth :195) as `static unsigned char alt_mode; static void ls_run(const uint32_t* q, unsigned n); /* NAT5F; NAT5N: 0 stream, 1 nd_nr_exec, 2 ls_run */`. Delete the old :367 line. nd_nr_run becomes `{ const unsigned n = (rec[-2] >> 16) & 0xffu; if (alt_mode == 2u) ls_run(rec, n); else { ep_stream = alt_mode == 0u; nd_nr_exec(rec, n); ep_stream = 0; } }`. gt_body becomes `{ nd_gtr_ntab++; if (nd_ls_on) nd_lsr_geo(); const unsigned char m = alt_mode; alt_mode = nd_ls_on ? 2u : 1u; nd_alt_run(rec + 6); alt_mode = m; }`. The NAT5F form took ls_run exactly when nd_ls_on: the same rule.
4. nr_call gets a 5th parameter `const uint32_t* ab` (NULL: the static table). ent_dl_walk passes NULL. nr_ctl CALL: `nr_call(dbg, (const NdW2*)at, a & ~2u, (a & 2u) ? ((const uint32_t*)(uintptr_t)b)[0] : b, (a & 2u) ? (const uint32_t*)(uintptr_t)b + 1 : NULL); /* NAT5N */`. Keep `if (a & 1u) r = ~0u;`. The run step at the end of nr_call (the arms stay):
```c
    if (ab) { const unsigned char m = alt_mode; /* NAT5N: the nested alt range, with the executor the static table took */
        if (!ex_depth) { PM_WALK_OWN(); gv_epoch++; dlc_on = 1; gp_prev_op = -1; pm_walk_side++; ex_depth = 1; alt_mode = 1u; nd_alt_run(ab); ex_depth = 0; pm_walk_side--; gv_streak_flush(); tex_streak_flush(); } /* nd_nr_list keep 1 */
        else if (ex_depth < ND_LIST_DEPTH) { ex_depth++; if (nt && nd_ls_on) nd_lsr_geo(); alt_mode = (nt && nd_ls_on) ? 2u : 1u; nd_alt_run(ab); ex_depth--; }
        alt_mode = m; }
    else if (!ex_depth) { ... the old 2 lines ... }
```
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat5n_host.txt 2>&1`: PASS (an lkey census count: fix it in 3ds/tests/lkey/allow.txt). Commit `NAT5N 1: nested list alt ranges`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat5n_base_<B>.txt 2>&1`. 3. `sh 3ds/tools/rowloop.sh --row NAT5N --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT5N: nested list alt ranges" > build3ds/out/nat5n_loop.txt 2>&1`. 4. `grep -H "drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT5N_cmp_*.txt > build3ds/out/nat5n_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT5N/pm_3ds.elf > build3ds/out/nat5n_c0gate.txt 2>&1`. The base is the start-commit ELF of the bench base lane.
## Gate (exact): host PASS. Rowloop exit 0 (4 drec equal). c0gate calls, interp, state as base (report the values). Fix rights: compile errors, prototypes, the allow.txt count. A DIFF, a crash or a tool fault: stop, send nat5n_c0src.txt and the first DIFF line to oracle NATIVE3 (a86ed4bedc3a8575e), wait.
## Hand-back: last commit `NAT5N 2: gate. 4 drec equal; nested list alt ranges` with `git add -f build3ds/out/nat5n_c0src.txt build3ds/out/nat5n_loop.txt`. It deletes this page. One SubagentHandback: hash, gate line per bench, c0gate line.
