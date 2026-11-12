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
   file end; one commit. Debug: `warp=mac_01:0 pos=0:0:200 shots shotwin=49:112:4 loglevel=debug det` (no prof switch: prof mode
   sets `log=off`, which is why try 1's spot_mac01_det run printed no `tsten` and no `warp:` line). Until: `shot \d+ frame 157\b`
   (the last of 28 shots: frames 49, 53, ..., 157). Try 4 (ORACLE6): the window `1:160:4` shot frames 1 to 157, and the two base
   runs at 111132143 differed on shot 8 = frame 33. Frame 33 is inside GAME_MODE_ENTER_WORLD (mode 7, frames 15 to 48: the base
   log has `game mode 7 frame 15`, the map load lines `load map`, `spawn npcs` and `seamfix` between shots 5 and 9, then `game
   mode 4 frame 49`). What a load frame shows depends on the load progress (ROM stream reads on another thread), which det does
   not fix; shots 9 to 39 (frames 37 to 157) were equal in both runs. So the window starts at frame 49, the first WORLD frame (the
   fade-in runs after the map load). Fix of the committed entry: change only the shotwin value (scenario file only, one
   commit). Cap 30, deadline 25. expected_events: only `warp: mac_01 entry 0` (try 3, oracle 5: the base build has no
   tsten line, so an expected tsten event made every base fail; the tsten count is the log grep of the gate below). Fix of the
   committed entry c107a5556: delete the tsten object from its expected_events list, commit (scenario file only). Bench: kind
   azahar_pic, scenario sten_enter_det, cap 30, the same until, tags azahar, pic. Purpose: "S1TSTEN byte gate: the Mario stencil
   fade-in of the world entry". Why the window: a warp boot enters GAME_MODE_ENTER_WORLD (3ds/src/pm_warp.c:1149), so
   state_init_enter_world sets TRANSITION_ENTER_WORLD (src/state_map_transitions.c:28) and the fade-in draws
   OVERLAY_VIEWPORT_MARIO (screen_overlays.c:722) through render_screen_overlay_frontUI (main_loop.c:285) after the map load,
   which is before frame 120 (the map load line comes before `audiohash f120` in try 1's log).

## Try 5 (ORACLE7, after try 4 at 1a959cf65: sten_enter_det 28/28 equal, spot_mac01_det 0/6; row spec fault, keep Haiku)
Code read of the branch diff (git diff main...S1TSTEN) and the try 4 records:
- R1, gate fault: the stencil draws only at frames 35 to 48 (head sten_enter_det record: `tsten f35` .. `tsten f48`, 14 entries,
  then `game mode 4 frame 49`; the fade ends in state_step_enter_world, src/state_map_transitions.c:91-97, OVERLAY_NONE and
  GAME_MODE_WORLD). The window 49:112:4 holds no stencil frame, so 28/28 proves nothing about this row.
- R2, code fault: screen_overlays.c (branch) pm_tsten_run, the test `if (nd_tex_win_desc(...) && nd_direct_ready())`.
  nd_direct_ready (3ds/src/nd_direct.c:83-88) returns -1 to -4 when the direct draw is not allowed (not the main colour image,
  cimg_zero, a non-frame-buffer cimg, a valid m1 shaded raster, an aux off-screen tile 1). Those values are true in C, so the
  direct draw runs where the old ops must run. Every other caller tests `== 1` (nd_direct.c:201, :272, :304).
- spot_mac01_det 0/6: the 6 head shots are byte-identical to each other (one md5 for 0000..0005); the 6 base shots all differ
  from each other. The head run's prof values equal the base's (audiohash f120 to f480, `dr 26`, `tri 163`, the ndc and cls
  counts), so the game state and the recorded draws of frames 88 to 388 are the base's. No code of this diff runs after f48
  (no stencil overlay in GAME_MODE_WORLD). The read finds no path from the f35-48 direct draws to a whole top target that stops
  changing; R2 is the only path that draws under a state the old ops handle differently. The run shared slot 1 Azahar pid 33892
  with S7EFX2, S7EFX4 and S7ENT runs by warm restart (pool.log 14:12-14:17; the run before it was S7EFX4's at 14:15:12 with
  `warm resend no BootGame`). The code read says no more: the gate below decides.
Edits (one commit, then the gate):
1. screen_overlays.c pm_tsten_run: `nd_direct_ready()` becomes `nd_direct_ready() == 1`. Nothing else in the function.
2. azt_scenarios.json sten_enter_det: debug `warp=mac_01:0 pos=0:0:200 shots shotwin=37:12:1 loglevel=debug det`, until
   `shot \d+ frame 48\b`, subject and reason "12 shots at frames 37 to 48: the Mario stencil fade-in (tsten f35..f48)". Frames 37+
   were equal in both try 4 base runs (frame 33 differed: map load). bench_defs.json sten_enter_det: the same until and purpose.
   Frames 35 and 36 stay out (no base proof).

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- Base: `bench.py base sten_enter_det` (new scenario hash: 2 runs, expected stored 12 shots); spot_mac01_det is stored at the
  merge-base f8b5d03fb (after a merge of main: `bench.py base spot_mac01_det`).
- One list: `bench.py run sten_enter_det,spot_mac01_det --row S1TSTEN --why "B2a try 5: ready == 1, stencil window"` (2 starts).
  Expected: cmp sten_enter_det 12/12 and spot_mac01_det 6/6 byte-equal; the head sten run prints 14 entries `tsten f35 .. f48
  id 0 direct 1` (a `direct 0` entry now means nd_direct_ready said no: hand back its frames).
- Budget: reset once by the orchestrator for this row correction; 4 starts planned (2 base, 2 list).
- Merge note for the orchestrator: the branch runs.jsonl holds about 141000 added lines (pre-TOOLDEDUP copies): append only
  the S1TSTEN run lines (fact 4 of the oracle handoff).

## Stop
- sten_enter_det differs: stop, commit, hand back the cmp output and the tsten entries (R2 fixed, so a difference is in the
  direct draw itself: the oracle reads the bind desc next).
- spot_mac01_det differs again: stop, commit, hand back the cmp output, the md5 of each head shot (`md5sum` of the 6 files) and
  the pool.log entries of this run's slot from 2 minutes before it. Six identical head shots twice from one build is a code
  fact for the oracle; one identical set and one passing run is a tool fault (CLAUDE.md 4): the orchestrator opens a tool row.
- 2 tries per named cause. Delete this row file in the completing commit.
