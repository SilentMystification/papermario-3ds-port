# Row S7MAP2: shrink the .dlc sidecars (vertex copies as references into the .nds shape data) so the 3dsx fits the netloader

Model: Sonnet (pre-approved: S7 rows).
- Why not Haiku: a format change of the .dlc section (writer nd_shape_conv.c, readers dlcp_load and dlcp_check in nd_shape.c).
  The writer must know which pool vertex is a plain copy of a shape vertex and which one nd_dlc_compile made (T-point splits,
  tp_vtx/tp_mid in nd_dlc.c:56-86). Those are choices in the compile path.
- Why not smaller: writer and reader change together (a mixed pair fails to load).

## Facts (ORACLE6, main cd4783813; the user's XL crash of 2026-11-12 14:23)
- The crash dump build3ds/hw_o3ds_dumps/crash_dump_00000002.dmp is not the game's. Its code bytes are newlib strcpy: they
  match `strcpy` at 0x430020 in the 4877 ELF, and the same bytes in the 4576 3dsx. pc 0x13abbc is nowhere near strcpy in
  either build, so the faulting process is a different 3dsx_app: the Homebrew Launcher's netloader. The stack holds the
  3dslink argv string "de01a8c0_3DSLINK_" (host 192.168.1.222). The abort is a write to address 0 (r0 = 0, far 0) inside
  strcpy: a failed allocation while it took the received file.
- The 4877 3dsx is 51,283,664 bytes; build 4576 was 34,118,220. The growth is the romfs: build3ds/romfs/shape holds 489 .dlc
  files of 18 MB in total (S7MAP), next to 20 MB of .nds. In applet mode the netloader has too little free RAM for the file
  (test_catalog.md C7, NETLOADER REFUSED).
- The game log of the same minute (GPU stuck at frame 87, load_mtx 0x71f at frame 23) is from build 4576 (67b270c94, 301
  commits behind main): an older 3dsx launched after the failed send. It says nothing about main's S5-S7 code.
- dlcp_load (nd_shape.c:249-303) reads `nvtx` Vtx copies (16 bytes each) per list from the .dlc. Most are copies of shape Vtx that
  the .nds already holds (nd_env_vtx: `ptr_of(w1)` inside gMapShapeData, nd_shape.c:122-126).

## Blocker and base
None (S7MAP is merged). `sh 3ds/tools/wt_new.sh S7MAP2`; PM_ROW=S7MAP2 PM_LANE=S7MAP2. Base `git merge-base HEAD main`.

## Design (fixed; the coder chooses the inner steps)
1. Writer (nd_shape_conv.c): for each list and variant, describe the vertex copies as runs `{shape offset, count}` (copies of
   consecutive shape Vtx), plus literal Vtx only for the vertices nd_dlc_compile made itself (T-point inserts). Find the source
   of each copy by matching it against the shape data (the env's vtx() source pointers, recorded in the host env).
2. .dlc version + 1: a section holds the runs and the literals instead of all Vtx. Indices and groups are unchanged.
3. Reader (dlcp_load): fill the pool from gMapShapeData + offset for each run and from the file for the literals. The rest is as
   today (rebase, commits). dlcp_check compares the pool contents as before, so dlcchk proves the new form.
4. Expected size: .dlc total under 4 MB (hand back the before and after totals and the 3dsx size).

## Readers and writers
Writer nd_shape_conv.c; readers dlcp_open (version check), dlcp_load, dlcp_check (nd_shape.c); build.sh gen_shapes stamp (if a
new input file). grep `dlcp_\|\.dlc` in 3ds before the commit and name each reader.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS. Hand back `ls -la build3ds/pm_3ds.3dsx` (expected about 35 MB, near build 4576) and the .dlc total.
- Bases spot_mac01_det, mdl5_iwa10_det; one list `bench.py run spot_mac01_det,mdl5_iwa10_det,dlcchk_mac01 --row S7MAP2 --why
  "S7MAP2: vertex runs"` (3 starts): 6/6 and 6/6 byte-equal; dlcchk_mac01 `bad 0`, `pre` above 0. Budget 6.

## Stop
A byte difference, `bad` above 0, or a .dlc total above 6 MB: stop, commit, hand back. Delete this row file in the completing commit.
