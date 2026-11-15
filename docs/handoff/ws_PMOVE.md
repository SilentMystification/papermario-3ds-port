# ws_PMOVE: particle lists (NDG_F_PARTS) may move; 189 effect lists move to the gfxin twins (C5 -842, C6 -189, C7 -212)
Model: Haiku. PM_ROW, PM_LANE = PMOVE. Branch PMOVE, worktree build3ds/wt/PMOVE. Budget: 1 build, 8 Azahar runs. Start now. Parallel with ws_GSETW (disjoint lines).
Owns: 3ds/tools/nd_gfx_conv.c, 3ds/include/nd_gtab.h (one new line after :44), 3ds/src/nd_walk.c (:441, :489), 3ds/tests/host.d/pmove.sh (new), 3ds/tools/gset.d/PMOVE.txt (new),
3ds/tools/azt.d/PMOVE.json and 3ds/tools/bench.d/PMOVE.json (new), src/effects/spiky_white_aura.c:8-9, and the files gset.py writes (src/effects, 3ds/tools/gfxin). Edit no other file.
FACT (nd_gfx_conv.c:865): gset_check refuses every list with NDG_F_PARTS (179 lists). FACT (nd_geom.c:345-352): nd_part_draw reads only t->op, no list word. FACT (nd_gfx_conv.c:131-139):
a parts list has only VTX, TRI and END ops; a VTX op points to a Vtx symbol of a .vtx include that stays on the device. FACT (oracle dry run, scratch worktree, :865 rule removed):
`gconv gset ok 1396 of 1546` (was 1211); gset.py on src/effects/gfx/*.c: `gset moved 189 files 76 keep 2` (keeps: spiky_white_aura.c:8-9); listcen C5 10743 -> 9901, C6 915 -> 726, C7 1577 -> 1365.
## Group 1: converter and device (0 builds, 0 runs, commit `PMOVE 1: ...`)
1. nd_gtab.h after :44 add `#define NDG_F_MOVED 2u /* PMOVE: the device v1 table of a moved particle list: list = &ndd_<name>; the fallback runs its NdNDev units */`.
2. nd_gfx_conv.c:865. Before `... && has_ent[i] && !g_parts[i];` After `... && has_ent[i];`. Delete line :879 (`else if (g_parts[i]) { ... particle run (NDG_F_PARTS) ... }`).
   In the comment at :859-860 remove the words of the particle run condition (from `its v1` to `nd_rec.c), and`).
3. conv(), before :154 add `const int dev = g->moved && parts; /* PMOVE: a moved particle list keeps ndgo and a device ndg (no list word) */` and `if (dev) printf("#endif\n");`.
   main wraps conv() of a moved list in `#ifdef ND_HOST_GFX` (:914-916). This `#endif` puts ndgo_<name> in both forms.
4. conv() line :168. Before: one printf of `};\nconst NdGTab ndg_%s = { (const uint32_t*)%s, ...};\n`. After: `printf("};\n");`, then `if (dev) printf("#ifdef ND_HOST_GFX\n");`, then the old
   ndg printf without its leading `};\n`, then `if (dev) printf("#else\nconst NdGTab ndg_%s = { (const uint32_t*)&ndd_%s, ndgo_%s, %u, %uu, %u, %u };\n#endif\n#ifdef ND_HOST_GFX\n",
   g->name, g->name, g->name, no, NDG_F_PARTS | NDG_F_MOVED, nvtx, ntri);`. The last `#ifdef` is closed by the `#endif` of main :915.
5. main :918-924 (nd_gtab_idx). For a moved list with g_parts[i]: print `#ifdef ND_HOST_GFX\n    { X, &ndg_X },\n#else\n    { &ndd_X, &ndg_X },\n#endif\n` (X = the name).
   Other lists stay as now. Count those lists in `n_mp`. The nd_gtab_n line :925: device value `n_lists - n_moved + n_mp`.
6. nd_walk.c:441 and :489. Before `if (!nd_part_draw(t)) nd_ops_run(t->list, ND_LIST_MAX, 1);`
   After `if (!nd_part_draw(t)) { if (t->flags & NDG_F_MOVED) nd_ntab_run((const NdNDev*)(const void*)t->list); else nd_ops_run(t->list, ND_LIST_MAX, 1); }`.
7. New 3ds/tests/host.d/pmove.sh: `n=$(grep -c "(const uint32_t\*)&ndd_" /tmp/gconv/nd_gfx_tab.c)`; n must be 0 now (no list moved); print `pmove ok <n>`.
   Seeded fault: a copy of nd_gfx_tab.c with one extra line `const NdGTab x = { (const uint32_t*)&ndd_x` must count 1.
8. `sh 3ds/tools/nd_host_test.sh > build3ds/out/pmove_h1.txt 2>&1`: `ND HOST TESTS PASS`, `gconv gset ok 1396 of 1546 lists`, `pmove ok 0`. Commit.
## Group 2: the move (0 builds, 0 runs, commit `PMOVE 2: ...`)
9. `python 3ds/tools/listcen.py > build3ds/out/pmove_lc0.txt`. `cut -d' ' -f1,2,4- build3ds/out/ntab_entries.txt > build3ds/out/pmove_e0.txt`.
10. spiky_white_aura.c:8-9. Before `extern Gfx* D_09000440_34EBB0[];` After `extern Gfx D_09000440_34EBB0[];`. Same edit for D_090004E8_34EC58 (the uses pass the address only).
11. `ls src/effects/gfx/*.c > 3ds/tools/gset.d/PMOVE.txt` (LF). `python 3ds/tools/gset.py 3ds/tools/gset.d/PMOVE.txt build3ds/out/gset_ok.txt > build3ds/out/pmove_set.txt 2>&1`.
    Last lines: `gset moved 189 files 76 keep 0` (GUESS: files 76), `gset ok`. Example (snowfall.c twin, new): line 1 `#include "src/effects/gfx/snowfall.c"`, then the 3 moved includes.
    Example (spiky_white_aura.c:189). Before `nd_draw_list(D_09000440_34EBB0, ND_SELF);` After `nd_draw_list(NDN(D_09000440_34EBB0), ND_SELF);`.
12. nd_host_test.sh into pmove_h2.txt: `ND HOST TESTS PASS`, `bad 0` and `moved 1313` on the `gconv lists` line, no `gconv: moved` line, `pmove ok <n>` with n above 0.
    The same cut into pmove_e1.txt; `cmp build3ds/out/pmove_e0.txt build3ds/out/pmove_e1.txt` prints nothing (B1: no entry changes). Commit.
## Group 3: the gate (1 build, 8 runs, commit `PMOVE 3: ...`)
13. New bench fxp_drec: copy the fxr_drec objects of 3ds/tools/azt.d/FXENT3.json and bench.d/FXENT3.json into azt.d/PMOVE.json and bench.d/PMOVE.json. Change name and scenario
    to fxp_drec, debug to `warp=mac_01:0 pos=0:0:200 fxtest=28,56,61,69,90,112,132 loglevel=debug det drec=412:7:44`, until and the drec event to `drec f676 draws [1-9]\\d* bytes \\d+`,
    cap 25, deadline 20. These are the parts effects (assets/us/effects/effect_defs.h: 0x1C, 0x38, 0x3D, 0x45, 0x5A, 0x70, 0x84) that fxr_drec does not run.
14. One build: rc 0, memcheck margin above 0 KB (FACT main: 438 KB). For B in fxp_drec spot_mac01_drec sbk14_drec fxr_drec: `bench.py base $B --row PMOVE --commit $(git merge-base HEAD main)`,
    `bench.py run $B --row PMOVE --why "PMOVE 3"`, `bench.py cmp $B --row PMOVE --commit $(git merge-base HEAD main)`. Pass: byte-equal (one bit fails).
15. `python 3ds/tools/listcen.py > build3ds/out/pmove_lc1.txt` (page FACT: C5 -842, C6 -189, C7 -212, a 2-line difference in C7 is the step 10 edit). Delete this page. Commit.
Fix rights: a converter compile error in your own edit lines. Step 11 or 12 not as given: `git checkout -- src 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin`, send the output path to the oracle.
A cmp difference or a base run that fails: send the output path to the oracle (CLAUDE.md 4 and 5). Never change a base.
Stop: group 3 committed and gated. Hand-back build3ds/out/handback_PMOVE.txt, first 8 lines: commit hashes, `gset ok` and `gset moved` lines, `pmove ok <n>`, the 4 cmp results,
C5, C6, C7 (lc0, lc1), memcheck margin, Azahar runs used.
