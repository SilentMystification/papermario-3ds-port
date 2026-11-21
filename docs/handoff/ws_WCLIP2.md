# ws_WCLIP2 (Haiku): Fullscreen places rectangles at 1.25 x, as meshes and scissors (plan_widewin.md 2 cause 2)
Designer: oracle NATIVE20. Worktree build3ds/wt/WCLIP2, branch WCLIP2. PM_ROW=WCLIP2 PM_LANE=WCLIP2. Start: main with WCLIP1 merged.
User default (MENUGEN design, pm_gpu.c:217 "HUD included"): Fullscreen stretches text, HUD and window rects too. Counter: census out in Fullscreen.
FACT: rects use px = 40 + x (pm_gpu_rect.c:82) and fills 40 + x (:161-162). No rect path reads pm_stretch. Meshes scale 1.25 (pm_gpu.c:236-237),
scissors scale 1.25 (nd_sci.c, WCLIP1). Readers of the rect x: px (tex_raw_rect :252, its pieces and splits take the px columns), the fill (:161-162).
Not touched: the wide edge rule (:253-257, pm_wide only), the grab quad (pm_gpu.c:2873, already Fullscreen-aware), the gutters (wide only).
## Edits (3ds/src, then 3ds/tools/azt.d, 3ds/tools/bench.d)
1. pm_gpu_rect.c, after the includes: `extern int pm_stretch; /* pm_gpu.c: Fullscreen */`.
2. pm_gpu_rect.c:82. Before: `static float px(int x) { return gs.off_active ? (float)x : 40.f + (float)x; }`. After:
   `static float px(int x) { return gs.off_active ? (float)x : pm_stretch ? 1.25f * (float)x : 40.f + (float)x; } /* WCLIP2: Fullscreen 1.25 x */`
3. pm_gpu_rect.c:161-162. After:
```c
        int xa = gs.off_active ? x0 : pm_stretch ? x0 * 5 / 4 : (pm_wide && x0 <= 0) ? 0 : 40 + x0; /* WCLIP2 */
        int xb = gs.off_active ? x1 + 1 : pm_stretch ? (x1 + 1) * 5 / 4 : (pm_wide && x1 >= 319) ? 400 : 40 + x1 + 1;
```
4. pm_gpu.c drec_begin, next to the `drecsc` line: `if (pm_stretch) pm_log_evt("wclip2 f%u", pm_frame_count()); /* WCLIP2: expected-diff marker */`.
5. azt.d/WCLIP2.json, bench.d/WCLIP2.json: pause_full_drec, debug `preset=toad padscript=wait80,START*3 fullscr loglevel=debug det drec=116:1`,
   until `drec f116 draws [1-9]\d* bytes \d+`, cap 25 (form of WCLIP0.json pause_wide_drec).
## Tests (build3ds/out/, last 20 lines). Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. Build; c0gate 0 / 0 / 0.
2. `bench.py cmp msgw_full_drec --row WCLIP2 --expect wclip2`: `expected-diff PASS`. Row log: every clipg line of f330 has `out 0`.
3. Run pause_full_drec once (new bench, no base). Row log: every clipg line of f116 has `out 0`.
Native and Widescreen need no run: every edit is behind pm_stretch, which is 0 there (pm_gpu.c:1881, game_modes.c:284). Name this in the commit.
Fix rights: compile errors, the scenario JSON. An out above 0, an expected-diff FAIL: send the output path to oracle NATIVE20.
Stop: commit `WCLIP2 1: gate` (deletes this page). Hand-back: one SubagentHandback: hash, cmp lines, the clipg lines of tests 2 and 3.
