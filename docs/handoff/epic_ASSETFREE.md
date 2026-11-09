# Epic ASSETFREE (parked: NOT to be started; user, 2026-11-09)

User: a future epic removes the game assets from the ELF. The console makes them on first boot from the user's own ROM.
Do not start, plan or write rows for it until the user says so.

Known facts for that time:
- Today build.sh compiles ROM-derived data (assets/us, ver/us/build) into the binary, so every built 3dsx is a personal build.
- HC4 (2026-11-09) converts the display lists at build time: S1 by the gs* macros in nd_gbi.h, S2 by the host tool
  3ds/tools/nd_shape_conv.c into romfs. Both are host C with one encoding source (nd_op.h), so the converter can later run on the console.
