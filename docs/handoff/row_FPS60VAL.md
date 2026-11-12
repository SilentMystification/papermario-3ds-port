# Row FPS60VAL: host validator of the 60 fps key frame copy (the patched command words that only real hardware rejects)

Model: Sonnet (plan_endgame.md G6 for one path; the PICA command format decode is a design choice in a new tool).
- Why not Haiku: the validator decodes PICA command headers, consecutive writes and the patch rule of kc_build; the checks are
  choices about what the GPU requires.
- Why not Opus: one data dump and one host tool; no draw code changes.

## Facts (ORACLE7 code read, main 5be532ef9)
- kc_build (pm_gpu.c:2894-2933) copies the frame's used command words [cb, cb + coff) to kc_buf (192 KB linear), refuses a frame with
  a buffer split, a list outside the range or more than KC_WORDS, then writes the K words of each patch (nd_kp): base = a command
  start, k = (header >> 20) & 0xFF the extra parameter count, words written at off (j = 0) and off + 1 + j (j >= 1) for j <= k.
- pm_gpu_key_show (:2936-2963) queues the recorded GX entries again (lists rebased into kc_buf, fills, VRAM copies) and copies the
  open tail (kc_tail_n words) into the K frame's command buffer when it fits (`coff + kc_tail_n < csz`, else the tail is dropped).
- What hardware requires and Azahar forgives: a command list address and size aligned to 16 bytes, each command starting on an
  8-byte boundary, a header's parameter count inside the list, a patch that writes only parameter words of its own command, the
  list ending with the finalize write citro3d adds at the frame end.

## Design (fixed; the coder chooses the inner steps)
1. Debug switch `kcdump=<frame>` (pm_debug_value, read once): at that frame's kc_build, after the patches, write to
   sdmc:/3ds/PaperMario/kcdump_<frame>.bin: a header (coff, kc_tail_w, kc_tail_n, kc_n, nd_kp_n), kc_buf words, the kc_ent entries,
   the patch list (base offsets, first, k words), and the K frame's final list words as queued by pm_gpu_key_show (taken after its
   C3D_FrameEnd from the command buffer range it used), then one pm_log_evt entry `kcdump f<frame> words <n>`. It writes, it never
   changes a draw.
2. Host tool 3ds/tools/kcval.py: decodes every list of the dump (header id, mask, count, consecutive bit; extra words padded to 8
   bytes), checks the five hardware rules above for the L copy and for the K list, checks every patch word index against its
   command's parameter range, prints `kcval lists <n> cmds <c> patches <p> findings <f>` and the first 20 findings by offset.
3. Self test: `kcval.py --selftest` builds lists in memory (one valid; one with a header count one too large and one patch past its
   command): it must report 0 and 2 findings.

## Files and places
3ds/src/pm_gpu.c (the end of kc_build and the end of pm_gpu_key_show: the dump calls only; FPS60W edits :1655, :1842, :2461, :2479,
:2789, :2941 and one new function above pm_gx_room: no overlap), new 3ds/tools/kcval.py.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS; `python 3ds/tools/kcval.py --selftest` prints 0 and 2.
- One Azahar run of a copy scenario `kcdump_mac01` (spot_mac01_fps60_det's debug plus `kcdump=210`, until `kcdump f210 `), then
  `python 3ds/tools/kcval.py <the dump copied from the slot sdmc>`: hand back the summary. findings 0: the copy is valid by the
  hardware rules (the cause is then outside the copy; FPS60W's dump names it on the console). findings above 0: the first finding
  is the cause by value; stop and hand back (the oracle writes the fix). Budget 4 Azahar starts.

## Stop
As above. Delete this row file in the completing commit.
