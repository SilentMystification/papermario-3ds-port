# plan_buildspeed: a row gate loop under 10 min (oracle BUILDSPEED, main 4ee5f9508c, 2026-11-16)
Read-only design. BENCHSPEED (agent a84e568e389461aaf) owns stage timers and shared caches in 3ds/build.sh,
3ds/tools/nd_host_test.sh and 3ds/tools/bench.py. Every row below that edits one of these 3 files starts after Merge BENCHSPEED.
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
| own build: sync code only, generator caches hit, ccache | 11 min | 1.5 min | GUESS: sync 8 s, compile-link 43 s FACT |
| base: built at merge, 2 Azahar starts per bench | 11-27 min x2 | 1 min x2 | FACT 16 s per warm run, GUESS start cost |
| gate: 2 runs + 2 cmp | 2 min | 2 min | FACT 46 s a run |
| patch, listcen, codemod | 1 min | 1 min | GUESS |
| total | about 70 min | about 7.5 min | GUESS. Pass: under 10 min, from BENCHSPEED STAGE lines |
## Rows (Haiku, oracle patch per row; the oracle writes each page after its wait ends)
- PAGEHOST (docs only, start now, no Haiku needed): pages run the host test 1 time, after the change. The orchestrator removes
  the baseline host step from docs/handoff/row_BATCHTEMPLATE.md. The oracle writes new pages with 1 host run. Saves 9 min (FACT).
- BASEBIN (3ds/tools/bench.py only; after Merge BENCHSPEED). (a) `bench.py base b1,b2` builds 1 time for all benches
  (cmd_base :1882 splits the names, base_build_run takes the list). (b) Keep the build: build3ds/basebin/<commit>/pm_3ds.3dsx and
  .elf in the main folder, with key.txt (the build.sh PM_KEY_ONLY key). base_build_run uses it and skips scratch_build. Keep the
  newest 6 (GUESS 1.7 GB). (c) `bench.py base --bin` stores the main folder build when HEAD is clean and the key is equal (the
  base_fill check, bench.py:1906-1930). Self-test: a basebin with a wrong key.txt: base refuses it and builds.
- MAINBIN (3ds/tools/merge_row.sh only; after Merge BASEBIN): after the merge commit, run `sh 3ds/build.sh` in the main folder,
  then `python 3ds/tools/bench.py base --bin`. Then every new row finds its merge-base build. Cost: 1 build per merge.
- VOLGEN (3ds/build.sh and a new 3ds/tools/src_sync.sh; after Merge BENCHSPEED). (a) Move the sync (build.sh:214-239) into
  src_sync.sh, run it before the generators. (b) One asset volume pm3ds-assets-<12 hex of the asset manifest>, shared by all lanes.
  The lane source volume holds code only. (c) The generator docker runs mount the source volume, not C:\. They write their outputs
  into the volume and copy them to the worktree (the key reads them, build.sh:158). Drop any part BENCHSPEED already did.
- VOLHOST (3ds/tools/nd_host_test.sh only; after Merge VOLGEN): sync with src_sync.sh, then run the docker block (:20-82) on
  the source volume and the asset volume. Keep the bind mount only for build3ds/out.
## Gates of each tool row
1. `sh 3ds/tools/nd_host_test.sh`: ND HOST TESTS PASS. 2. One build. 3. `bench.py run spot_mac01_drec`, then
`bench.py cmp spot_mac01_drec --commit $(git merge-base HEAD main)`: 1/1 byte-equal. 4. The STAGE lines of a second build and a
second host run: each value in the target table or lower. The time counters are T1 host, T2 build, T3 base, T4 loop.
## Risks
- A wrong basebin gives a false byte-equal. Key check plus the seeded-fault self-test. Store only a clean HEAD build.
- A stale asset volume. The volume name holds the manifest hash, so a changed asset makes a new volume.
- Memory: 3 slots x 4 GB in 14 GB WSL2 (FACT). Do not add slots before the host test leaves the slot pool (OPEN after VOLHOST).
- All rows except PAGEHOST wait for Merge BENCHSPEED. A BENCHSPEED cache that covers VOLGEN (c) removes that part.
