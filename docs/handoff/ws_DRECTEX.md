# ws_DRECTEX (Haiku, worktree build3ds/wt/DRECTEX, branch DRECTEX). No counter moves (tool fault of the drec record).
Objective: drec writes `h gpu` in place of the texel hash when the texels lie in a block that the GPU writes (copy_blk, full_blk[], small_blk[]).
Cause (FACT, code): drec_draw hashes texels with the CPU at record time (pm_gpu.c:667, nd_drec.c:26-29). Draw 316 samples grab_tex (256x512 RGBA8, :2569): its copy is queued at :2801, the quad is recorded at :2814, no GX queue run between them. Draw 317 samples a 512x256 RGBA5551 slot block (:369 or :401).
The GPU writes these blocks (:412, :2633, render target :2225). The hashed bytes depend on GPU and emulator state, not on the recorded draw.
## Edits, in this order
1. 3ds/include/nd_drec.h. After :10 add `typedef struct NdDrecSpan { uintptr_t lo; uint32_t n; } NdDrecSpan; /* bytes [lo, lo + n) the GPU writes */`.
   In NdDrec, after :14 add `uint8_t tex_gpu[3]; /* 1: GPU-written texels: the t line says h gpu */`. After :18 add `int nd_drec_in_span(uintptr_t a, const NdDrecSpan* s, uint32_t ns);`.
2. 3ds/src/nd_drec.c. Add the function. Before: one fprintf with `h %016llx` (:27-29). After: the two fprintf calls below. A line without tex_gpu stays byte-equal.
```c
int nd_drec_in_span(uintptr_t a, const NdDrecSpan* s, uint32_t ns) {
    uint32_t i;
#ifdef ND_DREC_SEED_FAULT
    for (i = 0; i < ns; i++) if (s[i].n && a - s[i].lo <= s[i].n) return 1; /* DRECTEX seeded fault: the end byte counts */
#else
    for (i = 0; i < ns; i++) if (s[i].n && a - s[i].lo < s[i].n) return 1;
#endif
    return 0;
}
fprintf(f, "t %u type %08x dim %08x bytes %u h ", (unsigned)u, (unsigned)r->tex_type[u], (unsigned)r->tex_dim[u], (unsigned)r->tex[u].n);
if (r->tex_gpu[u]) fprintf(f, "gpu\n"); else fprintf(f, "%016llx\n", (unsigned long long)nd_drec_fnv(r->tex[u].p, r->tex[u].n));
```
3. 3ds/tools/nd_drec_test.c. Add draw 2: `build(&c, 2, vtx); c.tex_gpu[0] = 1;`, write it after draw 1. Append its 8 lines to `expect`: the draw 0 block with
   `draw 2` and `t 0 type 0000000d dim 00040004 bytes 16 h gpu`. Add span_fails(): spans {0x1000,0x100},{0x8000,0x40},{0x9000,0}. Cases (address, want):
   0x1000 1, 0x10FF 1, 0x1100 0, 0x0FFF 0, 0x803F 1, 0x8040 0, 0x9000 0. Print `drec ok 3` only when the text is equal and span_fails() is 0.
4. 3ds/tests/host.d/drec.sh. Append `|| exit 1` to line 3. Add: the same gcc line with `-DND_DREC_SEED_FAULT -o /tmp/drec_seed`, then `if /tmp/drec_seed > /dev/null; then echo "drec FAIL: the seeded fault passed"; exit 1; fi; echo "drec seed ok"`.
5. 3ds/src/pm_gpu.c. After :595 add `static NdDrecSpan drec_span[1 + OFF_N + SMALL_N]; static uint32_t drec_nspan; /* DRECTEX */`.
   In drec_begin after `drec_seq = drec_over = 0;` (:611) fill the table: copy_blk with BLK_FULL, each non-NULL full_blk[i] (i < OFF_N) with BLK_FULL16,
   small_blk[k] (k < small_n) with SMALL_BLK. After :668 add `rec.tex_gpu[u] = (uint8_t)nd_drec_in_span((uintptr_t)rec.tex[u].p, drec_span, drec_nspan);`.
## Gate (at the first failure, send its output path to the oracle with SendMessage)
G1. `sh 3ds/tests/host.d/drec.sh` prints `drec ok 3` and `drec seed ok`. Then `PM_HOSTSEL=all sh 3ds/tools/nd_host_test.sh`: ND HOST TESTS PASS.
G2. One build: `sh 3ds/build.sh`. Base: `python 3ds/tools/bench.py base fxops_drec` (merge-base, a tool cost, no budget).
G3. `python 3ds/tools/bench.py run fxops_drec --row DRECTEX --why "DRECTEX gate"` until two warm runs exist (cap 3 starts; cmp refuses a cold run).
    Copy drec.txt after the first warm run to build3ds/out/drectex_a.txt. `cmp build3ds/out/drectex_a.txt build3ds/azt/shots/fxops_drec/DRECTEX/drec.txt`: equal.
G4. B=$(git rev-parse --git-common-dir)/../build3ds/basecache/$(git merge-base HEAD main)/fxops_drec/drec.txt, R=row drec.txt. Same line count. Each differing line
    is a base t line `... h <16 hex>` whose row text is the same with `gpu` for the hex. Expected: diff 9, bad [] (FACT row f8f224beba: 2 grab/slot + 7 small_blk).
    Self-test: row line 6463 with `h gpu` set to `h 0000000000000000` gives bad [6463]. A plain `bench.py cmp fxops_drec` fails by design. Max 1 build, 3 starts.
## Edit rights, stop, hand-back
To fix a failing host test, edit only the 5 files above. Do not edit bench.py, bench_defs.json or azt_scenarios.json. Old stored bases stay valid
(key basecache/<commit>/<bench>, bench.py:1436, :1486). Stop when G1-G4 pass. Commit the 5 files, delete this page there. Hand back: hash, G1-G4, `h gpu` count.
