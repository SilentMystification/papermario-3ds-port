# Row S1TSTEN: the screen transition stencil rectangle drawn directly (NATIVEAPI B2a, split from S1STEN)

(Model: Haiku, long row. It copies the NAPI2B HUD pattern: a pso, an NdBindDesc from nd_tex_win_desc, the rectangle words,
nd_direct_rect, nd_direct_end, and a fallback to the old ops. Every value is below.
- Why not Sonnet: the pattern is merged and the texture windows are fixed data.
- Split: the darkness stencil (off-screen pass into the z image) is not in this row. It needs off-screen targets in the direct
  path, which belongs to the S6T design.)

## Blocker and base
NAPI2B and S1FILL merged. `sh 3ds/tools/wt_new.sh S1TSTEN`; PM_ROW=S1TSTEN PM_LANE=S1TSTEN. Base: `git merge-base HEAD main`.

## What it draws (code read)
- _render_transition_stencil (src/screen_overlays.c:253-275), for the cases VIEWPORT/SCREEN MARIO, STAR and SPOTLIGHT, first runs
  a load list as walker ops: Gfx_LoadStencilTex_Mario :57-62, _Star :64-70, _SharpCircle :49-54, each through _CommonParams
  :30-45. CommonParams sets cycle 1, filter AVERAGE, combine PM_CC_3A, render mode CLD_SURF pair and clears geometry.
- It then calls appendGfx_screen_transition_stencil (src/screen_render_util.c:258-301): a scissor, the combine
  PM_CC_PRIM_FULL_ALPHA only when progress == 255, the prim colour and one gSPTextureRectangle, then a PipeSync.
- TRANSITION_ENTER_WORLD uses OVERLAY_VIEWPORT_MARIO (screen_overlays.c:578-581, set at state_map_transitions.c:28), so the
  Mario stencil fades in at every world entry, including a warp boot.

## Touch only
- src/screen_overlays.c: the six stencil cases :253-275 (one line each, before the call); a new global, a new table and a new
  runner, all placed right above _render_transition_stencil. S1FILL's fill_run lines and S5PQUAD's call sites are other lines:
  S5PQUAD may run at the same time.
- src/screen_render_util.c: appendGfx_screen_transition_stencil only. S1PREV edits appendGfx_draw_prev_frame_buffer in the same
  file (another function), so S1PREV may run at the same time.
- 3ds/tools/azt_scenarios.json and bench_defs.json: one new entry each, after `spot_mac01_det`.

## Exact edits
1. screen_overlays.c, under `#ifdef TARGET_3DS`, above _render_transition_stencil:
   - `u8 gPmStencilId;` (0 Mario, 1 Star, 2 SharpCircle).
   - `typedef struct { u32 w0, w1, hs, hd, prim, full; } NdTStenRec;` (24 bytes: 3 units).
   - `void pm_tsten_run(const uint32_t* u)`, built like hud_rect_direct plus hud_rect_run (src/hud_element.c after NAPI2B):
     a. The texture window: `NdBindDesc d; unsigned wrap[4];` from this table (I 4-bit images, the HUD fmt-1 arithmetic with
        sh = 1):
        | id | image | stride | wrap cms, cmt, masks, maskt | ldx, ldy, ldw, ldh | uls, ult, lrs, lrt (10.2) |
        |----|-------|--------|-----------------------------|--------------------|---------------------------|
        | 0 | ui_stencil_mario_png | 32 | 2, 2, 6, 6 | 0, 0, 32, 64 | 0, 0, 252, 252 |
        | 1 | ui_stencil_star_png | 16 | 3, 2, 5, 6 | 0, 0, 16, 64 | 0, 0, 0xFC, 0xFC |
        | 2 | ui_stencil_sharp_circle_png | 16 | 3, 3, 5, 5 | 0, 0, 16, 32 | 0, 0, 0xFC, 0xFC |
        Call `nd_tex_win_desc(&d, img, stride, G_IM_FMT_I, G_IM_SIZ_4b, wrap, ldx, ldy, ldw, ldh, uls, ult, lrs, lrt, NULL, 0)`.
        Where the table values come from: the load lists, the HUD formula `ldw = (((lrs << 1) - (uls << 1)) >> 2) + 1`, and the
        SetTileSize of the Star and SharpCircle lists. A row that does not match the list on reading: stop and hand back.
     b. The pso: `NdPsoN64 pd`, with cw0/cw1 from HUD_CW-style macros. Use PM_CC_PRIM_FULL_ALPHA when `full`, else PM_CC_3A, and
        build the words as hud_element.c does (copy its HUD_CW_/HUD_CW macros into this file under another name, e.g. TST_CW).
        Then `pd.oml = G_RM_CLD_SURF | G_RM_CLD_SURF2`, `pd.omh = G_TT_NONE | G_TF_AVERAGE` and
        `pd.omh_m = (3u << G_MDSFT_TEXTLUT) | (3u << G_MDSFT_TEXTFILT)`.
     c. Direct path: `if (nd_tex_win_desc(...) && nd_direct_ready()) { NdPso id = nd_pso_n64(&pd); if (nd_direct_bind(id,
        r->prim, nd_direct_env())) { nd_direct_tex(&d); nd_direct_rect(r->w0, r->w1, r->hs, r->hd); nd_direct_end(); return; } }`.
     d. Fallback (as hud_rect_run): write the old ops into `static Gfx buf[8]` with the gbi macros. That is the combine when
        `full`, then gDPSetPrimColor from prim, then the rectangle as `ND_GX(p++, r->w0, r->w1)` plus the two
        `gImmp1(p++, G_RDPHALF_1, r->hs)` and `gImmp1(p++, G_RDPHALF_2, r->hd)` words (nd_gbi.h:206-209 form). Then
        `nd_walk_state((const uint32_t*)buf, (unsigned)(p - buf));`.
     e. Debug count, for the gate: `static unsigned n; if (n++ < 40) pm_log_d("tsten f%u id %u direct %d", pm_frame_count(),
        gPmStencilId, ok);`, where ok is 1 on the direct path and 0 on the fallback. Declare pm_log_d and pm_frame_count extern.
   - In each of the six cases, before the appendGfx call, under `#ifdef TARGET_3DS`, set `gPmStencilId = 0` (Mario), `1` (Star)
     or `2` (Spotlight).
   - gPmStencilId is read on core 1 when the record is built, never on core 0: the record carries it. Add the field: the record
     becomes `{ w0, w1, hs, hd, prim, full_and_id }` with `full_and_id = full | id << 1`, still 24 bytes.
2. screen_render_util.c appendGfx_screen_transition_stencil, under `#ifdef TARGET_3DS`, after the `progress == 0` return:
   - keep the gDPSetScissor op;
   - replace the combine line, the prim colour line and the gSPTextureRectangle with one record:
     `w0 = ND_W0_TR(G_TEXRECT, rectRight * 4, y2 * 4)` and `w1 = ND_W1_TR(G_TX_RENDERTILE, rectLeft * 4, y1 * 4)`;
     hs and hd with ND_TR_H1 and ND_TR_H2 from the same float expressions as :297-299 (copy them as written: the macro casts
     the same way, so the words are equal);
     `prim = (u32)(u8)primR << 24 | (u32)(u8)primG << 16 | (u32)(u8)primB << 8 | (u8)primA`; full = progress == 255.0f;
     id = gPmStencilId (`extern u8 gPmStencilId;`);
     `nd_rec_napi(pm_tsten_run, &rec, 3)` with `extern void pm_tsten_run(const uint32_t*);`;
   - keep the trailing gDPPipeSync. The `#else` keeps the old lines.
   Include "nd_api.h" and, for the ND_* macros, the header that gives them (nd_gbi.h is already in the 3DS gbi chain: check with
   a build).
3. Scenario and bench `sten_enter_det` (try 2, oracle 4: no probe run). Insert both right after spot_mac01_det, never at the
   file end; one commit. Debug: `warp=mac_01:0 pos=0:0:200 shots shotwin=1:160:4 loglevel=debug det` (no prof switch: prof mode
   sets `log=off`, which is why try 1's spot_mac01_det run printed no `tsten` and no `warp:` line). Until: `shot \d+ frame 157\b`
   (shot 39 of 40). Cap 30, deadline 25. expected_events: only `warp: mac_01 entry 0` (try 3, oracle 5: the base build has no
   tsten line, so an expected tsten event made every base fail; the tsten count is the log grep of the gate below). Fix of the
   committed entry c107a5556: delete the tsten object from its expected_events list, commit (scenario file only). Bench: kind
   azahar_pic, scenario sten_enter_det, cap 30, the same until, tags azahar, pic. Purpose: "S1TSTEN byte gate: the Mario stencil
   fade-in of the world entry". Why the window: a warp boot enters GAME_MODE_ENTER_WORLD (3ds/src/pm_warp.c:1149), so
   state_init_enter_world sets TRANSITION_ENTER_WORLD (src/state_map_transitions.c:28) and the fade-in draws
   OVERLAY_VIEWPORT_MARIO (screen_overlays.c:722) through render_screen_overlay_frontUI (main_loop.c:285) after the map load,
   which is before frame 120 (the map load line comes before `audiohash f120` in try 1's log).

## Gate (expected values)
- Build, memcheck OK; nd_host PASS (done in try 1: commit 113879c16).
- `PM_ROW=S1TSTEN PM_LANE=S1TSTEN python 3ds/tools/bench.py base sten_enter_det` and the same for spot_mac01_det (merge-base:
  `git merge-base HEAD main`; the base runs your scenario file). Expected: stored, 40 and 6 shots.
- One list: `bench.py run sten_enter_det,spot_mac01_det --row S1TSTEN --why "B2a: transition stencil direct"` (2 starts).
  sten_enter_det log: `tsten f<n> id 0 direct 1` lines (at least 1; hand back the first and last frame), no `direct 0`. `bench.py cmp` both: every shot byte-equal.
- Census: `grep -c "tsten f[0-9]* id 0 direct 1"` on the head sten_enter_det log only (the base prints none): at least 1.
- Budget 6 Azahar starts, reset once for the try 3 row fault (a base of a new bench takes 2 runs).

## Stop
A byte difference or a `direct 0` line: stop, commit, hand back the cmp output and the tsten lines (2 tries per named cause).
Delete this row file in the completing commit.
