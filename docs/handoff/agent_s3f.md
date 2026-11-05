# Agent S3F: handoff

## STATE
branch: nds1 | HEAD at write time: see git log (row file e84cca737)
done: nothing committed for S3F-1..4 yet. Analysis only.
S3F-1 blockers: matrix words (model.c gSPMatrix at finalMtx, two sites) need a 3-word record: w0 G_MTX flags, w1 finalMtx, hdr (header w1). Feasible. Lights (model.c gSPSetLights0/N at ~1539): consumer must emit the same lit state. Word encoding under the nd gbi bridge (3ds/include/nd_gbi.h, nd_gbi_leaf.h) not verified; calling those macros from the consumer may touch nd_st. Needs a decision on the light encoding.
S3F-2..4: not started. Row 4 spots: bench spot_mac01_pic, spot_mac00_pic, spot_sbk56_pic (other session, BENCH4). Not in bench list when this was written.
Azahar starts used on S3F: 0.
next: decide the light path (encode via F3DEX2 macros with nd bridge off, or a direct light record), then S3F-1 matrix record first.
