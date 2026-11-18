# Build pipeline (BUILDO; edit in place, 60 lines max)
Entry: `sh 3ds/build.sh` (lane: PM_LANE=<row>). Output: build3ds/lane-<row>/pm_3ds.{3dsx,elf,map}. Stage lines: `STAGE <name> <s>`.
## Stages, in order
1. key-inputs (3ds/tools/build_key.py pre, one python process, no docker run except one image inspect):
   map_restore files, code ids (in_manifest: git blob id, else hash-object), image ids, asset ids (stat cache, git index
   rule), ROM sha, generator cache lookups (a hit copies the outputs), shape key, build key, skip decision.
2. gen: only the misses run, at the same time, under one build slot: texkey (python) | shapes (host image, into the
   docker volume pm3ds-shape) | gfx_tab then pso_tab (host image). A hit costs no process.
3. key: the build key equals build_key.txt and the 3dsx hash equals build_out.sha1: "build up to date", exit. No slot.
4. slot-wait: build_slot.sh, 6 slots (PM_BUILD_SLOTS). The slot number k names the workspace.
5. One container in workspace k (volumes pm3ds-wssrc-ws<k> = /src, pm3ds-wsbuild-ws<k> = /work): asset-check (exit 7),
   romfs from /sh/<shape key> (exit 8), sync (exit 9 on an id mismatch), configure (only on a new build number, flag or
   path set), ninja -j16 with ccache (one volume per build image), copy-out. Exits 7-9 come before any write to /src.
6. tail: elf copy to 3ds/logs/builds, memcheck line, hotset line (detached).
## Keys (every key is a content hash; every use checks key.txt and out.sha1)
| item | key | store |
| code sync | path + content id per file, against the slot manifest | build3ds/wsmeta/ws<k>.{manifest,id}, /src/.pm_manifest_id |
| texkey | tool, python version, ROM, asset ids | build3ds/gencache/texkey |
| shapes | command, host image, ROM, ids of the files inc_scan.py lists | volume pm3ds-shape /sh/<sha1>, marker gencache/shapevol |
| gfx_tab, pso_tab | K0 (command, image, assets, path list) + ids of the traced reads | build3ds/gencache/{gfx_tab,pso_tab} |
| build | code ids of KEY_IN, generator outputs, asset ids, shape key, image ids | lane build_key.txt + build_out.sha1 |
| base build | commit tree, build number, assets, ROM, images | build3ds/basebin (bench.py) |
## Cold gate (PM_CACHE=off, post-merge only)
New workspace, asset and shape volumes (cold<pid>), no ccache, no cache read or write, all generators. The compile runs
during the generators (phase 1 with empty generator files of an old mtime, phase 2 with the real files). The cold asset
volume is a copy of the shared one only when every file has the blob id of a fresh hash of the host files.
Proof of equal bytes: `PM_LANE=<l> PM_BUILD_REV=<c> PM_CACHE=off sh 3ds/build.sh` with the old and the new tools: same elf, same 3dsx.
## Rules for a tool change
1. Never change the container paths /src and /work: the elf holds them (debug info), so the bytes change.
2. A new input of a step goes into its key. Never key on a time, a path or a commit id alone.
3. A new cache entry is a temp folder and one rename. A failed check removes the entry and the step runs again.
4. A step that can fail on a cache goes before the first write to /src (exits 7, 8, 9), so the slot manifest stays true.
5. Do not add a shell loop over a file list in the key phase: put it in build_key.py (FACT: 715 traced commands, 13.5 s).
6. Never name a docker volume pm3ds-src-<x> or pm3ds-build-<x>: older build.sh copies remove those (lane cap loop).
7. Host tests use their own slot pool (build3ds/hostslots, PM_HOST_SLOTS 4). They never take a build slot.
8. Self-tests: 3ds/tests/host.d/buildkey.sh, romfscache.sh, gtin.sh, genkey.sh, volgen.sh. Gate of a tool change: cold
   build bytes equal before and after, host tests PASS, binchk OK, one bench byte-equal, 3 builds at once.
9. Disk: `sh 3ds/tools/wt_gc.sh --apply` (VOLSWEEP) removes lane volumes, cold volumes over 2 h and old asset volumes.
## Times (FACT, BUILDO runs 2026-11-18, other coders built at the same time)
| case | before | after |
| repeat, no change | 25 s | 4 s |
| new lane, 1 file changed (warm slot) | 106 s | 18 s |
| new lane, first use of a slot | 106 s | 24-50 s (full sync 3-5 s, 3400 ccache hits 11-14 s) |
| cold PM_CACHE=off | 266 s | 93 s |
| key-only (bench.py run build_stale) | 12 s | 4.4 s |
| scratch tree of a base commit | 13.5 s | 2.3 s |
