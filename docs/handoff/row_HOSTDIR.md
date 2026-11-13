# Row HOSTDIR: nd_host runs every per-row test script of 3ds/tests/host.d (no shared test list)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Why (ORACLE8, main 5a53dd0a2)
- 3ds/tools/nd_host_test.sh:15-74 runs every host test as one line in one docker `sh -c '...'` block. NOLIST, HOSTCORE1,
  FRAMEHASH1, NPSO1 and CONV2B each add a host test. Five rows that add lines to that block edit one place: merge conflicts
  (CLAUDE.md 8). This row makes the block run every script in 3ds/tests/host.d, so each row adds its own file.
- Inside the block the cwd is /work with links 3ds, include, src, assets, ver/us/build (:17-20), and `set -e` is on (:16).

Start: `sh 3ds/tools/wt_new.sh HOSTDIR`. PM_ROW=HOSTDIR PM_LANE=HOSTDIR on every call.
## Touch only (places)
3ds/tools/nd_host_test.sh: the two lines `gcc ... kccmd_test.c` and `/tmp/kcc` (:72-73) stay. Add one line after :73.
New files 3ds/tests/host.d/README.txt and 3ds/tests/host.d/00_hostd.sh. Nothing else.

## Exact edits
1. nd_host_test.sh, after the line `  /tmp/kcc` (:73) add:
```
  for t in 3ds/tests/host.d/*.sh; do [ -f "$t" ] || continue; echo "host.d: $t"; sh "$t"; done  # HOSTDIR: per-row host tests
```
2. New 3ds/tests/host.d/README.txt (3 lines):
```
Per-row host tests. nd_host_test.sh runs every *.sh here in name order inside its docker block (cwd /work, set -e, gcc -m32).
A row adds its own <row>.sh. A script prints its result line and exits non-zero on a failure. Never edit another row's script.
Flags for game headers: -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include
```
3. New 3ds/tests/host.d/00_hostd.sh (LF line ends):
```
#!/bin/sh
# HOSTDIR: the host.d loop runs. Prints one line.
echo "hostd ok"
```
4. Commit: `HOSTDIR: nd_host runs the per-row scripts of 3ds/tests/host.d`.

## Gate (expected values)
1. Seeded fault: make edits 1-3 and add a file 3ds/tests/host.d/99_seed.sh with the two lines `#!/bin/sh` and `exit 1`.
   `PM_ROW=HOSTDIR python 3ds/tools/bench.py run nd_host --row HOSTDIR --why "HOSTDIR seeded fault"`. Expected: the run
   fails (rc not 0, `ND HOST TESTS FAIL`).
2. Delete 99_seed.sh. Run the same command with --why "HOSTDIR gate". Expected: PASS, the output holds `host.d:
   3ds/tests/host.d/00_hostd.sh` and `hostd ok`, then `ND HOST TESTS PASS`.
3. No Azahar start, no console. Counter: none (harness row of plan section 5).

## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit. Hand back: the ready hash and the two outputs.
