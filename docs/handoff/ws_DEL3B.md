# ws_DEL3B (Haiku): a compiled def record (nd_scomp_run) runs its units as ops plus the cut of the old run. The core 0 compiled run goes: sc_go, sc_exec, sc_decide, sc_replay, sc_block, sc_units, sc_omh, the decision cache and s6chk. Counter: none of C1-C12. It deletes the nd_scomp.c call sites of nd_comb_decide, comb_in_walker, op_setcombine, set_geom and nd_set_texture, so DEL2A-C can delete those functions. Gate: 2 drec byte-equal.
Designer: oracle DEL helper. Start commit main (2f28f3b558 or later). No WAIT. Shared files: DSDUP edits nd_scomp.c 163-165 (3 lines above this row's 169; no shared line). DEL3A (W9, nd_rec_nr) may edit rec_ops (nd_scomp.c:140-153): this row does not edit 139-168. DEL5B2 shares no file (it deletes the nd_scomp_ok call in nd_walk.c nd_ent_pieces; this row keeps nd_scomp_ok). Worktree build3ds/wt/DEL3B, branch DEL3B. PM_ROW=DEL3B PM_LANE=DEL3B.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_scomp.c (1-5, 127-137, 169-346), 3ds/tools/bench_defs.json (the 2 s6chk benches and legacy.names).
## Facts (code read, main 2f28f3b558; find the quoted text, line numbers move)
- FACT nd_scomp_run (nd_scomp.c:346) -> sc_go (:342) -> sc_exec (:260-267): the pre value units (nd_nr_exec), sc_block or sc_units (the static units, with the cached decision sc_decide :226), `sr_ok = 0; gv_epoch++; pm_gbi_cur_op = d->last_op;`, the post value units. Its only recorder is nd_scomp_rec (:162), called by ND_RM_REC (nd_rmdef.h:75) from animator.c, entity_model.c, item_entity.c.
- FACT a def has no value unit between 2 static units (nd_scomp_analyse :77). So the op path order below is the sc_exec order. NREC5 made the same change for list defs (gt_ops, entity gaps) and gated fxops and entcape drec byte-equal (37087ad978). GUESS: drec stays equal (a table miss of sync_combine gives the fixed fallback, sc_decide gave nd_comb_decide).
- FACT s6chk is a debug switch read only at :343. bench_defs.json has 2 benches of it: s6chk_mac01 (:3912) and s6chk_yard (:7262); their until line is the s6chk line.
## Edits
1. nd_scomp.c: delete :127-131 (the pm_gpu_flush macro, comb_in_walker, pm_tex_set_filter) and :133-137 (ls_use to tst_f). Keep :132 (nd_rec_nr). In the header comment (:2-4) replace the text from `on core 0 each run applies` to `for the static units.` with `on core 0 a record runs its units as ops in def order, then the cut of the old run (DEL3B).`
2. nd_scomp.c: replace from `/* ---- core 0: the run ---- */` (:169) to the line `void nd_scomp_run(const uint32_t* rec) { sc_go(...); }` (:346) with:
```c
/* ---- core 0: the run ---- */
/* DEL3B: a compiled record in the order of the old sc_exec: the pre value units, the static and no-op units of d in def order (nd_walk_state
 * skips a no-op), the cut and the render resync, the post value units. The static units take the walker decision (sync_combine), as a list def (NREC5). */
static __attribute__((cold, noinline)) void sr_note(void) { static unsigned char seen; if (!seen) { seen = 1; pm_log_evt("scomp run f%u", pm_frame_count()); } } /* DEL3B: the path ran */
void nd_scomp_run(const uint32_t* rec) {
    const NdSDef* d = (const NdSDef*)(uintptr_t)rec[0];
    const uint32_t* v = rec + 2, * t = (const uint32_t*)d->u;
    sr_note();
    if (d->pre) nd_nr_exec(v, d->pre);
    for (unsigned k = 0; k < d->n; k++) if (!(d->vmask >> k & 1u)) nd_nr_exec(t + 2u * k, 1u);
    sr_ok = 0; gv_epoch++; pm_gbi_cur_op = d->last_op; /* the cut of the old compiled run */
    if (d->nv > d->pre) nd_nr_exec(v + 2u * d->pre, d->nv - d->pre);
}
```
   Keep nd_scomp_ok and the `#endif` after it.
3. bench_defs.json: delete the 2 bench objects named s6chk_mac01 and s6chk_yard (each from its `{` to its `},`). Add `"s6chk_mac01",` and `"s6chk_yard",` to the list `legacy.names` (after `"warpmenu_sub",`).
4. `git grep -nE "(sc_exec|sc_go|sc_decide|sc_block|sc_units|sc_replay|sc_omh|s6_run|nd_comb_decide|op_setcombine|set_geom|nd_set_texture|comb_in_walker) *\(" -- 3ds/src/nd_scomp.c > build3ds/out/del3b_grep.txt`: empty (comments without a `(` stay).
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-4. `python 3ds/tools/bench.py list > build3ds/out/del3b_list.txt 2>&1`: exit 0. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del3b_host.txt 2>&1`: PASS. Commit `DEL3B 1: compiled def records run as units; the core 0 compiled run is gone`.
2. `python 3ds/tools/bench.py base entcape_kmr03_drec --commit $(git merge-base HEAD main) > build3ds/out/del3b_base1.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row DEL3B --bench entcape_kmr03_drec --no-host --why "DEL3B scomp units" > build3ds/out/del3b_loop1.txt 2>&1` (it builds). Then `grep -a "scomp run f" build3ds/azt/logs/entcape_kmr03_drec.txt > build3ds/out/del3b_note1.txt`.
3. The same 3 commands for spot_mac01_drec (del3b_base2.txt, del3b_loop2.txt, del3b_note2.txt).
## Gate (exact)
Host PASS. bench.py list exit 0. Both rowloops exit 0 (byte-equal). The grep of edit 4 is empty. At least one of del3b_note1.txt and del3b_note2.txt holds a `scomp run f` line (the path ran in a gate scene).
## Fix rights and stop
Fix rights: the 2 files, compile errors and unused names that this page deletes. Stop: a DIFF, no `scomp run` line in both scenes, a crash, a tool fault. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time. One bench per rowloop call. PM_CACHE=off only as a prefix of one command.
Hand-back: last commit `DEL3B 2: gate. entcape and spot_mac01 drec equal; scomp core 0 run deleted` deletes this page. One SubagentHandback: commit hash, the scomp run lines.
