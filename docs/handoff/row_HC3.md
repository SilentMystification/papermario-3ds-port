# Row HC3: -Os per draw file (experiment, CMake flags only)

(Model: Haiku. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC3`; PM_ROW=PM_LANE=HC3 on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Touch only
3ds/CMakeLists.txt (a new set_property line after :447, the same form). No source edits.
## Work
1. For each TU in turn, append -Os, build, run `bench.py run hotset_size --row HC3 --why "HC3 -Os <file>"`, then revert. TUs: src/pm_gpu.c, pm_tex.c, nd_comb.c, nd_tex_set.c, nd_model.c, pm_comb.c, nd_shape.c, nd_dlc.c, nd_ent.c, nd_walk.c, nd_geom.c, nd_rect.c, pm_gpu_rect.c, nd_tmpl.c, nd_set.c, nd_recs.c, nd_emit.c.
2. Candidate set = the TUs whose M1 class sum fell. Build it once: hotset_size, plus `bench.py run fncov_mac01_det` (M3).
3. Byte gate: `bench.py cmp spot_mac01_det --row HC3`. Expected equal (no FP flag change). If it is not equal, report which TU and stop.
4. Commit the candidate set. Give the XL prof push recipe (spot_mac01_prof, mac_01 0 0 200) to the orchestrator for the console decision. The user keeps -Os only if cycles and fps do not get worse.
## Stop
Azahar: 2 starts (fncov, cmp).
