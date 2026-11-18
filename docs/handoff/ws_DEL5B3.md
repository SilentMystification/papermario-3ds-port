# ws_DEL5B3 (Haiku): the converter writes the v1 tables (ndg_, ndgo_, nd_gtab_idx) for the host only; nd_gtab_find is host only. Counter: C12 elf data 365 -> 1 (FACT elfgate basebin 7dd3cc76d7956f86: ndg 182, ndgo 182, gMainGfxPos 1). Gate: drec byte-equal, elfgate data 1.
Designer: oracle DEL helper. WAIT: branch from main after the DEL5B2 merge (after it no device code calls nd_gtab_find). Overlaps: none. Worktree build3ds/wt/DEL5B3, branch DEL5B3. PM_ROW=DEL5B3 PM_LANE=DEL5B3.
Edit only (Edit tool, keep CRLF): 3ds/tools/nd_gfx_conv.c (conv :160-179, main :1004-1015), 3ds/src/nd_gtab.c (:11-29, :79). gen_gfx_tab.sh: no edit (it runs the converter as now, gen_gfx_tab.sh:27-29).
## Facts (code read, main 3c2469a282 and its nd_gfx_tab.c)
- FACT every list of the set is moved: the device index count is n_lists - n_moved + n_mp (nd_gfx_conv.c:1015) = 182 = n_mp (nd_gfx_tab.c `nd_gtab_n = 182u`, 182 `&ndd_` keys). main wraps conv() of a moved list in `#ifdef ND_HOST_GFX` (nd_gfx_conv.c:993-995); only the `dev` prints (:160-161, :177, :179) and the index parts branch (:1008-1012) give device v1 symbols.
- FACT the host builds of nd_gtab.c define ND_SCOMP_HOST (conv2c.sh:8, nd_host_test.sh:266) and link nd_gfx_tab.o built with -DND_HOST_GFX (nd_host_test.sh:180). The device part of nd_gtab.c starts at `#ifndef ND_SCOMP_HOST` (:80).
## Edits
1. nd_gfx_conv.c conv(): delete `const int dev = g->moved && parts; ...` (:160), `if (dev) printf("#endif\n");` (:161), `if (dev) printf("#ifdef ND_HOST_GFX\n");` (:177) and the line `if (dev) printf("#else\nconst NdGTab ndg_%s = ...` (:179).
2. nd_gfx_conv.c main(): replace from `printf("NdGIdx nd_gtab_idx[] = {\n");` (:1004) to the printf of nd_gtab_n (:1015) with:
```c
    printf("#ifdef ND_HOST_GFX /* DEL5B3: the v1 tables are host only (the particles read the v2 entry) */\nNdGIdx nd_gtab_idx[] = {\n");
    for (unsigned i = 0; i < gconv_nsym; i++) if (gconv_sym[i].kind == GS_GFX) printf("    { %s, &ndg_%s },\n", gconv_sym[i].name, gconv_sym[i].name);
    printf("};\nconst unsigned nd_gtab_n = %uu;\n#endif\n", n_lists);
```
3. nd_gtab.c: put `#ifdef ND_SCOMP_HOST /* DEL5B3: the v1 index is host only */` before `static int gt_sorted;` (:11) and `#endif` after the closing `}` of nd_gtab_find (:29). nd_tab_init (:79) -> the 5 lines:
```c
void nd_tab_init(void) { /* GKEY2 calls it before the logic thread starts; DEL5B3: the v1 sort on the host only */
#ifdef ND_SCOMP_HOST
    if (!gt_sorted) gt_sort();
#endif
    if (!nt_sorted) nt_sort(); }
```
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 2 Azahar starts of 8)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del5b3_host.txt 2>&1`: PASS (conv2c `sort gtab` and `ntab dev` lines as before). Commit `DEL5B3 1: v1 tables host only`.
2. `python 3ds/tools/bench.py base fxparts_drec --commit $(git merge-base HEAD main) > build3ds/out/del5b3_base.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row DEL5B3 --bench fxparts_drec --no-host --why "DEL5B3 v1 host only" > build3ds/out/del5b3_loop.txt 2>&1` (it builds the tables).
3. `python 3ds/tools/elfgate.py build3ds/lane-DEL5B3/pm_3ds.elf > build3ds/out/del5b3_elf.txt`.
## Gate (exact)
Host PASS. The rowloop exits 0 (byte-equal). del5b3_elf.txt last line `elfgate func <n> data 1 ...` and the data list names only gMainGfxPos.
## Fix rights and stop
Fix rights: the 2 files, compile and link errors only. A device link error for nd_gtab_idx or ndg_: a device caller of nd_gtab_find is left (a DEL5B2 fault): stop and send it. Stop: a DIFF, a crash, a tool fault, data above 1. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait.
## Hand-back
Last commit `DEL5B3 2: gate. fxparts drec equal; elf data 365 -> 1` deletes this page. One SubagentHandback: commit hash, the elfgate line.
