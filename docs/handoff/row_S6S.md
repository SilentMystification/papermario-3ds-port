# Row S6S: pure state ranges through the S6T compiler (cam_main, status_icons rest, gfx_init_state, render_messages prologue, setup lists)

Model: Sonnet (user decides).
- Why not Haiku: each range has per-frame values mixed into it (scissor, viewport, perspNorm, matrices). The coder splits each
  range into the compiled part and the value part by reading which op depends on the frame.
- Why not Opus: the compiler form is S6T's.

## Blocker and base
S6T merged. First step: read S6T's design note (row_S6T.md in its merged commit) and 3ds/include/nd_scomp.h. Use that API only.
`sh 3ds/tools/wt_new.sh S6S`; PM_ROW=S6S PM_LANE=S6S.

## Ranges (S6T did cam_main render_frame :155-208 and the status_icons prologue as its first use)
- src/cam_main.c: the rest of render_frame (:252, :272-275: the second-pass matrix, the colour image back).
- src/background_gfx.c gfx_init_state.
- src/msg.c render_messages prologue (6 ops).
- The static setup lists called with gSPDisplayList from these producers: D_80293970 (src/battle/190B20.c:44; S5PQUAD keeps it
  as ops), D_8014C620 (item entity setup; S5ITEM), DefaultImgFXSetupGfx (S5IMGFX), Gfx_LoadStencilTex_CommonParams (S1TSTEN keeps
  it as ops).
- Pre-flight: grep `gSPDisplayList(gMainGfxPos++, ` over src/*.c. List every static list that holds only state ops: no G_VTX, no
  triangles, no rectangles.

## Design
Each listed range or list becomes an S6T range id: a static list is compiled once at first use, keyed by its address and the
inherited signature, and the call site records the id instead of the gSPDisplayList. Value ops stay as value units.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- DIRECT byte set, one list (3 starts), plus battle_wheel_nowide_det (1 start). Every shot byte-equal. s6chk 0 mismatches on
  spot_mac01_det.
- napicen_mac01 and napicen_btl: cam_main.o and background_gfx.o ops down to value ops.
- listcen: build and data down. Budget 8.

## Stop
A byte difference or an s6chk mismatch: stop, commit, hand back. Delete this row file in the completing commit.
