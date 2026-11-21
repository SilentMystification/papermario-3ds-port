# plan_seams: cracks on buildings (oracle NATIVE24)
User report (FACT, rule 1): seams show on buildings, as cracks or lines along shared edges of walls, roofs and models.
User asks: a new seam gate, and one consistent float path from the vertex to the pixel. Counters C1..C12: none. Row metrics: the census lines below.

## 1. The path of a static map vertex (FACT, code read)
1. Vertex: N64 Vtx s16 x y z in object units (shape file). The ring and the pool keep the s16 words (attr4 GPU_SHORT, pm_gpu.c:1950). An s16 is exact in f24.
2. Model matrix: identity list = no load (IGNORE_MATRIX, model.c:2137, nd_recs.c:66). Other list = G_MTX LOAD of savedMtx = guMtxF2L of the
   shape tree product (model.c:3036, pm_gu.c:38-50, truncation), unpacked exactly (nd_mtx.c:31-43). Camera = view x perspective in 16.16 (cam_main.c:187).
3. game = nd_mtx_mul(MV, P) in f32 (nd_mtx.c:26-29, :112). palette = nd_mtx_pcomp(post, game) in f32 (nd_mtx.c:117-137). Every path uses these
   2 functions: walker range (pm_gpu.c:1069), native model miss (nd_model_set.c:29-30), cache hit (pm_gpu.c:4315-4316), sprite (pm_gpu.c:4748).
4. Upload: f32 words with bit 31 (f32 mode) on all 3 writers (nd_emit.c:21, pm_gpu.c:1342, :2374). The GPU makes the only f32 to f24 conversion
   of position data. Its rounding mode: OPEN. Software f32tof24 only for depth map scale and offset (nd_tmpl.c:369-370) and the clip plane (pm_gpu.c:799-802).
5. Shader: dp4 of the palette rows with (x, y, z, 1) in f24 (1 sign, 7 exponent, 16 mantissa bits). pm4 and pm5 have the same position block
   (pm4.v.pica main4, pm5.v.pica main5: mov r1.xyz to mov outpos.w). Mono frames add 0 (eye 0). Raster snap 1/16 px: GUESS (nd_seam.c:4, Citra).
6. Size: a building point near 500 units has an f24 step of 2^-8 = 0.004 units (FACT arithmetic). GUESS: 0.002 to 0.01 px per rounding at 300 to 500 units.
7. Azahar runs the shader in f32: GUESS (Citra design, not read here). So the console can show f24 cracks that Azahar does not show.

## 2. Path audit (each place with 2 numeric paths)
| Place | file:line | Two paths? | Seam? |
|---|---|---|---|
| Map list model matrix | nd_recs.c:66, nd_mtx.c:96, nd_mtx_set.c:79-98 | yes: id lists use P, other lists MV x P | yes, f24 split (R2). FACT path, GUESS size |
| Lists under rotation or fraction (cls 2) | nd_seam.c:351, :224 | world points not integer, T points rounded in object space | yes, gaps up to 0.87 unit (G). FACT path |
| Step 4 hole raster | nd_seam_test.c:215, :237 | skips cls 2 lists, one camera matrix for all | the gate never saw G or R2. FACT |
| T points | nd_seam.c, nd_dlc.c:183-200 | entities and the pool-full ring get none (nd_shape.c:388-392, commit f7af448f12) | yes (T). FACT path |
| Palette product | nd_mtx.c:112, :132 | no: one function pair | no. FACT |
| Cache hit, DO1 memo, MSNAP2 | pm_gpu.c:4313-4316, nd_rec.c:340, nd_msnapw.h:6 | no: the hit runs the same product, the memo copies the new matrix pointer (r[5]), the snap holds no matrix | no. FACT |
| Uniform upload | nd_emit.c:21, pm_gpu.c:1342, :2374 | no: f32 mode everywhere | no. FACT |
| K frame (fps60) | nd_emit.c:84-92, pm_gpu.c:1280, :1348, pm_interp.c:163 | K words patched into the L stream, L matrices are float lerps | only in motion. GUESS. Tools: twinchk (nd_mtx_set.c:217), icmp (pm_gpu.c:2508) |
| zmark pass 2 | nd_model_set.c:67 | changes palette row 2 (z) only | no xy seam. FACT |
| Post, viewport, scissor | pm_gpu.c:232-256 | one post per frame for 3D draws, wide edge only for ortho (pm_gpu.c:3706) | no. FACT |
| Stereo eye shift | pm4.v.pica main4 (mad r5.xyz) | w-dependent term, 3D slider only | adds to R2. GUESS |
| PICA clip | pm_gpu.c:797-803 (w >= 0.05) | clip vertices per triangle | GUESS, low |
| Texture edge | nd_model.c:41-46, pm_options.c:463 | texel centre off by default (+0.5 texel only on the switch) | lines of the far texture border. GUESS |
| Fog, depth, decal bias | pm5.v.pica fog, nd_tmpl.c:344-370 | per vertex or depth only | no crack. FACT |

## 3. Ranked causes
1. G: gaps at cls 2 lists. mac_01 has 44 of 148 lists in cls 2 (FACT, host log build3ds/out/base_src_mdlblk/build3ds/out/CLEAN2_host1.txt).
   Their world points are not integers and their T points round in object space (nd_seam.c:224). GUESS: 0.3 to 1 px slivers. No gate measures it.
2. T: residual T points. mac_01 keeps 20 (FACT, same log), 13 holes off T edges in the single-matrix raster (FACT). Near-duplicate vertices (commit f7af448f12).
3. R2: 2 palettes for 1 world point (11 int and 44 cls 2 lists against 93 identity lists on mac_01, FACT). GUESS: 0.002 to 0.02 px, a pinhole where the 1/16 snap differs.
4. Models without T points: entities, NPCs, the pool-full ring. FACT path. Count: OPEN.
5. Texture border lines (texel centre off). GUESS. 6. K frames in motion. GUESS. 7. Stereo shift, clip. GUESS.
   Outlines around alpha-test overlays (key colour in the bilinear mix) are not seams: plan_decal.md (oracle NATIVE25) owns them.

## 4. Consistency rule (the user's guarantee)
- N1. A position reaches the PICA only as s16 vertex words and f32 uniform words with bit 31. The GPU makes the only f32 to f24 conversion.
- N2. One product chain: game = nd_mtx_mul(MV, P), palette = nd_mtx_pcomp(post, game). No other function makes a palette.
- N3. pm4 and pm5 keep one identical position block.
- N4. A static map point that 2 lists share reaches the GPU as the same (vertex bits, palette bits). Census: split counter (target 0 after CRK4).
- Loud failure: seamlint.sh (N1 to N3, every host test run), host steps 5 and 6 (N4 and the crack numbers, exact expected lines).
- The orchestrator adds rule N1 to N4 as one line to hw_guidelines.md (oracles do not edit it).

## 5. The seam gate
- Host (every commit, no Azahar): nd_seam_test step 5 (world census, 6 maps) and step 6 (mac_01 camera census in f24, round-to-nearest and
  round-to-zero, 1/16 snap, holes with all lists). Exact expected lines in the test. Seeds: SEAM_SEED=5 (1 ulp in one identity matrix: split rises),
  SEAM_SEED=6 (one identity list rounds to zero: the same-input counter rises). Each seeded run must exit 1.
- Bench (runtime, real device draws): seam_mac01_drec (Merlon house spot, drec=200:1 drecvx) and seamcen.py (split, snap counts). The drec text
  stays byte-exact. Join: the orchestrator adds seam_mac01_drec to the wave set after its base exists (2 runs).
- Lint: seamlint.sh counts the numeric path sites per file and checks the shader block. A new site fails until an oracle reviews it.

## 6. Rows
| Row | Model | Page | Depends | Moves |
|---|---|---|---|---|
| CRK0 | Haiku | ws_CRK0.md | main | step 5 lines (baseline) |
| CRK1 | Haiku | ws_CRK1.md | Merge CRK0 | step 6 line (baseline px, holes) |
| CRK2 | Haiku | ws_CRK2.md | main | seamcen line of seam_mac01_drec |
| CRK3 | Haiku | ws_CRK3.md | main (other hunk of nd_host_test.sh than CRK0) | seamlint OK |
Fix rows (pages after CRK1 numbers; the oracle ranks by gpx and holes):
- CRK4 (Sonnet, user yes): world variant for static int lists. The converter writes dlc variant 2 (vertices + t, T points in world). The device
  draws it with the base palette nd_mtx_mul(nd_mv[nd_mv_sp - 1], nd_proj) and its K twin, only for hdr bit 8 static, LOAD, not billboard.
  Moves split s01, s11, s12 to 0. Expected-diff gate on the baked draws. Reason for Sonnet: converter, dlc format, palette and K twin in one row.
- CRK5 (Sonnet, user yes): world variant for static cls 2 lists, rounded to integers (shape moves up to 0.87 unit), normals rotated for lit lists.
  The seam table then sees them as cls 0. Moves gap gc2 and holes. Fallback: keep cls 2 lists and weld only points within 0.5 unit.
- CRK6 (Haiku): weld near-duplicate points of static int lists (0 < d <= 1.5) in the converter. Moves gint.
- CRK7: T points for entity lists. CRK8: texel centre default, after a uv border census. Both OPEN, no page.

## 7. Open
- PICA f24 rounding mode and dp4 order. The census reports both modes. A console probe needs the user.
- The model matrix product of transform group members (model.c:2667, MUL): step 6 mtxchk shows if the census matrices equal the device ones.
- Does the user see the seams on Azahar or on the console? Both: G is first. Console only: R2 rises in rank.
