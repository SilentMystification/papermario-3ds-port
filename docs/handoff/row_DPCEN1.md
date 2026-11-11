# Row DPCEN1: census of bytes moved per frame (switch dpcen): texture upload and hash bytes, cache flush bytes, ring and command bytes

(Model: Haiku. Base: main. Edits 3ds/include/pm_port.h (3 enum entries), 3ds/src/pm_tex.c load_miss (2 lines), 3ds/src/pm_gpu.c
(the flush helper near :2943, pm_gpu_end_w one line, one new function), 3ds/src/pm_fxtest.c (pm_cnt_on also for dpcen). Debug tool,
no draw change. Not nd_tex_set.c (TILEWIN2), not nd_shape.c (SEAMHOST). Reads this file and CLAUDE.md.)

## Facts (oracle code read)
- FXCEN1 made pm_cnt_all (never reset) counted by PH_ADD while pm_cnt_on (pm_port.h PH_ADD, pm_fxtest.c). Existing counters: C_TEXMISS,
  C_TEXBYTE, C_CMD, C_FLKB, C_GVTRI, C_NDRAW, C_NIDX (pm_port.h:88 enum). Missing: bytes uploaded to VRAM, source bytes hashed on a
  miss, data-cache flush bytes per call.
- load_miss (pm_tex.c:1394) knows gpu_size (the VRAM bytes of the new texture) and slen0 (the source bytes it hashes, :1400-1401).
- The ring and texture flushes go through svcFlushProcessDataCache(CUR_PROCESS_HANDLE, p, n) (pm_gpu.c:2943).

## Start
`sh 3ds/tools/wt_new.sh DPCEN1`. PM_ROW=DPCEN1 PM_LANE=DPCEN1 on every build and bench call.

## Exact edits
1. pm_port.h enum (:88): add `C_UPB, C_HASHB, C_FLB,` before `C_N` (bytes uploaded, bytes hashed, bytes flushed).
2. pm_tex.c load_miss: after `const unsigned shash0 = ...;` add `PH_ADD(C_HASHB, slen0);`; after gpu_size is final (after the for loop)
   add `PH_ADD(C_UPB, gpu_size);`.
3. pm_gpu.c: in the function that calls svcFlushProcessDataCache (:2943), first line `PH_ADD(C_FLB, n);`.
4. pm_fxtest.c: where pm_cnt_on is set, also set it when `pm_debug_has("dpcen")` (at the first tick: `if (!pm_cnt_on && pm_debug_has("dpcen")) pm_cnt_on = 1;`
   at the top of pm_fxtest_tick, before its nsel test).
5. pm_gpu.c: `static void dp_frame(unsigned f)`: every 300 frames (`f >= next`, `next += 300u`, no division) print with pm_log_evt
   `dpcen f%u frames %u upkb %u hashkb %u flkb %u texmiss %u cmd %u ndraw %u nidx %u gvtri %u` (deltas of pm_cnt_all since the last
   line; bytes >> 10). pm_gpu_end_w: after the keymiss line add `if (__builtin_expect(pm_cnt_on, 0) && PMDL("dpcen")) dp_frame(pm_frame_count());`.
6. Scenarios and benches dpcen_mac01 (`warp=mac_01:0 pos=0:0:200 dpcen loglevel=debug`), dpcen_kzn08 (`warp=kzn_08:0 dpcen loglevel=debug`),
   form of kzn04_shadestat, until `dpcen f(6|7)\\d\\d `, cap 45, deadline 40, first_marker `warp: <map> entry 0`. `bench.py rebuild`.

## Gate (CLAUDE.md 2; Azahar budget 4 of 8)
- Build, memcheck OK; nd_host PASS; `bench.py base spot_mac01_det`, run, cmp: 6/6 byte-equal (switch off).
- One list `bench.py run dpcen_mac01,dpcen_kzn08 --row DPCEN1 --why "bytes per frame census"`: each LOG: copy has a dpcen line of
  frame 600-799. Hand back the lines as they are.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line, the dpcen lines.
