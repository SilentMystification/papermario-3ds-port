# ws_C1HOT (Haiku: tool-driven, byte-equal): a fresh hot code group for both cores; the core 1 recorder gets its own contiguous classes
Designer: oracle NATIVE15 (plan_perf.md F2). Branch from main. Worktree build3ds/wt/C1HOT, branch C1HOT. PM_ROW=C1HOT PM_LANE=C1HOT.
Facts:
- Console (build 7569, main 0b1ba0f322, FACT capture): core 1 ic1 92K I misses and dc1 51K D misses per step, lgms 44.4 ms.
- 3ds/ld/hot_order.ld (344 lines) puts a hot group first in .text (3dsx.ld.in:31-33). hot_order.py made it from an old census: its roots
  (hotset.json classes A-I, for example nd_nr_exec, nd_comb_set, decide) are deleted functions, and it lists no core 1 recorder function.
- The census lane instruments every 3ds/src/nd_*.c (CMakeLists.txt:384-399), so nd_rec.c, nd_ds.c, nd_snap.c, nd_snap_in.c are counted.
Objective: the hot functions of each core sit together, core 0 classes first, then the core 1 classes. c0gate stays 0 / 0 / 1.
Proxy (Azahar): `python 3ds/tools/hotset.py --line` M2 stale names 0, and the core 1 classes J, K, L listed in hot_order.ld with their bytes.
Console number to move later: core 1 ic1 (92K per step) and lgms (44.4 ms); core 0 pmu imiss.
Edit only: 3ds/tools/hotset.json (classes), 3ds/ld/hot_order.ld (generated). No source edit.
## Edits
1. hotset.json classes: keep A to I, but replace their roots with these (current names):
   A `stream_run`, `nd_cpy_run`, `nd_tsz_set`, `tex_tile1`. B `nd_snapchk_tri`, `nd_comb_apply`, `pm_gpu_set_plan`, `const_rgb_tex_alpha`, `name_colour`.
   C `rec_run`, `pm_gpu_model_hit`, `pm_gpu_model_keyb`, `nd_tex_get`. D to I: keep their roots.
   Add J `{"name": "core 1 stream units", "roots": ["nd_rec_nr", "nd_ds_apply", "ds_pairs", "nd_alt_put", "nd_alt_mark", "xr_put", "nd_sv_enc", "sh_drop"]}`,
   K `{"name": "core 1 snap", "roots": ["snap_build0", "nd_snap_in", "nd_snap_rnd", "nd_snap_bind", "nd_lsc_unit", "nd_render_state", "nd_snap_mix"]}`,
   L `{"name": "core 1 records", "roots": ["nd_rec_model", "nd_ds_model", "nd_msnap_rec", "nd_msnap_take", "nd_rec_ent_draw", "nd_rec_spr", "txr_take", "nd_mkey_a"]}`.
   Load the JSON with python after the edit. A root that `hotset.py` reports missing from the map: delete it and name it in the commit.
2. Census (inputs, not the gate): `PM_FNCOV=1 PM_LANE=C1HOT_fncov sh 3ds/build.sh`, then
   `PM_ROW=C1HOT python 3ds/tools/bench.py run fncov_mac01_det,fncov_kmr10_det,fncov_mac00_det --row C1HOT --lane C1HOT_fncov --why "C1HOT: census of the hot group"`.
3. `PM_LANE=C1HOT sh 3ds/build.sh`, then `PM_LANE=C1HOT python 3ds/tools/hot_order.py build3ds/azt/logs/fncov_mac01_det.txt build3ds/azt/logs/fncov_kmr10_det.txt build3ds/azt/logs/fncov_mac00_det.txt`.
   It rewrites 3ds/ld/hot_order.ld. Then `PM_LANE=C1HOT sh 3ds/build.sh` again (the link reads the new group).
## Checklist (hw_guidelines.md rules 1, 10)
- Placement only: no code, no data change. The group lists core 0 classes A to I, then core 1 classes J to L: each core reads one contiguous span.
- Report the bytes per class (hotset.py full report) and the span of J to L. A class over 16 KB is a fact for the next row, not a failure.
## Tests (output to build3ds/out/, read the last 20 lines; gate: 1 build, 2 Azahar starts; census above: 1 build, 3 starts)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/c1hot_host.txt` (timeout 600000): PASS. `PM_LANE=C1HOT python 3ds/tools/hotset.py --line > build3ds/out/c1hot_hotset.txt`: M2 stale 0.
   Commit `C1HOT 1: hot group from a fresh census, core 1 classes`.
2. `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: 0 / 0 / 1. memcheck OK.
3. Base, run and cmp (`bench.py base <bench> --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_drec,entcape_kmr03_drec --row C1HOT`, `bench.py cmp`): every shot byte-equal.
Fix rights: hotset.json roots, compile or link errors. A census run with no `fncov f... done` line, a DIFF or M2 stale > 0: stop, send the output path to oracle NATIVE15.
Stop: last commit `C1HOT 2: gate` deletes this page. One SubagentHandback: commit hash, host, c0gate, memcheck, cmp, hotset line, bytes of J, K, L.
