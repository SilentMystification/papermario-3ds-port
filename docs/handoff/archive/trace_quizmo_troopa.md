# TRACE QUIZMO AND JR TROOPA (row QJT), 2026-11-03

Branch qjt (worktree papermario-3ds-port-qjt, from native ca4ea0f46). No source change, no debug hook committed.
Azahar starts used: 3 of 6 (QJT runs 1 to 3). Status file: 3ds/logs/perf/status_QJT.txt (untracked).
Binary: copy of the main folder build3ds/pm_3ds.3dsx (same commit ca4ea0f46). Switch: fxtest (src/pm_fxtest.c).
Shots: build3ds/shots/ in the worktree (untracked).

## Item 2: Jr Troopa transform, pink effect. PARTLY PROVEN, edge cause NOT PROVEN
- Trigger: src/battle/area/kmr_part_3/actor/jr_troopa.c:486 to 489. In PHASE_ENEMY_BEGIN, when HP is 1 or less, the script calls PlayEffect(EFFECT_GATHER_ENERGY_PINK) with type 0 and type 1, scale 1, duration 30. The same call is in egg, mage, para and final Jr Troopa files.
- Dispatch: src/evt/fx_api.c:422 to 423, then src/effect_loads.c:93 to 95, then src/effects/gather_energy_pink.c.
- Draw (gather_energy_pink.c, appendGfx): setup list D_E00269F8[type], geometry list D_E00269F0[type]. Type 0 uses setup D_09001138 (I4 64x64 texture D_09000400, wrap, 2 cycle, RM_CLD_SURF2) and geometry D_090010B0 (26 vertices, 16 triangles, source assets/us/effects/gfx/D_090010B0_33CC80.gfx.inc.c).
- Alpha: vertex alpha is 0 at the base ring (z 0) and 255 at the far ring (z -2000). Source: assets/us/effects/gfx/D_09000F10_33CAE0.vtx.inc.c. Alpha ramp in update: unk_24 rises over 10 frames and falls over the last 6.
- Texture coordinates: s runs to 8192 (1/32 texel units, 256 texels), 4 wraps round the ring at a 64 texel mask. Wrap is G_TX_WRAP, so a seam appears only if the 64x64 texture is not tileable at its edge (not checked).
- Scroll: tile 0 origin is unk_40 mod 256 (quarter texel). Tile 1 origin unk_38 grows by 96 per frame for type 1 (no wrap in update). The port reads SetTileSize uls and lrs as 12 bit fields: src/pm_gbi.c:3069 to 3071. Tex window clamp for negative starts: src/pm_gbi.c:2873 to 2885.
- Shot (run 2, fxtest=14 type 0, mac_01, Mario at -264 20 -326, spawn frame 402, shot a frame 412): build3ds/shots/0013_1_2.ppm. By eye: a ring of hard-edged magenta blocks with radial streaks. Zoom: scratchpad qjt_r2_f412_zoom.png. No smooth gradient read at the ring edge.
- Not proven: the 16 triangle draw of D_090010B0 is not in the frame 412 drawlog (chunks 69 to 183 only, no tri 16 line). No texel dump. Type 1 (the tile 1 growth path) was NOT run: a new switch entry needs a binary build, not done in this row.
- Run 1 (kmr_20, fxtest=14): no effect visible. Mario was not settled (spawn y -975). Shot shows yard only.

## Item 1: Chuck Quizmo stretched geometry. NOT REACHED (Quizmo scene not run)
- Quizmo effect run (run 3, fxtest=72 quizmo_stage at mac_01, same position, shot a frame 412): picture shows the purple wall and a flat dark block in the lower half. No stretched shape read by eye.
- A stretched triangle is in the frame 412 drawlog of run 2 and run 3: chunk tri 2, screen box -223 207 to 3740 8291, clip w range 5.29 to 291.43, texture timg 0x0100141c (16x16, fmt 2). The w value 5.29 is near the near plane, so the perspective divide blows up the box. It is a mac_01 world draw, not a Quizmo draw. Source file not mapped (display list address 012dffb0 in the drawlog).
- Not reached: the Quizmo NPC scenes (dro_01, dro_02, end_00, hos_03, mac_02) and the Quizmo show. The quiz start needs a hook or the NPC talk path. Lead from agent_grab3.md: stage light D_09001518_39A0D8 (SetTileSize 252 by 60, T clamp). Not seen in the mac_01 shot.

## Fix proposal (not applied, no code change in this row)
- Item 2: run fxtest for type 1 with a debug entry appended after gather_energy_pink in src/pm_fxtest.c:46 (uncommitted), drawlog on frame 412, and dump the tri 16 chunk. If the edge is the tile 1 origin (unk_38 grows without bound), the fix is a mod 256 on the origin in gather_energy_pink.c update (one line, src/effects/gather_energy_pink.c, near the unk_38 update). Test: type 1 shot on mac_01, picture of the ring edge.
- Item 1: find the source of the chunk at drawlog frame 412 (mac_01, dl 012dffb0). A near plane clip for tri 2 draws (pm_gbi.c or pm_gpu.c vertex path) is the candidate. Not proven.

## Budget
- QJT starts used: 3 of 6. Remaining: 3 (not used).

## Update 2 (coordinator follow-up)
Starts: 5 of 6 used (runs 4 and 5 below). Source edit: src/pm_fxtest.c, one entry E(gather_energy_pink_t1, fx_gather_energy_pink(1, X, Y, Z, 1.0f, 30)) after the type 0 line (uncommitted). Lane build qjt (PM_LANE=qjt, gen_map_restore run, table restored, not committed) contains the entry. Quizmo stage index is now 73.

### Item 2, type 1: NOT PROVEN (gate failed: effect not visible)
- Run 4 (mac_01, Mario at -264 20 -326, fxtest=15, spawn frame 402): shot a (frame 412, alpha at full) and shot b (frame 432) show no pink effect, full frame and 4x crop. Build3ds/shots/0013_1_2.ppm and 0015_1_2.ppm in the worktree.
- Drawlog at frame 412: no 16 triangle chunk and no 64x16 tile 0 chunk from the effect. Only 26 chunk lines (#125 to #183) are logged for the frame. The effect draw is not in the drawlog, so the type 1 path is not reached on the screen or it is drawn outside the logged list.
- Ruled out: scale. evt_get_float_variable (src/evt/evt.c:1944) returns a small raw integer as its value, so the scale argument 1 in jr_troopa.c:489 is 1.0.
- Type 0 (run 2) showed the effect, so the spawn position is valid. Type 1 uses perspective FOV 130 (gather_energy_pink.c appendGfx) and scale unk_20 = 1.0. Not checked further.
- Edge line: NOT PROVEN for either type.

### Item 1, Quizmo: NOT PROVEN (appearance not reached)
- Run 5 (dro_01 entry 0, pos=-400:0:130, 30 units from Chuck Quizmo at -400 0 100, shots 200 to 204, drawlog 200). Settled at the position (readback).
- Drawlog frame 200 (43 chunk lines): no triangle with clip w below 20 and no triangle with a screen box beyond the screen by a large factor that has near-plane w. Large boxes have w 150 to 860, which is normal off-screen floor.
- Picture: a purple figure at the left edge, not confirmed as Chuck Quizmo. Frame 200 is before the appearance. The quiz start was not triggered.
- The stretched chunk seen in runs 2 and 3 (mac_01, frame 412, tri 2, box to x 3740 y 8291, w 5.29) is still the only near-plane stretch found. It is not in dro_01.
- Census (NDT switch) was not run. The census switch name was not found in src (only cen log lines in pm_gpu.c:525).

### Next gates (not run)
- Item 2: a drawlog that logs all chunks of the frame (the current log shows only 26 per frame), then the chunk with tri 16 and timg of the effect. Needs a log change in pm_gbi.c (uncommitted).
- Item 1: a quiz start hook (talk to the NPC or the quiz event) on dro_01 near (-400, 0, 100), then the drawlog at the appearance frames.
- Budget: 1 start left (QJT).

## Update 3: Quizmo stage show, stretched triangle (PROVEN draw and vertex values, matrix cause PARTLY PROVEN)
Budget: QJT reset to 8. Starts used after reset: 7 (runs 6 to 12). Uncommitted debug (not in this commit): src/pm_fxtest.c (type 1 entry), src/pm_gbi.c (badw line in tri_dd_log), src/effects/quizmo_stage.c (qmtx line at the left wall matrix). Entries already in pm_fxtest.c: quizmo_stage 73, quizmo_audience 79, quizmo_assistant 121 (no new entries needed for them).

### Run design (mac_01, Mario at -264 20 -326, fxtest spawns the effect at Mario, same as Items 1 and 2)
- Run 6: quizmo_stage, drawlog 412, census (drawcensus 412). Census summary: 63 batches, 186 tris, 49 distinct, 0 over.
- Run 7: no effect (baseline), drawlog 412, census: 58 batches, 170 tris, 44 distinct.
- Run 8: quizmo_audience (79). Adds 8 chunks, all clean: clip w 273 to 290, boxes inside the screen (e.g. scr 32 111 74 188).
- Run 9: quizmo_assistant (121). Adds 1 chunk, clean (w 343).
- Run 6 minus run 7, key without chunk index (src, dl, timg): 6 chunks only in the stage run, all at src 004e0640 to 004e07c0 (dl 004dba10 parent, timg 004db810 32x32 I4, 004db550 64x16 CI, 004dcfb0 64x64 CI, 004da270 64x16 IA).
- Chunks #13 and #14 of run 6 (frame 412): tri 4 each, clip w range -7.66e6 to 5.86e6 (w sign flips inside the triangle). Chunk #15 (same parent, src 004e0640, tri 2) is fine (w 287 to 436).

### Run 10: per vertex dump of the bad triangles (frame 412, badw log, pm_gbi.c tri_dd_log)
- Chunk #13, tri 1: vertex (1834953, 1987322, -3991221, w -7652123); vertex (-1402742, -1770414, 3059798, w 5858979).
- Chunk #14, tri 2: vertex (-1403200, -1485473, 3050518, w 5850100); vertex (1952068, 2272300, -4000438, w -7660943).
- 24 bad vertex lines on frame 412, all in chunks #13 and #14. Clip x, y, z reach 4e6 and w reaches 7.7e6 in magnitude. The triangles straddle the w = 0 plane, so the screen projection blows up (the drawlog box of 125 to 150 px is the clipped result, not the true extent).

### Run 11: frames 402 to 431 (drawlog 402:30)
- Logged frames with bad triangles: 404, 407, 411, 414, 418, 421 to 431. All logged frames show chunks #13 and #14 with 12 bad vertex lines each. The drawlog did not print lines for frames 402, 403, 405, 406, 408, 409, 410, 412, 413, 415 to 417, 419, 420 in this window (logging gate not resolved). So the bad triangles persist over the whole life of the stage run that was logged.

### Identity of the two draws (display lists, assets/us/effects/gfx)
- Chunk #13 = D_09006F48_39FB08 (8 vertices, vertex source D_090064E8_39F0A8, 4 triangles). Called by D_09006FB0_39FB70 (left wall), which is drawn at src/effects/quizmo_stage.c:145 after the matrix D_09004148_39CD08[0] at src/effects/quizmo_stage.c:139 and the left wall rotation at lines 141 to 144.
- Chunk #14 = D_09006EB8_39FA78 (8 vertices, vertex source D_09006568_39F128, 4 triangles). Called by D_09006F20_39FAE0, drawn at src/effects/quizmo_stage.c:153.
- Both lists load the texture D_090017B8_39A378 (32x32 I4, wrap, mask 5, texture D_090015B8_39A178). This matches timg 004db810.
- Vertex data is sane in the ROM asset: coordinates within plus or minus 150 (assets/us/effects/gfx/D_090064E8_39F0A8.vtx.inc.c, D_09006568_39F128.vtx.inc.c). So the bad clip values come from the matrix chain, not from the vertex data.

### Run 12: the matrix (frame 404 to 413, qmtx log)
- guMtxL2F of D_09004148_39CD08[0] (the matrix used by the left wall at quizmo_stage.c:139): row 0 (-1, 0, 0, 0), row 1 (0, 256, 0, 0), row 2 (0, 0, 0, 256), row 3 (0, -31745, 256, 16640). Same on every logged frame.
- An identity-like matrix has 1.0 where these rows have 256.0 (row 1 col 1, row 2 col 3, row 3 col 2). The value 256 equals 0x0100, the byte-swapped form of 0x0001. The -1 (0xFFFF) does not change under a byte swap, which fits the pattern. Hypothesis: the port reads this ROM Mtx with the integer halves byte-swapped (asset or read path). NOT PROVEN: no ROM bytes were checked (no base ROM in the repo). Each multiply by a matrix with a 256 scale multiplies the coordinates by 256 again, which is consistent with clip values near 6e6 after two or three multiplies (not computed).
- Run 12 gave no dd or badw lines (the drawlog gate did not fire on frame 412 in this run). The matrix lines are from the same run.

### Verdict
- Stretched triangle, PROVEN: chunks #13 and #14 at frame 412 (and on logged frames 404 to 431), D_09006F48 (left wall, quizmo_stage.c:145) and D_09006EB8 (quizmo_stage.c:153). Clip w up to 7.7e6, vertex sign flip across w = 0.
- Bad value, PARTLY PROVEN: the left wall matrix D_09004148_39CD08[0] holds 256.0 where 1.0 belongs. Source: ROM Mtx read path (byte order of the integer halves, hypothesis). Not proven.
- Lead the coordinator named (stage light D_09001518, 64x16 CI, tile 252x60): the stage light draw is chunk #17 (timg 004db550 64x16 CI, tile 0 window 0 0 252 124). Not stretched (w 295 to 298). The handoff T clamp claim is not confirmed: the log shows cmt mode 1 (mirror), not clamp.

### Fix proposal (not applied, no source commit)
1. Check the Mtx read path of D_09004148_39CD08 (the effect blob, 0x09004148 in effects/gfx). Compare the integer halves with the N64 layout (big endian). Test: a shot where the left wall is visible (stage raise with leftWallRaiseAmt 255 and the wall in view), or a log of the 16 int halves as raw u16 (1 start).
2. If the Mtx halves are byte-swapped in the asset, fix the converter for this blob (one row of the asset tool), and test the stage show at frame 412 with the badw count at 0.
3. Next gate: 1 start left (QJT). Not used.

## Update 4: why the effect Mtx reads 256.0 (code reading, no Azahar start)
Starts: none in this update (QJT used 7 of 8 after the reset, 1 left).

### 1. Where the blob comes from
- Symbol: src/effects/gfx/quizmo_stage.c:31, declared `char D_09004148_39CD08[] = { ... }` (256 bytes, comment "what's this?"). It is not typed as Mtx.
- Located inside the gfx section of the splat, not a separate blob: ver/us/splat.yaml, effect_gfx_quizmo_stage, entry `[0x39CC68, gfx, D_090040A8_39CC68]` covers ROM 0x39CC68 to 0x39CE08. The Mtx starts at ROM 0x39CD08 (offset 0xA0 in that gfx) and is 0x100 bytes: four Mtx of 64 bytes each.
- The splat has no matrix type. The gfx type writes Gfx arrays. The raw bytes were written as a char array by hand (no converter writes them). Other effect data in the same file uses typed arrays (vtx, gfx, ci4, ia4), which the C compiler lays out correctly.
- Only the quizmo effect refers to this symbol: `extern Mtx D_09004148_39CD08[]` in src/effects/quizmo_stage.c:6. Four uses: gSPMatrix at lines 128 ([2]), 139 ([0]), 148 ([1]), 161 ([3]). Only one `extern Mtx` exists in src/effects (count 1).

### 2. Port read path (proof of the byte order)
- G_MTX handler: src/pm_gbi.c:1846 to 1847 (flags, `Mtx* src = ptr_of(w1)`), then load_mtx calls unpack() at src/pm_gbi.c:1869 to 1880 region.
- ptr_of: src/pm_gbi.c:417. A segment 0x09 address resolves to segbase[9] plus offset. The blob is read in place, no copy.
- Element read: src/pm_gbi.c:440 to 447 (elem). It reads host u32 words. For element (row, col): word idx = row*2 + col/2, integer half = high half for even col, low half for odd col; the fraction half is at word idx+8. On a little-endian host the u32 is assembled from bytes in little-endian order.
- Runtime matrices: guMtxF2L (src/pm_gu.c:38 to 49) writes host u32 words: integer word = (e1>>16)<<16 | (e2>>16), fraction word = (e1<<16)|(e2 & 0xffff). elem() reads that layout correctly. guMtxL2F (pm_gu.c:55 onward) uses the same reader.
- Static literals: RDP_MATRIX (include/macros.h:272 to 290, _RDP_PACK_WHOLE at line 262) packs the same host word layout (word = whole(A) << 16 | whole(B)). Static matrices in draw_box.c:327, main_loop.c:36 and model.c:600 use it, so they are correct.
- The effect blob is N64 big-endian bytes in a char array. On a little-endian host each 32-bit word has its two halves swapped, and each half is read little-endian. That gives exactly the logged floats.

### 3. Proof with the first 64 bytes (from src/effects/gfx/quizmo_stage.c:32, print from the source)
```
00: 00 00 FF FF 00 00 00 00 00 01 00 00 00 00 00 00
10: 00 00 00 00 00 01 00 00 FF 83 00 00 00 41 00 01
20: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
30: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```
- Read as N64 big-endian 16-bit integer halves (bytes 0-1 = column 0, 2-3 = column 1, and so on, row by row), Mtx[0] integer parts are: row 0 (0, -1, 0, 0); row 1 (0, 1, 0, 0); row 2 (0, 0, 1, 0); row 3 (-125, 0, 65, 1). Fraction halves (bytes 32-63) are all zero.
- Read as the port reads now (elem() on the same bytes, emulated in Python, script in the scratchpad mtx_check.py): row 0 (-1, 0, 0, 0); row 1 (0, 256, 0, 0); row 2 (0, 0, 0, 256); row 3 (0, -31745, 256, 16640). This equals the run 12 qmtx log line exactly.
- The value 256 is 0x0100, which is the little-endian reading of the bytes 00 01. The value -31745 is the reading of FF 83. The value 16640 is the reading of 00 41.
- Fix check (same script): reverse the bytes of each 32-bit word, then run elem(). Mtx[0] becomes row 0 (0, -1, 0, 0), row 1 (1, 0, 0, 0), row 2 (0, 0, 1, 0), row 3 (-125, 0, 65, 1). All four matrices become unit rotations (entries 0, +1, -1) with clean translations.
  - Mtx[1] after fix: (0,1,0,0), (-1,0,0,0), (0,0,1,0), (125,0,65,1).
  - Mtx[2] after fix: (1,0,0,0), (0,-1,0,0), (0,0,-1,0), (125,0,-85,1).
  - Mtx[3] after fix: (1,0,0,0), (0,0,-1,0), (0,1,0,0), (-50,0,10,1).

### 4. Other effects with the same read
- Only this blob is read as a Mtx by an effect (see section 1). Other raw char/u8 blobs in src/effects/gfx (17 files, e.g. ice_pillar.c:14, partner_buff.c:8) are textures or vertex-like data, not gSPMatrix sources.
- Static matrices used by gSPMatrix outside effects: D_8009BAA8 (curtains.c:220, BSS, written by guMtxF2L at runtime, correct) and gBoxMatrix, MasterIdentityMtx, ReferenceIdentityMtx (RDP_MATRIX, correct).
- Runtime matrices from guMtxF2L (camera, billboard, dust, emote) are correct by the same reader.

### 5. Fix location (not applied, source not committed)
- Primary: src/effects/gfx/quizmo_stage.c:31. Replace the char array with `Mtx D_09004148_39CD08[4]` built from the four matrices above (RDP_MATRIX, or a guMtxF2L-compatible word layout). The four matrices above are in the port's [row][col] convention used by elem().
- Do not change elem() (src/pm_gbi.c:440). It is right for runtime matrices. Changing it would break every guMtxF2L matrix.
- Structural (later): the splat gfx path does not know Mtx. The gfx section at ver/us/splat.yaml 0x39CC68 needs a type that emits Mtx for 0x39CD08 (four 64-byte blocks), or a byte-swap step in the converter for that range.

### 6. Uncommitted debug edits (list, nothing reverted)
- Keep: src/pm_fxtest.c (type 1 entry, index 15; it is the only way to spawn that path without a hook).
- Keep until the fix is tested: src/effects/quizmo_stage.c:140 (qmtx log line; it shows the matrix after the fix).
- Revert after the fix test: src/pm_gbi.c (badw line in tri_dd_log). It prints every bad vertex and costs log space.
- Untracked, not committed: assets/ (copy of main assets/us), build3ds/ and lane binaries.

## Update 5: fix applied and tested (run 13, last QJT start, 8 of 8 used)
- Fix: src/effects/gfx/quizmo_stage.c, the raw char array at line 31 replaced by `Mtx D_09004148_39CD08[4]` with the 64 big-endian 32-bit words of the ROM bytes. elem() (src/pm_gbi.c:440) is unchanged.
- Debug edits reverted: pm_gbi.c badw line, src/effects/quizmo_stage.c qmtx line.
- Kept (committed): src/pm_fxtest.c type 1 entry, appended at the end of FXLIST (index 127). Existing indexes are unchanged: gather_energy_pink 14, quizmo_stage 72 (was 73 in earlier runs), quizmo_audience 78, quizmo_assistant 120.
- Test: fxtest=72 quizmo_stage, frame 412, drawlog 412 and drawcensus 412 (run 6 settings). Frame 412 logged: no chunk with clip w below 0 or above 1e4. The chunks that had w -7.66e6 to 5.86e6 (run 6, src 004e07c0) now show w 266.79 to 436.21. Other stage chunks: w 287 to 436 (src 004e0640), 295 to 298 (src 004e0700).
- Shot: docs picture before (run 12 shot 0013, left) and after (run 13 shot 0013, right): build3ds/shots/0013_1_2.ppm after. Scratchpad: qjt_before_after_f412.png. Before: a dark brown block covers the lower left of the frame (the stretched wall). After: the block is gone, the room floor and a striped stage edge show.
- Other raw arrays in src/effects/gfx (53 total): the splat sections are palette 31, gfx 21 (inside gfx sections), vtx 1 (ice_pillar.c:14 D_09001198_40B358, 16 bytes shown, no consumer found). None of them is named in effect code or in a gfx list. They are reached by segment address (texture loads), not by gSPMatrix. Only quizmo D_09004148 is read as a matrix. Texel byte order for the texture loader is not checked here.
