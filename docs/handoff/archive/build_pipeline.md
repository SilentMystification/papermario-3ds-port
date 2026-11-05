# Build pipeline: measurements (3ds/build.sh, lane pipe)

Host: Windows 11, Docker Desktop, 32 CPUs, 31.3 GiB for the Docker VM.
Branch dlc-compiler-c1c2. All times are wall seconds from the STAGE lines of `3ds/build.sh`.

## Changes in this lane

| Commit | Change |
|---|---|
| 11f2583cc | Sources in named volume `pm3ds-src` (`pm3ds-src-<lane>`). Sync copies changed files only. Bind mounts removed. |
| 6b8c17dbb | One `SRC_DIRS` list at the top, no exclude lines. |
| 3df385832 | `cmake --build --parallel $(nproc)`. |

Sync manifest: `build3ds/<lane>/src_manifest.txt` (path, size, mtime). A file is copied when size or mtime differs. A removed file is deleted in the volume. A missing source volume clears the manifest.
A git-tree stamp was not used. Ignored inputs (generated files) can change without git showing it.

## Stage table

| Run | sync | configure | compile-link | copy-out | total (build.sh) | notes |
|---|---|---|---|---|---|---|
| Cold (empty build, src, ccache volumes) | 17 s (24757 files) | 1 s | 102 s | 2 s | 126 s | exit 0. CPU 3304% to 4938% during compile (about 33 to 49 cores). Before the parallel change: 228%. |
| No change | 4 s (0 files) | 0 s | 0 s | 3 s | about 7 s | ninja up to date |
| One file (`pm_main.c`, `pm_build.h` touched) | 5 s (2 files) | 0 s | 20 s | 2 s | 29 s (wall 29.3 s) | link is the cost |
| Cache hit (build volume removed, ccache and src volume kept) | 5 s (1 file) | 1 s | 24 s | 2 s | 37 s | exit 0. Compile served by ccache, link about 17 s of the 24 s. |

Single-file compile, `pm_main.c`, run in the container on the source volume, ccache off: 674 ms, 688 ms, 680 ms (3 runs).
Single-file compile with ccache on the bind mount: not measured (bind mounts removed).

Branch switch: not simulated. The cache-hit row removes the build volume only. A real branch switch changes many files, and its time is not measured.

## Serial steps

| Step | Time | Action |
|---|---|---|
| Sync (host) | 4 s to 18 s | Under 5 s when few files change. |
| Configure | 0 s to 1 s | Already under 5 s. |
| Link (`pm_3ds.elf`) | 17.3 s | Over 5 s. One serial step. See options below. |
| 3dsxtool (3DSX) | 0.4 s | Under 5 s. |
| smdhtool | 3 ms | Under 5 s. |
| Copy-out | 2 s to 3 s | Copies a 240 MB ELF each build. Under 5 s. |
| Header generation (`pm_build.h`) | under 1 s | Written in build.sh before the sync. Not separately timed. |
| `gen_map_restore.py` | not in build.sh | Run by hand before a build (project rule). Not measured. |

## Link options tried (ELF link only, same build volume)

ELF size: 240190624 bytes with the default link. The size comes from DWARF in the objects built with `-g`.

| Option | Link time | ELF size | Result |
|---|---|---|---|
| Default | 17.3 s | 240190624 | kept |
| `-fuse-ld=gold` | fails | - | rejected. No ld for gold in the arm toolchain (`cannot find 'ld'`). |
| `-Wl,--threads` | fails | - | rejected. `ld: unrecognized option '--threads'`. |
| `-Wl,--no-keep-memory` | 17.6 s | 240190624 | rejected. No gain. |
| `-Wl,--compress-debug-sections=zlib` | 25.5 s | 171409608 | rejected. Slower. |
| `-Wl,--strip-debug` | 10.0 s | 17465840 | rejected. Removes DWARF. `3ds/tools/decode_dump.py` uses addr2line and needs it. |

Rule for experiments: a CMake `-DCMAKE_EXE_LINKER_FLAGS=` override replaces the toolchain value. The value must keep `-L/opt/devkitpro/libctru/lib -L/opt/devkitpro/portlibs/3ds/lib -specs=3dsx.specs`, or the link fails with `cannot find -lctru`.

Open: the link is 17 s on every one-file rebuild. A cut needs a change to the compile flags (`-g` on the objects) or a per-file split. Neither was tested. Both change debug output used by the crash-dump tools, so they need a decision.

## Commands

- Build lane: `PM_LANE=pipe PM_BUILD_CAP=900 sh 3ds/build.sh`
- Cold reset (measurement only): `docker volume rm pm3ds-build-pipe pm3ds-ccache pm3ds-src-pipe`
- Build volume only (cache-hit test): `docker volume rm pm3ds-build-pipe`
- CPU: `docker stats pm3ds-build-pipe --no-stream`
