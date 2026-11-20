# ws_T1FIX (Haiku): the tile 1 bind of a model miss reads the snap of its own record
Designer: oracle NATIVE17 (plan_perf.md section 6). Start now from main. Parallel with NC0S (other files). Worktree build3ds/wt/T1FIX, branch T1FIX.
PM_ROW=T1FIX PM_LANE=T1FIX. Lines: main d0483da1ba. Grep the quoted text if a line moved.
Why Haiku: one move of one line, one cold marker function, a fixed gate. No design read is left.
Objective: pm_gbi_bind_tile1 (nd_tex_set.c:27-35) picks tile 0 or 1 from the combine words of the record that runs.
Counter: none (a fault fix). c0gate stays 0 / 0 / 1. nd_c0 size stays.
## Fault (FACT code read, main d0483da1ba)
- nd_recs.c:206 calls pm_gbi_bind_tile1(qw). nd_tex_set.c:30 reads `nd_msnap_w()[MSW_CW0]` and `[MSW_CW1]`.
- nd_msnap_w (nd_msnap.c:35) returns ms_cur. nd_msnap_use(q) sets ms_cur at nd_recs.c:209, after the bind. So the bind reads the snap of the record
  that ran nd_msnap_use before (the last miss, or a nested record of the rec_range in rec_apply).
- nd_tex_set.c:29 (nd_msnap_bq(q)) and nd_recs.c:204 (nd_msnap_dec(q)) already read record q. Only the tile index (nd_cw_t1only) is wrong.
- Other ms_cur readers (nd_model_set.c:32, :39, pm_gpu.c:3843) run after :209. Moving the use before :204 changes none of them (FACT grep of nd_msnap_w, nd_msnap_gvc, nd_msnap_td0).
## Edits (nd_recs.c only)
1. Delete line 209 `nd_msnap_use((const uint32_t*)q); /* NREC1B: model_draw reads this snap */`.
2. After line 201 `rec_apply(q, mdone, mce);` insert:
   `const uint32_t* const pw_ = nd_msnap_w(); /* T1FIX: the snap of the record before (the old tile 1 source, marker only) */`
   `nd_msnap_use((const uint32_t*)q); /* NREC1B: model_draw reads this snap; T1FIX: before the tile 1 bind (nd_tex_set.c:30 reads it) */`
3. Line 206 before: `... &cb_))) t1 = pm_gbi_bind_tile1(qw);` after: `... &cb_))) { t1fix_note(pw_); t1 = pm_gbi_bind_tile1(qw); }`
4. After mmiss_note (line 56) add the marker of the expected-diff gate:
```c
static unsigned t1_fr = ~0u; /* T1FIX: the bind took the tile of the snap before: marker of the expected-diff gate */
static __attribute__((cold, noinline)) void t1fix_note(const uint32_t* pw) {
    const uint32_t* const mw = nd_msnap_w();
    if (!plan_active || !cur_plan.uses_tex1 || nd_cw_t1only(pw[MSW_CW0], pw[MSW_CW1]) == nd_cw_t1only(mw[MSW_CW0], mw[MSW_CW1])) return;
    const unsigned f = pm_frame_count(); if (f != t1_fr) { t1_fr = f; pm_log_evt("t1fix f%u", f); }
}
```
## Checklist (hw_guidelines.md 1, 6): rec_miss is cold; the marker is cold and runs per miss only. PICA: the same nd_tile_bind calls; only the tile index changes.
## Tests (build3ds/out/, last 20 lines). Gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=T1FIX sh 3ds/build.sh`; c0gate 0 / 0 / 1.
2. `bench.py run spot_mac01_drec,entcape_kmr03_drec --row T1FIX --why "T1FIX gate"`. Then per bench `bench.py cmp <b> --row T1FIX --expect t1fix`.
   PASS: byte-equal, or `expected-diff PASS` (a `t1fix f<m>` line at or before the frame of the first differing draw). Note `grep -c "t1fix f"` per log.
Fix rights: compile errors (an extern or include for plan_active, cur_plan, nd_cw_t1only). A DIFF with expected-diff FAIL, a crash: stop, send the path to oracle NATIVE17.
Stop: last commit `T1FIX 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hash, host, c0gate, the 2 cmp lines, t1fix line counts.
