# Row ENTRYCAP: device switch `entcap` logs the walker's NdPsoDesc at each run of a list without a v2 table, with its call site

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Release after CONV2D2 merges: the Needs
line then holds. ORACLE2 checked it on 2026-11-13.)
## Facts (main 5c30af943)
- 426 static lists inherit the pipeline state of their call (build3ds/out/ntab_inherit.txt, CONV2C). ENTRY1..n write that state
  as ND_PSO_DESC at each call site. This row captures it on the device.
- nd_rec_dl (3ds/src/nd_gtab.c:64-86, core 1) records the table and the state-only flag at :84-85
  `const uint32_t rec[2] = { (uint32_t)(uintptr_t)t, fl };`. nd_rec_napi takes 1 to 255 8-byte units (3ds/include/nd_api.h:11).
  nd_rec_dl is a real function in its own file and the build has no -flto, so `__builtin_return_address(0)` in it is the call site.
- nd_gtab_run (3ds/src/nd_walk.c:568, core 0) reads rec[0] and rec[1]. The arm for lists that are not state-only starts at :579
  `if (__builtin_expect(pm_walk_dbg, 0)) {` (CONV2D2 moves it down: find the quoted text). An inheriting list has a triangle,
  so it is never state-only.
- nd_walker_desc(NdPsoDesc*) (3ds/src/nd_comb.c:206) gives the walker description. NdPsoDesc is 44 bytes (11 words,
  3ds/include/nd_api.h:25-33).
- Test scene: fxtest 20 is explosion (FXLIST index, 3ds/src/pm_fxtest.c:52). src/effects/explosion.c:195 calls the inheriting
  list D_090008F0_344860 with nd_rec_dl. The spawn is at frame 402. pm_fxtest.c:259 logs `fxtest: done` at frame 444, after the
  explosion slot clears (:278).
- 3ds/tools/decode_dump.py:16-20 finds arm-none-eabi-addr2line (PATH or /opt/devkitpro). nm is in the same folder.
## Start
`sh 3ds/tools/wt_new.sh ENTRYCAP`. PM_ROW=ENTRYCAP PM_LANE=ENTRYCAP on every build and bench call.
## Touch only
Files: 3ds/src/nd_gtab.c (nd_rec_dl :84-85), 3ds/src/nd_walk.c (entcap_rec added before nd_gtab_run, one line at the top of
the non-state-only arm), new 3ds/tools/entcap.py, new 3ds/tools/azt.d/ENTRYCAP.json, new 3ds/tools/bench.d/ENTRYCAP.json.
New names: fxpic_20_entcap. Host link: 3ds/tests/host.d/conv2c.sh links nd_gtab.c with -DND_SCOMP_HOST (nd_rec_dl is outside it).
Needs: 3ds/include/nd_gtab.h has nd_ntab_find
## Exact edits
1. Commit 1 holds only the scene. azt.d/ENTRYCAP.json: scenario fxpic_20_entcap, debug
   `warp=mac_01:0 pos=0:0:200 fxtest=20 loglevel=debug det entcap`, until `fxtest: done`, expected_events: warp
   (`warp: mac_01 entry 0`), `entcap l [0-9a-f]{8} s [0-9a-f]{8}` and the until. cap 40, deadline 35, needs_picture false.
   bench.d/ENTRYCAP.json: fxpic_20_entcap, kind azahar_pic, tags azahar det, in the form of 3ds/tools/bench.d/HOSTCORE2.json.
2. nd_gtab.c, before the 2 lines :84-85 (they stay):
   `if (__builtin_expect(PMDL("entcap"), 0)) { const uint32_t r4[4] = { (uint32_t)(uintptr_t)t, fl, (uint32_t)(uintptr_t)__builtin_return_address(0), 0u }; nd_rec_napi(nd_gtab_run, r4, 2u); return; }`
3. nd_walk.c: a cold noinline static function `entcap_rec(const NdGTab* t, uint32_t site)` before nd_gtab_run, and as the first
   line of the non-state-only arm `if (__builtin_expect(PMDL("entcap"), 0)) entcap_rec(t, rec[2]);`. entcap_rec:
   - return when nd_ntab_find(t->list) is not NULL (the list has a v2 table);
   - `NdPsoDesc d; nd_walker_desc(&d);` key = FNV-1a 32 over the list address, the site and the 44 bytes of d;
   - a static table of 1024 keys (linear search). A new key: store it and log
     `pm_log_evt("entcap l %08x s %08x d %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x", list, site, the 11 words of d)`.
     A new key with a full table: once, `pm_log_evt("entcap over")`.
4. entcap.py <log> [--elf <elf>]: for each `entcap l` line print `<list symbol> <site function> <file:line> <the 11 words>`. The
   list symbol is the nm symbol at the address. The site is addr2line -f of site - 4 (the decode_dump.py:16-20 lookup).
   The last line is `entcap.py lists <n> sites <m> over <count of entcap over lines>`.
5. Commit 2: `ENTRYCAP: switch entcap logs the walker entry description of lists without a v2 table, entcap.py names them`.
## Gate (CLAUDE.md 2)
1. Build, memcheck OK. nd_host PASS (no host file changes).
2. At commit 1: `python 3ds/tools/bench.py base spot_mac01_drec`. After commit 2:
   `bench.py run fxpic_20_entcap,spot_mac01_drec --row ENTRYCAP --why "ENTRYCAP"`. `bench.py cmp spot_mac01_drec --row ENTRYCAP`
   (switch off): drec.txt byte-equal.
3. The new tool: `python 3ds/tools/entcap.py <the fxpic_20_entcap log>`: a line `D_090008F0_344860 ... explosion.c:<n>` and `over 0`.
   Seed first (a value change in entcap.py, then undo): the site + 0x100000. Then the explosion.c line is missing.
4. Budget 8 Azahar starts. Counter: none moves. This is the capture that ENTRY1..n use (plan v3 section 6).
## Stop and end
No entcap line for D_090008F0_344860, a drec difference, a base pair that differs, or a tool fault: stop, commit, hand back the
exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
