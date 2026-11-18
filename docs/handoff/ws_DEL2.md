# ws_DEL2 (Haiku): the core 0 draw functions that listcen MODEL_NAMES and the elfgate func line name get native names; the setter files name ND_FS_ ids, not NDO_. No logic change. Counters: C9 model 11 -> 0 (FACT listcen main f2ab50d38d), C11 elf func -26 (FACT elfgate basebin 309a79ac9ab7e378: the 26 old names below), C8 reader -53 (FACT listcen rules per line: nd_set.c 39, nd_gtab.c 7, nd_tex_set.c 4 after DEL1D1, nd_mtx_set.c 3). Gate: 2 drec byte-equal.
Designer: oracle DEL helper. WAIT: branch from main after the DEL1D2 and DEL5B3 merges (DEL1D2 writes the same nd_walk.c lines; DEL5B3 writes nd_gtab.c :11-29, :79). DEL2E comes after this row. Worktree build3ds/wt/DEL2, branch DEL2. PM_ROW=DEL2 PM_LANE=DEL2.
Why one row, not DEL2A-F: the prototypes of these functions share nd_int.h (:26-150) and their callers share nd_walk.c, nd_rect.c, nd_tex_set.c; per-file rows conflict there.
Edit only: the files that edit 1 writes (3ds/src/*.c, 3ds/include/*.h, 3ds/tools/*.c, 3ds/tools/nd_host_test.sh, 3ds/tools/hotset.json, 3ds/ld/hot_order.ld); 3ds/tests/lkey/allow.txt (count only). Not: elfgate_patterns.txt, listcen.py, azt_scenarios.json, *.md, src/.
## Facts (code read, main f2ab50d38d)
- FACT every old name has callers on the core 0 draw path (grep, this oracle): none can go without a snap source for the model_draw reads (pm_gpu.c:3854), the particle combine (nd_walk.c:448) and the texture units. So this row renames; the state model stays on core 0.
- FACT no new name exists in 3ds, src or include (git grep -w, main f2ab50d38d). Every NDO_ name of the 9 files of edit 1b has an ND_FS_ twin of the same value (nd_fs.h:8-46).
- FACT the old names are words in comments, the hot order (hot_order.ld :6-365), hotset.json :7-13, host stubs (nd_direct_test.c, nd_host_test.sh:187) and one log text (nd_mtx_set.c:125). The word rule (`\b`) keeps longer names (nd_texrect_run, ensure_tile_slow, ent_compile_t) whole.
## Edits
1. One command (keeps CRLF: bytes in, bytes out):
```python
python - <<'EOF'
import re, glob, os
M = {'sync_combine':'nd_comb_sync','nd_comb_decide':'nd_comb_dec','op_setcombine':'nd_comb_set','sync_render':'nd_rnd_sync','nd_op_same':'nd_unit_same','nd_op_apply':'nd_unit_apply',
     'gbi_setenv':'nd_envc_set','set_tilesize':'nd_tsz_set','ensure_tile':'nd_tile_bind','ensure_tile_slow':'nd_tile_bind_slow','nd_set_timg':'nd_tx_timg','nd_set_tile':'nd_tx_tile',
     'nd_set_tilesize':'nd_tx_tsz','nd_set_loadblock':'nd_tx_ldblk','nd_set_loadtile':'nd_tx_ldtile','nd_set_loadtlut':'nd_tx_ldtlut','nd_set_texture':'nd_tx_texon','nd_texrect':'nd_trect_draw',
     'nd_fillrect':'nd_frect_draw','off_screen_check':'nd_img_check','gv_tri':'nd_gv_tri','load_mtx':'nd_xf_load','gbi_movemem':'nd_mv_mem','gbi_viewport':'nd_vp_set','gbi_popmtx':'nd_xf_popb','gbi_setcimg':'nd_cimg_set'}
rx = re.compile(rb'\b(' + b'|'.join(k.encode() for k in M) + rb')\b')
F = glob.glob('3ds/src/*.c') + glob.glob('3ds/include/*.h') + glob.glob('3ds/tools/*.c') + ['3ds/tools/nd_host_test.sh', '3ds/tools/hotset.json', '3ds/ld/hot_order.ld']
NDO = {os.path.normpath('3ds/src/' + f) for f in ('nd_set.c', 'nd_tex_set.c', 'nd_mtx_set.c', 'nd_target.c', 'nd_gtab.c')} | {os.path.normpath('3ds/include/' + f) for f in ('nd_int.h', 'nd_scomp.h', 'nd_gtab.h', 'nd_api.h')}
n = 0
for p in F:
    b = open(p, 'rb').read(); c = rx.sub(lambda m: M[m.group(1).decode()].encode(), b)
    if os.path.normpath(p) in NDO: c = re.sub(rb'\bNDO_', b'ND_FS_', c)
    if c != b: open(p, 'wb').write(c); n += 1
print('del2 files', n) # dry run on main f2ab50d38d: 47 files
EOF
```
2. A file of edit 1b that has no `#include "nd_fs.h"` and fails to compile: add the include after its last `#include`.
3. `git grep -nwE "sync_combine|nd_comb_decide|op_setcombine|sync_render|nd_op_same|nd_op_apply|gbi_setenv|set_tilesize|ensure_tile|ensure_tile_slow|nd_set_(timg|tile|tilesize|loadblock|loadtile|loadtlut|texture)|nd_texrect|nd_fillrect|off_screen_check|gv_tri|load_mtx|gbi_movemem|gbi_viewport|gbi_popmtx|gbi_setcimg" -- 3ds/src 3ds/include 3ds/tools/*.c 3ds/ld > build3ds/out/del2_grep.txt`: no line.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del2_host.txt 2>&1`: PASS. Commit `DEL2 1: the core 0 draw functions have native names`.
2. `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main) > build3ds/out/del2_base1.txt 2>&1`. `sh 3ds/tools/rowloop.sh --row DEL2 --bench spot_mac01_drec --no-host --why "DEL2 renames" > build3ds/out/del2_loop1.txt 2>&1`.
3. The same 2 commands for entcape_kmr03_drec (del2_base2.txt, del2_loop2.txt). `python 3ds/tools/listcen.py > build3ds/out/del2_listcen.txt`. `python 3ds/tools/elfgate.py build3ds/lane-DEL2/pm_3ds.elf > build3ds/out/del2_elf.txt`.
## Gate (exact)
Host PASS. Both rowloops exit 0 (byte-equal). del2_grep.txt empty. listcen model 0; reader 53 below the start commit; no file reader line for nd_set.c, nd_gtab.c, nd_tex_set.c, nd_mtx_set.c. del2_elf.txt names none of the 26 old names.
## Fix rights and stop
Fix rights: the files of edit 1; compile errors of the rename (a missed caller outside the globs: rename it with the same map, name it in the commit). `lkey census bad <file> <n> listed <m>`: set that count in allow.txt to n. Stop: a DIFF, a crash, a tool fault, a counter off the gate. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time; one bench per rowloop call.
## Hand-back
Last commit `DEL2 2: gate. spot_mac01 and entcape drec equal; model 0, elf func -26` deletes this page. One SubagentHandback: commit hash, listcen totals, the elfgate line.
