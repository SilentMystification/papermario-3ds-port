# Row S6M1: mac_01 crystal ball custom gfx builders as S6T compiled ranges (map custom gfx, file 1)

(Model: Haiku. One file, src/world/area_mac/mac_01/crystal_ball_gfx.c. The S6S2 def form; draws and list calls stay ops.)

## Blocker and base
SCOMP0 merged (texture and PRIM units as ordered value units). `sh 3ds/tools/wt_new.sh S6M1`; PM_ROW=S6M1 PM_LANE=S6M1.

## Code read (oracle 4, main 2fdcf9a8b)
- The builders run inside a custom gfx stream range (model.c build_custom_gfx :4312, nd_cgfx_rng; core 0 replays it with
  nd_range_run, nd_walk.c:494, through nd_stream_run), so an NDC_NAPI record of nd_scomp_rec inside a builder replays in order.
- pre (:21-26): LookAtX, LookAtY (MOVEMEM values), Hilite1Tile, Hilite2Tile (one SETTILESIZE each: ordered values before the first
  static), Texture(0x1000, 0x1000, 0, RENDERTILE, ON), SetGeometryMode(G_TEXTURE_GEN) (static).
- post (:30-31): ClearGeometryMode(G_TEXTURE_GEN) (static); gSPEndDisplayList stays an op (it ends the range).
- inside (:90-126): R1 :90-95 CycleType, RenderMode, ColorImage(zbuffer) (value), CombineMode, PrimColor (value after the last
  static), PipeSync (no-op); :96 gSPDisplayList stays; R2 :97-108 PipeSync, ColorImage(cfb) (value), DepthSource, PrimDepth
  (value), RenderMode, TextureFilter, TexturePersp, Texture, TextureLUT, TextureDetail, TextureLOD, PipeSync; :113-118 the load,
  combine and rectangle stay ops; R3 :121-126 PipeSync, DepthSource, RenderMode, CombineMode, TexturePersp, Texture (all static).
  A colour image unit is a value (S6T design note 3: the decision reads no target field).

## Touch only
src/world/area_mac/mac_01/crystal_ball_gfx.c: the three builders and a def block above the first one; `#include "nd_scomp.h"`
under `#ifdef TARGET_3DS` after the includes. No other row edits this file.

## Exact edits (under `#ifdef TARGET_3DS`; `#else` keeps the old lines)
1. Def block: one `static const Gfx` array per range with the gs forms of its lines in order (drop the PipeSync lines), value slots
   as placeholders: `gsSPLookAtX(0)`, `gsSPLookAtY(0)`, `gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0, 0)` (Hilite1),
   `gsDPSetTileSize(G_TX_MIRROR, 0, 0, 0, 0)` (Hilite2; check its tile argument in gbi.h gDPSetHilite2Tile and copy it),
   `gsDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, 0)`, `gsDPSetPrimColor(0, 0, 0, 0, 0, 0)`, `gsDPSetPrimDepth(0, 0)`.
   Arrays: cb_pre_u (6 units), cb_post_u (1), cb_r1_u (5), cb_r2_u (10), cb_r3_u (5); one `static NdSDef` each
   (`{ array, sizeof array / sizeof array[0] }`).
2. Each builder: write the value units with the g macros into a local `Gfx v[4]; Gfx* p = v;` in their order (the exact
   argument expressions of the old lines), then `nd_scomp_rec(&def, v, (unsigned)(p - v));`. nv must be: pre 4, post 0, r1 2,
   r2 2, r3 0. If a macro wrote another count, the record logs `scomp: def ... has N value units`: stop and hand back.
   The post builder keeps `gSPEndDisplayList(gMainGfxPos++);` after its record. The inside builder keeps :96 and :113-118 as they
   are, between the records, and the early `return` at :48-50.

## Gate (expected values)
- `PM_ROW=S6M1 PM_LANE=S6M1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. `bench.py run nd_host --row S6M1 --why
  "S6M1"`: PASS.
- `PM_ROW=S6M1 PM_LANE=S6M1 python 3ds/tools/bench.py base spot_mac01_det`; one list: `bench.py run spot_mac01_det,s6chk_mac01,
  napicen_mac01 --row S6M1 --why "S6M1 crystal ball ranges"` (3 starts). `bench.py cmp spot_mac01_det --row S6M1`: every shot
  byte-equal. s6chk `bad 0`, `ranges` above main's last s6chk_mac01 value. napicen_mac01: the crystal_ball_gfx ops per frame drop
  (napicen3 had 32; hand back the number). No `scomp: def` line. Budget 5.

## Stop and end
A byte difference, `bad` above 0, a scomp error line, an edit that does not fit, or a test-tool fault: commit, hand back the exact
output. Delete this row file in the completing commit. Hand back: the ready hash, the gate values, the memcheck line.
