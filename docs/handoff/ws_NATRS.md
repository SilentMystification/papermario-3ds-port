# ws_NATRS (Haiku): the R11 range calls leave the interpreter count; rec_dl, nd_list_run and nomodelnat go
READY. Start commit: main. Oracle NATIVE11 (a24e60c166c1d5e70). Worktree build3ds/wt/NATRS, branch NATRS. PM_ROW=NATRS PM_LANE=NATRS. WAIT: none.
Regions: 3ds/src/nd_recs.c (:26-35, :50, :63, :237, :249-250, :279); 3ds/src/nd_walk.c (nd_list_run :402, nd_range_run :546-555); 3ds/include/nd.h:554; 3ds/tools/c0gate_names.txt:8; 3ds/tests/lkey/allow.txt:3. NAT6A inserts after nd_walk.c:383 (no overlap). Line numbers are main e5ae433d74: find each line by its text.
Counter C13: c0gate calls -5 (FACT: nd_recs.c :50, :250, :279 leave the names; nd_recs.c:34 and the nd_nr_list call in nd_walk.c:402 are deleted). Interp -2 (FACT: FUNC nd_range_run and nd_list_run in build3ds/pm_3ds.elf of 09abf0a661). State 0. FACT main e5ae433d74: 80 / 32 / 23, so expect 75 / 30 / 23.
## Facts (the proof)
- FACT rec_dl runs only at nd_recs.c:249 under `!nat && (ndm || nmn_sw())`. ndm reads switch ndmcheck (pm_gpu.c:3784), nmn_sw reads nomodelnat (:63). No drec scenario in 3ds/tools/azt.d/*.json sets either switch. So no drec bench runs rec_dl, and `!nmn_sw() && X` is X there.
- FACT nd_range_run (nd_walk.c:548-565) writes `sc_ok = 0; sr_ok = 0;` at :555. The writes before it (ex_depth, gv_epoch, dlc_on, gp_prev_op) and PM_WALK_OWN (pm_port.h:143, nd_wq.c:198) read neither. nd_sr_forget is `sr_ok = 0` (nd_set.c:197). The only callers are nd_recs.c :50, :250, :279. So the same writes in the caller, just before the call, are exact. :278 already writes both before :279.
- Coverage: FACT kkj_14 main.c:211-212 and isk_01 haze.c:10-11 set a pre range (R11 w0) at map load, post nullptr. GUESS: the model draws in frames 200-201. OPEN: no bench is known for a post range (:250, :279). Those 2 edits are exact by the fact above.
- After the row, ndmcheck keeps its bracket (:179, :182, :185, :235, :252, :263) and its ndmcache compare. A record that ndmcheck takes draws no geometry. This is a debug switch only.
## Edits (new or changed lines end with `/* NATRS */`)
1. nd_recs.c: replace :26-35 (the comment `/* S3G-1: a model list of a record ...` and the whole rec_dl function) with:
```c
static unsigned rng_seen; /* NATRS: bit 0 pre, bit 1 post: the first run of each kind logs rngrun once (coverage proof) */
static void rec_range(const uint32_t* a, unsigned k) { /* NATRS: an R11 custom gfx range; the skip state reset of the old nd_range_run list start */
    sc_ok = 0; nd_sr_forget();
    if (__builtin_expect(!(rng_seen & k), 0)) { rng_seen |= k; pm_log_evt("rngrun f%u %s", pm_frame_count(), k == 1u ? "pre" : "post"); }
    nd_range_run(a);
}
```
2. nd_recs.c :50, :250, :279. Example :50. before: `    if (q[11].words.w0) nd_range_run((const uint32_t*)(uintptr_t)q[11].words.w0); /* S3G-1: custom gfx pre, ...` after: `    if (q[11].words.w0) rec_range((const uint32_t*)(uintptr_t)q[11].words.w0, 1u); /* S3G-1: custom gfx pre, ... NATRS */`. :250 and :279 use `q[11].words.w1` and `2u`. Keep :278.
3. nd_recs.c: delete :63 (`static int nmn_sw(void) ...`) and :249 (`if (!nat && (ndm || nmn_sw())) rec_dl(...`). :237 before: `    else if (!nmn_sw() && pm_gpu_model_native((const uint32_t*)qs, mwp,` after: `    else if (pm_gpu_model_native((const uint32_t*)qs, mwp, /* NATRS */`.
4. nd_walk.c: delete :402 (`void nd_list_run(const void* l) { nd_nr_list(l, ND_LIST_MAX, 0); }`) and :555 (`    sc_ok = 0; sr_ok = 0;` inside nd_range_run only). In the comment at :547, replace `a full combine and render sync: DIP2` with `the caller resets the skip state: nd_recs.c rec_range, NATRS`.
5. nd.h: delete :554 (`void nd_list_run(const void* l); ...`). c0gate_names.txt: delete line 8 `interp nd_range_run`. Keep `interp nd_list_run`.
6. 3ds/tests/lkey/allow.txt:3: `3ds/src/nd_recs.c 45` becomes `44` (FACT: :249 was a `words.w0` reader line, lkey_cen.py R_WORD).
Use the Edit tool (no `sed -i`: CRLF files).
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, foreground only; 1 build, at most 8 Azahar starts)
1. Edits 1-6. Commit `NATRS 1: R11 range reset in the caller; rec_dl, nd_list_run, nomodelnat deleted`.
2. `python 3ds/tools/bench.py base kkj14_cgfx_drec > build3ds/out/natrs_base.txt 2>&1`, then the same for isk01_cgfx_drec (append `>>`).
3. `sh 3ds/tools/rowloop.sh --row NATRS --bench kkj14_cgfx_drec,isk01_cgfx_drec --why "NATRS: R11 range reset, rec_dl gone" > build3ds/out/natrs_loop.txt 2>&1`.
4. `grep -H "rngrun f" build3ds/azt/logs/kkj14_cgfx_drec.txt build3ds/azt/logs/isk01_cgfx_drec.txt > build3ds/out/natrs_rng.txt`.
5. One script: `python 3ds/tools/c0gate.py build3ds/lane-NATRS/pm_3ds.elf > build3ds/out/natrs_c0.txt 2>&1`. Then extract `git archive $(git merge-base HEAD main)` to build3ds/out/natrs_src and run `python build3ds/out/natrs_src/3ds/tools/c0gate.py c:/Users/Andrew/Documents/GitHub/papermario-3ds-port/build3ds/pm_3ds.elf >> build3ds/out/natrs_c0.txt 2>&1` (the base line).
## Gate (exact): LOOP PASS (host PASS, build OK, 2 drec cmp byte-equal); natrs_rng.txt has a `rngrun f<n> pre` line in each log; c0gate head minus base: calls -5, interp -2, state 0.
Fix rights: compile errors, unused-declaration warnings in nd_recs.c, the allow.txt count. A DIFF, a crash, a missing rngrun line, another c0gate delta or a tool fault: stop. SendMessage the output path and the first DIFF line to oracle NATIVE11 (a24e60c166c1d5e70). Wait for the reply.
## Hand-back: last commit `NATRS 2: gate. <LOOP lines> <rngrun lines> <c0gate lines>` with `git add -f build3ds/out/natrs_loop.txt build3ds/out/natrs_rng.txt build3ds/out/natrs_c0.txt`. It deletes this page. One SubagentHandback: hashes, cmp line per bench, rngrun lines, c0gate lines.
