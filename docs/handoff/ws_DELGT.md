# ws_DELGT (Haiku): the port code names its 8-byte word pair NdW2, not the N64 Gfx. Same layout, no logic change. Counter: C7 gfxtype to 1 (FACT listcen rules per line, main a5dcbad433: every Gfx line of 3ds/src and include/gbi_custom.h:33 goes; the one left is the typedef include/PR/gbi.h:1657). Gate: 2 drec byte-equal.
Designer: oracle DEL helper. WAIT: branch from main after the DEL2E, DEL3C2 and DEL3C3 merges (each writes lines that hold Gfx: nd_ent.c, nd_gbi.h, nd_rec.c, nd_walk.c). Worktree build3ds/wt/DELGT, branch DELGT. PM_ROW=DELGT PM_LANE=DELGT.
Edit only: 3ds/src/*.c, 3ds/include/*.h (not nd_nolist.h), include/gbi_custom.h (the macro line 33 and its comments), 3ds/tools/*.c (casts only, fix rights), 3ds/tests/lkey/allow.txt (count only).
## Facts (code read, main a5dcbad433)
- FACT Gfx is the union of include/PR/gbi.h:1628-1657: 8 bytes, `words` is Gwords `{ unsigned int w0; unsigned int w1; }`, `long long force_structure_alignment` sets the 8-byte alignment. The port code reads only `.words.w0`, `.words.w1` and the size (git grep `\.words\.` in 3ds/src).
- FACT game TUs build with `-Wno-incompatible-pointer-types` (CMakeLists.txt:326); port TUs (3ds/src) do not, and the host tests use -Werror: a missed Gfx/NdW2 pair is a compile error, not a silent change.
- FACT the game code names no Gfx (listcen gfxtype has no src file line). include/nd_nolist.h poisons the name Gfx for the game (keep it).
## Edits
1. 3ds/include/nd_cmd.h, after its includes:
```c
/* DELGT: the 8-byte word pair of a record or a unit (the layout of the N64 Gfx union: w0, w1, 8-byte aligned) */
typedef union NdW2 { struct { unsigned int w0, w1; } words; long long force_structure_alignment; } NdW2;
_Static_assert(sizeof(NdW2) == 8 && _Alignof(NdW2) == 8, "NdW2");
```
2. One command (bytes in, bytes out, keeps CRLF; nd_cmd.h is skipped so the typedef stays):
```python
python - <<'EOF'
import re, glob, os
skip = {os.path.normpath(p) for p in ('3ds/include/nd_cmd.h', '3ds/include/nd_nolist.h')}
n = 0
for p in glob.glob('3ds/src/*.c') + glob.glob('3ds/include/*.h') + ['include/gbi_custom.h']:
    if os.path.normpath(p) in skip: continue
    b = open(p, 'rb').read(); c = re.sub(rb'\bGfx\b', b'NdW2', b)
    if c != b: open(p, 'wb').write(c); n += 1
print('delgt files', n)
EOF
```
3. A file of edit 2 that names NdW2 and does not see nd_cmd.h: add `#include "nd_cmd.h"` after its last include (include/gbi_custom.h: put it before the macros).
4. `git grep -nw "Gfx" -- 3ds/src 3ds/include/*.h include/gbi_custom.h ':!3ds/include/nd_nolist.h' > build3ds/out/delgt_grep.txt`: no line.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/delgt_host.txt 2>&1`: PASS. Commit `DELGT 1: the port word pair is NdW2`.
2. `python 3ds/tools/bench.py base entcape_kmr03_drec --commit $(git merge-base HEAD main) > build3ds/out/delgt_base1.txt 2>&1`. `sh 3ds/tools/rowloop.sh --row DELGT --bench entcape_kmr03_drec --no-host --why "DELGT word pair type" > build3ds/out/delgt_loop1.txt 2>&1`. Then the same 2 commands for spot_mac01_drec (delgt_base2.txt, delgt_loop2.txt).
3. `python 3ds/tools/listcen.py > build3ds/out/delgt_listcen.txt`.
## Gate (exact)
Host PASS. Both rowloops exit 0 (byte-equal). delgt_grep.txt empty. listcen gfxtype 1 (`listcen file gfxtype include/PR/gbi.h 1` only) when DEL3C1 is merged; reader and model not higher.
## Fix rights and stop
Fix rights: the listed files; compile errors of the retype: an incompatible pointer between a port NdW2* and a game or host Gfx* (cast it at that call, `(NdW2*)` or `(Gfx*)`, name each cast in the commit). `lkey census bad <file> <n> listed <m>`: set that count in allow.txt to n. Stop: a DIFF, a crash, a tool fault, more than 10 casts (send the list). Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time; one bench per rowloop call.
## Hand-back
Last commit `DELGT 2: gate. entcape and spot_mac01 drec equal; gfxtype -> 1` deletes this page. One SubagentHandback: commit hash, listcen totals, the cast list.
