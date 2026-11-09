# Row HC2: hot link order from the fncov census (plan_hotcode.md stage HC2)

## User facts
User (2026-11-09): shrink the hot code; queue the hotcode stages and work in parallel. HC0: the linker hot group (3ds/ld/3dsx.ld.in
:34-37) holds 20.8 KB of mostly old code and 3 stale names (nd_walk_run, pm_gbi_run, flush_impl_body); only about 20 of the
functions that run now are in it. The 124 KB set spans 3.1 MB of .text (26-39 lines per I-cache set against 4 ways).

## Start state
`sh 3ds/tools/wt_new.sh HC2`, branch HC2. PM_ROW=HC2, PM_LANE=HC2. One Haiku agent. Read plan_hotcode.md sections 0, 2 (HC2), 3.
HC1 (cold split) runs in parallel and edits the draw C files: HC2 edits only 3ds/ld/*, 3ds/tools/* and CMake/build lines.

## Steps
1. 3ds/tools/hot_order.py: input = the fncov logs of fncov_mac01_det, fncov_kmr10_det, fncov_mac00_det (union) + the map. Output
   3ds/ld/hot_order.ld: the functions that run, by class (hotset.json), then by calls per frame; citro3d/libctru members by archive
   pattern (`*libcitro3d.a:*(.text.C3D_*)` style). Every name must exist in the map (else the tool fails).
2. 3dsx.ld.in includes hot_order.ld in place of the stale hand list; the hot group starts at the beginning of .text.
3. hotset.py M2 reads the new group; the hotset_size bench fails on a missing name. build.sh regenerates hot_order.ld only when the
   fncov logs change (commit the .ld; regeneration is a tool step, not every build).
4. Optional experiment: -falign-functions=32 on the draw TUs; keep it only if M2 (span) or the console shows a gain.

## Gates (light)
- Normal build: memcheck OK; M2 = the contiguous span of the M3 set (report bytes).
- nd_host PASS. Byte-equal against stored base shots: spot_mac01_det, battle_start_f182_cold (code order only).
- Console (user): prof push at mac_01 0 0 200, pmu_i per frame vs de415917f.
