# ws_DEC1: key colour fill for RGBA5551 textures (plan_decal.md section 4)
Row DEC1. Model Haiku. Start main after the DEC0 merge. Worktree build3ds/wt/DEC1, branch DEC1, PM_ROW=DEC1, PM_LANE=DEC1. Oracle NATIVE25.
Objective: each alpha-0 texel next to an opaque texel holds the mean colour of its opaque neighbours. C1..C12: none. Metric: --fill 5551 bad 0 (GUESS before 39635).
Order: 3ds/src/pm_tex_key.h (new), 3ds/src/pm_tex.c (2 edits, CRLF: use the Edit tool), 3ds/tools/pm_tex_key_test.c (new), 3ds/tools/nd_host_test.sh (1 edit).
## 1. 3ds/src/pm_tex_key.h (new, write it as below). Guard it like pm_tex_tx1.h (#ifndef PM_TEX_KEY_H).
```c
#include <stdint.h>
/* DEC1: t is a tiled RGBA5551 image (pm_tex.c tiled16 order), pw and ph powers of two 8..1024. An alpha-0 texel with an opaque 8-neighbour takes the floor mean RGB of those neighbours, alpha stays 0. Opaque texels never change, so the walk order does not matter. ws: pw + ph free words. Returns the changed count. */
static __attribute__((noinline)) unsigned tk_fill5551(uint16_t* t, int pw, int ph, uint32_t* ws) {
    static const uint32_t rc[9] = { 0u, 65536u, 32768u, 21846u, 16384u, 13108u, 10923u, 9363u, 8192u }; /* ceil(65536 / c): (s * rc[c]) >> 16 = s / c for s <= 31 c */
    uint32_t *ox = ws, *oy = ws + pw; unsigned n = 0;
    for (int x = 0; x < pw; x++) ox[x] = (uint32_t)(x >> 3) * 64u + (uint32_t)((x & 1) | (x & 2) << 1 | (x & 4) << 2);
    for (int y = 0; y < ph; y++) oy[y] = (uint32_t)(y >> 3) * (uint32_t)(pw >> 3) * 64u + (uint32_t)((y & 1) << 1 | (y & 2) << 2 | (y & 4) << 3);
    for (int y = 0; y < ph; y++) for (int x = 0; x < pw; x++) {
        uint16_t* p = t + oy[y] + ox[x]; if (*p & 1u) continue;
        unsigned r = 0, g = 0, b = 0, c = 0;
        for (int j = y > 0 ? y - 1 : 0; j <= y + 1 && j < ph; j++) for (int i = x > 0 ? x - 1 : 0; i <= x + 1 && i < pw; i++) {
#ifdef TK_SEED_FAULT
            if (i != x && j != y) continue; /* seeded fault: 4-neighbours only */
#endif
            const unsigned v = t[oy[j] + ox[i]]; if (v & 1u) { r += v >> 11; g += (v >> 6) & 31u; b += (v >> 1) & 31u; c++; } }
        if (!c) continue; const uint16_t nv = (uint16_t)(((r * rc[c]) >> 16) << 11 | ((g * rc[c]) >> 16) << 6 | ((b * rc[c]) >> 16) << 1);
        if (nv != *p) { *p = nv; n++; } }
    return n;
}
/* DEC1: 1 when a big-endian N64 palette of n entries has an alpha-0 entry (bit 0 of the low byte) */ static int tk_pal_a0(const unsigned char* p, int n) { if (!p) return 0; for (int i = 0; i < n && i < 256; i++) if (!(p[2 * i + 1] & 1u)) return 1; return 0; }
```
## 2. pm_tex.c. After `#include "pm_tex_tx1.h" /* TX1 */` (:34) add `#include "pm_tex_key.h" /* DEC1 */` and `static unsigned tk_said; /* DEC1: marker once */`.
- Before (:1632-1634): `staged = (const u8*)upload;`, `}`, `if (s->vram_stage) { stage_probe(s); ...`. After: the same 3 lines, these 3 lines between `}` and `if (s->vram_stage)`:
  `if (gf == GPU_RGBA5551 && !lut && !src_native && staged == scratch + TEX_SCRATCH_PLANE && (fmt != FMT_CI || tk_pal_a0((const u8*)tlut, tlut_n))) { /* DEC1 */`
  `    unsigned tkn = tk_fill5551((uint16_t*)(void*)(scratch + TEX_SCRATCH_PLANE), pw, ph, (uint32_t*)(void*)scratch); /* plane 0 is free here */`
  `    if (tkn && !tk_said++) pm_log_evt("tkd f%u n %u", pm_frame_count(), tkn); }`
## 3. Host test: pm_tex_key_test.c: offsets from a copy of tiled16 (pm_tex.c:443-450) / 2. (1) 8x8 zeros, (3,3) = 0xF801: returns 8, its 8 neighbours = 0xF800, the rest 0. (2) 16x16, (7,7) = 0x07C1:
  (8,8) = 0x07C0. (3) 8x8, (1,0) = 0x5001, (0,1) = 0x0501: (0,0) = 0x2A80. (4) tk_pal_a0: {00 01, 00 00} n 2 = 1, {00 01} n 1 = 0, NULL = 0. Print `tkd test PASS`, exit 0, else exit 1.
- nd_host_test.sh after `  /tmp/idxt` (:232) add `  gcc -m32 -Wall -Wextra -Werror -I 3ds/src -o /tmp/tkd 3ds/tools/pm_tex_key_test.c` and `  /tmp/tkd`.
Gate (1 build, 2 Azahar row runs, the bases from the tool): 1. `PM_LANE=DEC1 sh 3ds/build.sh`: memcheck OK. 2. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. The test built with -DTK_SEED_FAULT exits 1. 3. c0gate: calls 0 interp 0 state 0.
4. `python 3ds/tools/tex_fringe.py . --fill`: CI4 0 CI8 0 RGBA16 0. 5. `bench.py base <bench> --commit $(git merge-base HEAD main)` for kmr11_drec and spot_mac01_drec, then `bench.py run kmr11_drec,spot_mac01_drec --row DEC1 --why "DEC1 gate"`.
6. `bench.py cmp <bench> --row DEC1 --expect tkd`: `expected-diff PASS` (exit 2) or byte-equal, gst.txt equal. 7. `drec_state.py tdiff --bench <bench> --row DEC1`: other 0, tex_opaque 0.
Fix scope: pm_tex_key.h, pm_tex_key_test.c, the DEC1 lines of pm_tex.c. Stop: tex_opaque > 0, other > 0 or no tkd marker: SendMessage NATIVE25 with the output path.
Hand-back: commit hash, the 7 gate values, the tdiff lines. The last commit deletes this page.
