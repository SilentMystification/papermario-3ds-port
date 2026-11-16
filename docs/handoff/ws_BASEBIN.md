# ws_BASEBIN (Haiku): one base build per commit, kept in a content-keyed store; content keys on the base shots (counter T3)
Designer: the oracle (SendMessage). Worktree build3ds/wt/BASEBIN, branch BASEBIN, PM_ROW=BASEBIN PM_LANE=BASEBIN. Start now. No shared file with VOLGEN (build.sh) or BENCHSPEED (nd_host_test.sh). Plan: docs/handoff/plan_buildspeed.md.
## Objective and counters
- FACT now: `bench.py base` builds the merge-base once per bench (bench.py:1882-1903) and deletes the build (:1696-1711, :1849-1851).
  scratch_build copies 18426 asset files (:1295-1303). runs.jsonl: base cc671d2df was built at 16:05 and again at 16:32.
- Pass: `base b1,b2` makes 1 build. A second use of the same commit makes 0 builds. Every store use checks key and file hashes.
## Files (all in docs/handoff/ws_BASEBIN.patch). Edit only these 4.
3ds/tools/bench.py, 3ds/tools/basebin.py (new), 3ds/tests/basebin/test_basebin.py (new), 3ds/tests/host.d/basebin.sh (new).
## Cache keys (cache safety: content only; review this list)
- basebin (<main>/build3ds/basebin/<16 hex>/): commit id and build number (both go into pm_build.h), sha1 of `git ls-tree -r <commit>`
  (every tracked file, build.sh and Dockerfiles too), every asset file (git blob ids), ROM sha1, both image ids, PM_FNCOV.
- base shots (meta.json key): the basebin key, the full bench and scenario definitions, slot 1 azahar.exe sha1, azt.ps1,
  run_azahar.ps1, azslot.ps1. Not in the key: the slot qt-config.ini (OPEN: Qt rewrites it at exit). Old entries without a key are stale.
## Edit shape (the patch does it)
Before (bench.py:1826): `rc, build_no = scratch_build(commit, gitdir, scratch, lane, row)`.
After: `rc, build_no = basebin_build(commit, gitdir, scratch, lane, row, src3dsx)` (store hit: copy, no build; miss: build, then bb.put).
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_BASEBIN.patch | git apply -`. A reject: stop, send the output to the oracle.
2. `python 3ds/tests/basebin/test_basebin.py`: `basebin ok`. `sh 3ds/tools/nd_host_test.sh > build3ds/out/bb_host.txt 2>&1`: `ND HOST TESTS PASS`, `basebin ok`.
3. Commit the 4 files: `BASEBIN 1: one base build per commit in a content-keyed store; content keys on base shots`.
4. `python 3ds/tools/bench.py base spot_mac01_drec,mdl5_iwa10_det > build3ds/out/bb_1.txt 2>&1`. Pass: 1 line `build stored in basebin`,
   no second build, and 2 lines `two runs equal, stored`.
5. `python 3ds/tools/bench.py base --prebuild > build3ds/out/bb_2.txt 2>&1`. Pass: `build from the basebin store`, `basebin ... ready`.
6. Seeded fault, basebin: flip byte 4096 of the 3dsx of the entry:
   `python -c "import pathlib,sys;p=next(pathlib.Path(sys.argv[1]).glob('*/pm_3ds.3dsx'));b=bytearray(p.read_bytes());b[4096]^=1;p.write_bytes(b)" "$(cygpath -m "$(git rev-parse --git-common-dir)/../build3ds/basebin")"`.
   Then step 5 again to bb_3.txt. Pass: `failed its check`, then `build stored in basebin`.
7. Seeded fault, shots (spot_mac01_drec stores drec.txt, FACT basecache/ae6d66d3e): flip byte 0 of the stored drec.txt:
   `python -c "import pathlib,sys;p=pathlib.Path(sys.argv[1]);b=bytearray(p.read_bytes());b[0]^=1;p.write_bytes(b)" "$(cygpath -m "$(git rev-parse --git-common-dir)/../build3ds/basecache/$(git merge-base HEAD main)/spot_mac01_drec/drec.txt")"`.
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row BASEBIN --commit $(git merge-base HEAD main) > build3ds/out/bb_4.txt 2>&1`.
   Pass: `a stored file differs from its hash: removed`. Then `python 3ds/tools/bench.py base spot_mac01_drec` (Pass: `build from the basebin store`, `stored`).
8. Gate: `PM_ROW=BASEBIN PM_LANE=BASEBIN sh 3ds/build.sh > build3ds/out/bb_build.txt 2>&1` (rc 0), then
   `python 3ds/tools/bench.py run spot_mac01_drec --row BASEBIN --why "BASEBIN gate"` and
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row BASEBIN --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
Budget: 3 builds (2 base, 1 row), 7 Azahar starts.
## Fail
- A python error that a step names in one of the 4 files: fix that line only. 2 tries. Never change a key input list: send the output path to the oracle.
- Any other failed pass line: stop. Send the output path to the oracle.
## Stop and hand-back
All steps pass. The last commit `BASEBIN 2: gate` deletes ws_BASEBIN.md and ws_BASEBIN.patch. Hand back (one SubagentHandback):
commits, the pass lines of steps 4-7, the base wall time of step 4 and of step 7 (base), the cmp line.
