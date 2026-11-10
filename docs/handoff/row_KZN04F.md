# Row KZN04F: shaded load with less work per call (CPU path, byte-equal)

(Model: Haiku. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh KZN04F`; PM_ROW=PM_LANE=KZN04F on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Touch only
3ds/src/pm_tex.c (pm_tex_load_shaded, expand_dual), 3ds/src/nd_tex_set.c only after HC7 and COINSTAR merge: NOT in this row.
## Exact edits
1. expand_dual: replace the full memset with memsets of the pad only: columns w..pw-1 of rows 0..h-1, and rows h..ph-1.
2. pm_tex_load_shaded: before the lut hash loop, the hash of the lut only needs the 256 values once. Keep it, but unroll it 4 at a time (no other change). Do not change the key value.
3. Add to the shadestat line: the count of tex invalidations in the window (a static counter at pm_tex_invalidate's entry, debug only).
## Gate
`bench.py run kzn04_shadestat --row KZN04F --why "less work per shaded load"`: us per call falls against the merged numbers (3.6 ms per frame), and the invalidation count prints. Byte-equal: add `shots shotwin=300:1` to a copy scenario kzn04_shot (same commit), then base and cmp. 3 Azahar starts.
Then the oracle uses the invalidation count to decide on the GPU design row.
