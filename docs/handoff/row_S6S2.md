# Row S6S2: render_messages' state prologue as an S6T compiled range

(Model: Haiku. The cam_main pattern of S6T: a def with value placeholders, the values made into a local packet per call. One block
changes.)

## Pattern (S6T merged b16f91c46)
See src/cam_main.c:115-143 (def with value placeholders such as `gsSPViewport(0)` and `gsSPMatrix(0, ...)`) and :255-258 (the
values made with the g macros into `Gfx scv[n]`, then `nd_scomp_rec(&cam_range, scv, n)`).

## Code read
src/msg.c render_messages (:584-), inside the printer loop, the first printer with MSG_STATE_FLAG_2 runs (:595-602):
`gSPViewport(&D_8014C280)`; guOrtho into `matrix`; `gSPMatrix(OS_K0_TO_PHYSICAL(matrix), NOPUSH | LOAD | PROJECTION)`;
`gDPPipeSync`; `gDPSetCycleType(G_CYC_1CYCLE)`; `gSPClearGeometryMode(G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN |
G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH)`; `gSPSetGeometryMode(G_SHADE | G_SHADING_SMOOTH)`; break.

## Start
`sh 3ds/tools/wt_new.sh S6S2`; PM_ROW=S6S2 PM_LANE=S6S2. Only render_messages in src/msg.c is edited (S3MSG edits other
functions of msg.c later).

## Exact edits (src/msg.c)
1. Above render_messages, under `#ifdef TARGET_3DS`: `#include "nd_scomp.h"` and
   ```
   static const Gfx msg_range_u[] = {
       gsSPViewport(0),
       gsSPMatrix(0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION),
       gsDPPipeSync(),
       gsDPSetCycleType(G_CYC_1CYCLE),
       gsSPClearGeometryMode(G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH),
       gsSPSetGeometryMode(G_SHADE | G_SHADING_SMOOTH),
   };
   static NdSDef msg_range = { msg_range_u, sizeof msg_range_u / sizeof msg_range_u[0] };
   ```
2. In the loop, under `#ifdef TARGET_3DS`, replace the 7 gbi lines (keep guOrtho before them):
   `{ Gfx scv[2]; gSPViewport(&scv[0], &D_8014C280); gSPMatrix(&scv[1], OS_K0_TO_PHYSICAL(matrix), G_MTX_NOPUSH | G_MTX_LOAD |
   G_MTX_PROJECTION); nd_scomp_rec(&msg_range, scv, 2); }`. `#else` keeps the old lines.

## Gate (try 2, oracle 4: the talk scene had no det)
- Done (code 7eff04aa2): build, memcheck, nd_host PASS, pause_tab_det_pic 101/101 byte-equal vs 95aec5aee, s6chk_mac01 f200
  `bad 0`. The pause scene does not draw this prologue: it runs only inside the printer loop of render_messages for an open
  message printer (msg.c:595-602); the pause menu draws its text with draw_msg, not printers. So the talk gate is needed.
- Step 1: `git merge main` (NAPI3B is merged and added the det talk scene msgbox_talk_det: `preset=yard nowide uishow=talk
  shots shotwin=330:1 loglevel=debug det`, until frame 330; it edits msg_draw.c, not msg.c, so no conflict). Do not edit any
  scenario. The base commit is then `git merge-base HEAD main` (the main you merged): use it as `--commit <that hash>`.
- `PM_ROW=S6S2 PM_LANE=S6S2 python 3ds/tools/bench.py base msgbox_talk_det --commit <merge-base>` (stored already, or 2 runs;
  expected 1 shot). Rebuild after the merge; nd_host PASS.
- `python 3ds/tools/bench.py run msgbox_talk_det --row S6S2 --why "S6S2 talk gate"` (1 start); `bench.py cmp msgbox_talk_det
  --row S6S2 --commit <merge-base>`: byte-equal. At most 3 starts (3 of 5 left).

## Stop
A byte difference or a scomp error line: stop, commit, hand back. Delete this row file in the completing commit.
