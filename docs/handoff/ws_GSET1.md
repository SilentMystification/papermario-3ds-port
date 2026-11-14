# ws_GSET1: the tool that moves Gfx lists off the device (host-only twins), and the pilot set fright_jar (moves C5, C6, C12)
Model: Haiku. PM_ROW, PM_LANE = GSET1. Budget: 4 Azahar runs. Start after `Merge GKEY2` shows in `git log --oneline main`. Branch GSET1.
Files: 3ds/include/nd_gtab.h (macros only), 3ds/tools/nd_gconv.h, gen_gfx_syms.py, gen_gfx_tab.sh, nd_gfx_conv.c, nd_gtab_test.c, nd_host_test.sh (one flag), listcen.py,
new 3ds/tools/gset.py, new 3ds/tools/gset.d/GSET1.txt, new 3ds/tests/gset/ (case), new 3ds/tests/host.d/gset.sh, 3ds/tools/gfxin/ (generated twin, committed), the src files gset.py rewrites.
## Facts (main 3eba999c4)
- An effect list is an asset include: src/effects/gfx/water_splash.c:6 `#include "effects/gfx/D_09000100_3BCB90.gfx.inc.c"` (assets/us, git-ignored). Moving a list = moving
  that include line into the twin 3ds/tools/gfxin/<same path> (first line `#include "<path>"`), which only gen_gfx_tab.sh compiles (host, -I . -I assets/us, gen_gfx_tab.sh:14).
- Users: src/effects/blast.c:13 `extern Gfx D_09001300_37ECD0[];`, :25 `Gfx* D_E007C510[] = {` (handles), :110 `nd_draw_list(D_09001300_37ECD0, ND_SELF);`.
- listcen counts every assets/**/*.c (listcen.py:249), so C5 and C6 fall only with the filter of step 7. fxpic_91_drec draws fright_jar (bench.d/GOLDFX1.json:24).
## Group 1: converter and syms (host test first, no list moves, commit `GSET1 a: ...`)
1. nd_gtab.h after `#define NDG(sym) (&ndg_##sym)`: `#define NDN_DECL(sym) extern const NdNDev ndd_##sym` and `#define NDN(sym) ((Gfx*)&ndd_##sym) /* GSET: the key of a moved list (nd_draw_list, handles) */`.
2. nd_gconv.h GSym: after `const void* end;` add `int moved; /* GSET: defined in a 3ds/tools/gfxin twin */`. gen_gfx_syms.py: scan(path, out, moved=False) appends (kind, name, moved) and passes
   moved to its .inc.c recursion. In main, a source that starts with `3ds/tools/gfxin/`: read its first line `#include "<orig>"`, scan(orig, syms, False), then scan the twin text with moved=True.
   The GSym line ends `, 1 }},` for a moved symbol (others unchanged). gen_gfx_tab.sh:15: inside the loop set `p=<the path it echoes now>; [ -f "3ds/tools/gfxin/$p" ] && p="3ds/tools/gfxin/$p"; echo "$p"`.
3. nd_gfx_conv.c. (a) main: print `extern const NdNDev ndd_%s;` for every moved GS_GFX. For a moved g put `#ifdef ND_HOST_GFX` / `#endif` around its `extern Gfx` line, around
   conv(g) and around its nd_gtab_idx line. `NdGIdx nd_gtab_idx[%u] = {` -> `NdGIdx nd_gtab_idx[] = {`. The count: `#ifdef ND_HOST_GFX\nconst unsigned nd_gtab_n = %uu;\n#else\nconst unsigned nd_gtab_n = %uu;\n#endif\n` (n_lists, n_lists - moved).
   (b) v2_unit_add, kind GS_GFX: when the target symbol is moved, the text is `(const char*)&ndd_<name>` and an offset other than 0 is v2_unsup("call into a moved list at an offset").
   (c) v2_emit_table: for a moved list wrap the `const NdNTab ndn_%s = ...` line in `#ifdef ND_HOST_GFX` / `#endif`. (d) The device entry of a moved list (GKEY1 loop):
   `const NdNDev ndd_%s __attribute__((aligned(8))) = { (const void*)&ndd_%s, %uu, ndnu_%s, %uu, %uu };` with ndnu_ of v2_nm[i] and nu = v2_nk[i] (EMIT4: ndnu keeps the NOPs).
   (e) conv(): record each NDG_CALL edge (caller index, callee `o->s - gconv_sym`). After the tables: ok = has a device entry and not NDG_F_PARTS; repeat until stable: drop X when a callee
   has no device entry or is dropped, drop X when a caller is not ok. Write the ok names to argv[8] (gset_ok.txt; main: `argc < 8` -> `argc < 9`, usage adds `<gset_ok.txt>`). A moved list not ok, or with an unmoved caller: `gconv: moved <X>: <reason>`, exit 1.
   (f) nd_ntab_idx.c: `extern const NdNDev ndd_<list>;` per table and `const NdNDev* const ndn_dev_of[] = { &ndd_<list of table i>, ... };` in ndn_all order.
4. gen_gfx_tab.sh: the nd_gfx_conv line gets `"$T/gset_ok.txt"` last; add gset_ok.txt to the `for lg in` list. nd_host_test.sh: the nd_gfx_tab.o gcc line gets `-DND_HOST_GFX`.
5. nd_gtab_test.c v2_dev(t, name, fault, n): `dv = ndn_dev_of[n]`; key = `g->moved ? (const void*)dv : t->list`; fail `device index key` when nd_ntab_find(key) != dv or (g->moved and dv->list != dv);
   moved: u must be `(const uint32_t*)t->ops[0].p` of the first table of that list, else u == t->list. Count moved; after `ntab dev flag` print `ntab moved %u`.
## Group 2: gset.py and the pilot (unit test first, then one build, one gate, commit `GSET1 b: ...`)
6. gset.py <gset.d file>: each line a device source (src/effects/gfx/<x>.c). Read build3ds/out/gset_ok.txt. Move each `#include "<inc>.gfx.inc.c"` line whose assets/us/<inc> file
   defines `Gfx X[]` with X ok into the twin (order kept). In src/**/*.c, src/**/*.h, include/**/*.h (not src/effects/gfx): `extern Gfx X[];` -> `NDN_DECL(X);`, any other `\bX\b` -> `NDN(X)`;
   a line with `X[`, `sizeof`, `&X` or `BranchList` prints `gset keep <file>:<line>` and stays. Add `#include "nd_gtab.h"` after the first #include of a rewritten file without it.
   Prints `gset moved <k> files <f> keep <c>`. Case 3ds/tests/gset/case (a fake device file with 2 includes, one ok, a user file with all 4 use shapes) with expected output; gset.sh
   runs it and cmp; seeded: a copy without the keep rule must differ. Prints `gset ok` or `gset FAIL`.
7. listcen.py: skip an assets file whose path after assets/us/ appears in an `#include` line of a file under 3ds/tools/gfxin.
8. gset.d/GSET1.txt = `src/effects/gfx/fright_jar.c`. Run the tool. keep above 0: edit those lines by hand to nd_draw_list(NDN(X), <its desc>) or stop.
## Gate (1 build, 2 Azahar runs)
nd_host PASS (`ntab moved <k>` with k above 0, `bad 0` lines, gset ok). Build rc 0 (a link error names a caller left: remove that list from the set by hand in the twin, then rerun).
Memcheck margin 310 KB or more. `bench.py run` then `cmp --commit <merge-base>` byte-equal: fxpic_91_drec, gold_12_drec. listcen C5 and C6 fall (values in the commit). Commit deletes this page.
Fix rights: the files above. A FAIL, a drec difference or k 0: no commit, write build3ds/out/gset1.txt, send the path to the oracle (CLAUDE.md rule 5). Hand back: 8 lines.
