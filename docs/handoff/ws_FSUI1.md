# ws_FSUI1 (Haiku): Fullscreen takes the Widescreen output path (UI 1:1 at 40 + x, 3D 1.25 x); undo the WCLIP2 rect stretch
Designer: the oracle in the launch prompt. Worktree build3ds/wt/FSUI1, branch FSUI1. PM_ROW=FSUI1 PM_LANE=FSUI1. Start: main dfb703014f or later. Counter: none of C1 to C12.
Objective (plan_widewin.md section 6): one flag `pm_wout = pm_wide || pm_stretch` replaces pm_wide at each GPU output read. Native and Widescreen have pm_stretch 0,
so pm_wout == pm_wide there and their bits do not change. Line numbers are at dfb703014f: grep the old text before each edit. CRLF sources: use the Edit tool.
## Directory order (then edits): 3ds/include/nd_int.h, 3ds/src/pm_gpu_int.h, 3ds/src/pm_gpu.c, 3ds/src/pm_gpu_rect.c, 3ds/src/pm_main.c, 3ds/src/nd_tex_set.c, 3ds/src/nd_sci.c,
3ds/src/pm_options.c, 3ds/tools/nd_sci_test.c, 3ds/tests/host.d/wclip1.sh, src/game_modes.c, src/background_gfx.c. Edit no other file.
1. Declare. nd_int.h:353, pm_gpu_int.h:77 (after `extern int pm_wide;`) and background_gfx.c:14 (after `extern int pm_wide; /* F07-4 */`): add `extern int pm_wout; /* FSUI1: pm_gpu.c */`.
   pm_gpu.c:219: replace `stretched 1.25x in X over 400 px, HUD included. Only the final viewport (rebuild_3d) and scissor (stage_scissor) and the grab columns read it.` by
   `with the 3D picture 1.25x in X over 400 px; UI and HUD 1:1 as Widescreen (pm_wout, FSUI1).` Add after it: `int pm_wout; /* FSUI1: 1 = the 400 px output of Widescreen and Fullscreen (pm_wide || pm_stretch) */`
2. Set. pm_gpu.c:1942: append ` pm_wout = pm_wide || pm_stretch;` before its comment. pm_options.c:304: add `, pm_wout` to the extern list; after :310 add `pm_wout = pm_wide || pm_stretch;`.
   game_modes.c:281-284 (the `pm_wide = pm_wide_cfg && !(mode == ...` statement and the `{ extern int pm_stretch_cfg, pm_stretch; ... }` line) become:
```c
    {   /* FSUI1: logos, title, file select, intro and the chapter screen keep the Native picture in Widescreen and in Fullscreen */
        const s32 n43 = mode == GAME_MODE_LOGOS || mode == GAME_MODE_TITLE_SCREEN || mode == GAME_MODE_LANGUAGE_SELECT || mode == GAME_MODE_END_LANGUAGE_SELECT ||
            mode == GAME_MODE_FILE_SELECT || mode == GAME_MODE_END_FILE_SELECT || mode == GAME_MODE_INTRO || chapter;
        extern int pm_stretch_cfg, pm_stretch, pm_wout;
        pm_wide = pm_wide_cfg && !n43; pm_stretch = pm_stretch_cfg && !pm_wide && !n43; pm_wout = pm_wide || pm_stretch;
    }
```
3. View matrix pm_gpu.c:238-239. :238: delete `(pm_stretch && !gs.off_active) ? gs.vp_sx * 1.25f : `; :239: delete `pm_stretch ? gs.vp_tx * 1.25f : `; in both write `pm_wout` for `pm_wide`.
   After: `view.r[0].x = (pm_wout && !gs.off_active && (!ui_ortho || wedge_x)) ? gs.vp_sx * 1.25f : gs.vp_sx;` and `view.r[0].w = gs.off_active ? gs.vp_tx : (pm_wout && !ui_ortho) ? gs.vp_tx * 1.25f : 40.f + gs.vp_tx;`
   In the :238 comment `MENUGEN: Fullscreen scales every draw` becomes `FSUI1: Fullscreen as Widescreen`.
4. Rename shape: the one token `pm_wide` becomes `pm_wout`; the rest of the line stays. Example pm_gpu.c:3460 before `pm_gv_rejm = (pm_wide && ortho) ? 0x2c : 0x2f;`
   after `pm_gv_rejm = (pm_wout && ortho) ? 0x2c : 0x2f;`. Sites (15): pm_gpu.c:1259 `int ui = pm_wide &&`, :2401 `(pm_wide && siz == 2`, :3460, :3479, :3679, :3823, :3849,
   :3855, :3862 (`pm_wide && !gs.off_active`), :4001 `(uint32_t)pm_wide;`; pm_gpu_rect.c:254 `if (pm_wide && !gs.off_active && xb > xa`; pm_main.c:268 `extern int pm_wide;`,
   :271 `(pm_wide && in_buf(`; nd_tex_set.c:221 `if (pm_wide ||`; background_gfx.c:374 `if (pm_wide) nd_mark(G_PM_ND_GUTTER`. Change no other pm_wide read and no `pm_wide || pm_stretch` read.
5. Undo WCLIP2 in pm_gpu_rect.c. :83 after: `static float px(int x) { return gs.off_active ? (float)x : 40.f + (float)x; } /* F07-3: 2D is 1:1 and centered, also with wide and Fullscreen (FSUI1) */`
   :162 after: `int xa = gs.off_active ? x0 : (pm_wout && x0 <= 0) ? 0 : 40 + x0; /* FSUI1: Fullscreen as Widescreen */` :163 after: `int xb = gs.off_active ? x1 + 1 : (pm_wout && x1 >= 319) ? 400 : 40 + x1 + 1;`. Keep :13.
6. Marker pm_gpu.c:673: `"wclip2 f%u"` becomes `"fsui f%u"`; its comment `WCLIP2: expected-diff marker` becomes `FSUI1: expected-diff marker`.
7. nd_sci.c:17 before `const int s = mode == 1 && ui;` after `const int s = ui; /* FSUI1: Widescreen and Fullscreen: a 2D or orthographic batch is 1:1 at 40 + x */`.
   nd_sci.c:3: `(1:1 at 40 + x in Widescreen)` becomes `(1:1 at 40 + x in Widescreen and Fullscreen)`. nd_sci_test.c, after `chk(-20, 0, 92, 240, 2, 0, 0, 115);`: add the comment
   `/* Fullscreen 2D or ortho (mode 2, ui 1): as Widescreen (FSUI1) */` and one line each: `chk(0, 0, 319, 239, 2, 1, 0, 400);` `chk(12, 20, 308, 220, 2, 1, 0, 400);`
   `chk(100, 0, 200, 240, 2, 1, 140, 240);` `chk(-20, 0, 92, 240, 2, 1, 20, 132);` `chk(250, 0, 340, 240, 2, 1, 290, 380);`. Grid: `mode == 1 && ui == 1 && lo >= 1` becomes `mode != 0 && ui == 1 && lo >= 1`.
   wclip1.sh: after the 4th seed add `seed 's#const int s = ui;#const int s = mode == 1 \&\& ui;#' || exit 1`; the last echo becomes `wclip1 seeds: 5 FAIL`.
## Tests (output to build3ds/out/ of the worktree, read the last 20 lines). 1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS with `wclip1 ok` and `wclip1 seeds: 5 FAIL`. 2. `PM_LANE=FSUI1 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: calls 0 interp 0 state 0.
3. Widescreen: `bench.py base msgw_wide_drec --commit $(git merge-base HEAD main)`, `bench.py run msgw_wide_drec --row FSUI1 --why "wide unchanged"`, `bench.py cmp msgw_wide_drec --row FSUI1`: all byte-equal.
4. Fullscreen: `bench.py base pause_full_drec --commit $(git merge-base HEAD main)`, `bench.py run pause_full_drec --row FSUI1 --why "fullscreen ui 1:1"`, `bench.py cmp pause_full_drec --row FSUI1 --expect fsui`:
   `expected-diff PASS`. The run log holds `gpu: wide=0 stretch=1`, `fsui f116`, and every `clipg f116` line has `out 0`.
Fix rights: the 12 files. A compile fault from a page edit: fix it and note it in the commit. A DIFF in test 3, an expected-diff FAIL, a clipg line with out 1 or more, or a host FAIL: send the output path to the oracle in the launch prompt with SendMessage. Apply its reply.
Stop: commit `FSUI1 1: gate` with this page deleted. Hand-back: one SubagentHandback: commit hash, host result, c0gate, both cmp lines, the clipg f116 lines.
