# ws_DO0 (Haiku): GPU state hash at each native draw (gst.txt) and its compare in bench.py: the gate tool of DRAWOBJ
Designer: oracle NATIVE17 (plan_drawobj.md section 4). User rule: the user reads plan_drawobj.md before any DRAWOBJ coder starts.
Worktree build3ds/wt/DO0, branch DO0. PM_ROW=DO0 PM_LANE=DO0. Lines: main 5beccb695c or later; grep the quoted text.
Why Haiku: a closed parser spec, a file writer next to drec.txt, a compare and a seeded self-test. No draw changes.
Objective: in drec frames, after each native draw, one line `d <seq> h <hash>` in sdmc:/3ds/PaperMario/shots/gst.txt. The hash is the GPU register
state the draw runs with, addresses left out. Later rows that change the emitted words (DO3) must keep gst.txt equal.
## Facts (FACT code read)
- Native draws emit in place: pm_gpu.c:1312-1314 (`nd_emit_draw(cb + coff, &ndn_sh, d)`, then GPUCMD_SetBufferOffset). citro3d words (C3Di_UpdateContext)
  sit in the same buffer before them (:1307-1311). drec.txt opens at pm_gpu.c:615 and closes at the drec end (:628-636).
- Command format (nd_emit.c:8-15, the PICA format): word 0 the value, word 1 the header: reg = h & 0xffff, byte mask = (h >> 16) & 0xf,
  n = ((h >> 20) & 0xff) + 1 values, consecutive = h >> 31; then n - 1 more values; a pad word when n is even.
- bench.py shot_names (:1763-1766) lists the compared files; drec_cmp (:1795-1805) compares drec.txt line by line.
## Edits
1. pm_gpu.c, a cold noinline parser `gst_scan(const u32* a, const u32* b)`: walk the commands from a to b. Keep `static u32 gst_r[0x300]`
   (byte-mask merge: new = (old & ~m) | (v & m), m from the 4 mask bits), `static u32 gst_f[96][4]` (float uniforms: GPUREG_VSH_FLOATUNIFORM_CONFIG sets the
   index (low 8 bits) and the mode (bit 31: f32, 4 words per vec4; else f24, 3 words per vec4); DATA0-7 words fill gst_f in order and the index moves on).
   Other LUT data registers (fog, lighting, proctex: their INDEX and DATA0-7) go into one FNV per LUT that restarts when its INDEX is written.
   Left out (value kept 0): GPUREG_TEXUNIT0_ADDR1 to ADDR6, TEXUNIT1_ADDR, TEXUNIT2_ADDR, COLORBUFFER_LOC, DEPTHBUFFER_LOC, ATTRIBBUFFERS_LOC,
   ATTRIBBUFFER0_OFFSET + 3k (k 0 to 11), INDEXBUFFER_CONFIG, CMDBUF_ADDR0, CMDBUF_ADDR1, CMDBUF_JUMP0, CMDBUF_JUMP1 (names of <3ds/gpu/registers.h>).
2. pm_gpu.c:1312-1314: when drec_on, scan from the last scanned word to `cb + coff + nw_` (restart at cb when the buffer or a smaller offset shows a split),
   then hash FNV-1a over gst_r, gst_f and the LUT hashes and write `d %u h %08x` (drec_seq of this draw) to gst.txt.
   gst.txt opens next to drec.txt (:615) with a 64 KB heap stdio buffer and closes with it at the drec end.
3. bench.py shot_names: add "gst.txt" to the list. cmp: a gst.txt pair uses drec_cmp's line compare (its first DIFF names `d <seq>`).
   `bench.py selftest-gst`: 2 temporary files, equal gives PASS, one changed hash gives a DIFF. Print `selftest-gst PASS` or FAIL.
## Tests (build3ds/out/, last 20 lines). Gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=DO0 sh 3ds/build.sh`; c0gate 0 / 0 / 0. `python 3ds/tools/bench.py selftest-gst` PASS.
2. `bench.py run spot_mac01_drec,entcape_kmr03_drec --row DO0 --why "DO0 gate"`; `bench.py cmp <b> --row DO0` byte-equal (the base has no gst.txt: not compared).
3. The tip shot folders hold gst.txt with one line per drec draw (170 and 172 lines). Copy `wc -l` of both into the hand-back.
Fix rights: compile errors, register names (use the citro3d names; a missing name: its number from <3ds/gpu/registers.h>). A DIFF, a crash: stop, send the path to oracle NATIVE17.
Stop: last commit `DO0 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hash, host, selftest-gst, the 2 cmp lines, the line counts.
