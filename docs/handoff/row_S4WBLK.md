# Row S4WBLK: water_block components on the native particle path (NATIVEAPI S4)

(Model: Haiku: the S4EMB pattern, one loop, no design choice. Runs in parallel with S4E3D, S4SNAK, FSELCEN: different files and
different insert points in the two JSON files.)

## Design
water_block_appendGfx (src/effects/water_block.c:216-304): after `gSPDisplayList(D_090003B0_3B6FA0)` (:278), per component with
`data->unk_88[i] >= 0` (:280-288): the float matrix (guPositionF), `gSPMatrix(PUSH|MUL|MODELVIEW)`, `gSPDisplayList(D_090004A0_3B7090)`,
`gSPPopMatrix`. Same macros into a local Gfx[3], nd_parts_add with pre 1 (matrix), post 1 (pop). The rest of the function (the
vertex mesh at :290-303) stays as it is. The previous-frame refraction is water_splash, not this effect.

## Start
`sh 3ds/tools/wt_new.sh S4WBLK`. PM_ROW=S4WBLK PM_LANE=S4WBLK on every build and bench call. Base: `git merge-base HEAD main` (main is
20d613506 at writing).

## Touch only
src/effects/water_block.c (water_block_appendGfx, the include list); 3ds/tools/azt_scenarios.json and 3ds/tools/bench_defs.json:
`fxpic_84` inserted right after the `fxpic_105` entry, `fxfncov_84` right after the `fxfncov_55` entry (both files).

## Exact edits
1. `#include "nd_api.h"` after the includes. Declare `NdParts pb;` under `#ifdef TARGET_3DS` with the other locals.
2. After `gSPDisplayList(gMainGfxPos++, D_090003B0_3B6FA0);` (:278): `#ifdef TARGET_3DS` `nd_parts_begin(&pb, 1, 1);` `#endif`.
3. Inside `if (data->unk_88[i] >= 0)` keep guPositionF and guMtxF2L; the 3 gbi lines (:284-286) become `#ifdef TARGET_3DS`
   `{ Gfx g[3]; gSPMatrix(&g[0], &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
   gSPDisplayList(&g[1], D_090004A0_3B7090); gSPPopMatrix(&g[2], G_MTX_MODELVIEW); nd_parts_add(&pb, (const uint32_t*)g); }` `#else`
   the old 3 lines `#endif`.
4. After the loop, before `gSPDisplayList(gMainGfxPos++, D_09000300_3B6EF0);` (:290): `#ifdef TARGET_3DS` `nd_parts_end(&pb);` `#endif`.
5. Scenario and bench `fxpic_84` (copy of `fxpic_50`, `fxtest=84`) and `fxfncov_84` (copy of `fxfncov_50`, `fxtest=84`).

## Gate
- Build, memcheck OK; nd_host PASS.
- `bench.py base fxpic_84 --commit <merge-base>`; `bench.py run fxpic_84 --row S4WBLK --why "S4: water_block components as one
  NDC_NAPI record"`; `bench.py cmp fxpic_84 --row S4WBLK --commit <merge-base>`: every shot byte-equal.
- Speed gate (census): `PM_FNCOV=1 PM_ROW=S4WBLK PM_LANE=S4WBLK sh 3ds/build.sh`; `bench.py run fxfncov_base,fxfncov_84 --row S4WBLK
  --lane S4WBLK_fncov --why "S4 speed gate"`; `hotset.py --fncov <log> --fncovfn --lane S4WBLK` on both logs. Pass (effect minus
  base): nd_part_draw > 0; gv_tri, draw_tri, nd_geom_vtx, nd_ops_run and ent_gdl each < 20 (the mesh at :295-301 is not a part:
  if only its calls remain above 20, hand back the deltas; do not change the mesh). Hand back the 15 largest deltas.
- Budget: 5 Azahar starts (1 base, 1 byte run, 2 census, 1 spare). Console: none here; the S4 batch console run after the merge.

## Try 2 (oracle code read; the code edit 12cf5c703 stays, no code change)
- Facts of try 1: fxpic_84 6/6 byte-equal; census at fxfncov_84 f420: nd_part_draw 0, gv_tri +80, ent_gdl +42, rec_open +448.
- Cause: the census frame is after the water block is gone. fxtest spawns at slot frame 2 (pm_fxtest.c:179-180, :263-275: START 400,
  so spawn at f402) with `fx_water_block(FX_WATER_BLOCK_DESTROY, X, Y, Z, 1.5f, 10)` (pm_fxtest.c:116). duration 10 sets timeLeft 10
  (water_block.c:91); at timeLeft < 0 the DESTROY type starts water_splash and removes the block (water_block.c:145-153), about f413.
  At f420 only the splash draws: its drops (water_splash.c:193-228, a LoadMultiTile and a list per drop) are the deltas. No component
  draw happens in that frame, so nd_part_draw 0 is not a shape fallback. The list D_090004A0_3B7090 (VTX 4 at slot 0, TRI2, PipeSync,
  End) has the same shape as embers' D_090003B0_3DC2F0, which passes parts_ok (nd_rec.c:283-299). Components are live from the first
  update: unk_88 starts at -13, -9, ..., +111 (water_block.c:105-107).
- The base frame f396 of fxfncov_base is by design: it is before the spawn (f402), so it is the frame with no effect.
- Scenario edits (one commit): `fxfncov_84` gets `fncov=406` (4 frames after the spawn, inside the block's life). `fxpic_84` gets
  `shotwin=404:4` and until `shot \d+ frame 407\b` (the old shots 412-415 straddle the destroy at about f413).
- Runs (4 starts; budget 3 used): `bench.py base fxpic_84 --commit <merge-base>`; `bench.py run fxpic_84 --row S4WBLK --why "S4WBLK
  try 2: shots inside the block life"`; `bench.py cmp fxpic_84 --row S4WBLK --commit <merge-base>`. Then the census lane:
  `bench.py run fxfncov_base,fxfncov_84 --row S4WBLK --lane S4WBLK_fncov --why "S4WBLK census at f406"`; hotset with `--lane S4WBLK`.
- Pass: every fxpic_84 shot byte-equal. Census (effect minus base, per frame): nd_part_draw above 0; nd_geom_vtx at most 2 per live
  instance (the mesh loads at water_block.c:295 and :299); gv_tri and draw_tri at most 28 per instance plus 20 (the mesh lists
  D_090004D8 and D_09000538 hold 10 + 4 TRI2 = 28 triangles); ent_gdl and nd_ops_run each below 20 per instance plus 20. Expected:
  nd_part_draw equals the number of live components. The fxtest spawns one instance: the per-instance limits are the numbers.

## Try 3 (oracle 3 code read of 12cf5c703; the orchestrator reset the budget once; no code change, no new Azahar run)
Facts of try 2: fxpic_84 7/7 byte-equal. Census, f406 effect minus f396 base: nd_part_draw 0 against 0, gv_tri +28, draw_tri +28,
nd_geom_vtx, nd_ops_run and ent_gdl 0.

Cause: this is a spec fault in the gate, not in the code. `hotset.py --fncovfn` prints only functions with 20 or more calls in the
frame (3ds/tools/hotset.py:319-326, `if n >= 20`), so "nd_part_draw 0" means fewer than 20 calls, not none. In this frame the block
has one live component:
- NUM_WATER_BLOCK_COMPONENTS is 4 (include/effects.h:1554);
- the create loop sets unk_88 = { -1, -5, -9, -13 } (water_block.c:105-107);
- each update adds 1 (:179-180) from f403, so at f406 component 0 is at 3 (live) and component 1 at -1.
So one instance: 1 call of nd_part_draw, which the filter hides.

The component draws natively:
- its list D_090004A0 is VTX 4 at slot 0, TRI2, PipeSync, End (assets/us/effects/gfx/D_090004A0_3B7090.gfx.inc.c), the same
  shape as embers' D_090003B0_3DC2F0, and it passes parts_ok (nd_rec.c:283-299);
- nd_ops_run and ent_gdl stayed 0, so there was no fallback.
The 28 gv_tri are exactly the vertex mesh: D_090004D8 has 10 TRI2 and D_09000538 has 4 TRI2, so 28 triangles (water_block.c:297,
:301). That is not a part, and 28 is inside the try-2 limit of 48.

Step (no build, no run: the try-2 census logs build3ds/azt/logs/fxfncov_84.txt and fxfncov_base.txt are the data):
1. Write build3ds/out/fncall.py:
   ```
   import sys, re; sys.path.insert(0, "3ds/tools"); import hotset as h
   m = h.parse_map(h.lane_dir("S4WBLK", True) / "pm_3ds.map")
   want = {a: e["name"] for a, e in m.items() if e["name"] in ("nd_part_draw", "nd_parts_run", "gv_tri", "nd_ops_run", "ent_gdl")}
   tot = {}
   for l in open(sys.argv[1], encoding="utf-8", errors="replace"):
       mm = re.search(r"fncov f\d+ ((?:[0-9a-f]{8}:\d+ ?)+)", l)
       if not mm: continue
       for t in mm.group(1).split():
           a, n = t.split(":"); a = int(a, 16)
           if a in want: tot[want[a]] = tot.get(want[a], 0) + int(n)
   print(sys.argv[1], tot)
   ```
   Run it on both logs: `python build3ds/out/fncall.py build3ds/azt/logs/fxfncov_84.txt` and the same for fxfncov_base.txt.
   The S4WBLK_fncov lane map must be the one of the census build of try 2. If the lane was rebuilt since, rebuild it first with
   `PM_FNCOV=1 PM_ROW=S4WBLK PM_LANE=S4WBLK sh 3ds/build.sh` (same commit, same map).
2. Pass (effect minus base): nd_part_draw >= 1 (expected 1) and nd_parts_run >= 1 (expected 1). Hand back both lines.
3. If both pass, the row is complete (byte gate 7/7 from try 2, census pass): delete this row file in one commit and hand back the
   hash. If nd_part_draw is 0 in the raw counts: stop and hand back both lines.

## Stop
A failed gate or an edit that does not fit: stop, commit, hand back the output. Delete this row file in the completing commit.
