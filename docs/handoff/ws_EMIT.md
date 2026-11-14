# ws_EMIT: a v2 list runs from its table, not from its Gfx array (NEMIT part 1; blocker of GSET, C5, C6, C12)
Model: Haiku. PM_ROW, PM_LANE = EMIT. Budget: 4 Azahar runs. Branch EMIT (`sh 3ds/tools/wt_new.sh EMIT`), start now. Files: 3ds/include/nd_gtab.h,
3ds/src/nd_walk.c, 3ds/tools/nd_gfx_conv.c, 3ds/tools/nd_gtab_test.c, 3ds/tests/host.d/conv2c.sh. No other workstream owns them. CONV2C2 starts after `Merge EMIT`.
## Facts (the reason the edits are draw-equal)
- nd_ntab_run (nd_walk.c:594) walks e->list, the Gfx array. The converter already writes the unit copy ndnu_<list> (nd_gfx_conv.c:567-571): the list's
  units without NOPs, to the end or the branch. nd_gtab_test.c:480-492 proves it per table. nd_walk_state skips ND_NOP_A with no side effect (nd_walk.c:369).
- ent_gdl of a list with no triangle returns 0 or -1 (nd_ent.c:275 and :282-283 set why 8, :444 returns 0, :441 returns -1). For a list outside
  gMapShapeData both values lead to gt_body (dlc_try returns 0 at nd_shape.c:868).
- walk_call (nd_walk.c:224-247) walks a callee at depth from its Gfx words (:245). At depth 0 no v2 list arrives: nd_rec_dl records nd_gtab_run for every
  list with a table (nd_gtab.h:66-67), and every v2 list has a v1 table (nd_gtab_test.c:519).
## E1: the device index carries the unit copy (host test first). Readers and writers of NdNDev (grep): nd_gtab.h:102, nd_gfx_conv.c:616-619, nd_gtab.c:41-60
(struct copy and find: no edit), nd_walk.c:505, :597 (find only: no edit), nd_walk.c:594, nd_gtab_test.c:495-511.
1. nd_gtab.h:102 Before `typedef struct NdNDev { const void* list; uint32_t n; } NdNDev;` After `typedef struct NdNDev { const void* list; uint32_t n;
   const uint32_t* u; uint32_t nu; uint32_t ntri; } NdNDev;`
2. nd_gfx_conv.c:207: add `static unsigned v2_nu[V2_MAXTAB], v2_ntri[V2_MAXTAB];`. After :607 add `v2_nu[v2_n_tab] = cx->nu; v2_ntri[v2_n_tab] = cx->nidx / 3u;`.
   :617 Before `printf("    { (const void*)%s, %uu },\n", v2_tab[i]->name, v2_nk[i]);` After `printf("    { (const void*)%s, %uu, ndnu_%s, %uu, %uu },\n",
   v2_tab[i]->name, v2_nk[i], v2_tab[i]->name, v2_nu[i], v2_ntri[i]);`. :618 `{ 0, 0 }` becomes `{ 0, 0, 0, 0, 0 }`.
3. nd_gtab_test.c: v2_dev(t, name, fault) (:495, call :526 `v2_dev(t, name, fault && tables == 0)`). After :510 add: `if (dv->u != (const uint32_t*)t->ops[0].p
   || dv->nu != t->ops[0].b + (fault ? 1u : 0u)) { v2_dev_bad++; v2_fail("device index units", name, 0); }` and the same form for ntri against the sum of
   `b` of the NDN_DRAW ops / 3 (`device index triangles`). After :544 print `ntab notri <tables with ntri 0> of <ndn_dev_n>`. conv2c.sh after the fault
   run: `grep -q "^ntab dev entries [0-9]* bad 0$" /tmp/ndntab_fault.txt && { echo "seeded dev fault not seen"; exit 1; }`.
4. nd_walk.c:594 Before `void nd_ntab_run(const NdNDev* e) { nd_walk_state((const uint32_t*)e->list, e->n); }` After `void nd_ntab_run(const NdNDev* e) {
   nd_walk_state(e->u, e->nu); }`.
## E3: a callee with a v2 entry runs its unit copy. walk_call nd_walk.c:226: after `Gfx* next = ...` add `const NdNDev* nt = nd_ntab_find(next);`.
:245 Before `else if (ex_depth < ND_LIST_DEPTH) { ex_depth++; nd_walk_state((const uint32_t*)next, ND_LIST_MAX); ex_depth--; }` After `else if (ex_depth <
ND_LIST_DEPTH) { ex_depth++; if (nt) nd_ntab_run(nt); else nd_walk_state((const uint32_t*)next, ND_LIST_MAX); ex_depth--; }`. :244 (depth 0) stays.
Gate A (E1 + E3, one build): nd_host_test.sh PASS (conv2c: bad 0, dev bad 0, the seeded faults seen), memcheck margin 310 KB or more, drec byte-equal:
gold_12_drec (`--commit 930de29e1`) and fxpic_91_drec (`--commit d29deacc3`). Commit `EMIT E1 E3: ...` with the notri line value.
## E2: a v2 list with no triangle skips ent_gdl (second group, after gate A)
nd_gtab_run nd_walk.c:621-622 and walk_call :234-235: `else if (dlc_on && ...)` becomes `else if (dlc_on && !(nt && !nt->ntri) && ...)`. In nd_gtab_run add
`const NdNDev* nt = nd_ntab_find(t->list);` after :603. gt_body(t, nt) at :608 and :635. gt_body (:596-600) uses nt, not its own find. Gate B: nd_host PASS, one build,
the two drec runs of gate A byte-equal. Commit `EMIT E2: ...`. A difference: no commit (ent_gdl resets the caches at nd_ent.c:419-420 and fills the
ent table, nd_ent.c:425), write build3ds/out/emit_e2.txt, send the path to the oracle.
Fix rights: the five files above. Stop rule: CLAUDE.md rule 5. Hand back: 8 lines, the commit hashes, the notri line, the drec results
(details build3ds/out/handback_EMIT.txt).
## Not in this page: E4 (oracle design next, no Sonnet core)
A v2 list with triangles still reaches ent_gdl, and ent_compile (nd_ent.c:253) and ent_run (:486 ent_hash every frame) read its Gfx words. Entity records
read them through ent_draw (:449-457). Design for the next oracle: the converter writes the ent_flat copy (rule nd_ent.c:133-148) and the ent_hash_walk value
(:112-125) per v2 table, and ent_compile and ent_run take both from the device entry. Host test: the copied functions against the converter output.
