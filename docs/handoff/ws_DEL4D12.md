# ws_DEL4D12 (Haiku): the gbi macros of gbi.h and gbi_custom.h leave the device include folder (W14 DEL4D1 + DEL4D2, early: no device caller). Counters: C1 build 306 -> 0, C5 data 191 -> 0, C7 gfxtype 230 -> 124 (GUESS), C8 reader 417 -> 419 (GUESS, 2 tokens in nd_gbi.h).
Designer: oracle DEL (coach). Start commit main ae113a0f29. No WAIT: no running row edits these files. Worktree build3ds/wt/DEL4D12, branch DEL4D12. PM_ROW=DEL4D12 PM_LANE=DEL4D12.
Edit only: include/PR/gbi.h, include/gbi_custom.h (both by the script), new 3ds/tools/gbimove.py (from ws_DEL4D12.patch), new 3ds/tools/gbi_inc/PR/gbi.h (script output), 3ds/include/nd_gbi.h (end), 3ds/src/nd_gtab.c (nd_rec_dl), 3ds/tools/*.sh and 3ds/tests/host.d/*.sh (the -I sites), 3ds/tools/gen_nolist.py (HEADERS), 3ds/tools/gbiprune.py (HDR).
## Facts (code read, main ae113a0f29)
- FACT listcen: build is gbi.h 245 + gbi_custom.h 61; data is gbi.h 191; gfxtype gbi.h 99, gbi_custom.h 9. Game src build 0.
- FACT device callers of the gbi macros: nd_walk.c:804-811 (pm_gbi_fallback, through nd_gbi.h) and nd_gtab.c:128 (`gSPDisplayList(gMainGfxPos++, list);`). nd_gtab.c does not include nd_gbi.h: the line writes a raw G_DL word at nd_rec_top. stray_word (nd_rec.c:231-237) logs it and drops it.
- FACT the fallback needs 3 derived forms that nd_gbi.h lacks: gDPSetCycleType (gbi.h:2506), gDPSetFillColor (:2715), gSPMatrix (:1712, F3DEX_GBI_2). The others (gDPFillRectangle, gSPVertex, gSP1Triangle, gSPEndDisplayList, gDma2p, gDPSetColor, gSPSetOtherMode) are in nd_gbi.h.
- FACT gconv (gen_gfx_tab.sh:13-14), the host tests and ppcmp.sh need the macros. 56 `-I include ` sites in 3ds/tools/*.sh and 3ds/tests/host.d/*.sh. Device CMake (3ds/CMakeLists.txt:22-29) has no 3ds/tools folder.
- FACT scratch run of gbimove.py on a copy: `gbimove include/PR/gbi.h blocks 220 moved 1441 kept 2546`, `gbimove include/gbi_custom.h blocks 11 moved 192 kept 63`. gbi_custom.h keeps gPmEntDrawRecord (nd_rec.c:294 uses it).
## Edits
1. `git apply docs/handoff/ws_DEL4D12.patch` (adds 3ds/tools/gbimove.py). Run `python 3ds/tools/gbimove.py 3ds/tools/gbi_inc/PR/gbi.h ND_GBI_INC_H include/PR/gbi.h include/gbi_custom.h`. Expect the 2 lines above.
2. nd_gbi.h: before the last line `#endif` add:
```c
/* DEL4D12: the derived forms of the device fallback (nd_walk.c pm_gbi_fallback); the other gbi macros are in 3ds/tools/gbi_inc/PR/gbi.h (host only) */
#undef gDPSetCycleType
#define gDPSetCycleType(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_CYCLETYPE, 2, type)
#undef gDPSetFillColor
#define gDPSetFillColor(pkt, d) gDPSetColor(pkt, G_SETFILLCOLOR, (d))
#undef gSPMatrix
#define gSPMatrix(pkt, m, p) gDma2p((pkt), G_MTX, (m), sizeof(Mtx), (p) ^ G_MTX_PUSH, 0)
```
3. nd_gtab.c nd_rec_dl. Before: `        gSPDisplayList(gMainGfxPos++, list);` After: `        if (gtr_dl <= 4u) pm_log_e("ndc: list %p has no table f%u", list, pm_frame_count()); /* DEL4D12: no raw G_DL word (stray_word dropped it) */` (the `gtr_dl++;` line above stays).
4. `sed -b -i 's#-I include #-I 3ds/tools/gbi_inc -I include #g' $(grep -l -- '-I include ' 3ds/tools/*.sh 3ds/tests/host.d/*.sh)`. Check: `grep -ho -- '-I 3ds/tools/gbi_inc -I include ' <same files> | wc -l` equals the count of `-I include ` before the sed.
5. gen_nolist.py HEADERS and gbiprune.py HDR: add `"3ds/tools/gbi_inc/PR/gbi.h"` after the gbi_custom.h entry. gbiprune.py docstring line 2: add the name.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`)
0. Before any edit: `PM_LANE=DEL4D12 sh 3ds/build.sh > build3ds/out/del_b0.txt 2>&1`. Copy 3ds/src/nd_gfx_tab.c, nd_ntab_descs.c, nd_pso_tab.c to build3ds/out/base_<name>.
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del_host.txt 2>&1`: PASS. `python 3ds/tools/gen_nolist.py && git diff --ignore-cr-at-eol --exit-code 3ds/include/nd_nolist.h`: no diff; then `git checkout -- 3ds/include/nd_nolist.h`. Commit `DEL4D12 1: gbi macros to 3ds/tools/gbi_inc`.
2. `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`. `sh 3ds/tools/rowloop.sh --row DEL4D12 --bench spot_mac01_drec --no-host --why "DEL4D12 gbi macros out"` (it builds).
3. `cmp` each of the 3 generated files with its base copy: all equal. `python 3ds/tools/listcen.py > build3ds/out/del_listcen.txt`. `grep -a "gtabrun\|has no table" build3ds/azt/logs/spot_mac01_drec.txt | tail -3` (data).
## Gate (exact)
Host PASS. nolist no diff. 3 cmp equal. rowloop LOOP PASS (byte-equal). listcen: build 0, data 0. Report gfxtype, reader, model, core1 (base FACT main ae113a0f29: gfxtype 230, reader 417, model 11, core1 455).
## Fix rights
The listed files. A host, gconv or gen compile that fails on a gDP/gSP/gsDP/gsSP/gDma/gImmp name: add `-I 3ds/tools/gbi_inc` before `-I include` on that line (a Python argument list: `"-I", "3ds/tools/gbi_inc",` before `"-I", "include",`). Note each fix in the commit. A device compile error, a cmp difference, a DIFF, build or data above 0: stop. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait.
## Hand-back
Last commit `DEL4D12 2: gate. spot_mac01 drec equal; build 306 -> 0, data 191 -> 0, gfxtype 230 -> <n>` deletes this page and ws_DEL4D12.patch. One SubagentHandback: commit hash, listcen totals, the gtabrun line.
