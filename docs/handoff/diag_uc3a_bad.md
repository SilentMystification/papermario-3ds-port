# diag_uc3a_bad: 184 bad words in mac01t_rec_chk f307 (row UC3A-BAD, branch uc2c)

Read-only diagnosis. No source edit. No Azahar start (the existing UC2D log and the code decode the word).
Tags: CONFIRMED (read in code, disassembly or log), INFERRED (reasoned, not measured).

## 1. Result

- The first bad word (j 103) is the texture unit 0 address word, GPU register 0x085 (GPUREG_TEXUNIT0_ADDR1). Not register 0x081. CONFIRMED.
- The record holds the physical address of the linear stage copy of the texture (FCRAM). The live frame holds the physical address of the VRAM copy. CONFIRMED.
- The record key cannot tell a staged slot from an uploaded slot. Cause: RecKey has no staging bit, and the upload does not bump the slot generation. CONFIRMED (code). Why mac01t and not yard: INFERRED.
- The rule that makes it equal: refuse the capture (store no record) while a bound texture slot is in its load frame. Or add a staged bit per slot to the key. Not tested.
- Explained words: 1 of 184 (j 103) CONFIRMED. The other 183 are in the set in section 4. Their split is not measured.

## 2. Decode of the word j 103

Header 0x804F0081 at word 99. Param0 at word 98 (the log "cmd 98"). The UC2D line reports reg from the header (0x081), not from word j.

From the UC2D line: doff 3, so j = 98 + 2 + 3 = 103. Incremental group, base 0x081, word order param0 then header then params:

| Word | Register | Value written (libcitro3d C3Di_SetTex, unit 0) |
|---|---|---|
| 98 | 0x081 BORDER_COLOR | tex+16 (border) |
| 100 | 0x082 DIM | tex+8 (width and height) |
| 101 | 0x083 PARAM | tex+12 (filter and wrap) |
| 102 | 0x084 LOD | tex+20 (LOD parameter) |
| 103 | 0x085 ADDR1 | osConvertVirtToPhys(tex->data) >> 3 |

Sources:
- Register names: /opt/devkitpro/libctru/include/3ds/gpu/registers.h:146 to 150 (docker devkitpro/devkitarm). CONFIRMED.
- C3Di_SetTex disassembly, /opt/devkitpro/libctru/lib/libcitro3d.a (dated 2023-10-28), member base.o region: GPUCMD_Add(0x800f0081, sp+8, 5) with sp+8 = tex+16, sp+12 = tex+8, sp+16 = tex+12, sp+20 = tex+20, sp+24 = phys(data) >> 3. Header 0x800f0081 plus (5 - 1) << 20 gives 0x804F0081. CONFIRMED.
- Walk and reccheck mask: 3ds/src/pm_gpu.c:2389 to 2420 (pm_gpu_rec_same; the address mask loop is 2406 to 2416; k = 4 gives w = i + 5 = 103, r = 0x85). CONFIRMED.

Values:

| Side | Word | Physical address (word << 3) | Memory |
|---|---|---|---|
| Record a | 0x040E8660 | 0x20743300 | FCRAM (linear memory, linearAlloc) |
| Live b | 0x03025780 | 0x1812BC00 | VRAM (texture cache slab) |

The record is the first argument of pm_gpu_rec_same (called from rec_check, pm_gbi.c:3495). The live frame is the second. CONFIRMED.

## 3. Writer and mechanism

Writer in the frame and in the record (same code path):
- pm_tex_bind (3ds/src/pm_tex.c:1465 to 1490) calls C3D_TexBind(0, tp), with tp = tex_for_draw(&slots[cur]).
- tex_for_draw (pm_tex.c:627 to 634): when slot_draws_stage(s) is true, tex.data = vram_stage (linear). Otherwise tex.data = tex_vram_ptr (VRAM). CONFIRMED.
- slot_draws_stage (pm_tex.c:537 to 539): vram_up && vram_stage && tex.size. CONFIRMED.
- citro3d C3D_TexBind (flag at ctx+0x20 bit 0 and the unit pointer) then emits the 5-word block at the next flush (C3Di_UpdateContext). CONFIRMED (disassembly).

Lifetime of the stage state:
- Load: pm_tex.c:1236 (vram_stage = linearAlloc), :1249 (vram_up = 1). The slot is in the stage state from its load frame.
- Frame start: upload_vram_slot (pm_tex.c:541 to 552), called from pm_tex_upload_vram_before_frame (pm_tex.c:566 to 569). It clears vram_up and sets tex.data = tex_vram_ptr. CONFIRMED.
- upload_vram_slot does not change sgen. The only sgen bumps are at drop and invalidate (pm_tex.c:614, 895, 912, 952). CONFIRMED.

Key that does not see the stage state:
- RecKey (pm_gbi.c:3398): ver, gen, hgen, tgen0, tgen1, off, len, state, nb, s0, s1, grp[7]. No staging bit. CONFIRMED.
- tgen0 and tgen1 = pm_tex_slot_gen = sgen (pm_gbi.c:3416, pm_tex.c:620 to 623). CONFIRMED.
- grp (pm_gbi.c:3397) holds the fragment-op group and the texture unit config, not the address. CONFIRMED.

So: a record captured in a slot's load frame stores the linear address. Later frames of the same slot generation match the key. Their live address is VRAM. The compare counts the 0x085 word as bad. CONFIRMED (sequence). Frame timing of the captures on mac01t: INFERRED (no load events in the log, fl1log is off).

Why yard_rec does not show it: INFERRED. yard_rec binds no slot in its capture frame, or binds only slots loaded before the capture window. No per-load log line exists to check this.

Why UC2 passed: UC2 masked 0x085, 0x095 and 0x09D (pm_gpu.c:2413). The mismatch was invisible. CONFIRMED.

Hazard for UC3 replay (not in the compare): the linear copy is freed two frames after the load (pm_tex.c:47 comment, pm_tex.c:600 and 830 frees). A replay of such a record would sample freed FCRAM. INFERRED from the free timing.

## 4. Which bad words are explained

UC3a compared set = every word of the record except: pal slots (idx in [u4_pal, u4_pal + 4 * GPAL)), and the 0x227 param0. UC2 masked everything in UC2 that UC3a compares, plus 0x085, 0x095, 0x09D. UC2 gave bad 0 on mac01t f307. CONFIRMED (agent_uc3.md section 2, pm_gpu.c:2389 to 2417).

So each of the 184 bad words is one of:
- (A) a non-pal uniform data word (uvso, cmul, cadd, zmix, uvx), or
- (B) a texture address word: 0x085 (unit 0), 0x095 (unit 1), 0x09D (unit 2). CONFIRMED (set arithmetic).

- j 103 is class B, unit 0. CONFIRMED.
- The other 183: class B and class A are both possible. The split is not measured. INFERRED that most are class B, because the mechanism in section 3 gives one bad word per staged bind. Not proven.

## 5. Fix candidates (not applied, not tested)

1. Capture refusal (smallest): in rec_done (pm_gbi.c:3500 to 3530, store at about 3519 to 3527), store no record when cur or aux slot has vram_up. pm_tex_cur_vram_pending (pm_tex.c, exists) covers cur. aux needs an export like it. Then the next frame captures the VRAM address. Mask stays narrowed.
2. Key bit: add a staged bit for s0 and s1 to RecKey. Larger change (the key layout changes). Not preferred.

Gate for either fix (one shot each): mac01t_rec_chk f307 with the UC3a mask, bad 0, hit above 0. Yard_rec bad 0. Byte-equal top shots.

## 6. Open items

- Measure the split of the 184 words by class (A, 0x085, 0x095, 0x09D). One log quantity. One Azahar start after the log line is added.
- Confirm yard_rec has no staged bind at capture. One load-event log line, or the fix gate itself.
- The UC3 plan (plan_uc3.md section 2b) says the patch table covers pal only. The address words need a per-record rule too (plan_uc3.md section 1d says the same). Confirmed here as necessary.

## 7. Acronyms

UC2 = reccheck row. UC3 = replay row. UC3a = pal-only mask row. VRAM = video RAM (GPU memory). FCRAM = main RAM of the 3DS (linear memory). PICA = GPU command set. ctx = citro3d context. sgen = slot generation. tgen = texture generation in the key. tex = C3D_Tex (citro3d texture object). LOD = level of detail. DIM = width and height word. GPUCMD = GPU command list. bad = compared words that differ. hit = compares. fl1log = texture stage log flag.
