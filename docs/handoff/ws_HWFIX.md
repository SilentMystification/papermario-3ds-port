# ws_HWFIX: console-only faults of audit_hw (A1 ring colour flush, A2 stream chunks at boot, A3 viewport pool, B1 render mode index, B4 VRAM extents, B2 vertex bytes)
Model: Haiku. PM_ROW, PM_LANE = HWFIX. Branch HWFIX, worktree build3ds/wt/HWFIX. Budget: 4 builds (1 head and 1 base lane per group), 6 Azahar runs. Group 1 starts now (main fdf73a739). Group 2 starts after `Merge ISEED` is in `git log main`.
Owns: 3ds/src/pm_gpu.c, nd_rec.c, pm_main.c, pm_nusys.c, pm_vpsnap.c (new), nd_recs.c, pm_tex.c, 3ds/tools/pm_vpsnap_test.c (new), 3ds/tests/host.d/hwfix.sh (new),
src/battle/area/kzn2/actor/lava_piranha.c, src/world/common/lava_piranha/part2.inc.c; group 2 only: src/effects/lightning_bolt.c, stop_watch.c, whirlwind.c. Edit no other file.
Counters: no C1..C12 counter moves (FACT: no list, Gfx or gbi line changes). Proof: host tests, byte-equal drec, memcheck margin above 0 KB (FACT 454 KB at 59cd90449; GUESS about 447 KB after: +7 KB bss).
## Group 1 (1 build, 4 runs, commit `HWFIX 1: ...`)
1. A1 pm_gpu.c:3667 (a colour write into a ring slot that a kick split flushed stays in L1; the GPU reads the old bytes). Before `... c[2] = (u8)(rgba >> 8); c[3] = (u8)rgba; }`
   After `... c[2] = (u8)(rgba >> 8); c[3] = (u8)rgba; if (v < nb.gring_fl) pm_fl_dc(c, 4); /* HWFIX: the slot is below the flushed mark (a split ran after its load) */ }`.
2. A2 nd_rec.c: after ch_enter (:49-53) add this function. pm_main.c:952: after the `pm_zbuf_reserve();` line add `    nd_rec_reserve(); /* HWFIX: stream chunks 0 and 1 of both contexts, while the heap is whole */`
   and above `void pm_zbuf_reserve(void) {` (:277) add `void nd_rec_reserve(void); /* nd_rec.c */`.
   `/* HWFIX: chunks 0 and 1 of both chains at boot (pm_main.c, before the logic thread). On the O3DS the commit is full after a map load, so a chunk taken in game can fail (ch_get stops the game). */`
   `void nd_rec_reserve(void) {` / `    for (unsigned k = 0; k < 2u; k++)` / `        for (unsigned i = chain[k].nch; i < 2u; i++) {` / `            chain[k].ch[i] = (uint32_t*)malloc(CH_WORDS * 4u);`
   `            if (!chain[k].ch[i]) { pm_log_e("ndc: no memory for stream chunk %u of context %u at boot", i, k); return; }` / `            chain[k].nch = i + 1u;` / `        }` / `}`
3. A3 new 3ds/src/pm_vpsnap.c (CMake globs src/*.c, 3ds/CMakeLists.txt:346). Delete pm_nusys.c:11-25 (the CORE2-3 comment, PM_VP_POOL, the extern and pm_vp_snap). The new file:
   `#include <string.h>` / `#include "ultra64.h"` / `#include "pm_port.h"` / `extern s32 gCurrentDisplayContextIndex;` / the old CORE2-3 comment, then
   `/* HWFIX: a repeat of one of the last 2 values of this frame gets the same slot (sprite.c:338-339 snaps vpAlt and vpMain per component). */` / `#define PM_VP_POOL 128`
   then the old body, with 2 changes: after `if (f != last_f) { last_f = f; n = 0; }` add `    Vp* const p = pool[gCurrentDisplayContextIndex & 1];` /
   `    for (unsigned k = n > 2u ? n - 2u : 0u; k < n; k++) if (!memcmp(&p[k], v, sizeof *v)) return &p[k];`. Before `Vp* c = &pool[gCurrentDisplayContextIndex & 1][n++];` After `Vp* c = &p[n++];`.
4. Host test 3ds/tools/pm_vpsnap_test.c: includes stdio.h, string.h, "ultra64.h"; declares `Vp* pm_vp_snap(const Vp* v);`; defines `s32 gCurrentDisplayContextIndex;`, `static unsigned frame, warns;`,
   `unsigned pm_frame_count(void) { return frame; }`, `void pm_log_w(const char* fmt, ...) { (void)fmt; warns++; }`, mk(k) = a zeroed Vp with vp.vscale[0] = k and vp.vtrans[1] = 3k. Checks in order, each prints
   `vpsnap FAIL <name>` and returns 1 on a fail: copy (pa = snap(a): pa != &a, memcmp equal); dedupe (pb = snap(b), then snap(a) == pa and snap(b) == pb); full (snap mk(3)..mk(128), then snap(&x) of x = mk(129)
   returns &x and warns == 1); frame (frame++, snap(&x) != &x); context (gCurrentDisplayContextIndex = 1, frame++, p1 = snap(a): p1 != pa, memcmp equal). End: print `vpsnap ok 5/5`, return 0.
5. 3ds/tests/host.d/hwfix.sh (LF), the hunit.sh form: FL = the README flags with `-m32 -Wall`. `sed 's/k < n; k++/k < 0u; k++/' 3ds/src/pm_vpsnap.c > /tmp/vps_seed.c`, build the test with /tmp/vps_seed.c,
   the output must be `vpsnap FAIL dedupe` (else print `vpsnap seed FAIL: got [...]`, exit 1). Then build with 3ds/src/pm_vpsnap.c and run it (`|| exit 1`).
6. B1 nd_recs.c (the index is a record byte, nd_rm_tab has ND_RM_N 64 rows, nd_op.h:148). :42 rec_rm: first line `    if (i >= ND_RM_N) return; /* HWFIX */`.
   :149 Before `hd_run(d, nd_rm_tab[i], nd_rm_n[i]); }` After `if (i < ND_RM_N) hd_run(d, nd_rm_tab[i], nd_rm_n[i]); }`. :266 Before `nd_rm_tab[(qs->words.w0 >> 16) & 0xffu]` After `nd_rm_tab[(qs->words.w0 >> 16) & (ND_RM_N - 1u)]`.
7. B4 pm_tex.c:569 Before `#define SLAB_EXT 640` After `#define SLAB_EXT 1160 /* HWFIX: free extents <= live slab blocks + 2 bank blocks (slab_free merges both sides); live blocks <= SLOTS + PF_N */`.
   After :761 `enum { PF_N = 128 };` add `_Static_assert(SLAB_EXT >= SLOTS + PF_N + 2, "HWFIX: the extent table holds every free gap");`.
8. B2 (drec bytes only: no draw reads these fields, FACT combine and render mode of the 5 lists). lava_piranha.c after :496 and after :506, part2.inc.c after :249 and after :259 (each `vtx->cn[2] = j * 30;`)
   add `            vtx->cn[3] = 0; vtx->flag = 0; /* HWFIX: nd_rec_data memory is not cleared */`.
9. `sh 3ds/tools/nd_host_test.sh > build3ds/out/hwfix_h1.txt 2>&1`: `ND HOST TESTS PASS`, `vpsnap ok 5/5`. One build rc 0, `memcheck ... margin` above 0 KB. For B = spot_mac01_drec, then sbk14_drec:
   `python 3ds/tools/bench.py base B --row HWFIX --commit $(git merge-base HEAD main)`, `python 3ds/tools/bench.py run B --row HWFIX --why "HWFIX 1"`, `python 3ds/tools/bench.py cmp B --row HWFIX --commit $(git merge-base HEAD main)`: byte-equal. Commit.
## Group 2 (after Merge ISEED: 1 build, 2 runs, commit `HWFIX 2: ...`)
10. `git merge main --no-edit`. Edit form of chomp_drop.c (b31e07f23). stop_watch.c (2 vertices, :215-227): after each `vtx->ob[2] = 0;` add `vtx->flag = 0;`, after each `vtx->tc[1] = i * 128;` add
    `vtx->cn[0] = vtx->cn[1] = vtx->cn[2] = vtx->cn[3] = 0;`. whirlwind.c (:194-205): after each `vtx->v.tc[1] = ...;` add `vtx->v.flag = 0; vtx->v.cn[0] = vtx->v.cn[1] = vtx->v.cn[2] = vtx->v.cn[3] = 0;`.
    lightning_bolt.c after :299 and :309 (`vtx->cn[2] = i * 30;`) add `vtx->cn[3] = 0; vtx->flag = 0;`. Quote each line from the file after the merge (ISEED can move lines).
11. nd_host PASS. One build rc 0, margin above 0 KB. sbk14_drec: base, run, cmp as step 9 (`--why "HWFIX 2"`): byte-equal. Delete this page. Commit.
Fix rights: a compile error in pm_vpsnap_test.c or hwfix.sh: fix the test text (not the checks). An undeclared name in a step: add the include that declares it. Else 2 tries per named cause, then the stop rule (CLAUDE.md 5).
Stop: group 2 committed and gated. Hand-back build3ds/out/handback_HWFIX.txt, first 8 lines: commit hashes, host line (vpsnap, seed), the 3 cmp results, memcheck margin before and after, counters unchanged.
