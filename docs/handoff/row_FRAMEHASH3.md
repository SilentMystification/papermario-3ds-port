# Row FRAMEHASH3: the fhash switch hashes the command words of each frame on the device (pm_gpu.c hook)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main f2749d74d)
- 3ds/include/nd_fhash.h: NdFhEnv {map(pa, n, ctx) -> readable bytes or NULL, role(pa, ctx) -> small id, ctx} and
  `int nd_fh_frame(uint64_t* h, const uint32_t* w, uint32_t n, const NdFhEnv* e)` (FRAMEHASH1, FRAMEHASH2 merged). ND_FH_INIT.
- pm_gpu.c records the GX entries of a frame only while kc_rec is set: begin_wait :2913 `kc_rec = pm_interp_on > 0; ...`, kc_take
  (:~2970, static) copies q->entries into kc_ent, kc_build (:3129, before C3D_FrameEnd) takes them. A process list entry is
  `data[0] & 0xFF == 1`, data[1] = list address (virtual), data[2] = bytes. The open tail list is GPUCMD_GetBuffer (base, toff).
- Physical to virtual (correction 1): libctru has no osConvertPhysToVirt. pm_gpu.c:617-620 drec_p2v(phys, dl) (HOSTCORE2) maps
  VRAM 0x18000000-0x18600000 to +0x07000000 and the linear heap by dl = phys - virt of any linear pointer. nd_fh_frame reads
  vertex and index bytes (ring and static area: linear heap), texels (linear stage copies or VRAM slots); targets give only a
  role. Every read is VRAM or linear heap. drec_p2v has no range check: fh_map adds it. Targets: target and targetR colorBuf / depthBuf, the off-screen slots
  gs.offs[k].tex.data (pm_gpu.c :2051-2140).
Start: `sh 3ds/tools/wt_new.sh FRAMEHASH3`. PM_ROW=FRAMEHASH3 PM_LANE=FRAMEHASH3 on every call. Blocker FRAMEHASH2: merged.
## Touch only (places)
3ds/src/pm_gpu.c: one forward declaration `static int fh_on(void);` right above pm_gpu_begin_wait (correction 1), the kc_rec line in pm_gpu_begin_wait (:2913), one new cold function block right above pm_gpu_end_w, one call
line right before `kc_build();` (:3129). New azt.d/FRAMEHASH3.json, bench.d/FRAMEHASH3.json. FPS60VAL (not started) waits.
## Exact edits
1. Above pm_gpu_end_w add a cold block, starting with `#include "nd_fhash.h"` (correction 1): `static int fh_sw = -2, fh_w0, fh_w1;` and `static int fh_on(void)` that reads once
   `fhash` (pm_debug_has) and `fhwin=<first>:<count>` (pm_debug_value, sscanf "%d:%d") and returns 1 when the switch is on and
   pm_frame_count() is in [first, first + count). Callbacks: `fh_map(pa, n, ctx)`: VRAM (pa in
   0x18000000..0x18600000 - n) -> drec_p2v(pa, 0); else v = pa - dl with dl = osConvertVirtToPhys(kc_base) - (u32)kc_base (set in
   fh_frame, passed in ctx); return v when [v, v + n) lies in [__ctru_linear_heap, __ctru_linear_heap + __ctru_linear_heap_size)
   (`extern u32 __ctru_linear_heap, __ctru_linear_heap_size;`), else NULL (nd_fh_frame then feeds 0xDEADBEEF and n),
   `fh_role(pa, ctx)`: 1 colour and 2 depth of target, 3 and 4 of targetR, 16 + k for off-screen slot k (compare pa with
   osConvertVirtToPhys of each buffer), else 255. `static __attribute__((cold, noinline)) void fh_frame(void)`: kc_take(q);
   h = ND_FH_INIT; words = 0; for each kc_ent with id 1: nd_fh_frame(&h, (const u32*)data[1], data[2] / 4, &env), words +=
   data[2] / 4; then the open tail (GPUCMD_GetBuffer: base + 0 .. toff) the same way; then
   `pm_log_evt("fhash f%u h %016llx draws %u words %u", pm_frame_count(), (unsigned long long)h, draws_frame, words);`.
2. :2913: `kc_rec = pm_interp_on > 0 || fh_on();` (the rest of the line stays).
3. Right before `kc_build();` (:3129): `if (__builtin_expect(fh_on(), 0)) fh_frame();` (fh_on reads the switches at its first call).
4. Scenario `spot_mac01_hash` (all fields as spot_mac01_det): its debug with `shots shotwin=200:6` replaced by `fhash fhwin=200:6`,
   until `fhash f205 `, needs_picture false, expected_events [{name "fhash f200", regex `fhash f200 h [0-9a-f]{16}`}, {name
   "fhash f205", regex `fhash f205 h [0-9a-f]{16}`}]. Bench `spot_mac01_hash`, kind azahar_hash.
5. Commit: `FRAMEHASH3: fhash switch, per-frame command word hash at the frame end, scene spot_mac01_hash`.
## Gate (expected values, correction 2: a new switch has no base at the merge-base)
The merge-base (0316d7f46) has no fhash switch, so its build never prints `fhash f205`: `bench.py base spot_mac01_hash` at the
merge-base can only end "no until". The base of the new scene is taken at this row's own commit.
1. Build, memcheck OK. nd_host PASS. (Done: static 21660 margin 326.)
2. Remove the tool's `"reason": "broken: no until in 35 s ..."` from 3ds/tools/azt.d/FRAMEHASH3.json (back to the row's reason
   text) and commit the real code (no seed). Then `bench.py base spot_mac01_hash --commit <that commit> --row FRAMEHASH3`: two
   runs equal, stored (6 frames in fhash.txt).
3. Seeded fault (a value change, every variable stays used): in fh_frame, right before the pm_log_evt line, feed one constant
   byte into the hash: `{ const uint8_t sd = 0x5A; h = nd_fh_bytes(h, &sd, 1); }`. Build, `bench.py run spot_mac01_hash --row
   FRAMEHASH3 --why "FRAMEHASH3 seeded fault"`, `bench.py cmp spot_mac01_hash --row FRAMEHASH3 --commit <the real-code commit>`:
   6/6 frames DIFF. Remove the seed, build again.
4. No draw change: `bench.py base spot_mac01_det` at the merge-base (stored or one base build), `bench.py run spot_mac01_det`
   with the real code, `bench.py cmp spot_mac01_det --row FRAMEHASH3`: 6/6 byte-equal.
5. Budget: the orchestrator resets it (row spec fault). A base pair that differs is a tool fault (BASEKEEP line): stop.
## Stop and end
Two runs that differ without the seed (hand back the BASEKEEP line), a tool fault, or a line that does not match: stop, commit,
hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
