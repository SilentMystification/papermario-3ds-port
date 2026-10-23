# Display-list compiler and full GPU path plan

Written 2026-10-22, changed 2026-10-23 (user direction: the display-list compiler is the core of the plan, the GPU moves come after it). Read-only plan. No build, no Azahar run. Line numbers are the working tree of 2026-10-23. Another agent edits pm_gpu.c and pm_gbi.c now, so lines can move.
Data letters: M measured on the console, A measured in Azahar, E estimate. Console ratio for gb: 4.5 x Azahar (build 1036). Older tables use 3.5.
Basis: build 1073, town mac_01 gb 31.91 ms of cpu50 55.6 ms (M). 98 percent of spike steps have gbi as the main phase (M). About 5,800 ops per step, so about 1,470 cycles per op at 268 MHz (E). Draw cost about 79 us on the console (M). Azahar toad: gb 7.09 ms, 73 draws (A).

Contents: Part A (sections 1 to 6) is the display-list compiler: the core, stages C0 to C7. Part B (sections 7 to 11) is the GPU moves GP1 to GP5 and the texture moves. Part C (section 12) is the whole-pipeline review (compile, state ids, instancing, VRAM, shaders, threading). Section 13 is the questions for the console.

# Part A: display-list compiler (the core)

## 1. Why a per-command walk needs cache lookups

The interpreter keeps no identity of a list between frames. It reads each RDP or RSP word, changes raw N64 state (tile descriptors, TMEM records, combiner words, other modes), and then derives PICA objects from that raw state: a converted texture in linear memory, a TEV configuration, blend and depth registers, uniforms. The derivation is expensive (a texture conversion is 1.21 ms fixed per miss before FL1, M). So each op hashes or compares its inputs to find the object it derived last time. Every lookup below exists only because the walk forgets which list produced the state.

The input to these lookups does not change: all static G_DL targets stay equal over 30 frames (1122 of 1122, A, DLC.md DLC0). The same list at the same address with the same content gives the same PICA objects in every frame. So a key on list identity (address, region generation, state-in) can replace all lookups of a list with one lookup.

### 1.1 Lookups per command type (profile: Azahar toad, dlc on, `gbiprof`, net time per op; console estimate x4.5)

| Op | calls per step | ms per step A (console E) | Lookups and compares that run per call (file:line) |
|---|---|---|---|
| G_SETTILESIZE (tile 0) | 162 | 1.5 (6.8) before OPT-TS and OPT-LK, which saved 0.42 A | Window math and S25 sign fix (pm_gbi.c:2849-2870). Scan of 4 `ldrec` TMEM records (2973-2978) and for tile 1 also 4 `blk` records (2892-2901). Palette bank select `tlut_pal[tile_pal]` (3002). PG1 skip: compare of 8 fields plus `pm_tex_epoch()` and `pm_is_cfb` (3003-3004). `pm_gpu_flush` (3018). `pm_tex_load` (3023): palette hash, FNV over all `tlut_n` entries for CI (pm_tex.c:655-673, 0.2 ms A), direct index `lidx[256]` by image, palette, size, format, stride, dkey (677), recent list of 12, scan of up to 167 slots (slot lookup 0.42 ms A). `pm_tex_origin` for off-screen slots (3026). `grab_cur` for 16-bit RGBA (3022) |
| G_DL | 381 | 0.95 (4.3) | `ptr_of` (301: `is_host`, segment table `segbase[16]`, K0 mask), `is_host` again (2299), `dlc_try`: `dlc_find` in a 2048-entry hash table by address (pm_dlc.inc.c:198), generation compare, box test per group (227), replay (`pm_gpu_grange`). About 1.1 ms A of this is the replay itself |
| G_SETCOMBINE | 186 | 0.89 (4.0) | Decode of 16 fields and the TEXEL1 rename (2533-2584). `sync_combine` (1006): FNV hash of 11 words (DIP2), compare with the last signature, 16-entry memo `scm_find` (OPT-SC), replay of the outputs into `pm_gpu_set_plan` or `pm_gpu_set_combine` (each compares with the current value) |
| G_VTX | 41 | 0.58 (2.6) | `ptr_of`, `is_host`, `gv_matrix_id` (1329: memcmp of 64 B with the last entry, on a miss a scan of up to 256 entries of `gm_tab`), ring copy, per-vertex product and clip bits (2390-2401) |
| G_SETOTHERMODE_H | 454 | 0.54 (2.4) | Mask build, filter compare and flush (2523-2526), `sync_combine` hash when the cycle-type field changes (2530) |
| G_SETTILE | 458 | 0.37 (1.7) | `same_tile`: compare of 11 fields plus `pm_tex_wrap_gen()` (2774-2776), flush and `pm_tex_set_wrap` on a change |
| G_TEXRECT | 59 | 0.34 (1.5) | `img_role` lookups for the colour image (2696-2698), `off_screen_check`, `box_fold_sync`, `sync_render` (514: blender decode, `img_role`, 4 to 6 `pm_gpu_set_*` compares), `ensure_tile`, `dual_bind` (FNV of 11 words), `pm_tex_size`, `t1_fold` |
| G_LOADTLUT | 118 | 0.32 (1.4) | Bank slot, scan of the 2 bound tiles for a CI tile on this bank (2838-2844), flush and rebind on a change |
| every op that is not geometry, matrix or list flow | about 4,500 | in the ops above | `gv_epoch++` (2285). The next triangle then runs `gv_epoch_enter` (1374): `gv_state_why` (1362: `img_role`), `ensure_tile`, `dual_bind` (FNV of 11 words), `sync_render` |
| per draw (`flush_draw`, pm_gpu.c:432) | 63 to 73 draws | gp 0.63, gd 1.10 (2.8, 5.0) | TevKey build (20 stores) and memcmp, up to 7 `sh_*` compares, texture bind compare, palette upload per entry, then citro3d `C3D_DrawElements` (dirty-flag scan of 96 uniforms, out-of-line command writes) |

Sum of the listed ops: 5.5 ms of gb 8.0 ms (A). The rest is G_TRI ops, G_MTX, small state ops and the dispatch.
Reading: on the console these ops cost about 25 ms of the 31.9 ms gb (E: 5.5 x 4.5). Most of each op is a lookup or compare that confirms that nothing changed since the last frame.

## 2. End-to-end design: compile once, replay a pre-resolved stream

### 2.1 Principle
The compiler runs the interpreter ONCE per key (list address, state-in id, region generation) in record mode. It records the outputs that the interpreter gives to pm_gpu and pm_tex (not the input words). The replay applies the outputs and runs only the dynamic parts. The interpreter stays as the compiler and as the path for ops that the compiler refuses.

### 2.2 Compiled stream (one entry per key)
| Record | Content | Replay cost |
|---|---|---|
| PSO | Pipeline-state id: index into a table of resolved PICA state (TEV stages, combiner plan, blend, depth test and write, alpha test, fog mix, cull, texture unit config, filter, wrap). Colour constants that come from prim, env, fog, blend registers are references to the register, not values | 1 compare with the current id, a table pointer |
| TEX | pm_tex slot index plus a slot generation (new u32 per slot, bumped at eviction and `pm_tex_invalidate`, DLC.md 2.4), plus the uvso inputs (tile origin, shift, size). A generation mismatch re-resolves through `pm_tex_load` once and patches the record | 1 compare |
| VTX | DLC2 static area range (static lists) or a source address plus count (mutable lists: copy at replay) | none or 1 memcpy |
| MTX | Palette slot reference: "matrix at list entry" or "G_MTX at offset k" (the Mtx data is read at replay, 64 B unpack) | 1 unpack per G_MTX |
| DRAW | Index range in the static index area, cull type, group box for the reject | 1 box test per group, 1 memcpy of indices |
| PARAM | A dynamic word the replay reads from memory: G_SETPRIMCOLOR, G_SETENVCOLOR, G_SETTILESIZE scroll, segment address, light | 1 load and 1 uniform or offset write |
| EXIT | Snapshot of the interpreter state that the list writes (GbiState fields, about 300 B), so that later interpreted ops see the same state | 1 memcpy, only when the next op is interpreted |
Batch boundaries are decided at compile: two consecutive DRAW records with the same PSO and TEX and a palette that fits share one batch (the rule of `pm_gpu_gtrip`, pm_gpu.c:1629-1637, applied once).

### 2.3 Key, state-in and the read set
- A list reads some state that it inherits (for example a shape list draws with the texture and render mode that the lists before it set). The compiler records the read set: the GbiState fields that the list reads before it writes them.
- State-in id: the interpreter keeps a current state id. A compiled list sets it to its exit id at replay. An interpreted state op sets it to "unknown". At a G_DL with an unknown id, the walk hashes the read-set fields of that list (about 10 to 30 words, under 1 us) and looks up the id. So a run of compiled lists costs 1 table lookup per list, and the hash runs only after an interpreted state op.
- A texture list or a render mode list writes all the fields it uses. Its read set is small (E: partial other-mode bits only). Stage C0 measures this.

### 2.4 Classes and invalidation
| Class | Examples | Rule |
|---|---|---|
| Static | map shape lists (gMapShapeData), texture lists in TextureHeap, render mode and combine lists in .data (`Gfx_RM*`, `SolidCombineModes`) | Region generation: shape `pm_dlc_gen` (exists), texheap bumped by the ROM read and dma_copy hook and `mdl_load_all_textures` (DLC.md 3.1), rodata never. `dlccheck`-style hash at every replay in the check mode only |
| Mutable by patch | entity heap and general heap lists (entity models, animator lists), lists with segment addresses, lists with G_MTX inside | Content hash of the list words at first reuse after a heap generation bump. Segment values that the list used are part of the key (1 compare per used segment). G_MTX inside: an MTX record, the data is read at replay. Game writes into a list word (patching) change the content hash, so a patched list recompiles or uses its PARAM record |
| Per-frame dynamic | the main list (gMainGfxPos, rebuilt each step), sprite heap lists, HUD, text, effects that build lists in the main list | Not compiled by address. Interpreted, with the fast paths of section 2.6 and templates (stage C5) |
| Refused | lists with G_MODIFYVTX, G_SETCIMG, G_SETZIMG, G_FILLRECT into a depth or CPU-fill role, G_TEXRECT that reads a frame buffer or the depth copy buffer (F03, pm_gbi.c:2693), unknown ops (the interpreter has no case for G_BRANCH_Z: it goes to `note`, 3073; the compiler refuses it and the counter shows if it occurs), lists over 4096 ops | Interpreted every time, counted by reason |
G_DL_NOPUSH (a branch) inside a compiled list is flattened at compile (as DLC1 does). A G_DL into a non-static class from a static list: the child stays a separate entry with its own key.

### 2.5 Why this removes the lookups
- G_SETTILESIZE, G_SETTILE, G_LOADTLUT, G_LOADBLOCK, G_SETTIMG inside a texture list: no op runs. The TEX record holds the slot. No window math, no TMEM scans, no palette hash, no slot probe.
- G_SETCOMBINE, G_SETOTHERMODE, G_TEXTURE, render mode lists: no op runs. The PSO id holds the plan and the PICA state. No DIP2 hash, no memo lookup.
- Epoch prep (`gv_epoch_enter`): runs at compile. The replay sets the PSO id.
- flush_draw: the PSO record holds a pre-built TevKey result. Stage C6 replaces the setter calls with a pre-built register packet.

### 2.6 Dynamic lists (characters, effects, UI, text)
- The main list calls static sub-lists for most of its work: texture lists, render mode lists and shape lists replay from the cache. The main-list ops between them (G_MTX, prim and env colours, tile scroll, segment) are interpreted. They are about 35 percent of the ops (A, DLC0).
- Templates (stage C5): a run of main-list ops between two compiled G_DL calls has the same op sequence each frame (for example G_MTX, G_DL texture, G_DL render mode, G_SETPRIMCOLOR, G_DL shape, G_POPMTX), with other pointer and colour words. Key = hash of the op skeleton with the parameter words masked (G_MTX w1, colour w1, tile-size window, G_VTX w1). The replay reads the parameter words. A hash of the skeleton costs about 10 cycles per word against about 1,470 cycles per interpreted op (E).
- Sprites: sprite component lists in the sprite heap (class 6): content hash at reuse, as the mutable class. Stage C0 measures how many repeat.
- UI and text: texture rectangles. The font glyphs come from one static raster (charset). PT86 measured the file select: draws 231 against 53 because each text line is its own texture batch (M, Azahar). A glyph atlas per font gives one PSO and one TEX per string, so one draw per string (stage C7).

### 2.7 Validation mode (migration only)
Switch `pdlcheck` for Azahar: in the same frame, the compiled replay writes its draws into capture A, and the interpreter runs the same list from a GbiState copy into capture B (pm_gpu emit off, as `dlccheck=2` does). Compare per draw: PSO id against a fresh PSO from the interpreter state, texture slot and generation, uvso, cmul, cadd, zmix, cull, index list, palette matrices. Compare the exit state. Counters: `drawbad`, `exitbad`, `texbad`, `idxbad`, all 0 on toad, yard, battle, pause1, V16b, V18, V19, mim_01, tik_01 and the route play1. Rule 19: the old per-op path for compiled classes is deleted in the same stage that turns the compiled path on. The interpreter stays only as the compiler and for refused and dynamic lists. `pdlcheck` stays as a test tool like `dlccheck`, or is deleted after stage C7, as the user decides.

### 2.8 Memory budget (O3DS)
Heap margin: about 4,669 KB (E, MEMORY_BUDGET.md option A). The DLC cap is 512 KB of newlib heap plus the 256 KB static vertex area in linear memory (DLC2).
| Item | KB (E) |
|---|---|
| PSO table, 512 entries x 96 B | 48 |
| Texture and render mode list entries: about 190 per map x (records 64 B + EXIT 300 B) | 70 |
| Shape entries (exist, DLC2) | about 110 per map |
| Mutable lists (entity, animator), stage C4 | 50 to 150 |
| Templates, stage C5: 256 x 200 B | 50 |
| Total | 330 to 430, inside the 512 KB cap |
LRU by bytes over entries. A map change frees the shape and texheap entries at the next compile. The texture slab (FL1) is not changed. A TEX record does not pin a slot: the generation check re-resolves an evicted texture (evictions are 0 in town, M).

### 2.9 Threading
- Compile on core 1: the compile is bursty (map entry: 174 lists, DLC.md risk 7) and needs no citro3d call. A compile job records ops into a buffer on core 1. The frame that misses interprets the list as today. The next frame replays. Texture handles are resolved on core 0 at the first replay (pm_tex is not thread safe). The O3DS has about 6 to 12 ms of free core 1 per step beside the audio (E, arch_review.md 1.5), which is enough for compile bursts.
- The full producer and consumer split (gbi on core 1, game on core 0) is section 12 item 6. The compiler makes the gbi smaller, so it fits better in the core 1 budget.

### 2.10 N3DS cores (user decision 2026-10-23)
Hardware core 0 is full. Hardware core 2 is full and has no APT limit. Hardware core 1 is reduced (`APT_SetAppCpuTimeLimit`, system and GSP). Hardware core 3 is not usable by the app. Game logic stays on core 0. The second worker (audio submit and the display-list walk) is pinned to core 2. The DSP mixes. The GPU does the graphics work the PICA can run. Today the audio thread is on core 1 (`pm_audio.c`). That is the reduced core. This pin is N3DS only. The O3DS has no core 2.

## 3. Stages (core). Expected console gain is gb in town mac_01, E

Every stage: one code agent, PM_ROW set, at most 8 Azahar starts. Standard Azahar checks: golden toad (0.00 to 0.01), yard (0.00), battle (2.47, limit 3.0); statehash yard 15eea14d and battle 7e1f3949 unchanged; dlccheck zeros; `pdlcheck` counters 0 from C1 on. Standard console gate: push as hw_batch.md 8.0h, town gb against the build before, draws not higher, no new CRASH, FAIL or `lines dropped`.

| Stage | Content | Console gain (E) | Risk | Checks beyond the standard ones | Files |
|---|---|---|---|---|---|
| C0 | GbiState struct: gather the state that the state ops write (tiles, tile sizes, TMEM records ldrec and blk, timg, tlut banks, combine words and decoded fields, other modes, geometry mode, texture, prim, env, fog, blend, keys, convert, prim depth, scissor, lights) into one struct, no behaviour change (DLC3a was built once and dropped). Profile counters per G_DL target of the static classes: read-set size, distinct state-in hashes per target per frame, the share of targets whose (address, state-in) was seen the frame before with the same exit hash | 0 | low | golden 0.00 against the build before, statehash equal, gbiops equal | pm_gbi.c, pm_dlc.inc.c |
| C1 | Compile texture lists and render mode lists (classes texheap and rodata) into TEX, PSO-input and EXIT records. Replay: state-in lookup, EXIT apply, TEX with generation check, lazy bind at the first surviving triangle (ensure_tile). Slot generation in pm_tex. `pdlcheck`. Delete the per-op path for these classes | 4 to 8 (the ops inside about 190 static state lists per step: SETTILESIZE, SETTILE, LOADTLUT, SETCOMBINE, OTHERMODE, about 70 percent of their 16 ms, E) | high: exit state exactness, CI palette banks (V20), frame buffer grabs, off-screen slots, texture panning in the main list | pdlcheck zeros on 9 scenes and route play1; golden pause1 to 4, mim_01, tik_01, iwa_00, V12c, V20 | pm_gbi.c, pm_dlc.inc.c, pm_tex.c |
| C2 | PSO table: the epoch prep (`gv_epoch_enter`, `sync_render`, `dual_bind`, combiner plan) keyed by PSO id. Compiled shape groups (DLC2) carry the PSO id of their state-in. Delete `gv_epoch`, the DIP2 hash and the OPT-SC memo for compiled paths | 1 to 3 | medium | pdlcheck `drawbad` 0, draws equal | pm_gbi.c, pm_dlc.inc.c, pm_gpu.c |
| C3 | Batch boundaries at compile: shape groups in one entry with equal PSO, TEX and palette fit become one DRAW range. Merge across consecutive compiled lists | 0.5 to 1.5 (fewer draws, about 79 us each) | medium | draws per step not higher, golden | pm_dlc.inc.c, pm_gpu.c |
| C4 | Mutable class: entity and animator lists (DLC4, kind M): content hash at reuse after a heap bump, MTX records into the palette, PARAM records for colours. Sprite heap lists by content hash | 1 to 3 (battle more) | high: heap writes that no hook sees | pdlcheck on battle, V19, route play1; statehash on the route | pm_dlc.inc.c, pm_gbi.c, entity and heap hooks |
| C5 | Main-list templates: op skeleton hash, PARAM records for G_MTX, colours, tile scroll, segment | 2 to 5 (the main list is 35 percent of the ops) | high: a skeleton match with a different meaning (guard: the skeleton includes the op bytes and every non-parameter word) | pdlcheck on all 9 scenes and the route, full golden | pm_gbi.c, pm_dlc.inc.c |
| C6 | Pre-built register packet per PSO (DLC5): copy the packet at a draw instead of the citro3d setters | 2 to 4, unproven (DR1 and RAW1 did not get under 79 us, M) | high | draws equal, golden 10 scenes | pm_gpu.c |
| C7 | UI and text: glyph atlas per font, rectangle batches per PSO and TEX (PT86 file select: 231 draws against 53) | 0 in town; 1 to 6 in menus and text scenes | medium | golden pause1 to 4, file select, a text box scene | pm_gbi.c, pm_gpu.c, pm_tex.c |

Sum (E): gb in town from 31.9 ms to about 12 to 20 ms after C5. Each stage gates the next on the console.
Order: static scenery first (C0 to C3, the shape part exists), then models with patch points (C4, C5), then the rest (C6, C7).

## 4. Fit with existing work
- DLC1 and DLC2 are stage "C-1": shape lists compile and replay with one box per group (gain 8 ms, M).
- DLC3b (skip a state list when its exit state is already there) found 0 candidates (A). C1 is different: it does not skip, it replaces the op work with records. The DLC3b data (every repeat call sees other values) means C1 must apply the EXIT snapshot each time. That costs one memcpy, not the ops.
- OPT-TS, OPT-LK, OPT-PH, OPT-SC and PG1 are per-op caches. They are deleted for the compiled classes in C1 and C2 (rule 19) and stay for the interpreted lists until C5.

## 5. First stage the main-tree agent can start now
C0: the GbiState struct (mechanical, no behaviour change) plus the C0 profile counters (read-set size, state-in repeat rate, exit determinism per static target, one `pdl0` line every 300 drawn steps). About 300 to 400 lines in pm_gbi.c and pm_dlc.inc.c. Checks: golden toad, yard, battle 0.00 against the build before, statehash equal, gbiops equal, then one Azahar run of the toad preset and route play1 for the counters. Decision rule for C1: 90 percent or more of the static state-list calls have a (address, state-in) key seen the frame before with the same exit hash. Start after the agent that edits pm_gbi.c now (PT86) ends its row: same file. If C0 cannot start at once, GP1 (section 10) is the alternative: it is smaller and touches only the G_VTX case and gv_tri.

## 6. Risks of Part A
- Exit state: a field that the interpreter reads and the EXIT snapshot does not hold gives a wrong picture after a replay. Guard: `pdlcheck` exitbad and drawbad on 9 scenes plus the route.
- Hidden writers into static regions (a decomp memcpy into TextureHeap): `dlccheck` hash on golden scenes before each stage merges (DLC.md risk 5).
- Statehash: the render must not feed the game state. F03 depth queries and CPU fills (D03) are refused ops, so they stay interpreted.
- Compile spikes at map entry: compile on core 1 (2.9) or a compile budget of 1.5 ms per frame.

# Part B: GPU moves (after the compiler stages or in parallel when the file is free)

## 7. Area 1: matrix product and clip bits in non-DLC G_VTX

### 7.1 What the CPU does now
- pm_gbi.c:2319-2422 (case G_VTX). Per load: prefetch (2331), slot clamp, `gv_matrix_id` (1329), `pm_gpu_gvtx_load` (pm_gpu.c:1745: memcpy of n x 16 B into the ring, rgen clear).
- Per vertex (2381-2401): 8 slot stores (vert_ring, vert_gmi, vert_tss, vert_mid, vert_live, vert_scr, vert_litf, vert_gvf), the product with `combined` (16 multiply-adds, 2394-2397), `clip_bits4` (1192, 6 compares) and the near bit at w <= 0.05 (2398).
- Who reads the result: gv_tri (1394 reject, 1395 near refusal). The CPU fallback reads vert_p through gv_xf (1289), which recomputes only when `vert_gvf & 1` is 0.
- Cost: 0.58 ms per step A, 2.0 to 2.6 ms console (E). The product and clip bits are a part of it: about 25 to 60 VFP cycles per vertex (E), against 2.05 us per vertex for the whole old load (M, build 926). The share is not measured (question 1).
- Earlier test (A): gv1box (one box per load, GPU clip for the rest) gb 10.64 to 10.77 ms, draws 70 to 75. gv1raw (no CPU test) 12.46 ms, draws 70 to 131. In Azahar the per-vertex product costs about the same as the box. Azahar runs VFP code through a host JIT, so the console ratio can differ.

### 7.2 Design: exact hybrid box per load (no extra batches)
1. Per load: the s16 box of the n vertices (6 integer min/max per vertex). Test it with `combined` against the 5 planes of clip_bits4 and the near limit (`dlc_box_out`, pm_dlc.inc.c:227, takes lo and hi arrays).
2. In: vert_cb = 0, vert_gvf = 0 for the n slots. Exact.
3. Out of plane set P: vert_cb = P, vert_gvf = 0. P is a subset of the exact bits. A triangle whose 3 slots share a P bit rejects as today. A triangle that does not reject on the subset bits calls gv_xf on each slot with `vert_gvf & 1` clear and tests again. So the reject set and the near refusals equal today in all cases.
4. Crosses a plane or the margin: the per-vertex product as now.
5. Draws per step are equal to today by construction. Counters: loads and vertices per class (in, out, cross), subset recomputes.

## 8. Area 2: per-triangle checks and index append
- Now: gv_tri (pm_gbi.c:1391-1460): AND of 3 clip bytes, near, G_MODIFYVTX screen vertex, texture scale mismatch, lit-state mismatch, epoch test, gv_params_calc, then `pm_gpu_gappend` (pm_gpu.c:1566) or `pm_gpu_gtrip` (1615). About 1 to 3 us per triangle on the console (E). Non-static triangles in town: about 90 to 250 per step, 0.1 to 0.75 ms (E).
- Cannot move to the GPU: the index list (a G_TRI2 word holds the op byte and slot x 2 values in reversed byte order, so it is not a PICA index buffer; the PICA has no compute stage; a geometry shader cannot fetch indices). The batch-open decision (only the CPU knows if a triangle survives before the state is applied).
- Can move: the near refusal (GVR_NEAR) with the PICA user clip plane w - 0.05 >= 0 on the output position (w row of gv_post is (0,0,0,1), pm_gpu.c:1590-1599; registers GPUREG_FRAGOP_CLIP and GPUREG_FRAGOP_CLIP_DATA0 to 3, 0x047 to 0x04B, check the names in libctru registers.h). The same plane removes the near fallback of DLC groups.
- Stays on the CPU, rare: G_MODIFYVTX screen vertices (underwater), a texture scale mismatch inside one triangle, a lit-state mismatch.
- Per list instead of per triangle: stage C4 (DLC4).

Design against the extra-batch problem: (1) lazy state until a triangle survives (holds in gv_tri now; not for the texture: G_SETTILESIZE loads at once, pm_gbi.c:3018-3023; C1 and GP3 make it lazy); (2) the cheap per-load test of 7.2; (3) a batch that stays open across state-equal chunks (holds now; the remaining false break is a full palette: 21 entries pm4, 8 entries pm5, counter gv_palflush; GP5).

## 9. Area 3: texture decode and upload
- Hit path: the lookups of section 1.1 (G_SETTILESIZE). C1 removes them for static lists.
- Miss path (pm_tex.c:720-894): gpu_fmt (135: RGBA32 to RGBA8, IA and I to LA8, others and CI to RGBA5551, 727), expand, Morton tile and vertical flip (TX1 803-812, old path 813-867), FL1 slab and one merged flush per split (338). tx 0.2 ms per step in town, 0 evictions in 270 s (M). 5 of 5447 spike steps have tex as the main phase (M, build 1073).

| Part | GPU option | Verdict |
|---|---|---|
| Morton tiling and vertical flip of 16 and 32 bit output | GX DisplayTransfer with the tiled output flag and the vertical flip, input RGBA8 or RGB5A1 linear, queued before the frame command list | Possible. The CPU still writes the linear expand (byte swap, palette lookup). Saves the Morton address math only. Size rules and queue order: question 4 |
| I8, I4, IA8, IA4 | PICA L8, L4, LA4, alpha from R in the TEV | Possible. Combiner changes in pm_comb.c and flush_draw. Nibble order to check. Near 0 ms on a steady frame |
| CI4 and CI8 palette lookup | CPU expand once per cache entry (now). A GPU pass needs one pass per palette entry with an alpha test EQUAL (no dependent texture read in the TEV): 16 draws for CI4 (1.3 ms), 256 for CI8. Pre-baked textures: PG2L, no gain (SD reads) | Not possible on the GPU at a gain. CI stays on the CPU, once per cache entry |
| ETC1 | Needs an encoder. Runtime is slower than the expand. Offline is the PG2L case | Not done |
| Upload to VRAM | GX TextureCopy (O13) | No CPU gain. User decision 2026-10-23: the texture cache is part of the GPU path. Shrink it (the 4.75 MB slab is larger than the N64's 4 MB RAM) and place the whole allocation in the 6 MB VRAM, beside the framebuffers and off-screen targets. Do that after the world is on screen again. |

## 10. GPU stages (after C1, or earlier when pm_gbi.c is free)

| Stage | Content | Console gain (E) | Risk | Azahar checks (beyond standard) | Console gate | Files |
|---|---|---|---|---|---|---|
| GP1 | Exact hybrid box per non-DLC G_VTX (7.2). Delete the per-vertex product for in and out loads. Counters per class | 0.2 to 1.0 | low (exact by construction) | draws per step equal to the build before in toad, yard, battle; gbiprof G_VTX net us lower; gv_fb counts equal; gv1check stale 0 | gb town lower or equal | pm_gbi.c (G_VTX 2381-2401, gv_tri 1394), pm_dlc.inc.c (227) |
| GP2 | PICA user clip plane at w = 0.05 for all raw batches. Delete GVR_NEAR, the near bit 64, the near box test, the DLC near interpreter run | 0.1 to 0.6 | medium (picture near the camera, Azahar emulation) | golden yard (V02 cliff), kmr_20 wall, V19 dive, pause1; gv_fb NEAR 0; dlc i_near 0 | photo close to a wall against Azahar | pm_gpu.c, pm_gbi.c, pm_dlc.inc.c |
| GP3 | Lazy texture bind for the interpreted lists (G_SETTILESIZE stores td[0], ensure_tile loads at the first surviving triangle or rectangle). C1 does the same for compiled lists. Only if counters show 20 percent or more of tile 0 loads without a surviving triangle | 0 to 2 | medium (rectangles, origin, grabs) | golden pause1 to 4, V12c, V20, PT82 | gb town lower | pm_gbi.c |
| GP5 | Palette of 28 entries in pm4 (per-batch projection, 3-row affine model view). Only if gv_palflush is over 5 per step | 0 to 0.4 | medium | draws lower; golden toad, battle, isk_01 | draws lower | pm4.v.pica, pm5.v.pica, pm_gpu.c |
| TX-A | DisplayTransfer tiling for RGBA8 and RGBA5551 misses. Only after question 5 | spikes only | medium | fl1log hashes equal | map entry step time lower | pm_tex.c, pm_gpu.c |
| TX-B | Native L8, L4, LA4 for I and IA | about 0 (memory) | medium | golden 10 scenes | standard | pm_tex.c, pm_comb.c, pm_gpu.c |
(GP4 of the first version is now stage C4.)

## 11. Blockers (all parts)
- Matrix palette: 21 entries pm4 (pal[84] of 96 float uniforms), 8 in pm5 (pal5[56]). The float uniform file is shared by all programs.
- Vertex shader limits: 512 instructions, 128 operand descriptors per DVLB. pm4 about 30 instructions, pm5 about 100. Part A needs no shader change.
- Loads that mix matrices: handled by the palette (one index per vertex).
- State applied before a draw is per batch. Lazy application keeps the draw order (flush before a change that the open batch reads).
- Batch-break rule of pm_gpu_gtrip: kept. C3 applies it at compile.
- Rule 19: each stage replaces the old path in the same commit. No switch except the test tools.

# Part C: the whole pipeline on the 3DS

## 12. Evaluation of a 3DS-native design
| # | Design | State | Console gain (E) | Risk | Must stay equal | Verdict |
|---|---|---|---|---|---|---|
| 1 | Compile each list once, cached by address plus content hash of mutable parts | Shape lists done (DLC1, DLC2, 8 ms M). The rest: Part A stages C1, C4, C5 | 7 to 16 (C1, C4, C5) | high | golden, statehash, pdlcheck | Core of this plan |
| 2 | Texture and render state resolved at compile time to a PSO id and texture handles | Part A C1, C2 | included in item 1 plus 1 to 3 (C2) | high | same | Core |
| 3 | Instance-style drawing, one draw per material | Palette done. Merge of adjacent equal state done (3.6 percent of draws equal the one before, 16 percent without colour and matrix, A). One draw per material needs a reorder of opaque z-tested draws by PSO id | 1 to 3 | medium (equal-depth ties and decals change pixels; translucent and decal draws keep order) | golden 0.00 on opaque scenes | After C2 (the PSO id is the sort key) |
| 4 | Static geometry in VRAM | Linear FCRAM static area done (DLC2) | 0 on the CPU (GPU 7.7 ms, M) | low | golden | Not now |
| 5 | Vertex work in the PICA vertex shader | Done: transform, palette, uv, colour, z mix, lighting, texgen, fog, TEXEL1, clip, cull. Left: GP1, GP2. The PICA has a geometry shader (one vertex unit in GS mode). The only N64 fan-out is G_TEXRECT (59 per step, A). A GS cannot read indices or reject before a batch opens | GP1 and GP2: 0.3 to 1.6. GS for rectangles 0.2 to 0.5 | GS: medium (program switch, fewer vertex units) | golden pause and HUD scenes | GP1, GP2 yes. GS no |
| 6 | Producer and consumer: game on core 0 builds step N, the port runs step N-1 on another core | Not done. Core 1 holds the audio (42 to 62 percent of core 1, E) and the GSP service. The N64 ran the RSP one task behind the CPU and the decomp double-buffers the display context, so most data is safe for one step of overlap | O3DS with the gbi beside the audio on core 1: 6 to 12 ms free of 31.9 needed: no gain. O3DS with the audio on core 0 (AU2 option A) and the gbi alone on core 1 (80 percent of 33.3 ms = 26.6 ms): step from 55.6 to about 40 ms (about 15 ms). After C1 to C5 the gbi fits in 26.6 ms. N3DS: core 2 free at 804 MHz | high: one drawn step of input latency (user decision), data that the port reads after the game wrote it (sprite and animated vertex buffers, frame buffer grabs, F03 depth queries, buffer_bridge writes), all citro3d calls on the consumer thread, FS1 pacing | statehash equal on route play1, golden equal (same picture one step later), audiohash equal | Largest single lever. Needs a user decision and the bench of question 9 |

Best order:
1. C0 (or GP1 if pm_gbi.c is busy for C0 but free for a small change).
2. Console bench of core 1 time (question 9), one push, no game change. Ask the user about one drawn step of latency at the same time.
3. C1, C2, C3 (static scenery), each with its console gate.
4. Item 6 (producer and consumer), if the bench shows 25 ms or more of core 1 per step with the audio on core 0 and the user accepts the latency. On N3DS the worker is hardware core 2 (section 2.10), not hardware core 1. Gate: statehash equal on route play1, town step time lower by 8 ms or more.
5. C4, C5 (models with patch points, main-list templates).
6. GP1, GP2, GP3 by their counters, where not done in step 1.
7. C6, C7, item 3, GP5, TX-A, TX-B by console data.
Not now: VRAM geometry, the geometry shader, a GPU-only reject, a GPU CI palette pass, runtime ETC1.
Without item 6, the single-core end point is about gb 12 to 20 ms in town (E, section 3 sum).

## 13. Questions only the console can answer
1. Cost of the per-vertex product and clip bits on the ARM11 VFP against the box (G_VTX net us per call, `phase phasegbi`, build before and after GP1).
2. Cost of one extra draw in the build of today (79 us was measured before GV5 and OPT-SC).
3. Does the PICA user clip plane give the same picture as Azahar near the camera.
4. GX DisplayTransfer limits for texture tiling (minimum size, formats) and whether a transfer before the command list adds a wait.
5. The fixed cost per texture miss after FL1 (was 1.21 ms before FL1).
6. The Azahar to console ratio per op type (G_VTX, G_TRI, G_SETTILESIZE, G_SETCOMBINE): 4.5 is the gb average. This sets the real gain of C1.
7. The GPU time per frame (phc gpu field) after GP2 and C4: it must stay under 15 ms.
8. Battle gb split: the share of animator triangles (decides the order of C4 for battle).
9. The core 1 time that is free for the app with the audio running, and the wall time of a fixed CPU job on core 1 against core 0 (decides item 6).
10. The cost of a 300 B EXIT memcpy and a state-in hash on the console (the replay floor of C1).
