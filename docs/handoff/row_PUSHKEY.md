# Row PUSHKEY: PM_KEY_ONLY prints only the build key, so a push with -NoBuild can pass

(Model: Haiku. 60 lines or fewer. The agent reads only this file, CLAUDE.md and what this file names.)

## Tool fault (orchestrator fact, CLAUDE.md 4)
`go.ps1 push o -ip 192.168.1.170 -NoBuild -Interactive -Profile log` from build3ds/wt/FPS60FL exited 5:
"STALE 3DSX: build key a28e293514b8, tree key texkey_bake: / gen_shapes: / gen_gfx_tab:". No push with -NoBuild can pass today.

## Cause (oracle code read, ORACLE8, main 3c7d977ab)
- 3ds/push_3ds.sh:176 `now=$(PM_LANE= PM_KEY_ONLY=1 sh 3ds/build.sh || true)` takes ALL stdout of build.sh. :177 compares it with
  build_key.txt (one line, the key).
- 3ds/build.sh runs 3 generators before the PM_KEY_ONLY exit (:131). They print to stdout: gen_texkey (:57, texkey_bake.py:190
  `print("texkey_bake: ...")`), gen_shapes (:79, `echo "gen_shapes: up to date"` :71), gen_gfx_tab (:101, `echo "gen_gfx_tab: up to
  date"` :89). So $now holds 3 lines before the key and never equals build_key.txt.
- bench.py build_stale (:953-954) takes the last line only, so it works. 3ds/tests/push/run.sh case (b2) (:48) writes the same
  multi-line output into its key file, so the test compared 4 lines with 4 lines and missed the fault.
- The generators must still run in key-only mode: key_now (:122-128) hashes their output files. Only their stdout moves.

## Start
`sh 3ds/tools/wt_new.sh PUSHKEY` (build3ds/wt/PUSHKEY, branch PUSHKEY from main). Use PM_ROW=PUSHKEY PM_LANE=PUSHKEY on every call.

## Touch only
Places: 3ds/build.sh, the 3 generator call lines `gen_texkey` (:57), `gen_shapes` (:79), `gen_gfx_tab` (:101).
3ds/tests/push/run.sh, the block after case (b2) (after :52). No other file. No other line. Do not change key_now.

## Exact edits
1. 3ds/build.sh :57: replace the line `gen_texkey` with `gen_texkey >&2 # PUSHKEY: stdout of build.sh is the key only (PM_KEY_ONLY)`.
2. 3ds/build.sh :79: replace `gen_shapes` with `gen_shapes >&2 # PUSHKEY`.
3. 3ds/build.sh :101: replace `gen_gfx_tab` with `gen_gfx_tab >&2 # PUSHKEY`.
4. 3ds/tests/push/run.sh: after the (b2) block (the line that ends with `fail=1; fi` after "FAIL matching key") add:
```
# (b3) PUSHKEY: the key-only output is one line of 40 hex digits, twice the same, and equal to the build_key.txt of a built folder.
kf=build3ds/${PM_LANE:+lane-$PM_LANE/}build_key.txt
k1=$(PM_KEY_ONLY=1 sh 3ds/build.sh 2>/dev/null); k2=$(PM_KEY_ONLY=1 sh 3ds/build.sh 2>/dev/null)
if [ "$(printf '%s\n' "$k1" | wc -l | tr -d ' ')" = 1 ] && echo "$k1" | grep -Eq '^[0-9a-f]{40}$' && [ "$k1" = "$k2" ] && [ -f "$kf" ] && [ "$k1" = "$(cat "$kf")" ]; then echo "ok   key only: one key line, equal to $kf"
else echo "FAIL key only (k1 '$(printf '%s' "$k1" | tr '\n' '|')', file $kf '$(cat "$kf" 2>/dev/null)')"; fail=1; fi
```
5. Commit: `PUSHKEY: build.sh PM_KEY_ONLY prints only the key, generator output to stderr, push test case b3`.

## Gate (expected values)
1. Seeded fault first: make edit 4 only. `PM_ROW=PUSHKEY PM_LANE=PUSHKEY sh 3ds/build.sh > build3ds/out/build.txt 2>&1`
   (build OK, memcheck OK), then `PM_LANE=PUSHKEY sh 3ds/tests/push/run.sh > build3ds/out/push_test_before.txt 2>&1`.
   Expected: the line `FAIL key only` with `texkey_bake:` in k1, and `PUSH TESTS FAIL`.
2. Make edits 1-3. Build again as in step 1 (the key changes: build.sh is in KEY_IN). Then
   `PM_LANE=PUSHKEY sh 3ds/tests/push/run.sh > build3ds/out/push_test.txt 2>&1`. Expected: `ok   key only`, every other case ok,
   `PUSH TESTS PASS`, exit 0.
3. No Azahar start. No console push. Counter: none (tool row).

## Stop
A failed gate, a line that does not match the text above, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the two test outputs, the memcheck line.
