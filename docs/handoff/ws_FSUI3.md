# ws_FSUI3 (Haiku): Fullscreen scene background (sky, background colour, side bars) at 1.25 x like the 3D picture
Designer: the oracle in the launch prompt. Worktree build3ds/wt/FSUI3, branch FSUI3. PM_ROW=FSUI3 PM_LANE=FSUI3. Start: the FSUI1 gate commit or main after `Merge FSUI1`
(FSUI2 and FSUI3 share no file: they can run at the same time). Counter: none of C1 to C12.
Objective (plan_widewin.md section 6): after FSUI1 every rectangle is 1:1 at 40 + x. The background of gfx_draw_background (background_gfx.c:376-479) is part of the
scene: in Fullscreen it must sit under the 3D picture (game x 12..308 at columns 15..385). A walker bracket G_PM_ND_SCENE2D marks it. The game emits the bracket only
when pm_stretch is 1, so Native and Widescreen get no new record and their bits do not change. Grep the old text before each edit. Use the Edit tool (CRLF).
## Directory order: include/gbi_custom.h, 3ds/src/nd_target.c, 3ds/src/pm_gpu_rect.c, 3ds/src/pm_gpu.c, src/background_gfx.c. Edit no other file.
## Edits
1. gbi_custom.h, after `#define G_PM_ND_SPRSHADE 4` (:53), add:
   `/* FSUI3: G_PM_ND_SCENE2D (arg 1 opens, 0 closes) brackets the scene background of gfx_draw_background in Fullscreen. Inside, the walker places`
   ` * rectangles and fills at 1.25 x like the 3D picture (pm_gpu_rect.c px, pm_gpu_fill_rect). */` and `#define G_PM_ND_SCENE2D 5`.
2. nd_target.c, after the line `unsigned hold_grab_f = ~0u;` (:48): add `int nd_scene2d; unsigned nd_scene2d_n; /* FSUI3: the G_PM_ND_SCENE2D bracket (background_gfx.c, nd_native_rec); opens since boot */`.
   In nd_native_rec, after the G_PM_ND_GUTTER line (:55): `if (kind == G_PM_ND_SCENE2D) { nd_scene2d = (int)on; if (on) nd_scene2d_n++; } /* FSUI3 */`.
3. pm_gpu_rect.c, after `extern int pm_stretch; /* pm_gpu.c: Fullscreen */` (:13): add `extern int nd_scene2d; /* FSUI3: nd_target.c, the scene background bracket */`.
   Shape: an extra first test `(pm_stretch && nd_scene2d) ? <1.25 x form> :` after `gs.off_active ? ... :`. px (:84 after FSUI1) before
   `static float px(int x) { return gs.off_active ? (float)x : 40.f + (float)x; }` after `static float px(int x) { return gs.off_active ? (float)x : (pm_stretch && nd_scene2d) ? 1.25f * (float)x : 40.f + (float)x; }`
   (keep the comment, add `FSUI3: the scene background at 1.25 x`). pm_gpu_fill_rect xa: insert `(pm_stretch && nd_scene2d) ? x0 * 5 / 4 : ` after `gs.off_active ? x0 : `;
   xb: insert `(pm_stretch && nd_scene2d) ? (x1 + 1) * 5 / 4 : ` after `gs.off_active ? x1 + 1 : `.
4. pm_gpu.c drec_end, after the line `pm_log_evt("clipc f%u groups %u over %u", ...); /* WCLIP0 */` (:693): add
   `{ extern unsigned nd_scene2d_n; pm_log_evt("scn2d f%u opens %u", pm_frame_count(), nd_scene2d_n); } /* FSUI3: the census of the scene bracket */`.
5. background_gfx.c: after `extern int pm_wout; /* FSUI1: pm_gpu.c */` (:15 after FSUI1) add `extern int pm_stretch; /* FSUI3: pm_gpu.c */`. In the default case:
   after `nd_target_end();` of the z-buffer fill (:388, the line before `nd_val_fill(PACK_FILL_COLOR(camera->bgColor[0], ...`) add
   `if (pm_stretch) nd_mark(G_PM_ND_SCENE2D, 1); /* FSUI3: the scene background at 1.25 x in Fullscreen */`. Before the `break;` that ends the default case (:479, after the
   `nd_rect_fill(0, backgroundMaxY, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);` block) add `if (pm_stretch) nd_mark(G_PM_ND_SCENE2D, 0); /* FSUI3 */`.
## Tests (output to build3ds/out/ of the worktree, read the last 20 lines)
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS. 2. `PM_LANE=FSUI3 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: calls 0 interp 0 state 0.
3. Native: `bench.py base msgbox_talk_nowide --commit $(git merge-base HEAD main)`, `bench.py run msgbox_talk_nowide --row FSUI3 --why "native unchanged"`,
   `bench.py cmp msgbox_talk_nowide --row FSUI3`: byte-equal.
4. Fullscreen: `bench.py base msgw_full_drec --commit $(git merge-base HEAD main)`, `bench.py run msgw_full_drec --row FSUI3 --why "fullscreen scene background"`,
   `bench.py cmp msgw_full_drec --row FSUI3 --expect fsui`: `expected-diff PASS`. The log holds `gpu: wide=0 stretch=1` and `scn2d f330 opens N` with N 1 or more.
Fix rights: the 5 files. A compile fault from a page edit: fix it, note it in the commit. Any other FAIL, DIFF or `opens 0`: send the output path to the oracle in the launch prompt.
Stop: commit `FSUI3 1: gate` with this page deleted. Hand-back: one SubagentHandback: commit hash, host result, c0gate, both cmp lines, the scn2d line.
