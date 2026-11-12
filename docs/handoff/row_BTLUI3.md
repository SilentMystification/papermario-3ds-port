# Row BTLUI3: core 0 trace of the battle menu IA8 quads (spot, beam, reel): what the walker does with each triangle

(Model: Haiku. Bug: user fact: the main battle menu panel (projector beam) and the lower-left reel do not render; the icons do. Code
only. BTLUI2 (merged): drawn on core 1 with flags clear, opacity 150-254, menu open, no counted drop, no thread race.)

## Code read (oracle, main after BTLUI2)
render_complex_hud_element case 2 (src/hud_element.c:1537 on): quad in transform->quadBuffers[FrameQuadIndex] (HudElemTemplateQuad:
normals 0, colour 0,0,0,255; ob +-width/2, +-height/2; tc 0..width*32), gDPLoadTextureBlock IA8 64x64 (spot 32x32), PM_CC_47
(TEXEL0 x PRIM, alpha the same), prim = tint and opacity, render mode TEX_EDGE / CLD_SURF (no Z_CMP), lighting on with HudElemLights
and G_ZBUFFER (hud_element_draw_complex_hud_first: ortho 0..320, -240..0, z -1000..1000; scissor full in wide). The census counts a
refused or dropped triangle but not a triangle rejected as outside the view (gv_tri :202, :207: n_out, no drop count), nor a draw with
alpha 0 or a texture of zero alpha. Which of these hits only the IA8 quads is not decidable by reading; this trace decides.

## Start
`sh 3ds/tools/wt_new.sh BTLUI3`. PM_ROW=BTLUI3 PM_LANE=BTLUI3.

## Touch only
src/hud_element.c (render_complex_hud_element case 2: one debug note), 3ds/src/nd_geom.c (nd_geom_vtx, gv_tri), 3ds/src/pm_gpu.c
(the raw batch close nd_raw_native), a small cold table in 3ds/src/nd_geom.c. Debug switch `cplxtrace`, no draw change.

## Exact edits
1. nd_geom.c: `const void* ctr_vtx[8]; unsigned ctr_n;` (cold). hud_element.c case 2 under the switch (read once, as cplxhud): after
   the gSPVertex line store `&transform->quadBuffers[FrameQuadIndex]` into ctr_vtx[ctr_n++ & 7].
2. nd_geom_vtx (core 0): when the switch is on and src equals one of ctr_vtx[]: `pm_log_d("ctrv f%u src %p n %u gmi %u cls %d cb %02x
   %02x %02x %02x p %d,%d,%d,%d", ...)` (gmi, the GP1 class, vert_cb of the 4 slots, and the clip x,y of vertex 0 and 2 as ints after
   the CPU transform when made: vert_p[v][0..1] * 100 / vert_p[v][3]); remember a flag `ctr_live = 1` until the next vertex load.
3. gv_tri while ctr_live: one line per triangle `ctrt f%u out %d why %d app %d trip %d` (out: the early reject at :202 or :207;
   why: the refusal code; app: gappend taken; trip: gtrip taken).
4. nd_raw_native (pm_gpu.c, the batch close of those triangles: the first close after a ctrt line): `ctrb f%u comb %d tex %d slot %d
   dim %dx%d fmt %d prim %08x blend %d atest %d z %d %d use5 %d` (gs.combine, the bound slot and its size and format, prim, the render
   state, nb.g_use5).
5. Scene btlui_trace: btlui_menu_det with `cplxtrace` instead of cplxhud, until `ctrb f2[2-9]\d` (frames 220 on), cap 40.

## Gate (measurement)
Build, memcheck OK, nd_host PASS. One Azahar start: `bench.py run btlui_trace --row BTLUI3 --why "core 0 fate of the battle menu
IA8 quads"`. Hand back the ctrv, ctrt and ctrb lines of frames 220-224 as printed. Stop after it.

## At the end
Delete this row file in the completing commit.
