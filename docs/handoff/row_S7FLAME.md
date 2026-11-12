# Row S7FLAME: the flame effect lists through the S7 tables (host stub for D_800B32E0 and D_800B3AE0)

(Model: Haiku. The S7EFX pattern for one effect plus a 2-line host stub. No blocker: S7RUN merged, S7EFX3 merged without flame.)

## Cause (ORACLE7 code read, main 87a256ef5)
- src/effects/gfx/flame.c:8-9 declares `extern unsigned char D_800B32E0[], D_800B3AE0[]` (defined in src/main.c:12-13 on the
  device). gen_gfx_tab.sh links gconv_in.o (which includes every GCONV_SRC source) into the host tool nd_gfx_conv and nd_host's
  nd_gtab_test; neither link has those two symbols, so S7EFX3 dropped flame. The tables use list units in place (nd_gfx_conv.c:153
  `&list[k]`), so the host value of the two buffers is never used: a host definition is enough.
- gen_gfx_syms.py main() writes gconv_in.c: the source includes, then `#include "nd_gconv.h"`, then the symbol map. gconv_in.c is
  the only translation unit that sees the stub, so plain definitions do not clash.
- Call sites: src/effects/flame.c:184, :186, :197 (`preset->dlist`), :212. No TARGET_3DS branch in the file. fxtest index of flame:
  27 (3ds/src/pm_fxtest.c FXLIST order).

## Start
`sh 3ds/tools/wt_new.sh S7FLAME`. PM_ROW=S7FLAME PM_LANE=S7FLAME on every build and bench call.

## Touch only
New file 3ds/tools/gconv_host_syms.h; 3ds/tools/gen_gfx_syms.py (main() only: S7ENTX edits scan()); 3ds/tools/gen_gfx_tab.sh (one new
line right after the `# S7MDL` line); 3ds/build.sh (the `want=` stamp line of gen_gfx_tab() only); src/effects/flame.c (the 4 call
sites and one include); azt_scenarios.json and bench_defs.json (one entry `fxpic_27` each, right after `fxpic_84`).

## Exact edits
1. 3ds/tools/gconv_host_syms.h: a comment line (S7FLAME: host definitions of device buffers that gfx sources name; only gconv_in.c
   includes this file), then `unsigned char D_800B32E0[0x800] __attribute__((aligned(16)));` and the same for D_800B3AE0.
2. gen_gfx_syms.py main(): after `lines += ['#include "nd_gconv.h"']` add `lines += ['#include "gconv_host_syms.h"']`.
3. gen_gfx_tab.sh: after the line ending `# S7MDL` add `GCONV_SRC="$GCONV_SRC flame" # S7FLAME`.
4. build.sh gen_gfx_tab() `want=` line: add `3ds/tools/gconv_host_syms.h src/entity/model/*.c src/entity/Shadow.c` after
   `3ds/tools/gconv_inc/include_asset.h` (the entity sources of S7ENT and S7ENTX regenerate the tables when they change).
5. flame.c: each `gSPDisplayList(gMainGfxPos++, X);` of :184, :186, :197, :212 becomes, under `#ifdef TARGET_3DS`, `nd_rec_dl(X);`,
   with the old line in `#else`. Add `#ifdef TARGET_3DS` / `#include "nd_gtab.h"` / `#endif` after the file's includes.
6. Scenario and bench `fxpic_27`: a copy of fxpic_84 with fxtest=27 (flame) in debug, subject, proves and reason; same window,
   until, cap. Same commit as step 5.

## Gate (expected values)
- `PM_ROW=S7FLAME PM_LANE=S7FLAME sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: `gen_gfx_tab` ran (not "up to date"), memcheck OK.
- `bench.py run nd_host --row S7FLAME --why "S7FLAME gtab"`: PASS; the `gtab lists <n> ... bad 0` line with n above main's.
- `bench.py base fxpic_27` (new bench: 2 runs) and `bench.py base spot_mac01_det`; one list `bench.py run fxpic_27,spot_mac01_det
  --row S7FLAME --why "S7FLAME: flame lists from tables"` (2 starts). cmp both: every shot byte-equal.
- Census: the fxpic_27 `gtabrun f<n> tab <a> dl <b>` entry after frame 400: `dl 0`, `tab` above 0. Budget 8 Azahar starts (6 planned).

## Stop
A gen or link error of the tool, `bad` above 0, a byte difference or dl above 0: stop, commit, hand back the exact output
(2 tries per named cause). Delete this row file in the completing commit. Hand back: the ready hash, the gate values, memcheck.
