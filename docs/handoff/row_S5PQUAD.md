# Row S5PQUAD: the battle prim quads as a native fill record (NATIVEAPI E5b)

(Model: Haiku. It copies the S1FILL fill record. The one new part is a kind field that picks the combine and render words from a
table; no design choice. Why not smaller: there is no smaller model.)

## Blocker and base
S1FILL merged (fill_run and NdFillRec in src/screen_overlays.c, through the NAPI2B helpers). Then `sh 3ds/tools/wt_new.sh S5PQUAD`;
PM_ROW=S5PQUAD PM_LANE=S5PQUAD. Base: `git merge-base HEAD main`.

## Cause of the work (napicen3 btl f600)
battle/190B20.o records 32 typed ops a frame. That is btl_appendGfx_prim_quad (src/battle/190B20.c:3019-3037): 8 ops per quad,
4 quads a frame. The quads are the HP bar digits backs and the red bars (src/battle/16C8E0.c:932-948, drawn while the battle menu
is open) and two clear quads in btl_states_menus.c:1013, :1027. Each quad is: the setup list D_80293970 (:44-55), render mode and
combine (OPA_SURF with PM_CC_PRIM_FULL_ALPHA when a == 255, else XLU_SURF with G_CC_PRIMITIVE), prim colour, one 1-cycle
FillRectangle, then render mode TEX_EDGE and combine DECALRGBA.

## Touch only
- 3ds/include/nd_api.h: the NdFillRec type and the nd_fill_run declaration, at the end.
- 3ds/src/nd_direct.c: nd_fill_run, at the end.
- src/screen_overlays.c: delete fill_run and NdFillRec; the two call sites name nd_fill_run.
- src/battle/190B20.c: btl_appendGfx_prim_quad only.
No running row edits these places. NAPI3B edits msg_draw.c, and S1PREV and S1STEN edit screen_render_util.c: different files.

## Exact edits
1. Move S1FILL's record and runner to nd_direct.c, unchanged in what they draw:
   - nd_api.h: `typedef struct { int32_t x0, y0, x1, y1; uint32_t rgba, kind; } NdFillRec;` (24 bytes, 3 units) and
     `void nd_fill_run(const uint32_t* u);`.
   - kind replaces S1FILL's `opaque` field: 0 overlay translucent (G_RM_CLD_SURF pair, G_CC_PRIMITIVE), 1 overlay opaque
     (CVG_DST_SAVE | G_RM_OPA_SURF pair, G_CC_PRIMITIVE), 2 quad opaque (G_RM_OPA_SURF pair, PM_CC_PRIM_FULL_ALPHA),
     3 quad translucent (G_RM_XLU_SURF pair, G_CC_PRIMITIVE).
   - nd_fill_run takes the combine words and oml from a `static const` table of 4 rows. Build the words the way S1FILL's fill_run
     builds them, from the macros. PM_CC_PRIM_FULL_ALPHA is in include/macros.h:328: nd_direct.c may include "macros.h" only if
     it builds. If it does not, write the 8 terms out (`0, 0, 0, PRIMITIVE, 0, 0, 0, 1`) with a comment that names macros.h:328.
   - The body of fill_run stays as it is (nd_direct_bind with env nd_direct_env(), the exclusive lower right,
     pm_gpu_set_replace(0), pm_gpu_fill_rect, nd_direct_end). If the merged fill_run passes env 0, change it to nd_direct_env():
     nd_direct_bind stores env in nd_ws.envc, and later walker draws read it.
   - screen_overlays.c: the two records set kind = opaque ? 1 : 0 and pass nd_fill_run to nd_rec_napi.
2. 190B20.c btl_appendGfx_prim_quad: `#include "nd_api.h"` at the top. Under `#ifdef TARGET_3DS`:
   - keep `gDPPipeSync` and `gSPDisplayList(gMainGfxPos++, D_80293970)` (the walker keeps that state for later draws);
   - replace the render mode, combine, prim colour and fill lines (:3023-3032) with
     `{ NdFillRec q = { left, top, right, bottom, (u32)r << 24 | (u32)g << 16 | (u32)b << 8 | a, a == 255 ? 2u : 3u }; nd_rec_napi(nd_fill_run, &q, 3); }`;
   - keep the trailing PipeSync, TEX_EDGE render mode and DECALRGBA combine lines (later walker draws inherit them).
   The `#else` keeps the old lines.
2b. S1FILL gap, fixed here. fill_run skips off_screen_check. The walker fill (nd_rect.c:306-334) handles a CPU-fill or depth
   colour image (img_role(1)) and calls off_screen_check (:327) before it draws. Make nd_fill_run start with
   `if (!nd_direct_ready()) { fallback; return; }`. nd_direct_ready refuses every colour image that is not the main target.
   Fallback: into `static Gfx buf[8]`, with the gbi macros, write SetCombineMode and SetRenderMode of the kind's table row,
   SetPrimColor from rgba, and `gDPFillRectangle(p++, x0, y0, x1, y1)` (the record's exclusive lower right, as the call site
   passed it). Then `nd_walk_state((const uint32_t*)buf, (unsigned)(p - buf));`. The walker fill then takes its own path for
   those images. Count it: `static unsigned nfb; if (nfb++ < 16) pm_log_d("fill fallback f%u", pm_frame_count());`.
3. Pre-flight check (do it, hand back the line if it fails): `grep -n "nd_rec_napi" 3ds/include/nd_api.h` shows the 3-argument form
   (nd_api.h:11), and `grep -n "fill_run" src/screen_overlays.c` shows the S1FILL runner you move.

## Try 2 (row fault: try 1 did not list the host-test link; the orchestrator resets the budget once)
Fact: code 7dd4a8ee6 builds, but nd_host fails to link 3ds/tools/nd_direct_test.c (nd_host_test.sh:45 links nd_direct_test.c and
nd_api.c only; the test includes nd_direct.c): `undefined reference to pm_gpu_set_replace` and `pm_gpu_fill_rect`, which
nd_fill_run now calls. Step: in 3ds/tools/nd_direct_test.c, next to the other pm_gpu_* stubs (:56-68), add test stubs with the
signatures of 3ds/include/pm_gpu.h:109-110:
`void pm_gpu_set_replace(int on) { (void)on; }` and
`void pm_gpu_fill_rect(int x0, int y0, int x1, int y1, unsigned rgba) { (void)x0; (void)y0; (void)x1; (void)y1; (void)rgba; }`.
If the link then names nd_walk_state (the step 2b fallback), add `void nd_walk_state(const uint32_t* q, unsigned n) { (void)q;
(void)n; }` the same way. Do not change nd_host_test.sh. Then run the gate below.
Merge note: S5TRECT runs at the same time and also appends to the end of nd_api.h and nd_direct.c. Before your final gate run,
`git merge main`. If S5TRECT merged first, keep both appends: yours first, then S5TRECT's, or the order main has.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- `bench.py base battle_wheel_nowide_det` and `bench.py base xfer_map_shots` (the S1FILL scene: the moved runner). Then one list:
  `bench.py run battle_wheel_nowide_det,xfer_map_shots,napicen_btl --row S5PQUAD --why "E5b: battle prim quads as fill records"`
  (3 starts).
- `bench.py cmp` on both det scenes: every shot byte-equal.
- napicen_btl: the battle/190B20.o line is gone, or 0 ops a frame (it was 32).
- Budget 6 Azahar starts.

## Stop
A byte difference: stop. The likely cause is a later draw that inherited a state the record no longer leaves (prim colour or
geometry mode). Hand back the cmp output and do not add ops. Commit. Delete this row file in the completing commit.
