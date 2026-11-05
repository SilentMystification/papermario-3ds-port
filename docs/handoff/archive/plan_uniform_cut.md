# plan_uniform_cut: key-pure uniform path for copied records (SR3 to SR5)

Plan only (Opus). Base HEAD ebcf1411a. No source edit, no run, no git change except this file.
Inputs: CLAUDE.md, agent_sr3.md, decision_jump_closed.md, plan_stateful_records.md, inv_citro3d_decode.md, hw_guidelines.md,
3ds/src/pm_gpu.c (lines 96 to 140, 454 to 514, 740 to 1070, 1894 to 2110), pm_dlc.inc.c 241, pm_gbi.c 1517, and read-only
objdump of libcitro3d.a (base.o, uniforms.o) in image pm3ds-build:1. Tags: [C] read from code or disassembly, [I] inference.

## 0. Result first

- citro3d does not write a uniform when C3D_FVUnifSet runs. The setter stores 16 B in `C3D_FVUnif[0][id]` and sets
  `C3D_FVUnifDirty[0][id]` (c3d/uniforms.h 7, 11, 15 to 20) [C]. C3D_DrawElements then emits each run of dirty vec4 as
  `[0x80000000|start][0x000F02C0]` plus one 0x2C1 data write (inv_citro3d_decode 2) [C]. So the uniform words of a draw
  are the set of Set calls since the last draw. The shadow skips in flush_draw choose that set from draw history.
- Cut: (a) in capture, force every uniform Set of every record draw (no shadow skip). The record then has a fixed shape.
  (b) The capture walker stores a patch table: for each emitted f32 vec4, the word offset and the uniform index.
  (c) At replay, the same uniform code runs with the live inputs, then the copy writes each patch slot from
  `C3D_FVUnif[0][idx]` (16 B, the exact bytes citro3d would send). (d) At run end, every uniform shadow is cleared.
- The record keeps every header and config word. No uniform data word of the capture is trusted at replay.
- C3D_BindProgram does not dirty all float uniforms [C]: it calls C3Di_LoadShaderUniforms (int, bool and DVLE constants
  only, base.o C3D_BindProgram 0xac, 0xf4). C3Di_DirtyUniforms (all ever-dirty vec4) has one caller, C3Di_AptEventHook
  (HOME menu resume) [C]. So the dirty set at a record draw is only the forced Sets plus DVLE constants.

## 1. What flush_draw writes (record draws take the raw path, `graw`, with `gpal` set)

Dependence: R = value from the record (key-pure). F = value changes per frame (live input). H = emitted or skipped by
draw history (a shadow compare). A record draw is a raw draw: pm_gpu_grange (pm_dlc.inc.c 241) or pm_gpu_gtrip
(pm_gbi.c 1517) set g_* from `ep_p` and `gm_tab` (pm_gpu.c 1989 to 2004, 2091 to 2106).

### 1a. Float uniforms (vertex shader, registers 0x2C0 config and 0x2C1 data)

| Line | Uniform | Value source | Value | Emit |
|---|---|---|---|---|
| 894, 926 | u5_uvso, u4_uvso | g_uvso = ep_p.uvso (texture scale and panner offset) | R plain, F panner | H: sh5_uvso, sh_uvso, sh5_ok, sh_ok |
| 895, 927 | u5_cmul, u4_cmul | g_cmul = ep_p.cm | R plain, F tint | H: sh5_cmul, sh_cmul |
| 896, 928 | u5_cadd, u4_cadd | g_cadd = ep_p.ca | R plain, F tint | H: sh5_cadd, sh_cadd |
| 897, 929 | u5_zmix, u4_zmix | g_zmix = ep_p.zm (zmix_y_dbg) | R | H: sh5_zmix, sh_zmix |
| 899 | u5_lflag | g_fx.lflag = ep_p.fx | F (lights, fog on or off) | H: sh5_fx (whole PmFx) |
| 900 | u5_fogu | g_fx.fogu | F (fog) | H: sh5_fx |
| 758 to 764 (from 902) | u5_lamb, u5_lcol 0..6, u5_ldir 0..6, u5_lk0, u5_lk1 (17 vec4) | g_fx | F (lights, look-at) | H: sh5_fx, only when lflag[0] != 0 |
| 917 | u5_pal + 7k, 4 vec4 | pcomp[gm] = gv_post x gm_tab[gm] (pcomp_calc 766) | F (camera, model matrix) | H: sh5_pal_ok, sh5_pal_gm, sh5_pal_gen, post_gen |
| 918 | u5_pal + 7k + 4..6, 3 vec4 | pal_ntab[gm] (normal rows) | F (model matrix) | H: same as 917 |
| 939 | u4_pal + 4k, 4 vec4 | pcomp[gm] | F | H: sh_pal_ok, sh_pal_gm, sh_pal_gen, post_gen |
| 951 | u_proj, 4 vec4 | shader_3d, off_proj or projection | F | H: proj_ok, proj_last. Only when !gpal (CPU program). Not in a record draw. It aliases pal slot 0 (registers 0 to 3, RAW1 comment 951, 964) |
| 965 | u_uvx or u4_uvx | t1_s (tile scale of texture unit 1) | R (texture key tgen1) | always when tex1 and not pm5. Not H |
| 970 | u_nzo, u4_nzo, u5_nzo | noise_off | F (animated) | always when noise. Not H |

Slot index k (917, 939) is the order of pal_g[] in the batch: R (it comes from the draw's matrix list).
Shadow resets that make H: 908 (pm5 clears pm4 shadows), 924 (pm4 clears pm5 shadows), 951 and 964 (clear pal slot 0
and 1), raw1_reset 129 (frame begin). sh_clear 480 does not clear sh_ok, sh5_ok or the pal shadows; it clears proj_ok.

### 1b. Other registers flush_draw writes

| Line | Write | Value | Emit |
|---|---|---|---|
| 893, 925 | C3D_BindProgram, C3D_SetAttrInfo (program code, attribute config, then int, bool, DVLE constants) | R (program of the draw) | H: `bound` |
| 1042 to 1048 | BufInfo (gring, stride 16) | R | H: `buf_cur` |
| 959 to 963 | C3D_TexBind unit 0 and 1 (pm_tex_bind, dc_bind) | R (key tgen0, tgen1, hgen) | H: pm_tex bind shadow (pm_tex_bind_reset) |
| 985 to 987 | TexEnv stages (flush_tev_rebuild) | R; TEV constant colors prim, cconst are F for the tint class (SR5) | H: tev_valid, tev_key |
| 992 to 1003, 1006, 1007, 1008 | depth test, depth map, early depth, alpha test (citro3d effect group) | R (decal refused) | H: sh.has bits |
| 995 to 997, 1005 | stencil test and op | R (zmark refused, gtrip 1968) | H: stencil_on. The effect group is sent whole, so the stencil value in the words is the same either way [I, reccheck checks] |
| 1012 to 1027 | scissor | R (sci_*, pm_wide; global) | H: sh.has SH_SC |
| 1029 | cull | g_cullm from ct and gv_wind (2000, 2103). gv_wind is the sign of the camera post matrix: F in principle | H: SH_CU |
| 1030 to 1035 | blend | R | H: SH_BL |
| 1038, 510 to 513 | FRAGOP_CLIP and CLIP_DATA0..3 (direct GPUCMD_AddWrite, not deferred) | R | H: SH_UC |
| 1049 | index buffer address and draw (C3D_DrawElements) | gidx + gi_start: F (frame ring) | always |

Every H item of 1b is made key-pure by the capture start (sh_clear plus raw1_reset): the first draw of the record emits
it, and later draws of the same record skip by the record's own earlier draws. Only 1a needs a cut. Of 1b, two values
are F: the index address (plan_stateful_records a2: copy indices into the arena, patch the address) and the cull mode
(store g_cullm in the record; at replay a different live g_cullm sends the model to the old path this frame).

## 2. How the record stores each dependent value

Choice: patch by offset (option 2 of the task). The data words stay in the record. A small table names where they are.
Re-emit was rejected: it splits the body at each uniform block and adds a segment list. Per-frame block write once a
frame was rejected: values change between draws of one record (pal slot k holds a different gm per draw).

### 2a. Capture (forced shape)
- `pm_gpu_rec_begin`: pm_gpu_flush (close the open batch), raw1_reset (bound = 0, buf_cur = 0, all uniform shadows 0),
  sh_clear, store GPUCMD offset. `pm_gpu_rec_end`: pm_gpu_flush, store the offset.
- In flush_draw at the start of `if (graw)` (890): `if (rec_cap) { sh_ok = 0; sh5_ok = 0; memset(sh_pal_ok, 0, ..);
  memset(sh5_pal_ok, 0, ..); }`. Then every 1a Set of the draw runs. The emitted index set per draw is fixed by
  (program, pal_n, pal_ntab != 0, lflag[0] != 0, tex1). The capture stores those five in RecDraw (2c).
- Refuse the build (state 2) when the capture has: noise (970), a CPU draw (flush_draw_cpu, so 951), pm_interp_on.

### 2b. Walker (one pass over the captured words, at rec_end)
Command layout (section 3): `[param0][header][param1 .. param c][0 if c odd]`, c = (header >> 20) & 0xFF,
words = 2 + c + (c & 1), register = header & 0x3FF, incremental = header >> 31.
- Header 0x000F02C0 (single) with param0 bit 31 = 1: `cur = param0 & 0x7F` (start vec4) [I bits 0 to 7, inv_citro3d_decode 2].
- Header reg 0x2C1, not incremental: c + 1 data words, 4 per vec4. Per vec4 one patch entry, then cur++.
  The first vec4 of the write is split by the header: word 0 at param0, words 1 to 3 after the header.
- Incremental 0x2C0 (header 0x800F02C0, DVLE constants, 24-bit) is R: no patch.
- End check: the walk must stop exactly at the end offset. Else count `wbad` and refuse the build.
Patch entry, 4 B: `u16 off` (word offset of word 0 in the body), `u8 idx` (uniform vec4 index), `u8 split` (1 = the
first vec4 of a 0x2C1 write: words at off, off+2, off+3, off+4; 0 = off .. off+3).

### 2c. Per-draw inputs (RecDraw, CPU only, normal heap)
`u8 use5, pal_n, ntab, tex1; u8 pal_g[8]` for pm5 or `[21]` for pm4 (gm indices); `s8 cullm`; `float vp[4]`.
uvso, cmul, cadd, zmix, fx are not stored: replay reads them live from ep_p (they are F for SR5 classes and R for plain;
reading live is correct in both cases). 32 B (pm5) or 44 B (pm4) per draw.

### 2d. Replay (SR4, per record draw, in draw order)
1. Set g_uvso, g_cmul, g_cadd, g_zmix, g_fx, g_use5 from ep_p, pal_tab = gm_tab, pal_n and pal_g from RecDraw.
   If vp differs from vp_*: rebuild_3d (as 2099). If the derived g_cullm differs from RecDraw.cullm: stop the run,
   this model takes the old path this frame.
2. Force shadows (as 2a) and run the uniform block of flush_draw (892 to 944, moved into `static void unif_raw(void)`,
   called by flush_draw and by replay; same code, same order). It fills C3D_FVUnif and pcomp.
3. Copy the body words of this draw into the command buffer with GPUCMD_AddRawCommands, one span between patch slots,
   and write 16 B from `C3D_FVUnif[0][idx]` at each slot. One sequential pass (hw_guidelines rule 3).
4. Run end: raw1_reset and sh_clear (all shadows, bound, buf_cur, proj_ok, tev_valid, tex bind). The C3D_FVUnifDirty
   bits set in step 2 make the next old-path draw send those vec4 again. That is correct (citro3d's array equals the
   GPU registers) and costs one redundant upload per run, not per model.

### 2e. Bytes
| Item | pm4, 1 matrix | pm5, 1 matrix, lights on |
|---|---|---|
| vec4 emitted per draw | uvso, cmul, cadd, zmix, pal 4, uvx 1 = 9 | uvso, cmul, cadd, zmix, lflag, fogu, light 17, pal 7 = 30 |
| uniform words in the body | 144 B data + up to 5 runs x 16 B = 224 B | 480 B + up to 8 runs x 16 B = 608 B |
| patch table | 9 x 4 = 36 B | 30 x 4 = 120 B |
| RecDraw | 44 B | 32 B |
| CPU stores per frame | 144 B (inside the copy, not extra) | 480 B |
Toad Town estimate [I]: 105 models x 1.5 draws x 224 B = 35 KB of uniform words copied per frame, 6 KB of patch
tables read. The old path sends fewer uniform words (shadow skips), so GPU command words go up. Azahar shows direction
only; the console prof `wk` decides (plan_stateful_records g).

## 3. Header count layout

- gpu.h line 11 in the image: `GPUCMD_HEADER(incremental, mask, reg) (((incremental)<<31)|(((mask)&0xF)<<16)|((reg)&0x3FF))`.
  No count field in the macro [C, read again for this plan].
- GPUCMD_Add (gpu.o 0xc0 `sub r8,r6,#1`, 0xc8 `orr ip,r9,r8,lsl #20`) puts n - 1 at bits 20 to 27 [C disassembly].
  The 256-param chunk limit (gpu.o 0x84 `cmp r5,#256`) equals an 8-bit field. Meaning on the hardware: INFERRED (3dbrew),
  the same layout the walker in 2b uses.
- State: count field = bits 20 to 27, value = params - 1. CONFIRMED as what libctru writes; the hardware reading is
  inferred. Read-only confirm on the image (no Azahar start):
  `MSYS_NO_PATHCONV=1 docker run --rm pm3ds-build:1 sh -c 'B=/opt/devkitpro/devkitARM/bin; cd /tmp; $B/arm-none-eabi-ar x /opt/devkitpro/libctru/lib/libctru.a gpu.o; $B/arm-none-eabi-objdump -d gpu.o | grep -n "lsl #20\|cmp.*#256"'`
  In the build: the walker end check (`wbad` 0 on every capture) proves the layout on the words citro3d wrote.

## 4. Rows (epic SR, one Haiku code agent, branch sr; each under 60 changed lines)

UC rows replace the "cut per-frame uniform writes" item of SR3 and the glue item of SR4. Records stay undrawn until UC3.

| Row | Name | Files | Lines | Content |
|---|---|---|---|---|
| UC1 | Forced uniform shape and walker | 3ds/src/pm_gpu.c | about 50 | rec_cap flag, pm_gpu_rec_begin and pm_gpu_rec_end (2a), forced shadows in flush_draw (2a), walker and patch table (2b), RecDraw store (2c), refuse list (2a). Counters `pt` (patch entries) and `wbad` on the `rec` line. Capture runs, records are not drawn |
| UC2 | Patch-masked reccheck | 3ds/src/pm_gbi.c, pm_gpu.c, azt_scenarios.json, test_catalog.md | about 35 | reccheck: walk this frame's old-path words of a valid record model with the same walker; compare headers, config words and the patch table; skip the data words of patch slots. Counts hit, build, refuse, bad, miss, wbad. Adds scenarios mac01t_rec, yard_rec (base + `reccheck`, until `rec f\d+`) if SR3 has not added them |
| UC3 | Replay uniform path | 3ds/src/pm_gpu.c | about 55 | Move 892 to 944 into `unif_raw()` (flush_draw calls it, same order). Replay per draw (2d steps 1 to 3), run end (2d step 4), cullm check. First one record (mac01t_rec), then all eligible |
| UC4 | pm5 and fx classes | 3ds/src/pm_gpu.c | about 20 | Accept pm5 draws (light table, lflag, fogu) in the forced shape. UC1 to UC3 accept pm4 only (refuse use5) |

## 5. Gates (rule 5; PM_ROW set; status line before each start)

| Step | Row | Command | Pass | Starts |
|---|---|---|---|---|
| 0 | first UC row | HEAD build: `azt.ps1 mac01t_cap1 -Row UC1 -Try` twice, `azt.ps1 yard -Row UC1 -Try` twice | each pair byte-equal (top shots). Not equal: stop, report the scene | 4 |
| 1 | UC1 | mac01t_cap1, yard (capture on, records not drawn) | top shots byte-equal to step 0 (forced Sets send the same values); `rec` line shows build > 0, wbad 0 | 2 |
| 2 | UC2 | mac01t_rec, yard_rec | bad 0, miss 0, wbad 0, hit > 0; top shots byte-equal to step 0 | 2 |
| 3 | UC3 | mac01t_rec (one record), mac01t_rec (all), yard_rec | top shots byte-equal to step 0; bad 0; runs > 0 | 3 |
| 4 | UC4 | mac01t_rec, yard_rec | byte-equal; hit up against step 3; bad 0 | 2 |
Not byte-equal: revert the row files, one fix, one more shot of that quantity. Two unchanged results end the series.
bad > 0: a fresh investigation agent names the register and the slot; no code agent from a guess.

## 6. Risks

| # | Risk | Handling |
|---|---|---|
| u1 | CONFIG param bits 0 to 7 = start index is inferred | wbad and reccheck bad compare the uniform index of each slot against the index flush_draw Set (UC2). A wrong field shows as bad > 0 |
| u2 | A vec4 dirty before rec_begin but not sent (a Set with no draw after it) lands in the record | pm_gpu_flush at rec_begin draws the open batch; flush_draw Sets come just before a draw (grep: no C3D_FVUnif call outside flush_draw and flush_light_tab). An early return in flush_draw (857 to 875) returns before any Set. reccheck catches the rest |
| u3 | HOME menu resume (C3Di_AptEventHook, C3Di_DirtyUniforms) dirties all ever-dirty vec4 | the first draw after resume sends more runs; the capture on that frame has another shape. Bump pm_rec_ctx on resume (aptHook) or accept: reccheck shows it as bad on one frame |
| u4 | Program upload in every record: raw1_reset at rec_begin forces C3D_BindProgram, so the body holds the shader code transfer (size not measured, about 0.4 to 1.2 KB [I]) | UC1 logs `pk` = words of writes to registers 0x280 to 0x2DF except 0x2C0 and 0x2C1. If pk x records is large, a later row cuts the program block into one blob per program and copies it only when the GPU program changes |
| u5 | Redundant uniform upload after each run (dirty bits from replay Sets) | bounded by 96 vec4 per run. Clearing C3D_FVUnifDirty for the patched indices is the upgrade; not in these rows |
| u6 | gv_wind flip (camera orientation) changes the cull mode in the body | RecDraw.cullm compare at replay; mismatch takes the old path |
| u7 | pal slot 0 aliasing with the projection (951) | the record draws never write u_proj (gpal set). Run end clears proj_ok and the pal shadows (2d step 4) |
| u8 | The patch copy breaks the hot loop L1I budget | the copy is a memcpy plus 4-word stores; unif_raw is the existing code moved, not duplicated |
| u9 | The Azahar picture is equal while the console differs | console prof push at epic end (plan_stateful_records g), not per row |
