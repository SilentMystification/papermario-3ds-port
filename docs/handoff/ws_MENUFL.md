# ws_MENUFL (Haiku): a Select menu redraw does not show the colour frame on the top screen. Start after Merge GRABSEQ (both edit pm_gpu.c).
Objective (FACT, oracle code read, main 89725d888c). User: "switching select menus with R makes the top screen flash color for a frame".
R changes the tab (pm_options_ui.c:139-141) and draws. Every menu draw sets menu_dirty (pm_gpu_ui.c:57). The frozen pass presents (pm_main.c:611).
pm_gpu_menu_present_w (pm_gpu.c:1914-1925) runs a full frame: C3D_FrameEnd (:1923) sends the colour target to the top screen and swaps.
Then pm_gpu_menu_grey (:1948-1966) waits for the queue (up to 100 ms), greys 400 x 240 x 3 bytes on the CPU and swaps again.
The colour frame shows for at least one vblank. Fix: after the top is grey, a redraw flushes the buffers and does not send the top again.
Counters: none. Model: Haiku. Worktree build3ds/wt/MENUFL, branch MENUFL. PM_ROW=MENUFL PM_LANE=MENUFL on every bench call.
Files: 3ds/src/pm_gpu.c, 3ds/tests/host.d/menufl.sh (new). Edit no other file.
## Edit 1 (pm_gpu.c:1913). After `static int sr_on; /* the stereo record state, ... */` add:
`static int menu_grey_top; /* MENUFL: the top screen shows the grey picture of this freeze (pm_gpu_menu_grey); a menu redraw does not send the colour target again */`
## Edit 2 (pm_gpu.c:1915-1916). Before: `    if (!menu_dirty || !gs.gpu_ok || !target) return;` / `    menu_dirty = 0;`
After: the same 2 lines, then the new line `    if (menu_grey_top) { gfxFlushBuffers(); return; } /* MENUFL: the bottom text only; the top keeps its grey picture */`
## Edit 3 (pm_gpu.c:1943). Before: `static void pm_gpu_menu_dirty_set_w(void) { menu_dirty = 1; }`
After: `static void pm_gpu_menu_dirty_set_w(void) { menu_dirty = 1; menu_grey_top = 0; } /* MENUFL: the freeze open (pm_main.c:601) sends the top once */`
## Edit 4 (pm_gpu.c:1964). Before: `    gfxScreenSwapBuffers(GFX_TOP, false);` (inside pm_gpu_menu_grey). After: the same line, then `    menu_grey_top = 1; /* MENUFL */`
## Edit 5: 3ds/tests/host.d/menufl.sh (LF endings), exactly:
```sh
#!/bin/sh
# MENUFL: a menu redraw after the grey does not send the colour target to the top screen (pm_gpu.c pm_gpu_menu_present_w).
chk() { awk '/^static void pm_gpu_menu_present_w\(void\) \{/{f=1} f&&/C3D_FrameBegin/{print (s?"ok":"late"); exit} f&&/if \(menu_grey_top\) \{ gfxFlushBuffers\(\); return; \}/{s=1}' "$1"; }
mkdir -p /tmp/menufl_seed; grep -v "if (menu_grey_top) { gfxFlushBuffers(); return; }" 3ds/src/pm_gpu.c > /tmp/menufl_seed/pm_gpu.c
[ "$(chk /tmp/menufl_seed/pm_gpu.c)" = "late" ] || { echo "menufl seed FAIL"; exit 1; }
[ "$(chk 3ds/src/pm_gpu.c)" = "ok" ] || { echo "menufl FAIL: no skip before C3D_FrameBegin"; exit 1; }
grep -q "menu_dirty = 1; menu_grey_top = 0;" 3ds/src/pm_gpu.c || { echo "menufl FAIL: no reset at the freeze open"; exit 1; }
[ "$(grep -c "menu_grey_top = 1;" 3ds/src/pm_gpu.c)" = "1" ] || { echo "menufl FAIL: no set after the grey swap"; exit 1; }
echo "menufl ok"
```
## Loop (1 build, at most 2 Azahar starts; base runs not charged)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/menufl_host.txt 2>&1`. Pass: `ND HOST TESTS PASS` and `menufl ok`. Commit `MENUFL 1`.
2. One build. memcheck margin above 0.
3. Gate: `python 3ds/tools/bench.py base spot_mac01_drec`, `python 3ds/tools/bench.py run spot_mac01_drec --row MENUFL --why "MENUFL gate"`,
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row MENUFL --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal (one differing bit fails).
FACT: no bench opens the Select menu. The user checks the R tab switch on the console.
## Fix scope, stop and hand-back
A host test failure: fix only menufl.sh or the 4 edits. A gate that differs: write the cmp output to build3ds/out/, send the path to the oracle.
Stop: host PASS and the gate byte-equal. Final commit `MENUFL 2: gate` with the values. It deletes this page.
Hand-back: build3ds/out/handback_MENUFL.txt (line 1 commit hash, gate value, memcheck margin, counters: none).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
