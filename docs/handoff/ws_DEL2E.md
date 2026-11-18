# ws_DEL2E (Haiku): the entity and map shape geometry compile functions get native names. No logic change: they build the static GPU pool from the v2 entry units. Counter: C11 elf func -5, to 0 after DEL5B2, DEL1A, DEL1D2, DEL2 (FACT elfgate basebin 309a79ac9ab7e378: 44 = 3 + 2 + 8 + 26 + 5). Gate: 2 drec byte-equal.
Designer: oracle DEL helper. WAIT: branch from main after the DEL2 merge (same files: nd_int.h, nd_walk.c, hot_order.ld, hotset.json). Worktree build3ds/wt/DEL2E, branch DEL2E. PM_ROW=DEL2E PM_LANE=DEL2E.
Edit only: the files that edit 1 writes (3ds/src/*.c, 3ds/include/*.h, 3ds/tools/*.c, 3ds/tools/hotset.json, 3ds/ld/hot_order.ld); 3ds/tests/lkey/allow.txt (count only). Not: elfgate_patterns.txt, listcen.py, azt_scenarios.json, *.md, src/.
## Facts (code read, main f2ab50d38d)
- FACT ent_compile reads the units of the v2 entry (nd_ent.c:225-227: nd_ntab_find, nd_ntab_flat) and compiles them into the static GPU pool (nd_ent.c:258-274); ent_run draws its segments with dlc_seg (nd_ent.c:319, :444). The flat buffer holds typed units (ND_FS_ ids, nd_ent.c:144-145, :176, :181).
- FACT a switch to the ring path (the units on core 0) changes drec: the vertex bytes of a static draw come from nb.rchunk, of a ring draw from nb.gring (pm_gpu.c:669). That switch is not this row (user decision, default no).
- FACT no new name exists (git grep -w, main f2ab50d38d). Dry run of edit 1 on main f2ab50d38d: 15 files, ent_compile 7, ent_run 11, ent_flat 8, dlc_try 22, dlc_build 7 words. The word rule keeps ent_compile_t, ent_flat_buf, ent_flat_call, ent_flat_w whole.
## Edits
1. One command (bytes in, bytes out, keeps CRLF):
```python
python - <<'EOF'
import re, glob
M = {'ent_run': 'nd_eg_run', 'ent_flat': 'nd_eg_flat', 'ent_compile': 'nd_eg_build', 'dlc_try': 'nd_sg_try', 'dlc_build': 'nd_sg_build'}
rx = re.compile(rb'\b(' + b'|'.join(k.encode() for k in M) + rb')\b')
n = 0
for p in glob.glob('3ds/src/*.c') + glob.glob('3ds/include/*.h') + glob.glob('3ds/tools/*.c') + ['3ds/tools/hotset.json', '3ds/ld/hot_order.ld']:
    b = open(p, 'rb').read(); c = rx.sub(lambda m: M[m.group(1).decode()].encode(), b)
    if c != b: open(p, 'wb').write(c); n += 1
print('del2e files', n)
EOF
```
2. `git grep -nwE "ent_run|ent_flat|ent_compile|dlc_try|dlc_build" -- 3ds/src 3ds/include 3ds/tools/*.c 3ds/ld 3ds/tools/hotset.json > build3ds/out/del2e_grep.txt`: no line.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del2e_host.txt 2>&1`: PASS. Commit `DEL2E 1: the entity and shape compile functions have native names`.
2. `python 3ds/tools/bench.py base entcape_kmr03_drec --commit $(git merge-base HEAD main) > build3ds/out/del2e_base1.txt 2>&1`. `sh 3ds/tools/rowloop.sh --row DEL2E --bench entcape_kmr03_drec --no-host --why "DEL2E renames" > build3ds/out/del2e_loop1.txt 2>&1`.
3. The same 2 commands for spot_mac01_drec (del2e_base2.txt, del2e_loop2.txt). `python 3ds/tools/elfgate.py build3ds/lane-DEL2E/pm_3ds.elf > build3ds/out/del2e_elf.txt`. `python 3ds/tools/listcen.py > build3ds/out/del2e_listcen.txt`.
## Gate (exact)
Host PASS. Both rowloops exit 0 (byte-equal). del2e_grep.txt empty. del2e_elf.txt: `elfgate func 0` when DEL5B2, DEL1A, DEL1D2 and DEL2 are merged (else 5 below the start commit). listcen totals equal to the start commit.
## Fix rights and stop
Fix rights: the files of edit 1; compile errors of the rename (a missed caller: rename it with the same map, name it in the commit). `lkey census bad <file> <n> listed <m>`: set that count in allow.txt to n. Stop: a DIFF, a crash, a tool fault, a counter off the gate. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time; one bench per rowloop call.
## Hand-back
Last commit `DEL2E 2: gate. entcape and spot_mac01 drec equal; elf func -5` deletes this page. One SubagentHandback: commit hash, the elfgate line.
