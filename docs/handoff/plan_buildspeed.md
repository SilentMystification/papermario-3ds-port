# plan_buildspeed: a row gate loop under 10 min (oracle BUILDSPEED, main 4ee5f9508c, 2026-11-16)
Read-only design. BENCHSPEED (agent a84e568e389461aaf) owns stage timers in 3ds/tools/nd_host_test.sh only (orchestrator scope
change 2026-11-16): BASEBIN and VOLGEN start now. VOLHOST waits for Merge BENCHSPEED and Merge VOLGEN.
## Measured facts (row DLSITE2 and the code)
- FACT (brief): row loop about 70 min. Host test about 9 min, run 2 times. Own build about 11 min. Base build 11-27 min per bench.
- FACT (brief, logs CGFX/FXREN build3ds/out): sync-host 18-22 s with `changed 24668`. compile-link 30-43 s (3393 ninja steps, ccache hits).
- FACT (runs.jsonl, commit 49157bf623): base cc671d2df ran for spot_mac01_drec at 16:05 and again for dls2_fx_drec at 16:32.
  The 2 gate runs: 16:36:33 and 16:37:19 (46 s a run). Two runs in one Azahar list: 16 s apart.
- FACT: assets/us 14789 files 73 MB, ver/us/build 3637 files 25 MB, tracked code 7163 files. 3dsx 38 MB, elf 246 MB.
- FACT (.wslconfig): WSL2 memory 14 GB, swap 8 GB. Docker VM 32 CPUs. Each build container has --memory 4g (build.sh:250).
## Root causes (code read)
1. Base build per bench, cold, from a full copy. `bench.py base <one bench>` (bench.py:1882-1903) calls base_build_run([bd]).
   scratch_build (bench.py:1281-1321) extracts git archive and copies all 18426 untracked asset files with copy2 (:1295-1303).
   build.sh drops the source volume for PM_BUILD_REV (build.sh:216, :276): the sync copies all 24668 files. The scratch $OUT has
   no stamp: shapes, gconv and pso run again (build.sh:77, :96, :117). drop_base_lane (bench.py:1696-1711) and :1849-1851 delete
   the build after the store. So a second bench at the same commit builds it again. Main already built that commit at merge.
2. Cold lane per row. A row has one build in a new lane (wt_new.sh, PM_LANE=<row>): a new source volume (full sync), a new
   build volume (3393 steps), and new stamps in build3ds/lane-<row> (all generators run). texkey_bake has no stamp (build.sh:62-67).
3. Host tests and generators read C:\ through Docker bind mounts (WSL2 9p): nd_host_test.sh:20 and :88 (`-v $ROOT:/w`),
   build.sh:85, :103, :123. gen_gfx_tab.sh:17-21 compiles 244 sources (FACT gconv_src.d) through that mount, in the host test
   (nd_host_test.sh:73) and again in build.sh (:107). Only the ninja compile reads a named volume (build.sh:232, :250).
4. The page loop runs the host test 2 times: a baseline before the patch and the proof after it (ws_DLSITE2 steps 1 and 4).
5. The host test holds a build slot (nd_host_test.sh:6-7) for its full time. Base builds hold slots 11-27 min. With 3 slots
   (build_slot.sh:7) the waits reach 364 s (FACT brief).
## Answers to the brief questions
- Docker per step: the generators came one per row, each with the nd_host_test.sh mount layout (build.sh:72 note). devkitARM
  (Dockerfile.build) has no gcc-multilib, so the host steps need pm3ds-host:1. Keep 2 images. Change the mounts, not the images.
- Native build in WSL ext4: named volumes are ext4 in the same WSL2 VM. A native build saves only container start: GUESS 1-2 s
  per docker run, about 6 runs a build. Cost: a second devkitARM install that must stay equal to the image (byte gates). Do not do it.
- Base build per row: the store keeps shots per (commit, bench) (bench.py:1466), never the build. Fix in row BASEBIN.
- Host test 2 times: a page rule, not a tool rule. Fix in row PAGEHOST.
## Target loop (per row, 2 benches)
| step | now | target | label |
| host test, 1 run, volume, gconv cache hit | 9 min x2 | 2 min | GUESS (OPEN: BENCHSPEED bs_host_stages.txt gives the split) |
| own build: sync code only, generator caches hit, ccache | 11 min | 1.5 min | sync 4.2 s (FACT parts), asset hash 3.7 s FACT, compile-link 43 s FACT |
| base: built at merge, 2 Azahar starts per bench | 11-27 min x2 | 1 min x2 | FACT 16 s per warm run, GUESS start cost |
| gate: 2 runs + 2 cmp | 2 min | 2 min | FACT 46 s a run |
| patch, listcen, codemod | 1 min | 1 min | GUESS |
| total | about 70 min | about 7.5 min | GUESS. Pass: under 10 min, from BENCHSPEED STAGE lines |
## Rows (Haiku, oracle patch per row: docs/handoff/ws_<row>.md and .patch)
- PAGEHOST (docs only, start now): pages run the host test 1 time, after the change. The orchestrator removes the baseline host
  step from docs/handoff/row_BATCHTEMPLATE.md. Saves 9 min a row (FACT brief).
- VOLGEN (start now): 3ds/build.sh, new 3ds/tools/gencache.sh, new 3ds/tools/src_sync.sh, 2 new tests. Shared generator cache
  (texkey, shapes, gfx_tab, pso_tab), one asset volume per asset content, code-only sync by content ids, image-keyed ccache and
  build dir, PM_CACHE=off|check. Target on a new lane of an unchanged tree: sync 5 s or less, generators 1 s or less.
  Sync budget: git manifest 0.3 s, tar of 6243 files (40 MB) 3.0 s, one docker run 0.9 s (all FACT, oracle host timing) = 4.2 s.
  Asset hash 3.1-3.7 s (FACT) moves out of sync into STAGE asset-hash (the old key_now find took 3.9 s FACT).
- BASEBIN (start now): 3ds/tools/bench.py, new 3ds/tools/basebin.py, 2 new tests. One build for `base b1,b2`; base builds kept
  in <main>/build3ds/basebin/ by content key; no asset copy in scratch_build; content keys and file hashes on the base shots.
- MAINBIN (after Merge BASEBIN and Merge VOLGEN): 3ds/tools/merge_row.sh, new 3ds/tools/post_merge.sh, its test. After each merge:
  generator cachecheck, basebin prebuild of main HEAD, basebin and shot cachecheck. Then the cold gate. Cost GUESS 12-18 min a merge.
- GFXINC (start now; BENCHSPEED is merged): gen_gfx_tab.sh, nd_host_test.sh, new gconv_objcache.py, 2 new tests. One cache
  entry per gconv object (80, FACT), checked by the sha1 of every file the compile read (gcc -MD, as --MD for .incbin), in the
  volume pm3ds-gconvobj-<host image id> shared by nd_host_test.sh and build.sh. Fixes the gfxcache key of nd_host_test.sh:31-33.
- GFXINC2 (after Merge VOLGEN and Merge GFXINC): one line in build.sh mounts that volume for gen_gfx_tab.
- VOLHOST (after Merge GFXINC and Merge VOLGEN; page not written yet): nd_host_test.sh syncs with src_sync.sh and runs on the
  source and asset volumes, not on C:\ bind mounts. Its page carries the cache safety rules if it adds a cache.
- VOLGEN2 (later, not planned in detail): the generators read the volumes, so a miss (an src edit misses gfx_tab and pso_tab) is fast.
## Cache safety (user rule 2026-11-16; every page carries it)
1. Key = hash of the content of every input: tool files, sources read, ROM, assets, image id, flags, command text. Never a commit id
   alone, a path or a time. A dirty tree hashes its files (src_sync.sh in_manifest: blob id or git hash-object of the file).
2. Each entry stores key.txt and out.sha1. Every use recomputes the key and checks the file hashes. A mismatch deletes the entry
   and the step builds again (gencache.sh gc_ok, basebin.py check, bench.py stored, build.sh skip with build_out.sha1, the asset
   volume check in the build container: exit 7 removes the volume).
3. A temp folder, then one rename (mv -T, os.rename): no half entry. The asset volume: temp folder renamed to /a/c.
4. Seeded-fault self-tests in the host test: 3ds/tests/host.d/volgen.sh and basebin.sh (one input byte, one stored byte, a key
   clash, PM_CACHE=off, cachecheck). In-run faults in the pages: VOLGEN steps 6 and 8, BASEBIN steps 6 and 7.
5. Cachecheck after every merge, detached (POSTASYNC: postmerge_bg.sh queue, one runner, worktree build3ds/wt/postmerge):
   generators and gconv objects cold vs entry, basebin prebuild and key sweep on every merge; the cold rebuild (PM_CACHE=off)
   on merge count mod 3 = 0, the shot rerun on mod 3 = 1; after a FAIL every check runs all. postmerge_FAIL.txt stops
   merge_row.sh and push_3ds.sh until a full pass on a newer main commit.
6. Cold gate at each phase end: PM_CACHE=off build (no cache, no ccache, new build dir, new asset volume) plus all gates (MAINBIN step 5).
7. Shared caches carry the image id: gencache keys (host image), basebin key (both images), ccache volume and build dir (build image).
- Exception (OPEN for the user): the lane source volume is a mirror, not a keyed entry. It is guarded by content ids, the volume id
  check, a new volume on any id mismatch, and the cold gate. A full content check per build would cost GUESS 2.6 s (sha1 of 6243 files).
- OPEN: ninja does not see a romfs change; build.sh removes the 3dsx when the shape entry changes (VOLGEN).
## Gates of each tool row
1. `sh 3ds/tools/nd_host_test.sh`: ND HOST TESTS PASS. 2. One build. 3. `bench.py run spot_mac01_drec`, then
`bench.py cmp spot_mac01_drec --commit $(git merge-base HEAD main)`: 1/1 byte-equal. 4. The STAGE lines of a second build and a
second host run: each value in the target table or lower. The time counters are T1 host, T2 build, T3 base, T4 loop.
## Risks
- A wrong basebin gives a false byte-equal. Content key, file hashes on every use, seeded faults, cachecheck at every merge.
- A src edit misses gfx_tab and pso_tab (src is in their key: gen_gfx_tab.sh compiles with -I src). Until VOLGEN2 such a build
  runs gconv: GUESS 2-4 min. Correct before fast.
- The first build after VOLGEN is fully cold (new image-keyed ccache volume): GUESS 5-10 min, PM_BUILD_CAP=1800 in the page.
- The post-merge check runs detached (POSTASYNC): the merge lane waits GUESS 1-2 s. A ccache fault shows within 3 merges.
- Memory: 3 slots x 4 GB in 14 GB WSL2 (FACT .wslconfig). Do not add slots before VOLHOST.
