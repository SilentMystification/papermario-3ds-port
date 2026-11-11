# Row HCCUT4: class B on core 0: the combine decision miss and the plan evaluators cold (byte-equal)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (oracle code read, main a3458b866; M3CEN 29bfe4297: class B runs 5016 B per frame in all three scenes, M1-hot 7.1 KB, target 3 KB)
- HC5 is closed (T_SYNC 99.3 %): class B stays on core 0, so it shrinks only by moving rare work out of the per-sync path.
- decide (3ds/src/nd_comb.c:380-432): the hot part is the key and the lookup (:381-390); the miss part (:391-431: a new Dec, cc_decode,
  pm_comb_build, 24 sample evaluations with cc_eval, cc_legacy, pm_comb_colours, pm_comb_eval) runs once per new combine key.
- Callers: cc_eval (:287) and cc_legacy (:205) only in decide's miss part; pm_comb_eval (pm_comb.c:497) only there; pm_comb_build
  (pm_comb.c:393) in decide's miss, nd_tmpl_build's miss (cold since HCCUT2) and ndplan_check (pm_gpu.c:3686, cold).
- dual_bind (nd_comb.c:624-660, 18-20 calls per frame): the block `if (key != aux_key || !pm_tex_aux_valid()) { ... }` (:633-642, a new
  tile 1 texture: flush, wrap, grab, pm_tex_load, set_aux) is rare.
- sync_combine_work (:540-563): the PG0 census block `if (gp_on() && !comb_dry) { ... }` (:541-546) runs only in phase census mode.
- nd_model_state_in (3ds/src/nd_model.c:310-388, class H, 3260 B, 1-7 calls per frame): the two inline debug prints (:316 m2_ block,
  :319-322 m3_ block, switch mdl0) sit in the refusal arms.
- Other rows: HUDRING (nd_geom.c), HCCUT2 (nd_tmpl.c, nd_rect.c, hot_order.ld:246, hotset.json), HCCUT3 (pm_gpu_gtrip, nd_set.c
  sync_render), HCCUT5 (nd_tmpl.c nd_tev_build, after HCCUT2). No other row edits nd_comb.c, pm_comb.c or nd_model.c now.
## Start
`sh 3ds/tools/wt_new.sh HCCUT4`; PM_ROW=HCCUT4 PM_LANE=HCCUT4. `git merge main` before the completing commit.
## Exact edits (same statements in the same order; only placement changes)
1. nd_comb.c: new `static __attribute__((cold, noinline)) Dec* decide_miss(int two, unsigned key) { <the block :391-431 unchanged,
   the outer braces dropped> }` above decide; in decide replace that block with `return decide_miss(two, key); /* HCCUT4 */`.
2. nd_comb.c: `void cc_legacy(` (:70 prototype and :205) and `void cc_eval(` (:287) get `__attribute__((cold))` before the type.
3. pm_comb.c: `int pm_comb_build(` (:393) and `void pm_comb_eval(` (:497) get `__attribute__((cold))` before the type.
4. nd_comb.c dual_bind: move the body of `if (key != aux_key || !pm_tex_aux_valid()) { ... }` into
   `static __attribute__((cold, noinline)) void dual_bind_load(unsigned key)` above dual_bind; call it as
   `if (key != aux_key || !pm_tex_aux_valid()) dual_bind_load(key); /* HCCUT4: a new tile 1 texture */`.
5. nd_comb.c sync_combine_work: move the PG0 block body into `static __attribute__((cold, noinline)) void scw_census(void)` (keep its
   `static unsigned last_sig`); call `if (gp_on() && !comb_dry) scw_census();`.
6. nd_model.c: move the m2_ statement (:316) into `static __attribute__((cold, noinline)) void mdl0_note2(const uint32_t* rec, const NdMCtx* c)`
   and the m3_ statement (:319-322) into `static __attribute__((cold, noinline)) void mdl0_note3(const uint32_t* rec)`, both above
   nd_model_state_in, with their statics; call them unconditionally in the same arms (the arms are refusals, rare).
## Gate (CLAUDE.md 2)
- build: memcheck OK. `bench.py run hotset_size --row HCCUT4 --lane HCCUT4 --why "class B cut"`: write M1hot B and H before and after
  (B expected about -3 KB; rc 1 only for load_impl while HCCUT2 is unmerged).
- `bench.py run nd_host --row HCCUT4 --why "HCCUT4"`: PASS.
- `bench.py base` mdl5_iwa10_det and spot_mac01_det; one `run mdl5_iwa10_det,spot_mac01_det`; `cmp` each: 6/6.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
