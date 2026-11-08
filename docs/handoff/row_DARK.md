# Row DARK: kgr_01 darkness overlay draws as a garbled plus with a quarter stencil at the top left

## User facts (bug, 2026-11-08)
Trigger: enter kgr_01 (inside the whale; the map runs EVS_CreateDarkness, src/world/common/atomic/CreateDarkness.inc.c, which sets the BACK screen overlay to a spotlight centred on Mario each frame). Seen in play (console). Wrong look: a garbled plus-shaped texture crosses where the lit circle around Mario's face should be; the rest of the scene stays visible (no dark fill); a quarter of the stencil texture shows at the top left of the screen. Expected: the whole screen dark except a soft circle of light around Mario. Reproducing scenario: add one (warp=kgr_01:0, a shot after the overlay starts; until-line = a log line the build prints when the overlay draws).

## Goal
Investigate by code first (CLAUDE.md rule 5). The overlay is _render_transition_stencil (src/screen_overlays.c:180-330): a 320x240 ortho, a stencil mesh (D_8014E8F0 / D_8014EA48 style vertex lists) textured with ui_stencil_sharp_circle_png (I4 32x32, G_TX_MIRROR | G_TX_CLAMP on s and t, mask 5, screen_overlays.c:52), plus fill rectangles (lines 224-243). Name for each draw: the vertex positions and uv range, the tile wrap the port gives the PICA, and where the fill goes.
Lead to check first (not a conclusion): N64 MIRROR|CLAMP with a mask means "mirror once inside the mask range, then clamp". The PICA has repeat, mirrored repeat, clamp to edge and clamp to border, but not mirror-once. A plain clamp shows one quadrant (the top-left quarter); a mirrored repeat tiles the circle (a plus/grid pattern). pm_tex.c:155-156, 386, 1615 already build mask-sized images for mirror/repeat windows: check whether that path runs for this tile, and whether mask 5 on a 32-wide image plus clamp gets a 64x64 mirrored image with clamp to edge. Also check why the outer fill rectangles are missing (combine G_CC_PRIMITIVE fill, wide mode x range, scissor).

## Start state
Worktree build3ds/wt/dark, branch dark from nds1 (current). Copy assets/us and ver/us/build into it.

## Touch only
The native draw path the trace names (pm_tex.c mask/mirror image build, nd_tex_set.c tile wrap, nd_rect.c fills). No src/ game change unless the trace proves the port's own draw cannot express it.

## Done criteria
1. Code trace in the handback: each overlay draw, uv range, the PICA wrap, and the failing value.
2. Build; nd_host PASS.
3. Picture gate: the new kgr_01 scenario shot shows the screen dark with a lit circle around Mario (wide mode on, as the user plays).
4. spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015 byte-equal to the base, unless the trace shows they use the changed path (then name the pixels). Other stencil users (screen wipe transitions with the Mario head and star stencils) use the same path: one shot of one wipe.
Affected scenarios (no others): the new kgr_01 scenario, one wipe scenario, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=dark PM_ROW=dark on every build.sh and bench call. No git stash. No toggles, no stubs. 2 tries per named cause. Scenario and bench in the same commit.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=dark PM_LANE=dark sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=dark PM_LANE=dark python 3ds/tools/bench.py run <bench> --row dark --lane dark --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "dark ready <hash>".
