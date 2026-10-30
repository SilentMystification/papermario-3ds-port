# Diagnosis: ROM drop list (ROMDROP)

No source edit. No Azahar run. Tags: [M] measured, [C] code, [E] estimate.

## 1. Packer and drop rule
- Packer: 3ds/tools/gen_rom_remap.py writes 3ds/src/rom_remap.h. pm_os.c reads pm_rom_keep and pm_rom_drop. [C]
- Rule 1, isdrop(): CODE_OVERLAYS below 0x1CC310; BLOCK 0x3169F0..0x800880 except effect_gfx_* and world_model_anim_kzn_*; dead_*, copy_*, unk_1C84D30; every segment in 0x800880..0xE20110. [C]
- Rule 2, RR1: every splat segment of type `code` loses all bytes except the ranges in 3ds/rom_code_keep.txt. [C]
- Result: 27 drop intervals, 16,088,156 B (15.34 MiB). Packed image 25,854,884 B (24.66 MiB). [M]
- Conflict: RR1 undoes the S19b exemption. effect_gfx_* and world_model_anim_kzn_* are type `code`, so RR1 drops them too. [M]
- pm_os.c:330-357 zero-fills every read that touches a drop interval (unless switch romfull). It logs "ROM READ IN DROPPED RANGE" once per address. [C]

## 2. The 27 drop intervals (merged by hand)
| Start..End | Bytes | Content |
|---|---|---|
| 00000040..00014041 | 81,921 | boot, main code |
| 00014261..0002F5C1 (6 pieces) | 108,736 | main code gaps |
| 00030A61..0010F1B0 | 911,183 | main, engine1/2, evt, entity, font code |
| 001142B0..00164B8 (3 pieces) | 7,434 | charset, heaps3 gaps |
| 0011A440..001CC310 | 728,784 | charset, pause, filemenu, battle_code |
| 003169F0..00328EA0 | 74,928 | audio, world partner code, effect_landing_dust code |
| 0032C110..0035A5E0 | 189,648 | effect code and effect_gfx_* (walking_dust..) |
| 0035B9D0..00E3B870 | 11,402,912 | 829 segments: effect code and gfx, map and battle overlays, entity models |
| 00E3C320..00E3DDB0 | 6,800 | entity_model_RedBlock, PushBlock |
| 00E3E260..00E4B2E0 | 53,376 | entity_model_ShatteringBrickBlock.. |
| 00E4E7F0..00E56BA0 | 33,712 | entity_model_BoardedFloor, StarBoxLauncher |
| 00E56E60..00E58CD0 | 7,792 | entity_model_Tweester |
| 00E59F00..00E5AB50 | 3,152 | entity_model_ArrowSign |
| 00E5CD80..00E639C0 | 27,712 | entity_model_PadlockRedFrame.. RedSwitch |
| 00E657A0..00E9ECD0 | 234,800 | entity_model_ScriptSpring_gfx.. BombableRock |
| 00EA0900..00F00000 | 390,912 | dead_kzn_17.. dead_flo_19 |
| 01942C40, 01B8220C | 4,532 | copy_1942C40, copy_1B8220C |
| 01C84D30..01E40000 | 1,815,248 | unk_1C84D30, copy_1E00000 |
| 027FEE22..02800000 | 4,574 | copy_27FEE22 |
Map overlays (8.84 MB) and battle areas (3.86 MB) are segment sums. They lie mostly in the 11.4 MB row. [M]

## 3. Reads that hit a dropped range
- Logs: 25 files under build3ds (including O3DS hardware logs build3ds/ftp_170*). 205 lines. [M]
- 91 distinct read addresses. 13 are in run_RR1b.txt. Those 13 lie in ranges that RR1 now keeps. 78 remain in dropped ranges. [M]
- Effect graphics read in a dropped range: 38 segments, 252,256 B. Examples: chapter_change 50,576 B, spirit_card 17,968 B, butterflies 15,856 B, lightning 14,896 B, lil_oink 13,296 B, emote 9,680 B, sun 2,560 B. [M]
- Entity models read in a dropped range: 20 segments, 81,152 B. Examples: WoodenCrate, BoardedFloor, SaveBlock, Padlock, Tweester, Hammer1..3Block, HeartBlock, RedSwitch, Signpost. [M]
- Read at 0x3169F0, n 34: the tail of a 256-byte read from sprite_shading_profiles runs into dropped code. Effect on the picture: unknown. [M][E]
- Dropped and not read in any log: effect gfx 72 segments, 389,280 B. Entity models 43 segments, 327,984 B. [M]
- world_model_anim_kzn_* (37 segments, 177,440 B): read by source (src/world/common/lava_piranha/part2.inc.c, src/battle/area/kzn2/area.c). No log hit. The kzn tour did not run. [C][E]
- Effect code (effect_* type code, 324,192 B), map and battle overlays: linked into the 3dsx. Their ROM symbols are empty or placeholder ((u8*)1) in battle_ld_addrs.h and gen_effect_ld.py. No log hit. [C][M]

## 4. Effect load path
- src/effects.c:336-354 load_effect(): dma_copy of the effect code (linked, empty range), then general_heap_malloc(graphicsDmaEnd - graphicsDmaStart) and dma_copy of the gfx. [C]
- assets/us/effects/effect_table.c: 127 FX_ENTRY, 112 distinct gfx. 121 entries have a dropped gfx byte. 6 entries use kept gfx (landing_dust, music_note). [M]
- 46 entries use a gfx that a log read hit. Sun: load_effect(0x82) uses effect_gfx_sun (2,560 B, 0x40BBA0). [M]

## 5. Cost and worth keeping
| Set | Bytes | KiB | Status |
|---|---|---|---|
| Effect gfx read in logs | 252,256 | 246.3 | keep first |
| Entity models read in logs | 81,152 | 79.3 | keep first |
| Read set total | 333,408 | 325.6 | keep first |
| Effect gfx not read | 389,280 | 380.2 | keep if a draw shows gaps [E] |
| Entity models not read | 327,984 | 320.3 | keep if a draw shows gaps [E] |
| world_model_anim_kzn | 177,440 | 173.3 | keep for Lava Piranha [C] |
| copy_*, unk_1C84D30 | 1,815,248 | 1,772.7 | not read, no keep [M] |
| dead_* | 390,912 | 381.8 | not read, no keep [M] |

- Packed image now 25,248 KiB. O3DS cap 26,950 KiB, margin 1,701 KiB (2 log lines). [M]
- Keep the read set: margin about 1,375 KiB on O3DS. [E]
- Keep all dropped effect gfx, entity models and anim: 1,199 KiB, margin about 502 KiB on O3DS. [E]
- MEM free 11.3 MB and arena 62 KB come from the brief. Not measured here. [given]
- Azahar cap 47,742 KiB. Azahar keeps all data. The O3DS cap is the limit. [M]

## 6. Next steps (not done here)
1. Add the 38 effect gfx and 20 entity segments to rom_code_keep.txt, or change RR1 to skip effect_gfx_* and world_model_anim_kzn_*. One change, then Azahar check with one quantity: the sun block (already done, no change).
2. The earlier sun test kept only 0x40BBA0. It gave 0 changed pixels in sun_sbk. The sun fault is not the drop alone. [M] (diag_sunrays SUN2)
