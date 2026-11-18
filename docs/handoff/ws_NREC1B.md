# ws_NREC1B (Haiku): model_draw reads the core 1 model snap, not the walker. Counter: none of C1-C12 (stage S6 consume). Gate: drec byte-equal, msnap nos 0 bad 0.
Designer: oracle NREC. Branch from main 27f33ea1da or later. Worktree build3ds/wt/NREC1B, branch NREC1B. PM_ROW=NREC1B PM_LANE=NREC1B. Runs parallel with NREC2.
Edit only (Edit tool, keep CRLF): 3ds/include/nd_msnap.h, 3ds/src/nd_msnap.c, 3ds/src/nd_msnap_in.c, 3ds/src/nd_rec.c (nd_rec_model only, lines 266-277), 3ds/src/nd_recs.c, 3ds/src/pm_gpu.c (model_draw only). New file 3ds/include/nd_msnapw.h.
## Facts (code read, main 27f33ea1da)
- FACT NREC1A data (fxops_snapchk f500): msnap cmp 260 bad 0 genv 0 wst 43 nos 1. nos = a chunk break between record and snap (nd_msnap.c:19).
- FACT model_draw (pm_gpu.c:3878) reads nd_ws.td[0] (:3888-3890), pm_gbi_sticky = nd_ws tss, geom, comb words (:3923, nd_comb.c:284), nd_ws.prim, prim_lod, gs.env_col (:3938), omode_h/l, om_kh/kl (:3940-3941), fogc, blendc through nd_mix1 (:3945, nd_set.c:191). nd_mix1 equals nd_snap_mix result 1 (nd_snap_in.c).
- FACT NdMSnap words (nd_msnap.h): magic 0, prim 1, prim_lod 2, omh 3, oml 4, kh 5, kl 6, tss 7, geom 8, cw0 9, cw1 10, fogc 11, blendc 12, envc 13, then b[0], b[1].
## Edits
1. New nd_msnapw.h (guard ND_MSNAPW_H, `#include <stdint.h>`): `enum { MSW_PRIM = 1, MSW_PLOD, MSW_OMH, MSW_OML, MSW_KH, MSW_KL, MSW_TSS, MSW_GEOM, MSW_CW0, MSW_CW1, MSW_FOGC, MSW_BLENDC, MSW_ENVC };` `struct NdTd; void nd_msnap_use(const uint32_t* q); const uint32_t* nd_msnap_w(void); const struct NdTd* nd_msnap_td0(void);`
2. nd_msnap_in.c: add `#include <stddef.h>` and `#include "nd_msnapw.h"`. After the includes: `_Static_assert(offsetof(NdMSnap, prim) == 4u * MSW_PRIM && offsetof(NdMSnap, tss) == 4u * MSW_TSS && offsetof(NdMSnap, envc) == 4u * MSW_ENVC, "NREC1B msw");`
3. nd_rec.c nd_rec_model. Line 271 before: `uint32_t* p = rec_take(2u + 2u * (G_PM_REC_WORDS + keep));` After: `uint32_t* p = rec_take(2u + 2u * (G_PM_REC_WORDS + keep) + 2u + 2u * ND_MSNAP_U); /* NREC1B: room for the jump and the model snap */`. Add `#include "nd_msnap.h"` to the includes. In nd_msnap.h add `#define ND_MSNAP_U ((unsigned)(sizeof(NdMSnap) / 8u))`.
   After line 274 (`... p[0] = NDC_HDR(NDC_MODEL, G_PM_REC_WORDS + keep, 0);`) add: `{ uint32_t* j = p + 2u + 2u * (G_PM_REC_WORDS + keep); j[0] = NDC_HDR(NDC_JUMP, 0, 0); j[1] = (uint32_t)(uintptr_t)(j + 2u + 2u * ND_MSNAP_U); } /* NREC1B */`
   Line 275: delete the text `if (__builtin_expect(PMDL("snapchk"), 0)) ` before `nd_msnap_rec(p, G_PM_REC_WORDS + keep);` (the snap is taken for every record).
4. nd_msnap.c. nd_msnap_rec body after: `NdMSnap* m = (NdMSnap*)(p + 2u + 2u * units + 2u); nd_msnap_take(&nd_ds, m); p[0] |= 1u; /* NREC1B: nd_rec_model reserved the room */`. Add `#include "nd_msnapw.h"`. Add `static unsigned ms_wb[4];` to the statics line. Add above nd_msnap_cmp:
```c
static NdMSnap ms_wk; static const NdMSnap* ms_cur = &ms_wk; /* NREC1B: the snap of the record that runs now */
static const NdMSnap* ms_of(const uint32_t* q) { const uint32_t h = q[-2]; const NdMSnap* s = (const NdMSnap*)(q + 2u * ((h >> 16) & 0xffu) + 2u); return ((h & 1u) && s->magic == ND_MSNAP_MAGIC) ? s : NULL; }
void nd_msnap_use(const uint32_t* q) { /* NREC1B: no snap: the walker inputs (fixed fallback, counted in nos) */
    const NdMSnap* s = ms_of(q);
    if (!s) { ms_nos++; ms_d.w = nd_ws; ms_d.fogc = fogc; ms_d.blendc = blendc; nd_msnap_take(&ms_d, &ms_wk); ms_wk.envc = pm_gpu_env_now(); s = &ms_wk; if (ms_nlog++ < 8u) pm_log_evt("msnap nos f%u", pm_frame_n); }
    ms_cur = s;
}
const uint32_t* nd_msnap_w(void) { return &ms_cur->magic; }
const NdTd* nd_msnap_td0(void) { return &ms_cur->b[0].td; }
```
   nd_msnap_cmp: replace its first 3 lines (h, s, the nos test) with `const NdMSnap* s = ms_of(q); if (!s) return; /* NREC1B: nd_msnap_use counts nos */`. Replace `if (nd_msnap_diff(s, &w, nm, 4, &nb)) ms_wst++;` with `const unsigned m = nd_msnap_diff(s, &w, nm, 4, &nb); if (m) ms_wst++; for (unsigned k = 0; k < 4u; k++) ms_wb[k] += (m >> k) & 1u; /* NREC1B: b0td b0t b1td b1t */`. Tick format: append ` w0td %u w0t %u w1td %u w1t %u` and the args `ms_wb[0], ms_wb[1], ms_wb[2], ms_wb[3]`.
5. nd_recs.c: add `#include "nd_msnapw.h"`. Line 255 before: `if (__builtin_expect(PMDL("snapchk"), 0)) nd_msnap_cmp(...)`. Insert before it: `nd_msnap_use((const uint32_t*)q); /* NREC1B: model_draw reads this snap */`.
6. pm_gpu.c: add `#include "nd_msnapw.h"` after `#include "pm_flipfps.h"`. After line 3877 add `int nd_snap_mix(unsigned, unsigned, unsigned, int, unsigned*); /* nd_snap_in.c */`. First line of model_draw body: `const uint32_t* const msw = nd_msnap_w(); const NdTd* const t0 = nd_msnap_td0(); /* NREC1B */`.
   Step A (scalars). :3923 before: `unsigned sw_tss, sw_geo, sw_cw0, sw_cw1; pm_gbi_sticky(&sw_tss, &sw_geo, &sw_cw0, &sw_cw1);` after: `const unsigned sw_tss = msw[MSW_TSS], sw_geo = msw[MSW_GEOM], sw_cw0 = msw[MSW_CW0], sw_cw1 = msw[MSW_CW1]; /* NREC1B */`.
   :3938 `nd_ws.prim` to `msw[MSW_PRIM]`, `nd_ws.prim_lod` to `msw[MSW_PLOD]`, `gs.env_col` to `msw[MSW_ENVC]`. :3940-3941 `nd_ws.omode_h` to `msw[MSW_OMH]`, `nd_ws.omode_l` to `msw[MSW_OML]`, `nd_ws.om_kh` to `msw[MSW_KH]`, `nd_ws.om_kl` to `msw[MSW_KL]` (every use on these 2 lines).
   :3945 before: `if (nd_mix1(ol_, &mixc))` after: `if (nd_snap_mix(ol_, msw[MSW_FOGC], msw[MSW_BLENDC], pm_nofog, &mixc) == 1)`.
   Step B (tile 0). :3888-3890: change every `nd_ws.td[0].` to `t0->` (13 uses).
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 5 Azahar starts of 8)
1. Edits 1-5 and step A. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nrec1b_host.txt 2>&1`: PASS. Commit `NREC1B 1: model snap always, model_draw scalars from the snap`.
2. Step B. Host test again: PASS. Commit `NREC1B 2: model_draw tile 0 from the snap`.
3. `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main) > build3ds/out/nrec1b_base.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row NREC1B --bench spot_mac01_drec --no-host --why "NREC1B model snap consume" > build3ds/out/nrec1b_rowloop.txt 2>&1`.
4. `PM_ROW=NREC1B PM_LANE=NREC1B python 3ds/tools/bench.py run fxops_snapchk --row NREC1B --why "NREC1B msnap" > build3ds/out/nrec1b_snapchk_run.txt 2>&1`. Then `grep -a "msnap\|snapchk f\|ndc: stream" build3ds/azt/logs/fxops_snapchk.txt > build3ds/out/nrec1b_msnap.txt`.
Gate (exact): host PASS. spot_mac01_drec byte-equal (rowloop exit 0). Last msnap line: cmp above 0, nos 0, bad 0. Last snapchk line: bad 0. No `ndc: stream` line. Report wst, w0td, w0t, w1td, w1t (data, not judged). On a DIFF: check out commit 1, run the rowloop of step 3 once more (1 start), report both.
Stop: a DIFF, nos above 0, bad above 0, a crash, a fatal line, a tool fault. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Fix rights: the files of this page, compile errors only. Pitfalls: one host test at a time. One bench per rowloop call. PM_CACHE=off only as a prefix of one command. `bench.py run` does not build: rowloop builds the lane before step 4.
Hand-back: last commit `NREC1B 3: gate. spot_mac01 drec equal; msnap cmp <n> nos 0 bad 0 w0td <n>` deletes this page. One SubagentHandback: commit hash, the last msnap and snapchk lines.
