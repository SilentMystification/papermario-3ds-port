# Row S7TEX1: tex_win0 and nd_tex_win_desc out of nd_tex_set.c into a pure file the host tool can link (S7 NDG_TEX, step 1)

(Model: Haiku. A move of two functions and one typedef, no logic change. Step 2 (S7TEX2, Sonnet: the tool emits NDG_TEX ops with
an NdBindDesc for a load whose inputs are all mapped, and nd_gtab_run binds it) is written after this and S7RUN merge.)

## Code read (main 3cef2c284, 3ds/src/nd_tex_set.c)
- :141 `typedef struct NdWin { const u8* src; int w, h, stride, x0; } NdWin;`
- :142-187 `static void tex_win0(const NdTst* t, const NdTexM* m, const void* timg, int uls, int ult, int lrs, int lrt, NdWin* o)`:
  pure (reads only its arguments; plan_nativeapi.md section 6 item 3). Callers: set_tilesize :210, nd_tex_win_desc :762.
- :747-769 nd_tex_win_desc: pure except `nuGfxZBuffer` (:763, the z-buffer address test).
- NdTst and NdTexM are in 3ds/include/nd_int.h (:192 NdTexM).

## Start
`sh 3ds/tools/wt_new.sh S7TEX1`. PM_ROW=S7TEX1 PM_LANE=S7TEX1 on every call.

## Touch only
3ds/src/nd_tex_set.c (remove the 3 pieces), new 3ds/src/nd_texwin.c, 3ds/include/nd_int.h (2 declarations after NdTexM),
3ds/tools/nd_host_test.sh (one compile line at the end of the test list).

## Exact edits
1. New 3ds/src/nd_texwin.c: a header comment ("S7TEX1: the pure tile 0 window (tex_win0) and the descriptor of a tile load
   (nd_tex_win_desc), moved from nd_tex_set.c unchanged so the host table tool can link them"), the includes nd_tex_set.c uses
   for these types (<stdint.h>, <string.h>, "ultra64.h", "nd.h", "nd_int.h", "nd_api.h"), `extern u16 nuGfxZBuffer[];` only if
   no included header declares it, then the typedef NdWin, tex_win0 and nd_tex_win_desc moved verbatim. tex_win0 loses `static`.
2. nd_tex_set.c: delete the typedef (:141), tex_win0 (:142-187) and nd_tex_win_desc (:747-769). Leave the comment lines above
   tex_win0 with it (move them too).
3. nd_int.h after the NdTexM typedef: the NdWin typedef (moved there instead of nd_texwin.c if nd_tex_set.c still needs the type:
   it does, set_tilesize uses NdWin) and `void tex_win0(const NdTst* t, const NdTexM* m, const void* timg, int uls, int ult, int lrs,
   int lrt, NdWin* o);`. So the typedef lives in nd_int.h, not in nd_texwin.c.
4. The device build picks up 3ds/src/*.c (check 3ds/CMakeLists.txt: if it lists files by name, add nd_texwin.c there).
5. nd_host_test.sh: one line that compiles nd_texwin.c for the host with the nd_direct_test flags (`gcc -m32 -c ... -o
   /tmp/ndtexwin.o 3ds/src/nd_texwin.c`, the device defines and includes of the nd_direct_test line): a compile check only.

## Gate (expected values)
- Build, memcheck OK (code size equal within a few bytes); `bench.py run nd_host --row S7TEX1 --why "S7TEX1 move"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det --row S7TEX1
  --why "S7TEX1: tex_win0 moved"` (2 starts): 6/6 and 6/6 byte-equal (set_tilesize is shared draw code). Budget 4.

## Stop
A build error that needs more than the move, or a byte difference: stop, commit, hand back. Delete this row file in the
completing commit.
