# Fall-through sweep (H06a class 1)

Read-only sweep. No build, no Azahar run. Date 2026-10-19. Follows FX3d (commit 881f7d770, snowfall).

## Method
- Script parsed every `Gfx X[] = {...};` in assets/us/**/*.gfx.inc.c (all 730 effect lists and the rest of assets/us), and in src/**/*.c and *.h.
- A hit is a list whose last command is not gsSPEndDisplayList and not gsSPBranchList. A last command gsSPDisplayList would also be a hit. None was found.
- Looked for u8 and other data arrays in src/effects/gfx/*.c that hold non-zero list bytes. Only snowfall had one (fixed in FX3d). The others are zero filled texture buffers (`u8 X[0x1E0] = {}`) and 8 byte zero arrays (D_09001908_3E4268, D_090017A8_4009F8, D_090003E0_3E6310). Nothing references the 8 byte arrays.
- check_data_layout.py --scan on src/effects: 0 neighbour-read patterns. check_data_order.py on build3ds/pm_3ds.map: 0 mismatches.
- Result: 4 unterminated lists, all in src/effects/gfx. The rest of src has none (draw_box.c gBoxCombineModes and model.c pm_tex_unloaded are not display lists).

## Hits that need a fix (2)

### 1. fire_breath: D_09000B88_373F68
- File: assets/us/effects/gfx/D_09000B88_373F68.gfx.inc.c, included by src/effects/gfx/fire_breath.c line 10.
- List: PipeSync, ClearGeometryMode(G_LIGHTING), SetGeometryMode(CULL_BACK|SHADING_SMOOTH), SPVertex(D_09000B38_373F18, 5). Size 0x20.
- ROM next: D_09000BA8_373F88 at 0x09000BA8 (= B88 + 0x20). It holds 2 x gsSP2Triangles and gsSPEndDisplayList. The ROM runs B88 then the triangles by fall-through.
- Used by: src/effects/fire_breath.c, table D_E006EC00 (3 entries, all D_09000B88_373F68), drawn at lines 187 to 222.
- Fix: gsSPBranchList(D_09000BA8_373F88) after the last command.
- Shows in: fire_breath effect. The 5 vertex quad is loaded but never drawn on the 3DS. Not in the FX3f list.

### 2. peach_star_beam: D_09005090_3EAFC0
- File: assets/us/effects/gfx/D_09005090_3EAFC0.gfx.inc.c, included by src/effects/gfx/peach_star_beam.c line 64.
- List: setup (PipeSync, G_TEXTURE, 2 cycle, render mode, combine, ClearGeometryMode). 12 commands, size 0x60.
- ROM next: D_090050F0_3EB020 at 0x090050F0 (= 5090 + 0x60). It holds SetTextureLUT, LoadTLUT_pal16, LoadTextureTile_4b (frame 0 texture) and gsSPEndDisplayList. D_090050F0 is also entry 0 of table D_E010CA38.
- Used by: src/effects/peach_star_beam.c lines 177 and 224 (gSPDisplayList of the setup list).
- Fix: gsSPBranchList(D_090050F0_3EB020) after the last command.
- Shows in: peach_star_beam effect. The setup list runs into whatever follows on the 3DS. Texture and palette state is not the same as on the ROM. Spirit lists set their own texture, so the main effect is the beam quad (D_E010CA10) state.

### How to apply the fix
assets/us is generated (git ignore line 50), so do not edit the .inc.c. Use the wrapper method in the effect gfx .c. Rename the included list with a macro and define the original name as a two command list:

    #define D_09000B88_373F68 D_09000B88_373F68_body
    #include "effects/gfx/D_09000B88_373F68.gfx.inc.c"
    #undef D_09000B88_373F68
    Gfx D_09000B88_373F68[] = {
        gsSPDisplayList(D_09000B88_373F68_body),
        gsSPBranchList(D_09000BA8_373F88),
    };

The next list must come before the wrapper in the file (or have an extern). Apply the same to D_09005090_3EAFC0 with D_090050F0_3EB020. Check the symbol is not in a header with an extern of a different type. Test with the fire_breath and peach_star_beam fxtest slots (1 shot each).

## Hits that need no fix (2)

### 3. fright_jar: D_09008CB0_3CA850
- Included by src/effects/gfx/fright_jar.c line 13. 4 commands (SetTextureImage, SetTile, LoadSync, LoadTile), size 0x20.
- ROM next: D_09008CD0_3CA870 (PipeSync, tile loads, 10 quads, EndDisplayList).
- No effect code references either list. src/effects/fright_jar.c builds the same commands with gDP calls in its loop (lines 160 to 200). The ROM never runs these two lists. No fault.

### 4. lil_oink: D_09000B90_3FFDE0
- Included by src/effects/gfx/lil_oink.c line 30. One command, gsDPNoOp, size 8.
- ROM next: D_09000B98_3FFDE8, a Vtx array (data, not commands). Nothing references D_09000B90_3FFDE0 (star_outline.c uses D_09000B90_414B30, a different effect). No fault. The decomp list is an 8 byte pad.

## Row FX3f (14 effects with nothing visible)
Effects: 7 footprint, 9 snowflake, 11 emote, 13 shape_spell, 14 gather_energy_pink, 30 rising_bubble, 34 smoke_burst, 36 sleep_bubble, 53 aura, 100 energy_in_out, 102 shiny_flare, 116 lil_oink, 122 sun, 126 effect_86.
- None of the 14 effects uses an unterminated list. The only hit that touches one (lil_oink D_09000B90_3FFDE0) is not referenced.
- None of the 14 has a non-zero u8 array in src/effects/gfx like snowfall had.
- So the ROM fall-through class does not explain FX3f. Cause is elsewhere: spawn arguments, life time, or frame choice (the fxtest slot is 44 frames, short life effects such as footprint, emote, smoke_burst and shiny_flare may end before the shot). Keep FX3f for the shot timing check.
- Other fall-through forms not found by this sweep: a Gfx table entry that points into the middle of a list (`&D_X[n]`), and a Vtx read across symbol borders. check_data_layout.py --scan finds 0 in src/effects. Run it again with the nm file of a fresh ELF for a full check (the ELF is not in build3ds now).
