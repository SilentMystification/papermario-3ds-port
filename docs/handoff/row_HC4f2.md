# Row HC4f2: romfs shape loader

(Model: Haiku; after HC3. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC4f2`; PM_ROW=PM_LANE=HC4f2 on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Touch only
3ds/CMakeLists.txt (ctr_create_3dsx(... ROMFS build3ds/<lane>/romfs)), 3ds/src/pm_main.c (romfsInit at boot), 3ds/src/pm_map.c (:391 call site).
## Work
1. Loader: read romfs:/shape/<name>.nds into the decode buffer. Add the buffer base to every reloc offset. The root comes from word 0.
2. Check switch shpchk: run the old path into the same buffer first, copy it to the heap, run the loader, memcmp. Log "shpchk <map> diff <n>".
3. When shpchk shows diff 0 on mac_01 and kmr_10, delete fix_shape and its helpers in the next commit.
## Gate
shpchk diff 0 on 2 maps (one run with a warp list). Then byte-equal spot_mac01_det and mdl5_iwa10_det.
