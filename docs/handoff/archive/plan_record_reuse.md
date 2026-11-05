# plan_record_reuse: a record key that holds still (epic SR, copy path)

Plan only (Opus). Base HEAD 14456994d (source equal to 821b01baf). No source edit, no run, no git change except this file.
Inputs: CLAUDE.md, agent_rc1.md, status_RC1.txt, build3ds/last_log.txt (rec lines), the reverted RC1 patch
(scratchpad rc1_patch.diff, path in section 5), plan_record_capture.md, plan_uniform_cut.md, plan_stateful_records.md,
decision_jump_closed.md, hw_guidelines.md, and grep reads of src/model.c, src/evt/map_api.c, 3ds/src/pm_gbi.c,
pm_tex.c, pm_dlc.inc.c, pm_map.c. Tags: [C] read from code or log, [I] inference.

## 0. Result first

- The RC1 key has two fields only: `ver` = pmRecVer of the slot, and `gen` = pm_dlc_gen ^ (gstat_gen << 16) [C, patch].
  The matrix pointer, frame counter, noise, light, fog, shadow state and texture pointers are NOT in the RC1 key.
- `gen` did not change in window 2 [C]: the f614 line shows `n 17` records alive with `build 0`. A `gen` change runs
  rec_reset (memset rk), which sets n to 0. So the per-frame change is `ver` of a few slots.
- `ver` changes each frame at src/model.c 2852 [C]: mdl_update_transform_matrices calls PM_REC_BUMP(model) in the
  "first frame with dirty matrix" branch on every pass, not only when MODEL_FLAG_IGNORE_MATRIX changes. A model that a
  script moves each frame (TranslateModel, RotateModel: MATRIX_DIRTY set each frame) gets a new pmRecVer each frame.
  This is the same branch where finalMtx gets a new matrix stack pointer, so "the matrix pointer changes" reaches the key
  through this bump.
- A second defect in the RC1 patch [C]: the arena is bump-only (`ra_n += ...` in rec_done, no reuse of the slot's old
  space). Window 1: `kb 300` against a 256 KB arena. The arena filled near f280. In window 2 every capture with words
  failed the arena test and counted as `ref` (1100) with `kb 0`. So window 2 shows `build 0`.
- Numbers [C, last_log.txt]: f307 `cap 867 build 392 ref 95 emp 380 n 17`; f614 `cap 1561 build 0 ref 1100 emp 461 n 17`.
  1561 / 307 frames = 5.1 captures per frame. `emp` 461 = 1.5 per frame: moved models out of view, captured empty each frame.

## 1. Key fields that change per frame

| Field (RC1 key or candidate) | In RC1 key | Changes per frame | Where [C] | Cost |
|---|---|---|---|---|
| pmRecVer (ver) | yes | yes, for each model with MATRIX_DIRTY each frame | src/model.c 2851 to 2852 (mdl_update_transform_matrices) | 5.1 captures per frame in mac_01. Each capture: flush (closes the DR1 batch, c4), raw1_reset and sh_clear (full state re-emit, with the program upload, u4), GPUCMD_GetBuffer twice, header walk of n words, memcpy up to 4 KB, bump of the arena. The arena fills in about 280 frames, then every capture is waste. Console cost not measured; Azahar is direction only |
| global gen (pm_dlc_gen, gstat_gen) | yes | no in mac01t window 2 (n 17 kept) | 3ds/src/pm_dlc.inc.c 7, pm_tex.c 854, pm_map.c 379, pm_gpu.c 2052 | 0 in window 2 |
| matrix pointer (finalMtx), pmMtxVer, w1 bit 8 | no | yes (matrix stack of the alternate DisplayContext) | src/model.c 2830 to 2832, 2859 | reaches the key only through the 2852 bump |
| frame counter | no | yes | n/a | 0 |
| noise offset | no | yes | pm_gpu.c flush_draw noise block (RC1 sets rec_ref) | 0: noise draws refuse the build, the state 2 key holds |
| light, fog value, tint value | no | can | uniforms (plan_uniform_cut 1a) | 0 now. Fog on or tint makes the model not plain (src/model.c 1469 to 1471), so w1 bit 9 is 0 and no capture runs |
| uniform and register shadow state | no | yes | pm_gpu.c raw1_reset 129, sh_clear 480 | 0: the capture starts after raw1_reset and sh_clear |
| texture pointers (VRAM address, cache slot) | no | can (slot drop or invalidation) | pm_tex.c 614, 893, 910, 950 (sgen++) | 0 now. Becomes a cost when RK2 adds tgen, if a slot churns (risk k2) |

## 2. The new key

Per model (RecKey, 32 B, aligned 32, one cache line per model; the rk array grows from 4 KB to 8 KB):

| Field | Source | Covers |
|---|---|---|
| ver | pmRecVer, with the 2852 bump only on a change of MODEL_FLAG_IGNORE_MATRIX | model version: renderMode (map_api.c 778), textureVariation (184), panner and custom gfx flags (139, 154, 172, 200), transform group (model.c 3821, 3839), custom gfx index (3967 to 3979, 4072), local vertex copy (4298), creation (2710) |
| hgen (u16) | tb_gen[handle & 0x7f] (pm_gbi.c 2575), handle = model textureID + textureVariation, read through a model.c accessor | texture handle and version: the texture gfx list (format, tile, load) rebuilt by pm_texbind_set |
| s0, s1 (u8), tgen0, tgen1 (u16) | cache slots bound on unit 0 and 1 at the capture end (pm_tex `cur`, `aux`), and pm_tex_slot_gen(s) (pm_tex.c 620) | the VRAM address and format of the texture in the record body |
| nb (u8) | builds of this slot since the last arena reset | churn guard: at 4 builds the slot goes to state 3 (no capture) until the next arena reset |
| state, off, len | as RC1 | arena place |

Global word (compared once per call, as RC1): pm_dlc_gen ^ (gstat_gen << 16). A change resets the arena and rk.

Combine and geometry mode get no own field [I]: they come from the model display list (shape data: pm_dlc_gen), the
render mode setup in appendGfx_model (renderMode, renderClass: ver; fog and tint: not plain), and the texture gfx (hgen).
UC2 reccheck proves this: a `miss` (key equal, words outside patch slots differ) names a missing field.
pm_rec_ctx is not added: a new model in a reused slot has a new pmRecVer (one global sequence), and a texture reload
bumps tb_gen. Add it only when reccheck shows a miss after a context change.

Excluded from the key (each is cut or refused instead):

| Value | How it stays out |
|---|---|
| model matrix value and pointer, pmMtxVer, camera | pal uniforms: patch table (plan_uniform_cut 2b, UC1), value read live at replay (UC3) |
| light, fog and fx uniforms | pm5 refused in UC1 to UC3; patch table in UC4 |
| fog enable, tint | model not plain (w1 bit 9 = 0), no capture |
| panner UV, custom gfx | not plain |
| noise offset | noise draw refuses the build |
| index buffer address (frame ring) | patched at replay (plan_stateful_records a2) |
| cull mode, viewport | RecDraw.cullm and RecDraw.vp compare at replay (plan_uniform_cut 2c, 2d) |
| shadow state | capture starts after raw1_reset and sh_clear |
| frame counter | never read by the capture |

## 3. Rows (epic SR, one Haiku code agent, branch sr, in order)

### RK1: record capture with the matrix bump fixed (reapply RC1)

- Apply the reverted RC1 patch (section 5 path) with `git apply`. It is about 90 lines, already built, and its picture
  gate passed (top shots equal to HEAD on mac01t_cap1 and yard, wbad 0). New lines in this row: about 12.
- src/model.c 2851 to 2852: `if (model->flags & MODEL_FLAG_IGNORE_MATRIX) { model->flags &= ~MODEL_FLAG_IGNORE_MATRIX; PM_REC_BUMP(model); }`. 3 lines.
- 3ds/src/pm_gbi.c rec_try: when `k->state != 0` and the key differs, `rec_rb++; rec_ls = slot;`. rec line adds
  `rb <n> ls <slot>` (recaptures of a slot with an earlier key, and the last such slot). 3 lines. It names the bump site
  if the gate fails, with no new run.
- 3ds/tools/azt_scenarios.json and docs/handoff/test_catalog.md: new scenario `mac01t_rec` = mac01t_cap1 debug string,
  until `rec f6\d\d ` (the window 2 line), cap 120, deadline 20. Same commit (rule 5). About 6 lines.
- Cost (hw_guidelines): the 2852 change removes work. rec_try gains one store on the recapture path only (cold).

Gate (PM_ROW=RK1; status line in 3ds/logs/perf/status_RK1.txt before each start):

| Step | Command | Pass | Starts |
|---|---|---|---|
| 1 | `azt.ps1 mac01t_rec -Row RK1` | window 2 rec line (f6xx): cap < 5, wbad 0, build >= 0, rb < 5; top shots (frame 500 to 505) byte-equal to the RC1H HEAD shots of mac01t_cap1 | 1 |
| 2 | `azt.ps1 yard -Row RK1` | top shot byte-equal to the RC1H yard shot; rec line wbad 0, build > 0 | 1 |

Baseline: RC1H shots (scratchpad shots_rc1h_mac01t_cap1_1, shots_rc1h_yard_1). The source of HEAD equals 821b01baf, the
binary of those shots. If those folders are gone, run the RC1H HEAD pair again (4 starts; total 6 of 8).
cap >= 5: do not edit again. A fresh investigation agent reads `rb` and `ls` and names the site that bumps that slot.

### RK2: texture versions in the key, arena reuse, churn guard

- 3ds/src/pm_gbi.c: RecKey to 32 B (section 2). rec_try compares ver, hgen, tgen0, tgen1. One miss counter per field on
  the rec line: `mv mh mt`. rec_done: store hgen, s0, s1, tgen0, tgen1; keep the old off and len before state is set;
  when the old state is 1 and n <= old len, copy into the old place (no bump); `nb++`, at 4 set state 3. About 35 lines.
- src/model.c: accessor `unsigned pm_rec_tex(unsigned slot)` = textureID + textureVariation of the slot (0 when no
  texture). 3 lines. 3ds/src/pm_tex.c: `void pm_tex_cur_slots(int* a, int* b)` returns `cur`, `aux`. 3 lines.
- Cost: rec_try reads one more cache line per plain model per frame (rk 32 B), plus 3 loads (tb_gen, 2 slot gens). No
  division, no double, no per-frame malloc. The slot gen reads are 2 calls; inline them as one accessor if the console
  prof shows them.

Gate (PM_ROW=RK2): `azt.ps1 mac01t_rec -Row RK2`, `azt.ps1 yard -Row RK2`. Pass: window 2 cap < 5, mt < 5, wbad 0;
top shots byte-equal to the RC1H shots. 2 starts. mt >= 5: a fresh investigation names the slot and the sgen++ site.

### UC1 to UC4 (unchanged, after RK2)

As plan_record_capture.md (UC1) and plan_uniform_cut.md sections 4 and 5. One change: UC2 does not add tgen0, tgen1,
hgen or pm_rec_ctx (RK2 holds them). UC2 reccheck `miss` is now the check that the key is complete (section 2).
UC1 keeps the per-frame uniform values out of the record words (forced shape plus patch table). Each UC gate adds
"window 2 cap < 5 on mac01t_rec" so a later row cannot bring the per-frame capture back.

## 4. Risks

| # | Risk | Handling |
|---|---|---|
| k1 | Another site bumps pmRecVer each frame (a script loop on SetRenderMode, SetModelTexVariant, mdl_group_set_custom_gfx 3967 in other maps; enable or disable_transform_group) | RK1 `rb` and `ls` name the slot. RK2 churn guard (nb 4, state 3) caps the cost at 4 captures per slot per arena life. Fix at the site: bump only when the value changes |
| k2 | A texture cache slot churns (animated texture writes run pm_tex_invalidate, sgen++ each frame) | RK2 `mt` counter; churn guard. Such a model stays on the old path |
| k3 | The key misses a static input (combine or geometry mode not from ver, dlc gen or hgen) | UC2 reccheck `miss` > 0 stops the epic before records are drawn (UC3) |
| k4 | The 2852 guard changes behaviour | pmRecVer has no reader except the rec code [C grep]. The flag clear is the same. Picture gate byte-equal |
| k5 | A moved model (MATRIX_DIRTY each frame) holds a record built at another pal_n | pal_n is stored per draw in RecDraw (UC1); a different matrix count changes the G_MTX words only, not the record words [I]. reccheck checks it |
| k6 | Arena u16 `off` in words: 65536 words is the limit | arena is 65536 words; reuse keeps it well under. Arena full: state 2, `ref` count |
| k7 | Models out of view at capture (`emp`) are never retried while the key holds | as c5 of plan_record_capture: a later row decides on retry. Records are not drawn yet |
| k8 | The RC1 patch file is in the orchestrator session scratchpad and can be lost | copy it to docs/handoff before RK1 starts, or rebuild it from the diff in this file's inputs (agent_rc1.md lists every change) |
| k9 | Azahar picture equal while the console differs | console prof at epic end only (plan_stateful_records g) |

## 5. Commands and files

- RC1 patch: `C:/Users/Andrew/AppData/Local/Temp/claude/c--Users-Andrew-Documents-GitHub-papermario-3ds-port/d6d00d1c-7fdf-4fe7-b586-9f6c1655ea55/scratchpad/rc1_patch.diff`
- Build: `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from its .bak.
  Never commit 3ds/src/map_restore_table.c.
- Rec line: grep `rec f` in build3ds/last_log.txt. Window 2 is the f6xx line.
- Commit the row files on branch sr when the gate passes. Revert them when it fails.
