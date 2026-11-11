# Row NAPI0: census of typed ops recorded per frame by producer function (switch napicen), for the native draw API order

(Model: Haiku. Base: main. Edits 3ds/src/nd_rec.c nd_gx_w (one line) and one new cold function; new 3ds/tools/napicen.py; scenarios
and benches. Disjoint from FILLA0R, PMUPHFIX2. Reads this file, row_NATIVEAPI.md section 2 and CLAUDE.md.)

## Facts (oracle code read)
- Every gDP*/gSP* macro of game code is ND_GX (3ds/include/nd_gbi.h:18) -> nd_gx_w(pkt, w0, w1) (nd_rec.c:348) on core 1. A unit written
  at the stream top (`g == gMainGfxPos - 1`) is recorded (nd_rec_state); a unit written into a local buffer returns at :349 (model
  records, built lists). __builtin_return_address(0) inside nd_gx_w is in the calling game function (the macro is expanded there).

## Start
`sh 3ds/tools/wt_new.sh NAPI0`. PM_ROW=NAPI0 PM_LANE=NAPI0 on every build and bench call.

## Exact edits
1. nd_rec.c: `static __attribute__((cold, noinline)) void napi_note(uintptr_t ra)`: a heap table (calloc at first use) of 512 entries
   {u32 ra, u32 n}, open addressing by (ra >> 2) & 511; count the caller; every 300 frames (pm_frame_count, `f >= next`, `next += 300u`)
   print the 40 largest with `pm_log("napicen f%u ra %08x n %u", f, ra, n / 300)` (ops per frame) and `pm_log_evt("napicen f%u done
   total %u", f, total / 300)`, then zero the counts.
2. nd_gx_w, after the `if (g != gMainGfxPos - 1) { ... return; }` line: `if (__builtin_expect(PMDL("napicen"), 0)) napi_note((uintptr_t)__builtin_return_address(0));`.
3. 3ds/tools/napicen.py <log> [--map build3ds/lane-<L>/pm_3ds.map]: maps each ra to the function whose .text.<name> section holds it
   (the map's ` .text.<name> 0x<addr> 0x<size> <object>` lines) and its object file (src path); sums per object file and per function;
   writes build3ds/out/napicen_<label>.txt: per file ops per frame, then per function, largest first.
4. Scenarios and benches napicen_mac01 (`warp=mac_01:0 pos=0:0:200 napicen loglevel=debug`), napicen_talk (the msgbox_talk_nowide route
   with napicen), napicen_btl (the battle_start route with napicen): form of kzn04_shadestat, until `napicen f(6|7)\\d\\d done`, cap 45.

## Gate (CLAUDE.md 2; Azahar budget 5 of 8)
- Build, memcheck OK; nd_host PASS; `bench.py base spot_mac01_det`, run, cmp: 6/6 byte-equal (switch off).
- One list `bench.py run napicen_mac01,napicen_talk,napicen_btl --row NAPI0 --why "typed ops per producer"`; napicen.py on each log.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates and the first 25 lines (per file) of each napicen file.
