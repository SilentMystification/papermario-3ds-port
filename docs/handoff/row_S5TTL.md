# Row S5TTL: title screen images through the NdTRect record (S5 call sites)

(Model: Haiku. The pattern edit of row_S5TIMG.md, applied to one file. Blocker: S5TIMG merged: its step 0 adds `pad` bit 0 =
keep the walker's prim; set `r.pad = 1` at a site where the function sets no PrimColor before the pass.)

## Start
`sh 3ds/tools/wt_new.sh S5TTL`; PM_ROW=S5TTL PM_LANE=S5TTL. Only src/state_title_screen.c is edited. Start after S5TIMG merges.

## Sites (main ccdb7e3f5; re-grep `gDPLoadTexture\|gSPTextureRectangle\|gSPScisTextureRectangle` first, the lines may move)
| function | load | rect |
|---|---|---|
| the logo loop (:504-520) | :510 gDPLoadTextureTile RGBA 32b | :518 gSPScisTextureRectangle |
| title_screen_draw_menu | :619 gDPLoadTextureBlock IA 8b | :623 gSPTextureRectangle |
| title_screen_draw_menu | :629 gDPLoadTextureBlock IA 8b | :633 gSPTextureRectangle |
| title_screen_draw_menu | :639 gDPLoadTextureBlock IA 8b | :642 gSPTextureRectangle |
| title_screen_draw_copyright | :684 gDPLoadTextureTile_4b CI (after LoadTLUT_pal16 :683) | :688 gSPTextureRectangle |
| title_screen_draw_copyright loop | :695 gDPLoadTextureTile IA 8b | :699 gSPTextureRectangle |

## Exact edit per site (under `#ifdef TARGET_3DS`; `#else` keeps the old lines). Same rules as row_S5TIMG.md step 2.
- Keep every state op: the TitleSetupGfx calls (:504, :613, :667), the combine, TLUT and render mode lines, and the LoadTLUT.
   The S5TRECT fallback (nd_trect_run, nd_direct_ready() != 1) replays only the texture load and the rectangle: it relies on the
   state ops and the LoadTLUT kept in the stream. Remove none of them; the record carries the palette only for the direct path.
- Load and rect pairs become one NdTRect record:
  - gDPLoadTextureTile(img, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, ...): copy the arguments.
  - gDPLoadTextureBlock(img, fmt, siz, width, height, pal, cms, cmt, masks, maskt, ...) loads the whole image: uls 0, ult 0,
    lrs width - 1, lrt height - 1.
  - The _4b form: siz G_IM_SIZ_4b.
- Rect words:
  - gSPTextureRectangle: ND_W0_TR, ND_W1_TR, ND_TR_H1, ND_TR_H2 with the call's expressions.
  - gSPScisTextureRectangle: copy the word arithmetic of hud_rect_direct in src/hud_element.c (the PM_SCIS_* form of w0, w1
    and hs), with this call's arguments.
- pso:
  - cw0 and cw1: the combine the function set before the pair. Where it comes from TitleSetupGfx, read that list for its
    combine and render mode and use those values.
  - oml: the render mode pair.
  - omh: TLUT (G_TT_RGBA16 for the copyright CI image, else G_TT_NONE) plus filter (from TitleSetupGfx).
  - prim: the prim set before the pair, else nd_direct_prim() through the runner.


## Merge rule (orchestrator, 2026-11-12)
Rows that append to the end of 3ds/include/nd_api.h, 3ds/src/nd_direct.c or 3ds/tools/nd_direct_test.c conflict when two merge in
sequence. Right before the gate runs: `git merge main`; at a conflict at those file ends keep both appends (main's first, then
yours), rebuild, nd_host; then take the bases at the new `git merge-base HEAD main` and run the gate.

## Copyright US loop (ORACLE6 check of try 1, c657597b3): correct as written
The loop's `alpha = 0` writes only the C local (no gfx), so the state at each rectangle is the pre-loop block: PM_CC_02 and prim
alpha `alpha` (clamped at 0) when `alpha < 255`, else TitleSetupGfx's G_CC_DECALRGBA (state_title_screen.c:95) with the walker prim
(pad 1). ttlSet/ttlPrim taken after the block keep exactly that. No change.

## Gate (ORACLE6 correction: try 1 found no title det scene and a window/until mismatch; budget reset once)
- Build, memcheck OK; nd_host PASS (done in try 1 at c657597b3; repeat only after a new code edit).
- The title is game mode 2 (mode 1 is the logos). No det title scene exists. Add scenario and bench `title_det` (one commit, then
  `bench.py rebuild`), inserted in 3ds/tools/azt_scenarios.json right after the `intro_native` entry and in bench_defs.json right
  after the `newgame_handover` entry (not at the end; S5MENU0 inserts after fsel_fncov_pick, another place):
  - scenario: name `title_det`, status unverified, subject "Title screen (hold state) with det: fast intro, START at frame 60
    skips to the title (game mode 2, about frame 84 per the INTROSNAP discovery run), shots 200 to 203.", proves "S5TTL byte gate:
    the title logo, menu and copyright rectangles.", debug `fast padscript=wait60,START*4 det fixedseed fs1off loglevel=debug
    shots shotwin=200:4`, until `shot \d+ frame 203\b`, cap 30, deadline 25, first_marker `game mode 1 frame`, expects none,
    needs_picture true, expected_events: `game mode 2 frame` (name "title") and `shot \d+ frame 203\b` (name "last shot").
  - bench: name `title_det`, kind azahar_pic, speed normal, default false, scenario title_det, cap 30, the same until, tags
    azahar, pic, det, gates []. Purpose "S5TTL byte gate: title screen hold state, det, shots 200 to 203."
  - Why 200: the hold state lasts TITLE_START_TIME 400 frames (state_title_screen.c:32) from the title start, so frames 200-203
    are inside it; the window has 4 shots, so the until frame is 203 (shotwin F:4 gives F to F+3).
- Try 3 fix (ORACLE6, base run of 346ed226c: 4 of 4 shots, no until in 30 s). Cause: the committed JSON has the until and the
  "last shot" regex as the JSON text "shot \\d+ frame 203\b" (one backslash before b). In a JSON string \b is the backspace
  escape, so the regex ends in a
  backspace character instead of a word boundary and never matches the line `shot 3 frame 203` (pm_gpu.c:1987, written at :2014
  by pm_log_evt). The run output shows it: `until /shot \d+ frame 203/` (no \b). intro_native's JSON text is
  "shot \\d+ frame 334\\b". Edit (nothing else): in azt_scenarios.json (title_det until and the "last shot" regex) and in
  bench_defs.json (title_det until) the JSON text must be "shot \\d+ frame 203\\b" (two backslashes before d and two before b).
  Delete the uncommitted "reason": "broken" entry the tool wrote, keep status "unverified", commit both files. Expected in each
  base run and in the head run: `shot 0 frame 200` to `shot 3 frame 203`, then the until hit and the end within about 1 s.
- `bench.py base title_det` (2 runs of the merge-base build; they must agree), then one run `bench.py run title_det --row S5TTL
  --why "S5TTL: title rects as NdTRect"` (1 start), `bench.py cmp title_det --row S5TTL`: 4/4 byte-equal. No `trect fallback`
  lines. A log with `game mode 14` (file select) before frame 203 means START landed on the title: stop and hand back the
  `game mode` lines (scene fault, not a code fault). Budget 4 (3 used by this plan).

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit.
