# Row KEYMISS: census of native model cache misses by key word (debug switch keymiss)

(Model: Haiku. Debug tool only, no draw change. Base: main 436abd053. The agent reads only this file, CLAUDE.md and what this file names.)

## Cause (oracle code read, main 436abd053)
- Key A = nd_mkey_a (pm_gpu.c:3200): FNV of record words 0-27 (w3 = 0; shroud: w14 = 0, w8 alpha = 0) and the R13 hash. Key B =
  pm_gpu_model_keyb (pm_gpu.c:3213): walker omode, geometry, handle and dlc generations. A key miss runs rec_miss (nd_recs.c:221),
  which builds the full key (92 words, pm_gpu_model_key pm_gpu.c:3172) at nd_recs.c:224.
- Record words that change per frame (src/model.c appendGfx_model): w5 finalMtx (model.c:3042, :3054, a matrix stack slot each frame
  while the model moves), w8 fog colour, w11 fog position (:2080-2083 shroud lerp, :1577 depth tint), w12/w13 prim/env (:1577, :1587),
  w17 billboard matrix (cam_main.c:221, a stack slot of this frame), w18-w21 panner tile sizes (:2160-2189), w22/w23 pre and post lists
  (frame stream ranges, model.c:4307). nd_mc_store (nd_model.c:646) replaces the last entry of the same model, so a value that
  alternates between frames misses every frame. nd_mc_miss (nd_model.c:497) counts only the first differing word: this census counts all.

## Start and touch only
`sh 3ds/tools/wt_new.sh KEYMISS`; PM_ROW=KEYMISS PM_LANE=KEYMISS on every build and bench call. New 3ds/src/pm_keymiss.c; 3ds/CMakeLists.txt (two source lists); nd_recs.c rec_miss (one line) and one prototype line; append-only entries
in 3ds/tools/azt_scenarios.json (list `scenarios`) and 3ds/tools/bench_defs.json (list `benches`). No other file.

## Exact edits
1. 3ds/src/pm_keymiss.c (includes stdio.h, stdlib.h, string.h, nd.h, pm_port.h). `#define KM_N 32u`, `KM_W (NDMC_RW + NDMC_SW)`.
   Owner struct: `uint32_t geo, r0, f; uint32_t w[KM_W]; uint16_t c[KM_W]; unsigned miss, fr, pre, ref; uint8_t used, hk;`.
   Statics: `KmOwn* km; unsigned km_g[KM_W], km_miss, km_new, km_pre, km_ref, km_f0, km_next = 300u;`.
   `__attribute__((cold, noinline)) void pm_km_note(const NdMKey* k, int cls)` (cls 0 key miss, 1 pre-list record, 2 refused hit):
   f = pm_frame_count(). First call: km = calloc(KM_N, sizeof *km) (NULL: return), km_f0 = f. If f >= km_next: km_print(f), then
   `while (km_next <= f) km_next += 300u;` (no division). Owner: geo = k->w[24], r0 = k->w[0]; h = ((geo >> 2) ^ (geo >> 11) ^ r0) &
   (KM_N - 1u); linear probe to an entry with used == 0 or equal geo and r0; none free: return. Unused: used = 1, geo, r0, km_new++.
   If o->f != f: o->fr++, o->f = f. cls 2: o->ref++, km_ref++, return. Else km_miss++, o->miss++, cls 1 also o->pre++, km_pre++.
   If o->hk: for each j < KM_W with o->w[j] != k->w[j]: o->c[j]++, km_g[j]++. Then memcpy(o->w, k->w, sizeof o->w), o->hk = 1.
   `static void km_print(unsigned f)`: pm_log_evt("keymiss f%u frames %u miss %u new %u pre %u ref %u", f, f - km_f0, ...); then
   pm_log("keymissw f%u%s", f, b) with b the nonzero km_g as " j:n" in j order (stop adding at 140 chars); then up to 8 times: the owner
   with the largest miss + ref (> 0), its 4 largest c[j] as " j:n", pm_log("keymisso f%u geo %08x r0 %08x miss %u fr %u pre %u ref %u w%s",
   ...), then set its miss and ref to 0. Reset: km_g, km_miss, km_new, km_pre, km_ref, every owner's c, miss, fr, pre, ref to 0;
   km_f0 = f. Owners keep geo, r0, w, hk. Comment at the top: words 0-27 are record words (R0 w0 to R13 w1), 28-91 walker words.
2. 3ds/CMakeLists.txt: a line `    src/pm_keymiss.c` after `    src/nd_recs.c` (:393); add `src/pm_keymiss.c` to the -Os list at :446.
3. nd_recs.c: above the comment line `/* HC6 step 3: everything of a record but the hit` (:219) add
   `void pm_km_note(const NdMKey* k, int cls); /* KEYMISS census (pm_keymiss.c, switch keymiss) */`. In rec_miss, after the line
   `if (!cached) { pm_xf.mdl_miss++; ...` (:225) add:
   `if (__builtin_expect(PMDL("keymiss"), 0) && q[12].words.w0 && (!cached || tried)) pm_km_note(mkp, cached ? 2 : q[11].words.w0 ? 1 : 0);`
4. Scenarios (copy the form of kzn04_shadestat; status unverified, needs_picture false, cap 30, deadline 20, until
   `keymiss f(6|7)\\d\\d `, expects warp, first_marker `warp: <map> entry 0`, events warp and `keymiss f[0-9]+ frames [0-9]+ miss`):
   keymiss_mac01 `warp=mac_01:0 pos=0:0:200 keymiss loglevel=debug`; keymiss_sbk56 `warp=sbk_56:0 pos=20:0:440 keymiss loglevel=debug`;
   keymiss_kzn08 `warp=kzn_08:0 keymiss loglevel=debug`; keymiss_hos01 `warp=hos_01:0 story=ch0_toad_town pos=-110:250:-170
   padscript=wait60,go_-36_-170_60,wait10,A,wait40,A,wait40,A keymiss loglevel=debug` (cap 35, deadline 25). Benches of the same
   names: kind azahar_stream, speed normal, default false, the scenario name, cap and until as the scenario, tags azahar keymiss, gates [].
   Then `python 3ds/tools/bench.py rebuild` and `bench.py show keymiss_mac01`. Scenarios and benches in one commit.

## Gate (CLAUDE.md 2; Azahar budget 6 of 8: base 1, spot 1, list 4)
- `PM_ROW=KEYMISS PM_LANE=KEYMISS sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `bench.py run nd_host --row KEYMISS --why "pm_keymiss.c build, nd_recs.c hook"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py run spot_mac01_det --row KEYMISS --why "switch off: the hook changes no draw"`,
  `bench.py cmp spot_mac01_det --row KEYMISS`: 6/6 byte-equal.
- One list on one slot: `bench.py run keymiss_mac01,keymiss_sbk56,keymiss_kzn08,keymiss_hos01 --row KEYMISS --why "census: misses
  per key word, 4 scenes"`: each log holds a keymiss line of frame 600-799.

## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit. Hand back: the hash, gate results, memcheck line, and for each of the 4 logs (path) the
keymiss, keymissw and keymisso lines of the window of frame 600-799 copied as they are. No interpretation.
