# plan_fastloop: one row loop (change, host test, build, gate) under 60 s (oracle FASTLOOP, main 341349fe3b, 2026-11-17)
User goal (FACT, user 2026-11-17): the loop of one row takes well under 1 min. The cache rules of plan_buildspeed.md stay binding.
Case: a one-file edit in src/ or 3ds/src, a warm lane, 1 bench, a quiet host. Load: FACT NTGT host run container-end 196 s
under 4 parallel host tests, 51 s alone (3.8x). The 60 s target needs at most 2 rows at the same time (GUESS).
## Measured (oracle probes on lane GMERLEE and main; read-only containers, no build, no Azahar start)
| step | now | floor | cause of the gap (file:line) |
| host: keys, batches, tar, crash page | 6+7+5+2 s FACT | 2 s GUESS | asset hash 3.2 s FACT; Git Bash spawn per host.d script (nd_host_test.sh batch loop); tar of HOST_IN |
| host: inline steps + host.d | 14 + 37 s FACT (61 scripts, sum 63 s, nmdl0.sh 15 s) | 0-15 s | every test runs on every edit; no per-test key |
| host: gfx table on a src edit | 14 s FACT (ws_GCONVSPLIT) | 0 s | GT_IN holds all of src (build.sh:145-146, nd_host_test.sh:35) |
| build: key prefix (asset hash, docker build -q, gens hit, key) | 11.7-12.3 s FACT (key-only run x2) | 5 s GUESS | asset hash 3.2 s FACT, docker build -q 1.4 s FACT, gc_ok sha1 of 1468 shape files |
| build: gen on a src edit | 21 s FACT (brief) | 5 s | gfx_tab key covers all of src (build.sh:145-153) |
| build: vol_state + sync | 7-8 s FACT (GMERLEE, 2 files) | 2 s GUESS | 2 docker runs 1.1 s each FACT, in_manifest 1.0 s FACT |
| compile one TU | 0.2-7.2 s FACT (model.c 7.2, pm_gpu.c 3.9) | same | - |
| link (ld.bfd 2.46, no gold, no lld) | 21.2-23.0 s FACT | 2.1 s FACT | 2143 file statements of map_restore_bss.ld 10.6 s FACT; debug info -g 7-8 s FACT (391 MB of objects) |
| 3dsxtool | 4.5 s FACT | 0.6 s FACT | romfs (27 MB, 1468 files) read from the C: bind mount /out (CMakeLists.txt:487) |
| copy-out + ELF archive | 3 + 1.1 s FACT | 0.5 s GUESS | 245 MB ELF (-g) |
| post: lane volume cap | up to 7.9 s FACT (0.7 s per docker volume inspect) | 0.3 s | one inspect per lane (build.sh:349-352) |
| gate: key check (build_stale) | 11.7-12.3 s FACT | 0 s | runs before the Azahar run (bench.py:1218) |
| gate: Azahar run | 17-19 s FACT (status line to log end) | 5 s FACT (wall_s 4-5 warm) | run_azahar.ps1:619-622 sleeps 3 s after every normal run |
| gate: cmp | 1-6 s FACT | 1 s | - |
| coder tool calls | 6-10 calls FACT (pages) | 1 call | one call per step |
FACT (GMERLEE .ninja_log): link 20.4 s, 3dsx 4.7 s in the real build. FACT probe: SORT_BY_NAME per map gives a byte-equal ELF; objects without debug give a byte-equal 3dsx; -g1 .text equal (84 objects).
## Rows (cumulative total, machine time; now about 190 s GUESS from the FACT steps above)
1. LINKFAST (Haiku, ws_LINKFAST + patch; oracle checks: apply --check, generator output = probe script, romfscache.sh seeded fault fails): SORT_BY_NAME per map, -g1, romfs in the build volume, one volume inspect,
   docker build stamp, romfscache.sh. Link 23 to about 5 s GUESS (FACT 4.4 s probe at -g0), 3dsx 4.5 to 0.6 s FACT. Total about 160 s GUESS.
2. AZTFAST (Haiku, ws_AZTFAST.patch only, no page; oracle checks: git apply --check, py_compile, PowerShell parse OK): key check during the run (void on stale), no 3 s sleep. Gate 32 to about 17 s GUESS. Total about 145 s.
3. LOOP1 (Haiku, ws_LOOP1.patch only, no page; sh -n OK): 3ds/tools/rowloop.sh: host test and build at the same time, run, cmp, 1 call.
   Total = max(host 85, build 40) + 17 = about 100 s GUESS.
4. GENKEY (Sonnet, no page, after Merge LINKFAST and Merge GCONVSPLIT): gfx_tab keys on the files gen_gfx_tab.sh reads.
   A src edit outside that set hits. Build gen 21 to 5 s, host gfx table 14 to 0 s. Total about 88 s GUESS.
5. HOSTSEL (Sonnet, no page): one result cache per test, keyed by the files the test read (strace). Host 71 to
   about 8 s GUESS. Total = max(8, 25) + 17 = about 42 s GUESS. Only this row takes the loop under 60 s.
Why Sonnet (4, 5): new cache designs with a read-set key; a wrong key gives a false PASS. Sonnet needs the user's yes.
## Rules for every row
- Cache safety of plan_buildspeed.md (key = content of every input, check on every use, temp then rename, seeded self-test).
- Gate: `sh 3ds/tools/nd_host_test.sh` PASS, 1 lane build, `bench.py run spot_mac01_drec`, `bench.py cmp spot_mac01_drec
  --commit $(git merge-base HEAD main)` 1/1 byte-equal. Counters: T1 host, T2 build, T4 loop (LOOP TIME line of rowloop.sh).
- OPEN: a gconv source edit misses gfx_tab in the host test and in the build (2 runs of the same table, GUESS 14 s each).
  A later row shares one entry. A header edit that many TUs read compiles at 8 jobs (PM_BUILD_JOBS, build.sh:313).
