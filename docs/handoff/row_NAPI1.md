# Row NAPI1: native draw API skeleton (NDC_NAPI command, nd_rec_napi, pso table)

(Model: Haiku. Plan: docs/handoff/plan_nativeapi.md S0. No producer uses it yet; NAPI2A and NAPI3A are the first callers.)

## Design (oracle code read, main 6e171b39a)
- One NDC_NAPI command per draw: word 1 = the core 0 runner (a function pointer), payload = its input record. stream_run (nd_rec.c:500)
  calls the runner, so a new producer never edits nd_rec.c (rows in different producer files never conflict).
- rec_take (nd_rec.c:164) already resets the recorder shadow (sh_all) for any non-state command, so ops after an NDC_NAPI are not dropped.
- nd_r11_walk (3ds/src/nd_model.c:280) returns 0 for an unknown command (default arm): no edit. No other stream reader exists.
- Pso table: nd_pso_n64 gives an id per distinct N64 state description; an entry never changes after its id is returned.

## Start
`sh 3ds/tools/wt_new.sh NAPI1`. PM_ROW=NAPI1 PM_LANE=NAPI1 on every build and bench call.

## Touch only
3ds/include/nd_cmd.h (enum), 3ds/src/nd_rec.c (new nd_rec_napi after nd_rec_texrect; one case in stream_run), new 3ds/include/nd_api.h,
new 3ds/src/nd_api.c, new 3ds/tools/nd_api_test.c, 3ds/tools/nd_host_test.sh (2 lines), 3ds/CMakeLists.txt (1 line).

## Exact edits
1. nd_cmd.h enum: after the NDC_STATE entry (its 2 comment lines) add
   `    NDC_NAPI = 10,   /* NATIVEAPI: one native draw (nd_api.h); word 1: the core 0 runner (NdNapiRun); payload: its record */`
2. New 3ds/include/nd_api.h (guard ND_API_H, `#include <stdint.h>`), with a 3-line header comment saying the Design bullets above, and:
   `typedef void (*NdNapiRun)(const uint32_t* rec);` (core 0: draw one recorded command; rec = the payload, 8-byte aligned)
   `void nd_rec_napi(NdNapiRun run, const void* rec, unsigned units);` (nd_rec.c, core 1: rec holds units 8-byte units, 1 to 255)
   `void nd_walk_state(const uint32_t* q, unsigned n);` (nd_walk.c, core 0: the typed op executor, as NDC_STATE runs it)
   `typedef uint16_t NdPso;` `#define ND_PSO_MAX 256u`
   `typedef struct NdPsoN64 { uint32_t cw0, cw1, oml, omh, omh_m; } NdPsoN64;` (combine words with the 0xfc byte; othermode L render
   mode value of gDPSetRenderMode; othermode H bits the draw sets and their mask)
   `NdPso nd_pso_n64(const NdPsoN64* d);` (core 1) and `const NdPsoN64* nd_pso_get(NdPso id);` (core 0)
3. New 3ds/src/nd_api.c: `#include <string.h>`, `#include "nd_api.h"`, prototypes `void pm_log_e(const char* fmt, ...);` and
   `void pm_fatal_stop(const char* msg);`. `static NdPsoN64 pso_tab[ND_PSO_MAX]; static unsigned pso_n, pso_last;`
   A cold noinline `pso_full(void)`: pm_log_e("napi: pso table full (%u)", ND_PSO_MAX); pm_fatal_stop("napi: pso table full");
   nd_pso_n64: return pso_last if pso_last < pso_n and memcmp equal; else linear memcmp search (hit: pso_last = i, return i);
   else if pso_n == ND_PSO_MAX call pso_full(); else copy d to pso_tab[pso_n], pso_last = pso_n, return pso_n++.
   nd_pso_get: `return &pso_tab[id];`
4. nd_rec.c: `#include "nd_api.h"`. After nd_rec_texrect: a cold noinline `napi_bad(unsigned units)` (pm_log_e("ndc: native draw
   record of %u units (1 to 255)", units); pm_fatal_stop("ndc: bad native draw record")), then nd_rec_napi: if units == 0 or > 255
   napi_bad(units); rec_open(); p = rec_take(2u + 2u * units); ndc_n[NDC_NAPI]++; p[0] = NDC_HDR(NDC_NAPI, units, 0);
   p[1] = (uint32_t)(uintptr_t)run; memcpy(p + 2, rec, units * 8u);
   stream_run: after `case NDC_STATE: ...` add `case NDC_NAPI: ((NdNapiRun)(uintptr_t)p[1])(p + 2); break;`
5. New 3ds/tools/nd_api_test.c: stubs pm_log_e (empty) and pm_fatal_stop (print "FAIL nd_api fatal", exit 1); a CHECK macro that prints
   "FAIL nd_api line N" and counts. Cases: a = {0xfc121824, 0xff33fffe, 0x00504340, 0, 0x3000}; b = a with oml 0x00552048.
   nd_pso_n64(&a) == 0; nd_pso_n64(&b) == 1; nd_pso_n64(&a) == 0; nd_pso_get(1)->oml == 0x00552048; for i = 2 .. ND_PSO_MAX - 1:
   c = a, c.cw1 = i, nd_pso_n64(&c) == i; then c.cw1 = 7 gives 7; nd_pso_n64(&b) == 1. Any fail: return 1. Else print "nd_api ok".
6. nd_host_test.sh: after the line `  /tmp/ndcm 3ds/tests/ndref/ndref_*.bin` add
   `  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ndapi 3ds/tools/nd_api_test.c 3ds/src/nd_api.c` and `  /tmp/ndapi`
7. 3ds/CMakeLists.txt: after the line `    src/nd_rec.c` add `    src/nd_api.c`.

## Gate (CLAUDE.md 2)
- `PM_ROW=NAPI1 PM_LANE=NAPI1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row NAPI1 --why "NAPI1: pso table host test"`: PASS and "nd_api ok" in its output.
- `PM_ROW=NAPI1 PM_LANE=NAPI1 python 3ds/tools/bench.py base spot_mac01_det`, then `bench.py run spot_mac01_det --row NAPI1 --why
  "NAPI1: stream_run gets the NDC_NAPI case; no producer records it"`, then `bench.py cmp spot_mac01_det --row NAPI1 --commit
  $(git merge-base HEAD main)`: 6/6 byte-equal. Budget: 2 Azahar starts.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
