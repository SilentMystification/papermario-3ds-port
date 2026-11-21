# ws_WCLIP1 (Haiku): one scissor rule for every draw in Widescreen and Fullscreen (plan_widewin.md 4); Native unchanged
Designer: oracle NATIVE20. Worktree build3ds/wt/WCLIP1, branch WCLIP1. PM_ROW=WCLIP1 PM_LANE=WCLIP1. Start: main with WCLIP0 merged.
Objective: msgw_wide_drec census group `g 0 319` moves from `c 0 359 ... out >= 1` to `c 0 400 ... out 0`. Native byte-equal. Counter: census out.
Edits (order: 3ds/include, 3ds/src, 3ds/tools, 3ds/tests/host.d). 1. New 3ds/include/nd_sci.h and 3ds/src/nd_sci.c (pure C, no 3DS header; CMake globs src/*.c, CMakeLists.txt:348):
```c
/* WCLIP1: x edges of the game scissor as screen columns (before the 400 - x flip of stage_scissor). x0, x1 raw (not clamped), y0, y1 clamped.
 * mode 0 Native, 1 Widescreen, 2 Fullscreen; ui: a 2D or orthographic batch (1:1 at 40 + x in Widescreen). */
static int ci(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
int nd_sci_full(int x0, int y0, int x1, int y1) { /* the full native screen (both decomp forms) or its safe area */
    return (x0 == 0 && y0 == 0 && (x1 == 320 || x1 == 319) && (y1 == 240 || y1 == 239)) || (x0 == 12 && y0 == 20 && x1 == 308 && y1 == 220);
}
void nd_sci_x(int x0, int y0, int x1, int y1, int mode, int ui, int* top, int* bottom) {
    if (x1 < x0) { const int t = x0; x0 = x1; x1 = t; } if (mode == 0) { *top = 40 + ci(x0, 0, 320); *bottom = 40 + ci(x1, 0, 320); return; }
    if (nd_sci_full(x0, y0, x1, y1)) { *top = 0; *bottom = 400; return; }
    const int s = mode == 1 && ui;
    *top = x0 == 0 ? 0 : s ? ci(40 + x0, 0, 400) : ci(x0, 0, 320) * 5 / 4;
    *bottom = (x1 == 319 || x1 == 320) ? 400 : s ? ci(40 + x1, 0, 400) : ci(x1, 0, 320) * 5 / 4;
}
```
2. pm_gpu.c:66: add `static int gsci_wx0, gsci_wx1 = 320; /* WCLIP1: raw x edges for the wide and Fullscreen rule */`.
   pm_gpu_set_scissor (:4819): first `const int rx0 = x0, rx1 = x1;`; flush test (:4829) add `|| ((pm_wide || pm_stretch) && (rx0 != gsci_wx0 || rx1 != gsci_wx1))`
   (Native batching stays); after `gsci_y1 = y1;` add `gsci_wx0 = rx0; gsci_wx1 = rx1;`.
   Frame begin (:3018-3021): add `gsci_wx0 = 0; gsci_wx1 = 320;`. Grab (:2901, :2904, :2913): save both in sc[4..5], set `gsci_wx0 = 0; gsci_wx1 = x1;`, restore.
3. stage_scissor (:1198-1205). Before: the `top` and `bottom` lines. After:
```c
        const int mode = pm_stretch ? 2 : pm_wide ? 1 : 0; int top, bottom; nd_sci_x(mode ? gsci_wx0 : gsci_x0, gsci_y0, mode ? gsci_wx1 : gsci_x1, gsci_y1, mode, ui, &top, &bottom);
        if (__builtin_expect(drec_on, 0)) wclip_note(top, bottom, ui); /* marker: the old rule gives other columns */
```
   wclip_note (cold, noinline, above stage_scissor): the old pair from the removed lines (gsci_x0, gsci_x1); when it differs: `pm_log_evt("wclip f%u g %d %d old %d %d new %d %d", pm_frame_count(), gsci_wx0, gsci_wx1, ot, ob, top, bottom)` (at most 4 lines per frame).
4. Host test: 3ds/tools/nd_sci_test.c and 3ds/tests/host.d/wclip1.sh (form of host.d/coin.sh: `-Wall -Wextra -Werror`, then seeds by sed on a copy).
   Table (x0 y0 x1 y1 mode ui -> top bottom): Native 0 0 320 240 -> 40 360; 0 0 319 239 -> 40 359; 100 0 200 240 -> 140 240. Wide ui: 0 0 320 240 -> 0 400;
   0 0 319 239 -> 0 400; 12 20 308 220 -> 0 400; 100 0 319 240 -> 140 400; -20 0 92 240 -> 20 132; 250 0 340 240 -> 290 380; 100 50 220 190 -> 140 260; 330 0 380 240 -> 370 400.
   Wide 3D: 0 0 319 239 -> 0 400; 100 0 200 240 -> 125 250. Fullscreen: 0 0 319 239 -> 0 400; 12 20 308 220 -> 0 400; 100 0 200 240 -> 125 250; -20 0 92 240 -> 0 115.
   Grid: x0, x1 in {-40 -20 0 1 12 100 160 250 308 319 320 340 360}, every mode and ui: 0 <= top <= bottom <= 400; Native top = 40 + ci(min); W ui with
   1 <= x <= 318 on both sides: top = 40 + x0, bottom = 40 + x1. Prints `wclip1 ok`. Seeds (each must print FAIL): right `(x1 == 319 || x1 == 320)` ->
   `x1 >= 320`; left `x0 == 0 ? 0` -> `x0 <= 0 ? 0`; safe `x0 == 12 && y0 == 20` -> `x0 == 13 && y0 == 20`; centred `s ? ci(40 + x0` -> `s ? ci(x0`.
## Tests (build3ds/out/, last 20 lines). Base: `bench.py base <b> --commit $(git merge-base HEAD main)`. 1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS with `wclip1 ok` and 4 seed FAIL lines. Build; c0gate 0 / 0 / 0.
2. `bench.py cmp msgbox_talk_nowide --row WCLIP1`: byte-equal (Native). `bench.py cmp msgw_wide_drec --row WCLIP1 --expect wclip`: `expected-diff PASS`; the row log has `clipg f330 g 0 319 c 0 400` with `out 0`.
Fix rights: compile errors, the test harness. A Native DIFF, an expected-diff FAIL, out above 0: send the output path to oracle NATIVE20. Stop: commit
`WCLIP1 1: gate` (deletes this page). Hand-back: one SubagentHandback: hash, host lines, cmp lines, the clipg and wclip lines.
