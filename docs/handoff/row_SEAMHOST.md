# Row SEAMHOST: the seam T-point table is built once on the host (romfs shape/<map>.seam), not at every map load

(Model: Haiku. Base: main. Edits 3ds/tools/nd_shape_conv.c (main, the per-shape write), 3ds/build.sh gen_shapes (:59-75),
3ds/src/nd_shape.c seam_get (:42-150) and one new function above it. Not pm_map.c (MAPROMFS). Reads this file and CLAUDE.md.)

## Facts (oracle code read)
- seam_get (nd_shape.c:42-150) runs nd_seam_build(&seam_tab, gMapShapeData, 0x40000) at the first list compile of every map load:
  16 to 77 ms on the O3DS (seamfix lines: sbk_62 16 ms, mac_00 77 ms, mac_01 65 ms, isk_bt04 37 ms), on the walker, inside the load.
- nd_seam_build is pure C (nd.h:205-218; nd_seam.c is built on the host by nd_host). It reads the relocated shape file and returns
  NdSeamTab { nlist, list (NdSeamList[]: u32 addr, i32 t[3], u32 first, u16 cnt, u8 cls, ok: 24 bytes), ntp, tp (NdTp[]: i16 p[3],
  u16 tri, u16 edge: 10 bytes), 12 u32 stats }. addr is a list address inside the buffer; nd_seam_walk takes base = (u32)buf (:418).
- nd_shape_conv.c is built with gcc -m32 (build.sh:75): 32-bit pointers, little endian, the same struct layout as the ARM build.
  Its buffer s.buf holds offsets in the pointer fields (s.reloc[] lists them); the console adds pm_shape_buf at load.

## Start
`sh 3ds/tools/wt_new.sh SEAMHOST`. PM_ROW=SEAMHOST PM_LANE=SEAMHOST on every build and bench call.

## Exact edits
1. nd_shape_conv.c: `#include "nd.h"`; after writing <name>.nds: copy s.buf into tmp (malloc s.size), add `base = (u32)(uintptr_t)tmp` to
   every reloc field (as the console loader does), `NdSeamTab t; memset; int r = nd_seam_build(&t, tmp, s.size);` then write
   <out>/<name>.seam: "NDSM", u32 version 1, u32 s.size (the shape size: the key against its .nds), u32 r == 0, u32 nlist, u32 ntp,
   the 12 stats in NdSeamTab order, then list[] with each addr - base, then tp[]. nd_seam_free(&t); free(tmp).
2. build.sh gen_shapes: add `/w/3ds/src/nd_seam.c` to the gcc line (:75) and `$(git hash-object 3ds/src/nd_seam.c) $(git hash-object 3ds/include/nd.h)`
   to the `want` key (:68), so a changed seam builder regenerates the files.
3. nd_shape.c: new `static int seam_load(NdSeamTab* t)` above seam_get: open `romfs:/shape/<pm_shape_name>.seam`; check magic, version 1,
   the shape size equals the loaded shape size (pm_shape_buf use: pass 0x40000-independent: compare with the size field of the .nds the
   loader kept; if not available, accept and rely on the version), ok flag 1; malloc list and tp (nd_seam_free frees them), read them,
   add `(u32)(uintptr_t)gMapShapeData` to each list[].addr; return 0, else -1 (file missing or bad).
   seam_get: `seam_ok = seam_load(&seam_tab) == 0 || nd_seam_build(&seam_tab, gMapShapeData, 0x40000u) == 0;` and add ` src %s` (file or
   build) to the seamfix line.

## Gate (CLAUDE.md 2; Azahar budget 4 of 8)
- `PM_ROW=SEAMHOST PM_LANE=SEAMHOST sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: gen_shapes writes a .seam per shape; memcheck OK.
- `bench.py run nd_host --row SEAMHOST --why "nd_seam host build"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det --row SEAMHOST
  --why "the stored seam table equals the built one"`; `bench.py cmp` of both: 6/6 byte-equal each. The run logs' seamfix lines say
  `src file` with the same tp, ins, mtx and lists counts as the base logs.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line, one seamfix line before and after.
