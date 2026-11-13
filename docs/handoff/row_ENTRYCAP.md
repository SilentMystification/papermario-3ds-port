# Row ENTRYCAP: device switch `entcap` logs the walker's NdPsoDesc at each run of a list without a v2 table, with its call site

Blocked by CONV2D2 (needs nd_ntab_find, nd_gtab.h; same function nd_gtab_run) and NPSO4 (merged 423801c53). Release this row after CONV2D2 merges. Then
re-check the line numbers below against that merge.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main 182ac5091)
- 426 static lists inherit the pipeline state of their call (build3ds/out/ntab_inherit.txt, CONV2C). ENTRY1..n write that state
  as ND_PSO_DESC at each call site. This row captures it on the device.
- nd_rec_dl (3ds/src/nd_gtab.c:64-86, core 1) records `rec[2] = {table, state-only flag}` with `nd_rec_napi(nd_gtab_run, rec, 1u)`
  (:84-85). nd_rec_napi takes 1 to 255 8-byte units (nd_api.h:11). nd_rec_dl is a real function in its own file, called from 119
  src files, and the build has no -flto. So `__builtin_return_address(0)` in it is the call site.
- nd_gtab_run (3ds/src/nd_walk.c:568, core 0) reads rec[0] and rec[1] only. Its arm for lists that are not state-only starts at
  :580 (`if (__builtin_expect(pm_walk_dbg, 0))`).
- nd_walker_desc(NdPsoDesc*) (nd_comb.c:206) gives the walker description. NdPsoDesc is 44 bytes (11 words, nd_api.h:25). The
  psochk logger (nd_comb.c:217-232) is the pattern: `PMDL`-style switch read, the pm_frame_n bucket of 100 frames, and pm_log_evt.
- Test scene: explosion (fxtest 20) calls the inheriting list D_090008F0_344860 (src/effects/explosion.c). fxtest spawns at frame
  400 (pm_fxtest.c:180 START).
- Symbols: 3ds/tools/decode_dump.py:18-20 finds arm-none-eabi-addr2line (PATH or /opt/devkitpro). nm is next to it.
Start: `sh 3ds/tools/wt_new.sh ENTRYCAP`. PM_ROW=ENTRYCAP PM_LANE=ENTRYCAP on every call.
## Touch only (places)
3ds/src/nd_gtab.c nd_rec_dl (:84-85); 3ds/src/nd_walk.c nd_gtab_run (top of the non-state-only arm); new 3ds/tools/entcap.py;
new 3ds/tools/azt.d/ENTRYCAP.json and 3ds/tools/bench.d/ENTRYCAP.json.
## Exact edits
1. Commit 1 holds only the scene. azt.d/ENTRYCAP.json is a copy of the fxpic_89 scenario named fxpic_20_entcap. Its `debug` is
   `warp=mac_01:0 pos=0:0:200 fxtest=20 loglevel=debug det entcap`. Its until is `entcap f5\d\d keys \d+`, and its
   expected_events are the warp and that until. It has cap 45, deadline 40 and needs_picture false. bench.d/ENTRYCAP.json:
   fxpic_20_entcap, kind azahar_pic, as fxpic_89 in bench_defs.json.
2. nd_gtab.c, in place of :84-85:
   `if (__builtin_expect(PMDL("entcap"), 0)) { const uint32_t r4[4] = { (uint32_t)(uintptr_t)t, fl, (uint32_t)(uintptr_t)__builtin_return_address(0), 0u }; nd_rec_napi(nd_gtab_run, r4, 2u); return; }`
   then the 2 old lines unchanged.
3. nd_walk.c: a cold static function `entcap_rec(const NdGTab* t, uint32_t site)` and, at the top of the non-state-only arm,
   `if (__builtin_expect(PMDL("entcap"), 0)) entcap_rec(t, rec[2]);`. entcap_rec:
   - return when nd_ntab_find(t->list) is not NULL (the list has a v2 table);
   - NdPsoDesc d; nd_walker_desc(&d); key = FNV-1a 32 over the list address, site and the 44 bytes of d;
   - a static table of 1024 keys (linear search); a new key: store it and
     `pm_log_evt("entcap l %08x s %08x d %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x", list, site, the 11 words of d)`;
     a full table counts `entcap_over`;
   - each time pm_frame_n / 100 changes: `pm_log_evt("entcap f%u keys %u over %u", pm_frame_n, keys, entcap_over)`.
4. entcap.py <log> [--elf <elf>]: for each `entcap l` line, print `<list symbol> <site function> <file:line> <the 11 words>`. The
   list symbol comes from nm (the symbol at the address). The site comes from addr2line -f of site - 4. Use the lookup of
   decode_dump.py:18-20. The last line is `entcap.py lists <n> sites <m>`.
5. Commit: `ENTRYCAP: switch entcap logs the walker entry description of lists without a v2 table, entcap.py names them`.
## Gate (expected values)
1. Build, memcheck OK. nd_host PASS (no host file changes).
2. At commit 1: `bench.py base spot_mac01_drec`. After edit 5: `bench.py run fxpic_20_entcap,spot_mac01_drec --row ENTRYCAP
   --why "ENTRYCAP"`. The cmp of spot_mac01_drec (switch off) is byte-equal: no draw change.
3. `python 3ds/tools/entcap.py <the fxpic_20_entcap log>`: a line `D_090008F0_344860 ... explosion.c:<n>` and `over 0`.
   Seed first (a value change): the logged site + 0x100000. Then the explosion.c line is missing. Undo the seed.
4. Budget 8 Azahar starts. Counter: none moves. This is the capture that ENTRY1..n use (plan v3 section 6).
## Stop and end
No entcap line for D_090008F0_344860, a drec difference, a base pair that differs, or a tool fault: stop, commit, hand back the
exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
