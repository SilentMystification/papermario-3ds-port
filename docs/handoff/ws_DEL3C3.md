# ws_DEL3C3 (Haiku): the NDO_ unit ids go; every file names the ND_FS_ twins. nd_op.h keeps the device helpers and the word translator (ND_X0, ND_X1) in ND_FS names; nd_op_n64 moves into its one host test. Counter: C8 reader -162 (FACT listcen rules per line, main a5dcbad433: nd_op.h 173 = 153 NDO_ + 9 in nd_op_n64 + 11 in ND_X0/ND_X1; the 11 stay, see the last fact). Gate: 2 drec byte-equal.
Designer: oracle DEL helper. WAIT: branch from main after the DEL2 and DEL3C2 merges (DEL2 renames NDO_ in the setter files; DEL3C2 writes nd_gbi.h). Worktree build3ds/wt/DEL3C3, branch DEL3C3. PM_ROW=DEL3C3 PM_LANE=DEL3C3.
Edit only: the files that edit 1 writes; 3ds/include/nd_op.h; 3ds/tools/nd_cmd_test.c; `git rm` 3ds/tests/host.d/ncv1a.sh and 3ds/tools/nd_fs_test.c; 3ds/tests/lkey/allow.txt (count only).
## Facts (code read, main a5dcbad433)
- FACT every NDO_ name has an ND_FS_ twin of the same value (nd_op.h:11-56, nd_fs.h:8-53; 45 names checked by this oracle, none without a twin). ncv1a (ncv1a.sh, nd_fs_test.c) only asserts that equality: with no NDO_ ids it has no subject.
- FACT 47 files name NDO_ today, among them game files (src/entity_model.c 8, src/model.c 5, src/imgfx.c 4, include/model.h 1 and 5 more), host tests (maplist_test.c, vcopy_test.c, mapconv_test.c) and vcopy.sh:10 (`grep -q "NDO_VTX"` on code that edit 1 renames too).
- FACT nd_op_n64 has one reader left after DEL1D1: nd_cmd_test.c:63, :147 (host).
- FACT ND_X0 and ND_X1 stay on the device: src/imgfx.c:1286-1294 DMA-copies raw F3DEX2 words of the imgfx_data ROM segment and nd_imgfx_type (nd_imgfx.h:12-13) types them at run time. Their 11 G_ tokens (nd_op.h:79-120) need a user decision (oracle hand-back), not this row.
## Edits
1. One command (bytes in, bytes out, keeps CRLF):
```python
python - <<'EOF'
import re, glob, os
F = set()
for g in ('3ds/src/*.c', '3ds/include/*.h', '3ds/tools/*.c', 'src/**/*.c', 'src/**/*.h', 'include/**/*.h', '3ds/tests/**/*.c', '3ds/tests/host.d/*.sh'):
    F |= {os.path.normpath(p) for p in glob.glob(g, recursive=True)}
F.discard(os.path.normpath('3ds/include/nd_op.h'))
n = 0
for p in sorted(F):
    b = open(p, 'rb').read(); c = re.sub(rb'\bNDO_', b'ND_FS_', b)
    if c != b: open(p, 'wb').write(c); n += 1
print('del3c3 files', n)
EOF
```
2. nd_op.h: `#include "ultra64.h" /* ... */` -> `#include "ultra64.h" /* the F3DEX2 op values of ND_X0 */` and add `#include "nd_fs.h"` after it. Delete old lines 11-56 (the 4 NDO_ enums, NDO_RECT_TEX, NDO_W0, NDO_COMB_W0, NDO_A_RENDERMODE). Delete nd_op_n64 with its comment (`/* The F3DEX2 op of each typed id` and the table, old :136-142). Then `python -c "import re;p='3ds/include/nd_op.h';b=open(p,'rb').read();open(p,'wb').write(re.sub(rb'\bNDO_',b'ND_FS_',b))"`. The header comment (:1-5): the ids are ND_FS_ (nd_fs.h); this file holds the unit helpers and the word translator of the imgfx anim lists (DEL3C3).
3. nd_cmd_test.c: before its first function, add the deleted table as `static const uint8_t nd_op_n64[ND_FS_END_ - ND_FS_BASE] = { ... };` (the same 31 values, the comment `/* DEL3C3: moved from nd_op.h, the F3DEX2 op of each unit id */`).
4. `git rm 3ds/tests/host.d/ncv1a.sh 3ds/tools/nd_fs_test.c`.
5. `git grep -nE "\bNDO_" -- 3ds src include ':!3ds/logs' ':!*.md' ':!*.log' ':!*.json' > build3ds/out/del3c3_grep.txt`: no line.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del3c3_host.txt 2>&1`: PASS (no ncv1a line; vcopy, maplist, mapconv, pipefx lines as before). Commit `DEL3C3 1: the NDO_ ids are ND_FS_`.
2. `python 3ds/tools/bench.py base entcape_kmr03_drec --commit $(git merge-base HEAD main) > build3ds/out/del3c3_base1.txt 2>&1`. `sh 3ds/tools/rowloop.sh --row DEL3C3 --bench entcape_kmr03_drec --no-host --why "DEL3C3 NDO ids" > build3ds/out/del3c3_loop1.txt 2>&1`. Then the same 2 commands for spot_mac01_drec (del3c3_base2.txt, del3c3_loop2.txt).
3. `python 3ds/tools/listcen.py > build3ds/out/del3c3_listcen.txt`.
## Gate (exact)
Host PASS. Both rowloops exit 0 (byte-equal). del3c3_grep.txt empty. listcen: reader 162 below the start commit; `listcen file reader 3ds/include/nd_op.h 11`.
## Fix rights and stop
Fix rights: the files of edit 1 and nd_op.h; compile errors (a file that names an ND_FS_ id and has no nd_fs.h: nd_op.h includes it now; a missed file outside the globs: the same rename, name it in the commit). `lkey census bad <file> <n> listed <m>`: set that count in allow.txt to n. Stop: a DIFF, a crash, a tool fault, a counter off the gate. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time; one bench per rowloop call.
## Hand-back
Last commit `DEL3C3 2: gate. entcape and spot_mac01 drec equal; reader -162` deletes this page. One SubagentHandback: commit hash, listcen totals.
