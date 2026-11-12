# Row S7MAP: map shape lists compiled at build time (nd_dlc_compile on the host into the .nds files; NATIVEAPI S7, map design core)

Model: Sonnet (pre-approved for the night run).
- Why not Haiku: the compile result (NdObj: groups, vertex copies, indices, the box class, the T point splits) must be stored in a
  relocatable form in the .nds files and loaded into the run-time pools (vertex and index pools in GPU memory) so that every
  later reader (dlc_batch_compile, the group runner, the native ranges, the seam census) sees the same DlcEnt as today. The
  SEAM2 rule makes one input (`stat`, a static matrix at the first draw) a run-time fact. These are design choices in shared code.
- Why not Opus: the compiler (nd_dlc.c nd_dlc_compile) and the seam table (nd_seam.c) are already pure C and host-built in nd_host
  (/tmp/ndseam on six shape bins), and the .nds container with relocs exists (nd_shape_conv.c, HC4f1/HC4f3).
- Why not smaller: the stored form has no reader without the loader, and the loader has no proof without the check below.
  The map custom gfx data left after S6M (dro_02 npc_merlee, hos_05 intro strips, end Credits) are NOT in this row: Haiku rows
  after it (row_S6M.md list).

## Facts (ORACLE6 code read, main 8301c42fb)
- Build: build.sh gen_shapes (:59-77) runs 3ds/tools/nd_shape_conv.c on ver/us/baserom.z64 and writes $OUT/romfs/shape/<map>.nds
  (version 2: typed list words, pointer fields as shape offsets with a reloc list).
- Run time: nd_shape.c dlc_build (:198-235) copies each static shape list into dlc_words, takes the T points of the list from the
  map seam table (seam_get :170-195, nd_seam_find; `use_tp = sl && sl->cnt && (sl->cls < 2 || stat)`, SEAM2), then calls
  nd_dlc_compile (nd_dlc.c:89) with env {nd_env_vtx, nd_env_valloc, nd_env_ialloc, tp}; on NDR_FULL it compiles again without
  pools. It copies NdObj groups into DlcGrp and runs dlc_batch_compile per group, then pm_gpu_gstat_commit / pm_gpu_gidxs_commit.
  DPCEN2 counts the compile time (pm_dp_cmp_n, pm_dp_cmp_us). nd_ent.c:242 also calls nd_dlc_compile (entity lists: not this row).

## Blocker and base
S7RUN and S7ENT merged (S7ENT changes nd_gtab.h and nd_ent.c; this row must not collide with it). `sh 3ds/tools/wt_new.sh S7MAP`;
PM_ROW=S7MAP PM_LANE=S7MAP. Base `git merge-base HEAD main`.

## Design (fixed; the coder chooses the inner steps)
1. nd_shape_conv.c links nd_dlc.c's compile part and nd_seam.c for the host (split nd_dlc.c only if its compile part needs device
   symbols: the S7TEX1 move pattern, a pure file next to it). For every static list of a shape it builds the seam table as
   seam_get does and runs nd_dlc_compile with a host env: vtx from the shape data, valloc and ialloc into per-shape arrays.
   Lists with `sl->cls >= 2 && sl->cnt` get two results (with T points, for stat 1; without, for stat 0); the others one.
2. .nds version 3 appends a section per list: {list offset, variant, NdObj scalar fields, groups (NdGrp), vertex copies, indices,
   tex decode if nd_dlc_compile fills it}, all offsets relative to the section (no host pointer). Version 2 readers stay valid
   for the old part of the file (pm_map.c / nd_shape.c read the version word: update both).
3. nd_shape.c dlc_build: when the loaded .nds has a section for (list, variant), copy its vertices and indices into the pools
   (nd_env_valloc / nd_env_ialloc give `first`: rebase vertex numbers and gbase by it), fill e->o from the section, then the
   existing group copy, dlc_batch_compile and commits run unchanged. No section, or no pool room: the run-time compile as today
   (counted: `dlcpre f<n> pre <a> rt <b> full <c>` per map load, pm_log_evt).
4. Debug switch `dlcchk`: dlc_build also runs nd_dlc_compile and compares the NdObj fields, groups, vertices (after rebase) and
   indices with the loaded section; mismatches counted in the dlcpre line as ` bad <d>`.

## Readers and writers
- Writers: nd_shape_conv.c (v3 sections), build.sh gen_shapes stamp (add nd_dlc.c, nd_seam.c and their headers to the hash).
- Readers: pm_map.c and nd_shape.c (.nds load: the version word, the section index), dlc_build (the use), the seam census line
  (seamfix ... src: keep its fields). grep `NDSH\|\.nds\|nd_dlc_compile` in 3ds before the commit; name each reader in the commit.

## Files and places
3ds/tools/nd_shape_conv.c, 3ds/build.sh (gen_shapes only), 3ds/src/nd_shape.c (dlc_build, the .nds section reader), 3ds/src/pm_map.c
(the .nds version check only), nd_dlc.c only for a pure split. No running or ready row edits these places.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS (the ndseam run unchanged).
- New scenario and bench `dlcchk_mac01` (one commit, inserted right after spot_mac01_det in both json files): debug `warp=mac_01:0
  pos=0:0:200 loglevel=debug det dlcchk`, until `dlcpre f\d+ pre \d+ rt \d+ full \d+ bad \d+`, cap 25, deadline 20,
  expected_events `warp: mac_01 entry 0` only; bench kind azahar_pic, speed normal, tags azahar. Log check only.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det,
  dlcchk_mac01 --row S7MAP --why "S7MAP: shape lists from the build"` (3 starts, all normal): cmp 6/6 and 6/6 byte-equal;
  dlcchk_mac01 `bad 0`, `pre` above 0 (hand back the line with `rt` and `full`). `bench.py base spot_mac00_det`; spot_mac00_det
  (turbo) is its own list (1 start): 6/6. Budget 8.

## Stop
A byte difference, `bad` above 0, or `pre` 0 on mac_01: stop, commit, hand back (2 tries per named cause). Delete this row
file in the completing commit. Hand back the .nds size total before and after (romfs, not the ELF).
