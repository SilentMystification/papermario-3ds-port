# Row BTLUICEN: which draw path drops the battle UI background and the lower-left item (census, head against the build before NAPI)

(Model: Haiku. Bug: user fact: part of the battle UI does not render anymore: the background and the item in the lower left corner;
it worked in earlier builds. Code only: no shot reading.)

## Code read (oracle, main a2265beb8)
Merged since the last good builds that draw battle UI: NAPI2A (d03ac017d: hud_element_draw_rect records NdHudRec, core 0
hud_rect_run replays the same macros, src/hud_element.c:267-500), NAPI3A (ba72c32c0: msg_draw_char glyph records), NAPI1 (7852be398:
NDC_NAPI in stream_run). Battle icons and boxes go through draw_hud_element_internal (hud_element.c:1983-2083: hud_element_draw_clipped,
_next, _without_clipping; btl_states_menus.c:1872-1963) and draw_box windows with a contents callback (draw_box.c:795-833). The read of
the NAPI2A record (header, pieces, emitter, runner) finds no path that drops a rect: same words, same order, the pieces bound holds.
Paths that drop a HUD or box draw with no fallback, each with a census counter:
 1. a texture rectangle refused at pm_gpu_tex_rect (pm_gpu_rect.c:267-277): `rect f<n> ... drop <n>` and `rectwhy` (ring, range, gpu);
 2. a fill rectangle refused (pm_gpu_rect.c:152-171), same lines;
 3. a ring triangle dropped (draw_box quads, nd_geom.c:291-293): `tridrop`;
 4. a raw batch refused (nraw_why, pm_gpu.c:1360-1389): `nrawwhy`;
 5. a record that never reaches core 0 (the stream: a stray word reset, nd_rec.c stray_word: `ndc: ... list words outside the stream`).
The census of the last build before NAPI1 (9eeb74b1a, FXSPLIT merge) against head names the path, or shows that the drop is
older than NAPI.

## Start
`sh 3ds/tools/wt_new.sh BTLUICEN`. PM_ROW=BTLUICEN PM_LANE=BTLUICEN.

## Exact edits (tool files only)
Scenario and bench btlui_cen: copy of battle_start_f182 with debug `warp=kmr_03:0 story=ch0_toad_town battle battlef=100 census
loglevel=debug det`, until `rectwhy f300`, cap 40, needs_picture false; bench azahar_stream, normal, default false. `bench.py rebuild`.
Commit both together.

## Gate (measurement)
Build (lane BTLUICEN), memcheck OK. `bench.py run btlui_cen --row BTLUICEN --why "battle UI drop path, head"` and `bench.py build-at
9eeb74b1a btlui_cen --row BTLUICEN --why "battle UI drop path, before NAPI"` (2 builds, 2 Azahar starts). Hand back, for both logs,
the f100, f200 and f300 lines rect, rectwhy, tridrop, nrawwhy, ndm, sprnat, and every `ndc:` error line, as printed. Stop after it.

## At the end
Delete this row file in the completing commit.
