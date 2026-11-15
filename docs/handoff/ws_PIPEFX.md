# ws_PIPEFX (Haiku): the imgfx anim list becomes typed units (Mario in a pipe)
Objective: USER FACT 2026-11-15: in a pipe Mario disappears. FACT cause: imgfx_load_anim (src/imgfx.c:1260-1284) reads raw F3DEX2
words from the ROM (gen_data_rom.py:7 imgfx_data). Since 6213e3cad6 the loop compares them with the typed ids NDO_VTX and NDO_END.
No raw word has these ids. So no vertex load is rebased, the list stays raw (the walker runs typed units only) and the loop runs past
the buffer end and writes w1 of any heap unit with id NDO_VTX. Counter: none (a picture fault). Worktree build3ds/wt/PIPEFX, branch PIPEFX.
## Edit 1: new file 3ds/include/nd_imgfx.h (exact text)
```c
/* PIPEFX: the imgfx anim list (raw F3DEX2 words from the ROM, imgfx.c imgfx_load_anim) to typed units in place. A vertex load points
 * into vb (ImgFXVtx is 12 bytes, Vtx 16). Stops after the end unit or after n units. Returns the units done. */
#ifndef ND_IMGFX_H
#define ND_IMGFX_H
#include <stdint.h>
#include "nd_op.h"
static inline unsigned nd_imgfx_type(uint32_t* g, unsigned n, uint32_t koff, uintptr_t vb) {
    unsigned j = 0;
    while (j < n) {
        const uint32_t w0 = g[2u * j], w1 = g[2u * j + 1u];
        const uint32_t t0 = ND_X0(w0, w1);
        uint32_t t1 = ND_X1(w0, w1);
        if ((t0 >> 24) == NDO_VTX) t1 = (uint32_t)((((int32_t)t1 - (int32_t)koff) / 3) * 4 + (int32_t)vb);
        g[2u * j] = t0; g[2u * j + 1u] = t1;
        j++;
        if ((t0 >> 24) == NDO_END) break;
    }
    return j;
}
#endif
```
## Edit 2: src/imgfx.c
Before (line 7): `#include "nd_op.h" /* HC4e2b: the typed unit of a list word */`. After: the same line, then `#include "nd_imgfx.h"`.
Before (lines 1258-1284): the comment `// Search through the state's displaylists ...` and the whole `for (i = 0; i < ARRAY_COUNT(state->gfxBufs); i++) { ... } while (cmd != NDO_END); }` block.
After (the 3 lines replace that block):
```c
        for (i = 0; i < ARRAY_COUNT(state->gfxBufs); i++) { /* PIPEFX: raw ROM words to typed units, vertex loads into vtxBufs[i] */
            nd_imgfx_type((uint32_t*)state->gfxBufs[i], anim->gfxCount, (uint32_t)(uintptr_t)anim->keyframesOffset, (uintptr_t)state->vtxBufs[i]);
        }
```
Remove a local variable only if the compiler says it is unused.
## Edit 3: host test 3ds/tools/nd_imgfx_test.c and 3ds/tests/host.d/pipefx.sh
Copy the form of 3ds/tools/nd_mesh_test.c and 3ds/tests/host.d/meshst.sh. The test builds 4 units in a uint32_t array:
G_VTX `(0x01u<<24)|(4u<<12)|(4u<<1)`, w1 `0x100 + 24` (koff 0x100); G_TRI1 `(0x05u<<24)|0x000204`, 0; G_ENDDL `0xDFu<<24`, 0;
a guard unit `NDO_W0(NDO_VTX, 0)`, `0x1234`. It calls nd_imgfx_type(g, 4, 0x100, 0x8000). Pass: return 3, unit 0 id NDO_VTX and w1 0x8020,
unit 1 id NDO_TRI1, unit 2 id NDO_END, guard unchanged. It prints `imgfx ok`, else `imgfx FAIL case <k>` (k = the first wrong check, 0..4).
Seed in pipefx.sh: `sed 's/ND_X0(w0, w1)/w0/'` on a copy of nd_imgfx.h. The seeded build must print `imgfx FAIL case 0`.
## Loop and gate (1 build, 2 Azahar runs)
1. Edit 1-3. Run `sh 3ds/tools/nd_host_test.sh > build3ds/out/pipefx_host.txt 2>&1`. Pass: PASS and `imgfx ok`. Commit.
2. One build. Then `bench.py run spot_mac01_drec --row PIPEFX` and `bench.py run sbk14_drec --row PIPEFX`, then
   `bench.py cmp <bench> --row PIPEFX --commit $(git merge-base HEAD main)` for each. Pass: 1/1 byte-equal each (GUESS: neither bench draws an imgfx anim).
3. A failing host test: fix only nd_imgfx.h or the test. A differing drec: stop. Write the cmp output to build3ds/out/ and send the path to the oracle.
Stop: both gates pass. Commit `PIPEFX 2: gate` with the values, delete this page in that commit.
Hand-back: build3ds/out/handback_PIPEFX.txt (commit, host result, 2 cmp results). The user checks the pipe on the console.
