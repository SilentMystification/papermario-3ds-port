# ws_PSOLK: the device takes the combine decision from the pso table (plan row NPSO2, device part; enabler of S9DEL2A)
Model: Haiku. PM_ROW, PM_LANE = PSOLK. Budget: 4 Azahar runs. Start now (no owner of these files runs). Branch PSOLK. No counter moves (C1-C12 stay): the hand-back
gives hit and miss counts. Files: 3ds/include/nd.h, 3ds/src/nd_decide.c, nd_comb.c, new nd_psolk.c, 3ds/tools/gen_pso_main.c, gen_pso_tab.py, new nd_psolk_test.c, psolk.sh, psotab.sh.
Why a hit is equal (code read, main 1149b5513): nd_comb_core (nd_decide.c:294) reads every NdCombIn field, but prim and env only for cc (nd_const_rgb :51; decide_miss :236 uses random colours), leg_mode never
(dc_legacy :91 writes it first), box_pat and mode5_prev only on the leg 4 return (:95-99) and in dual (:301). The table entry is nd_pso_build (:346) with prim 0,
env 0, box_pat 0, mode5 0. So: same other inputs + nd_pso_fix = nd_comb_core. The host table is built with SSE floats (x87 extended precision can flip use, :273).
## Group 1: host (commit `PSOLK 1: ...`)
1. nd.h after the 2 lines of `void nd_pso_colour(`: `int nd_pso_in_eq(const struct NdPsoDesc* d, const NdCombIn* w, unsigned vprim); /* NPSO2D: w equals d in every input of nd_comb_core but prim, env, leg_mode, box_pat */`,
   `void nd_pso_fix(NdCombOut* o, const NdCombIn* w, int mode5_prev); /* NPSO2D: a table decision under w's colours, box_pat and mode5 */`, `const struct NdPsoOut* nd_pso_find(const NdCombIn* w, unsigned vprim); /* nd_psolk.c */`.
2. nd_decide.c, append: `int nd_pso_in_eq(const NdPsoDesc* d, const NdCombIn* w, unsigned vprim) { uint32_t t[16];` `if (w->w0 != d->cw0 || w->w1 != d->cw1 || w->omode_h != d->omh ||
   (uint8_t)w->prim_lo8 != d->plod || w->key_c != d->key_c || w->key_s != d->key_s || w->conv_k4 != d->conv_k4 || vprim != (unsigned)d->vprim) return 0;` `if (w->tex_on != d->tex_on ||
   w->use_t1 != d->use_t1 || w->t1_valid != d->t1_valid || w->dual_on != d->dual_on || w->box_prod != d->box_prod || w->box_folded != d->box_folded || w->comb_set != d->comb_set ||
   w->comb_alpha_shade != d->comb_alpha_shade) return 0;` `nd_comb_tst(d->cw0, d->cw1, d->use_t1, d->box_folded, t); return memcmp(t, w->tst, sizeof t) == 0; }` and
   `void nd_pso_fix(NdCombOut* o, const NdCombIn* w, int mode5_prev) { if (o->leg == 4) { o->mode5 = mode5_prev; o->pat = w->box_pat; }` `if (o->cc_on) { unsigned v = 0;
   nd_const_rgb(w, &v); o->cc = v; } o->dual = w->dual_on | w->box_prod << 1 | o->pat << 2; }`.
3. New 3ds/src/nd_psolk.c (includes of nd_decide.c:14-20 minus math.h; `extern const NdPsoDesc nd_pso_dsc[]; extern const uint16_t nd_pso_ord[]; extern const unsigned nd_pso_n;`):
   nd_pso_find: binary search of the lower bound of (w->w0, w->w1) over `nd_pso_dsc[nd_pso_ord[m]]` ordered by (cw0, cw1); then walk while cw0, cw1 equal; the first id with
   nd_pso_in_eq(d, w, vprim) returns `nd_pso((NdPso)id)`; else return 0. No division (`>> 1`).
4. gen_pso_main.c before the `nd_pso_n` line: write `const NdPsoDesc nd_pso_dsc[%u] = {` (rows) with one `{ %uu, ... }` line per id, the 20 fields in nd_api.h:26-32 order
   (n 0: `{ 0 }`), and `const uint16_t nd_pso_ord[%u] = {...};`: the ids by insertion sort on (cw0, cw1), equal keys in id order; n above 1024: stderr line, return 1.
5. gen_pso_tab.py: `SSE = ["-msse2", "-mfpmath=sse"]`; insert `*SSE` after "-m32" in both gcc lines of main. psotab.sh: add `-msse2 -mfpmath=sse` after -m32 in its 2 gcc lines.
6. nd_psolk_test.c: copy nd_decide_test.c:15-71 (f32tof24, unpack8, rd32, St, add_state, walker_comb) and its reading loop and synthetic states (:149-174). Grid of :176-183
   plus box_pat bp 0..1, mode5_prev m5 0..1, leg 0 or 4. Per point build `in` and `d` as :100-111 (no tex_on fault), then `in.box_pat = bp`. ref: `pm_comb_set_k4(k4); nd_comb_core(&in, vprim, m5, &ref)`;
   got: `nd_pso_build(&d, &o); got = o.comb; if (!fault) nd_pso_fix(&got, &in, m5);`. nd_pso_in_eq 0: neq++; else ent++ and memcmp(&got, &ref, sizeof ref) != 0: bad++.
   Find part: for each id < nd_pso_n build w from nd_pso_dsc[id] as nd_decide.c pso_in (:335-342, tst by nd_comb_tst); nd_pso_find(&w, vprim) must return a p with
   p->comb byte-equal nd_pso(id)->comb; with w.w1 ^= 1 it must return 0. Print `psolk find <n> bad <b>`, then last `psolk entries <e> neq <q> bad <b>`; exit 1 on any bad.
7. psolk.sh (form of psotab.sh, SSE flags on all gcc lines): gen_pso_tab.py on 3ds/tests/pso into /tmp/psolk; link the test with nd_psolk.c, nd_decide.c, nd_tmpl.c, the table,
   pm_comb.o; `--fault` run first: its last line must match `bad [1-9]`; the real run must end `bad 0` with entries above 0. `sh 3ds/tools/nd_host_test.sh`: PASS.
## Group 2: sync_combine_work (1 build, 2 runs, commit `PSOLK 2: ...`)
nd_comb.c: line `static unsigned psochk_draws, ...` gets `static unsigned pk_hit, pk_miss, pk_bad;` after it. The psochk line (:230) becomes
`pm_log_evt("psochk f%u draws %u bad %u lk %u miss %u lkbad %u", pm_frame_n, psochk_draws, psochk_bad, pk_hit, pk_miss, pk_bad);`. After nd_psochk_draw add
`static void nd_comb_pick(const NdCombIn* in, NdCombOut* o) { const NdPsoOut* t = nd_pso_find(in, (unsigned)pm_comb_vprim);` `if (!t) { pk_miss++; nd_comb_core(in, (unsigned)pm_comb_vprim, comb_mode5, o); return; }`
`pk_hit++; *o = t->comb; nd_pso_fix(o, in, comb_mode5);` `if (psochk_on()) { NdCombOut r; nd_comb_core(in, (unsigned)pm_comb_vprim, comb_mode5, &r); if (memcmp(&r, o, sizeof r)) pk_bad++; } }`.
Before (sync_combine_work) `    nd_comb_core(&in, (unsigned)pm_comb_vprim, comb_mode5, &o);` After `    nd_comb_pick(&in, &o);`.
Gate: build rc 0, memcheck margin 310 KB or more. `bench.py run fxpic_20_psochk --row PSOLK --why "PSOLK 2"`: the last psochk line of its log (log_path of the run record) has bad 0, lk above 0, lkbad 0.
`bench.py run gold_12_drec`, then `bench.py cmp gold_12_drec --row PSOLK --commit 930de29e1`: byte-equal.
## Group 3: nd_comb_decide (1 build, 2 runs, commit `PSOLK 3: ...`)
nd_comb_decide (:259) Before `    nd_comb_core(in, (unsigned)pm_comb_vprim, comb_mode5, out);` After `    nd_comb_pick(in, out);`. Same gate. The last commit deletes this page.
Fix rights: the files above. A FAIL, lkbad above 0, lk 0 or a cmp difference: 2 tries per named cause, then no commit, write build3ds/out/psolk.txt, send the path to the
oracle (CLAUDE.md rule 5). Hand back 8 lines: hashes, psolk host lines (fault and real), the psochk lines of both groups (lk, miss, lkbad), cmp results, margin.
