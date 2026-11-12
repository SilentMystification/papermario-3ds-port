# Row S3MSG: msg.c draws (digits, numbers, rewind arrow, choice pointer, message prologue) as direct records (NATIVEAPI S3/S5)

Model: Sonnet (user decides).
- Why not Haiku: draw_number and draw_digit bind charset images through the glyph and cell arm that NAPI3B builds. The arrow and
  pointer are ROTSCALE-free texture rectangles with their own palette loads. The order of the state the message prologue leaves
  must stay as the walker had it for msg_draw's glyph records.
- Why not Opus: the NAPI2B and NAPI3B helpers state every draw here.

## Blocker and base
NAPI3B merged (glyph atlas bind in the direct path). `sh 3ds/tools/wt_new.sh S3MSG`; PM_ROW=S3MSG PM_LANE=S3MSG.

## Code read (src/msg.c; gbi calls per function)
draw_number 13 (:2298), msg_update_rewind_arrow 8, render_messages 6 (:584, the per-frame prologue), msg_draw_choice_pointer 6
(:2232), draw_digit 2 (:2276). napicen3 talk: msg.o 46 ops a frame.

## Design
1. Every texture rectangle in these functions becomes the HUD pattern (hud_rect_direct in src/hud_element.c):
   - a pso from nd_pso_n64 of its combine and render words;
   - an NdBindDesc from nd_tex_win_desc of its load (charset digits through NAPI3B's glyph bind arm);
   - the rectangle words through the ND_W0_TR / ND_W1_TR / ND_TR_H1 / ND_TR_H2 macros (nd_gbi.h:201-204);
   - nd_direct_rect, then nd_direct_end.
   Keep the fallback (the old ops into a local list and nd_walk_state) for walker states nd_direct_ready refuses, counted.
2. render_messages' prologue (6 state ops) stays as ops in this row; S6S compiles it.
3. Records are made on core 1 in the functions themselves (nd_rec_napi), as hud_element.c does.

## Files and places
src/msg.c (the 5 functions). No new declarations in nd_api.h. If a helper is needed, add it to the end of nd_direct.c and
nd_api.h only when no other running row appends there (orchestrator check).

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- One list, with bases at the merge-base: msgbox_talk_nowide_pic, or the det talk scene napicen_talk uses (pre-flight
  `bench.py show`; name it in the commit), plus pause_tab_det_pic (draw_number in the pause stats). 2 starts. Every shot
  byte-equal.
- napicen_talk (1 start): msg.o down to the render_messages prologue (hand back the per-function numbers); fallback count 0 in
  steady frames.
- listcen (host): build count down by the calls moved. Budget 6 Azahar starts.

## Stop
A byte difference: stop, commit, hand back (2 tries per named cause). Delete this row file in the completing commit.
