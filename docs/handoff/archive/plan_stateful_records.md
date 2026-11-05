# plan_stateful_records: retained per-model PICA records (epic SR)

Plan only (Opus). Read at HEAD 6a702d93c plus the working tree. No source edit, no run, no git change.
Inputs: CLAUDE.md, design_inputs_stateful.md (sections 1 to 6), plan_mr2_record.md, agent_drec1.md, plan_prof_mode.md,
inv_pmu_events.md, and grep reads of src/model.c, include/model.h, src/evt/map_api.c, 3ds/src/pm_gbi.c, pm_gpu.c, pm_tex.c.
Tags: [C] read from code, [I] inference (must be checked by the row), [E] estimate.

## 0. Result first

- Each eligible map model gets one record: the raw PICA words its old path wrote, captured once, stored in linear memory.
- The record is self-contained. The capture starts after sh_clear, so the record writes every register its draws read.
  The DREC1 failure (key did not cover the entry state) does not apply: a record does not read the entry state.
- Per-frame values (model matrix, lights, fog, tint, panner UV) are cut out of the record at capture. The CPU writes them
  each frame into a small glue segment. The GPU runs glue, then jumps into the record, then jumps back.
- A record runs only when every version in its key is equal. Else the model runs the old path this frame, and that run
  is the rebuild. The old path stays for every model that has no valid record.
- Decision quantity: console prof `wk` (mean work ms of a clean frame) in the Toad Town still view.

## (a) Data layout

### a1. Key line (CPU only, normal heap, never read by the GPU)
`RecKey rk[256]`, index = model slot (gCurrentModels index). 16 bytes, 2 keys per 32-byte cache line. Field order:

| Off | Field | Source |
|---|---|---|
| 0 | u32 ver | model->pmRecVer at build |
| 4 | u16 tgen0, u16 tgen1 | pm_tex_slot_gen (pm_tex.c 620) of the slots bound on unit 0 and 1 at build |
| 8 | u16 hgen | tb_gen[handle] (new, pm_texbind_set pm_gbi.c 2785) |
| 10 | u8 class, u8 state | class: plain, fog, tint, panner, billboard, custom. state: 0 none, 1 valid, 2 refused |
| 12 | u16 off | record offset in the arena, in 16-byte units |
| 14 | u16 len | record length, in 16-byte units |

Global key words (one compare per frame, not per model): `pm_rec_ctx` (context and texture reset), `pm_dlc_gen`
(pm_dlc.inc.c 7), `gstat_gen` (new, pm_gpu_gstat_reset pm_gpu.c 1996). One differs: all rk[].state = 0.
Fog and tint versions are global too, but only fog and tint class records compare them (SR5).
Read cost per frame: 105 models x 16 B = 1.7 KB, sequential by slot is not the draw order, so about 105 line reads in the worst case.

### a2. Record body (GPU reads, CPU writes only at build)
- Arena: `linearAlloc(512 KB)` at first map load, kept for the run. Bump allocation. Reset (off = 0) when pm_rec_ctx changes.
- Body: PICA command words, 16-byte aligned, padded with a NOP write. Layout: static state writes (shader config, attribute
  buffers pointing into the GSTAT area of gring, TEV stages, texture unit address and format, filter, wrap, depth, alpha,
  blend, stencil, cull, scissor), then index-buffer config and draw for each draw of the model, then one write to
  GPUREG_CMDBUF_JUMP0 (return). Per-frame registers are not in the body (a3).
- Indices: copied into the arena after the words. pm_gpu_grange (pm_gpu.c 2010) copies indices into the per-frame ring, so a
  captured index address is not valid next frame. The build rewrites the index-buffer address to the arena copy.
- Size [E]: 1 to 3 draws per plain model, 0.5 to 2 KB. 105 models fit in 210 KB. Arena full: state 2 (old path).

### a3. Per-frame block (glue)
- Two buffers of 32 KB, `linearAlloc` at init, swapped at pm_gpu_begin (pm_gpu.c 1707), so the CPU never writes the buffer the
  GPU reads.
- One glue segment per record model, in draw order, written once per frame, sequential:
  1. Per-frame uniform writes for the model (GPUREG_VSH_FLOATUNIFORM_INDEX plus a data burst): the `combined` matrix
     (4 vec4) always. Light, fog, tint and panner uniforms only when the value differs from the previous glue in the run.
  2. Per-frame non-uniform registers cut from the record (fog color, TEV constant colors of tint stages) for SR5 classes.
  3. ADDR0 and SIZE0 = the next glue segment (or the main-buffer return for the last model of a run).
  4. ADDR1 and SIZE1 = the record body, then JUMP1.
- Size: 96 bytes (matrix only) to 256 bytes (all fields). Toad Town: about 105 x 96 to 160 B = 10 to 17 KB per frame.
- Register IDs [I, check in libctru gpu_regs.h, not on disk]: CMDBUF_SIZE0 0x238, SIZE1 0x239, ADDR0 0x23A, ADDR1 0x23B,
  JUMP0 0x23C, JUMP1 0x23D. Size in 8-byte units, address = physical >> 3 (osConvertVirtToPhys).

### a4. Model fields (no size change)
include/model.h 66 `char unk_14[4]` becomes `u32 pmRecVer`. 76 `char unk_AA[6]` becomes `u8 pmSlot` (slot + 1, 0 = no record),
`u8 pad`, `u16 pmMtxVer`, `u8 pad2[2]`. Size stays 0xB0. grep shows no use of unk_14 or unk_AA on Model [C].

## (b) Version counters (SR1)

Rule: `PM_REC_BUMP(m)` = `(m)->pmRecVer = ++pm_rec_seq;` with `u32 pm_rec_seq` global. A value is never used twice, so a new
model in a reused slot or heap address never matches an old key (risk h2).

| Input | Write site (file:line) | Counter |
|---|---|---|
| model creation (all fields) | src/model.c mdl_create_model, after 2699 (slot = modelIdx, 2650) | PM_REC_BUMP, pmSlot = modelIdx + 1 |
| textureVariation | src/evt/map_api.c 180 | PM_REC_BUMP |
| renderMode | src/evt/map_api.c 770 | PM_REC_BUMP |
| texPannerID | src/model.c 4025 | PM_REC_BUMP |
| customGfxIndex | src/model.c 3952, 3957, 3962, 4053 | PM_REC_BUMP |
| flags HAS_TEX_PANNER | src/evt/map_api.c 138, 192, 194 | PM_REC_BUMP |
| flags USES_CUSTOM_GFX | src/evt/map_api.c 149, 151, 168; src/model.c 5085 | PM_REC_BUMP |
| flags TRANSFORM_GROUP_MEMBER | src/model.c 3808, 3825; src/battle/level_up.c 433, 436, 439, 442, 450 | PM_REC_BUMP |
| flags IGNORE_MATRIX | src/model.c 2699 (create), 2841 | PM_REC_BUMP |
| flags HAS_LOCAL_VERTEX_COPY | src/model.c 4278 | PM_REC_BUMP |
| group child copy | src/model.c 3165 (`Model newModel = *mdl`) | newModel.pmSlot = 0 (a stack copy never owns a record) |
| savedMtx content | src/model.c 2817; create 2695, 2698 | model->pmMtxVer++ (SR2) |
| fog | src/model.c 3988, 3992, 3996, 4001; init 2563 | pm_fog_ver++ |
| tint | src/model.c 4118, 4132, 4157; clear 2524 | pm_tint_ver++ |
| model context | src/model.c 2518, 2532, 2583, 2593 (gCurrentCustomModelGfxPtr) | pm_rec_ctx++ |
| all texture gfx cleared | src/model.c 2481 (mdl_load_all_textures) | pm_rec_ctx++ |
| texture handle rebuild | 3ds/src/pm_gbi.c 2785 pm_texbind_set (called by model.c 2160) | tb_gen[idx]++ |
| texture cache slot reuse | 3ds/src/pm_tex.c slot sgen (exists, read by pm_tex_slot_gen 620) | none new |
| shape buffer write | pm_dlc_gen (pm_dlc.inc.c 7, bumped pm_tex.c 854, pm_map.c 379) | none new |
| static vertex area reset | 3ds/src/pm_gpu.c 1996 pm_gpu_gstat_reset | gstat_gen++ |

Flags not counted: HIDDEN, INACTIVE, MATRIX_DIRTY, HAS_TRANSFORM, DO_BOUNDS_CULLING. They change whether or where the model
draws (per-frame matrix, or no draw), not the record words. BILLBOARD and IGNORE_FOG have no runtime writer [C grep].
Per-frame inputs (finalMtx pointer, texPanner U/V, mtxBillboard, custom gfx list content, camera) get no counter.

## (c) Dependency link and rebuild path

- Link: each record holds the versions of the inputs it read (a1). A record depends on: its model (pmRecVer), its texture
  handle (hgen), its two cache slots (tgen0, tgen1), and the global words (pm_rec_ctx, pm_dlc_gen, gstat_gen; fog and tint
  versions for those classes). An input change touches only the records that stored its version. Model-local writes touch
  one record. A handle rebuild touches the models that use that handle. A slot eviction touches the models bound to it.
  The global words touch all records, and they change only at map load, battle entry or exit, and texture reload.
- Rebuild: a model with a dirty key runs the old path (mg_run, pm_gbi.c 3562) in capture mode (SR3). The capture is the
  rebuild. The next frame uses the record. A rebuild costs one old-path run plus the copy (about 1 to 2 KB).
- Flush of new bodies: one pm_fl_dc over the span of bodies built in this frame, at the next submit (folded into
  gpu_flush_written, pm_gpu.c 268). Bump allocation keeps the bodies of one frame adjacent. Not per model.

## (d) Per-frame walk

At `case G_PM_MODEL` (pm_gbi.c 3696) and in mg_run:
1. Header w1 gives pmSlot, the plain bit, the static-matrix bit and pmMtxVer low 16 (SR2 sets w1 in model.c 1513 to 1515).
2. Dirty check: slot 0, or rk[slot].state != 1, or any key word differs: old path (capture when eligible). Cost: 16 B read,
   5 compares.
3. Valid: run the per-frame words of the group through the existing handlers (G_MTX to load_mtx, MOVEMEM lights, fog and
   tint words, panner SETTILESIZE). Skip the static words (G_PM_TEX to G_PM_RM) and the G_DL. The handlers set only shadow
   state and uniforms. No draw is queued.
4. Patch the glue: `pm_gpu_glue(dst, class)` copies the uniform words of the cut set from the same values flush_draw would
   upload. Run start: pm_gpu_flush (close the open batch of the old path), then GPUCMD_AddWrite of ADDR1, SIZE1, JUMP1 into
   the main buffer, pointing at the first glue of the run.
5. Run end (next model is not a valid record, a kick_check split is due, or the frame ends): set the last glue ADDR0 and
   SIZE0 to the main buffer after the jump. Then mark citro3d state dirty: sh_clear (pm_gpu.c 479), C3D_BindProgram,
   C3D_SetAttrInfo, C3D_SetBufInfo, C3D_TexEnvInit on all 6 stages, C3D_TexBind on unit 0 and 1, and C3D_FVUnifDirty for
   every uniform range the glue wrote [I: citro3d public API, check the header].
6. Submit: no new submit. The runs ride in the citro3d command list of C3D_FrameEnd (pm_gpu.c 1786) or of a kick split.

Hops per frame (CPU to GPU):

| Hop | Now | After SR4 |
|---|---|---|
| Submits (GX_ProcessCommandList) | 1 FrameEnd + up to 2 kick splits | same (0 added) |
| Cache flushes | 7 (dcsum, per submit and per span) | +1 glue span per submit that holds a run. Target 1: runs fall before the first kick split or after the last. Worst 3 |
| Per-model flush, submit, VRAM read, DMA | 0 | 0 |
| GPU jumps | 0 | 2 per record model (in and back), 2 per run in the main buffer |
| CPU reads of record bodies | n/a | 0 (key only) |

## (e) Fallback

- Any version mismatch, refused build, or ineligible model: that model runs the old path (mg_run words, dlc_try) this frame.
  The old path run captures and rebuilds that one record.
- Build refusal (state 2, until the next version change): a draw from the per-frame gring area (vertex index below
  GRING_VTX, pm_gpu.c 93), a CPU-transform draw (flush_draw_cpu, pm_gpu.c 793), a texture miss or C3D_TexLoadImage in the
  capture, slot_draws_stage (pm_tex.c 625, per-draw flush), pm_is_cfb, pm_interp_on (fps60 matrices), a decal or translucent
  render mode (SR3 to SR4; SR5 opens fog and tint), a write to a framebuffer or viewport register, body over 4 KB, arena full.
- The old path stays in the tree until the SR4 picture gate passes, and after that it stays as the path for non-record models
  and rebuilds. It is not a switch. Debug tool `reccheck` stays (rule 7).

## (f) Rows (one epic, one Haiku code agent, branch sr; each under 60 changed lines)

| Row | Files | Lines | Content |
|---|---|---|---|
| SR1 | include/model.h, src/model.c, src/evt/map_api.c, src/battle/level_up.c, 3ds/src/pm_gbi.c, pm_gpu.c | about 45 | a4 fields, pm_rec_seq and PM_REC_BUMP, every site of table (b) except 2817, tb_gen, gstat_gen, pm_fog_ver, pm_tint_ver, pm_rec_ctx. No reader. No picture change. |
| SR2 | src/model.c 1513 to 1515 and 2817, 3ds/src/pm_gbi.c (mg_run header, load_mtx 1894) | about 30 | 2817 `model->pmMtxVer++`. Header w1 = pmSlot, bit 8 = (finalMtx == &savedMtx), bits 16 to 31 = pmMtxVer. load_mtx: when the header static bit is set, `src == ` that model's savedMtx, `lm == 0` (no fps60 entry) and the per-slot tag equals pmMtxVer, copy the cached float matrix and skip unpack. Else unpack and store. Cache `float mc[256][16]` plus `u16 mt[256]`. |
| SR3 | 3ds/src/pm_gbi.c, pm_gpu.c, azt_scenarios.json, test_catalog.md | about 58 code | RecKey array, arena, capture: at an eligible dirty plain model, pm_gpu_flush, sh_clear, mark GPUCMD offset; at the group end pm_gpu_flush and mark again. Copy words, cut per-frame uniform writes (parse header word: id, mask, count, consecutive bit), copy indices and patch the index address, append JUMP0, store key. Records are built but not drawn. `reccheck`: for a valid record, compare the words the old path wrote this frame (cut the same way) against the body; count hit, build, refuse, bad, miss. Line `rec f<n> hit build refuse bad miss kb`. New scenarios mac01t_rec and yard_rec (base + `reccheck`, until `rec f\d+`). |
| SR4 | 3ds/src/pm_gbi.c, pm_gpu.c | about 58 | Glue buffers, pm_gpu_glue, walk step 3 to 5 (skip static words and G_DL, write glue, run jumps, return sizing, citro3d dirty), glue span in gpu_flush_written, run end before kick_check splits. `rec` line adds `runs`, `glkb`, `fl`. First this row proves one run with one record (jump and return) on mac01t_rec, then all eligible. |
| SR5 | 3ds/src/pm_gbi.c, pm_gpu.c | about 50 | Classes fog, tint, panner, billboard, custom: per-class cut sets (fog uniforms and fog color, TEV constant colors, uvso uniform, billboard matrix). Fog class key adds pm_fog_ver only when the fog value is in the record (it is cut, so no). Tint key adds pm_tint_ver for the tint type. Custom gfx: build only when every register write of the custom list falls in a cut set, else refuse. |

Return sizing (SR4) [I]: the return segment of the main buffer ends at the next run jump (exact, known when it is written)
or at the split or frame end. For the last one, size = words up to now plus the words GPUCMD_Split appends (libctru:
FINALIZE write plus 16-byte padding). SR4 checks this against the libctru source before the first run. If the size cannot
be made exact, the run end before a split is moved earlier: the last record run of a split ends with a main-buffer
segment of known size that holds only a NOP, and the split follows it.

## (g) Gates

Common: build with `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c.
Every test shell sets PM_ROW=<row>. Picture subject: Mario on the Toad Town street (mac01t_cap1 top shots) and the yard
with the green pipe, Mario and partner (yard). The row changes src/model.c and pm_gbi.c and pm_gpu.c (shared draw code),
so byte-equal top shots on mac01t_cap1 and yard are required. No other scene.

| Step | Row | Command | Pass | Starts |
|---|---|---|---|---|
| 0 | SR1, once per epic | HEAD build: `azt.ps1 mac01t_cap1 -Row SR1 -Try` twice, `azt.ps1 yard -Row SR1 -Try` twice | HEAD against HEAD: each pair byte-equal. Not equal: stop, report the scene (no gate is valid on it) | 4 |
| 1 | SR1 | mac01t_cap1, yard | top shots byte-equal to step 0 | 2 |
| 2 | SR2 | mac01t_cap1, yard; then `mac01t_prof` (PF3; if PF3 has not added it, SR2 adds it: mac01t_cap1 + `prof=16`, until `prof f\d+ w300 `) | byte-equal; prof line prints (wk is direction only) | 3 |
| 3 | SR3 | mac01t_rec, yard_rec | `rec` bad 0, miss 0, hit > 0, build > 0 in both; top shots byte-equal (records not drawn yet) | 2 |
| 4 | SR4 | mac01t_rec (one record), mac01t_rec (all), yard_rec, mac01t_prof | rec bad 0; runs > 0; fl not above the submit count; top shots byte-equal on both scenes; prof wk lower than step 2 (direction) | 4 |
| 5 | SR5 | mac01t_rec, yard_rec | byte-equal; hit count up against step 4 (more classes); bad 0 | 2 |

Stop rules (rule 5): shot not byte-equal: revert the row files, one fix, one more shot of the same quantity. Two unchanged
results end the series. rec bad > 0: a fresh investigation agent names the register; no code agent starts from a guess.

Console (rule 6, epic end only, after SR5 passes on Azahar):
1. Baseline: push the step 0 HEAD build. 2. Push the SR5 build. Both:
   `PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 prof`
   Stand still in Toad Town (mac_01 at pos `M x 11 y 0 z 40`) for 2 prof windows (600 drawn frames).
- Decision quantity: prof `wk` (mean work ms of a clean frame), mean of the 2 windows. Also read ft, fps, o34.
- Pass: wk down 3.0 ms or more against the baseline. Fail: revert SR3 to SR5 and the SR1 counters that SR2 does not read.
  SR2 stays when its own byte-equal gate passed.
- Estimate [E]: mr0 89 plain models x 230 us = 20.7 ms phase on. Phase-off cost per model is not measured. If it is one
  third (77 us) and a record model costs 2 us, the gain is about 6.7 ms. A gain of ft larger than wk can be absent if the
  GPU becomes the limit; ft and fps show that.

## (h) Risks

| # | Risk | Earlier failure | Handling |
|---|---|---|---|
| h1 | Wrong key: the record depends on state the key does not hold | DREC1 (agent_drec1.md): a per-handle snapshot read live entry state (rings, td, cur_tile, tsz_last) | Capture after sh_clear, so the body sets every register it reads. reccheck compares bytes with equal versions (bad, miss). A missed src/ write site shows as miss > 0 |
| h2 | Stale record after a heap change: a new model in a reused slot or heap address matches an old key | diag_dlr.md line 39 (pm_pdl_heap_gen changed every frame, no entry lived); PDL compiler deleted (DLF23) | pmRecVer from one global counter that never repeats. pm_rec_ctx at context and texture reset. gstat_gen at static vertex reset. Arena reset with pm_rec_ctx. No heap address in the key |
| h3 | Texture cache eviction moves a texture in VRAM | none | tgen0 and tgen1 from pm_tex_slot_gen. Staged textures refused |
| h4 | Per-frame index ring | none | indices copied into the arena at build |
| h5 | Draw order | none | runs keep game order; no reorder, no skip. dlc_box_out: the capture records the groups drawn in the capture frame only. A group out of view at capture and in view later is missing. Mitigation: capture runs dlc_try with the box test off for that model (no culling rule, memory). The byte-equal gate checks it |
| h6 | citro3d state out of date after a run | none | step 5 dirty list. A missed item shows on the next old-path model: byte-equal gate |
| h7 | Jump return size | none | (f) return sizing; one-record proof first in SR4 |
| h8 | fps60 interpolation | none | pm_interp_on refuses the build |
| h9 | L1D 16 KB | none | key 16 B, glue written sequential, 10 to 17 KB per frame streamed. mc[] in SR2 is 16 KB but one 64-byte entry per model is read, the same as the Mtx it replaces |
| h10 | N3DS L2 | none | no row relies on it. The push at 192.168.1.170 (CLAUDE.md rule 6) decides; the console model at that address is not stated in the inputs |
| h11 | Azahar does not model PICA jumps like the console | none | the console push is the decision; Azahar byte-equal only proves the words |
