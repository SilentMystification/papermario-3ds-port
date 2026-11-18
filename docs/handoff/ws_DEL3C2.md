# ws_DEL3C2 (Haiku): nd_gbi.h keeps only the device word helpers, in ND_FS form; the gbi macro redirects move to a host header for the unit tests. nd_pso_som compares a named value. Counter: C8 reader -12 (FACT listcen rules per line, main a5dcbad433: nd_gbi.h 11, nd_st.c 1). Gate: 2 drec byte-equal.
Designer: oracle DEL helper. WAIT: branch from main after the DEL3C1 merge (nd_gx_host.c includes nd_gbi.h; DEL1D2 writes nd_direct.c next to :343). Worktree build3ds/wt/DEL3C2, branch DEL3C2. PM_ROW=DEL3C2 PM_LANE=DEL3C2.
Edit only: 3ds/include/nd_gbi.h, nd_at.h, nd_st.h; 3ds/src/nd_st.c, nd_direct.c (:343); new 3ds/tools/gbi_inc/nd_gbi_host.h; 3ds/tools/nd_at_test.c, nd_geo_test.c, nd_st_test.c, nd_tx_test.c, nd_val_test.c, nd_gx_host.c; 3ds/tests/nmdl0/old_mtg.c; 3ds/tests/lkey/allow.txt (count only).
## Facts (code read, main a5dcbad433; find the quoted text, line numbers move)
- FACT after DEL3C1 no device file uses a gbi macro redirect (git grep of 3ds/src, src: only pm_gbi_fallback did). The device uses of nd_gbi.h are the helpers ND_CMB_W (324 uses), ND_CMB_NOP, ND_W0_CMB, ND_W1_CMB, ND_W0_TR, ND_W1_TR, ND_TR_H1, ND_TR_H2, ND_W0_PMND, ND_W0_D2 (nd_walk.c fallback, nd_direct.c), ND_W0_VTX (nd_direct.c:343 only).
- FACT the new forms give the same words: G_SETCOMBINE is 0xFC (ND_FS_COMB_W0, nd_fs.h:52). ND_X0 of a combine word is `NDO_W0(NDO_COMB, w0)`, of G_NOOP is ND_NOP_A, of a G_PM_ND word is `NDO_W0(NDO_MISC, NDO_MISC_ND << 16 | (kind & 0xff) << 8 | arg)`; ND_X1 gives w1, w1 and 0 (nd_op.h:79-122).
- FACT the A/B unit tests include nd_gbi.h for the macro forms (nd_st_test.c and 5 more, old_mtg.c) and compile with `-I 3ds/tools/gbi_inc` (each of the 6 host.d scripts).
## Edits
1. New 3ds/tools/gbi_inc/nd_gbi_host.h: a guard ND_GBI_HOST_H, a 2-line comment (DEL3C2: the gbi macro redirect of the host unit tests), `#include "nd_gbi.h"`, then the old nd_gbi.h lines 13-241 without the definitions that edit 2 keeps (ND_W0_CMB, ND_W1_CMB, ND_CMB_W_, ND_CMB_W, ND_CMB_NOP, ND_W0_D2, ND_W0_PMND, ND_W0_TR, ND_W1_TR, ND_TR_H1, ND_TR_H2).
2. 3ds/include/nd_gbi.h becomes (its header comment rewritten to 2 lines: the device word helpers; the macro redirect is nd_gbi_host.h):
```c
#ifndef ND_GBI_H
#define ND_GBI_H
#include "nd_cmd.h"
#include "nd_op.h"
#include "nd_fs.h"
#define ND_W0_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    ND_FS_COMB_W0(GCCc0w0(G_CCMUX_##a0, G_CCMUX_##c0, G_ACMUX_##Aa0, G_ACMUX_##Ac0) | GCCc1w0(G_CCMUX_##a1, G_CCMUX_##c1)) /* DEL3C2: the 0xFC combine word */
<ND_W1_CMB as it is>
#define ND_CMB_W_(w0, w1) { ND_FS_W0(ND_FS_COMB, (w0)), (w1) } /* DEL3C2: the typed combine unit (ND_X0, ND_X1 of the combine word) */
#define ND_CMB_W(...) ND_CMB_W_(ND_W0_CMB(__VA_ARGS__), ND_W1_CMB(__VA_ARGS__))
#define ND_CMB_NOP { ND_NOP_A, 0u }
<ND_W0_D2, ND_W0_PMND, ND_W0_TR, ND_W1_TR, ND_TR_H1, ND_TR_H2 as they are>
#endif
```
3. nd_at.h NDCTL_ND -> `#define NDCTL_ND(O, kind, arg) O(ND_FS_W0(ND_FS_MISC, (uint32_t)ND_FS_MISC_ND << 16 | ((uint32_t)(kind) & 0xffu) << 8 | ((uint32_t)(arg) & 0xffu)), 0u) /* DEL3C2: the unit of the G_PM_ND word */`.
4. nd_direct.c :343 `ND_W0_VTX(4, 0) & 0xffffffu` -> `ND_NR_VTX(4, 0) & 0xffffffu`.
5. nd_st.h, after its includes: `#define ND_SOM_H 0xe3u /* DEL3C2: G_SETOTHERMODE_H of gbi.h, the high word value of the cmd argument of nd_pso_som */`. nd_st.c:59 `(cmd) == G_SETOTHERMODE_H` -> `(cmd) == ND_SOM_H`. nd_st_test.c, after its includes: `_Static_assert(ND_SOM_H == G_SETOTHERMODE_H, "ND_SOM_H");`.
6. `#include "nd_gbi.h"` -> `#include "nd_gbi_host.h"` in nd_at_test.c, nd_geo_test.c, nd_st_test.c, nd_tx_test.c, nd_val_test.c, nd_gx_host.c and 3ds/tests/nmdl0/old_mtg.c.
7. `grep -nE "\bG_(DL|VTX|TRI1|TRI2|TEXRECT|SETCOMBINE|SETOTHERMODE_[HL]|MTX|POPMTX)\b|\bNDO_|ND_X[01]\b" 3ds/include/nd_gbi.h 3ds/src/nd_st.c > build3ds/out/del3c2_grep.txt`: no line.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-7. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del3c2_host.txt 2>&1`: PASS (stunit, texunit, geounit, hunit, nmdl0, ns3unit as before). Commit `DEL3C2 1: nd_gbi.h device helpers only`.
2. `python 3ds/tools/bench.py base wscmb_pause_drec --commit $(git merge-base HEAD main) > build3ds/out/del3c2_base1.txt 2>&1`. `sh 3ds/tools/rowloop.sh --row DEL3C2 --bench wscmb_pause_drec --no-host --why "DEL3C2 combine units" > build3ds/out/del3c2_loop1.txt 2>&1`. Then the same 2 commands for spot_mac01_drec (del3c2_base2.txt, del3c2_loop2.txt).
3. `python 3ds/tools/listcen.py > build3ds/out/del3c2_listcen.txt`.
## Gate (exact)
Host PASS. Both rowloops exit 0 (byte-equal). del3c2_grep.txt empty. listcen reader 12 below the start commit; no file reader line for nd_gbi.h, nd_st.c.
## Fix rights and stop
Fix rights: the listed files; compile errors (a device file that used a moved helper: stop and send it, do not move the helper back). `lkey census bad <file> <n> listed <m>`: set that count in allow.txt to n. Stop: a DIFF, a crash, a tool fault, a counter off the gate. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time; one bench per rowloop call; `sed -b` only.
## Hand-back
Last commit `DEL3C2 2: gate. wscmb_pause and spot_mac01 drec equal; reader -12` deletes this page. One SubagentHandback: commit hash, listcen totals.
