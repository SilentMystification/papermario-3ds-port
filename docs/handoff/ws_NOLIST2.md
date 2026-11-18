# ws_NOLIST2 (Haiku): nd_nolist.h is generated again from the current headers (305 -> 179 poisoned names). Counter: none of C1-C12 (tool header). Gate: host PASS, the header equals the generator output.
Designer: oracle DEL (coach). Start commit main (after e898a2c242, the DEL4D12 merge). No WAIT: no running row edits these files. Worktree build3ds/wt/NOLIST2, branch NOLIST2. PM_ROW=NOLIST2 PM_LANE=NOLIST2.
Edit only: 3ds/include/nd_nolist.h (generated, CRLF).
## Facts (code read, main 25e4e6f121)
- FACT nd_nolist.h was written once by gen_nolist.py (c0ecf0cbb9, 303 macro names + Gfx + gMainGfxPos = 305).
- FACT GBIHDR 2 (451cbe1b7e) deleted 125 unused macros: the generator then gives 180. DEL4D34 (bd2f6628f2) deleted gt.h: 179. DEL4D12 moved the macros to 3ds/tools/gbi_inc/PR/gbi.h and added it to HEADERS (gen_nolist.py:20): still 179, 0 names added.
- FACT none of the 126 dropped names occurs in src, include, 3ds/src, 3ds/include, 3ds/tools/gbi_inc or 3ds/tools/gfxin (token scan). The header is stale. The generator is correct.
- FACT gen_nolist.py writes LF (gen_nolist.py:58). The committed file is CRLF (core.autocrlf true). nolist.sh (3ds/tests/host.d) poisons gDPPipeSync, gsSPEndDisplayList, gSPDisplayList: all 3 are in the 179.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 0 builds, 0 Azahar starts)
1. `python 3ds/tools/gen_nolist.py > build3ds/out/nolist2_gen.txt 2>&1`. Then `grep -c "pragma GCC poison" 3ds/include/nd_nolist.h`: 179.
2. `git add 3ds/include/nd_nolist.h` (autocrlf stores it as in the index form). `git diff --cached --stat`: one file, about 252 lines deleted, 0 added.
3. `PM_HOSTSEL=all sh 3ds/tools/nd_host_test.sh > build3ds/out/nolist2_host.txt 2>&1`: PASS. The line `nolist ok good 2 bad 5` is in the output.
4. Commit `NOLIST2 1: nd_nolist.h generated again (179 names)`.
## Gate (exact)
Host PASS. `nolist ok good 2 bad 5`. 179 poison lines. `git diff --cached` of step 2 shows only deleted `#undef` and `#pragma GCC poison` lines.
## Fix rights
None. An added line, a count other than 179, a host FAIL: stop. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait.
## Hand-back
Last commit `NOLIST2 2: gate. 179 names; host PASS` deletes this page. One SubagentHandback: commit hash, the poison count, the nolist line.
