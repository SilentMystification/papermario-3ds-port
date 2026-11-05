# trace_wall_tile: hos_01 wall at frame 200 (WALL, branch native)

## Result (short)
- PROVEN (Azahar log, this build): the wall is chunk #27, texture 0x010026ac (I4 64x64, slot 8), not 0x0100169c.
- PROVEN: the draw state is tile 0 = cms 1, cmt 1, masks 6, maskt 6 (MIRROR, 64 texel window). The PICA bind is wrap 3 3 (MIRRORED_REPEAT). The bind is not stale.
- PROVEN: all 5 triangles of chunk #27 stay inside the 64 texel window. UV s 12.000 to 35.999, t 0.000 to 63.999 (texels). The mirror is not used except the band t 63.999 (linear weight about 0.001).
- PROVEN: the N64 result (mirror, mask 6, UV inside 0 to 64) equals the PICA result. Wrap cannot change the wall picture. This explains the TEXD3 finding (wrapov=repeat: no measurable change).
- PROVEN: the notes that say "chunk #27 tile is cms 0, cmt 0, masks 5" are wrong. The dd tile line is printed one chunk late (see Finding 2).
- NOT PROVEN: the console-only difference. This trace does not show the console output. Candidate cause, INFERRED only: texture upload timing (section "Candidates").

## Build and run
- Debug edits (not committed, reverted): tex_op op log (pm_gbi.c, `tx f<frame> op ...`), per-triangle `uvt` line in tri_dd_log (pm_gbi.c), `pm_tex_dbg_state` (pm_tex.c). Diff kept at `scratchpad/wall_debug.diff`.
- Build: lane native, PM_ROW=WALL. Start 1 of 8 (WALL budget). Scenario hos01_texword (default, no switch), cap 25 s. Log: build3ds/lane-native/last_log.txt (copy in scratchpad `wall_run1_log.txt`).
- Scenario texwrap_wall was not used: it adds `wrapov=repeat` (changes the bound wrap) and has no drawlog. The default state is needed.
- Result of the scenario: `texword bind 0100169c` not found. Expected, because the address is not in this build (Finding 1). Shot at frame 204 (build3ds/azt/shots/hos01_texword/0000_5_1.ppm): blue wall, large, on the left and centre of the top screen. Read by an agent, not by a person.

## Finding 1: the texture address moved
- `0x0100169c` is a pointer into TextureHeap (.bss). TextureHeap is at 0x00fff9a4 in build3ds/pm_3ds.map (Nov 3) and at 0x010019e4 in build3ds/lane-native/pm_3ds.map (Nov 4, with the debug lines). The address depends on the binary layout.
- In this build the wall is 0x010026ac (TextureHeap + 0xcc8). The identity of the texture must come from the chunk (draw order, size, screen area), not from the address.
- Not proven that the debug lines caused the move. The move is proven by the two maps.

## Finding 2: the drawlog tile line is one chunk late
- `dd_line` (pm_gbi.c:309-316, prints `tst1.tile_wrap[0]`) runs in `dd_end` (pm_gbi.c:174). `dd_end` runs when the next chunk starts (G_VTX slot 0, pm_gbi.c:2888 and 3949).
- The `tw` value on `dd f200 #N` is the tile state at the start of chunk N+1. Proof: `dd f200 #27` (log #633) shows tw 0 0 5 5. The tile ops of chunk 28 (log #611 to #614: cms 0, cmt 0, masks 5, maskt 5) are the state of that line. Chunk 27 has cms 1, cmt 1, masks 6, maskt 6.
- The per-draw state of the uvt line is correct. Use `uvt` lines, not `dd ... tw`, for tile state.

## Finding 3: wall frame 200, every draw (chunk #27)
Ops (log line numbers from build3ds/lane-native/last_log.txt of this run):

| Log | Op | Words | Decoded | Effect |
|---|---|---|---|---|
| #587 | G_SETTIMG (0xfd) | fd900000 010026ac | fmt 4 (I), siz 2, addr 0x010026ac | source |
| #588 | G_SETTILE (0xf5) | f5900000 07058160 | tile 7, cms 1, cmt 1, masks 6, maskt 6 | load tile only, no set_wrap |
| #589 | G_LOADBLOCK (0xf3) | f3000000 073ff200 | tile 7, 1024 texels | TMEM load |
| #590 | G_SETTILE | f5800800 00058160 | tile 0, fmt 4, siz 0, cms 1, cmt 1, masks 6, maskt 6 | pm_tex_set_wrap(1,1,6,6): wrap_s/t = MIRRORED_REPEAT (3 3), mk 64 64 |
| #591 | G_SETTILESIZE | f2000000 000fc0fc | tile 0, uls 0, ult 0, lrs 63, lrt 63 | window 64x64 |

- Triangles: 5 (tri0 to tri4), 15 vertex lines (log #595 to #609). `use_t1 0`, `cur_tile 0`, `slot 8`, `tw 64 th 64`, `wrap 3 3`, `shr 0 0`, `mk 64 64`, `mirror 1 1` on every line.
- Texture bound to PICA unit 0: slot 8 (0x010026ac). Filter: 1 (GPU_LINEAR) from the BIS1 notes, not printed in this run.
- UV (texels, normalised u times 64, from poly u, v): s 12.000 to 35.999, t 0.000 to 63.999.
- Other chunks at frame 200 (by draw state): the overall UV range is s -176.466 to 204.809, t -16.000 to 340.620 (chunk 28 and others, repeat with a 32 window). Max magnitude below 1024 texels.

## Finding 4: wrap comparison for the wall
- Tile cms 1 (mirror), mask 6: N64 result = texel (s mod 128, reflected above 64). UV inside 0 to 64 gives identity. The PICA mirror (MIRRORED_REPEAT on a 64 texel texture) gives identity too.
- Wrap 2 (REPEAT) would differ only at the band t 63.999 to 64 (sample of row 0 instead of row 63), linear weight about 0.001. No visible change. This matches the TEXD3 picture result.
- Window 32 (mask 5): not relevant for this wall. Mask 6 is the tile state.
- Precision: UV max 64 texels = 0.99998 normalised. PICA float24 step near 1.0 is about 1.5e-5, about 0.001 texel. No precision issue at this UV range. The G_TEXTURE scale is 0xffff (not 0x10000), which gives the 63.999 values. The N64 uses the same scale.

## Candidates for the console-only garble (NOT PROVEN)
1. Texture upload timing. `upload_vram_slot` (pm_tex.c:551-557) flushes the staging buffer (pm_fl_dc) and calls C3D_TexLoadImage. The copy runs on the GPU. Azahar completes it at once. If the first draw of slot 8 reads the VRAM before the copy ends, the console shows old or mixed texels. Check: the frame of the upload (`up_frame`, pm_tex.c:46) against the first draw of slot 8 (frame 200 here).
2. I4 to GPU conversion (fmt 4, siz 0 to the slot format). Same code in Azahar. Not a console-only difference unless the hardware layout differs from Azahar.
- Neither candidate has a test in this trace. Test 1 needs a console run (not Azahar).

## Fix proposal
- No change to the wrap code. The bound wrap for the wall is the tile wrap, and the N64 result is the same.
- Do not add a wrap switch or a stale-wrap fix for this wall (TEXD1 and TEXD3 lines are not the cause for this draw).
- Next step (for a code agent): check candidate 1 with a console run (drawlog on frame 200 and the upload frame of slot 8). Keep the uvt line as the tile source.

## Commands
- Log excerpt: scratchpad `wall_run1_log.txt` (full log of run 1).
- Decoder: scratchpad `dec.py` (tx lines to readable form), `chunks.py` (per-chunk UV range and state).
