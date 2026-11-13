# Row NPSO3: gen_pso_tab, the build-time pipeline table (every ND_PSO_DESC decided once on the host), and nd_pso()

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main ac0e68cd5)
- NPSO1: nd_api.h has NdPsoDesc (44 bytes), `#define ND_PSO_DESC(name, ...) static const NdPsoDesc name = { __VA_ARGS__ }`, the
  typedef NdPsoOut and `const NdPsoOut* nd_pso(NdPso id);` (declared, not defined). NPSO2: nd.h:494 `struct NdPsoOut { NdCombOut
  comb; NdRender rnd; NdTevOut tev; };`, nd_decide.c nd_pso_build(desc, out) (host-linkable: 3ds/tests/host.d/decide.sh links
  nd_decide.c, nd_tmpl.c and pm_comb.c, the test defines f32tof24).
- No ND_PSO_DESC exists in src yet (grep). The table is empty until NPSO5. The pattern of a generated file: 3ds/build.sh
  gen_gfx_tab (:84-100, called at :101), git-ignored 3ds/src/nd_gfx_tab.c (.gitignore:29), hashed by key_now.
Start: `sh 3ds/tools/wt_new.sh NPSO3`. PM_ROW=NPSO3 PM_LANE=NPSO3 on every call.
## Touch only (places)
New 3ds/tools/gen_pso_tab.py, 3ds/tools/gen_pso_main.c, 3ds/tests/pso/fix_descs.c, 3ds/tools/nd_psotab_test.c,
3ds/tests/host.d/psotab.sh. 3ds/build.sh: a new function gen_pso_tab right after gen_gfx_tab's closing `}` (:100) and its call
after `gen_gfx_tab >&2 # PUSHKEY` (:101), and `3ds/src/nd_pso_tab.c` added to the sha1sum list in key_now. .gitignore: one line.
## Exact edits
1. gen_pso_tab.py `<out.c> <src dir>...`: scan every .c file under the dirs for `ND_PSO_DESC(` and take the whole call (balanced
   parentheses). Write a host C file /tmp/pso_descs.c: `#include "nd_api.h"` only (correction 1: common.h pulls include/functions.h,
   whose `round` and `atan2` prototypes fail -Werror=builtin-declaration-mismatch; the fixture and NdPsoDesc need nothing else.
   A later desc that uses a G_ macro needs `#include <PR/ultratypes.h>` and `#include <PR/gbi.h>`: NPSO5 adds them), one `ND_PSO_DESC(<same
   text>);` per call, then `const NdPsoDesc* const pso_descs[] = { &name0, ..., NULL };` and `const char* const pso_names[] = {
   "name0", ..., NULL };`. Duplicate names: error with both file:line. Then compile and run gen_pso_main.c with it (gcc -m32,
   flags of decide.sh plus -I src, linking nd_decide.c, nd_tmpl.c, pm_comb.c and a f32tof24 copy as nd_decide_test.c has it).
2. gen_pso_main.c: for each desc call nd_pso_build and print <out.c>: `#include "nd.h"`, `_Static_assert(sizeof(struct NdPsoOut)
   == <host sizeof>, "NdPsoOut layout");`, `static const uint8_t nd_pso_raw[][<size>] __attribute__((aligned(4))) = {{...}};`
   (the bytes of each NdPsoOut; with 0 descs one all-zero row), `const unsigned nd_pso_n = <n>;`,
   `const NdPsoOut* nd_pso(NdPso id) { return id < nd_pso_n ? (const NdPsoOut*)(const void*)nd_pso_raw[id] : 0; }`, and as
   comments one line per id with its name. Ids follow the scan order (file name order, then line).
3. build.sh gen_pso_tab(): like gen_gfx_tab (docker pm3ds-host:1, the same mounts, a stamp of the inputs PT_IN: the scan dirs src
   and 3ds/src, the two tools, and 3ds/include (correction 1: nd.h and nd_api.h set the NdPsoOut layout and the desc type)), output 3ds/src/nd_pso_tab.c, replaced only when its content changes. Call it after gen_gfx_tab.
4. fix_descs.c: two descs, `ND_PSO_DESC(pso_fix_a, .cw0 = 0xFC121824u, .cw1 = 0xFF33FFFFu, .oml = 0x00552048u, .tex_on = 1);`
   and `ND_PSO_DESC(pso_fix_b, .cw0 = 0xFCFFFFFFu, .cw1 = 0xFFFE793Cu, .oml = 0x00552078u);`.
   nd_psotab_test.c: links the table that gen_pso_tab.py makes from 3ds/tests/pso, and for i 0 and 1 compares nd_pso(i) byte for
   byte with nd_pso_build of the same desc. Prints `psotab entries 2 bad 0`, and with `--fault` it compares id i with desc
   (i + 1) % 2 (a value change of the index): `psotab entries 2 bad 2`. host.d/psotab.sh runs both and fails unless the fault
   line shows bad above 0 and the real line bad 0.
5. .gitignore: `/3ds/src/nd_pso_tab.c`. Commit: `NPSO3: gen_pso_tab (build-time pipeline table), nd_pso(), host test`.
## Gate (expected values)
1. `python 3ds/tools/bench.py run nd_host --row NPSO3 --why "NPSO3"`: PASS with `psotab entries 2 bad 2` (fault) then
   `psotab entries 2 bad 0`.
2. `PM_ROW=NPSO3 PM_LANE=NPSO3 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: build OK (nd_pso_tab.c with 0 entries), memcheck
   OK. A second build prints the stamp skip of gen_pso_tab. No Azahar: no draw uses nd_pso yet. Counter: none (design core of
   plan section 2, used by NPSO4, NPSO5, CONV2D).
## Stop and end
A layout assert that fails on the device, a desc the host cannot compile (hand back the file:line), or a tool fault: stop, commit,
hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
