# ws_VINE (Haiku): the 2 lava piranha vine lists move to gfxin twins (C5 data -34, C1 build -1, C2 gfxpos -1)
Designer: the oracle (SendMessage). Worktree build3ds/wt/VINE, branch VINE. PM_ROW=VINE PM_LANE=VINE. Start now.
No shared file with NMDL0, GFXINC2, MAINBIN, ROOTREST, DLCCHK (FACT: git diff --name-only of each branch, oracle 2026-11-16).
## Objective and counters
- FACT (gset_ok.txt of build 0bfe6ce6f6, lines 1665-1666): kmr_22_lava_piranha_vine_gfx and kzn_19_lava_piranha_vine_gfx are gset ok.
  Each is a state-only setup list (assets/us/world/lava_piranha/vine.gfx.inc.c:3-20). The only user is part2.inc.c:199 (FACT git grep).
- FACT (code read): gset.py cannot move them. kzn_19's asset names the list `Gfx N(x)[]` (gset.py:46 DEF needs a plain name).
  kmr_22's include sits in npc/lava_piranha.inc.c:9, not in the listed source kmr_22/npc.c (gset.d/GSETI.txt:10). The patch moves them by hand
  in the gset form: the twin includes the device source, then the asset (example 3ds/tools/gfxin/src/world/area_kmr/kmr_21/main.c).
- FACT (listcen file data): vine.gfx.inc.c 17, lava_piranha_vine.gfx.inc.c 17. Expected: data -34, build -1, gfxpos -1.
  GUESS: data_arrays -2, gfxtype -2. reader, model, recdl: 0 change.
## Files (all in docs/handoff/ws_VINE.patch). Edit only these 5.
src/world/area_kzn/kzn_19/npc.c, src/world/area_kmr/kmr_22/npc/lava_piranha.inc.c, src/world/common/lava_piranha/part2.inc.c,
new: 3ds/tools/gfxin/src/world/area_kzn/kzn_19/npc.c, new: 3ds/tools/gfxin/src/world/area_kmr/kmr_22/npc.c.
## Edit shapes (the patch does them; use them to check a reject)
Before (src/world/area_kzn/kzn_19/npc.c:14): `#include "world/lava_piranha/vine.gfx.inc.c"`  After: `NDN_DECL(N(lava_piranha_vine_gfx));`
Before (kmr_22/npc/lava_piranha.inc.c:9): `#include "world/area_kmr/kmr_22/lava_piranha_vine.gfx.inc.c"`  After: `NDN_DECL(kmr_22_lava_piranha_vine_gfx);`
Both files also get `#include "nd_gtab.h"` after `#include "include_asset.h"`.
Before (part2.inc.c:199): `gSPDisplayList(gMainGfxPos++, N(lava_piranha_vine_gfx));`  After: `nd_draw_list(NDN(N(lava_piranha_vine_gfx)), ND_SELF);`
New twin 3ds/tools/gfxin/src/world/area_kzn/kzn_19/npc.c (2 lines): `#include "src/world/area_kzn/kzn_19/npc.c"`, `#include "world/lava_piranha/vine.gfx.inc.c"`.
## Loop (1 build, 1 Azahar start; base runs not charged)
1. `python 3ds/tools/listcen.py > build3ds/out/vine_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_VINE.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/vine_host.txt 2>&1`. Read the last 20 lines. Pass: `ND HOST TESTS PASS`, `lkey census ok`.
4. `grep -c "lava_piranha_vine_gfx" build3ds/out/ntab_tables.txt` gives 2, and `grep -c "lava_piranha_vine_gfx" build3ds/out/gset_ok.txt` gives 2.
5. `python 3ds/tools/listcen.py > build3ds/out/vine_lc1.txt`. Pass vs vine_lc0.txt: data -34, build -1, gfxpos -1. Write the data_arrays
   and gfxtype change in the commit. Commit the 5 files: `VINE 1: the lava piranha vine lists move to gfxin twins; part2 site takes ND_SELF`.
6. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0. Then
   `grep -c "ndd_kzn_19_lava_piranha_vine_gfx\|ndd_kmr_22_lava_piranha_vine_gfx" 3ds/src/nd_gfx_tab.c` gives 2 or more.
7. `python 3ds/tools/bench.py base spot_mac01_drec`. Gate: `python 3ds/tools/bench.py run spot_mac01_drec --row VINE --why "VINE gate"`, then
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row VINE --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
   FACT: no bench reaches kzn_19 or kmr_22. The host test and step 6 prove the tables. The user checks the vine on the console.
## Fail
- A failing test or a compile error: you may edit only the 5 files. 2 tries for one cause. Then write the output to build3ds/out/ and
  send the path to the oracle.
- A base run that fails or hangs, or a byte diff in the gate: stop. Send the log or cmp output path to the oracle.
## Stop
The gate passes. The last commit `VINE 2: gate` deletes ws_VINE.md and ws_VINE.patch.
## Hand-back (one SubagentHandback message)
Commits. listcen data, data_arrays, gfxtype, build, gfxpos before and after. memcheck margin. The cmp line.
