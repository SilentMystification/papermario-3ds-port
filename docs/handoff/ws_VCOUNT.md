# ws_VCOUNT (Haiku): mdl_get_vertex_count reads the device units of a moved list (the map change crash)
Objective: USER FACT (build 5774): crash at mac_00 -> mac_01, 7B440.c:123 reads mac_01_settings.entryList = 0x50a301f0. FACT cause: HiddenPanel.c:325-330
gives NDN(Gfx_HiddenPanel_RenderTop) = (Gfx*)&ndd_ (an NdNDev, nd_gtab.h:73) to mdl_project_tex_coords (HiddenPanel.c:36 each frame). mdl_get_vertex_count
(src/model.c:4226) reads the NdNDev as units up to NDO_END. FACT (python over pm_3ds_5774.elf): minVtx 0x91e9b0, maxVtx 0xbe31d0, 181378 vertices;
model.c:4520-4524 write tc and cn RGB in each 16-byte slot of that .data. Counter: none (a crash). Worktree build3ds/wt/VCOUNT, branch VCOUNT.
Edit only: 3ds/include/nd_vcount.h (new), src/model.c, 3ds/tools/nd_vcount_test.c (new), 3ds/tests/host.d/vcount.sh (new). Edit 1: new file 3ds/include/nd_vcount.h (exact text):
```c
/* VCOUNT: mdl_get_vertex_count (model.c): at most n units (a moved list: NdNDev nu, no end unit), else up to NDO_END; N64 min and max. */
#ifndef ND_VCOUNT_H
#define ND_VCOUNT_H
#include "nd_op.h"
static inline unsigned nd_vcount(const uint32_t* u, unsigned n, uint32_t* lo, uint32_t* hi) {
    uint32_t mn = 0, mx = 0; unsigned k = 0;
    while (k < n) {
        const uint32_t w0 = u[2u * k], w1 = u[2u * k + 1u]; k++;
        if ((w0 >> 24) == NDO_VTX) { const uint32_t e = w1 + ((w0 >> 12) & 0xffu) * 16u; if (mn == 0) { mn = w1; mx = e; } if (mx < e) mx = e; if (mn > e) mn = e; }
        if ((w0 >> 24) == NDO_END) break;
    }
    *lo = mn; *hi = mx; return k;
}
#endif
```
## Edit 2: src/model.c. Before (line 12): `#include "nd_geo.h"`. After: the same line, then `#include "nd_gtab.h"` and `#include "nd_vcount.h"`.
Before (lines 4243-4271): from `        Gfx* baseGfx = gfx;` to `        w1 = 64; // TODO required to match -- can be any operation that stores w1`. After (remove a local only on a compiler warning):
```c
        const NdNDev* dev = nd_ntab_find(gfx); /* VCOUNT: a moved list key (NDN, nd_gtab.h:73) is its NdNDev: read its nu device units */
        u32 lo, hi;
        *gfxCount = (s32)nd_vcount(dev != nullptr ? dev->u : (const uint32_t*)gfx, dev != nullptr ? (unsigned)dev->nu : 0xFFFFFFFFu, &lo, &hi);
        *numVertices = ((s32)hi - (s32)lo) >> 4;
        *baseVtx = (Vtx*)lo;
```
## Edit 3: host test (copy the form of 3ds/tools/nd_mesh_test.c and 3ds/tests/host.d/meshst.sh). A = 0x00100000u. Case 0: {ND_NOP_A,0,
ND_OP_VTX_W0(4,0),A, NDO_W0(NDO_TRI2,0),0, ND_OP_VTX_W0(8,0),0x00be0000u, NDO_W0(NDO_END,0),0}, n 3: return 3, lo A, hi A+64. Case 1:
{ND_OP_VTX_W0(3,0),A, ND_OP_VTX_W0(2,0),A-64, NDO_W0(NDO_END,0),0}, n 0xFFFFFFFF: return 3, lo A-32, hi A+48. Case 2: {NDO_W0(NDO_END,0),0},
n 0xFFFFFFFF: return 1, lo 0, hi 0. Print `vcount ok`, else `vcount FAIL case <k>`. Seed in vcount.sh: `sed 's/while (k < n)/while (1)/'` on a
copy of nd_vcount.h. The seeded build must print `vcount FAIL case 0`.
Loop and gate (1 build, 2 Azahar runs). 1. Edits 1-3. Run `sh 3ds/tools/nd_host_test.sh > build3ds/out/vcount_host.txt 2>&1`. Pass: PASS and `vcount ok`. Commit `VCOUNT 1`.
2. One build, memcheck margin above 0 KB. Run `bench.py run spot_mac01_drec --row VCOUNT`, `bench.py run sbk14_drec --row VCOUNT`, then `bench.py cmp
   <bench> --row VCOUNT --commit $(git merge-base HEAD main)` for each. Pass: 1/1 byte-equal each. No bench walks mac_00 -> mac_01: the user checks it.
3. A failing host test: fix only nd_vcount.h or the test. A differing drec: stop, write the cmp output to build3ds/out/, send the path to the oracle.
   Stop: both gates pass. Commit `VCOUNT 2: gate` with the values and delete this page. Hand-back: build3ds/out/handback_VCOUNT.txt (commit, host, margin, 2 cmp).
