# Row FSELLAG2: clear only the needed band of an off-screen slot (file select lag P1), Sonnet design core, needs the user's yes

Model: Sonnet (not in the 5 accepted Sonnet rows: the orchestrator asks the user). Reason in one line: the band to clear needs the
memory orientation of a PICA render-target texture (which buffer rows a game row lands in, citro3d C3D_RenderTargetCreateFromTex)
and the bilinear edge reads of every full-slot user, which no single code line gives. P2 (the full sh_clear) is row FSELLAG1
(Haiku). Height source (decided): the scissor that draw_box sets before the fill (draw_box.c:789 gDPSetScissor 0, 0, width,
height) is in force at the first draw of the pass, which calls off_screen_check -> pm_gpu_off_begin (nd_target.c:88-93): pass
that scissor height (nd_set.c:59-60 sci_y1) as a new argument, 0 for every other caller (whole slot, as today).

## User fact (2026-11-12)
The file select screen is laggy while the save rectangles rotate. Build 5101 (main a2b28500a, FSELFIX included) is on the O3DS XL.

## Code read (ORACLE8, main f555551c4; earlier read row_ORACLE_NEXT.md:149-160)
- A ROTSCALE window draws its contents into an off-screen slot each frame: draw_box.c:786-800 sets the colour image to
  mdl_address (G_SETCIMG), fills the window area 0..width-1 x 0..height-1 (:793), runs fpDrawContents, sets the colour image
  back to the frame buffer (:801), then draws the strips that read the slot (FSELFIX: one vertex batch).
- G_SETCIMG on core 0: set_cimg (nd_target.c:95-97) calls pm_gpu_off_end when the image changes. The next draw calls
  off_screen_check (nd_target.c:88-93) -> pm_gpu_off_begin (pm_gpu.c:2051). The census count 86 per pick frame counts calls:
  most return at :2056 (same slot already active). A real pass costs:
  P1 a GX memory fill of the whole full slot, C3D_RenderTargetClear (:2121), 512 x 256 x 2 bytes = 256 KB, when the slot is new
     in the frame or reused (OFF_N = 3 full slots, the 4th window and later reuse one, :2091-2100 "V05 demand"). The contents
     pass then fills its own area first (draw_box.c:793). Only the area outside the window (texture edge, bilinear) needs it.
  P2 two full target switches per window: flush (:2057), rebuild_3d, sh_clear (nd_leave_native marks every citro3d state dirty,
     raw1_reset: program and buffers rebound), C3D_FrameDrawOn (framebuffer flush and rebind) at begin, and the same at off_end
     (:2420) back to the main target. With 11 windows: 22 full state re-emissions per frame.
  P3 the slot texture binds for the strips (ensure_tile_slow 135 per pick frame after FSELFIX).
- A slot clear is needed only for the pixels a later read samples and no draw of the pass writes. Users of full slots: draw_box
  contents passes (fill first), pm_gpu.c:2656-2667 (the frame buffer slot: a copy overwrites, "cleared on first use"), and any
  other G_SETCIMG target (grep the users: list them in the commit).

## Design (fixed. The coder chooses the inner steps)
1. P1: clear only what is needed. Allowed forms: (a) no GX fill when the pass's first draw is a fill rectangle that covers the
   area the later strips sample (record the covered area of the first fill per pass, clear the rest lazily only if a later read
   samples outside it), or (b) a fill restricted to the 8-row block band of the image height (the tiled layout keeps a band of
   block rows contiguous) plus one block row for bilinear. The coder picks one and proves it by the byte gate.
2. P2: row FSELLAG1.
3. P3: only if 1 and 2 do not end the lag on the console (not in this row).

## Places
3ds/src/pm_gpu.c: pm_gpu_off_begin (signature, the clear at :2118), its 3 other callers (:2656, :2667, :2685: argument 0),
3ds/include/pm_gpu.h prototype, 3ds/src/nd_target.c off_screen_check (:88-93). Waits for FSELLAG1 (same function).
Not src/draw_box.c (plan row NBOX waits).

## Gate (expected values)
- Build, memcheck OK. nd_host PASS.
- Byte compare (rule 2): `bench.py base fsel_det_pic` at the merge-base, run, `bench.py cmp fsel_det_pic --row FSELLAG`: every
  shot byte-equal. Second scene `pause_tab_det_pic` (the pause flip shares the path): byte-equal.
- Census: fsel_fncov_pick before and after: C3D_RenderTargetClear calls per pick frame and sh_clear calls per pick frame go down
  (hand back both pairs of numbers). Budget 8 Azahar starts.
- The user checks the lag on the console (one push of the build after the merge).

## Stop
A shot difference, a slot user whose pass does not overwrite what it samples, or a tool fault: stop, commit, hand back (2 tries per
named cause). Delete this row file in the completing commit.
