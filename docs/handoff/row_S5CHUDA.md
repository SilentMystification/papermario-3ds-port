# Row S5CHUDA: the direct textured quad helper nd_direct_quad (NATIVEAPI E1, design core of the S5CHUD split)

Model: Sonnet (pre-approved for the night run).
- Why not Haiku: the first direct draw of ring vertices with a matrix. It must keep the walker's matrix (and the fps60 twin), the
  ring load, the clip and cull of gv_tri, and it must not let the triangle path rebind tile 0 from the stale walker tile state.
  The epoch rule (below) is a design decision inside shared triangle code.
- Why not Opus: the walker functions are reused, not rewritten; one quad shape.
- Why not smaller: the helper, its epoch rule and its host case are one unit. The call site is row_S5CHUDB.md (Haiku, after this
  merges); this row also writes the record and runner given in step 4.

## Start
`sh 3ds/tools/wt_new.sh S5CHUDA`. PM_ROW=S5CHUDA PM_LANE=S5CHUDA on every call. Base: `git merge-base HEAD main`.

## Code read (main 5c3865487)
- The old ops of a mode 2 complex HUD element (src/hud_element.c:1647-1840): filter, gSPMatrix(VIRTUAL_TO_PHYSICAL(
  &matrixStack[k]), PUSH|LOAD|MODELVIEW), combine PM_CC_47, prim, render mode, gDPLoadTextureBlock IA8 w x h, TLUT NONE,
  gSPVertex(quadBuffers[FrameQuadIndex], 4, 0), gSP1Triangle 0 1 2, gSP1Triangle 0 2 3, gSPPopMatrix. VIRTUAL_TO_PHYSICAL is the
  identity on 3DS (include/macros.h:69).
- The walker runs the geometry ops in op_geo (3ds/src/nd_walk.c:267-296): NDO_MTX -> load_mtx(a, b, 0) (nd_mtx_set.c:130; the
  fps60 twin comes from nd_mtx_replace(src) keyed by the source address, so passing the same Mtx address keeps the twin);
  NDO_VTX -> nd_geom_vtx(a, b, at) (nd_geom.c:429); NDO_TRI1 -> tri_prep_ok = 0; draw_tri(a) (nd_geom.c:314); NDO_POPMTX ->
  gbi_popmtx(b) (nd_mtx_set.c:195). Matrix ops do not raise gv_epoch; state ops do.
- The direct bind (nd_direct.c:90-147): nd_direct_bind writes the walker other mode, combine and prim; nd_direct_tex binds the
  descriptor (nd_tex_bind_desc, ens_* = desc) and sets tile_x0/y0/fx/fy, but not nd_ws.td[0] or tst1 tile 0.
- The hazard: the first triangle after a state change runs gv_epoch_enter (nd_geom.c:201-224), which calls
  ensure_tile(nd_ws.use_t1 ? 1 : 0) and dual_bind(). ensure_tile (nd_tex_set.c:490) compares ens_* with the stale nd_ws.td[0]
  and rebinds the old walker tile: the quad would sample the wrong texture.

## Design (the helper; the coder may choose the inner steps, the API and the gate are fixed)
1. API, appended at the end of 3ds/include/nd_api.h:
   - `void nd_direct_quad(uint32_t mtx, const void* vtx4);` core 0, between nd_direct_bind/nd_direct_tex and nd_direct_end: the
     matrix push-load of mtx (the w1 the old gSPMatrix wrote), the vertex load of the 4 Vtx at vtx4 to slots 0-3, triangles
     (0,1,2) and (0,2,3), the pop. Each step through the walker function the op would run (load_mtx, nd_geom_vtx, draw_tri,
     gbi_popmtx) with the words the old op had (w0 of the matrix: the low byte (G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW) ^
     G_MTX_PUSH, as the F3DEX2 macro writes; the vertex word ND_W0_VTX(4, 0); the triangle words of gSP1Triangle; popmtx 64).
     Set pm_gbi_cur_op per step as op_geo does.
   - `int nd_tex_blk_desc(NdBindDesc* o, const void* timg, unsigned fmt, unsigned siz, int w, int h, const unsigned wrap[4],
     const void* pal, int pal_n);` core 0: tile 0 after gDPLoadTextureBlock(timg, fmt, siz, w, h, 0, cms, cmt, masks, maskt, 0, 0)
     (wrap = cms, cmt, masks, maskt); 0 = not valid. It may wrap nd_tex_win_desc with the full window (ldx 0, ldy 0, ldw w, ldh h,
     uls 0, ult 0, lrs (w - 1) << 2, lrt (h - 1) << 2, stride w for 8b) when the host case shows the walker's result for a block
     load equals that window; else it computes what set_tilesize leaves after the block load. Name the rule in the commit.
2. The epoch rule: after nd_direct_bind and nd_direct_tex the walker state changed, so the helper raises gv_epoch once before the
   vertex load, and the first triangle must not rebind tile 0 from nd_ws.td[0]. Recommended: a flag set by nd_direct_tex and
   cleared by nd_direct_end; gv_epoch_enter skips ensure_tile and dual_bind while it is set (nd_direct_bind already cleared
   dual_on and ran sync_render). Any other rule that keeps ensure_tile from rebinding is allowed. Every other caller of
   gv_epoch_enter must see no change (the flag is 0 outside a direct draw).
3. Host case in 3ds/tools/nd_direct_test.c (end of file; the walker functions are stubs there): stubs of load_mtx, nd_geom_vtx,
   draw_tri, gbi_popmtx that log their arguments; nd_direct_quad(0x1234, v) calls them in the order MTX, VTX, TRI, TRI, POP
   with the exact words of the old ops (compare with the words that gSPMatrix, gSPVertex, gSP1Triangle and gSPPopMatrix write
   into a local Gfx[5] run through the typed recorder macros, low 24 bits for w0 as op_geo passes them). A second case for
   nd_tex_blk_desc against nd_tex_win_desc on 32 x 32 and 24 x 16 IA8.

4. The record and runner for row_S5CHUDB, in src/hud_element.c right after hud_rect_run (near :568), as written here (adjust only
   to the final helper signatures; keep the field list and the order of the fallback ops):
   ```c
   /* S5CHUDB (NATIVEAPI E1): one complex HUD quad of mode 2 (IA8): the matrix stack entry, the 4 vertices, the image and the state. */
   typedef struct NdCHudRec { const void* img; uint32_t mtx; const void* vtx; int32_t w, h; uint32_t flags, prim, pad; } NdCHudRec;
   _Static_assert(sizeof(NdCHudRec) == 32, "NdCHudRec: 4 units");
   static void cplx_quad_run(const uint32_t* u) {
       static const u32 cw_47[2] = HUD_CW(PM_CC_47, PM_CC_47);
       static const unsigned wrap[4] = { G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK };
       const NdCHudRec* r = (const NdCHudRec*)u;
       const int filt = (r->flags & HUD_ELEMENT_FLAG_FILTER_TEX) != 0, aa = (r->flags & HUD_ELEMENT_FLAG_ANTIALIASING) != 0;
       const int xlu = (r->flags & HUD_ELEMENT_FLAG_TRANSPARENT) != 0;
       NdBindDesc d;
       NdPsoN64 pd;
       pd.cw0 = cw_47[0]; pd.cw1 = cw_47[1];
       if (!xlu) pd.oml = aa ? (G_RM_AA_TEX_EDGE | G_RM_AA_TEX_EDGE2) : (G_RM_TEX_EDGE | G_RM_TEX_EDGE2);
       else pd.oml = aa ? ((AA_EN | G_RM_CLD_SURF) | (AA_EN | G_RM_CLD_SURF2)) : (G_RM_CLD_SURF | G_RM_CLD_SURF2);
       pd.omh = G_TT_NONE | (filt ? G_TF_AVERAGE : G_TF_POINT);
       pd.omh_m = (3u << G_MDSFT_TEXTLUT) | (3u << G_MDSFT_TEXTFILT);
       if (nd_tex_blk_desc(&d, r->img, G_IM_FMT_IA, G_IM_SIZ_8b, r->w, r->h, wrap, NULL, 0) && nd_direct_ready() == 1 &&
           nd_direct_bind(nd_pso_n64(&pd), r->prim, nd_direct_env())) {
           nd_direct_tex(&d);
           nd_direct_quad(r->mtx, r->vtx);
           nd_direct_end();
           return;
       }
       nd_direct_fallback(40u);
       {
           static Gfx buf[32];
           Gfx* p = buf;
           gDPSetTextureFilter(p++, filt ? G_TF_AVERAGE : G_TF_POINT);
           gSPMatrix(p++, r->mtx, G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
           gDPSetCombineMode(p++, PM_CC_47, PM_CC_47);
           gDPSetPrimColor(p++, 0, 0, r->prim >> 24, (r->prim >> 16) & 0xff, (r->prim >> 8) & 0xff, r->prim & 0xff);
           if (!xlu) { if (!aa) gDPSetRenderMode(p++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2); else gDPSetRenderMode(p++, G_RM_AA_TEX_EDGE, G_RM_AA_TEX_EDGE2); }
           else { if (!aa) gDPSetRenderMode(p++, G_RM_CLD_SURF, G_RM_CLD_SURF2); else gDPSetRenderMode(p++, AA_EN | G_RM_CLD_SURF, AA_EN | G_RM_CLD_SURF2); }
           gDPLoadTextureBlock(p++, r->img, G_IM_FMT_IA, G_IM_SIZ_8b, r->w, r->h, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
                               G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
           gDPSetTextureLUT(p++, G_TT_NONE);
           gSPVertex(p++, r->vtx, 4, 0);
           gSP1Triangle(p++, 0, 1, 2, 0);
           gSP1Triangle(p++, 0, 2, 3, 0);
           gSPPopMatrix(p++, G_MTX_MODELVIEW);
           nd_walk_state((const uint32_t*)buf, (unsigned)(p - buf));
       }
   }
   ```

## Files and places
3ds/src/nd_direct.c (end), 3ds/include/nd_api.h (end), 3ds/src/nd_geom.c (gv_epoch_enter only), 3ds/tools/nd_direct_test.c
(end), src/hud_element.c (the record and runner after hud_rect_run only; render_complex_hud_element is row S5CHUDB). The
runner has no caller in this row: the compiler may warn of an unused static; mark it `__attribute__((unused))` until S5CHUDB.

## Merge rule (orchestrator, 2026-11-12)
Rows that append to the end of nd_api.h, nd_direct.c or nd_direct_test.c conflict in sequence. Right before the gate:
`git merge main`; at a conflict at those file ends keep both appends (main's first, then yours), rebuild, nd_host; then take the
bases at the new `git merge-base HEAD main`.

## Gate (expected values)
- Build, memcheck OK; `bench.py run nd_host --row S5CHUDA --why "S5CHUDA nd_host"`: PASS with the 2 new cases.
- gv_epoch_enter is shared draw code: `bench.py base spot_mac01_det`, `bench.py base battle_wheel_nowide_det`; one list
  `bench.py run spot_mac01_det,battle_wheel_nowide_det --row S5CHUDA --why "S5CHUDA: direct quad helper, epoch rule"` (2 starts).
  cmp: spot_mac01_det 6/6, battle_wheel_nowide_det 1/1 byte-equal (no caller: nothing may change).
- Budget 8 Azahar starts.

## Stop
A byte difference or a failed host case: stop, commit, hand back (2 tries per named cause). Hand back the final signatures and
the nd_tex_blk_desc rule (row_S5CHUDB.md uses them as written above; a signature change is a stop: hand back). Delete this row
file in the completing commit.
