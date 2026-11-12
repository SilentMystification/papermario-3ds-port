# Row S7FLAME: the flame effect lists through the S7 tables (the two flame buffers defined in the flame gfx source)

(Model: Haiku. The S7EFX pattern for one effect plus a move of two definitions. No stub, no stand-in (user rule, 2026-11-12).)

## Cause (ORACLE7 code read, main e2f67f9d3)
- src/effects/gfx/flame.c:8-9 declares `extern unsigned char D_800B32E0[], D_800B3AE0[]`. On the device they are defined in
  src/main.c:11-13 (`// used in effects/gfx/flame.c`, two 0x800-byte ALIGNED(16) buffers; pm_3ds.map: .bss of src/main.o).
  src/bss/main_post_bss.c:35-36 is the N64 bss layout file: CMakeLists.txt does not build it. flame.c is their only user
  (grep src, include, 3ds/src: only a comment in pm_gpu.c:2075).
- gen_gfx_tab.sh links gconv_in.o (it includes every GCONV_SRC source) into nd_gfx_conv and nd_host's nd_gtab_test. src/main.c
  cannot go into that link (boot code, many outside symbols), so S7EFX3 dropped flame.
- The real fix: the buffers belong to the flame effect. Under TARGET_3DS their one definition moves into src/effects/gfx/flame.c
  (the file that uses them). The device build links the same buffers from flame.o instead of main.o; the converter compiles the
  same definitions. Nothing is invented for the host.
- Call sites: src/effects/flame.c:184, :186, :197 (`preset->dlist`), :212. fxtest index of flame: 27.

## Start
The branch S7FLAME holds try 1 (279491a34, a host stub: rejected). In build3ds/wt/S7FLAME: `git revert --no-edit 279491a34`,
then the edits below. PM_ROW=S7FLAME PM_LANE=S7FLAME on every build and bench call.

## Touch only
src/main.c (:11-13 only), src/effects/gfx/flame.c (:8-9 only), src/effects/flame.c (4 call sites, one include), gen_gfx_tab.sh (one
new line right after `# S7MDL`), 3ds/build.sh (the gen_gfx_tab() `want=` line), azt_scenarios.json and bench_defs.json (`fxpic_27`
right after `fxpic_84`). Not gen_gfx_syms.py (S7ENTX edits it).

## Exact edits
1. src/main.c :11-13: wrap the comment and the two definitions in `#ifndef TARGET_3DS` / `#endif` with the comment line
   `// TARGET_3DS: defined in src/effects/gfx/flame.c (S7FLAME: the gfx table converter compiles that file)`.
2. src/effects/gfx/flame.c :8-9 becomes `#ifdef TARGET_3DS` / `unsigned char D_800B32E0[0x800] __attribute__((aligned(16)));` /
   the same for D_800B3AE0 / `#else` / the two extern lines / `#endif`. (ALIGNED comes from common.h, included later in the file:
   use the attribute directly.)
3. gen_gfx_tab.sh: after the line ending `# S7MDL` add `GCONV_SRC="$GCONV_SRC flame" # S7FLAME`.
4. build.sh gen_gfx_tab() `want=` line: add `src/entity/model/*.c src/entity/Shadow.c` after `3ds/tools/gconv_inc/include_asset.h`
   (the entity sources of S7ENT and S7ENTX regenerate the tables when they change).
5. flame.c: each `gSPDisplayList(gMainGfxPos++, X);` of :184, :186, :197, :212 becomes, under `#ifdef TARGET_3DS`, `nd_rec_dl(X);`,
   the old line in `#else`. Add `#ifdef TARGET_3DS` / `#include "nd_gtab.h"` / `#endif` after the file's includes.
6. Scenario and bench `fxpic_27`: a copy of fxpic_84 with fxtest=27 (flame). Same commit as step 5 (try 1 has them: keep its
   entries if the revert left them out, re-add them).

## Gate (expected values)
- Build: gen_gfx_tab ran, the link has one definition of each buffer (pm_3ds.map: .bss.D_800B32E0 from src/effects/gfx/flame.o),
  memcheck OK. `bench.py run nd_host --row S7FLAME --why "S7FLAME gtab"`: PASS, `gtab lists <n> ... bad 0`, n above main's.
- `bench.py base fxpic_27` (new bench: 2 runs) and `bench.py base spot_mac01_det`; one list `bench.py run fxpic_27,spot_mac01_det
  --row S7FLAME --why "S7FLAME: flame lists from tables"` (2 starts). cmp both: every shot byte-equal (the buffers move address;
  a difference here is a fact for the oracle, not a rounding case).
- Census: the fxpic_27 `gtabrun` entry after frame 400: `dl 0`, `tab` above 0. Budget 8 Azahar starts (6 planned).

## Stop
A link error, `bad` above 0, a byte difference or dl above 0: stop, commit, hand back the exact output (2 tries per named
cause). Delete this row file in the completing commit.
