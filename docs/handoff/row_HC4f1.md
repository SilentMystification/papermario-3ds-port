# Row HC4f1: host shape converter (lists still words)

(Model: Haiku. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC4f1`; PM_ROW=PM_LANE=HC4f1 on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Touch only
3ds/tools/nd_shape_conv.c (new), 3ds/build.sh.
## Work
- Host C, built by build.sh with the host gcc as texkey_bake.py is run. It reads ver/us/baserom.z64.
- Copy, adapted to host memory and big-endian reads: find_asset (pm_map.c:405-449), yay0_decode_raw (:297-345), fix_shape and fix_node/fix_group/fix_props/fix_gfx/fix_strlist/swap_* (:46-296).
- in_shape gives buffer offsets, not pointers. Each pointer field the code writes is recorded in a reloc list as (offset, value is a shape offset).
- Output: romfs/shape/<asset>.nds = { "NDSH", version 1, size, n_reloc, reloc[n_reloc] (u32 offsets), data[size] (native LE, pointer fields hold shape offsets) }.
- Do this for every table entry whose name ends in _shape.
- Report line: "shapeconv maps N bytes B reloc R".
## Gate
build.sh prints the line. No Azahar start.
