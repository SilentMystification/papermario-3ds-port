# MA1 list: ARM11 math hot spots (step 1, read only)

Date: 2026-10-18. Row MA1 in 3ds/SLICES.md. This file lists sites only. No code changed. No build. No Azahar run.

## 1. Method and limits

- Grep over 3ds/src (port layer) and src (decomp). Counts are code lines with `/` or `%` (comment lines removed). Counts are not run counts.
- Frequency guesses use the PT01 counters in docs/HISTORY.md (mac_00 steady frame): 2237 vertices loaded by G_VTX, 4071 triangle corners transformed, 1357 triangle commands (291 drawn), 120 draws, 784 flush calls, about 1 to 3 texture uploads per frame, 2 to 3 audio ticks per frame.
- ARM11 facts are general knowledge and UNVERIFIED on the O3DS. Azahar does not model divide cost. The real cost is unknown until the phase timer runs on hardware (slice S28 or PO1 follow-up).
- The phase timer gives the scale. GBI interpret (self) is 11 to 15 ms per frame in kmr_20 and mac_00. Texture convert and upload is about 0.75 ms per upload. Audio tick is 7 to 10 ms per frame. Math sites below sit inside those phases.

## 2. Counts per file (code lines with `/` or `%`, plus math calls)

| file | div or mod lines | sqrt | libm sin/cos | notes |
|---|---|---|---|---|
| 3ds/src/pm_gbi.c | about 100 (all patterns) | 0 | 0 | GBI, transform, clip, colour, texel paths |
| 3ds/src/pm_gpu.c | about 30 | 0 | 0 | colour put and debug paths |
| 3ds/src/pm_os.c | about 26 | 0 | 0 | message queue and ROM read |
| 3ds/src/pm_audio.c | about 14 | 0 | 0 | one 64-bit divide per retrace |
| 3ds/src/pm_tex.c | about 10 | 0 | 0 | tiling per texel in upload |
| 3ds/src/pm_rsp_audio.c | about 10 | 0 | 0 | no divide in per-sample loops |
| 3ds/src/pm_gu.c | about 9 | 1 | 6 (sinf, cosf) | only real libm calls in the tree |
| src/model.c | 75 | 0 | 0 | model load loops, projection, tex coords |
| src/audio (all) | 61 | 0 | 0 | double in 5 places |
| src/effects (all) | about 2090 | 17 | 0 | 221 sin_deg, cos_deg, sin_rad, cos_rad calls |
| src/battle/1A5830.c | 69 | 6 | 0 | game atan2 about 10 calls |
| src/os (libultra ports) | 320 | 5 | 0 | 38 lines use 64-bit (s64) |
| src/npc.c | 7 | 3 | 0 | 20 sin_rad, cos_rad calls |
| src/collision.c | 9 | 3 | 0 | |
| src/sprite_shading.c | 6 | 4 | 0 | |
| src/cam_math.c | 7 | 2 | 0 | 8 sin, cos table calls |
| src/entity.c | 5 | 1 | 0 | |
| src/encounter.c | 6 | 4 | 0 | 3 mod |
| src/item_entity.c | 4 | 3 | 0 | |
| src/world/action/walk.c, land.c | 3 | 5 | 0 | |

The decomp has no sinf, cosf, atan2f, fmodf, powf, or tanf calls in src/. The game uses its own functions: `sin_deg`, `cos_deg`, `sin_rad`, `cos_rad` (src/43F0.c, table `sintable` in src/os/sintable.inc.c, 1024 entries of s16), `atan2` (src/43F0.c:612, f32, own code with one divide and table `sAtanFactors`), and `round(f32)` (src/43F0.c:567, own). Name matches for `atan2(` and `round(` in the grep are the game functions, not libm.

## 3. Hot spot table

Columns: ID, file and function, line, operation, call frequency guess, replacement idea, change type. Change type: `render` (GPU output only), `audio` (sound output only), `load` (load time only), `logic` (game state, needs the replay check and user approval).

### 3a. Port layer, per frame paths

| ID | file, function | line | operation | frequency guess | replacement idea | change type |
|---|---|---|---|---|---|---|
| P1 | pm_gbi.c, vert_color | 422-424 | 3 float divides by 127.f (signed normal byte to float) | per G_VTX vertex that is lit (up to 2237 per frame, mac_00) | multiply by constant 1/127.f | render (lighting, not bit exact) |
| P2 | pm_gbi.c, draw_tri | 951 | 2 float divides `x / w` and `y / w` (same w) per triangle corner | per triangle corner (about 4071 per frame, mac_00) | one reciprocal of w per corner, then 2 multiplies | render (screen coordinates) |
| P3 | pm_gbi.c, draw_tri | 987 | float divide by 255.f per lit channel | per vertex with env or lighting | multiply by 1/255.f | render |
| P4 | pm_gbi.c, clip_near | 510 | modulo `(i + n - 1) % n` per clip edge | per triangle that crosses the near plane (count not measured) | `i == 0 ? n - 1 : i - 1`, no divide | render |
| P5 | pm_gbi.c, clip_near | 517 | one float divide per crossing edge | per near-plane crossing | keep, or reuse one reciprocal | render |
| P6 | pm_gbi.c, unpack8 | 1087-1090 | 4 float divides by 255.f | per call: 245-246, 719, 931, 968, 983, 1109-1110, 1207 (about 120 draws per frame) | multiply by 1/255.f, or a 256-entry float table | render (colour) |
| P7 | pm_gpu.c, put4 | 400-403 | 4 float divides by 255.f | per call (caller frequency not checked) | multiply by 1/255.f | render |
| P8 | pm_gbi.c, tile_texel | 1253-1254 | `s %= w`, `t %= h` with runtime w, h (calls __aeabi_idivmod) | per texel in box-pattern and combiner paths (rare path, count not measured) | mask `& (w - 1)` when w is a power of two | render |
| P9 | pm_gbi.c, tile_texel | 1277-1279 | `(x * 255) / 31` integer divides by constant 31 | per texel in CPU tile conversion (miss path) | 32-entry lookup table | render |
| P10 | pm_gbi.c, box_pattern_rect | 1320-1322 | `a * b / 255u` per channel per pixel | per pixel of box-pattern rects (rare path) | `(a * b + 128) * 257 >> 16` (approximate) | render |
| P11 | pm_gbi.c, pm_gbi_run (G_TEXRECT) | 1626-1639 | 4 divides by tw, th (runtime) per texrect | per G_TEXRECT (HUD and 2D sprites, count not measured) | compute 1/tw and 1/th once per rect, then multiply | render |
| P12 | pm_gbi.c, pm_gbi_run | 1594 | divide by 32767.f | per fill rect with z | multiply by constant | render |
| P13 | pm_tex.c, tiled16 (called from pm_tex_invalidate) | 130, 133 | `y % (2 * mh)`, `y % mh`, `x % (2 * mw)`, `x % mw` (runtime sizes) | per texel of every texture upload on the miss path (up to 464 KB per frame in bursts, about 0.75 ms per upload) | `& (size - 1)` when size is a power of two; mirror branch unchanged | render (texel layout must match exactly) |
| P14 | pm_tex.c, expand_dual (floor4) | 203, 210, 214 | signed `v / 4` per texel | per texel of dual-texture expansion (rare path) | `v >> 2` (arithmetic shift is floor) | render (exact) |
| P15 | pm_tex.c, pm_tex_invalidate | 334-335 | `d / row` and `d % row` (runtime row, __aeabi_uidivmod) | per invalidated block (count not measured) | shift if row is a power of two, else keep | render |
| P16 | pm_gu.c, guRotateRPYF | 86-96 | 6 calls to sinf or cosf (libm, soft float on ARM11) | per call. Callers of guRotateRPYF: src/npc.c and src/animator.c. The decomp also has src/os/rotateRPY.c. Check which version links | sin_deg and cos_deg table, or a sin and cos per NPC once | logic if the matrix sets NPC or model position: replay check. Render otherwise |
| P17 | pm_gu.c, guNormalize | 125 | `1.f / sqrtf(len)` | per call (callers not checked) | one reciprocal sqrt approximation (no NEON on ARM11, so integer bit trick plus Newton step) | render (lighting) |

### 3b. Port layer, OS and audio

| ID | file, function | line | operation | frequency guess | replacement idea | change type |
|---|---|---|---|---|---|---|
| P18 | pm_os.c, osSendMesg, osRecvMesg | 72, 85 | `% mq->msgCount` (runtime, __aeabi_uidivmod) | per message send or receive (tens per frame, not measured) | `if (++i == count) i = 0` (same result, no divide) | none (same result) |
| P19 | pm_os.c, nuPiReadRom | 350 | `(w - ASSET_TABLE_START) % ASSET_ENTRY_SIZE` per 4-byte word inside the asset table range | per word of ROM reads that overlap the table (up to 15643 reads of 66 bytes in one map load) | mask if ASSET_ENTRY_SIZE is a power of two (value not checked yet) | load (output identical) |
| P20 | pm_audio.c, pm_audio_retraces | 146-148 | `(now - au_last) / per` on u64 (calls __aeabi_uldivmod) | once per audio retrace (1 to 4 per frame) | fixed-point reciprocal, or a retrace counter; result must equal the divide | audio (tick count must match, else audio drifts) |
| P21 | pm_rsp_audio.c, aResampleImpl | 514 | `pitch_accumulator %= 0x10000` (uint32) | per resample call | none (compiler emits mask). Per-sample loops have no divide | none |

### 3c. Decomp, render and load paths

| ID | file, function | line | operation | frequency guess | replacement idea | change type |
|---|---|---|---|---|---|---|
| D1 | src/model.c, raster size and LOD loops | 2115-2142, 2244-2271, 3242-3280 | runtime integer divides `mainW / d`, `/ lodDivisor` (__aeabi_idiv) | per model load (rare) | shift, the divisor is always 2 to the power k | load (exact) |
| D2 | src/model.c, projection macro (render_models) | 2795-2800 | `1.0f / outW` per model vertex | per model vertex per frame (large count) | no exact cheap form on ARM11. Keep the divide | render. Check first: distance at line 2973 (`outZ / outW * 10000`) may feed logic |
| D3 | src/model.c, mdl_project_tex_coords | 4243-4286 | repeated divides by the same `f14`, `f10a` | per model with projected tex coords, per frame | one reciprocal of the shared divisor | render (texture coordinates, not bit exact) |
| D4 | src/audio/core/pull_voice.c, reverb.c | pull_voice 63, reverb 338 | `/ UNITY_PITCH` (0x8000, power of two) | per voice per tick | none needed. Compiler emits a shift | none |
| D5 | src/audio/core/pull_voice.c | 363 | `inv = (1.0 / count)`, double precision divide | per pull call (caller rate not checked) | float reciprocal or table of counts (rounding changes) | audio |
| D6 | src/audio/core/reverb.c | 130-132, 241, 434 | f64 math (double) | per reverb setup and parameter change (rare) | keep. Rare path | audio |
| D7 | src/audio/core/engine.c, bgm_player.c | engine 545, 599-604, 576; bgm 303-304, 829-855, 1602-1653, 2075 | runtime divides for fade and tempo steps | per fade start, per tempo or volume change (rare) | none needed | audio |

### 3d. Decomp, game logic (replay check and user approval needed for any change)

| ID | file, function | line | operation | frequency guess | replacement idea | change type |
|---|---|---|---|---|---|---|
| D8 | src/npc.c, NPC movement | 438, 453-454, 510, 524 | `sqrtf` (__builtin_sqrtf, VFP instruction, not libm), `sin_rad`, `cos_rad` (table) | per NPC per frame (20 table calls in the file) | none needed for trig. Squared-distance compare only if the sqrt is used for a threshold test | logic |
| D9 | src/43F0.c, atan2 (game function) | 612-650 | 2 fabs, 1 float divide, `round`, table `sAtanFactors`, one multiply | per call. About 10 call sites in src/battle/1A5830.c, 10 or more in src/battle/dmg_player.c, 7B440.c (stick angle), 7BB60.c, 77480.c (ground normal, per frame), 38F00.c | none needed (one divide per call). Any change needs replay check | logic |
| D10 | src/collision.c, walk.c, land.c, sprite_shading.c, item_entity.c, entity.c, encounter.c | various | sqrtf (about 20 sites) and 40 divides | per collision test or per entity per frame | squared-distance compare where exact. Else keep | logic |
| D11 | src/cam_math.c | 7 divides, 2 sqrt, 8 sin_deg or cos_deg | camera math per frame | keep. Table already in use | logic (camera position) |
| D12 | src/battle/1A5830.c | 63 divides, 6 sqrt, 6 mod, 10 atan2 | battle actor movement per actor per frame | keep unless the profile shows it | logic |
| D13 | src/effects/effect_3D.c | 29, 51 | `-1.0f / sqrtf(x)`, `1.0f / sqrtf(x)` (vector normalize) | per 3D effect per frame | keep. Reciprocal sqrt approximation if measured hot | render. Check if effect position feeds logic |
| D14 | src/effects/star.c | 76, 93, 130, 85-86 | sqrtf and sin_deg, cos_deg per star | per star per frame | keep | render. Check callers |
| D15 | src/effects (all files) | many | about 2090 lines with `/` or `%`, about 221 table trig calls, 17 sqrtf | per particle per frame | no single fix. Profile first | mostly render |
| D16 | src/os (libultra ports) | 320 lines | 38 lines use s64 (long long) | caller frequency not checked. If called per frame, each 64-bit divide calls __aeabi_ldivmod | check the callers first | depends on caller |

## 4. Sites checked and found cheap (no action)

- pm_gbi.c L197, L1167-1171, L1292-1293, L1445-1450, L1630-1631, L1628-1629 (`/ 65536.f`, `/ 32.f`, `/ 1024.f`, `/ 4.f`): power-of-two divides. The compiler multiplies. No cost.
- pm_gbi.c L840-842 (`(int)(w & 0xff) / 2`): exact shift. The compiler emits a shift.
- pm_tex.c L438 (`/ 2u`), pm_audio.c L58 (`bytes / 2`), L81 (`bytes / 4`), pm_os.c L455 (`/ 128u`): constant power-of-two divides.
- pm_rsp_audio.c L368 (`/ sizeof(int16_t)`): constant.
- pm_gbi.c L1342, L1457 (`nbytes / 8`, `w1 / 64`): once per display list or per command. Rare.
- pm_gbi.c L1872-1873 (`1.f / 160.f`, `1.f / 120.f`): constant, fallback path only.
- pm_gpu.c L364 and pm_gbi.c L66, L878, L926, L946-951 debug and log paths (switch controlled). Not hot in a normal run.

## 5. Other findings

- No libm call exists in src/ for sinf, cosf, atan2f, fmodf, powf, tanf. The only libm calls in the tree are 6 sinf and cosf in 3ds/src/pm_gu.c (P16).
- No int64 division exists in the per-sample audio loops. One u64 divide per audio retrace (P20).
- Double precision (double or f64): src/audio/core/pull_voice.c L363 (D5), reverb.c L130-132, L241, L434 (D6). The port layer has no double in its hot paths.
- Float to int casts in loops: pm_gbi.c L840-842 (per triangle, exact shifts), pm_tex.c L334-335 (per invalidate). Other casts are in logs or debug paths.
- The audio resampler per-sample loop in pm_rsp_audio.c has no divide. Audio cost is in the audio tick (pm_audio_tick, 7 to 10 ms per frame, PT01 table), not in math.

## 6. Open checks (before any change)

1. Confirm which guRotateRPY is linked (3ds/src/pm_gu.c or src/os/rotateRPY.c) and how often it runs (P16).
2. Count calls per frame for P1, P2, P6, P13 with a counter in a debug build. Divide cost and call count decide the order.
3. Check ASSET_ENTRY_SIZE and whether mq->msgCount and tile sizes are powers of two (P13, P18, P19).
4. Check whether D2 distance (model.c L2973) and D13 effect position feed game logic.
5. Read the callers of D16 (src/os 64-bit functions) for per-frame use.
6. Measure divide and sqrt cost on the O3DS with the phase timer on hardware before any rewrite.

## 7. Summary of counts

- Hot spot rows: 37 (P1 to P21 and D1 to D16). Port layer rows 21. Decomp rows 16.
- Rows that change render output only: P1 to P17, D1 (load), D2, D3, D13, D14, D15.
- Rows that change audio output only: P20, D5, D6, D7.
- Rows with no output change: P18, P21, D4.
- Rows that can change game logic: D8, D9, D10, D11, D12, D16 (depends on caller), and P16 if it sets NPC or model position.
