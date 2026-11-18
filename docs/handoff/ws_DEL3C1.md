# ws_DEL3C1 (Haiku): gMainGfxPos and the gbi macro redirect (nd_gx_w) leave the device; the host unit tests keep them in a host file. The boot fallback writes its units directly. The napicen census goes. Counters: C12 elf data -1 (gMainGfxPos; data 0 after DEL5B3), C7 gfxtype -8 (FACT listcen rules per line after DEL1A: nd_rec.c 4, nd_walk.c 2, game_stubs.c 1, include/variables.h 1). Gate: 2 drec byte-equal.
Designer: oracle DEL helper. WAIT: branch from main after the DEL1A and DEL1D2 merges (both write nd_rec.c or nd_walk.c next to these lines). Then DEL3C2. Worktree build3ds/wt/DEL3C1, branch DEL3C1. PM_ROW=DEL3C1 PM_LANE=DEL3C1.
Edit only: 3ds/src/nd_rec.c, nd_walk.c, game_stubs.c; include/variables.h (line 111 only); 3ds/include/nd_cmd.h; new 3ds/tools/nd_gx_host.c; 3ds/tests/host.d/geounit.sh, hunit.sh, nmdl0.sh, ns3unit.sh, stunit.sh, texunit.sh; azt_scenarios.json and bench_defs.json (the 3 napicen objects); `git rm 3ds/tools/napicen.py`; 3ds/tests/lkey/allow.txt (count only).
## Facts (code read, main a5dcbad433; find the quoted text, line numbers move)
- FACT the only device use of a gbi macro is pm_gbi_fallback (nd_walk.c:670-677; git grep of 3ds/src and src). So no device code calls nd_gx_w after edit 3, and gMainGfxPos has no device reader after edit 2 (game_stubs.c:9, :14; nd_rec.c:204-215, :499-504).
- FACT the 6 unit tests link nd_rec.c through one LIBS line each (geounit.sh:7, hunit.sh:7, nmdl0.sh:9, ns3unit.sh:8, stunit.sh:7, texunit.sh:8) and define gMainGfxPos themselves (nd_st_test.c:20 and 4 more). stunit.sh:12-13 seeds a copy of nd_rec.c by the st_put line, which stays.
- FACT the units of edit 3 equal ND_X0, ND_X1 of each old macro word (nd_op.h:79-122): CYCLETYPE a = 20 | 2 << 8 (ND_OTH_A), FILL a 0, RECT a = ND_NR_XY10(319, 239), MTX a = the low 24 bits of the D2 word, VTX a = 3 << 12 | 3 << 1, TRI1 a = __gsSP1Triangle_w1f(0, 1, 2, 0), END a 0.
## Edits
1. New 3ds/tools/nd_gx_host.c (host only): a 3-line header comment (DEL3C1: the gbi macro redirect of the host unit tests; the game writes no gbi macro), the includes `<stdint.h>`, "ultra64.h", "pm_port.h", "nd_cmd.h", "nd_op.h", "nd_fs.h", "nd_gbi.h", then `extern Gfx* gMainGfxPos; /* the test defines it */`, `#define IS_NOP(w0) ((w0) == ND_NOP_A)`, `Gfx nd_rec_top[8];`, `Gfx* nd_rec_task(void) { return nd_rec_top; }`, then the lines moved from nd_rec.c: `static uint32_t tr_w[3];` to the end of nd_gx_w, without the NAPI0 census (from `/* NAPI0 census` to the end of napi_note) and without the napicen line of nd_gx_w; `rec_inline(w1)` -> `nd_rec_inline(w1)`.
2. nd_rec.c: delete the D9 comment, `Gfx nd_rec_top[8];`, stray_word (:200-210) and `Gfx* nd_rec_task(void) ...` (:215). rec_open -> `static void rec_open(void) { task_open(); } /* DEL3C1: no top-level packet on the device */`. Delete the moved lines (:461-517). `static int rec_inline(uint32_t a)` -> `int nd_rec_inline(uint32_t a)`, its call in nd_rec_list too. nd_cmd.h after nd_rec_list: `int nd_rec_inline(uint32_t a); /* DEL3C1: nd_rec.c, a push call of an in-line state list; nd_gx_host.c */`.
3. nd_walk.c pm_gbi_fallback: `static Gfx dl[8];` -> `static uint32_t dl[16];`. Replace from `Gfx* g = dl;` to `gSPEndDisplayList(g++);` with:
```c
        uint32_t* nrp = dl; /* DEL3C1: the units of the old gbi macros (ND_X0, ND_X1 of each word) */
        ND_NR_AT(ND_NR_OM(1, G_MDSFT_CYCLETYPE, 2), G_CYC_FILL);
        ND_NR_AT(ND_FS_W0(ND_FS_FILL, 0u), 0x22292229u);
        ND_NR_AT(ND_FS_W0(ND_FS_RECT, ND_NR_XY10(319, 239)), ND_NR_XY10(0, 0));
        ND_NR_AT(ND_NR_OM(1, G_MDSFT_CYCLETYPE, 2), G_CYC_1CYCLE);
        ND_NR_AT(ND_FS_W0(ND_FS_MTX, ND_W0_D2(0, sizeof(Mtx), (G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH) ^ G_MTX_PUSH, 0)), (uint32_t)(uintptr_t)&fm);
        ND_NR_AT(ND_NR_VTX(3, 0), (uint32_t)(uintptr_t)fv);
        ND_NR_AT(ND_FS_W0(ND_FS_TRI1, __gsSP1Triangle_w1f(0, 1, 2, 0)), 0u);
        ND_NR_AT(ND_FS_W0(ND_FS_END, 0u), 0u);
```
   Add `#include "nd_nr.h"` after `#include "nd_gbi.h"`; that include comment -> `/* ND_W0_D2 of the fallback matrix unit */`.
4. game_stubs.c: delete line 9 (`Gfx* gMainGfxPos; ...`) and line 14 (`gMainGfxPos = nd_rec_task(); ...`). include/variables.h: delete line 111 `extern Gfx* gMainGfxPos;`.
5. `sed -b -i 's#3ds/src/nd_rec.c #3ds/src/nd_rec.c 3ds/tools/nd_gx_host.c #' 3ds/tests/host.d/geounit.sh 3ds/tests/host.d/hunit.sh 3ds/tests/host.d/nmdl0.sh 3ds/tests/host.d/ns3unit.sh 3ds/tests/host.d/stunit.sh 3ds/tests/host.d/texunit.sh`.
6. Delete the objects napicen_mac01, napicen_talk, napicen_btl from azt_scenarios.json and bench_defs.json (the `{` line to its `},` line; a scenario that is absent is no error), then `python -c "import json; json.load(open('3ds/tools/azt_scenarios.json')); json.load(open('3ds/tools/bench_defs.json')); print('json ok')"`. `git rm 3ds/tools/napicen.py`.
7. `git grep -nwE "gMainGfxPos|nd_gx_w|nd_rec_top|napi_note|napicen" -- 3ds/src src/*.c include/variables.h > build3ds/out/del3c1_grep.txt`: comment lines only.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-7. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del3c1_host.txt 2>&1`: PASS (stunit, texunit, geounit, hunit, nmdl0, ns3unit lines as before). Commit `DEL3C1 1: gMainGfxPos and nd_gx_w host only`.
2. `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main) > build3ds/out/del3c1_base1.txt 2>&1`. `sh 3ds/tools/rowloop.sh --row DEL3C1 --bench spot_mac01_drec --no-host --why "DEL3C1 gMainGfxPos" > build3ds/out/del3c1_loop1.txt 2>&1`. Then the same 2 commands for kkj14_cgfx_drec (del3c1_base2.txt, del3c1_loop2.txt).
3. `python 3ds/tools/elfgate.py build3ds/lane-DEL3C1/pm_3ds.elf > build3ds/out/del3c1_elf.txt`. `python 3ds/tools/listcen.py > build3ds/out/del3c1_listcen.txt`.
## Gate (exact)
Host PASS. Both rowloops exit 0 (byte-equal). The device link has no undefined nd_gx_w. del3c1_grep.txt holds comments only. json ok. del3c1_elf.txt names no gMainGfxPos (data 0 when DEL5B3 is merged). listcen gfxtype 8 below the start commit; reader equal.
## Fix rights and stop
Fix rights: the listed files; compile and link errors (a host test that misses nd_gx_host.c: add it to its LIBS line, name it in the commit). `lkey census bad <file> <n> listed <m>`: set that count in allow.txt to n. Stop: a DIFF, a crash, a tool fault, a device reference to nd_gx_w. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time; one bench per rowloop call; `sed -b` only.
## Hand-back
Last commit `DEL3C1 2: gate. spot_mac01 and kkj14_cgfx drec equal; elf data -1, gfxtype -8` deletes this page. One SubagentHandback: commit hash, the elfgate line, listcen totals.
