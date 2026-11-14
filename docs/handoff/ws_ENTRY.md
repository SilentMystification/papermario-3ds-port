# ws_ENTRY: src/effects lists to nd_draw_list (ND_SELF, entry descriptions) and the effect state calls (C10, C1, C2)
Model: Haiku. Branch ENTRYFX (`sh 3ds/tools/wt_new.sh ENTRYFX`), PM_ROW and PM_LANE = ENTRYFX. Edit only src/effects/*.c and the new files
named here. Budget: 7 Azahar runs. FACT (`python 3ds/tools/listcen.py`, 6fbd4d6c6): C1 build 2008, C2 gfxpos 1438, C10 recdl 359 (effects 319).
FACT (`codemod.py --dry --state --ntab build3ds/out/ntab_tables.txt src/effects`, 6fbd4d6c6): value 103, nd_rec_dl value 90 keep 229.
## E1: effect state calls and self-contained lists (one commit `ENTRYFX E1: ...`)
1. `cp ../../../build3ds/out/ntab_tables.txt build3ds/out/` (main's file, 922 names). listcen before -> build3ds/out/lc_e1_before.txt.
2. `python 3ds/tools/codemod.py --state --ntab build3ds/out/ntab_tables.txt src/effects`. 3. explosion.c has one ND_PSO_DESC, so codemod
   gives it to an inheriting list. Undo explosion.c:198. Before: `nd_draw_list(D_090008F0_344860, pe_explosion_0);` After: `nd_rec_dl(D_090008F0_344860);`
- Shape ND_SELF (blast.c:109). Before: `nd_rec_dl(D_09001300_37ECD0);` After: `nd_draw_list(D_09001300_37ECD0, ND_SELF);`
- Shape state (cold_breath.c:209). Before: `gDPSetAlphaDither(gMainGfxPos++, G_AD_NOISE);` After: `nd_pso_aldith(G_AD_NOISE);`
- Includes: a file with an nd_pso_ or nd_tex_on call and no `#include "nd_st.h"` gets it after the last #include of its top include block
  (cold_breath.c: after `#include "nd_xf.h"`, :5). The same rule for nd_val.h (nd_val_ calls) and nd_xf.h (nd_xf_ calls).
4. Gate: nd_host_test.sh and codemod run.sh PASS, 1 build, memcheck margin 310 KB or more. listcen build, gfxpos = before - 103, recdl = before
   - 89 (6fbd4d6c6: 1905, 1335, 270). Run gold_12_drec, then `bench.py cmp gold_12_drec --row ENTRYFX --commit 930de29e1`: byte-equal.
## E2: new 3ds/tools/entapply.py and 3ds/tests/entapply/run.sh (host only, one commit)
`python 3ds/tools/entapply.py [--dry] <entcap.py output>...`. A line counts when it ends with 11 hex words (`((?: [0-9a-f]{8}){11})\s*$`).
Its site is `(src/effects/\w+\.c):(\d+)` in the line; no site: count `other`. Per site: the set of distinct 11-word tuples.
- Two or more tuples: print `entapply multi <file>:<line> <n>`. Site line (1-based) not matching `^(\s*)nd_rec_dl\((.+)\);\s*$`: print
  `entapply skip <file>:<line>`. File without the line `#include "nd_gtab.h"`: print `entapply skip <file> include`. These get no edit.
- Else name = `pe_<file stem>_<line>`; the line becomes `<indent>nd_draw_list(<arg>, <name>);`; print `entapply site <file>:<line> <name>`.
  Desc: `ND_PSO_DESC(<name>, <w0..w7 as 0x%08x>, <12 bytes>);`, bytes = `[(w >> (8 * k)) & 0xff for k in range(4)]` of w8, w9, w10 (decimal),
  joined by ", ". Per file: line edits first, then the desc lines in line order directly after `#include "nd_gtab.h"`.
- Last line: `entapply sites <n> multi <m> skip <s> other <o> files <f>`. --dry writes no file. Test pass: `entapply: 1/1 PASS`.
- run.sh (form of 3ds/tests/codemod/run.sh): one case in a temp dir: src/effects/fx.c (the include line, 4 sites: one tuple, two tuples,
  `if (a) nd_rec_dl(D_X);`, one not in the log), entcap.txt, expected fx.c and stdout. Fixture tuple: `fcffb3ff ff65feff 1e080cf0 00504340
  00000005 00000000 00000000 00000000 00010001 00010000 00000004` at `/w/src/effects/explosion.c:232` gives the text of explosion.c:20 with
  name pe_explosion_232; add one 3ds/src/nd_gtab.c:114 line. Seeded fault: a copy made with `sed 's/8 \* k/8 * (3 - k)/'` must fail.
## E3: capture and apply (commit 1: the scene files, commit 2: the apply)
- New 3ds/tools/azt.d/ENTRYFX.json, bench.d/ENTRYFX.json: 6 copies of scenario and bench fxcen_c0 (azt_scenarios.json, bench_defs.json:
  azahar_stream, turbo, cap 45, until `fxtest: done`, events warp and done, tags azahar det). fxent_a/b/c debug `warp=mac_01:0 pos=0:0:200
  det fxtest=<list> loglevel=debug entcap`; fxchk_a/b/c the same with `psochk` for `entcap`. Lists: a 0..42, b 43..85, c 86..127.
1. Run fxent_a, b, c. After each: `python 3ds/tools/entcap.py build3ds/lane-ENTRYFX/slot1/last_log.txt > build3ds/out/entcap_<x>.txt`.
   Its last line must say `over 0` (else a tool fault: stop). 2. `python 3ds/tools/entapply.py` on the 3 files > build3ds/out/entapply.txt.
- Shape (ENTRY1 pilot, explosion.c:232). Before: `nd_rec_dl(D_E00328B0[unk_34 % 3]);` After: `nd_draw_list(D_E00328B0[unk_34 % 3], pe_explosion_0);`
3. Gate: nd_host PASS, 1 build (gen_pso_tab: no duplicate; memcheck margin 310 KB or more), listcen recdl = before - n (n = sites), build and
   gfxpos unchanged. Run fxchk_a, b, c: no line matches `psochk bad [a-z]`, each last `psochk f\d+ draws \d+ bad \d+` line has bad 0, and the
   draws of the 3 last lines add to 1 or more.
Fix rights: only include lines, explosion.c:198, entapply.py, the run.sh case, the 2 scene files. A listcen or cmp difference or a psochk bad line:
no commit of that group, write build3ds/out/wse_<g>.txt, send it to the oracle. Never run codemod on src/effects after E3 (a file with one
desc gives it to every nd_rec_dl). Stop after E3. Hand-back (8 lines): hashes, listcen C1 C2 C10, entapply last line, psochk lines, memcheck.
