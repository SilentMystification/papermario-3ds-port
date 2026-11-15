# ws_LIT1: light units in the converter, level_up.c and starpoint.c join the set and move (moves C5 data, C6 data_arrays, C7 gfxtype)
Model: Haiku. PM_ROW, PM_LANE = LIT1. Branch LIT1, worktree build3ds/wt/LIT1. Budget: 2 builds (1 head, 1 base lane), 4 Azahar runs. Start now (main d7d4d6197).
Owns: 3ds/tools/nd_gfx_conv.c, 3ds/tools/nd_gconv.h, 3ds/tools/gen_gfx_syms.py, 3ds/tools/gconv_src.d/LIT1.txt (new), 3ds/tools/gset.d/LIT1.txt (new), 3ds/tests/host.d/lit1.sh (new),
src/level_up.c, src/starpoint.c, 3ds/tools/gfxin/src/level_up.c and starpoint.c (gset.py writes them). Edit no other file. Never edit by hand a file that gset.py rewrote.
FACT (listcen, main d7d4d6197): C5 data 12798, C6 data_arrays 1100, C7 gfxtype 1768. level_up.c 125/10/10, starpoint.c 71/12/12. GUESS: C5 falls by 100 to 196, C6 and C7 by 12 to 22.
Design (oracle). gsSPSetLights1 is 1 MOVEWORD NUMLIGHT and 2 MOVEMEM units (include/PR/gbi.h:2526). The device runs only the units of a table (nd_ntab_run, nd_walk.c:599), so a light needs
no table op. Only the unit address needs a symbol: a `Lights1 NAME =` object (level_up.c:7, starpoint.c:7, :95). Every light list clears G_LIGHTING before its triangles (FACT grep).
## Group 1: tools and host test (0 builds, 0 runs, commit `LIT1 1: ...`)
1. New 3ds/tools/gconv_src.d/LIT1.txt, 2 lines: `src/level_up.c` / `src/starpoint.c`. New 3ds/tests/host.d/lit1.sh (LF), 6 lines: `#!/bin/sh` /
   `# LIT1: a list with a Lights1 light unit gets a v2 table (nd_gfx_conv.c v2_walk NDO_MOVEMEM). Reads /tmp/gconv of nd_host_test.sh.` /
   `for l in D_802A76E8_7A5628 D_802A20D0_7A9A70; do` / `  grep -qx "$l" /tmp/gconv/ntab_tables.txt || { echo "lit1 FAIL: $l has no v2 table"; exit 1; }` / `done` / `echo "lit1 ok"`.
2. Seeded check: `sh 3ds/tools/nd_host_test.sh > build3ds/out/lit1_h0.txt 2>&1`. It must hold `lit1 FAIL: D_802A76E8_7A5628 has no v2 table` (the old converter: MOVEWORD unsup). Else stop, send the file to the oracle.
3. nd_gconv.h:4. Before `enum { GS_GFX = 1, GS_VTX, GS_MTX, GS_IMG };` After `enum { GS_GFX = 1, GS_VTX, GS_MTX, GS_IMG, GS_LIT /* LIT1: a Lights1 object */ };`.
4. gen_gfx_syms.py. After :16 (`MTX = ...`) add `LIT = re.compile(r"^Lights1\s+(?:N\((\w+)\)|(\w+))\s*=", re.M)  # LIT1`. After :64 (the MTX loop) add 2 lines:
   `    for m in LIT.finditer(text):` / `        out.append(("Lit", name_of(path, ns, m.group(1), m.group(2), pre_bad), moved))`.
   :99 Before `gk = {"Gfx": "GS_GFX", "Vtx": "GS_VTX", "Mtx": "GS_MTX"}[k]` After `gk = {"Gfx": "GS_GFX", "Vtx": "GS_VTX", "Mtx": "GS_MTX", "Lit": "GS_LIT"}[k]`.
   :100 Before `if k == "Mtx":` After `if k in ("Mtx", "Lit"):`. :7 docstring: after `` `Mtx NAME =` `` add `` , `Lights1 NAME =` ``.
5. nd_gfx_conv.c (6 edits).
   a. :209 after `    int inh; char inh_groups[64], inh_first[160];` add `    unsigned lit; /* LIT1: the walk saw a light unit (NDO_MOVEMEM of a Lights1 symbol) */`.
   b. :406 after `    else if (id == NDO_MTX) kind = GS_MTX;` add `    else if (id == NDO_MOVEMEM) kind = GS_LIT;`.
   c. :408 Before `    if (kind == GS_MTX) { /* an Mtx is not an array: its address is the object itself */` After `    if (kind == GS_MTX || kind == GS_LIT) { /* an Mtx or a Lights1 is not an array: its address is the object itself */`. :410 Before `        const GSym* ms = sym_of(w1, GS_MTX, &off);` After `        const GSym* ms = sym_of(w1, kind, &off);`.
   d. :474 Before `        case NDO_MW: if ((a >> 16) != NDO_MW_NONE) v2_unsup(v2_opname(id), k); break;` After `        case NDO_MW: if ((a >> 16) != NDO_MW_NONE && (a >> 16) != NDO_MW_NUMLIGHT) v2_unsup(v2_opname(id), k); break;`.
      :475 Before `        case NDO_FILL: case NDO_PRIMZ: case NDO_SCISSOR: case NDO_MOVEMEM: case NDO_RARE: case NDO_RECT: case NDO_MISC:` After 6 lines:
      `        case NDO_MOVEMEM: { /* LIT1: a light of a Lights1 symbol is a unit only (nd_ntab_run runs the units, nd_walk.c:599) */` / `            unsigned loff;` /
      `            if ((a & 0xffu) != G_MV_LIGHT || !sym_of(w1, GS_LIT, &loff)) v2_unsup(v2_opname(id), k);` / `            else cx->lit = 1;` / `            break;` / `        }`,
      then `        case NDO_FILL: case NDO_PRIMZ: case NDO_SCISSOR: case NDO_RARE: case NDO_RECT: case NDO_MISC:`.
   e. :513 before `            v2_inherit_check(k, g, id);` add `            if (cx->lit && (s->geom & G_LIGHTING)) { v2_unsup("lit draw after a light unit", k); break; }`.
   f. :831 after `        else if (s->kind == GS_MTX) printf("extern Mtx %s;\n", s->name);` add `        else if (s->kind == GS_LIT) printf("extern Lights1 %s;\n", s->name);`. :176 comment: Before `(lights, scissor,` After `(lights outside a Lights1 symbol, scissor,`.
6. `sh 3ds/tools/nd_host_test.sh > build3ds/out/lit1_h1.txt 2>&1`: `ND HOST TESTS PASS`, `lit1 ok`, `bad 0`. Write the `gconv ntab tables` and `gconv gset ok` lines of h0 and h1 to the commit text. Commit.
## Group 2: the move and the gates (1 build, 4 runs, commit `LIT1 2: ...`)
7. `python 3ds/tools/listcen.py > build3ds/out/lit1_lc0.txt`. New 3ds/tools/gset.d/LIT1.txt, 2 lines: `src/level_up.c` / `src/starpoint.c`.
   `python 3ds/tools/gset.py 3ds/tools/gset.d/LIT1.txt build3ds/out/gset_ok.txt > build3ds/out/lit1_set.txt`: last line `gset ok`.
   Rewrite example (starpoint.c:201): Before `EntityModelScript EMS_starpoint_starpoint = STANDARD_ENTITY_MODEL_SCRIPT(D_802A2158_7A9AF8, RENDER_MODE_CLOUD_NO_ZCMP);` After `EntityModelScript EMS_starpoint_starpoint = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A2158_7A9AF8), RENDER_MODE_CLOUD_NO_ZCMP);` (the form of src/entity/Block.c:536).
   `git status --short` lists only the 2 sources, the 2 twins under 3ds/tools/gfxin/src and the LIT1 files. The JP-branch lists stay (gen_gfx_syms drops `#if VERSION_JP`).
8. nd_host PASS, `lit1 ok`, `bad 0`, no `gconv: moved` line. One build rc 0, `memcheck ... margin` above 0 KB. For B = spot_mac01_drec, then sbk14_drec:
   `python 3ds/tools/bench.py base B --row LIT1 --commit $(git merge-base HEAD main)`, `python 3ds/tools/bench.py run B --row LIT1 --why "LIT1 2"`,
   `python 3ds/tools/bench.py cmp B --row LIT1 --commit $(git merge-base HEAD main)`: byte-equal. `python 3ds/tools/listcen.py > build3ds/out/lit1_lc1.txt`. Delete this page. Commit.
Fix rights (step 7-8): a `gset keep <file>:<line>`, a `gconv: moved X` line, a changed file outside step 7's list, or a build error that names a list: `git checkout -- src include 3ds/tools/gfxin`,
`git clean -fdq -- 3ds/tools/gfxin`, remove that source from gset.d/LIT1.txt, do step 7 again (2 tries per source). A host FAIL in group 1 or a cmp difference: 2 tries per named cause, then the stop rule (CLAUDE.md 5).
Stop: group 2 committed and gated. Hand-back build3ds/out/handback_LIT1.txt, first 8 lines: commit hashes, the h0 and h1 gconv lines, the `gset moved` line, the 2 cmp results, C5, C6, C7 before and after (lc0, lc1), memcheck margin, sources taken out.
