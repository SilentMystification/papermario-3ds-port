# TXTFLICK: right side of the text box moves (diagnosis)

Row TXTFLICK. Diagnosis only. No source edit. Branch uc2c, HEAD eebf38d57.

## Symptom

- First report: the right side of the dialog text box flickers.
- Update from the user: the right side moves (jitters). It is not only a colour change.
- Console build of the user: 45fca7dd0.

## Result in one line

No box motion in the pause dialog of the one Azahar run. The likeliest right-side mover in the code is the next-page star and blink arrow at the right corner of a waiting box. This is INFERRED. Not measured.

## Azahar run (1 of 3 diagnostic starts)

- Scenario: `text_menu` (pause status page, dialog "Check here to see what Mario's current status is."), row TXTFLICK, build pm_3ds.3dsx (no rebuild). Three shots: `build3ds\azt\shots\text_menu\0000_1_1.ppm` (frame 170), `0001_1_1.ppm`, `0002_1_1.ppm` (frames 177 to 180).
- Log checks 3 of 3 pass. The picture read by the agent.
- Shot format: P6, 240 wide by 400 high. The game top screen is stored rotated. The text runs down the image. INFERRED: game right edge is at stored rows about 290 to 330, game left column at stored x about 25 to 55.
- Pixel diff, pair 0000 to 0001 and pair 0001 to 0002:
  - dialog box, stored x 20 to 60, y 60 to 340: 0 and 0 changed pixels.
  - dialog right end, stored x 20 to 60, y 290 to 340: 0 and 0.
  - stored right 16 columns (x 224 to 239, all rows): 0 and 0. This strip is the game top edge, not the dialog right side. The request for "right 16 columns" was the wrong edge for this view.
  - Changed pixels elsewhere: 96 (x 90 to 101, y 191 to 201) and 287 (x 90 to 213, y 147 to 202). These are sprites and menu icons, not the box.
- CONFIRMED: this box does not move in two consecutive shots of one run.
- The pause dialog is not in the waiting state (no next-page star is drawn in the shot). This run cannot show the star.

## Code map (draw path of a text box)

| Element | Where | Status |
|---|---|---|
| Speech bubble body (left, mid, right pieces) | src/msg_draw.c:1853 (msg_draw_speech_bubble), right piece quad src/msg_draw.c:69 and 1888 to 1891, right texture load 1951 | CONFIRMED static at scale 1.0: integer position `bubbleX = posX + windowBasePos.x` (src/msg_draw.c:587 to 589), integer vertex x. Right piece texture is CLAMP, 32 texels over 31 px (src/msg_draw.c:69 to 74). |
| Speech bubble open and close (scale lerp per draw call) | src/msg_draw.c:517 to 548 | CONFIRMED: right edge moves only while fadeInCounter or fadeOutCounter runs. Not in steady state. |
| Message window pop (rotate and scale, table D_8014C580, rotZ) | src/msg_draw.c:133 to 170 | CONFIRMED: moves each draw while in state D or E. Whole box, not right only. |
| Frame quads with right corners (rounded frame, INSPECT and CHOICE styles) | src/msg_draw.c:2153 to 2216 (quads 4, 8, 15, 3, 6, 10, 14) | CONFIRMED integer. Static. |
| Text clip at right | src/msg_draw.c:653 (INSPECT clipX[1] = box x + width - 8), src/msg_draw.c:568 (normal box clipX[1] = SCREEN_WIDTH - 20) | CONFIRMED static. |
| Next-page star: swings +-30 degrees, phase += 15 per draw call, period 24 draw calls | src/msg.c:2171 to 2172 (phase), 2171 (angle = cosine(phase) * 30), matrix 2183 to 2189 (guMtxF2L) | CONFIRMED in code. Drawn in state WAITING only (src/msg.c:612 to 615). Position at box right: src/msg_draw.c:844 to 845 (x = box x + width - 10). |
| Next-page blink arrow: on 6 draw calls, off 6, period 12 draw calls | src/msg.c:2198 to 2210 (msg_draw_rewind_arrow), drawn in state C (src/msg.c:617) | CONFIRMED in code. Not a 2 s blink. At 24 to 28 fps console, period about 0.43 to 0.5 s. |
| Draw calls that advance the star and the blink | src/main_loop.c:241, 257, 269 (render_messages per draw build) | CONFIRMED that the counters run per draw build, not per game step. |
| Port: matrices recorded by guMtxF2L when fps60 on | 3ds/src/pm_gu.c:39 | CONFIRMED. |
| Port: t = 0.5 pass of the swap task (matrix mix of this and the last draw) | 3ds/src/pm_nusys.c:111 and 113; pm_interp_lerp 3ds/src/pm_interp.c:126 to 160 | CONFIRMED code. Effect on star: INFERRED. |
| Port: step start opens the record | 3ds/src/pm_phase.c:671 | CONFIRMED. |
| Port: replaced matrix read in load_mtx | 3ds/src/pm_gbi.c:1807 | CONFIRMED. |

## Named cause (INFERRED)

The next-page star at the right corner of a waiting box moves on each draw call. Its angle is a cosine of a counter that advances once per draw build (src/msg.c:2171 to 2172). The port can mix each star matrix with the last draw (3ds/src/pm_nusys.c:111, pm_interp.c:126). When the mix and the plain draw alternate (pm_interp_skip path, pm_nusys.c:110, or a leaf pairing miss in pm_interp.c:140 to 146), the star angle alternates between a half step and a full step. That reads as a right-corner jitter. The box edge itself does not move in steady state.

Why this is the best fit:
- The star and blink sit only at the right of the box (src/msg_draw.c:844, src/msg.c:2202).
- The whole box is static in steady state (src/msg_draw.c:587 to 589), so a box-wide cause does not fit a right-only symptom.

Points that do not fit:
- The "2 s blink" hypothesis: the code has 12 draw calls for the blink and 24 for the swing. Not 2 s. No 2 s constant found (grep of msg.c and msg_draw.c).
- Stereo second pass and wide viewport crop (commits 6c59757fe and 6de4e162b): NOT in console build 45fca7dd0 (git merge-base --is-ancestor). They are on uc2c HEAD only. They cannot be the console cause.
- Texture past the edge (right speech piece, CLAMP, 32 texels over 31 px): static, so it gives a fixed edge stripe, not motion. Not a jitter cause.
- Speech bubble right corner (msg_draw.c:1888): static at scale 1.0. Moves only in open or close (msg_draw.c:517 to 548).

## What would settle it

1. Star check (one start, row TXTFLICK, budget 2 of 3 left): a scene with a waiting box and the star (NPC text, for example `ruins`, story text box at the right, or `yard` with an NPC). Two consecutive shots of the same box. Diff the 16 by 16 pixel patch at the box bottom-right corner (game bottom-right, stored x about 25 to 40 and y about 300 to 330 in the pause view; re-measure for the NPC view). If the patch changes and the box edge pixels do not, the star moves. Needs the star to be drawn in the shot.
2. Log check (no new start): add a line in render_messages that prints rewindArrowSwingPhase, the draw call count, and pm_interp_repl per draw. If the phase steps in 15 degree steps and pm_interp_repl alternates, the mix is the cause.
3. Console (user): with a waiting box on screen, note whether the star wobbles alone or the box edge moves with it. Star alone: this cause. Box edge also moves: the cause is in the box (open or close scale, or the bubble path), not in the star.
4. Console (user): with fps60 off (pm_interp_on 0), does the right side still move? If not, the port interpolation mix (pm_nusys.c:111) is the cause.

## Limits

- One Azahar run. Not in the waiting state. The star was not shown.
- Scene-to-pixel mapping is INFERRED from one picture.
- The console log of build 45fca7dd0 was not read. No pull of logs (rule 6).
