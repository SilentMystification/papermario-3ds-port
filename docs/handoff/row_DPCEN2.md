# Row DPCEN2: census of resident bytes per pool and class, and of load-time transforms (switch dpcen)

(Model: Haiku. Base: main after DPCEN1. Edits 3ds/src/pm_gpu.c dp_frame (DPCEN1's function: one more line), src/sprite.c
spr_load_sprite (two lines), 3ds/src/nd_shape.c and 3ds/src/nd_ent.c (one timer pair at each nd_dlc_compile call), one new file
3ds/src/pm_dpcen.c. Starts after PALGEN merges only if PALGEN touched dp_frame (it does not): can start with PALGEN. Reads this file and CLAUDE.md.)

## Facts (oracle code read)
- No census gives resident bytes per pool: the boot line has pool totals (pm_mem.c, `mem boot: ... heap ... linear ...`). Readable at run
  time: linearSpaceFree() and vramSpaceFree() (libctru), mallinfo().uordblks (newlib heap in use), nd_mc_fill(&ent, &pool)
  (nd_model.c:478: model cache entries and pooled draws), the ring and stream sizes are fixed buffers.
- Load-time transforms with no counter: sprite archive decode (spr_load_sprite, src/sprite.c:1162 and :850: Yay0 of a whole sprite at
  load), static list compile (nd_dlc_compile at nd_shape.c:206/:211 and nd_ent.c:234), the seam table (now from romfs, SEAMHOST).

## Start
`sh 3ds/tools/wt_new.sh DPCEN2`. PM_ROW=DPCEN2 PM_LANE=DPCEN2 on every build and bench call.

## Exact edits
1. New 3ds/src/pm_dpcen.c (add to 3ds/CMakeLists.txt beside pm_keymiss.c and to its -Os list): `unsigned pm_dp_spr_n, pm_dp_spr_us,
   pm_dp_cmp_n, pm_dp_cmp_us;` and `void pm_dp_res_line(unsigned f)`: pm_log_evt(`dpres f%u heapkb %u linfreekb %u vramfreekb %u mcent %u
   mcpool %u sprload %u sprus %u compiles %u compus %u`, f, mallinfo().uordblks >> 10, linearSpaceFree() >> 10, vramSpaceFree() >> 10,
   ent, pool (nd_mc_fill), pm_dp_spr_n, pm_dp_spr_us, pm_dp_cmp_n, pm_dp_cmp_us), then zero the four counters.
2. pm_gpu.c dp_frame (DPCEN1): after its dpcen line add `pm_dp_res_line(f);` (declare it above).
3. src/sprite.c spr_load_sprite: at entry `#ifdef TARGET_3DS unsigned dp_t0 = pm_ticks(); #endif`, before each return of the function
   `#ifdef TARGET_3DS { extern unsigned pm_dp_spr_n, pm_dp_spr_us; pm_dp_spr_n++; pm_dp_spr_us += (pm_ticks() - dp_t0) / 268u; } #endif`.
4. nd_shape.c (:206 and :211) and nd_ent.c (:234): around each `nd_dlc_compile(...)` call: `unsigned dp_t0 = pm_ticks();` before and
   `{ extern unsigned pm_dp_cmp_n, pm_dp_cmp_us; pm_dp_cmp_n++; pm_dp_cmp_us += (pm_ticks() - dp_t0) / 268u; }` after (both cold paths).
5. Scenario and bench dpcen_xfer: the route of xfer_map_mac01 (mac_01 -> mac_02 loading zone) with `dpcen` added, until `dpcen f(6|9)\\d\\d `
   (or the last window after the load), cap 45; `bench.py rebuild`.

## Gate (CLAUDE.md 2; Azahar budget 4 of 8)
- Build, memcheck OK; nd_host PASS; `bench.py base spot_mac01_det`, run, cmp: 6/6 byte-equal (switch off).
- One list `bench.py run dpcen_mac01,dpcen_xfer --row DPCEN2 --why "resident bytes and load transforms"`: dpres lines in both LOG: copies.
  Hand back the lines as they are.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line, the dpres lines.
