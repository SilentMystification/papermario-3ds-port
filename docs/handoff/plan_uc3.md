# plan_uc3: replay of plain opaque records (row UC3, epic SR, branch uc2c)

Plan only (Opus). Base HEAD 5409fc17b on branch uc2c (source of 0ecfddd28 plus docs). No source edit, no Azahar start,
no git change except this file. 3ds/src/map_restore_table.c not read.
Inputs: agent_uc2c.md, plan_uniform_cut.md, plan_record_capture.md, plan_record_reuse.md, plan_stateful_records.md (d, e, g),
plan_dl_design.md (section 1 numbers), hw_guidelines.md, trace_uc2_vs_gdx3ds.md (read from commit df71384b5, branch
uc2-percall: the file is not in the uc2c tree), and the code: 3ds/src/pm_gbi.c, pm_gpu.c, pm_dlc.inc.c, pm_tex.c, src/model.c.
research_3ds_gpu_repos.md is not in the uc2c tree (commit ebba79176). Its replay summary is in trace_uc2_vs_gdx3ds.md section 1.
Tags: CONFIRMED (file:line read for this plan), INFERRED (not read, or a reasoned result). Line numbers are for HEAD 5409fc17b.

Acronyms: UC = uniform cut rows (UC1 capture shape, UC2 reccheck, UC3 replay). RecKey = per-model record key. dlc = display
list compiler (static map shape lists). gidx = per-frame index ring. gstat = static vertex area. pal = matrix palette uniform.
pcomp = palette matrix (post matrix x game matrix). ctx = citro3d context. PICA = 3DS GPU command set. TEV = texture
environment. L1 = level 1 cache. GX = GPU command queue of the GSP (graphics service). LRU = least recently used.

## 0. Result first

| Point | Decision |
|---|---|
| (a) indices and vertices | Copy the indices once, at capture, into a persistent linear index arena. rec_done rewrites the 0x227 word of the stored record to point at that arena. Replay writes no index, runs no grange and touches no gidx word. Vertices need no work: a record holds only static-area draws (gstat), and the gstat area holds still while gstat_gen (in the record gen word) holds still. |
| (b) split into draws | The split is frozen at capture. Replay copies the record words verbatim, so each C3D_DrawElements of the capture frame is one draw at replay. Replay does not need per-draw data: every draw of a record uses one game matrix (refuse otherwise), so one pal value patches every pal slot. |
| (c) RecDraw store | No per-draw RecDraw. RecKey grows from 52 B to 64 B (aligned 32, two cache lines) with the per-record replay inputs. The patch table (4 B per vec4) and two exit words go into the word arena after the body. |

Replay route per model: rec_try (group start) arms the replay. mg_run runs the group words as now (render mode, texture
bind, G_MTX, G_POPMTX). At the one G_DL of the group, a new branch copies the record instead of dlc_try. So the shape list
walk (dlc_try groups, box tests, grange index copy) and every flush_draw of the model are cut. The CPU state of the walker
and of pm_gpu stays equal to the old path, because the same group ops still run.

## 1. Facts from the code

### 1a. Where indices and vertices come from in a record

- A record is the words between pm_gpu_rec_begin and pm_gpu_rec_end. rec_try is called before mg_run and rec_done after it
  (pm_gbi.c:3668 to 3671) CONFIRMED.
- A static map shape list goes dlc_try (pm_gbi.c:3588, pm_dlc.inc.c:452) to dlc_static_group (pm_dlc.inc.c:506) to dlc_seg
  (pm_dlc.inc.c:231) to pm_gpu_grange (pm_dlc.inc.c:241). pm_gpu_grange is called only there (grep) CONFIRMED.
- pm_gpu_grange copies `idx` (the persistent static index list g->sd, heap, built once per map at pm_dlc.inc.c:186 to 225)
  into gidx at gidx_n (pm_gpu.c:2156, 2180) CONFIRMED. So the index values are a fixed function of the map list. Only their
  place in gidx changes per frame. gidx is reset each frame (pm_gpu.c:1825 `gidx_n = 0; gi_start = 0`) CONFIRMED.
- The static vertex indices point into the gstat area (GRING_VTX and up, pm_gpu.c:2112 to 2117). That area is written once
  per map and rewritten only after pm_gpu_gstat_reset, which bumps gstat_gen (pm_gpu.c:2125) CONFIRMED. gstat_gen is in the
  record gen word (pm_gbi.c:3434) CONFIRMED. Every static vertex has palette flag 0 (pm_dlc.inc.c:199) CONFIRMED, so a static
  draw reads pal slot 0 only.
- The ring path (pm_gpu_gtrip pm_gpu.c:2036, pm_gpu_gappend pm_gpu.c:1972, pm_gpu_gvtx_load pm_gpu.c:2189) writes per-frame
  vertex copies and palette flags into gring (pm_gpu.c:2089, 2096). A record with a ring draw cannot replay. Today the
  capture does not refuse it CONFIRMED (no rec_cap test in gtrip or gappend).
- dlc_try skips a group whose box is out of view (pm_dlc.inc.c:481 to 488) CONFIRMED. The record of a capture frame then
  lacks that group. A later frame with another camera needs it. reccheck does not see this: it compares words, and 0x228
  (count) equal does not prove equal index content. INFERRED.
- The 0x227 word (index buffer config) holds the physical index address minus the buffer base, with bit 31 = u16 type.
  reccheck masks it as a per-frame address (pm_gpu.c:2409) CONFIRMED. Its layout is INFERRED from citro3d C3D_DrawElements.
- The gidx span of one record is contiguous: rec_begin flushes (gi_start = gidx_n after flush_draw, pm_gpu.c:1091), and inside
  the window only grange and gtrip append to gidx. With gtrip refused, the span is [gidx_n at begin, gidx_n at end). INFERRED
  from pm_gpu.c:1091, 2156, 2180.

### 1b. How the window splits into draws today

- A draw closes at each flush(): a change of the batch inputs (pm_gpu.c:2161 to 2162), a G_PM_TEX or render mode change
  through the pm_gpu setters, the rec_end flush (pm_gpu.c:2346) CONFIRMED. Consecutive static groups with equal inputs merge
  into one draw (pm_gpu.c:2154 to 2159, C3 merge) CONFIRMED.
- One DrawElements per draw (pm_gpu.c:1083) CONFIRMED. rec_end refuses when the draw count is not the raw draw count
  (pm_gpu.c:2349) CONFIRMED.
- reccheck bad 0 on yard_rec and mac01t_rec_chk window f307 (agent_uc2c.md section 3) shows equal headers, equal counts (0x228
  is not masked) and equal draw count across frames with an equal key CONFIRMED (log, not code).
- Each draw re-sends pal slot 0 under the forced shape (pm_gpu.c:920 to 921, 965 to 977) CONFIRMED. In a grange batch
  pal_n = 1 and pal_g[0] = gm (pm_gpu.c:2169) CONFIRMED.
- gm comes from dlc_gmi (gv_matrix_id(0)) once per list for unlit groups (pm_dlc.inc.c:502) CONFIRMED.

### 1c. What rec_done keeps today

- RecKey (pm_gbi.c:3398): ver, gen, hgen, tgen0, tgen1, off, len, state, nb, s0, s1, grp[7]. Sum 52 B, not aligned to 32
  CONFIRMED (field sum). The record words go to `ra` (heap, pm_gbi.c:3426, 3524) CONFIRMED.
- The patch table rec_pt (pm_gpu.c:2356 to 2380) holds the last walk only. rec_done does not store it (pm_gbi.c:3519)
  CONFIRMED. No RecDraw exists (grep) CONFIRMED.

### 1d. State that a verbatim copy leaves stale (the gdx-3ds lesson)

- citro3d sends a state group only when its dirty bit in ctx+0x20 is set (agent_uc2c.md section 4, trace_uc2_vs_gdx3ds.md
  section 1) CONFIRMED in those docs. A replay writes GPU registers behind citro3d. After it, the GPU holds the record values
  and ctx holds the old values with clean bits. gdx-3ds re-arms the dirty flags after a replay (C3Di_DirtyUniforms, a body
  dirty mask, DrawUsed) (trace_uc2_vs_gdx3ds.md section 1) CONFIRMED in that doc.
- C3D_BindProgram sends the program config only when ctx->program changes INFERRED (citro3d source not read). rec_begin sets
  bound = 0 (pm_gpu.c:159, 2329), so flush_draw calls C3D_BindProgram(&prog4) (pm_gpu.c:959). If ctx->program is prog4
  already, the record has no program config. That record then is not self-contained INFERRED. prog and prog4 share one
  DVLB, so a switch sends no shader code (pm_gpu.c:1260) CONFIRMED.
- pm_tex keeps a slot from LRU eviction in a frame only when the slot epoch is the frame epoch (pm_tex.c:655, TOUCH
  pm_tex.c:71) CONFIRMED. The old path touches it in pm_tex_bind (flush_draw pm_gpu.c:993). Replay skips flush_draw, so it
  must touch s0 and s1 itself, else a later load in the same frame can reuse the VRAM that the replayed words sample INFERRED.
- A slot that draws from its linear stage copy has another address (pm_tex.c:627 to 631) and no sgen bump at upload
  (pm_tex.c:541 to 550) CONFIRMED. A record captured or replayed while a slot stages has a wrong texture address INFERRED.
  This explains the 0x85, 0x95, 0x9D mask of UC2 (pm_gpu.c:2413) INFERRED.
- kick_check splits the frame at a draw count (pm_gpu.c:573 to 586) CONFIRMED. Replay must add its draws to draws_frame.
- C3D_DrawElements sets DrawUsed. citro3d sends the framebuffer flush at a target change or split only when DrawUsed is set
  INFERRED (trace_uc2_vs_gdx3ds.md cites citro3d base.c:166 to 168).

## 2. Decisions with data layout

### 2a. Indices: one copy at capture, into a linear index arena

Who writes, at capture (pm_gpu.c, new function next to pm_gpu_rec_end):
- `static u16* ia; static u32 ia_n; static int ia_hold;` `ia = linearAlloc(REC_IDX_N * 2)` once, REC_IDX_N = 65536 (128 KB
  linear). Bump only, no in-place reuse: the GPU of a split part can still read a region (KICK1). Full: the capture refuses.
- pm_gpu_rec_begin stores `rb_gi = gidx_n` (after its flush). pm_gpu_rec_end stores `rb_ge = gidx_n`.
- `int pm_gpu_rec_idx(u32* w, u32 n)`: w is the record copy in `ra` (never the live command buffer). cnt = rb_ge - rb_gi.
  Return -1 when ia_n + cnt > REC_IDX_N or ia_hold. memcpy(ia + ia_n, gidx + rb_gi, cnt * 2).
  `d = osConvertVirtToPhys(ia + ia_n) - osConvertVirtToPhys(gidx + rb_gi)`. Walk w (same loop as pm_gpu.c:2406 to 2416):
  each non-incremental 0x227 command gets `w[i] += d`. Count them in nd. pm_fl_dc(ia + ia_n, cnt * 2) once. ia_n += (cnt + 7) & ~7.
  Return nd. The caller refuses when nd != the raw draw delta of the window.
- `void pm_gpu_rec_idx_reset(void)`: ia_n = 0. Called by rec_reset (pm_gbi.c:3424). When a replay ran in this frame, set
  ia_hold = 1 instead (captures refuse until pm_gpu_begin clears it), so a reset does not overwrite indices that this
  frame's command list still reads.
- Layout: u16 per index, sequential, 16 B aligned per record. Written once per capture. Read only by the GPU.

Who reads, at replay: nobody on the CPU. The GPU reads ia through the patched 0x227 word.

Index content check (reccheck): `int pm_gpu_rec_ichk(const u32* rec, u32 nr, const u32* live, u32 nl)`. First 0x227 p0 of
both: the record span address is `PhysToVirt(Phys(gidx + rb_gi) + (p0_rec - p0_live))` (bit 31 equal in both). memcmp
cnt * 2 bytes against gidx + rb_gi. A difference counts `ibad` on the rec line. This proves (a) on real frames.

Box skip: dlc_try does not skip an out-of-view group while a capture or check window is open (pm_dlc.inc.c:481:
`if (!rec_on && dlc_box_out(g, m))`). The record then holds every group of the list. The GPU clips the rest. Pixels are
equal INFERRED (the byte-equal gate checks it). `static int rec_on;` must be declared before pm_gbi.c:1558
(`#include "pm_dlc.inc.c"`).

Ring refuse: `if (rec_cap) rec_ref = 1;` at the top of pm_gpu_gtrip (pm_gpu.c:2036) and pm_gpu_gappend (pm_gpu.c:1972).

### 2b. Split: frozen at capture, copied verbatim

- Replay copies `len` words with GPUCMD_AddRawCommands (libctru, INFERRED: the code agent greps
  /opt/devkitpro/libctru/include/3ds/gpu/gpu.h in image pm3ds-build:1. Fallback: GPUCMD_GetBuffer, memcpy, GPUCMD_SetBufferOffset).
- Room: GPUCMD_GetBuffer(&b, &size, &off). Refuse the replay when off + len + 512 > size (512 words slack for the split and
  FrameEnd words, INFERRED).
- One game matrix per record. Capture refuses when a draw has pal_n != 1 or pal_g[0] differs from the first draw of the
  window (flush_draw rec_cap block, pm_gpu.c:920). Capture refuses a second dlc list in one group (`rec_dln > 1`, counted in
  dlc_try when rec_on).
- Patch at replay: pcomp of the live gm into every pal slot vec4 of the record. All other uniform data words stay as captured.
  This needs uvso, cmul, cadd, zmix and uvx to be key-pure (R in plan_uniform_cut 1a). Row UC3a proves it (reccheck with a
  pal-only mask). If UC3a fails, the plan stops and the failing uniform index is the next investigation.
- Exact bytes: pm_gpu_rec_play calls C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, u4_pal, &pcomp[gm]) and copies the 16 B of
  C3D_FVUnif[0][idx] into each slot with u4_pal <= idx < u4_pal + 4. split = 1: words off, off+2, off+3, off+4. split = 0:
  off to off+3 (pm_gpu.c:2354 to 2373 layout) CONFIRMED layout, INFERRED that citro3d sends C3D_FVUnif words in memory order
  (same assumption as plan_uniform_cut 2d step 3).

### 2c. Store: RecKey 64 B plus the arena tail (no RecDraw)

RecKey, `static RecKey rk[256] __attribute__((aligned(32)));` 16 KB, one model reads 2 lines in order:

| Offset | Field | Bytes | Written at capture by | Read at replay by |
|---|---|---|---|---|
| 0 | ver, gen | 4 + 4 | rec_done (as now) | rec_try (as now) |
| 8 | hgen, tgen0, tgen1, off, len | 2 x 5 | rec_done (as now) | rec_try, rec_play |
| 18 | state, nb | 1 + 1 | rec_done | rec_try |
| 20 | s0, s1 | 2 + 2 | rec_done | rec_try, rec_play (touch, stage test) |
| 24 | grp[7] | 28 | rec_g0_set at capture start | rec_try (after its flush) |
| 52 | npt | 1 | rec_done (walk result; refuse > 255) | rec_play |
| 53 | nd | 1 | rec_done (pm_gpu_rec_idx result) | rec_play (draws_frame) |
| 54 | fl | 1 | rec_done: bit 0 = (pm_gpu_gv_wind() <= 0), bit 1 = tri_since_vtx at dlc_try exit | rec_play |
| 55 | pad | 1 | | |
| 56 | geom_in | 4 | dlc_try entry `geom` when rec_on (pm_dlc.inc.c:470) | rec_play compare with live geom |
| 60 | view | 4 | rec_done: `pm_gpu_view_key()` = hash of sci_x0, sci_x1, sci_y0, sci_y1, pm_wide, ui_ortho, off_active | rec_play compare |

Arena `ra` per record: `[body len words][npt RecPatch, 4 B each][geom_out][pad]`, 16 B aligned. need = len + npt + 1.
geom_out = geom after dlc_try exit (pm_dlc.inc.c:523) when rec_on. RecPatch moves from pm_gpu.c:2356 to 3ds/include/pm_gpu.h.
Reuse of the old place (pm_gbi.c:3521) tests need, not n.

Why not the plan_uniform_cut 2c RecDraw: use5 is refused (pm4 only), pal_n is 1, pal_g is one gm computed live, ntab is
pm5 only, tex1 and cullm are fixed by the key and the words (cull word: ct from geom_in, sign from fl bit 0).

Run end, inside pm_gpu_rec_play after the copy (the gdx-3ds re-arm):
1. C3D_FVUnifDirty[0][pt[e].idx] = true for every entry (citro3d resends what it thinks the registers hold).
2. ctx+0x20 |= REC_DIRTY_END = REC_DIRTY_CAP | DrawUsed.
3. raw1_reset(), sh_clear() (pm_gpu.c:159, 510): pm_gpu shadows, FRAGOP_CLIP (direct write), texture bind shadow.
4. draws_frame += nd, pm_dc_c[DC_RAW] += nd, kick_check().

Capture start (pm_gpu_rec_begin, pm_gpu.c:2328): ctx+0x20 |= REC_DIRTY_CAP after sh_clear. REC_DIRTY_CAP = AttrInfo | BufInfo
| Effect | Scissor | Program | Tex(0..2) | TexEnv(0..5) (and TexStatus if it is its own bit). Every group the record can write is
then in the record (the record is self-contained, as gdx MarkBodySelfContained). pm_gpu_grp_get (pm_gpu.c:2342) ORs the same
mask into g[5], so capture and replay compare the same value. Bits: Effect = bit 4, Tex(n) = bit 23 + n, TexEnv(n) = bit 26 + n
are in agent_uc2c.md and the trace (INFERRED there). The others are INFERRED from the citro3d enum. The UC3b agent confirms all
bits first, read only, no start:
`MSYS_NO_PATHCONV=1 docker run --rm pm3ds-build:1 sh -c 'B=/opt/devkitpro/devkitARM/bin; cd /tmp; $B/arm-none-eabi-ar x /opt/devkitpro/libctru/lib/libcitro3d.a base.o; $B/arm-none-eabi-objdump -d base.o > base.txt; grep -n "tst\|bic" base.txt | head -80'`
(library path INFERRED. If absent: `find /opt/devkitpro -name libcitro3d.a`).

Foreign registers: pm_gpu_rec_walk (pm_gpu.c:2362) returns -1 (wbad) for a write to the viewport, framebuffer and TEV update
buffer (fog mode) registers. The agent reads the numbers from libctru 3ds/gpu/registers.h (GPUREG_VIEWPORT_*,
GPUREG_FRAMEBUFFER_*, GPUREG_COLORBUFFER_*, GPUREG_DEPTHBUFFER_*, GPUREG_TEXENV_UPDATE_BUFFER, GPUREG_FOG_*). These hold
target or fog state that the key does not hold.

## 3. Replay path (functions and lines)

pm_gbi.c:
- `static RecKey* rec_rp;` set by rec_try, cleared in case G_PM_MODEL right after mg_run (pm_gbi.c:3670 to 3671), on every path.
- rec_try (pm_gbi.c:3442 to 3446): key equal, state 1, reccheck off: `pm_gpu_flush(); rec_g0_set();` equal grp: `rec_rp = k;
  return 0;` (no capture). grp differs: rec_mg++, then capture as now.
- mg_run case G_DL (pm_gbi.c:3582 to 3591): before `if (dlc_on && dlc_try(next))` add `if (rec_rp && rec_play(next)) break;`.
  One load and one branch per G_DL in mg_run. rec_play is noinline (hw_guidelines rule 1, 10).
- `static __attribute__((noinline)) int rec_play(const Gfx* l)`: refuse (rec_rpx++, return 0, dlc_try runs) when
  geom != geom_in, fl bit 0 != (pm_gpu_gv_wind() <= 0), pm_interp_on > 0, view differs, pm_tex_rec_use(s0) or
  pm_tex_rec_use(s1) fails, or gm = dlc_gmi() is GV_NONE. Then `pm_gpu_rec_play(...)`. On 1: dlc_set_geom(geom_out),
  memset(vert_live, 0, sizeof vert_live), dlc_dirty = 1, tri_since_vtx = fl bit 1, ep_open = 0 (the exit of dlc_try for a
  static list, pm_dlc.inc.c:509, 523 to 525), rec_runs++, return 1.
- rec line (pm_gbi.c:3533): add `runs <n> rpx <n> ibad <n>`.

pm_gpu.c:
- `int pm_gpu_rec_play(const u32* w, u32 n, const RecPatch* pt, unsigned npt, unsigned nd, const float (*tab)[4][4],
  unsigned gm, const float vp[4])`: refuse when off_active or no room. flush(). `if (!draw_3d) draw_3d = 1;`. vp as
  pm_gpu.c:2172 (rebuild_3d when it differs). pal_tab = tab. `if (pcomp_gen[gm] != post_gen) pcomp_calc(gm);`
  (pm_gpu.c:796). C3D_FVUnifMtx4x4 (2b). Copy. Patch. Run end (2c). Return 1.
- vp comes from pm_gbi vp_sx, vp_sy, vp_tx, vp_ty (the ep_p.vp source, pm_gbi.c:1294) CONFIRMED.

pm_tex.c:
- `int pm_tex_rec_use(int s)`: s == REC_NO_SLOT (passed as -1): 1. Slot not live or slot_draws_stage (pm_tex.c:537): 0.
  Else TOUCH(&slots[s]) and 1.
- `int pm_tex_slot_staged(int s)` for the capture refuse (flush_draw rec_cap: a bound slot that stages refuses the record).

## 4. Rows (each at most 60 changed lines, one code agent, branch uc2c, in order)

Baseline shots: C:\Users\Andrew\AppData\Local\Temp\claude\uc2c\base_yard_rec (top 0000_0_11) and base_mac01t_rec (0000_1_2,
0001_1_2), HEAD 617540255 (agent_uc2c.md section 3). If they are gone, take the HEAD pair again (2 starts, -Try).
Every row: PM_ROW=<row>, the justification line in 3ds/logs/perf/status_<row>.txt before each start, build with
`python 3ds/tools/gen_map_restore.py`, `PM_LANE=UC3 sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c.
Start: `PM_ROW=<row> PM_LANE=UC3 powershell -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build UC3 -Try`.
Revert rule (all rows): a gate quantity fails or a top shot is not byte-equal: `git checkout -- <row files>` before the next
stage. One fix, one more shot of the same quantity. Two unchanged results end the series. bad > 0 or ibad > 0: a fresh
investigation agent names the register or the index span from the UC2D, UC2L and rec lines. No code agent from a guess.
Commit the row files when the gate passes (never map_restore_table.c).

| Row | Files | Lines | Change | Gate (starts) |
|---|---|---|---|---|
| UC3a pal-only mask | 3ds/src/pm_gpu.c (pm_gpu_rec_same 2400 to 2416) | about 10 | Mask only the data words of patch entries with u4_pal <= idx < u4_pal + 4 * GPAL, and the 0x227 param0. Delete the 0x85, 0x95, 0x9D mask. Measurement row: proves that uvso, cmul, cadd, zmix, uvx and texture addresses are key-pure | mac01t_rec_chk window f307 and yard_rec: bad 0, miss 0, wbad 0, hit > 0. Top shots byte-equal (no draw change expected). bad > 0: stop the epic plan, investigation names reg and idx (2) |
| UC3b self-contained capture | pm_gpu.c (rec_begin 2328, grp_get 2342, gtrip 2036, gappend 1972, flush_draw 920 to 923 and after 994, rec_walk 2362), pm_gbi.c (rec_on before 1558, set in rec_try, cleared in rec_done), pm_dlc.inc.c 481, pm_tex.c (pm_tex_slot_staged) | about 40 | Read-only bit check first (2c command). REC_DIRTY_CAP at rec_begin and in grp_get. Refuse ring draws, a second matrix, off_active, a staging slot, a slot change on unit 0 or 1 between draws, foreign registers. No box skip while rec_on | mac01t_rec_chk, yard_rec: bad 0, miss 0, wbad 0, hit > 0, build > 0; top shots byte-equal to the baseline (2) |
| UC3c index arena | pm_gpu.c (rec_begin, rec_end, pm_gpu_rec_idx, pm_gpu_rec_idx_reset, pm_gpu_rec_ichk), pm_gbi.c (rec_done 3519 to 3527, rec_check 3491 to 3499, rec_reset 3424, rec line), 3ds/include/pm_gpu.h | about 50 | 2a: linear arena, copy and 0x227 rewrite in the `ra` copy, nd test, ibad check in reccheck, ia_hold | mac01t_rec_chk, yard_rec: bad 0, ibad 0, hit > 0, build > 0; top shots byte-equal; boot memory line shows the 128 KB linear alloc succeeded (2) |
| UC3d record tail | pm_gbi.c (RecKey 3398, rk alignment, rec_done, dlc_try entry and exit lines pm_dlc.inc.c 470 and 523, rec_dln), pm_gpu.c (pm_gpu_view_key, pm_gpu_rec_pt accessor), pm_gpu.h (RecPatch) | about 45 | 2c: RecKey 64 B, patch table and geom_out after the body, npt, nd, fl, geom_in, view. Refuse npt > 255 and rec_dln != 1. Records still not drawn | mac01t_rec_chk: bad 0, ibad 0, build > 0 (the new refusals lower build: report build and ref against UC3c); top shots byte-equal (1) |
| UC3e replay | pm_gbi.c (rec_rp, rec_try 3442 to 3446, G_PM_MODEL 3670, mg_run G_DL 3582, rec_play, rec line), pm_gpu.c (pm_gpu_rec_play), pm_tex.c (pm_tex_rec_use), 3ds/tools/azt_scenarios.json and docs/handoff/test_catalog.md (new `yard_rp`) | about 58 code | Section 3. New scenario `yard_rp` = yard debug string, until `rec f34\d ` (yard_rec has reccheck, which turns replay off). Same commit for the scenario and the catalog row (rule 5) | mac01t_rec and yard_rp: runs > 0, rec line wbad 0; top shots byte-equal to the baseline. Then mac01t_rec_chk once: bad 0 (the check path still works beside the new rec_try branch) (3) |

Gate subjects: Mario on the Toad Town street (mac01t shots frame 500 to 505) and Mario in the yard (frame 200 to 204). The
until-line is the rec window line. Tests end at it (rule 5). Budget per row 8, used 1 to 3.

Console (rule 6): at the end of the epic only, after UC3e and UC4 pass on Azahar. Not per row.

## 5. What stays on the old path

| Case | Reason | Where |
|---|---|---|
| pm5 draws (lighting, texgen, fog uniforms) | use5 refused until UC4 | flush_draw rec_cap (pm_gpu.c:920) |
| fog, tint, panner, billboard, custom gfx, non-opaque render modes | not plain, header bit 9 = 0 | src/model.c:1500 to 1520 |
| lit groups, ring-room and matrix-table fallbacks, local vertex copies, any ring draw | ring vertices change per frame | 2a ring refuse |
| CPU draws, noise draws, fps60 interpolation | refused by rec_end and flush_draw | pm_gpu.c:2349, 1001 |
| two lists or two game matrices in one group | one gm per record | 2b |
| texture slot staging, slot change inside the model, off-screen pass | address or target not in the key | 2a, 2c |
| key change (ver, hgen, tgen, gen), grp change, churn state 3 | recapture, or no capture after 4 builds | pm_gbi.c:3438 to 3456 |
| replay refusal at the G_DL: geom, wind, view, interp, slot, gm, room | dlc_try runs that list this frame | rec_play |
| reccheck on, dd, gp, pm_ph_fine | mg_run returns at once or check mode | pm_gbi.c:3542, 3443 |
| the group ops themselves (render mode, texture bind, G_MTX) | still run each frame: they keep walker and pm_gpu state equal to the old path | mg_run |

Not done by this plan: GPU jumps into a stored list (copy path, decision_jump_closed.md), one replay for a run of models, and
a smaller re-emit after a replay (the run end makes the next old-path draw send its full state).

## 6. Expected cost saved

Data (console, phase on, Toad Town still view, plan_dl_design.md section 1, CONFIRMED in that doc):
- 90 plain models per frame, 230 to 231 us each = 20.7 to 20.8 ms. Split: ops 176 to 177 us (15.8 to 15.9 ms),
  close (flush_draw) 53 to 54 us (4.8 to 4.9 ms). The dlc share of the ops is not split out.
- ms0: 71 opaque plain models per frame (plan_dl_design.md:23).
- reccheck hits (agent_uc2c.md section 3): mac01t f307 2878 per window, yard 2003 per window. Window about 300 frames, so
  about 9.6 and 6.7 valid records per frame INFERRED.

Estimate INFERRED:
- Per replayed model the cut is the close (53 to 54 us) plus the dlc_try part of the 176 us ops (not measured).
  The added work is one copy of 0.77 KB on average (kb 300 for 392 builds, plan_record_reuse.md section 0), 16 patch words per
  draw, one Mtx_Multiply, npt dirty stores and the next old-path draw's full re-emit (not measured).
- At today's 9.6 valid records per frame: at least 9.6 x 54 us = 0.5 ms per frame (console, phase on).
- If all 71 opaque plain models replay: at least 71 x 54 us = 3.8 ms per frame. Phase off is lower (wall 43.1 against 53.3 to
  55.2 ms in the same doc). The console prof `wk` decides at the epic end. Azahar shows direction only.
- To raise the replay share: the f614 window of mac01t_rec_chk had hit 0 (agent_uc2c.md section 5, open). That cause comes
  before any claim above 0.5 ms.

## 7. Risks

| # | Risk | Handling |
|---|---|---|
| r1 | A uniform other than pal is per-frame | UC3a gate. Fail: stop, investigation names idx |
| r2 | A ctx dirty bit value is wrong | read-only objdump check before UC3b. A wrong bit shows as bad > 0 or a changed shot |
| r3 | No box skip in captures makes those frames slower and draws more triangles | capture frames only (cap < 5 per window). Replay draws the whole list (the GPU clips). No culling is added |
| r4 | 128 KB linear arena on the O3DS | boot memory line in the UC3c gate. Alloc fail: every capture refuses |
| r5 | Index arena reset while the frame still reads it | ia_hold (2a) |
| r6 | The next model inherits state that the replayed model's dlc list set | the group ops still run. geom_out, vert_live, tri_since_vtx, dlc_dirty are set as dlc_try exit does. The byte-equal gate checks the rest |
| r7 | The program config of prog4 is missing from a record | REC_DIRTY_CAP has Program. prog and prog4 share one DVLB (no code words) |
| r8 | Scissor or target change between capture and replay | view word compare |
| r9 | Command buffer overflow at the copy | room test with 512 words slack |
| r10 | The Azahar picture is equal and the console differs | console prof at the epic end only |
