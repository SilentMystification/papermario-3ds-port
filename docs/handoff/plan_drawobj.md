# plan_drawobj.md: DRAWOBJ, retained draw objects (oracle NATIVE17; user yes 20 Nov: design pass, pages only, the user reads before coders start)
Tags: FACT (console capture or code read), GUESS, OPEN. Numbers: console build 7680 (plan_perf.md section 11). Gates stay byte-exact.

## 0. Base
- MSNAP2 merged (FACT): snn 19572 to 3957 at spot_mac01_drec f201 (Azahar), byte-equal. PROF3 merged: the c1g split comes with the next console capture.
- Now (FACT): core 1 35.2 ms per step, core 0 30.7 ms; core 1 model records rmdl 4.33, entity records rent 1.07 and alt 1.85; core 0 model key, matrix and hit
  5.82 (c0r), submit 5.97 for 113 draws per frame.

## 1. What a draw object is in this game
- Model object: one per map or prop model (PM map geometry is the model tree: static scene geometry is models). Static part: the geometry groups, texture
  handle, render mode, combine, the native draws of the core 0 model cache entry (NdMEnt with NdNatC draws, FACT nd_recs.c, pm_gpu.c:4141). Dynamic part
  per step: the model matrix, prim, env, fog and blend colours, panner tile offsets, lights, the visible group range.
- Entity object: one per compiled entity list (EntKey, nd_ent.c). Dynamic part: the matrix stack, the segment table. Core 0 already draws them from pools.
- Out of scope (immediate stays): sprites (player and NPCs, per-frame animated quads, rspr 0.23 ms FACT), HUD, windows, messages, effects, backgrounds.

## 2. How the game draw code stays compatible
- The game code does not change in rows DO0 to DO4. appendGfx_model still computes the matrix and fills its G_PM_REC record; entities still call their lists.
- The retention sits below the record: core 1 reuses the stream block of an unchanged model (DO1); core 0 finds the object of a record by an id, not a key
  hash (DO2), and cuts the per-draw submit cost (DO3). Entities stop writing copy ranges that core 0 never runs (DO4).
- What it cannot remove: the game draw code itself (render queues, model tree walk, matrix math, sprite animation, effects, HUD). That is part of the
  9.7 ms (GUESS) game draw time on core 1. DRAWOBJ removes the recording cost (rmdl, rent, alt, part of the snap and unit costs) and the core 0 per-draw cost.
  The game draw code is the subject of plan_gamedraw.md (oracle NATIVE18, rows GD0 to GD6). DO5 is dropped (user, 20 Nov).

## 3. Work per step and per frame after DRAWOBJ (GUESS)
- Core 1 per model and step: a before-state snapshot (about 300 B), one memcmp with the object's last one, a copy of the last stream block, matrix word patches,
  the state delta into nd_ds: about 2 us against about 46 us now. Models whose colours, panners or lights change take the full path.
- Core 0 per model and frame: the object by id (no key B, no hash probe), the matrix, then per draw a copy of the pre-built state words, patches of the
  uniforms and texture addresses, the draw: about 2 to 4 us per draw against about 44 us per model hit plus 53 us per submit now.
- Key frames (fps60) replay the same objects with lerped matrices: the same saving applies twice per step (GUESS).

## 4. Pre-built PICA state words
- Today emit_core (nd_emit.c) compares each register with the shadow and writes the changed ones, per draw, every frame (FACT nd_emit.c:40-130).
- DO3: at the first hit after a store, the entry keeps the words emit_core produced for a cleared shadow (the full state of the draw) and a list of the
  dynamic words (matrix and palette uniforms, texture addresses, shroud colour). At a hit: when the previous draw used the same block, only the dynamic
  words go out; else the block is copied. The emitted words change (fewer or more redundant writes), the GPU state at each draw does not.
- So the DO gates compare the GPU state at each draw, not the words: DO0 builds that tool first (a register file and uniform arrays parsed from the command
  buffer, hashed at each draw, addresses left out).

## 5. Cache and memory
- ARM11 L1 is 16 KB I and 16 KB D (hw_guidelines.md). The core 0 hit loop becomes one function (under 2 KB of code) that streams the object entry
  (32 to 64 B), the state words (sequential copy) and 64 B of matrix. Core 1 memo: one 300 B compare and a sequential copy per model.
- Memory (heap, GUESS): DO1 memo 300 B x about 150 models = 45 KB; DO3 state words 160 B x about 1000 cached draws = 160 KB; DO2 id table 4 KB.
  Total about 210 KB of heap. The O3DS margin is 402 KB (coordinator FACT); the blocks go into the model cache pool budget (MDLBLK freed 256 KB) and are
  freed with the pool. A heap line at map load reports the use (DO3 gate note).

## 6. Rows (order; each row has a byte-exact gate; ms GUESS)
| row | model | what | gate | core 1 | core 0 |
|---|---|---|---|---|---|
| DO0 | Haiku | GPU state hash at each draw (`gst f<F> n<draws> h<hash>` per drec frame) and bench.py cmp --gst, seeded self-test | drec byte-equal; selftest-gst PASS | 0 | 0 |
| DO1 | Sonnet | core 1 model record memo: an unchanged model copies its last stream block; census mmemo first | spot_mac01, entcape_kmr03 drec byte-equal, mmemo 0 | -2.5 to -3.5 | 0 |
| DO2 | Sonnet | core 0 object id in the record (core 1 assigns it from the DO1 memo); the key check stays as a guard. Page after DO1 merges | drec and gst equal | 0 | -0.3 to -0.9 |
| DO3 | Sonnet | step 1 split of the submit (C3Di_UpdateContext runs at every draw: cnt upd 113 per frame, FACT); step 2 cuts the named cause | gst equal, drec equal | 0 | -3 to -4 |
| DO4 | Sonnet | entities: no copy range for keys core 0 draws natively; gap units only (ALT2C data). Needs a core 0 to core 1 table of segment op ranges: page after DO1 and capture 1 | drec byte-equal, entw | -1 to -1.5 | -0.3 |
Total rows DO1 to DO4 (GUESS): core 1 -3.5 to -5, core 0 -4 to -7. With MSNAP2 and the section 11 rows this reaches the plan_60fps.md budget in
mac_01 (GUESS). Game draw code rows: plan_gamedraw.md (GD0 to GD6, -2.4 to -7.3 ms core 1 GUESS).
- Recorder side of GD6 (= plan_perf.md D2, owner NATIVE17, agreed): a replayed setter block must give the same stream bytes and the same nd_ds after as the
  unit path (census `gdblk diff 0`, as the DO1 census); blocks hold state setters only (no TRI or RECT unit: those take snaps); the entry state compare
  includes sc_lost, sc_dirty and the shadow sh (the ep rule and sh_drop read them); a block never spans a CALL or a model record.

## 7. Risks
- A memo or id that survives a change (map load, model hidden, texture swap, panner move): each row keeps the full key check as a guard and a census step.
- The two display contexts alternate matrix pointers (record words R1, R2, R8 point into the context): DO1 patches these words, the census proves the rest equal.
- Pre-built words and the citro3d context: C3Di_UpdateContext writes between draws (pm_gpu.c:1307-1311); DO3 must leave the shadow reset rule as now.
- Heap use at map load on the O3DS XL (memory budget): DO3 reports it; a full pool falls back to the emit path (as the model cache does today).
- Cross-core: DO2 and DO4 read data the other core wrote in an earlier step: only through the stream or a table written before the queue hand-off.

## 8. Fallback
- DO0 fails (state hash not stable between equal runs): use word hashes for DO1, DO2, DO4 (their words do not change) and stop at DO3.
- DO1 census shows mismatches it cannot explain: keep full recording; take C1 delta snaps (plan_perf.md section 11) instead.
- DO3 gst differs: keep emit_core, cache only the template ids and the palette (the MDLHIT2 form, -1.5 to -2.5 ms core 0 GUESS).
- DO4 refusal risk (an entity that turns ENT_DYN after it went native): keep writing the copy for keys that changed state in the last 300 frames.
