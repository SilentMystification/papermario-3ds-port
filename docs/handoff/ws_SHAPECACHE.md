# ws_SHAPECACHE (Haiku): gen_shapes never stores or keeps a partial shape set; a key-only build does not touch the lane shapes (tool fault of NST2J)
Designer: oracle FIN. Branch from main (no blocker). Worktree build3ds/wt/SHAPECACHE, branch SHAPECACHE. PM_ROW=SHAPECACHE PM_LANE=SHAPECACHE. Edit only: 3ds/build.sh (gen_shapes).
## Facts (code read and the NST2J lanes, 2026-11-18)
- NST2J crash cause (FACT): the NST2J and NST2JB runs log `shape load fail mac_01_shape` (lane-NST2JB/slot1/last_log.txt #24, pm_map.c:144). Their shape
  cache entry build3ds/gencache/shape/3ae2c60cfbbd7a66 holds 689 files; the 11 other entries hold 1467. mac_01_shape.nds is not in the lane romfs/shape.
  With no shape, world.c:214 loads no model, and RotateModel writes through a NULL Model. The NST2J code is not the cause.
- gen_shapes (build.sh:128-147) runs before the key exit (build.sh:249), so a key-only build (PM_KEY_ONLY=1, bench.py build_stale during every
  Azahar run) also runs it. On a miss it deletes $OUT/romfs/shape (:143) and converts into the same lane folder as the real build.
  GUESS: two builds of one lane overlapped, and one stored the partial folder (entries 3ae2c60c at 00:52:45 and 316e71f4 at 00:52:58).
- gc_put_dir (gencache.sh:98-120) stores any folder; gc_get_dir (:85-95) checks only the entry's own out.sha1. Nothing checks the file count.
- nd_shape_conv prints `shapeconv maps <n> ...` (nd_shape_conv.c:768) and writes <name>.nds per map (:743).
## Edits (build.sh gen_shapes only)
1. After line :141 (shape_key.txt): `if [ -n "$PM_KEY_ONLY" ] && [ "${PM_CACHE:-on}" != check ]; then return 0; fi # SHAPECACHE: the key needs shape_key.txt only`
2. Replace the docker line :145 by a captured run (keep its command text):
   `so=$(MSYS_NO_PATHCONV=1 docker run ... sh -c "$SH_CMD") || { echo "gen_shapes failed" >&2; exit 1; }; printf '%s\n' "$so"`
   then: `n=$(printf '%s\n' "$so" | sed -n 's/^shapeconv maps \([0-9]*\) .*/\1/p'); [ -n "$PM_SHAPE_SEED" ] && n=$((n + 1)) # SHAPECACHE seed`
   `c=$(ls "$OUT/romfs/shape" | grep -c '\.nds$'); if [ -z "$n" ] || [ "$c" != "$n" ]; then echo "gen_shapes: $c .nds files, the converter wrote $n maps" >&2; rm -rf "$OUT/romfs/shape" "$OUT/shape_entry.txt"; exit 1; fi`
3. The hit line :142 becomes: `if gc_get_dir shape "$want" "$OUT/romfs/shape" "$OUT/shape_entry.txt" && [ "$(ls "$OUT/romfs/shape" | grep -c '\.nds$')" -ge 400 ]; then echo "gen_shapes: cache hit"; return 0; fi`
   and the next line: `rm -rf "$(gc_dir shape "$want")" # SHAPECACHE: a partial entry goes; the miss path makes it again`.
   400: GUESS lower bound (a good set has 489 .nds, a partial one 230 or so). Write the real .nds count of a good entry into the commit message.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 3 builds, 1 Azahar start)
1. Edits 1-3. `rm -rf build3ds/gencache/shape/3ae2c60cfbbd7a66` in the MAIN folder (the bad entry; git-ignored).
2. Seed: `PM_CACHE=off PM_SHAPE_SEED=1 sh 3ds/build.sh > build3ds/out/shapecache_seed.txt 2>&1`. Pass: exit not 0 and the line `gen_shapes: <c> .nds files, the converter wrote <c+1> maps`.
3. `PM_CACHE=off sh 3ds/build.sh` rc 0 (a cold shape run passes the check), then `sh 3ds/build.sh` rc 0 (a hit). `sh 3ds/tools/nd_host_test.sh` PASS.
   Commit `SHAPECACHE 1: gen_shapes checks the .nds count; key-only builds keep the lane shapes`.
4. bench.py base spot_mac01_drec (merge-base, not charged). rowloop --row SHAPECACHE --bench spot_mac01_drec: LOOP PASS, byte-equal.
Fix rights: build.sh gen_shapes; a wrong sed pattern of edit 2 (read the real converter line in shapecache_seed.txt). Any other failure: stop, send the output path to the oracle.
Stop: last commit `SHAPECACHE 2: gate. seed seen; spot_mac01_drec byte-equal; nds count <n>` deletes this page. One SubagentHandback.
