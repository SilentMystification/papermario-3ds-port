# Row CONV2D2: nd_gtab_run runs a list with a v2 table through a device index entry (list, unit count); nd_ntab_find

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Work in build3ds/wt/CONV2D2 on top of
82f55531a. Do not merge main. Correction 3 by ORACLE2, 2026-11-13: it replaces the edits of 82f55531a.)
## Facts (code read)
- 82f55531a failed memcheck (static 21884 KB, margin 102 KB, safety 256 KB; at 37bae9aa2: 21660, 326). Cause: NdNIdx holds
  `&ndn_<list>`, so the linker keeps every v2 table (unit copies 74.5 KB, ops 59.9 KB, indices 38.2 KB, binds 31.8 KB). Before,
  nothing referenced them and the linker dropped them.
- Size plan: the device gets only `{ list, n }` (922 x 8 = 7.4 KB). The host test v2_units (3ds/tools/nd_gtab_test.c:473-492,
  nd_host bad 0) proves that the unit copy is the list words with the NOPs left out. nd_walk_state skips a NOP
  (3ds/src/nd_walk.c:368). So nd_walk_state over the list runs the unit copy, with n = the word pairs to the stop, NOPs counted.
  The v2 tables stay host data. The copy goes to the device when a row deletes the Gfx arrays (net size 0 then).
- fxpic_89 calls D_09000100_3BCB90 at src/effects/water_splash.c:189. That list has only state ops, so nd_rec_dl flags it
  state-only (3ds/src/nd_gtab.c:56-63) and nd_gtab_run takes the state-only arm (3ds/src/nd_walk.c:571-578). The other splash
  lists have no v2 table (inherit). So the v2 run goes in both arms (:575 and :601 on main, +5 lines in this branch).
- Risk (console, not a gate): a v2 list now runs its units, not the compiled defs of gt_ops (nd_walk.c:519-523).
## Touch only
Files: 3ds/include/nd_gtab.h (the CONV2D2 block at its end), 3ds/tools/nd_gfx_conv.c (V2C, v2_tab line, v2_units_top, the
v2_convert_all table loop and dev index print), 3ds/src/nd_gtab.c (nd_ntab_find inside the ND_SCOMP_HOST guard), 3ds/src/nd_walk.c
(nd_ntab_run, gt_body added, the two `gt_ops(t);` calls of nd_gtab_run), 3ds/tools/nd_gtab_test.c (v2_dev added, v2_compare).
New names: fxpic_89_drec. Host link: 3ds/tests/host.d/conv2c.sh and 3ds/tools/nd_host_test.sh link nd_gtab.c and nd_gfx_tab.o.
Needs: 3ds/tools/nd_gfx_conv.c has v2_units_top
## Exact edits (PM_ROW=CONV2D2 PM_LANE=CONV2D2 on every call)
1. nd_gtab.h: replace the NdNIdx typedef, its extern and the 2 prototypes with
   `typedef struct NdNDev { const void* list; uint32_t n; } NdNDev;`, `extern NdNDev ndn_dev_idx[];`,
   `extern const unsigned ndn_dev_n;`, `const NdNDev* nd_ntab_find(const void* list);`, `void nd_ntab_run(const NdNDev* e);`.
   Comment: core 0 only; n = the word pairs of the list from unit 0 to the stop, NOPs counted.
2. nd_gfx_conv.c: (a) a new line after the `unsigned nu;` line of V2C: `unsigned nk; /* CONV2D2: words to the stop */`.
   (b) after `static const GSym* v2_tab[V2_MAXTAB];`: `static unsigned v2_nk[V2_MAXTAB];`.
   (c) v2_units_top: `cx->nk = nu;` before the loop; `if (id == NDO_END) { cx->nk = k; break; }`;
   `if (id == NDO_CALL && (w0 & 1u)) { cx->nk = k + 1u; break; }`.
   (d) loop of v2_convert_all: `v2_nk[v2_n_tab] = cx->nk;` before `v2_tab[v2_n_tab++] = g;`.
   (e) the dev index print: type `NdNDev`, entry `printf("    { (const void*)%s, %uu },\n", v2_tab[i]->name, v2_nk[i]);`.
3. nd_gtab.c nd_ntab_find (inside `#ifndef ND_SCOMP_HOST`): NdNIdx becomes NdNDev, the return type `const NdNDev*`, the hit
   returns `&ndn_dev_idx[mid]`.
4. nd_walk.c: `void nd_ntab_run(const NdNDev* e) { nd_walk_state((const uint32_t*)e->list, e->n); }`. After it:
   `static void gt_body(const NdGTab* t) { const NdNDev* nt = nd_ntab_find(t->list); if (nt) { nd_gtr_ntab++; nd_ntab_run(nt); } else gt_ops(t); }`.
   In nd_gtab_run, both `gt_ops(t);` lines (state-only arm and the last arm) become `gt_body(t);`.
5. nd_gtab_test.c, inside `#ifdef NTAB_V2` after v2_units: `static unsigned v2_dev_bad;` and `static void v2_dev(const NdNTab* t,
   const char* name)`. It finds the GSym g of t->list and the entry of ndn_dev_idx with list == t->list (none: v2_dev_bad++ and
   `v2_fail("device index entry", name, 0)`). It walks the words of g as v2_units does with `unsigned ks = nl;`, at NDO_END
   `ks = k`, after a branch call `ks = k + 1u`. When `dv->n != ks`: v2_dev_bad++ and `v2_fail("device index count", name, ks)`.
   v2_compare calls `v2_dev(t, name);` after `v2_units(t, name);` and after the `ntab tables` line prints
   `printf("ntab dev entries %u bad %u\n", ndn_dev_n, v2_dev_bad);`.
6. Commit: `CONV2D2: device index of the v2 lists (list, unit count), nd_ntab_run runs the list words in both nd_gtab_run arms`.
## Gate (expected values)
1. `sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK, margin 310 KB or more (hand back static and margin).
   `bench.py run nd_host --row CONV2D2 --why "CONV2D2 host"`: PASS with `ntab tables 922 draws`, `bad 0` and
   `ntab dev entries 922 bad 0`.
2. The bases of fxpic_89_drec and spot_mac01_drec at 37bae9aa2 exist (runs.jsonl of this branch). Do not base again.
3. Seed first: in nd_ntab_run `e->n` becomes `e->n & 1u` (the 20 words of D_09000100_3BCB90 run 0). Build (the ELF sha1
   changes), `bench.py run fxpic_89_drec --row CONV2D2 --why "CONV2D2 seed"`, `bench.py cmp fxpic_89_drec --row CONV2D2`:
   must differ. Undo the seed.
4. `bench.py run fxpic_89_drec,spot_mac01_drec --row CONV2D2 --why "CONV2D2 drec"`, then `bench.py cmp` of each: byte-equal.
   Hand back the `gtabrun` and `ntabrun` lines of the fxpic_89_drec log (record only, no gate).
5. Budget 8 Azahar starts (4 used by the bases). Counter: none moves (runner row, it unblocks CONV2F and ENTRYCAP).
## Stop and end
A memcheck margin below 310 KB, nd_host bad above 0, a seed with no difference, a drec difference with the real edit, or a
tool fault: stop, commit, hand back the exact output. Delete this row file in the completing commit.
