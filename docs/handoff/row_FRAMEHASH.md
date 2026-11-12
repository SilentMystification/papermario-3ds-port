# Row FRAMEHASH: a per-frame hash of what the draw core submits replaces the shot compare (no image anywhere)

Model: Sonnet (user order 2026-11-12: no screenshot comparisons). Role in plan_endgame.md v2: the gate of the large groups
(model, HUD, messages, imgfx, sprites, item entities) and of the phase ends; small and medium groups use golden masters.
- Why not Haiku: the hash must parse the PICA command words of a frame and replace every address word by the content it points to
  (vertex, index and texture bytes, render target role); which registers hold addresses and how large each referenced block is are
  design choices in the GPU submit code.
- Why not Opus: the frame's command words are already collected in one place (the INTERPB3 K copy: kc_base, the GX entry record
  kc_take, kc_build pm_gpu.c:2894-2933); this row reads them, it changes no draw.

## Facts (ORACLE7 code read, main 1278b4f02)
- Every draw reaches the GPU as PICA command words in the citro3d command buffer plus GX queue entries (process list 0x01, memory
  fill 0x02, display transfer 0x03, texture copy 0x04). pm_gpu.c: frame begin records `kc_base` (:2804-2805); kc_take (:2863)
  copies the GX entries queued since the last take; the frame end pm_gpu_end_w (:2964-3022) runs flush, sr_replay, kc_build, then
  C3D_FrameEnd. The words of one frame are [kc_base, kc_base + coff) plus the lists named by the 0x01 entries (kc_build checks
  every list lies inside that range, else the frame had a buffer split: cmd_split_n).
- Addresses in the words differ between two builds even when the picture is the same: attribute buffer base and offsets,
  index buffer, texture unit addresses, colour and depth buffer locations, command list jumps. Pointer values never mean the picture.
- Shots today: bench.py base/cmp on azahar_pic benches store and compare build3ds/azt/shots ppm files (store key: commit, bench,
  scen_hash; bench.py:1405-1450, cmp :1893).

## Design (fixed; the coder chooses the inner steps)
1. Debug switch `fhash` (pm_debug_has, read once; the det scenes set it). Off: one predicted-false test at the frame end, nothing
   else (hw_guidelines: no hot-loop cost). It is a debug tool: it reads, it never changes a draw.
2. With fhash on, kc_take records every frame (as kc_rec does for fps60; add `|| fh_on` at :2805, no other K-copy change). At the
   frame end, before C3D_FrameEnd, a cold function `fh_frame()` walks the frame's command words in submit order (the 0x01 lists in
   queue order, then the open tail), decodes each PICA command (header: id, mask, count, consecutive: the kc_build decode), and
   feeds a 64-bit FNV-1a with (register id, mask, value) for every written register, except address registers, which feed instead:
   - attribute buffers (GPUREG_ATTRIBBUFFERS_LOC and ATTRIBBUFFERi_OFFSET/CONFIG): at each draw command (DRAWARRAYS / DRAWELEMENTS),
     the bytes of every enabled buffer for the vertex range the draw reads (count and first from the draw registers, stride from
     the buffer config; for DRAWELEMENTS the min..max index of the index bytes);
   - the index buffer: the index bytes of the draw;
   - texture units 0-2 (TEXUNITi_ADDR1, the size and format registers): the texel bytes of level 0 (width x height x bits / 8);
   - colour and depth buffer locations: the role (main top, main right eye, off-screen slot k by pm_gpu's slot table), not the address;
   - command list jumps and the GX entries: the entry kind and its sizes; a fill or copy feeds its value, size and target role.
   Per frame one entry `fhash f<frame> h <16 hex> draws <n> words <w>` (pm_log_evt). With `fhashdraw=<frame>` also one entry per
   draw of that frame `fhd f<frame> d <i> h <16 hex>` (the hash state after the draw), to name the first differing draw.
3. bench.py: new kind `azahar_hash`. `base` stores the fhash entries of the scenario window as fhash.txt in the base store (same
   key: commit, bench, scen_hash; two runs must give equal files, else the base is not stored: as for shots). `cmp` prints per frame
   `f<n> equal` or `f<n> DIFF`, then `<k>/<n> frames equal`, and for the first differing frame tells the user to rerun with
   `fhashdraw=<n>`. No ppm, no image file. The window: the scenario's `fhwin=<first>:<count>` switch (the game logs only those frames).
4. Scenarios: one hash twin per byte scene, inserted right after it: `spot_mac01_hash` (spot_mac01_det's debug with `shots
   shotwin=200:6` replaced by `fhash fhwin=200:6`, until `fhash f205 `), `mdl5_iwa10_hash`, `spot_mac00_hash`, `fxpic_63_hash`,
   `ent_kmr03_hash`, `msgbox_talk_hash`. One commit for scenarios and benches.

## Readers and writers
Writers: pm_gpu.c (fh_frame at the frame end, the kc_rec condition), bench.py (kind azahar_hash in base, cmp, base --fill),
azt_scenarios.json, bench_defs.json, docs/handoff/test_catalog.md (one paragraph: the hash kind replaces shots). Readers: bench.py
cmp; ingest keeps the `fhash` entries out of the prof metrics.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- Determinism: `bench.py base spot_mac01_hash` stores (two runs equal: 6 frames). The same at a second commit that changes no draw
  code (this row's own head vs its merge-base): `bench.py cmp spot_mac01_hash --row FRAMEHASH`: 6/6 equal. That proves addresses are
  masked (the two builds have different link addresses).
- Sensitivity, by code only: name in the commit which draw inputs reach the hash (registers, vertex, index, texel bytes, targets).
- Budget 8 Azahar starts.

## Stop
Two base runs of one build that differ (a word not masked: hand back the first differing draw with fhashdraw), or a head vs base
difference with no draw change: stop, commit, hand back (2 tries per named cause). Delete this row file in the completing commit.

## After this row (the orchestrator; the user confirms the CLAUDE.md text)
- Gates that change from "byte set" to "frame hash set": every row that runs `bench.py cmp` on an azahar_pic bench: plan_endgame.md
  (phase gates and S9GATE: "the DIRECT byte set" becomes "the DIRECT hash set: spot_mac01_hash, mdl5_iwa10_hash, spot_mac00_hash"),
  S1TSTEN, S7ENT, S7EFX7, S7EFX8, S6E3, N64DEL1-3, NPSO, CONV2, NXF, S9DEL1-5, S9GATE. Rows written later use the hash benches only.
- Proposed CLAUDE.md rule 2 text: "2. NO VISUAL CHECKS AND NO SHOTS. No agent looks at a shot, describes a picture, counts pixels or
  compares images. Proof is the code read, nd_host and the frame hash compare (`bench.py base <bench>_hash`, then `bench.py cmp
  <bench>_hash --row <row>`) on at most 2 scenes that use the changed shared draw code. The user checks the look on the console."
