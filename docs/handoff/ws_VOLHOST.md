# ws_VOLHOST (Haiku): the host test reads a tar of its key files and the asset volume, not the C:\ bind mount (counter T1)
Designer: the oracle (SendMessage). Worktree build3ds/wt/VOLHOST, PM_ROW=VOLHOST PM_LANE=VOLHOST. Start now, parallel with GCONVSPLIT.
## Objective
- FACT (oracle run, main 7eb679a916, main folder, gfx and host cache empty): host test 401 s. STAGE gfx-table 215 s, container-end
  161 s (host.d 133 s). About 19 s of key hashing (hash_tree: Git Bash sha1sum of 73 MB assets, 2 times) is in no STAGE line.
- FACT (oracle runs of this patch and ws_GCONVSPLIT.patch, temp worktree): after a src edit 77 s (keys 5, batches 4, tar-assets 4,
  gfx-table 10, container-end 51, host.d 37). The first run in a new worktree: tar-assets 51 s (GUESS: first read of new files).
- Target: T1 120 s or less after a src edit. The 3 key gaps go: gtin.sh reads 3ds/build.sh, captool.sh 3ds/push_3ds.sh,
  maplag.sh 3ds/CMakeLists.txt. The old HOST_IN (nd_host_test.sh:36) has none of them.
## Files (all in docs/handoff/ws_VOLHOST.patch). Edit only these 3.
3ds/tools/nd_host_test.sh, 3ds/tests/host.d/README.txt (1 line), 3ds/tests/volhost/run.sh (new seeded self-test).
## Cache keys (plan_buildspeed.md "Cache safety")
- HOST_IN: 3ds/tools 3ds/src 3ds/include 3ds/stub_include 3ds/tests 3ds/build.sh 3ds/push_3ds.sh 3ds/CMakeLists.txt include src
  docker/Dockerfile.build docker/Dockerfile.host. Key lines: in_manifest (src_sync.sh:11), path and b:<blob> or h:<hash-object>.
- GFX_KEY: sha1 of `gfx v2`, the HOST_IN lines under GT_IN, `assets <AHASH>` (src_sync.sh:33), host image id, sha1 of the script.
  TEST_KEY: sha1 of `host v2`, all HOST_IN lines, AHASH, image id, script sha1, GFX_KEY.
- The tar list is the key list. A container holds no other tree file, so a read outside the key fails the test.
- Assets: the volume pm3ds-assets-<16 hex of AHASH> of build.sh. Every container checks its out.sha1 first. Exit 7 removes it.
  PM_CACHE=off makes a new cold volume and removes it at exit. gfxcache keeps its 8 newest entries.
## Edit shape (the patch does it). Before (nd_host_test.sh:85, :89): `docker run ... -v "$ROOT:/w" ... sh -c '`, `ln -s "/w/$d" "/work/$d"`.
After: `docker run --rm -i ... -v $AVOL:/a:ro ... sh -c "set -e; $HOST_AV"'` ... `' > "$TMPLOG" 2>&1 < $OUT/host_in.tar || hrc=$?`.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_VOLHOST.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/vh_host1.txt 2>&1`: `ND HOST TESTS PASS`, `cache selftest ok`, the STAGE lines
   keys, batches, tar-assets, gfx-table, container-end, and one `HOSTD` line for each of the 61 host.d scripts.
3. `sh 3ds/tests/volhost/run.sh > build3ds/out/vh_seed.txt 2>&1`: `volhost ok` (control, outside-file fault, asset byte fault).
4. `PM_CACHE=off sh 3ds/tools/nd_host_test.sh > build3ds/out/vh_cold.txt 2>&1`: PASS, `gfxcache: miss`, `gconvobj: hit 0`. Then
   `docker volume ls -q | grep pm3ds-assets-cold` prints nothing.
5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/vh_host2.txt 2>&1`: `hostcache: hit`, PASS.
6. Commit the 3 files: `VOLHOST 1: the host test reads a tar of its key files and the asset volume (content-id keys, self-test)`.
7. `PM_ROW=VOLHOST PM_LANE=VOLHOST sh 3ds/build.sh > build3ds/out/vh_build.txt 2>&1`: rc 0.
8. `PM_LANE=VOLHOST python 3ds/tools/bench.py run spot_mac01_drec --row VOLHOST --why "VOLHOST gate"`, then
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row VOLHOST --commit $(git merge-base HEAD main)`: 1/1 byte-equal.
Budget: 4 host runs, 1 build, 1 Azahar start.
## Fail
- A host.d FAIL that names a missing file: that script reads a file outside HOST_IN. Add the path to HOST_IN (1 line). Note it in
  the commit. Any other fail: 2 tries per cause, then send the output path to the oracle.
- `volhost FAIL`: stop. Send the path to the oracle. Do not change the test.
## Stop and hand-back. Last commit `VOLHOST 2: gate` deletes ws_VOLHOST.md and ws_VOLHOST.patch. One SubagentHandback: commits,
the STAGE lines of steps 2 and 4, `volhost ok`, the cmp line.
