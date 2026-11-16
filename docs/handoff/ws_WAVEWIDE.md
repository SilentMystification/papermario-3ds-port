# ws_WAVEWIDE (Haiku): Tidal Wave blue square in wide
Designer: oracle 37 (SendMessage to the oracle). Worktree build3ds/wt/WAVEWIDE, branch WAVEWIDE from main. PM_ROW=WAVEWIDE PM_LANE=WAVEWIDE.
No counter (C1..C12) moves. Model: Haiku (closed blocks).
Cause, FACT (code read and console log build3ds/hw_tidal2_live.txt #207574-#207682):
- Each frame first grabs the old frame buffer (grab_prev, slot 0), then the wave grabs the frame buffer drawn now (slot 1).
  Log: f84377 `grab 0 src 0xff7e10` (old), then `fbbind n0 src 0x101d610` and `grab 0 src 0x101d610` (the wave).
- The two frame buffers are adjacent: gFrameBuf1 = gFrameBuf0 + 153600 (FACT pm_3ds.map 0xff7e10, 0x101d610). A full slot spans
  256 rows of 640 bytes = 163840 (pm_tex.h:6). pm_tex.c:1712-1715 takes the FIRST slot that holds the address. With the drawn
  buffer at 0x101d610 slot 0 (base 0xff7e10 - 80 in wide, pm_gpu.c:2245) holds it at d 153680: off_oy 240, off_ox 40.
- So the mesh samples slot rows 240-255. The grab quad fills rows 0-239 only (pm_gpu.c:2749). The rows past 239 hold the clear (0).
  TEXEL0 = 0, PM_CC_48 (macros.h:501) gives prim * prim_a: a solid blue square. With 0xff7e10 drawn, slot 0 starts after the image:
  the wave is right. The buffers alternate, so the square flickers. 60 FPS has no part in it. Non-wide has the same fault (d 153600).
- The mesh is 4:3 by design (underwater.c:183-194, game x 12..308). In wide the end columns do not reach the screen edges.
Design:
- New pm_tex_offpick.h: pm_offt_pick takes the matching slot with the smallest offset (the nearest base). load_inner and
  pm_tex_off_rewin use it. One match: same slot as before.
- underwater.c: in wide, column 0 goes to game x -40 and column 18 to game x 360, texel = position. Per triangle no vertex lies in the
  edge band (pm_gpu.c:3379), so the wide edge rule moves nothing: screen 0 and 400. Non-wide: no change.
Files: 3ds/src/pm_tex.c, src/effects/underwater.c, new: 3ds/include/pm_tex_offpick.h, 3ds/tools/pm_tex_offpick_test.c,
3ds/tests/host.d/offpick.sh. Do not edit any other file.
## Edit shape (pm_tex.c:139) Before: `static struct { const u8* base; unsigned stride; C3D_Tex* tex; int siz, w, h; } offt[10]; /* siz as pm_gpu_off_begin */`
After: `static PmOffT offt[10]; /* siz as pm_gpu_off_begin; WAVEWIDE: the type is in pm_tex_offpick.h (tex holds the C3D_Tex*) */`
## Loop: blocks A to F of ws_WAVEWIDE_code.md, exact text. 1 build, 2 Azahar runs per bench.
1. Blocks A, B, C, D, E. Run `sh 3ds/tools/nd_host_test.sh > build3ds/out/wavewide_host1.txt 2>&1`.
   Pass: `ND HOST TESTS PASS` and `offpick ok 8/8`. Commit `WAVEWIDE 1: the nearest off-screen slot holds an address (pm_offt_pick)`.
2. Block F. Commit `WAVEWIDE 2: Tidal Wave mesh end columns at game x -40 and 360 in wide`.
3. Run `sh 3ds/build.sh > build3ds/out/wavewide_build.txt 2>&1`. Pass: rc 0, memcheck margin above 0.
4. Gate, B = `$(git merge-base HEAD main)`. For spot_mac01_drec and fxr_drec: `python 3ds/tools/bench.py base <bench> --commit B`,
   `python 3ds/tools/bench.py run <bench> --row WAVEWIDE --why "WAVEWIDE gate, byte-equal"`,
   `python 3ds/tools/bench.py cmp <bench> --row WAVEWIDE --commit B`. Pass: each 1/1 byte-equal. fxr_drec cold flake: one warm rerun (bench.py:2023).
## Fix scope
A failing host test or seed: compare your files with the blocks byte for byte (LF). Do not change an expected text or the seed.
A compile error: check the include line (block B) and the extern line (block F). A byte difference: stop, send the cmp output path to the oracle.
## Stop and hand-back
Stop: host PASS, build rc 0, both gates 1/1 byte-equal. Final commit `WAVEWIDE 3: gate` with the values deletes both page files.
Hand-back: build3ds/out/handback_WAVEWIDE.txt (line 1 commit hash, then host lines, build number, memcheck margin, 2 gate values).
Any other failure after 2 tries: write the output to build3ds/out/, send the path to the oracle (SendMessage), stop.
User check on the O3DS XL (wide, Sushie Tidal Wave in battle): the wave shows the scene, no blue square, edge to edge.
