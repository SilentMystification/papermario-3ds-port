# Row HOSTCORE2 (correction 2): golden draw records on the device at nd_native_submit, not a host build of core 0 (Sonnet)

Model: Sonnet (plan v3, accepted). Reason: the record content (template words, texture and vertex bytes behind physical
addresses) is a design choice in shared emit code. Why not Opus: one hook, one format (HOSTCORE1), one bench file (FRAMEHASH4 form).

## Why the boundary changed (ORACLE8, main e2e8260ba, facts of the coder of the first try)
- Every draw of the game reaches the GPU through one function: nd_native_submit (pm_gpu.c:1206). The old walker path goes
  flush_draw (:1579) -> nd_raw_native -> nd_native_submit. The native paths call it directly (:1318 pm_gpu_native_submit_draw,
  :1426, :1443, :1564, :4434). Only nd_emit_draw and blk_put write draw words, both inside it (:1231). No C3D_DrawElements call
  exists in 3ds/src.
- So the old code and the converted code of a game file meet at the NdDraw of nd_native_submit. A record there proves equal
  draws for both. The words after it (nd_emit_draw, blk_put) do not change in a conversion row: FRAMEHASH (device words) and
  nd_emit_test (host) cover them.
- A host build of core 0 (the first design) needs the link closure of pm_frame_walk: 335 undefined symbols after 20 files,
  31 citro3d calls, DVLB parsing, pm_gpu_init state. The device has all of it. Cost of the device form: one Azahar run per
  capture (warm about 20 s) instead of a host run.

## Design (fixed. The coder chooses the inner steps)
1. Debug switch `drec=<first>:<count>` (pm_debug_value, read once, as ndref at pm_gpu.c:487-570): for the game frames
   first..first+count-1, nd_native_submit fills one NdDrec (3ds/include/nd_drec.h) per draw and appends its text
   (nd_drec_write semantics, 3ds/tools/nd_drec.c: move the writer to 3ds/src/nd_drec.c so the device links it) into a RAM buffer.
   At the window end one file sdmc:/3ds/PaperMario/shots/drec.txt and one pm_log_evt entry `drec f<last> draws <n> bytes <b>`.
   Off: one predicted-false test. It reads, it never changes a draw.
2. Record content, no address anywhere: regs = the template words of d->tmpl (nd_tmpl_words) with the stage colours of
   d->tev_col put in as nd_emit_draw does (nd_emit.c:44-61), the texture unit words (dim, param, lod, type, addr 0), and the
   float uniforms nd_emit_draw would write (palette rows, uvso, cmul, cadd, zmix, uvx, nzo) as pseudo registers that the coder
   names in the commit. tex[u] = the level 0 texel bytes of each unit on (address from d->u[u].addr << 3, physical to virtual).
   vtx = the 16-byte vertices of the index range min..max of the draw (the vertex base of the submit: ring chunk or static area,
   pm_gpu.c:1216-1218). idx = the index bytes. Template ids are not written (they are LRU slots).
3. bench: shot_names (bench.py, FRAMEHASH4) also lists drec.txt, cmp compares it line by line and names the first differing
   draw block (`drec draw <seq> DIFF`). azt.ps1 already copies every new file of the shots folder (azt.ps1:426-433).
4. A scene with the switch: spot_mac01_drec (spot_mac01_det with `drec=200:2` in place of the shots), per-row files
   3ds/tools/azt.d/HOSTCORE2.json and bench.d/HOSTCORE2.json, kind azahar_pic.

## Places
3ds/src/pm_gpu.c: nd_native_submit (one call at its end) and a new cold function block next to ndref (:487-570). New
3ds/src/nd_drec.c (the writer moved from 3ds/tools/nd_drec.c, which then includes it or is deleted: say which).
3ds/tools/bench.py: shot_names and the cmd_cmp loop. Per-row azt.d and bench.d files.

## Gate (expected values)
- Build, memcheck OK. nd_host PASS (drec test of HOSTCORE1 unchanged).
- `bench.py base spot_mac01_drec` stores (two runs equal, so the record is deterministic). The same run at this row's head vs
  its merge-base: `bench.py cmp spot_mac01_drec --row HOSTCORE2` equal (this row changes no draw).
- Seeded fault (shown first, then removed): one vertex byte of the record changed in the writer for draw 0: cmp names draw 0.
- Budget 8 Azahar starts.

## After this row (oracle)
HOSTCORE3 becomes the per-group capture scenes (fxtest per effect id, spot scenes), GMHARN and GMCOV become COVMAP coverage per
file on those scenes, GOLD rows store drec bases. plan_endgame.md section 5 and 6 text changed with this correction.

## Stop
A draw whose content the record cannot reach without an address, two base runs that differ (name the first differing draw), or a
cmp difference at head vs merge-base: stop, commit, hand back (2 tries per named cause). Delete this row file at the end.
