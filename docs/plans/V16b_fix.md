# V16b: rectangle around a Shy Guy hit by Thunder Rage (root cause found, fix is in a main-owned file)

Status: root cause found. Fix not applied. The file is 3ds/src/pm_gpu.c (owned by the main-tree agent until PG0 to PG2 are merged). Row stops at "fix ready, shared file".

## Reproduction (Azahar)
- Switches: `fast fixedseed demoscene=12 preset=toad shots shotwin=270:1` (demo scene 12 = battle index 3, Watt Thunder Rage on Shy Guys, BTL_DIG_FORMATION_03).
- The shot of game frame 270 (the shot is the last finished frame, so draw frame 269) shows a yellow and white rectangle over one Shy Guy. It flickers (every other frame) and moves to the next Shy Guy.
- Shots: 3ds/logs/perf/v16b/zoom1.png (frames 270 to 314, crop), 3ds/logs/perf/v16b/only81_84.png (only the four guilty draws drawn).

## Root cause
- The rectangle is not the sprite flash. It is the effect `flashing_box_shockwave` (src/effects/flashing_box_shockwave.c), type FX_SHOCK_OVERLAY_SHOCK_HIT. It draws 9 quads (vertex buffers D_090006A8 and D_090007E8) with a 32x32 I8 noise texture (D_09000200) and the odd-frame flicker `time % 2`.
- Draw log (switch drawlog=269:1): chunks #81 to #84 (img 0046f3d0, 10 and 8 triangles, prim ffff00ff). `ddonly=81:84` draws only the rectangle.
- Render mode of this draw: oml 0c184f50 = G_RM_ZB_OVL_SURF2 (2 cycle, Z_CMP, ZMODE_DEC = decal, blend with 1-alpha). The combiner is exact (plan USED, planerr 0): colour (ENV-PRIM)*T0+PRIM, alpha T0*SHADE*PRIM.
- On the N64 a decal passes the depth test only where the new depth is within a small distance of the stored depth, so the overlay only lands on pixels of the object it decorates. Here that is the Shy Guy sprite (the sprite writes depth only for opaque texels). Around the sprite the stored depth is the far wall, so the quads fail.
- The port maps a decal to GPU_GEQUAL plus a depth bias of 0.001 toward the eye (3ds/src/pm_gpu.c:429 and :436 to :439). GEQUAL passes everywhere the quad is nearer than the stored depth, so the overlay also lands on the wall behind the sprite. The result is the full rectangle of the quads.

## Fix (needs the owner of pm_gpu.c)
The decal test must pass only when the stored depth is within a band around the quad depth. The PICA has one depth test, so use two passes with the stencil (only for draws with r_decal and blend):
1. Pass 1, colour write off, depth write off: depth test LEQUAL with the quad depth moved away from the eye by EPS (C3D_DepthMap bias -EPS). Stencil: write 1 where it passes (the stored depth is not farther than quad - EPS).
2. Pass 2, normal colour draw with the present GEQUAL and bias, plus stencil test EQUAL 1, then clear or leave the stencil (the B3b zmark code at pm_gpu.c:422 to :426 already uses the stencil in the same way).
EPS: start with 0.003 (decal bias 0.001 times 3), tune with the switch decalbias. Cost: a second pass for decal draws only (shadows, overlays).
A test showed the principle only through reading, not through a run: I used up the 8 starts of row V16b before I could test a depth variant (GPU_EQUAL as a quick check). The first test after the fix: the same switches and shot as above, the rectangle must follow the Shy Guy sprite.
Risk: every decal shadow and decal overlay in the game uses this path. After the fix run golden toad, yard and battle and compare floor shadows.

## Question for the user
None needed for the cause. The look after the fix is the N64 look (an electric flicker that follows the sprite).
