# Endian scan (PT41)

Class: game code reads a big-endian ROM table through a (u16*), (s16*), (u32*) or (f32*) cast. On the 3DS the read swaps the bytes.

## Fixed
- src/msg_draw.c:2093 to :2095: ui_msg_palettes word 4 (choice box fill). Now read with PAL_MEM. The box is light grey (216,216,208), not orange. Shot: Toad Town inn "Take a nap" box.
- src/battle/16C8E0.c:755 to :764: the tattle background palette read gGameStatusPtr->backgroundPalette (big-endian words) without PAL_MEM and wrote gTattleBgPalette in little-endian order. Now both sides use PAL_MEM. Not tested in Azahar (needs a tattle with fog alpha below 255).

## Checked and correct
- src/background.c:192, :206 (BG_PAL), src/npc.c:1476 to :1485, src/battle/actor_rendering.c (PAL_MEM on every read and write).
- src/101b90_len_8f0.c, src/model.c:2014, src/background.c:42 to :50, src/collision.c: the data is swapped at load.
- src/hud_element.c:287, :1541: only a pointer is passed, no colour math.

## Not checked
- Map data in src/world/area_* and src/battle/area_* (swapped by pm_map.c and pm_mapdata.c).
