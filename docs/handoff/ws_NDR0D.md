# ws_NDR0D (Haiku): the snap folds the box combine, core 1 decides a table miss with its own table and no core 0 global (UD4) (stage S4; counter: snapchk bad)
Designer: oracle FIN. Branch from main (NDR0B, NDR0C merged). Worktree build3ds/wt/NDR0D, branch NDR0D. PM_ROW=NDR0D PM_LANE=NDR0D. Edit only: 3ds/src/nd_snap_in.c,
3ds/src/nd_snap.c, 3ds/include/nd_snap.h, 3ds/tools/nd_snap_test.c, 3ds/src/nd_decide.c, 3ds/tools/nd_decide_test.c, 3ds/tests/host.d/decide.sh, 3ds/src/pm_comb.c, pm_gpu.h (2 lines), nd.h (1 line).
## Facts (code read, main 3e4ef5a3e7). NDR0C snapchk named only t1_valid (0a3ebad0da). NDR0B (merged later) gives td on core 1, and its dschk names no td.
- Core 0 folds tst1 and use_t1 in box_fold_sync (nd_comb.c:372-387) before the snapchk compare (nd_geom.c:339-340). nd_snap_in sets box_folded 0 (nd_snap_in.c:20).
- Core 1 runs at the same time as core 0. The miss path writes shared state: pm_comb_vprim (nd_decide.c:232-244, read by build pm_comb.c:379 and core 0
  nd_comb.c:259), the table dec, dec_n, dec_next, last_i, lcg_s (nd_decide.c:166-221), the build statics (pm_comb.c:81-89, used :91-400, :409-413). pm_k4 (:417) is core 0 data.
## Edits
1. nd_snap_in.c: add `int nd_td_white(const NdTd* d)`: a copy of tile0_white (nd_comb.c:354-368) that reads d in place of nd_ws.td[0] (G_IM_FMT_I is 4); declare it in
   nd_snap.h. nd_snap_in end: `if (s->box_prod && s->w.td[1].valid && nd_td_white(&s->w.td[0])) { /* NDR0D: box_fold_sync nd_comb.c:377-381 */ in->tst[0] = 3; in->tst[1] = 5; in->tst[2] = 1; in->tst[3] = 5; in->tst[4] = 7; in->tst[5] = 7; in->tst[6] = 7; in->tst[7] = 1; in->use_t1 = 1; in->box_folded = 1; }`.
2. pm_comb.c: after :89 `static int b_vprim, cb_lock; /* NDR0D: vprim of the running build; the lock of the build statics (core 0 and core 1 build) */`. :379 `pm_comb_vprim &&` -> `b_vprim &&`.
   pm_comb_build (:407-415) becomes `__attribute__((cold)) int pm_comb_build_v(unsigned w0, unsigned w1, int two, int t1only, int vprim, PmPlan* out)`: first
   `while (__atomic_exchange_n(&cb_lock, 1, __ATOMIC_ACQUIRE)) { }` and `b_vprim = vprim;`, then the old body, then `__atomic_store_n(&cb_lock, 0, __ATOMIC_RELEASE);` before
   `return rc;`. Add `__attribute__((cold)) int pm_comb_build(unsigned w0, unsigned w1, int two, int t1only, PmPlan* out) { return pm_comb_build_v(w0, w1, two, t1only, pm_comb_vprim, out); }`.
   Add a last parameter `unsigned k4` to k_val :430, fold_colour :441, kh_colour :455, x_colour :464, name_colour :476 and pass k4 at every call (:447, :459, :470, :471,
   :477-:483, :497-:499). Before (:447) `k_val(p->fold[0], p->fold[1], c3, pr, en, plod)`, after `k_val(p->fold[0], p->fold[1], c3, pr, en, plod, k4)`. :433 and :491 read k4, not pm_k4.
   Rename pm_comb_colours (:496) to pm_comb_colours_k with a last parameter `unsigned k4`. Add `void pm_comb_colours(PmPlan* p, unsigned prim, unsigned env, unsigned plod,
   unsigned key, unsigned keys) { pm_comb_colours_k(p, prim, env, plod, key, keys, pm_k4); }`. pm_gpu.h: declare pm_comb_build_v after :76, pm_comb_colours_k after :80.
3. nd_decide.c: delete :23. Replace :166-170 by `typedef struct { Dec* dec; int cap, n, next; unsigned last; } DecTab; /* NDR0D: one decision table per core */`,
   `static Dec dec0[128], dec1[64]; static DecTab dt0 = { dec0, 128, 0, 0, 0 }, dt1 = { dec1, 64, 0, 0, 0 };`, `static float lcg_f(unsigned* s) { *s = *s * 1664525u + 1013904223u; return (float)((*s >> 8) & 0xffff) / 65535.f; }`.
   decide_miss gets `(DecTab* tb, const NdCombIn* c, int two, unsigned key, unsigned vprim, int own_k4)`: slot `if (tb->n < tb->cap) d = &tb->dec[tb->n++]; else { d = &tb->dec[tb->next];
   if (++tb->next == tb->cap) tb->next = 0; }`. :186 `unsigned ls = 777u;`, each lcg_f() -> lcg_f(&ls). :182 pm_comb_build -> `pm_comb_build_v(c->w0, c->w1, two, c->use_t1, (int)vprim, &d->plan)`.
   :202 `if (own_k4) pm_comb_colours_k(&t, pc, ec, (uint8_t)c->prim_lo8, c->key_c, c->key_s, c->conv_k4 & 255u); else` + the old call.
   decide gets `(DecTab* tb, const NdCombIn* c, int two, unsigned vprim, int own_k4)`: :217 `(unsigned)pm_comb_vprim` -> `vprim`; last_i -> tb->last, dec_n -> tb->n, dec[ -> tb->dec[.
   nd_comb_core body (:231-243 without :232 :234 :244) becomes `static void comb_core(DecTab* tb, int own_k4, const NdCombIn* in, unsigned vprim, int mode5_prev, NdCombOut* out)`
   with `decide(tb, &c, (int)cyc, vprim, own_k4)`. `void nd_comb_core(...) { comb_core(&dt0, 0, in, vprim, mode5_prev, out); }` (keep the line start: hwfix1.sh:12 greps it),
   `void nd_comb_core1(...) { comb_core(&dt1, 1, in, vprim, mode5_prev, out); }` (same parameters). nd.h after :488: declare nd_comb_core1. Update the comment :12-13.
4. nd_snap.c :46-52: `r.key = nd_pso_find_i(&r.in, 0u, &o); if (r.key >= 0) nd_pso_fix(&o, &r.in, sn_mode5); else nd_comb_core1(&r.in, 0u, sn_mode5, &o);`
   `sn_leg = o.leg; sn_mode5 = o.mode5; sn_pat = o.pat; r.plan_on = o.plan_on;`. :97-104: compare leg, mode5, pat, plan_on for every record; `if (sc_cur.key < 0) sc_miss++;`.
5. nd_snap_test.c: NdDs with box_prod 1, td[1].valid 1, td[0] valid, fmt 4, siz 0, w 2, h 2, stride 2, src {0xff, 0xff}: box_folded 1, tst[0] 3, tst[2] 1, tst[7] 1, use_t1 1;
   src {0xff, 0xfe}: box_folded 0. Print `ndr0d fold bad <b>`; `--fault` expects tst[2] 2. nd_decide_test.c: file scope `extern int pm_comb_vprim;`. In one() after old_path:
   nd_comb_core(&in, vprim, 0, &a), nd_comb_core1(&in, vprim, fault ? 1 : 0, &b); memcmp(&a, &b) or pm_comb_vprim != 0 -> n_c1bad++. Print `ndr0d core1 bad <n>` before the 2
   decide lines (`decide entries` stays last); exit 1 when n_c1bad. decide.sh after `tail -1 /tmp/nddec_fault.txt`: `grep -q "^ndr0d core1 bad [1-9]" /tmp/nddec_fault.txt ||
   { echo "ndr0d seeded fault not seen"; exit 1; }` (mode5_prev 1 differs at leg 4, nd_decide.c:31-35).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh` PASS with `ndr0a in bad 0`, `ndr0d fold bad 0`, `ndr0d core1 bad 0`, seeds seen. Commit `NDR0D 1: snap box fold, core 1 miss path`.
2. bench.py base spot_mac01_drec,fxops_drec (merge-base, not charged). rowloop --row NDR0D --bench the same 2: LOOP PASS, byte-equal (core 0 output does not change).
3. bench.py run nimgm_strip_snapchk and wscmb_pause_snapchk; grep -a the snapchk lines > build3ds/out/ndr0d_snapchk.txt. Pass: recs above 0 and bad 0 in the last counter line
   of both (GUESS: bad 0). Else: commit the grep, send the output path to the oracle, wait. dec1 adds about 12 KB bss (GUESS); a memcheck warn is noted, not a fail.
Fix rights: the listed files; compile errors. A DIFF, a crash, a failed seed: stop, send the output path to the oracle. Wait. Stop: last commit `NDR0D 2: gate. 2 benches byte-equal; snapchk bad <2> fbad <2> miss <2>; names <list>` deletes this page. One SubagentHandback.
