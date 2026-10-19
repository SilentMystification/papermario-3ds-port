# Display lists and vertex arrays read past their end

Scan V22 (static, all of src/ with the includes of assets/us expanded). Scan script: not in the repo.
Checks: (1) a Gfx array whose last command is not gsSPEndDisplayList. (2) a u8 D_09 raw list. (3) gsSPVertex that loads more vertices than its Vtx array holds, and a triangle that indexes past the array.
Result: no hit outside src/effects/gfx except the two items below.

## Confirmed and added to EFFECT_GFX_SRC (3ds/CMakeLists.txt)
Triangles index past the Vtx array into the next array: flashing_box_shockwave, huff_puff_breath, ice_pillar, merlin_house_stars, purple_ring.
Already in the list before V22: fire_breath, fright_jar, lil_oink, peach_star_beam, snowman_doll, effect_75 and the raw u8 files.

## Not added
- explosion.c: D_09000978, D_090009D8, D_09000A38 load 8 vertices from a 4 vertex array. No triangle uses index 4 or higher. No fault expected.
- draw_box.c gBoxCombineModes: indexed table, not a display list.

## Unsure
- src/screen_overlays.c D_8014EA48: the list has no gsSPEndDisplayList (decomp, 105 commands, 0x348 bytes from rom 0xE5148). It ends 0x80 bytes before the imgfx data at rom 0xE5510 (ImgFXCurrentTexturePtr). The bytes between are probably zero (no-op commands). It is the last data of the file, so -fno-data-sections would not change what follows. The port reads whatever the linker places next. Check with a shot of the stencil overlay that uses D_8014EA48 (gSPDisplayList in _render_transition_stencil). Needs the real ROM bytes at rom 0xE5490 to 0xE5510 to judge.
